#include <assert.h>
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <curses.h>
#include <term.h>
#include <unistd.h>
#include "progress.h"

static bool initialized = false;

void progress_init(void)
{
	if (!initialized)
	{
		if (setupterm(NULL, STDOUT_FILENO, NULL) == OK)
		{
			initialized = true;
		}
	}
	printf("\n\n");
}

static void draw(const char *name, long offset, long count)
{
	if(count == 0){
		printf("%s skip\n", name);
		return;
	}
	const int barnum = 100 / 5;
	int persent = (offset * 100) / count;
	char bar[barnum + 3 + 1];
	char *t = bar;
	int i;
	assert(persent <= 100);
	printf("%s 0x%06x/0x%06x ", name, (int)offset, (int)count);
	*t++ = '|';
	for(i = 0; i < persent / 5; i++){
		if(i == barnum / 2){
			*t++ = '|';
		}
		*t++ = '#';
	}
	for(; i < barnum; i++){
		if(i == barnum / 2){
			*t++ = '|';
		}
		*t++ = ' ';
	}
	*t++ = '|';
	*t = '\0';
	puts(bar);
}

void progress_draw(long program_offset, long program_count, long charcter_offset, long charcter_count)
{
	if (initialized)
	{
		putp(tparm(cursor_up));
		putp(tparm(cursor_up));
		putp(tparm(column_address, 0));
	}
	
	draw("program memory  ", program_offset, program_count);
	draw("character memory", charcter_offset, charcter_count);
	fflush(stdout);
}
