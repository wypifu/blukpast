#ifdef __cplusplus
extern "C" {
#endif

#include <stdlib.h>
#include <stdio.h>
#include <string.h>


#include "include/bkp_fs.h"
#include "include/bkp_log.h"
#include "include/macro.h"
#include "include/bkp_allocator.h"



/*Copyright*/
/*
 * AUTHOR: lilington
 * addr	: lilington@yahoo.fr
 *
 */

/**************************************************************************
*	Defines & Maro
**************************************************************************/

/**************************************************************************
*	Structs, Enum, Unio and Typesdef
**************************************************************************/

/**************************************************************************
*	Globals
**************************************************************************/
/*
    __linux__       Defined on Linux
    __sun           Defined on Solaris
    __FreeBSD__     Defined on FreeBSD
    __NetBSD__      Defined on NetBSD
    __OpenBSD__     Defined on OpenBSD
    __APPLE__       Defined on Mac OS X
    __hpux          Defined on HP-UX
    __osf__         Defined on Tru64 UNIX (formerly DEC OSF1)
    __sgi           Defined on Irix
    _AIX            Defined on AIX
    _WIN32          Defined on Windows
*/

#ifdef _WIN32
    const char gPathDelim = '\\';
#else
    const char gPathDelim = '/';
#endif

/***************************************************************************
*	Prototypes
**************************************************************************/

/**************************************************************************
*	Main
**************************************************************************/



/**************************************************************************
*	Implementations
**************************************************************************/

/*_________________________________________________________________________________*/
char * bkp_loadTxtFile(const char * path)
{
    FILE *inf = NULL;
    size_t s_file,
           val;
    char *str = NULL;

    inf = fopen(path,"rb");
    if(inf==NULL)
        return str;

    fseek(inf,0,SEEK_END);
    s_file = ftell(inf);

	str = bkpAlloc(sizeof(char) * s_file + 1);
    //ALLOC_L(char,str,(s_file+1));

    fseek(inf,0,SEEK_SET);
    val=fread(str,s_file,sizeof(char),inf);
    if((val<1)&&(ferror(inf)!=0))
    {
        fprintf(stderr,"I/O error in %s %d\n",__func__,__LINE__-3);
        bkpFree(str);
        return NULL;
    }
    fclose(inf);

    str[s_file]='\0';

    return str;
}

/*_________________________________________________________________________________*/
char * bkp_loadBinary(const char * path, size_t * s_size)
{
    FILE *inf = NULL;
    size_t s_file,
           val;
    char *str = NULL;

    inf = fopen(path,"rb");
    if(inf==NULL)
        return str;

    fseek(inf,0,SEEK_END);
    s_file = ftell(inf);

    //ALLOC_L(char,str,(s_file));
	str = bkpAlloc(sizeof(char) * s_file);

    fseek(inf,0,SEEK_SET);
    val=fread(str,s_file,sizeof(char),inf);
    if((val<1)&&(ferror(inf)!=0))
    {
        fprintf(stderr,"I/O error in %s %d\n",__func__,__LINE__-3);
        bkpFree(str);
        return NULL;
    }
    fclose(inf);

	*s_size = s_file;

    return str;
}

/*_________________________________________________________________________________*/
const char * bkp_getFileExtension(const char * path)
{
	size_t path_len = strlen(path);

	for(size_t i = path_len -1; i > 0; --i)
	{
		if(path[i] == '.')
		{
			return &path[i + 1];
		}
	}

	return NULL;
}

/*_________________________________________________________________________________*/
char * bkp_readBufferLine(char * buffer, size_t max_line, char * line, uint32_t * line_length)
{
	if(*buffer == '\0')
	{
		return NULL;
	}

	char * pB = buffer;
	char * pE = pB;

	while(pE)
	{
		if(*pE == 0x0A || *pE == 0x0)
		{
			uint32_t i = 0;
			for(i = 0; pB <= pE; ++pB, ++i)
			{
				line[i] = *pB;
				if(i == max_line)
				{
					break;
				}
			}

			line[i] = '\0';
			if(*pE != 0x0)
			{
				if(*(pE -1) == '\r')
				{
					--i;
				}
				++pE;
			}
			else
			{
				--i;
			}
			*line_length = i;
			return pE;
		}
		++pE;
	}

	return pE;
}

/*_________________________________________________________________________________*/

/******************** static functions ********************************/

/*_________________________________________________________________________________*/

/*_________________________________________________________________________________*/
#ifdef __cplusplus
}
#endif
