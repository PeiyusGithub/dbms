#ifndef _ix_h_
#define _ix_h_

#include <vector>
#include <string>

#include "../rbf/rbfm.h"

# define IX_EOF (-1)  // end of the index scan

class IX_ScanIterator;
class IXFileHandle;
class IndexManager;
class IndexPage;
class LeafPage;

class IndexPage {
public:
    int n, nxt, len, keyType, Left;
    vector<int> Int, Right;
    vector<float> Real;
    vector<string> Varchar;
    IndexPage();
    void encode(void* data, int type);
    void decode(void* data);
};

class LeafPage {
public:
    int n, nxt, len, keyType;
    vector<int> Int;
    vector<float> Real;
    vector<string> Varchar;
    vector<RID> Rid;
    LeafPage();
    void encode(void* data, int type);
    void decode(void* data);
    void update();
};

class IndexManager {
    public:
        char* tmp;
        static IndexManager* instance();
        // Create an index file.
        RC createFile(const string &fileName);
        RC changeHiddenPage(char *s, int val, int type);
        // Delete an index file.
        RC destroyFile(const string &fileName);
        RC getKeyType(IXFileHandle &ixFileHandle);
        RC getRootPage(IXFileHandle &ixFileHandle);

        // Open an index and return an ixfileHandle.
        RC openFile(const string &fileName, IXFileHandle &ixfileHandle);

        // Close an ixfileHandle for an index.
        RC closeFile(IXFileHandle &ixfileHandle);

        // Insert an entry into the given index that is indicated by the given ixfileHandle.
        RC insertEntry(IXFileHandle &ixfileHandle, const Attribute &attribute, const void *key, const RID &rid);

        // Delete an entry from the given index that is indicated by the given ixfileHandle.
        RC deleteEntry(IXFileHandle &ixfileHandle, const Attribute &attribute, const void *key, const RID &rid);

        // Initialize and IX_ScanIterator to support a range search
        RC scan(IXFileHandle &ixfileHandle,
                const Attribute &attribute,
                const void *lowKey,
                const void *highKey,
                bool lowKeyInclusive,
                bool highKeyInclusive,
                IX_ScanIterator &ix_ScanIterator);

        // Print the B+ tree in pre-order (in a JSON record format)
        void printBtree(IXFileHandle &ixfileHandle, const Attribute &attribute) const;

    protected:
        IndexManager();
        ~IndexManager();

    private:
        static IndexManager *_index_manager;
};


class IX_ScanIterator {
    public:
        IXFileHandle ixFileHandle;

        char* tmp;
        bool lowKeyNull;
        bool highKellNull;
        bool lowKeyInclu;
        bool highKeyInclu;

        int currentPage;
        int position;
        int type;
        RID Rid;

        int lowKeyInt;
        int highKeyInt;
        float lowKeyReal;
        float highKeyReal;
        string lowKeyVar;
        string highKeyVar;

		// Constructor
        IX_ScanIterator();

        // Destructor
        ~IX_ScanIterator();

        // Get next matching entry
        RC getNextEntry(RID &rid, void *key);



        // Terminate index scan
        RC close();
};



class IXFileHandle {
    public:
    string fileName;
    FileHandle fileHandle;
    // variables to keep counter for each operation
    unsigned ixReadPageCounter;
    unsigned ixWritePageCounter;
    unsigned ixAppendPageCounter;

    // Constructor
    IXFileHandle();

    // Destructor
    ~IXFileHandle();

	// Put the current counter values of associated PF FileHandles into variables
	RC collectCounterValues(unsigned &readPageCount, unsigned &writePageCount, unsigned &appendPageCount);

};

#endif
