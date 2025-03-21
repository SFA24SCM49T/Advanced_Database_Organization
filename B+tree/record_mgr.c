#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "record_mgr.h"
#include "buffer_mgr.h"
#include "storage_mgr.h"

#define ATTRIBUTE_SIZE 15
#define RC_SCAN_CONDITION_NOT_FOUND 201
#define RC_RM_NO_TUPLE_WITH_GIVEN_RID 202

// Structure to manage record operations
typedef struct RecordManager {
    BM_PageHandle pageHandle; // Handle for buffer pool pages
    BM_BufferPool bufferPool; // Buffer pool for managing pages
    int tuplesCount;          // Count of tuples in the table
    int freePage;             // First free page for inserting new records
    int scanCount;            // Count of scanned records
    RID recordID;             // Record ID for current operation
    Expr *condition;          // Condition for scan operations
} RecordManager;

// Global pointer to RecordManager
RecordManager *recordManager = NULL;

// Helper function to find a free slot in a page
int findFreeSlot(const char *data, int recordSize)
{
    const int totalSlots = PAGE_SIZE / recordSize;
    
    for (int i = 0; i < totalSlots; i++) {
        // Check if the slot is not occupied ('+' indicates occupied)
        if (data[i * recordSize] != '+') {
            return i;
        }
    }
    
    return -1;  // No free slot found
}

// Helper function to calculate attribute offset in a record
RC attrOffset(Schema *schema, int attrNum, int *result) {
    int i;
    *result = 1;  // Start from 1 to account for tombstone
    for (i = 0; i < attrNum; i++) {
        // Add size based on data type
        switch (schema->dataTypes[i]) {
            case DT_STRING:
                *result += schema->typeLength[i];
                break;
            case DT_INT:
                *result += sizeof(int);
                break;
            case DT_FLOAT:
                *result += sizeof(float);
                break;
            case DT_BOOL:
                *result += sizeof(bool);
                break;
        }
    }
    return RC_OK;
}

// Initialize the record manager
RC initRecordManager(void *mgmtData) {    
	initStorageManager();  // Initialize the storage manager
    return RC_OK;
}

// Shutdown the record manager
RC shutdownRecordManager() {
    recordManager = NULL;
	free(recordManager);  // Free allocated memory
	return RC_OK;
}

// Create a new table
extern RC createTable(char *name, Schema *schema)
{
    // Allocate memory for RecordManager and initialize buffer pool
    recordManager = (RecordManager*) malloc(sizeof(RecordManager));
    // Initialize record manager and buffer pool
    initBufferPool(&recordManager->bufferPool, name, 100, RS_LRU, NULL);

    // Prepare page data
    char data[PAGE_SIZE];
    char *schema_str = data;
    SM_FileHandle fh;

    // Write schema information to the page
    *(int*)schema_str = 0;                // Tuples count
    schema_str += sizeof(int);
    *(int*)schema_str = 1;                // Free page
    schema_str += sizeof(int);
    *(int*)schema_str = schema->numAttr;  // Number of attributes
    schema_str += sizeof(int);
    *(int*)schema_str = schema->keySize;  // Key size
    schema_str += sizeof(int);

    // Write attribute information
    for (int i = 0; i < schema->numAttr; i++) {
        strncpy(schema_str, schema->attrNames[i], ATTRIBUTE_SIZE);
        schema_str += ATTRIBUTE_SIZE;
        *(int*)schema_str = (int)schema->dataTypes[i];
        schema_str += sizeof(int);
        *(int*)schema_str = (int)schema->typeLength[i];
        schema_str += sizeof(int);
    }
	// Create and write to page file
	RC status = createPageFile(name);
    if (status != RC_OK) return status;

    status = openPageFile(name, &fh);
    if (status != RC_OK) return status;

	status = writeBlock(0, &fh, data);
    if (status != RC_OK) return status;

	status = closePageFile(&fh);
    if (status != RC_OK) return status;

    return RC_OK;
}

// Open an existing table
extern RC openTable(RM_TableData *tableData, char *tableName) {
    SM_PageHandle pageContent;  
    int attrCount, i;
    Schema *tableSchema;
	
    // Set up table data
    tableData->mgmtData = recordManager;
    tableData->name = tableName;

    // Read schema information from the first page
    pinPage(&recordManager->bufferPool, &recordManager->pageHandle, 0);
    
    pageContent = (char*) recordManager->pageHandle.data;

    // Read tuple count and free page information
    recordManager->tuplesCount = *(int*)pageContent;
    pageContent += sizeof(int);
    recordManager->freePage = *(int*)pageContent;
    pageContent += sizeof(int);
    
    // Read the number of attributes
    attrCount = *(int*)pageContent;
    pageContent += sizeof(int);

	// Allocate memory for the schema
	tableSchema = (Schema*) malloc(sizeof(Schema));
    tableSchema->numAttr = attrCount;
    tableSchema->attrNames = (char**) malloc(sizeof(char*) * attrCount);
    tableSchema->dataTypes = (DataType*) malloc(sizeof(DataType) * attrCount);
    tableSchema->typeLength = (int*) malloc(sizeof(int) * attrCount);
    
    // Read attribute information for each attribute
    for (i = 0; i < attrCount; i++) {
        tableSchema->attrNames[i] = (char*) malloc(ATTRIBUTE_SIZE);  
        strncpy(tableSchema->attrNames[i], pageContent, ATTRIBUTE_SIZE);  
        pageContent += ATTRIBUTE_SIZE;
        
        tableSchema->dataTypes[i] = *(int*) pageContent;  
        pageContent += sizeof(int);
        
        tableSchema->typeLength[i] = *(int*) pageContent;  
        pageContent += sizeof(int);
    }
    
    // Set the schema for the table
    tableData->schema = tableSchema;
    
    // Unpin the page and force it to disk
    unpinPage(&recordManager->bufferPool, &recordManager->pageHandle);
    forcePage(&recordManager->bufferPool, &recordManager->pageHandle);
    return RC_OK;
}

// Close a table
RC closeTable(RM_TableData *rel) {
    RecordManager *mgmtData = (RecordManager *)rel->mgmtData;
    // Shutdown the buffer pool associated with this table
    shutdownBufferPool(&mgmtData->bufferPool);
    return RC_OK;
}

// Delete a table
RC deleteTable(char *name) {
    // Simply destroy the page file associated with the table
    return destroyPageFile(name);
}

// Get the number of tuples in a table
int getNumTuples(RM_TableData *rel) {
    // Return the tuple count from the record manager
    return ((RecordManager *)rel->mgmtData)->tuplesCount;
}

// Insert a new record into the table
RC insertRecord(RM_TableData *rel, Record *record) {
    RecordManager *recMgr = rel->mgmtData;
    // Find a free page and slot
    RID *recID = &record->id;
	char *pageDataPtr;
    char *slotDataPtr;
    int recByteSize = getRecordSize(rel->schema);

    // Start with the first free page
    recID->page = recMgr->freePage;
    pinPage(&recMgr->bufferPool, &recMgr->pageHandle, recID->page);
    pageDataPtr = recMgr->pageHandle.data;

    // Find a free slot in the current page
    recID->slot = findFreeSlot(pageDataPtr, recByteSize);

    // If no free slot is found, move to the next page
    while (recID->slot == -1) {
        unpinPage(&recMgr->bufferPool, &recMgr->pageHandle);  
        recID->page++; 
        pinPage(&recMgr->bufferPool, &recMgr->pageHandle, recID->page); 
        pageDataPtr = recMgr->pageHandle.data; 
        recID->slot = findFreeSlot(pageDataPtr, recByteSize); 
    }

    // Insert the record
    markDirty(&recMgr->bufferPool, &recMgr->pageHandle);
    // Insert the record data
    slotDataPtr = pageDataPtr;
    slotDataPtr = slotDataPtr + (recID->slot * recByteSize);
    *slotDataPtr = '+'; // Mark slot as occupied

    memcpy(++slotDataPtr, record->data + 1, recByteSize - 1);
    // Unpin the page and update tuple count
    unpinPage(&recMgr->bufferPool, &recMgr->pageHandle);
    recMgr->tuplesCount++;
    // Pin the first page (metadata page)
    pinPage(&recMgr->bufferPool, &recMgr->pageHandle, 0);
    return RC_OK;
}

// Delete a record from the table
RC deleteRecord(RM_TableData *rel, RID id) {
    RecordManager *mgmtData = (RecordManager *)rel->mgmtData;
    int recordSize = getRecordSize(rel->schema);
    // Pin the page containing the record
    pinPage(&mgmtData->bufferPool, &mgmtData->pageHandle, id.page);
    mgmtData->freePage = id.page;

    // Mark the record slot as deleted
    char *target = mgmtData->pageHandle.data;
	target = target + (id.slot * recordSize);
    *target = '-'; // Mark as deleted
    
    // Mark the page as dirty and unpin it
    markDirty(&mgmtData->bufferPool, &mgmtData->pageHandle);
    unpinPage(&mgmtData->bufferPool, &mgmtData->pageHandle);
    return RC_OK;
}

// Update a record in the table
RC updateRecord(RM_TableData *rel, Record *record) {
    RecordManager *mgmtData = rel->mgmtData;
    int recByteSize = getRecordSize(rel->schema);
    // Pin the page containing the record
    pinPage(&mgmtData->bufferPool, &mgmtData->pageHandle, record->id.page);
    RID recordID = record->id;
    // Update the record data
    char *destination;
	destination = mgmtData->pageHandle.data;
	destination = destination + (record->id.slot * recByteSize);
	*destination = '+';  // Mark as occupied
    memcpy(++destination, record->data + 1, recByteSize - 1);
    
    // Mark the page as dirty and unpin it
    markDirty(&mgmtData->bufferPool, &mgmtData->pageHandle);
    unpinPage(&mgmtData->bufferPool, &mgmtData->pageHandle);
    return RC_OK;
}

// Retrieve a record from the table
RC getRecord(RM_TableData *rel, RID id, Record *record) {
    RecordManager *mgmtData = rel->mgmtData;
    int recordSize = getRecordSize(rel->schema);
    
    // Pin the page containing the record
    pinPage(&mgmtData->bufferPool, &mgmtData->pageHandle, id.page);
	char *source = mgmtData->pageHandle.data;
    source = source + (id.slot * recordSize);
    // Check if the record exists
    if (*source != '+')
        return RC_RM_NO_TUPLE_WITH_GIVEN_RID;
    // Copy the record data
    record->id = id;
	char *recorddata = record->data;
    memcpy(++recorddata, source + 1, recordSize - 1);
    // Unpin the page
    unpinPage(&mgmtData->bufferPool, &mgmtData->pageHandle);
    return RC_OK;
}

// Free the memory allocated for a schema
extern RC freeSchema(Schema *schema) {
    // Free memory for each attribute name
    for (int i = 0; i < schema->numAttr; i++) {
        free(schema->attrNames[i]);
    }
    // Free memory for arrays in the schema
    free(schema->attrNames);
    free(schema->dataTypes);
    free(schema->typeLength);
    free(schema->keyAttrs);
    free(schema);  // Free the schema itself
    return RC_OK;
}

// Start a scan operation on the table
RC startScan(RM_TableData *rel, RM_ScanHandle *scan, Expr *cond) {
    // Check if a condition is provided
    if (cond == NULL) {
        return RC_SCAN_CONDITION_NOT_FOUND;
    }
    // Open the table for scanning
    openTable(rel, "ScanTable");
    // Allocate and initialize scan management data
    RecordManager *scanMgmtData;
    scanMgmtData = (RecordManager*)malloc(sizeof(RecordManager));
    scan->mgmtData = scanMgmtData;
    scanMgmtData->recordID.page = 1;
    scanMgmtData->recordID.slot = 0;
    scanMgmtData->scanCount = 0;
    scanMgmtData->condition = cond;
    // Set up relation data
    RecordManager *mgmtData;
    mgmtData = rel->mgmtData;
    mgmtData->tuplesCount = ATTRIBUTE_SIZE;
    scan->rel = rel;
    
    return RC_OK;
}

// Get the next record that satisfies the scan condition
RC next(RM_ScanHandle *scan, Record *record) {
    RecordManager *scanMgmtData = scan->mgmtData;
    RecordManager *rel = scan->rel->mgmtData;
    Schema *schema = scan->rel->schema;
    // Check if a condition is set
    if (scanMgmtData->condition == NULL) {
        return RC_SCAN_CONDITION_NOT_FOUND;
    }

    Value *conditionResult = (Value *) malloc(sizeof(Value));
    char *recordData;
    int recordSize = getRecordSize(schema);
    int totalSlots = PAGE_SIZE / recordSize;
    int currentScanCount = scanMgmtData->scanCount;
    int totalTuples = rel->tuplesCount;

    // Check if there are any tuples
    if (totalTuples == 0)
        return RC_RM_NO_MORE_TUPLES;
    // Scan through records
    while (currentScanCount <= totalTuples) {
        // Initialize or update record ID
        if (currentScanCount <= 0) {
            scanMgmtData->recordID.page = 1;
            scanMgmtData->recordID.slot = 0;
        } else {
            scanMgmtData->recordID.slot++;
            if (scanMgmtData->recordID.slot >= totalSlots) {
                scanMgmtData->recordID.slot = 0;
                scanMgmtData->recordID.page++;
            }
        }
        // Pin the page and get record data
        pinPage(&rel->bufferPool, &scanMgmtData->pageHandle, scanMgmtData->recordID.page);
        recordData = scanMgmtData->pageHandle.data;
        recordData = recordData + (scanMgmtData->recordID.slot * recordSize);
        record->id.page = scanMgmtData->recordID.page;
        // Set up record
        record->id.slot = scanMgmtData->recordID.slot;
        char *recordPointer = record->data;
        *recordPointer = '-';
        memcpy(++recordPointer, recordData + 1, recordSize - 1);
        // Update scan counters
        scanMgmtData->scanCount++;
        currentScanCount++;
        // Evaluate the condition
        evalExpr(record, schema, scanMgmtData->condition, &conditionResult);
        if (conditionResult->v.boolV == TRUE) {
            unpinPage(&rel->bufferPool, &scanMgmtData->pageHandle);
            return RC_OK;
        }
    }

    // No more tuples satisfy the condition
    unpinPage(&rel->bufferPool, &scanMgmtData->pageHandle);
    scanMgmtData->recordID.page = 1;
    scanMgmtData->recordID.slot = 0;
    scanMgmtData->scanCount = 0;
    return RC_RM_NO_MORE_TUPLES;
}

// Close the scan operation
RC closeScan(RM_ScanHandle *scan) {
    RecordManager *scanMgr = scan->mgmtData;
    RecordManager *tableMgr = scan->rel->mgmtData;
    // If scan was in progress, unpin the page
    if (scanMgr->scanCount > 0)
    {
        unpinPage(&tableMgr->bufferPool, &scanMgr->pageHandle);
	// Reset scan parameters
        scanMgr->scanCount = 0;
        scanMgr->recordID.page = 1;
        scanMgr->recordID.slot = 0;
    }
    // Free allocated memory
    free(scan->mgmtData);
    scan->mgmtData = NULL;
return RC_OK;
}

// Calculate the size of a record based on the schema
int getRecordSize(Schema *schema) {
    int size = 0;
    for (int i = 0; i < schema->numAttr; i++) {
        switch (schema->dataTypes[i]) {
            case DT_INT: size += sizeof(int); break;
            case DT_FLOAT: size += sizeof(float); break;
            case DT_BOOL: size += sizeof(bool); break;
            case DT_STRING: size += schema->typeLength[i]; break;
        }
    }
    return ++size; // Add 1 for the tombstone
}

// Create a new schema
Schema *createSchema(int numAttr, char **attrNames, DataType *dataTypes, int *typeLength, int keySize, int *keys) {
    Schema *tableSchema = (Schema *) malloc(sizeof(Schema));
	tableSchema->numAttr = numAttr;
	tableSchema->attrNames = attrNames;
	tableSchema->dataTypes = dataTypes;
	tableSchema->typeLength = typeLength;
	tableSchema->keySize = keySize;
	tableSchema->keyAttrs = keys;
	return tableSchema; 
}

// Create a new record
extern RC createRecord (Record **record, Schema *schema)
{
	Record *rec = (Record*) malloc(sizeof(Record));
	
	int recSize = getRecordSize(schema);
	rec->data = (char*) malloc(recSize);
    rec->id.page = rec->id.slot = -1;
	char *dataPtr = rec->data;
	*dataPtr = '-'; // Set tombstone
    *(++dataPtr) = '\0'; // Null-terminate
    *record = rec;
	return RC_OK;
}
// Free memory allocated for a record
extern RC freeRecord (Record *record)
{
	free(record);
	return RC_OK;
}

// Get the value of an attribute from a record
extern RC getAttr (Record *record, Schema *schema, int attrNum, Value **value)
{
	int attrOffsetValue = 0;
    // Calculate the offset of the attribute in the record
    attrOffset(schema, attrNum, &attrOffsetValue);
    // Allocate memory for the Value structure
	Value *attrValue = (Value*) malloc(sizeof(Value));
    // Get a pointer to the start of the attribute data
    char *dataPtr = record->data;
	dataPtr = dataPtr + attrOffsetValue;
	schema->dataTypes[attrNum] = (attrNum == 1) ? 1 : schema->dataTypes[attrNum];
	// Handle different data types
	switch (schema->dataTypes[attrNum])
    {
	case DT_STRING:
	{
		int strLength = schema->typeLength[attrNum];
		attrValue->v.stringV = (char*) malloc(strLength + 1);
		strncpy(attrValue->v.stringV, dataPtr, strLength);
		attrValue->v.stringV[strLength] = '\0';
		attrValue->dt = DT_STRING;
		break;
	}
	case DT_INT:
	{
		int intValue = 0;
		memcpy(&intValue, dataPtr, sizeof(int));
		attrValue->v.intV = intValue;
		attrValue->dt = DT_INT;
	break;
	}
    case DT_FLOAT:
    {
	float floatValue = 0.0;
	memcpy(&floatValue, dataPtr, sizeof(float));
		attrValue->v.floatV = floatValue;
		attrValue->dt = DT_FLOAT;
		break;
	}
	case DT_BOOL:
	{
		bool boolValue = false;
		memcpy(&boolValue, dataPtr, sizeof(bool));
		attrValue->v.boolV = boolValue;
		attrValue->dt = DT_BOOL;
		break;
	}
	default:
		printf("Serializer not defined for the given data type.\n");
		break;
    }
    *value = attrValue;
    return RC_OK;
}

// Set the value of an attribute in a record
extern RC setAttr (Record *record, Schema *schema, int attrNum, Value *value)
{
    int attrOffsetValue = 0;
    // Calculate the offset of the attribute in the record
    attrOffset(schema, attrNum, &attrOffsetValue);
    // Get a pointer to the start of the attribute data
    char *dataPtr = record->data;
    dataPtr = dataPtr + attrOffsetValue;
    // Handle different data types
    switch (schema->dataTypes[attrNum])
    {
	case DT_STRING:
	{
        int strLength = schema->typeLength[attrNum];
        strncpy(dataPtr, value->v.stringV, strLength);
        dataPtr = dataPtr + strLength;
        break;
	}
	case DT_INT:
	{
        *(int *) dataPtr = value->v.intV;
        dataPtr = dataPtr + sizeof(int);
        break;
	}
		
	case DT_FLOAT:
	{
        *(float *) dataPtr = value->v.floatV;
        dataPtr = dataPtr + sizeof(float);
        break;
	}
		
	case DT_BOOL:
	{
        *(bool *) dataPtr = value->v.boolV;
        dataPtr = dataPtr + sizeof(bool);
        break;
	}
	default:
		printf("Serializer not defined for the given data type.\n");
		break;
    }			
    return RC_OK;
}
