#include <minigui/common.h>
#include <minigui/minigui.h>
#include <minigui/gdi.h>
#include <minigui/window.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "html.h"
#include "dw_widget.h"
#include "mgwidget.h"
#include "spidermonkey.h"
#include "jsmisc.h"

#ifdef JS_SUPPORT

/* extern select's objects */
extern const JSClass option_class;
extern const JSPropertySpec option_props[];
extern const JSFunctionSpec option_funcs[]; 
/* local data and function definition */
enum select_prop { 
	JSP_SEL_VALUE,
	JSP_SEL_SELIDX = 1,
	JSP_SEL_DISABLED,
	JSP_SEL_FORM,
	JSP_SEL_ID,
	JSP_SEL_LENGTH,
	JSP_SEL_MULTIPLE,
	JSP_SEL_NAME,
	JSP_SEL_SIZE,
	JSP_SEL_TYPE
};

static JSBool	select_get_property(JSContext *ctx, JSObject *obj, jsval id, jsval *vp);
static JSBool 	select_set_property(JSContext *ctx, JSObject *obj, jsval id, jsval *vp);
static int 		get_select_idx(JSContext *ctx, JSObject *obj);
static void 	set_select_idx(JSContext *ctx, JSObject *obj, int idx);
static char* 	get_select_value(JSContext *ctx, JSObject *obj);
static JSBool select_blur(JSContext *ctx, JSObject *obj, uintN argc,jsval *argv, jsval *rval);
static JSBool select_focus(JSContext *ctx, JSObject *obj, uintN argc,jsval *argv, jsval *rval);
static JSBool select_remove(JSContext *ctx, JSObject *obj, uintN argc,jsval *argv, jsval *rval);

static void	set_select_length(JSContext *ctx, JSObject *obj, const jsdouble *length);
static int 	get_select_length(JSContext *ctx, JSObject *obj);
/* export data sructure */
const JSClass select_class = {
    "select",
    JSCLASS_HAS_PRIVATE,
    JS_PropertyStub, JS_PropertyStub,
    select_get_property, select_set_property,
    JS_EnumerateStub, JS_ResolveStub, JS_ConvertStub, JS_FinalizeStub
};

const JSPropertySpec select_props[] = {
	{ "selectedIndex",	JSP_SEL_SELIDX,		JSPROP_ENUMERATE },
	{ "value",			JSP_SEL_VALUE,		JSPROP_ENUMERATE },
	{ "disabled",		JSP_SEL_DISABLED,		JSPROP_ENUMERATE },
	{ "form",			JSP_SEL_FORM,		JSPROP_ENUMERATE |JSPROP_READONLY },
	{ "id",			    JSP_SEL_ID,		JSPROP_ENUMERATE },
	{ "length",			JSP_SEL_LENGTH,		JSPROP_ENUMERATE },
	{ "multiple",		JSP_SEL_MULTIPLE,		JSPROP_ENUMERATE |JSPROP_READONLY},
	{ "name",			JSP_SEL_NAME,		JSPROP_ENUMERATE },
	{ "size",			JSP_SEL_SIZE,		JSPROP_ENUMERATE },
	{ "type",			JSP_SEL_TYPE,		JSPROP_ENUMERATE |JSPROP_READONLY },
    { NULL }
};

const JSFunctionSpec select_funcs[] = {
	{ "blur",	select_blur,		0 },
	{ "focus",	select_focus,	0 },
	{ "remove",	select_remove,	1 },
    { NULL }
};

const char *select_propidx[] = {
	/* props */
	"accessKey",	
	"align",		
	"disabled",		
	"form",			
	"id",
	"length",
	"multiple",
	"name",
	"selectedIndex",
	"size",
	"tabIndex",
	"type",
	"value",
	/* events */
	"onBlur",
	"onChange",
	"onClick",
	"onFocus"
};
const int select_propidxlen = sizeof(select_propidx)/sizeof(char*);
/* select objects property and method */
static JSBool
select_get_property(JSContext *ctx, JSObject *obj, jsval id, jsval *vp)
{
	int idx;
    int length,ret;
	char *value = NULL;
	struct jsval_property prop;
	jsobject *jsobj = NULL;
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
	case JSP_SEL_SELIDX:
		idx = get_select_idx(ctx, obj);
		set_prop_int(&prop, idx);
		break;
	case JSP_SEL_VALUE:
		value =  get_select_value(ctx, obj);
		if ( value ) {
			set_prop_string(&prop, (unsigned char*)value);
		}
		break;
	case JSP_SEL_DISABLED:
		ret = get_jsobj_disabled(jsobj);
		if ( ret != -1 ) {
			set_prop_boolean(&prop, ret); 
        }
		break;
	case JSP_SEL_FORM:
        if((((jsobject*)jsobj->jsparent)->jstype == jsform)
                &&(((jsobject*)jsobj->jsparent)->smobj!=NULL))
			set_prop_object(&prop,(JSObject *)(((jsobject*)jsobj->jsparent)->smobj)); 
		break;
	case JSP_SEL_ID:
	   	set_prop_string(&prop, (unsigned char*)jsobj->jsid); 
		break;
	case JSP_SEL_LENGTH:
        length =  get_select_length(ctx, obj);
		if ( length ) {
	        set_prop_int(&prop, length);
		}
        break;
	case JSP_SEL_MULTIPLE:
        ret=get_props_index(select_propidx,select_propidxlen,"multiple");
        if (jsobj->jsprops[ret] == NULL)
            jsobj->jsprops[ret]= strdup("false");
        if (strcasecmp(jsobj->jsprops[ret], "false") == 0)
            set_prop_boolean(&prop,0);
        else
            set_prop_boolean(&prop,1);
		break;
	case JSP_SEL_NAME:
        set_prop_string(&prop, (unsigned char*)jsobj->jsname);
		break;
	case JSP_SEL_SIZE:
        ret=get_props_index(select_propidx,select_propidxlen,"size");
        if (jsobj->jsprops[ret] == NULL){
            if ((int)jsobj->pvar3 == MSPIDER_HTML_INPUT_SELECT)
                set_prop_int(&prop, -1);
            else
                set_prop_int(&prop, 10);
        }else
            set_prop_int(&prop,atoi(jsobj->jsprops[ret]));
		break;
	case JSP_SEL_TYPE:
        set_prop_string(&prop, (unsigned char *)("select"));
		break;
	default:
		return JS_TRUE;
	}

	value_to_jsval(ctx, vp, &prop);
	return JS_TRUE;
}

static JSBool
select_set_property(JSContext *ctx, JSObject *obj, jsval id, jsval *vp)
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
	case JSP_SEL_SELIDX:
		jsval_to_value(ctx, vp, JSTYPE_NUMBER, &v);
		set_select_idx(ctx, obj, (int)*v.number);
		break;
    case JSP_SEL_DISABLED:
		jsval_to_value(ctx, vp, JSTYPE_BOOLEAN, &v);
		set_jsobj_disabled(v.boolean, jsobj);
		break;
    case JSP_SEL_ID:
        jsval_to_value(ctx, vp, JSTYPE_STRING, &v);
        if(jsobj->jsid!=NULL) g_free(jsobj->jsid);
        jsobj->jsid=strdup((char*)(v.string)); 
		break;
    case JSP_SEL_NAME:
        jsval_to_value(ctx, vp, JSTYPE_STRING, &v);
        if(jsobj->jsname!=NULL) g_free(jsobj->jsname); 
        jsobj->jsname=strdup((char*)(v.string));
		break;
    case JSP_SEL_SIZE:
		break;
	case JSP_SEL_LENGTH:
        jsval_to_value(ctx, vp, JSTYPE_NUMBER, &v);
        set_select_length(ctx, obj, (jsdouble *)(v.number));
        break;
	default:
		return JS_TRUE;
	}
	return JS_TRUE;
}

static JSBool select_blur(JSContext *ctx, JSObject *obj, uintN argc,jsval *argv, jsval *rval)
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
static JSBool select_focus(JSContext *ctx, JSObject *obj, uintN argc,jsval *argv, jsval *rval)
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
static JSBool select_remove(JSContext *ctx, JSObject *obj, uintN argc,jsval *argv, jsval *rval)
{
	HWND hwnd;
    mSpiderHtmlInputType type;
	union jsval_union v;
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
    
    jsval_to_value(ctx, &argv[0], JSTYPE_STRING, &v);

     type = (mSpiderHtmlInputType)jsobj->pvar3;
     
     if (type == MSPIDER_HTML_INPUT_SELECT)
         SendMessage(hwnd, CB_DELETESTRING, atoi((char *)v.string), 0);
     else if(type == MSPIDER_HTML_INPUT_SEL_LIST)
         SendMessage(hwnd, LB_DELETESTRING, atoi((char *)v.string), 0);

	return JS_TRUE;
}
static int get_select_idx(JSContext *ctx, JSObject *obj)
{
	int ret;
	HWND hwnd;

    mSpiderHtmlInputType type;
	jsobject *jsobj = NULL;

	jsobj = JS_GetPrivate(ctx, obj);
	if ( !jsobj || !(jsobj->htmlobj) ) {
		return 0;
	}
	hwnd = ((DwMgWidget*)jsobj->htmlobj)->window;
    
    type = (mSpiderHtmlInputType)jsobj->pvar3;
    
    if (type == MSPIDER_HTML_INPUT_SELECT){
        ret =  SendMessage(hwnd, CB_GETCURSEL, 0, 0);
        if ( ret == CB_ERR ) {
            /* cur sel == 0th  */
            SendMessage(hwnd, CB_SETCURSEL, 0, 0);
            ret = 0;
        }
    }else if(type == MSPIDER_HTML_INPUT_SEL_LIST){
        ret =  SendMessage(hwnd, LB_GETCURSEL, 0, 0);
        if ( ret == LB_ERR ) {
            /* cur sel == 0th  */
            SendMessage(hwnd, LB_SETCURSEL, 0, 0);
            ret = 0;
        }
    }
	return ret;
}

static void set_select_idx(JSContext *ctx, JSObject *obj, int idx)
{
	HWND hwnd;

    mSpiderHtmlInputType type;
	jsobject *jsobj = NULL;

	jsobj = JS_GetPrivate(ctx, obj);
	if ( !jsobj || !(jsobj->htmlobj) ) {
		return; 
	}
	hwnd = ((DwMgWidget*)jsobj->htmlobj)->window;
    
    type = (mSpiderHtmlInputType)jsobj->pvar3;
    
    if (type == MSPIDER_HTML_INPUT_SELECT)
        SendMessage(hwnd, CB_SETCURSEL, idx, 0);
    else if(type == MSPIDER_HTML_INPUT_SEL_LIST)
        SendMessage(hwnd, LB_SETCURSEL, idx, 0);
}

static char* get_select_value(JSContext *ctx, JSObject *obj)
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
	if ( jsobj ) {
		formobj = jsobj;
		while ( formobj && (formobj->jstype != jsform) )  {
			formobj = formobj->jsparent;
		}
		if ( !formobj ) {
			return NULL;
		}

		formid = (int)formobj->pvar1;
		doc = (mSpiderDoc*)gethtmldoc(jsobj);
		if ( doc ) {
			lb = doc->html_block;
			if ( lb ) {
				form = &lb->forms[formid]; 
				if ( form ) {
					input = &form->inputs[(int)jsobj->pvar2];
					if ( input ) {
						idx = get_select_idx(ctx, obj);
                        if (idx == -1)
                            return NULL;
						if ( input->select && input->select->options ) {
							return input->select->options[idx].value;
						}
					}
				}
			}
		}
	}

	return NULL;
}

static void	set_select_length(JSContext *ctx, JSObject *obj, const jsdouble *length)
{
    HWND hwnd;
    mSpiderHtmlInputType type;
    int count, idx;
    char s[2]=" ";
	jsobject *jsobj = NULL;

    if (*length < 0)
        return;

	jsobj = JS_GetPrivate(ctx, obj);
	if ( !jsobj || !(jsobj->htmlobj) ) {
		return;
	}
	hwnd = ((DwMgWidget*)jsobj->htmlobj)->window;
	if ( !hwnd ) {
        return;
	}
 
    type = (mSpiderHtmlInputType)jsobj->pvar3;
    
    if (type == MSPIDER_HTML_INPUT_SELECT)
        count = SendMessage (hwnd, CB_GETCOUNT, 0 , 0);
    else if (type == MSPIDER_HTML_INPUT_SEL_LIST)
        count = SendMessage (hwnd, LB_GETCOUNT, 0 , 0);

    if (count == *length)
        return;
    
    if (count < *length)
    {
        if (type == MSPIDER_HTML_INPUT_SELECT)
            for(idx = count; idx<*length; idx++)
                SendMessage (hwnd, CB_ADDSTRING, 0 , (LPARAM)s);
        else if (type == MSPIDER_HTML_INPUT_SEL_LIST)
            for(idx = count; idx<*length; idx++)
                SendMessage (hwnd, LB_ADDSTRING, 0 , (LPARAM)s);
    }else{
        if (type == MSPIDER_HTML_INPUT_SELECT)
            for (idx = count; idx > *length ; idx--)
                SendMessage (hwnd, CB_DELETESTRING, idx-1, 0);
        else if (type == MSPIDER_HTML_INPUT_SEL_LIST)
            for (idx = count; idx > *length ; idx--)
                SendMessage (hwnd, LB_DELETESTRING, idx-1, 0);
    }
}

static int get_select_length(JSContext *ctx, JSObject *obj)
{
	HWND hwnd;
    mSpiderHtmlInputType type;
	jsobject *jsobj = NULL;

	jsobj = JS_GetPrivate(ctx, obj);
	if ( !jsobj || !(jsobj->htmlobj) ) {
		return 0;
	}
	hwnd = ((DwMgWidget*)jsobj->htmlobj)->window;
	if ( !hwnd ) {
        return 0;
	}
     type = (mSpiderHtmlInputType)jsobj->pvar3;
 
     if (type == MSPIDER_HTML_INPUT_SELECT)
         return SendMessage (hwnd, CB_GETCOUNT, 0 , 0);
     else if(type == MSPIDER_HTML_INPUT_SEL_LIST)
         return SendMessage (hwnd, LB_GETCOUNT, 0 , 0);

     return 0;
}
const char* getselecteventstr(jsobject *jsobj, int rc)
{
	int idx = -1;
    mSpiderHtmlInputType type;
    
    type = (mSpiderHtmlInputType)jsobj->pvar3;
    
    if (type == MSPIDER_HTML_INPUT_SELECT)
        switch ( rc ) {
            case CBN_SELCHANGE:
                idx = get_props_index(select_propidx, select_propidxlen, "onChange");
                break;
#if ((MINIGUI_MAJOR_VERSION == 1 && MINIGUI_MICRO_VERSION == 9 && MINIGUI_MINOR_VERSION == 6)||\
        ((MINIGUI_MAJOR_VERSION >= 2 && MINIGUI_MICRO_VERSION >= 3 && MINIGUI_MINOR_VERSION >= 0)))
            case CBN_CLICKED:
                idx = get_props_index(select_propidx, select_propidxlen, "onClick");
                break;
#endif
            case CBN_SETFOCUS:
                idx = get_props_index(select_propidx, select_propidxlen, "onFocus");
                break;
            case CBN_KILLFOCUS:
                idx = get_props_index(select_propidx, select_propidxlen, "onBlur");
                break;
            default:
                break;
            }
    else if(type == MSPIDER_HTML_INPUT_SEL_LIST)
        switch ( rc ) {
            case LBN_SELCHANGE:
                idx = get_props_index(select_propidx, select_propidxlen, "onChange");
                break;
            case LBN_CLICKED:
                idx = get_props_index(select_propidx, select_propidxlen, "onClick");
                break;
            case LBN_SETFOCUS:
                idx = get_props_index(select_propidx, select_propidxlen, "onFocus");
                break;
            case LBN_KILLFOCUS:
                idx = get_props_index(select_propidx, select_propidxlen, "onBlur");
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
