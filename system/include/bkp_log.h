#ifndef BKP_LOG_H_
#define BKP_LOG_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

#include "bkp_export.h"

/*********************************************************************
 * Defines
*********************************************************************/

#define GDLOG_LINESIZE 1024

#define LOG(LEVEL, TAG, ...) bkp_log(LEVEL, TAG, "", __FILE__, __func__, __LINE__, __VA_ARGS__)
#define LOGC(LEVEL, TAG, color, ...) bkp_log(LEVEL, TAG, color, __FILE__, __func__, __LINE__, __VA_ARGS__)

/*********************************************************************
 * Type def & enum
*********************************************************************/

enum {eVERBOSE, eDEBUG, eINFO, eWARNING, eERROR, eFATAL, ePRIORITY};
enum {eTERMOUT = 1,eFILEOUT = 2,eDRAWOUT = 4, eALLOUT = 100};
enum {eONONE, eOWRITE, eOAPPEND};


/*********************************************************************
 * Struct
*********************************************************************/

/*********************************************************************
 * Functions
*********************************************************************/

BKP_EXPORTED int bkp_log_init(int level, int output, const char * logdir, char append);
BKP_EXPORTED void bkp_log(int level, const char * tag, const char * colors, const char * fName, const char * funcName, int fLine, const char * message,...);
BKP_EXPORTED void bkp_Log_terminate(void);
BKP_EXPORTED void bkp_log_setMaxFileSize(uint32_t size);

#ifdef __cplusplus
}
#endif

#endif /* BKP_LOG_H */
