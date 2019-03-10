#ifndef _SCRIPT_COMMON_H_
#define _SCRIPT_COMMON_H_
struct range{
	long start, end;
};
SQInteger script_nop(HSQUIRRELVM v);
SQInteger range_check(HSQUIRRELVM v, const char *name, long target, const struct range *range);
SQInteger cpu_write_check(HSQUIRRELVM v);
SQInteger script_require(HSQUIRRELVM v);
#endif
