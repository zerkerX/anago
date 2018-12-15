/*
famicom ROM cartridge utility - unagi
script engine

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

todo: 
* 変数管理のグローバル値を、logical_test(), excute() ローカルにしたい
*/
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "memory_manage.h"
#include "type.h"
#include "file.h"
#include "reader_master.h"
#include "textutil.h"
#include "config.h"
#include "header.h"
#include "script_syntax.h"
#include "script.h"

/*
MAPPER num
MIRROR [HV]
CPU_ROMSIZE num
CPU_RAMSIZE num
PPU_ROMSIZE num
DUMP_START
CPU_READ address size
CPU_WRITE address data -> 変数展開+演算子使用可能
PPU_READ address size
STEP_START variable start end step -> for(i=start;i<end;i+=step)
STEP_END
DUMP_END
*/
//#include "syntax.h"

//変数管理
//step の変数は小文字の a-z ではじまること。2文字目以降は無視する
struct variable_manage{
	char name;
	long start,end,step;
	long val;
	const struct script *Continue;
};

enum{
	STEP_MAX = 3,
	VARIABLE_MAX = STEP_MAX
};

static const struct variable_manage VARIABLE_INIT = {
	.name = '\0', 
	.start = 0, .end = 0, .step = 0,
	.val = 0,
	.Continue = NULL
};
static struct variable_manage variable_bank[VARIABLE_MAX];
static int variable_num = 0;

static void variable_init_single(int num)
{
	memcpy(&variable_bank[num], &VARIABLE_INIT, sizeof(struct variable_manage));
}

static void variable_init_all(void)
{
	int i;
	variable_num = 0;
	for(i = 0; i < VARIABLE_MAX; i++){
		variable_init_single(i);
	}
}

static int variable_get(char name, long *val)
{
	int i;
	struct variable_manage *v;
	v = variable_bank;
	for(i = 0; i < variable_num; i++){
		if(v->name == name){
			*val = v->val;
			return OK;
		}
		v++;
	}
	return NG;
}

static int expression_value_get(const struct st_variable *v, long *data)
{
	if(v->type == EXPRESSION_TYPE_VARIABLE){
		if(variable_get(v->variable, data) == NG){
			return NG;
		}
	}else{
		*data = v->value;
	}
	return OK;
}

//変数展開
static int expression_calc(const struct st_expression *e, long *val)
{
	long left, right;
	if(expression_value_get(&e->left, &left) == NG){
		return NG;
	}
	if(e->operator == OPERATOR_NONE){
		*val = left;
		return OK;
	}
	if(expression_value_get(&e->right, &right) == NG){
		return NG;
	}
	switch(e->operator){
	case OPERATOR_PLUS:
		*val = left + right;
		break;
	case OPERATOR_SHIFT_LEFT:
		*val = left >> right;
		//*val &= 1;
		break;
	case OPERATOR_SHIFT_RIGHT:
		*val = left << right;
		break;
	case OPERATOR_AND:
		*val = left & right;
		break;
	case OPERATOR_OR:
		*val = left | right;
		break;
	case OPERATOR_XOR:
		*val = left ^ right;
		break;
	}
	
	return OK;
}

static int step_new(char name, long start, long end, long step, const struct script *Continue)
{
	if(variable_num >= VARIABLE_MAX){
		return NG; //変数定義が多すぎ
	}
	struct variable_manage *v;
	int i;
	v = variable_bank;
	for(i = 0; i < variable_num; i++){
		if(v->name == name){
			return NG; //変数名重複
		}
		v++;
	}
	v = variable_bank;
	v += variable_num;
	v->name = name;
	v->start = start;
	v->end = end;
	v->step = step;
	v->val = start;
	v->Continue = Continue;
	variable_num++;
	return OK;
}

static const struct script *step_end(const struct script *Break)
{
	//現在のループの対象変数を得る
	struct variable_manage *v;
	v = variable_bank;
	v += (variable_num - 1);
	//変数更新
	v->val += v->step;
	if(v->val < v->end){
		return v->Continue;
	}
	//ループが終わったのでその変数を破棄する
	variable_init_single(variable_num - 1);
	variable_num--;
	return Break;
}

//int syntax_check(char **text, int text_num, struct script *s, int mode);
/*
logical_check() 用サブ関数とデータ
*/
static const char LOGICAL_ERROR_PREFIX[] = "logical error:";
enum{
	SCRIPT_PPUSIZE_0 = 0,
	SCRIPT_MEMORYSIZE = 10,
};
struct logical_romsize{
	const struct script *data[SCRIPT_MEMORYSIZE], *constant;
	int count;
};
static int logical_flashsize_set(int region, struct logical_romsize *l, const struct script *s)
{
	//PPU 用データの 0 は size 0, 定数 0 として予約されており、0の場合はここを上書きする
	if((region == MEMORY_AREA_PPU) && (s->value[0] == 0)){
		l->data[SCRIPT_PPUSIZE_0] = s;
		return OK;
	}
	if(l->count >= SCRIPT_MEMORYSIZE){
		const char *opstr;
		opstr = OPSTR_CPU_ROMSIZE; //warning 対策
		switch(region){
		case MEMORY_AREA_CPU_ROM:
			opstr = OPSTR_CPU_ROMSIZE;
			break;
		case MEMORY_AREA_PPU:
			opstr = OPSTR_PPU_ROMSIZE;
			break;
		default:
			assert(0);
		}
		printf("%d:%s %s override count over\n", s->line, LOGICAL_ERROR_PREFIX, opstr);
		return NG;
	}
	l->data[l->count] = s;
	l->count += 1;
	return OK;
}

static int logical_flashsize_get(long transtype, long size, struct logical_romsize *l)
{
	int i;
	for(i = 0; i < l->count; i++){
		const struct script *s;
		s = l->data[i];
		if((s->value[0] == size) && (
			(s->value[1] == transtype) || 
			(s->value[1] == VALUE_TRANSTYPE_FULL)
		)){
			l->constant = s;
			return OK;
		}else if((s->value[0] == size) && (s->value[1] == VALUE_TRANSTYPE_EMPTY)){
			l->constant = s;
			return OK;
		}
	}
	printf("%s flashsize not found\n", LOGICAL_ERROR_PREFIX);
	return NG;
}

static long constant_get(long val, const struct script *cpu, const struct script *ppu)
{
	switch(val){
	case VALUE_CONTANT_CPU_STEP_START:
		return cpu->value[2];
	case VALUE_CONTANT_CPU_STEP_END:
		return cpu->value[3];
	case VALUE_CONTANT_PPU_STEP_START:
		return ppu->value[2];
	case VALUE_CONTANT_PPU_STEP_END:
		return ppu->value[3];
	}
	assert(0);
	return -1;
}

static void logical_print_capacityerror(int line, const char *area)
{
	printf("%d:%s %s area flash memory capacity error\n", line, LOGICAL_ERROR_PREFIX, area);
}

static inline void logical_print_illgalarea(int line, const char *area, long address)
{
	printf("%d:%s illgal %s area $%06x\n", line, LOGICAL_ERROR_PREFIX, area, (int) address);
}

static inline void logical_print_illgallength(int line, const char *area, long length)
{
	printf("%d:%s illgal %s length $%04x\n", line, LOGICAL_ERROR_PREFIX, area, (int) length);
}

static inline void logical_print_overdump(int line, const char *area, long start, long end)
{
	printf("%d:%s %s area over dump $%06x-$%06x\n", line, LOGICAL_ERROR_PREFIX, area, (int)start ,(int)end);
}

static inline void logical_print_access(int line, const char *area, const char *rw, long addr, long len)
{
	printf("%d:%s %s $%04x $%02x\n", line, area, rw, (int) addr, (int) len);
}

static inline void logical_print_byteerror(int line, const char *area, long data)
{
	printf("%d:%s write data byte range over, %s $%x\n", line, LOGICAL_ERROR_PREFIX, area, (int) data);
}

static int dump_length_conform(const char *name, long logicallength, long configlength)
{
	if(configlength != logicallength){
		printf("%s %s dump length error\n", LOGICAL_ERROR_PREFIX, name);
		printf("%s: 0x%06x, dump length: 0x%06x\n", name, (int) configlength, (int) logicallength);
		return 1;
	}
	return 0;
}
static inline int is_region_cpurom(long address)
{
	return (address >= 0x8000) && (address < 0x10000);
}

static inline int is_region_cpuram(long address)
{
	return (address >= 0x6000) && (address < 0x8000);
}

static inline int is_region_ppurom(long address)
{
	return (address >= 0) && (address < 0x2000);
}

static inline int is_data_byte(long data)
{
	return (data >= 0) && (data < 0x100);
}

//これだけ is 系で <= 演算子を使用しているので注意
static inline int is_range(long data, long start, long end)
{
	return (data >= start) && (data <= end);
}
static const char STR_REGION_CPU[] = "cpu";
static const char STR_REGION_PPU[] = "ppu";
static const char STR_ACCESS_READ[] = "read";
static const char STR_ACCESS_WRITE[] = "write";

enum{
	SETTING, DUMP, END, STEP_THOUGH
};
static int command_mask(const int region, const long address, const long offset, long size, struct flash_order *f)
{
	const char *str_region = STR_REGION_CPU;
	if(region == MEMORY_AREA_PPU){
		str_region = STR_REGION_PPU;
	}
	switch(region){
	case MEMORY_AREA_CPU_ROM:
		switch(offset){
		case 0x8000: case 0xa000: case 0xc000:
			break;
		default:
			printf("%s %s_COMMAND area offset error\n", LOGICAL_ERROR_PREFIX, str_region);
			return NG;
		}
		switch(size){
		case 0x2000: case 0x4000: case 0x8000:
			break;
		default:
			printf("%s %s_COMMAND area mask error\n", LOGICAL_ERROR_PREFIX, str_region);
			return NG;
		}
		break;
	case MEMORY_AREA_PPU:
		switch(offset){
		case 0x0000: case 0x0400: case 0x0800: case 0x0c00:
		case 0x1000: case 0x1400: case 0x1800: case 0x1c00:
			break;
		default:
			printf("%s %s_COMMAND area offset error\n", LOGICAL_ERROR_PREFIX, str_region);
			return NG;
		}
		switch(size){
		case 0x0400: case 0x0800: case 0x1000: case 0x2000: 
			break;
		default:
			printf("%s %s_COMMAND area mask error\n", LOGICAL_ERROR_PREFIX, str_region);
			return NG;
		}
		break;
	default:
		assert(0); //unknown memory area
	}

	const long mask = size - 1;
	const long data = (address & mask) | offset;
	switch(address){
	case 0:
		f->command_0000 = data;
		break;
	case 0x2aaa: case 0x02aa: 
		f->command_2aaa = data;
		break;
	case 0x5555: case 0x0555:
		f->command_5555 = data;
		break;
	default:
		printf("%s %s_COMMAND unknown commnand address\n", LOGICAL_ERROR_PREFIX, str_region);
		return NG;
	}
	return OK;
}

enum{
	STEP_VALIABLE = 0, STEP_START, STEP_END, STEP_NEXT,
	STEP_NUM
};
static const struct script SCRIPT_PPU_ROMSIZE_0 = {
	.opcode = SCRIPT_OPCODE_PPU_ROMSIZE,
	.line = -1,
	.value = {0, VALUE_TRANSTYPE_EMPTY, 0, 0},
	//.expression, .variable 未定義
};

static int logical_check(const struct script *s, const struct st_config *c, struct romimage *r)
{
	//(CPU|PPU)_(READ|PROGRAM|RAMRW) の length 加算値
	long cpu_romsize = 0, cpu_ramsize = 0, ppu_romsize = 0;
	//DUMP_START 直後に ROM or RAM image を開くフラグ
	//use program mode or ram write mode
	int imagesize = 0; //for write or program mode
	int status = SETTING;
	//override (CPU|PPU)_ROMSIZE pointer. Program mode only
	struct logical_romsize script_cpu_flashsize = {
		.constant = NULL,
		.count = 0
	};
	struct logical_romsize script_ppu_flashsize = {
		.constant = NULL,
		.count = 1
	};
	script_ppu_flashsize.data[0] = &SCRIPT_PPU_ROMSIZE_0;
	//logical error count. 戻り値
	int error = 0;
	
	variable_init_all();
	while(s->opcode != SCRIPT_OPCODE_DUMP_END){
		if((status == DUMP) && (s->opcode < SCRIPT_OPCODE_DUMP_START)){
			printf("%d:%s config script include DUMP_START area\n", s->line, LOGICAL_ERROR_PREFIX);
			error += 1;
		}

		//romimage open for write or program mode
		if((imagesize == 0) && (status == DUMP)){
			switch(c->mode){
			case MODE_RAM_WRITE: //CPU_RAMSIZE check
				assert(r->cpu_ram.attribute == MEMORY_ATTR_READ);
				r->cpu_ram.data = buf_load_full(c->ramimage, &imagesize);
				if(r->cpu_ram.data == NULL){
					printf("%s RAM image open error\n", LOGICAL_ERROR_PREFIX);
					imagesize = -1;
					error += 1;
				}else if(r->cpu_ram.size != imagesize){
					printf("%s RAM image size is not same\n", LOGICAL_ERROR_PREFIX);
					Free(r->cpu_ram.data);
					r->cpu_ram.data = NULL;
					imagesize = -1;
					error += 1;
				}
				break;
			case MODE_ROM_PROGRAM: //MAPPER check
				assert(c->cpu_flash_driver->program != NULL);
				assert(r->cpu_rom.attribute == MEMORY_ATTR_READ);
				assert(r->ppu_rom.attribute == MEMORY_ATTR_READ);
				if(nesfile_load(LOGICAL_ERROR_PREFIX, c->romimage, r)== false){
					error += 1;
				}
				//定数宣言エラーは無限ループの可能性があるのでスクリプト内部チェックをせずに止める
				if(logical_flashsize_get(c->transtype_cpu, r->cpu_rom.size, &script_cpu_flashsize) == NG){
					return error + 1;
				}
				if(logical_flashsize_get(c->transtype_ppu, r->ppu_rom.size, &script_ppu_flashsize) == NG){
					return error + 1;
				}
				//flash memory capacity check
				if(r->cpu_rom.size > c->cpu_flash_driver->capacity){
					logical_print_capacityerror(s->line, r->cpu_rom.name);
					error += 1;
				}
				if((r->ppu_rom.size != 0) && (r->ppu_rom.size > c->ppu_flash_driver->capacity)){
					logical_print_capacityerror(s->line, r->ppu_rom.name);
					error += 1;
				}
				imagesize = -1;
				break;
			default: 
				imagesize = -1;
				break;
			}
		}
	
		switch(s->opcode){
		case SCRIPT_OPCODE_COMMENT:
			break;
		case SCRIPT_OPCODE_MAPPER:
			r->mappernum = s->value[0];
			break;
		case SCRIPT_OPCODE_MIRROR:
			r->mirror = s->value[0];
			break;
		case SCRIPT_OPCODE_CPU_ROMSIZE:{
			const long size = s->value[0];
			r->cpu_rom.size = size;
			if(memorysize_check(size, MEMORY_AREA_CPU_ROM)){
				printf("%s %s length error\n", LOGICAL_ERROR_PREFIX, OPSTR_CPU_ROMSIZE);
				error += 1;
			}
			}break;
		case SCRIPT_OPCODE_CPU_FLASHSIZE:
			if(logical_flashsize_set(MEMORY_AREA_CPU_ROM, &script_cpu_flashsize, s) == NG){
				error += 1;
			}break;
		case SCRIPT_OPCODE_CPU_RAMSIZE:
			//memory size は未確定要素が多いので check を抜く
			r->cpu_ram.size = s->value[0];
			break;
		case SCRIPT_OPCODE_CPU_COMMAND:
			if(command_mask(MEMORY_AREA_CPU_ROM, s->value[0], s->value[1], s->value[2], &(r->cpu_flash)) == NG){
				error += 1;
			}
			break;
		case SCRIPT_OPCODE_PPU_ROMSIZE:{
			const long size = s->value[0];
			r->ppu_rom.size = size;
			if(memorysize_check(size, MEMORY_AREA_PPU)){
				printf("%s %s length error\n", LOGICAL_ERROR_PREFIX, OPSTR_PPU_ROMSIZE);
				error += 1;
			}
			}break;
		case SCRIPT_OPCODE_PPU_FLASHSIZE:
			if(logical_flashsize_set(MEMORY_AREA_PPU, &script_ppu_flashsize, s) == NG){
				error += 1;
			}
			break;
		case SCRIPT_OPCODE_PPU_COMMAND:
			if(command_mask(MEMORY_AREA_PPU, s->value[0], s->value[1], s->value[2], &(r->ppu_flash)) == NG){
				error += 1;
			}
			break;
		case SCRIPT_OPCODE_DUMP_START:
			status = DUMP;
			break;
		case SCRIPT_OPCODE_CPU_READ:{
			const long address = s->value[0];
			const long length = s->value[1];
			const long end = address + length - 1;
			
			assert(r->cpu_rom.attribute == MEMORY_ATTR_WRITE);
			//length filter. 0 はだめ
			if(!is_range(length, 1, 0x4000)){
				logical_print_illgallength(s->line, STR_REGION_CPU, length);
				error += 1;
			}
			//address filter
			else if(!is_region_cpurom(address)){
				logical_print_illgalarea(s->line, STR_REGION_CPU, address);
				error += 1;
			}else if(end >= 0x10000){
				logical_print_overdump(s->line, STR_REGION_CPU, address, end);
				error += 1;
			}
			cpu_romsize += length;
			status = DUMP;
			}
			break;
		case SCRIPT_OPCODE_CPU_WRITE:{
			const long address = s->value[0];
			long data;
			if(expression_calc(&s->expression, &data) == NG){
				printf("%d:%s expression calc error\n", s->line, LOGICAL_ERROR_PREFIX);
				error += 1;
			}
			if(address < 0x5000 || address >= 0x10000){
				logical_print_illgalarea(s->line, STR_REGION_CPU, address);
				error += 1;
			}else if(!is_data_byte(data)){
				logical_print_byteerror(s->line, STR_REGION_CPU, data);
				error += 1;
			}
			status = DUMP;
			}
			break;
		case SCRIPT_OPCODE_CPU_RAMRW:{
			const long address = s->value[0];
			const long length = s->value[1];
			const long end = address + length - 1;
			switch(c->mode){
			case MODE_RAM_READ:
				assert(r->cpu_ram.attribute == MEMORY_ATTR_WRITE);
				break;
			case MODE_RAM_WRITE:
				assert(r->cpu_ram.attribute == MEMORY_ATTR_READ);
				break;
			}
			//length filter. 0 はだめ
			if(!is_range(length, 1, 0x2000)){
				logical_print_illgallength(s->line, STR_REGION_CPU, length);
				error += 1;
			}
			//address filter
			else if(address < 0x5c00 || address >= 0xc000){
				logical_print_illgalarea(s->line, STR_REGION_CPU, address);
				error += 1;
			}else if(0 && end >= 0x8000){
				logical_print_overdump(s->line, STR_REGION_CPU, address, end);
				error += 1;
			}
			cpu_ramsize += length;
			status = DUMP;
			}
			break;
		case SCRIPT_OPCODE_CPU_PROGRAM:{
			const long address = s->value[0];
			const long length = s->value[1];
			const long end = address + length - 1;
			
			assert(r->cpu_rom.attribute == MEMORY_ATTR_READ);
			assert(r->ppu_rom.attribute == MEMORY_ATTR_READ);
			//length filter.
			if(!is_range(length, 0x80, 0x4000)){
				logical_print_illgallength(s->line, STR_REGION_CPU, length);
				error += 1;
			}
			//address filter
			else if(!is_region_cpurom(address)){
				logical_print_illgalarea(s->line, STR_REGION_CPU, address);
				error += 1;
			}else if(end >= 0x10000){
				logical_print_overdump(s->line, STR_REGION_CPU, address, end);
				error += 1;
			}
			cpu_romsize += length;
			status = DUMP;
			}
			break;
		case SCRIPT_OPCODE_PPU_RAMFIND:
			//ループ内部に入ってたらエラー
			if(variable_num != 0){
				printf("%d:%s PPU_RAMTEST must use outside loop\n", s->line, LOGICAL_ERROR_PREFIX);
				error += 1;
			}
			break;
		case SCRIPT_OPCODE_PPU_SRAMTEST:
		case SCRIPT_OPCODE_PPU_READ:{
			const long address = s->value[0];
			const long length = s->value[1];
			const long end = address + length - 1;
			assert(r->ppu_rom.attribute == MEMORY_ATTR_WRITE);
			//length filter. 0 を容認する
			long min = 0;
			if(s->opcode == SCRIPT_OPCODE_PPU_SRAMTEST){
				min = 1;
			}
			if(!is_range(length, min, 0x2000)){
				logical_print_illgallength(s->line, STR_REGION_PPU, length);
				error += 1;
			}
			//address filter
			else if(!is_region_ppurom(address)){
				logical_print_illgalarea(s->line, STR_REGION_PPU, address);
				error += 1;
			}else if (end >= 0x2000){
				logical_print_overdump(s->line, STR_REGION_PPU, address, end);
				error += 1;
			}
			//dump length update
			if((s->opcode == SCRIPT_OPCODE_PPU_READ) && is_region_ppurom(address)){
				ppu_romsize += length;
			}
			status = DUMP;
			}
			break;
		case SCRIPT_OPCODE_PPU_WRITE:{
			if(DEBUG==0){
				break;
			}
			const long address = s->value[0];
			long data;
			if(expression_calc(&s->expression, &data) == NG){
				printf("%d:%s expression calc error\n", s->line, LOGICAL_ERROR_PREFIX);
				error += 1;
			}
			status = DUMP;
			if(!is_region_ppurom(address)){
				logical_print_illgalarea(s->line, STR_REGION_PPU, address);
				error += 1;
			}else if(!is_data_byte(data)){
				logical_print_byteerror(s->line, STR_REGION_PPU, data);
				error += 1;
			}
			status = DUMP;
			}
			break;
		case SCRIPT_OPCODE_PPU_PROGRAM:{
			const long address = s->value[0];
			const long length = s->value[1];
			const long end = address + length - 1;
			
			assert(r->ppu_rom.attribute == MEMORY_ATTR_READ);
			assert(r->ppu_rom.size != 0);

			//length filter.
			if(!is_range(length, 0x80, 0x1000)){
				logical_print_illgallength(s->line, STR_REGION_PPU, length);
				error += 1;
			}
			//address filter
			else if(!is_region_ppurom(address)){
				logical_print_illgalarea(s->line, STR_REGION_PPU, address);
				error += 1;
			}else if(end >= 0x2000){
				logical_print_overdump(s->line, STR_REGION_PPU, address, end);
				error += 1;
			}
			ppu_romsize += length;
			status = DUMP;
			}
			break;
		case SCRIPT_OPCODE_STEP_START:{
			const struct step_syntax{
				long start, end;
				int constant;
				const char *name;
			} RANGE[STEP_NUM] = {
				{.start = 0, .end = 0, .constant = NG, .name = "variable"},
				{.start = 0, .end = 0xff, .constant = OK, .name = "start"},
				{.start = 0, .end = 0x100, .constant = OK, .name = "end"},
				{.start = 1, .end = 0x100, .constant = NG, .name = "next"}
			};
			int i;
			for(i = STEP_START; i < STEP_NUM; i++){
				const struct step_syntax *ss;
				ss = &RANGE[i];
				//定数object を実数に変換するため、const を外して書き換える
				long *v;
				v = (long *) &s->value[i];
				if((ss->constant == OK) && (is_range(*v, VALUE_CONTANT_CPU_STEP_START, VALUE_CONTANT_PPU_STEP_END))){
					//VALUE_CONSTANT_xxx を実際に使われる定数にして、スクリプトデータを張り替える
					*v = constant_get(*v, script_cpu_flashsize.constant, script_ppu_flashsize.constant);
				}
				if(!is_range(*v, ss->start, ss->end)){
					printf("%d:%s step %s must %d-0x%x 0x%x\n", s->line, ss->name, LOGICAL_ERROR_PREFIX, (int) ss->start, (int) ss->end, (int) *v);
					error += 1;
				}
			}
			//charcter RAM の様に step 内部を行わない場合のscript状態の変更
			if(s->value[STEP_START] >= s->value[STEP_END]){
				status = STEP_THOUGH;
			}
			//ループの戻り先はこの命令の次なので s[1]
			else if(step_new(s->variable, s->value[STEP_START], s->value[STEP_END], s->value[STEP_NEXT], &s[1]) == NG){
				printf("%d:%s step loop too much\n", s->line, LOGICAL_ERROR_PREFIX);
				error += 1;
				return error;
			}else{
				status = DUMP;
			}
			}break;
		case SCRIPT_OPCODE_DUMP_END:
			status = END;
			break;
		}
		
		//status 別に script の制御. while を抜けるので switch を使えない
		if(status == END){
			break;
		}else if(status == STEP_THOUGH){
			int stepcount = 1;
			int end = 0;
			while(s->opcode != SCRIPT_OPCODE_DUMP_END){
				switch(s->opcode){
				case SCRIPT_OPCODE_STEP_START:
					stepcount++;
					break;
				case SCRIPT_OPCODE_STEP_END:
					stepcount--;
					if(stepcount == 0){
						end = 1;
					}
					break;
				}
				s++;
				if(end == 1){
					break;
				}
			}
			status = DUMP;
		}
		//opcode 別に script の制御. while を抜けるので switch を使えない
		if(s->opcode == SCRIPT_OPCODE_STEP_END){
			if(variable_num == 0){
				printf("%d:%s loop closed, missing STEP_START\n", s->line, LOGICAL_ERROR_PREFIX);
				return error + 1;
			}
			s = step_end(&s[1]);
			status = DUMP;
		}else if(s->opcode == SCRIPT_OPCODE_DUMP_END){
			break;
		}else{
			s++;
		}
	}
	
	//loop open conform
	if(variable_num != 0){
		printf("%d:%s loop opened, missing STEP_END\n", s->line, LOGICAL_ERROR_PREFIX);
		error += 1;
	}
	//dump length conform
	error += dump_length_conform(OPSTR_CPU_ROMSIZE, cpu_romsize, r->cpu_rom.size);
	error += dump_length_conform(OPSTR_CPU_RAMSIZE, cpu_ramsize, r->cpu_ram.size);
	error += dump_length_conform(OPSTR_PPU_ROMSIZE, ppu_romsize, r->ppu_rom.size);
	
	//command line config override
	if(c->mirror != CONFIG_OVERRIDE_UNDEF){
		r->mirror = c->mirror;
	}
	if(c->backupram != CONFIG_OVERRIDE_UNDEF){
		r->backupram = 1;
	}
	if(c->mapper != CONFIG_OVERRIDE_UNDEF){
		//program mode で mapper 変更を防ぐ
		assert(c->mode == MODE_ROM_DUMP);
		r->mappernum = c->mapper;
	}
	if(c->syntaxtest == 1){
		if(error == 0){
			printf("syntax ok!\n");
		}
		error += 1;
	}
	return error;
}

/*
execute() 用サブ関数とデータ
*/
static int execute_connection_check(const struct reader_driver *d)
{
	int ret = OK;
	const int testsize = 0x80;
	int testcount = 3;
	uint8_t *master, *reload;
	master = Malloc(testsize);
	reload = Malloc(testsize);

	d->cpu_read(0xfee0, testsize, master);
	
	while(testcount != 0){
		d->cpu_read(0xfee0, testsize, reload);
		if(memcmp(master, reload, testsize) != 0){
			ret = NG;
			break;
		}
		testcount--;
	}
	
	Free(master);
	Free(reload);
	return ret;
}

enum {PPU_TEST_RAM, PPU_TEST_ROM};
const uint8_t PPU_TEST_DATA[] = "PPU_TEST_DATA";
static int ppu_ramfind(const struct reader_driver *d)
{
	const long length = sizeof(PPU_TEST_DATA);
	const long testaddr = 123;
	uint8_t writedata[length];
	//ppu ram data fill 0
	memset(writedata, 0, length);
	d->ppu_write(testaddr, length, writedata);
	
	//ppu test data write
	d->ppu_write(testaddr, length, PPU_TEST_DATA);

	d->ppu_read(testaddr, length, writedata);
	if(memcmp(writedata, PPU_TEST_DATA, length) == 0){
		return PPU_TEST_RAM;
	}
	return PPU_TEST_ROM;
}

static int ramtest(const int region, const struct reader_driver *d, long address, long length, uint8_t *writedata, uint8_t *testdata, const long filldata)
{
	memset(writedata, filldata, length);
	switch(region){
	case MEMORY_AREA_CPU_RAM:
		d->cpu_write_6502(address, length, writedata);
		break;
	case MEMORY_AREA_PPU:
		d->ppu_write(address, length, writedata);
		break;
	default:
		assert(0);
	}
	switch(region){
	case MEMORY_AREA_CPU_RAM:
		d->cpu_read(address, length, testdata);
		break;
	case MEMORY_AREA_PPU:
		d->ppu_read(address, length, testdata);
		break;
	default:
		assert(0);
	}
	if(memcmp(writedata, testdata, length) == 0){
		return 0;
	}
	return 1;
}

static const long SRAMTESTDATA[] = {0xff, 0xaa, 0x55, 0x00};
static int sramtest(const int region, const struct reader_driver *d, long address, long length)
{
	uint8_t *writedata, *testdata;
	int error = 0;
	int i;
	testdata = Malloc(length);
	writedata = Malloc(length);
	for(i = 0; i < sizeof(SRAMTESTDATA) / sizeof(long); i++){
		const long filldata = SRAMTESTDATA[i];
		error += ramtest(region, d, address, length, testdata, writedata, filldata);
	}
	Free(testdata);
	Free(writedata);
	return error;
}

static void readbuffer_print(const struct memory *m, long length)
{
	if(length >= 0x10){
		length = 0x10;
	}
	printf("%s ROM 0x%05x:", m->name, m->offset);
	int offset = 0;
	const uint8_t *data;
	data = m->data;
	while(length != 0){
		char safix;
		switch(offset & 0xf){
		default:
			safix = ' ';
			break;
		case 0x7:
			safix = '-';
			break;
		case 0xf:
			safix = ';';
			break;
		}
		printf("%02x%c", (int) *data, safix);
		data++;
		offset++;
		length--;
	}
}

static void checksum_print(const uint8_t *data, long length)
{
	int sum = 0;
	while(length != 0){
		sum += (int) *data;
		data++;
		length--;
	}
	printf(" 0x%06x\n", sum);
}

static void read_result_print(const struct memory *m, long length)
{
	readbuffer_print(m, length);
	checksum_print(m->data, length);
	fflush(stdout);
}

static void execute_program_begin(const struct memory *m, const long length)
{
	int tail = m->offset + (int) length - 1;
	printf("writing %s area 0x%06x-0x%06x ... ", m->name, m->offset, tail);
	fflush(stdout);
}

static const char STR_OK[] = "OK";
static const char STR_NG[] = "NG";

//memcmp の戻り値が入るので 0 が正常
static void execute_program_finish(int result)
{
	const char *str;
	str = STR_NG;
	if(result == 0){
		str = STR_OK;
	}
	printf("%s\n", str);
	fflush(stdout);
}
static const char EXECUTE_ERROR_PREFIX[] = "execute error:";
static const char EXECUTE_PROGRAM_PREPARE[] = "%s device initialize ... ";
static const char EXECUTE_PROGRAM_DONE[] = "done\n";
static void execute_cpu_ramrw(const struct reader_driver *d, const struct memory *ram, int mode, long address, long length)
{
	if(mode == MODE_RAM_WRITE){
		d->cpu_write_6502(address, length, ram->data);
/*		const uint8_t *writedata;
		long a = address;
		long l = length;
		writedata = ram->data;
		while(l != 0){
			d->cpu_write_6502(a++, *writedata, wait);
			writedata += 1;
			l--;
		}*/
		uint8_t *compare;
		compare = Malloc(length);
		d->cpu_read(address, length, compare);
		if(memcmp(ram->data, compare, length) == 0){
			printf("RAM data write success\n");
		}else{
			printf("RAM data write failed\n");
		}
		Free(compare);
	}else{
		d->cpu_read(address, length, ram->data);
	}
}

static int execute(const struct script *s, const struct st_config *c, struct romimage *r)
{
	const struct reader_driver *const d = c->reader;
	switch(d->open_or_close(READER_OPEN)){
	case OK:
		d->init();
		break;
	case NG:
		printf("%s driver open error\n", EXECUTE_ERROR_PREFIX);
		return NG;
	default:
		assert(0);
	}
	if(execute_connection_check(d) == NG){
		printf("%s maybe connection error\n", EXECUTE_ERROR_PREFIX);
		d->open_or_close(READER_CLOSE);
		return NG;
	}
	uint8_t *program_compare;
	program_compare = NULL;
	if(c->mode == MODE_ROM_PROGRAM){
		printf("flashmemory/SRAM program mode. To abort programming, press Ctrl+C\n");
		int size = r->cpu_rom.size;
		if(size < r->ppu_rom.size){
			size = r->ppu_rom.size;
		}
		program_compare = Malloc(size);
	}
	struct memory cpu_rom, ppu_rom, cpu_ram;
	cpu_rom = r->cpu_rom;
	ppu_rom = r->ppu_rom;
	cpu_ram = r->cpu_ram;
	
	int status = DUMP;
	int programcount_cpu = 0, programcount_ppu = 0;
	int flashcommand_change_cpu = 0, flashcommand_change_ppu = 0;
	variable_init_all();
	while(s->opcode != SCRIPT_OPCODE_DUMP_END){
		//printf("%s\n", SCRIPT_SYNTAX[s->opcode].name);
		switch(s->opcode){
		case SCRIPT_OPCODE_CPU_COMMAND:
			command_mask(MEMORY_AREA_CPU_ROM, s->value[0], s->value[1], s->value[2], &(r->cpu_flash));
			flashcommand_change_cpu = 1;
			break;
		case SCRIPT_OPCODE_CPU_READ:{
			struct memory *m;
			const long address = s->value[0];
			const long length = s->value[1];
			m = &cpu_rom;
			d->cpu_read(address, length, m->data);
			read_result_print(m, length);
			m->data += length;
			m->offset += length;
			}break;
		case SCRIPT_OPCODE_CPU_WRITE:{
			long data;
			uint8_t d8;
			expression_calc(&s->expression, &data);
			d8 = data & 0xff;
			d->cpu_write_6502(s->value[0], 1, &d8);
			}
			break;
		case SCRIPT_OPCODE_CPU_RAMRW:{
			const long address = s->value[0];
			const long length = s->value[1];
			if(c->mode == MODE_RAM_WRITE){
				if(sramtest(MEMORY_AREA_CPU_RAM, d, address, length) != 0){
					printf("SRAM test NG\n");
					status = END;
					break;
				}
			}
			execute_cpu_ramrw(d, &cpu_ram, c->mode, address, length);
			read_result_print(&cpu_ram, length);
			cpu_ram.data += length;
			cpu_ram.offset += length;
			}
			break;
		case SCRIPT_OPCODE_CPU_PROGRAM:{
			if(c->cpu_flash_driver->id_device == FLASH_ID_DEVICE_DUMMY){
				break;
			}
			if(flashcommand_change_cpu != 0){
				r->cpu_flash.config(
					r->cpu_flash.command_0000,
					r->cpu_flash.command_2aaa,
					r->cpu_flash.command_5555,
					r->cpu_flash.pagesize
				);
				flashcommand_change_cpu = 0;
			}
			if(programcount_cpu++ == 0){
				printf(EXECUTE_PROGRAM_PREPARE, cpu_rom.name);
				fflush(stdout);
				//device によっては erase
				c->cpu_flash_driver->init(&(r->cpu_flash), c->cpu_flash_driver->erase_wait);
				printf(EXECUTE_PROGRAM_DONE);
				fflush(stdout);
			}
			const long address = s->value[0];
			const long length = s->value[1];
			execute_program_begin(&cpu_rom, length);
			c->cpu_flash_driver->program(
				&(r->cpu_flash),
				address, length,
				&cpu_rom
			);
			int result;
			if(1){
				d->cpu_read(address, length, program_compare);
				result = memcmp(program_compare, cpu_rom.data, length);
				execute_program_finish(result);
			}else{
				puts("skip");
				result = 1;
			}
			cpu_rom.data += length;
			cpu_rom.offset += length;
			
			if((DEBUG==0) && (result != 0)){
				status = END;
			}
			}
			break;
		case SCRIPT_OPCODE_PPU_COMMAND:
			command_mask(MEMORY_AREA_PPU, s->value[0], s->value[1], s->value[2], &(r->ppu_flash));
			flashcommand_change_ppu = 1;
			break;
		case SCRIPT_OPCODE_PPU_RAMFIND:
			if(ppu_ramfind(d) == PPU_TEST_RAM){
				printf("PPU_RAMFIND: charcter RAM found\n");
				r->ppu_rom.size = 0;
				status = END;
			}
			break;
		case SCRIPT_OPCODE_PPU_SRAMTEST:{
			const long address = s->value[0];
			const long length = s->value[1];
			printf("PPU_SRAMTEST: 0x%06x-0x%06x ", (int)ppu_rom.offset, (int) (ppu_rom.offset + length) - 1);
			if(sramtest(MEMORY_AREA_PPU, d, address, length) == 0){
				printf("%s\n", STR_OK);
			}else{
				printf("%s\n", STR_NG);
				//status = END;
			}
			}break;
		case SCRIPT_OPCODE_PPU_READ:{
			const long address = s->value[0];
			const long length = s->value[1];
			if(length == 0){
				/*for mmc2,4 protect.
				このときは1byte読み込んで、その内容はバッファにいれない*/
				uint8_t dummy;
				d->ppu_read(address, 1, &dummy);
			}else{
				d->ppu_read(address, length, ppu_rom.data);
				read_result_print(&ppu_rom, length);
			}
			ppu_rom.data += length;
			ppu_rom.offset += length;
			}
			break;
		case SCRIPT_OPCODE_PPU_WRITE:
			if(DEBUG == 1){
				long data;
				uint8_t d8;
				expression_calc(&s->expression, &data);
				d8 = data;
				d->ppu_write(s->value[0], 1, &d8);
			}
			break;
		case SCRIPT_OPCODE_PPU_PROGRAM:{
			if(c->ppu_flash_driver->id_device == FLASH_ID_DEVICE_DUMMY){
				break;
			}
			if(flashcommand_change_ppu != 0){
				r->ppu_flash.config(
					r->ppu_flash.command_0000,
					r->ppu_flash.command_2aaa,
					r->ppu_flash.command_5555,
					r->ppu_flash.pagesize
				);
				flashcommand_change_ppu = 0;
			}
			if(programcount_ppu++ == 0){
				printf(EXECUTE_PROGRAM_PREPARE, ppu_rom.name);
				fflush(stdout);
				c->ppu_flash_driver->init(&(r->ppu_flash), c->ppu_flash_driver->erase_wait);
				printf(EXECUTE_PROGRAM_DONE);
				fflush(stdout);
			}
			const long address = s->value[0];
			const long length = s->value[1];
			execute_program_begin(&ppu_rom, length);
			c->ppu_flash_driver->program(
				&(r->ppu_flash),
				address, length,
				&ppu_rom
			);
			d->ppu_read(address, length, program_compare);
			const int result = memcmp(program_compare, ppu_rom.data, length);
			execute_program_finish(result);
			ppu_rom.data += length;
			ppu_rom.offset += length;
			
			if((DEBUG==0) && (result != 0)){
				status = END;
			}
			}
			break;
		case SCRIPT_OPCODE_STEP_START:{
			if(s->value[STEP_START] >= s->value[STEP_END]){
				status = STEP_THOUGH;
			}else{
				//ループの戻り先はこの命令の次なので &s[1]
				step_new(s->variable, s->value[STEP_START], s->value[STEP_END], s->value[STEP_NEXT], &s[1]);
			}
			}break;
		case SCRIPT_OPCODE_DUMP_END:
			status = END;
			break;
		}
		if(status == END){
			break;
		}else if(status == STEP_THOUGH){
			//こぴぺ
			int stepcount = 1;
			int end = 0;
			while(s->opcode != SCRIPT_OPCODE_DUMP_END){
				switch(s->opcode){
				case SCRIPT_OPCODE_STEP_START:
					stepcount++;
					break;
				case SCRIPT_OPCODE_STEP_END:
					stepcount--;
					if(stepcount == 0){
						end = 1;
					}
					break;
				}
				s++;
				if(end == 1){
					break;
				}
			}
			status = DUMP;
		}
		
		if(s->opcode == SCRIPT_OPCODE_STEP_END){
			s = step_end(++s);
		}else{
			s++;
		}
	}
	d->open_or_close(READER_CLOSE);
	if(program_compare != NULL){
		Free(program_compare);
	}
	return OK;
}

void script_load(const struct st_config *c)
{
	struct script *s;
	{
		int scriptsize = 0;
		char *buf;
		
		buf = buf_load_full(c->script, &scriptsize);
		if(buf == NULL){
			printf("scriptfile open error\n");
			return;
		}
		char **text;
		text = Malloc(sizeof(char*) * TEXT_MAXLINE);
		const int text_num = text_load(buf, scriptsize, text);
		if(text_num == 0){
			printf("script line too much\n");
			Free(buf);
			Free(text);
			return;
		}
		s = Malloc(sizeof(struct script) * (text_num + 1));
		//logical_check, execute 共に s->opcode が DUMP_END になるまで続ける。DUMP_END の入れ忘れ用に末尾のscriptに必ず DUMP_END をいれる
		{
			struct script *k;
			k = s;
			k += text_num;
			k->opcode = SCRIPT_OPCODE_DUMP_END;
		}
		const int error = syntax_check(text, text_num, s, c->mode);
		Free(buf);
		Free(text);
		if(error != 0){
			Free(s);
			return;
		}
	}
	struct romimage r = {
		.cpu_rom = {
			.size = 0, .offset = 0,
			.data = NULL,
			.attribute = MEMORY_ATTR_NOTUSE,
			.name = "program"
		},
		.ppu_rom = {
			.size = 0, .offset = 0,
			.data = NULL,
			.attribute = MEMORY_ATTR_NOTUSE,
			.name = "charcter"
		},
		.cpu_ram = {
			.size = 0, .offset = 0,
			.data = NULL,
			.attribute = MEMORY_ATTR_NOTUSE,
			.name = STR_REGION_CPU
		},
		//device に応じた関数ポインタを flash_order に渡す
		.cpu_flash = {
			.command_0000 = 0x8000,
			.command_2aaa = 0,
			.command_5555 = 0,
			.pagesize = c->cpu_flash_driver->pagesize,
			.command_mask = c->cpu_flash_driver->command_mask,
			.config = c->reader->cpu_flash_config,
			.erase = c->reader->cpu_flash_erase,
			.program = c->reader->cpu_flash_program,
			.write = NULL //c->reader->cpu_write_6502
		},
		.ppu_flash = {
			.command_0000 = 0,
			.command_2aaa = 0,
			.command_5555 = 0,
			.pagesize = c->ppu_flash_driver->pagesize,
			.command_mask = c->ppu_flash_driver->command_mask,
			.config = c->reader->ppu_flash_config,
			.erase = c->reader->ppu_flash_erase,
			.program = c->reader->ppu_flash_program,
			.write = NULL //c->reader->ppu_write
		},
		.mappernum = 0,
		.mirror = MIRROR_PROGRAMABLE
	};
	//attribute はその struct data に対しての RW なので要注意
	switch(c->mode){
	case MODE_ROM_DUMP:
		r.cpu_rom.attribute = MEMORY_ATTR_WRITE;
		r.ppu_rom.attribute = MEMORY_ATTR_WRITE;
		break;
	case MODE_RAM_READ:
		r.cpu_ram.attribute = MEMORY_ATTR_WRITE;
		break;
	case MODE_RAM_WRITE:
		r.cpu_ram.attribute = MEMORY_ATTR_READ;
		break;
	case MODE_ROM_PROGRAM:
		r.cpu_rom.attribute = MEMORY_ATTR_READ;
		r.ppu_rom.attribute = MEMORY_ATTR_READ;
		break;
	default:
		assert(0);
	}
	
	if(logical_check(s, c, &r) == 0){
		//dump RAM 領域取得
		if(nesbuffer_malloc(&r, c->mode) == false){
			Free(s);
			if((c->mode == MODE_RAM_WRITE) && (r.cpu_ram.data != NULL)){
				Free(r.cpu_ram.data);
			}
			return;
		}
		//script execute!!
		if(execute(s, c, &r) == OK){
			//成果出力
			switch(c->mode){
			case MODE_ROM_DUMP:
				nesfile_create(&r, c->romimage);
				break;
			case MODE_RAM_READ:
				backupram_create(&(r.cpu_ram), c->ramimage);
				break;
			}
		}
		//dump RAM 領域解放
		nesbuffer_free(&r, c->mode);
		if((c->mode == MODE_RAM_WRITE) && (r.cpu_ram.data != NULL)){
			Free(r.cpu_ram.data);
		}
	}
	Free(s);
}
