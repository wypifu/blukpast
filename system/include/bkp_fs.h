#ifndef BKP_FS_H_
#define BKP_FS_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stddef.h>

#include "bkp_export.h"
#include "bkp_path.h"

/*********************************************************************
 * Defines
*********************************************************************/


#define BKP_MAX_FILE_PATH 256
#define BKP_MAX_BASE_PATH 512
#define BKP_MAX_PATH_LENGTH BKP_MAX_BASE_PATH + BKP_MAX_FILE_PATH

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
BKP_EXPORTED char * bkp_loadTxtFile(const char * path);
BKP_EXPORTED char * bkp_loadBinary(const char * path, size_t * s_size);
BKP_EXPORTED char * bkp_readBufferLine(char * buffer, size_t max_line, char * line, uint32_t * line_length);
BKP_EXPORTED const char * bkp_getFileExtension(const char * path);
#ifdef __cplusplus
}
#endif

#endif

