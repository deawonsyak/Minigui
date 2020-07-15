#include <minigui/common.h>
#include <minigui/minigui.h>
#include <minigui/gdi.h>
#include <minigui/window.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "glist.h"
#include "html.h"
#include "dw_widget.h"
#include "mgwidget.h"
#include "spidermonkey.h"
#include "form.h"

#ifdef JS_SUPPORT
/* local data and function definition */
enum elements_prop { 
	JSP_FORM_ELEMENTS_LENGTH = -1 
};

static JSBool 	elements_get_property(JSContext *ctx, JSObject *obj, jsval id, jsval *vp);
static JSBool 	elements_set_property(JSContext *ctx, JSObject *obj, jsval id, jsval *vp);
static JSObject*get_input_object(JSContext *ctx, JSObject *pobj, int idx);
/* export data sructure */
const JSClass elements_class = {
    "elements",
    JSCLASS_HAS_PRIVATE,
    JS_PropertyStub, JS_PropertyStub,
    elements_get_property, elements_set_property,
    JS_EnumerateStub, JS_ResolveStub, JS_ConvertStub, JS_FinalizeStub
};

const JSPropertySpec elements_props[] = {
    { "length",	JSP_FORM_ELEMENTS_LENGTH,	JSPROP_ENUMERATE | JSPROP_READONLY },
    { NULL }
};

const JSFunctionSpec elements_funcs[] = {
    { NULL }
};

/* elements objects property and method */
static JSBool
elements_get_property(JSContext *ctx, JSObject *obj, jsval id, jsval *vp)
{
    int counter = 0;
	struct jsval_property prop;
	JSObject *inobj= NULL;
	jsobject *formobj = NULL;
	JSObject *fobj = NULL;
	jsobject *findobj = NULL;
	
	formobj = JS_GetPrivate(ctx, obj);
	fobj = formobj->smobj;
	set_prop_undef(&prop);
    if (JSVAL_IS_STRING(id)) {
        union jsval_union v;
        jsval_to_value(ctx, &id, JSTYPE_STRING, &v);
		findobj = js_findobjbyname((char *)(v.string), formobj);
		inobj = getinputsmobj(ctx, fobj, findobj);
		set_prop_object(&prop, inobj );
		value_to_jsval(ctx, vp, &prop);
		return JS_TRUE;
    } else if (!JSVAL_IS_INT(id)) {
        return JS_TRUE;
    }
	
	switch (JSVAL_TO_INT(id)) {
	case JSP_FORM_ELEMENTS_LENGTH: 
		counter=g_list_length(formobj->children);
		set_prop_int(&prop, counter);
		break;
	default:
		inobj = get_input_object(ctx, obj, JSVAL_TO_INT(id));
		set_prop_object(&prop, inobj );
		break;
	}

	value_to_jsval(ctx, vp, &prop);
	return JS_TRUE;
}

static JSBool
elements_set_property(JSContext *ctx, JSObject *obj, jsval id, jsval *vp)
{
	union jsval_union v;

    if (JSVAL_IS_STRING(id)) {
        jsval_to_value(ctx, &id, JSTYPE_STRING, &v);
    } else if (!JSVAL_IS_INT(id)) {
        return JS_TRUE;
    }

	switch (JSVAL_TO_INT(id)) {
	default:
		return JS_TRUE;
	}

	return JS_TRUE;
}

/* other function */
static JSObject *get_input_object(JSContext *ctx, JSObject *pobj, int idx)
{
    int i;
	jsobject *formjsobj = NULL;
	JSObject *formobj = NULL;
	jsobject *inputobj = NULL;
	jsobject *findobj = NULL;
    GList *inputlist = NULL;

	if ( idx < 0 ) {
		return NULL;
	}

	formjsobj = JS_GetPrivate(ctx, pobj);
	if ( !formjsobj ) {
		return NULL;
	}

	formobj = (JSObject*)formjsobj->smobj;
	if ( !formobj ) {
		return NULL;
	}

	for( i=0; i<g_list_length(formjsobj->children); i++ ) {
		inputlist = g_list_nth(formjsobj->children, i);
		inputobj = inputlist->data;
		if ( (int)inputobj->InputID == idx ) {
			findobj = inputobj;
			break;
		}
	}       
   	if( !findobj )
   	{
		return NULL;
   	}

	return getinputsmobj(ctx, formobj, findobj);
}

#endif
