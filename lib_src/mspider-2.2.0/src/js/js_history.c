#include <minigui/common.h>
#include <minigui/minigui.h>
#include <minigui/gdi.h>
#include <minigui/window.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "browser.h"
#include "dw_viewport.h"
#include "nav.h"
#include "interface.h"
#include "spidermonkey.h"
#include "mgdconfig.h"

#ifdef JS_SUPPORT

extern mSpiderDoc *getdocsdoc(JSContext *ctx, JSObject *obj);
/* local data and function definition */
enum history_prop {
	JSP_HIS_LENGTH
};
static JSBool history_get_property(JSContext *ctx, JSObject *obj, jsval id, jsval *vp);
static JSBool history_set_property(JSContext *ctx, JSObject *obj, jsval id, jsval *vp);
static JSBool history_back(JSContext *ctx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);
static JSBool history_forward(JSContext *ctx, JSObject *obj, uintN argc,jsval *argv, jsval *rval);

static JSBool history_go(JSContext *ctx, JSObject *obj, uintN argc,jsval *argv, jsval *rval);
/* export data sructure */
const JSClass history_class = {
	"history",
	JSCLASS_HAS_PRIVATE,
	JS_PropertyStub, JS_PropertyStub,
	history_get_property, history_set_property,
	JS_EnumerateStub, JS_ResolveStub, JS_ConvertStub, JS_FinalizeStub
};

const JSPropertySpec history_props[] = {
{ "length",	JSP_HIS_LENGTH,	JSPROP_ENUMERATE | JSPROP_READONLY},
{ NULL}
};

const JSFunctionSpec history_funcs[] = {
	{ "back",	history_back,1 },
	{ "forward",	history_forward,1 },
	{ "go",	history_go,		0 },
	{ NULL }
};

/* history objects property and method */
static JSBool
history_get_property(JSContext *ctx, JSObject *obj, jsval id, jsval *vp)
{
	struct jsval_property prop;
	mSpiderDoc *doc = NULL;
    int temp;


	set_prop_undef(&prop);
	if (JSVAL_IS_STRING(id)) {
		union jsval_union v;
		jsval_to_value(ctx, &id, JSTYPE_STRING, &v);
		return JS_TRUE;
	} else if (!JSVAL_IS_INT(id)) {
		return JS_TRUE;
	}

	switch (JSVAL_TO_INT(id)) {
	case JSP_HIS_LENGTH:
        doc = getdocsdoc(ctx, obj);
        temp =a_Nav_stack_size(doc);
        set_prop_int(&prop, temp);
		break;
	default:
		return JS_TRUE;
	}

	value_to_jsval(ctx, vp, &prop);
	return JS_TRUE;
}

static JSBool
history_set_property(JSContext *ctx, JSObject *obj, jsval id, jsval *vp)
{
	return JS_TRUE;
}

static JSBool
history_back(JSContext *ctx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
    int i,temp;
	union jsval_union v;
	mSpiderDoc *doc = NULL;

	doc = getdocsdoc(ctx, obj);
	jsval_to_value(ctx, &argv[0], JSTYPE_STRING, &v);

    if(argc==0)
    {
        a_Nav_back(doc);
    }
    else
    {
        temp=atoi((char*)(v.string));
        if(argc==1)
        {
            printf("the input data is %d\n",temp);
            for(i=0;i<abs(temp);i++)
                a_Nav_back(doc);
        }
        else
        {
            return JS_TRUE;
        }
    }
	return JS_TRUE;
}

static JSBool
history_forward(JSContext *ctx, JSObject *obj, uintN argc,jsval *argv, jsval *rval)
{
    int i,temp;
	union jsval_union v;
	mSpiderDoc *doc = NULL;

	jsval_to_value(ctx, &argv[0], JSTYPE_STRING, &v);
	doc = getdocsdoc(ctx, obj);

    if(argc==0)
    {
        a_Nav_forw(doc);
    }
    else
    {
        if(argc==1)
        {
            temp=atoi((char*)(v.string));
            for(i=0;i<abs(temp);i++)
                a_Nav_forw(doc);
        }
        else
        {
            return JS_TRUE;
        }
    }
	return JS_TRUE;
}

static JSBool
history_go(JSContext *ctx, JSObject *obj, uintN argc,jsval *argv, jsval *rval)
{
    int i,temp;
	union jsval_union v;
	mSpiderDoc *doc = NULL;

	jsval_to_value(ctx, &argv[0], JSTYPE_STRING, &v);
	doc = getdocsdoc(ctx, obj);
    if(argc==0)
    {
        return JS_TRUE;
    }
    else
    {
        temp=atoi((char*)(v.string));
        if(argc==1)
        {
            if(temp>0)
            {
               for(i=0;i<abs(temp);i++)
                 a_Nav_forw(doc);
            }
            else
            {
               for(i=0;i<abs(temp);i++)
                 a_Nav_back(doc);
            }
        }
        else
        {
            return JS_TRUE;
        }
    }
	return JS_TRUE;
}
#endif
