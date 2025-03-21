# B+ Tree Manager

## Project Overview
This B+ tree manager provides a comprehensive implementation of a B+ tree data structure, supporting typical operations such as insertion, deletion, and searching of key-value pairs. The B+ tree manager is integrated with record scanning, enabling sequential access to records in sorted order. This implementation is designed to work within a database management system context.

### Features
* Insertion, deletion, and searching of keys within a B+ tree
* Key-value pairs storage with efficient retrieval
* Support for dynamic tree growth with node splitting and merging
* Sequential scanning of records in sorted order
* Management of underflows and overflows with node redistribution and merging
* Detailed error handling for each operation

### File Structure
* `btree_mgr.c`: Main implementation file for the B+ tree manager
* `btree_mgr.h`: Header file defining the interface and data structures
* `storage_mgr.c`: Implementation of the storage manager
* `storage_mgr.h`: Header file with interface declarations
* `buffer_mgr.c`: Implementation of the buffer manager
* `buffer_mgr.h`: Header file with interface declarations
* `buffer_mgr_stat.c`: Implementation of the buffer manager stat
* `buffer_mgr_stat.h`: Header file with interface declarations
* `record_mgr.c`: Implementation of the record manager
* `record_mgr.h`: Header file with interface declarations
* `rm_serializer.c`: Contains functions for serializing and deserializing various data structures
* `tables.h`: Defines key data structures and types
* `dberror.c`: Implementation of error handling functions
* `dberror.h`: Header file with error code definitions
* `expr.c`: Performs boolean operations
* `expr.h`: Header file with interface declarations
* `dt.h`: Defines boolean data type and related macros
* `tables.h`: Defines key data structures and types, including Value and RID (record identifier)
* `test_helper.h`: Provided header file for test cases
* `test_assign4_1.c`: Test cases for the B+ tree manager
* `Makefile`: Used to compile the project and run test cases

### Implementation Details
* **Tree Management**: Functions to create, open, close, and delete B+ trees, along with handling metadata such as the number of nodes and entries.
* **Node Management**: Creation of leaf and internal nodes, with dynamic handling of node overflows (splitting) and underflows (merging or borrowing from siblings).
* **Key Insertion**: Handles recursive insertion, including splitting of nodes when they reach capacity and adjusting parent nodes as needed.
* **Key Deletion**: Support for key deletion with underflow handling, including merging nodes and redistributing keys.
* **Record Scanning**: Allows sequential access to records in sorted order through a scan interface.
* **Error Handling**: Comprehensive error handling, with custom error codes for missing keys, tree structure issues, and scan operations.
* **Memory Management**: Efficient use of dynamic memory for nodes, with clean-up operations upon closing or deleting the tree.

### Key Functions
* **Tree Management Functions**:
  * `initIndexManager`: Initializes the B+ tree index manager, setting up storage dependencies.
  * `shutdownIndexManager`: Shuts down the index manager, releasing any resources.
  * `createBtree`: Creates a new B+ tree with a specified key type and node capacity.
  * `openBtree`: Opens an existing B+ tree for read and write operations.
  * `closeBtree`: Closes an open B+ tree, flushing any remaining changes.
  * `deleteBtree`: Deletes an existing B+ tree from the storage.

* **Access Information Functions**:
  * `getNumNodes`: Retrieves the number of nodes in the B+ tree.
  * `getNumEntries`: Retrieves the total number of entries (key-value pairs) in the B+ tree.
  * `getKeyType`: Returns the data type of the keys stored in the B+ tree.

* **Core Tree Operations**:
  * `insertKey`: Inserts a key-RID pair into the B+ tree, with recursive node handling for splits.
  * `findKey`: Searches for a key in the B+ tree, returning the associated RID if found.
  * `deleteKey`: Deletes a key from the B+ tree, managing any necessary underflow handling.

* **Scanning Operations**:
  * `openTreeScan`: Initializes a scan on the B+ tree to access records sequentially.
  * `nextEntry`: Retrieves the next entry in the scan, providing sorted access to records.
  * `closeTreeScan`: Closes an ongoing scan, releasing associated resources.

* **Helper Functions**:
  * `splitChild`: Splits a node when it reaches capacity, redistributing keys and adjusting parent nodes.
  * `insertNonFull`: Inserts a key into a node that is not yet at full capacity.
  * `deleteKeyRecursive`: Recursively deletes a key, managing internal node adjustments.
  * `handleUnderflow`: Manages underflow scenarios by merging or redistributing keys between nodes.

### Compilation and Testing
* Use the provided Makefile to compile the project:
  * Go to the project root directory (Assignment4) using a terminal.
  * Run `make clean` to delete old compiled `.o` files.
  * Run `make` to compile all project files.
  * Run `make run_test1` to execute the test file `test_assign4_1.c`.

## Group 26 Members:
   * Sathvika Sagar Tavitireddy
     * CWID: A20560449
     * Email: stavitireddy@hawk.iit.edu
   * Chaitanya Durgesh Nynavarapu
     * CWID: A20561894
     * Email: cnynavarapu@hawk.iit.edu
