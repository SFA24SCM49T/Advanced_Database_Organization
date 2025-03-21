#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "storage_mgr.h"
#include "dberror.h"
#include "test_helper.h"

// test name
char *testName;

/* test output files */
#define TESTPF "test_pagefile.bin"

/* prototypes for test functions */
static void extraTestCases(void);

/* main function running all tests */
int main(void) {
    testName = "";
 
    initStorageManager();
    extraTestCases();

    return 0;
}

void extraTestCases(void)
{
    SM_FileHandle fh;
    SM_PageHandle ph;
    int i;

    testName = "test single page content";

    ph = (SM_PageHandle) malloc(PAGE_SIZE);

    // create a new page file
    TEST_CHECK(createPageFile (TESTPF));
    TEST_CHECK(openPageFile (TESTPF, &fh));
    printf("created and opened file\n");

    //Append 3 empty pages
    for (i = 0; i < 3; i++)
       TEST_CHECK(appendEmptyBlock(&fh));

    ASSERT_TRUE((fh.totalNumPages == 4), "file should have 4 pages");

    // Ensure capacity of 6 pages
    TEST_CHECK(ensureCapacity(6, &fh));
    ASSERT_TRUE((fh.totalNumPages == 6), "file should have 6 pages after ensuring capacity");


    // Write data to pages
    for (i = 0; i < 6; i++) {
        //fill the entire page with i + '0' characters
        for (int j=0; j < PAGE_SIZE; j++) {
            ph[j] = i + '0';
        }
        TEST_CHECK(writeBlock(i, &fh, ph));
    }

    // Read and verify data from pages
    for (i = 0; i < 6; i++) {
        TEST_CHECK(readBlock(i, &fh, ph));
        ASSERT_TRUE((ph[0] == i + '0'), "character in page read from disk is the one we expected.");
    }

     // Test relative read methods
    TEST_CHECK(readFirstBlock(&fh, ph));
    ASSERT_TRUE((ph[0] == '0'), "first block should contain '0'");

    TEST_CHECK(readLastBlock(&fh, ph));
    ASSERT_TRUE((ph[0] == '5'), "last block should contain '5'");

    TEST_CHECK(readPreviousBlock(&fh, ph));
    ASSERT_TRUE((ph[0] == '4'), "previous block should contain '4'");

    TEST_CHECK(readCurrentBlock(&fh, ph));
    ASSERT_TRUE((ph[0] == '4'), "current block should still contain '4'");

    TEST_CHECK(readNextBlock(&fh, ph));
    ASSERT_TRUE((ph[0] == '5'), "next block should contain '5'");

    TEST_CHECK(closePageFile(&fh));
    TEST_CHECK(destroyPageFile(TESTPF));

    free(ph);
    TEST_DONE();
}