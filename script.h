#ifndef _SCRIPT_H_
#define _SCRIPT_H_
struct st_config;
void script_load(const struct st_config *config);

struct st_variable{
	int type;
	char variable;
	long value;
};

struct st_expression{
	struct st_variable left, right;
	int operator;
};

struct script{
	int opcode;
	int line;
	long value[4];
	struct st_expression expression;
	char variable;
};

enum{
	VALUE_EXPRESSION = 0x1000000,
	VALUE_VARIABLE,
	VALUE_CONTANT_CPU_STEP_START,
	VALUE_CONTANT_CPU_STEP_END,
	VALUE_CONTANT_PPU_STEP_START,
	VALUE_CONTANT_PPU_STEP_END,
	VALUE_TRANSTYPE_EMPTY,
	VALUE_TRANSTYPE_TOP,
	VALUE_TRANSTYPE_BOTTOM,
	VALUE_TRANSTYPE_FULL,
	VALUE_UNDEF
};
enum{
	EXPRESSION_TYPE_VARIABLE,
	EXPRESSION_TYPE_VALUE
};

#endif
