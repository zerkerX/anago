#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include "memory_manage.h"
#include "file.h"

int buf_load(u8 *buf, const char *file, int size)
{
	FILE *fp;

	fp = fopen(file, "rb");
	if(fp == NULL){
		return NG;
	}
	fseek(fp, 0, SEEK_SET);
	fread(buf, sizeof(u8), size, fp);
	fclose(fp);
	return OK;
}

void* buf_load_full(const char *file, int *size)
{
	FILE *fp;
	u8 *buf;

	*size = 0;
	fp = fopen(file, "rb");
	if(fp == NULL){
		return NULL;
	}
	fseek(fp, 0, SEEK_END);
	*size = ftell(fp);
	if(*size == 0){
		fclose(fp);
		return NULL;
	}
	fseek(fp, 0, SEEK_SET);
	buf = Malloc(*size);
	fread(buf, sizeof(u8), *size, fp);
	fclose(fp);
	return buf;
}

void buf_save(const void *buf, const char *file, int size)
{
	FILE *fp;

	fp = fopen(file, "wb");
	fseek(fp, 0, SEEK_SET);
	fwrite(buf, sizeof(u8), size, fp);
	fclose(fp);
}

static char script_path[PATH_MAX];

/* TODO: Use config generator! */
#define INSTALL_PREFIX "/usr/local/"
#define SCRIPT_DIR INSTALL_PREFIX "share/anago/"

/* TODO: Find via some better means. Also maybe don't hardcode .config after */
#define HOME_PATH "/home/zerker/"
#define CONFIG_DIR HOME_PATH ".config/anago/"

/* Tests if the specified name is in the path. Returns NULL if not found
 * or not suitable, otherwise returns the combined path. */
static const char * test_path(const char * path, const char * name)
{
	const char * result = NULL;
	
	if (strlen(path) + strlen(name) < PATH_MAX)
	{
		FILE * fp;
		
		/* Work in our static string storage, because that's what
		 * we will be returning eventually */
		strcpy(script_path, path);
		strcat(script_path, name);
		
		fp = fopen(script_path, 'r');
		
		if (fp != NULL)
		{
			fclose(fp);
			result = script_path;
		}
	}
}

const char * find_script(const char * name)
{
	const char * result = NULL;
	
	/* Search in user's config folder first */
	result = test_path(CONFIG_DIR, name);
	if (result == NULL)
	{
		/*Not found. Check share next */
		result = test_path(SCRIPT_DIR, name);
		
		if (result == NULL)
		{
			/* Still not found. Just try opening what 
			 * the user provided. */
			result = name;
		}
	}
	
	return result;
}
