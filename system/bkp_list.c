#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "include/bkp_list.h"
#include "include/bkp_allocator.h"

/**************************************************************************
*	Defines & Maro
**************************************************************************/

/**************************************************************************
*	Structs, Enum, Unio and Typesdef
**************************************************************************/
typedef struct
{
	size_t size;
	void * first;
	void * last;
	size_t stride;
	size_t groupId;
}ListHeader;

typedef struct
{
	void * prev;
	void * next;
	char payload [];
}Entry;

/**************************************************************************
*	Globals
**************************************************************************/

/***************************************************************************
*	Prototypes
**************************************************************************/

/**************************************************************************
*	Implementations
**************************************************************************/

/*_________________________________________________________________________________*/
void * bkp_list_create(size_t stride, size_t groupId, const char * file, unsigned int line)
{
	ListHeader * p = bkpAlloc_f(sizeof(ListHeader), bkpGetMemoryDefaultAligment(), groupId, file, line);
	p->size = 0;
	p->stride = stride;
	p->groupId = groupId;
	p->first = NULL;
	p->last = NULL;

	return p;
}

/*_________________________________________________________________________________*/
void bkp_list_add(void ** ptr_, void * value)
{
	ListHeader * ptr = (ListHeader *)*ptr_;
	Entry * p = bkpAlloc_f(ptr->stride + sizeof(void *) * 2, bkpGetMemoryDefaultAligment(), ptr->groupId, __FILE__, __LINE__);
	memcpy(p->payload , value, ptr->stride);
	p->prev = ptr->last;
	p->next = NULL;
	if(ptr->first == NULL)
	{
		ptr->first = p;
	}
	if(p->prev != NULL)
	{
		((Entry *)p->prev)->next = p;
	}
	ptr->last = p;
	++ptr->size;
}

/*_________________________________________________________________________________*/
void * bkp_list_first(void * ptr_)
{
	ListHeader * ptr = (ListHeader *)ptr_;
	if(ptr->first == NULL) return NULL;
	return ((Entry * )ptr->first)->payload;
}

/*_________________________________________________________________________________*/
void * bkp_list_last(void * ptr_)
{
	ListHeader * ptr = (ListHeader *)ptr_;
	if(ptr->last == NULL) return NULL;
	return ((Entry * )ptr->last)->payload;
}

/*_________________________________________________________________________________*/
void * bkp_list_next(void * it)
{
	Entry * p = it - sizeof(void *) * 2;

	return p->next == NULL ? NULL : ((Entry *)p->next)->payload;
}

/*_________________________________________________________________________________*/
void * bkp_list_previous(void * it)
{
	Entry * p = it - sizeof(void *) * 2;
	return p->prev == NULL ? NULL : ((Entry *)p->prev)->payload;
}

/*_________________________________________________________________________________*/
void bkp_list_insert_before(void ** ptr_, void * it, void * value)
{
	ListHeader * ptr = (ListHeader *)*ptr_;
	Entry * p = bkpAlloc_f(ptr->stride + sizeof(void *) * 2, bkpGetMemoryDefaultAligment(), ptr->groupId, __FILE__, __LINE__);
	memcpy(p->payload , value, ptr->stride);

	Entry * p_0 = it - sizeof(void *) * 2;
	p->next = p_0;
	p->prev = p_0->prev;
	p_0->prev = p;
	if(p->prev != NULL)
	{
		((Entry *)p->prev)->next = p;
	}
	++ptr->size;
	if(ptr->first == p_0)
	{
		ptr->first = p;
	}
}

/*_________________________________________________________________________________*/
void bkp_list_insert_after(void ** ptr_, void * it, void * value)
{
	ListHeader * ptr = (ListHeader *)*ptr_;
	Entry * p = bkpAlloc_f(ptr->stride + sizeof(void *) * 2, bkpGetMemoryDefaultAligment(), ptr->groupId, __FILE__, __LINE__);
	memcpy(p->payload , value, ptr->stride);

	Entry * p_0 = it - sizeof(void *) * 2;
	p->next = p_0->next;
	p->prev = p_0;
	p_0->next = p;
	if(p->next != NULL)
	{
		((Entry *)p->next)->prev = p;
	}
	++ptr->size;
	if(ptr->last == p_0)
	{
		ptr->last = p;
	}
}

/*_________________________________________________________________________________*/
void bkp_list_remove(void ** ptr_, void * it)
{
	ListHeader * ptr = (ListHeader *)*ptr_;
	Entry * p = it - sizeof(void *);
	if(p->prev != NULL)
	{
		((Entry *)p->prev)->next = p->next;
	}

	if(p->next != NULL)
	{
		((Entry *)p->next)->prev = p->prev;
	}
	--ptr->size;
}

/*_________________________________________________________________________________*/
void bkp_list_pop(void ** ptr_, void * value)
{
	ListHeader * ptr = (ListHeader *)*ptr_;
	Entry * p = ptr->first;
	memcpy(value, p->payload, ptr->stride);
	ptr->first = p->next;
	if(p->next != NULL)
	{
		((Entry *)p->next)->prev = NULL;
	}
	bkpFree(p);
	--ptr->size;
}
/*_________________________________________________________________________________*/
void bkp_list_clear(void **ptr_)
{
	ListHeader * ptr = (ListHeader *)*ptr_;
	Entry * p = ptr->first;
	while(p != NULL)
	{
		Entry * q = p;
		bkpFree(q);
		p = p->next;
	}
	ptr->first = ptr->last = NULL;
	ptr->size = 0;
}

/*_________________________________________________________________________________*/
void bkp_list_destroy(void ** ptr_)
{
	bkp_list_clear(ptr_);
	bkpFree(*ptr_);
}

/*_________________________________________________________________________________*/
size_t bkp_list_size(void * ptr)
{
	return *((size_t *) ptr);
}

/*_________________________________________________________________________________*/
void * bkp_list_find(void * ptr_, void * data, BkpContainerFunction func)
{
	ListHeader * ptr = (ListHeader *)ptr_;
	Entry * p = ptr->first;
	while(p != NULL)
	{
		if(func(data, p->payload) == 1)
		{
			return p->payload;
		}
		p = p->next;
	}
	return NULL;
}


/******************** static functions ********************************/

/*_________________________________________________________________________________*/

/*_________________________________________________________________________________*/

/*_________________________________________________________________________________*/

/*_________________________________________________________________________________*/

/*_________________________________________________________________________________*/



