#ifdef __cplusplus
extern "C" {
#endif

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#ifdef _WIN32
#include <windows.h>
#else
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#endif

#include "include/bkp_path.h"

static int createMyDirectory(const char * path);

/*______________________________________________________________________________________________________________*/
void bkpDos2posix(char * path)
{
	for(char * p = path; *p != '\0'; ++p)
	{
		if(*p == '\\')
		{
			*p = '/';
		}
	}
}

/*______________________________________________________________________________________________________________*/
int bkpSplitPath(BkpPath * spath, const char * path)
{
	spath->directory[0] = '\0';
	spath->base_name[0] = '\0';
	spath->file_name[0] = '\0';
	spath->extension[0] = '\0';

	if(path == NULL || path[0] == '\0') return -1;

	char szPath[BKP_MAX_FULL_PATH_SIZE] = {0};

	strcpy(szPath, path);
	bkpDos2posix(szPath);

	if(path[strlen(path) - 1] == '/')
	{
		strcpy(spath->directory, szPath);
		return 0;
	}

	//fprintf(stdout, "%s -> %s\n", path, szPath);

	char * tmp = strrchr(szPath, '/');
	char * p = NULL;

	if(tmp == NULL)
	{
		spath->directory[0] = '.';
		spath->directory[1] = '/';
		spath->directory[2] = '\0';
		strcpy(spath->base_name, szPath);
		strcpy(spath->file_name, szPath);
	}
	else
	{
		p = szPath;
		char * q = spath->directory;
		for(; p <= tmp; ++p, ++q)
		{
			*q = *p;
		}

		*q = '\0';

		p = tmp + 1;
		strcpy(spath->base_name, p);
		strcpy(spath->file_name, p);
	}


	if(spath->base_name[0] != '\0')
	{
		tmp = strrchr(spath->base_name, '.');
		if(tmp != NULL)
		{
			if(strlen(tmp) < BKP_MAX_EXTENSION_SIZE)
			{
				p = tmp;
				*p = '\0';
				++tmp;
				strcpy(spath->extension, tmp);
			}
		}
	}
	/*
	fprintf(stdout, "Director : %s\nFileName : %s\nBaseName : %s\nExtension : %s\n------------------------------------------\n",
			spath->directory,
			spath->file_name,
			spath->base_name,
			spath->extension);
	*/

	return 0;
}

/*______________________________________________________________________________________________________________*/
int bkpPathExists(const char * path)
{
#ifdef _WIN32
	DWORD dwAttrib = GetFileAttributes(path);
	return (dwAttrib != INVALID_FILE_ATTRIBUTES);
#else
	struct stat info;

	return (stat(path, &info) == 0) ? 1 : 0;
#endif
}

/*______________________________________________________________________________________________________________*/
static int createMyDirectory(const char * path)
{
	int ret;
#ifdef _WIN32
	 ret = CreateDirectory(path, NULL);
	return ret == 0 ? -1 : 0;
#else
	ret = mkdir(path, S_IRWXU);
	return ret == 0 ? 0 : -1;
#endif
}

/*______________________________________________________________________________________________________________*/
int bkpCreateDirectory(const char * path)
{
	if(path == NULL || path[0] == '\0') return 0;

	int total = 0;

	if(bkpPathExists(path))
	{
		return 0;
	}
	else
	{
		char szPath[BKP_MAX_FULL_PATH_SIZE] = {0};

		strcpy(szPath, path);
		bkpDos2posix(szPath);

		char * p = szPath;
		char * q = p;

		if(p[0] == '/')
		{
			q =  strchr(q, '/') + 1;
		}

		do
		{
			q =  strchr(q, '/');

			if(q == NULL)
			{
				break;
			}

			*q = '\0';
			if(bkpPathExists(p) == 0)
			{
				int ret = createMyDirectory(p);
				if(ret != 0)
				{
					fprintf(stderr, "Cannot Create : '%s'\n", p);
					return -1;
				}
				++total;
			}
			*q = '/';
			++q;
		}
		while(1);

		if(bkpPathExists(p) == 0)
		{
			int ret = createMyDirectory(p);
			if(ret != 0)
			{
				fprintf(stderr, "Cannot Create : '%s'\n", p);
				return -1;
			}
			++total;
		}
	}

	return total;
}

/*______________________________________________________________________________________________________________*/
static char s_exe_path[BKP_MAX_FULL_PATH_SIZE];

const char * bkpExePath(const char * relative)
{
#ifdef _WIN32
    char buf[MAX_PATH];
    GetModuleFileNameA(NULL, buf, MAX_PATH);
    char * sep = strrchr(buf, '\\');
    if(sep) *sep = '\0';
    snprintf(s_exe_path, sizeof(s_exe_path), "%s\\%s", buf, relative);
#else
    char buf[BKP_MAX_PATH_SIZE];
    ssize_t len = readlink("/proc/self/exe", buf, sizeof(buf) - 1);
    if(len <= 0) { return relative; }
    buf[len] = '\0';
    char * sep = strrchr(buf, '/');
    if(sep) *sep = '\0';
    snprintf(s_exe_path, sizeof(s_exe_path), "%s/%s", buf, relative);
#endif
    return s_exe_path;
}

/*______________________________________________________________________________________________________________*/
#ifdef __cplusplus
}
#endif
