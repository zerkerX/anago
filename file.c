#include <stdio.h>
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

