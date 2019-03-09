#include <assert.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdbool.h>
#include <squirrel.h>
#include <sqstdio.h>
#include <sqstdaux.h>

#ifdef SQUNICODE 
#define scvprintf vwprintf 
#else 
#define scvprintf vprintf 
#endif 
static void printfunc(HSQUIRRELVM v, const SQChar *s, ...) 
{
	va_list arglist;
	va_start(arglist, s);
	scvprintf(s, arglist);
	va_end(arglist);
}

HSQUIRRELVM qr_open(void)
{
	HSQUIRRELVM v = sq_open(0x400);
	sqstd_seterrorhandlers(v);
	sqstd_register_iolib(v);
	sq_setprintfunc(v, printfunc, printfunc);
	sq_pushroottable(v);
	return v;
}

//SQInteger 
void qr_function_register_global(HSQUIRRELVM v, const char *name, SQFUNCTION f)
{
	sq_pushroottable(v);
	sq_pushstring(v, name, -1);
	sq_newclosure(v, f, 0);
	sq_createslot(v, -3); 
	sq_pop(v, 1);
}

SQRESULT qr_call(HSQUIRRELVM v, const SQChar *functionname, SQUserPointer up, bool settop, int argnum, ...)
{
	SQRESULT r = SQ_ERROR;
	SQInteger top = sq_gettop(v);
	sq_pushroottable(v);
	sq_pushstring(v, _SC(functionname), -1);
	if(SQ_SUCCEEDED(sq_get(v,-2))){
		int i;
		va_list ap;
		va_start(ap, argnum);
		sq_pushroottable(v);
		sq_pushuserpointer(v, up);
		for(i = 0; i < argnum; i++){
			sq_pushinteger(v, va_arg(ap, long));
		}
		r = sq_call(v, 2 + argnum, SQFalse, SQTrue); //calls the function 
	}
	if(settop == true){
		sq_settop(v, top); //restores the original stack size
	}
	return r;
}

void qr_close(HSQUIRRELVM v)
{
	sq_pop(v, 1);
	sq_close(v); 
}

static bool long_get(HSQUIRRELVM v, SQInteger index, long *d)
{
	if(sq_gettype(v, index) != OT_INTEGER){
		return false;
	}
	SQInteger i;
	if(SQ_FAILED(sq_getinteger(v, index, &i))){
		return false;
	}
	*d = (long) i;
	return true;
}

SQRESULT qr_argument_get(HSQUIRRELVM v, SQInteger num, ...)
{
	va_list ap;
	if(sq_gettop(v) != (num + 2)){ //roottable, up, arguments...
		return sq_throwerror(v, "argument number error");
	}
	va_start(ap, num);
	SQInteger i;
	for(i = 0; i < num; i++){
		if(long_get(v, i + 3, va_arg(ap, long *)) == false){
			return sq_throwerror(v, "argument type error");
		}
	}
	return SQ_OK;
}

SQRESULT qr_userpointer_get(HSQUIRRELVM v, SQUserPointer *up)
{
	SQRESULT r;
	assert(sq_gettype(v, 2) == OT_USERPOINTER);
	r = sq_getuserpointer(v, 2, up);
	if(SQ_FAILED(r)){
		return sq_throwerror(v, "1st argument must be d (userpointer)");
	}
	return r;
}
