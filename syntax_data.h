//included from syntax.c only
enum syntax_argvtype{
	SYNTAX_ARGVTYPE_NULL,
	SYNTAX_ARGVTYPE_VALUE,
	SYNTAX_ARGVTYPE_HV,
	SYNTAX_ARGVTYPE_EXPRESSION,
	SYNTAX_ARGVTYPE_VARIABLE,
	SYNTAX_ARGVTYPE_CONSTANT,
	SYNTAX_ARGVTYPE_TRANSTYPE
};
enum syntax_compare{
	SYNTAX_COMPARE_EQ,
	SYNTAX_COMPARE_GT
};
enum{
	SYNTAX_ARGV_TYPE_NUM = 4
};
enum syntax_permittion{
	PERMITTION_ROM_DUMP = 1 << MODE_ROM_DUMP,
	PERMITTION_RAM_READ = 1 << MODE_RAM_READ,
	PERMITTION_RAM_WRITE = 1 << MODE_RAM_WRITE,
	PERMITTION_ROM_PROGRAM = 1 << MODE_ROM_PROGRAM,
	PERMITTION_ALL = 0xffff
};
struct script_syntax{
	const char *name;
	enum script_opcode script_opcode;
	enum syntax_permittion permittion;
	int argc;
	enum syntax_compare compare;
	const enum syntax_argvtype *argv_type;
};
//これらの文字列は script_engine.c でも使用する
const char OPSTR_CPU_ROMSIZE[] = "CPU_ROMSIZE";
const char OPSTR_CPU_RAMSIZE[] = "CPU_RAMSIZE";
const char OPSTR_CPU_FLASHSIZE[] = "CPU_FLASHSIZE";
const char OPSTR_PPU_ROMSIZE[] = "PPU_ROMSIZE";
const char OPSTR_PPU_FLASHSIZE[] = "PPU_FLASHSIZE";
const char OPSTR_CPU_RAMRW[] = "CPU_RAMRW";

static const enum syntax_argvtype 
ARGV_TYPE_VALUE_ONLY[SYNTAX_ARGV_TYPE_NUM] = {
	SYNTAX_ARGVTYPE_VALUE, SYNTAX_ARGVTYPE_NULL,
	SYNTAX_ARGVTYPE_NULL, SYNTAX_ARGVTYPE_NULL
};
static const enum syntax_argvtype 
ARGV_TYPE_HV[SYNTAX_ARGV_TYPE_NUM] = {
	SYNTAX_ARGVTYPE_HV, SYNTAX_ARGVTYPE_NULL,
	SYNTAX_ARGVTYPE_NULL, SYNTAX_ARGVTYPE_NULL
};
static const enum syntax_argvtype 
ARGV_TYPE_NULL[SYNTAX_ARGV_TYPE_NUM] = {
	SYNTAX_ARGVTYPE_NULL, SYNTAX_ARGVTYPE_NULL,
	SYNTAX_ARGVTYPE_NULL, SYNTAX_ARGVTYPE_NULL
};
static const enum syntax_argvtype 
ARGV_TYPE_ADDRESS_EXPRESSION[SYNTAX_ARGV_TYPE_NUM] = {
	SYNTAX_ARGVTYPE_VALUE,
	SYNTAX_ARGVTYPE_EXPRESSION, SYNTAX_ARGVTYPE_EXPRESSION, SYNTAX_ARGVTYPE_EXPRESSION
};
static const enum syntax_argvtype 
ARGV_TYPE_ADDRESS_LENGTH[SYNTAX_ARGV_TYPE_NUM] = {
	SYNTAX_ARGVTYPE_VALUE, SYNTAX_ARGVTYPE_VALUE,
	SYNTAX_ARGVTYPE_NULL, SYNTAX_ARGVTYPE_NULL
};
static const enum syntax_argvtype 
ARGV_TYPE_STEP_START[SYNTAX_ARGV_TYPE_NUM] = {
	SYNTAX_ARGVTYPE_VARIABLE, SYNTAX_ARGVTYPE_CONSTANT,
	SYNTAX_ARGVTYPE_CONSTANT, SYNTAX_ARGVTYPE_VALUE
};
static const enum syntax_argvtype 
ARGV_TYPE_ADDRESS_COMMAND[SYNTAX_ARGV_TYPE_NUM] = {
	SYNTAX_ARGVTYPE_VALUE, SYNTAX_ARGVTYPE_VALUE,
	SYNTAX_ARGVTYPE_VALUE, SYNTAX_ARGVTYPE_NULL
};
static const enum syntax_argvtype 
ARGV_TYPE_FLASHSIZE[SYNTAX_ARGV_TYPE_NUM] = {
	SYNTAX_ARGVTYPE_VALUE, SYNTAX_ARGVTYPE_TRANSTYPE,
	SYNTAX_ARGVTYPE_VALUE, SYNTAX_ARGVTYPE_VALUE
};
static const struct script_syntax SCRIPT_SYNTAX[] = {
	{
		name: "MAPPER",
		script_opcode: SCRIPT_OPCODE_MAPPER,
		permittion: PERMITTION_ROM_DUMP | PERMITTION_ROM_PROGRAM,
		argc: 1, compare: SYNTAX_COMPARE_EQ,
		argv_type: ARGV_TYPE_VALUE_ONLY
	},{
		name: "MIRROR",
		script_opcode: SCRIPT_OPCODE_MIRROR,
		permittion: PERMITTION_ROM_DUMP,
		argc: 1, compare: SYNTAX_COMPARE_EQ,
		argv_type: ARGV_TYPE_HV
	},{
		name: OPSTR_CPU_ROMSIZE,
		script_opcode: SCRIPT_OPCODE_CPU_ROMSIZE,
		permittion: PERMITTION_ROM_DUMP,
		argc: 1, compare: SYNTAX_COMPARE_EQ,
		argv_type: ARGV_TYPE_VALUE_ONLY
	},{
		name: OPSTR_CPU_RAMSIZE,
		script_opcode: SCRIPT_OPCODE_CPU_RAMSIZE,
		permittion: PERMITTION_RAM_READ | PERMITTION_RAM_WRITE,
		argc: 1, compare: SYNTAX_COMPARE_EQ,
		argv_type: ARGV_TYPE_VALUE_ONLY
	},{
		name: OPSTR_CPU_FLASHSIZE,
		script_opcode: SCRIPT_OPCODE_CPU_FLASHSIZE,
		permittion: PERMITTION_ROM_PROGRAM,
		argc: 4, compare: SYNTAX_COMPARE_EQ,
		argv_type: ARGV_TYPE_FLASHSIZE
	},{
		name: OPSTR_PPU_ROMSIZE,
		script_opcode: SCRIPT_OPCODE_PPU_ROMSIZE,
		permittion: PERMITTION_ROM_DUMP,
		argc: 1, compare: SYNTAX_COMPARE_EQ,
		argv_type: ARGV_TYPE_VALUE_ONLY
	},{
		name: OPSTR_PPU_FLASHSIZE,
		script_opcode: SCRIPT_OPCODE_PPU_FLASHSIZE,
		permittion: PERMITTION_ROM_PROGRAM,
		argc: 4, compare: SYNTAX_COMPARE_EQ,
		argv_type: ARGV_TYPE_FLASHSIZE
	},{
		name: "CPU_COMMAND",
		script_opcode: SCRIPT_OPCODE_CPU_COMMAND,
		permittion: PERMITTION_ROM_PROGRAM,
		argc:3, compare: SYNTAX_COMPARE_EQ,
		argv_type: ARGV_TYPE_ADDRESS_COMMAND
	},{
		name: "PPU_COMMAND",
		script_opcode: SCRIPT_OPCODE_PPU_COMMAND,
		permittion: PERMITTION_ROM_PROGRAM,
		argc:3, compare: SYNTAX_COMPARE_EQ,
		argv_type: ARGV_TYPE_ADDRESS_COMMAND
	},{
		name: "DUMP_START",
		script_opcode: SCRIPT_OPCODE_DUMP_START,
		permittion: PERMITTION_ALL,
		argc: 0, compare: SYNTAX_COMPARE_EQ,
		argv_type: ARGV_TYPE_NULL
	},{
		name: "CPU_READ",
		script_opcode: SCRIPT_OPCODE_CPU_READ,
		permittion: PERMITTION_ROM_DUMP,
		argc: 2, compare: SYNTAX_COMPARE_EQ,
		argv_type: ARGV_TYPE_ADDRESS_LENGTH
	},{
		name: "CPU_WRITE",
		script_opcode: SCRIPT_OPCODE_CPU_WRITE,
		permittion: PERMITTION_ALL,
		argc: 2, compare: SYNTAX_COMPARE_GT,
		argv_type: ARGV_TYPE_ADDRESS_EXPRESSION
	},{
		name: OPSTR_CPU_RAMRW,
		script_opcode: SCRIPT_OPCODE_CPU_RAMRW,
		permittion: PERMITTION_RAM_READ | PERMITTION_RAM_WRITE,
		argc: 2, compare: SYNTAX_COMPARE_EQ,
		argv_type: ARGV_TYPE_ADDRESS_LENGTH
	},{
		name: "CPU_PROGRAM",
		script_opcode: SCRIPT_OPCODE_CPU_PROGRAM,
		permittion: PERMITTION_ROM_PROGRAM,
		argc: 2, compare: SYNTAX_COMPARE_EQ,
		argv_type: ARGV_TYPE_ADDRESS_LENGTH
	},{
		name: "PPU_RAMFIND",
		script_opcode: SCRIPT_OPCODE_PPU_RAMFIND,
		permittion: PERMITTION_ROM_DUMP,
		argc: 0, compare: SYNTAX_COMPARE_EQ,
		argv_type: ARGV_TYPE_NULL
	},{
		name: "PPU_SRAMTEST",
		script_opcode: SCRIPT_OPCODE_PPU_SRAMTEST,
		permittion: PERMITTION_ROM_DUMP,
		argc: 2, compare: SYNTAX_COMPARE_EQ,
		argv_type: ARGV_TYPE_ADDRESS_LENGTH
	},{
		name: "PPU_READ",
		script_opcode: SCRIPT_OPCODE_PPU_READ,
		permittion: PERMITTION_ROM_DUMP,
		argc: 2, compare: SYNTAX_COMPARE_EQ,
		argv_type: ARGV_TYPE_ADDRESS_LENGTH
	},
#if DEBUG==1
	{
		name: "PPU_WRITE",
		script_opcode: SCRIPT_OPCODE_PPU_WRITE,
		permittion: PERMITTION_ROM_DUMP | PERMITTION_ROM_PROGRAM,
		argc: 2, compare: SYNTAX_COMPARE_GT,
		argv_type: ARGV_TYPE_ADDRESS_EXPRESSION
	},
#endif
	{
		name: "PPU_PROGRAM",
		script_opcode: SCRIPT_OPCODE_PPU_PROGRAM,
		permittion: PERMITTION_ROM_PROGRAM,
		argc: 2, compare: SYNTAX_COMPARE_EQ,
		argv_type: ARGV_TYPE_ADDRESS_LENGTH,
	},
	{
		name: "STEP_START",
		script_opcode: SCRIPT_OPCODE_STEP_START,
		permittion: PERMITTION_ALL,
		argc: 4, compare: SYNTAX_COMPARE_EQ,
		argv_type: ARGV_TYPE_STEP_START
	},{
		name: "STEP_END",
		script_opcode: SCRIPT_OPCODE_STEP_END,
		permittion: PERMITTION_ALL,
		argc: 0, compare: SYNTAX_COMPARE_EQ,
		argv_type: ARGV_TYPE_NULL
	},{
		name: "DUMP_END",
		script_opcode: SCRIPT_OPCODE_DUMP_END,
		permittion: PERMITTION_ALL,
		argc: 0, compare: SYNTAX_COMPARE_EQ,
		argv_type: ARGV_TYPE_NULL
	}
};

static const char *STR_TRANSTYPE[] = {
	"EMPTY", "TOP", "BOTTOM", "FULL", NULL
};

static const char *STR_CONSTANTNAME[] = {
	"C_START", "C_END",
	"P_START", "P_END", NULL
};
