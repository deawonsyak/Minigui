#include <minigui/common.h>
#include <minigui/minigui.h>
#include <minigui/gdi.h>
#include <minigui/window.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "mgdconfig.h"
#include "spidermonkey.h"
#include "dw_widget.h"
#include "prefs.h"
#include "html.h"
#include "web.h"
#include "form.h"
#include "cookies.h"
#include "colors.h"
#include "jsmisc.h"
#include "nav.h"
#include "dw_viewport.h"

#ifdef JS_SUPPORT

extern mSpiderPrefs prefs;
extern void Html_write_raw(mSpiderHtml *html, char *Buf, int BufSize, int Eof);
extern void Html_write(mSpiderHtml *html, char *Buf, int BufSize, int Eof);
extern void Html_close(mSpiderHtml *html, int ClientKey); 
/* extern document's objects */
/* image */
extern const JSClass image_class;
extern const JSPropertySpec image_props[];
extern const JSFunctionSpec image_funcs[];
/* from */
extern const JSClass form_class;
extern const JSPropertySpec form_props[];
extern const JSFunctionSpec form_funcs[];
/* elements */
extern const JSClass elements_class;
extern const JSPropertySpec elements_props[];
extern const JSFunctionSpec elements_funcs[];
/* local data and function definition */
enum document_prop { 
	JSP_DOC_LOCATION = 1, 
    JSP_DOC_ALINKCOLOR,
    JSP_DOC_BGCOLOR,   
    JSP_DOC_BODY, 
    JSP_DOC_COOKIES,    
    JSP_DOC_DOCELEMENT,
    JSP_DOC_DOMAIN,    
    JSP_DOC_FGCOLOR,   
    JSP_DOC_LASTMODIF, 
    JSP_DOC_LINKCOLOR, 
    JSP_DOC_REF,       
    JSP_DOC_TITLE,     
    JSP_DOC_URL,       
    JSP_DOC_VLINKCOLOR
};

static JSBool 	document_get_property(JSContext *ctx, JSObject *obj, jsval id, jsval *vp);
static JSBool 	document_set_property(JSContext *ctx, JSObject *obj, jsval id, jsval *vp);
static char*	get_doc_location(JSContext *ctx, JSObject *obj);
static char*	get_doc_alinkcolor(JSContext *ctx, JSObject *obj);
static char* 	get_doc_bgcolor(JSContext *ctx, JSObject *obj);
static char*	get_doc_domain(JSContext *ctx, JSObject *obj);
static char*	get_doc_fgcolor(JSContext *ctx, JSObject *obj);
static char*	get_doc_lastmodif(JSContext *ctx, JSObject *obj);
static char*	get_doc_linkcolor(JSContext *ctx, JSObject *obj);
static char*	get_doc_title(JSContext *ctx, JSObject *obj);
static char*	get_doc_vlinkcolor(JSContext *ctx, JSObject *obj);
static void 	set_doc_location(JSContext *ctx, JSObject *obj, const char* url);
static void 	set_doc_alinkcolor(JSContext *ctx, JSObject *obj, int color);
static void 	set_doc_bgcolor(JSContext *ctx, JSObject *obj, char * subtag);
static void 	set_doc_fgcolor(JSContext *ctx, JSObject *obj, int color);
static void 	set_doc_linkcolor(JSContext *ctx, JSObject *obj, int color);
static void 	set_doc_vlinkcolor(JSContext *ctx, JSObject *obj, int color);
static JSBool 	document_clear(JSContext *ctx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);
static JSBool 	document_close(JSContext *ctx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);
static JSBool	document_createAttribute(JSContext *ctx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);
static JSBool	document_createElement(JSContext *ctx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);
static JSBool	document_createTextNode(JSContext *ctx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);
static JSBool	document_focus(JSContext *ctx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);
static JSBool	document_getElementById(JSContext *ctx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);
static JSBool	document_getElementByName(JSContext *ctx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);
static JSBool	document_getElement(JSContext *ctx, JSObject *obj, uintN argc, jsval *argv, jsval *rval, int type);
static JSBool	document_getElementByTagName(JSContext *ctx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);
static JSBool	document_open(JSContext *ctx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);
static JSBool	document_write(JSContext *ctx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);
static JSBool	document_writeln(JSContext *ctx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);
/*****************************************************************************/

JSObject *get_img_object(JSContext *ctx, JSObject *pobj, jsobject *jsobj);

JSObject *get_form_object(JSContext *ctx, JSObject *pobj, jsobject *jsobj);
mSpiderDoc *getdocsdoc(JSContext *ctx, JSObject *obj);

/* export data sructure */
const JSClass document_class = {
    "document",
    JSCLASS_HAS_PRIVATE,
    JS_PropertyStub, JS_PropertyStub,
    document_get_property, document_set_property,
    JS_EnumerateStub, JS_ResolveStub, JS_ConvertStub, JS_FinalizeStub
};

const JSPropertySpec document_props[] = {
    { "location",   		JSP_DOC_LOCATION,   JSPROP_ENUMERATE },
    { "alinkColor",			JSP_DOC_ALINKCOLOR,	JSPROP_ENUMERATE },
    { "bgColor",			JSP_DOC_BGCOLOR,    JSPROP_ENUMERATE },
    { "body",				JSP_DOC_BODY,    	JSPROP_ENUMERATE },
    { "cookie",		    	JSP_DOC_COOKIES,    	JSPROP_ENUMERATE },
    { "documentElement",	JSP_DOC_DOCELEMENT, JSPROP_ENUMERATE | JSPROP_READONLY },
    { "domain",				JSP_DOC_DOMAIN,    	JSPROP_ENUMERATE | JSPROP_READONLY },
    { "fgColor",			JSP_DOC_FGCOLOR,    JSPROP_ENUMERATE },
    { "lastModified",		JSP_DOC_LASTMODIF,  JSPROP_ENUMERATE | JSPROP_READONLY },
    { "linkColor",			JSP_DOC_LINKCOLOR,  JSPROP_ENUMERATE },
    { "referrer",   		JSP_DOC_REF,    	JSPROP_ENUMERATE | JSPROP_READONLY },
    { "title",  			JSP_DOC_TITLE,  	JSPROP_ENUMERATE | JSPROP_READONLY }, /* TODO: Charset? */
    { "URL",    			JSP_DOC_URL,    	JSPROP_ENUMERATE | JSPROP_READONLY },
    { "vlinkColor",			JSP_DOC_VLINKCOLOR,	JSPROP_ENUMERATE },
    { NULL }
};

const JSFunctionSpec document_funcs[] = {
    { "clear",      			document_clear,     			0 },
    { "close",      			document_close,     			0 },
    { "createAttribute",		document_createAttribute,		1 },
    { "createElement",      	document_createElement,     	1 },
    { "createTextNode",			document_createTextNode,		1 },
    { "focus",      			document_focus,					0 },
    { "getElementById",			document_getElementById,		1 },
    { "getElementsByName",		document_getElementByName,		1 },
    { "getElementsByTagName",	document_getElementByTagName,	1 },
    { "open",					document_open,					2 },
    { "write",					document_write,					1 },
    { "writeln",				document_writeln,				1 },
    { NULL }
};

const char *document_propidx[] = {
	/* props */
	"alinkColor",
	"bgColor",
	"body",
	"cookies",
	"documentElement",
	"domain",
	"fgColor",
	"lastModified",
	"linkColor",
	"referrer",
	"title",
	"URL",
	"vlinkColor",
	/* events */
	"onClick",
	"onDblClick",
	"onFocus",
	"onKeyDown",
	"onKeyPress",
	"onKeyUp",
	"onMouseDown",
	"onMouseMove",
	"onMouseOut",
	"onMouseOver",
	"onMouseUp",
	"onResize"
};
const int document_propidxlen = sizeof(document_propidx)/sizeof(char*);
/* document objects property and method */
static JSBool
document_get_property(JSContext *ctx, JSObject *obj, jsval id, jsval *vp)
{
	struct jsval_property prop;
	jsobject *docobj = NULL;
	jsobject *findobj = NULL;
	JSObject *smobj = NULL;
#ifdef ENABLE_COOKIES
    char *urlbuf;
	mSpiderDoc *doc = NULL;
#endif
	set_prop_undef(&prop);
	docobj = JS_GetPrivate(ctx, obj);
    if (JSVAL_IS_STRING(id)) {
        union jsval_union v;
        jsval_to_value(ctx, &id, JSTYPE_STRING, &v);

		findobj = js_findobjbyname((char*)(v.string), docobj);
#ifdef FINDBYID
		if ( !findobj ) {
			findobj = js_findobjbyid((char*)(v.string), docobj);
		}
#endif 

		if ( findobj && findobj->jsparent && 
			((jsobject*)findobj->jsparent)->jstype == jsdocument ) {
			if ( findobj->jstype == jsimage ) {
				smobj = get_img_object(ctx, obj, findobj);
			} else if ( findobj && findobj->jstype == jsform ) {
				smobj = get_form_object(ctx, obj, findobj);
			}

			if ( smobj ) {
				set_prop_object(&prop, smobj);
			}
			value_to_jsval(ctx, vp, &prop);
			return JS_TRUE;
		}
		return JS_TRUE;
    } else if (!JSVAL_IS_INT(id)) {
        return JS_TRUE;
    }	

	switch (JSVAL_TO_INT(id)) {
	case JSP_DOC_LOCATION:
		set_prop_string(&prop, (unsigned char*)get_doc_location(ctx, obj));
		break;
    case JSP_DOC_ALINKCOLOR:
		set_prop_string(&prop, (unsigned char*)get_doc_alinkcolor(ctx, obj));
		break;
    case JSP_DOC_BGCOLOR:
		set_prop_string(&prop, (unsigned char*)get_doc_bgcolor(ctx, obj));
		break;
    case JSP_DOC_BODY:
		set_prop_object(&prop, NULL);
		break;
    case JSP_DOC_COOKIES:
#ifdef ENABLE_COOKIES
	    doc = getdocsdoc(ctx, obj);
        urlbuf=a_Cookies_get(doc->PageUrls->Url);
		set_prop_astring(&prop, (unsigned char*)urlbuf);
#else
		set_prop_string(&prop,NULL);
#endif
		break;
    case JSP_DOC_DOCELEMENT:
		set_prop_object(&prop, obj);
		break;
    case JSP_DOC_DOMAIN:
		set_prop_string(&prop, (unsigned char*)get_doc_domain(ctx, obj));
		break;
    case JSP_DOC_FGCOLOR:
		set_prop_string(&prop, (unsigned char*)get_doc_fgcolor(ctx, obj));
		break;
    case JSP_DOC_LASTMODIF:
		set_prop_string(&prop, (unsigned char*)get_doc_lastmodif(ctx, obj));
		break;
    case JSP_DOC_LINKCOLOR:
		set_prop_string(&prop, (unsigned char*)get_doc_linkcolor(ctx, obj));
		break;
    case JSP_DOC_REF:
		set_prop_string(&prop, NULL);	
		break;
    case JSP_DOC_TITLE:
		set_prop_string(&prop, (unsigned char*)get_doc_title(ctx, obj));
		break;
    case JSP_DOC_URL:
		set_prop_string(&prop, (unsigned char*)get_doc_location(ctx, obj));
		break;
    case JSP_DOC_VLINKCOLOR:
		set_prop_string(&prop, (unsigned char*)get_doc_vlinkcolor(ctx, obj));
		break;
	default:
		break;
	}

	value_to_jsval(ctx, vp, &prop);
	return JS_TRUE;
}

static JSBool
document_set_property(JSContext *ctx, JSObject *obj, jsval id, jsval *vp)
{
	union jsval_union v;
#ifdef ENABLE_COOKIES
    GList *cookielist=NULL;
	mSpiderDoc *doc = NULL;
#endif
    if (JSVAL_IS_STRING(id)) {
        union jsval_union v;
        jsval_to_value(ctx, &id, JSTYPE_STRING, &v);
    } else if (!JSVAL_IS_INT(id)) {
        return JS_TRUE;
    }

	switch (JSVAL_TO_INT(id)) {
	case JSP_DOC_LOCATION:
		jsval_to_value(ctx, vp, JSTYPE_STRING, &v);
		set_doc_location(ctx, obj, (char*)(v.string));
		break;
    case JSP_DOC_ALINKCOLOR:
		jsval_to_value(ctx, vp, JSTYPE_STRING, &v);
		set_doc_alinkcolor(ctx, obj, *v.number);
		break;
    case JSP_DOC_BGCOLOR:
		jsval_to_value(ctx, vp, JSTYPE_STRING, &v);
		set_doc_bgcolor(ctx, obj, (char *)v.string);
		break;
    case JSP_DOC_BODY:
		break;
    case JSP_DOC_COOKIES:
		jsval_to_value(ctx, vp, JSTYPE_STRING, &v);
#ifdef ENABLE_COOKIES
	    doc = getdocsdoc(ctx, obj);
        cookielist = g_list_append(cookielist,v.string);
        a_Cookies_set(cookielist,doc->PageUrls->Url);
#endif
		break;
    case JSP_DOC_FGCOLOR:
		jsval_to_value(ctx, vp, JSTYPE_STRING, &v);
		set_doc_fgcolor(ctx, obj, *v.number);
		break;
    case JSP_DOC_LINKCOLOR:
		jsval_to_value(ctx, vp, JSTYPE_STRING, &v);
		set_doc_linkcolor(ctx, obj, *v.number);
		break;
    case JSP_DOC_VLINKCOLOR:
		jsval_to_value(ctx, vp, JSTYPE_STRING, &v);
		set_doc_vlinkcolor(ctx, obj, *v.number);
		break;
	default:
		break;
	}

	return JS_TRUE;
}

/***********************************************************************/
static char *get_doc_location(JSContext *ctx, JSObject *obj)
{
	mSpiderDoc *doc = NULL;

	doc = getdocsdoc(ctx, obj);
	if ( doc ) {
		return URL_STR(doc->PageUrls->Url);
	}
	
	return NULL;
}

static char* get_doc_alinkcolor(JSContext *ctx, JSObject *obj)
{
	mSpiderDoc *doc = NULL;
	mSpiderHtmlLB *lb = NULL;
    static char buf[8];

	doc = getdocsdoc(ctx, obj);
	if ( doc ) {
		lb = doc->html_block;
		if ( lb ) {
            sprintf(buf, "#%06x", lb->link_color);
			return buf;
		}
	}

	return NULL;
}

static char* get_doc_bgcolor(JSContext *ctx, JSObject *obj)
{
	DwWidget *widget = NULL;
	jsobject *jsobj = NULL;
	DwStyleColor *color = NULL;
    static char buf[8];

	jsobj = JS_GetPrivate(ctx, obj);
	if ( jsobj ) {
		widget = (DwWidget*)jsobj->htmlobj;
		if ( widget ) {
			color = p_Dw_widget_get_bg_color(widget);
			if ( color ) {
                sprintf(buf, "#%06x", color->color_val);
				return buf;
			}
		}
	}

	return NULL;
}

static char *get_doc_domain(JSContext *ctx, JSObject *obj)
{
	static char buf[0x100];
	char *url = NULL;
	char *tmpstr = NULL;

	url = get_doc_location(ctx, obj);
	if ( url ) {
		strncpy(buf, url, 0x100);
		if ( strncmp(buf,"http://", 7) == 0 ) {
			tmpstr = strchr(buf+7, '/');
		} else if ( strncmp(buf, "https://", 8) == 0 ) {
			tmpstr = strchr(buf+8, '/');
		} else if ( strncmp(buf, "file:///", 8) == 0 ) {
			tmpstr = strchr(buf+8, '/');
		}

		if ( tmpstr ) {
			*tmpstr = '\0';
			return buf;
		}

		return url;
	}

	return NULL;
}

static char* get_doc_fgcolor(JSContext *ctx, JSObject *obj)
{
    mSpiderDoc *doc = NULL;
    static char buf[8];

	doc = getdocsdoc(ctx, obj);
	if ( doc ) {
        sprintf(buf, "#%06x", ((DwStyle *)doc->style)->color->color_val);
        return buf;
	}


	return NULL;
}

static char *get_doc_lastmodif(JSContext *ctx, JSObject *obj)
{
	return NULL;
}

static char* get_doc_linkcolor(JSContext *ctx, JSObject *obj)
{
	mSpiderDoc *doc = NULL;
	mSpiderHtmlLB *lb = NULL;
    static char buf[8];

	doc = getdocsdoc(ctx, obj);
	if ( doc ) {
		lb = doc->html_block;
		if ( lb ) {
            sprintf(buf, "#%06x", lb->link_color);
			return buf;
		}
	}

	return NULL;
}

static char *get_doc_title(JSContext *ctx, JSObject *obj)
{
	mSpiderDoc *doc = NULL;
	doc = getdocsdoc(ctx, obj);
	if ( doc ) {
		return doc->pagetitle;
	}

	return NULL;
}

static char* get_doc_vlinkcolor(JSContext *ctx, JSObject *obj)
{
	mSpiderDoc *doc = NULL;
	mSpiderHtmlLB *lb = NULL;
    static char buf[8];

	doc = getdocsdoc(ctx, obj);
	if ( doc ) {
		lb = doc->html_block;
		if ( lb ) {
            sprintf(buf, "#%06x", lb->visited_color);
            return buf;
		}
	}

	return NULL;
}

/*******************************************************************/
static void set_doc_location(JSContext *ctx, JSObject *obj, const char* url)
{
	mSpiderDoc *doc = NULL;
	mSpiderUrl *durl = NULL;

	if ( url ) {
		doc = getdocsdoc(ctx, obj);
		if ( doc ) {
			url = correcturl(doc, url);
			durl = a_Url_new(url, NULL, 0, 0, 0);
			if ( durl ) {
				a_Nav_push(doc, durl);
   				a_Url_free(durl);
			}
		}
	}
}

static void set_doc_alinkcolor(JSContext *ctx, JSObject *obj, int color)
{
	mSpiderDoc *doc = NULL;
	mSpiderHtmlLB *lb = NULL;

	doc = getdocsdoc(ctx, obj);
	if ( doc ) {
		lb = doc->html_block;
		if ( lb ) {
			lb->link_color = color;
		}
	}
}

static void set_doc_bgcolor(JSContext *ctx, JSObject *obj, char * subtag)
{
    	DwStyle style_attrs,* style;
	DwWidget *widget = NULL;
	jsobject *jsobj = NULL;
	DwStyleColor *dwcolor = NULL;
    	int err = 1;
	gint32 color = 0;

	jsobj = JS_GetPrivate(ctx, obj);
        color = a_Color_parse(subtag, prefs.bg_color, &err);

	if ( jsobj ) {
		widget = (DwWidget*)jsobj->htmlobj;

        if ( widget ) {
            dwcolor = a_Dw_style_color_new (color, HWND_NULL);
            style_attrs = *(widget->style);
            style_attrs.background_color = dwcolor;
            style = a_Dw_style_new (&style_attrs, ((mSpiderDoc*)widget)->bw->main_window);
            a_Dw_widget_set_style (widget, style );
            a_Dw_style_unref (style);
            InvalidateRect(widget->viewport->hwnd, NULL, TRUE); 
		}
	}
}

static void set_doc_fgcolor(JSContext *ctx, JSObject *obj, int color)
{
}

static void set_doc_linkcolor(JSContext *ctx, JSObject *obj, int color)
{
	set_doc_alinkcolor(ctx, obj, color);
}

static void set_doc_vlinkcolor(JSContext *ctx, JSObject *obj, int color)
{
	mSpiderDoc *doc = NULL;
	mSpiderHtmlLB *lb = NULL;

	doc = getdocsdoc(ctx, obj);
	if ( doc ) {
		lb = doc->html_block;
		if ( lb ) {
			lb->visited_color = color;
		}
	}
}

/***************************************************************************/
static JSBool 
document_clear(JSContext *ctx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
	return JS_TRUE;
}

static JSBool
document_close(JSContext *ctx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
	return JS_TRUE;
}

static JSBool
document_createAttribute(JSContext *ctx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
	return JS_TRUE;
}

static JSBool
document_createElement(JSContext *ctx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
	return JS_TRUE;
}

static JSBool
document_createTextNode(JSContext *ctx, JSObject *obj, uintN argc, jsval *argv, jsval *rval
)
{
	return JS_TRUE;
}

static JSBool
document_focus(JSContext *ctx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
	return JS_TRUE;
}

static JSBool
document_getElementById(JSContext *ctx, JSObject *obj, uintN argc, jsval *argv, jsval *rval
)
{
	document_getElement(ctx, obj ,argc, argv, rval, 0);
	return JS_TRUE;
}

/* The return value of document_getElementByName is not a collection HERE! */
static JSBool
document_getElementByName(JSContext *ctx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
	document_getElement(ctx, obj ,argc, argv, rval, 1);
	return JS_TRUE;
}

/* 
	type == 0 by id;
	type == 1 by name;
*/
static JSBool
document_getElement(JSContext *ctx, JSObject *obj, uintN argc, jsval *argv, jsval *rval, int type)	
{
	struct jsval_property prop;
    union jsval_union v;
	jsobject *findobj = NULL;
	JSObject *smobj = NULL;
	jsobject *formjsobj = NULL;
	JSObject *formobj = NULL;
	jsobject *docjsobj = NULL;

	docjsobj = JS_GetPrivate(ctx, obj);
	jsval_to_value(ctx, &argv[0], JSTYPE_STRING, &v);

	if ( type == 0 ) {
		findobj = js_findobjbyid((char*)(v.string), docjsobj);

	} else if ( type == 1 ) {
		findobj = js_findobjbyname((char*)(v.string), docjsobj);
	}

	if ( findobj ) {
		if ( findobj->jstype == jsimage ) {
            /*image object*/
			smobj = get_img_object(ctx, obj, findobj);
        } else if ( findobj->jstype == jsform ) {
			/* form object */
			smobj = get_form_object(ctx, obj, findobj);
		} else if ( (findobj->jstype >= jsbutton)  && (findobj->jstype <= jshidden) ) {
			/* input object */
			/* get form's JSObject */
			formjsobj = findobj->jsparent;
			if ( formjsobj && (formjsobj->jstype == jsform) ) {
				formobj = formjsobj->smobj;
				if ( !formobj ) {
					formobj = get_form_object(ctx, obj, formjsobj);
				}
				if ( formobj ) {
					smobj = getinputsmobj(ctx, formobj, findobj);
				}
			}
		}
		
		set_prop_object(&prop, smobj);
		value_to_jsval(ctx, rval, &prop);
	}

	return JS_TRUE;
}

static JSBool
document_getElementByTagName(JSContext *ctx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
	return JS_TRUE;
}

static JSBool
document_open(JSContext *ctx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
	return JS_TRUE;
}

static JSBool
document_write(JSContext *ctx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
    jsscript_interpreter *pdata;
	union jsval_union v;
    mSpiderHtml *html;
    
	if (argc != 1) 
		return JS_TRUE;

	jsval_to_value(ctx, &argv[0], JSTYPE_STRING, &v);
	if (!v.string || !*v.string)
		return JS_TRUE;	
    
    pdata=(jsscript_interpreter *)JS_GetContextPrivate(ctx);

    if((pdata->valid)&&(pdata->dealing==0))
    {
      html=(mSpiderHtml *)(pdata->point);
      
      Html_write_raw(html, (char*)(v.string),strlen((char*)(v.string)),1);
    }
	return JS_TRUE;
}

static JSBool
document_writeln(JSContext *ctx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
    return document_write(ctx, obj, argc, argv, rval);
}
/************************************************************************/
/* other function */
JSObject *get_img_object(JSContext *ctx, JSObject *pobj, jsobject *jsobj)
{
    JSObject *imgobj = NULL;

    if ( !(jsobj->smobj) ) {
        imgobj = JS_NewObject(ctx, (JSClass*)&image_class, NULL, pobj);
        JS_DefineProperties(ctx, imgobj, (JSPropertySpec *) image_props);
        JS_DefineFunctions(ctx, imgobj, (JSFunctionSpec *) image_funcs);
        JS_SetPrivate(ctx, imgobj, jsobj);
        jsobj->smobj = imgobj;
    }
    return jsobj->smobj;
}

JSObject *get_form_object(JSContext *ctx, JSObject *pobj, jsobject *jsobj)
{
	JSObject *formobj = NULL;
	JSObject *elmsobj = NULL;

	if ( !(jsobj->smobj) ) {
		formobj = JS_NewObject(ctx, (JSClass*)&form_class, NULL, pobj);
		JS_DefineProperties(ctx, formobj, (JSPropertySpec *) form_props);
		JS_DefineFunctions(ctx, formobj, (JSFunctionSpec *) form_funcs);
		JS_SetPrivate(ctx, formobj, jsobj);
		jsobj->smobj = formobj;
		
		elmsobj = JS_InitClass(ctx, formobj, NULL,
		    (JSClass *)&elements_class, NULL, 0,
		    (JSPropertySpec *)elements_props,
		    (JSFunctionSpec *)elements_funcs,
		    NULL, NULL);
		JS_SetPrivate(ctx, elmsobj, jsobj);
	}

	return jsobj->smobj;
}

mSpiderDoc *getdocsdoc(JSContext *ctx, JSObject *obj)
{
	jsobject *wjsobj = NULL;
	jsobject *jsobj = NULL;

	jsobj = JS_GetPrivate(ctx, obj);
	if ( jsobj ) {
		wjsobj = jsobj->jsparent;
		if ( wjsobj && 
			(wjsobj->jstype == jswindow || wjsobj->jstype == jstabwindow) ) {
			return (mSpiderDoc*)wjsobj->htmlobj;
		}
	}

	return NULL;
}

#endif
