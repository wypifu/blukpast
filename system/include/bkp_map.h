#ifndef BKP_MAP_H_
#define BKP_MAP_H_

#include <stdint.h>
#include "bkp_map_f.h"

#define bkpMapInsert(ptr, key, value)\
		({\
		__auto_type value___ = value;\
		__auto_type key___ = key;\
		bkp_map_insert(ptr, &key___, &value___);\
		})\

#define bkpMapGet(map, key, value)\
		({\
		__auto_type key___ = key;\
		bkp_map_get(map, &key___, value);\
		})\

#define bkpMapHasKey(map, key)\
		({\
		__auto_type key___ = key;\
		bkp_map_has_key(map, &key___);\
		})\

#define bkpMapRemove(map, key, value)\
		({\
		__auto_type key___ = key;\
		bkp_map_remove(map, &key___, value);\
		})\

#define bkpMapErase(map, key)\
		({\
		__auto_type key___ = key;\
		bkp_map_erase(map, &key___);\
		})\

#endif
