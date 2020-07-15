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
#include "frameset.h"
#include "mgdconfig.h"

#ifdef JS_SUPPORT
/* local data and function definition */
enum frames_prop { 
	JSP_FRAMES_LENGTH = -1 
};

static JSBool 	frames_get_property(JSContext *ctx, JSObject *obj, jsval id, jsval *vp);
static JSBool 	frames_set_property(JSContext *ctx, JSObject *obj, jsval id, jsval *vp);
static int		getframeslen(JSContext *ctx, JSObject *obj);
static JSObject*getframebyidx(JSContext *ctx, JSObject *obj, int idx);
/* export data sructure */
const JSClass frames_class = {
    "frames",
    JSCLASS_HAS_PRIVATE,
    JS_PropertyStub, JS_PropertyStub,
    frames_get_property, frames_set_property,
    JS_EnumerateStub, JS_ResolveStub, JS_ConvertStub, JS_FinalizeStub
};

const JSPropertySpec frames_props[] = {
    { "length",	JSP_FRAMES_LENGTH,	JSPROP_ENUMERATE | JSPROP_READONLY },
    { NULL }
};

const JSFunctionSpec frames_funcs[] = {
    { NULL }
};

/* frames objects property and method */
static JSBool
frames_get_property(JSContext *ctx, JSObject *obj, jsval id, jsval *vp)
{
	union jsval_union v;
	struct jsval_property prop;
	jsobject *wjsobj = NULL;
	JSObject *wobj = NULL;
	jsobject *findobj = NULL;
	
	wjsobj = JS_GetPrivate(ctx, obj);
	wobj = wjsobj->smobj;
	set_prop_undef(&prop);
    if (JSVAL_IS_STRING(id)) {
        jsval_to_value(ctx, &id, JSTYPE_STRING, &v);
		findobj = js_findobjbyname((char*)v.string, NULL);
		if ( findobj && (findobj->jstype == jswindow) ) {
			set_prop_object(&prop, findobj->smobj);
		}
		value_to_jsval(ctx, vp, &prop);
		return JS_TRUE;
    } else if (!JSVAL_IS_INT(id)) {
        return JS_TRUE;
    }
	
	switch (JSVAL_TO_INT(id)) {
	case JSP_FRAMES_LENGTH: 
		set_prop_int(&prop, getframeslen(ctx, obj));
		break;
	default:
		set_prop_object(&prop, getframebyidx(ctx, obj, JSVAL_TO_INT(id)));
		break;
	}

	value_to_jsval(ctx, vp, &prop);
	return JS_TRUE;
}

static JSBool
frames_set_property(JSContext *ctx, JSObject *obj, jsval id, jsval *vp)
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

static int getframeslen(JSContext *ctx, JSObject *obj)
{
	jsobject *wjsobj = NULL;
	mSpiderDoc *doc = NULL;
	DwFrameset *fset = NULL;

	wjsobj = JS_GetPrivate(ctx, obj);
	doc = (mSpiderDoc*)wjsobj->htmlobj;
	fset = (DwFrameset*)doc->frameset;
	if ( fset ) {
		/* have frame */
		return fset->area_used;
	}
	
	return 0;
}

extern  mSpiderDoc **mspider_doc;
extern  gint num_dd, num_dd_max;
static JSObject *getframebyidx(JSContext *ctx, JSObject *obj, int idx)
{
	int i, j;
	jsobject *wjsobj = NULL;
	mSpiderDoc *doc = NULL;
	DwFrameset *fset = NULL;

	wjsobj = JS_GetPrivate(ctx, obj);
	doc = (mSpiderDoc*)wjsobj->htmlobj;
	fset = (DwFrameset*)doc->frameset;
	if ( fset ) {
		/* have frame */
		j = 0;
		for ( i=0; i<num_dd; i++ ) {
			if ( mspider_doc[i]->parent == doc ) {
				if ( idx == j++ ) {
					return mspider_doc[i]->jsobj->smobj;
				}
			}
		}
	}

	return NULL;
}

#endif
