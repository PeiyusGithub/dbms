#include "ix.h"
//ian start to play this game :)
IndexManager* IndexManager::_index_manager = 0;
IndexManager* IndexManager::instance()
{
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
    float floatKey = 0.0;
    int varLen = 0;
    string varKey = "";
    if (type == TypeInt)    intKey = *(int*)key;
    else if (type == TypeReal)  floatKey = *(char*)key;
    else {
        varLen = *(int*)key;
        for (int i = 0; i < varLen; ++i) varKey += *(char*)(key + 4 + i);
    }
    ixfileHandle.fileHandle.readPage(now, tmp);
    ++ixfileHandle.ixReadPageCounter;
    vector<int> rootIndexPage, rootPos;
    IndexPage indexPage;
    LeafPage leafPage;
    bool isLeaf = ((*(int*)tmp) == 0);
    while (!isLeaf) {
        indexPage.encode(tmp, type);
    }
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
    return -1;
}

void IndexManager::printBtree(IXFileHandle &ixfileHandle, const Attribute &attribute) const {
}

IX_ScanIterator::IX_ScanIterator()
{
}

IX_ScanIterator::~IX_ScanIterator()
{
}

RC IX_ScanIterator::getNextEntry(RID &rid, void *key)
{
    return -1;
}

RC IX_ScanIterator::close()
{
    return -1;
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
            Real.pb(*(int*)((char*)data + offset));
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

void IndexPage::encode(void *data, int type) {

}

LeafPage ::LeafPage() {
    n = nxt = keyType = 0;
    len = 12;
    Int.clear(), Real.clear(), Varchar.clear();
}
RC IXFileHandle::collectCounterValues(unsigned &readPageCount, unsigned &writePageCount, unsigned &appendPageCount) {
    ixReadPageCounter = readPageCount;
    ixWritePageCounter = writePageCount;
    ixAppendPageCounter = appendPageCount;
    return 0;
}

