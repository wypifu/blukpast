#ifndef BKP_LIST_F_H
#define BKP_LIST_F_H


#ifdef __cplusplus
extern "C" {
#endif

#include "bkp_export.h"
#include "bkp_container_functions.h"
/*********************************************************************
 * Defines
*********************************************************************/

#define BkpList(object) object*

#define bkpListCreate(typ) bkp_list_create(sizeof(typ), 0, __FILE__, __LINE__)
#define bkpListCreateFrom(typ, id) bkp_list_create(sizeof(typ), id, __FILE__, __LINE__)

#define bkpListFirst(ptr) bkp_list_first(ptr)
#define bkpListLast(ptr) bkp_list_last(ptr)
#define bkpListNext(iterator) bkp_list_next(iterator)
#define bkpListPrevious(iterator) bkp_list_previous(iterator)
#define bkpListRemove(ptr, iterator) bkp_list_remove((void **) ptr, iterator);
#define bkpListPop(ptr, value) bkp_list_pop((void **) ptr, value)
#define bkpListClear(ptr) bkp_list_clear((void **) ptr)
#define bkpListDestroy(ptr) bkp_list_destroy((void **) ptr)
#define bkpListSize(ptr) bkp_list_size((void *) ptr)

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
BKP_EXPORTED void * bkp_list_create(size_t stride, size_t groupId, const char * file, unsigned int line);
BKP_EXPORTED void bkp_list_add(void ** ptr, void * value);
BKP_EXPORTED void * bkp_list_first(void * ptr);
BKP_EXPORTED void * bkp_list_last(void * ptr);
BKP_EXPORTED void * bkp_list_next(void * iterator);
BKP_EXPORTED void * bkp_list_previous(void * iterator);
BKP_EXPORTED void bkp_list_insert_before(void ** ptr, void * iterator, void * value);
BKP_EXPORTED void bkp_list_insert_after(void ** ptr,  void * iterator,void * value);
BKP_EXPORTED void bkp_list_remove(void ** ptr, void * it);
BKP_EXPORTED void bkp_list_pop(void ** ptr, void * value);
BKP_EXPORTED void bkp_list_clear(void **ptr);
BKP_EXPORTED void bkp_list_destroy(void ** ptr);
BKP_EXPORTED size_t bkp_list_size(void * ptr);
BKP_EXPORTED void * bkp_list_find(void * ptr, void * data, BkpContainerFunction func);

#ifdef __cplusplus
}
#endif

#endif

