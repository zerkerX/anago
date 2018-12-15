#ifndef _CONFIG_H_
#define _CONFIG_H_
#include "reader_master.h"
enum{
	CONFIG_OVERRIDE_UNDEF = 4649,
	CONFIG_OVERRIDE_TRUE = 1,
	CONFIG_STR_LENGTH = 20
};
struct st_config{
	//override config
	long mapper;
	int mirror, backupram;
	//target filename
	const char *ramimage, *romimage;
	const char *script;
	//device driver function pointer struct
	const struct reader_driver *reader;
	const struct flash_driver *cpu_flash_driver, *ppu_flash_driver;
	//data mode
	int mode, syntaxtest;
	long transtype_cpu, transtype_ppu;
	//debug member
	long write_wait;
	char flash_test_device[CONFIG_STR_LENGTH];
	char flash_test_mapper[CONFIG_STR_LENGTH];
};

enum{
	MODE_TEST,
	MODE_ROM_DUMP,
	MODE_RAM_READ,
	MODE_RAM_WRITE,
	MODE_ROM_PROGRAM
};

#endif
