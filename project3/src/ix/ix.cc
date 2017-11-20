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
    int now = *(int*)tmp;
    if (now == 0) {
        now = 1;
        changeHiddenPage(tmp, now, type);
        ixfileHandle.fileHandle.writePage(0, tmp);
        ++ixfileHandle.ixWritePageCounter;
        //leftpage
        memset(tmp, 0, PAGE_SIZE);
        *(tmp + 8) = -1;//no next
        ixfileHandle.fileHandle.appendPage(tmp);
        ++ixfileHandle.ixAppendPageCounter;
    }
    int intKey = 0;
    float realKey = 0.0;
    int varLen = 0;
    string varKey = "";
    if (type == TypeInt)    intKey = *(int*)key;
    else if (type == TypeReal)  realKey = *(float*)key;
    else {
        varLen = *(int*)key;
        for (int i = 0; i < varLen; ++i) varKey += *((char*)key + 4 + i);
    }
    ixfileHandle.fileHandle.readPage(now, tmp);
    ++ixfileHandle.ixReadPageCounter;
    vector<int> rootIndexPage, rootPos;
    IndexPage indexPage;
    LeafPage leafPage;
    bool isLeaf = ((*(int*)tmp) == 0);
    //TODO binary search
    while (!isLeaf) {
        indexPage.encode(tmp, type);
        int p = 0;
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
    return 0;
}

RC IndexManager::deleteEntry(IXFileHandle &ixfileHandle, const Attribute &attribute, const void *key, const RID &rid)
{
    return -1;
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
    if (!ixfileHandle.fileHandle.open) return -1;


    int type = IndexManager :: getKeyType(ixfileHandle);
    int pageNum = IndexManager :: getRootPage(ixfileHandle);
    ix_ScanIterator.type = type;
    //initialize iterator
    ix_ScanIterator.ixFileHandle = ixfileHandle;
    ix_ScanIterator.highKeyInclu = highKeyInclusive;
    ix_ScanIterator.lowKeyInclu = lowKeyInclusive;

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
        } else if (type = TypeReal) {
            if (lowKeyInclusive && leafPage.Real[i] >= ix_ScanIterator.lowKeyReal) {
                ix_ScanIterator.position = i;
                return 0;
            }

            if (!lowKeyInclusive & leafPage.Real[i] >= ix_ScanIterator.lowKeyReal) {
                ix_ScanIterator.position = i;
                return 0;
            }
        } else {
            if (lowKeyInclusive && leafPage.Varchar[i] >= ix_ScanIterator.lowKeyVar) {
                ix_ScanIterator.position = i;
                return 0;
            }

            if (lowKeyInclusive && leafPage.Varchar[i] >= ix_ScanIterator.lowKeyVar) {
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
    highKellNull = true;
    lowKeyNull = true;
    highKeyInclu = true;
    lowKeyInclu = true;
    lowKeyVar = "";
    highKeyVar = "";
}

IX_ScanIterator::IX_ScanIterator() {
    tmp = new char[PAGE_SIZE];
    highKellNull = true;
    lowKeyNull = true;
    highKeyInclu = true;
    lowKeyInclu = true;
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
            for (int j = 0; j < l; ++j) now += (char*)(data + offset + j);
            offset += l;
            Varchar.pb(now);
            Right.pb(*(int*)((char*)data + offset));
            offset += 4;
        }
    }
    len = offset;
}

void IndexPage::decode(void *data) {
    *(char*)data = 1;//index
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
            *(int*)((char*)data + offset) = Real[i];
            offset += 4;
            *(int*)((char*)data + offset) = Right[i];
            offset += 4;
        }
        else {
            int l = Varchar.size();
            *(int*)((char*)data + offset) = l;
            offset += 4;
            for (int j = 0; j < l; ++j) *((char*)data + offset + j) = Varchar[i][j];
            offset += l;
            *(int*)((char*)data + offset) = Right[i];
            offset += 4;
        }
    }
}

void LeafPage::encode(void *data, int type) {
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
    *(char*)data = 1;//leaf
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
            *(int*)((char*)data + offset) = Varchar.size();
            offset += 4;
            for (int j = 0; j < Varchar.size(); ++j)    *((char*)data + offset + j) = Varchar[i][j];
            offset += Varchar.size();
            *(int*)((char*)data + offset) = Rid[i].pageNum;
            offset += 4;
            *(int*)((char*)data + offset) = Rid[i].slotNum;
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

LeafPage ::LeafPage() {
    n = nxt = keyType = 0;
    len = 12;
    Int.clear(), Real.clear(), Varchar.clear();
}

RC IXFileHandle::collectCounterValues(unsigned &readPageCount, unsigned &writePageCount, unsigned &appendPageCount) {
    readPageCount = ixReadPageCounter;
    writePageCount = ixWritePageCounter;
    appendPageCount = ixAppendPageCounter;
    return 0;
}

