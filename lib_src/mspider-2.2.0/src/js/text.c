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

extern void Dw_page_rewrap            (DwPage *page);

/* local data and function definition */
enum text_prop { 
	JSP_TEXT_DFTVALUE = 1,
	JSP_TEXT_DISABLED,
	JSP_TEXT_FORM,
	JSP_TEXT_ID,
	JSP_TEXT_MAXLENGTH,
	JSP_TEXT_NAME,
	JSP_TEXT_READONLY,
	JSP_TEXT_SIZE,
	JSP_TEXT_TYPE,
	JSP_TEXT_VALUE
};

static JSBool 	text_get_property(JSContext *ctx, JSObject *obj, jsval id, jsval *vp);
static JSBool 	text_set_property(JSContext *ctx, JSObject *obj, jsval id, jsval *vp);
static char* 	get_text_value(JSContext *ctx, JSObject *obj);
static void 	set_text_value(JSContext *ctx, JSObject *obj, const char *text);
static JSBool	text_blur(JSContext *ctx, JSObject *obj, uintN argc,jsval *argv, jsval *rval);
static JSBool 	text_focus(JSContext *ctx, JSObject *obj, uintN argc,jsval *argv, jsval *rval);
static JSBool	text_select(JSContext *ctx, JSObject *obj, uintN argc,jsval *argv, jsval *rval);
static JSBool   text_click(JSContext *ctx, JSObject *obj, uintN argc,jsval *argv, jsval *rval);

static int      get_text_size(JSContext *ctx, JSObject *obj);
static void 	set_text_size(JSContext *ctx, JSObject *obj, const jsdouble *size);

/* export data sructure */
const JSClass text_class = {
    "text",
    JSCLASS_HAS_PRIVATE,
    JS_PropertyStub, JS_PropertyStub,
    text_get_property, text_set_property,
    JS_EnumerateStub, JS_ResolveStub, JS_ConvertStub, JS_FinalizeStub
};

const JSPropertySpec text_props[] = {
	{ "defaultValue",	JSP_TEXT_DFTVALUE,	JSPROP_ENUMERATE },
	{ "disabled",		JSP_TEXT_DISABLED,	JSPROP_ENUMERATE },
	{ "form",			JSP_TEXT_FORM,		JSPROP_ENUMERATE | JSPROP_READONLY},
	{ "maxLength",		JSP_TEXT_MAXLENGTH,	JSPROP_ENUMERATE },
	{ "id",		        JSP_TEXT_ID,	JSPROP_ENUMERATE },
	{ "name",			JSP_TEXT_NAME,		JSPROP_ENUMERATE },
	{ "readOnly",		JSP_TEXT_READONLY, 	JSPROP_ENUMERATE },
	{ "size",			JSP_TEXT_SIZE,		JSPROP_ENUMERATE },
	{ "type",			JSP_TEXT_TYPE, 		JSPROP_ENUMERATE |JSPROP_READONLY },
	{ "value",			JSP_TEXT_VALUE, 	JSPROP_ENUMERATE },
    { NULL }
};

const JSFunctionSpec text_funcs[] = {
	{ "blur",	text_blur,		0 },
	{ "focus",	text_focus,		0 },
	{ "select",	text_select,	0 },
	{ "click",	text_click, 	0 },
    { NULL }
};

const char *text_propidx[] = {
	/* props */
	"accept",		
	"accessKey",	
	"align",		
	"alt",			
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
	"onChange",		
	"onClick",		
	"onFocus",		
	"onKeyDown",	
	"onKeyPress",	
	"onKeyUp",		
	"onSelect",		
	"onSelectStart"
}; 
const int text_propidxlen = sizeof(text_propidx)/sizeof(char*);
/* text objects property and method */
static JSBool
text_get_property(JSContext *ctx, JSObject *obj, jsval id, jsval *vp)
{
	struct jsval_property prop;
	char *str = NULL;
	int ret; 
    int size;
    jsobject *jsobj = NULL;
    union jsval_union v;
	HWND hwnd;

    jsobj = JS_GetPrivate(ctx, obj);
	set_prop_undef(&prop);
    if (JSVAL_IS_STRING(id)) {
        jsval_to_value(ctx, &id, JSTYPE_STRING, &v);
		return JS_TRUE;
    } else if (!JSVAL_IS_INT(id)) {
        return JS_TRUE;
    }	

    hwnd = ((DwMgWidget*)jsobj->htmlobj)->window;
	switch (JSVAL_TO_INT(id)) {
        case JSP_TEXT_DFTVALUE:
            ret = get_props_index(text_propidx, text_propidxlen, "defaultValue");
            if (jsobj->jsprops[ret]==NULL){
                str = get_text_value(ctx, obj);
                set_prop_astring(&prop, (unsigned char*)str);
            }
            else
                set_prop_string(&prop, (unsigned char*)jsobj->jsprops[ret]);
            break;
        case JSP_TEXT_VALUE:
            str = get_text_value(ctx, obj);
            set_prop_astring(&prop, (unsigned char*)str);
            break;
        case JSP_TEXT_DISABLED:
            ret = get_jsobj_disabled(jsobj);
            if ( ret != -1 ) {
                set_prop_boolean(&prop, ret);
            }  
            break;
        case JSP_TEXT_FORM:
            if((((jsobject*)jsobj->jsparent)->jstype == jsform)
                    &&(((jsobject*)jsobj->jsparent)->smobj!=NULL))
                set_prop_object(&prop,(JSObject *)(((jsobject*)jsobj->jsparent)->smobj)); 
            break;
        case JSP_TEXT_MAXLENGTH:
            ret=get_props_index(text_propidx, text_propidxlen,"maxLength");
            if (jsobj->jsprops[ret])
                set_prop_int(&prop, atoi(jsobj->jsprops[ret]));
            else
                set_prop_int(&prop, -1);
            break;
        case JSP_TEXT_ID:
            if ( jsobj->jsid ) {
                set_prop_string(&prop, (unsigned char *)jsobj->jsid);
            }
            break;
        case JSP_TEXT_NAME:
            set_prop_string(&prop, (unsigned char*)jsobj->jsname);
            break;
        case JSP_TEXT_READONLY:
            ret=get_props_index(text_propidx, text_propidxlen,"readOnly");
            if (jsobj->jsprops[ret]== NULL)
                jsobj->jsprops[ret]=strdup("false");
            if (strcasecmp(jsobj->jsprops[ret], "false") == 0)
                set_prop_boolean(&prop, 0); 
            else
                set_prop_boolean(&prop, 1); 
            break;
        case JSP_TEXT_SIZE:
            size = get_text_size(ctx, obj);
            set_prop_int(&prop, size);
            break;
        case JSP_TEXT_TYPE:
            set_prop_string(&prop,(unsigned char*)("text"));
            break;
        default:
            return JS_TRUE;
	}

	value_to_jsval(ctx, vp, &prop);
	return JS_TRUE;
}

static JSBool
text_set_property(JSContext *ctx, JSObject *obj, jsval id, jsval *vp)
{
	union jsval_union v;
    jsobject *jsobj = NULL;
	HWND hwnd;
    int index;

    jsobj = JS_GetPrivate(ctx, obj);
    hwnd = ((DwMgWidget*)jsobj->htmlobj)->window;

    if (JSVAL_IS_STRING(id)) {
        union jsval_union v;
        jsval_to_value(ctx, &id, JSTYPE_STRING, &v);
    } else if (!JSVAL_IS_INT(id)) {
        return JS_TRUE;
    }

	switch (JSVAL_TO_INT(id)) {
        case JSP_TEXT_DFTVALUE:
            jsval_to_value(ctx, vp, JSTYPE_STRING, &v);
            index = get_props_index(text_propidx, text_propidxlen, "defaultValue");
            if (jsobj->jsprops[index]!=NULL) g_free (jsobj->jsprops[index]);
            jsobj->jsprops[index] = strdup((char *)(v.string));
            break;
        case JSP_TEXT_VALUE:
            jsval_to_value(ctx, vp, JSTYPE_STRING, &v);
            set_text_value(ctx, obj, (char*)(v.string));
            break;
        case JSP_TEXT_DISABLED:
            jsval_to_value(ctx, vp, JSTYPE_BOOLEAN, &v);
            set_jsobj_disabled(v.boolean, jsobj);
            break;
        case JSP_TEXT_MAXLENGTH:
            jsval_to_value(ctx, vp, JSTYPE_STRING, &v);
            set_jsobj_props(text_propidx,text_propidxlen,jsobj,"maxLength", (char*)v.string);
            SendMessage(hwnd, EM_LIMITTEXT, atoi((char *)v.string), 0);
            break;
        case JSP_TEXT_NAME:
            jsval_to_value(ctx, vp, JSTYPE_STRING, &v);
            if(jsobj->jsname!=NULL) g_free(jsobj->jsname); 
            jsobj->jsname=strdup((char*)(v.string));
            break;
        case JSP_TEXT_ID:
            jsval_to_value(ctx, vp, JSTYPE_STRING, &v);
            if (jsobj->jsid != NULL) g_free(jsobj->jsid);
            jsobj->jsid = strdup((char *)(v.string));
            break;
        case JSP_TEXT_READONLY:
            jsval_to_value(ctx, vp, JSTYPE_BOOLEAN, &v);
            if(v.boolean) {
               SendMessage(hwnd,EM_SETREADONLY,TRUE,0); 
               set_jsobj_props(text_propidx,text_propidxlen,jsobj,"readOnly", "true");
            } else {    
               SendMessage(hwnd,EM_SETREADONLY,FALSE,0); 
               set_jsobj_props(text_propidx,text_propidxlen,jsobj,"readOnly", "false");
            }
            break;
        case JSP_TEXT_SIZE:
            jsval_to_value(ctx, vp, JSTYPE_NUMBER, &v);
            set_text_size(ctx, obj, (jsdouble *)(v.number));
            break;
        default:
            return JS_TRUE;
	}

	return JS_TRUE;
}

static char * get_text_value(JSContext *ctx, JSObject *obj)
{
	HWND hwnd;
    int bsize;
	char *buf = NULL;
	jsobject *jsobj = NULL;

	jsobj = JS_GetPrivate(ctx, obj);
	if ( !jsobj || !(jsobj->htmlobj) ) {
		return NULL;
	}
	hwnd = ((DwMgWidget*)jsobj->htmlobj)->window;
	if ( !hwnd ) {
		return NULL;	
	}

    bsize = GetWindowTextLength(hwnd);
    buf = malloc (bsize+1);
    memset (buf , 0 , bsize+1);

	GetWindowText(hwnd, buf, bsize);

	return buf;
}

static void set_text_value(JSContext *ctx, JSObject *obj, const char *text)
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

const char* gettexteventstr(jsobject *jsobj, int rc)
{
	int idx = -1;

	switch ( rc ) {
        case EN_CLICKED:
            idx = get_props_index(text_propidx, text_propidxlen, "onClick");
            break;
        case EN_SETFOCUS:
            idx = get_props_index(text_propidx, text_propidxlen, "onFocus");
            break;
        case EN_KILLFOCUS:
            idx = get_props_index(text_propidx, text_propidxlen, "onBlur");
            break;
#if ((MINIGUI_MAJOR_VERSION == 1 && MINIGUI_MICRO_VERSION == 9 && MINIGUI_MINOR_VERSION == 6)||\
        ((MINIGUI_MAJOR_VERSION >= 2 && MINIGUI_MICRO_VERSION >= 3 && MINIGUI_MINOR_VERSION >= 0)))
        case EN_CONTCHANGED:
            idx = get_props_index(text_propidx, text_propidxlen, "onChange");
            break;
        case EN_SELCHANGED:
            idx = get_props_index(text_propidx, text_propidxlen, "onSelect");
            break;
#endif
#if 0
        case :
            idx = get_props_index(text_propidx, text_propidxlen, "onKeyDown");
            break;
        case :
            idx = get_props_index(text_propidx, text_propidxlen, "onKeyPress");
            break;
        case :
            idx = get_props_index(text_propidx, text_propidxlen, "onKeyUp");
            break;
        case :
            idx = get_props_index(text_propidx, text_propidxlen, "onSelectStart");
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

static JSBool
text_blur(JSContext *ctx, JSObject *obj, uintN argc,jsval *argv, jsval *rval)
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

static JSBool text_click(JSContext *ctx, JSObject *obj, uintN argc,jsval *argv, jsval *rval)
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
static JSBool
text_focus(JSContext *ctx, JSObject *obj, uintN argc,jsval *argv, jsval *rval)
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

static JSBool
text_select(JSContext *ctx, JSObject *obj, uintN argc,jsval *argv, jsval *rval)
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
    
    PostMessage(hwnd, MSG_LBUTTONDBLCLK, 0, 0);
    SetFocusChild(hwnd);

    return JS_TRUE;
}
static int	get_text_size(JSContext *ctx, JSObject *obj)
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
static void	set_text_size(JSContext *ctx, JSObject *obj, const jsdouble *size)
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
