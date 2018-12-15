#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "type.h"
#include "textutil.h"
#ifndef DEBUG
 #define DEBUG 0
#endif

#define PRINT(msg) \
	if(DEBUG == 1){ \
		printf("%s %s\n",  __FUNCTION__, msg);\
	}

int text_load(char *buf, int length, char **text)
{
	int line = 0;
	char pastdata = '#';
	
	text[line] = buf;
	line++;
	while(length != 0){
		//正確な行数把握と手抜きのため CR only の改行コードを未サポートとする
		int current_lf = 0;
		
		switch(*buf){
		case '\n':
			*buf = '\0';
			current_lf = 1;
			break;
		case '\r':
			*buf = '\0';
			break;
		}
		switch(pastdata){
		case '\0':
			if(line >= TEXT_MAXLINE){
				PRINT("line over")
				return 0;
			}
			if(current_lf == 0){
				text[line] = buf;
				line++;
			}
			break;
		}
		pastdata = *buf;
		buf++;
		length--;
	}
	buf--;
	*buf = '\0';
	return line;
}

int word_load(char *buf, char **text)
{
	int word = 0;
	char pastdata = '#';
	
	switch(*buf){
	case '\t':
	case ' ':
		break;
	case '#': //行頭コメント
		text[word] = buf;
		return 1;
	default:
		text[word] = buf;
		word++;
		break;
	}
	
	while(*buf != '\0'){
		int current_spc = 0;
		switch(*buf){
		case '\t':
		case ' ':
			*buf = '\0';
			current_spc = 1;
			break;
		}
		switch(pastdata){
		case '\0':
			if(word >= TEXT_MAXWORD){
				PRINT("word over")
				return 0;
			}
			if(current_spc == 0){
				/*if(*buf == '#'){
					return word;
				}*/
				text[word] = buf;
				word++;
			}
			break;
		}
		pastdata = *buf;
		buf++;
	}
	return word;
}

int value_get(const char *str, long *val)
{
	int base = 10;
	int sign = 1;
	//-がついてるか
	switch(*str){
	case '\0':
		return NG;
	case '-':
		sign = -1;
		str++;
		if(*str == '\0'){
			return NG;
		}
		break;
	}
	//0x, 0b, $, % がついてるか
	switch(*str){
	case '0':
		switch(str[1]){
		case '\0':
			//たんなる 0 なので OK
			break;
		case 'x':
			base = 0x10;
			str += 2;
			break;
		case 'b':
			base = 2;
			str += 2;
			break;
		case '0': case '1': case '2': case '3':
		case '4': case '5': case '6': case '7':
		case '8': case '9':
			//Cなら8進数扱いだけど10進数扱いにする
			break;
		default:
			return NG;
		}
		break;
	case '$':
		base = 0x10;
		str += 1;
		break;
	case '%':
		base = 2;
		str += 1;
		break;
	}
	//この時点での str は 数字の先頭としてた場所
	char *error;
	*val = strtol(str, &error, base);
	if(error[0] != '\0'){
		//x01
		//4M\0
		if((error[1] == '\0') && (base == 10)){
			switch(error[0]){
			case 'K':
				*val *= 0x400;
				break;
			case 'M':
				*val *= 0x20000;
				break;
			default:
				return NG;
			}
		}else{
			return NG;
		}
	}
	
	if(sign == -1){
		*val = -(*val);
	}
	return OK;
}

struct operator_cmp{
	char *str;
	int operator;
};
static const struct operator_cmp CMP[] = {
	{"+", OPERATOR_PLUS},
	{">>", OPERATOR_SHIFT_LEFT},
	{"<<", OPERATOR_SHIFT_RIGHT},
	{"&", OPERATOR_AND},
	{"|", OPERATOR_OR},
	{"^", OPERATOR_XOR}
};

int operator_get(char *str)
{
	const struct operator_cmp *c;
	int i = OPERATOR_ERROR;
	c = CMP;
	while(i != 0){
		if(strcmp(c->str, str) == 0){
			return c->operator;
		}
		c++;
		i--;
	}
	return OPERATOR_ERROR;
}

