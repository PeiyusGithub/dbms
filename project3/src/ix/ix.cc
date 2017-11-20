#include <unistd.h>
#include "ix.h"
#include "../rbf/rbfm.h"

IndexManager* IndexManager::_index_manager = 0;

IndexManager* IndexManager::instance() {
    if(!_index_manager)
        _index_manager = new IndexManager();

    return _index_manager;
}

IndexManager::IndexManager() {
    tmp = new char[PAGE_SIZE];
}

IndexManager::~IndexManager() {
    delete [] tmp;
}

RC IndexManager::createFile(const string &fileName) {
    const char* s = fileName.c_str();
    FILE* file = fopen(s, "rb");
    if (file) {
        fclose(file);
        return -1;
    }
    file = fopen(s, "wb");
    if (!file)	return -1;
    char* data = new char[PAGE_SIZE];
    memset(data, 0, sizeof(data));
    changeHiddenPage(data, 0, -1);
    fwrite(data, 1, PAGE_SIZE, file);
    delete [] data;
    fclose(file);
    return 0;
}

RC IndexManager::changeHiddenPage(char *s, int val, int type) {
    memcpy(s, &val, sizeof(int));
    memcpy(s + 4, &type, sizeof(int));
    return 0;
}

RC IndexManager::destroyFile(const string &fileName) {
    return PagedFileManager::instance() -> destroyFile(fileName);
}

RC IndexManager::openFile(const string &fileName, IXFileHandle &ixfileHandle) {
    if (ixfileHandle.fileHandle.open)   return -1;
    else ixfileHandle.fileHandle.open = 1;
    const char* s = fileName.c_str();
    FILE* file = fopen(s, "rb+");
    if (!file)  return -1;
    ixfileHandle.fileName = fileName;
    ixfileHandle.fileHandle.file = file;
    return 0;
}

RC IndexManager::closeFile(IXFileHandle &ixfileHandle) {
    ixfileHandle.fileHandle.open = 0;
    fclose(ixfileHandle.fileHandle.file);
    return 0;
}

RC IndexManager::insertEntry(IXFileHandle &ixfileHandle, const Attribute &attribute, const void *key, const RID &rid) {
    int type = attribute.type;
    ixfileHandle.fileHandle.readPage(0, tmp);
    int now = *(int*)tmp, now2;
    if (now == 0) {
        now = 1;
        changeHiddenPage(tmp, now, type);
        ixfileHandle.fileHandle.writePage(0, tmp);
        ++ixfileHandle.ixWritePageCounter;
        //hidden page
        memset(tmp, 0, PAGE_SIZE);
        *(int*)(tmp + 8) = -1;//no next
        ixfileHandle.fileHandle.appendPage(tmp);
        ++ixfileHandle.ixAppendPageCounter;
    }
    int intKey = 0, intKey2 = 0;
    float realKey = 0.0, realKey2 = 0.0;
    int varLen = 0, varLen2 = 0;
    string varKey = "", varKey2 = "";
    if (type == TypeInt)    intKey = *(int*)key;
    else if (type == TypeReal)  realKey = *(float*)key;
    else {
        varLen = *(int*)key;
        for (int i = 0; i < varLen; ++i) varKey += *((char*)key + 4 + i);
    }
    ixfileHandle.fileHandle.readPage(now, tmp);
    ++ixfileHandle.ixReadPageCounter;
    vector<int> rootIndexPage, rootPos;
    IndexPage indexPage, indexPage2;
    LeafPage leafPage, leafPage2;
    bool isLeaf = ((*(int*)tmp) == 0);
    //TODO binary search
    while (!isLeaf) {
        indexPage.encode(tmp, type);
        int p = -1;
        while (p + 1 < indexPage.n) {
            if (type == TypeInt) {
                if (intKey >= indexPage.Int[p + 1]) ++p;
                else break;
            }
            else if (type == TypeReal) {
                if (realKey >= indexPage.Real[p + 1])   ++p;
                else break;
            }
            else {
                if (varKey >= indexPage.Varchar[p + 1]) ++p;
                else break;
            }
        }
        rootIndexPage.pb(now);
        rootPos.pb(p);
        if(p == -1) now = indexPage.Left;
        else now = indexPage.Right[p];
        ixfileHandle.fileHandle.readPage(now, tmp);
        ++ixfileHandle.ixReadPageCounter;
        isLeaf = (*(int*)tmp == 0);
    }
    leafPage.encode(tmp, type);
    int p = -1;
    //TODO binary search
    while (p + 1 < leafPage.n) {
        if (type == TypeInt) {
            if (intKey >= leafPage.Int[p + 1])  ++p;
            else break;
        }
        else if (type == TypeReal) {
            if (realKey >= leafPage.Real[p + 1])   ++p;
            else break;
        }
        else {
            if (varKey >= leafPage.Varchar[p + 1])  ++p;
            else break;
        }
    }
    if (type == TypeInt)    leafPage.Int.insert(leafPage.Int.begin() + p + 1, intKey);
    else if (type == TypeReal)  leafPage.Real.insert(leafPage.Real.begin() + p + 1, realKey);
    else leafPage.Varchar.insert(leafPage.Varchar.begin() + p + 1, varKey);
    leafPage.Rid.insert(leafPage.Rid.begin() + p + 1, rid);
    ++leafPage.n;
    leafPage.update();
    vector<int> tmpInt;
    vector<float> tmpReal;
    vector<string> tmpVar;
    vector<RID> tmpRid;
    if (leafPage.len > PAGE_SIZE) { //need new page
        int n1 = leafPage.n >> 1;
        for (int i = 0; i < n1; ++i) {
            if (type == TypeInt)    tmpInt.pb(leafPage.Int[i]);
            else if (type == TypeReal)  tmpReal.pb(leafPage.Real[i]);
            else tmpVar.pb(leafPage.Varchar[i]);
            tmpRid.pb(leafPage.Rid[i]);
        }
        leafPage2.len = 12, leafPage2.nxt = 0;
        leafPage2.n = leafPage.n - n1;
        leafPage2.keyType = type;
        for (int i = n1; i < leafPage.n; ++i) {
            if (type == TypeInt)    leafPage2.Int.pb(leafPage.Int[i]);
            else if (type == TypeReal)  leafPage2.Real.pb(leafPage.Real[i]);
            else leafPage2.Varchar.pb(leafPage.Varchar[i]);
            leafPage2.Rid.pb(leafPage.Rid[i]);
        }
        if (type == TypeInt)    leafPage.Int = tmpInt;
        else if (type == TypeReal)  leafPage.Real = tmpReal;
        else leafPage.Varchar = tmpVar;
        leafPage2.nxt = leafPage.nxt;
        leafPage2.decode(tmp);
        ixfileHandle.fileHandle.appendPage(tmp);
        ++ixfileHandle.ixAppendPageCounter;
        leafPage.Rid = tmpRid;
        leafPage.n = n1;
        leafPage.nxt = now2 = ixfileHandle.fileHandle.getNumberOfPages() - 1;
        leafPage.decode(tmp);
        ixfileHandle.fileHandle.writePage(now, tmp);
        ++ixfileHandle.ixWritePageCounter;
        //index
        if (type == TypeInt)    intKey2 = leafPage2.Int[0];
        else if (type == TypeReal)  realKey2 = leafPage2.Real[0];
        else varKey2 = leafPage2.Varchar[0];
        while (1) {
            if (rootIndexPage.empty()) {
                ixfileHandle.fileHandle.readPage(0, tmp);
                ++ixfileHandle.ixReadPageCounter;
                int pre = *(int*)tmp;
                memset(tmp, 0, PAGE_SIZE);
                *(int*)tmp = 1;
                indexPage.encode(tmp, type);
                indexPage.Left = pre;
                indexPage.n = 1;
                if (type == TypeInt)    indexPage.Int.pb(intKey2);
                else if (type == TypeReal)  indexPage.Real.pb(realKey2);
                else indexPage.Varchar.pb(varKey2);
                indexPage.Right.pb(now2);
                indexPage.decode(tmp);
                ixfileHandle.fileHandle.appendPage(tmp);
                ++ixfileHandle.ixAppendPageCounter;
                now2 = ixfileHandle.fileHandle.getNumberOfPages() - 1;
                ixfileHandle.fileHandle.readPage(0, tmp);
                ++ixfileHandle.ixReadPageCounter;
                changeHiddenPage(tmp, now2, type);
                ixfileHandle.fileHandle.writePage(0, tmp);  //root pagenum
                ++ixfileHandle.ixWritePageCounter;
                break;
            }
            now = rootIndexPage.back();
            rootIndexPage.pop_back();
            p = rootPos.back();
            rootPos.pop_back();
            ixfileHandle.fileHandle.readPage(now, tmp);
            ++ixfileHandle.ixReadPageCounter;
            indexPage.encode(tmp, type);
            if (type == TypeInt)    indexPage.Int.insert(indexPage.Int.begin() + p + 1, intKey2);
            else if (type == TypeReal)  indexPage.Real.insert(indexPage.Real.begin() + p + 1, realKey2);
            else indexPage.Varchar.insert(indexPage.Varchar.begin() + p + 1, varKey2);
            indexPage.Right.insert(indexPage.Right.begin() + p + 1, now2);
            ++indexPage.n;
            indexPage.update();
            vector<int> tmpRight;
            if (indexPage.len > PAGE_SIZE) {//index split
                n1 = indexPage.n >> 1;
                if (type == TypeInt)   intKey2 = indexPage.Int[n1];
                else if (type == TypeReal)  realKey2 = indexPage.Real[n1];
                else varKey2 = indexPage.Varchar[n1];
                tmpInt.clear(), tmpReal.clear(), tmpVar.clear(), tmpRight.clear();
                for (int i = 0; i < n1; ++i) {
                    if (type == TypeInt) tmpInt.pb(indexPage.Int[i]);
                    else if (type == TypeReal)  tmpReal.pb(indexPage.Real[i]);
                    else tmpVar.pb(indexPage.Varchar[i]);
                    tmpRight.pb(indexPage.Right[i]);
                }
                indexPage2.len = 12;
                indexPage2.n = indexPage.n - n1 - 1;
                indexPage2.keyType = type;
                indexPage2.Left = indexPage.Right[n1];
                for (int i = n1 + 1; i < indexPage.n; ++i) {
                    if (type == TypeInt)    indexPage2.Int.pb(indexPage.Int[i]);
                    else if (type == TypeReal)  indexPage2.Real.pb(indexPage.Real[i]);
                    else indexPage2.Varchar.pb(indexPage.Varchar[i]);
                    indexPage2.Right.pb(indexPage.Right[i]);
                }
                indexPage2.decode(tmp);
                ixfileHandle.fileHandle.appendPage(tmp);
                ++ixfileHandle.ixAppendPageCounter;
                now2 = ixfileHandle.fileHandle.getNumberOfPages() - 1;
                if (type == TypeInt)    indexPage.Int = tmpInt;
                else if (type == TypeReal)  indexPage.Real = tmpReal;
                else indexPage.Varchar = tmpVar;
                indexPage.Right = tmpRight;
                indexPage.n = n1;
                indexPage.decode(tmp);
                ixfileHandle.fileHandle.writePage(now, tmp);
                ++ixfileHandle.ixWritePageCounter;
            }
            else {
                indexPage.decode(tmp);
                ixfileHandle.fileHandle.writePage(now, tmp);
                ++ixfileHandle.ixWritePageCounter;
                break;
            }
        }
    }
    else {
        leafPage.decode(tmp);
        ixfileHandle.fileHandle.writePage(now, tmp);
        ++ixfileHandle.ixWritePageCounter;
    }
    return 0;
}

RC IndexManager::deleteEntry(IXFileHandle &ixfileHandle, const Attribute &attribute, const void *key, const RID &rid) {
    int now = getRootPage(ixfileHandle);
    int type = getKeyType(ixfileHandle);
    int intKey = 0, varLen = 0;
    float realKey = 0.0;
    string varKey = "";
    IndexPage indexPage;
    LeafPage leafPage;
    if (type == TypeInt)    intKey = *(int*)key;
    else if (type == TypeReal)  realKey = *(float*)key;
    else {
        varLen = *(int*)key;
        for (int i = 0; i < varLen; ++i)    varKey += *((char*)key + 4 + i);
    }
    ixfileHandle.fileHandle.readPage(now, tmp);
    ++ixfileHandle.ixReadPageCounter;
    bool isLeaf = (*(int*)tmp == 0);
    while (!isLeaf) {
        indexPage.encode(tmp, type);
        int p = -1;
        while (p + 1 < indexPage.n) {
            if (type == TypeInt) {
                if (intKey >= indexPage.Int[p + 1]) ++p;
                else break;
            }
            else if (type == TypeReal) {
                if (intKey >= indexPage.Real[p + 1])    ++p;
                else break;
            }
            else  {
                if (varKey >= indexPage.Varchar[p + 1]) ++p;
                else break;
            }
        }
        if (p == -1)    now = indexPage.Left;
        else now = indexPage.Right[p];
        ixfileHandle.fileHandle.readPage(now, tmp);
        ++ixfileHandle.ixReadPageCounter;
        isLeaf = (*(int*)tmp == 0);
    }
    leafPage.encode(tmp, type);
    bool find = 0;
    vector<int> tmpInt;
    vector<float> tmpReal;
    vector<string> tmpVar;
    vector<RID> tmpRid;
    for (int i = 0; i < leafPage.n; ++i) {
        if (type == TypeInt) {
            if (intKey == leafPage.Int[i] && rid.pageNum == leafPage.Rid[i].pageNum && rid.slotNum == leafPage.Rid[i].slotNum)   find = 1;
            else tmpInt.pb(leafPage.Int[i]), tmpRid.pb(leafPage.Rid[i]);
        }
        else if (type == TypeReal) {
            if (realKey == leafPage.Real[i] && rid.pageNum == leafPage.Rid[i].pageNum && rid.slotNum == leafPage.Rid[i].slotNum)    find = 1;
            else tmpReal.pb(leafPage.Real[i]), tmpRid.pb(leafPage.Rid[i]);
        }
        else {
            if (varKey == leafPage.Varchar[i] && rid.pageNum == leafPage.Rid[i].pageNum && rid.slotNum == leafPage.Rid[i].slotNum)  find = 1;
            else tmpVar.pb(leafPage.Varchar[i]), tmpRid.pb(leafPage.Rid[i]);
        }
    }
    leafPage.n = tmpRid.size();
    if (type == TypeInt)    leafPage.Int = tmpInt;
    else if (type == TypeReal)  leafPage.Real = tmpReal;
    else leafPage.Varchar = tmpVar;
    leafPage.Rid = tmpRid;
    leafPage.decode(tmp);
    ixfileHandle.fileHandle.writePage(now, tmp);
    ++ixfileHandle.ixWritePageCounter;
    if (!find)  return -1;
    return 0;
}

RC IndexManager::scan(IXFileHandle &ixfileHandle,
                      const Attribute &attribute,
                      const void      *lowKey,
                      const void      *highKey,
                      bool			lowKeyInclusive,
                      bool        	highKeyInclusive,
                      IX_ScanIterator &ix_ScanIterator)
{
    //file should be opened
    if (!ixfileHandle.fileHandle.open || access(ixfileHandle.fileName.c_str(), 0) != 0) return -1;


    int type = IndexManager :: getKeyType(ixfileHandle);
    int pageNum = IndexManager :: getRootPage(ixfileHandle);
    ix_ScanIterator.type = type;
    //initialize iterator
    ix_ScanIterator.ixFileHandle = ixfileHandle;
    ix_ScanIterator.highKeyInclu = highKeyInclusive;


    if (type == TypeInt) {
        ix_ScanIterator.lowKeyInt = (lowKey) ? *(int*)lowKey : INT_MIN;
        ix_ScanIterator.highKeyInt = (highKey) ? *(int*)highKey : INT_MAX;
    } else if (attribute.type == TypeReal) {
        ix_ScanIterator.lowKeyReal = (lowKey) ? *(float*)lowKey : (float)INT_MIN;
        ix_ScanIterator.highKeyReal = (highKey) ? *(float*)highKey : (float)INT_MAX;
    } else{
        if(lowKey) {
            int len = *(int*)lowKey;
            for (int i = 0; i < len; ++i)
                ix_ScanIterator.lowKeyVar += *((char*)lowKey + 4 + i);
        }
        if (highKey) {
            int len = *(int*)highKey;
            for (int i = 0; i < len; ++i) {
                ix_ScanIterator.highKeyVar += *((char*)highKey + 4 + i);
            }
        } else {
            ix_ScanIterator.highKellNull = true;
        }
    }
    //store the current root page into tmp
    ixfileHandle.fileHandle.readPage(pageNum,ix_ScanIterator.tmp);
    ixfileHandle.ixReadPageCounter++;

    bool isLeaf = (*(int*)ix_ScanIterator.tmp == 0);
    // when it is index page
    while(!isLeaf) {
        //searching leaf node
        IndexPage indexPage;
        indexPage.encode(ix_ScanIterator.tmp,type);
        int pos = -1;
        //locate the interval
        while (pos+1 < indexPage.n) {
            if (type == TypeInt) {
                if (ix_ScanIterator.lowKeyInt >= indexPage.Int[pos+1])
                    pos++;
                else
                    break;
            } else if (type == TypeReal) {
                if (ix_ScanIterator.lowKeyReal >= indexPage.Real[pos+1])
                    pos++;
                else
                    break;
            } else {
                if (ix_ScanIterator.lowKeyVar >= indexPage.Varchar[pos+1])
                    pos++;
                else
                    break;
            }
        }
        //update page number
        if (pos == -1) pageNum = indexPage.Left;
        else pageNum = indexPage.Right[pos];

        ixfileHandle.fileHandle.readPage(pageNum,ix_ScanIterator.tmp);
        ixfileHandle.ixReadPageCounter++;
        isLeaf = (*(int*)ix_ScanIterator.tmp == 0);
    }

    if (pageNum < 1 ) return -1;

    ix_ScanIterator.currentPage = pageNum;
    ix_ScanIterator.position = 0;

    LeafPage leafPage;
    leafPage.encode(ix_ScanIterator.tmp,type);

    for (int i = 0; i < leafPage.n; ++i) {
        if (type == TypeInt) {
            if (lowKeyInclusive && leafPage.Int[i] >= ix_ScanIterator.lowKeyInt) {
                ix_ScanIterator.position = i;
                return 0;
            }

            if (!lowKeyInclusive && leafPage.Int[i] > ix_ScanIterator.lowKeyInt) {
                ix_ScanIterator.position = i;
                return 0;
            }
        } else if (type == TypeReal) {
            if (lowKeyInclusive && leafPage.Real[i] >= ix_ScanIterator.lowKeyReal) {
                ix_ScanIterator.position = i;
                return 0;
            }

            if (!lowKeyInclusive && leafPage.Real[i] > ix_ScanIterator.lowKeyReal) {
                ix_ScanIterator.position = i;
                return 0;
            }
        } else {
            if (lowKeyInclusive && leafPage.Varchar[i] >= ix_ScanIterator.lowKeyVar) {
                ix_ScanIterator.position = i;
                return 0;
            }

            if (!lowKeyInclusive && leafPage.Varchar[i] > ix_ScanIterator.lowKeyVar) {
                ix_ScanIterator.position = i;
                return 0;
            }
        }
    }
    return 0;
}

RC IndexManager::getKeyType(IXFileHandle &ixFileHandle) {
    ixFileHandle.fileHandle.readPage(0,tmp);
    ixFileHandle.ixReadPageCounter++;
    return *(int*)(tmp+4);
}

RC IndexManager::getRootPage(IXFileHandle &ixFileHandle) {
    ixFileHandle.fileHandle.readPage(0,tmp);
    ixFileHandle.ixReadPageCounter++;
    return *(int*)tmp;
}

void IndexManager::printBtree(IXFileHandle &ixfileHandle, const Attribute &attribute){
    int rootPage = getRootPage(ixfileHandle);
    ixfileHandle.fileHandle.readPage(rootPage,tmp);
    bool isLeaf = *(int*)tmp == 0;
    int type = getKeyType(ixfileHandle);


}
IX_ScanIterator::IX_ScanIterator(IXFileHandle ixFileHandle) : ixFileHandle(ixFileHandle) {
    tmp = new char[PAGE_SIZE];
    highKellNull = false;
    highKeyInclu = true;
    lowKeyVar = "";
    highKeyVar = "";
}

IX_ScanIterator::IX_ScanIterator() {
    tmp = new char[PAGE_SIZE];
    highKellNull = false;
    highKeyInclu = true;
    lowKeyVar = "";
    highKeyVar = "";
}

IX_ScanIterator::~IX_ScanIterator()
{
    delete [] tmp;
}
RC IX_ScanIterator::getNextEntry(RID &rid, void *key)
{
    LeafPage leafPage;
    leafPage.encode(tmp,type);

    while(1){

        if (position < leafPage.n && leafPage.Rid[position].pageNum == Rid.pageNum && leafPage.Rid[position].slotNum == Rid.slotNum)
            position++;

        //once the page is scaned , go to next page
        while (position >= leafPage.n) {
            if (leafPage.nxt != -1) {
                currentPage = leafPage.nxt;
                position = 0;
                ixFileHandle.fileHandle.readPage(currentPage,tmp);
                ixFileHandle.ixReadPageCounter++;
                leafPage.encode(tmp,type);
            } else {
                return IX_EOF;
            }
        }


        if (type == TypeInt) {
            if (highKeyInclu) {
                if(leafPage.Int[position] <=  highKeyInt) {
                    *(int*) key = leafPage.Int[position];
                    rid = leafPage.Rid[position];
                    Rid = rid;
                    return 0;
                } else
                    return IX_EOF;
            } else {
                if(leafPage.Int[position] < highKeyInt) {
                    *(int*) key = leafPage.Int[position];
                    rid = leafPage.Rid[position];
                    Rid = rid;
                    return 0;
                } else
                    return IX_EOF;
            }
        } else if (type == TypeReal) {
            if (highKeyInclu) {
                if (leafPage.Real[position] <= highKeyReal) {
                    *(float*) key = leafPage.Real[position];
                    rid = leafPage.Rid[position];
                    Rid = rid;
                    return 0;
                } else
                    return IX_EOF;
            } else {
                if (leafPage.Real[position] < highKeyReal) {
                    *(float*) key = leafPage.Real[position];
                    rid = leafPage.Rid[position];
                    Rid = rid;
                    return 0;
                } else
                    return IX_EOF;
            }
        } else {
            if (highKeyInclu) {
                if (highKellNull || leafPage.Varchar[position] <= highKeyVar) {
                    *(int*)key = leafPage.Varchar[position].length();
                    memcpy((char*)key+4,leafPage.Varchar[position].c_str(),*(int*)key);
                    rid = leafPage.Rid[position];
                    Rid = rid;
                    return 0;
                } else
                    return IX_EOF;

            } else {
                if(highKellNull || leafPage.Varchar[position] < highKeyVar) {
                    *(int*)key = leafPage.Varchar[position].length();
                    memcpy((char*)key+4,leafPage.Varchar[position].c_str(),*(int*)key);
                    rid = leafPage.Rid[position];
                    Rid = rid;
                    return 0;
                } else
                    return IX_EOF;
            }
        }

    }
}

RC IX_ScanIterator::close()
{
    return 0;
}


IXFileHandle::IXFileHandle()
{
    ixReadPageCounter = 0;
    ixWritePageCounter = 0;
    ixAppendPageCounter = 0;
    fileName = "";
}

IXFileHandle::~IXFileHandle()
{
}

IndexPage ::IndexPage() {
    n = nxt = keyType = 0;
    len = 12;
    Int.clear(), Real.clear(), Varchar.clear();
}

void IndexPage::encode(void* data, int type) {
    Int.clear(), Real.clear(), Varchar.clear(), Right.clear();
    keyType = type;
    int offset = 4;
    n = *(int*)((char*)data + offset);
    offset += 4;
    Left = *(int*)((char*)data + offset);
    offset += 4;
    for (int i = 0; i < n; ++i) {
        if (type == TypeInt) {
            Int.pb(*(int*)((char*)data + offset));
            offset += 4;
            Right.pb(*(int*)((char*)data + offset));
            offset += 4;
        }
        else if (type == TypeReal) {
            Real.pb(*(float*)((char*)data + offset));
            offset += 4;
            Right.pb(*(int*)((char*)data + offset));
            offset += 4;
        }
        else {
            int l = *(int*)((char*)data + offset);
            offset += 4;
            string now = "";
            for (int j = 0; j < l; ++j) now += *((char*)data + offset + j);
            offset += l;
            Varchar.pb(now);
            Right.pb(*(int*)((char*)data + offset));
            offset += 4;
        }
    }
    len = offset;
}

void IndexPage::decode(void *data) {
    *(int*)data = 1;//index
    int offset = 4;
    *(int*)((char*)data + offset) = n;
    offset += 4;
    *(int*)((char*)data + offset) = Left;
    offset += 4;
    for (int i = 0; i < n; ++i) {
        if (keyType == TypeInt) {
            *(int*)((char*)data + offset) = Int[i];
            offset += 4;
            *(int*)((char*)data + offset) = Right[i];
            offset += 4;
        }
        else if (keyType == TypeReal) {
            *(float*)((char*)data + offset) = Real[i];
            offset += 4;
            *(int*)((char*)data + offset) = Right[i];
            offset += 4;
        }
        else {
            int l = Varchar[i].size();
            *(int*)((char*)data + offset) = l;
            offset += 4;
            for (int j = 0; j < l; ++j) *((char*)data + offset + j) = Varchar[i][j];
            offset += l;
            *(int*)((char*)data + offset) = Right[i];
            offset += 4;
        }
    }
}

void IndexPage:: update() {
    len = 12;
    for (int i = 0; i < n; ++i) {
        len += 8;
        if (keyType == TypeVarChar) len += Varchar[i].size();
    }
}

LeafPage ::LeafPage() {
    n = nxt = keyType = 0;
    len = 12;
    Int.clear(), Real.clear(), Varchar.clear();
}

void LeafPage::encode(void *data, int type) {
    Int.clear(), Real.clear(), Varchar.clear(), Rid.clear();
    keyType = type;
    int offset = 4;
    n = *(int*)((char*)data + offset);
    offset += 4;
    nxt = *(int*)((char*)data + offset);
    offset += 4;
    RID now;
    for (int i = 0; i < n; ++i) {
        if (type == TypeInt) {
            Int.pb(*(int*)((char*)data + offset));
            offset += 4;
            now.pageNum = (*(int*)((char*)data + offset));
            offset += 4;
            now.slotNum = (*(int*)((char*)data + offset));
            offset += 4;
        }
        else if (type == TypeReal) {
            Real.pb(*(float*)((char*)data + offset));
            offset += 4;
            now.pageNum = (*(int*)((char*)data + offset));
            offset += 4;
            now.slotNum = (*(int*)((char*)data + offset));
            offset += 4;
        }
        else {
            int l = *(int*)((char*)data + offset);
            offset += 4;
            string t = "";
            for (int j = 0; j < l; ++j) t += *((char*)data + offset + j);
            Varchar.pb(t);
            offset += l;
            now.pageNum = *(int*)((char*)data + offset);
            offset += 4;
            now.slotNum = *(int*)((char*)data + offset);
            offset += 4;
        }
        Rid.pb(now);
    }
    len = offset;
}

void LeafPage ::decode(void *data) {
    *(int*)data = 0;//leaf
    int offset = 4;
    *(int*)((char*)data + offset) = n;
    offset += 4;
    *(int*)((char*)data + offset) = nxt;
    offset += 4;
    for (int i = 0; i < n; ++i) {
        if (keyType == TypeInt) {
            *(int*)((char*)data + offset) = Int[i];
            offset += 4;
            *(int*)((char*)data + offset) = Rid[i].pageNum;
            offset += 4;
            *(int*)((char*)data + offset) = Rid[i].slotNum;
            offset += 4;
        }
        else if (keyType == TypeReal) {
            *(float*)((char*)data + offset) = Real[i];
            offset += 4;
            *(int*)((char*)data + offset) = Rid[i].pageNum;
            offset += 4;
            *(int*)((char*)data + offset) = Rid[i].slotNum;
            offset += 4;
        }
        else {
            *(int*)((char*)data + offset) = Varchar[i].size();
            offset += 4;
            for (int j = 0; j < Varchar[i].size(); ++j)    *((char*)data + offset + j) = Varchar[i][j];
            offset += Varchar[i].size();
            *(int*)((char*)data + offset) = Rid[i].pageNum;
            offset += 4;
            *(int*)((char*)data + offset) = Rid[i].slotNum;
            offset += 4;
        }
    }
}

void LeafPage::update() {
    len = 12;
    for (int i = 0; i < n; ++i) {
        len += 12;//key pageNum slotNum
        if (keyType == TypeVarChar) len += Varchar[i].size();
    }
}

RC IXFileHandle::collectCounterValues(unsigned &readPageCount, unsigned &writePageCount, unsigned &appendPageCount) {
    readPageCount = ixReadPageCounter;
    writePageCount = ixWritePageCounter;
    appendPageCount = ixAppendPageCounter;
    return 0;
}