#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>
#include <stdlib.h>
#include "include/bkp_thread.h"


/*_________________________________________________________________________________*/
void bkpCreateMutex(BkpMutex * mu)
{

#ifdef _WIN32
	mu->mutex = CreateMutex(NULL, FALSE, NULL);
	if(mu->mutex == NULL)
    {
        fprintf(stderr, "CreateMutex error: %lu\n", GetLastError());
        exit(-1);
    }
#else
	if(pthread_mutex_init(&mu->mutex, NULL))
	{
        fprintf(stderr, "CreateMutex error\n");
        exit(-1);
	}
#endif
	mu->initialized = 1;
}

/*_________________________________________________________________________________*/
int bkpLockMutex(BkpMutex * mu)
{
#ifdef _WIN32
	mu->result = WaitForSingleObject(mu->mutex, INFINITE);
	if(mu->result !=  WAIT_OBJECT_0)
	{
		return 0;
	}
#else
	pthread_mutex_lock(&mu->mutex);
#endif
	return 1;
}

/*_________________________________________________________________________________*/
int bkpUnlockMutex(BkpMutex * mu)
{
#ifdef _WIN32
	if (!ReleaseMutex(mu->mutex))
	{
		return 0;
	}
#else
	pthread_mutex_unlock(&mu->mutex);
#endif
	return 1;
}

/*_________________________________________________________________________________*/
void bkpDestroyMutex(BkpMutex * mu)
{
#ifdef _WIN32
	CloseHandle(mu->mutex);
#else
	pthread_mutex_destroy(&mu->mutex);
#endif
	mu->initialized = 0;
}

#ifdef __cplusplus
}
#endif
