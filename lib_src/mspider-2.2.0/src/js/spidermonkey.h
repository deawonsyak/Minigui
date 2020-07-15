#ifndef	_SPIDERMONKERY_H_ 
#define _SPIDERMONKERY_H_

#define XP_UNIX
#include <js/jsapi.h>
#include "js.h"
#include "mgdconfig.h"

#ifdef JS_SUPPORT

/* Common data structure and function */
enum prop_type {
	JSPT_UNDEF,
	JSPT_INT,
	JSPT_DOUBLE,
	JSPT_STRING,
	JSPT_ASTRING,
	JSPT_BOOLEAN,
	JSPT_OBJECT,
};

struct jsval_property {
	enum prop_type type;
	union {
		int boolean;
		int number;
		jsdouble floatnum;
		JSObject *object;
		unsigned char *string;
	} value;
};

union jsval_union {
	jsint boolean;
	jsdouble *number;
	unsigned char *string;
};

void	set_prop_undef(struct jsval_property *prop);
void 	set_prop_object(struct jsval_property *prop, JSObject *object);
void 	set_prop_boolean(struct jsval_property *prop, int boolean);
void 	set_prop_string(struct jsval_property *prop, unsigned char *string);
void 	set_prop_astring(struct jsval_property *prop, unsigned char *string);
void 	set_prop_int(struct jsval_property *prop, int number);
void 	set_prop_double(struct jsval_property *prop, jsdouble floatnum);

void 	value_to_jsval(JSContext *ctx, jsval *vp, struct jsval_property *prop);
void 	jsval_to_value(JSContext *ctx, jsval *vp, JSType type, union jsval_union *var); 

/* export */
void 	spidermonkey_init(void);
void 	spidermonkey_done(void);

void*	spidermonkey_get_interpreter(jsscript_interpreter *interpreter, void *wjsobj);
void 	spidermonkey_put_interpreter(jsscript_interpreter *interpreter, int mod);

int 	spidermonkey_eval(jsscript_interpreter *interpreter, jsstring *code);

#endif

#endif /* _SPIDERMONKERY_H_ */
