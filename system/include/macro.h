#ifndef MACRO_H_
#define MACRO_H_

#include <stdlib.h>

/**************************************************************************
###########################################################################
*
*		Type & Enum Def
*
###########################################################################
**************************************************************************/

/**************************************************************************
###########################################################################
*
*		Macro Def
*
###########################################################################
**************************************************************************/

#define CRASH()	*(int *) 0 = 0

#define REALLOC(type,array,size){\
	type* ptmp0=realloc(array,sizeof(type)*size);\
	if(ptmp0==NULL)\
	{free(array);\
		fprintf(stderr,"Allocation error in %s %d\n",__func__,__LINE__);\
		exit(EXIT_FAILURE);}\
	array=ptmp0;\
}

#define ALLOC(ptrType,ptrVarr,ptrSize){ptrVarr=(ptrType*)malloc(sizeof(ptrType) *(ptrSize));\
	if(ptrVarr == NULL){fprintf(stderr,"Allocation Error in %s at %s:%d\n",__FILE__,__func__,__LINE__);exit(1);}}

#define ALLOC_L(ptrType,ptrVarr,ptrSize){ptrVarr=(ptrType*)malloc(sizeof(ptrType) *(ptrSize));\
	if(ptrVarr == NULL){bkp_log(eFATAL, "System", NULL, __FILE__, __func__, __LINE__, "Allocation Error\n");CRASH();}}

#define CALLOC(ptrType,ptrVarr,ptrSize){ptrVarr=(ptrType*)calloc(ptrSize, sizeof(ptrType));\
	if(ptrVarr == NULL){fprintf(stderr,"Allocation Error in %s at %s:%d\n",__FILE__,__func__,__LINE__);exit(1);}}

#define CALLOC_L(ptrType,ptrVarr,ptrSize){ptrVarr=(ptrType*)calloc(ptrSize, sizeof(ptrType));\
	if(ptrVarr == NULL){bkp_log(eFATAL, "System", NULL, "Allocation Error\n",__FILE__,__func__,__LINE__);CRASH();}}

#define MSG_EXIT(msg,exit_val){fprintf(stderr,"%s\n",msg);exit(exit_val);}

#define DIE(msg,exit_val){fprintf(stderr,"DIE [%s -> %s : %d] -> %s\n",__FILE__,__func__,__LINE__,msg);exit(exit_val);}
#define DIE_L(msg,TAG, exit_val){bkp_log(eFATAL, TAG , NULL, __FILE__, __func__, __LINE__, "%s\n",msg);exit(exit_val);}

#define M_ABS_MAX(x,y)((abs(x) > (abs(y))? abs(x) : abs(y)))


#ifdef DEBUG
#define ASSERT(exp) {if((exp) == 0){fprintf(stderr,\
		"Assertion failed \n\t[%s -> %s : %d]\n\t\"%s\"\n",__FILE__,__func__,__LINE__,#exp);*(int *)0 = 0;}}
#define ASSERT_L(exp) {if((exp) == 0){bkp_logger_write(BKP_LOGGER_FATAL,\
		"Assertion failed \n\t[%s -> %s : %d]\n\t\"%s\"\n",__FILE__,__func__,__LINE__,#exp);*(int *)0 = 0;}}
#else
#define ASSERT(exp)
#define ASSERT_L(exp)
#endif



/**************************************************************************
###########################################################################
*
*		Structures Def
*
###########################################################################
**************************************************************************/

/**************************************************************************
###########################################################################
*
*		Functions Def
*
###########################################################################
**************************************************************************/

#endif

