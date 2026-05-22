#ifndef BKP_STRING_H
#define BKP_STRING_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>
#include <stdint.h>
#include "bkp_export.h"

/*********************************************************************
 * Defines
*********************************************************************/
#define BkpString char*

#define bkpStringCreate(str) bkp_string_create(str, 0, __FILE__, __LINE__)
#define bkpStringCreateFrom(str, id) bkp_string_create(str, id, __FILE__, __LINE__)
#define bkpStringDestroy(str) bkp_string_destroy(str)
#define bkpStringcpy(dest, src) bkp_string_cpy(dest, src)
#define bkpStringcat(dest, src) bkp_string_cat(dest, src)
#define bkpStringLength(str) bkp_string_length(str)
#define bkpStringFormat(str, format, ...) bkp_string_format(str, format, __VA_ARGS__)

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
BKP_EXPORTED char * bkp_string_create(const char * src, size_t groupId, const char * file, unsigned int line);
BKP_EXPORTED void bkp_string_destroy(BkpString * str);
BKP_EXPORTED void bkp_string_cpy(BkpString * str, const char * src);
BKP_EXPORTED void bkp_string_cat(BkpString * str, const char * src);
BKP_EXPORTED size_t bkp_string_length(BkpString str);
BKP_EXPORTED void bkp_string_format(BkpString * str, const char * format, ...);

#ifdef __cplusplus
}
#endif

#endif

