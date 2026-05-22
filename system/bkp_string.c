#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>
#include <string.h>
#include <stdarg.h>

#include "include/bkp_string.h"
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
	size_t capacity;
	size_t groupId;
}
StringHeader;

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

/*_________________________________________________________________________________*/

/*_________________________________________________________________________________*/

/*_________________________________________________________________________________*/

/*_________________________________________________________________________________*/


/******************** static functions ********************************/

/*_________________________________________________________________________________*/
char * bkp_string_create(const char * src, size_t groupId, const char * file, unsigned int line)
{
	size_t s = 1;
	if(src != NULL)
	{
		s += strlen(src);
	}
	char * str = bkpAlloc_f(sizeof(StringHeader) + s, 1, groupId, file, line);
	StringHeader * p = (StringHeader *) str;
	p->size = s - 1;
	p->capacity = s;
	p->groupId = groupId;

	str += sizeof(StringHeader);
	strcpy(str, src);

	return str;
}

/*_________________________________________________________________________________*/
void bkp_string_destroy(BkpString *str_)
{
    if(*str_ == NULL) return;
	char * str = *str_ - sizeof(StringHeader);
	StringHeader * h = (StringHeader *)str;
	bkpFree(h);
}

/*_________________________________________________________________________________*/
void bkp_string_cpy(BkpString * str_, const char * src)
{
	char * str = *str_;
	StringHeader * h = (StringHeader *)(str - sizeof(StringHeader));
	size_t  s = strlen(src) + 1;
	if(s > h->capacity)
	{
		h = bkpRealloc_f(h,  sizeof(StringHeader) + s, bkpGetMemoryDefaultAligment(), __FILE__, __LINE__);
		str = (char*) h + sizeof(StringHeader);
		str[h->size] = '\0';
	}

	strcpy(str, src);
	h->capacity = s;
	h->size = s - 1;
	*str_ = str;
}

/*_________________________________________________________________________________*/
void bkp_string_cat(BkpString * str_, const char * src)
{
	char * str = *str_;
	StringHeader * h = (StringHeader *)(str - sizeof(StringHeader));
	size_t  s = strlen(src);
	size_t t = s + h->size + 1;

	if(t >  h->capacity)
	{
		h = bkpRealloc_f(h,  sizeof(StringHeader) + t, bkpGetMemoryDefaultAligment(), __FILE__, __LINE__);
		str = (char*) h + sizeof(StringHeader);
		str[h->size] = '\0';
	}
	strcat(str, src);
	h->capacity = t;
	h->size = t - 1;
	*str_ = str;
}

/*_________________________________________________________________________________*/
size_t bkp_string_length(char * str)
{
	char * p = str - sizeof(StringHeader);
	return ((StringHeader *) p)->size;
}

/*_________________________________________________________________________________*/
void bkp_string_format(BkpString * str_, const char * format, ...)
{
	va_list argptr;

	char * str = *str_;
	size_t s = strlen(format) + 1 + 256;
	StringHeader * h = (StringHeader *)(str - sizeof(StringHeader));
	if(s > h->capacity)
	{
		//fprintf(stderr, "REALLOC : \n");
		h = bkpRealloc_f(h,  sizeof(StringHeader) + s, bkpGetMemoryDefaultAligment(), __FILE__, __LINE__);
		str = (char*) h + sizeof(StringHeader);
	}

	va_start(argptr, format);
	vsprintf(str, format, argptr);
	va_end(argptr);
	h->capacity = s;
	h->size = strlen(str);
	*str_ = str;
}

/*_________________________________________________________________________________*/

/*_________________________________________________________________________________*/

/*_________________________________________________________________________________*/


#ifdef __cplusplus
}
#endif

