#ifndef __ORB_STRINGS_H__
#define __ORB_STRINGS_H__

typedef enum exp_string_type{
	EXP_STRING_TYPE_NONE,
	EXP_STRING_TYPE_NUMBER,
	EXP_STRING_TYPE_STRING,
	EXP_STRING_TYPE_PARENTHESIS,
	EXP_STRING_TYPE_OPERTOR,
}exp_string_type_e;

typedef struct exp_string_t{
	exp_string_type_e type;
	char *str;
	struct exp_string_t *next;
}exp_string_t;

struct exp_string_t *exp_decode_string(char *str);
struct exp_string_t *exp_lines_decode(char *str);
void show_exp_string_t(struct exp_string_t *exp);
void exp_string_destroy(exp_string_t *exp_str);

#endif
