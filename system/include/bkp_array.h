#ifndef BKP_ARRAY_H_
#define BKP_ARRAY_H_

#include <stdint.h>
#include "bkp_array_f.h"

#ifdef __cplusplus
extern "C" {
#endif

#define bkpArrayFind(ptr, value, func)\
		({\
		__auto_type value___ = value;\
		bkp_array_find((void *) ptr, &value___, func);\
		})\

#define bkpArrayPush(ptr, value)\
		({\
		__auto_type value___ = value;\
		bkp_array_push((void **)ptr, &value___, __FILE__, __LINE__);\
		})\

#define bkpArrayInsertAt(ptr, value, pos)\
		({\
		__auto_type value___ = value;\
		bkp_arrayInsertAt((void **)ptr, &value___, pos, __FILE__, __LINE__);\
		})\

#ifdef __cplusplus
}
#endif


#endif
