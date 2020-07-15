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
#include "list.h"
#include "jsmisc.h"

#ifdef JS_SUPPORT

/* extern option's objects */

/* local data and function definition */
enum option_prop { 
	JSP_OPT_VALUE= 1,
	JSP_OPT_TEXT,
	JSP_OPT_ID,
	JSP_OPT_INDEX,
	JSP_OPT_LABEL,
	JSP_OPT_DEFSELECTED,
	JSP_OPT_DISABLED,
	JSP_OPT_FORM,
	JSP_OPT_SELECTED
};

static JSBool 	option_get_property(JSContext *ctx, JSObject *obj, jsval id, jsval *vp);
static JSBool 	option_set_property(JSContext *ctx, JSObject *obj, jsval id, jsval *vp);
static void 	set_option_value(JSContext *ctx, JSObject *obj, const char *value);
static void 	set_option_text(JSContext *ctx, JSObject *obj, const char *text);
static char*	get_option_value(JSContext *ctx, JSObject *obj);
static char*	get_option_text(JSContext *ctx, JSObject *obj);

static int get_option_idx(JSContext *ctx, JSObject *obj);
static void set_option_selected(JSContext *ctx, JSObject *obj, int boolean);
static int get_option_selected(jsobject *jsobj);

static JSBool	option_click(JSContext *ctx, JSObject *obj, uintN argc,jsval *argv, jsval *rval);
/* export data sructure */
const JSClass option_class = {
    "option",
    JSCLASS_HAS_PRIVATE,
    JS_PropertyStub, JS_PropertyStub,
    option_get_property, option_set_property,
    JS_EnumerateStub, JS_ResolveStub, JS_ConvertStub, JS_FinalizeStub
};

const JSPropertySpec option_props[] = {
	{ "value",	            JSP_OPT_VALUE,	        JSPROP_ENUMERATE },
	{ "text",	            JSP_OPT_TEXT,	        JSPROP_ENUMERATE },
	{ "id",	                JSP_OPT_ID,	            JSPROP_ENUMERATE },
	{ "defaultSelected",    JSP_OPT_DEFSELECTED,	JSPROP_ENUMERATE },
	{ "index",              JSP_OPT_INDEX,      	JSPROP_ENUMERATE },
	{ "disabled",	        JSP_OPT_DISABLED,   	JSPROP_ENUMERATE },
	{ "form",	            JSP_OPT_FORM,       	JSPROP_ENUMERATE   |JSPROP_READONLY },
	{ "label",	            JSP_OPT_LABEL,       	JSPROP_ENUMERATE },
	{ "selected",	        JSP_OPT_SELECTED,   	JSPROP_ENUMERATE },
    { NULL }
};

const JSFunctionSpec option_funcs[] = {
	{ "click",	option_click,	0 },
    { NULL }
};

const char *option_propidx[] = {
	/* props */
	"defaultSelected",
	"disabled",
	"form",
	"id",
	"index",
	"label",
	"selected",
	"text",
	"value"
	/* events */
}; 
const int option_propidxlen = sizeof(option_propidx)/sizeof(char*);
/* option objects property and method */
static JSBool
option_get_property(JSContext *ctx, JSObject *obj, jsval id, jsval *vp)
{
	struct jsval_property prop;
	char *str = NULL;
	jsobject *jsobj = NULL;

    int ret;
    int idx;
    
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
        case JSP_OPT_VALUE:
            str = get_option_value(ctx, obj);
            if ( str ) {
                set_prop_string(&prop, (unsigned char *)str);
            }else{
                str = get_option_text(ctx, obj);
                if ( str ) {
                    set_prop_string(&prop, (unsigned char *)str);
                }
            }
            break;
        case JSP_OPT_TEXT:
            str = get_option_text(ctx, obj);
            if ( str ) {
                set_prop_string(&prop, (unsigned char *)str);
            }
            break;
        case JSP_OPT_ID:
            set_prop_string(&prop,(unsigned char*)(jsobj->jsid));
            break;
        case JSP_OPT_DEFSELECTED:
            ret=get_props_index(option_propidx,option_propidxlen,"defaultSelected");
            if(jsobj->jsprops[ret] == NULL)
                jsobj->jsprops[ret]=strdup("false");
            
            if (strcasecmp(jsobj->jsprops[ret], "false") == 0)
                set_prop_boolean(&prop, 0); 
            else
                set_prop_boolean(&prop, 1); 
            break;
        case JSP_OPT_DISABLED:
            break;
        case JSP_OPT_INDEX:
            idx = get_option_idx(ctx, obj);
            set_prop_int(&prop, idx);
            break;
        case JSP_OPT_LABEL:
            ret=get_props_index(option_propidx,option_propidxlen,"label");
            set_prop_string(&prop,(unsigned char *)(jsobj->jsprops[ret])); 
            break;
        case JSP_OPT_FORM:
            jsobj =jsobj->jsparent;
            if((((jsobject*)jsobj->jsparent)->jstype == jsform)
                    &&(((jsobject*)jsobj->jsparent)->smobj!=NULL))
                set_prop_object(&prop,(JSObject *)(((jsobject*)jsobj->jsparent)->smobj)); 
            break;
        case JSP_OPT_SELECTED:
            ret=get_option_selected(jsobj);
            set_prop_boolean(&prop, ret);
            break;
        default:
            return JS_TRUE;
	}
	value_to_jsval(ctx, vp, &prop);
	return JS_TRUE;
}

static JSBool
option_set_property(JSContext *ctx, JSObject *obj, jsval id, jsval *vp)
{
	union jsval_union v;
    jsobject *jsobj = NULL;
    
    jsobj = JS_GetPrivate(ctx, obj);

    if (JSVAL_IS_STRING(id)) {
        union jsval_union v;
        jsval_to_value(ctx, &id, JSTYPE_STRING, &v);
    } else if (!JSVAL_IS_INT(id)) {
        return JS_TRUE;
    }

	switch (JSVAL_TO_INT(id)) {
        case JSP_OPT_VALUE:
            jsval_to_value(ctx, vp, JSTYPE_STRING, &v);
            set_option_value(ctx, obj, (char*)(v.string));
            break;
        case JSP_OPT_TEXT:
            jsval_to_value(ctx, vp, JSTYPE_STRING, &v);
            set_option_text(ctx, obj, (char*)(v.string));
            break;
        case JSP_OPT_ID:
            jsval_to_value(ctx, vp, JSTYPE_STRING, &v);
            if (jsobj->jsid != NULL)
                free(jsobj->jsid);
            jsobj->jsid = strdup((char *)(v.string));
            break;
        case JSP_OPT_DISABLED:
            break;
        case JSP_OPT_LABEL:
            jsval_to_value(ctx, vp, JSTYPE_STRING, &v);
            set_jsobj_props(option_propidx,option_propidxlen,jsobj,"label",(char*)v.string);
            break;
        case JSP_OPT_SELECTED:
            jsval_to_value(ctx, vp, JSTYPE_BOOLEAN, &v);
            set_option_selected(ctx, obj, v.boolean);
            break;
        case JSP_OPT_DEFSELECTED:
            jsval_to_value(ctx, vp, JSTYPE_STRING, &v);
            set_jsobj_props(option_propidx,option_propidxlen,jsobj,"defaultSelected",(char*)v.string);
            break;
        default:
            return JS_TRUE;
	}
	return JS_TRUE;
}

static void set_option_value(JSContext *ctx, JSObject *obj, const char *value)
{
	int idx;
	int no;
	int inputid;
	int formid;
	jsobject *jsobj = NULL;
	jsobject *pjsobj = NULL;
	jsobject *formobj = NULL;
	mSpiderDoc *doc = NULL;
	mSpiderHtmlLB *lb = NULL;
	mSpiderHtmlForm *form = NULL;
    mSpiderHtmlInput *input = NULL;

	jsobj = JS_GetPrivate(ctx, obj);
	if ( jsobj ) {
		idx = get_props_index(option_propidx, option_propidxlen, "value");
        if (jsobj->jsprops[idx]!=NULL) g_free(jsobj->jsprops[idx]);
		jsobj->jsprops[idx] = g_strdup(value);

		/* sync data stored in mspider */
		pjsobj = jsobj->jsparent;	/* get select */
		if ( pjsobj ) {
			inputid = (int)pjsobj->pvar2;

			/* get form */
			formobj = pjsobj;
			while ( formobj && (formobj->jstype != jsform) )  {
				formobj = formobj->jsparent;
			}
			if ( !formobj || (formobj->jstype != jsform) ) {
				return;
			}

			formid = (int)formobj->pvar1;
			doc = (mSpiderDoc*)gethtmldoc(jsobj);
			if ( doc ) {
				lb = doc->html_block;
				if ( lb ) {
					form = &lb->forms[formid]; 
					if ( form ) {
						input = &(form->inputs[inputid]);
						if ( input ) {
							no = input->select->num_options;
							/* maybe have some bugs */
							if ( (int)jsobj->pvar1 == no ) {
			        			a_List_add(input->select->options, no, 
									input->select->num_options_max);
						        input->select->options[no].menuitem = NULL;
								input->select->num_options ++;
							}
							if ( &(input->select->options[(int)jsobj->pvar1]) != NULL ) {
								/* g_free(input->select->options[jsobj->pvar1-1]); */
							}
							
							if ( (int)jsobj->pvar1 < input->select->num_options ) {
								input->select->options[(int)jsobj->pvar1].value = 
									g_strdup(value);
							}
						}
					}
				}
			}
		}
	}
}

static void set_option_text(JSContext *ctx, JSObject *obj, const char *text)
{
	int oldsel;
	int idx;
	int optnum;
	HWND hwnd;
	jsobject *jsobj = NULL;
	jsobject *pjsobj = NULL;

	if ( !text ) { 
		return;
	}

	jsobj = JS_GetPrivate(ctx, obj);
	if ( !jsobj ) {
		return;
	}

	idx = (int)jsobj->pvar1;
	pjsobj = jsobj->jsparent;
	if ( !pjsobj || (pjsobj->jstype != jsselect) || !(pjsobj->htmlobj) ) {
		return;
	}

	hwnd = ((DwMgWidget*)pjsobj->htmlobj)->window;
	if ( !hwnd ) {
		return;
	}
	optnum = SendMessage(hwnd, CB_GETCOUNT, 0, 0);
	oldsel = SendMessage(hwnd, CB_GETCURSEL, 0, 0);
	if ( idx > optnum ) {
		return;
	} else if ( idx < optnum ) {
		/* find and set */
		if ( !strlen(text) ) {
			SendMessage(hwnd, CB_INSERTSTRING, idx, (int)(char*)" "); 
		} else {
			SendMessage(hwnd, CB_INSERTSTRING, idx, (int)text); 
		}
		SendMessage(hwnd, CB_DELETESTRING, idx+1, 0);
	} else {
		/* create and set */
		SendMessage(hwnd, CB_ADDSTRING, 0, (LPARAM)text);
	}
	SendMessage(hwnd, CB_SETCURSEL, oldsel, 0);
}

static char *get_option_value(JSContext *ctx, JSObject *obj)
{
	int idx;
	jsobject *jsobj = NULL;

	jsobj = JS_GetPrivate(ctx, obj);
	if ( !jsobj ) {
		return NULL;
	}

	idx = get_props_index(option_propidx, option_propidxlen, "value");
	return jsobj->jsprops[idx];
}

static char *get_option_text(JSContext *ctx, JSObject *obj)
{
	int ret;
	int idx;
	int optnum;
	static char buf[0x100];
	jsobject *jsobj = NULL;
	jsobject *pjsobj = NULL;
	jsobj = JS_GetPrivate(ctx, obj);
	if ( !jsobj ) {
		return NULL;
	}

	idx = (int)jsobj->pvar1;	/* idx */
	pjsobj = jsobj->jsparent;
	if ( !pjsobj || (pjsobj->jstype != jsselect) || !(pjsobj->htmlobj) ) {
		return NULL;
	}
	if ( !(((DwMgWidget*)pjsobj->htmlobj)->window) ) {
		return NULL;
	}

	optnum = SendMessage(((DwMgWidget*)pjsobj->htmlobj)->window,
		CB_GETCOUNT, 0, 0);
	if ( idx >= optnum ) {
		return NULL;
	} else if ( idx < optnum ) {
		/* find and set */
		ret = SendMessage(((DwMgWidget*)pjsobj->htmlobj)->window,
			CB_GETLBTEXT, idx, (int)buf);
		if ( ret == CB_ERR ) {
			return NULL;
		}
		return buf;
	}

	return NULL;
}
static int get_option_idx(JSContext *ctx, JSObject *obj)
{
	int ret;
	HWND hwnd;
	jsobject *jsobj = NULL;

	jsobj = JS_GetPrivate(ctx, obj);
	
    if ( !jsobj || !(jsobj->jsparent) ) {
		return 0;
	}
    jsobj = jsobj->jsparent;

	hwnd = ((DwMgWidget*)jsobj->htmlobj)->window;
	
    ret =  SendMessage(hwnd, CB_GETCURSEL, 0, 0);
	if ( ret == CB_ERR ) {
		/* cur sel == 0th  */
		SendMessage(hwnd, CB_SETCURSEL, 0, 0);
		ret = 0;
	}

	return ret;
}

static void set_option_selected(JSContext *ctx, JSObject *obj, int boolean)
{
	HWND hwnd;
    int sindex;
	jsobject *jsobj = NULL;

	jsobj = JS_GetPrivate(ctx, obj);
	
    if ( !jsobj || !(jsobj->jsparent) ) {
		return ;
	}
    sindex =(int)jsobj->pvar1;
    
    jsobj = jsobj->jsparent;

	hwnd = ((DwMgWidget*)jsobj->htmlobj)->window;
    
    SendMessage(hwnd, CB_SETCURSEL, sindex, 0);
}

static int get_option_selected(jsobject *jsobj)
{
 
	HWND hwnd;
    int index, sindex;
    if ( !jsobj || !(jsobj->jsparent) ) {
		return 0;
	}
    sindex = (int)jsobj->pvar1;

    jsobj = jsobj->jsparent;

	hwnd = ((DwMgWidget*)jsobj->htmlobj)->window;
    
    index = SendMessage(hwnd, CB_GETCURSEL, 0, 0);
    
    if (index == sindex)
        return 1;
    else
        return 0;
}
static JSBool	option_click(JSContext *ctx, JSObject *obj, uintN argc,jsval *argv, jsval *rval)
{
	HWND hwnd;
    int sindex;
	jsobject *jsobj = NULL;

	jsobj = JS_GetPrivate(ctx, obj);
	if ( !jsobj || !(jsobj->jsparent) ) {
		return JS_TRUE;
	}
    
    sindex =(int)jsobj->pvar1;
    
    jsobj = jsobj->jsparent;
	hwnd = ((DwMgWidget*)jsobj->htmlobj)->window;
	if ( !hwnd ) {
        return JS_TRUE;
	}

    if (!IsWindowEnabled (hwnd))
        return JS_TRUE;

    SendMessage(hwnd, CB_SETCURSEL, sindex, 0);

    return JS_TRUE;
}
#endif
