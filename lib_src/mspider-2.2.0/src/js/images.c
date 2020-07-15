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
extern JSObject*get_img_object(JSContext *ctx, JSObject *pobj, jsobject *jsobj);
/* local data and function definition */
enum images_prop { 
	JSP_IMAGES_LENGTH = -1 
};

static JSBool 	images_get_property(JSContext *ctx, JSObject *obj, jsval id, jsval *vp);
static JSBool 	images_set_property(JSContext *ctx, JSObject *obj, jsval id, jsval *vp);

static int		getimageslen(JSContext *ctx, JSObject *obj);
static JSObject*getimagebyidx(JSContext *ctx, JSObject *obj, int idx);

/* export data sructure */
const JSClass images_class = {
    "images",
    JSCLASS_HAS_PRIVATE,
    JS_PropertyStub, JS_PropertyStub,
    images_get_property, images_set_property,
    JS_EnumerateStub, JS_ResolveStub, JS_ConvertStub, JS_FinalizeStub
};

const JSPropertySpec images_props[] = {
    { "length",	JSP_IMAGES_LENGTH,	JSPROP_ENUMERATE | JSPROP_READONLY },
    { NULL }
};

const JSFunctionSpec images_funcs[] = {
    { NULL }
};

/* images objects property and method */
static JSBool
images_get_property(JSContext *ctx, JSObject *obj, jsval id, jsval *vp)
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
		if ( findobj && (findobj->jstype == jsimage) ) {
			if ( !findobj->smobj ) {
				get_img_object(ctx, docobj, findobj);
			}
			set_prop_object(&prop, findobj->smobj);
		}
		value_to_jsval(ctx, vp, &prop);
		return JS_TRUE;
    } else if (!JSVAL_IS_INT(id)) {
        return JS_TRUE;
    }
	
	switch (JSVAL_TO_INT(id)) {
	case JSP_IMAGES_LENGTH: 
		set_prop_int(&prop, getimageslen(ctx, docobj));
		break;
	default:
		set_prop_object(&prop, getimagebyidx(ctx, docobj, JSVAL_TO_INT(id)));
		break;
	}

	value_to_jsval(ctx, vp, &prop);
	return JS_TRUE;
}

static JSBool
images_set_property(JSContext *ctx, JSObject *obj, jsval id, jsval *vp)
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

static int getimageslen(JSContext *ctx, JSObject *obj)
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
					return g_list_length(lb->images);
				}
			}
		}
	}

	return 0;
}
static JSObject *getimagebyidx(JSContext *ctx, JSObject *obj, int idx)
{
	int count;
	int imageslen;
	jsobject *docjsobj	= NULL;
	jsobject *jsobj = NULL;
	GList *list = NULL;

	imageslen = getimageslen(ctx, obj);
	if ( idx >= imageslen ) {
		return NULL;
	}

	docjsobj = JS_GetPrivate(ctx, obj);	/* get documents's jsobj */
	if ( docjsobj && docjsobj->children ) {
		list = docjsobj->children;
		count = 0;
		while ( list ) {
			jsobj = (jsobject*)list->data;
			if ( jsobj && (jsobj->jstype == jsimage) ) {
				if ( idx == count ) {
					if ( !jsobj->smobj ) {
						get_img_object(ctx, docjsobj->smobj, jsobj);
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
