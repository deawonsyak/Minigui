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
#include "mgdconfig.h"

#ifdef JS_SUPPORT
extern JSObject*get_form_object(JSContext *ctx, JSObject *pobj, jsobject *jsobj);
/* local data and function definition */
enum forms_prop { 
	JSP_FORMS_LENGTH = -1 
};

static JSBool 	forms_get_property(JSContext *ctx, JSObject *obj, jsval id, jsval *vp);
static JSBool 	forms_set_property(JSContext *ctx, JSObject *obj, jsval id, jsval *vp);
static int		getformslen(JSContext *ctx, JSObject *obj);
static JSObject*getformbyidx(JSContext *ctx, JSObject *obj, int idx);
/* export data sructure */
const JSClass forms_class = {
    "forms",
    JSCLASS_HAS_PRIVATE,
    JS_PropertyStub, JS_PropertyStub,
    forms_get_property, forms_set_property,
    JS_EnumerateStub, JS_ResolveStub, JS_ConvertStub, JS_FinalizeStub
};

const JSPropertySpec forms_props[] = {
    { "length",	JSP_FORMS_LENGTH,	JSPROP_ENUMERATE | JSPROP_READONLY },
    { NULL }
};

const JSFunctionSpec forms_funcs[] = {
    { NULL }
};

/* forms objects property and method */
static JSBool
forms_get_property(JSContext *ctx, JSObject *obj, jsval id, jsval *vp)
{
	union jsval_union v;
	struct jsval_property prop;
	jsobject *docjsobj = NULL;
	JSObject *docobj = NULL;
	jsobject *findobj = NULL;
	
	docjsobj = JS_GetPrivate(ctx, obj);	/* document's jsobj */
	docobj = docjsobj->smobj;
	set_prop_undef(&prop);
    if (JSVAL_IS_STRING(id)) {
        jsval_to_value(ctx, &id, JSTYPE_STRING, &v);
		findobj = js_findobjbyname((char*)(v.string), docjsobj);
		if ( findobj && (findobj->jstype == jsform) ) {
			if ( !findobj->smobj ) {
				get_form_object(ctx, docobj, findobj);
			}
			set_prop_object(&prop, findobj->smobj);
		}
		value_to_jsval(ctx, vp, &prop);
		return JS_TRUE;
    } else if (!JSVAL_IS_INT(id)) {
        return JS_TRUE;
    }
	
	switch (JSVAL_TO_INT(id)) {
	case JSP_FORMS_LENGTH: 
		set_prop_int(&prop, getformslen(ctx, docobj));
		break;
	default:
		set_prop_object(&prop, getformbyidx(ctx, docobj, JSVAL_TO_INT(id)));
		break;
	}

	value_to_jsval(ctx, vp, &prop);
	return JS_TRUE;
}

static JSBool
forms_set_property(JSContext *ctx, JSObject *obj, jsval id, jsval *vp)
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

static int getformslen(JSContext *ctx, JSObject *obj)
{
	jsobject *jsobj	= NULL;
	jsobject *wjsobj = NULL;
	mSpiderDoc *doc = NULL;
	mSpiderHtmlLB *lb = NULL;

	jsobj = JS_GetPrivate(ctx, obj);	/* get documents's jsobj */
	if ( jsobj ) {
		wjsobj = jsobj->jsparent;
		if ( wjsobj ) {
			doc = (mSpiderDoc*)wjsobj->htmlobj;
			if ( doc ) {
				lb = (mSpiderHtmlLB*)doc->html_block;
				if ( lb ) {
					return lb->num_forms;
				}
			}
		}
	}

	return 0;
}

static JSObject *getformbyidx(JSContext *ctx, JSObject *obj, int idx)
{
	int count;
	int formslen;
	jsobject *docjsobj	= NULL;
	jsobject *jsobj = NULL;
	GList *list = NULL;

	formslen = getformslen(ctx, obj);
	if ( idx >= formslen ) {
		return NULL;
	}

	docjsobj = JS_GetPrivate(ctx, obj);	/* get documents's jsobj */
	if ( docjsobj && docjsobj->children ) {
		list = docjsobj->children;
		count = 0;
		while ( list ) {
			jsobj = (jsobject*)list->data;
			if ( jsobj && (jsobj->jstype == jsform) ) {
				if ( idx == count ) {
					if ( !jsobj->smobj ) {
						get_form_object(ctx, docjsobj->smobj, jsobj);
					}
					return jsobj->smobj;
				}
				count++;
			}
			list = list->next;
		}
	}	

	return NULL;
}

#endif
