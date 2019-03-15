#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include "memory_manage.h"
#include "build_config.h"
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

/* Apparently PATH_MAX needs to be in a function to be used, 
 * but we need to pick something... */
#define MAX_ALLOWED_PATH 4096
static char script_path[MAX_ALLOWED_PATH];

#define SHARE_DIR "/share/anago/"
#define CONFIG_DIR "/.config/anago/"
/* TODO: Potentially do a more comprehensive honoring of the XDG spec */

/* Tests if the specified name is in the path. Returns NULL if not found
 * or not suitable, otherwise returns the combined path. */
static const char * test_path(const char * path, const char * name)
{
	const char * result = NULL;
	
	if (strlen(path) + strlen(name) < MAX_ALLOWED_PATH)
	{
		FILE * fp;
		
		/* Work in our static string storage, because that's what
		 * we will be returning eventually */
		strcpy(script_path, path);
		strcat(script_path, name);
		
		fp = fopen(script_path, "r");
		
		if (fp != NULL)
		{
			fclose(fp);
			result = script_path;
		}
	}
	
	return result;
}

const char * find_script(const char * name)
{
	const char * result = NULL;
	char temppath[MAX_ALLOWED_PATH];
	
	/* Obtain path to $HOME/$CONFIG_DIR. If $HOME not defined, this
	 * should safely fall through to just a weird path that probably
	 * won't exist, and we'll try the next case instead */
	strcpy(temppath, getenv("HOME"));
	strcat(temppath, CONFIG_DIR);
	
	/* Search in user's config folder first */
	result = test_path(temppath, name);
	if (result == NULL)
	{
		/*Not found. Check share next */
		strcpy(temppath, CMAKE_INSTALL_PREFIX);
		strcat(temppath, SHARE_DIR);
		
		result = test_path(temppath, name);
		if (result == NULL)
		{
			/* Still not found. Just try opening what 
			 * the user provided. */
			result = name;
		}
	}
	
	return result;
}
