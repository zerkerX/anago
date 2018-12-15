#ifndef _MEMORY_MANAGE_H_
#define _MEMORY_MANAGE_H_
void *mm_malloc(const char *file, int line, const char *function, int size);
void mm_free(void *addr);
#if DEBUG == 0
  #define mm_init(a)
  #define mm_end(a)
#include <stdlib.h>
  #define Malloc(size) malloc(size)
  #define Free(addr) free(addr)
#else
  void mm_init(void);
  void mm_end(void);
  #define Malloc(size) mm_malloc(__FILE__, __LINE__, __FUNCTION__, size)
  #define Free(addr) mm_free(addr)
#endif
#endif
