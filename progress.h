#ifndef _PROGRESSS_H_
#define _PROGRESSS_H_
void progress_init(void);
void progress_draw(long program_offset, long program_count, long charcter_offset, long charcter_count);
void progress_term(void);
#endif
