/*
famicom ROM cartridge utility - unagi
パラレルポート共有定義
*/
#ifndef _PARALELL_PORT_INLINE_H_
#define _PARALELL_PORT_INLINE_H_
#include "giveio.h"
#include <windows.h>
#define ASM_ENABLE (0)
enum{
	PORT_DATA = 0x0378,
	PORT_BUSY,
	PORT_CONTROL
};

int _inp(int);
#if ASM_ENABLE==0
void _outp(int, int);
#else
static inline void _outp(int address, int data){
	asm(
		" movl %0,%%edx\n"
		" movl %1,%%eax\n"
		" out %%al,%%dx\n"
		:: "q"(address), "q"(data): "%edx", "%eax"
	);
}
#endif

#endif
