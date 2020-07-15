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

#ifdef JS_SUPPORT

/* extern options's objects */
extern const JSClass option_class;
extern const JSPropertySpec option_props[];
extern const JSFunctionSpec option_funcs[]; 
/* local data and function definition */
enum options_prop { 
	JSP_OPTS_LENGTH = -1,
};

static JSBool 	options_get_property(JSContext *ctx, JSObject *obj, jsval id, jsval *vp);
static JSBool 	options_set_property(JSContext *ctx, JSObject *obj, jsval id, jsval *vp);
static JSObject*get_option_object(JSContext *ctx, JSObject *pobj, int idx);
static JSObject*get_optsmobj(JSContext *ctx, JSObject *pobj, int idx);
/* export data sructure */
const JSClass options_class = {
    "options",
    JSCLASS_HAS_PRIVATE,
    JS_PropertyStub, JS_PropertyStub,
    options_get_property, options_set_property,
    JS_EnumerateStub, JS_ResolveStub, JS_ConvertStub, JS_FinalizeStub
};

const JSPropertySpec options_props[] = {
	{ "length",		JSP_OPTS_LENGTH,	JSPROP_ENUMERATE|JSPROP_READONLY},
    { NULL }
};

const JSFunctionSpec options_funcs[] = {
    { NULL }
};

/* options objects property and method */
static JSBool
options_get_property(JSContext *ctx, JSObject *obj, jsval id, jsval *vp)
{
	struct jsval_property prop;
	jsobject *seljsobj = NULL;
	JSObject *selsmobj = NULL;
	JSObject *optsmobj = NULL;
	
	/* get parent's(select ctrl) jsobj */
	seljsobj = JS_GetPrivate(ctx, obj);
	selsmobj = seljsobj->smobj;
	set_prop_undef(&prop);
    if (JSVAL_IS_STRING(id)) {
        union jsval_union v;
        jsval_to_value(ctx, &id, JSTYPE_STRING, &v);
		return JS_TRUE;
    } else if (!JSVAL_IS_INT(id)) {
        return JS_TRUE;
    }
	
	switch (JSVAL_TO_INT(id)) {
	case JSP_OPTS_LENGTH:
		break;
	default:
		optsmobj = get_option_object(ctx, selsmobj, JSVAL_TO_INT(id));
		if ( optsmobj ) {
			set_prop_object(&prop, optsmobj);
		}
		break;
	}

	value_to_jsval(ctx, vp, &prop);
	return JS_TRUE;
}

static JSBool
options_set_property(JSContext *ctx, JSObject *obj, jsval id, jsval *vp)
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
static JSObject *get_option_object(JSContext *ctx, JSObject *pobj, int idx)
{
	int i;
	int optnum;
	int jsoptnum;
	DwWidget *dw = NULL;
	JSObject *retobj = NULL;
	JSObject *smobj = NULL;
	jsobject *pjsobj = NULL;
	jsobject *jsobj = NULL;
	
	if ( !pobj) {
		return NULL;
	}

	pjsobj = JS_GetPrivate(ctx, pobj);
	if ( !pjsobj || idx < 0 ) {
		return NULL;
	}

	jsoptnum = (int)pjsobj->pvar1;	/* opts count */
	dw = (DwWidget*)pjsobj->htmlobj;
	if ( !dw || !((DwMgWidget*)dw)->window ) {
		return NULL;
	}

	optnum = SendMessage(((DwMgWidget*)dw)->window, CB_GETCOUNT, 0, 0);
	if ( idx > optnum ) {
		return NULL;
	} else {
		if ( idx == optnum ) {
			optnum ++;
		}
		if ( jsoptnum < optnum ) {
			/* create a option list or add option item for select ctrl */
			for ( i=jsoptnum; i<optnum; i++ ) {
				/* jsobject */
				/* create a option list or add option item for select ctrl */
				jsobj = g_malloc0(sizeof(jsobject));
				if ( !jsobj ) {
					return NULL;
				}
				jsobj->jstype = jsoption;
				jsobj->jsparent = pjsobj;
		 		jsobj->htmlobj = NULL;	/* nothing */
				jsobj->pvar1 = (void*)i;	/* save index */
				pjsobj->children = 
					g_list_append(pjsobj->children, jsobj);
				/* JSObject */
				smobj = JS_NewObject(ctx, (JSClass*)&option_class, NULL, pobj);
				if ( !smobj ) {
					return NULL;
				}
				JS_DefineProperties(ctx, smobj, (JSPropertySpec *)option_props);
				JS_DefineFunctions(ctx, smobj, (JSFunctionSpec *)option_funcs);
				JS_SetPrivate(ctx, smobj, jsobj);
				jsobj->smobj = smobj;
				if ( i == idx) {
					retobj = smobj;
				}
			}
			pjsobj->pvar1 = (void*)optnum;
		}
		if ( !retobj ) {
			/* retobj = (JSObject*)get_optsmobj(pjsobj->children, idx); */
			retobj = (JSObject*)get_optsmobj(ctx, pobj, idx);
		}
	}
    return retobj; 
}

static JSObject *get_optsmobj(JSContext *ctx, JSObject *pobj, int idx)
{
	GList *list = NULL;
	jsobject *jsobj = NULL;
	jsobject *pjsobj = NULL;
	JSObject *smobj = NULL;

	if ( !pobj ) {
		return NULL;
	}

	pjsobj = JS_GetPrivate(ctx, pobj);
	list = pjsobj->children;

	if ( !list || idx < 0) {
		return NULL;
	}
	while ( list && list->data ) {
		jsobj =  (jsobject*)list->data;
		if ( (int)jsobj->pvar1 == idx ) {
			if ( !(jsobj->smobj) ) {
				smobj = JS_NewObject(ctx, (JSClass*)&option_class, NULL, pobj);
				if ( !smobj ) {
					return NULL;
				}
				JS_DefineProperties(ctx, smobj, (JSPropertySpec *)option_props);
				JS_DefineFunctions(ctx, smobj, (JSFunctionSpec *)option_funcs);
				JS_SetPrivate(ctx, smobj, jsobj);
				jsobj->smobj = smobj;
			}
			return (JSObject*)jsobj->smobj;
		}
		list = list->next;
	}
	return NULL;
}

#endif
