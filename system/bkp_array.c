#ifdef __cplusplus
extern "C" {
#endif
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "include/bkp_array.h"
#include "include/bkp_allocator.h"

/**************************************************************************
*	Defines & Maro
**************************************************************************/

/**************************************************************************
*	Structs, Enum, Unio and Typesdef
**************************************************************************/

/**************************************************************************
*	Globals
**************************************************************************/

/***************************************************************************
*	Prototypes
**************************************************************************/

/*_________________________________________________________________________________*/
static uint8_t * getArrayHeader(void * arr)
{
	size_t nSize = *((size_t *) arr - 1);
	uint64_t addr = (uint64_t)((uint64_t *) arr) - nSize;
	return  (uint8_t *)addr;
}


/**************************************************************************
*	Implementations
**************************************************************************/

/*_________________________________________________________________________________*/
void * bkp_array_create(size_t stride, size_t groupId, const char * file, unsigned int line)
{
	//fprintf(stderr, "***Creating array\n");
	size_t alignment = bkpGetMemoryDefaultAligment();
	size_t nSize = sizeof(size_t) * 4;
	if(nSize % alignment != 0)
	{
		nSize = (nSize / alignment + 1) * alignment;
	}

	//void * p = bkpAlloc_f((sizeof(size_t ) * 3), alignment, groupId, file, line);
	void * p = bkpAlloc_f(nSize , alignment, groupId, file, line);
	*((size_t *)p) = 0;
	*((size_t *)p + 1) = 0;
	*((size_t *)p + 2) = stride;

	uint64_t addr = (uint64_t)p + (nSize);
	uint8_t * returnPtr = (uint8_t *)addr;
	*((size_t *)returnPtr - 1) = nSize;

	//fprintf(stderr, "***after creating Array should give %llu : %d\n", addr, addr % alignment == 0);
	//fprintf(stderr, "\t new addr %llu\n", (uint64_t)((uint64_t *)returnPtr));

	return returnPtr;
}
/*_________________________________________________________________________________*/
size_t bkp_array_capacity(void * array)
{
	uint8_t * p = getArrayHeader(array);
	return *((size_t *)p);
}

/*_________________________________________________________________________________*/
size_t bkp_array_size(void * array)
{
	uint8_t * p = getArrayHeader(array);
	return *((size_t *)p + 1);
}

/*_________________________________________________________________________________*/
size_t bkp_arrayStride(void * array)
{
	uint8_t * p = getArrayHeader(array);
	return *((size_t *)p + 2);
}

/*_________________________________________________________________________________*/
void bkp_destroy_array(void ** ptr)
{
	uint8_t * p = getArrayHeader(*ptr);
	bkpFree(p);
}

/*_________________________________________________________________________________*/
void bkp_array_change_capacity(void ** ptr_, size_t newSize, const char * file, unsigned int line)
{
	//fprintf(stderr, "___ resizing array with %llu elements\n", newSize);
	void * ptr = *ptr_;
	size_t capacity = newSize;
	size_t size= bkp_array_size(ptr);
	size_t stride = bkp_arrayStride(ptr);
	size_t nSize = *((size_t *) ptr - 1);

	uint8_t * q = getArrayHeader(ptr);
	ptr = bkpRealloc_f(q, (stride * capacity) + nSize, 0,  file, line);
	/* bkpRealloc always succeed or crash the programe */

	if(newSize < size)
	{
		size = newSize;
	}

	*((size_t *)ptr) = capacity;
	*((size_t *)ptr + 1) = size;
	*((size_t *)ptr + 2) = stride;

	uint64_t addr = (uint64_t)ptr + (nSize);
	uint8_t * returnPtr = (uint8_t *)addr;
	*((size_t *)returnPtr - 1) = nSize;

	*ptr_ = returnPtr;
	//fprintf(stderr, "___ after resize Array should give %llu : %d\n", addr, addr % bkpGetMemoryDefaultAligment() == 0);
	//fprintf(stderr, "\t new addr %llu\n", (uint64_t)((uint64_t *)returnPtr));
}

/*_________________________________________________________________________________*/
void bkp_array_reserve(void ** ptr, size_t s, const char * file, unsigned int line)
{
	size_t capacity = bkp_array_capacity(*ptr);
	if(s < capacity)
	{
		return;
	}

	bkp_array_change_capacity(ptr, s, file, line);
}

/*_________________________________________________________________________________*/
void bkp_resize_array(void ** ptr, size_t size, const char * file, unsigned int line)
{
	bkp_array_change_capacity(ptr, size, file, line);
	uint8_t * p = getArrayHeader(*ptr);
	*((size_t *)p + 1) = size;
}

/*_________________________________________________________________________________*/
void bkp_array_push(void ** ptr_, void * value, const char * file, unsigned int line)
{
	void * ptr = *ptr_;
	size_t size = bkp_array_size(ptr);
	size_t capacity  = bkp_array_capacity(ptr);

	if(size >= capacity)
	{
		capacity = (capacity == 0) ? 1 : capacity * 2;
		bkp_array_change_capacity(ptr_, capacity, file, line);
		ptr = *ptr_;
		size = bkp_array_size(ptr);
	}

	size_t stride = bkp_arrayStride(ptr);
	uint8_t * p = (uint8_t *) ptr + (size * stride);
	memcpy(p, value, stride);
	p = getArrayHeader(ptr);
	*((size_t *)p + 1) += 1;
}

/*_________________________________________________________________________________*/
void bkp_arrayRemoveAt(void * ptr, size_t pos)
{
	size_t stride = bkp_arrayStride(ptr);
	uint8_t * p = (uint8_t *) ptr + (pos * stride);
	memmove(p, (void *)(p + stride), stride * (bkp_array_size(ptr) - pos - 1));
	p = getArrayHeader(ptr);
	*((size_t *)p + 1) -= 1;
}

/*_________________________________________________________________________________*/
void bkp_arrayInsertAt(void ** ptr, void * value, size_t pos, const char * file, unsigned int line)
{
	size_t size = bkp_array_size(*ptr);
	size_t total = size - pos;
	size_t capacity  = bkp_array_capacity(*ptr);
	size_t stride = bkp_arrayStride(*ptr);
	if(size >= capacity)
	{
		capacity = (capacity == 0) ? 1 : capacity * 2;
		bkp_array_change_capacity(ptr, capacity, file, line);
		size = bkp_array_size(*ptr);
	}

	uint8_t * p = (uint8_t * )*ptr + (pos * stride);
	memmove(p + stride, p, stride *  total);
	memcpy(p, value, stride);
	p = getArrayHeader(*ptr);
	*((size_t *)p + 1) += 1;
}

/*_________________________________________________________________________________*/
void bkp_array_pop(void * ptr)
{
	uint8_t * p = getArrayHeader(ptr);
	*((size_t *)p + 1) -= 1;
}

/*_________________________________________________________________________________*/
void bkp_array_clear(void * ptr)
{
	uint8_t * p = getArrayHeader(ptr);
	*((size_t *)p + 1) = 0;
}

/*_________________________________________________________________________________*/
void * bkp_array_find(void * ptr, void * data, BkpContainerFunction func)
{
	size_t s = bkp_array_size(ptr);
	size_t stride = bkp_arrayStride(ptr);
	uint64_t addr = (uint64_t )ptr;
	for(size_t i = 0; i < s; ++i, addr += stride)
	{
		uint64_t * p = (uint64_t *)addr;
		if(func(data, p) == 1)
		{
			return p;
		}
	}
	return NULL;
}

/*_________________________________________________________________________________*/

/*_________________________________________________________________________________*/

/*_________________________________________________________________________________*/


/******************** static functions ********************************/

/*_________________________________________________________________________________*/

/*_________________________________________________________________________________*/

/*_________________________________________________________________________________*/

/*_________________________________________________________________________________*/

/*_________________________________________________________________________________*/

#ifdef __cplusplus
}
#endif
