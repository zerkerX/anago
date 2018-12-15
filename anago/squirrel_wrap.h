#ifndef _SQUIRREL_WRAP_H_
#define _SQUIRREL_WRAP_H_
HSQUIRRELVM qr_open(void);
void qr_function_register_global(HSQUIRRELVM v, const char *name, SQFUNCTION f);
SQRESULT qr_call(HSQUIRRELVM v, const SQChar *functionname, SQUserPointer up, bool settop, int argnum, ...);
void qr_close(HSQUIRRELVM v);
SQRESULT qr_argument_get(HSQUIRRELVM v, SQInteger num, ...);
SQRESULT qr_userpointer_get(HSQUIRRELVM v, SQUserPointer *up);
#endif
