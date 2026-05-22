#ifdef __cplusplus
extern "C" {
#endif

#include <sys/types.h>
#include "include/bkp_systemTime.h"
#include <time.h>

#ifdef _WIN32
#include <windows.h>
#else
#include <sys/select.h>
#endif


/**************************************************************************
*	Defines & Maro
**************************************************************************/

/**************************************************************************
*	Structs, Enum, Union and Typesdef
**************************************************************************/

/**************************************************************************
*	Globals
**************************************************************************/

/**************************************************************************
*	Classes
**************************************************************************/

/***************************************************************************
*	Prototypes
**************************************************************************/

/**************************************************************************
*	Main
**************************************************************************/




/**************************************************************************
*	Implementations
**************************************************************************/

/*--------------------------------------------------------------------------------*/
uint64_t bkp_time_getClockNano()
{
#ifdef _WIN32
	static LARGE_INTEGER freq = {0};
	if (freq.QuadPart == 0) QueryPerformanceFrequency(&freq);
	LARGE_INTEGER counter;
	QueryPerformanceCounter(&counter);
	return (uint64_t)(counter.QuadPart * 1000000000ULL / freq.QuadPart);
#else
	struct timespec now;
	clock_gettime(CLOCK_MONOTONIC_RAW, &now);
	return now.tv_sec  * 1000000000 + now.tv_nsec;
#endif
}

/*--------------------------------------------------------------------------------*/
uint64_t bkp_time_getClockMicro()
{
	uint64_t val = bkp_time_getClockNano();
	return val / 1000;
}

/*--------------------------------------------------------------------------------*/
uint64_t bkp_time_getClockMilli()
{
	uint64_t val = bkp_time_getClockNano();
	return val / 1000000;
}

/*--------------------------------------------------------------------------------*/
uint64_t bkp_time_getClock()
{
	uint64_t val = bkp_time_getClockNano();
	return val / 1000000000;
}

/*--------------------------------------------------------------------------------*/
time_t bkp_time_getEpoch()
{
	time_t     now;
	time(&now);
	return now;
}

/*--------------------------------------------------------------------------------*/
void bkp_time_freeze(uint32_t seconds)
{
#ifdef _WIN32
	HANDLE timer;
	LARGE_INTEGER ft;

	ft.QuadPart = -(10 * (__int64)seconds * 1000 * 1000);

	timer = CreateWaitableTimer(NULL, TRUE, NULL);
	SetWaitableTimer(timer, &ft, 0, NULL, NULL, 0);
	WaitForSingleObject(timer, INFINITE);
	CloseHandle(timer);
#else
	struct timeval tv;
	tv.tv_sec = seconds;
	tv.tv_usec = 0;
	select(0, NULL, NULL, NULL, &tv);
#endif
}
/*--------------------------------------------------------------------------------*/
void bkp_time_mfreeze(uint32_t mseconds)
{
#ifdef _WIN32
	HANDLE timer;
	LARGE_INTEGER ft;

	ft.QuadPart = -(10 * (__int64)mseconds * 1000);

	timer = CreateWaitableTimer(NULL, TRUE, NULL);
	SetWaitableTimer(timer, &ft, 0, NULL, NULL, 0);
	WaitForSingleObject(timer, INFINITE);
	CloseHandle(timer);
#else
	struct timeval tv;
	tv.tv_sec  = mseconds / 1000;
	tv.tv_usec = (mseconds % 1000) * 1000;
	select(0, NULL, NULL, NULL, &tv);
#endif
}
/*--------------------------------------------------------------------------------*/
void bkp_time_ufreeze(uint32_t useconds)
{
#ifdef _WIN32
	HANDLE timer;
	LARGE_INTEGER ft;

	ft.QuadPart = -(10 * (__int64)useconds);

	timer = CreateWaitableTimer(NULL, TRUE, NULL);
	SetWaitableTimer(timer, &ft, 0, NULL, NULL, 0);
	WaitForSingleObject(timer, INFINITE);
	CloseHandle(timer);

#else
	struct timeval tv;
	tv.tv_sec  = useconds / 1000000;
	tv.tv_usec = useconds % 1000000;
	select(0, NULL, NULL, NULL, &tv);
#endif
}

/*--------------------------------------------------------------------------------*/

/*--------------------------------------------------------------------------------*/

#ifdef __cplusplus
}
#endif
