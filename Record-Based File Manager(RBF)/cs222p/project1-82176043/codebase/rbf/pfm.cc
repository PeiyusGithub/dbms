#include "pfm.h"

PagedFileManager* PagedFileManager::_pf_manager = 0;

PagedFileManager* PagedFileManager::instance() {
    if(!_pf_manager)
        _pf_manager = new PagedFileManager();

    return _pf_manager;
}

PagedFileManager::PagedFileManager() {

}

PagedFileManager::~PagedFileManager() {
}

void initPage(FileHandle& fileHandle) {
    fseek(fileHandle.file, 0, SEEK_SET);
    const char *s = " ";
    fwrite(&s, 1, PAGE_SIZE, fileHandle.file);
}

RC PagedFileManager::createFile(const string &fileName) {
	const char* s = fileName.c_str();
    FILE* file = fopen(s, "rb");
    if (file) {
    	fclose(file);
    	return -1;
    }
    file = fopen(s, "wb");
    if (!file)	return -1;

	fclose(file);
	return 0;
}

RC PagedFileManager::destroyFile(const string &fileName) {
	const char* s = fileName.c_str();
    FILE* file = fopen(s, "rb");
    if (!file)	return -1;
    fclose(file);
    int t = remove(s);
    if (t)	return -1;
	return 0;
}

RC PagedFileManager::openFile(const string &fileName, FileHandle &fileHandle) {
	const char* s = fileName.c_str();
    FILE* file = fopen(s, "rb+");
    if (!file)	return -1;
    if (fileHandle.file)	return -1;
    fileHandle.file = file;
	initPage(fileHandle);
    return 0;
}

RC PagedFileManager::closeFile(FileHandle &fileHandle) {
	if (!fileHandle.file)	return -1;
	int t = fclose(fileHandle.file);
	if (t)	return -1;
	return 0;
}

FileHandle::FileHandle() {
    readPageCounter = 0;
    writePageCounter = 0;
    appendPageCounter = 0;
    totPageCounter = 0;
    file = NULL;
}

FileHandle::~FileHandle() {
}


RC FileHandle::readPage(PageNum pageNum, void *data) {
    if (pageNum >= getNumberOfPages())	return -1;// 0-base
    int t = fseek(file, (pageNum + 1) * PAGE_SIZE, SEEK_SET);
    if (t)	return -1;
    size_t sz = fread(data, PAGE_SIZE, 1, file);
    if (sz != 1)	return -1;
    ++readPageCounter;
    writeCounterInFile();
	return 0;
}

RC FileHandle::writePage(PageNum pageNum, const void *data) {
    if (pageNum >= getNumberOfPages())	return -1;// 0-base
    int t = fseek(file, (pageNum + 1) * PAGE_SIZE, SEEK_SET);
    if (t)	return -1;
    size_t sz = fwrite(data, PAGE_SIZE, 1, file);
    if (sz != 1)	return -1;
    ++writePageCounter;
    writeCounterInFile();
    return 0;
}

RC FileHandle::appendPage(const void *data) {
	int t = fseek(file, 0, SEEK_END);
	if (t)	return -1;
	size_t sz = fwrite(data, PAGE_SIZE, 1, file);
	if (sz != 1)	return -1;
	++appendPageCounter;
	++totPageCounter;
	writeCounterInFile();
    return 0;
}

unsigned FileHandle::getNumberOfPages() {
	int t = fseek(file, 0, SEEK_END);
	if (t)	return -1;
    return (ftell(file) / PAGE_SIZE) - 1;
}

RC FileHandle::collectCounterValues(unsigned &readPageCount, unsigned &writePageCount, unsigned &appendPageCount) {
	readPageCount = readPageCounter;
	writePageCount = writePageCounter;
	appendPageCount = appendPageCounter;
//	cout << "After AppendPage R:" << readPageCount << " W:" << writePageCount << " A:" << appendPageCount << endl;
    return 0;
}

void FileHandle::writeCounterInFile() {
	size_t sz = sizeof(int);
	unsigned a[4] = {totPageCounter, readPageCounter, writePageCounter, appendPageCounter};
	//for (int i = 0; i < 4; ++i)	cout << a[i] << endl;
	fseek(file, 0, SEEK_SET);
	fwrite(a, sz, 4, file);
}
