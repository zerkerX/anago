#include <assert.h>
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <curses.h>
#include "progress.h"

void progress_init(void)
{
	printf("\n\n");
	initscr();
}

void progress_term(void)
{
	endwin();
}

static void draw(const char *name, long offset, long count)
{
	char status_str[256];
	if(count == 0){
		sprintf(status_str, "%s skip\n", name);
		addstr(status_str);
		return;
	}
	const int barnum = 100 / 5;
	int percent = (offset * 100) / count;
	char bar[barnum + 3 + 1];
	char *t = bar;
	int i;
	assert(percent <= 100);

	// Basic status with name and values
	sprintf(status_str, "%s 0x%06x/0x%06x ", 
		name, (int)offset, (int)count);
	
	// Construct the progress bar string next
	*t++ = '|';
	for(i = 0; i < percent / 5; i++){
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
	
	// Draw both to the display
	addstr(status_str);
	addstr(bar);
}

void progress_draw(long program_offset, long program_count, long charcter_offset, long charcter_count)
{
	move(0,0);
	draw("program memory  ", program_offset, program_count);
	draw("character memory", charcter_offset, charcter_count);
	refresh();
}
