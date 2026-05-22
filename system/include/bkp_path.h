#ifndef BKP_PATH_H
#define BKP_PATH_H

#ifdef __cplusplus
extern "C" {
#endif

#include "bkp_export.h"

#define BKP_MAX_PATH_SIZE 512
#define BKP_MAX_FILENAME_SIZE 256
#define BKP_MAX_EXTENSION_SIZE 16
#define BKP_MAX_FULL_PATH_SIZE BKP_MAX_PATH_SIZE + BKP_MAX_FILENAME_SIZE + BKP_MAX_EXTENSION_SIZE


typedef struct
{
	char directory[BKP_MAX_PATH_SIZE];
	char base_name[BKP_MAX_FILENAME_SIZE];
	char file_name[BKP_MAX_FILENAME_SIZE];
	char extension[BKP_MAX_EXTENSION_SIZE];
}BkpPath;

BKP_EXPORTED void        bkpDos2posix(char * path);
BKP_EXPORTED int         bkpSplitPath(BkpPath * spath, const char * path);
BKP_EXPORTED int         bkpPathExists(const char * path);
BKP_EXPORTED int         bkpCreateDirectory(const char * path);
BKP_EXPORTED const char *bkpExePath(const char * relative);

#ifdef __cplusplus
}
#endif

#endif

