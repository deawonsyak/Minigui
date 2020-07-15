#include <minigui/common.h>
#include <minigui/minigui.h>
#include <minigui/gdi.h>
#include <minigui/window.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "browser.h"
#include "interface.h"
#include "spidermonkey.h"
#include "html.h"
#include "form.h"
#include "jsmisc.h"
#include "mgdconfig.h"

#ifdef JS_SUPPORT

/* extern form's objects */
/* button */
extern const JSClass button_class;
extern const JSPropertySpec button_props[];
extern const JSFunctionSpec button_funcs[];
/* select */
extern const JSClass select_class;
extern const JSPropertySpec select_props[];
extern const JSFunctionSpec select_funcs[];
/* options */
extern const JSClass options_class;
extern const JSPropertySpec options_props[];
extern const JSFunctionSpec options_funcs[];
/* text */
extern const JSClass text_class;
extern const JSPropertySpec text_props[];
extern const JSFunctionSpec text_funcs[]; 
/* checkbox */
extern const JSClass checkbox_class;
extern const JSPropertySpec checkbox_props[];
extern const JSFunctionSpec checkbox_funcs[]; 
/* radio */
extern const JSClass radio_class;
extern const JSPropertySpec radio_props[];
extern const JSFunctionSpec radio_funcs[]; 
/* password */
extern const JSClass password_class;
extern const JSPropertySpec password_props[];
extern const JSFunctionSpec password_funcs[]; 
/* hidden  */
extern const JSClass hidden_class;
extern const JSPropertySpec hidden_props[];
extern const JSFunctionSpec hidden_funcs[]; 
/* textarea  */
extern const JSClass textarea_class;
extern const JSPropertySpec textarea_props[];
extern const JSFunctionSpec textarea_funcs[]; 


/* local data and function definition */
enum form_prop {
		JSP_FORM_ACTION = 1,
        JSP_FORM_ID,
        JSP_FORM_ENCODE,
        JSP_FORM_ENCTYP,
        JSP_FORM_LENGTH,
        JSP_FORM_METHOD,
        JSP_FORM_NAME,  
		JSP_FORM_TARGET
};
static JSBool 	form_get_property(JSContext *ctx, JSObject *obj, jsval id, jsval *vp);
static JSBool	form_set_property(JSContext *ctx, JSObject *obj, jsval id, jsval *vp);
static JSObject*get_button_object(JSContext *ctx, JSObject *pobj, jsobject *jsobj);
static JSObject*get_checkbox_object(JSContext *ctx, JSObject *pobj, jsobject *jsobj);
static JSObject*get_radio_object(JSContext *ctx, JSObject *pobj, jsobject *jsobj);
static JSObject*get_text_object(JSContext *ctx, JSObject *pobj, jsobject *jsobj);
static JSObject*get_password_object(JSContext *ctx, JSObject *pobj, jsobject *jsobj);
static JSObject*get_hidden_object(JSContext *ctx, JSObject *pobj, jsobject *jsobj);
static JSObject*get_select_object(JSContext *ctx, JSObject *pobj, jsobject *jsobj);
static JSObject*get_textarea_object(JSContext *ctx, JSObject *pobj, jsobject *jsobj);
static char* 	get_form_action(jsobject *jsobj);
static void 	set_form_action(const char *action, jsobject *jsobj);
static char* 	get_form_method(jsobject *jsobj);
static void 	set_form_method(const char *method, jsobject *jsobj);
static char* 	get_form_target(jsobject *jsobj);
static void 	set_form_target(const char *target, jsobject *jsobj);
static JSBool 	form_submit(JSContext *ctx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);
static JSBool 	form_reset(JSContext *ctx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);
/* export data sructure */
const JSClass form_class = {
	"form",
	JSCLASS_HAS_PRIVATE,
	JS_PropertyStub, JS_PropertyStub,
	form_get_property, form_set_property,
	JS_EnumerateStub, JS_ResolveStub, JS_ConvertStub, JS_FinalizeStub
};

const JSPropertySpec form_props[] = {
	{ "action",		JSP_FORM_ACTION,	JSPROP_ENUMERATE },
	{ "id",		    JSP_FORM_ID,	    JSPROP_ENUMERATE },
	{ "encoding",	JSP_FORM_ENCODE,	JSPROP_ENUMERATE },
	{ "enctype",	JSP_FORM_ENCTYP,	JSPROP_ENUMERATE },
	{ "length",   	JSP_FORM_LENGTH,  	JSPROP_ENUMERATE | JSPROP_READONLY },
	{ "method",   	JSP_FORM_METHOD,  	JSPROP_ENUMERATE },
	{ "name",   	JSP_FORM_NAME,  	JSPROP_ENUMERATE },
	{ "target",		JSP_FORM_TARGET,  	JSPROP_ENUMERATE },
	{ NULL }
};

const JSFunctionSpec form_funcs[] = {
	{ "reset", 		form_reset,		0 },
	{ "submit",		form_submit,	0 },
	{ NULL }
};

const char *form_propidx[] = {
	/* props */	
	"acceptCharset",
	"action",
	"encoding",
	"enctype",
	"id",
	"length",
	"method",
	"name",
	"tabIndex",
	"target",
	/* events */
	"onReset",
	"onSubmit"
};
const int form_propidxlen = sizeof(form_propidx)/sizeof(char*);
/* form objects property and method */
static JSBool
form_get_property(JSContext *ctx, JSObject *obj, jsval id, jsval *vp)
{
	union jsval_union v;
	struct jsval_property prop;
    int counter = 0;
	jsobject *formobj = NULL;
	jsobject *findobj = NULL;
	JSObject *smobj = NULL;
    
	set_prop_undef(&prop);
	formobj = JS_GetPrivate(ctx, obj);

	if (JSVAL_IS_STRING(id)) {
		jsval_to_value(ctx, &id, JSTYPE_STRING, &v);
		/* for elements */
		/* if ( v.string && (strcmp(v.string, "elements") == 0) ) {
			return JS_TRUE;
		} */
		findobj = js_findobjbyname((char*)(v.string), formobj);
#ifdef FINDBYID
		if ( !findobj ) {
			findobj = js_findobjbyid((char*)(v.string), formobj);
		}
#endif
		smobj = getinputsmobj(ctx, obj, findobj);
		if ( smobj ) {
			set_prop_object(&prop, smobj);
			value_to_jsval(ctx, vp, &prop);
		}
		return JS_TRUE;
	} else if (!JSVAL_IS_INT(id)) {
		return JS_TRUE;
	}

	switch (JSVAL_TO_INT(id)) {
	case JSP_FORM_ACTION:
		set_prop_string(&prop, (unsigned char*)get_form_action(formobj));
		break;
	case JSP_FORM_ENCODE:
		set_prop_string(&prop, NULL);
		break;
	case JSP_FORM_ENCTYP:
		set_prop_string(&prop, NULL);
		break;
    case JSP_FORM_ID:
        set_prop_string(&prop, (unsigned char*)formobj->jsid);
        break;
	case JSP_FORM_LENGTH:
		counter = g_list_length(formobj->children);
		set_prop_int(&prop, counter);
		break;
	case JSP_FORM_METHOD:
		set_prop_string(&prop, (unsigned char*)get_form_method(formobj));
		break;
	case JSP_FORM_NAME:
		set_prop_string(&prop, (unsigned char*)formobj->jsname);
		break;
	case JSP_FORM_TARGET:
		set_prop_string(&prop, (unsigned char*)get_form_target(formobj));
		break;	
	default:
		return JS_TRUE;
	}

	value_to_jsval(ctx, vp, &prop);
	return JS_TRUE;
}

static JSBool
form_set_property(JSContext *ctx, JSObject *obj, jsval id, jsval *vp)
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
	case JSP_FORM_ACTION:
		jsval_to_value(ctx, vp, JSTYPE_STRING, &v);
		set_form_action((char*)(v.string), jsobj);
	case JSP_FORM_ENCODE:
		break;
	case JSP_FORM_ENCTYP:
		break;
    case JSP_FORM_ID:
        jsval_to_value(ctx, vp, JSTYPE_STRING, &v);
        if (jsobj->jsid != NULL)
            g_free(jsobj->jsid);
        jsobj->jsid = strdup((char *)(v.string));
        break;
	case JSP_FORM_LENGTH:
		break;
	case JSP_FORM_METHOD:
		jsval_to_value(ctx, vp, JSTYPE_STRING, &v);
		set_form_method((char*)(v.string), jsobj);
		break;
	case JSP_FORM_NAME:
		jsval_to_value(ctx, vp, JSTYPE_STRING, &v);
		if ( jsobj->jsname ) 
			g_free(jsobj->jsname);
		jsobj->jsname = g_strdup((char*)(v.string));
		break;
	case JSP_FORM_TARGET:
		jsval_to_value(ctx, vp, JSTYPE_STRING, &v);
		set_form_target((char*)(v.string), jsobj);
		break;	
	default:
		break;
	}
	return JS_TRUE;
}

JSObject *get_button_object(JSContext *ctx, JSObject *pobj, jsobject *jsobj)
{
	JSObject *buttonobj = NULL;

	if ( !(jsobj->smobj) ) {
		buttonobj = JS_NewObject(ctx, (JSClass*)&button_class, NULL, pobj);
		JS_DefineProperties(ctx, buttonobj, (JSPropertySpec *) button_props);
		JS_DefineFunctions(ctx, buttonobj, (JSFunctionSpec *) button_funcs);
		JS_SetPrivate(ctx, buttonobj, jsobj);
		jsobj->smobj = buttonobj;
	}
	return jsobj->smobj;
}

JSObject *get_checkbox_object(JSContext *ctx, JSObject *pobj, jsobject *jsobj)
{
	JSObject *checkboxobj = NULL;

	if ( !(jsobj->smobj) ) {
		checkboxobj = JS_NewObject(ctx, (JSClass*)&checkbox_class, NULL, pobj);
		JS_DefineProperties(ctx, checkboxobj, (JSPropertySpec *) checkbox_props);
		JS_DefineFunctions(ctx, checkboxobj, (JSFunctionSpec *) checkbox_funcs);
		JS_SetPrivate(ctx, checkboxobj, jsobj);
		jsobj->smobj = checkboxobj;
	}
	return jsobj->smobj;
}
JSObject *get_radio_object(JSContext *ctx, JSObject *pobj, jsobject *jsobj)
{
	JSObject *radioobj = NULL;

	if ( !(jsobj->smobj) ) {
		radioobj = JS_NewObject(ctx, (JSClass*)&radio_class, NULL, pobj);
		JS_DefineProperties(ctx, radioobj, (JSPropertySpec *) radio_props);
		JS_DefineFunctions(ctx, radioobj, (JSFunctionSpec *) radio_funcs);
		JS_SetPrivate(ctx, radioobj, jsobj);
		jsobj->smobj = radioobj;
	}
    return jsobj->smobj;
}

JSObject *get_select_object(JSContext *ctx, JSObject *pobj, jsobject *jsobj)
{
	JSObject *selobj = NULL;
	JSObject *optsobj = NULL;

	if ( !(jsobj->smobj) ) {
		selobj = JS_NewObject(ctx, (JSClass*)&select_class, NULL, pobj);
		JS_DefineProperties(ctx, selobj, (JSPropertySpec *) select_props);
		JS_DefineFunctions(ctx, selobj, (JSFunctionSpec *) select_funcs);
		JS_SetPrivate(ctx, selobj, jsobj);
		jsobj->smobj = selobj;
	
		optsobj = JS_InitClass(ctx, selobj, NULL,
		    (JSClass *) &options_class, NULL, 0,
		    (JSPropertySpec *) options_props,
		    (JSFunctionSpec *) options_funcs,
		    NULL, NULL);
		JS_SetPrivate(ctx, optsobj, jsobj);
	}
	return jsobj->smobj;
}

JSObject *get_text_object(JSContext *ctx, JSObject *pobj, jsobject *jsobj)
{
	JSObject *textobj= NULL;

	if ( !(jsobj->smobj) ) {
		textobj= JS_NewObject(ctx, (JSClass*)&text_class, NULL, pobj);
		JS_DefineProperties(ctx, textobj, (JSPropertySpec *) text_props);
		JS_DefineFunctions(ctx, textobj, (JSFunctionSpec *)text_funcs);
		JS_SetPrivate(ctx, textobj, jsobj);
		jsobj->smobj = textobj;
	}
	return jsobj->smobj;
}

JSObject *get_hidden_object(JSContext *ctx, JSObject *pobj, jsobject *jsobj)
{
	JSObject *hiddenobj= NULL;

	if ( !(jsobj->smobj) ) {
		hiddenobj= JS_NewObject(ctx, (JSClass*)&hidden_class, NULL, pobj);
		JS_DefineProperties(ctx,hiddenobj, (JSPropertySpec *) hidden_props);
		JS_DefineFunctions(ctx, hiddenobj, (JSFunctionSpec *)hidden_funcs);
		JS_SetPrivate(ctx,hiddenobj, jsobj);
		jsobj->smobj =hiddenobj;
	}
	return jsobj->smobj;
}

JSObject *get_password_object(JSContext *ctx, JSObject *pobj, jsobject *jsobj)
{
	JSObject *pswobj= NULL;

	if ( !(jsobj->smobj) ) {
		pswobj= JS_NewObject(ctx, (JSClass*)&password_class, NULL, pobj);
		JS_DefineProperties(ctx, pswobj, (JSPropertySpec *)password_props);
		JS_DefineFunctions(ctx, pswobj, (JSFunctionSpec *)password_funcs);
		JS_SetPrivate(ctx, pswobj, jsobj);
		jsobj->smobj = pswobj;
	}
	return jsobj->smobj;
}

JSObject *get_textarea_object(JSContext *ctx, JSObject *pobj, jsobject *jsobj)
{
	JSObject *textareaobj = NULL;

	if ( !(jsobj->smobj) ) {
		textareaobj = JS_NewObject(ctx, (JSClass*)&textarea_class, NULL, pobj);
		JS_DefineProperties(ctx, textareaobj, (JSPropertySpec *) textarea_props);
		JS_DefineFunctions(ctx, textareaobj, (JSFunctionSpec *) textarea_funcs);
		JS_SetPrivate(ctx, textareaobj, jsobj);
		jsobj->smobj = textareaobj;
	}
	return jsobj->smobj;
}
static char* get_form_action(jsobject *jsobj)
{
#ifdef FULL_ACTION
	int formid;
	mSpiderUrl *url = NULL;
	mSpiderDoc *doc = NULL;
	mSpiderHtmlLB *lb = NULL;
	mSpiderHtmlForm *form = NULL;

	formid = jsobj->pvar1;
	doc = (mSpiderDoc*)gethtmldoc(jsobj);
	if ( doc ) {
		lb = (mSpiderHtmlLB*)doc->html_block;
		if ( lb ) {
			form = &lb->forms[formid];
			if ( form ) {
				url = form->action;
				if ( url && url->url_string ) {
					return url->url_string->str;
				}
			}
		}
	}
	
	return NULL;
#else
	int idx;

	idx = get_props_index(form_propidx, form_propidxlen, "action");
	return jsobj->jsprops[idx];
#endif
}

static void set_form_action(const char *action, jsobject *jsobj)
{
	int idx;
	int formid;
	mSpiderUrl *url = NULL;
	mSpiderDoc *doc = NULL;
	mSpiderHtmlLB *lb = NULL;
	mSpiderHtmlForm *form = NULL;

	if ( !action ) {
		return;
	}

	idx = get_props_index(form_propidx, form_propidxlen, "action");
	if ( jsobj->jsprops[idx] ) {
		g_free(jsobj->jsprops[idx]);
	}
	jsobj->jsprops[idx] = g_strdup(action);
	
	/* sync mspider */
	formid = (int)jsobj->pvar1;
	doc = (mSpiderDoc*)gethtmldoc(jsobj);
	if ( doc ) {
		action = correcturl(doc, action);
		if ( !action ) {
			return;
		}
		url = a_Url_new(action, NULL, 0, 0, 0);
		lb = (mSpiderHtmlLB*)doc->html_block;
		if ( lb ) {
			form = &lb->forms[formid];
			if ( form ) {
				if ( form->action ) {
					a_Url_free(form->action);
				}
				form->action = url;
			}
		}
	}
}

static char* get_form_method(jsobject *jsobj)
{
	int idx;

	idx = get_props_index(form_propidx, form_propidxlen, "method");
	return jsobj->jsprops[idx];
}

static void set_form_method(const char *method, jsobject *jsobj)
{
	int idx;
	int formid;
	mSpiderDoc *doc = NULL;
	mSpiderHtmlLB *lb = NULL;
	mSpiderHtmlForm *form = NULL;

	if ( !method ) {
		return;
	}

	idx = get_props_index(form_propidx, form_propidxlen, "method");
	if ( jsobj->jsprops[idx] ) {
		g_free(jsobj->jsprops[idx]);
	}
	jsobj->jsprops[idx] = g_strdup(method);
	
	/* sync mspider */
	formid = (int)jsobj->pvar1;
	doc = (mSpiderDoc*)gethtmldoc(jsobj);
	if ( doc ) {
		lb = (mSpiderHtmlLB*)doc->html_block;
		if ( lb ) {
			form = &lb->forms[formid];
			if ( form ) {
				if ( strcmp(method, "get") == 0 ) {
					form->method= MSPIDER_HTML_METHOD_GET;
				} else if ( strcmp(method, "post") == 0 ) {
					form->method= MSPIDER_HTML_METHOD_POST;
				} else {
					form->method= MSPIDER_HTML_METHOD_UNKNOWN;
				}
			}
		}
	}
}

static char* get_form_target(jsobject *jsobj)
{
	int idx;

	idx = get_props_index(form_propidx, form_propidxlen, "target");
	return jsobj->jsprops[idx];
}

static void set_form_target(const char *target, jsobject *jsobj)
{
	int idx;
	
	if ( !target ) {
		return;
	}

	idx = get_props_index(form_propidx, form_propidxlen, "target");
	if ( jsobj->jsprops[idx] ) {
		g_free(jsobj->jsprops[idx]);
	}
	jsobj->jsprops[idx] = g_strdup(target);
}

static JSBool 
form_submit(JSContext *ctx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
	int formid;
	char *target = NULL;
	jsobject *wjsobj = NULL;
	mSpiderDoc *tdoc = NULL;
	jsobject *jsobj = NULL;
	mSpiderDoc *doc = NULL;
	mSpiderHtmlLB *lb = NULL;
	mSpiderHtmlForm *form = NULL;

	jsobj = JS_GetPrivate(ctx, obj);
	formid= (int)jsobj->pvar1;
	doc = (mSpiderDoc*)gethtmldoc(jsobj);
	if ( doc ) {
		lb = (mSpiderHtmlLB*)doc->html_block;
		if ( lb ) {
			form = &lb->forms[formid];
			if ( form ) {
				target = get_form_target(jsobj);
				if ( target ) {
					wjsobj = js_findobjbyname(target, NULL);
					if ( wjsobj && 
						((wjsobj->jstype == jswindow) || 
						(wjsobj->jstype == jstabwindow)) ) {
						tdoc = (mSpiderDoc*)wjsobj->htmlobj;
						if ( tdoc ) {
							Html_submit_form(lb, form, NULL, tdoc);
						} else {
							/* do this? */
							Html_submit_form(lb, form, NULL, NULL);
						}
					} else {
					    Html_submit_form(lb, form, NULL, NULL);
                    }
				} else {
					/* target name is null */
					Html_submit_form(lb, form, NULL, NULL);
				}
			}
		}
	}

	return JS_TRUE;
}

static JSBool 
form_reset(JSContext *ctx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
	int i;
	int formid;
	jsobject *jsobj = NULL;
	mSpiderDoc *doc = NULL;
	mSpiderHtmlLB *lb = NULL;
	mSpiderHtmlForm *form = NULL;

	jsobj = JS_GetPrivate(ctx, obj);
	formid= (int)jsobj->pvar1;
	doc = (mSpiderDoc*)gethtmldoc(jsobj);
	if ( doc ) {
		lb = (mSpiderHtmlLB*)doc->html_block;
		if ( lb ) {
			form = &lb->forms[formid];
			if ( form ) {
				/* onReset */
				js_onformevent(form->jsobj, 0/*reset*/);
   				for ( i = 0; i < form->num_inputs; i++ ) {
			        Html_reset_input(&(form->inputs[i]));
			   }
			}
		}
	}

	return JS_TRUE;
}

JSObject* getinputsmobj(JSContext *ctx, JSObject *obj, jsobject *findobj)
{
	JSObject *smobj = NULL;

	if ( findobj && findobj->jsparent &&
		((jsobject*)findobj->jsparent)->jstype == jsform ) {
		if ( findobj->jstype == jssubmit 
			|| findobj->jstype == jsreset
			|| findobj->jstype == jsbutton ) {
			smobj = get_button_object(ctx, obj, findobj);
		} else if ( findobj->jstype == jsselect ) {
			smobj = get_select_object(ctx, obj, findobj);
		} else if ( findobj->jstype == jstext ) {
			smobj = get_text_object(ctx, obj, findobj);
		} else if ( findobj->jstype == jscheckbox ) {
			smobj = get_checkbox_object(ctx, obj, findobj);
		} else if ( findobj->jstype == jspassword ) {
			smobj = get_password_object(ctx, obj, findobj);
		} else if ( findobj->jstype == jsradio ) {
			smobj = get_radio_object(ctx, obj, findobj);
		} else if ( findobj->jstype == jshidden) {
			smobj = get_hidden_object(ctx, obj, findobj);
		} else if ( findobj->jstype == jstextarea) {
			smobj = get_textarea_object(ctx, obj, findobj);
		}
	}

	return smobj;
}

#endif
