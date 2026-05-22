#ifndef BKP_MAP_F_H
#define BKP_MAP_F_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>
#include "bkp_export.h"
#include "bkp_container_functions.h"

/*********************************************************************
 * Defines
*********************************************************************/
struct BkpMapEntryInfo;
struct BkpMap_;
typedef struct BkpMap_ * BkpMap;

typedef struct
{
	int8_t * key;
	void * value;
}BkpMapPair;

typedef BkpMapPair * BkpMapIterator;

#define bkpMapCreate(keysize, size, capacity) bkp_map_create(sizeof(keysize), sizeof(size), capacity, 0, __FILE__, __LINE__)
#define bkpMapCreateFrom(keysize, size,  capacity, id) bkp_map_create(sizeof(keysize), sizeof(size), capacity, id, __FILE__, __LINE__)
#define bkpMapstrCreate(size, capacity) bkp_map_create(0, sizeof(size), capacity, 0, __FILE__, __LINE__)
#define bkpMapstrCreateFrom(size, capacity, id) bkp_map_create(0, sizeof(size), capacity, id, __FILE__, __LINE__)
#define bkpMapInitIterator(map, it) bkp_map_initIterator(map, it)


/*********************************************************************
 * Type def & enum
*********************************************************************/

/*********************************************************************
 * Macro
*********************************************************************/

/*********************************************************************
 * Struct
*********************************************************************/

/*********************************************************************
 * Global
*********************************************************************/

/*********************************************************************
 * Struct
*********************************************************************/

/*********************************************************************
 * Functions
*********************************************************************/

#define bkpMapDestroy(ptr) bkp_map_destroy(ptr)
#define bkpMapSize(ptr) bkp_map_size(ptr)
#define bkpMapCapacity(ptr) bkp_map_capacity(ptr)
#define bkpMapNextEntry(map, it) bkp_map_next_entry(map, it)

BKP_EXPORTED void * bkp_map_create(size_t keyStride, size_t stride, size_t capacity, size_t groupId, const char * file, unsigned int line);
BKP_EXPORTED void bkp_map_destroy(BkpMap * ptr);
BKP_EXPORTED void bkp_map_initIterator(BkpMap map, BkpMapIterator * it);
BKP_EXPORTED void bkp_map_next_entry(BkpMap map, BkpMapIterator * it);
BKP_EXPORTED uint8_t bkp_map_get(BkpMap map, void * key, void * value);
BKP_EXPORTED uint8_t bkp_map_has_key(BkpMap map, void * key);
BKP_EXPORTED uint8_t bkp_map_remove(BkpMap map, void * key, void * value);
BKP_EXPORTED void bkp_map_erase(BkpMap map, void * key);

BKP_EXPORTED size_t bkp_map_size(BkpMap map);
BKP_EXPORTED size_t bkp_map_capacity(BkpMap map);

BKP_EXPORTED void bkp_map_insert(BkpMap * ptr, void * key, void * value);

#ifdef __cplusplus
}
#endif

#endif

