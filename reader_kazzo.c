#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <usb.h>
#include <kazzo_request.h>
#include <kazzo_task.h>
#include "memory_manage.h"
#include "reader_master.h"
#include "usb_device.h"
#include "reader_kazzo.h"

static usb_dev_handle *device_open(void)
{
	usb_dev_handle *handle = NULL;
	const unsigned char rawVid[2] = {USB_CFG_VENDOR_ID};
	const unsigned char rawPid[2] = {USB_CFG_DEVICE_ID};
	char vendor[] = {USB_CFG_VENDOR_NAME, 0};
	char product[] = {USB_CFG_DEVICE_NAME, 0};
	int vid, pid;

	/* compute VID/PID from usbconfig.h so that there is a central source of information */
	vid = (rawVid[1] << 8) | rawVid[0];
	pid = (rawPid[1] << 8) | rawPid[0];

	if(usbOpenDevice(&handle, vid, vendor, pid, product, NULL, NULL, NULL) == 0){
		return handle;
	}
	fprintf(stderr, "Could not find USB device \"%s\" with vid=0x%x pid=0x%x\n", product, vid, pid);
	return NULL;
}

static usb_dev_handle *handle = NULL;
static int kazzo_open_close(enum reader_control oc)
{
	switch(oc){
	case READER_OPEN:
		handle = device_open();
		return handle == NULL ? NG : OK;
	case READER_CLOSE:
		usb_close(handle);
		handle = NULL;
		return OK;
	}
	return NG;
}
enum{
	TIMEOUT = 4000
};
//-------- read sequence --------
static void device_read(usb_dev_handle *handle, enum request r, enum index index, 
    long address, long length, uint8_t *data)
{
	int cnt = usb_control_msg(
		handle, 
		USB_TYPE_VENDOR | USB_RECIP_DEVICE | USB_ENDPOINT_IN, 
		r, address, 
		index, (char *)data, length, TIMEOUT
	);
	if(cnt != length){
		puts(__FUNCTION__);
		puts(usb_strerror());
		exit(1);
	}
}

static void read_main(const enum request r, enum index index, long address, long length, uint8_t *data)
{
	const int packet = READ_PACKET_SIZE;
	while(length >= packet){
		device_read(handle, r, index, address, packet, data);
		data += packet;
		address += packet;
		length -= packet;
	}
	if(length != 0){
		device_read(handle, r, index, address, length, data);
	}
}

static void kazzo_cpu_read(long address, long length, uint8_t *data)
{
	read_main(REQUEST_CPU_READ, INDEX_IMPLIED, address, length, data);
//	read_main(REQUEST_CPU_READ_6502, address, length, data);
}

static void kazzo_ppu_read(long address, long length, uint8_t *data)
{
	read_main(REQUEST_PPU_READ, INDEX_IMPLIED, address, length, data);
}

//-------- write sequence --------
/*
When host send data that contains 0xff many times, v-usb loses some 
bits. To prevent losting bits, mask data xor 0xa5;
*/
static void device_write(usb_dev_handle *handle, enum request w, 
    enum index index, long address, long length, const uint8_t *data)
{
	uint8_t *d = malloc(length);
	int i;
	memcpy(d, data, length);
	for(i = 0; i < length; i++){
		d[i] ^= 0xa5;
	}
	int cnt = usb_control_msg(
		handle, 
		USB_TYPE_VENDOR | USB_RECIP_DEVICE | USB_ENDPOINT_OUT,
		w, address, 
		index, (char *)d, length, TIMEOUT
	);
	if(cnt != length){
		puts(__FUNCTION__);
		puts(usb_strerror());
		exit(1);
	}
	Free(d);
}

static void kazzo_init(void)
{
	device_write(handle, REQUEST_PHI2_INIT, INDEX_IMPLIED, 0, 0, NULL);
}

static void write_memory(enum request r, long address, long length, const uint8_t *data)
{
	while(length != 0){
		long l = length >= FLASH_PACKET_SIZE ? FLASH_PACKET_SIZE : length;
		device_write(handle, r, INDEX_IMPLIED, address, l, data);
		address += l;
		data += l;
		length -= l;
	}
}
static void kazzo_cpu_write_6502(long address, long length, const uint8_t *data)
{
	write_memory(REQUEST_CPU_WRITE_6502, address, length, data);
}
/*static void kazzo_cpu_write_flash(long address, long data)
{
	uint8_t d = (uint8_t) (data & 0xff);
	device_write(handle, REQUEST_CPU_WRITE_FLASH, address, 1, &d);
}*/
static void kazzo_ppu_write(long address, long length, const uint8_t *data)
{
	write_memory(REQUEST_PPU_WRITE, address, length, data);
}

static inline void pack_short_le(long l, uint8_t *t)
{
	t[0] = l & 0xff;
	t[1] = (l >> 8) & 0xff;
}
static void flash_config(enum request r, enum index index, long c000x, long c2aaa, long c5555, long unit)
{
	const int size = 2 * 4;
	uint8_t t[size];
	assert(unit >= 1 && unit < 0x400);
	pack_short_le(c000x, t);
	pack_short_le(c2aaa, t + 2);
	pack_short_le(c5555, t + 4);
	pack_short_le(unit, t + 6);
	device_write(handle, r, index, 0, size, t);
}
static void kazzo_cpu_flash_config(long c000x, long c2aaa, long c5555, long unit)
{
	flash_config(REQUEST_FLASH_CONFIG_SET, INDEX_CPU, c000x, c2aaa, c5555, unit);
}
static void kazzo_ppu_flash_config(long c000x, long c2aaa, long c5555, long unit)
{
	flash_config(REQUEST_FLASH_CONFIG_SET, INDEX_PPU, c000x, c2aaa, c5555, unit);
}

static inline void flash_execute(enum request p, enum request s, enum index index, long address, const uint8_t *data, int size, bool dowait)
{
	uint8_t status;
	device_write(handle, p, index, address, size, data);
	if(dowait == true){
		do{
			wait(10);
			device_read(handle, s, index, 0, 1, &status);
		}while(status != KAZZO_TASK_FLASH_IDLE);
	}
}
static void kazzo_cpu_flash_erase(long address, bool dowait)
{
	flash_execute(REQUEST_FLASH_ERASE, REQUEST_FLASH_STATUS, INDEX_CPU, address, NULL, 0, dowait);
}
static void kazzo_ppu_flash_erase(long address, bool dowait)
{
	flash_execute(REQUEST_FLASH_ERASE, REQUEST_FLASH_STATUS, INDEX_PPU, address, NULL, 0, dowait);
}

static void dump(const uint8_t *w, const uint8_t *r, long length)
{
	while(length != 0){
		if(memcmp(r, w, 0x10) != 0){
			int i;
			printf("* ");
			for(i = 0; i < 0x10; i+=4){
				printf("%02x %02x %02x %02x-", w[i], w[i+1], w[i+2], w[i+3]);
			}
			puts("");
			printf("  ");
			for(i = 0; i < 0x10; i+=4){
				printf("%02x %02x %02x %02x-", r[i], r[i+1], r[i+2], r[i+3]);
			}
			puts("");
		}
		w += 0x10;
		r += 0x10;
		length -= 0x10;
	}
}
static long flash_program(enum index index, long address, long length, const uint8_t *data, bool dowait)
{
	enum request p = REQUEST_FLASH_PROGRAM;
	enum request s = REQUEST_FLASH_STATUS;
	if(dowait == false){
		flash_execute(p, s, index, address, data, FLASH_PACKET_SIZE, dowait);
		return FLASH_PACKET_SIZE;
	}
	long count = 0;
	uint8_t *d = Malloc(FLASH_PACKET_SIZE);
	while(length >= FLASH_PACKET_SIZE){
		flash_execute(p, s, index, address, data, FLASH_PACKET_SIZE, dowait);
		if(0){
			//device_read(handle, REQUEST_FLASH_BUFFER_GET, index, 0, FLASH_PACKET_SIZE, d);
			if(memcmp(d, data, FLASH_PACKET_SIZE) != 0){
				puts("packet send error");
				dump(data, d, FLASH_PACKET_SIZE);
			}
		}
		address += FLASH_PACKET_SIZE;
		data += FLASH_PACKET_SIZE;
		count += FLASH_PACKET_SIZE;
		length -= FLASH_PACKET_SIZE;
	}
	Free(d);
	return count;
}
static long kazzo_cpu_flash_program(long address, long length, const uint8_t *data, bool dowait)
{
	return flash_program(INDEX_CPU, address, length, data, dowait);
}
static long kazzo_ppu_flash_program(long address, long length, const uint8_t *data, bool dowait)
{
	return flash_program(INDEX_PPU, address, length, data, dowait);
}

static void kazzo_flash_status(uint8_t s[2])
{
	read_main(REQUEST_FLASH_STATUS, INDEX_BOTH, 0, 2, s);
}
static void kazzo_cpu_flash_device_get(uint8_t s[2])
{
	read_main(REQUEST_FLASH_DEVICE, INDEX_CPU, 0, 2, s);
}
static void kazzo_ppu_flash_device_get(uint8_t s[2])
{
	read_main(REQUEST_FLASH_DEVICE, INDEX_PPU, 0, 2, s);
}
static uint8_t kazzo_vram_connection(void)
{
	uint8_t s;
	read_main(REQUEST_VRAM_CONNECTION, INDEX_IMPLIED, 0, 1, &s);
	return s;
}
const struct reader_driver DRIVER_KAZZO = {
	.name = "kazzo",
	.open_or_close = kazzo_open_close,
	.init = kazzo_init,
	.cpu_read = kazzo_cpu_read, .ppu_read = kazzo_ppu_read,
	.cpu_write_6502 = kazzo_cpu_write_6502,
	.flash_support = true,
	.ppu_write = kazzo_ppu_write,
	.cpu_flash_config = kazzo_cpu_flash_config,
	.cpu_flash_erase = kazzo_cpu_flash_erase,
	.cpu_flash_program = kazzo_cpu_flash_program,
	.cpu_flash_device_get = kazzo_cpu_flash_device_get,
	.ppu_flash_config = kazzo_ppu_flash_config,
	.ppu_flash_erase = kazzo_ppu_flash_erase,
	.ppu_flash_program = kazzo_ppu_flash_program,
	.ppu_flash_device_get = kazzo_ppu_flash_device_get,
	.flash_status = kazzo_flash_status,
	.vram_connection = kazzo_vram_connection
};
