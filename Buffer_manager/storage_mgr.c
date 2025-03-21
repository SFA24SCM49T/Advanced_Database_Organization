#include "storage_mgr.h"
#include "dberror.h"
#include<stdio.h>
#include<stdlib.h>
#include<string.h>


// storage manager doesn't require any initialization. It takes no paramneters, return nothing
extern void initStorageManager (void)
{
	printf("Assignment group 26\n");
	printf("Sathvika Sagar Tavitireddy , CWID : A20560449\n");
	printf("Komma Sanjay Bhargav,  CWID : A20554191\n");
	printf("Chaitanya Durgesh Nynavarapu,  CWID : A20561894\n");

    printf("--------Initializing storage manager ---------\n");
}


// create a page file with given fileName as input parameters
extern RC createPageFile (char *fileName)
{
    // open a file with "filename" for reading and writing in binary mode
    FILE *page_file = fopen(fileName, "wb+");

    //check if the file is opened
    if(page_file == NULL)
    {
        return RC_FILE_NOT_FOUND;   //if file opening is failed
    }
    
    // create an empty page by allocating memory for one page (initializes memory with zero('\0') bytes)
    SM_PageHandle empty_page = (SM_PageHandle)calloc(PAGE_SIZE, sizeof(char));

    if(empty_page == NULL)
    {
        // if failed to allocate memory
        fclose(page_file);      // close the file to prevent resource leaks
        return RC_WRITE_FAILED; 
    }

    // write the empty page to the page file pointer
    if (fwrite(empty_page, sizeof(char), PAGE_SIZE, page_file) < PAGE_SIZE) 
    {
        //if the number of bytes written to page file is less than PAGE_SIZE, write operation is not successful
        free(empty_page);     // free the allocated memory
        fclose(page_file);    // close the file to prevent resource leaks
        return RC_WRITE_FAILED;
    }

    // if everything succeeds, cleanup the allocated memory and return ok
    free(empty_page);
    fclose(page_file);

    return RC_OK;
}


// open an existing pagefile
extern RC openPageFile (char *fileName, SM_FileHandle *fHandle)
{
    //open the file in read binary plus mode. "rb+" allows reading an existing binary file.
     FILE *page_file = fopen(fileName, "rb+");

    if(page_file == NULL)
    {
        return RC_FILE_NOT_FOUND;   //if file opening is failed
    }

    // to calculate file size we followed below steps:
    fseek(page_file, 0, SEEK_END);  //use fseek to move to the end of file
    long file_end_position = ftell(page_file);  //ftell gives the current position (in this case, its the end position)
    fseek(page_file, 0, SEEK_SET);   //move back to initial position of file

    // total number of pages that are stored on file can calculated by didviding file end position by page size
    int total_no_of_pages = file_end_position/PAGE_SIZE;

    // store the filename in filehandle
    fHandle->fileName = fileName;

    //store the total number of pages in file to filehandle
    fHandle->totalNumPages = total_no_of_pages;

    //set the current position to first page
    fHandle->curPagePos = 0;

    //store the any additional information about file that might be needed
    fHandle->mgmtInfo = page_file;

    return RC_OK;
}


//close an open page file
extern RC closePageFile (SM_FileHandle *fHandle)
{
    // check if filehandle is valid to prevent operations on uninitialized handle
    if(fHandle == NULL)
    {
        //file handle = null indicates that file handle is not valid
        return RC_FILE_HANDLE_NOT_INIT;
    }

    // if file handle is initilaized
    // cast the file handle mgmtinfo(additional file info) to a file pointer
    FILE *page_file = (FILE *)fHandle->mgmtInfo;

    // if the file pointer is not null then close the file
    if (page_file != NULL) {
        fclose(page_file);
    }

    // after closing the file, set all fields of file handle to initial values
    // to ensure this file handle cannot be used without intializing it again
    fHandle->fileName = NULL;
    fHandle->totalNumPages = 0;
    fHandle->curPagePos = 0;
    fHandle->mgmtInfo = NULL;

    //return ok , if all the operations are successful
    return RC_OK;
}


// destroy/delete a page file
extern RC destroyPageFile (char *fileName)
{
    //check if filename is valid. If not, return error 
    if(fileName == NULL)
        return RC_FILE_NOT_FOUND;

    // if filename exists, then try to remove the file
    if (remove(fileName) != 0)     // remove() returns 0 if successful, returns non zero if failed
    {
        // If remove fails, return an error
        return RC_FILE_NOT_FOUND;
    }

    // if remove operation is successful , return ok
    return RC_OK;
}


// read the block at pageNum position from a file and store its content
extern RC readBlock (int pageNum, SM_FileHandle *fHandle, SM_PageHandle memPage)
{
    // check if filehandle is valid to prevent operations on uninitialized handle
    if(fHandle == NULL)
    {
        //file handle = null indicates that file handle is not valid
        return RC_FILE_HANDLE_NOT_INIT;
    }

    //retrieve file pointer from filehandle's management info
    FILE *page_file = (FILE *)fHandle->mgmtInfo;
    
    //check if the file pointer is valid
    if(page_file == NULL)
    {
        return RC_FILE_NOT_FOUND;   // if not valid
    }

    //check if pageNum is valid. It should be >= 0 and less than filehandle's total number of pages
    if(pageNum < 0 || pageNum >= fHandle->totalNumPages)
        return RC_READ_NON_EXISTING_PAGE;

    //move the filepointer to the start position of requested page (requested page position = pageNum * pagesize)
    if(fseek(page_file, pageNum * PAGE_SIZE, SEEK_SET) != 0)
    {   
        return RC_READ_NON_EXISTING_PAGE;   //if fseek operation failed
    }

    //read page content and store it into memeory pointed my memPage
    size_t read_bytes_size = fread(memPage, sizeof(char), PAGE_SIZE, page_file);

    //check if correct number of bytes are read (if read_bytes size is equal to page size)
    if(read_bytes_size != PAGE_SIZE)
    {
        return RC_READ_NON_EXISTING_PAGE;
    }

    //update current page position in the file handle after successful read operation
    fHandle->curPagePos = pageNum;
    return RC_OK;
}


//returns an int, which is current block page position
extern int getBlockPos (SM_FileHandle *fHandle)
{
    // check if filehandle is valid to prevent operations on uninitialized handle
    if(fHandle == NULL || fHandle->mgmtInfo == NULL)
    {
        //file handle = null indicates that file handle is not valid
        return -1;
    }

    return fHandle->curPagePos;
}

// read the first page of file
extern RC readFirstBlock (SM_FileHandle *fHandle, SM_PageHandle memPage)
{
    int firstPageNum = 0; //indicates first page
    //pass this pageNum as input parameter to readBlock() function
    return readBlock(firstPageNum, fHandle, memPage);
}


//read the previous page of file handles curPagePos
extern RC readPreviousBlock (SM_FileHandle *fHandle, SM_PageHandle memPage)
{
    // check if filehandle is valid to prevent operations on uninitialized handle
    if(fHandle == NULL)
    {
        //file handle = null indicates that file handle is not valid
        return RC_FILE_HANDLE_NOT_INIT;
    }
    int prevPageNum = fHandle->curPagePos - 1;

    //pass this pageNum as input parameter to readBlock() function
    return readBlock(prevPageNum, fHandle, memPage);
}


//read the current page 
extern RC readCurrentBlock (SM_FileHandle *fHandle, SM_PageHandle memPage)
{
    // check if filehandle is valid to prevent operations on uninitialized handle
    if(fHandle == NULL)
    {
        //file handle = null indicates that file handle is not valid
        return RC_FILE_HANDLE_NOT_INIT;
    }
    int currPageNum = fHandle->curPagePos;

    //pass this pageNum as input parameter to readBlock() function
    return readBlock(currPageNum, fHandle, memPage);
}


//read the next page of file handles curPagePos
extern RC readNextBlock (SM_FileHandle *fHandle, SM_PageHandle memPage)
{
    // check if filehandle is valid to prevent operations on uninitialized handle
    if(fHandle == NULL)
    {
        //file handle = null indicates that file handle is not valid
        return RC_FILE_HANDLE_NOT_INIT;
    }
    int currPageNum = fHandle->curPagePos + 1;

    //pass this pageNum as input parameter to readBlock() function
    return readBlock(currPageNum, fHandle, memPage);
}


//read the last page of file handles curPagePos
extern RC readLastBlock (SM_FileHandle *fHandle, SM_PageHandle memPage)
{
    // check if filehandle is valid to prevent operations on uninitialized handle
    if(fHandle == NULL)
    {
        //file handle = null indicates that file handle is not valid
        return RC_FILE_HANDLE_NOT_INIT;
    }
    int lastPageNum = fHandle->totalNumPages - 1;
    //pass this pageNum as input parameter to readBlock() function
    return readBlock(lastPageNum, fHandle, memPage);
}

// write a page to disk
extern RC writeBlock(int pageNum, SM_FileHandle *fHandle, SM_PageHandle memPage) 
{
    // Check if file handle is valid
    if (fHandle == NULL || fHandle->mgmtInfo == NULL || fHandle->fileName == NULL) 
    {
        return RC_FILE_NOT_FOUND;   //if file handle is invalid
    }

    // Check if the given pageNum is valid
    if (pageNum < 0 || pageNum >= fHandle->totalNumPages) 
    {
        return RC_WRITE_FAILED;    // Writing to a non-existing page is considered a failure
    }

    //retrieve file pointer from filehandle's management info
    FILE *page_file = (FILE *)fHandle->mgmtInfo;

    // check if the file is opened
    if (page_file == NULL)
        return RC_FILE_NOT_FOUND;
    
    //move the filepointer to the start position of requested page (requested page position = pageNum * pagesize)
    if(fseek(page_file, pageNum * PAGE_SIZE, SEEK_SET) != 0)
    {
        return RC_READ_NON_EXISTING_PAGE;   //if fseek operation failed
    }

    // Write the block from the memory buffer to the file
    size_t write_bytes_size = fwrite(memPage, sizeof(char), PAGE_SIZE, page_file);

    //check if correct number of bytes are written (if write_bytes_size size is equal to page size)
    if (write_bytes_size != PAGE_SIZE) 
    {
        fclose(page_file);
        return RC_WRITE_FAILED;
    }

    // Update the current page position in file handle
    fHandle->curPagePos = pageNum;
    // Return success
    return RC_OK;
}


//write the current page
extern RC writeCurrentBlock(SM_FileHandle *fHandle, SM_PageHandle memPage) 
{
    // Check if file handle is valid
    if (fHandle == NULL || fHandle->mgmtInfo == NULL || fHandle->fileName == NULL)
        return RC_FILE_NOT_FOUND;

    // Get the current page position
    int curPage = fHandle->curPagePos;

    // Call the writeBlock function to write the current block
    return writeBlock(curPage, fHandle, memPage);
}


//append a new last page that is filled with zero bytes and increase total number of pages by one.
extern RC appendEmptyBlock(SM_FileHandle *fHandle) 
{

    // Check if file handle is valid
    if (fHandle == NULL || fHandle->mgmtInfo == NULL || fHandle->fileName == NULL)
        return RC_FILE_NOT_FOUND;

    //retrieve file pointer from filehandle's management info
    FILE *page_file = (FILE *)fHandle->mgmtInfo;
    if (page_file == NULL)
        return RC_FILE_NOT_FOUND;

    // create an empty page by allocating memory for one page (initializes memory with zero('\0') bytes)
    SM_PageHandle emptyBlock = (SM_PageHandle)calloc(PAGE_SIZE, sizeof(char));

    if(emptyBlock == NULL)
    {
        // if failed to allocate memory
        fclose(page_file);      // close the file to prevent resource leaks
        return RC_WRITE_FAILED; 
    }

	//move to the end of file
    fseek(page_file, 0, SEEK_END);
    // Write the empty block to the file
    fwrite(emptyBlock, sizeof(char), PAGE_SIZE, page_file);


    // Update total number of pages and current page position in file handle
    // total number of pages that are stored on file is increased by 1
    fHandle->totalNumPages++;

    // Return success
    return RC_OK;
}

//ensures the file has at least the specified number of pages. 
//If the file already has enough pages, it does nothing. 
//If not, it appends empty pages until the desired capacity is reached.
extern RC ensureCapacity(int numberOfPages, SM_FileHandle *fHandle) 
{
    // Check if file handle is valid
    if (fHandle == NULL || fHandle->mgmtInfo == NULL || fHandle->fileName == NULL)
        return RC_FILE_NOT_FOUND;

    // Calculate the difference between the requested number of pages and the current total number of pages
    int pagesToAdd = numberOfPages - fHandle->totalNumPages;

    // If no additional pages are needed, return success
    if (pagesToAdd <= 0)
        return RC_OK;

    // Add the required number of empty blocks to meet the capacity
    while (pagesToAdd > 0) {
        RC appendStatus = appendEmptyBlock(fHandle);
        if (appendStatus != RC_OK)
            return appendStatus;

        pagesToAdd--;
    }

    // Return success
    return RC_OK;
}