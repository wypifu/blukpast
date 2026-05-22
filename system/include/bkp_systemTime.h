#ifndef SYSTEM_TIME_H_
#define SYSTEM_TIME_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <time.h>

#include "bkp_export.h"

/*********************************************************************
 * Defines
*********************************************************************/

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

/** @brief Return a monotonic timestamp in seconds. */
BKP_EXPORTED uint64_t bkp_time_getClock(void);
/** @brief Return a monotonic timestamp in milliseconds. */
BKP_EXPORTED uint64_t bkp_time_getClockMilli(void);
/**
 * @brief Return a monotonic timestamp in microseconds.
 *
 * Use two consecutive calls to measure elapsed time and derive a frame
 * delta-time in seconds:
 * @code
 *   uint64_t prev = bkp_time_getClockMicro();
 *   // ... per-frame work ...
 *   float dt = (float)(bkp_time_getClockMicro() - prev) * 1e-6f;
 * @endcode
 *
 * @return  Current clock value in microseconds.  The epoch is unspecified —
 *          only differences between calls are meaningful.
 */
BKP_EXPORTED uint64_t bkp_time_getClockMicro(void);
/** @brief Return a monotonic timestamp in nanoseconds. */
BKP_EXPORTED uint64_t bkp_time_getClockNano(void);
BKP_EXPORTED time_t bkp_time_getEpoch(void);

BKP_EXPORTED void bkp_time_freeze(uint32_t seconds);
BKP_EXPORTED void bkp_time_mfreeze(uint32_t mseconds);
BKP_EXPORTED void bkp_time_ufreeze(uint32_t nseconds);

#ifdef __cplusplus
}
#endif

#endif
