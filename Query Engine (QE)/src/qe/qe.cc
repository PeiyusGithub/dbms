#include "qe.h"

#define mp make_pair

const int BUF_SIZE = 500;


Filter::Filter(Iterator* input, const Condition &condition) {
    this -> input = input;
    this -> condition = condition;
    input -> getAttributes(attrs);
}

void Filter::getAttributes(vector<Attribute> &attrs) const {
    attrs = this -> attrs;
}

Project::Project(Iterator *input, const vector<string> &attrNames) {
    this -> input = input;
    this -> attrNames = attrNames;
    input -> getAttributes(attrs);
}

void Project::getAttributes(vector<Attribute> &attrs) const {
    attrs.clear();
    for(auto attr1 : attrNames)
        for (auto attr2 : this -> attrs) {
            if (attr1 == attr2.name) {
                attrs.pb(attr2);
                break;
            }
        }
}

RC Project::getNextTuple(void *data) {
    while (input -> getNextTuple(data) != QE_EOF) {
        Record record(attrs, data);
        record.decode2(data, attrNames);
        return 0;
    }
    return QE_EOF;
}

void BNLJoin::getAttributes(vector<Attribute> &attrs) const {
    attrs = this -> attrs;
}

BNLJoin::BNLJoin(Iterator *leftIn, TableScan *rightIn, const Condition &condition, const unsigned numPages) {
    this -> leftIn = leftIn;
    this -> rightIn = rightIn;
    this -> condition = condition;
    this -> inloop = 0;
    this -> numPages = numPages;
    this -> tmp = (char*)malloc(2048);
    leftIn -> getAttributes(leftAttr);
    rightIn -> getAttributes(rightAttr);
    attrs = leftAttr;
    for (auto &t : rightAttr)   attrs.pb(t);
    for (auto &t : leftAttr) {
        if (t.name == condition.lhsAttr)    type = t.type;
    }
}

RC BNLJoin::getNextTuple(void *data) {
    if (inloop) {
        while (rightIn -> getNextTuple(data) != QE_EOF) {
            Record record(rightAttr, data);
            vector<string> attrname;
            attrname.pb(condition.rhsAttr);
            record.decode2(tmp, attrname);
            if (type == TypeInt) {
                int intV = *(int *)(tmp + 1);
                if (recordIntMap.count(intV)) {
                    Record now(recordIntMap[intV]);
                    int lSize = (leftAttr.size() + 1) << 2, rSize = (rightAttr.size() + 1) << 2;
                    int nullSize = (leftAttr.size() + rightAttr.size() + 7) >> 3;
                    int leftNullSize = (leftAttr.size() + 7) >> 3, rightNullSize = (rightAttr.size() + 7) >> 3;
                    memset(data, 0, nullSize);
                    now.decode(data);
                    for (int i = 0; i < nullSize; ++i)
                        *(char *) ((char *) data + i) = 255;
                    memcpy((char *) data + nullSize, now.tmp + lSize, now.length - lSize);
                    memcpy((char *) data + now.length - lSize + nullSize, record.tmp + rSize, record.length - rSize);
                    return 0;
                }
            } else if (type == TypeReal) {
                float realV = *(float *)(tmp + 1);
                if (recordRealMap.count(realV)) {
                    Record now(recordRealMap[realV]);
                    int lSize = (leftAttr.size() + 1) << 2, rSize = (rightAttr.size() + 1) << 2;
                    int nullSize = (leftAttr.size() + rightAttr.size() + 7) >> 3;
                    int leftNullSize = (leftAttr.size() + 7) >> 3, rightNullSize = (rightAttr.size() + 7) >> 3;
                    memset(data, 0, nullSize);
                    now.decode(data);
                    for (int i = 0; i < nullSize; ++i)
                        *(char *) ((char *) data + i) = 255;
                    memcpy((char *) data + nullSize, now.tmp + lSize, now.length - lSize);
                    memcpy((char *) data + now.length - lSize + nullSize, record.tmp + rSize, record.length - rSize);
                    return 0;
                }
            } else {
                int len = *(int*)(tmp + 1);
                string varcharV = "";
                for (int i = 0; i < len; ++i) varcharV += *(tmp + 5 + i);
                Record now(recordVarcharMap[varcharV]);
                int lSize = (leftAttr.size() + 1) << 2, rSize = (rightAttr.size() + 1) << 2;
                int nullSize = (leftAttr.size() + rightAttr.size() + 7) >> 3;
                int leftNullSize = (leftAttr.size() + 7) >> 3, rightNullSize = (rightAttr.size() + 7) >> 3;
                memset(data, 0, nullSize);
                now.decode(data);
                for (int i = 0; i < nullSize; ++i)
                    *(char *) ((char *) data + i) = 255;
                memcpy((char *) data + nullSize, now.tmp + lSize, now.length - lSize);
                memcpy((char *) data + now.length - lSize + nullSize, record.tmp + rSize, record.length - rSize);
                return 0;
            }
        }
        inloop = 0;
        return getNextTuple(data);
    }
    else {
        inloop = 1;
        recordIntMap.clear(), recordRealMap.clear(), recordVarcharMap.clear();
        if (leftIn -> getNextTuple(data) == QE_EOF) return QE_EOF;
        int sz = numPages * PAGE_SIZE;
        do {
            Record record(leftAttr, data);
            vector<string> attrname;
            attrname.pb(condition.lhsAttr);
            record.decode2(tmp, attrname);
            sz -= record.length;
            if (type == TypeInt) {
                int intV = *(int*)(tmp + 1);
                recordIntMap.insert(mp(intV, record));
            }
            else if (type == TypeReal) {
                float realV = *(float*)(tmp + 1);
                recordRealMap.insert(mp(realV, record));
            }
            else {
                int len = *(int*)(tmp + 1);
                string varcharV = "";
                for (int i = 0; i < len; ++i)   varcharV += *(tmp + 5 + i);
                recordVarcharMap.insert(mp(varcharV, record));
            }
        }while (sz >= 0 && leftIn -> getNextTuple(data) != QE_EOF);
        rightIn -> setIterator();
        return getNextTuple(data);
    }
    return QE_EOF;
}

void INLJoin::getAttributes(vector<Attribute> &attrs) const {
    attrs = this -> attrs;
}

INLJoin::INLJoin(Iterator *leftIn, IndexScan *rightIn, const Condition &condition) {
    this -> leftIn = leftIn;
    this -> rightIn = rightIn;
    this -> condition = condition;
    this -> cur = (char*)malloc(2048);
    this -> tmp = (char*)malloc(2048);
    this -> inloop = 0;
    leftIn -> getAttributes(leftAttr);
    rightIn -> getAttributes(rightAttr);
    attrs = leftAttr;
    for (auto &t : rightAttr)   attrs.pb(t);
}

RC INLJoin::getNextTuple(void *data) {
    if (inloop) {
        if (rightIn -> getNextTuple(data) != QE_EOF) {
            Record left(leftAttr, cur);
            Record right(rightAttr, data);
            int lSize = (leftAttr.size() + 1) << 2, rSize = (rightAttr.size() + 1) << 2;
            int nullSize = (leftAttr.size() + rightAttr.size() + 7) >> 3;
            int leftNullSize = (leftAttr.size() + 7) >> 3, rightNullSize = (rightAttr.size() + 7) >> 3;
            for (int i = 0; i < nullSize; ++i)
                *(char *) ((char *)data + i) = 255;
            memcpy((char *) data + nullSize, left.tmp + lSize, left.length - lSize);
            memcpy((char *) data + left.length - lSize + nullSize, right.tmp + rSize, right.length - rSize);
            return 0;
        }
        else {
            inloop = 0;
            return getNextTuple(data);
        }
    }
    else {
        inloop = 1;
        if (leftIn -> getNextTuple(cur) != QE_EOF) {
            Record record(leftAttr, cur);
            vector<string> attrname;
            attrname.pb(condition.lhsAttr);
            record.decode2(tmp, attrname);
            rightIn -> setIterator(tmp + 1, tmp + 1, true, true);
            return getNextTuple(data);
        }
        else return QE_EOF;
    }
    return QE_EOF;
}

void Aggregate::getAttributes(vector<Attribute> &attrs) const {
    attrs = this -> attrs;
}

Aggregate::Aggregate(Iterator *input, Attribute aggAttr, AggregateOp op) {
    this -> input = input;
    this -> aggAttr = aggAttr;
    this > op = op;
    this -> hasGroup = 1;
    char *data = (char*)malloc(BUF_SIZE);
    this -> finish = 0;
    float value = 0.0;
    while (input -> getNextTuple(data) != QE_EOF) {
        vector<string> attrNames;
        attrNames.pb(aggAttr.name);
        Record record(attrs, data);
        record.decode2(data, attrNames);
        if (aggAttr.type == TypeInt)    value = (float)(*(int*)(data + 1));
        else value = *(float*)(data + 1);
        if (!vis) {
            mn = mx = value;
            vis = 1;
        }
        else {
            mn = min(mn, value);
            mx = max(mx, value);
        }
        cnt += 1;
        sum += value;
        avg = sum / cnt;
    }
    free(data);
}

Aggregate::Aggregate(Iterator *input, Attribute aggAttr, Attribute groupAttr, AggregateOp op) {
    this -> input = input;
    this -> aggAttr = aggAttr;
    this -> groupAttr = groupAttr;
    this -> op = op;
    char *data = (char*)malloc(BUF_SIZE);
    this -> hasGroup = 1;
    float aggV = 0.0;
    int groupIntV = 0;
    float groupRealV = 0.0;
    while (input -> getNextTuple(data) != QE_EOF) {
        vector<string> attrNames;
        attrNames.pb(aggAttr.name), attrNames.pb(groupAttr.name);
        Record record(attrs, data);
        record.decode2(data, attrNames);
        if (aggAttr.type == TypeInt) aggV = (float) (*(int *) (data + 1));
        else aggV = *(float *) (data + 1);
        int offset = 5;
        if (groupAttr.type == TypeInt) {
            groupIntV = *(int*)(data + offset);
            if (groupIntMap.count(groupIntV) != 0) {
                groupIntMap[groupIntV].cnt += 1;
                groupIntMap[groupIntV].sum += aggV;
                groupIntMap[groupIntV].avg = groupIntMap[groupIntV].sum / groupIntMap[groupIntV].cnt;
                groupIntMap[groupIntV].mn = min(groupIntMap[groupIntV].mn, aggV);
                groupIntMap[groupIntV].mx = max(groupIntMap[groupIntV].mx, aggV);
            }
            else groupIntMap[groupIntV] = AggregateData(aggV, aggV, aggV, aggV, aggV);
        }
        else if (groupAttr.type == TypeReal){
            groupRealV = *(float*)(data + offset);
            if (groupRealMap.count(groupRealV) != 0) {
                groupRealMap[groupRealV].cnt += 1;
                groupRealMap[groupRealV].sum += aggV;
                groupRealMap[groupRealV].avg = groupRealMap[groupRealV].sum / groupRealMap[groupRealV].cnt;
                groupRealMap[groupRealV].mn = min(groupRealMap[groupRealV].mn, aggV);
                groupRealMap[groupRealV].mx = max(groupRealMap[groupRealV].mx, aggV);
            }
            else groupRealMap[groupRealV] = AggregateData(aggV, aggV, aggV, aggV, aggV);
        }
        else {

        }
    }
    free(data);
}

RC Aggregate::getNextTuple(void *data) {
    if (!hasGroup) {
        switch (op) {
            case MIN:
                *(float *) ((char *) data + 1) = mn;
                break;
            case MAX:
                *(float *) ((char *) data + 1) = mx;
                break;
            case COUNT:
                *(float *) ((char *) data + 1) = cnt;
                break;
            case SUM:
                *(float *) ((char *) data + 1) = sum;
                break;
            case AVG:
                *(float *) ((char *) data + 1) = avg;
                break;
        }
        if (!finish) {
            finish = 1;
            return 0;
        }
        else return QE_EOF;
    }
    else {
        int offset = 1;
        switch(groupAttr.type){
            case TypeInt:
                if(groupIntMap.size() == 0) return QE_EOF;
                *(int *)(data + offset) = groupIntMap.begin() -> first;
                offset += sizeof(int);
                switch(op){
                    case MIN:
                        *(float *)(data + offset) = groupIntMap.begin() -> second.mn;
                        break;
                    case MAX:
                        *(float *)(data + offset) = groupIntMap.begin() -> second.mx;
                        break;
                    case COUNT:
                        *(float *)(data + offset) = groupIntMap.begin() -> second.cnt;
                        break;
                    case SUM:
                        *(float *)(data + offset) = groupIntMap.begin() -> second.sum;
                        break;
                    case AVG:
                        *(float *)(data + offset) = groupIntMap.begin() -> second.avg;
                        break;
                }
                groupIntMap.erase(groupIntMap.begin());
                break;
            case TypeReal:
                if(groupRealMap.size() == 0) return QE_EOF;
                *(float *)(data + offset) = groupRealMap.begin() -> first;
                offset += sizeof(int);
                switch(op){
                    case MIN:
                        *(float *)(data + offset) = groupRealMap.begin() -> second.mn;
                        break;
                    case MAX:
                        *(float *)(data + offset) = groupRealMap.begin() -> second.mx;
                        break;
                    case COUNT:
                        *(float *)(data + offset) = groupRealMap.begin() -> second.cnt;
                        break;
                    case SUM:
                        *(float *)(data + offset) = groupRealMap.begin() -> second.sum;
                        break;
                    case AVG:
                        *(float *)(data + offset) = groupRealMap.begin() -> second.avg;
                        break;
                }
                groupRealMap.erase(groupRealMap.begin());
                break;
            case TypeVarChar:
                break;
        }
        return 0;
    }
}