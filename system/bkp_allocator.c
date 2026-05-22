#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>
#include <stdint.h>
#include <string.h>

#include "include/bkp_allocator.h"
#include "include/bkp_thread.h"
#include "include/bkp_log.h"

/**************************************************************************
*	Defines & Maro
**************************************************************************/
#ifdef _WIN32
#define getFILENAME(fname00) strrchr(fname00, '\\') ? strrchr(fname00, '\\') + 1 : fname00
#else
#define getFILENAME(fname00) strrchr(fname00, '/') ? strrchr(fname00, '/') + 1 : fname00
#endif

#define MAX_ALLOC_GRP 256
#define MAX_PAGES 256
#define DEFAULT_PAGE_SIZE SIZE_100_MIB
#define BLOCK_HEADER_SIZE sizeof(struct BlockHeader)
#define MIN_META_SIZE (sizeof(struct BlockHeader) + sizeof(size_t) + 1)

 #define ALIGN_TO(size, alignment) ((size + alignment - 1) & ~(alignment - 1))

static size_t gDefaultAlignment = 16;

/**************************************************************************
*	Structs, Enum, Unio and Typesdef
**************************************************************************/
//typedef struct bkpMemoryPage BkpMemoryPage;
struct BlockHeader
{
#ifdef DEBUG
	const char * debugFile;
	unsigned int debugLine;
#endif
	size_t size;
	size_t payloadSize;
   	uint8_t * lFree;			//previous free block
   	uint8_t * rFree;			//next free block
	uint8_t * previous;
	uint8_t * next;
	uint8_t used;
	uint8_t groupId;
	uint8_t pageId;
};

typedef struct
{
	BkpMutex mutex;
	uint8_t * data;
	uint8_t * freeBlock;
	size_t freeSize;
	size_t bigestFree;
	uint8_t groupId;
	uint8_t pageId;

}BkpMemoryPage;

typedef struct
{
	BkpMutex mutex;
	const char * name;
	BkpMemoryPage pages[MAX_PAGES];
	size_t pageSize;
	size_t pageCount;
	uint8_t id;

}BkpAllocationGroup;

/**************************************************************************
*	Globals
**************************************************************************/
static BkpAllocationGroup * gAllocGrp = NULL;
static const char * gSzGeneralGroup = "General";
static size_t gGroupIndex = 0;
static size_t gMaxGroupCount = 16;
static BkpMutex gMutex;
static const char * gSzUnit[8] = {"Bytes", "KiB", "MiB", "GiB", "TiB"};

static const char * gTagColor = "\x1b[0; 34m";
static const char * gTag	= "Memory";

/***************************************************************************
*	Prototypes
**************************************************************************/
static uint64_t bkpAlignAddress(uint64_t addr, unsigned int alignment, unsigned short int shift);
static size_t clearAllocGroup(BkpAllocationGroup * grp);
static size_t freeGroupPages(BkpAllocationGroup * grp);
static void createPage(BkpAllocationGroup * grp);
static void * allocateFromPage(BkpMemoryPage * page, size_t size, size_t alignment, const char * file, int line);
static void freeBlockFromPage(BkpMemoryPage * page, uint8_t * p);
static size_t findBigestFree(BkpMemoryPage * page);
static void remove_from_free(BkpMemoryPage * page, uint8_t * block);
static void add_to_free(BkpMemoryPage * page, uint8_t * block);
static void schrink(BkpMemoryPage * page, void * block, size_t amount, size_t metaSize);
static void enlarge(BkpMemoryPage * page, void * block, size_t amount);
static void * eatPrevious(BkpMemoryPage * page, void * block, size_t amout, size_t offset);
static uint8_t * createNewBlock(BkpMemoryPage * page, uint8_t * origin, size_t size);
static struct BlockHeader * getBlockHeader(void * ptr);
static void printMemoryAllocationGroup(BkpAllocationGroup * grp);
static void printMemoryPage(BkpMemoryPage * page);

/**************************************************************************
*	Implementations
**************************************************************************/

/*_________________________________________________________________________________*/
void bkpSetMemoryDefaultAligment(size_t v)
{
	gDefaultAlignment = v;
}

/*_________________________________________________________________________________*/
size_t bkpGetMemoryDefaultAligment()
{
	return gDefaultAlignment;
}

/*_________________________________________________________________________________*/
size_t bkpAlignSize(size_t size, unsigned int alignment)
{
	return ALIGN_TO(size, alignment);
	//return ((size / alignment) + 1) * alignment;
}

/*_________________________________________________________________________________*/
static uint64_t bkpAlignAddress(uint64_t addr, unsigned int alignment, unsigned short int shift)
{
	return ((addr/ alignment) + shift) * alignment;
}

/*_________________________________________________________________________________*/
unsigned char bkpIsPow2(size_t value)
{
	return value && (!(value & (value - 1)));
}

/*_________________________________________________________________________________*/
int bkpFindMemoryGroupByName(const char * name, unsigned short * id)
{
	for(int i = 0; i < gGroupIndex; ++i)
	{
		if(strncmp(name, gAllocGrp[i].name, 256) == 0)
		{
			*id = i;
			return 1;
		}
	}
	return 0;
}

/*_________________________________________________________________________________*/
void bkpStartAllocator(size_t maxGroupCount, size_t defaultPageSize)
{
	gAllocGrp = malloc(sizeof(BkpAllocationGroup) * maxGroupCount);
	gMaxGroupCount = maxGroupCount;
	bkpCreateMutex(&gMutex);
	BkpAllocationGroupInfo info = {gSzGeneralGroup, defaultPageSize};
	bkpAddAllocatorGroup(&info);
}

/*_________________________________________________________________________________*/
void bkpStopAllocator()
{
	size_t leak = 0;
	for(size_t i = 0; i < gGroupIndex; ++i)
	{
		leak += clearAllocGroup(&gAllocGrp[i]);
	}
	gGroupIndex = 0;
	free(gAllocGrp);
	bkpDestroyMutex(&gMutex);

	if(leak > 0)
	{
		LOGC(eFATAL, gTag, gTagColor, "\t***   Memory leak detected count : %lu   ***", leak);
	}
	else
	{
		LOGC(eDEBUG, gTag, gTagColor, "\t***   No Memory leak detected ! ***", leak);
	}

}

/*_________________________________________________________________________________*/
static size_t freeGroupPages(BkpAllocationGroup * grp)
{
	size_t leak = 0;
	for(size_t i = 0; i < grp->pageCount; ++i)
	{
		BkpMemoryPage * page = &grp->pages[i];
		if(page->freeSize != grp->pageSize)
		{
			if(leak == 0)
			{
				float fTotal;
				float fUsed;
				const char * szTotal = bkpMemoryUnits(grp->pageSize, &fTotal);
				const char * szUsed = bkpMemoryUnits(grp->pageSize - page->freeSize, &fUsed);
				LOGC(eERROR, gTag, gTagColor, "Memory LEAK: (%.1f %s) %zu Allocated, (%.1f %s) %zu used",
						fTotal, szTotal, grp->pageSize,
						fUsed, szUsed, grp->pageSize - page->freeSize);
				LOGC(eERROR, gTag, gTagColor, "Leak details :");
			}
#ifdef DEBUG
			uint8_t * p = page->data;
			while(p != NULL)
			{
				struct BlockHeader * bH = (struct BlockHeader *) p;
				if(bH->used == 1)
				{
					++leak;
					LOGC(eERROR, gTag, gTagColor, "\t#%zu [%p (%s : %u)]", leak, (void *)p, bH->debugFile, bH->debugLine);
				}
				p = bH->next;
			}
#endif
		}
		free(page->data);
		page->data = NULL;
		bkpDestroyMutex(&page->mutex);
	}
	return leak;
}

/*_________________________________________________________________________________*/
size_t clearAllocGroup(BkpAllocationGroup * grp)
{
	size_t leak = freeGroupPages(grp);
	bkpDestroyMutex(&grp->mutex);
	return leak;
}

/*_________________________________________________________________________________*/
void bkpClearAllocGroup(size_t groupId)
{
	BkpAllocationGroup * grp = &gAllocGrp[groupId];
	bkpLockMutex(&grp->mutex);
	size_t leak = freeGroupPages(grp);
	grp->pageCount = 0;
	bkpUnlockMutex(&grp->mutex);

	if(leak > 0)
	{
		LOGC(eWARNING, gTag, gTagColor, "bkpClearAllocGroup('%s'): %zu live allocation(s) forcibly freed — dangling pointers ahead", grp->name, leak);
	}
	else
	{
		LOGC(eDEBUG, gTag, gTagColor, "bkpClearAllocGroup('%s'): all pages released to OS", grp->name);
	}
}

/*_________________________________________________________________________________*/
size_t bkpGetAllocGroupPageSize(size_t groupId)
{
	return gAllocGrp[groupId].pageSize;
}

/*_________________________________________________________________________________*/
void bkpResizeAllocGroup(size_t groupId, size_t newPageSize)
{
	BkpAllocationGroup * grp = &gAllocGrp[groupId];
	bkpLockMutex(&grp->mutex);
	size_t leak = freeGroupPages(grp);
	grp->pageCount = 0;
	grp->pageSize  = newPageSize;
	bkpUnlockMutex(&grp->mutex);
	if(leak > 0)
		LOGC(eWARNING, gTag, gTagColor, "bkpResizeAllocGroup('%s'): %zu live allocation(s) freed — caller must release pointers first", grp->name, leak);
	else
		LOGC(eDEBUG, gTag, gTagColor, "bkpResizeAllocGroup('%s'): resized to %zu bytes", grp->name, newPageSize);
}

/*_________________________________________________________________________________*/
size_t bkpAddAllocatorGroup(BkpAllocationGroupInfo * info)
{
	bkpLockMutex(&gMutex);
	BkpAllocationGroup * grp = &gAllocGrp[gGroupIndex];
	bkpCreateMutex(&grp->mutex);

	bkpLockMutex(&grp->mutex);
	{
		grp->name = info->name;
		grp->pageSize = info->pageSize;
		grp->pageCount = 0;
		grp->id = gGroupIndex;
	}
	bkpUnlockMutex(&grp->mutex);

	++gGroupIndex;
	bkpUnlockMutex(&gMutex);

	return gGroupIndex - 1;
}

/*_________________________________________________________________________________*/
void * bkpAlloc_f(size_t size, size_t alignment, size_t id, const char * file, int line)
{
	if(alignment == 0)
	{
		alignment = gDefaultAlignment;
	}

	BkpAllocationGroup * grp = &gAllocGrp[id];
	BkpMemoryPage * page = NULL;
	size_t alignShift = alignment < 2 ? 0 : alignment * 2;
	size_t size_ = MIN_META_SIZE - 1 + size + alignShift;

	if(size_ >= grp->pageSize)
	{
		LOGC(eFATAL, gTag, gTagColor, "Cannot allocat that much %ld max %ld", size_, grp->pageSize);
		exit(-1);
	}

	bkpLockMutex(&grp->mutex);
   	size_t pageCount = grp->pageCount;
	bkpUnlockMutex(&grp->mutex);

	for(size_t i = 0; i < pageCount; ++i)
	{
		bkpLockMutex(&grp->pages[i].mutex);
		size_t bigestFree = grp->pages[i].bigestFree;
		bkpUnlockMutex(&grp->pages[i].mutex);

		if(bigestFree >= size_)
		{
			page = &grp->pages[i];
			break;
		}
	}

	if(page == NULL)
	{
		if(pageCount < MAX_PAGES)
		{
		   	createPage(grp);
			page = &grp->pages[grp->pageCount - 1];
		}
		else
		{
			LOGC(eFATAL, gTag, gTagColor, "Out of Memory!");
		}
	}

	void * result = allocateFromPage(page, size_, alignment, file, line);
	struct BlockHeader * bH = getBlockHeader(result);
	bH->payloadSize = size;
	return result;
}

/*_________________________________________________________________________________*/
void * bkpAllocAlign_f(size_t size, size_t alignment, size_t id, const char * file, int line)
{
	if(alignment == 0)
	{
		alignment = gDefaultAlignment;
	}

	if(bkpIsPow2(alignment) == 0)
	{
		LOGC(eFATAL, gTag, gTagColor, "Bad Alignement should be a power of 2\n");
	}

	if(alignment != 1)
	{
		if(alignment % sizeof(void *) != 0)
		{
			alignment = ((alignment / sizeof(void *)) + 1) * sizeof(void *);
		}
	}

	void * result = bkpAlloc_f(size, alignment, id, file, line);
	struct BlockHeader * bH = getBlockHeader(result);
	bH->payloadSize = size;
	//memset(result, 0, size);
	return result;
}

/*_________________________________________________________________________________*/
void bkpFree_f(void * ptr)
{
	if(ptr == NULL) return;

	struct BlockHeader * bH = getBlockHeader(ptr);
	if(bH->used == 0)
	{
		LOGC(eFATAL, gTag, gTagColor, "** Double free corruption **\n");
	}
	BkpMemoryPage * page = &gAllocGrp[bH->groupId].pages[bH->pageId];
	freeBlockFromPage(page, (uint8_t *)bH);
}

/*_________________________________________________________________________________*/
void * bkpRealloc_f(void * ptr, size_t size, size_t alignment, const char * file, int line)
{
	if(ptr == NULL)
	{
		LOGC(eWARNING, gTag, gTagColor, "Cannot retrive groupId from from NULL pointer, using 0");
		return bkpAlloc_f(size, alignment, 0, file, line);
	}

	if(size == 0)
	{
		bkpFree(ptr);
		return NULL;
	}

	struct BlockHeader * bH = getBlockHeader(ptr);
	BkpMemoryPage * page = &gAllocGrp[bH->groupId].pages[bH->pageId];

	if(size < bH->payloadSize)
	{
		size_t metaSize = (uint64_t)ptr - (uint64_t)bH;
		schrink(page, (void *)bH, bH->payloadSize - size, metaSize);
		bH->payloadSize = size;
		return ptr;
	}

	struct BlockHeader * bHn = (struct BlockHeader *) bH->next;
	struct BlockHeader * bHp = (struct BlockHeader *) bH->previous;
	size_t amount = size - bH->payloadSize;
	if(bHn && bHn->used == 0)
	{
		size_t tsize = bHn->size + bH->size;
		if(tsize >= bH->size + amount)
		{
			enlarge(page, bH, amount);
			return ptr;
		}
	}
	else if (bHp && bHp->used == 0)
	{
		size_t tsize = bHp->size + bH->size;
		if(tsize >= bH->size + amount)
		{
			return eatPrevious(page, bH, amount, (uint64_t)ptr - (uint64_t)bH);
		}
	}

	void * newPtr = bkpAlloc_f(size, alignment, bH->groupId, file, line);
	memcpy(newPtr, ptr, bH->payloadSize);
	bkpFree_f(ptr);

	return newPtr;

}

/*_________________________________________________________________________________*/
static void createPage(BkpAllocationGroup * grp)
{

#ifdef DEBUG
	LOGC(eDEBUG, gTag, gTagColor, "creating new page\n");
#endif

	bkpLockMutex(&grp->mutex);
	size_t index = grp->pageCount;

	bkpCreateMutex(&grp->pages[index].mutex);
	bkpLockMutex(&grp->pages[index].mutex);
	{
		grp->pages[index].freeSize = grp->pageSize;
		grp->pages[index].bigestFree = grp->pageSize;
		grp->pages[index].data = (uint8_t *) malloc(sizeof(uint8_t) * grp->pageSize);
		grp->pages[index].pageId = index;
		grp->pages[index].groupId = grp->id;
		grp->pages[index].freeBlock = grp->pages[index].data;

		struct BlockHeader * block = (struct BlockHeader *) grp->pages[index].data;
		block->size = grp->pageSize;
		block->rFree = block->lFree = block->previous = block->next = NULL;
		block->payloadSize = 0;
		block->used = 0;
		uint8_t * p = (uint8_t *)block + BLOCK_HEADER_SIZE; //Padding offset
		*p = 0; //padding = 0
	}
	bkpUnlockMutex(&grp->pages[index].mutex);

	++grp->pageCount;
	bkpUnlockMutex(&grp->mutex);
}

/*_________________________________________________________________________________*/
static void * allocateFromPage(BkpMemoryPage * page, size_t size, size_t alignment, const char * file, int line)
{
	bkpLockMutex(&page->mutex);
	//fprintf(stdout, "allocating size : %lu, freePages : %lu, biger block : %lu\n", size, page->freeSize, page->bigestFree);

	uint8_t * p = page->freeBlock;
	struct BlockHeader * block = (struct BlockHeader *) p;
	size_t bonus = 0; //In case there is not enought place to allocate a new block we give everything to the new block
	//struct BlockHeader * pBlock = (struct BlockHeader *) block->previous;

	if(p == NULL)
	{
		createNewBlock(page, p, size);
	}
	else
	{
		for(; p != NULL; p = block->rFree)
		{
			block = (struct BlockHeader *) p;
			//fprintf(stderr, "0 Block vs size : (%lu vs %lu) block + header %lu\n", block->size, size, size + BLOCK_HEADER_SIZE);
			if(size > block->size)
			{
				continue;
			}

			remove_from_free(page, p);
			if((size + BLOCK_HEADER_SIZE) < block->size )
			{
				createNewBlock(page, p, size);
				break;
			}
			else
			{
				if(block->lFree)
				{
					struct BlockHeader * bH = (struct BlockHeader *) block->lFree;
					bH->rFree = block->rFree;
				}
				if(block->rFree)
				{
					struct BlockHeader * bH = (struct BlockHeader *) block->rFree;
					bH->lFree = block->lFree;
				}
				if(page->freeBlock == p)
				{
					page->freeBlock = block->rFree;
				}
				bonus = block->size - size;
				break;
			}
		}
	}

	page->bigestFree = findBigestFree(page);

	size_t nSize = BLOCK_HEADER_SIZE + sizeof(size_t);
	uint8_t * returnPtr = p + nSize;
	uint64_t addr = (uint64_t)((uint64_t *)returnPtr);

	//fprintf(stderr, "\t%llu is aligned with %lu : %d\n", addr, alignment, addr% alignment == 0);
	if((alignment != 1) && (addr % alignment != 0))
	{
		uint64_t addr2 = bkpAlignAddress(addr, alignment, 1);
		if((addr2 - addr) < sizeof(size_t))
		{
			addr2 = bkpAlignAddress(addr, alignment, 2);
		}
		returnPtr = (uint8_t *)addr2;
		//fprintf(stderr, "\t\t%llu -> %llu\n",addr, addr2);
		nSize = addr2 - (uint64_t)((uint64_t *)p);
	}

	size_t * q = (size_t *)(returnPtr - sizeof(size_t));
	*q = nSize;
	//fprintf(stderr, "\talignment %llu return addr %llu, nsize%llu\n", alignment, (uint64_t)returnPtr, nSize);
	//fprintf(stderr, "\t%llu is aligned with %llu : %d\n", (uint64_t)returnPtr, alignment, (uint64_t)returnPtr % alignment == 0);

	block->used = 1;
	block->pageId = page->pageId;
	block->groupId = page->groupId;

	//fprintf(stderr, "\t\t\t size %llu payload %llu groupId %u, at %llu\n", block->size, block->payloadSize, block->groupId, (uint64_t)((uint64_t *)block));

#ifdef DEBUG
	block->debugFile = getFILENAME(file);
	block->debugLine = line;
# endif

	page->freeSize -= (size + bonus);

	bkpUnlockMutex(&page->mutex);
	return returnPtr;
}

/*_________________________________________________________________________________*/
static void freeBlockFromPage(BkpMemoryPage * page, uint8_t * block)
{
	bkpLockMutex(&page->mutex);
	uint8_t hasPrev = 0;

	struct BlockHeader * bH = (struct BlockHeader *) block;
	bH->used = 0;
	bH->payloadSize = 0;
	page->freeSize += bH->size;

	if(bH->previous)
	{
		struct BlockHeader * bHp = (struct BlockHeader *) bH->previous;
		if(bHp->used == 0)
		{
			hasPrev = 1;
			bHp->size += bH->size;
			bHp->next = bH->next;
			if(bH->next)
			{
				struct BlockHeader * bHn = (struct BlockHeader *) bH->next;
				bHn->previous = bH->previous;
			}
			//remove_from_free(page, block);
			block = (uint8_t* )bHp;
			bH = (struct BlockHeader *) block;
		}
	}

	if(hasPrev == 0)
	{
		add_to_free(page, block);
	}

	if(bH->next)
	{
		struct BlockHeader * bHn = (struct BlockHeader *) bH->next;
		if(bHn->used == 0)
		{
			remove_from_free(page, bH->next);
			bH->size += bHn->size;
			bH->next = bHn->next;
			if(bHn->next)
			{
				struct BlockHeader * bHnn = (struct BlockHeader *) bHn->next;
				bHnn->previous = (uint8_t*)bH;
			}
		}
	}

	if(bH->size > page->bigestFree)
	{
		page->bigestFree = bH->size;
	}

	bkpUnlockMutex(&page->mutex);
}

/*_________________________________________________________________________________*/
void bkpPrint_memoryUsage()
{
	LOGC(eINFO, gTag, gTagColor, "Memory Usage :");
	for(size_t i = 0; i < gGroupIndex; ++i)
	{
		if(gAllocGrp[i].pageCount == 0) continue;

		uint64_t freed = 0;
		uint64_t total = gAllocGrp[i].pageSize * gAllocGrp[i].pageCount;
		float usage;

		LOGC(eINFO, gTag, gTagColor, "\t * Group '%s'", gAllocGrp[i].name);

		for(size_t j = 0; j < gAllocGrp[i].pageCount; ++j)
		{
			BkpMemoryPage * page = &gAllocGrp[i].pages[j];
			freed += page->freeSize;
			size_t used =gAllocGrp[i].pageSize - page->freeSize;
			const char * szUsage = bkpMemoryUnits(used, &usage);
			LOGC(eINFO, gTag, gTagColor, "\t\tPage #%lu, used : (%.1f %s) %lu", j, usage, szUsage, used);
		}
		size_t used = total - freed;
		float ftotal;
		const char * szUsage = bkpMemoryUnits(used, &usage);
		const char * szTotal = bkpMemoryUnits(total, &ftotal);
		LOGC(eINFO, gTag, gTagColor, "\t   Allocated : (%.1f %s) %lu, used : (%.1f %s) %lu", ftotal, szTotal, total, usage, szUsage, used);
	}
	LOGC(eINFO, gTag, gTagColor, "\n");
}

/*_________________________________________________________________________________*/
void bkpPrint_memory()
{
	for(size_t i = 0; i < gGroupIndex; ++i)
	{
		printMemoryAllocationGroup(&gAllocGrp[i]);
		/*
		//fprintf(stdout, "\tGroup '%s' : pageSize %lu, pagesCount : %lu", gAllocGrp[i].name, gAllocGrp[i].pageSize, gAllocGrp[i].pageCount);
		for(size_t j = 0; j < (size_t)gAllocGrp[i].pageCount; ++j)
		{
			BkpMemoryPage * page = &gAllocGrp[i].pages[j];
			//fprintf(stdout, "\t\tPage #%lu, bigBlock : %lu, free : %lu", j, page->bigestFree, page->freeSize);
			//fprintf(stdout, "\t\t\tBlocks : [NULL <-> ");
			uint8_t * p = page->data;
			while(p != NULL)
			{
				struct BlockHeader * bH = (struct BlockHeader *) p;
				//fprintf(stdout, "%p (%lu : %lu) <-> ", p, bH->size, bH->payloadSize);
				p = bH->next;
			}
			//fprintf(stdout, "NULL]\n");
			//fprintf(stdout, "\t\t\tFree   : [NULL <-> ");
			p = page->freeBlock;
			while(p != NULL)
			{
				struct BlockHeader * bH = (struct BlockHeader *) p;
				//fprintf(stdout, "%p<-> ", p);
				p = bH->rFree;
			}
			//fprintf(stdout, "NULL]\n");
		}
		*/
	}
	//fprintf(stdout, "________________________________________________________________________________\n\n");
}

/*_________________________________________________________________________________*/
void bkpPrintMemoryAllocationGroup(unsigned short int id)
{
	printMemoryAllocationGroup(&gAllocGrp[id]);
}


/******************** static functions ********************************/

/*_________________________________________________________________________________*/
static size_t findBigestFree(BkpMemoryPage * page)
{
	uint8_t * p = page->freeBlock;
	size_t val = 0;
	while(p != NULL)
	{
		struct BlockHeader * bH = (struct BlockHeader *) p;
		if(val < bH->size)
		{
			val = bH->size;
		}
		p = bH->rFree;
	}

	return val;
}

/*_________________________________________________________________________________*/
static void remove_from_free(BkpMemoryPage * page, uint8_t * block)
{
	uint8_t * p = page->freeBlock;
	while(p != NULL)
	{
		struct BlockHeader * bH = (struct BlockHeader *) p;
		if(p == block)
		{
			if(bH->lFree)
			{
				struct BlockHeader * bHl = (struct BlockHeader *) bH->lFree;
				bHl->rFree = bH->rFree;
			}
			if(bH->rFree)
			{
				struct BlockHeader * bHr = (struct BlockHeader *) bH->rFree;
				bHr->lFree = bH->lFree;
			}

			if(page->freeBlock == block)
			{
				page->freeBlock = bH->rFree;
			}

			return;
		}
		p = bH->rFree;
	}
}

/*_________________________________________________________________________________*/
static void add_to_free(BkpMemoryPage * page, uint8_t * block)
{
	struct BlockHeader * bH = (struct BlockHeader *) block;
	bH->lFree = NULL;
	bH->rFree = page->freeBlock;
	if(page->freeBlock != NULL)
	{
		struct BlockHeader * bHr = (struct BlockHeader *) page->freeBlock;
		bHr->lFree = block;
	}
	page->freeBlock = block;
}

/*_________________________________________________________________________________*/
void schrink(BkpMemoryPage * page, void * block, size_t amount, size_t metaSize)
{
	bkpLockMutex(&page->mutex);
	struct BlockHeader * bH = (struct BlockHeader *)block;
	uint64_t schrinkSize = bH->size - metaSize - (bH->payloadSize - amount);

	if(schrinkSize < MIN_META_SIZE)
	{
		bkpUnlockMutex(&page->mutex);
		return;
	}

	size_t newSize = ((struct BlockHeader *) block)->size - schrinkSize;

	bH = (struct BlockHeader *) createNewBlock(page, block, newSize);
	page->freeSize += bH->size;

	if(bH->next)
	{
		struct BlockHeader * bHn = (struct BlockHeader *)bH->next;
		if(bHn->used == 0)
		{
			remove_from_free(page, bH->next);
			bH->size += bHn->size;
			bH->next = bHn->next;
			if(bHn->next)
			{
				struct BlockHeader * bHnn = (struct BlockHeader *) bHn->next;
				bHnn->previous = (uint8_t*)bH;
			}
		}
	}

	if(bH->size > page->bigestFree)
	{
		page->bigestFree = bH->size;
	}
}

/*_________________________________________________________________________________*/
static void enlarge(BkpMemoryPage * page, void * block, size_t amount)
{
	bkpLockMutex(&page->mutex);
	struct BlockHeader * bH = (struct BlockHeader *) block;
	struct BlockHeader * bHn = (struct BlockHeader *) bH->next;

	remove_from_free(page, bH->next);

	if((bHn->size - amount) <= MIN_META_SIZE)
	{
		bH->size += bHn->size;
		bH->next = bHn->next;
		if(bHn->next)
		{
			struct BlockHeader * bHnn = (struct BlockHeader *) bHn->next;
			bHnn->previous = block;
		}

		bH->payloadSize += bHn->size;
		page->freeSize -= bHn->size;
	}
	else
	{
		uint8_t * previous = bHn->previous;
		uint8_t * next = bHn->next;
		size_t bHnSize = bHn->size;
		bH->size += amount;
		uint8_t * p = (uint8_t *) bH + bH->size;
		bH->next = p;
		struct BlockHeader * bHn_ = (struct BlockHeader *)p;
		bHn_->size = bHnSize - amount;
		bHn_->payloadSize = 0;
		bHn_->previous = previous;
		bHn_->next = next;
		bHn_->used = 0;
		add_to_free(page, p);
		bH->payloadSize += amount;
		page->freeSize -= amount;
		if(bHn_->next)
		{
			struct BlockHeader * bHnn = (struct BlockHeader *) bHn_->next;
			bHnn->previous = p;
		}
	}
	bkpUnlockMutex(&page->mutex);
}

static void * eatPrevious(BkpMemoryPage * page, void * block, size_t amount, size_t offset)
{
	bkpLockMutex(&page->mutex);
	struct BlockHeader * bH = (struct BlockHeader *) block;
	struct BlockHeader * bHp = (struct BlockHeader *) bH->previous;
	size_t size_to_move = bH->size;
	void * result = NULL;

	if((bHp->size - amount) <= MIN_META_SIZE)
	{
		remove_from_free(page, bH->previous);
		bH->size += bHp->size;
		bH->previous = bHp->previous;
		bH->payloadSize += bHp->size;
		page->freeSize -= bHp->size;
		if(bH->next)
		{
			struct BlockHeader * bHnn = (struct BlockHeader *) bH->next;
			bHnn->previous = (uint8_t *)bHp;
		}
		memmove(bHp, bH, size_to_move);
		bHp->used = 1;

		result = (uint8_t *)bHp + offset;
	}
	else
	{
		bH->size += amount;
		bH->payloadSize += amount;

		uint8_t * p = (uint8_t *)bH - amount;
		memmove(p, bH, size_to_move);
		bH = (struct BlockHeader *) p;
		bHp->next = p;
		bHp->size -= amount;

		if(bH->next)
		{
			struct BlockHeader * bHnn = (struct BlockHeader *) bH->next;
			bHnn->previous = p;
		}

		page->freeSize -= amount;
		result = p + offset;
	}
	bkpUnlockMutex(&page->mutex);
	return result;
}

/*_________________________________________________________________________________*/
static uint8_t * createNewBlock(BkpMemoryPage * page, uint8_t * origin, size_t size)
{
	uint8_t * newBlock = origin + size;
	struct BlockHeader * bH = (struct BlockHeader *) origin;
	struct BlockHeader * newBlockHeader = (struct BlockHeader *) newBlock;

	newBlockHeader->size = bH->size - size;
	newBlockHeader->payloadSize = 0;
	newBlockHeader->previous = origin;
	newBlockHeader->next = bH->next;
	newBlockHeader->used = 0;

	add_to_free(page, newBlock);

	if(bH->next)
	{
		struct BlockHeader * bHn = (struct BlockHeader *) bH->next;
		bHn->previous = newBlock;
	}

	bH->next = newBlock;
	bH->size = size;
	if(page->freeBlock == origin)
	{
		page->freeBlock = newBlock;
	}

	return newBlock;
}

/*_________________________________________________________________________________*/
static struct BlockHeader * getBlockHeader(void * ptr)
{
	size_t nSize = *((size_t *) ptr - 1);
	uint64_t addr = (uint64_t)((uint64_t *) ptr) - nSize;

	return (struct BlockHeader *) addr;
}

/*_________________________________________________________________________________*/
const char * bkpMemoryUnits(float value, float * result)
{
	int index = 0;
	float f = value;
	while(f > 1024)
	{
		f /= 1024;
		++index;
	}

	index = index > 4 ? 4 : index;
	*result = f;

	return gSzUnit[index];
}

/*_________________________________________________________________________________*/
size_t bkpGetMemoryGroupCount()
{
	return gGroupIndex + 1;
}

/*_________________________________________________________________________________*/
size_t bkpGetAvailableMemoryGroupCount()
{
	return gMaxGroupCount - gGroupIndex;
}

/*_________________________________________________________________________________*/
int bkpGetUsageInfo(BkpMemoryUsageInfo * info)
{
	info->totalSize = info->freed = 0;
	size_t count = 0;
	for(size_t i = 0; i < info->groupCount; ++i)
	{
		uint8_t id = (uint8_t)info->groups[i].id;
		//if(id >= MAX_ALLOC_GRP) continue;
		if(gAllocGrp[id].pageCount == 0) continue;

		info->groups[i].name = gAllocGrp[id].name;

		info->groups[i].pageCount = gAllocGrp[id].pageCount;
		info->groups[i].freed = 0;
		for(size_t j = 0; j < gAllocGrp[i].pageCount; ++j)
		{
			info->groups[i].freed += gAllocGrp[i].pages[j].freeSize;
		}
		info->groups[i].totalSize = gAllocGrp[id].pageSize * gAllocGrp[id].pageCount;
		info->groups[i].szTotal = bkpMemoryUnits(info->groups[i].totalSize, &info->groups[i].ftotalSize);
		info->groups[i].szFreed = bkpMemoryUnits(info->groups[i].freed, &info->groups[i].ffreed);

		info->totalSize += info->groups[i].totalSize;
		info->freed 	+= info->groups[i].freed;
		++count;
	}

	info->groupCount = count;
	info->szTotal = bkpMemoryUnits(info->totalSize, &info->ftotalSize);
	info->szFreed = bkpMemoryUnits(info->freed, &info->ffreed);

	return 0;
}

/*_________________________________________________________________________________*/
static void printMemoryAllocationGroup(BkpAllocationGroup * grp)
{
	fprintf(stdout,"Group '%s' : id %u, %zu pages, pageSize = %zu\n",
		grp->name, grp->id, grp->pageCount, grp->pageSize);
	for(size_t i = 0; i < grp->pageCount; ++i)
	{
		printMemoryPage(&grp->pages[i]);
	}

}

/*_________________________________________________________________________________*/
static void printMemoryPage(BkpMemoryPage * page)
{
	fprintf(stdout, "\tPage #%u, bigBlock : %zu, free : %zu\n", page->pageId, page->bigestFree, page->freeSize);
	fprintf(stdout, "\t\tBlocks : [NULL <-> ");
	uint8_t * p = page->data;
	while(p != NULL)
	{
		struct BlockHeader * bH = (struct BlockHeader *) p;
		fprintf(stdout, "%p (%zu : %zu) <-> ", p, bH->size, bH->payloadSize);
		p = bH->next;
	}
	fprintf(stdout, "NULL]\n\n");
	fprintf(stdout, "\t\tFree   : [NULL <-> ");
	p = page->freeBlock;
	while(p != NULL)
	{
		struct BlockHeader * bH = (struct BlockHeader *) p;
		fprintf(stdout, "%p<-> ", p);
		p = bH->rFree;
	}
	fprintf(stdout, "NULL]\n");
}

#ifdef __cplusplus
}
#endif
