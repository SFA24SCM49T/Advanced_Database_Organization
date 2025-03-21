# Storage Manager

## Project Overview
This project implements a simple storage manager - a module capable of reading blocks from a file on disk into memory and writing blocks from memory to a file on disk. The storage manager deals with fixed-size pages (blocks) (PAGE_SIZE).

### Features
* Create, open, and close files
* Read and write pages to/from disk
* Maintain file information (total pages, current page position, file name)
* Append empty blocks and ensure capacity

### File Structure
* storage_mgr.c: Implementation of the storage manager
* storage_mgr.h: Header file with interface declarations
* dberror.c: Implementation of error handling functions
* dberror.h: Header file with error code definitions
* test_assign1_1.c: Provided test cases
* test_assign1_extended.c: Additional test cases
* Makefile: For compiling the project

### Implementation Details
* Page size is defined as PAGE_SIZE in dberror.h
* SM_FileHandle struct maintains File information
* SM_PageHandle is a pointer to a memory area storing page data
* Error handling uses RC (Return Code) values defined in dberror.h

### Key Functions
* createPageFile: Creates a new page file with one page
* openPageFile: Opens an existing page file
* closePageFile: Closes an open page file
* destroyPageFile: Deletes a page file
* readBlock: Reads a specific block from the file
* writeBlock: Writes a block to the file
* appendEmptyBlock: Adds a new page filled with zeros
* ensureCapacity: Ensures the file has at least a specified number of pages

### Compilation and Testing
* Use the provided Makefile to compile the project:
  * Go to Project root (Assignment1) using Terminal.
  * Type "make clean" to delete old compiled .o files.
  * Type "make" to compile all project files.
  * Type "make run_test1" to run "test_assign1_1.c" file.
  * Type "make run_test2" to run "test_assign1_2.c" file.

Group 26 Members:
   * Sathvika Sagar Tavitireddy
     * CWID:  A20560449
     * Email: stavitireddy@hawk.iit.edu
   * Komma Sanjay Bhargav
     * CWID:  A20554191
     * Email: skomma@hawk.iit.edu
   * Chaitanya Durgesh Nynavarapu
     * CWID: A20561894
     * Email: cnynavarapu@hawk.iit.edu 
