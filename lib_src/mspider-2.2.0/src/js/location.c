#include <minigui/common.h>
#include <minigui/minigui.h>
#include <minigui/gdi.h>
#include <minigui/window.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "browser.h"
#include "dw_viewport.h"
#include "nav.h"
#include "interface.h"
#include "spidermonkey.h"
#include "jsmisc.h"

#include "url.h"
#ifdef JS_SUPPORT
extern mSpiderDoc *getdocsdoc(JSContext *ctx, JSObject *obj);
/* local data and function definition */
enum location_prop {
    JSP_LOC_HASH = 1,		
    JSP_LOC_HOST,		
    JSP_LOC_HOSTNAME,	
    JSP_LOC_HREF,		
    JSP_LOC_PATHNAME,	
    JSP_LOC_PORT,   	
    JSP_LOC_PROTOCOL,  	
    JSP_LOC_SEARCH
};

static JSBool location_get_property(JSContext *ctx, JSObject *obj, jsval id, jsval *vp);
static JSBool location_set_property(JSContext *ctx, JSObject *obj, jsval id, jsval *vp);
static JSBool location_reload(JSContext *ctx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);
static JSBool location_replace(JSContext *ctx, JSObject *obj, uintN argc,jsval *argv, jsval *rval);
static JSBool location_assign(JSContext *ctx, JSObject *obj, uintN argc,jsval *argv, jsval *rval);

static char* get_property(JSContext *ctx, JSObject *obj, jsval id);
static void set_property(JSContext *ctx, JSObject *obj, const char *href, jsval id);

static int get_port(JSContext *ctx, JSObject *obj);

/* export data sructure */
const JSClass location_class = {
	"location",
	JSCLASS_HAS_PRIVATE,
	JS_PropertyStub, JS_PropertyStub,
	location_get_property, location_set_property,
	JS_EnumerateStub, JS_ResolveStub, JS_ConvertStub, JS_FinalizeStub
};

const JSPropertySpec location_props[] = {
	{ "hash",		JSP_LOC_HASH,		JSPROP_ENUMERATE},
	{ "host",		JSP_LOC_HOST,		JSPROP_ENUMERATE},
	{ "hostname",	JSP_LOC_HOSTNAME,	JSPROP_ENUMERATE},
	{ "href",		JSP_LOC_HREF,		JSPROP_ENUMERATE},
	{ "pathname",	JSP_LOC_PATHNAME,	JSPROP_ENUMERATE},
	{ "port",   	JSP_LOC_PORT,   	JSPROP_ENUMERATE},
	{ "protocol",  	JSP_LOC_PROTOCOL,  	JSPROP_ENUMERATE},
	{ "search",		JSP_LOC_SEARCH,		JSPROP_ENUMERATE},
	{ NULL}
};

const JSFunctionSpec location_funcs[] = {
	{ "reload",		location_reload,	1 },
	{ "replace",	location_replace,	1 },
	{ "assign", 	location_assign,	1 },
	{ NULL }
};

/* location objects property and method */
static JSBool
location_get_property(JSContext *ctx, JSObject *obj, jsval id, jsval *vp)
{
	struct jsval_property prop;
    int port;

	set_prop_undef(&prop);
	if (JSVAL_IS_STRING(id)) {
		union jsval_union v;
		jsval_to_value(ctx, &id, JSTYPE_STRING, &v);
		return JS_TRUE;
	} else if (!JSVAL_IS_INT(id)) {
		return JS_TRUE;
	}

	switch (JSVAL_TO_INT(id)) {
        case JSP_LOC_HASH:
            set_prop_string(&prop, (unsigned char*)get_property(ctx, obj, JSP_LOC_HASH));
            break;
        case JSP_LOC_HOST:
            set_prop_string(&prop, (unsigned char*)get_property(ctx, obj, JSP_LOC_HOST));
            break;
        case JSP_LOC_HOSTNAME:
            set_prop_string(&prop, (unsigned char*)get_property(ctx, obj, JSP_LOC_HOSTNAME));
            break;
        case JSP_LOC_HREF:
            set_prop_string(&prop, (unsigned char*)get_property(ctx, obj, JSP_LOC_HREF));
            break;
        case JSP_LOC_PATHNAME:
            set_prop_string(&prop, (unsigned char*)get_property(ctx, obj, JSP_LOC_PATHNAME));
            break;
        case JSP_LOC_PORT:
            port = get_port(ctx, obj);
            if (port)
                set_prop_int(&prop, port);
            else if (port == -1)
                set_prop_string(&prop, NULL);
            break;
        case JSP_LOC_PROTOCOL:
            set_prop_string(&prop, (unsigned char*)get_property(ctx, obj, JSP_LOC_PROTOCOL));
            break;
        case JSP_LOC_SEARCH:
            set_prop_string(&prop, (unsigned char*)get_property(ctx, obj, JSP_LOC_SEARCH));
            break;
        default:
            return JS_TRUE;
	}

	value_to_jsval(ctx, vp, &prop);
	return JS_TRUE;
}

static JSBool
location_set_property(JSContext *ctx, JSObject *obj, jsval id, jsval *vp)
{
   	union jsval_union v;
	struct jsval_property prop;

	set_prop_undef(&prop);
	if (JSVAL_IS_STRING(id)) {
		jsval_to_value(ctx, &id, JSTYPE_STRING, &v);
		return JS_TRUE;
	} else if (!JSVAL_IS_INT(id)) {
		return JS_TRUE;
	}

    switch (JSVAL_TO_INT(id))
    {
        case JSP_LOC_HASH:
            jsval_to_value(ctx, vp, JSTYPE_STRING, &v);
            set_property(ctx, obj, (char*)(v.string), JSP_LOC_HASH);
            break;
        case JSP_LOC_HOST:
            jsval_to_value(ctx, vp, JSTYPE_STRING, &v);
            set_property(ctx, obj, (char*)(v.string), JSP_LOC_HOST);
            break;
        case JSP_LOC_HOSTNAME:
            jsval_to_value(ctx, vp, JSTYPE_STRING, &v);
            set_property(ctx, obj, (char*)(v.string), JSP_LOC_HOSTNAME);
            break;
        case JSP_LOC_HREF:
            jsval_to_value(ctx, vp, JSTYPE_STRING, &v);
            set_property(ctx, obj, (char*)(v.string), JSP_LOC_HREF);
            break;
        case JSP_LOC_PATHNAME:
            jsval_to_value(ctx, vp, JSTYPE_STRING, &v);
            set_property(ctx, obj, (char*)(v.string), JSP_LOC_PATHNAME);
            break;
        case JSP_LOC_PORT:
            jsval_to_value(ctx, vp, JSTYPE_STRING, &v);
            set_property(ctx, obj, (char*)(v.string), JSP_LOC_PORT);
            break;
        case JSP_LOC_PROTOCOL:
            jsval_to_value(ctx, vp, JSTYPE_STRING, &v);
            set_property(ctx, obj, (char*)(v.string), JSP_LOC_PROTOCOL);
            break;
        case JSP_LOC_SEARCH:
            jsval_to_value(ctx, vp, JSTYPE_STRING, &v);
            set_property(ctx, obj, (char*)(v.string), JSP_LOC_SEARCH);
            break;
        default:
            return JS_TRUE;
   }
    return JS_TRUE;
}

static JSBool
location_reload(JSContext *ctx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
	union jsval_union v;
	mSpiderDoc *doc = NULL;
	doc = getdocsdoc(ctx, obj);

	jsval_to_value(ctx, &argv[0], JSTYPE_STRING, &v);
     
    a_Nav_reload(doc);
	return JS_TRUE;
}

static JSBool
location_replace(JSContext *ctx, JSObject *obj, uintN argc,jsval *argv, jsval *rval)
{
	union jsval_union v;
    mSpiderUrl *url;
	mSpiderDoc *doc = NULL;

	doc = getdocsdoc(ctx, obj);

	jsval_to_value(ctx, &argv[0], JSTYPE_STRING, &v);

    url = a_Url_new((char*)(v.string), NULL, 0, 0, 0);
    if (url)
    {
       a_Nav_push(doc, url);
       a_Url_free(url);
    }
	return JS_TRUE;
}

static JSBool
location_assign(JSContext *ctx, JSObject *obj, uintN argc,jsval *argv, jsval *rval)
{
	union jsval_union v;
    mSpiderUrl *url;
	mSpiderDoc *doc = NULL;

	doc = getdocsdoc(ctx, obj);

	jsval_to_value(ctx, &argv[0], JSTYPE_STRING, &v);

    url = a_Url_new((char*)(v.string), NULL, 0, 0, 0);
    if (url)
    {
       a_Nav_push(doc, url);
       a_Url_free(url);
    }
#if 0
	struct jsval_property prop;
	JSObject *wobj = NULL;
	jsobject *wjsobj = NULL;
	mSpiderDoc *doc = NULL;

	wobj = JS_GetParent(ctx, obj);
	if ( wobj ) {
		wjsobj = JS_GetPrivate(ctx, wobj);
		if ( wjsobj ) {
			doc = (mSpiderDoc*)wjsobj->htmlobj;
			if ( doc ) {
				set_prop_string(&prop, (unsigned char*)URL_STR(doc->PageUrls->Url));
				value_to_jsval(ctx, rval, &prop);
			}
		}
	}
#endif
	return JS_TRUE;
}

static char* get_property(JSContext *ctx, JSObject *obj, jsval id)
{
	JSObject *wobj = NULL;
	jsobject *wjsobj = NULL;
	mSpiderDoc *doc = NULL;

	wobj = JS_GetParent(ctx, obj);
	if ( wobj ) {
		wjsobj = JS_GetPrivate(ctx, wobj);
		if ( wjsobj ) {
			doc = (mSpiderDoc*)wjsobj->htmlobj;
			if ( doc ) {
                switch (id) {
                    case JSP_LOC_HASH:
				        return (char *)URL_FRAGMENT(doc->PageUrls->Url);
                    case JSP_LOC_HOST:
				        return (char *)URL_AUTHORITY(doc->PageUrls->Url);
                    case JSP_LOC_HOSTNAME:
				        return (char *)URL_HOST(doc->PageUrls->Url);
                    case JSP_LOC_HREF:
				        return (char *)URL_STR(doc->PageUrls->Url);
                    case JSP_LOC_PATHNAME:
				        return (char *)URL_PATH(doc->PageUrls->Url);
                    case JSP_LOC_PROTOCOL:
                        return (char *)URL_SCHEME(doc->PageUrls->Url);
                    case JSP_LOC_SEARCH:
				        return (char *)URL_QUERY(doc->PageUrls->Url);
                    default:
                        return NULL;
                }
			}
		}
	}
	return NULL;
}

static int get_port(JSContext *ctx, JSObject *obj)
{
	JSObject *wobj = NULL;
	jsobject *wjsobj = NULL;
	mSpiderDoc *doc = NULL;

	wobj = JS_GetParent(ctx, obj);
	if ( wobj ) {
		wjsobj = JS_GetPrivate(ctx, wobj);
		if ( wjsobj ) {
			doc = (mSpiderDoc*)wjsobj->htmlobj;
			if ( doc ) {
                return URL_PORT(doc->PageUrls->Url);
            }
        }
    }
	return -1;
}
static void set_property(JSContext *ctx, JSObject *obj, const char *property, jsval id)
{
	JSObject *wobj = NULL;
	jsobject *wjsobj = NULL;
	mSpiderDoc *doc = NULL;
	mSpiderUrl *base_url = NULL;
	mSpiderUrl *new_url = NULL;
	mSpiderHtmlLB *lb = NULL;
    GString *gstr = NULL;
    gchar *private =NULL;
    static char buf[8];

	jsscript_interpreter *interpreter = NULL;

	interpreter = JS_GetContextPrivate(ctx);
	if ( interpreter->inuse == 2 ) {
		/* html parsering... delay porcess */
		return;
	}
    
    if (property == NULL)
        return;

	wobj = JS_GetParent(ctx, obj);
	if ( wobj ) {
		wjsobj = JS_GetPrivate(ctx, wobj);
		if ( wjsobj ) {
			doc = (mSpiderDoc*)wjsobj->htmlobj;
			if ( doc ) {
                lb = doc->html_block;
                if (lb){
                    base_url = lb->base_url;
                    switch (id){
                        case JSP_LOC_HREF:
                            private = correcturl(doc, property);
                            new_url = a_Url_new(private, NULL, 0, 0, 0);
                            break;
                        case JSP_LOC_PROTOCOL:
                            gstr = g_string_sized_new(124);
                            g_string_sprintf(
                                    gstr, "%s%s%s%s%s%s%s%s%s%s",
                                    property            ? property : "",
                                    property            ? ":" : "",
                                    base_url->authority ? "//" : "",
                                    base_url->authority ? base_url->authority : "",
                                    (base_url->path && base_url->path[0] != '/' && base_url->authority) ? "/" : "",
                                    base_url->path      ? base_url->path : "",
                                    base_url->query     ? "?" : "",
                                    base_url->query     ? base_url->query : "",
                                    base_url->fragment  ? "#" : "",
                                    base_url->fragment  ? base_url->fragment : "");
                            new_url = a_Url_new(gstr->str, NULL, 0, 0, 0);
                            g_string_free(gstr, TRUE);
                            break;
                        case JSP_LOC_PATHNAME:
                            gstr = g_string_sized_new(124);
                            g_string_sprintf(
                                    gstr, "%s%s%s%s%s%s%s%s%s%s",
                                    base_url->scheme    ? base_url->scheme : "",
                                    base_url->scheme    ? ":" : "",
                                    base_url->authority ? "//" : "",
                                    base_url->authority ? base_url->authority : "",
                                    (property && property[0] != '/' && base_url->authority) ? "/" : "",
                                    property            ? property : "",
                                    base_url->query     ? "?" : "",
                                    base_url->query     ? base_url->query : "",
                                    base_url->fragment  ? "#" : "",
                                    base_url->fragment  ? base_url->fragment : "");
                            new_url = a_Url_new(gstr->str, NULL, 0, 0, 0);
                            g_string_free(gstr, TRUE);
                            break;
                        case JSP_LOC_HOST:
                            if (strcasecmp(base_url->scheme, "http") !=0)
                                return;
                            gstr = g_string_sized_new(124);
                            g_string_sprintf(
                                    gstr, "%s%s%s%s%s%s%s%s%s%s",
                                    base_url->scheme    ? base_url->scheme : "",
                                    base_url->scheme    ? ":" : "",
                                    property            ? "//" : "",
                                    property            ? property : "",
                                    (base_url->path && base_url->path[0] != '/' && property) ? "/" : "",
                                    base_url->path      ? base_url->path : "",
                                    base_url->query     ? "?" : "",
                                    base_url->query     ? base_url->query : "",
                                    base_url->fragment  ? "#" : "",
                                    base_url->fragment  ? base_url->fragment : "");
                            new_url = a_Url_new(gstr->str, NULL, 0, 0, 0);
                            g_string_free(gstr, TRUE);
                            break;
                        case JSP_LOC_SEARCH:
                            gstr = g_string_sized_new(124);
                            g_string_sprintf(
                                    gstr, "%s%s%s%s%s%s%s%s%s%s",
                                    base_url->scheme    ? base_url->scheme : "",
                                    base_url->scheme    ? ":" : "",
                                    base_url->authority ? "//" : "",
                                    base_url->authority ? base_url->authority : "",
                                    (base_url->path && base_url->path[0] != '/' && property) ? "/" : "",
                                    base_url->path      ? base_url->path : "",
                                    property     ? "?" : "",
                                    property     ? property : "",
                                    base_url->fragment  ? "#" : "",
                                    base_url->fragment  ? base_url->fragment : "");
                            new_url = a_Url_new(gstr->str, NULL, 0, 0, 0);
                            g_string_free(gstr, TRUE);
                            break;
                        case JSP_LOC_HASH:
                            gstr = g_string_sized_new(124);
                            g_string_sprintf(
                                    gstr, "%s%s%s%s%s%s%s%s%s%s",
                                    base_url->scheme    ? base_url->scheme : "",
                                    base_url->scheme    ? ":" : "",
                                    base_url->authority ? "//" : "",
                                    base_url->authority ? base_url->authority : "",
                                    (base_url->path && base_url->path[0] != '/' && property) ? "/" : "",
                                    base_url->path      ? base_url->path : "",
                                    base_url->query     ? "?" : "",
                                    base_url->query     ? base_url->query : "",
                                    property            ? "#" : "",
                                    property            ? property : "");
                            new_url = a_Url_new(gstr->str, NULL, 0, 0, 0);
                            new_url->flags = 1;
                            g_string_free(gstr, TRUE);
                            break;
                        case JSP_LOC_HOSTNAME:
                            if (strcasecmp(base_url->scheme, "http") !=0)
                                return;
                            if (base_url->authority == NULL)
                                return;
                            private = strchr (base_url->authority, ':');
                            if (private != NULL)
                                sprintf(buf, "%d" ,URL_PORT_(base_url));
                            gstr = g_string_sized_new(124);
                            g_string_sprintf(
                                    gstr, "%s%s%s%s%s%s%s%s%s%s%s%s",
                                    base_url->scheme    ? base_url->scheme : "",
                                    base_url->scheme    ? ":" : "",
                                    property            ? "//" : "",
                                    property            ? property : "",
                                    private             ? ":" : "",
                                    buf                 ? buf : "",
                                    (base_url->path && base_url->path[0] != '/' && property) ? "/" : "",
                                    base_url->path      ? base_url->path : "",
                                    base_url->query     ? "?" : "",
                                    base_url->query     ? base_url->query : "",
                                    base_url->fragment  ? "#" : "",
                                    base_url->fragment  ? base_url->fragment : "");
                            new_url = a_Url_new(gstr->str, NULL, 0, 0, 0);
                            g_string_free(gstr, TRUE);
                            break;
                        case JSP_LOC_PORT:
                            if (strcasecmp(base_url->scheme, "http") !=0)
                                return;
                            if (base_url->authority == NULL)
                                return;
                            gstr = g_string_sized_new(124);
                            private =(char *)URL_HOST_(base_url);
                            g_string_sprintf(
                                    gstr, "%s%s%s%s%s%s%s%s%s%s%s%s",
                                    base_url->scheme    ? base_url->scheme : "",
                                    base_url->scheme    ? ":" : "",
                                    private             ? "//" : "",
                                    private             ? private : "",
                                    property            ? ":" : "",
                                    property            ? property : "",
                                    (base_url->path && base_url->path[0] != '/' && base_url->authority) ? "/" : "",
                                    base_url->path      ? base_url->path : "",
                                    base_url->query     ? "?" : "",
                                    base_url->query     ? base_url->query : "",
                                    base_url->fragment  ? "#" : "",
                                    base_url->fragment  ? base_url->fragment : "");
                            new_url = a_Url_new(gstr->str, NULL, 0, 0, 0);
                            g_string_free(gstr, TRUE);
                            break;
                        default:
                            return;
                    }
                    if ( new_url ) {
                        a_Nav_push(doc, new_url);
                        a_Url_free(new_url);
                    }
                }
            }
	    }
    }
}
#endif
