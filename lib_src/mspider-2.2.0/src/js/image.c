#include <minigui/common.h>
#include <minigui/minigui.h>
#include <minigui/gdi.h>
#include <minigui/window.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "html.h"
#include "spidermonkey.h"
#include "mgdconfig.h"
#include "mgwidget.h"
#include "image.h"
#include "web.h"
#include "dw_image.h"
#include "dw_viewport.h"
#include "dw_widget.h"
#include "dw_page.h"
#include "dicache.h"
#include "dw_widget.h"
#include "jsmisc.h"

#ifdef JS_SUPPORT
/* local data and function definition */

enum image_prop {
    JSP_IMG_ALIGN = 1,    
    JSP_IMG_ALT,      
    JSP_IMG_BORDER,   
    JSP_IMG_COMPLETE, 
    JSP_IMG_HEIGHT,   
    JSP_IMG_HSPACE,   
    JSP_IMG_ID,       
    JSP_IMG_ISMAP,    
    JSP_IMG_LONGDESC, 
    JSP_IMG_LOWSRC,   
    JSP_IMG_NAME,     
    JSP_IMG_SRC,      
    JSP_IMG_USEMAP,   
    JSP_IMG_VSPACE,   
    JSP_IMG_WIDTH    
};
static JSBool image_get_property(JSContext *ctx, JSObject *obj, jsval id, jsval *vp);
static JSBool image_set_property(JSContext *ctx, JSObject *obj, jsval id, jsval *vp);

static JSBool	image_blur(JSContext *ctx, JSObject *obj, uintN argc,jsval *argv, jsval *rval);
static JSBool	image_click(JSContext *ctx, JSObject *obj, uintN argc,jsval *argv, jsval *rval);
static JSBool 	image_focus(JSContext *ctx, JSObject *obj, uintN argc,jsval *argv, jsval *rval);

static void  	set_image_width(JSContext *ctx, JSObject *obj, const char *width_str);
static void  	set_image_height(JSContext *ctx, JSObject *obj, const char *height_str);
static void  	set_image_hspace(JSContext *ctx, JSObject *obj, const jsdouble *hspace);
static void  	set_image_vspace(JSContext *ctx, JSObject *obj, const jsdouble *vspace);
static void  	set_image_border(JSContext *ctx, JSObject *obj, const jsdouble *border);
static void  	set_image_usemap(JSContext *ctx, JSObject *obj, const char     *usemap);
/* export data sructure */
const JSClass image_class = {
	"image",
	JSCLASS_HAS_PRIVATE,
	JS_PropertyStub, JS_PropertyStub,
	image_get_property, image_set_property,
	JS_EnumerateStub, JS_ResolveStub, JS_ConvertStub, JS_FinalizeStub
};

const JSPropertySpec image_props[] = {
    { "align",      JSP_IMG_ALIGN,      JSPROP_ENUMERATE}, 		
    { "alt" ,       JSP_IMG_ALT,        JSPROP_ENUMERATE},
	{ "border",     JSP_IMG_BORDER,     JSPROP_ENUMERATE},		
	{ "complete",   JSP_IMG_COMPLETE,   JSPROP_ENUMERATE | JSPROP_READONLY},
    { "height",     JSP_IMG_HEIGHT,     JSPROP_ENUMERATE},
	{ "hspace",	    JSP_IMG_HSPACE,     JSPROP_ENUMERATE},
	{ "id",         JSP_IMG_ID,         JSPROP_ENUMERATE },
    { "isMap",      JSP_IMG_ISMAP,      JSPROP_ENUMERATE},
    { "longDesc",   JSP_IMG_LONGDESC,   JSPROP_ENUMERATE},
    { "lowsrc",     JSP_IMG_LOWSRC,     JSPROP_ENUMERATE},
	{ "name",       JSP_IMG_NAME,       JSPROP_ENUMERATE },
	{ "src",        JSP_IMG_SRC,        JSPROP_ENUMERATE },
	{ "useMap",     JSP_IMG_USEMAP,     JSPROP_ENUMERATE},
	{ "vspace",     JSP_IMG_VSPACE,     JSPROP_ENUMERATE},
    { "width",      JSP_IMG_WIDTH,      JSPROP_ENUMERATE},
	{ NULL }
};

const JSFunctionSpec image_funcs[] = {
	{ "blur",	image_blur,	     	0 },
    { "click",  image_click,        0 },
	{ "focus",	image_focus,		0 },
	{ NULL }
};

const char *image_propidx[] = {
	/* props */
	"align",		
	"alt",	
	"border",		
	"complete",			
    "height",
	"hspace",	
	"id",
    "isMap",
    "longDesc",
    "lowsrc",
	"name",
    "src",
	"useMap",
	"vspace",
    "width",
	/* events */
    "onAbort",
	"onBlur",
	"onClick",
	"onError",
	"onFocus",
	"onLoad"
};
const int image_propidxlen = sizeof(image_propidx)/sizeof(char*);
/* image objects property and method */
static JSBool
image_get_property(JSContext *ctx, JSObject *obj, jsval id, jsval *vp)
{
	struct jsval_property prop;
    int hspace, vspace, border, height, width;
    DwImage * dw;
    DwWidget * widget;
	jsobject *jsobj = NULL;
    char *str= NULL;

	set_prop_undef(&prop);
	jsobj = JS_GetPrivate(ctx, obj);

     dw = (DwImage *) jsobj->pvar1;
     widget = DW_WIDGET(dw);

	if (JSVAL_IS_STRING(id)) {
		union jsval_union v;
		jsval_to_value(ctx, &id, JSTYPE_STRING, &v);
		return JS_TRUE;
	} else if (!JSVAL_IS_INT(id)) {
		return JS_TRUE;
	}
	
	switch ( JSVAL_TO_INT(id) ) {
        case JSP_IMG_ALIGN:
            break;
         case JSP_IMG_ALT:
            str = dw->alt_text;
            set_prop_string(&prop, (unsigned char*)str);
            break;
         case JSP_IMG_BORDER:
            border = widget->style->border_width.top;
            if (border)
                set_prop_int(&prop, border);
            else
                set_prop_int(&prop, 0);
            break;
         case JSP_IMG_COMPLETE:
            if (dw->Y == (dw->height-1))
                set_prop_boolean(&prop, 1);
            else
                set_prop_boolean(&prop, 0);
            break;
         case JSP_IMG_HEIGHT:
            height = DW_WIDGET_CONTENT_HEIGHT(widget);
            set_prop_int(&prop, height);
            break;
         case JSP_IMG_HSPACE:
            hspace = widget->style->margin.left;
            if (hspace)
                set_prop_int(&prop, hspace);
            else
                set_prop_int(&prop, -1);
            break;
         case JSP_IMG_ID:
            if ( jsobj ) {
                if ( jsobj->jsid )
                    set_prop_string(&prop,(unsigned char*)jsobj->jsid);
            } 
            break;
         case JSP_IMG_ISMAP:
            set_prop_boolean(&prop, dw->ismap);
            break;
         case JSP_IMG_LONGDESC:
           break;
         case JSP_IMG_LOWSRC:
            break;
         case JSP_IMG_NAME:
            if ( jsobj ) {
                if ( jsobj->jsname )
                    set_prop_string(&prop,(unsigned char*)jsobj->jsname);
            } 
           break;
         case JSP_IMG_SRC:
           if ( jsobj ) {
               if ( jsobj->jssrc )
                   set_prop_string(&prop, (unsigned char *)jsobj->jssrc);
           }
           break;
         case JSP_IMG_USEMAP:
           if (dw->usemap_url){
               if ((unsigned char*)dw->usemap_url->fragment)
                   set_prop_string(&prop, (unsigned char*)dw->usemap_url->fragment);
           }
            break;
         case JSP_IMG_VSPACE:
            vspace = widget->style->margin.top;
            if (vspace)
                set_prop_int(&prop, vspace);
            else
                set_prop_int(&prop, -1);
            break;
         case JSP_IMG_WIDTH:
            width = DW_WIDGET_CONTENT_WIDTH(widget);
            set_prop_int(&prop, width);
            break;
         default:
            break;
    }
	value_to_jsval(ctx, vp, &prop);
	return JS_TRUE;
}

static JSBool
image_set_property(JSContext *ctx, JSObject *obj, jsval id, jsval *vp)
{
	jsobject *jsobj = NULL;
	union jsval_union v;
    mSpiderDoc *imagedoc;
    mSpiderUrl *url;
    mSpiderImage *Image;
    DwImage *dw;
    DICacheEntry *DicEntry = NULL;
    int index;

	jsobj = JS_GetPrivate(ctx, obj);
    imagedoc = (mSpiderDoc *)gethtmldoc(jsobj);
    
    dw =(DwImage *) jsobj->pvar1;

    if (JSVAL_IS_STRING(id)) {
		union jsval_union v;
		jsval_to_value(ctx, &id, JSTYPE_STRING, &v);
		return JS_TRUE;
	} else if (!JSVAL_IS_INT(id)) {
		return JS_TRUE;
	}
	
	switch ( JSVAL_TO_INT(id) ) {
        case JSP_IMG_ALIGN:
            break;
         case JSP_IMG_ALT:
            jsval_to_value(ctx, vp, JSTYPE_STRING, &v);
            if (dw->alt_text != NULL) g_free (dw->alt_text);
            dw->alt_text =strdup ((char *) (v.string));
            index = get_props_index(image_propidx, image_propidxlen, "alt");
            if (jsobj->jsprops[index]!=NULL) g_free (jsobj->jsprops[index]);
            jsobj->jsprops[index] = strdup((char *)(v.string));
            break;
         case JSP_IMG_BORDER:
            jsval_to_value(ctx, vp, JSTYPE_NUMBER, &v);
            set_image_border(ctx, obj, (jsdouble *)(v.number));
            break;
         case JSP_IMG_HEIGHT:
            jsval_to_value(ctx, vp, JSTYPE_STRING, &v);
            set_jsobj_props(image_propidx,image_propidxlen,jsobj,"height", (char *)v.string);
            set_image_height(ctx, obj, (char *)(v.string));
            break;
         case JSP_IMG_HSPACE:
            jsval_to_value(ctx, vp, JSTYPE_NUMBER, &v);
            set_image_hspace(ctx, obj, (jsdouble *)(v.number));
            break;
         case JSP_IMG_ID:
            jsval_to_value(ctx, vp, JSTYPE_STRING, &v);
            if(jsobj->jsid!=NULL) g_free(jsobj->jsid);
            jsobj->jsid=strdup((char*)(v.string)); 
            break;
         case JSP_IMG_ISMAP:
            jsval_to_value(ctx, vp, JSTYPE_BOOLEAN, &v);
            if(v.boolean) {
               dw->ismap=1; 
               set_jsobj_props(image_propidx,image_propidxlen,jsobj,"isMap", "true");
            } else {    
               dw->ismap=0; 
               set_jsobj_props(image_propidx,image_propidxlen,jsobj,"isMap", "false");
            }
            break;
         case JSP_IMG_LONGDESC:
           break;
         case JSP_IMG_LOWSRC:
            break;
         case JSP_IMG_NAME:
            jsval_to_value(ctx, vp, JSTYPE_STRING, &v);
            if(jsobj->jsname!=NULL) g_free(jsobj->jsname);
            jsobj->jsname=strdup((char*)(v.string));
           break;
         case JSP_IMG_SRC:
           if ( jsobj && dw->is_widget && 
               (dw->Y == 0 || (dw->height-1) == dw->Y || dw->height == dw->Y)) {
               jsval_to_value(ctx, vp, JSTYPE_STRING, &v);
               
                a_Cache_stop_client(dw->client_id);
                a_Doc_remove_client(imagedoc, dw->client_id);
               
               if (jsobj->jssrc!=NULL) g_free (jsobj->jssrc);
                jsobj->jssrc =strdup((char *) correcturl(imagedoc,(char*)v.string));
                url = a_Url_new(jsobj->jssrc, NULL, 0, 0, 0); 
                if (dw->scaled_buffer) {
                    g_free (dw->scaled_buffer);
                    dw->scaled_buffer = NULL;
                }
                dw->buffer = NULL;
#ifdef ENABLE_ANIMATION
                if (dw->timer)
                   g_source_remove (dw->timer);
                if (dw->frames)
                   destroy_gif_frames (dw);
                if (dw->cmap)
                   g_free (dw->cmap);

                dw->cmap = NULL;
                dw->frames = NULL;
                dw->nr_frames = 0;
#endif
                Image = a_Image_jsnew(dw);
                //DicEntry = a_Dicache_get_entry(url);
                DicEntry = a_Dicache_ref (url, 0);
                if ( !DicEntry ) {
                    Html_reload_image(imagedoc,url, Image);
                } else {
                    dw->width = DicEntry->width;
                    dw->height =DicEntry->height;
                    dw->Y =DicEntry->height-1;
                    dw->client_id = -1;
                    a_Dw_image_set_buffer (dw, DicEntry->ImageBuffer, DicEntry->url, DicEntry->version);
                    p_Dw_widget_queue_draw(DW_WIDGET(dw));
                }
                a_Url_free (url);
            }
           break;
         case JSP_IMG_USEMAP:
            jsval_to_value(ctx, vp, JSTYPE_STRING, &v);
            set_image_usemap(ctx, obj, (char *) (v.string));
            break;
         case JSP_IMG_VSPACE:
            jsval_to_value(ctx, vp, JSTYPE_NUMBER, &v);
            set_image_vspace(ctx, obj, (jsdouble *)(v.number));
            break;
         case JSP_IMG_WIDTH:
            jsval_to_value(ctx, vp, JSTYPE_STRING, &v);
            set_jsobj_props(image_propidx,image_propidxlen,jsobj,"width", (char *)v.string);
            set_image_width(ctx, obj, (char *)(v.string));
            break;
         default:
            break;
    }
	return JS_TRUE;
}
static JSBool	image_blur(JSContext *ctx, JSObject *obj, uintN argc,jsval *argv, jsval *rval)
{
	return JS_TRUE;
}
static JSBool	image_click(JSContext *ctx, JSObject *obj, uintN argc,jsval *argv, jsval *rval)
{
	jsobject *jsobj = NULL;
    DwImage *dw;
    mSpiderDoc *dd;
    DwWidget *widget;
    int link = 0;
	
    jsobj = JS_GetPrivate(ctx, obj);

   	if ( !jsobj || !(jsobj->htmlobj) ) {
		return JS_TRUE;
	}

    dw = (DwImage *) jsobj->pvar1;
    dd = (mSpiderDoc *) jsobj->pvar3;
    widget = DW_WIDGET(dw);
    link = widget->style->x_link;
    if (link == -1)
        return JS_TRUE;

    Html_link_clicked (widget, link, 0, 0, 0x00020000, (mSpiderHtmlLB *)dd->html_block);
    InvalidateRect (widget->viewport->hwnd, NULL, TRUE);
    return JS_TRUE;
}
static JSBool 	image_focus(JSContext *ctx, JSObject *obj, uintN argc,jsval *argv, jsval *rval)
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
    SetFocusChild(hwnd);
    return JS_TRUE;
}

static void	set_image_width(JSContext *ctx, JSObject *obj, const char *width_str)
{
    DwImage *dw;   
    DwWidget *widget;
    DwPage *page;
    jsobject *jsobj = NULL;

	jsobj = JS_GetPrivate(ctx, obj);
	if ( !jsobj || !(jsobj->htmlobj) ) {
		return;
	}

    dw = (DwImage *) jsobj->pvar1;
    widget = DW_WIDGET (dw);

    page = (DwPage *)(jsobj->pvar2);
    
    widget->style->width = width_str ?
        Html_parse_length (NULL, width_str) : DW_STYLE_LENGTH_AUTO;
    if ((gulong)widget->style->width*widget->style->height
         > (gulong)(G_MAXLONG >> 1)) {
      widget->style->width = DW_STYLE_LENGTH_AUTO;
      widget->style->height = DW_STYLE_LENGTH_AUTO;
   }

    p_Dw_widget_queue_resize(widget, 0, TRUE);
    Dw_page_rewrap (page);
}
static void	set_image_height(JSContext *ctx, JSObject *obj, const char *height_str)
{
    DwImage *dw;   
    DwWidget *widget;
    DwPage *page;
    jsobject *jsobj = NULL;

	jsobj = JS_GetPrivate(ctx, obj);
	if ( !jsobj || !(jsobj->htmlobj) ) {
		return;
	}

    dw = (DwImage *) jsobj->pvar1;
    widget = DW_WIDGET (dw);

    page = (DwPage *)(jsobj->pvar2);
    
    widget->style->height = height_str ?
        Html_parse_length (NULL, height_str) : DW_STYLE_LENGTH_AUTO;

    if ((gulong)widget->style->width*widget->style->height
         > (gulong)(G_MAXLONG >> 1)) {
      widget->style->width = DW_STYLE_LENGTH_AUTO;
      widget->style->height = DW_STYLE_LENGTH_AUTO;
   }

    p_Dw_widget_queue_resize(widget, 0, TRUE);
    Dw_page_rewrap (page);
}
static void	set_image_hspace(JSContext *ctx, JSObject *obj, const jsdouble *hspace)
{
    DwImage *dw;   
    DwWidget *widget;
    DwPage *page;
    jsobject *jsobj = NULL;

	jsobj = JS_GetPrivate(ctx, obj);
	if ( !jsobj || !(jsobj->htmlobj) ) {
		return;
	}

    dw = (DwImage *) jsobj->pvar1;
    widget = DW_WIDGET (dw);

    widget->style->margin.left = widget->style->margin.right = *hspace;
   
    p_Dw_widget_queue_resize(widget, 0, TRUE);
    page = (DwPage *)(jsobj->pvar2);
    Dw_page_rewrap (page);
}
static void	set_image_vspace(JSContext *ctx, JSObject *obj, const jsdouble *vspace)
{
    DwImage *dw;   
    DwWidget *widget;
    DwPage *page;
    jsobject *jsobj = NULL;

	jsobj = JS_GetPrivate(ctx, obj);
	if ( !jsobj || !(jsobj->htmlobj) ) {
		return;
	}

    dw = (DwImage *) jsobj->pvar1;
    widget = DW_WIDGET (dw);

    widget->style->margin.top = widget->style->margin.bottom = *vspace;
   
    p_Dw_widget_queue_resize(widget, 0, TRUE);
    page = (DwPage *)(jsobj->pvar2);
    Dw_page_rewrap (page);
}
static void	set_image_border(JSContext *ctx, JSObject *obj, const jsdouble *border)
{
    DwImage *dw;   
    DwWidget *widget;
    DwPage *page;
    jsobject *jsobj = NULL;
    mSpiderDoc *dd;
	
    jsobj = JS_GetPrivate(ctx, obj);
	if ( !jsobj || !(jsobj->htmlobj) ) {
		return;
	}

    dw = (DwImage *) jsobj->pvar1;
    widget = DW_WIDGET (dw);
    dd = (mSpiderDoc *)jsobj->pvar3;

    a_Dw_style_box_set_border_color (widget->style,
            a_Dw_style_shaded_color_new (widget->style->color->color_val, dd->bw->main_window));

    a_Dw_style_box_set_border_style (widget->style, DW_STYLE_BORDER_SOLID);
    a_Dw_style_box_set_val (&(*(DwStyle *)widget->style).border_width, *border);

    p_Dw_widget_queue_resize(widget, 0, TRUE);
    page = (DwPage *)(jsobj->pvar2);

    Dw_page_rewrap (page);
}
static void	set_image_usemap(JSContext *ctx, JSObject *obj, const char *usemap)
{
    DwImage *dw;   
    DwWidget *widget;
    DwViewport * viewport;
    DwPage *page;
    jsobject *jsobj = NULL;
    mSpiderUrl *usemap_url = NULL;
    mSpiderDoc *dd;
    
    jsobj = JS_GetPrivate(ctx, obj);
	if ( !jsobj || !(jsobj->htmlobj) ) {
		return;
	}
    
    dw = (DwImage *) jsobj->pvar1;
    widget = DW_WIDGET (dw);
    dd = (mSpiderDoc *)jsobj->pvar3;
    
    viewport = widget->viewport;
    usemap_url = a_Url_new(usemap, URL_STR_(((mSpiderHtmlLB *)dd->html_block)->base_url), 0, 0, 0);
    if (usemap_url) {
        a_Dw_image_set_usemap (dw, &((mSpiderHtmlLB *)dd->html_block)->maps, usemap_url);
        a_Url_free (usemap_url);
    } 

    p_Dw_widget_queue_resize(widget, 0, TRUE);
    
    page = (DwPage *)(jsobj->pvar2);
    Dw_page_rewrap (page);

}
const char* getimageeventstr(jsobject *jsobj, int rc)
{
	int idx = -1;
	switch ( rc ) {
	case BN_CLICKED:
		idx = get_props_index(image_propidx, image_propidxlen, "onClick");
		break;
	case BN_PAINT:
		break;
	case BN_HILITE:	/* BN_PUSHED */
		break;
	case BN_UNHILITE:	/* BN_UNPUSHED */
		break;
	case BN_DISABLE:
		break;
	case BN_DOUBLECLICKED:	/* BN_DBLCLK */
		break;
	case BN_SETFOCUS:
		idx = get_props_index(image_propidx, image_propidxlen, "onFocus");
		break;
	case BN_KILLFOCUS:
		idx = get_props_index(image_propidx, image_propidxlen, "onBlur");
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
