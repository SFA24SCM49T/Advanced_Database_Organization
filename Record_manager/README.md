# Record Manager

## Project Overview
This record manager implementation provides a solution for managing records in a database system, with support for table operations, record manipulation, and scanning.

### Features
* Fixed schema tables
* Record insertion, deletion, and updating
* Table scanning with search conditions
* Schema and attribute management
* Expression evaluation for scan conditions

### File Structure
* storage_mgr.c: Implementation of the storage manager
* storage_mgr.h: Header file with interface declarations
* buffer_mgr.c: Implementation of the buffer manager
* buffer_mgr.h: Header file with interface declarations
* buffer_mgr_stat.c: Implementation of the buffer manager stat
* buffer_mgr_stat.h: Header file with interface declarations
* record_mgr.c: Implementation of the record manager
* record_mgr.h: Header file with interface declarations
* rm_serializer.c: Contains functions for serializing and deserializing various data structures
* tables.h: Defines key data structures and types
* dberror.c: Implementation of error handling functions
* dberror.h: Header file with error code definitions
* expr.c: Performs boolean operations
* expr.h: Header file with interface declarations
* dt.h: Defines boolean data type and related macros
* test_assign3_1.c: Provided test cases
* test_expr.c: Defines expression evaluation functionality
* test_helper.h: Provided header file for test cases
* Makefile: For compiling the project

### Implementation Details
* Table Management: Creation, opening, closing, and deletion of tables
* Record Operations: Insertion, deletion, updating, and retrieval of records
* Scanning: Implementation of table scans with condition evaluation
* Schema Handling: Creation and management of table schemas
* Attribute Manipulation: Getting and setting attribute values in records
* Expression Evaluation: Support for various operators and attribute references
* Memory Management: Proper allocation and deallocation of resources
* Error Handling: Robust error checking and appropriate return codes

### Key Functions
* initRecordManager: Initializes the record manager
* shutdownRecordManager: Shuts down the record manager
* createTable: Creates a new table with the specified schema
* openTable: Opens an existing table for operations
* closeTable: Closes a table and writes changes to disk
* deleteTable: Deletes a table from the database
* getNumTuples: Returns the number of tuples in a table
* insertRecord: Inserts a new record into a table
* deleteRecord: Deletes a record from a table
* updateRecord: Updates an existing record in a table
* getRecord: Retrieves a record from a table
* startScan: Initiates a scan on a table with a given condition
* next: Retrieves the next record that satisfies the scan condition
* closeScan: Closes a scan operation
* getRecordSize: Returns the size of a record for a given schema
* createSchema: Creates a new schema
* freeSchema: Deallocates memory used by a schema
* createRecord: Creates a new record for a given schema
* freeRecord: Deallocates memory used by a record
* getAttr: Retrieves an attribute value from a record
* setAttr: Sets an attribute value in a record

### Compilation and Testing
* Use the provided Makefile to compile the project:
  * Go to Project root (Assignment3) using Terminal.
  * Type "make clean" to delete old compiled .o files.
  * Type "make" to compile all project files.
  * Type "make run_test1" to run "test_assign3_1.c" file.

## Group 26 Members:
   * Sathvika Sagar Tavitireddy
     * CWID:  A20560449
     * Email: stavitireddy@hawk.iit.edu
   * Komma Sanjay Bhargav
     * CWID:  A20554191
     * Email: skomma@hawk.iit.edu
   * Chaitanya Durgesh Nynavarapu
     * CWID: A20561894
     * Email: cnynavarapu@hawk.iit.edu 
