#include <minigui/common.h>
#include <minigui/minigui.h>
#include <minigui/gdi.h>
#include <minigui/window.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "browser.h"
#include "dw_viewport.h"
#include "url.h"
#include "nav.h"
#include "interface.h"
#include "spidermonkey.h"
#include "jsmisc.h"

#ifdef JS_SUPPORT
/* local data and function definition */
enum navigator_prop {
	JSP_NAV_APPCODENAME = 1,
    JSP_NAV_APPNAME,
    JSP_NAV_APPVERSION,
    JSP_NAV_COOKIEENABLED,
    JSP_NAV_PLATFORM,
    JSP_NAV_USERAGENT
};
static JSBool navigator_get_property(JSContext *ctx, JSObject *obj, jsval id, jsval *vp);
static JSBool navigator_set_property(JSContext *ctx, JSObject *obj, jsval id, jsval *vp);
static JSBool navigator_javaEnabled(JSContext *ctx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);
static JSBool navigator_taintEnabled(JSContext *ctx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);

/* export data sructure */
const JSClass navigator_class = {
	"navigator",
	JSCLASS_HAS_PRIVATE,
	JS_PropertyStub, JS_PropertyStub,
	navigator_get_property, navigator_set_property,
	JS_EnumerateStub, JS_ResolveStub, JS_ConvertStub, JS_FinalizeStub
};

const JSPropertySpec navigator_props[] = {
	{ "appCodeName",	JSP_NAV_APPCODENAME,	JSPROP_ENUMERATE | JSPROP_READONLY },
	{ "appName",		JSP_NAV_APPNAME,		JSPROP_ENUMERATE | JSPROP_READONLY },
	{ "appVersion",		JSP_NAV_APPVERSION,		JSPROP_ENUMERATE | JSPROP_READONLY },
	{ "cookieEnabled",	JSP_NAV_COOKIEENABLED,	JSPROP_ENUMERATE | JSPROP_READONLY },
	{ "platform",		JSP_NAV_PLATFORM,		JSPROP_ENUMERATE | JSPROP_READONLY },
	{ "userAgent",		JSP_NAV_USERAGENT,		JSPROP_ENUMERATE | JSPROP_READONLY },
	{ NULL}
};

const JSFunctionSpec navigator_funcs[] = {
	{ "javaEnabled",	navigator_javaEnabled,	0 },
	{ "taintEnabled",	navigator_taintEnabled,	0 },
	{ NULL }
};

/* navigator objects property and method */
static JSBool
navigator_get_property(JSContext *ctx, JSObject *obj, jsval id, jsval *vp)
{
	char buf[0x100];
	struct jsval_property prop;
	FILE *fp = NULL;

	set_prop_undef(&prop);
	if (JSVAL_IS_STRING(id)) {
		union jsval_union v;
		jsval_to_value(ctx, &id, JSTYPE_STRING, &v);
		return JS_TRUE;
	} else if (!JSVAL_IS_INT(id)) {
		return JS_TRUE;
	}

	switch (JSVAL_TO_INT(id)) {
	case JSP_NAV_APPCODENAME:
		set_prop_string(&prop, (unsigned char*)"mSpider");
		break;
    case JSP_NAV_APPNAME:
		set_prop_string(&prop, (unsigned char*)"mSpider");
		break;
	case JSP_NAV_APPVERSION:
		set_prop_string(&prop, (unsigned char*)"1.8.0");
		break;
	case JSP_NAV_COOKIEENABLED:
		set_prop_boolean(&prop, 0);
#ifdef ENABLE_COOKIES
		set_prop_boolean(&prop, 1);
#endif
		break;
	case JSP_NAV_PLATFORM:
		set_prop_string(&prop, (unsigned char*)"Linux i686");
		fp = fopen("/proc/version", "r");
		if ( fp ) {
			if ( fgets(buf, 0x100, fp) > 0 ) {
				set_prop_string(&prop, (unsigned char*)buf);
			}
			fclose(fp);
		}
		break;
	case JSP_NAV_USERAGENT:
		set_prop_string(&prop, (unsigned char*)"welcome to http://www.minigui.com");
		break;
	default:
		break;
	}

	value_to_jsval(ctx, vp, &prop);
	return JS_TRUE;
}

static JSBool
navigator_set_property(JSContext *ctx, JSObject *obj, jsval id, jsval *vp)
{
	return JS_TRUE;
}

static JSBool 
navigator_javaEnabled(JSContext *ctx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
	struct jsval_property prop;

	set_prop_boolean(&prop, 0);
	value_to_jsval(ctx, rval, &prop);

	return JS_TRUE;
}

static JSBool 
navigator_taintEnabled(JSContext *ctx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
	struct jsval_property prop;

	set_prop_boolean(&prop, 0);
	value_to_jsval(ctx, rval, &prop);

	return JS_TRUE;
}

#endif
