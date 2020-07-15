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
#include "dw_page.h"
#include "dw_widget.h"
#include "mgwidget.h"
#include "dw_viewport.h"
#include "spidermonkey.h"
#include "jsmisc.h"
#include "mgdconfig.h"

#ifdef JS_SUPPORT

/* local data and function definition */
enum button_prop {
	JSP_BUTTON_DFTVALUE = 1,
	JSP_BUTTON_DISABLED,
	JSP_BUTTON_VALUE,
	JSP_BUTTON_FORM,
	JSP_BUTTON_ID,
	JSP_BUTTON_NAME,
	JSP_BUTTON_TYPE
};
static JSBool 	button_get_property(JSContext *ctx, JSObject *obj, jsval id, jsval *vp);
static JSBool 	button_set_property(JSContext *ctx, JSObject *obj, jsval id, jsval *vp);
static char*	get_button_value(jsobject *jsobj);
static void 	set_button_value(const char *value, jsobject *jsobj);
static int 		get_button_disabled(jsobject *jsobj);
static void 	set_button_disabled(int boolean, jsobject *jsobj);

static JSBool	button_blur(JSContext *ctx, JSObject *obj, uintN argc,jsval *argv, jsval *rval);
static JSBool	button_click(JSContext *ctx, JSObject *obj, uintN argc,jsval *argv, jsval *rval);
static JSBool 	button_focus(JSContext *ctx, JSObject *obj, uintN argc,jsval *argv, jsval *rval);
static JSBool 	button_select(JSContext *ctx, JSObject *obj, uintN argc,jsval *argv, jsval *rval);

/* export data sructure */
const JSClass button_class = {
	"button",
	JSCLASS_HAS_PRIVATE,
	JS_PropertyStub, JS_PropertyStub,
	button_get_property, button_set_property,
	JS_EnumerateStub, JS_ResolveStub, JS_ConvertStub, JS_FinalizeStub
};

const JSPropertySpec button_props[] = {
	{ "defaultValue",	JSP_BUTTON_DFTVALUE,	JSPROP_ENUMERATE },
    { "value",   	JSP_BUTTON_VALUE,    	JSPROP_ENUMERATE },
    { "disabled",  	JSP_BUTTON_DISABLED,   	JSPROP_ENUMERATE },
    { "form",  	JSP_BUTTON_FORM,   	JSPROP_ENUMERATE |JSPROP_READONLY},
    { "id",  	JSP_BUTTON_ID,   	JSPROP_ENUMERATE },
    { "name",  	JSP_BUTTON_NAME,   	JSPROP_ENUMERATE },
    { "type",  	JSP_BUTTON_TYPE,   	JSPROP_ENUMERATE |JSPROP_READONLY},
	{ NULL }
};

const JSFunctionSpec button_funcs[] = {
	{ "blur",	button_blur,		0 },
	{ "click",	button_click,	0 },
	{ "focus",	button_focus,		0 },
	{ "select",	button_select,		0 },
	{ NULL }
};

const char *button_propidx[] = {
	/* props */
	"accept",		
	"accessKey",	
	"align",		
	"alt",			
	"defaultValue",	
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
	"onMouseDown",
	"onMouseUp"
};
const int button_propidxlen = sizeof(button_propidx)/sizeof(char*);
/* button objects property and method */
static JSBool
button_get_property(JSContext *ctx, JSObject *obj, jsval id, jsval *vp)
{
	int ret = 0;
	struct jsval_property prop;
	char *value = NULL;
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
        case JSP_BUTTON_DFTVALUE:
            if (jsobj->jstype == jsreset||jsobj->jstype == jssubmit)
            {
                ret = get_props_index(button_propidx, button_propidxlen, "defaultValue");
                if (jsobj->jsprops[ret]==NULL){
                    value = get_button_value(jsobj);
                    if (value)
                        set_prop_string(&prop, (unsigned char*)value);
                }
                else
                    set_prop_string(&prop, (unsigned char*)jsobj->jsprops[ret]);

            }
            break;
        case JSP_BUTTON_VALUE:
            value = get_button_value(jsobj);
            if ( value ) {
                set_prop_string(&prop, (unsigned char*)value);
            }
            break;
        case JSP_BUTTON_DISABLED:
            ret = get_button_disabled(jsobj);
            if ( ret != -1 ) {
                set_prop_boolean(&prop, ret); 
            }
            break;
        case JSP_BUTTON_FORM:
            if((((jsobject*)jsobj->jsparent)->jstype == jsform)
                    &&(((jsobject*)jsobj->jsparent)->smobj!=NULL))
                set_prop_object(&prop,(JSObject *)(((jsobject*)jsobj->jsparent)->smobj)); 
            break;
        case JSP_BUTTON_ID:
            set_prop_string(&prop,(unsigned char*)jsobj->jsid); 
            break;
        case JSP_BUTTON_NAME:
            set_prop_string(&prop,(unsigned char*)jsobj->jsname);
            break;
        case JSP_BUTTON_TYPE:
            if (jsobj->jstype == jsbutton)
                set_prop_string(&prop,(unsigned char*)("button"));
            else if(jsobj->jstype == jsreset)
                set_prop_string(&prop,(unsigned char*)("reset"));
            else if(jsobj->jstype == jssubmit)
                set_prop_string(&prop,(unsigned char*)("submit"));
            break;
        default:
            return JS_TRUE;
	}

	value_to_jsval(ctx, vp, &prop);
	return JS_TRUE;
}

static JSBool
button_set_property(JSContext *ctx, JSObject *obj, jsval id, jsval *vp)
{
	union jsval_union v;
	jsobject *jsobj = NULL;
    int index;
	
    jsobj = JS_GetPrivate(ctx, obj);
    
	if (JSVAL_IS_STRING(id)) {
		union jsval_union v;
		jsval_to_value(ctx, &id, JSTYPE_STRING, &v);
		return JS_TRUE;
	} else if (!JSVAL_IS_INT(id)) {
		return JS_TRUE;
	}

	switch (JSVAL_TO_INT(id)) {
        case JSP_BUTTON_DFTVALUE:
            if (jsobj->jstype == jsreset||jsobj->jstype == jssubmit)
            {
                jsval_to_value(ctx, vp, JSTYPE_STRING, &v);
                index = get_props_index(button_propidx, button_propidxlen, "defaultValue");
                if (jsobj->jsprops[index]!=NULL) g_free (jsobj->jsprops[index]);
                jsobj->jsprops[index] = strdup((char *)(v.string));
            }
            break;
        case JSP_BUTTON_VALUE:
            jsval_to_value(ctx, vp, JSTYPE_STRING, &v);
            set_button_value((char*)(v.string), jsobj);
            break;
        case JSP_BUTTON_DISABLED:
            jsval_to_value(ctx, vp, JSTYPE_BOOLEAN, &v);
            set_button_disabled(v.boolean, jsobj);
            break;
        case JSP_BUTTON_NAME:
            jsval_to_value(ctx, vp, JSTYPE_STRING, &v);
            if(jsobj->jsname!=NULL)
                free(jsobj->jsname); 
            jsobj->jsname=strdup((char*)(v.string));
            break;
        case JSP_BUTTON_ID:
            jsval_to_value(ctx, vp, JSTYPE_STRING, &v);
            if (jsobj->jsid != NULL)
                free(jsobj->jsid);
            jsobj->jsid = strdup((char *)(v.string));
            break;
        default:
            return JS_TRUE;
	}
	return JS_TRUE;
}

static JSBool	button_blur(JSContext *ctx, JSObject *obj, uintN argc,jsval *argv, jsval *rval)
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
static JSBool	button_click(JSContext *ctx, JSObject *obj, uintN argc,jsval *argv, jsval *rval)
{
	HWND hwnd;
	jsobject *jsobj = NULL;
    int temp;

	jsobj = JS_GetPrivate(ctx, obj);
	if ( !jsobj || !(jsobj->htmlobj) ) {
		return JS_TRUE;
	}
	hwnd = ((DwMgWidget*)jsobj->htmlobj)->window;
	if ( !hwnd ) {
        return JS_TRUE;
	}

    if (!IsWindowEnabled(hwnd))
        return JS_TRUE;

    temp=SendMessage(hwnd,BM_GETSTATE,0,0);
    SendMessage(hwnd,BM_CLICK,0,0);
    return JS_TRUE;
}
static JSBool 	button_focus(JSContext *ctx, JSObject *obj, uintN argc,jsval *argv, jsval *rval)
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
static char* get_button_value(jsobject *jsobj)
{
	static char buf[0x100];
	DwWidget *dw = NULL;
	DwMgWidget *mgdw = NULL;

	dw = (DwWidget*)jsobj->htmlobj;
	if ( !dw ) {
		return NULL;
	}
	mgdw = (DwMgWidget*)dw;
	if ( !mgdw->window ) {
		return NULL;
	}

    memset(buf , 0 , 0x100);
	GetWindowText(mgdw->window, buf, 0xFF);
	/* ... */
	return buf;
}

static void set_button_value(const char *value, jsobject *jsobj)
{
    HWND hwnd;
    static RECT rec;
    static int x, y, w, h, pw, ph;
    DwPage *page;
    DwMgWidget * mgwidget;
    DwWidget *widget;
    DwViewport * viewport;

	if ( !jsobj || !(jsobj->htmlobj) ) {
		return;
	}
	hwnd = ((DwMgWidget*)jsobj->htmlobj)->window;
	if ( !hwnd ) {
		return;	
	}

    mgwidget = (DwMgWidget*)jsobj->htmlobj;
    
    Dw_MgWidget_get_text_metrics ("0", 1, &pw, &ph);
   
	GetWindowRect(hwnd, &rec);
    x = rec.left;
    y = rec.top;
    w = strlen(value)*pw+ADD_LENGTH;
    h = ph+ADD_LENGTH;
    
    mgwidget->size.cx = w;
    mgwidget->size.cy = h;
    widget = &(mgwidget->dw_widget);
    viewport = widget->viewport;
 
    p_Dw_widget_queue_resize(widget, 0, TRUE);
    
    page = (DwPage *)(jsobj->pvar1);
    Dw_page_rewrap (page);
    
    InvalidateRect (viewport->hwnd, NULL, TRUE);
    SetWindowText(mgwidget->window, value); 
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
static int get_button_disabled(jsobject *jsobj)
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

	if ( IsWindowEnabled(mgdw->window) )	{
		/* enable */
		return 0;
	} else {
		return 1;
	}
}

static void set_button_disabled(int boolean, jsobject *jsobj)
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

	if ( boolean ) {
		EnableWindow(mgdw->window, FALSE);
	} else {
		EnableWindow(mgdw->window, TRUE);
	}
}

const char* getbuttoneventstr(jsobject *jsobj, int rc)
{
	int idx = -1;

	switch ( rc ) {
	case BN_CLICKED:
		idx = get_props_index(button_propidx, button_propidxlen, "onClick");
		break;
	case BN_PUSHED:
		idx = get_props_index(button_propidx, button_propidxlen, "onMouseDown");
		break;
	case BN_UNPUSHED:
		idx = get_props_index(button_propidx, button_propidxlen, "onMouseUp");
		break;
	case BN_SETFOCUS:
		idx = get_props_index(button_propidx, button_propidxlen, "onFocus");
		break;
	case BN_KILLFOCUS:
		idx = get_props_index(button_propidx, button_propidxlen, "onBlur");
		break;
	default:
		break;
	}

	if ( idx == -1 ) {
		return NULL;
	}

	return jsobj->jsprops[idx];
}
static JSBool 	button_select(JSContext *ctx, JSObject *obj, uintN argc,jsval *argv, jsval *rval)
{

    return JS_TRUE;
}
#endif
