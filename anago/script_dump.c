#include <assert.h>
#include <stdio.h>
#include <squirrel.h>
#include <sqstdio.h>
#include <sqstdaux.h>
#include "type.h"
#include "header.h"
#include "progress.h"
#include "memory_manage.h"
#include "reader_master.h"
#include "squirrel_wrap.h"
#include "script_common.h"
#include "script_dump.h"

struct dump_driver{
	const char *target;
	struct memory_driver{
		struct memory memory;
		long read_count;
		void (*const write)(long address, long length, const uint8_t *data);
		void (*const read)(long address, long length, u8 *data);
	}cpu, ppu;
	uint8_t (*const vram_connection)(void);
	bool progress;
};
static SQInteger write(HSQUIRRELVM v, struct memory_driver *t)
{
	long address, data;
	SQRESULT r = qr_argument_get(v, 2, &address, &data);
	if(SQ_FAILED(r)){
		return r;
	}
	uint8_t d8 = (uint8_t) data;
	t->write(address, 1, &d8);
	return 0;
}
static SQInteger cpu_write(HSQUIRRELVM v)
{
	struct dump_driver *d;
	SQRESULT r =  qr_userpointer_get(v, (SQUserPointer) &d);
	if(SQ_FAILED(r)){
		return r;
	}
	return write(v, &d->cpu);
}

static void buffer_show(struct memory *t, long length)
{
	int i;
	const uint8_t *buf = t->data + t->offset;
	printf("%s 0x%06x:", t->name, t->offset);
	for(i = 0; i < 0x10; i++){
		char dump[3+1];
		sprintf(dump, "%02x", buf[i]);
		switch(i){
		case 7:
			dump[2] = '-';
			break;
		case 0x0f:
			dump[2] = '\0';
			break;
		default:
			dump[2] = ' ';
			break;
		}
		dump[3] = '\0';
		printf("%s", dump);
	}
	int sum = 0;
	while(length != 0){
		sum += (int) *buf;
		buf++;
		length--;
	}
	printf(":0x%06x\n", sum);
	fflush(stdout);
}

static void progress_show(struct dump_driver *d)
{
	if(d->progress == true){
		progress_draw(d->cpu.memory.offset, d->cpu.memory.size, d->ppu.memory.offset, d->ppu.memory.size);
	}
}
static SQInteger read(HSQUIRRELVM v, struct memory_driver *t, bool progress)
{
	long address, length;
	SQRESULT r = qr_argument_get(v, 2, &address, &length);
	if(SQ_FAILED(r)){
		return r;
	}
	t->read(address, length == 0 ? 1: length, t->memory.data + t->memory.offset);
	if((length != 0) && (progress == false)){
		buffer_show(&t->memory, length);
	}
	t->memory.offset += length;
	return 0;
}
static SQInteger cpu_read(HSQUIRRELVM v)
{
	struct dump_driver *d;
	SQRESULT r =  qr_userpointer_get(v, (SQUserPointer) &d);
	if(SQ_FAILED(r)){
		return r;
	}
	r = read(v, &d->cpu, d->progress);
	progress_show(d);
	return r;
}

static SQInteger ppu_read(HSQUIRRELVM v)
{
	struct dump_driver *d;
	SQRESULT r =  qr_userpointer_get(v, (SQUserPointer) &d);
	if(SQ_FAILED(r)){
		return r;
	}
	r = read(v, &d->ppu, d->progress);
	progress_show(d);
	return r;
}

static SQInteger ppu_ramfind(HSQUIRRELVM v)
{
	struct dump_driver *d;
	enum{
		testsize = 8,
		testaddress = 1234
	};
	static const uint8_t test_val[testsize] = {0xaa, 0x55, 0, 0xff, 0x46, 0x49, 0x07, 0x21};
	static const uint8_t test_str[testsize] = "pputest";
	uint8_t test_result[testsize];

	SQRESULT r =  qr_userpointer_get(v, (SQUserPointer) &d);
	if(SQ_FAILED(r)){
		return r;
	}
	d->ppu.write(testaddress, testsize, test_val);
	d->ppu.read(testaddress, testsize, test_result);
	if(memcmp(test_val, test_result, testsize) != 0){
		sq_pushbool(v, SQFalse);
		return 1;
	}
	d->ppu.write(testaddress, testsize, test_str);
	d->ppu.read(testaddress, testsize, test_result);
	if(memcmp(test_str, test_result, testsize) != 0){
		sq_pushbool(v, SQFalse);
		return 1;
	}
	d->ppu.memory.offset = 0;
	d->ppu.memory.size = 0;
	sq_pushbool(v, SQTrue);
	return 1;
}

//test 時/1度目の call で使用
static SQInteger memory_new(HSQUIRRELVM v)
{
	struct dump_driver *d;
	SQRESULT r =  qr_userpointer_get(v, (SQUserPointer) &d);
	if(SQ_FAILED(r)){
		return r;
	}
	r = qr_argument_get(v, 2, &d->cpu.memory.size, &d->ppu.memory.size);
	if(SQ_FAILED(r)){
		return r;
	}
	d->cpu.memory.offset = 0;
	d->cpu.memory.data = Malloc(d->cpu.memory.size);
	d->ppu.memory.offset = 0;
	d->ppu.memory.data = Malloc(d->ppu.memory.size);
	return 0;
}

//dump 時/2度目の call で nesfile_save として使用
static SQInteger nesfile_save(HSQUIRRELVM v)
{
	struct dump_driver *d;
	SQRESULT r =  qr_userpointer_get(v, (SQUserPointer) &d);
	if(SQ_FAILED(r)){
		return r;
	}
	struct romimage image;
	long mirrorfind;
	r = qr_argument_get(v, 2, &image.mappernum, &mirrorfind);
	if(SQ_FAILED(r)){
		return r;
	}
	image.cpu_rom = d->cpu.memory;
	image.cpu_ram.data = NULL;
	image.ppu_rom = d->ppu.memory;
	image.mirror = MIRROR_PROGRAMABLE;
	if(mirrorfind == 1){
		if(d->vram_connection() == 0x05){
			image.mirror = MIRROR_VERTICAL;
		}else{
			image.mirror = MIRROR_HORIZONAL;
		}
	}
	image.backupram = 0;
	nesfile_create(&image, d->target);
	nesbuffer_free(&image, 0); //0 is MODE_xxx_xxxx
	
	d->cpu.memory.data = NULL;
	d->ppu.memory.data = NULL;
	return 0;
}

//dump 時/1度目の call で nesfile_save として使用
static SQInteger length_check(HSQUIRRELVM v)
{
	struct dump_driver *d;
	SQRESULT r =  qr_userpointer_get(v, (SQUserPointer) &d);
	if(SQ_FAILED(r)){
		return r;
	}
	bool cpu = true, ppu = true;
	r = 0;
	if(d->cpu.memory.size != d->cpu.read_count){
		cpu = false;
	}
	if(cpu == false){
		printf("cpu_romsize is not connected 0x%06x/0x%06x\n", (int) d->cpu.read_count, (int) d->cpu.memory.size);
	}
	if(d->ppu.memory.size != d->ppu.read_count){
		ppu = false;
	}
	if(ppu == false){
		printf("ppu_romsize is not connected 0x%06x/0x%06x\n", (int) d->ppu.read_count, (int) d->ppu.memory.size);
	}
	if(cpu == false || ppu == false){
		r = sq_throwerror(v, "script logical error");
	}
	return r;
}

static SQInteger read_count(HSQUIRRELVM v, struct memory_driver *t, const struct range *range_address, const struct range *range_length)
{
	long address, length;
	SQRESULT r = qr_argument_get(v, 2, &address, &length);
	if(SQ_FAILED(r)){
		return r;
	}
	r = range_check(v, "length", length, range_length);
	if(SQ_FAILED(r)){
		return r;
	}
	if((address < range_address->start) || ((address + length) > range_address->end)){
		printf("address range must be 0x%06x to 0x%06x", (int) range_address->start, (int) range_address->end);
		return sq_throwerror(v, "script logical error");;
	}
	t->read_count += length;
	return 0;
}
static SQInteger cpu_read_count(HSQUIRRELVM v)
{
	static const struct range range_address = {0x8000, 0x10000};
	//length == 0 は 対象アドレスを呼んで、バッファにいれない。mmc2, mmc4 で使用する。
	static const struct range range_length = {0x0000, 0x4000};
	struct dump_driver *d;
	SQRESULT r =  qr_userpointer_get(v, (SQUserPointer) &d);
	if(SQ_FAILED(r)){
		return r;
	}
	return read_count(v, &d->cpu, &range_address, &range_length);
}

static SQInteger ppu_read_count(HSQUIRRELVM v)
{
	static const struct range range_address = {0x0000, 0x2000};
	static const struct range range_length = {0x0001, 0x2000};
	struct dump_driver *d;
	SQRESULT r =  qr_userpointer_get(v, (SQUserPointer) &d);
	if(SQ_FAILED(r)){
		return r;
	}
	return read_count(v, &d->ppu, &range_address, &range_length);
}

static bool script_execute(HSQUIRRELVM v, struct config_dump *c, struct dump_driver *d)
{
	bool ret = true;
	if(SQ_FAILED(sqstd_dofile(v, _SC("dumpcore.nut"), SQFalse, SQTrue))){
		printf("dump core script error\n");
		ret = false;
	}else if(SQ_FAILED(sqstd_dofile(v, _SC(c->script), SQFalse, SQTrue))){
		printf("%s open error\n", c->script);
		ret = false;
	}else{
		SQRESULT r = qr_call(
			v, "dump", (SQUserPointer) d, true, 
			3, c->mappernum, c->increase.cpu, c->increase.ppu
		);
		if(SQ_FAILED(r)){
			ret = false;
			Free(d->cpu.memory.data);
			Free(d->ppu.memory.data);
			d->cpu.memory.data = NULL;
			d->ppu.memory.data = NULL;
		}
	}
	return ret;
}
void script_dump_execute(struct config_dump *c)
{
	struct dump_driver d = {
		.cpu = {
			.memory = {
				.name = "program",
				.size = 0, .offset = 0,
				.attribute = MEMORY_ATTR_WRITE,
				.transtype = TRANSTYPE_FULL,
				.data = NULL
			},
			.read_count = 0,
			.write = c->reader->cpu_write_6502,
			.read = c->reader->cpu_read
		},
		.ppu = {
			.memory = {
				.name = "charcter",
				.size = 0, .offset = 0,
				.attribute = MEMORY_ATTR_WRITE,
				.transtype = TRANSTYPE_FULL,
				.data = NULL
			},
			.read_count = 0,
			.write = c->reader->ppu_write,
			.read = c->reader->ppu_read
		},
		.vram_connection = c->reader->vram_connection,
		.target = c->target,
		.progress = c->progress
	};
	{
		HSQUIRRELVM v = qr_open(); 
		qr_function_register_global(v, "ppu_ramfind", script_nop);
		qr_function_register_global(v, "cpu_write", cpu_write_check);
		qr_function_register_global(v, "memory_new", memory_new);
		qr_function_register_global(v, "nesfile_save", length_check);
		qr_function_register_global(v, "cpu_read", cpu_read_count);
		qr_function_register_global(v, "ppu_read", ppu_read_count);
		qr_function_register_global(v, "require", script_require);
		if(script_execute(v, c, &d) == false){
			qr_close(v);
			return;
		}
		qr_close(v);
	}
	if(c->progress == true){
		progress_init();
	}
	{
		HSQUIRRELVM v = qr_open(); 
		qr_function_register_global(v, "memory_new", script_nop);
		qr_function_register_global(v, "nesfile_save", nesfile_save);
		qr_function_register_global(v, "cpu_write", cpu_write);
		qr_function_register_global(v, "cpu_read", cpu_read);
		qr_function_register_global(v, "ppu_read", ppu_read);
		qr_function_register_global(v, "ppu_ramfind", ppu_ramfind);
		qr_function_register_global(v, "require", script_require);
		script_execute(v, c, &d);
		qr_close(v);
	}
}
