#ifndef _rbfm_h_
#define _rbfm_h_

#include <string>
#include <vector>
#include <climits>
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include "../rbf/pfm.h"

using namespace std;
#define pb push_back
// Record ID
typedef struct
{
  int pageNum;    // page number
  int slotNum;    // slot number in the page
} RID;


// Attribute
typedef enum { TypeInt = 0, TypeReal, TypeVarChar, Typebool} AttrType;

typedef unsigned AttrLength;

struct Attribute {
    string   name;     // attribute name
    AttrType type;     // attribute type
    AttrLength length; // attribute length
	int valid;
};

// Comparison Operator (NOT needed for part 1 of the project)
typedef enum { EQ_OP = 0, // no condition// = 
           LT_OP,      // <
           LE_OP,      // <=
           GT_OP,      // >
           GE_OP,      // >=
           NE_OP,      // !=
           NO_OP       // no condition
} CompOp;

class Record{
public:
	vector<Attribute> recordDescriptor;
	char* tmp;
	int length;
	Record(const vector<Attribute> &recordDescriptorTmp, const void *data);
	Record(const vector<Attribute> &recordDescriptorTmp, int size, const void *data);
	Record(const Record& record2);
	~Record();
	bool compared(string &conditionAttribute, const CompOp compOp, const void *value);
	void decode(void *data);
	void decode2(vector<string> &attributeNames, void *data);
	bool cmp(string &conditionAttribute, const CompOp compOp, const void *value);
};

class Page{
public:
	vector<int> slotLength, slotOffset;
	char* tmp;
	unsigned int slotNum, freeSpace;
	void encode(char *data);
	void decode(char *data);
	Page(char* tmp);
	Page();
	~Page();
};
/********************************************************************************
The scan iterator is NOT required to be implemented for the part 1 of the project 
********************************************************************************/

# define RBFM_EOF (-1)  // end of a scan operator

// RBFM_ScanIterator is an iterator to go through records
// The way to use it is like the following:
//  RBFM_ScanIterator rbfmScanIterator;
//  rbfm.open(..., rbfmScanIterator);
//  while (rbfmScanIterator(rid, data) != RBFM_EOF) {
//    process the data;
//  }
//  rbfmScanIterator.close();

class RBFM_ScanIterator {
public:
  FileHandle fileHandle;
  vector<Attribute> recordDescriptor;
  string conditionAttribute;
  CompOp compOp;
  vector<string> attributeNames;
  char* iterator;
  RID pre;
  RBFM_ScanIterator();
  ~RBFM_ScanIterator();
  // Never keep the results in the memory. When getNextRecord() is called, 
  // a satisfying record needs to be fetched from the file.
  // "data" follows the same format as RecordBasedFileManager::insertRecord().
  RC getNextRecord(RID &rid, void *data);
  RC close();
};


class RecordBasedFileManager {
public:
  static RecordBasedFileManager* instance();
  RC createFile(const string &fileName);



  RC destroyFile(const string &fileName);
  
  RC openFile(const string &fileName, FileHandle &fileHandle);
  
  RC closeFile(FileHandle &fileHandle);

  //  Format of the data passed into the function is the following:
  //  [n byte-null-indicators for y fields] [actual value for the first field] [actual value for the second field] ...
  //  1) For y fields, there is n-byte-null-indicators in the beginning of each record.
  //     The value n can be calculated as: ceil(y / 8). (e.g., 5 fields => ceil(5 / 8) = 1. 12 fields => ceil(12 / 8) = 2.)
  //     Each bit represents whether each field value is null or not.
  //     If k-th bit from the left is set to 1, k-th field value is null. We do not include anything in the actual data part.
  //     If k-th bit from the left is set to 0, k-th field contains non-null values.
  //     If there are more than 8 fields, then you need to find the corresponding byte first, 
  //     then find a corresponding bit inside that byte.
  //  2) Actual data is a concatenation of values of the attributes.
  //  3) For Int and Real: use 4 bytes to store the value;
  //     For Varchar: use 4 bytes to store the length of characters, then store the actual characters.
  //  !!! The same format is used for updateRecord(), the returned data of readRecord(), and readAttribute().
  // For example, refer to the Q8 of Project 1 wiki page.
  RC insertRecord(FileHandle &fileHandle, const vector<Attribute> &recordDescriptor, const void *data, RID &rid);

  RC readRecord(FileHandle &fileHandle, const vector<Attribute> &recordDescriptor, const RID &rid, void *data);
  
  // This method will be mainly used for debugging/testing. 
  // The format is as follows:
  // field1-name: field1-value  field2-name: field2-value ... \n
  // (e.g., age: 24  height: 6.1  salary: 9000
  //        age: NULL  height: 7.5  salary: 7500)
  RC printRecord(const vector<Attribute> &recordDescriptor, const void *data);

/******************************************************************************************************************************************************************
IMPORTANT, PLEASE READ: All methods below this comment (other than the constructor and destructor) are NOT required to be implemented for the part 1 of the project
******************************************************************************************************************************************************************/
  RC deleteRecord(FileHandle &fileHandle, const vector<Attribute> &recordDescriptor, const RID &rid);

  // Assume the RID does not change after an update
  RC updateRecord(FileHandle &fileHandle, const vector<Attribute> &recordDescriptor, const void *data, const RID &rid);

  RC readAttribute(FileHandle &fileHandle, const vector<Attribute> &recordDescriptor, const RID &rid, const string &attributeName, void *data);

  // Scan returns an iterator to allow the caller to go through the results one by one. 
  RC scan(FileHandle &fileHandle,
      const vector<Attribute> &recordDescriptor,
      const string &conditionAttribute,
      const CompOp compOp,                  // comparision type such as "<" and "="
      const void *value,                    // used in the comparison
      const vector<string> &attributeNames, // a list of projected attributes
      RBFM_ScanIterator &rbfm_ScanIterator);

  void builtAttributes(vector<Attribute> &attrs, string name, AttrLength length, AttrType type, int valid) {
  	Attribute temp;
  	temp.name = name;
  	temp.length = length;
  	temp.type = type;
  	temp.valid = valid;
  	attrs.push_back(temp);
  }

  void prepareTable(int &offset,void* record, unsigned char *nullsIndicator,vector<Attribute> &attrs,int tableid,string tablename,string filename){
  	offset = 0;

  	memcpy((char*)record + offset,nullsIndicator,ceil((double)(attrs.size())/ CHAR_BIT));
  	offset += ceil((double)(attrs.size())/ CHAR_BIT);

  	memcpy((char*)record+offset,&tableid,sizeof(int));
  	offset += sizeof(int);

  	int l = tablename.size();
  	memcpy((char*)record+offset, &l, sizeof(int));
  	offset += sizeof(int);

  	memcpy((char*)record+offset, tablename.c_str(),tablename.size());
  	offset += tablename.size();

  	l = filename.length();
  	memcpy((char*)record+offset, &l, sizeof(int));
  	offset += sizeof(int);

  	memcpy((char*)record+offset, filename.c_str(),filename.size());
  	offset += filename.size();
  }

  void prepareColumn(int &offset,void* record, unsigned char *nullsIndicator,vector<Attribute> &attrs,int tableid,string columnname,int columntype, int columnlength,int columnposition, int valid){
  	offset = 0;
  	memcpy((char*)record+offset,nullsIndicator,ceil((double)(attrs.size())/ CHAR_BIT));
  	offset += ceil((double)(attrs.size())/ CHAR_BIT);

  	memcpy((char*)record+offset,&tableid,sizeof(int));
  	offset += sizeof(int);

  	int l = columnname.size();
	memcpy((char*)record+offset, &l, sizeof(int));
	offset += sizeof(int);

  	memcpy((char*)record+offset, columnname.c_str(),columnname.size());
  	offset += columnname.size();

  	memcpy((char*)record+offset, &columntype,sizeof(int));
  	offset += sizeof(int);

  	memcpy((char*)record+offset, &columnlength,sizeof(int));
  	offset += sizeof(int);

  	memcpy((char*)record+offset, &columnposition,sizeof(int));
  	offset += sizeof(int);

  	memcpy((char*)record+offset, &valid,sizeof(int));
  	offset += sizeof(int);
  }
public:

protected:
  RecordBasedFileManager();
  ~RecordBasedFileManager();

private:
  static RecordBasedFileManager *_rbf_manager;
  char* tmp;
};

#endif
