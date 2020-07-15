#include <minigui/common.h>
#include <minigui/minigui.h>
#include <minigui/gdi.h>
#include <minigui/window.h>
#include <minigui/control.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "browser.h"
#include "interface.h"
#include "nav.h"
#include "list.h"
#include "js.h"
#include "spidermonkey.h"
#include "jsmisc.h"
#include "frameset.h"
#include "p_window.h"

#ifdef JS_SUPPORT

/* local data and function definition */

enum window_prop {
	JSP_WIN_CLOSED = 1,
	JSP_WIN_DFTSTATUS,
	JSP_WIN_LENGTH,
	JSP_WIN_NAME,
	JSP_WIN_OPENER,
	JSP_WIN_PARENT,
	JSP_WIN_SELF,
	JSP_WIN_STATUS,
	JSP_WIN_TOP,
	JSP_FRM_FRAMEBORDER,	
	JSP_FRM_MARGINHEIGHT,	
	JSP_FRM_MARGINWIDTH,	
	JSP_FRM_NORESIZE,		
	JSP_FRM_SCROLLING,		
	JSP_FRM_SRC			
};
static JSBool window_get_property(JSContext *ctx, JSObject *obj, jsval id, jsval *vp);
static JSBool window_set_property(JSContext *ctx, JSObject *obj, jsval id, jsval *vp);

static JSBool window_alert(JSContext *ctx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);
static JSBool window_blur(JSContext *ctx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);
static JSBool window_clearInterval(JSContext *ctx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);
static JSBool window_clearTimeout(JSContext *ctx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);
static JSBool window_close(JSContext *ctx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);
static JSBool window_confirm(JSContext *ctx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);
static JSBool window_focus(JSContext *ctx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);
static JSBool window_moveBy(JSContext *ctx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);
static JSBool window_moveTo(JSContext *ctx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);
static JSBool window_open(JSContext *ctx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);
static JSBool window_print(JSContext *ctx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);
static JSBool window_prompt(JSContext *ctx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);
static JSBool window_scrollBy(JSContext *ctx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);
static JSBool window_scrollTo(JSContext *ctx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);
static JSBool window_setInterval(JSContext *ctx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);
static JSBool window_setTimeout(JSContext *ctx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);

/********************************************************************/
static void js_settimer(JSContext *ctx, JSObject *obj, uintN argc,jsval *argv, jsval *rval, int type);
static void js_cleartimer(int tid);
static gint js_timeoutproc(gpointer data);
static jsobject *getdocobj(jsobject *wjsobj);
static mSpiderDoc* gettabwindow(jsobject *wjsobj);
static int getframecount(JSContext *ctx, JSObject *obj);
static char* getname(JSContext *ctx, JSObject *obj);
static void setname(JSContext *ctx, JSObject *obj, char* name);
static JSObject* getparent(JSContext *ctx, JSObject *obj);
static JSObject* gettopwin(JSContext *ctx, JSObject *obj);
static char* getsrc(JSContext *ctx, JSObject *obj);
static void setsrc(JSContext *ctx, JSObject *obj, char* url);

static void setstatus(JSContext *ctx, JSObject *obj, char* msg);
static char* getstatus(JSContext *ctx, JSObject *obj);

/* export data sructure */
const JSClass window_class = {
	"window",
	JSCLASS_HAS_PRIVATE,
	JS_PropertyStub, JS_PropertyStub,
	window_get_property, window_set_property,
	JS_EnumerateStub, JS_ResolveStub, JS_ConvertStub, JS_FinalizeStub
};

const JSPropertySpec window_props[] = {
	{ "window",			JSP_WIN_SELF,			JSPROP_ENUMERATE | JSPROP_READONLY },
	{ "closed",			JSP_WIN_CLOSED,			JSPROP_ENUMERATE | JSPROP_READONLY },
	{ "defaultStatus",	JSP_WIN_DFTSTATUS,		JSPROP_ENUMERATE },
	{ "length",			JSP_WIN_LENGTH,			JSPROP_ENUMERATE },
	{ "name",			JSP_WIN_NAME,			JSPROP_ENUMERATE },
	{ "opener",			JSP_WIN_OPENER,			JSPROP_ENUMERATE },
	{ "parent",			JSP_WIN_PARENT,			JSPROP_ENUMERATE | JSPROP_READONLY },
	{ "self",			JSP_WIN_SELF,			JSPROP_ENUMERATE | JSPROP_READONLY },
	{ "status",			JSP_WIN_STATUS,			JSPROP_ENUMERATE },
	{ "top",			JSP_WIN_TOP,			JSPROP_ENUMERATE },
	/* for frame only */
	{ "frameBorder",	JSP_FRM_FRAMEBORDER,	JSPROP_ENUMERATE },
	{ "marginHeight",	JSP_FRM_MARGINHEIGHT,	JSPROP_ENUMERATE },
	{ "marginWidth",	JSP_FRM_MARGINWIDTH,	JSPROP_ENUMERATE },
	{ "noResize",		JSP_FRM_NORESIZE,		JSPROP_ENUMERATE },
	{ "scrolling",		JSP_FRM_SCROLLING,		JSPROP_ENUMERATE },
	{ "src",			JSP_FRM_SRC,			JSPROP_ENUMERATE },
	{ NULL }
};

const JSFunctionSpec window_funcs[] = {
	{ "alert",			window_alert,			1 },
	{ "blur",			window_blur,			0 },
	{ "clearInterval",	window_clearInterval,	1 },
	{ "clearTimeout",	window_clearTimeout,	1 },
	{ "close",			window_close,			0 },
	{ "confirm",		window_confirm,			1 },
	{ "focus",			window_focus,			0 },
	{ "moveBy",			window_moveBy,			2 },
	{ "moveTo",			window_moveTo,			2 },
	{ "open",			window_open,			3 },	/* IE(>3.02) support */
	{ "print",			window_print,			0 },
	{ "prompt",			window_prompt,			2 },
	{ "scrollBy",		window_scrollBy,		2 },
	{ "scrollTo",		window_scrollTo,		2 },
	{ "setInterval",	window_setInterval,		3 },
	{ "setTimeout",		window_setTimeout,		3 },	/* IE(>3.02) support */
	{ NULL }
};

const char *window_propidx[] = {
	/* props */
	"closed",
	"defaultStatus",
	"dialogArguments",
	"dialogHeight",
	"dialogLeft",
	"dialogTop",
	"dialogWidth",
	"frameElement",
	"length",
	"name",
	"offscreenBuffering",
	"opener",
	"parent",
	"returnValue",
	"screenLeft",
	"screenTop",
	"self",
	"status",
	"top",
	/* events */
	"onBlur",
	"onError",
	"onFocus",
	"onLoad",
	"onResize",
	"onUnload"
	/* frame's props */
	"frameBorder",
	"marginHeight",
	"marginWidth",
	"noResize",
	"scrolling",
	"src"
};
const int window_propidxlen = sizeof(window_propidx)/sizeof(char*);
/* window objects property and method */
static JSBool
window_get_property(JSContext *ctx, JSObject *obj, jsval id, jsval *vp)
{
	struct jsval_property prop;
	jsobject *findobj = NULL;
	jsobject *wjsobj = NULL;

	wjsobj = JS_GetPrivate(ctx, obj);
	set_prop_undef(&prop);
	if (JSVAL_IS_STRING(id)) {
		union jsval_union v;
		jsval_to_value(ctx, &id, JSTYPE_STRING, &v);
		findobj = js_findobjbyname((char *)(v.string), wjsobj);
		if ( findobj ) {
			set_prop_object(&prop, findobj->smobj);
			value_to_jsval(ctx, vp, &prop);
		}	
		return JS_TRUE;
	} else if (!JSVAL_IS_INT(id)) {
		return JS_TRUE;
	}

	switch (JSVAL_TO_INT(id)) {
	case JSP_WIN_CLOSED:
		set_prop_boolean(&prop, 0);
		break;
	case JSP_WIN_DFTSTATUS:
		set_prop_string(&prop, NULL);
		break;
	case JSP_WIN_LENGTH:
		set_prop_int(&prop, getframecount(ctx, obj));
		break;
	case JSP_WIN_NAME:
		set_prop_string(&prop, (unsigned char*)getname(ctx, obj));
		break;
	case JSP_WIN_OPENER:
		/* here may cause some bug */
		set_prop_object(&prop, NULL);
		break;
	case JSP_WIN_PARENT:
		set_prop_object(&prop, getparent(ctx, obj));
		break;
	case JSP_WIN_SELF:
		set_prop_object(&prop, obj);
		break;
	case JSP_WIN_STATUS:

		set_prop_string(&prop, (unsigned char*)getstatus(ctx, obj));
		break;
	case JSP_WIN_TOP:
		set_prop_object(&prop, gettopwin(ctx, obj));
		break;
	case JSP_FRM_FRAMEBORDER:
		break;
	case JSP_FRM_MARGINHEIGHT:
		break;
	case JSP_FRM_MARGINWIDTH:
		break;
	case JSP_FRM_NORESIZE:
		break;
	case JSP_FRM_SCROLLING:
		break;
	case JSP_FRM_SRC:
		set_prop_string(&prop, (unsigned char *)getsrc(ctx, obj));
		break;
	default:
		return JS_TRUE;
	}

	value_to_jsval(ctx, vp, &prop);
	return JS_TRUE;
}

static JSBool
window_set_property(JSContext *ctx, JSObject *obj, jsval id, jsval *vp)
{
	union jsval_union v;

	if (JSVAL_IS_STRING(id)) {
		union jsval_union v;
		jsval_to_value(ctx, &id, JSTYPE_STRING, &v);

		if ( strcasecmp((char *)(v.string), "location") == 0 ) {
			jsval_to_value(ctx, vp, JSTYPE_STRING, &v);
			setsrc(ctx, obj, (char*)(v.string));
		}
		return JS_TRUE;
	} else if (!JSVAL_IS_INT(id)) {
		return JS_TRUE;
	}

	switch (JSVAL_TO_INT(id)) {
	case JSP_WIN_DFTSTATUS:
		break;
	case JSP_WIN_LENGTH:
		break;
	case JSP_WIN_NAME:
		jsval_to_value(ctx, vp, JSTYPE_STRING, &v);
		setname(ctx, obj, (char*)(v.string));
		break;
	case JSP_WIN_OPENER:
		break;
	case JSP_WIN_STATUS:
		jsval_to_value(ctx, vp, JSTYPE_STRING, &v);
		setstatus(ctx, obj, (char*)(v.string));

		break;
	case JSP_WIN_TOP:
		break;
	case JSP_FRM_FRAMEBORDER:
		break;
	case JSP_FRM_MARGINHEIGHT:
		break;
	case JSP_FRM_MARGINWIDTH:
		break;
	case JSP_FRM_NORESIZE:
		break;
	case JSP_FRM_SCROLLING:
		break;
	case JSP_FRM_SRC:
		jsval_to_value(ctx, vp, JSTYPE_STRING, &v);
		setsrc(ctx, obj, (char*)(v.string));
		break;
	default:
		return JS_TRUE;
	}

	return JS_TRUE;
}

static JSBool
window_alert(JSContext *ctx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
	union jsval_union v;
	struct jsval_property prop;
	jsobject *wjsobj; 
	mSpiderDoc *doc; 
	
	set_prop_undef(&prop);
	wjsobj = NULL;
	doc = NULL;

	if ( argc != 1 ) {
		return JS_TRUE;
	}
	jsval_to_value(ctx, &argv[0], JSTYPE_STRING, &v);
	if ( !v.string) {
		return JS_TRUE;	
	}

	wjsobj = JS_GetPrivate(ctx, obj);
	if ( wjsobj ) {
		doc = gettabwindow(wjsobj);
		if ( doc ) {
            if (doc->bw->CB_MESSAGE_BOX)
                (*(doc->bw->CB_MESSAGE_BOX))(doc->bw->main_window, (char*)(v.string), "Javascript");
		}
	}

	set_prop_boolean(&prop, 1);
	value_to_jsval(ctx, rval, &prop);
	return JS_TRUE;
}

static JSBool 
window_blur(JSContext *ctx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
	return JS_TRUE;
}

static JSBool 
window_clearInterval(JSContext *ctx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
	union jsval_union v;

	if ( argc != 1 ) {
		return JS_TRUE;
	}
	jsval_to_value(ctx, &argv[0], JSTYPE_NUMBER, &v);

	js_cleartimer(*v.number);

	return JS_TRUE;
}

static JSBool 
window_clearTimeout(JSContext *ctx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
	union jsval_union v;

	if ( argc != 1 ) {
		return JS_TRUE;
	}
	jsval_to_value(ctx, &argv[0], JSTYPE_NUMBER, &v);

	js_cleartimer(*v.number);

	return JS_TRUE;
}

static JSBool
window_close(JSContext *ctx, JSObject *obj, uintN argc,jsval *argv, jsval *rval)
{
	jsobject *wjsobj = NULL;
	mSpiderDoc *doc = NULL;
	
	wjsobj = JS_GetPrivate(ctx, obj);
	if ( wjsobj ) {
		doc = (mSpiderDoc*)wjsobj->htmlobj;
		if ( doc && (doc->parent == NULL)) {
                PostMessage(doc->bw->main_window, MSG_CLOSE, 0, 0);
		}
	}
	return JS_TRUE;
}

static JSBool 
window_confirm(JSContext *ctx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
	union jsval_union v;
	struct jsval_property prop;
	jsobject *wjsobj = NULL;
	mSpiderDoc *doc = NULL;

	set_prop_boolean(&prop, 0);
	
	if ( argc != 1 ) {
		return JS_TRUE;
	}
	jsval_to_value(ctx, &argv[0], JSTYPE_STRING, &v);
	if ( !v.string ) {
		return JS_TRUE;
	}

	wjsobj = JS_GetPrivate(ctx, obj);
	if ( wjsobj ) {
		doc = gettabwindow(wjsobj);
		if ( doc ) {
            if (doc->bw->CB_CONFIRM_BOX) {
                if ((*(doc->bw->CB_CONFIRM_BOX))(doc->bw->main_window, (char*)v.string, "Javascript"))
                        set_prop_boolean(&prop, 1);
            }
		}
	}


	value_to_jsval(ctx, rval, &prop);
	return JS_TRUE;
}

static JSBool 
window_focus(JSContext *ctx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
	return JS_TRUE;
}

static JSBool 
window_moveBy(JSContext *ctx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
	return JS_TRUE;
}

static JSBool 
window_moveTo(JSContext *ctx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
	return JS_TRUE;
}

static JSBool
window_open(JSContext *ctx, JSObject *obj, uintN argc,jsval *argv, jsval *rval)
{
	union jsval_union vurl;
	union jsval_union vname;
	union jsval_union vargs;

	jsobject *jsobj = NULL;
	mSpiderDoc *doc = NULL;
	char *url = NULL;
    char * ps;
    RECT wrc;
    int x, y, w, h;
    unsigned int flags;

	jsobj = JS_GetPrivate(ctx, obj);
	if ( !jsobj ) {
		return JS_TRUE;
	}
	doc = jsobj->htmlobj;
	if ( !doc ) {
		return JS_TRUE;
	}

    GetWindowRect (doc->bw->main_window , &wrc);
    x = wrc.left;
    y = wrc.top;
    w = wrc.right - wrc.left;
    h = wrc.bottom - wrc.top;

	jsval_to_value(ctx, &argv[0], JSTYPE_STRING, &vurl);
	jsval_to_value(ctx, &argv[1], JSTYPE_STRING, &vname);
	jsval_to_value(ctx, &argv[2], JSTYPE_STRING, &vargs);

    if ( vurl.string && (strlen((char*)(vurl.string)) != 0) ) {
        url = correcturl(doc, (char*)(vurl.string));
    } else {
        url = (char*)(vurl.string);
    }

    flags = NEW_BW_TOOLBAR | NEW_BW_LOCATIONBAR | NEW_BW_STATUSBAR | NEW_BW_PROGRESSBAR | NEW_BW_MENUBAR;

    if ( url ) {
        if ((ps = strstr ((char*)(vargs.string) , "width")) != NULL)
            sscanf (ps, "width=%d" , &w);
        if (w < 100) w = 100;
            
        if ((ps = strstr ((char*)(vargs.string) , "height")) != NULL)
            sscanf (ps, "height=%d" ,&h);
        if (h < 100) h = 100;
    
        if ((ps = strstr ((char*)(vargs.string) , "top")) != NULL)
            sscanf (ps, "top=%d" ,&y);

        if ((ps = strstr ((char*)(vargs.string) , "left")) != NULL)
            sscanf (ps, "left=%d" ,&x);


        if ((ps = strstr ((char*)(vargs.string) , "location")) != NULL) {
            if ((strncmp (ps+9 , "no", 2) == 0)||(strncmp (ps+9 , "0", 1) == 0))
                flags &= ~NEW_BW_LOCATIONBAR;
        }
        if ((ps = strstr ((char*)(vargs.string) , "menubar")) != NULL) {
            if ((strncmp (ps+8 , "no", 2) == 0)||(strncmp (ps+8 , "0", 1) == 0))
                flags &= ~NEW_BW_MENUBAR;
        }
        if ((ps = strstr ((char*)(vargs.string) , "status")) != NULL) {
            if ((strncmp (ps+7 , "no", 2) == 0)||(strncmp (ps+7 , "0", 1) == 0))
                flags &= ~NEW_BW_STATUSBAR;
        }
        if ((ps = strstr ((char*)(vargs.string) , "toolbar")) != NULL) {
            if ((strncmp (ps+8 , "no", 2) == 0)||(strncmp (ps+8 , "0", 1) == 0))
                flags &= ~NEW_BW_TOOLBAR;
        }


        a_Pop_Window (HWND_DESKTOP, (char*)(vname.string), flags, x, y, w, h, url);
    }

	return JS_TRUE;
}

static JSBool 
window_print(JSContext *ctx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
	return JS_TRUE;
}

static JSBool 
window_prompt(JSContext *ctx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
	union jsval_union v0, v1;
	struct jsval_property prop;
	char *buf = NULL;
	jsobject *wjsobj = NULL;
	mSpiderDoc *doc = NULL;

	set_prop_undef(&prop);

	if ( argc < 1 || argc > 2) {
		return JS_TRUE;
	}

	jsval_to_value(ctx, &argv[0], JSTYPE_STRING, &v0);
	if ( !v0.string || (strlen((char*)(v0.string)) == 0) ) {
		return JS_TRUE;
	}

	v1.string = NULL;
	if ( argc == 2 ) {
		jsval_to_value(ctx, &argv[1], JSTYPE_STRING, &v1);
	}

	wjsobj = JS_GetPrivate(ctx, obj);
	if ( wjsobj ) {
		doc = gettabwindow(wjsobj);
		if ( doc ) {
            if (doc->bw->CB_PROMPT_BOX) {
               if ((buf = (*(doc->bw->CB_PROMPT_BOX))(doc->bw->main_window, 
                                    (char*)v0.string, (char*)v1.string, "Javascript")) != NULL)
		            set_prop_string(&prop,(unsigned char*)buf);
            }
		}
	}

	value_to_jsval(ctx, rval, &prop);
	return JS_TRUE;
}

static JSBool 
window_scrollBy(JSContext *ctx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
	return JS_TRUE;
}

static JSBool 
window_scrollTo(JSContext *ctx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
	return JS_TRUE;
}

static JSBool 
window_setInterval(JSContext *ctx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
	js_settimer(ctx, obj, argc, argv, rval, conti);

	return JS_TRUE;
}

static JSBool
window_setTimeout(JSContext *ctx, JSObject *obj, uintN argc,jsval *argv, jsval *rval)
{
	js_settimer(ctx, obj, argc, argv, rval, once);
	return JS_TRUE;
}

/********************************************************************/

static GList *js_timerlist;
static void invalid_timer (gpointer data, gpointer user_data)
{
	tpara *tp = (tpara*)data;
	jsobject *jsobj = (jsobject*)user_data;

    if (tp->jsobj == jsobj)
		tp->canceled = 1;
}

void js_cleartimers (jsobject * obj)
{
   g_list_foreach (js_timerlist, invalid_timer, (gpointer)obj);
}

static void 
js_settimer(JSContext *ctx, JSObject *obj, uintN argc,jsval *argv, jsval *rval, int type)
{
	unsigned int to;
	union jsval_union v;
	struct jsval_property prop;
	tpara *tp = NULL;
	jsobject *wjsobj = NULL;
	jsobject *jsobj = NULL;
	
	set_prop_int(&prop, 0);
	wjsobj = JS_GetPrivate(ctx, obj);
	if ( !wjsobj ) {
		return;
	}

	jsobj = getdocobj(wjsobj);
	if ( !jsobj ) {
		return;
	}

	tp = g_malloc0(sizeof(tpara));
	tp->type = type;
	tp->canceled = 0;
    tp->jsobj = jsobj;
	tp->ctx	= ctx;
	tp->interpreter = &(jsobj->jsinterpreter); 

	jsval_to_value(ctx, &argv[0], JSTYPE_STRING, &v);
	if ( !v.string || (strlen((char*)(v.string)) == 0) ) {
		g_free(tp);
		return;
	}
	tp->jsscript = g_strdup((char*)(v.string));

	jsval_to_value(ctx, &argv[1], JSTYPE_NUMBER, &v);
	to = (int)*v.number;
	if ( to < 0 ) {
		g_free(tp);
		return;
	}

    if(to < 100)
    {
        to =100;
    }
	g_timeout_add(to, (GSourceFunc)js_timeoutproc, tp);
	js_timerlist = g_list_append(js_timerlist, tp);

	set_prop_int(&prop, (int)tp);
	value_to_jsval(ctx, rval, &prop);
}

static void js_cleartimer(int tid)
{
	tpara *tp = NULL;

	tp = (tpara*)tid;
	if ( g_list_find(js_timerlist, tp) ) {
		tp->canceled = 1;
	}
}

static gint js_timeoutproc(gpointer data)
{
	jsstring code;
	tpara *tp = (tpara*)data;
	jsscript_interpreter *interpreter = NULL;
	
	if ( !tp ) {
		return FALSE;
	}

	if ( tp->canceled ) {
		goto clear;
	}

	code.source = tp->jsscript;
	code.length = strlen(tp->jsscript);
	interpreter = *(tp->interpreter); 
	if (!interpreter || interpreter->inuse == 0 || interpreter->backend_data == NULL) {
		goto clear;
	}

	/* temp */
	js_eval(interpreter, &code);

	if ( tp->type == once) {
		goto clear;
	} else {
		return TRUE;
	}

clear:
	js_timerlist = g_list_remove(js_timerlist, tp);
	if ( tp->jsscript ) {
		g_free(tp->jsscript);
	}
	g_free(tp);

	return FALSE;
}

static jsobject *getdocobj(jsobject *wjsobj)
{
	GList *list = NULL;

	if ( !wjsobj ) {
		return NULL;
	}

	if ( !(list = wjsobj->children) || !(list->data) ) {
		return NULL;
	}

	if ( ((jsobject*)list->data)->jstype == jsdocument ) {
		return (jsobject*)list->data;
	}

	return NULL;
}

static mSpiderDoc* gettabwindow(jsobject *wjsobj)
{
	mSpiderDoc *doc = NULL;

	if ( wjsobj ) {
		doc = (mSpiderDoc*)wjsobj->htmlobj;
		while ( doc && doc->parent ) {
			doc = doc->parent;	
		}
		return doc;
	}
	return NULL;
}


static int getframecount(JSContext *ctx, JSObject *obj)
{
	jsobject *wjsobj = NULL;
	mSpiderDoc *doc = NULL;
	DwFrameset *fset = NULL;

	wjsobj = JS_GetPrivate(ctx, obj);
	if ( !wjsobj ) {
		return 0;
	}
	doc = (mSpiderDoc*)wjsobj->htmlobj;
	if ( !doc ) {
		return 0;
	}
	fset = doc->frameset;
	if ( !fset ) {
		return 0;
	}
	return fset->area_used;
}

/* note: get/set dd name */
static char* getname(JSContext *ctx, JSObject *obj)
{
	jsobject *wjsobj = NULL;

	wjsobj = JS_GetPrivate(ctx, obj);
	if ( wjsobj && wjsobj->htmlobj ) {
		return ((mSpiderDoc*)wjsobj->htmlobj)->name;
	}
	return NULL;
}

static void setname(JSContext *ctx, JSObject *obj, char* name)
{
	jsobject *wjsobj = NULL;
	mSpiderDoc *doc = NULL;

	if ( name ) {
		wjsobj = JS_GetPrivate(ctx, obj);
		if ( wjsobj ) {
			doc = (mSpiderDoc*)wjsobj->htmlobj;
			if ( doc ) {
				if ( doc->name ) {
					g_free(doc->name);
				}
				doc->name = g_strdup(name);
			}
		}
	}
}
static char* getstatus(JSContext *ctx, JSObject *obj)
{
	jsobject *wjsobj = NULL;
	mSpiderDoc *doc = NULL;

    wjsobj = JS_GetPrivate(ctx, obj);
    if ( wjsobj ) {
        doc = (mSpiderDoc*)wjsobj->htmlobj;
        if ( doc ){
             if (doc->bw->CB_GET_STATUS)
                return (*(doc->bw->CB_GET_STATUS))(doc->bw->main_window);
        }
	}
	return NULL;
}
static void setstatus(JSContext *ctx, JSObject *obj, char* msg)
{
	jsobject *wjsobj = NULL;
	mSpiderDoc *doc = NULL;

	if ( msg ) {
		wjsobj = JS_GetPrivate(ctx, obj);
		if ( wjsobj ) {
			doc = (mSpiderDoc*)wjsobj->htmlobj;
			if ( doc ) {
			  if (doc->bw->CB_SET_STATUS)
                (*(doc->bw->CB_SET_STATUS))(doc->bw->main_window, msg);
            } 
        }
	}
}

static JSObject* getparent(JSContext *ctx, JSObject *obj)
{
	jsobject *wjsobj = NULL;
	mSpiderDoc *doc = NULL;

	wjsobj = JS_GetPrivate(ctx, obj);
	if ( wjsobj ) {
		doc = (mSpiderDoc*)wjsobj->htmlobj; 
		if ( doc ) {
			if ( doc->parent&& doc->parent->jsobj ) {
				return doc->parent->jsobj->smobj;
            }else
				return doc->jsobj->smobj;
		}
	}
	return NULL;
}

static JSObject* gettopwin(JSContext *ctx, JSObject *obj)
{
	jsobject *wjsobj = NULL;
	mSpiderDoc *doc = NULL;

	wjsobj = JS_GetPrivate(ctx, obj);
	while ( wjsobj && (wjsobj->jstype != jstabwindow) ) {
		doc = (mSpiderDoc*)wjsobj->htmlobj;
		if ( !doc ) {
			break;
		}
		doc = doc->parent;
		if ( !doc ) {
			break;
		}
		wjsobj = doc->jsobj;
	}

	if ( wjsobj && (wjsobj->jstype == jstabwindow) ) {
		return wjsobj->smobj;
	}

	return NULL;
}

static char* getsrc(JSContext *ctx, JSObject *obj)
{
	jsobject *wjsobj = NULL;
	mSpiderDoc *doc = NULL;

	wjsobj = JS_GetPrivate(ctx, obj);
	if ( wjsobj ) {
		doc = (mSpiderDoc*)wjsobj->htmlobj;
		if ( doc ) {
			return URL_STR(doc->PageUrls->Url);
		}
	}
	return NULL;
}

static void setsrc(JSContext *ctx, JSObject *obj, char* url)
{
	jsobject *wjsobj = NULL;
	mSpiderDoc *doc = NULL;
	mSpiderUrl *durl = NULL;
	jsscript_interpreter *interpreter = NULL;

	interpreter = JS_GetContextPrivate(ctx);
	if ( interpreter->inuse == 2 ) {
		/* html parsering... delay porcess */
		return;
	}

	if ( url ) {
		wjsobj = JS_GetPrivate(ctx, obj);
		if ( wjsobj ) {
			doc = (mSpiderDoc*)wjsobj->htmlobj;
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
}

#endif
