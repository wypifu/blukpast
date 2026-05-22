#ifdef __cplusplus
extern "C" {
#endif

#ifdef _WIN32
#include <windows.h>
#else

#include <unistd.h>
#include <pthread.h>
#include <dirent.h>

#endif

#include <string.h>
#include <stdarg.h>

#include "include/bkp_systemTime.h"
#include "include/bkp_log.h"
#include "include/bkp_path.h"

#ifdef _WIN32
#define getFILENAME(fname00) strrchr(fname00, '\\') ? strrchr(fname00, '\\') + 1 : fname00
#else
#define getFILENAME(fname00) strrchr(fname00, '/') ? strrchr(fname00, '/') + 1 : fname00
#endif
#define M 128 

struct SBuff 
{
  char * data;
  size_t size;
}gBuff;

static const char * TAG = "Blukpast logger";
static int stc_output = eTERMOUT;
static int stc_inited = 0;
#ifdef _WIN32
	HANDLE stc_mutex_;
#else
	static pthread_mutex_t stc_mutex_;
#endif
static FILE * outf;
static int stc_loglevel,
	stc_maxbuffer_line,
	stc_fileOk;

static uint32_t stc_maxFileSize = 1026 * 128;  // around 100kb
static uint32_t stc_currentFileSize ;
static char stc_path [GDLOG_LINESIZE];
static char stc_mydir[GDLOG_LINESIZE / 2];

static char Ll[7][64] = {"\x1b[1;35m VERBOSE \x1b[0m", "\x1b[1;34m  DEBUG  \x1b[0m","\x1b[1;92m  INFO   \x1b[0m","\x1b[0;33m WARNING \x1b[0m",
	"\x1b[0;31m  ERROR  \x1b[0m","\x1b[7;31m  FATAL  \x1b[0m", "\x1b[5;38;2;0;0;0;48;2;200;200;200mEXTERNAL\x1b[0m"};
//static char Ll2[7][32] = {" VERBOSE ", "  DEBUG  ","  INFO   "," WARNING ","  ERROR  ","  FATAL  ", "EXTERNAL"};

/***************************************************************************
*	Prototypes
**************************************************************************/

static void write_file(const char * head, const char * meta, const char * mess);
static void getFileTime(char tmp[64]);

/**************************************************************************
*	Implementations
**************************************************************************/

int bkp_log_init(int level, int output, const char * logfile, char append)
{
	stc_fileOk = 0;
	stc_inited = 0;
	stc_currentFileSize = 0;

	if(level < 0)
	{
		fprintf(stderr, "Bad log level, Exiting\n");
		return -1;
	}

	stc_maxbuffer_line = 1; //default, flush every lines to modify use gdLogger_setFlushAt()
	stc_output = output;
	stc_loglevel = level;

	if(NULL == logfile)
	{
		sprintf(stc_mydir,"%s","./logs");
	}
	else
	{
		sprintf(stc_mydir,"%s",logfile);
	}

	char tmp2[64];

	getFileTime(tmp2);
	sprintf(stc_path, "%s_%s_.log", stc_mydir, tmp2);


	bkpCreateDirectory(stc_mydir);
	stc_fileOk = 1;
	if(append ==  eOAPPEND)
	{
		outf = fopen(stc_path, "at");
		fprintf(stderr, "openning 1 '%s'\n", stc_path);
	}
	else
	{
		outf = fopen(stc_path, "wt");
		fprintf(stderr, "openning 2 '%s'\n", stc_path);
		if(append != eOWRITE)
		{
			bkp_log(eWARNING, TAG, "", __FILE__, __func__, __LINE__, "Unexpected openning file mode, LOGGER will open file as 'w'");
		}
	}

	if(outf == NULL)
	{
		fprintf(stderr, "Cannot open '%s'\n", stc_path);
	}
	else
	{
		stc_fileOk = 1;
		fclose(outf);
	}

#ifdef _WIN32
	stc_mutex_ = CreateMutex(NULL, FALSE, NULL);
#else
	pthread_mutex_t lk = PTHREAD_MUTEX_INITIALIZER;
	stc_mutex_ = lk;
#endif

	stc_inited = 1;
  
  gBuff.size = 1024;
  gBuff.data = (char *)malloc(sizeof(char) * gBuff.size);
  if(gBuff.data == NULL)
  {
    fprintf(stderr, "ALLocation Error\n");
    *(volatile int*)0 = 0;
  }

	return 0;
}

/*--------------------------------------------------------------------------------*/
void bkp_log(int level, const char * tag, const char * color, const char * fName, const char * funcName, int fLine, const char * message,...)
{
	if(0 == stc_inited)
		return;

	FILE* pOUT = stdout;
	if(stc_loglevel <= level)
	{
		time_t     now;
		char head[256];
		char meta[1024];
		char tmp[64];
		struct tm  ts;
		va_list argptr;

		now = bkp_time_getEpoch();

		ts = *localtime(&now);

		strftime(tmp, M, "%Y-%m-%d %H:%M:%S", &ts);

		uint32_t pId;
#ifdef _WIN32
		pId = 0;
#else
		pId = pthread_self();
#endif

		meta[0] = 0;
		if(level != eINFO)
		{
			pOUT = stderr;
			if(fName != NULL)
			{
				sprintf(meta, "|%s(%s : %d)| ", getFILENAME(fName), funcName, fLine);
			}
		}


    size_t l_total;
		va_start(argptr, message);
#ifdef _WIN32
	  l_total = _vscprintf(message, argptr);	
#else
	  l_total = vsnprintf(NULL, 0, message, argptr);	
#endif
		va_end(argptr);

    if(l_total > gBuff.size)
    {
#ifdef  _WIN32
			DWORD dwWaitResult = WaitForSingleObject(stc_mutex_, INFINITE);
			switch(dwWaitResult)
			{
				case WAIT_OBJECT_0:
          free(gBuff.data);
          gBuff.size = l_total + 1;
          gBuff.data = (char *)malloc(sizeof(char) * gBuff.size);
					break;
				case WAIT_ABANDONED:
					break;

			}
#else
			pthread_mutex_lock(&stc_mutex_);
      free(gBuff.data);
      gBuff.size = l_total + 1;
      gBuff.data = (char *)malloc(sizeof(char) * gBuff.size);
			pthread_mutex_unlock(&stc_mutex_);
#endif

      if(gBuff.data == NULL)
      {
        fprintf(stderr, "ALLocation Error\n");
        *(volatile int*)0 = 0;
      }
    }

		va_start(argptr, message);
		vsprintf(gBuff.data, message, argptr);
		va_end(argptr);

		if(stc_output == eTERMOUT || stc_output == (eTERMOUT | eFILEOUT) || stc_output > eDRAWOUT)
		{
			char thead[256];
			sprintf(thead,"[%s][ %u ][\x1b[0;95m %s \x1b[0m][%s %s \x1b[0m]", Ll[level], pId, tmp, color, tag);
#ifdef __linux__

			pthread_mutex_lock(&stc_mutex_);
			if (NULL != color)
			{
				fprintf(pOUT, "%s\x1b[2m%s\x1b[0m%s%s\x1b[0m\n", thead, meta, color, gBuff.data);
			}
			else
			{
				fprintf(pOUT, "%s\x1b[2m%s\x1b[0m %s\n", thead, meta, gBuff.data);
			}
			pthread_mutex_unlock(&stc_mutex_);

#elif _WIN32
			DWORD dwWaitResult = WaitForSingleObject(stc_mutex_, INFINITE);
			switch(dwWaitResult)
			{
				case WAIT_OBJECT_0:
					if (NULL != color)
					{
						fprintf(pOUT, "%s\x1b[2m%s\x1b[0m%s%s\x1b[0m\n", thead, meta, color, gBuff.data);
					}
					else
					{
						fprintf(pOUT, "%s\x1b[2m%s\x1b[0m %s\n", thead, meta, gBuff.data);
					}
					break;
				case WAIT_ABANDONED:
					break;

			}
#endif
		}

		if(stc_output == (eTERMOUT | eFILEOUT))
		{
			write_file(head, meta, gBuff.data);
		}

	}

	if(level == eFATAL)
	{
    fprintf(stderr, "Force crash\n");
		*(volatile int*)0 = 0;
	}

	return;
}

/*--------------------------------------------------------------------------------*/
static void write_file(const char * head, const char * meta, const char * mess)
{
	if(0 != stc_fileOk)
	{
#ifdef _WIN32
		DWORD dwWaitResult = WaitForSingleObject(stc_mutex_, INFINITE);
		switch(dwWaitResult)
		{
			case WAIT_OBJECT_0:

#else
		pthread_mutex_lock(&stc_mutex_);
#endif
		{
			outf = fopen(stc_path, "at");
			if(NULL != outf)
			{
				fwrite(head, strlen(head), sizeof(char), outf);
				fwrite(meta, strlen(meta), sizeof(char), outf);
				fwrite(mess, strlen(mess), sizeof(char), outf);
				fclose(outf);
			}
		}
#ifdef _WIN32
				break;
			case WAIT_ABANDONED:
				break;
		}
#else
		pthread_mutex_unlock(&stc_mutex_);
#endif
	}

	return;
}


/*--------------------------------------------------------------------------------*/
void bkp_Log_terminate(void)
{
	if(0 == stc_inited)
  {
		return;
  }

  free(gBuff.data);
  gBuff.size = 0;

	stc_inited = 0;
	return;
}

/*--------------------------------------------------------------------------------*/
void gdLogger_setMaxFileSize(uint32_t size)
{
	stc_maxFileSize = size;
	return;
}

/*--------------------------------------------------------------------------------*/
static void getFileTime(char tmp[64])
{
	time_t now = bkp_time_getEpoch();
	struct tm  ts;
	ts = *localtime((const time_t *)&now);
	strftime(tmp, M, "%Y_%m_%d__%H_%M_%S", &ts);

	return;
}

#ifdef __cplusplus
}
#endif
