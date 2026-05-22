#ifdef __cplusplus
extern "C" {
#endif

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
typedef struct
{
	size_t size;
	size_t stride;
	size_t groupId;
	void * next;
}StackHeader;

typedef struct
{
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
void * bkp_stack_create(size_t stride, size_t groupId, const char * file, unsigned int line)
{
	StackHeader * p = bkpAlloc_f(sizeof(StackHeader), bkpGetMemoryDefaultAligment(), groupId, file, line);
	p->size = 0;
	p->stride = stride;
	p->groupId = groupId;
	p->next = NULL;

	return p;
}

/*_________________________________________________________________________________*/
void bkp_stack_push(void ** ptr_, void * value)
{
	StackHeader * ptr = (StackHeader *)*ptr_;
	Entry * p = bkpAlloc_f(ptr->stride + sizeof(void *), bkpGetMemoryDefaultAligment(), ptr->groupId, __FILE__, __LINE__);
	p->next = ptr->next;
	memcpy(p->payload , value, ptr->stride);
	ptr->next = p;
	++ptr->size;
}

/*_________________________________________________________________________________*/
void bkp_stack_pop(void ** ptr_, void * value)
{
	StackHeader * ptr = (StackHeader *) *ptr_;
	if(ptr->next == NULL) return;
	Entry * p = ptr->next;
	memcpy(value, p->payload, ptr->stride);
	ptr->next = p->next;
	bkpFree(p);
	--ptr->size;
}

/*_________________________________________________________________________________*/
void bkp_stack_destroy(void ** ptr_)
{
	StackHeader * ptr = (StackHeader *)*ptr_;
	Entry * p = ptr->next;
	while(p != NULL)
	{
		Entry * q = p;
		p = p->next;
		bkpFree(q);
	}

	bkpFree(ptr);
}

/*_________________________________________________________________________________*/
size_t bkp_stackSize(void * ptr)
{
	return *((size_t *) ptr);
}


/******************** static functions ********************************/

/*_________________________________________________________________________________*/

/*_________________________________________________________________________________*/

/*_________________________________________________________________________________*/

/*_________________________________________________________________________________*/

/*_________________________________________________________________________________*/


#ifdef __cplusplus
}
#endif

