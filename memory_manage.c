#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include "memory_manage.h"
enum{
	MANAGE_NUM = 0x100
};

struct manage{
	const char *file, *function;
	int line;
	void *addr;
	int size;
};
static const struct manage EMPTY = {
	.file = NULL, .line = 0, .function = NULL,
	.addr = NULL, .size = 0
};
static struct manage management[MANAGE_NUM];
void mm_init(void)
{
	int i;
	for(i = 0; i < MANAGE_NUM; i++){
		management[i] = EMPTY;
	}
}
void *mm_malloc(const char *file, int line, const char *function, int size)
{
	int i;
	struct manage *t = management;
	for(i = 0; i < MANAGE_NUM; i++){
		if(t->addr == NULL){
			t->addr = malloc(size);
			t->size = size;
			t->file = file;
			t->line = line;
			t->function = function;
			return t->addr;
		}
		t++;
	}
	assert(0);
	return NULL;
}
void mm_free(void *addr)
{
	int i;
	struct manage *t = management;
	for(i = 0; i < MANAGE_NUM; i++){
		if(t->addr == addr){
			free(addr);
			*t = EMPTY;
			return;
		}
		t++;
	}
	assert(0);
}
void mm_end(void)
{
	int i;
	struct manage *t = management;
	for(i = 0; i < MANAGE_NUM; i++){
		if(t->addr != NULL){
			printf("**free forgot** %s:%d %s() size %d\n", t->file, t->line, t->function, t->size);
		}
		t++;
	}
}
