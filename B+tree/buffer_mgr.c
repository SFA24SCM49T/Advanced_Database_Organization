#include <stdlib.h>
#include <string.h>
#include "buffer_mgr.h"
#include "storage_mgr.h"

#define RC_ERROR 200
#define RC_PINNED_PAGES_IN_BUFFER 201

typedef struct PageFrame 
{
    SM_PageHandle data; // Pointer to page data
    PageNumber pageNum; // Page number
    bool isDirty;       // Flag indicating if the page is dirty
    int fixCount;       // Number of clients using this page
    int lastUsed;       // Timestamp of last use (for FIFO)
    int lruCount;       // Counter for LRU strategy
} PageFrame;

// Define the structure for management information
typedef struct MgmtInfo {
    PageFrame *frames;  // Array of page frames
    int readIO;         // Counter for read I/O operations
    int writeIO;        // Counter for write I/O operations
    int cacheHits;      // Counter for cache hits
} MgmtInfo;

// Function to find a frame to replace based on the replacement strategy
extern int findFrameToReplace(BM_BufferPool *bm) 
{
    MgmtInfo *mgmtData = (MgmtInfo *)bm->mgmtData;
    PageFrame *frames = (PageFrame *)mgmtData->frames;
    switch (bm->strategy) {
        case RS_FIFO:
        // FIFO strategy: find the frame with the lowest lastUsed value
            {
                int lastUsed = getNumReadIO(bm);
                int lastUsedIndex = -1;

                for (int i = 0; i < bm->numPages; i++) 
                {
                    if (frames[i].fixCount == 0) 
                    {
                        if (frames[i].lastUsed < lastUsed) 
                        {
                            lastUsed = frames[i].lastUsed;
                            lastUsedIndex = i;
                        }
                    }
                }

                return lastUsedIndex;
            }
        case RS_LRU:
        // LRU strategy: find the frame with the lowest lruCount
            {
                int lruCount = mgmtData->cacheHits;
                int leastUsedIndex = -1;

                for (int i = 0; i < bm->numPages; i++) 
                {
                    if (frames[i].fixCount == 0) 
                    {
                        if (frames[i].lruCount < lruCount) 
                        {
                            lruCount = frames[i].lruCount;
                            leastUsedIndex = i;
                        }
                    }
                }

                return leastUsedIndex;
            }
        default:
            return -1;
    }
    return -1;
}

// Function to initialize the buffer pool
extern RC initBufferPool(BM_BufferPool *const bm, const char *const pageFileName, 
                  const int numPages, ReplacementStrategy strategy, void *stratData) 
{
    // Check for invalid input parameters
    if (bm == NULL || pageFileName == NULL || numPages <= 0)
        return RC_ERROR;

    // Set buffer pool properties
    bm->pageFile = (char *)pageFileName;
    bm->numPages = numPages;
    bm->strategy = strategy;

    // Allocate memory for page frames
    PageFrame *frames = calloc(numPages, sizeof(PageFrame));
    if (frames == NULL)
        return RC_ERROR;

    // Initialize page frames
    for (int i = 0; i < numPages; i++) 
    {
        frames[i].pageNum = NO_PAGE;
        frames[i].data = NULL;
        frames[i].isDirty = false;
        frames[i].fixCount = 0;
        frames[i].lastUsed = 0;
    }

    // Initialize management data
    MgmtInfo *mgmtData = malloc(sizeof(MgmtInfo));
    mgmtData->frames = frames;
    mgmtData->readIO = 0;
    mgmtData->writeIO = 0;
    mgmtData->cacheHits = 0;

    bm->mgmtData = mgmtData;

    return RC_OK;
}

// Function to shut down the buffer pool
extern RC shutdownBufferPool(BM_BufferPool *const bm) 
{
    // Check for invalid input
    if (bm == NULL || bm->mgmtData == NULL) 
    {
        return RC_ERROR;
    }

    MgmtInfo *mgmtData = (MgmtInfo *)bm->mgmtData;
    PageFrame *frames = (PageFrame *)mgmtData->frames;

    // Force flush all pages
    RC rc = forceFlushPool(bm);
    if (rc != RC_OK) 
    {
        return rc;
    }

    // Check for pinned pages and free memory
    for (int i = 0; i < bm->numPages; i++) 
    {
        if (frames[i].fixCount != 0) 
        {
            return RC_PINNED_PAGES_IN_BUFFER; // Error: there are still pinned pages
        }
        free(frames[i].data);
    }
    free(frames);

    bm->mgmtData = NULL;

    return RC_OK;
}

// Force flush all dirty pages in the buffer pool
extern RC forceFlushPool(BM_BufferPool *const bm) 
{
    // Check for invalid input
    if (bm == NULL || bm->mgmtData == NULL) 
    {
        return RC_ERROR;
    }

    MgmtInfo *mgmtData = (MgmtInfo *)bm->mgmtData;
    PageFrame *frames = (PageFrame *)mgmtData->frames;

    // Iterate through all frames and flush dirty pages
    for (int i = 0; i < bm->numPages; i++) 
    {
        if (frames[i].isDirty && frames[i].fixCount == 0) 
        {
            SM_FileHandle fh;
            RC rc = openPageFile(bm->pageFile, &fh);

            rc = writeBlock(frames[i].pageNum, &fh, frames[i].data);
            mgmtData->writeIO++;
            frames[i].isDirty = false;
        }
    }
    return RC_OK;
}

// Function to mark a page as dirty
extern RC markDirty(BM_BufferPool *const bm, BM_PageHandle *const page) 
{
    // Check for invalid input
    if (bm == NULL || bm->mgmtData == NULL || page == NULL) 
    {
        return RC_ERROR;
    }

    MgmtInfo *mgmtData = (MgmtInfo *)bm->mgmtData;
    PageFrame *frames = (PageFrame *)mgmtData->frames;

    // Find the page in the buffer and mark it as dirty
    for (int i = 0; i < bm->numPages; i++) 
    {
        if (frames[i].pageNum == page->pageNum) 
        {
            frames[i].isDirty = true;
            return RC_OK;
        }
    }
    return RC_ERROR;
}

// Function to unpin a page
extern RC unpinPage(BM_BufferPool *const bm, BM_PageHandle *const page) 
{
    // Check for invalid input
    if (bm == NULL || bm->mgmtData == NULL || page == NULL) 
    {
        return RC_ERROR;
    }

    MgmtInfo *mgmtData = (MgmtInfo *)bm->mgmtData;
    PageFrame *frames = (PageFrame *)mgmtData->frames;

    // Find the page in the buffer and decrement its fix count
    for (int i = 0; i < bm->numPages; i++) 
    {
        if (frames[i].pageNum == page->pageNum) 
        {
            if (frames[i].fixCount > 0) 
            {
                frames[i].fixCount--;
                return RC_OK;
            } 
            else 
            {
                return RC_ERROR;  // Page is already unpinned, this might be an error condition
            }
        }
    }
    return RC_ERROR;
}

// Function to force a page to disk
extern RC forcePage(BM_BufferPool *const bm, BM_PageHandle *const page) 
{
    // Check for invalid input
    if (bm == NULL || bm->mgmtData == NULL || page == NULL) {
        return RC_ERROR;
    }

    MgmtInfo *mgmtData = (MgmtInfo *)bm->mgmtData;
    PageFrame *frames = (PageFrame *)mgmtData->frames;

    //Open the page file
    SM_FileHandle fh;
    RC rc = openPageFile(bm->pageFile, &fh);
    if (rc != RC_OK)
        return rc;
    
    // Find the page in the buffer and write it to disk
    for (int i = 0; i < bm->numPages; i++) 
    {
        if (frames[i].pageNum == page->pageNum) 
        {
            rc = writeBlock(page->pageNum, &fh, page->data);
            if (rc == RC_OK) 
            {
                frames[i].isDirty = false;
            }
            mgmtData->writeIO++;
            closePageFile(&fh);
            return rc;
        }
    }
    return RC_ERROR;
}

// Funtion to Pin page
extern RC pinPage(BM_BufferPool *const bm, BM_PageHandle *const page, const PageNumber pageNum) 
{
    // Check for invalid input
    if (bm == NULL || bm->mgmtData == NULL || page == NULL) {
        return RC_ERROR;
    }

    MgmtInfo *mgmtData = (MgmtInfo *)bm->mgmtData;
    PageFrame *frames = (PageFrame *)mgmtData->frames;

    // If the page is already in the buffer, update its metadata
    for (int i = 0; i < bm->numPages; i++) 
    {
        if (frames[i].pageNum == pageNum) 
        {
            page->pageNum = pageNum;
            page->data = frames[i].data;
            frames[i].fixCount++;
            mgmtData->cacheHits++;
            if(bm->strategy == RS_LRU)
            {
                frames[i].lruCount = mgmtData->cacheHits; // Update for LRU
            }
            return RC_OK;
        }
    }

    // Page not in buffer pool, find an empty frame or use replacement strategy
    int frameNum = -1;
    for (int i = 0; i < bm->numPages; i++) 
    {
        if (frames[i].pageNum == NO_PAGE) 
        {
            frameNum = i;
            break;
        }
    }

    // If no empty frame, use replacement strategy to Find a frame to replace
    if (frameNum == -1)
    {
        frameNum = findFrameToReplace(bm);
    }
    if (frameNum == -1) {
        return RC_ERROR;
    }
    
    // If the frame to be replaced is dirty, write it to disk
    if (frames[frameNum].isDirty) 
    {
        RC rc = forcePage(bm, &(BM_PageHandle){.pageNum = frames[frameNum].pageNum, .data = frames[frameNum].data});
        if (rc != RC_OK) return rc;
    }

    // Read the new page from disk
    SM_FileHandle fh;
    RC rc = openPageFile(bm->pageFile, &fh);
    if (rc != RC_OK) return rc;

    rc = ensureCapacity(pageNum + 1, &fh);
    if (rc != RC_OK) {
        closePageFile(&fh);
        return rc;
    }
    // Allocate memory for the page data and read the page from disk.
    frames[frameNum].data = (SM_PageHandle) malloc(PAGE_SIZE);
    rc = readBlock(pageNum, &fh, frames[frameNum].data);
    closePageFile(&fh);
    if (rc != RC_OK) return rc;

    mgmtData->readIO++;
    mgmtData->cacheHits++;

    // Update frame information
    frames[frameNum].pageNum = pageNum;
    frames[frameNum].isDirty = false;
    frames[frameNum].fixCount = 1;
    frames[frameNum].lastUsed = getNumReadIO(bm);
    if(bm->strategy == RS_LRU)
    {
        frames[frameNum].lruCount = mgmtData->cacheHits; // Update for LRU
    }

    // Set page handle information
    page->pageNum = pageNum;
    page->data = frames[frameNum].data;

    return RC_OK;
}


// Get frame contents
extern PageNumber *getFrameContents(BM_BufferPool *const bm) 
{
    // Check for invalid input
    if (bm == NULL || bm->mgmtData == NULL) {
        return NULL;
    }

    MgmtInfo *mgmtData = (MgmtInfo *)bm->mgmtData;
    PageFrame *frames = (PageFrame *)mgmtData->frames;;
    PageNumber *frameContents = malloc(sizeof(PageNumber) * bm->numPages);
    if (frameContents == NULL) {
        return NULL;
    }

    // Copy page numbers from frames to contents array
    for (int i = 0; i < bm->numPages; i++) {
        frameContents[i] =  (frames[i].pageNum != -1) ? frames[i].pageNum : NO_PAGE;
    }

    return frameContents;
}

// Get dirty flags
extern bool *getDirtyFlags(BM_BufferPool *const bm) 
{
    // Check for invalid input
    if (bm == NULL || bm->mgmtData == NULL) {
        return NULL;
    }

    MgmtInfo *mgmtData = (MgmtInfo *)bm->mgmtData;
    PageFrame *frames = (PageFrame *)mgmtData->frames;
    bool *dirtyFlags = (bool *)malloc(sizeof(bool) * bm->numPages);
    if (dirtyFlags == NULL) {
        return NULL;
    }

    // Copy dirty flags from frames to flags array, empty pages are considered clean
    for (int i = 0; i < bm->numPages; i++) {
        dirtyFlags[i] = frames[i].isDirty;
    }
    return dirtyFlags;
}

// Get fix counts
extern int *getFixCounts(BM_BufferPool *const bm) 
{
    // Check for invalid input
    if (bm == NULL || bm->mgmtData == NULL) {
        return NULL;
    }

    MgmtInfo *mgmtData = (MgmtInfo *)bm->mgmtData;
    PageFrame *frames = (PageFrame *)mgmtData->frames;
    int *fixCounts = (int *)malloc(sizeof(int) * bm->numPages);
    if (fixCounts == NULL) {
        return NULL;
    }

    // Copy fix counts from frames to counts array
    for (int i = 0; i < bm->numPages; i++) {
        if (frames[i].pageNum == NO_PAGE) 
        {
            fixCounts[i] = 0;  // Empty page frame
        } 
        else 
        {
            fixCounts[i] = frames[i].fixCount;
        }
    }
    return fixCounts;
}

// Get number of read IOs
extern int getNumReadIO(BM_BufferPool *const bm) 
{
    // Check for invalid input
    if (bm == NULL || bm->mgmtData == NULL) {
        return -1;
    }

    MgmtInfo *mgmtData = (MgmtInfo *)bm->mgmtData;
    return mgmtData->readIO;
}

// Get number of write IOs
extern int getNumWriteIO(BM_BufferPool *const bm) 
{
    // Check for invalid input
    if (bm == NULL || bm->mgmtData == NULL) {
        return -1;
    }
    MgmtInfo *mgmtData = (MgmtInfo *)bm->mgmtData;
    return mgmtData->writeIO;
}