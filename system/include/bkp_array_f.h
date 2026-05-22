#ifndef BKP_ARRAY_F_H
#define BKP_ARRAY_F_H

#ifdef __cplusplus
extern "C" {
#endif

#include "bkp_export.h"
#include "bkp_container_functions.h"

/*********************************************************************
 * Defines
*********************************************************************/

#define BkpArray(object) object*

#define bkpArrayCreate(typ) bkp_array_create(sizeof(typ), 0, __FILE__, __LINE__)
#define bkpArrayCreateFrom(typ, id) bkp_array_create(sizeof(typ), id, __FILE__, __LINE__)

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

#define bkpArrayResize(ptr, size) bkp_resize_array((void **) ptr, size, __FILE__, __LINE__)
#define bkpArrayDestroy(ptr) bkp_destroy_array((void **) ptr)
#define bkpArrayReserve(ptr, size) bkp_array_reserve((void **) ptr, size, __FILE__, __LINE__)
#define bkpArraySize(ptr) bkp_array_size((void **) ptr)
#define bkpArrayCapacity(ptr) bkp_array_capacity((void **) ptr)
#define bkpArrayClear(ptr) bkp_array_clear((void *) ptr)
#define bkpArrayPop(ptr) bkp_array_pop((void *) ptr);

BKP_EXPORTED void * bkp_array_create(size_t stride, size_t groupId, const char * file, unsigned int line);
BKP_EXPORTED void bkp_array_push(void ** ptr_, void * value, const char * file, unsigned int line);
BKP_EXPORTED void bkp_resize_array(void ** ptr, size_t size, const char * file, unsigned int line);
BKP_EXPORTED void bkp_destroy_array(void ** ptr);
BKP_EXPORTED void bkp_array_reserve(void ** ptr, size_t s, const char * file, unsigned int line);
BKP_EXPORTED void bkp_arrayInsertAt(void ** ptr, void * value, size_t pos, const char * file, unsigned int line);
BKP_EXPORTED size_t bkp_array_size(void * array);
BKP_EXPORTED size_t bkp_array_capacity(void * array);
BKP_EXPORTED void * bkp_array_find(void * ptr, void * data, BkpContainerFunction func);
BKP_EXPORTED void bkp_array_clear(void * ptr);
BKP_EXPORTED void bkp_array_pop(void * ptr);

#ifdef __cplusplus
}
#endif

#endif

