/*
famicom ROM cartridge utility - unagi
script syntax data and function

Copyright (C) 2008-2009 鰻開発協同組合

This library is free software; you can redistribute it and/or
modify it under the terms of the GNU Lesser General Public
License as published by the Free Software Foundation; either
version 2.1 of the License any later version.

This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public
License along with this library; if not, write to the Free Software
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "type.h"
#include "textutil.h"
#include "script.h"
#include "script_syntax.h"

static int syntax_check_expression(char **word, int word_num, struct st_expression *e)
{
	if(word_num == 0){
		return NG;
	}
	//left
	if(value_get(word[0], &(e->left.value)) == OK){
		e->left.type = EXPRESSION_TYPE_VALUE;
	}else{
		e->left.type = EXPRESSION_TYPE_VARIABLE;
		e->left.variable = word[0][0];
	}
	word_num--;
	if(word_num == 0){
		e->operator = OPERATOR_NONE;
		return OK;
	}
	//operator
	e->operator = operator_get(word[1]);
	if(e->operator == OPERATOR_ERROR){
		return NG;
	}
	word_num--;
	if(word_num == 0){
		return NG;
	}
	//right
	if(value_get(word[2], &(e->right.value)) == OK){
		e->right.type = EXPRESSION_TYPE_VALUE;
	}else{
		e->right.type = EXPRESSION_TYPE_VARIABLE;
		e->right.variable = word[2][0];
	}
	return OK;
}

static int strto_enum(const char **t, const char *word, int start, long *val)
{
	long i = start;
	while(*t != NULL){
		if(strcmp(*t, word) == 0){
			*val = i;
			return OK;
		}
		i++;
		t++;
	}
	return NG;
}

#include "syntax_data.h"
static const char SYNTAX_ERROR_PREFIX[] = "syntax error:";

/*
return: error count, ここでは 0 or 1
*/
static int syntax_check_phase(char **word, int word_num, struct script *s, const int mode)
{
	int i = sizeof(SCRIPT_SYNTAX) / sizeof(SCRIPT_SYNTAX[0]);
	const struct script_syntax *syntax;
	const char *opword;
	opword = word[0];
	syntax = SCRIPT_SYNTAX;
	while(i != 0){
		if(strcmp(syntax->name, opword) == 0){
			int j;
			
			s->opcode = syntax->script_opcode;
			if((mode & syntax->permittion) == 0){
				printf("%d:%s opcode %s is not allowed on current mode\n", s->line, SYNTAX_ERROR_PREFIX, syntax->name);
				return 1;
			};
			{
				int compare = 0;
				switch(syntax->compare){
				case SYNTAX_COMPARE_EQ:
					compare = (syntax->argc == (word_num - 1));
					break;
				case SYNTAX_COMPARE_GT:
					compare = ((word_num - 1) >= syntax->argc);
					compare &= (word_num <= 5);
					break;
				}
				if(!compare){
					printf("%d:%s parameter number not match %s\n", s->line, SYNTAX_ERROR_PREFIX, opword);
					return 1;
				}
			}
			//opcode pointer をずらして単語の起点を引数だけにあわせる
			word++;
			word_num--;
			for(j = 0; j < word_num; j++){
				switch(syntax->argv_type[j]){
				case SYNTAX_ARGVTYPE_NULL:
					printf("%d:%s ARGV_NULL select\n", s->line, SYNTAX_ERROR_PREFIX);
					return 1;
				case SYNTAX_ARGVTYPE_VALUE:
					if(value_get(word[j], &(s->value[j])) == NG){
						printf("%d:%s value error %s %s\n", s->line, SYNTAX_ERROR_PREFIX, opword, word[j]);
						return 1;
					}
					break;
				case SYNTAX_ARGVTYPE_HV:
					switch(word[j][0]){
					case 'H':
					case 'h':
						s->value[j] = MIRROR_HORIZONAL;
						break;
					case 'V':
					case 'v':
						s->value[j] = MIRROR_VERTICAL;
						break;
					case 'A':
					case 'a':
						s->value[j] = MIRROR_PROGRAMABLE;
						break;
					default:
						printf("%d:%s unknown scroll mirroring type %s\n", s->line, SYNTAX_ERROR_PREFIX, word[j]);
						return 1;
					}
					break;
				case SYNTAX_ARGVTYPE_EXPRESSION:
					s->value[j] = VALUE_EXPRESSION;
					//命令名の単語と単語数を除外して渡す
					if(syntax_check_expression(&word[j], word_num - 1, &s->expression) == NG){
						printf("%d:%s expression error\n", s->line, SYNTAX_ERROR_PREFIX);
						return 1;
					}
					//移行の単語は読まないことにする(まずいかも)
					return 0;
				case SYNTAX_ARGVTYPE_VARIABLE:{
					const char v = word[j][0];
					if(v >= 'a' && v <= 'z'){
						s->value[j] = VALUE_VARIABLE;
						s->variable = v;
					}else{
						printf("%d:%s variable must use [a-z] %s\n", s->line, SYNTAX_ERROR_PREFIX, word[j]);
						return 1;
					}
					}break;
				case SYNTAX_ARGVTYPE_CONSTANT:{
					if(value_get(word[j], &(s->value[j])) == OK){
						break;
					}else if(strto_enum(STR_CONSTANTNAME, word[j], VALUE_CONTANT_CPU_STEP_START, &(s->value[j])) == OK){
						break;
					}else{
						printf("%d:%s constant error %s %s\n", s->line, SYNTAX_ERROR_PREFIX, opword, word[j]);
						return 1;
					}
					}break;
				case SYNTAX_ARGVTYPE_TRANSTYPE:{
					if(strto_enum(STR_TRANSTYPE, word[j], VALUE_TRANSTYPE_EMPTY, &(s->value[j])) == NG){
						printf("%d:%s unknown trans type %s\n", s->line, SYNTAX_ERROR_PREFIX, word[j]);
						return 1;
					}
					}break;
				default:
					s->value[j] = VALUE_UNDEF;
					break;
				}
			}
			//opcode found and 入力文字種正常
			return 0;
		}
		syntax++;
		i--;
	}
	printf("%d:%s unknown opcode %s\n", s->line, SYNTAX_ERROR_PREFIX, opword);
	return 1;
}

/*
return: error count
*/
int syntax_check(char **text, int text_num, struct script *s, int mode)
{
	int error = 0;
	int i;
	mode = 1<< mode; //permittion は bitflag なのでここで変換する
	for(i = 0; i < text_num; i++){
		char *word[TEXT_MAXWORD];
		const int n = word_load(text[i], word);
		s->line = i + 1;
		switch(word[0][0]){
		case '#':
		case '\0':
			s->opcode = SCRIPT_OPCODE_COMMENT;
			break;
		default:
			error += syntax_check_phase(word, n, s, mode);
			break;
		}
		s++;
	}
	return error;
}

