#include "rbfm.h"

RecordBasedFileManager* RecordBasedFileManager::_rbf_manager = 0;

RecordBasedFileManager* RecordBasedFileManager::instance()
{
    if(!_rbf_manager)
        _rbf_manager = new RecordBasedFileManager();
    
    return _rbf_manager;
}

RecordBasedFileManager::RecordBasedFileManager()
{
    buf = new char[PAGE_SIZE];
}

RecordBasedFileManager::~RecordBasedFileManager()
{
    delete [] buf;
}

RC RecordBasedFileManager::createFile(const string &fileName) {
    return PagedFileManager::instance()->createFile(fileName);
}

RC RecordBasedFileManager::destroyFile(const string &fileName) {
    return PagedFileManager::instance()->destroyFile(fileName);
}

RC RecordBasedFileManager::openFile(const string &fileName, FileHandle &fileHandle) {
    return PagedFileManager::instance()->openFile(fileName, fileHandle);
}

RC RecordBasedFileManager::closeFile(FileHandle &fileHandle) {
    return PagedFileManager::instance()->closeFile(fileHandle);
}

bool RecordBasedFileManager::getNULLstatus(const void *data, const int pos) {
    Byte now = (char*)data + (pos/8);
    return ((*now) >> (7 - pos%8)) & 1;
}


RC RecordBasedFileManager::insertRecord(FileHandle &fileHandle, const vector<Attribute> &recordDescriptor, const void *data, RID &rid) {
    Record record(recordDescriptor, data);
    
    //read header
    int pageNum = fileHandle.getNumberOfPages();
    if(pageNum == 1) {   //this is hiddenpage
        memset(buf, 0, PAGE_SIZE);
        *(int*)buf = PAGE_SIZE-8;
        fileHandle.appendPage(buf);
        *(int*)buf = 0;
        Page tmp(buf);
        tmp.slotNum=0;
        tmp.freeSpace=PAGE_SIZE-8;
        tmp.decode(buf);
        fileHandle.appendPage(buf);
        pageNum += 2;
    }
    
    
    int storagePage = (pageNum-1 - 1)/1025*1025+1;
    int index = (pageNum-1 - 1)%1025;		//TODO : not -1?
    fileHandle.readPage(storagePage, buf);
    int target = -1;
    if(*(int*)(buf + index*4-4) < record.length+8) {    //last page can't fit it
        for(unsigned int i=1; i<index; ++i) {
            if(*(int*)(buf + index*4-4) >= record.length+8) {
                target = storagePage + i;
                break;
            }
        }
        if(target==-1) {
            for(unsigned int i=0; i<(pageNum-1 - 1)/1025; ++i) {
                storagePage = i*1025+1;
                fileHandle.readPage(storagePage, buf);
                for(int j=1; j<=1024; ++j)
                    if(*(int*)(buf + j*4-4) >= record.length+8) {
                        target = storagePage + j;
                        break;
                    }
                if(target!=-1) break;
            }
            if(target==-1) {
                
                //need append
                memset(buf, 0, PAGE_SIZE);
                if(index==1024) {  //append 2 pages
                    fileHandle.appendPage(buf);
                    pageNum++;
                }
                storagePage = (pageNum-1 - 1)/1025*1025 + 1;
                Page tmp(buf);
                tmp.slotNum=0;
                tmp.freeSpace=PAGE_SIZE-8;
                tmp.decode(buf);
                fileHandle.appendPage(buf);
                pageNum++;
                target = pageNum-1;
            }
        }
        
    } else target = pageNum-1;  //last page
    
    fileHandle.readPage(target, buf);
    Page page(buf);
    rid.pageNum = target;
    unsigned int i;
    for(i=0; i<page.slotNum; ++i)
        if(page.slotOffset[i]==0 && page.slotSize[i]==-1)
            break;
    rid.slotNum = i;//findSlot(page);
    
    int offset = PAGE_SIZE - page.freeSpace - (page.slotNum*2+2)*4;
    page.freeSpace -= record.length;
    
    if(i == page.slotNum) {
        page.slotNum++;
        page.freeSpace -= 8;
        page.slotOffset.push_back(offset);
        page.slotSize.push_back(record.length);
    } else {
        page.slotOffset[rid.slotNum]=offset;
        page.slotSize[rid.slotNum]=record.length;
    }

    
    memcpy(page.buf+offset, record.buf, record.length);
    page.decode(buf);
    fileHandle.writePage(target, buf);
    
    storagePage = (target-1 - 1)/1025*1025+1; //TODO: not -1?
    fileHandle.readPage(storagePage, buf);
    index = (target - 1)%1025;	
    *(int *)(buf + index*4-4) = page.freeSpace;
    fileHandle.writePage(storagePage, buf);
    

    //cout << "insert one in " << rid.pageNum << " " << rid.slotNum << endl;
    //printRecord(recordDescriptor, data);
    return SUCCESS;
}

RC RecordBasedFileManager::readRecord(FileHandle &fileHandle, const vector<Attribute> &recordDescriptor, const RID &rid, void *data) {
    fileHandle.readPage(rid.pageNum, buf);
    Page page(buf);
    RID now = rid;
    if(page.slotOffset[rid.slotNum] == 0 && page.slotSize[rid.slotNum] == -1) return -1;
    
    while(page.slotOffset[now.slotNum] < 0) {   //tomb stone   
        RID next;
        next.pageNum = -page.slotOffset[now.slotNum];
        next.slotNum = -page.slotSize[now.slotNum];
        fileHandle.readPage(next.pageNum, buf);
        now = next;
        page.encode(buf);
    }
    
    Record record(recordDescriptor, page.slotSize[now.slotNum], page.buf+page.slotOffset[now.slotNum]);
    record.decode(data);
    return SUCCESS;
}

RC RecordBasedFileManager::deleteRecord(FileHandle &fileHandle, const vector<Attribute> &recordDescriptor, const RID &rid) {
    fileHandle.readPage(rid.pageNum, buf);
    Page page(buf);
    RID now = rid;
    while(page.slotOffset[now.slotNum] < 0) {   //tomb stone
        RID next;
        next.pageNum = -page.slotOffset[now.slotNum];
        next.slotNum = -page.slotSize[now.slotNum];
        page.slotOffset[now.slotNum] = 0;
        page.slotSize[now.slotNum] = -1;
        page.decode(buf);
        fileHandle.writePage(now.pageNum, buf);
        fileHandle.readPage(next.pageNum, buf);
        now = next;
        page.encode(buf);
    }
    
    int offset = page.slotOffset[now.slotNum], size = page.slotSize[now.slotNum];
    page.slotOffset[now.slotNum] = 0;
    page.slotSize[now.slotNum] = -1;
    //[offset+size, PAGE_SIZE - page.freeSpace - (page.slotNum*2+2)*4]
    int len = PAGE_SIZE - page.freeSpace - (page.slotNum*2+2)*4 - offset - size;
    char* tmp = new char[len];
    memcpy(tmp, page.buf+offset+size, len);
    memcpy(page.buf+offset, tmp, len);
    for(unsigned int i=0; i<page.slotNum; ++i)
        if(page.slotOffset[i] > offset)
            page.slotOffset[i] -= size;
    page.freeSpace += size;
    page.decode(buf);
    fileHandle.writePage(now.pageNum, buf);
    
    int storagePage = (now.pageNum-1 - 1)/1025*1025+1;
    fileHandle.readPage(storagePage, buf);
    int index = (now.pageNum - 1)%1025;
    *(int *)(buf + index*4-4) = page.freeSpace;
    fileHandle.writePage(storagePage, buf);
    
    delete [] tmp;
    return SUCCESS;
}

RC RecordBasedFileManager::updateRecord(FileHandle &fileHandle, const vector<Attribute> &recordDescriptor, const void* data, const RID &rid) {
    deleteRecord(fileHandle, recordDescriptor, rid);
    
    Record record(recordDescriptor, data);
    
    fileHandle.readPage(rid.pageNum, buf);
    Page page(buf);
    if(page.freeSpace >= record.length) {
        int offset = PAGE_SIZE - page.freeSpace - (page.slotNum*2+2)*4;
        page.freeSpace -= record.length;
        page.slotOffset[rid.slotNum] = offset;
        page.slotSize[rid.slotNum] = record.length;
        
        memcpy(page.buf+offset, record.buf, record.length);
        page.decode(buf);
        fileHandle.writePage(rid.pageNum, buf);
        
        int storagePage = (rid.pageNum-1 - 1)/1025*1025+1;
        fileHandle.readPage(storagePage, buf);
        int index = (rid.pageNum - 1)%1025;
        *(int *)(buf + index*4-4) = page.freeSpace;
        fileHandle.writePage(storagePage, buf);
        
    } else {
        RID next;
        insertRecord(fileHandle, recordDescriptor, data, next);
        page.slotOffset[rid.slotNum] = -next.pageNum;
        page.slotSize[rid.slotNum] = -next.slotNum;
        page.decode(buf);
        fileHandle.writePage(rid.pageNum, buf);
    }
    
    return SUCCESS;
}

RC RecordBasedFileManager::readAttribute(FileHandle &fileHandle, const vector<Attribute> &recordDescriptor, const RID &rid, const string &attributeName, void *data) {
    fileHandle.readPage(rid.pageNum, buf);
    Page page(buf);
    RID now = rid;
    while(page.slotOffset[now.slotNum] < 0) {   //tomb stone
        RID next;
        next.pageNum = -page.slotOffset[now.slotNum];
        next.slotNum = -page.slotSize[now.slotNum];
        fileHandle.readPage(next.pageNum, buf);
        now = next;
        page.encode(buf);
    }
    
    Record record(recordDescriptor, page.slotSize[now.slotNum], page.buf + page.slotOffset[now.slotNum]);
    Byte tmp = new char[record.length];
    int n = 0;
	for(int i=0; i<recordDescriptor.size(); ++i)
		if(recordDescriptor[i].valid)
			n++;
	int NULLbyte = (n+7) / 8;
    record.decode(tmp);
    printRecord(recordDescriptor, tmp);
    for(int i=0; i<recordDescriptor.size(); ++i)
        if(recordDescriptor[i].name == attributeName) {
            int pos = *(int*)(record.buf+i*4), next = *(int*)(record.buf+(i+1)*4);
            if(i == 0) pos = (recordDescriptor.size()+1)*4;
            memcpy((char*)data+NULLbyte, record.buf+pos, next-pos);
            break;
        }
    memcpy(data, tmp, NULLbyte);
    delete [] tmp;
    
    return SUCCESS;
}


RC RecordBasedFileManager::printRecord(const vector<Attribute> &recordDescriptor, const void *data) {
    int n = 0;
	for(int i=0; i<recordDescriptor.size(); ++i)
		if(recordDescriptor[i].valid)
			n++;
	int NULLbyte = (n+7) / 8;
    Byte p = (char*)data + NULLbyte;
    int t=0;
    for(unsigned int i=0; i<recordDescriptor.size(); ++i) {
        Attribute x = recordDescriptor[i];
        if(!x.valid) continue;
        cout << x.name << ": ";
        if(getNULLstatus(data, t++)) {
            cout << "NULL ";
            continue;
        }
        switch(x.type) {
            case TypeInt:
                cout << *(int*)p << " "; p += 4; break;
            case TypeReal:
                cout << *(float*)p << " ";p += 4; break;
            case TypeVarChar:
                int len = *(int*)p;
                p += 4;
                for(int j=0; j<len; ++j)
                    cout << (char)*p++;
                cout << " ";
        }
    }
    cout << endl;
    return SUCCESS;
}

RC RecordBasedFileManager::scan(FileHandle &fileHandle, const vector<Attribute> &recordDescriptor, const string &conditionAttribute,
	      const CompOp compOp, const void *value, const vector<string> &attributeNames, RBFM_ScanIterator &rbfm_ScanIterator) {
	rbfm_ScanIterator.fileHandle = fileHandle;
	rbfm_ScanIterator.compOp = compOp;
	rbfm_ScanIterator.attributeNames = attributeNames;
	rbfm_ScanIterator.recordDescriptor = recordDescriptor;
	rbfm_ScanIterator.conditionAttribute = conditionAttribute;
	int size=0, i;
	for(i=0; i<recordDescriptor.size(); ++i)
		if(recordDescriptor[i].name == conditionAttribute) {
			if(recordDescriptor[i].type != TypeVarChar) {
				size = 4;
			} else {
				size = 4;
				size += *(int*)value;
			}
			break;
		}
	rbfm_ScanIterator.iteratorBuf = new char[size];
	memcpy(rbfm_ScanIterator.iteratorBuf, value, size);
	return SUCCESS;
}

RC RBFM_ScanIterator::getNextRecord(RID &rid, void *data) {
	Byte buf = new char[PAGE_SIZE];
	while(1) {
		if(last.pageNum >= fileHandle.getNumberOfPages()) {
			delete[] buf;
			return -1;
		}
		last.slotNum++;

		//cout << "in scan" << last.pageNum << " " << last.slotNum << endl;
		fileHandle.readPage(last.pageNum, buf);
		Page page(buf);
		if(page.slotNum <= last.slotNum) {
			if(last.pageNum % 1025 == 1024) last.pageNum++;
			last.pageNum++;
			last.slotNum = -1;
			continue;
		}
		if(page.slotOffset[last.slotNum] == 0 && page.slotSize[last.slotNum] == -1) continue;
		if(page.slotOffset[last.slotNum] < 0) continue;
		Record record(recordDescriptor, page.slotSize[last.slotNum], page.buf + page.slotOffset[last.slotNum]);
		if(record.compared(conditionAttribute, compOp, iteratorBuf)) {
			rid = last;
			record.decode2(data, attributeNames);
			delete[] buf;
			return SUCCESS;
		}
	}
	return SUCCESS;
}

RC RBFM_ScanIterator::close() {
	delete [] iteratorBuf;
	fclose(fileHandle.f);
	return SUCCESS;
}

RBFM_ScanIterator::RBFM_ScanIterator(){
	iteratorBuf = NULL;
	compOp = EQ_OP;
	last.pageNum = 2; last.slotNum=-1;
}

Record::Record(const vector<Attribute> &_recordDescriptor, const void *data) {
    recordDescriptor = _recordDescriptor;

    int n = 0;
    for(int i=0; i<recordDescriptor.size(); ++i)
    		if(recordDescriptor[i].valid)
    			n++;
    	int NULLbyte = (n+7) / 8;
    int offset = 0;
    Byte p = (char*)data + NULLbyte;
    vector<int> pos;
    for(unsigned int i=0; i<recordDescriptor.size(); ++i) {
        Attribute x = recordDescriptor[i];
        if(!x.valid || RecordBasedFileManager::getNULLstatus(data, i)) {
            pos.push_back(offset+(recordDescriptor.size()+1)*4);
            continue;
        }
        switch(x.type) {
            case TypeInt:
                offset += 4; break;
            case TypeReal:
                offset += 4; break;
            case TypeVarChar:
                int len = *(int*)(p+offset);
                offset += 4 + len;
        }
        pos.push_back(offset+(recordDescriptor.size()+1)*4);
    }
    length = (recordDescriptor.size()+1)*4 + offset;
    buf = new char[length];
    *(int*)buf = recordDescriptor.size();
    for(int i=0; i<recordDescriptor.size(); ++i)
        *(int*)(buf+(i+1)*4) = pos[i];
    memcpy(buf+(recordDescriptor.size()+1)*4, p, offset);
}

Record::Record(const vector<Attribute> &_recordDescriptor, int size, const void *data) {
    recordDescriptor = _recordDescriptor;
    length = size;
    buf = new char[length];
    memcpy(buf, data, size);
}

void Record::decode(void *data) {
	 int n = 0;
	 for(int i=0; i<recordDescriptor.size(); ++i)
		if(recordDescriptor[i].valid)
			n++;
    vector<int> pos;
    int NULLbyte = (n+7) / 8;
    Byte NULLbit = new char[NULLbyte];
    memset(NULLbit, 0, NULLbyte);
    pos.push_back((recordDescriptor.size()+1)*4);
    for(unsigned int i=0; i<recordDescriptor.size(); ++i)
        pos.push_back(*(int*)(buf+(i+1)*4));
    pos.push_back(length);



    int offset = 0;
    int t=0;
    for(unsigned int i=0; i<recordDescriptor.size(); ++i) {
    		if(!recordDescriptor[i].valid) continue;
    		int np = pos[i], next = pos[i+1];
    		memcpy((char*)data+NULLbyte+offset, buf+np, next-np);
    		offset += next-np;
    		if(np==next)
    		     *(NULLbit + t/8) |= (1 << (7 - t%8));
    		t++;
    }
    memcpy(data, NULLbit, NULLbyte);
    delete[] NULLbit;

    //memcpy((char*)data+NULLbyte, buf+(n+1)*4, length-(n+1)*4);
}

void Record::decode2(void *data, vector<string> &attributeNames) {
	int n = attributeNames.size(), NULLbyte = (n+7) / 8;
	Byte NULLbit = new char[NULLbyte];
	memset(NULLbit, 0, NULLbyte);
	int offset = 0;
	for(int i=0; i<attributeNames.size(); ++i)
		for(int j=0; j<recordDescriptor.size(); ++j)
			if(attributeNames[i] == recordDescriptor[j].name) {
				int pos = *(int*)(buf+j*4), next = *(int*)(buf+(j+1)*4);
				if(j == 0) pos = (recordDescriptor.size()+1)*4;
				memcpy((char*)data+NULLbyte+offset, buf+pos, next-pos);
				offset += next-pos;
				if(next==pos)
					*(NULLbit + i/8) |= (1 << (7 - i%8));
				break;
			}
	memcpy(data, NULLbit, NULLbyte);
	delete[] NULLbit;

}

bool Record::compared(string &conditionAttribute, const CompOp compOp, const void *value) {
	if(compOp == NO_OP) return 1;
	//int n =
	for(int i=0; i<recordDescriptor.size(); ++i)
		if(recordDescriptor[i].name == conditionAttribute) {
			int pos = *(int*)(buf+i*4), next = *(int*)(buf+(i+1)*4);
			if(i == 0) pos = (recordDescriptor.size()+1)*4;
			if(next-pos == 0) return 0;
			//memcpy((char*)data+NULLbyte+offset, buf+pos, next-pos);

			if(recordDescriptor[i].type == TypeInt) {
				int t1 = *(int*)(buf+pos), t2 = *(int*) value;
				switch(compOp) {
				case EQ_OP: return t1 == t2;
				case LT_OP: return t1 < t2;
				case LE_OP: return t1 <= t2;
				case GT_OP: return t1 > t2;
				case GE_OP: return t1 >= t2;
				case NE_OP: return t1 != t2;
				default : return 0;
				}
			} else if(recordDescriptor[i].type == TypeReal) {
				float t1 = *(float*)(buf+pos), t2 = *(float*) value;
				switch(compOp) {
				case EQ_OP: return t1 == t2;
				case LT_OP: return t1 < t2;
				case LE_OP: return t1 <= t2;
				case GT_OP: return t1 > t2;
				case GE_OP: return t1 >= t2;
				case NE_OP: return t1 != t2;
				default : return 0;
				}
			} else {
				string s1, s2;
				int l1, l2;
				l1 = *(int*)(buf+pos), l2 = *(int*)value;
				for(int j=0; j<l1; ++j) s1 += *(buf+pos+4+j);
				for(int j=0; j<l2; ++j) s2 += *((char*)value+4+j);
				switch(compOp) {
				case EQ_OP: return s1 == s2;
				case LT_OP: return s1 < s2;
				case LE_OP: return s1 <= s2;
				case GT_OP: return s1 > s2;
				case GE_OP: return s1 >= s2;
				case NE_OP: return s1 != s2;
				default : return 0;
				}
			}
			break;
		}

	return 0;
}


Record::~Record() {
    delete [] buf;
}

Page::Page() {
    buf = NULL;
    slotNum = 0;
    freeSpace = PAGE_SIZE - 8;
}

Page::Page(Byte _buf) {
    buf = new char[PAGE_SIZE];
    memcpy(buf, _buf, PAGE_SIZE);
    slotNum = *(int*)(buf+PAGE_SIZE-4);
    freeSpace = *(int*)(buf+PAGE_SIZE-8);
    for(int i=0; i<slotNum; ++i) {
        slotOffset.push_back(*(int*)(buf+PAGE_SIZE-8 - 8*i-4));
        slotSize.push_back(*(int*)(buf+PAGE_SIZE-8 - 8*i-8));
    }
}

Page::~Page() {
    delete [] buf;
}

void Page::encode(void *data) {
    memcpy(buf, data, PAGE_SIZE);
    slotNum = *(int*)(buf+PAGE_SIZE-4);
    freeSpace = *(int*)(buf+PAGE_SIZE-8);
    while(slotOffset.size()) slotOffset.pop_back();
    while(slotSize.size()) slotSize.pop_back();
    for(int i=0; i<slotNum; ++i) {
        slotOffset.push_back(*(int*)(buf+PAGE_SIZE-8 - 8*i-4));
        slotSize.push_back(*(int*)(buf+PAGE_SIZE-8 - 8*i-8));
    }
}

void Page::decode(void *data) {
    *(int*)(buf+PAGE_SIZE-4) = slotNum;
    *(int*)(buf+PAGE_SIZE-8) = freeSpace;
    for(int i=0; i<slotNum; ++i) {
        *(int*)(buf+PAGE_SIZE-8 - 8*i-4) = slotOffset[i];
        *(int*)(buf+PAGE_SIZE-8 - 8*i-8) = slotSize[i];
    }
    memcpy(data, buf, PAGE_SIZE);
}


