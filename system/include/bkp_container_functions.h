#ifndef BKP_CONTAINER_H_
#define BKP_CONTAINER_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

typedef uint8_t (*BkpContainerFunction )(void * target, void * stored);

uint8_t bkpContainerFindPtr(void * target, void * source);
uint8_t bkpContainerFindU64(void * target, void * source);
uint8_t bkpContainerFindU32(void * target, void * source);
uint8_t bkpContainerFindU16(void * target, void * source);
uint8_t bkpContainerFindU8(void * target, void * source);

#ifdef __cplusplus
}
#endif

#endif
