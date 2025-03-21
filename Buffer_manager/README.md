# Buffer Manager

## Project Overview
This buffer manager implementation provides a solution for managing page buffers in a database system, with support for various replacement strategies and detailed statistics tracking.

### Features
* Page size is defined as PAGE_SIZE in dberror.h
* SM_FileHandle struct maintains File information
* SM_PageHandle is a pointer to a memory area storing page data
* Error handling uses RC (Return Code) values defined in dberror.h

### File Structure
* storage_mgr.c: Implementation of the storage manager
* storage_mgr.h: Header file with interface declarations
* buffer_mgr.c: Implementation of the buffer manager
* buffer_mgr.h: Header file with interface declarations
* buffer_mgr_stat.c: Implementation of the buffer manager
* buffer_mgr_stat.h: Header file with interface declarations
* dberror.c: Implementation of error handling functions
* dberror.h: Header file with error code definitions
* dt.h: Defines boolean data type and related macros
* test_assign2_1.c: Provided test cases
* test_assign2_2.c: Provided test cases
* test_helper.h: Provided header file for test cases
* Makefile: For compiling the project

### Implementation Details
* PageFrame: Represents a page in memory, containing page data, metadata, and LRU-K information
* MgmtInfo: Manages buffer pool information, including frames, I/O counts, and cache hits
* FIFO: Replaces the page that has been in the buffer the longest
* LRU: Replaces the least recently used page based on access count
* LRU-K: Considers the K most recent accesses to make replacement decisions
* Dynamically allocates memory for page frames and management structures
* Properly frees allocated memory during shutdown to prevent memory leaks
* Robust error checking for invalid inputs and failed operations
* Returns appropriate error codes (RC_ERROR, RC_OK) for different scenarios
* Uses the storage manager to read pages from disk and write them back
* Optimizes I/O by only writing dirty pages and reading pages not already in memory
* Implements fix counts to track pinned pages and prevent premature eviction
* Implements cache hit tracking to measure and potentially improve buffer pool efficiency
* Uses efficient data structures and algorithms for page lookup and replacement

### Key Functions
* initBufferPool: Initializes the buffer pool with specified parameters
* shutdownBufferPool: Safely shuts down the buffer pool, flushing dirty pages
* forceFlushPool: Forces all dirty pages in the buffer pool to be written to disk
* pinPage: Pins a page in the buffer pool, reading it from disk if necessary
* unpinPage: Decreases the pin count of a page
* markDirty: Marks a page as dirty, indicating it needs to be written back to disk
* forcePage: Forces a specific page to be written to disk
* getFrameContents: Returns an array of page numbers for each frame
* getDirtyFlags: Returns an array of boolean values indicating which frames contain dirty pages
* getFixCounts: Returns an array of fix counts for each frame
* getNumReadIO: Returns the number of read I/O operations performed
* getNumWriteIO: Returns the number of write I/O operations performed

### Compilation and Testing
* Use the provided Makefile to compile the project:
  * Go to Project root (Assignment2) using Terminal.
  * Type "make clean" to delete old compiled .o files.
  * Type "make" to compile all project files.
  * Type "make run_test1" to run "test_assign2_1.c" file.
  * Type "make run_test2" to run "test_assign2_2.c" file.

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
