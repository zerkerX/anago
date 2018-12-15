LIBUSB_PATH = d:/dev/LibUSB-Win32
OBJ = \
	unagi.o header.o crc32.o \
	script_engine.o script_syntax.o \
	reader_master.o \
	reader_onajimi.o reader_hongkongfc.o reader_kazzo.o \
	flashmemory.o \
	file.o textutil.o giveio.o usb_device.o unagi.res.o
TARGET = unagi.exe
CC = gcc
CFLAGS += -Wall -Werror -Wmissing-declarations -I..
CFLAGS = -I$(LIBUSB_PATH)/include -I.. -I../../kazzo/firmware
LDFLAG = -L$(LIBUSB_PATH)/lib/gcc -lusb
