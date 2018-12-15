#ifndef _TEXTUTIL_H_
#define _TEXTUTIL_H_
enum{
	TEXT_MAXLINE = 0x100,
	TEXT_MAXWORD = 10
};
int text_load(char *buf, int length, char **text);
int word_load(char *buf, char **text);
int value_get(const char *str, long *val);

enum{
	OPERATOR_PLUS,
	OPERATOR_SHIFT_LEFT,
	OPERATOR_SHIFT_RIGHT,
	OPERATOR_AND,
	OPERATOR_OR,
	OPERATOR_XOR,
	OPERATOR_ERROR,
	OPERATOR_NONE,
};

int operator_get(char *str);
#endif
