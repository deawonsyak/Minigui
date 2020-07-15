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
#ifdef JS_SUPPORT

/* local data and function definition */
enum radio_prop {
	JSP_RADIO_CHECKED,
	JSP_RADIO_DISABLED,
	JSP_RADIO_DEFAULTCHECKED,
	JSP_RADIO_FORM,
	JSP_RADIO_ID,
	JSP_RADIO_NAME,
	JSP_RADIO_TYPE,
	JSP_RADIO_VALUE

};
static JSBool radio_get_property(JSContext *ctx, JSObject *obj, jsval id, jsval *vp);
static JSBool radio_set_property(JSContext *ctx, JSObject *obj, jsval id, jsval *vp);
static int get_radio_state(jsobject *jsobj);
static void set_radio_state(int boolean, jsobject *jsobj);
static JSBool radio_blur(JSContext *ctx, JSObject *obj, uintN argc,jsval *argv, jsval *rval);
static JSBool radio_click(JSContext *ctx, JSObject *obj, uintN argc,jsval *argv, jsval *rval);
static JSBool radio_focus(JSContext *ctx, JSObject *obj, uintN argc,jsval *argv, jsval *rval);
static JSBool radio_select(JSContext *ctx, JSObject *obj, uintN argc,jsval *argv, jsval *rval);
/* export data sructure */
const JSClass radio_class = {
	"radio",
	JSCLASS_HAS_PRIVATE,
	JS_PropertyStub, JS_PropertyStub,
	radio_get_property, radio_set_property,
	JS_EnumerateStub, JS_ResolveStub, JS_ConvertStub, JS_FinalizeStub
};

const JSPropertySpec radio_props[] = {
    { "checked",  	JSP_RADIO_CHECKED,   	JSPROP_ENUMERATE },
    { "disabled",  	JSP_RADIO_DISABLED,   	JSPROP_ENUMERATE },
    { "defaultChecked",  	JSP_RADIO_DEFAULTCHECKED,   	JSPROP_ENUMERATE },
    { "form",  	JSP_RADIO_FORM,   	JSPROP_ENUMERATE |JSPROP_READONLY  },
    { "name",  	JSP_RADIO_NAME,   	JSPROP_ENUMERATE },
    { "id",  	JSP_RADIO_ID,   	JSPROP_ENUMERATE },
    { "type",  	JSP_RADIO_TYPE,   	JSPROP_ENUMERATE },
    { "value",  	JSP_RADIO_VALUE,   	JSPROP_ENUMERATE },
	{ NULL }
};

const JSFunctionSpec radio_funcs[] = {
	{ "blur",	radio_blur,		0 },
	{ "click",	radio_click,	0 },
	{ "focus",	radio_focus,		0 },
	{ "select",	radio_select,		0 },
	{ NULL }
};

const char *radio_propidx[] = {
	/* props */
	"accept",		
	"accessKey",	
	"align",		
	"alt",			
	"checked",
	"defaultChecked",	
	"disabled",		
	"form",			
	"id",
	"name",
	"size",
	"tabIndex",
	"type",
	"value",
	/* events */
	"onBlur",
	"onClick",
	"onFocus",
	"onSelectStart"
};
const int radio_propidxlen = sizeof(radio_propidx)/sizeof(char*);
/* radio objects property and method */
static JSBool
radio_get_property(JSContext *ctx, JSObject *obj, jsval id, jsval *vp)
{
	int ret = 0;
	struct jsval_property prop;
    int temp;
	jsobject *jsobj = NULL;

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
	case JSP_RADIO_CHECKED:
		temp=get_radio_state(jsobj);
		set_prop_boolean(&prop, temp);
		break;
	case JSP_RADIO_DISABLED:
		ret = get_jsobj_disabled(jsobj);
		if ( ret != -1 ) {
			set_prop_boolean(&prop, ret); 
		}
		break;
	case JSP_RADIO_DEFAULTCHECKED:
        ret=get_props_index(radio_propidx,radio_propidxlen,"defaultChecked");
        
        if (jsobj->jsprops[ret]== NULL)
            jsobj->jsprops[ret]=strdup("false");
        if (strcasecmp(jsobj->jsprops[ret], "false") == 0)
            set_prop_boolean(&prop, 0); 
        else
            set_prop_boolean(&prop, 1); 
		break;
	case JSP_RADIO_FORM:
        if((((jsobject*)jsobj->jsparent)->jstype == jsform)
                &&(((jsobject*)jsobj->jsparent)->smobj!=NULL))
			set_prop_object(&prop,(JSObject *)(((jsobject*)jsobj->jsparent)->smobj)); 
		break;
	case JSP_RADIO_NAME:
        set_prop_string(&prop,(unsigned char*)jsobj->jsname);
		break;
    case JSP_RADIO_ID:
		if ( jsobj->jsid ) {
			set_prop_string(&prop, (unsigned char *)jsobj->jsid);
		}
		break;
	case JSP_RADIO_TYPE:
        set_prop_string(&prop,(unsigned char*)"radio");
		break;
	case JSP_RADIO_VALUE:
        ret=get_props_index(radio_propidx,radio_propidxlen,"value");
        set_prop_string(&prop,(unsigned char*)jsobj->jsprops[ret]);
       break; 
	default:
		return JS_TRUE;
	}

	value_to_jsval(ctx, vp, &prop);
	return JS_TRUE;
}

static JSBool
radio_set_property(JSContext *ctx, JSObject *obj, jsval id, jsval *vp)
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
	case JSP_RADIO_CHECKED:
		jsval_to_value(ctx, vp, JSTYPE_BOOLEAN, &v);
		set_radio_state(v.boolean, jsobj);
		break;
	case JSP_RADIO_DISABLED:
		jsval_to_value(ctx, vp, JSTYPE_BOOLEAN, &v);
		set_jsobj_disabled(v.boolean, jsobj);
		break;
	case JSP_RADIO_DEFAULTCHECKED:
        jsval_to_value(ctx, vp, JSTYPE_STRING, &v);
        set_jsobj_props(radio_propidx,radio_propidxlen,jsobj,"defaultChecked",(char*)v.string);
    
        if (strcasecmp((char*)v.string, "false") == 0)
		    set_radio_state(0, jsobj);
		else if ( strcasecmp((char*)v.string, "true") == 0 ) 
		    set_radio_state(1, jsobj);
		break;
	case JSP_RADIO_NAME:
        jsval_to_value(ctx, vp, JSTYPE_STRING, &v);
        if(jsobj->jsname!=NULL) g_free(jsobj->jsname); 
        jsobj->jsname=strdup((char*)v.string);
		break;
    case JSP_RADIO_ID:
        jsval_to_value(ctx, vp, JSTYPE_STRING, &v);
        if (jsobj->jsid != NULL) g_free(jsobj->jsid);
        jsobj->jsid = strdup((char *)(v.string));
        break;
    case JSP_RADIO_VALUE:
        jsval_to_value(ctx, vp, JSTYPE_STRING, &v);
        set_jsobj_props(radio_propidx,radio_propidxlen,jsobj,"value", (char*)v.string);
	default:
		return JS_TRUE;
	}

	return JS_TRUE;
}

static JSBool radio_blur(JSContext *ctx, JSObject *obj, uintN argc,jsval *argv, jsval *rval)
{
    HWND hwnd;
    jsobject *jsobj = NULL;

    jsobj = JS_GetPrivate(ctx, obj);
    if (!jsobj || !(jsobj->htmlobj)){
        return JS_TRUE;
    }
    hwnd = ((DwMgWidget *)jsobj->htmlobj)->window;
    if (!hwnd){
        return JS_TRUE;
    }
    if (IsWindowEnabled (hwnd))
#if ((MINIGUI_MAJOR_VERSION >= 2 && MINIGUI_MICRO_VERSION >= 3 && MINIGUI_MINOR_VERSION >= 0))
        SetNullFocus(GetParent(hwnd));
#else
        SendMessage(hwnd, MSG_KILLFOCUS, 0 , 0);
#endif
	return JS_TRUE;
}
static JSBool radio_click(JSContext *ctx, JSObject *obj, uintN argc,jsval *argv, jsval *rval)
{
	HWND hwnd;
	jsobject *jsobj = NULL;

	jsobj = JS_GetPrivate(ctx, obj);
	if ( !jsobj || !(jsobj->htmlobj) ) {
		return JS_TRUE;
	}
	hwnd = ((DwMgWidget*)jsobj->htmlobj)->window;
	if ( !hwnd ) {
        return JS_TRUE;
	}
    
    if (!IsWindowEnabled (hwnd))
        return JS_TRUE;


    SendMessage(hwnd,BM_CLICK,0,0);
    return JS_TRUE;
}
static JSBool radio_select(JSContext *ctx, JSObject *obj, uintN argc,jsval *argv, jsval *rval)
{
    HWND hwnd;
    jsobject *jsobj = NULL;

    jsobj = JS_GetPrivate(ctx, obj);
    if (!jsobj || !(jsobj->htmlobj)){
        return JS_TRUE;
    }

    hwnd = ((DwMgWidget *)jsobj->htmlobj)->window;
    if (!hwnd){
        return JS_TRUE;
    }

    if (!IsWindowEnabled (hwnd))
        return JS_TRUE;
    
    PostMessage(hwnd, MSG_LBUTTONDBLCLK, 0, 0);
    SetFocusChild(hwnd);

	return JS_TRUE;
}
static JSBool radio_focus(JSContext *ctx, JSObject *obj, uintN argc,jsval *argv, jsval *rval)
{
	HWND hwnd;
	jsobject *jsobj = NULL;

	jsobj = JS_GetPrivate(ctx, obj);
	if ( !jsobj || !(jsobj->htmlobj) ) {
		return JS_TRUE;
	}
	hwnd = ((DwMgWidget*)jsobj->htmlobj)->window;
	if ( !hwnd ) {
        return JS_TRUE;
	}
    
    if (IsWindowEnabled (hwnd))
        SetFocusChild(hwnd);
    return JS_TRUE;
}

static int get_radio_state(jsobject *jsobj)
{
	DwWidget *dw = NULL;
	DwMgWidget *mgdw = NULL;

	dw = (DwWidget*)jsobj->htmlobj;
	if ( !dw ) {
		return -1;
	}
	mgdw = (DwMgWidget*)dw;
	if ( !mgdw->window ) {
		return -1;
	}
    return SendMessage(mgdw->window, BM_GETCHECK, BST_CHECKED, 0);

}

static void set_radio_state(int boolean, jsobject *jsobj)
{
	DwWidget *dw = NULL;
	DwMgWidget *mgdw = NULL;

	dw = (DwWidget*)jsobj->htmlobj;
	if ( !dw ) {
		return;
	}
	mgdw = (DwMgWidget*)dw;
	if ( !mgdw->window ) {
		return;
	}
    SendMessage(mgdw->window, BM_SETCHECK, boolean , 0);
}

/*
 *	
 *	return value: 
 *
 *	-1:	error
 *	 0:	enabled
 *	 1:	disabled
 *
 */
const char* getradioeventstr(jsobject *jsobj, int rc)
{
	int idx = -1;

	switch ( rc ) {
	case BN_CLICKED:
		idx = get_props_index(radio_propidx, radio_propidxlen, "onClick");
		break;
	case BN_PAINT:
		break;
	case BN_HILITE:	/* BN_PUSHED */
		break;
	case BN_UNHILITE:	/* BN_UNPUSHED */
		break;
	case BN_DISABLE:
		break;
	case BN_DOUBLECLICKED:	/* BN_DBLCLK */
		break;
	case BN_SETFOCUS:
		idx = get_props_index(radio_propidx, radio_propidxlen, "onFocus");
		break;
	case BN_KILLFOCUS:
		idx = get_props_index(radio_propidx, radio_propidxlen, "onBlur");
		break;
	default:
		break;
	}

	if ( idx == -1 ) {
		return NULL;
	}

	return jsobj->jsprops[idx];
}

#endif
