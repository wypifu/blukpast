#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>
#include <stdio.h>
#include <string.h>

#include "include/bkp_allocator.h"
#include "include/bkp_map.h"
#include "include/bkp_hash.h"
#include "include/bkp_string.h"

/**************************************************************************
*	Defines & Maro
**************************************************************************/
#define MAX_PAIR_ENTRY 4

/**************************************************************************
*	Structs, Enum, Unio and Typesdef
**************************************************************************/
typedef struct
{
	char used;
	BkpMapPair pair;
}PairInfo;

typedef struct
{
	uint8_t entries;
	PairInfo info[MAX_PAIR_ENTRY];

}Entry;

struct BkpMap_
{
	size_t capacity;
	size_t size;
	size_t stride;
	size_t keyStride;
	size_t groupId;
	size_t mapIndex;
	Entry * entries;
	uint8_t * values;
	uint8_t entryIndex;
};

/**************************************************************************
*	Globals
**************************************************************************/
struct Person
{
	char name[32];
	char surname[32];
	int age;
};


/***************************************************************************
*	Prototypes
**************************************************************************/
static BkpMap resizeMap(BkpMap map);
static int isPrime(uint64_t num);
static uint64_t nextPrime(uint64_t num);
static uint8_t areKeysSame(int8_t * key1, int8_t * key2, size_t keyStride);
static BkpMapIterator getEntryPair(BkpMap map);

/**************************************************************************
*	Implementations
**************************************************************************/

/*_________________________________________________________________________________*/
void * bkp_map_create(size_t keyStride, size_t stride, size_t capacity, size_t groupId, const char * file, unsigned int line)
{
	struct BkpMap_ * map =  bkpAlloc_f(sizeof(struct BkpMap_), bkpGetMemoryDefaultAligment(), groupId, file, line);
	map->stride = stride;
	map->size = 0;
	map->groupId = groupId;
	map->capacity = capacity;
	map->keyStride = keyStride;
	map->entries = bkpAlloc_f(sizeof(Entry) * map->capacity, bkpGetMemoryDefaultAligment(), map->groupId, __FILE__, __LINE__);

	for(size_t i = 0; i < map->capacity; ++i)
	{
		map->entries[i].entries = 0;
		for(size_t j = 0; j < MAX_PAIR_ENTRY; ++j)
		{
			map->entries[i].info[j] = (PairInfo){0, {NULL, NULL}};
		}
	}

	map->values = bkpAlloc_f(map->stride * map->capacity * MAX_PAIR_ENTRY, bkpGetMemoryDefaultAligment(), map->groupId, __FILE__, __LINE__);

	return map;
}

/*_________________________________________________________________________________*/
void bkp_map_destroy(BkpMap * ptr)
{
	BkpMap map = *ptr;
	for(size_t i = 0; i < map->capacity; ++i)
	{
		for(size_t j = 0; j < MAX_PAIR_ENTRY; ++j)
		{
			if(map->entries[i].info[j].used == 1)
			{
				bkpFree(map->entries[i].info[j].pair.key);
			}
		}
	}

	bkpFree(map->entries);
	bkpFree(map->values);
	bkpFree(map);
}

/*_________________________________________________________________________________*/
void bkp_map_insert(BkpMap * ptr, void * key, void * value)
{
	BkpMap map = *ptr;
	size_t keyLen = map->keyStride;
	int8_t * pKey = key;
	if(keyLen  == 0)
	{
		pKey = *(int8_t**)key;
		keyLen = strlen((const char *)pKey);
	}

	size_t h = bkp_hash_jenkinsOAAT(pKey, keyLen);

  size_t hh = 0;
check_hash:
	hh = h % map->capacity;

	if((map->entries[hh].entries >= MAX_PAIR_ENTRY))
	{
		map = resizeMap(map);
		*ptr = map;
		goto check_hash;
	}

	for(size_t i = 0; i < MAX_PAIR_ENTRY; ++i)
	{
		if(map->entries[hh].info[i].used == 0)
		{
			uint8_t * p = map->values + (hh * map->stride * MAX_PAIR_ENTRY) + (i * map->stride);
			memcpy(p, value, map->stride);
			map->entries[hh].info[i].used = 1;
			if(map->keyStride == 0)
			{
				map->entries[hh].info[i].pair.key = bkpAlloc_f(keyLen + bkpGetMemoryDefaultAligment(), 1, map->groupId, __FILE__, __LINE__);
				memcpy(map->entries[hh].info[i].pair.key, pKey, keyLen + 1);
			}
			else
			{
				map->entries[hh].info[i].pair.key = bkpAlloc_f(keyLen, bkpGetMemoryDefaultAligment(), map->groupId, __FILE__, __LINE__);
				memcpy(map->entries[hh].info[i].pair.key, pKey, keyLen);
			}
			map->entries[hh].info[i].pair.value = p;
			++map->entries[hh].entries;
			++map->size;
			return;
		}
		else if(areKeysSame(map->entries[hh].info[i].pair.key, pKey, map->keyStride) == 1)
		{
			uint8_t * p = map->entries[hh].info[i].pair.value;
			memcpy(p, value, map->stride);
			return;
		}

	}
}

/*_________________________________________________________________________________*/
uint8_t bkp_map_has_key(BkpMap map, void * key)
{
	size_t keyLen = map->keyStride;
	int8_t * pKey = key;
	if(keyLen  == 0)
	{
		pKey = *(int8_t**)key;
		keyLen = strlen((const char *)pKey);
	}

	size_t h = bkp_hash_jenkinsOAAT(pKey, keyLen);
	size_t hh = h % map->capacity;

	if(map->entries[hh].entries > 0)
	{
		for(size_t i = 0; i < MAX_PAIR_ENTRY; ++i)
		{
			if((map->entries[hh].info[i].used == 1) && (areKeysSame(map->entries[hh].info[i].pair.key, pKey, map->keyStride) == 1))
			{
				return 1;
			}
		}
	}

	return 0;
}

/*_________________________________________________________________________________*/
uint8_t bkp_map_remove(BkpMap map, void * key, void * value)
{
	size_t keyLen = map->keyStride;
	int8_t * pKey = key;
	if(keyLen  == 0)
	{
		pKey = *(int8_t**)key;
		keyLen = strlen((const char *)pKey);
	}

	size_t h = bkp_hash_jenkinsOAAT(pKey, keyLen);
	size_t hh = h % map->capacity;

	if(map->entries[hh].entries > 0)
	{
		for(size_t i = 0; i < MAX_PAIR_ENTRY; ++i)
		{
			if((map->entries[hh].info[i].used == 1) && (areKeysSame(map->entries[hh].info[i].pair.key, pKey, map->keyStride) == 1))
			{
				int8_t * p = map->entries[hh].info[i].pair.value;
				memcpy(value, p, map->stride);
				map->entries[hh].info[i].used = 0;
				--map->entries[hh].entries;
				--map->size;
				bkpFree(map->entries[hh].info[i].pair.key);
				return 1;
			}
		}
	}

	return 0;
}

/*_________________________________________________________________________________*/
void bkp_map_erase(BkpMap map, void * key)
{
	size_t keyLen = map->keyStride;
	int8_t * pKey = key;
	if(keyLen  == 0)
	{
		pKey = *(int8_t**)key;
		keyLen = strlen((const char *)pKey);
	}

	size_t h = bkp_hash_jenkinsOAAT(pKey, keyLen);
	size_t hh = h % map->capacity;

	if(map->entries[hh].entries > 0)
	{
		for(size_t i = 0; i < MAX_PAIR_ENTRY; ++i)
		{
			if((map->entries[hh].info[i].used == 1) && (areKeysSame(map->entries[hh].info[i].pair.key, pKey, map->keyStride) == 1))
			{

				bkpFree(map->entries[hh].info[i].pair.key);
				map->entries[hh].info[i].used = 0;
				--map->entries[hh].entries;
				--map->size;
				return;
			}
		}
	}
}

/*_________________________________________________________________________________*/
void bkp_map_initIterator(BkpMap map, BkpMapIterator * it)
{
	*it = NULL;

	if(map->size == 0)
	{
		return;
	}

	map->mapIndex = 0;
	map->entryIndex = 0;

	while(map->mapIndex < map->capacity && map->entries[map->mapIndex].entries < 1) ++map->mapIndex;


	*it = getEntryPair(map);
}

/*_________________________________________________________________________________*/
void bkp_map_next_entry(BkpMap map, BkpMapIterator * it)
{
	++map->entryIndex;
	if(map->entryIndex < MAX_PAIR_ENTRY)
	{
		for(size_t i = map->entryIndex; i < MAX_PAIR_ENTRY; ++i)
		{
			if(map->entries[map->mapIndex].info[i].used == 1)
			{
				*it = &map->entries[map->mapIndex].info[i].pair;
				map->entryIndex = i;
				return ;
			}
		}
	}

	++map->mapIndex;
	map->entryIndex = 0;

	while(map->mapIndex < map->capacity && map->entries[map->mapIndex].entries < 1) ++map->mapIndex;

	*it = NULL;
	if(map->mapIndex == map->capacity)
	{
		return;
	}

	*it = getEntryPair(map);
}

/*_________________________________________________________________________________*/
size_t bkp_map_size(BkpMap map)
{
	return map->size;
}

/*_________________________________________________________________________________*/
size_t bkp_map_capacity(BkpMap map)
{
	return map->capacity * MAX_PAIR_ENTRY;
}

/*_________________________________________________________________________________*/
uint8_t bkp_map_get(BkpMap map, void * key, void * value)
{
	size_t keyLen = map->keyStride;
	int8_t * pKey = key;
	if(keyLen  == 0)
	{
		pKey = *(int8_t**)key;
		keyLen = strlen((const char *)pKey);
	}

	size_t h = bkp_hash_jenkinsOAAT(pKey, keyLen);
	size_t hh = h % map->capacity;

	if(map->entries[hh].entries > 0)
	{
		for(size_t i = 0; i < MAX_PAIR_ENTRY; ++i)
		{
			if((map->entries[hh].info[i].used == 1) && (areKeysSame(map->entries[hh].info[i].pair.key, pKey, map->keyStride) == 1))
			{
				int8_t * p = map->entries[hh].info[i].pair.value;
				memcpy(value, p, map->stride);
				return 1;
			}
		}
	}

	memset(value, 0, map->stride);
	return 0;
}


/******************** static functions ********************************/

/*_________________________________________________________________________________*/
static BkpMap resizeMap(BkpMap map)
{
	BkpMap newMap = bkp_map_create(map->keyStride, map->stride, nextPrime(map->capacity), map->groupId, __FILE__, __LINE__);
	BkpMapIterator itMap;

	if(map->keyStride == 0)
	{
		for(bkpMapInitIterator(map, &itMap); itMap != NULL; bkpMapNextEntry(map, &itMap))
		{
			bkp_map_insert(&newMap, &itMap->key, itMap->value);
		}
	}
	else
	{
		for(bkpMapInitIterator(map, &itMap); itMap != NULL; bkpMapNextEntry(map, &itMap))
		{
			bkp_map_insert(&newMap, itMap->key, itMap->value);
		}
	}

	bkpMapDestroy(&map);
	return newMap;
}

/*_________________________________________________________________________________*/
static int isPrime(uint64_t num)
{
	if(num < 1) return 0;
	if(num < 4) return 1;

	if((num % 2 == 0) || (num % 3 == 0)) return 0;

	for(uint64_t i = 5; i * i <= num; ++i)
	{
		if(num % i == 0)
		{
			return 0;
		}
	}

	return 1;
}

/*_________________________________________________________________________________*/
static uint64_t nextPrime(uint64_t num)
{
	num = (num % 2 == 0) ? num + 1: num + 2;
   	while(1)
	{
		if(isPrime(num) == 1)
		{
			return num;
		}
		num += 2;
	}
}

/*_________________________________________________________________________________*/
static uint8_t areKeysSame(int8_t * key1, int8_t * key2, size_t keyStride)
{
	if(keyStride == 0)
	{
		return strcmp((const char *)key1, (const char *)key2) == 0;
	}

	for(size_t i = 0; i < keyStride; ++i)
	{
		if(key1[i] != key2[i])
		{
			return 0;
		}
	}

	return 1;
}

/*_________________________________________________________________________________*/
static BkpMapIterator getEntryPair(BkpMap map)
{
	for(size_t i = map->entryIndex; i < MAX_PAIR_ENTRY; ++i)
	{
		if(map->entries[map->mapIndex].info[i].used == 1)
		{
			map->entryIndex = i;
			return &map->entries[map->mapIndex].info[i].pair;
		}
	}

	return NULL;
}

/*_________________________________________________________________________________*/



#ifdef __cplusplus
}
#endif
