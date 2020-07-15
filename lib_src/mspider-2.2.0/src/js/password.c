#include <minigui/common.h>
#include <minigui/minigui.h>
#include <minigui/gdi.h>
#include <minigui/window.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "html.h"
#include "dw_page.h"
#include "dw_widget.h"
#include "mgwidget.h"
#include "dw_viewport.h"
#include "spidermonkey.h"
#include "jsmisc.h"

#ifdef JS_SUPPORT

/* local data and function definition */
enum password_prop { 
	JSP_PASSWORD_VALUE = 1,
	JSP_PASSWORD_DEFVALUE,
	JSP_PASSWORD_DISABLED,
	JSP_PASSWORD_FORM,
	JSP_PASSWORD_ID,
	JSP_PASSWORD_MAXLENGTH,
	JSP_PASSWORD_NAME,
	JSP_PASSWORD_READONLY,
	JSP_PASSWORD_SIZE,
	JSP_PASSWORD_TYPE
};

static JSBool 	password_get_property(JSContext *ctx, JSObject *obj, jsval id, jsval *vp);
static JSBool 	password_set_property(JSContext *ctx, JSObject *obj, jsval id, jsval *vp);
static char* 	get_password_value(JSContext *ctx, JSObject *obj);
static void 	set_password_value(JSContext *ctx, JSObject *obj, const char *text);
static JSBool password_blur(JSContext *ctx, JSObject *obj, uintN argc,jsval *argv, jsval *rval);
static JSBool password_click(JSContext *ctx, JSObject *obj, uintN argc,jsval *argv, jsval *rval);
static JSBool password_focus(JSContext *ctx, JSObject *obj, uintN argc,jsval *argv, jsval *rval);
static JSBool password_select(JSContext *ctx, JSObject *obj, uintN argc,jsval *argv, jsval *rval);

static int      get_password_size(JSContext *ctx, JSObject *obj);
static void 	set_password_size(JSContext *ctx, JSObject *obj, const jsdouble *size);
/* export data sructure */
const JSClass password_class = {
    "password",
    JSCLASS_HAS_PRIVATE,
    JS_PropertyStub, JS_PropertyStub,
    password_get_property, password_set_property,
    JS_EnumerateStub, JS_ResolveStub, JS_ConvertStub, JS_FinalizeStub
};

const JSPropertySpec password_props[] = {
	{ "value",	JSP_PASSWORD_VALUE,	JSPROP_ENUMERATE },
	{ "defaultValue",	JSP_PASSWORD_DEFVALUE,	JSPROP_ENUMERATE },
	{ "disabled",	JSP_PASSWORD_DISABLED,	JSPROP_ENUMERATE },
	{ "form",	JSP_PASSWORD_FORM,	JSPROP_ENUMERATE |JSPROP_READONLY},
	{ "maxLength",	JSP_PASSWORD_MAXLENGTH,	JSPROP_ENUMERATE },
	{ "id",	JSP_PASSWORD_ID,	JSPROP_ENUMERATE},
	{ "name",	JSP_PASSWORD_NAME,	JSPROP_ENUMERATE },
	{ "readOnly",	JSP_PASSWORD_READONLY,	JSPROP_ENUMERATE },
	{ "size",	JSP_PASSWORD_SIZE,	JSPROP_ENUMERATE },
	{ "type",	JSP_PASSWORD_TYPE,	JSPROP_ENUMERATE |JSPROP_READONLY},
    { NULL }
};

const JSFunctionSpec password_funcs[] = {
	{ "blur",	password_blur,		0 },
	{ "click",	password_click,	0 },
	{ "focus",	password_focus,		0 },
	{ "select",	password_select,		0 },
    { NULL }
};

const char *password_propidx[] = {
	/* props */
	"accept",		
	"accessKey",	
	"defaultValue",	
	"disabled",		
	"form",			
	"id",			
	"maxLength",	
	"name",			
	"readOnly",		
	"size",			
	"tabIndex",		
	"type",			
	"value",		
	/* events */
	"onBlur",		
	"onClick",		
	"onFocus",		
	"onKeyDown",	
	"onKeyPress",	
	"onKeyUp",		
	"onSelectStart"
}; 
const int password_propidxlen = sizeof(password_propidx)/sizeof(char*);
/* password objects property and method */
static JSBool
password_get_property(JSContext *ctx, JSObject *obj, jsval id, jsval *vp)
{
	struct jsval_property prop;
	char *str = NULL;
    jsobject *jsobj = NULL;
    int ret;
    int size;
    HWND hwnd;
	

    jsobj = JS_GetPrivate(ctx, obj);
	set_prop_undef(&prop);
    if (JSVAL_IS_STRING(id)) {
        union jsval_union v;
        jsval_to_value(ctx, &id, JSTYPE_STRING, &v);
		return JS_TRUE;
    } else if (!JSVAL_IS_INT(id)) {
        return JS_TRUE;
    }	

    hwnd = ((DwMgWidget*)jsobj->htmlobj)->window;
	switch (JSVAL_TO_INT(id)) {
	case JSP_PASSWORD_VALUE:
		str = get_password_value(ctx, obj);
		set_prop_string(&prop, (unsigned char*)str);
		break;
	case JSP_PASSWORD_DEFVALUE:
        ret=get_props_index(password_propidx,password_propidxlen,"defaultValue");
        if (jsobj->jsprops[ret]==NULL){
            str = get_password_value(ctx, obj);
            set_prop_string(&prop, (unsigned char*)str);
        }else
            set_prop_string(&prop,(unsigned char *)(jsobj->jsprops[ret])); 
		break;
	case JSP_PASSWORD_DISABLED:
        ret = get_jsobj_disabled(jsobj);
        if ( ret != -1 ) {
            set_prop_boolean(&prop, ret);
        }  
		break;
	case JSP_PASSWORD_FORM:
        if((((jsobject*)jsobj->jsparent)->jstype == jsform)
                &&(((jsobject*)jsobj->jsparent)->smobj!=NULL))
			set_prop_object(&prop,(JSObject *)(((jsobject*)jsobj->jsparent)->smobj)); 
		break;
    case JSP_PASSWORD_ID:
		if ( jsobj->jsid ) {
			set_prop_string(&prop, (unsigned char *)jsobj->jsid);
		}
		break;
	case JSP_PASSWORD_MAXLENGTH:
        ret=get_props_index(password_propidx, password_propidxlen,"maxLength");
        if (jsobj->jsprops[ret])
            set_prop_int(&prop, atoi(jsobj->jsprops[ret]));
        else
            set_prop_int(&prop, -1);
		break;
	case JSP_PASSWORD_NAME:
        set_prop_string(&prop,(unsigned char*)(jsobj->jsname));
		break;
	case JSP_PASSWORD_READONLY:
        ret=get_props_index(password_propidx, password_propidxlen,"readOnly");
        if (jsobj->jsprops[ret]== NULL)
            jsobj->jsprops[ret]=strdup("false");
        if (strcasecmp(jsobj->jsprops[ret], "false") == 0)
            set_prop_boolean(&prop, 0); 
        else
            set_prop_boolean(&prop, 1); 
		break;
	case JSP_PASSWORD_SIZE:
        size = get_password_size(ctx, obj);
        set_prop_int(&prop, size);
		break;
	case JSP_PASSWORD_TYPE:
        set_prop_string(&prop,(unsigned char*)("password"));
		break;
	default:
		return JS_TRUE;
	}
	value_to_jsval(ctx, vp, &prop);
	return JS_TRUE;
}

static JSBool
password_set_property(JSContext *ctx, JSObject *obj, jsval id, jsval *vp)
{
	union jsval_union v;
    jsobject *jsobj = NULL;
	HWND hwnd;


    jsobj = JS_GetPrivate(ctx, obj);
    hwnd = ((DwMgWidget*)jsobj->htmlobj)->window;
    if (JSVAL_IS_STRING(id)) {
        union jsval_union v;
        jsval_to_value(ctx, &id, JSTYPE_STRING, &v);
    } else if (!JSVAL_IS_INT(id)) {
        return JS_TRUE;
    }

	switch (JSVAL_TO_INT(id)) {
	case JSP_PASSWORD_VALUE:
		jsval_to_value(ctx, vp, JSTYPE_STRING, &v);
		set_password_value(ctx, obj, (char*)v.string);
		break;
	case JSP_PASSWORD_DEFVALUE:
        jsval_to_value(ctx, vp, JSTYPE_STRING, &v);
        set_jsobj_props(password_propidx,password_propidxlen,jsobj,"defaultValue",(char*)v.string);
		break;
	case JSP_PASSWORD_DISABLED:
        jsval_to_value(ctx, vp, JSTYPE_BOOLEAN, &v);
        set_jsobj_disabled(v.boolean, jsobj);
		break;
    case JSP_PASSWORD_ID:
        jsval_to_value(ctx, vp, JSTYPE_STRING, &v);
        if (jsobj->jsid != NULL)
            free(jsobj->jsid);
        jsobj->jsid = strdup((char *)(v.string));
        break;
	case JSP_PASSWORD_MAXLENGTH:
        jsval_to_value(ctx, vp, JSTYPE_STRING, &v);
        set_jsobj_props(password_propidx,password_propidxlen,jsobj,"maxLength", (char*)v.string);
        SendMessage(hwnd,EM_LIMITTEXT,atoi((char *)v.string),0);
		break;
	case JSP_PASSWORD_NAME:
        jsval_to_value(ctx, vp, JSTYPE_STRING, &v);
        if(jsobj->jsname!=NULL)
            free(jsobj->jsname); 
        jsobj->jsname=strdup((char*)(v.string));
		break;
	case JSP_PASSWORD_READONLY:
        jsval_to_value(ctx, vp, JSTYPE_BOOLEAN, &v);
        if(v.boolean){
           SendMessage(hwnd,EM_SETREADONLY,TRUE,0); 
           set_jsobj_props(password_propidx,password_propidxlen,jsobj,"readOnly", "true");
        }else{
           SendMessage(hwnd,EM_SETREADONLY,FALSE,0); 
           set_jsobj_props(password_propidx,password_propidxlen,jsobj,"readOnly", "false");
        }
		break;
	case JSP_PASSWORD_SIZE:
        jsval_to_value(ctx, vp, JSTYPE_NUMBER, &v);
        set_password_size(ctx, obj, (jsdouble *)(v.number));
        break;
	default:
		return JS_TRUE;
	}
	return JS_TRUE;
}

static JSBool password_blur(JSContext *ctx, JSObject *obj, uintN argc,jsval *argv, jsval *rval)
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
#if ((MINIGUI_MAJOR_VERSION >= 2 && MINIGUI_MICRO_VERSION >= 3 && MINIGUI_MINOR_VERSION >= 0))
        SetNullFocus(GetParent(hwnd));
#else
        SendMessage(hwnd, MSG_KILLFOCUS, 0 , 0);
#endif


	return JS_TRUE;
}
static JSBool password_click(JSContext *ctx, JSObject *obj, uintN argc,jsval *argv, jsval *rval)
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

    if (!IsWindowEnabled(hwnd))
        return JS_TRUE;
    
    PostMessage(hwnd, MSG_LBUTTONUP, 0, 0);
    SetFocusChild(hwnd);

    return JS_TRUE;
}
static JSBool password_select(JSContext *ctx, JSObject *obj, uintN argc,jsval *argv, jsval *rval)
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
static JSBool password_focus(JSContext *ctx, JSObject *obj, uintN argc,jsval *argv, jsval *rval)
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
static char * get_password_value(JSContext *ctx, JSObject *obj)
{
	HWND hwnd;
	static char buf[0x100];
	jsobject *jsobj = NULL;

	jsobj = JS_GetPrivate(ctx, obj);
	if ( !jsobj || !(jsobj->htmlobj) ) {
		return NULL;
	}

	hwnd = ((DwMgWidget*)jsobj->htmlobj)->window;
	if ( !hwnd ) {
		return NULL;	
	}

	GetWindowText(hwnd, buf, 0x100);

	return buf;
}

static void set_password_value(JSContext *ctx, JSObject *obj, const char *text)
{
	HWND hwnd;
	jsobject *jsobj = NULL;

	jsobj = JS_GetPrivate(ctx, obj);
	if ( !jsobj || !(jsobj->htmlobj) ) {
		return;
	}

	hwnd = ((DwMgWidget*)jsobj->htmlobj)->window;
	if ( !hwnd ) {
		return;	
	}

	SetWindowText(hwnd, text);
}

const char* getpasswordeventstr(jsobject *jsobj, int rc)
{
	int idx = -1;
	
    switch ( rc ) {
        case EN_CLICKED:
            idx = get_props_index(password_propidx, password_propidxlen, "onClick");
            break;
        case EN_SETFOCUS:
            idx = get_props_index(password_propidx, password_propidxlen, "onFocus");
            break;
        case EN_KILLFOCUS:
            idx = get_props_index(password_propidx, password_propidxlen, "onBlur");
            break;
#if 0
        case MSG_KEYDOWN:
            idx = get_props_index(password_propidx, password_propidxlen, "onKeyDown");
            break;
        case
            idx = get_props_index(password_propidx, password_propidxlen, "onKeyPress");
            break;
        case MSG_KEYUP:
            idx = get_props_index(password_propidx, password_propidxlen, "onKeyUp");
            break;
        case :
            idx = get_props_index(password_propidx, password_propidxlen, "onSelectStart");
            break; 
#endif
        default:
            break;	
	}

	if ( idx == -1 ) {
		return NULL;
	}

	return jsobj->jsprops[idx];
}
static int	get_password_size(JSContext *ctx, JSObject *obj)
{
	HWND hwnd;
    int size;
    RECT rec;
    static int pw, ph;

	jsobject *jsobj = NULL;

	jsobj = JS_GetPrivate(ctx, obj);
	if ( !jsobj || !(jsobj->htmlobj) ) {
		return 0;
	}
	hwnd = ((DwMgWidget*)jsobj->htmlobj)->window;
	if ( !hwnd ) {
		return 0;	
	}

	GetWindowRect(hwnd, &rec);

    size =rec.right - rec.left;
    Dw_MgWidget_get_text_metrics ("0", 1,
            &pw, &ph);

    size = (size - ADD_LENGTH)/pw;

	return size;
}
static void	set_password_size(JSContext *ctx, JSObject *obj, const jsdouble *size)
{
	HWND hwnd;
    static RECT rec;
    static int x, y, w, h, pw, ph;
    DwPage *page;
    DwMgWidget * mgwidget;
    DwWidget *widget;
    DwViewport * viewport;

    jsobject *jsobj = NULL;

	jsobj = JS_GetPrivate(ctx, obj);
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
    w = *size*pw+ADD_LENGTH;
    h = ph+ADD_LENGTH;
    
    mgwidget->size.cx = w;
    mgwidget->size.cy = h;
    widget = &(mgwidget->dw_widget);
    viewport = widget->viewport;
 
    p_Dw_widget_queue_resize(widget, 0, TRUE);
    
    page = (DwPage *)(jsobj->pvar1);
    Dw_page_rewrap (page);
    
    InvalidateRect (viewport->hwnd, NULL, TRUE);
}

#endif
