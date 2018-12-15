#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include "reader_master.h"
#include "reader_dummy.h"

static void dummy_init(void)
{
}
static int dummy_open_close(enum reader_control oc)
{
	return OK;
}
//---- cpu ----
static void dummy_cpu_read(long address, long length, uint8_t *data)
{
	printf("%s %06x %04x\n", __FUNCTION__, (int) address, (int) length);
	memset(data, 0x55, length);
}
static void dummy_cpu_write_6502(long address, long length, const uint8_t *data)
{
	printf("%s %04x %04x %02x\n", __FUNCTION__, (int) address, (int) length, (int) *data);
}
static void dummy_cpu_flash_config(long c000x, long c2aaa, long c5555, long unit)
{
	printf("%s %04x %04x %04x %04x\n", __FUNCTION__, (int) c000x, (int) c2aaa, (int) c5555, (int) unit);
}
static long dummy_cpu_flash_program(long address, long length, const u8 *data, bool dowait)
{
	int i = 0x10;
	printf("%s %06x\n", __FUNCTION__, (int) address);
	while(i != 0){
		printf("%02x ", *data);
		data++;
		i--;
	}
	printf("\n");
	return 0x100;
}

static void dummy_cpu_flash_erase(long address, bool dowait)
{
	printf("%s %04x\n", __FUNCTION__, (int) address);
}

//---- ppu ----
static void dummy_ppu_read(long address, long length, u8 *data)
{
	printf("%s %06x %04x\n", __FUNCTION__, (int) address, (int) length);
	memset(data, 0x55, length);
}
static void dummy_ppu_write(long address, long length, const uint8_t *data)
{
	printf("%s %04x %04x %02x\n", __FUNCTION__, (int) address, (int) length, (int) *data);
}
static void dummy_ppu_flash_config(long c000x, long c2aaa, long c5555, long unit)
{
	printf("%s %04x %04x %04x %04x\n", __FUNCTION__, (int) c000x, (int) c2aaa, (int) c5555, (int) unit);
}
static long dummy_ppu_flash_program(long address, long length, const u8 *data, bool dowait)
{
	int i = 0x10;
	printf("%s %06x\n", __FUNCTION__, (int) address);
	while(i != 0){
		printf("%02x ", *data);
		data++;
		i--;
	}
	printf("\n");
	return 0x100;
}

static void dummy_ppu_flash_erase(long address, bool dowait)
{
	printf("%s %04x\n", __FUNCTION__, (int) address);
}

static void dummy_flash_status(uint8_t s[2])
{
	s[0] = 0;
	s[1] = 0;
}
static void dummy_flash_device_get(uint8_t s[2])
{
	s[0] = 0x01;
	s[1] = 0xa4;
}
static uint8_t dummy_vram_connection(void)
{
	return 0x05;
}
const struct reader_driver DRIVER_DUMMY = {
	.name = "tester",
	.open_or_close = dummy_open_close,
	.init = dummy_init,
	.cpu_read = dummy_cpu_read, .ppu_read = dummy_ppu_read,
	.cpu_write_6502 = dummy_cpu_write_6502,
	.flash_support = true,
	.ppu_write = dummy_ppu_write,
	.cpu_flash_config = dummy_cpu_flash_config,
	.cpu_flash_erase = dummy_cpu_flash_erase,
	.cpu_flash_program = dummy_cpu_flash_program,
	.cpu_flash_device_get = dummy_flash_device_get,
	.ppu_flash_config = dummy_ppu_flash_config,
	.ppu_flash_erase = dummy_ppu_flash_erase,
	.ppu_flash_program = dummy_ppu_flash_program,
	.ppu_flash_device_get = dummy_flash_device_get,
	.flash_status = dummy_flash_status,
	.vram_connection = dummy_vram_connection
};
