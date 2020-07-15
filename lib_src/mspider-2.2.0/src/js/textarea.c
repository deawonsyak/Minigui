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

enum textarea_prop{
	JSP_TAREA_ACCESSKEY,
    JSP_TAREA_COLS,
    JSP_TAREA_DEFAULTVALUE,
    JSP_TAREA_DISABLED,
    JSP_TAREA_FORM,
    JSP_TAREA_ID,
    JSP_TAREA_NAME,
    JSP_TAREA_READONLY,
    JSP_TAREA_ROWS,
    JSP_TAREA_TABINDEX,
    JSP_TAREA_TYPE,
    JSP_TAREA_VALUE
};


static JSBool	textarea_get_property(JSContext *ctx, JSObject *obj, jsval id, jsval *vp);
static JSBool 	textarea_set_property(JSContext *ctx, JSObject *obj, jsval id, jsval *vp);
static char* 	get_textarea_value(JSContext *ctx, JSObject *obj);
static void 	set_textarea_value(JSContext *ctx, JSObject *obj, const char *text);
static JSBool textarea_blur(JSContext *ctx, JSObject *obj, uintN argc,jsval *argv, jsval *rval);
static JSBool textarea_click(JSContext *ctx, JSObject *obj, uintN argc,jsval *argv, jsval *rval);
static JSBool textarea_focus(JSContext *ctx, JSObject *obj, uintN argc,jsval *argv, jsval *rval);
static JSBool textarea_select(JSContext *ctx, JSObject *obj, uintN argc,jsval *argv, jsval *rval);

static int 	    get_textarea_cols(JSContext *ctx, JSObject *obj);
static void 	set_textarea_cols(JSContext *ctx, JSObject *obj, const jsdouble *cols);
static int  	get_textarea_rows(JSContext *ctx, JSObject *obj);
static void  	set_textarea_rows(JSContext *ctx, JSObject *obj, const jsdouble *rows);

/* export data sructure */
const JSClass textarea_class = {
    "textarea",
    JSCLASS_HAS_PRIVATE,
    JS_PropertyStub, JS_PropertyStub,
    textarea_get_property, textarea_set_property,
    JS_EnumerateStub, JS_ResolveStub, JS_ConvertStub, JS_FinalizeStub
};

const JSPropertySpec textarea_props[] = {
	{ "accessKey",	    JSP_TAREA_ACCESSKEY,		JSPROP_ENUMERATE },
	{ "cols",			JSP_TAREA_COLS,		        JSPROP_ENUMERATE },
	{ "defaultValue",	JSP_TAREA_DEFAULTVALUE,		JSPROP_ENUMERATE },
	{ "disabled",		JSP_TAREA_DISABLED,		    JSPROP_ENUMERATE },
	{ "form",			JSP_TAREA_FORM,		        JSPROP_ENUMERATE |JSPROP_READONLY },
	{ "id",			    JSP_TAREA_ID,		        JSPROP_ENUMERATE },
	{ "name",			JSP_TAREA_NAME,		        JSPROP_ENUMERATE },
	{ "readOnly",		JSP_TAREA_READONLY,		    JSPROP_ENUMERATE},
	{ "rows",			JSP_TAREA_ROWS,		        JSPROP_ENUMERATE },
	{ "tabIndex",		JSP_TAREA_TABINDEX,		    JSPROP_ENUMERATE },
	{ "type",			JSP_TAREA_TYPE,		        JSPROP_ENUMERATE |JSPROP_READONLY },
	{ "value",			JSP_TAREA_VALUE,		    JSPROP_ENUMERATE },
    { NULL }
};

const JSFunctionSpec textarea_funcs[] = {
	{ "blur",	textarea_blur,		0 },
	{ "click",	textarea_click,		0 },
	{ "focus",	textarea_focus, 	0 },
	{ "select",	textarea_select,	0 },
    { NULL }
};

const char *textarea_propidx[] = {
	/* props */
	"defaultValue",		
	"disabled",		
	"form",			
	"name",
	"readOnly",
	"tabIndex",
	"type",
    "cols",
	"rows",
	"value",
	/* events */
	"onBlur",
	"onChange",
	"onClick",
	"onFocus",
	"onKeyDown",
	"onKeyPress",
	"onKeyUp",
	"onSelect"
};
const int textarea_propidxlen = sizeof(textarea_propidx)/sizeof(char*);
/* textarea objects property and method */

static JSBool
textarea_get_property(JSContext *ctx, JSObject *obj, jsval id, jsval *vp)
{
    int ret, cols, rows, index;
    struct jsval_property prop;
    char *str = NULL;
    jsobject *jsobj = NULL;

    jsobj = JS_GetPrivate(ctx, obj);
    set_prop_undef(&prop);
    if (JSVAL_IS_STRING(id)){
        union jsval_union v;
        jsval_to_value(ctx, &id, JSTYPE_STRING, &v);
        return JS_TRUE;
    }else if (!JSVAL_IS_INT(id)){
        return JS_TRUE;
    }
    switch (JSVAL_TO_INT(id)){
        case JSP_TAREA_ACCESSKEY:
            break;
        case JSP_TAREA_COLS:
            cols = get_textarea_cols(ctx, obj);
            set_prop_int(&prop, cols);
            break;
        case JSP_TAREA_DEFAULTVALUE:
            index = get_props_index(textarea_propidx, textarea_propidxlen, "defaultValue");
            set_prop_string(&prop, (unsigned char*)jsobj->jsprops[index]);
            break;
        case JSP_TAREA_DISABLED:
            ret = get_jsobj_disabled(jsobj);
            if (ret != -1){
                set_prop_boolean(&prop, ret);
            }
            break;
        case JSP_TAREA_FORM:
            if((((jsobject *)jsobj->jsparent)->jstype == jsform) &&
                    (((jsobject *)jsobj->jsparent)->smobj != NULL))
                set_prop_object(&prop, (JSObject *)(((jsobject *)jsobj->jsparent)->smobj));
            break;
        case JSP_TAREA_ID:
            set_prop_string(&prop, (unsigned char*)jsobj->jsid);
            break;
        case JSP_TAREA_NAME:
            set_prop_string(&prop, (unsigned char*)jsobj->jsname);
            break;
        case JSP_TAREA_READONLY:
            ret = 0;
            set_prop_boolean (&prop, ret);
            break;
        case JSP_TAREA_ROWS:
            rows = get_textarea_rows(ctx, obj);
            set_prop_int(&prop, rows);
            break;
        case JSP_TAREA_TABINDEX:
            break;
        case JSP_TAREA_TYPE:
            set_prop_string(&prop, (unsigned char *)("textarea"));
            break;
        case JSP_TAREA_VALUE:
            str = get_textarea_value(ctx, obj);
            set_prop_astring(&prop, (unsigned char*)str);
            break;
        default:
            return JS_TRUE;
    }
    
    value_to_jsval (ctx, vp, &prop);
    return JS_TRUE;
}

static JSBool
textarea_set_property(JSContext *ctx, JSObject *obj, jsval id, jsval *vp)
{
    union jsval_union v;
    HWND hwnd;
    jsobject *jsobj = NULL;
    int index;

    jsobj = JS_GetPrivate(ctx, obj);
    hwnd = ((DwMgWidget *)jsobj->htmlobj)->window;

    if (JSVAL_IS_STRING(id)){
        jsval_to_value(ctx, &id, JSTYPE_STRING, &v);
        return JS_TRUE;
    }else if (!JSVAL_IS_INT(id)){
        return JS_TRUE;
    }

    switch (JSVAL_TO_INT(id)){
        case JSP_TAREA_ACCESSKEY:
            break;
        case JSP_TAREA_COLS:
            jsval_to_value(ctx, vp, JSTYPE_NUMBER, &v);
            set_textarea_cols(ctx, obj, (jsdouble *)(v.number));
            break;
        case JSP_TAREA_DEFAULTVALUE:
            jsval_to_value(ctx, vp, JSTYPE_STRING, &v);
            index = get_props_index(textarea_propidx, textarea_propidxlen, "defaultValue");
            if (jsobj->jsprops[index]!=NULL) g_free (jsobj->jsprops[index]);
            jsobj->jsprops[index] = strdup((char *)(v.string));
            break;
        case JSP_TAREA_DISABLED:
            jsval_to_value(ctx, vp, JSTYPE_BOOLEAN, &v);
            set_jsobj_disabled(v.boolean, jsobj);
            break;
        case JSP_TAREA_ID:
            jsval_to_value(ctx, vp, JSTYPE_STRING, &v);
            if (jsobj->jsid != NULL) g_free(jsobj->jsid);
            jsobj->jsid = strdup((char *)(v.string));
            break;
        case JSP_TAREA_NAME:
            jsval_to_value(ctx, vp, JSTYPE_STRING, &v);
            if (jsobj->jsname != NULL) g_free(jsobj->jsname);
            jsobj->jsname = strdup((char *)(v.string));
            break;
        case JSP_TAREA_READONLY:
            jsval_to_value(ctx, vp, JSTYPE_BOOLEAN, &v);
            if (v.boolean)
                SendMessage (hwnd, EM_SETREADONLY, TRUE, 0);
            else
                SendMessage (hwnd, EM_SETREADONLY, FALSE, 0);
            break;
        case JSP_TAREA_ROWS:
            jsval_to_value(ctx, vp, JSTYPE_NUMBER, &v);
            set_textarea_rows(ctx, obj, (jsdouble *)(v.number));
            break;
        case JSP_TAREA_TABINDEX:
            break;
        case JSP_TAREA_VALUE:
            jsval_to_value(ctx, vp, JSTYPE_STRING, &v);
            set_textarea_value(ctx, obj, (char *)(v.string));
            break;
        default:
            return JS_TRUE;
    }
    return JS_TRUE;
}

static JSBool textarea_blur(JSContext *ctx, JSObject *obj, uintN argc,jsval *argv, jsval *rval)
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
static JSBool textarea_click(JSContext *ctx, JSObject *obj, uintN argc,jsval *argv, jsval *rval)
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

    PostMessage(hwnd, MSG_LBUTTONDOWN, 0, 0);
    SetFocusChild(hwnd);
    
    return JS_TRUE;
}
static JSBool textarea_focus(JSContext *ctx, JSObject *obj, uintN argc,jsval *argv, jsval *rval)
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
        SetFocusChild(hwnd);
    return JS_TRUE;
}
static JSBool textarea_select(JSContext *ctx, JSObject *obj, uintN argc,jsval *argv, jsval *rval)
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
    
    PostMessage(hwnd, EM_SELECTALL, 0, 0);
    SetFocusChild(hwnd);
    return JS_TRUE;
}


static char* 	get_textarea_value(JSContext *ctx, JSObject *obj)
{
	HWND hwnd;
    int bsize;
	char* buf = NULL;
    
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

static void 	set_textarea_value(JSContext *ctx, JSObject *obj, const char *text)
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
const char* gettextareaeventstr(jsobject *jsobj, int rc)
{
	int idx = -1;

	switch ( rc ) {
	case EN_CLICKED:
		idx = get_props_index(textarea_propidx, textarea_propidxlen, "onClick");
		break;
	case EN_SETFOCUS:
		idx = get_props_index(textarea_propidx, textarea_propidxlen, "onFocus");
		break;
	case EN_KILLFOCUS:
		idx = get_props_index(textarea_propidx, textarea_propidxlen, "onBlur");
		break;
#if ((MINIGUI_MAJOR_VERSION == 1 && MINIGUI_MICRO_VERSION == 9 && MINIGUI_MINOR_VERSION == 6)||\
        ((MINIGUI_MAJOR_VERSION >= 2 && MINIGUI_MICRO_VERSION >= 3 && MINIGUI_MINOR_VERSION >= 0)))
	case EN_CONTCHANGED:
		idx = get_props_index(textarea_propidx, textarea_propidxlen, "onChange");
		break;
	case EN_SELCHANGED:
		idx = get_props_index(textarea_propidx, textarea_propidxlen, "onSelect");
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

static int	get_textarea_cols(JSContext *ctx, JSObject *obj)
{
	HWND hwnd;
    int cols;
    RECT rec;
    static int pw,ph;

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
    
    cols =rec.right - rec.left;
    Dw_MgWidget_get_text_metrics ("0", 1,
            &pw, &ph);

    cols = (cols - APPEND_LENGTH)/pw;

	return cols;
}
static void	set_textarea_cols(JSContext *ctx, JSObject *obj, const jsdouble *cols)
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
    w = *cols*pw+APPEND_LENGTH;
    h = rec.bottom-rec.top;
    
    mgwidget->size.cx = w;
    mgwidget->size.cy = h;
    widget = &(mgwidget->dw_widget);
    viewport = widget->viewport;
 
    p_Dw_widget_queue_resize(widget, 0, TRUE);
    
    page = (DwPage *)(jsobj->pvar1);
    Dw_page_rewrap (page);
    
    InvalidateRect (viewport->hwnd, NULL, TRUE);
}
static int	get_textarea_rows(JSContext *ctx, JSObject *obj)
{
	HWND hwnd;
    int rows;
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

    rows =rec.bottom - rec.top;
    Dw_MgWidget_get_text_metrics ("0", 1,
            &pw, &ph);

    rows = (rows - APPEND_LENGTH)/ph;

	return rows;
}
static void	set_textarea_rows(JSContext *ctx, JSObject *obj, const jsdouble *rows)
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
    w = rec.right-rec.left;
    h = *rows*ph+APPEND_LENGTH;
    
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
