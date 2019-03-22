#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <locale.h>
#include "memory_manage.h"
#include "type.h"
#include "flash_device.h"
#include "header.h"
#include "reader_master.h"
#include "reader_kazzo.h"
#include "reader_dummy.h"
#include "script_flash.h"
#include "script_dump.h"

static bool transtype_flash_set(char mode, struct memory *t)
{
	switch(mode){
	case 't':
		t->transtype = TRANSTYPE_TOP;
		break;
	case 'e':
		t->transtype = TRANSTYPE_EMPTY;
		break;
	case 'b':
		t->transtype = TRANSTYPE_BOTTOM;
		break;
	case 'f':
		t->transtype = TRANSTYPE_FULL;
		break;
	default:
		return false;
	}
	return true;
}

static bool transtype_set(const char *mode, struct romimage *t)
{
	switch(mode[0]){
	case 'a': case 'f': case 'F':
		if(mode[1] == '\0'){
			t->cpu_rom.transtype = TRANSTYPE_FULL;
			t->ppu_rom.transtype = TRANSTYPE_FULL;
			return true;
		}
		if(transtype_flash_set(mode[1], &t->cpu_rom) == false){
			return false;
		}
		if(mode[2] == '\0'){
			t->ppu_rom.transtype = TRANSTYPE_FULL;
			return true;
		}
		if(transtype_flash_set(mode[2], &t->ppu_rom) == false){
			return false;
		}
		return true;
	}
	return false;
}

static bool config_parse(const char *romimage, const char *device_cpu, const char *device_ppu, struct config_flash *c)
{
	c->target = romimage;
	if(nesfile_load(__FUNCTION__, romimage, &c->rom) == false){
		return false;
	}
	c->rom.cpu_rom.offset = 0;
	c->rom.ppu_rom.offset = 0;
	if(flash_device_get(device_cpu, &c->flash_cpu) == false){
		printf("unkown flash memory device %s\n", device_cpu);
		return false;
	}
	if(flash_device_get(device_ppu, &c->flash_ppu) == false){
		printf("unkown flash memory device %s\n", device_ppu);
		return false;
	}
	if(c->flash_cpu.id_device == FLASH_ID_DEVICE_DUMMY){
		c->rom.cpu_rom.transtype = TRANSTYPE_EMPTY;
	}else if(c->flash_cpu.capacity < c->rom.cpu_rom.size){
		puts("cpu area ROM image size is larger than target device");
		return false;
	}
	if(
		(c->flash_ppu.id_device == FLASH_ID_DEVICE_DUMMY) ||
		(c->rom.ppu_rom.size == 0)
	){
		c->rom.ppu_rom.transtype = TRANSTYPE_EMPTY;
	}else if(c->flash_ppu.capacity < c->rom.ppu_rom.size){
		puts("ppu area ROM image size is larger than target device");
		return false;
	}
	return true;
}

static void program(int c, char **v)
{
	struct config_flash config;
	config.rom.cpu_rom.data = NULL;
	config.rom.ppu_rom.data = NULL;
	config.script = v[2];
	config.reader = &DRIVER_KAZZO;
	config.compare = false;
	switch(v[1][0]){
	case 'a':
		config.reader = &DRIVER_DUMMY;
		break;
	case 'F':
		config.compare = true;
		break;
	}
	if(transtype_set(v[1], &config.rom) == false){
		puts("mode argument error");
		return;
	}
	switch(c){
	case 5: //mode script target cpu_flash_device
		if(config_parse(v[3], v[4], "dummy", &config) == false){
			nesbuffer_free(&config.rom, 0);
			return;
		}
		break;
	case 6: //mode script target cpu_flash_device ppu_flash_device
		if(config_parse(v[3], v[4], v[5], &config) == false){
			nesbuffer_free(&config.rom, 0);
			return;
		}
		break;
	default:
		puts("mode script target cpu_flash_device ppu_flash_device");
		return;
	}
	if(config.reader->open_or_close(READER_OPEN) == NG){
		puts("reader open error");
		nesbuffer_free(&config.rom, 0);
		return;
	}
	config.reader->init();
	script_flash_execute(&config);
	nesbuffer_free(&config.rom, 0);
	config.reader->open_or_close(READER_CLOSE);
}

static void dump(int c, char **v)
{
	struct config_dump config;
	if(c < 4){
		puts("argument error");
		return;
	}
	config.increase.cpu = 1;
	config.increase.ppu = 1;
	config.progress = true;
	switch(v[1][0]){
	case 'D':
		config.progress = false;
		break;
	}
	switch(v[1][1]){
	case '2':
		config.increase.cpu = 2;
		break;
	case '4':
		config.increase.cpu = 4;
		break;
	}
	if(v[1][1] != '\0'){
		switch(v[1][2]){
		case '2':
			config.increase.ppu = 2;
			break;
		case '4':
			config.increase.ppu = 4;
			break;
		}
	}
	config.script = v[2];
	config.target = v[3];
	config.reader = &DRIVER_KAZZO;
	config.mappernum = -1;
	if(c == 5){
		config.mappernum = atoi(v[4]);
	}
	if(config.reader->open_or_close(READER_OPEN) == NG){
		puts("reader open error");
		return;
	}
	config.reader->init();
	script_dump_execute(&config);
	config.reader->open_or_close(READER_CLOSE);
}

static void usage(const char *v)
{
	puts("famicom bus simluator 'anago'");
	printf("%s [mode] [script] [target] ....\n", v);
}

int main(int c, char **v)
{
	mm_init();
	setlocale(LC_ALL, "");
	if(c >= 2){
		switch(v[1][0]){
		case 'a': case 'f': case 'F':
			program(c, v);
			break;
		case 'd': case 'D':
			dump(c,v);
			break;
		default:
			usage(v[0]);
			puts("mode are a, d, D, f, g");
			break;
		}
	}else{
		usage(v[0]);
	}
	mm_end();
	return 0;
}
