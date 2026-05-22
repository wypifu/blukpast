#ifndef BKP_ALLOCATOR_H
#define BKP_ALLOCATOR_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>
#include "bkp_export.h"

/*********************************************************************
 * Defines
*********************************************************************/
#define SIZE_1_KIB   1024
#define SIZE_10_KIB  10240
#define SIZE_100_KIB 102400
#define SIZE_1_MIB   1048576
#define SIZE_10_MIB  10485760
#define SIZE_100_MIB 104857600
#define SIZE_1_GIB   1073741824

#define bkpAlloc(size) bkpAlloc_f(size, 0, 0, __FILE__, __LINE__)
#define bkpAllocAligned(size, alignment) bkpAllocAlign_f(size, alignment, 0,  __FILE__, __LINE__)

#define bkpAllocFrom(size, groupId) bkpAlloc_f(size, 0, groupId, __FILE__, __LINE__)
#define bkpAllocAlignedFrom(size, alignment, groupId) bkpAllocAlign_f(size, alignment, groupId,  __FILE__, __LINE__)

#define bkpRealloc(ptr, size) bkpRealloc_f(ptr, size, 0,  __FILE__, __LINE__)
#define bkpReallocAligned(ptr, size, alignment) bkpRealloc_f(ptr, size, alignment,  __FILE__, __LINE__)
#define bkpFree(ptr) bkpFree_f(ptr)

/*********************************************************************
 * Type def & enum
*********************************************************************/

/*********************************************************************
 * Macro
*********************************************************************/

/*********************************************************************
 * Struct
*********************************************************************/
typedef struct
{
	const char * name;
	size_t pageSize;

}BkpAllocationGroupInfo;

typedef struct
{
	float ftotalSize;
	float ffreed;
	size_t pageCount;
	size_t totalSize;
	size_t freed;
	int id;
	const char * name;
	const char * szTotal;
	const char * szFreed;
}BkpMemoryGroupUsageInfo;

typedef struct
{
	size_t totalSize;
	size_t freed;
	float ftotalSize;
	float ffreed;
	const char * szTotal;
	const char * szFreed;
	BkpMemoryGroupUsageInfo * groups;
	size_t groupCount;

}BkpMemoryUsageInfo;

/*********************************************************************
 * Global
*********************************************************************/

/*********************************************************************
 * Struct
*********************************************************************/

/*********************************************************************
 * Functions
*********************************************************************/

BKP_EXPORTED void bkpSetMemoryDefaultAligment(size_t v);
BKP_EXPORTED size_t bkpGetMemoryDefaultAligment();
size_t bkpAlignSize(size_t size, unsigned int alignment);

BKP_EXPORTED unsigned char bkpIsPow2(size_t value);
BKP_EXPORTED int bkpFindMemoryGroupByName(const char * name, unsigned short * id);
BKP_EXPORTED void bkpStartAllocator(size_t maxGroupCount, size_t defaultPageSize);
BKP_EXPORTED void bkpStopAllocator();
BKP_EXPORTED size_t bkpAddAllocatorGroup(BkpAllocationGroupInfo * info);
BKP_EXPORTED void   bkpClearAllocGroup(size_t groupId);
BKP_EXPORTED void   bkpResizeAllocGroup(size_t groupId, size_t newPageSize);
BKP_EXPORTED size_t bkpGetAllocGroupPageSize(size_t groupId);

BKP_EXPORTED void * bkpAlloc_f(size_t size, size_t alignment, size_t id, const char * file, int line);
BKP_EXPORTED void * bkpAllocAlign_f(size_t size, size_t alignment, size_t id, const char * file, int line);
BKP_EXPORTED void * bkpRealloc_f(void * ptr, size_t size, size_t alignment, const char * file, int line);
BKP_EXPORTED void bkpFree_f(void * ptr);

BKP_EXPORTED void bkpPrint_memoryUsage();
BKP_EXPORTED void bkpPrint_memory();

BKP_EXPORTED const char * bkpMemoryUnits(float value, float * result);
BKP_EXPORTED size_t bkpGetMemoryGroupCount();
BKP_EXPORTED size_t bkpGetAvailableMemoryGroupCount();
BKP_EXPORTED int bkpGetUsageInfo(BkpMemoryUsageInfo * info);
BKP_EXPORTED void bkpPrintMemoryAllocationGroup(unsigned short int id);


#ifdef __cplusplus
}
#endif

#endif

