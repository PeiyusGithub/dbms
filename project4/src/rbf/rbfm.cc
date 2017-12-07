#include "rbfm.h"

RecordBasedFileManager* RecordBasedFileManager::_rbf_manager = 0;

RecordBasedFileManager* RecordBasedFileManager::instance() {
    if(!_rbf_manager)
        _rbf_manager = new RecordBasedFileManager();
    return _rbf_manager;
}

RecordBasedFileManager::RecordBasedFileManager() {
	tmp = new char[PAGE_SIZE];
}

RecordBasedFileManager::~RecordBasedFileManager() {
	delete[] tmp;
}

RC RecordBasedFileManager::createFile(const string &fileName) {
    PagedFileManager* pfm = PagedFileManager::instance();
    const char* s = fileName.c_str();
    return pfm -> createFile(s);
}

RC RecordBasedFileManager::destroyFile(const string &fileName) {
    PagedFileManager* pfm = PagedFileManager::instance();
    const char* s = fileName.c_str();
	return pfm -> destroyFile(s);
}

RC RecordBasedFileManager::openFile(const string &fileName, FileHandle &fileHandle) {
    PagedFileManager* pfm = PagedFileManager::instance();
	return pfm -> openFile(fileName, fileHandle);
}

RC RecordBasedFileManager::closeFile(FileHandle &fileHandle) {
    PagedFileManager* pfm = PagedFileManager::instance();
    return pfm -> closeFile(fileHandle);
}

RC RecordBasedFileManager::insertRecord(FileHandle &fileHandle, const vector<Attribute> &recordDescriptor, const void *data, RID &rid) {
	Record record(recordDescriptor, data);
	int n = fileHandle.getNumberOfPages();
	if (n == 1) {	//
		memset(tmp, 0, PAGE_SIZE);		//new page
		*(int*)tmp = PAGE_SIZE - 8;
		fileHandle.appendPage(tmp);
		*(int*)tmp = 0;
		Page page(tmp);
		page.slotNum = 0, page.freeSpace = PAGE_SIZE - 8; // N F
		page.decode(tmp);
		fileHandle.appendPage(tmp);
		n += 2;
	}
	int headPageNum = (n - 2) / 1025 * 1025 + 1, sz = record.length + 8;
	int id = (n - 2) % 1025;
	fileHandle.readPage(headPageNum, tmp);
	int now = -1;
	if (*(int*)(tmp + (id - 1) * 4) >= sz)	now = n - 1; //fit in the last page
	else {	
		for (int i = 0; i < id - 1; ++i) {
			if (*(int*)(tmp + i * 4) >= sz) {
				now = headPageNum + i + 1;
				break;
			}
		}
		if (now == -1) {
			for (int i = 0; i < (n - 2) / 1025; ++i) {
				headPageNum = i * 1025 + 1;
				fileHandle.readPage(headPageNum, tmp);
				for (int j = 0; j < 1024; ++j) {
					if (*(int*)(tmp + j * 4) >= sz)	{
						now = headPageNum + j + 1;
						break;
					}
				}
			}
			if (now == -1) {			//add new page
				memset(tmp, 0, PAGE_SIZE);
				if (id == 1024)	fileHandle.appendPage(tmp), ++n;
				headPageNum = (n - 2) / 1025 * 1025 + 1;
				Page page(tmp);
				page.slotNum = 0, page.freeSpace = PAGE_SIZE - 8;
				page.decode(tmp);
				fileHandle.appendPage(tmp);
				now = n++;
			}
		}
	}
	fileHandle.readPage(now, tmp);
	Page page(tmp);
	rid.pageNum = now;
	int slotNum = -1;
	for (int i = 0; i < page.slotNum; ++i)
		if (page.slotOffset[i] == 0 && page.slotLength[i] == -1) {
			slotNum = i;
			break;
		}
	rid.slotNum = (slotNum != -1 ? slotNum : page.slotNum);
	int offset = PAGE_SIZE - page.freeSpace - page.slotNum * 8 - 8;
	page.freeSpace -= record.length;
	if (rid.slotNum == page.slotNum) {
		++page.slotNum;
		page.freeSpace -= 8;
		page.slotOffset.pb(offset);
		page.slotLength.pb(record.length);
	}
	else {
		page.slotOffset[rid.slotNum] = offset;
		page.slotLength[rid.slotNum] = record.length;
	}
	memcpy(page.tmp + offset, record.tmp, record.length);
	page.decode(tmp);
	fileHandle.writePage(rid.pageNum, tmp);
	headPageNum = (rid.pageNum - 1) / 1025 * 1025 + 1;
	fileHandle.readPage(headPageNum, tmp);
	id = (rid.pageNum - 1) % 1025;
	*(int*)(tmp + (id - 1) * 4) = page.freeSpace;
	fileHandle.writePage(headPageNum, tmp);
	return 0;
}

RC RecordBasedFileManager::readRecord(FileHandle &fileHandle, const vector<Attribute> &recordDescriptor, const RID &rid, void *data) {
	fileHandle.readPage(rid.pageNum, tmp);
    Page page(tmp);
	if (page.slotOffset[rid.slotNum] == 0 && page.slotLength[rid.slotNum] == -1)	return -1;
	RID now = rid;
	while (page.slotOffset[now.slotNum] < 0) {
		RID nxt;
		nxt.pageNum = -page.slotOffset[now.slotNum];
		nxt.slotNum = -page.slotLength[now.slotNum];
		fileHandle.readPage(nxt.pageNum, tmp);
		now = nxt;
		page.encode(tmp);
	}
    Record record(recordDescriptor, page.slotLength[now.slotNum], page.tmp + page.slotOffset[now.slotNum]);
    record.decode(data);
	return 0;
}

RC RecordBasedFileManager::printRecord(const vector<Attribute> &recordDescriptor, const void *data) {
	int n = 0;
	for (auto&x : recordDescriptor)
		if (x.valid)
			++n;
	int nullSize = (n + 7) / 8;
	char* nullIndicator = new char[nullSize];
	memcpy(nullIndicator, data, nullSize);
	char* now = (char*)data + nullSize;
	for (int i = 0; i < recordDescriptor.size(); ++i) {
		if (!recordDescriptor[i].valid)	continue;
		int block = i / 8, pos = i % 8;
		if (nullIndicator[block] & (1 << (7 - pos)))	cout << recordDescriptor[i].name << ": NULL" << endl;
		else {
			cout << recordDescriptor[i].name << ": ";
			switch(recordDescriptor[i].type) {
				case TypeInt:
					cout << *(int*)now << endl;
					now += 4;
					break;
				case TypeReal:
					cout << *(float*)now << endl;
					now += 4;
					break;
				case TypeVarChar:
					int len = *(int*)now;
					now += 4;
					for (int j = 0; j < len; ++j)	cout << (char)*now++;
					cout << endl;
					break;
			}
		}
	}
	delete[] nullIndicator;
	return 0;
}

RC RecordBasedFileManager::deleteRecord(FileHandle &fileHandle, const vector<Attribute> &recordDescriptor, const RID &rid){
    fileHandle.readPage(rid.pageNum, tmp);
	Page page(tmp);
	RID now = rid, nxt;
	while (page.slotOffset[now.slotNum] < 0) {
		nxt.pageNum = -page.slotOffset[now.slotNum];
		nxt.slotNum = -page.slotLength[now.slotNum];
		page.slotOffset[now.slotNum] = 0;
		page.slotLength[now.slotNum] = -1;
		page.decode(tmp);
		fileHandle.writePage(now.pageNum, tmp);
		fileHandle.readPage(nxt.pageNum, tmp);
		now = nxt;
		page.encode(tmp);
	}  
	int offset = page.slotOffset[now.slotNum], sz = page.slotLength[now.slotNum];
	page.slotOffset[now.slotNum] = 0;
	page.slotLength[now.slotNum] = -1;
	int l = PAGE_SIZE - page.freeSpace - page.slotNum * 8 - 8 - offset - sz;
	char* data = new char[l];
	memcpy(data, page.tmp + offset + sz, l);//shift
	memcpy(page.tmp + offset, data, l);
	for (int i = 0; i < page.slotNum; ++i) {
		if (page.slotOffset[i] > offset)
			page.slotOffset[i] -= sz;
	}
	page.freeSpace += sz;
	page.decode(tmp);
	fileHandle.writePage(now.pageNum, tmp);
	int headPageNum = (now.pageNum - 1) / 1025 * 1025 + 1;
	fileHandle.readPage(headPageNum, tmp);
	int id = (now.pageNum - 1) % 1025;
	*(int*)(tmp + (id - 1) * 4) = page.freeSpace;
	fileHandle.writePage(headPageNum, tmp);
	delete[] data;
	return 0;
}

RC RecordBasedFileManager::updateRecord(FileHandle &fileHandle, const vector<Attribute> &recordDescriptor, const void *data, const RID &rid){
    deleteRecord(fileHandle, recordDescriptor, rid);
	Record record(recordDescriptor, data);
	fileHandle.readPage(rid.pageNum, tmp);
	Page page(tmp);
	if (page.freeSpace >= record.length) {		// could fit in
		int offset = PAGE_SIZE - page.freeSpace - page.slotNum * 8 - 8;
		page.slotOffset[rid.slotNum] = offset;
		page.slotLength[rid.slotNum] = record.length;
		page.freeSpace -= record.length;
		memcpy(page.tmp + offset, record.tmp, record.length);
		page.decode(tmp);
		fileHandle.writePage(rid.pageNum, tmp);
		int headPageNum = (rid.pageNum - 1) / 1025 * 1025 + 1;
		fileHandle.readPage(headPageNum, tmp);
		int id = (rid.pageNum - 1) % 1025;
		*(int*)(tmp + (id - 1) * 4) = page.freeSpace;
		fileHandle.writePage(headPageNum, tmp);
	}
	else {
		RID nxt;
		insertRecord(fileHandle, recordDescriptor, data, nxt);
		page.slotOffset[rid.slotNum] = -nxt.pageNum;
		page.slotLength[rid.slotNum] = -nxt.slotNum;
		page.decode(tmp);
		fileHandle.writePage(rid.pageNum, tmp);
	}
	return 0;
}

RC RecordBasedFileManager::readAttribute(FileHandle &fileHandle, const vector<Attribute> &recordDescriptor, const RID &rid, const string &attributeName, void *data) {
    fileHandle.readPage(rid.pageNum, tmp);
    Page page(tmp);
    RID now = rid, nxt;
    while(page.slotOffset[now.slotNum] < 0) { 
        nxt.pageNum = -page.slotOffset[now.slotNum];
        nxt.slotNum = -page.slotLength[now.slotNum];
        fileHandle.readPage(nxt.pageNum, tmp);
        now = nxt;
        page.encode(tmp);
    }
    Record record(recordDescriptor, page.slotLength[now.slotNum], page.tmp + page.slotOffset[now.slotNum]);
    int nullSize = 1;
	*(char *)data = 0;
	for(int i = 0; i < recordDescriptor.size(); ++i)
        if(recordDescriptor[i].name == attributeName) {
            int l1 = *(int*)(record.tmp + i * 4), l2 = *(int*)(record.tmp + (i + 1) * 4);
            if(i == 0) l1 = (recordDescriptor.size() + 1) * 4;
			if (l1 == l2)	*(char *)data = char(1 << 7);
            memcpy((char*)data + nullSize, record.tmp + l1, l2 - l1);
            break;
        }
    return 0;
}

RC RecordBasedFileManager::scan(FileHandle &fileHandle, const vector<Attribute> &recordDescriptor, const string &conditionAttribute, const CompOp compOp, const void *value, const vector<string> &attributeNames, RBFM_ScanIterator &rbfm_ScanIterator) {
	rbfm_ScanIterator.fileHandle = fileHandle;
	rbfm_ScanIterator.recordDescriptor = recordDescriptor;
	rbfm_ScanIterator.conditionAttribute = conditionAttribute;
	rbfm_ScanIterator.compOp = compOp;
	rbfm_ScanIterator.attributeNames = attributeNames;
	int sz = 0;
	for (auto&x : recordDescriptor) {
		if (x.name != conditionAttribute)	continue;
		if (x.type != TypeVarChar)	sz = 4;
		else sz += 4 + *(int*)value;
		break;
	}
	rbfm_ScanIterator.iterator = new char[sz];
	memcpy(rbfm_ScanIterator.iterator, value, sz);
	return 0;
}

RC RBFM_ScanIterator::getNextRecord(RID &rid, void *data) {
	char* now = new char[PAGE_SIZE];
	while (1) {
		if (pre.pageNum >= fileHandle.getNumberOfPages()) {
			delete[] now;
			return -1;
		}
		++pre.slotNum;
		fileHandle.readPage(pre.pageNum, now);
		Page page(now);
		if (page.slotNum <= pre.slotNum) {	//new page
			if ((pre.pageNum - 1) % 1025 == 1024)	++pre.pageNum;
			++pre.pageNum;
			pre.slotNum = -1;
			continue;
		}
		if (page.slotOffset[pre.slotNum] == 0 && page.slotLength[pre.slotNum] == -1)	continue;
		if (page.slotOffset[pre.slotNum] < 0)	continue;
		Record record(recordDescriptor, page.slotLength[pre.slotNum], page.tmp + page.slotOffset[pre.slotNum]);
		if (record.cmp(conditionAttribute, compOp, iterator)) {
			rid.pageNum = pre.pageNum, rid.slotNum = pre.slotNum;
			record.decode2(attributeNames, data);
			delete[] now;
			return 0;
		}
	}
	return 0;
}

bool Record::cmp(string &conditionAttribute, const CompOp compOp, const void *value) {
	if (compOp == NO_OP)	return 1;
	for (int i = 0; i < recordDescriptor.size(); ++i) {
		Attribute t = recordDescriptor[i];
		if (t.name == conditionAttribute) {
			int pos1 = (i == 0 ? (recordDescriptor.size() + 1) * 4 : (*(int*)(tmp + i * 4))), pos2 = *(int*)(tmp + i * 4 + 4);
			int sz = pos2 - pos1;
			if (sz == 0)	return 0;
			if (t.type == TypeInt) {
				int x = *(int*)(tmp + pos1), y = *(int*)value;
				switch(compOp) {
					case EQ_OP: return x == y;
					case LT_OP: return x < y;
					case LE_OP: return x <= y;
					case GT_OP:	return x > y;
					case GE_OP:	return x >= y;
					case NE_OP: return x != y;
					default : return 0;
				}
			}
			else if (t.type == TypeReal) {
				float x = *(float*)(tmp + pos1), y = *(float*)value;
				switch(compOp) {
					case EQ_OP: return x == y;
					case LT_OP: return x < y;
					case LE_OP: return x <= y;
					case GT_OP:	return x > y;
					case GE_OP:	return x >= y;
					case NE_OP: return x != y;
					default : return 0;
				}
			}
			else {
				string x = "", y = "";
				int lx = *(int*)(tmp + pos1), ly = *(int*)value;
				for (int i = 0; i < lx; ++i)	x += *(char*)(tmp + pos1 + 4 + i);
				for (int i = 0; i < ly; ++i)	y += *((char*)value + 4 + i);
				switch(compOp) {
					case EQ_OP: return x == y;
					case LT_OP: return x < y;
					case LE_OP: return x <= y;
					case GT_OP:	return x > y;
					case GE_OP:	return x >= y;
					case NE_OP: return x != y;
					default : return 0;
				}
			}
			break;
		}
	}
	return 0;
}

Record::Record(const vector<Attribute> &recordDescriptorTmp, int size, const void *data) {
    recordDescriptor = recordDescriptorTmp;
    length = size;
    tmp = new char[length];
    memcpy(tmp, data, size);
}

Record::Record(const vector<Attribute> &recordDescriptorTmp, const void *data) {
	recordDescriptor = recordDescriptorTmp;
	int n = 0;
	for (auto& x : recordDescriptor)
		if (x.valid)
			++n;
	int nullSize = (n + 7) / 8;
	char* nullIndicator = new char[nullSize];
	memcpy(nullIndicator, data, nullSize);
	int fieldCount = recordDescriptor.size();
	int offset = (fieldCount + 1) * 4;
	char* now = (char*)data + nullSize;
	vector<int> p;
	p.clear();
	for (int i = 0; i < fieldCount; ++i) {
		int block = i / 8, pos = i % 8;
		Attribute t = recordDescriptor[i];
		if (!t.valid || nullIndicator[block] & (1 << (7 - pos))) {
			p.pb(offset);
			continue;
		}
		switch(t.type) {
			case TypeInt:
			case TypeReal:
				offset += 4;
				break;
			case TypeVarChar:
				int len = *(int*)(now + offset - (fieldCount + 1) * 4);
				offset += len + 4;
				break;
		}
		p.pb(offset);
	}
	length = offset;
	tmp = new char[offset];
	*(int*)tmp = fieldCount;
	for (int i = 0; i < fieldCount; ++i)
		*(int*)(tmp + (i + 1) * 4) = p[i];
	memcpy(tmp + (fieldCount + 1) * 4, now, offset - (fieldCount + 1) * 4);
	delete[] nullIndicator;
}

void Record::decode(void *data) {
	int n = 0;
	for (auto& x : recordDescriptor)
		if (x.valid)	
			++n;
	int nullSize = (n + 7) / 8;
	vector<int> p;
	char* nullIndicator = new char[nullSize];
	memset(nullIndicator, 0, nullSize);
	p.pb((recordDescriptor.size() + 1) * 4);
	for (int i = 0; i < n; ++i) {
		char* now = tmp + (i + 1) * 4;	//offset
		p.pb(*(int*)now);
	}
	p.pb(length);
	int now = 0, offset = 0, tot = *(int*)(tmp);
	for (int i = 0; i < recordDescriptor.size(); ++i) {
		if (!recordDescriptor[i].valid)	continue;
		if (i >= tot) {
			int block = now / 8, pos = now % 8;
			nullIndicator[block] |= (1 << (7 - pos)); //not exist this attribute
			++now;
			continue;
		}
		int sz = p[i + 1] - p[i];
		memcpy((char*)data + nullSize + offset, tmp + p[i], sz);
		offset += sz;
		if (sz == 0) {
			int block = now / 8, pos = now % 8;
			nullIndicator[block] |= (1 << (7 - pos));
		}
		++now;
	}
	memcpy(data, nullIndicator, nullSize);
	delete[] nullIndicator;
}

void Record::decode2(vector<string> &attributeNames, void *data) {
	int n = attributeNames.size();
	int nullSize = (n + 7) / 8, offset = 0, tot = *(int*)(tmp);
	char* nullIndicator = new char[nullSize];
	memset(nullIndicator, 0, nullSize);
	for (int j = 0; j < attributeNames.size(); ++j)	 {
		for (int i = 0; i < recordDescriptor.size(); ++i) {
			if (recordDescriptor[i].name == attributeNames[j]) {
				if (i >= tot) {
					int block = j / 8, pos = j % 8;
					nullIndicator[block] |= (1 << (7 - pos)); //not exist this attribute
					break;
				}
				int pos1 = *(int*)(tmp + 4 * i), pos2 = *(int*)(tmp + 4 * i + 4);
				if (i == 0)	pos1 = (recordDescriptor.size() + 1) * 4;
				int sz = pos2 - pos1;
				memcpy((char*)data + nullSize + offset, tmp + pos1, sz);
				offset += sz;
				if (sz == 0) {
					int block = j / 8, pos = j % 8;
					nullIndicator[block] |= (1 << (7 - pos));
				}
				break;
			}
		}
	}
	memcpy(data, nullIndicator, nullSize);
	delete[] nullIndicator;
}

Record::Record(const Record &record2) {
	recordDescriptor = record2.recordDescriptor;
	length = record2.length;
	tmp = new char[length];
	memcpy(tmp, record2.tmp, length);
}

Record::~Record() {
	delete[] tmp;
}

Page::Page() {
	tmp = NULL;
	slotNum = 0;
	freeSpace = PAGE_SIZE - 8;
}

Page::~Page() {
	delete[] tmp;
}

Page::Page(char* buf) {
	tmp = new char[PAGE_SIZE];
	memset(tmp,0,PAGE_SIZE);
	memcpy(tmp, buf, PAGE_SIZE);
	char* now = tmp + PAGE_SIZE - 4;
	freeSpace = *(int*)(now);
	now -= 4;
	slotNum = *(int*)(now);
	for(int i = 0; i < slotNum; ++i) {
		now -= 4;
		slotOffset.pb(*(int*)(now));
		now -= 4;
		slotLength.pb(*(int*)(now));
	}
}

void Page::encode(char* data) {
	memcpy(tmp, data, PAGE_SIZE);
	char* now = tmp + PAGE_SIZE - 4;
	freeSpace = *(int*)(now);
	now -= 4;
	slotNum = *(int*)(now);
	while (slotOffset.size())	slotOffset.pop_back();
	while (slotLength.size())	slotLength.pop_back();
	for(int i = 0; i < slotNum; ++i) {
		now -= 4;
		slotOffset.pb(*(int*)(now));
		now -= 4;
		slotLength.pb(*(int*)(now));
	}
}

void Page::decode(char* data) {
	char* now = tmp + PAGE_SIZE - 4;
	*(int*)now = freeSpace;
	now -= 4;
	*(int*)now = slotNum;
	for (int i = 0; i < slotNum; ++i) {
		now -= 4;
		*(int*)now = slotOffset[i];
		now -= 4;
		*(int*)now = slotLength[i];
	}
	memcpy(data, tmp, PAGE_SIZE);
}

RC RBFM_ScanIterator::close() {
	delete[] iterator;
	fclose(fileHandle.file);
	return 0;
}

RBFM_ScanIterator::RBFM_ScanIterator() {
	iterator = NULL;
	compOp = EQ_OP;
	pre.pageNum = 2, pre.slotNum = -1;//hidden + head
}

RBFM_ScanIterator::~RBFM_ScanIterator() {

}

