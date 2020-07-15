#include <minigui/common.h>
#include <minigui/minigui.h>
#include <minigui/gdi.h>
#include <minigui/window.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "browser.h"
#include "interface.h"
#include "html.h"
#include "dw_widget.h"
#include "mgwidget.h"
#include "spidermonkey.h"
#include "jsmisc.h"
#include "mgdconfig.h"

#ifdef JS_SUPPORT

/* local data and function definition */
enum hidden_prop {
	JSP_HIDDEN_DEFVALUE,
	JSP_HIDDEN_FORM,
	JSP_HIDDEN_ID,
	JSP_HIDDEN_NAME,
	JSP_HIDDEN_TYPE,
	JSP_HIDDEN_VALUE
};
static JSBool 	hidden_get_property(JSContext *ctx, JSObject *obj, jsval id, jsval *vp);
static JSBool 	hidden_set_property(JSContext *ctx, JSObject *obj, jsval id, jsval *vp);
static char* 	get_hidden_value(jsobject *jsobj);
static void 	set_hidden_value(JSContext *ctx, JSObject *obj, const char *value);
/* export data sructure */
const JSClass hidden_class = {
	"hidden",
	JSCLASS_HAS_PRIVATE,
	JS_PropertyStub, JS_PropertyStub,
	hidden_get_property, hidden_set_property,
	JS_EnumerateStub, JS_ResolveStub, JS_ConvertStub, JS_FinalizeStub
};

const JSPropertySpec hidden_props[] = {
    { "defaultValue", 	JSP_HIDDEN_DEFVALUE,  	JSPROP_ENUMERATE },
    { "form", 	JSP_HIDDEN_FORM,  	JSPROP_ENUMERATE },
	{ "id",  	JSP_HIDDEN_ID,   	JSPROP_ENUMERATE },
    { "name",  	JSP_HIDDEN_NAME,   	JSPROP_ENUMERATE },
    { "type", 	JSP_HIDDEN_TYPE,  	JSPROP_ENUMERATE | JSPROP_READONLY },
    { "value", 	JSP_HIDDEN_VALUE,  	JSPROP_ENUMERATE },
	{ NULL }
};

const JSFunctionSpec hidden_funcs[] = {
	{ NULL }
};

const char *hidden_propidx[] = {
	/* props */
	"defaultValue",	
	"form",			
	"id",
	"name",
	"type",
	"value",
	/* events */
};
const int hidden_propidxlen = sizeof(hidden_propidx)/sizeof(char*);

/* hidden objects property and method */
static JSBool
hidden_get_property(JSContext *ctx, JSObject *obj, jsval id, jsval *vp)
{
	struct jsval_property prop;
	char *str = NULL;
	jsobject *jsobj = NULL;
    int ret;


	jsobj = JS_GetPrivate(ctx, obj);
	set_prop_undef(&prop);
	if (JSVAL_IS_STRING(id)) {
		union jsval_union v;
		jsval_to_value(ctx, &id, JSTYPE_STRING, &v);
		return JS_TRUE;
	} else if (!JSVAL_IS_INT(id)) {
		return JS_TRUE;
    }


	switch (JSVAL_TO_INT(id)) {
	case JSP_HIDDEN_DEFVALUE:
        ret=get_props_index(hidden_propidx,hidden_propidxlen,"defaultValue");
        set_prop_string(&prop,(unsigned char *)(jsobj->jsprops[ret])); 
		break;
	case JSP_HIDDEN_FORM:
        if((((jsobject*)jsobj->jsparent)->jstype == jsform)
                &&(((jsobject*)jsobj->jsparent)->smobj!=NULL))
			set_prop_object(&prop,(JSObject *)(((jsobject*)jsobj->jsparent)->smobj)); 
		break;
	case JSP_HIDDEN_ID:
		if ( jsobj->jsid ) {
			set_prop_string(&prop, (unsigned char *)jsobj->jsid);
		}
		break;
	case JSP_HIDDEN_NAME:
		if ( jsobj->jsname ) {
			set_prop_string(&prop,(unsigned char *)jsobj->jsname ); 
		}
		break;
	case JSP_HIDDEN_TYPE:
        set_prop_string(&prop,(unsigned char *)"hidden");
		break;
	case JSP_HIDDEN_VALUE:
		str = get_hidden_value(jsobj);
		if ( str ) {
			set_prop_string(&prop, (unsigned char *)str); 
		}
		break;
	default:
		return JS_TRUE;
	}

	value_to_jsval(ctx, vp, &prop);
	return JS_TRUE;
}

static JSBool
hidden_set_property(JSContext *ctx, JSObject *obj, jsval id, jsval *vp)
{
	union jsval_union v;
	jsobject *jsobj = NULL;

	jsobj = JS_GetPrivate(ctx, obj);
	if (JSVAL_IS_STRING(id)) {
		union jsval_union v;
		jsval_to_value(ctx, &id, JSTYPE_STRING, &v);
		return JS_TRUE;
	} else if (!JSVAL_IS_INT(id)) {
		return JS_TRUE;
	}

	switch (JSVAL_TO_INT(id)) {
	case JSP_HIDDEN_DEFVALUE:
        jsval_to_value(ctx, vp, JSTYPE_STRING, &v);
        set_jsobj_props(hidden_propidx,hidden_propidxlen,jsobj,"defaultValue",(char*)v.string);
		break;
	case JSP_HIDDEN_NAME:
        jsval_to_value(ctx, vp, JSTYPE_STRING, &v);
        if(jsobj->jsname!=NULL)
            free(jsobj->jsname);
        jsobj->jsname=strdup((char*)(v.string));
		break;
    case JSP_HIDDEN_ID:
        jsval_to_value(ctx, vp, JSTYPE_STRING, &v);
        if (jsobj->jsid != NULL)
            free(jsobj->jsid);
        jsobj->jsid = strdup((char *)(v.string));
        break;
	case JSP_HIDDEN_VALUE:
		jsval_to_value(ctx, vp, JSTYPE_STRING, &v);
		set_hidden_value(ctx, obj, (char*)(v.string));
		break;
	default:
		return JS_TRUE;
	}

	return JS_TRUE;
}

static char* get_hidden_value(jsobject *jsobj)
{
	int idx;

	idx = get_props_index(hidden_propidx, hidden_propidxlen, "value");
	return jsobj->jsprops[idx];
}

static void set_hidden_value(JSContext *ctx, JSObject *obj, const char *value)
{
	int idx;
	int formid;
	jsobject *jsobj = NULL;
	jsobject *formobj = NULL;
	mSpiderDoc *doc = NULL;
	mSpiderHtmlLB *lb = NULL;
	mSpiderHtmlForm *form = NULL;
	mSpiderHtmlInput *input = NULL;

	jsobj = JS_GetPrivate(ctx, obj);
	if ( !jsobj ) {
		return;
	}

	idx = get_props_index(hidden_propidx, hidden_propidxlen, "value");
	if ( jsobj->jsprops[idx] ) {
		g_free(jsobj->jsprops[idx]);
	}
	jsobj->jsprops[idx] = g_strdup(value);

	/* sync the data stored in mSpiderInput */
	formobj = jsobj;
	while ( formobj && (formobj->jstype != jsform) )  {
		formobj = formobj->jsparent;
	}
	if ( !formobj ) {
		return;
	}

	formid = (int)formobj->pvar1;	/* formid in mSpiderForm */
	doc = (mSpiderDoc*)gethtmldoc(jsobj);
	if ( doc ) {
		lb = doc->html_block;
		if ( lb ) {
			form = &lb->forms[formid]; 
			if ( form ) {
				input = &form->inputs[(int)jsobj->pvar1];
				if ( input ) {
					if ( input->init_str ) {
						g_free(input->init_str);
					}
					input->init_str = g_strdup(value);
				}
			}
		}
	}
}

#endif
