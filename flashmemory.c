/*
famicom ROM cartridge utility - unagi
flash memory driver

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

flashmemory.c だけの警告
このソースコードを参考、転用してシェアウェアなどで利益を得ないこと。
判明した場合は LGPL が適用され、該当箇所のソースを公開する必要がある。
*/
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>
#include "type.h"
#include "header.h"
#include "flashmemory.h"

/*
---- erase ----
*/
#if 0 //DEBUG==1
static void sram_erase(const struct flash_order *d)
{
	//bank 切り替えが伴うので実装できない
}
#endif

static void init_nop(const struct flash_order *d, long wait)
{
}

static void init_erase(const struct flash_order *d, long wait)
{
	assert(d->pagesize > 0);
	d->erase(d->command_2aaa, true);
	Sleep(wait);
}

static void program_dummy(const struct flash_order *d, long address, long length, const struct memory *m)
{
}
static void program_sram(const struct flash_order *d, long address, long length, const struct memory *m)
{
	d->write(address, length, m->data);
}
static void program_flash(const struct flash_order *d, long address, long length, const struct memory *m)
{
	d->program(address, length, m->data, true);
}
/*
デバイスリスト
*/
enum{MEGA = 0x20000};
const struct flash_driver FLASH_DRIVER_UNDEF = {
	.name = "undefined",
	.capacity = 0,
	.pagesize = 1,
	.erase_wait = 0,
	.command_mask = 0,
	.id_manufacurer = 0,
	.id_device = 0,
	.productid_check = NULL,
	.init = NULL,
	.program = NULL
};
static const struct flash_driver DRIVER_SRAM = {
	.name = "SRAM",
	.capacity = 4 * MEGA,
	.pagesize = 1,
	.erase_wait = 0,
	.command_mask = 0,
	.id_manufacurer = FLASH_ID_DEVICE_SRAM,
	.id_device = FLASH_ID_DEVICE_SRAM,
//	.productid_check = productid_sram,
	.init = init_nop,
	.program = program_sram
};

static const struct flash_driver DRIVER_DUMMY = {
	.name = "dummy",
	.capacity = 16 * MEGA,
	.pagesize = 1,
	.erase_wait = 0,
	.command_mask = 0,
	.id_manufacurer = FLASH_ID_DEVICE_DUMMY,
	.id_device = FLASH_ID_DEVICE_DUMMY,
//	.productid_check = productid_sram,
	.init = init_nop,
	.program = program_dummy
};

static const struct flash_driver DRIVER_W29C020 = {
	.name = "W29C020",
	.capacity = 2 * MEGA,
	.pagesize = 0x80,
	.erase_wait = 50,
	.command_mask = 0x7fff,
	.id_manufacurer = 0xda,
	.id_device = 0x45,
//	.productid_check = productid_check,
	.init = init_nop,
	.program = program_flash
};

static const struct flash_driver DRIVER_W29C040 = {
	.name = "W29C040",
	.capacity = 4 * MEGA,
	.pagesize = 0x100,
	.erase_wait = 50,
	.command_mask = 0x7fff,
	.id_manufacurer = 0xda,
	.id_device = 0x46,
//	.productid_check = productid_check,
	.init = init_nop,
	.program = program_flash
};

static const struct flash_driver DRIVER_W49F002 = {
	.name = "W49F002",
	.capacity = 2 * MEGA,
	.pagesize = 1,
	.erase_wait = 100, //typ 0.1, max 0.2 sec
	.command_mask = 0x7fff,
	.id_manufacurer = 0xda,
	.id_device = 0xae,
//	.productid_check = productid_check,
	.init = init_erase,
	.program = program_flash
};

/*
MANUFATUTER ID 0x7f1c
EN29F002T DEVICE ID 0x7f92
EN29F002B DEVICE ID 0x7f97

command address が 0x00555, 0x00aaa になってる
*/
static const struct flash_driver DRIVER_EN29F002T = {
	.name = "EN29F002T",
	.capacity = 2 * MEGA,
	.pagesize = 1,
	.erase_wait = 2000, //typ 2, max 5 sec
	.command_mask = 0x07ff,
	.id_manufacurer = 0x1c,
	.id_device = 0x92,
//	.productid_check = productid_check,
	.init = init_erase,
	.program = program_flash
};

static const struct flash_driver DRIVER_AM29F040B = {
	.name = "AM29F040B",
	.capacity = 4 * MEGA,
	.pagesize = 1,
	.erase_wait = 8000, //typ 8, max 64 sec
	.command_mask = 0x07ff,
	.id_manufacurer = 0x01,
	.id_device = 0xa4,
//	.productid_check = productid_check,
	.init = init_erase,
	.program = program_flash
};

static const struct flash_driver DRIVER_MBM29F080A = {
	.name = "MBM29F080A",
	.capacity = 8 * MEGA,
	.pagesize = 1,
	.erase_wait = 8000, //chip erase time is not written in datasheet!!
	.command_mask = 0x07ff,
	.id_manufacurer = 0x04,
	.id_device = 0xd5,
//	.productid_check = productid_check,
	.init = init_erase,
	.program = program_flash
};

static const struct flash_driver *DRIVER_LIST[] = {
	&DRIVER_W29C020, &DRIVER_W29C040, 
	&DRIVER_W49F002, &DRIVER_EN29F002T, &DRIVER_AM29F040B, &DRIVER_MBM29F080A,
	&DRIVER_SRAM, 
	&DRIVER_DUMMY,
	NULL
};

const struct flash_driver *flash_driver_get(const char *name)
{
	const struct flash_driver **d;
	d = DRIVER_LIST;
	while(*d != NULL){
		if(strcmp(name, (*d)->name) == 0){
			return *d;
		}
		d++;
	}
	return NULL;
}
