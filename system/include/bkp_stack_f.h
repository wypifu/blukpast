#ifndef BKP_STACK_F_H
#define BKP_STACK_F_H

#ifdef __cplusplus
extern "C" {
#endif

#include "bkp_export.h"

/*********************************************************************
 * Defines
*********************************************************************/

#define BkpStack(object) object*

#define bkpStackCreate(typ) bkp_stack_create(sizeof(typ), 0, __FILE__, __LINE__)
#define bkpStackCreateFrom(typ, id) bkp_stack_create(sizeof(typ), id, __FILE__, __LINE__)

#define bkpStackDestroy(ptr) bkp_stack_destroy((void **) ptr)
#define bkpStackSize(ptr) bkp_stackSize((void *) ptr)
#define bkpStackPop(ptr, value) bkp_stack_pop((void **) ptr, value)

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
BKP_EXPORTED void * bkp_stack_create(size_t stride, size_t groupId, const char * file, unsigned int line);
BKP_EXPORTED void bkp_stack_push(void ** ptr_, void * value);
BKP_EXPORTED void bkp_stack_pop(void ** ptr_, void * value);
BKP_EXPORTED void bkp_stack_destroy(void ** ptr_);
BKP_EXPORTED size_t bkp_stackSize(void * ptr_);

#ifdef __cplusplus
}
#endif

#endif

