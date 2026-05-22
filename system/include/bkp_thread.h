#ifndef BKP_THREAD_H_
#define BKP_THREAD_H_

#ifdef __cplusplus
extern "C" {
#endif

#ifdef _WIN32

#include <windows.h>

#else

#include <unistd.h>
#include <pthread.h>

#endif

#include "bkp_export.h"

typedef struct
{
#ifdef _WIN32
	HANDLE mutex;
	DWORD result;
#else
	pthread_mutex_t mutex;
	unsigned int result;
#endif
	int initialized;
}BkpMutex;


BKP_EXPORTED void bkpCreateMutex(BkpMutex * mu);
BKP_EXPORTED int bkpLockMutex(BkpMutex * mu);
BKP_EXPORTED int bkpUnlockMutex(BkpMutex * mu);
BKP_EXPORTED void bkpDestroyMutex(BkpMutex * mu);

#ifdef __cplusplus
}
#endif

#endif /* BKP_THREAD_H_ */
