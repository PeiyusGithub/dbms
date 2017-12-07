
#include "rm.h"
#include "../rbf/rbfm.h"


RelationManager* RelationManager::instance()
{
	static RelationManager _rm;
	return &_rm;
}

RelationManager::RelationManager()
{
}

RelationManager::~RelationManager()
{
}

RC RelationManager::createCatalog()
{
	RecordBasedFileManager* tab_rbfm = RecordBasedFileManager::instance();
	RecordBasedFileManager* col_rbfm = RecordBasedFileManager::instance();
	RecordBasedFileManager* id_rbfm = RecordBasedFileManager::instance();
	RID rid;
    rid.slotNum = 0;
    rid.pageNum = 0;

	//check re-create catalog
	if (tab_rbfm->createFile("Tables") == -1 || col_rbfm->createFile("Columns") == -1 || id_rbfm->createFile("id") == -1)
		return -1;

	//id number
	FileHandle fileHandleid;
	if (id_rbfm->openFile("id",fileHandleid) != 0) return -1;

	vector<Attribute> id_att;
	Attribute temp;
	temp.length = (AttrLength)4;
	temp.name = "id";
	temp.type = TypeInt;
	temp.valid = 1;
	id_att.push_back(temp);

	int offset = static_cast<int>(ceil((double)(id_att.size()) / CHAR_BIT));
	unsigned char *nullsIndicator_id = (unsigned char *) malloc(offset);
	memset(nullsIndicator_id, 0, offset);

	void *idrecord = malloc(sizeof(unsigned int) + offset);
	memcpy(idrecord,nullsIndicator_id,offset);

	unsigned int cur_id = 2;
	memcpy((char*)idrecord+offset,&cur_id,sizeof(unsigned int));

	id_rbfm->insertRecord(fileHandleid,id_att,idrecord,rid);
	id_rbfm->closeFile(fileHandleid);
	free(nullsIndicator_id);
	free(idrecord);


	void *temprecord = malloc(100);
	int recordsize = 0;

	//tables:
	FileHandle fileHandle1;
	if (tab_rbfm->openFile("Tables",fileHandle1) != 0) return -1;

	vector<Attribute> attrs_t;
	tab_rbfm->builtAttributes(attrs_t,  "table-id",  (AttrLength)4,  TypeInt,1);
	tab_rbfm->builtAttributes(attrs_t, "table-name", (AttrLength)50, TypeVarChar,1);
	tab_rbfm->builtAttributes(attrs_t, "file-name", (AttrLength)50, TypeVarChar,1);

	unsigned char *nullsIndicator_t = (unsigned char *) malloc(ceil((double)(attrs_t.size())/ CHAR_BIT));
	memset(nullsIndicator_t, 0, ceil((double)(attrs_t.size())/ CHAR_BIT));

	tab_rbfm->prepareTable(recordsize, temprecord, nullsIndicator_t,attrs_t,1,"Tables","Tables");
	void *trecord1 = malloc(recordsize);
	memcpy(trecord1,temprecord,recordsize);


	tab_rbfm->insertRecord(fileHandle1,attrs_t,trecord1,rid);
	free(trecord1);

	tab_rbfm->prepareTable(recordsize, temprecord,nullsIndicator_t,attrs_t,2,"Columns","Columns");
	void* trecord2 = malloc(recordsize);
	memcpy(trecord2,temprecord,recordsize);
	//tab_rbfm->printRecord(attrs_t,trecord2);
	tab_rbfm->insertRecord(fileHandle1,attrs_t,trecord2,rid);
	tab_rbfm->closeFile(fileHandle1);
	free(trecord2);

	free(nullsIndicator_t);

	//columns:
	FileHandle fileHandle2;
	col_rbfm->createFile("Columns");
	if (col_rbfm->openFile("Columns",fileHandle2) != 0) return -1;

	vector<Attribute> attrs_c;
	col_rbfm->builtAttributes(attrs_c, "table-id", (AttrLength)4, TypeInt,1);
	col_rbfm->builtAttributes(attrs_c, "column-name", (AttrLength)50, TypeVarChar,1);
	col_rbfm->builtAttributes(attrs_c, "column-type", (AttrLength)4, TypeInt,1);
	col_rbfm->builtAttributes(attrs_c, "column-length", (AttrLength)4, TypeInt,1);
	col_rbfm->builtAttributes(attrs_c, "column-position", (AttrLength)4, TypeInt,1);
	col_rbfm->builtAttributes(attrs_c, "column-valid", (AttrLength)4, TypeInt, 1);

	unsigned char *nullsIndicator_c = (unsigned char *) malloc(ceil((double)(attrs_c.size())/ CHAR_BIT));
	memset(nullsIndicator_c, 0, ceil((double)(attrs_c.size())/ CHAR_BIT));

	col_rbfm->prepareColumn(recordsize, temprecord, nullsIndicator_c,attrs_c,1,"table-id", TypeInt, 4 , 1, 1);
	void *crecord1 = malloc(recordsize);
	memcpy(crecord1,temprecord,recordsize);
	//tab_rbfm->printRecord(attrs_c,crecord1);
	col_rbfm->insertRecord(fileHandle2,attrs_c,crecord1,rid);
	free(crecord1);

	col_rbfm->prepareColumn(recordsize, temprecord, nullsIndicator_c,attrs_c,1, "table-name", TypeVarChar, 50, 2, 1);
	void *crecord2 = malloc(recordsize);
	memcpy(crecord2,temprecord,recordsize);
	col_rbfm->insertRecord(fileHandle2,attrs_c,crecord2,rid);
	free(crecord2);

	col_rbfm->prepareColumn(recordsize, temprecord, nullsIndicator_c,attrs_c,1, "file-name", TypeVarChar, 50, 3, 1);
	void *crecord3 = malloc(recordsize);
	memcpy(crecord3,temprecord,recordsize);
	col_rbfm->insertRecord(fileHandle2,attrs_c,crecord3,rid);
	free(crecord3);

	col_rbfm->prepareColumn(recordsize, temprecord, nullsIndicator_c,attrs_c,2, "table-id", TypeInt, 4, 1, 1);
	void *crecord4 = malloc(recordsize);
	memcpy(crecord4,temprecord,recordsize);
	col_rbfm->insertRecord(fileHandle2,attrs_c,crecord4,rid);
	free(crecord4);

	col_rbfm->prepareColumn(recordsize, temprecord, nullsIndicator_c,attrs_c,2, "column-name",  TypeVarChar, 50, 2, 1);
	void *crecord5 = malloc(recordsize);
	memcpy(crecord5,temprecord,recordsize);
	col_rbfm->insertRecord(fileHandle2,attrs_c,crecord5,rid);
	free(crecord5);

	col_rbfm->prepareColumn(recordsize, temprecord, nullsIndicator_c,attrs_c,2, "column-type", TypeInt, 4, 3, 1);
	void *crecord6 = malloc(recordsize);
	memcpy(crecord6,temprecord,recordsize);
	col_rbfm->insertRecord(fileHandle2,attrs_c,crecord6,rid);
	free(crecord6);

	col_rbfm->prepareColumn(recordsize, temprecord, nullsIndicator_c,attrs_c,2, "column-length", TypeInt, 4, 4, 1);
	void *crecord7 = malloc(recordsize);
	memcpy(crecord7,temprecord,recordsize);
	col_rbfm->insertRecord(fileHandle2,attrs_c,crecord7,rid);
	free(crecord7);

	col_rbfm->prepareColumn(recordsize, temprecord, nullsIndicator_c,attrs_c,2, "column-position", TypeInt, 4, 5, 1);
	void *crecord8 = malloc(recordsize);
	memcpy(crecord8,temprecord,recordsize);
	col_rbfm->insertRecord(fileHandle2,attrs_c,crecord8,rid);
	free(crecord8);

	col_rbfm->prepareColumn(recordsize, temprecord, nullsIndicator_c,attrs_c,2, "column-valid", TypeInt, 4, 6, 1);
	void *crecord9 = malloc(recordsize);
	memcpy(crecord9,temprecord,recordsize);
	col_rbfm->insertRecord(fileHandle2,attrs_c,crecord9,rid);
	free(crecord9);

	col_rbfm->closeFile(fileHandle2);
	free(nullsIndicator_c);
	free(temprecord);
	return 0;
}

RC RelationManager::deleteCatalog()
{
	RecordBasedFileManager* tab_rbfm = RecordBasedFileManager::instance();
	RecordBasedFileManager* col_rbfm = RecordBasedFileManager::instance();
	RecordBasedFileManager* id_rbfm = RecordBasedFileManager::instance();
	return min(min(tab_rbfm->destroyFile("Tables"),col_rbfm->destroyFile("Columns")), id_rbfm->destroyFile("id"));
}

RC RelationManager::createTable(const string &tableName, const vector<Attribute> &attrs)
{
	// get the current id number
	FileHandle fileHandleid;
	RecordBasedFileManager* id_rbfm   = RecordBasedFileManager::instance();
	if (id_rbfm->openFile("id",fileHandleid) != 0) return -1;

	vector<Attribute> att_id;
	Attribute temp;
	temp.length =(AttrLength)4;
	temp.name = "id";
	temp.type = TypeInt;
	temp.valid = 1;
	att_id.push_back(temp);

	RID rid;
	rid.pageNum = 2;
	rid.slotNum = 0;

	int offset = ceil((double)(att_id.size())/ CHAR_BIT);
	void* res = malloc(offset+sizeof(unsigned int));
	id_rbfm->readAttribute(fileHandleid,att_id,rid,"id",res);
	unsigned int idnum;
	memcpy(&idnum,(unsigned int *)((char*)res+offset),sizeof(unsigned int));



	//update tables
	int recordsize = 0;
	void* temprecord = malloc(100);

	RecordBasedFileManager* tab_rbfm = RecordBasedFileManager::instance();
	FileHandle fileHandle1;
	if (tab_rbfm->openFile("Tables",fileHandle1) != 0 ) return -1;

	vector<Attribute> attrs_t;
	tab_rbfm->builtAttributes(attrs_t,  "table-id",  (AttrLength)4,  TypeInt,1);
	tab_rbfm->builtAttributes(attrs_t, "table-name", (AttrLength)50, TypeVarChar,1);
	tab_rbfm->builtAttributes(attrs_t, "file-name", (AttrLength)50, TypeVarChar,1);

	unsigned char *nullsIndicator_t = (unsigned char *) malloc(ceil((double)(attrs_t.size())/ CHAR_BIT));
	memset(nullsIndicator_t, 0, ceil((double)(attrs_t.size())/ CHAR_BIT));

	tab_rbfm->prepareTable(recordsize, temprecord, nullsIndicator_t,attrs_t,idnum+1,tableName,tableName);
	void *trecord1 = malloc(recordsize);
	memcpy(trecord1,temprecord,recordsize);
	tab_rbfm->insertRecord(fileHandle1,attrs_t,trecord1,rid);
	tab_rbfm->closeFile(fileHandle1);
	free(trecord1);

	//update columns
	RecordBasedFileManager* col_rbfm = RecordBasedFileManager::instance();
	FileHandle fileHandle2;
	if(col_rbfm->openFile("Columns",fileHandle2) != 0) return -1;

	vector<Attribute> attrs_c;
	col_rbfm->builtAttributes(attrs_c, "table-id", (AttrLength)4, TypeInt,1);
	col_rbfm->builtAttributes(attrs_c, "column-name", (AttrLength)50, TypeVarChar,1);
	col_rbfm->builtAttributes(attrs_c, "column-type", (AttrLength)4, TypeInt,1);
	col_rbfm->builtAttributes(attrs_c, "column-length", (AttrLength)4, TypeInt,1);
	col_rbfm->builtAttributes(attrs_c, "column-position", (AttrLength)4, TypeInt,1);
	col_rbfm->builtAttributes(attrs_c, "column-valid", (AttrLength)4, TypeInt, 1);

	unsigned char *nullsIndicator_c = (unsigned char *) malloc(ceil((double)(attrs_c.size())/ CHAR_BIT));
	memset(nullsIndicator_c, 0, ceil((double)(attrs_c.size())/ CHAR_BIT));

	for (unsigned int i = 0; i < attrs.size(); ++i) {
		col_rbfm->prepareColumn(recordsize, temprecord, nullsIndicator_c,attrs_c,idnum+1,attrs[i].name, attrs[i].type, attrs[i].length, i+1, 1);
		void *crecord1 = malloc(recordsize);
		memcpy(crecord1,temprecord,recordsize);
		col_rbfm->insertRecord(fileHandle2,attrs_c,crecord1,rid);
		free(crecord1);
	}
	col_rbfm->closeFile(fileHandle2);


	//update id
	idnum++;
	memcpy((char*)res+offset,&idnum,sizeof(unsigned int));
	rid.pageNum = 2; rid.slotNum = 0;
	id_rbfm->updateRecord(fileHandleid,att_id,res,rid);
	id_rbfm->closeFile(fileHandleid);
	//free
	free(temprecord);
	free(nullsIndicator_t);
	free(nullsIndicator_c);
	free(res);

	RecordBasedFileManager* rbfm = RecordBasedFileManager::instance();
    RecordBasedFileManager* index_rbfm = RecordBasedFileManager::instance();
    string index_name = tableName + "indexFiles";
	int a = rbfm->createFile(tableName);
	int b = index_rbfm->createFile(index_name);

	return min(a, b);
}

RC RelationManager::deleteTable(const string &tableName)
{
	if (tableName == "Tables" || tableName == "Columns")	return -1;
	RecordBasedFileManager* rbfm   = RecordBasedFileManager::instance();
    RecordBasedFileManager* index_rbfm   = RecordBasedFileManager::instance();
    string index_name = tableName + "indexFiles";
	return min(rbfm->destroyFile(tableName), index_rbfm->destroyFile(index_name));
}

RC RelationManager::getAttributes(const string &tableName, vector<Attribute> &attrs)
{
	//scan table to get the id
	RecordBasedFileManager* tab_rbfm = RecordBasedFileManager::instance();
	FileHandle fileHandle1;
	if (tab_rbfm->openFile("Tables",fileHandle1) != 0) return -1;

	vector<Attribute> attrs_t;
	tab_rbfm->builtAttributes(attrs_t,  "table-id",  (AttrLength)4,  TypeInt,1);
	tab_rbfm->builtAttributes(attrs_t, "table-name", (AttrLength)50, TypeVarChar,1);
	tab_rbfm->builtAttributes(attrs_t, "file-name", (AttrLength)50, TypeVarChar,1);


	void* temp = malloc(tableName.size()+sizeof(int));
	int len = tableName.size();
	memcpy(temp,&len,sizeof(int));
	memcpy((char*)temp + sizeof(int),tableName.c_str(),tableName.size());
	vector<string> attributeNames;
	attributeNames.push_back("table-id");

	RID rid;
	void* returnData = malloc(200);
	RBFM_ScanIterator rbfm_ScanIterator;
	tab_rbfm->scan(fileHandle1,attrs_t,"table-name",EQ_OP,temp,attributeNames, rbfm_ScanIterator);

	rbfm_ScanIterator.getNextRecord(rid,returnData);
	rbfm_ScanIterator.close();

	//tab_rbfm->printRecord(attrs_t,returnData);

	int nullAttributesIndicatorActualSize = 0;
	nullAttributesIndicatorActualSize = ceil((double) attributeNames.size() / CHAR_BIT);

	int idnum;
	memcpy(&idnum,(char*)returnData+nullAttributesIndicatorActualSize,sizeof(unsigned int));
	free(temp);

	//scan by id to find attrs from columns
	RecordBasedFileManager* col_rbfm = RecordBasedFileManager::instance();
	FileHandle fileHandle2;
	if(col_rbfm->openFile("Columns",fileHandle2) != 0) return -1;

	vector<Attribute> attrs_c;
	col_rbfm->builtAttributes(attrs_c, "table-id", (AttrLength)4, TypeInt,1);
	col_rbfm->builtAttributes(attrs_c, "column-name", (AttrLength)50, TypeVarChar,1);
	col_rbfm->builtAttributes(attrs_c, "column-type", (AttrLength)4, TypeInt,1);
	col_rbfm->builtAttributes(attrs_c, "column-length", (AttrLength)4, TypeInt,1);
	col_rbfm->builtAttributes(attrs_c, "column-position", (AttrLength)4, TypeInt,1);
	col_rbfm->builtAttributes(attrs_c, "column-valid", (AttrLength)4, TypeInt, 1);

	void* val = malloc(sizeof(unsigned int));
	memcpy(val, &idnum, sizeof(unsigned int));

	vector<string> targetAtt;
	targetAtt.push_back("column-name");
	targetAtt.push_back("column-type");
	targetAtt.push_back("column-length");
	targetAtt.push_back("column-valid");

	RBFM_ScanIterator rbfm_ScanIteratorc;
	col_rbfm->scan(fileHandle2,attrs_c,"table-id",EQ_OP,val,targetAtt, rbfm_ScanIteratorc);
	free(val);


	while(rbfm_ScanIteratorc.getNextRecord(rid,returnData) != RM_EOF) {
		//string name;
		int offset = 0;
		offset = ceil((double) targetAtt.size() / CHAR_BIT);

		unsigned int len,length;
		AttrType type;
		int valid = 1;

		memcpy(&len,(char*)returnData+offset,sizeof(unsigned int));
		offset += sizeof(unsigned int);

		char* name = (char*)malloc(len+1);
		memcpy(name,(char*)returnData+offset,len+1);
		name[len] = '\0';


		offset += len;
		memcpy(&type,(char*)returnData+offset,sizeof(AttrType));
		//cout << type << " type  " << endl;
		offset += sizeof(AttrType);
		memcpy(&length,(char*)returnData+offset,sizeof(unsigned int));
		//cout << length << " length  " << endl;
		offset += sizeof(unsigned int);
		memcpy(&valid,(char*)returnData+offset,sizeof(unsigned int));
		//cout << valid << " valid  " << endl << endl << endl;
		offset += sizeof(unsigned int);


		Attribute att;
		att.name = name;
		att.length = length;
		att.type = type;
		att.valid = valid;
		//cout << name << " " << length << " " << type << " " << valid << endl;
		attrs.push_back(att);
		free(name);

	}
	rbfm_ScanIteratorc.close();
	free(returnData);

	return 0;
}

RC RelationManager::decodeIndex(const void* data, vector<Attribute> attrs, const string attName, void* key) {
    int len = attrs.size();
    int offset = ceil((double)(len)/CHAR_BIT);
    unsigned char* nullsIndicator = (unsigned char*) malloc(offset);
    memcpy(nullsIndicator, data, offset);
    for (int i = 0; i < attrs.size(); ++i) {
        int block = i / 8, pos = i % 8;
        if (nullsIndicator[block] & (1 << (7 - pos)))
            continue;
        else {
            if (attrs[i].name == attName) {
                switch (attrs[i].type) {
                    case TypeInt:
                        memcpy(key, (char*)data + offset, sizeof(int));
                        return 0;

                    case TypeReal:
                        memcpy(key, (char*)data + offset, sizeof(float));
                        return 0;

                    case TypeVarChar:
                        int len = *(int*)((char*)data + offset);
                        memcpy(key, (char*)data + offset, (sizeof(int) + len));
                        return 0;
                }
            } else {
                switch(attrs[i].type) {
                    case TypeInt:
                        offset += sizeof(int);
                        break;

                    case TypeReal:
                        offset += sizeof(float);
                        break;

                    case TypeVarChar:
                        int len = *(int*)((char*)data + offset);
                        offset += len + sizeof(int);
                        break;
                }
            }

        }
    }
    return -1;
}

RC RelationManager::insertTuple(const string &tableName, const void *data, RID &rid)
{
	//call getAttributes to get attrs
	RelationManager* mf = RelationManager::instance();
	vector<Attribute> attrs;
	mf->getAttributes(tableName,attrs);

	// call insert function
	RecordBasedFileManager* rbfm = RecordBasedFileManager::instance();
	FileHandle fileHandle;
	if (rbfm->openFile(tableName,fileHandle) != 0)
        return -1;
    else if (rbfm->insertRecord(fileHandle,attrs,data,rid) != 0)
        return -1;
    else
        rbfm->closeFile(fileHandle);

    //need to get all the index files attribute here


    vector<string> indexFileNames;
    string index_name = tableName + "indexFiles";
    getIndex(indexFileNames, index_name);

    void* key = malloc(100);
    int type = 0, len = 0;

    for (int i = 0; i < indexFileNames.size(); ++i) {
        Attribute att;
        for (int j = 0; j < attrs.size(); ++j) {
            if (attrs[j].name == indexFileNames[i]) {
                //update this attributte index file
                att = attrs[j];
                type = attrs[j].type;
                decodeIndex(data,attrs,att.name,key);
                break;
            }
        }
        switch(type) {
            case TypeReal: len = 4; break;
            case TypeInt: len = 4; break;
            case TypeVarChar: len = *(int*)key + 4;
        }

        void* tmp = malloc(len);
        memcpy(tmp, key, len);

        IndexManager* indexManager = IndexManager::instance();
        IXFileHandle ixFileHandle;
        string name = tableName + att.name;
        indexManager->openFile(name, ixFileHandle);
        indexManager->insertEntry(ixFileHandle, att, tmp, rid);
        indexManager->closeFile(ixFileHandle);
        free(tmp);
    }

    free(key);
	return 0;
}

RC RelationManager::deleteTuple(const string &tableName, const RID &rid)
{
	//call getAttributes to get attrs
	RelationManager* mf = RelationManager::instance();
	vector<Attribute> attrs;
	mf->getAttributes(tableName,attrs);

	// call delete function
	RecordBasedFileManager* rbfm = RecordBasedFileManager::instance();
	FileHandle fileHandle;
	if (rbfm->openFile(tableName,fileHandle) != 0) return -1;
    void* data = malloc(200);
    rbfm->readRecord(fileHandle, attrs,rid,data);
	if (rbfm->deleteRecord(fileHandle,attrs,rid) != 0)	return -1;
    rbfm->closeFile(fileHandle);

    vector<string> indexFileNames;
    string index_name = tableName + "indexFiles";
    getIndex(indexFileNames, index_name);
    void* key = malloc(200);
    for(int i = 0; i < indexFileNames.size(); ++i) {
        Attribute att;
        for (int j = 0; j < attrs.size(); ++j) {
            if (attrs[j].name == indexFileNames[i]) {
                att = attrs[j];
                decodeIndex(data,attrs,att.name,key);
                break;
            }
        }
        int len = 0;
        switch(att.type) {
            case TypeVarChar: len = *(int*)key + sizeof(int); break;
            case TypeInt: len = sizeof(int); break;
            case TypeReal: len = sizeof(float); break;
        }

        void* tmp = malloc(len);
        memcpy(tmp, key, len);
        IndexManager* indexManager = IndexManager::instance();
        IXFileHandle ixFileHandle;
        string name = tableName + att.name;
        indexManager->openFile(name, ixFileHandle);
        indexManager->deleteEntry(ixFileHandle,att,tmp,rid);
        indexManager->closeFile(ixFileHandle);
        free(tmp);
    }
    free(key);
    return 0;
}
RC RelationManager::updateTuple(const string &tableName, const void *data, const RID &rid)
{
	//call getAttributes to get attrs
	RelationManager* mf = RelationManager::instance();
	vector<Attribute> attrs;
	mf->getAttributes(tableName,attrs);

	// call update function
	RecordBasedFileManager* rbfm = RecordBasedFileManager::instance();
	FileHandle fileHandle;
    void* record = malloc(200);
	if (rbfm->openFile(tableName, fileHandle) != 0) return -1;
    rbfm->readRecord(fileHandle, attrs, rid, record);
	if (rbfm->updateRecord(fileHandle, attrs, data, rid) != 0) return -1;
    rbfm->closeFile(fileHandle);

    vector<string> indexFileNames;
    string index_name = tableName + "indexFiles";
    getIndex(indexFileNames, index_name);

    void* key = malloc(200);
    void* key2 = malloc(200);
    for(int i = 0; i < indexFileNames.size(); ++i) {
        Attribute att;
        for (int j = 0; j < attrs.size(); ++j) {
            if (attrs[j].name == indexFileNames[i]) {
                att = attrs[j];
                decodeIndex(record,attrs,att.name,key);
                decodeIndex(data,attrs,att.name,key2);

                int len = 0;
                switch(att.type) {
                    case TypeVarChar: len = *(int*)key + sizeof(int); break;
                    case TypeInt: len = sizeof(int); break;
                    case TypeReal: len = sizeof(float); break;
                }

                void* tmp = malloc(len);
                void* tmp2 = malloc(len);
                memcpy(tmp, key, len);
                memcpy(tmp2, key2, len);


                IndexManager* indexManager = IndexManager::instance();
                IXFileHandle ixFileHandle;
                string name = tableName + att.name;
                indexManager->openFile(name, ixFileHandle);
                indexManager->deleteEntry(ixFileHandle,att,tmp,rid);
                indexManager->insertEntry(ixFileHandle,att,tmp2,rid);

                indexManager->closeFile(ixFileHandle);
                free(tmp);
				free(tmp2);
                break;
            }
        }
    }
	free(key);
	free(key2);
    return 0;

}

RC RelationManager::readTuple(const string &tableName, const RID &rid, void *data)
{
	//call getAttributes to get attrs
	RelationManager* mf = RelationManager::instance();
	vector<Attribute> attrs;
	mf->getAttributes(tableName,attrs);

	// call read function
	RecordBasedFileManager* rbfm = RecordBasedFileManager::instance();
	FileHandle fileHandle;
	if(rbfm->openFile(tableName,fileHandle) != 0) return -1;

	if(rbfm->readRecord(fileHandle,attrs,rid,data) != 0)  {
		rbfm->closeFile(fileHandle);
		return -1;
	}
	return rbfm->closeFile(fileHandle);

}

RC RelationManager::printTuple(const vector<Attribute> &attrs, const void *data)
{
	RecordBasedFileManager* rbfm = RecordBasedFileManager::instance();
	FileHandle fileHandle;
	rbfm->printRecord(attrs,data);
	return 0;
}

RC RelationManager::readAttribute(const string &tableName, const RID &rid, const string &attributeName, void *data)
{
	//call getAttributes to get attrs
	RelationManager* mf = RelationManager::instance();
	vector<Attribute> attrs;
	mf->getAttributes(tableName,attrs);

	// call readAttribute function
	RecordBasedFileManager* rbfm = RecordBasedFileManager::instance();
	FileHandle fileHandle;
	if(rbfm->openFile(tableName,fileHandle) != 0) return -1;
	if (rbfm->readAttribute(fileHandle,attrs,rid,attributeName,data) != 0)	return -1;
	return rbfm->closeFile(fileHandle);

}

RC RelationManager::scan(const string &tableName,
						 const string &conditionAttribute,
						 const CompOp compOp,                  // comparison type such as "<" and "="
						 const void *value,                    // used in the comparison
						 const vector<string> &attributeNames, // a list of projected attributes
						 RM_ScanIterator &rm_ScanIterator)
{
	RecordBasedFileManager* rbfm = RecordBasedFileManager::instance();
	FileHandle fileHandle;
	if (rbfm->openFile(tableName,fileHandle) != 0) return -1;

	vector<Attribute> recordDescriptor;
	this->getAttributes(tableName,recordDescriptor);

	rbfm->scan(fileHandle,recordDescriptor,conditionAttribute,compOp,value,attributeNames,rm_ScanIterator.rbfm_ScanIterator);
	//rbfm->closeFile(fileHandle);
	return 0;
}

// Extra credit work
RC RelationManager::dropAttribute(const string &tableName, const string &attributeName)
{



	return -1;
}

// Extra credit work
RC RelationManager::addAttribute(const string &tableName, const Attribute &attr)
{
	return -1;
}

RC RelationManager::loadIndex(vector<string> indexFileNames, const string index_name) {

    RecordBasedFileManager* index_rbfm = RecordBasedFileManager::instance();
    FileHandle fileHandle;
    index_rbfm->openFile(index_name, fileHandle);
    int len = indexFileNames.size();
    void* data = malloc(PAGE_SIZE);
    int offset = 0;

    memcpy(data, &len, sizeof(int));
    offset += sizeof(int);

    for (int i = 0; i < len; ++i) {
        int l = indexFileNames[i].length();
        memcpy((char*)data + sizeof(int), &l, sizeof(int));
        offset += sizeof(int);

        memcpy((char*)data + offset, indexFileNames[i].c_str(),l);
        offset += l;
    }

    if (fileHandle.writePage(0,data) != 0){
         free(data);
        return -1;
    }
	free(data);
    return index_rbfm->closeFile(fileHandle);
}

RC RelationManager::getIndex(vector<string> indexFileNames, const string index_name) {
    RecordBasedFileManager* index_rbfm = RecordBasedFileManager::instance();
    FileHandle fileHandle;
    index_rbfm->openFile(index_name, fileHandle);
    void* data = malloc(PAGE_SIZE);
    if (fileHandle.readPage(0,data) != 0) return -1;
    int len = *(int*)data;
    int offset = sizeof(int);
    for (int i = 0; i < len; ++i) {
        int l = *(int*)((char*)data + offset);
        offset += sizeof(int);
        string tmp;
        for (int j = 0; j < l; ++j) {
            tmp += *(char*)((char*)data + offset + j);
        }
        offset += l;
        indexFileNames.push_back(tmp);
    }
	free(data);
    return 0;
}

RC RelationManager::createIndex(const string &tableName, const string &attributeName)
{
    RecordBasedFileManager* index_rbfm = RecordBasedFileManager :: instance();
    RecordBasedFileManager* tab_rbfm = RecordBasedFileManager :: instance();
    FileHandle fileHandle;
    FileHandle indexFileHandle;
    //check if the table exist
    if (tab_rbfm->openFile(tableName,fileHandle) != 0)
        return -1;
    else
        tab_rbfm->closeFile(fileHandle);

    string index_name = tableName + "indexFiles";
    //check if the indexFiles file exist, if not create one, this file record all the indexfile name of specific table
    if (index_rbfm->openFile(index_name, indexFileHandle) == 0 ) {
        ;
    }else if (index_rbfm->createFile(index_name) != 0 ) {
        return -1;
    } else if (index_rbfm->openFile(index_name, indexFileHandle) != 0 ){
        return -1;
    }

    void* data = malloc(PAGE_SIZE);

    vector<string> indexFileNames;
    if (indexFileHandle.readPage(0,data) != 0 ) {
        //new page
        indexFileNames.push_back(attributeName);
        loadIndex(indexFileNames,index_name);
    } else {
        getIndex(indexFileNames,index_name);
        indexFileNames.push_back(attributeName);
        loadIndex(indexFileNames,index_name);
    }
	free(data);
    //now we create the index file
    string index_file = tableName + attributeName;
    IndexManager* tmp = IndexManager::instance();
    IXFileHandle ixFileHandle;
    if (tmp->openFile(index_file,ixFileHandle) == 0){
		tmp->closeFile(ixFileHandle);
		return -1;
	} else
        tmp->createFile(index_file);


    return index_rbfm->closeFile(indexFileHandle);
}

RC RelationManager::destroyIndex(const string &tableName, const string &attributeName)
{
	vector<string> indexFileNames;
    string index_name = tableName + "indexFiles";
    getIndex(indexFileNames, index_name);
    for (int i = 0; i < indexFileNames.size(); ++i) {
        if (indexFileNames[i] == attributeName) {
            indexFileNames.erase(indexFileNames.begin() + i);
            break;
        }
    }
    loadIndex(indexFileNames,index_name);
    string file_name = tableName + attributeName;
	IndexManager * index_manager;
	return index_manager->destroyFile(file_name);
}

RC RelationManager::indexScan(const string &tableName,
							  const string &attributeName,
							  const void *lowKey,
							  const void *highKey,
							  bool lowKeyInclusive,
							  bool highKeyInclusive,
							  RM_IndexScanIterator &rm_IndexScanIterator)
{
    string index_file = tableName + attributeName;
	IXFileHandle ixFileHandle;
	IndexManager* indexManager;
	RM_IndexScanIterator rm_indexScanIterator;
	indexManager->openFile(index_file,ixFileHandle);

	RelationManager relationManager;
	vector<Attribute> tmp;
	Attribute attr;
	relationManager.getAttributes(tableName, tmp);
	for (auto i : tmp) {
		if (i.name == attributeName)
			attr = i;
	}
	return indexManager->scan(ixFileHandle,attr,lowKey,highKey,lowKeyInclusive,highKeyInclusive,rm_indexScanIterator.ix_scanIterator);
}

