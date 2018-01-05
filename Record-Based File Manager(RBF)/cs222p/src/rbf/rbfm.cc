#include "rbfm.h"

RecordBasedFileManager* RecordBasedFileManager::_rbf_manager = 0;

RecordBasedFileManager* RecordBasedFileManager::instance() {
    if(!_rbf_manager)
        _rbf_manager = new RecordBasedFileManager();
    return _rbf_manager;
}

RecordBasedFileManager::RecordBasedFileManager() {
	pageFreeSpace.resize(0);
	pageFreeSpace.clear();
	tmp = new char[PAGE_SIZE];
}

RecordBasedFileManager::~RecordBasedFileManager() {
	delete tmp;
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
    const char* s = fileName.c_str();
	return pfm -> openFile(s, fileHandle);
}

RC RecordBasedFileManager::closeFile(FileHandle &fileHandle) {
    PagedFileManager* pfm = PagedFileManager::instance();
    return pfm -> closeFile(fileHandle);
}

RC RecordBasedFileManager::insertRecord(FileHandle &fileHandle, const vector<Attribute> &recordDescriptor, const void *data, RID &rid) {
	int fieldCount = recordDescriptor.size();
	int nullSize = (fieldCount + 7) / 8;
	char* nullIndicator = new char[nullSize];
	memcpy(nullIndicator, data, nullSize);
	if (!nullIndicator)    return -1;
	int sz = (fieldCount + 1) * 4;	//num + n offsets
	char* now = (char*)data + nullSize;
	for (int i = 0; i < recordDescriptor.size(); ++i) {
		int block = i / 8, pos = i % 8;
		if (nullIndicator[block] & (1 << (7 - pos)))	continue;
		Attribute t = recordDescriptor[i];
		switch(t.type) {
			case TypeInt:
			case TypeReal:
				sz += 4;
				now += 4;
				break;
			case TypeVarChar:
				int len = *((int*)now);
				sz += len + 4;
				now += len + 4;
				break;
		}
	}
	int n = fileHandle.getNumberOfPages();
	Page page;
	int pageNum, slotNum;
	if (n == 0) {
		memset(tmp, 0, PAGE_SIZE);		//new page
		fileHandle.appendPage(tmp);
		page = Page(tmp);
		page.slotNum = 0, page.freeSpace = PAGE_SIZE - 8; // N F
		pageNum = 0;
		pageFreeSpace.pb(page.freeSpace);
	}
	else {
		bool ok = false;
		for (int i = 0; i < n; ++i) {
			if (pageFreeSpace[i] - 8 < sz)	continue;
			else {
				ok = true;
				pageNum = i;
				break;
			}
		}
		if (!ok) {
			memset(tmp, 0, PAGE_SIZE);
			fileHandle.appendPage(tmp);
			page = Page(tmp);
			page.slotNum = 0, page.freeSpace = PAGE_SIZE - 8; // N F
			pageNum = n;
			pageFreeSpace.pb(page.freeSpace);
		}
		else {
			fileHandle.readPage(pageNum, tmp);
			page = Page(tmp);
		}
	}
	rid.pageNum = pageNum;
	slotNum = -1;
	for (int i = 0; i < page.slotNum; ++i)
		if (page.slotOffset[i] < 0) {
			slotNum = i;
			break;
		}
	if (slotNum == -1)	slotNum = page.slotNum;
	rid.slotNum = slotNum;
	int offset = PAGE_SIZE - page.freeSpace - page.slotNum * 8 - 8;//TODO times 8?
	page.freeSpace -= sz;
	if (rid.slotNum == page.slotNum) {
		++page.slotNum;
		page.slotOffset.pb(offset);
		page.slotLength.pb(sz);
	}
	else {
		page.slotOffset[rid.slotNum] = offset;
		page.slotLength[rid.slotNum] = sz;
	}
	memcpy(page.tmp + offset, data, sz);
	page.decode(tmp);
	page = Page(tmp);
	pageFreeSpace[pageNum] = page.freeSpace;
	fileHandle.writePage(rid.pageNum, tmp);
	delete nullIndicator;
	return 0;
}

RC RecordBasedFileManager::readRecord(FileHandle &fileHandle, const vector<Attribute> &recordDescriptor, const RID &rid, void *data) {
    fileHandle.readPage(rid.pageNum, tmp);
    Page page = Page(tmp);
    Record record(recordDescriptor, page.tmp + page.slotOffset[rid.slotNum]);
    record.decode(data);
	return 0;
}

RC RecordBasedFileManager::printRecord(const vector<Attribute> &recordDescriptor, const void *data) {
	int fieldCount = recordDescriptor.size();
	int nullSize = ceil((double)fieldCount / 8);
	char* nullIndicator = new char[nullSize];
	memcpy(nullIndicator, data, nullSize);
	char* now = (char*)data + nullSize;
	for (int i = 0; i < recordDescriptor.size(); ++i) {
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
	delete nullIndicator;
	return 0;
}

Record::Record(const vector<Attribute> &recordDescriptorTmp, void *data) {
	recordDescriptor = recordDescriptorTmp;
	int fieldCount = recordDescriptor.size();
	int nullSize = ceil((double)fieldCount / 8);
	char* nullIndicator = new char[nullSize];
	memcpy(nullIndicator, data, nullSize);
	int offset = (fieldCount + 1) * 4;
	char* now = (char*)data + nullSize;
	vector<int> p;
	p.clear();
	for (int i = 0; i < fieldCount; ++i) {
		int block = i / 8, pos = i % 8;
		if (nullIndicator[block] & (1 << (7 - pos))) {
			p.pb(offset);
			continue;
		}
		Attribute t = recordDescriptor[i];
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
	delete nullIndicator;
}

void Record::decode(void *data) {
	int n = *(int*)tmp;
	vector<int> p;
	int nullSize = (n + 7) / 8;
	char* nullIndicator = new char[n];
	memset(nullIndicator, 0, nullSize);
	p.pb((n + 1) * 4);
	for (int i = 0; i < n; ++i) {
		char* now = tmp + (i + 1) * 4;	//offset
		p.pb(*(int*)now);
	}
	p.pb(length);
	for (int i = 0; i < n; ++i) {
		if (p[i] == p[i + 1]) {
			int block = i / 8, pos = i % 8;
			nullIndicator[block] |= (1 << (7 - pos));
		}
	}
	char* now = (char*)data;
	memcpy(now, nullIndicator, nullSize);
	memcpy(now + nullSize, tmp + (n + 1) * 4, length - (n + 1) * 4);
	delete nullIndicator;
}

Record::~Record() {
	free(tmp);
}

Page::Page() {
	tmp = NULL;
	slotNum = 0;
	freeSpace = PAGE_SIZE - 8;
}

Page::Page(char* buf) {
	tmp = new char[PAGE_SIZE];
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

Page::~Page() {

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
