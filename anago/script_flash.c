#include <assert.h>
#include <stdio.h>
#include <squirrel.h>
#include <sqstdio.h>
#include <sqstdaux.h>
#include <kazzo_task.h>
#include "type.h"
#include "header.h"
#include "memory_manage.h"
#include "reader_master.h"
#include "squirrel_wrap.h"
#include "flash_device.h"
#include "progress.h"
#include "script_common.h"
#include "script_flash.h"

struct anago_driver{
	struct anago_flash_order{
		bool command_change;
		struct{
			long address, length, count, offset;
		}programming, compare;
		long c000x, c2aaa, c5555;
		struct memory *const memory;
		struct flash_device *const device;
		void (*const config)(long c000x, long c2aaa, long c5555, long unit);
		void (*const device_get)(uint8_t s[2]);
		void (*const write)(long address, long length, const uint8_t *data);
		void (*const read)(long address, long length, u8 *data);
		void (*const erase)(long address, bool dowait);
		long (*const program)(long address, long length, const u8 *data, bool dowait);
	}order_cpu, order_ppu;
	void (*const flash_status)(uint8_t s[2]);
	uint8_t (*const vram_connection)(void);
	const enum vram_mirroring vram_mirroring;
	bool compare;
};

static SQInteger vram_mirrorfind(HSQUIRRELVM v)
{
	struct anago_driver *d;
	SQRESULT r =  qr_userpointer_get(v, (SQUserPointer) &d);
	if(SQ_FAILED(r)){
		return r;
	}
	if((d->vram_connection() == 0x05) != (d->vram_mirroring == MIRROR_VERTICAL)){
		puts("warning: vram mirroring is inconnect");
	}
	return 0;
}
static SQInteger command_set(HSQUIRRELVM v, struct anago_flash_order *t)
{
	long command, address ,mask;
	SQRESULT r = qr_argument_get(v, 3, &command, &address, &mask);
	if(SQ_FAILED(r)){
		return r;
	}
	long d = command & (mask - 1);
	d |= address;
	switch(command){
	case 0x0000:
		t->c000x = d;
		break;
	case 0x02aa: case 0x2aaa:
		t->c2aaa = d;
		break;
	case 0x0555: case 0x5555:
		t->c5555 = d;
		break;
	default:
		return sq_throwerror(v, "unknown command address");
	}
	t->command_change = true;
	return 0;
}
static SQInteger cpu_command(HSQUIRRELVM v)
{
	struct anago_driver *d;
	SQRESULT r =  qr_userpointer_get(v, (SQUserPointer) &d);
	if(SQ_FAILED(r)){
		return r;
	}
	return command_set(v, &d->order_cpu);
}
static SQInteger ppu_command(HSQUIRRELVM v)
{
	struct anago_driver *d;
	SQRESULT r =  qr_userpointer_get(v, (SQUserPointer) &d);
	if(SQ_FAILED(r)){
		return r;
	}
	return command_set(v, &d->order_ppu);
}
static SQInteger write(HSQUIRRELVM v, struct anago_flash_order *t)
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
	struct anago_driver *d;
	SQRESULT r =  qr_userpointer_get(v, (SQUserPointer) &d);
	if(SQ_FAILED(r)){
		return r;
	}
	return write(v, &d->order_cpu);
}
static SQInteger erase_set(HSQUIRRELVM v, struct anago_flash_order *t, const char *region)
{
	t->config(t->c000x, t->c2aaa, t->c5555, t->device->pagesize);
	t->command_change = false;
	if(t->device->erase_require == true){
		t->erase(t->c2aaa, false);
		printf("erasing %s memory...\n", region);
		fflush(stdout);
	}
	return 0;
}
static SQInteger cpu_erase(HSQUIRRELVM v)
{
	struct anago_driver *d;
	SQRESULT r =  qr_userpointer_get(v, (SQUserPointer) &d);
	if(SQ_FAILED(r)){
		return r;
	}
	return erase_set(v, &d->order_cpu, "program");
}
static SQInteger ppu_erase(HSQUIRRELVM v)
{
	struct anago_driver *d;
	SQRESULT r =  qr_userpointer_get(v, (SQUserPointer) &d);
	if(SQ_FAILED(r)){
		return r;
	}
	return erase_set(v, &d->order_ppu, "charcter");
}
static SQInteger program_regist(HSQUIRRELVM v, const char *name, struct anago_flash_order *t)
{
	SQRESULT r = qr_argument_get(v, 2, &t->programming.address, &t->programming.length);
	if(SQ_FAILED(r)){
		return r;
	}
	t->compare = t->programming;
	t->compare.offset = t->memory->offset & (t->memory->size - 1);
	if(t->command_change == true){
		t->config(t->c000x, t->c2aaa, t->c5555, t->device->pagesize);
		t->command_change = false;
	}
	
/*	printf("programming %s ROM area 0x%06x...\n", name, t->memory->offset);
	fflush(stdout);*/
	return sq_suspendvm(v);
}
static void program_execute(struct anago_flash_order *t)
{
	const long w = t->program(t->programming.address, t->programming.length, t->memory->data + t->memory->offset, false);
	t->programming.address += w;
	t->programming.length -= w;
	t->memory->offset += w;
	t->memory->offset &= t->memory->size - 1;
	t->programming.offset += w;
}

static bool program_compare(struct anago_flash_order *t)
{
	uint8_t *comparea = Malloc(t->compare.length);
	bool ret = false;
	t->read(t->compare.address, t->compare.length, comparea);
	if(memcmp(comparea, t->memory->data + t->compare.offset, t->compare.length) == 0){
		ret = true;
	}
	Free(comparea);
	return ret;
}
static SQInteger cpu_program_memory(HSQUIRRELVM v)
{
	struct anago_driver *d;
	SQRESULT r =  qr_userpointer_get(v, (SQUserPointer) &d);
	if(SQ_FAILED(r)){
		return r;
	}
	return program_regist(v, "program", &d->order_cpu);
}
static SQInteger ppu_program_memory(HSQUIRRELVM v)
{
	struct anago_driver *d;
	SQRESULT r =  qr_userpointer_get(v, (SQUserPointer) &d);
	if(SQ_FAILED(r)){
		return r;
	}
	return program_regist(v, "charcter", &d->order_ppu);
}

static long erase_timer_get(struct anago_flash_order *t)
{
	if(
		(t->memory->transtype != TRANSTYPE_EMPTY) && 
		(t->device->erase_require == true)
	){
		return t->device->erase_wait;
	}else{
		return 0;
	}
}
static SQInteger erase_wait(HSQUIRRELVM v)
{
	struct anago_driver *d;
	SQRESULT r =  qr_userpointer_get(v, (SQUserPointer) &d);
	if(SQ_FAILED(r)){
		return r;
	}
	if(0){
		long timer_wait = erase_timer_get(&d->order_cpu);
		long timer_ppu = erase_timer_get(&d->order_ppu);
		if(timer_wait < timer_ppu){
			timer_wait = timer_ppu;
		}
		Sleep(timer_wait);
	}else{
		uint8_t s[2];
		do{
			Sleep(2);
			d->flash_status(s);
		}while((s[0] != 0) && (s[1] != 0));
	}
	return 0;
}

static bool program_memoryarea(HSQUIRRELVM co, struct anago_flash_order *t, bool compare, const char *region, SQInteger *state, bool *console_update)
{
	if(t->programming.length == 0){
		if(t->programming.offset != 0 && compare == true){
			if(program_compare(t) == false){
				printf("%s memory compare error\n", region);
				return false;
			}
		}

		sq_wakeupvm(co, SQFalse, SQFalse, SQTrue/*, SQTrue*/);
		*state = sq_getvmstate(co);
	}else{
		program_execute(t);
		*console_update = true;
	}
	return true;
}

static SQInteger program_main(HSQUIRRELVM v)
{
	if(sq_gettop(v) != (1 + 3)){ //roottable, userpointer, co_cpu, co_ppu
		return sq_throwerror(v, "argument number error");
	}
	struct anago_driver *d;
	SQRESULT r =  qr_userpointer_get(v, (SQUserPointer) &d);
	if(SQ_FAILED(r)){
		return r;
	}
	HSQUIRRELVM co_cpu, co_ppu;
	if(SQ_FAILED(sq_getthread(v, 3, &co_cpu))){
		return sq_throwerror(v, "thread error");
	}
	if(SQ_FAILED(sq_getthread(v, 4, &co_ppu))){
		return sq_throwerror(v, "thread error");
	}
	SQInteger state_cpu = sq_getvmstate(co_cpu);
	SQInteger state_ppu = sq_getvmstate(co_ppu);
	const long sleepms = d->compare == true ? 6 : 2; //W29C040 で compare をすると、error が出るので出ない値に調整 (やっつけ対応)
	
	progress_init();
	while((state_cpu != SQ_VMSTATE_IDLE) || (state_ppu != SQ_VMSTATE_IDLE)){
		uint8_t s[2];
		bool console_update = false;
		Sleep(sleepms);
		d->flash_status(s);
		if(state_cpu != SQ_VMSTATE_IDLE && s[0] == KAZZO_TASK_FLASH_IDLE){
			if(program_memoryarea(co_cpu, &d->order_cpu, d->compare, "program", &state_cpu, &console_update) == false){
				return 0;
			}
		}
		if(state_ppu != SQ_VMSTATE_IDLE && s[1] == KAZZO_TASK_FLASH_IDLE){
			if(program_memoryarea(co_ppu, &d->order_ppu, d->compare, "charcter", &state_ppu, &console_update) == false){
				return 0;
			}
		}
		if(console_update == true){
			progress_draw(d->order_cpu.programming.offset, d->order_cpu.programming.count, d->order_ppu.programming.offset, d->order_ppu.programming.count);
		}
	}
	return 0;
}

static SQInteger program_count(HSQUIRRELVM v, struct anago_flash_order *t, const struct range *range_address, const struct range *range_length)
{
	SQRESULT r = qr_argument_get(v, 2, &t->programming.address, &t->programming.length);
	if(SQ_FAILED(r)){
		return r;
	}
	r = range_check(v, "length", t->programming.length, range_length);
	if(SQ_FAILED(r)){
		return r;
	}
	if((t->programming.address < range_address->start) || ((t->programming.address + t->programming.length) > range_address->end)){
		printf("address range must be 0x%06x to 0x%06x", (int) range_address->start, (int) range_address->end - 1);
		return sq_throwerror(v, "script logical error");;
	}
	t->programming.count += t->programming.length;
	return 0;
}
static SQInteger cpu_program_count(HSQUIRRELVM v)
{
	static const struct range range_address = {0x8000, 0x10000};
	static const struct range range_length = {0x0100, 0x4000};
	struct anago_driver *d;
	SQRESULT r =  qr_userpointer_get(v, (SQUserPointer) &d);
	if(SQ_FAILED(r)){
		return r;
	}
	return program_count(v, &d->order_cpu, &range_address, &range_length);
}

static SQInteger ppu_program_count(HSQUIRRELVM v)
{
	static const struct range range_address = {0x0000, 0x2000};
	static const struct range range_length = {0x0100, 0x2000};
	struct anago_driver *d;
	SQRESULT r =  qr_userpointer_get(v, (SQUserPointer) &d);
	if(SQ_FAILED(r)){
		return r;
	}
	return program_count(v, &d->order_ppu, &range_address, &range_length);
}

static bool script_execute(HSQUIRRELVM v, struct config_flash *c, struct anago_driver *d)
{
	bool ret = true;
	if(SQ_FAILED(sqstd_dofile(v, _SC("flashcore.nut"), SQFalse, SQTrue))){
		printf("flash core script error\n");
		ret = false;
	}else if(SQ_FAILED(sqstd_dofile(v, _SC(c->script), SQFalse, SQTrue))){
		printf("%s open error\n", c->script);
		ret = false;
	}else{
		SQRESULT r = qr_call(
			v, "program", (SQUserPointer) d, true, 
			1 + 3 * 2, c->rom.mappernum, 
			d->order_cpu.memory->transtype, d->order_cpu.memory->size, d->order_cpu.device->capacity,
			d->order_ppu.memory->transtype, d->order_ppu.memory->size, d->order_ppu.device->capacity
		);
		if(SQ_FAILED(r)){
			ret = false;
		}
	}
	return ret;
}

void script_flash_execute(struct config_flash *c)
{
	struct anago_driver d = {
		.order_cpu = {
			.command_change = true,
			.programming = {
				.count = 0, .offset = 0
			},
			.device = &c->flash_cpu,
			.memory = &c->rom.cpu_rom,
			.config = c->reader->cpu_flash_config,
			.device_get = c->reader->cpu_flash_device_get,
			.write = c->reader->cpu_write_6502,
			.read = c->reader->cpu_read,
			.erase = c->reader->cpu_flash_erase,
			.program = c->reader->cpu_flash_program
		},
		.order_ppu = {
			.command_change = true,
			.programming = {
				.count = 0, .offset = 0
			},
			.device = &c->flash_ppu,
			.memory = &c->rom.ppu_rom,
			.config = c->reader->ppu_flash_config,
			.device_get = c->reader->ppu_flash_device_get,
			.write = c->reader->ppu_write,
			.read = c->reader->ppu_read,
			.erase = c->reader->ppu_flash_erase,
			.program = c->reader->ppu_flash_program,
		},
		.flash_status = c->reader->flash_status,
		.vram_connection = c->reader->vram_connection,
		.vram_mirroring = c->rom.mirror,
		.compare = c->compare
	};
	{
		static const char *functionname[] = {
			"cpu_erase", "ppu_erase",
			"erase_wait", "program_main"
		};
		HSQUIRRELVM v = qr_open();
		int i;
		for(i = 0; i < sizeof(functionname)/sizeof(char *); i++){
			qr_function_register_global(v, functionname[i], script_nop);
		}
		qr_function_register_global(v, "cpu_write", cpu_write_check);
		qr_function_register_global(v, "cpu_command", cpu_command);
		qr_function_register_global(v, "cpu_program", cpu_program_count);
		
		qr_function_register_global(v, "ppu_program", ppu_program_count);
		qr_function_register_global(v, "ppu_command", ppu_command);
		qr_function_register_global(v, "vram_mirrorfind", vram_mirrorfind);
		
		if(script_execute(v, c, &d) == false){
			qr_close(v);
			return;
		}
		qr_close(v);
		assert(d.order_cpu.memory->size != 0);
		if(d.order_cpu.programming.count % d.order_cpu.memory->size  != 0){
			printf("logical error: cpu_programsize is not connected 0x%06x/0x%06x\n", (int) d.order_cpu.programming.count, (int) d.order_cpu.memory->size);
			return;
		}
		if(d.order_ppu.memory->size != 0){
			if(d.order_ppu.programming.count % d.order_ppu.memory->size != 0){
				printf("logical error: ppu_programsize is not connected 0x%06x/0x%06x\n", (int) d.order_ppu.programming.count, (int) d.order_ppu.memory->size);
				return;
			}
		}
	}
	d.order_cpu.command_change = true;
	d.order_ppu.command_change = true;
	{
		HSQUIRRELVM v = qr_open(); 
		qr_function_register_global(v, "cpu_write", cpu_write);
		qr_function_register_global(v, "cpu_erase", cpu_erase);
		qr_function_register_global(v, "cpu_program", cpu_program_memory);
		qr_function_register_global(v, "cpu_command", cpu_command);
		qr_function_register_global(v, "ppu_erase", ppu_erase);
		qr_function_register_global(v, "ppu_program", ppu_program_memory);
		qr_function_register_global(v, "ppu_command", ppu_command);
		qr_function_register_global(v, "program_main", program_main);
		qr_function_register_global(v, "erase_wait", erase_wait);
		qr_function_register_global(v, "vram_mirrorfind", script_nop);
		script_execute(v, c, &d);
		qr_close(v);
	}
}
