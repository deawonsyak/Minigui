#include <minigui/common.h>
#include <minigui/minigui.h>
#include <minigui/gdi.h>
#include <minigui/window.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "html.h"
#include "spidermonkey.h"

#ifdef JS_SUPPORT
/* extern global data */
/* window */
extern const JSClass window_class;
extern const JSPropertySpec window_props[];
extern const JSFunctionSpec window_funcs[];
/* document */
extern const JSClass document_class;
extern const JSPropertySpec document_props[];
extern const JSFunctionSpec document_funcs[];
/* history */
extern const JSClass history_class;
extern const JSPropertySpec history_props[];
extern const JSFunctionSpec history_funcs[];
/* location */
extern const JSClass location_class;
extern const JSPropertySpec location_props[];
extern const JSFunctionSpec location_funcs[];
/* navigator */
extern const JSClass navigator_class;
extern const JSPropertySpec navigator_props[];
extern const JSFunctionSpec navigator_funcs[];
/* frames */
extern const JSClass frames_class;
extern const JSPropertySpec frames_props[];
extern const JSFunctionSpec frames_funcs[];
/* forms */
extern const JSClass forms_class;
extern const JSPropertySpec forms_props[];
extern const JSFunctionSpec forms_funcs[];
/* images */
extern const JSClass images_class;
extern const JSPropertySpec images_props[];
extern const JSFunctionSpec images_funcs[];

/* common function */
void set_prop_undef(struct jsval_property *prop)
{
	memset(prop, 'J', sizeof(struct jsval_property)); /* Active security ;) */
	prop->type = JSPT_UNDEF;
}

void set_prop_object(struct jsval_property *prop, JSObject *object)
{
	prop->value.object = object;
	prop->type = JSPT_OBJECT;
}

void set_prop_boolean(struct jsval_property *prop, int boolean)
{
	prop->value.boolean = boolean;
	prop->type = JSPT_BOOLEAN;
}

void set_prop_string(struct jsval_property *prop, unsigned char *string)
{
	prop->value.string = string;
	prop->type = JSPT_STRING;
}

void set_prop_astring(struct jsval_property *prop, unsigned char *string)
{
	prop->value.string = string;
	prop->type = JSPT_ASTRING;
}

void set_prop_int(struct jsval_property *prop, int number)
{
	prop->value.number = number;
	prop->type = JSPT_INT;
}

void set_prop_double(struct jsval_property *prop, jsdouble floatnum)
{
	prop->value.floatnum = floatnum;
	prop->type = JSPT_DOUBLE;
}

void value_to_jsval(JSContext *ctx, jsval *vp, struct jsval_property *prop)
{
	switch (prop->type) {
	case JSPT_STRING:
	case JSPT_ASTRING:
		if (!prop->value.string) {
			*vp = JSVAL_NULL;
			break;
		}
		*vp = STRING_TO_JSVAL(JS_NewStringCopyZ(ctx, (char *)(prop->value.string)));
		if (prop->type == JSPT_ASTRING)
			free(prop->value.string);
		break;

	case JSPT_BOOLEAN:
		*vp = BOOLEAN_TO_JSVAL(prop->value.boolean);
		break;

	case JSPT_DOUBLE:
		*vp = DOUBLE_TO_JSVAL(prop->value.floatnum);
		break;

	case JSPT_INT:
		*vp = INT_TO_JSVAL(prop->value.number);
		break;

	case JSPT_OBJECT:
		*vp = OBJECT_TO_JSVAL(prop->value.object);
		break;

	case JSPT_UNDEF:
	default:
		*vp = JSVAL_NULL;
		break;
	}
}

void jsval_to_value(JSContext *ctx, jsval *vp, JSType type, union jsval_union *var)
{
	jsval val;

	if (JS_ConvertValue(ctx, *vp, type, &val) == JS_FALSE) {
		switch (type) {
		case JSTYPE_BOOLEAN:
			var->boolean = JS_FALSE;
			break;
		case JSTYPE_NUMBER:
			var->number = NULL;
			break;
		case JSTYPE_STRING:
			var->string = NULL;
			break;
		case JSTYPE_VOID:
		case JSTYPE_OBJECT:
		case JSTYPE_FUNCTION:
		case JSTYPE_LIMIT:
		default:
			fprintf(stderr, "Invalid type %d in jsval_to_value()", type);
			break;
		}
		return;
	}

	switch (type) {
	case JSTYPE_BOOLEAN:
		var->boolean = JSVAL_TO_BOOLEAN(val);
		break;
	case JSTYPE_NUMBER:
		var->number = JSVAL_TO_DOUBLE(val);
		break;
	case JSTYPE_STRING:
#if 0
		var->string = (unsigned char *)JS_GetStringCharsZ(JS_ValueToString(ctx, val));
#else
		var->string = (unsigned char *)JS_GetStringBytes(JS_ValueToString(ctx, val));
#endif
		break;
	case JSTYPE_VOID:
	case JSTYPE_OBJECT:
	case JSTYPE_FUNCTION:
	case JSTYPE_LIMIT:
	default:
		fprintf(stderr, "Invalid type %d in jsval_to_value()", type);
		break;
	}
}

/* SpiderMonkey relate */
static JSRuntime *jsrt;

void
spidermonkey_init(void)
{
	jsrt = JS_NewRuntime(0x400000UL);	/* 4 MBytes */
	if ( !jsrt ) {
		fprintf(stderr, "spidermonkey_init failed!\n");
	}
}

void
spidermonkey_done(void)
{
	JS_DestroyRuntime(jsrt);
	JS_ShutDown();
}

static void
error_reporter(JSContext *ctx, const char *message, JSErrorReport *report)
{
//	fprintf(stderr, "error_reporter:%s\n", message);
}

void *
spidermonkey_get_interpreter(jsscript_interpreter *interpreter, void *wjsobj)
{
	JSContext *ctx = NULL;
	JSObject *navigator_obj = NULL; 
	JSObject *window_obj = NULL; 
	JSObject *document_obj = NULL;
    JSObject *history_obj = NULL;
	JSObject *location_obj = NULL;
	JSObject *frames_obj = NULL;
    JSObject *forms_obj=NULL;
    JSObject *images_obj=NULL;
   
	ctx = JS_NewContext(jsrt, 0x8000UL/* Stack allocation chunk size */);
	if (!ctx)
		return NULL;

	interpreter->backend_data = (void*)ctx;
	JS_SetContextPrivate(ctx, interpreter);
	JS_SetErrorReporter(ctx, error_reporter);

	window_obj = JS_NewObject(ctx, (JSClass *) &window_class, NULL, NULL);
	if (!window_obj) {
		spidermonkey_put_interpreter(interpreter, 1);
		fprintf(stderr, "JS_NewObject (window) failed!\n");
		return NULL;
	}
	JS_InitStandardClasses(ctx, window_obj);
	JS_DefineProperties(ctx, window_obj, (JSPropertySpec *) window_props);
	JS_DefineFunctions(ctx, window_obj, (JSFunctionSpec *) window_funcs);
	JS_SetPrivate(ctx, window_obj, wjsobj);
	((jsobject*)wjsobj)->smobj = window_obj;

	navigator_obj = JS_InitClass(ctx, window_obj, NULL, 
		(JSClass *) &navigator_class, NULL, 0, 
		(JSPropertySpec *) navigator_props, 
		(JSFunctionSpec *) navigator_funcs, NULL, NULL);
	JS_SetPrivate(ctx, navigator_obj, interpreter->jsobj);

	document_obj = JS_InitClass(ctx, window_obj, NULL, 
		(JSClass *) &document_class, NULL, 0, 
		(JSPropertySpec *) document_props, 
		(JSFunctionSpec *) document_funcs, NULL, NULL);
	JS_SetPrivate(ctx, document_obj, interpreter->jsobj);
	((jsobject*)interpreter->jsobj)->smobj = document_obj;

	history_obj = JS_InitClass(ctx, window_obj, NULL, 
		(JSClass *) &history_class, NULL, 0, 
		(JSPropertySpec *) history_props, 
		(JSFunctionSpec *) history_funcs, NULL, NULL);
	JS_SetPrivate(ctx, history_obj, interpreter->jsobj);
    
	location_obj = JS_InitClass(ctx, window_obj, NULL, 
		(JSClass *) &location_class, NULL, 0, 
		(JSPropertySpec *) location_props, 
		(JSFunctionSpec *) location_funcs, NULL, NULL);
	JS_SetPrivate(ctx, location_obj, interpreter->jsobj);

	frames_obj = JS_InitClass(ctx, window_obj, NULL, 
		(JSClass *) &frames_class, NULL, 0, 
		(JSPropertySpec *) frames_props, 
		(JSFunctionSpec *) frames_funcs, NULL, NULL);
	JS_SetPrivate(ctx, frames_obj, wjsobj);

	forms_obj = JS_InitClass(ctx, document_obj, NULL, 
		(JSClass *) &forms_class, NULL, 0, 
		(JSPropertySpec *) forms_props, 
		(JSFunctionSpec *) forms_funcs, NULL, NULL);
	JS_SetPrivate(ctx, forms_obj, interpreter->jsobj);

        images_obj = JS_InitClass(ctx, document_obj, NULL, 
		(JSClass *) &images_class, NULL, 0, 
		(JSPropertySpec *) images_props, 
		(JSFunctionSpec *) images_funcs, NULL, NULL);
	JS_SetPrivate(ctx, images_obj, interpreter->jsobj);

    return ctx;
}

void
spidermonkey_put_interpreter(jsscript_interpreter *interpreter, int mod)
{
	JSContext *ctx;

	ctx = interpreter->backend_data;
    if (mod)
	    JS_DestroyContext(ctx);
    else
	    JS_DestroyContextNoGC(ctx);
	interpreter->backend_data = NULL;
}


int
spidermonkey_eval(jsscript_interpreter *interpreter,
                  jsstring *code)
{
	jsval rval;
	JSObject *obj = NULL;
	JSContext *ctx = NULL;

	if ( interpreter && code ) {
		ctx = interpreter->backend_data;

        if (!ctx)
            return 0;

		if ( interpreter->thisobj ) {
			obj = ((jsobject*)interpreter->thisobj)->smobj;
		}
		if ( !obj ) {
			obj = JS_GetGlobalObject(ctx);
		}

		if ( interpreter->inuse && ctx && 
			code->source && (code->length != 0) ) {
			return JS_EvaluateScript(ctx, obj,
			                  code->source, code->length, "", 0, &rval);
		}
	}
	return 0;
}

#endif
