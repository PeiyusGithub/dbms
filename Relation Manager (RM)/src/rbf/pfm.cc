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

void initPage(FileHandle &fileHandle) {
	FILE* file = fileHandle.file;
	int t = fseek(file, 0, SEEK_END);
	if (t)	return;
    if (ftell(file) / PAGE_SIZE != 0) {
    	fseek(file, 0, SEEK_SET);
    	int a[4];
    	fread(&a[0], sizeof(int), 4, file);
    	fileHandle.totPageCounter = a[0], fileHandle.readPageCounter = a[1], fileHandle.writePageCounter = a[2], fileHandle.appendPageCounter = a[3];
    	return;
    }
    //create;
    fseek(file, 0, SEEK_SET);
	void* data = malloc(sizeof(PAGE_SIZE)+8);
    fwrite(data, PAGE_SIZE, 1, file);
	int a[4] = {1, 0, 0, 0};
	fseek(file, 0, SEEK_SET);
	fwrite(&a[0], sizeof(int), 4, file);
	free(data);
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
	fileHandle.writeCounterInFile();
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
    int t = fseek(file, pageNum * PAGE_SIZE, SEEK_SET);
    if (t)	return -1;
    size_t sz = fread(data, PAGE_SIZE, 1, file);
    if (sz != 1)	return -1;
    ++readPageCounter;
	return 0;
}

RC FileHandle::writePage(PageNum pageNum, const void *data) {
    if (pageNum >= getNumberOfPages())	return -1;// 0-base
    int t = fseek(file, pageNum * PAGE_SIZE, SEEK_SET);
    if (t)	return -1;
    size_t sz = fwrite(data, PAGE_SIZE, 1, file);
    if (sz != 1)	return -1;
    ++writePageCounter;
    return 0;
}

RC FileHandle::appendPage(const void *data) {
	int t = fseek(file, 0, SEEK_END);
	if (t)	return -1;
	size_t sz = fwrite(data, PAGE_SIZE, 1, file);
	if (sz != 1)	return -1;
	++appendPageCounter;
	++totPageCounter;
    return 0;
}

int FileHandle::getNumberOfPages() {

	//cout << t << endl;
	if (fseek(file, 0, SEEK_END))	return -1;
    return (ftell(file) / PAGE_SIZE);
}

RC FileHandle::collectCounterValues(unsigned &readPageCount, unsigned &writePageCount, unsigned &appendPageCount) {
	readPageCount = readPageCounter;
	writePageCount = writePageCounter;
	appendPageCount = appendPageCounter;
    return 0;
}

void FileHandle::writeCounterInFile() {
	size_t sz = sizeof(int);
	int a[4] = {totPageCounter, readPageCounter, writePageCounter, appendPageCounter};
	fseek(file, 0, SEEK_SET);
	fwrite(&a[0], sz, 4, file);
}
