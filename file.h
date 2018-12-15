#ifndef _FILE_H_
#define _FILE_H_
#include "type.h"
int buf_load(u8 *buf, const char *file, int size);
void* buf_load_full(const char *file, int *size);
void buf_save(const void *buf, const char *file, int size);
#endif
