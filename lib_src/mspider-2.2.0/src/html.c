/* * File: html.c *
 *
 * Copyright (C) 2005-2006 Feynman Software
 * Copyright (C) 1997 Raph Levien <raph@acm.org>
 * Copyright (C) 1999 James McCollough <jamesm@gtwn.net>
 *
 * This program is g_free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

/*
 * mSpider HTML parsing routines
 */

#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <errno.h>

#include "mgdconfig.h"
#include "html.h"

#include "dw_widget.h"
#include "mgwidget.h"
#include "dw_viewport.h"
#include "dw_page.h"
#include "dw_bullet.h"
#include "dw_button.h"
#include "dw_hruler.h"
#include "dw_table.h"
#include "dw_table_cell.h"
#include "dw_list_item.h"
#include "dw_style.h"

#include "msg.h"
#include "debug.h"
#include "list.h"
#include "colors.h"
#include "prefs.h"
#include "cache.h"
#include "web.h"
#include "mspider.h"
#include "interface.h"
#include "history.h"
#include "nav.h"
#include "url.h"
#include "progressbar.h"
#include "binaryconst.h"
#include "dw_tooltip.h"
#include "doc.h"
#include "frameset.h"
#include "nav.h"
#include "io/url_io.h"
#include "linktrave.h"
#include "capi.h"

#include "spidermonkey.h"
#include "js.h"
#include "form.h"

#ifdef ENABLE_FLASH
#include "mflash.h"

HWND flashhwnd;

extern int soundonly,soundsecond;
extern int tt;
extern int tasknum;
extern int nn;
#endif

static int flashflag=0;

DwStyleBgImage* a_Dw_style_bgimage_new (DwImage *image);

#ifdef JS_SUPPORT
JSObject *get_img_object(JSContext *ctx, JSObject *pobj, jsobject *jsobj);
JSObject *get_form_object(JSContext *ctx, JSObject *pobj, jsobject *jsobj);
#endif

int stop_flag = 0;
#define USE_TABLES
#define SGML_SPCDEL 0

/*define the ID for all kinds of control*/

#define IDC_PASSWORD 1
#define IDC_SUBMIT   1

#ifdef AUTOFOLD_SUPPORT
    char tabstatck[0x40];
    int intable;
#endif
mSpiderPrefs prefs;
extern void a_MG_close_client (BrowserWindow* bw, int ClientKey);

static int mspider_dbg_rendering = 0;

extern char* set_charset;

#define DEBUG_LEVEL 10

#include "debug.h"

typedef void (*TagOpenFunct) (mSpiderHtml *Html, char *Tag, gint Tagsize);
typedef void (*TagCloseFunct) (mSpiderHtml *Html, gint TagIdx);

#undef isspace
#define isspace my_isspace

static inline int my_isspace (int ch)
{
    if (ch == 0x20 || (ch < 0x0E && ch > 0x08))
        return 1;
    return 0;
}

#define TAB_SIZE 8
/*
 * Forward declarations
 */
const char *Html_get_attr(mSpiderHtml *html,
                                 const char *tag,
                                 int tagsize,
                                 const char *attrname);
static const char *Html_get_attr2(mSpiderHtml *html,
                                  const char *tag,
                                  int tagsize,
                                  const char *attrname,
                                  mSpiderHtmlTagParsingFlags flags);
static char *Html_get_attr_wdef(mSpiderHtml *html,
                                const char *tag,
                                gint tagsize,
                                const char *attrname,
                                const char *def);
static void Html_add_widget(mSpiderHtml *html, DwWidget *widget,
                            char *width_str, char *height_str,
                            DwStyle *style_attrs);
int Html_write_raw(mSpiderHtml *html, char *buf, int bufsize, int Eof);
static void Html_write(mSpiderHtml *html, char *Buf, int BufSize, int Eof);
static void Html_close(mSpiderHtml *html, int ClientKey);
static void Html_callback(int Op, CacheClient_t *Client);
static mSpiderHtml *Html_new(mSpiderDoc * dd, const mSpiderUrl *url,
                const char* content_type);
static void Html_tag_open_input(mSpiderHtml *html, char *tag, int tagsize);
static int Html_add_input(mSpiderHtmlForm *form,
                           mSpiderHtmlInputType type,
                           DwMgWidget *widget,
                           const char *name,
                           const char *init_str,
                           mSpiderHtmlSelect *select,
                           gboolean init_val);
static gint Html_tag_index(char *tag);

/* exported function */
DwWidget *a_Html_text(const char *Type, void *P, CA_Callback_t *Call,
                      void **Data);
static mSpiderImage *Html_add_new_image(mSpiderHtml *html, char *tag,
                                      int tagsize, DwStyle *style_attrs,
                                      gboolean add);
static void Html_load_image(mSpiderHtml *html, mSpiderUrl *url, mSpiderImage *Image);

#ifdef JS_SUPPORT
char *vform = "<FORM NAME=vform METHOD=post ACTION="" target=""> </form>";
static jsobject *lastformobj;
static int inputid;
static int formid;
static jsobject *lastselobj;
static int GetJSFile(char *jsurl, gpointer data);
#endif 
#ifdef ENABLE_FLASH
static int GetFlashFile(char *url,HWND flash);
#endif
/*
 * Local Data
 */

/* The following array of font sizes has to be _strictly_ crescent */
static const int FontSizes[] = {8, 10, 12, 14, 16, 20};
static const int FontSizesNum = 6;
static const int FontSizesBase = 2;

/* Parsing table structure */
typedef struct {
   gchar *name;           /* element name */
   unsigned char Flags;   /* flags (explained near the table data) */
   gchar EndTag;          /* Is it Required, Optional or Forbidden */
   guchar TagLevel;       /* Used to heuristically parse bad HTML  */
   TagOpenFunct open;     /* Open function */
   TagCloseFunct close;   /* Close function */
} TagInfo;
static const TagInfo Tags[];

/*
 * Return the line number of the tag being processed by the parser.
 */
static int Html_get_line_number(mSpiderHtml *html)
{
    int i, ofs, line;
    const char *p = html->Start_Buf;

    if (p == NULL) return -1;

   ofs = (html->Buf_Size < html->CurrTagOfs)
      ? html->Buf_Size : html->CurrTagOfs;

    line = html->OldTagLine;
    for (i = html->OldTagOfs; i < ofs; ++i)
        if (p[i] == '\n')
            ++line;
    html->OldTagOfs = html->CurrTagOfs;
    html->OldTagLine = line;
    return line;
}

/*
 * Collect HTML error strings inside the linkblock.
 */
static void Html_msg (mSpiderHtml *html, const char *format, ... )
{
    va_list argp;
    char buf[512];

    snprintf (buf, 512, "HTML warning: line %d, ",
                Html_get_line_number(html));
    g_string_append (html->linkblock->page_bugs, buf);
    va_start (argp, format);
    vsnprintf (buf, 512, format, argp);
    va_end (argp);
    g_string_append (html->linkblock->page_bugs, buf);

#if 0
    a_Interface_bug_meter_update (html->bw, 
                                    ++html->linkblock->num_page_bugs);
#else
#endif
}

/*
 * Wrapper for a_Url_new that adds an error detection message.
 * (if use_base_url is TRUE, html->linkblock->base_url is used)
 */
static mSpiderUrl *Html_url_new(mSpiderHtml *html,
                              const char *url_str, const char *base_url,
                              int flags, Sint32 posx, Sint32 posy,
                              int use_base_url)
{
    mSpiderUrl *url;
    int n_ic;

    url = a_Url_new (url_str,
            (use_base_url) ? base_url : URL_STR_(html->linkblock->base_url),
            flags, posx, posy);
    if (url && (n_ic = URL_ILLEGAL_CHARS(url)) != 0)
        MSG_HTML("URL has %d illegal character%s (00-1F, 7F or space)\n",
                n_ic, (n_ic) > 1 ? "s" : "");
    return url;
}

/*
 * Set callback function and callback data for "html/text" MIME type.
 */
DwWidget *a_Html_text(const char *Type, void *P, CA_Callback_t *Call,
                      void **Data)
{
    mSpiderWeb *web = P;
    mSpiderHtml *html = Html_new(web->dd, web->url, Type);

    flashflag=0;
    *Data = (void *) html;
    *Call = (CA_Callback_t) Html_callback;

#ifdef ENABLE_FLASH
    soundonly=1;
    soundsecond=1;
    nn=0;
    tt=1;
#endif
    return html->dw;
}

/*
 * We'll make the linkblock first to get it out of the way.
 */
static mSpiderHtmlLB *Html_lb_new (mSpiderDoc *dd, const mSpiderUrl *url)
{
    mSpiderHtmlLB *html_lb = g_malloc0 (sizeof (mSpiderHtmlLB));

    html_lb->dd = dd;
    html_lb->base_url = a_Url_dup(url);
    html_lb->num_forms_max = 1;
    html_lb->num_forms = 0;
    html_lb->forms = NULL;

    html_lb->num_links_max = 1;
    html_lb->num_links = 0;
    html_lb->links = NULL;
    a_Dw_image_map_list_init (&html_lb->maps);

    html_lb->link_color = prefs.link_color;
    html_lb->visited_color = prefs.visited_color;

    html_lb->num_page_bugs = 0;
    html_lb->page_bugs = g_string_new("");

#ifdef JS_SUPPORT
    html_lb->images = NULL;
#endif
    return html_lb;
}

/*
 * Free the memory used by the linkblock
 */
static void Html_lb_free(void *lb)
{
    int i, j, k;
    mSpiderHtmlForm *form;
    mSpiderHtmlLB *html_lb = lb;

    DEBUG_MSG (3, "Html_lb_free called\n");

    a_Url_free (html_lb->base_url);

    for (i = 0; i < html_lb->num_forms; i++) {
        form = &html_lb->forms[i];
        a_Url_free (form->action);
        for (j = 0; j < form->num_inputs; j++) 
        {
            g_free (form->inputs[j].name);
            g_free (form->inputs[j].init_str);

            if (form->inputs[j].type == MSPIDER_HTML_INPUT_SELECT ||
                    form->inputs[j].type == MSPIDER_HTML_INPUT_SEL_LIST) {
                for (k = 0; k < form->inputs[j].select->num_options; k++) {
                    g_free (form->inputs[j].select->options[k].value);
                    g_free (form->inputs[j].select->options[k].menuitem);
                }
                g_free (form->inputs[j].select->options);
                g_free (form->inputs[j].select);
            }
        }
        g_free (form->inputs);
    }
    g_free (html_lb->forms);

    for (i = 0; i < html_lb->num_links; i++)
        if (html_lb->links[i])
            a_Url_free (html_lb->links[i]);
    g_free (html_lb->links);

    a_Dw_image_map_list_free (&html_lb->maps);

    g_string_free (html_lb->page_bugs, TRUE);

#ifdef JS_SUPPORT
    g_list_free(html_lb->images);
#endif

    g_free (html_lb);
}

#if 0
/*
 * Set the URL data for image maps.
 */
static void Html_set_link_coordinates (mSpiderHtmlLB *lb,
                                      int link, int x, int y)
{
   char data [64];

   if (x != -1) {
      snprintf (data, 64, "?%d,%d", x, y);
      a_Url_set_ismap_coords (lb->links[link], data);
   }
}
#endif

/*
 * Handle the status function generated by the dw scroller,
 * and show the url in the browser status-bar.
 */
static void Html_handle_status (DwWidget *widget, int link, int x, int y,
                               mSpiderHtmlLB *lb)
{
   mSpiderUrl *url;


   url = (link == -1) ? NULL : lb->links[link];
   if (url) {
//      Html_set_link_coordinates(lb, link, x, y);
      a_Interface_msg(lb->dd, "%s",
                         URL_ALT_(url) ? URL_ALT_(url) : URL_STR_(url));
      a_Dw_widget_set_cursor (widget, GetSystemCursor (IDC_HAND_POINT));
      lb->dd->status_is_link = 1;

   } else {
      if (lb->dd->status_is_link)
         a_Interface_msg(lb->dd, "");
      a_Dw_widget_set_cursor (widget, GetSystemCursor (IDC_ARROW));
   }
}

#if 0
/*
 * Popup the link menu ("link_pressed" callback of the page)
 */
{
    DwWidget *widget_at_cursor;
    gboolean show_oi = FALSE;

        Html_set_link_coordinates (lb, link, x, y);
        a_Menu_popup_set_url (lb->bw, lb->links[link]);

        /* if we've got an image, prepare the image popup */
        widget_at_cursor =
            a_Dw_gtk_scrolled_window_widget_at_viewport_point (
                GTK_DW_SCROLLED_WINDOW (lb->bw->docwin), event->x, event->y);
        if (widget_at_cursor && DW_IS_IMAGE (widget_at_cursor)) {
            DwImage *image = DW_IMAGE (widget_at_cursor);
            /* test image->url (it may have not started to arrive yet!) */
            if (image->url) {
                /* use the second URL for this popup */
                gtk_object_set_data (GTK_OBJECT (lb->bw->menu_popup.over_image),
                                "url2", GINT_TO_POINTER(2));
                a_Menu_popup_set_url2 (lb->bw, image->url);
                show_oi = TRUE;
            }
        }
        a_Menu_popup_ol_show_oi (lb->bw, show_oi);

        gtk_menu_popup (GTK_MENU (lb->bw->menu_popup.over_link), NULL, NULL,
                        NULL, NULL, event->button, event->time);
        return TRUE;
    }

    return FALSE;
}
#endif


/*
 * Activate a link ("link_clicked" callback of the page)
 */
gboolean Html_link_clicked (DwWidget *widget, int link, int x, int y,
                                  DWORD flags, mSpiderHtmlLB *lb)
{
   mSpiderDoc *named_dd;
   gchar *target;

#ifdef JS_SUPPORT
   char *urlstring;
   jsstring code;
   char *jsstr = NULL;
   jsobject *jsobj = NULL;
   jsscript_interpreter *interpreter = NULL;
    
   urlstring=a_Url_str(lb->links[link]);
  if(strstr(urlstring,"javascript:")!=0)
  { 
    jsobj = lb->dd->jsobj;
    jsobj = js_getdftdocobj((int)lb->dd);
    if ( !jsobj || !(interpreter = jsobj->jsinterpreter) )
        return FALSE;

    if ((interpreter->inuse) &&(interpreter->dealing == 0)) 
    {
        interpreter->dealing = 1;
        jsstr = g_strdup(urlstring);
        code.source = jsstr;
        code.length = strlen(jsstr);
        js_eval(interpreter, &code);
        free(jsstr);
        interpreter->dealing = 0;
        return TRUE;
    }
    else
        return FALSE;
  }
#endif               

    if (HIWORD (flags) == MSG_LBUTTONUP) {
       if((target = (gchar *) URL_TARGET_(lb->links[link])))
       {

           /* targeted link or base target, open in either existing
            * named document or new document */
          if ((named_dd = a_Doc_get_by_name(lb->dd, (gchar *) target)))
          {
             a_Nav_push(named_dd, lb->links[link]);
          }
          else 
          {
                a_Interface_set_location_text(lb->dd, lb->links[link]->url_string->str);
                if (lb->dd->is_iframe)
                    a_Nav_push(a_Doc_get_root(lb->dd), lb->links[link]);
                else
                    a_Nav_push(lb->dd, lb->links[link]);
          }
       }
       else
       {
        a_Interface_set_location_text(lb->dd, lb->links[link]->url_string->str);
        if (lb->dd->is_iframe)
           a_Nav_push(a_Doc_get_root(lb->dd), lb->links[link]);
        else
           a_Nav_push(lb->dd, lb->links[link]);
       }
    }
    
    else if (HIWORD (flags) == MSG_RBUTTONUP) {
        a_Interface_set_location_text(lb->dd, lb->links[link]->url_string->str);
    }
    else {
        return FALSE;
    }

    if (DW_IS_PAGE (widget))
        a_Dw_page_change_link_color (DW_PAGE (widget), link, lb->visited_color);
    
    return TRUE;
}

#if 0
/*
 * Popup the image menu ("button_press_event" callback of image)
 */
static gboolean Html_image_menu (DwWidget *widget,
                                Sint32 x, Sint32 y, GdkEventButton *event,
                                BrowserWindow *bw)
{
    DwImage *image = DW_IMAGE (widget);
    if (event->button == 3 && image->url) {
        a_Menu_popup_set_url (bw, image->url);
        gtk_menu_popup(GTK_MENU (bw->menu_popup.over_image), NULL, NULL,
                        NULL, NULL, event->button, event->time);
        return TRUE;
    }

    return FALSE;
}

/*
 * Popup the page menu ("button_press_event" callback of the viewport)
 */
static int Html_page_menu (MGWidget *viewport, GdkEventButton *event,
                          BrowserWindow *bw)
{
    POINT bug_pix;

    if (event->button == 3) {
        /* set the working URL */
        a_Menu_popup_set_url(bw, a_History_get_url(NAV_TOP(bw)));
        /* set "View page Bugs" sensitivity */
        bug_pix = gtk_object_get_data(GTK_OBJECT(bw->status_bug_meter), "bug");
        gtk_widget_set_sensitive(bw->viewbugs_menuitem,
                               GTK_WIDGET_VISIBLE(GTK_WIDGET(bug_pix)));
        gtk_menu_popup(GTK_MENU(bw->menu_popup.over_page), NULL, NULL,
                     NULL, NULL, event->button, event->time);
        return TRUE;
    }
    else
        return FALSE;
}
#endif

/*
 * Connect all signals of a page or an image.
 */
static void Html_connect_signals (mSpiderHtml *html, GObject *widget)
{
    g_signal_connect (widget, "link_entered",
                       G_CALLBACK (Html_handle_status),
                       (gpointer)html->linkblock);
#if 0
    gtk_signal_connect (widget, "link_pressed", GTK_SIGNAL_FUNC(Html_link_menu),
                       (gpointer)html->linkblock);
#endif
    g_signal_connect (widget, "link_clicked",
                       G_CALLBACK (Html_link_clicked),
                       (gpointer)html->linkblock);
}


/*
 * Create a new link in the linkblock, set it as the url's parent
 * and return the index.
 */
static int Html_set_new_link (mSpiderHtml *html, mSpiderUrl **url)
{
    int nl;

    nl = html->linkblock->num_links;
    a_List_add (html->linkblock->links, nl, html->linkblock->num_links_max);
    html->linkblock->links[nl] = (*url) ? *url : NULL;
    return html->linkblock->num_links++;
}


/*
 * Allocate and insert form information into the Html linkblock
 */
static int Html_form_new (mSpiderHtmlLB *html_lb,
                          mSpiderHtmlMethod method,
                          const mSpiderUrl *action,
                          mSpiderHtmlEnc enc)
{
    int nf;

    a_List_add (html_lb->forms, html_lb->num_forms, html_lb->num_forms_max);

    nf = html_lb->num_forms;
    html_lb->forms[nf].method = method;
    html_lb->forms[nf].action = a_Url_dup(action);
    html_lb->forms[nf].enc = enc;
    html_lb->forms[nf].num_inputs = 0;
    html_lb->forms[nf].num_inputs_max = 4;
    html_lb->forms[nf].inputs = NULL;
    html_lb->forms[nf].num_entry_fields = 0;
    html_lb->forms[nf].num_submit_buttons = 0;
    html_lb->num_forms++;
    

    _MSG("Html_form_new: action=%s nform=%d\n", action, nf);
    return nf;
}
/*
 * Change one toplevel attribute. var should be an identifier. val is
 * only evaluated once, so you can safely use a function call for it.
 */
#define HTML_SET_TOP_ATTR(html, var, val) \
   do { \
      DwStyle style_attrs, *old_style; \
       \
      old_style = (html)->stack[(html)->stack_top].style; \
      style_attrs = *old_style; \
      style_attrs.var = (val); \
      (html)->stack[(html)->stack_top].style = \
         a_Dw_style_new (&style_attrs, (html)->dd->bw->main_window); \
      a_Dw_style_unref (old_style); \
   } while (FALSE)

/*
 * Set the font at the top of the stack. BImask specifies which
 * attributes in BI should be changed.
 */
static void Html_set_top_font (mSpiderHtml *html, char *name, int size,
                              int BI, int BImask)
{
    DwStyleFont font_attrs;

    font_attrs = *html->stack[(html)->stack_top].style->font;
    if ( name )
        font_attrs.name = name;
    if ( size )
        font_attrs.size = size;
    if ( BImask & 1 )
        font_attrs.weight   = (BI & 1) ? 700 : 400;
    if ( BImask & 2 )
        font_attrs.style = (BI & 2) ?
            (prefs.use_oblique ?
            DW_STYLE_FONT_STYLE_OBLIQUE : DW_STYLE_FONT_STYLE_ITALIC) :
            DW_STYLE_FONT_STYLE_NORMAL;
    font_attrs.charset = html->logfont?html->logfont->charset:DEF_CHARSET;

    HTML_SET_TOP_ATTR (html, font, a_Dw_style_font_new (&font_attrs));
}

/*
 * Evaluates the ALIGN attribute (left|center|right|justify) and
 * sets the style at the top of the stack.
 */
static void Html_tag_set_align_attr(mSpiderHtml *html, char *tag, int tagsize)
{
    const char *align, *charattr;

    if ((align = Html_get_attr(html, tag, tagsize, "align"))) {
        if (g_strcasecmp (align, "left") == 0)
            HTML_SET_TOP_ATTR (html, text_align, DW_STYLE_TEXT_ALIGN_LEFT);
        else if (g_strcasecmp (align, "right") == 0)
            HTML_SET_TOP_ATTR (html, text_align, DW_STYLE_TEXT_ALIGN_RIGHT);
        else if (g_strcasecmp (align, "center") == 0)
            HTML_SET_TOP_ATTR (html, text_align, DW_STYLE_TEXT_ALIGN_CENTER);
        else if (g_strcasecmp (align, "justify") == 0)
            HTML_SET_TOP_ATTR (html, text_align, DW_STYLE_TEXT_ALIGN_JUSTIFY);
        else if (g_strcasecmp (align, "char") == 0) {
            /* todo: Actually not supported for <p> etc. */
            HTML_SET_TOP_ATTR (html, text_align, DW_STYLE_TEXT_ALIGN_STRING);
            if ((charattr = Html_get_attr(html, tag, tagsize, "char"))) {
                if (charattr[0] == 0)
                   /* todo: ALIGN=" ", and even ALIGN="&32;" will reult in
                    * an empty string (don't know whether the latter is
                    * correct, has to be clarified with the specs), so
                    * that for empty strings, " " is assumed. */
                    HTML_SET_TOP_ATTR (html, text_align_char, ' ');
                else
                    HTML_SET_TOP_ATTR (html, text_align_char, charattr[0]);
            } else
                /* todo: Examine LANG attr of <html>. */
                HTML_SET_TOP_ATTR (html, text_align_char, '.');
        }
    }
}

/*
 * Evaluates the VALIGN attribute (top|bottom|middle|baseline) and
 * sets the style in style_attrs. Returns TRUE when set.
 */
static gboolean Html_tag_set_valign_attr(mSpiderHtml *html, char *tag,
                                         int tagsize, DwStyle *style_attrs)
{
   const char *attr;

    if ((attr = Html_get_attr(html, tag, tagsize, "valign"))) {
        if (g_strcasecmp (attr, "top") == 0)
            style_attrs->valign = DW_STYLE_VALIGN_TOP;
        else if (g_strcasecmp (attr, "bottom") == 0)
            style_attrs->valign = DW_STYLE_VALIGN_BOTTOM;
        else if (g_strcasecmp (attr, "baseline") == 0)
            style_attrs->valign = DW_STYLE_VALIGN_BASELINE;
        else
            style_attrs->valign = DW_STYLE_VALIGN_MIDDLE;
        return TRUE;
    }
    else
        return FALSE;
}


/*
 * Add a new DwPage into the current DwPage, for indentation.
 * left and right are the horizontal indentation amounts, space is the
 * vertical space around the block.
 */
static void Html_add_indented_widget (mSpiderHtml *html, DwWidget *page,
                                     int left, int right, int space)
{
    DwStyle style_attrs, *style;

    style_attrs = *html->stack[html->stack_top].style;

    a_Dw_style_box_set_val(&style_attrs.margin, 0);
    a_Dw_style_box_set_val(&style_attrs.border_width, 0);
    a_Dw_style_box_set_val(&style_attrs.padding, 0);

   /* Activate this for debugging */
#if 0
   a_Dw_style_box_set_val(&style_attrs.border_width, 1);
   a_Dw_style_box_set_border_color
      (&style_attrs,
       a_Dw_style_shaded_color_new(style_attrs.color->color_val));
   a_Dw_style_box_set_border_style(&style_attrs, DW_STYLE_BORDER_DASHED);
#endif

    style_attrs.margin.left = left;
    style_attrs.margin.right = right;
    style = a_Dw_style_new (&style_attrs, html->dd->bw->main_window);

    a_Dw_page_add_parbreak (DW_PAGE (html->dw), space, style);
    a_Dw_page_add_widget (DW_PAGE (html->dw), page, style);
    a_Dw_page_add_parbreak (DW_PAGE (html->dw), space, style);
    html->stack[html->stack_top].page = html->dw = page;
    html->stack[html->stack_top].hand_over_break = TRUE;
    a_Dw_style_unref (style);

    /* Handle it when the user clicks on a link */
    Html_connect_signals(html, G_OBJECT(page));
}

/*
 * Create and add a new indented DwPage to the current DwPage
 */
static void Html_add_indented (mSpiderHtml *html, int left, int right, int space)
{
    DwWidget *page = a_Dw_page_new ();
    Html_add_indented_widget (html, page, left, right, space);
}

/*
 * Given a font_size, this will return the correct 'level'.
 * (or the closest, if the exact level isn't found).
 */
static int Html_fontsize_to_level(int fontsize)
{
    int i, level;
    double normalized_size = fontsize / prefs.font_factor,
                approximation   = FontSizes[FontSizesNum-1] + 1;

    for (i = level = 0; i < FontSizesNum; i++)
        if (approximation >= ABS (normalized_size - FontSizes[i])) {
            approximation = ABS (normalized_size - FontSizes[i]);
            level = i;
        } else {
            break;
    }

    return level;
}

/*
 * Given a level of a font, this will return the correct 'size'.
 */
static int Html_level_to_fontsize (int level)
{
    level = MAX(0, level);
    level = MIN(FontSizesNum - 1, level);

    return (int)(FontSizes[level]*prefs.font_factor);

}

/*
 * Miscelaneous initializations for a DwPage
 */
static void Html_set_dwpage (mSpiderHtml *html)
{
    DwWidget *widget;
    DwPage *page;
    DwStyle style_attrs;
    DwStyleFont font;
#ifdef JS_SUPPORT
    jsobject *jsobj = NULL;
    jsobject *wjsobj = NULL;
    jsscript_interpreter *jsinterpreter = NULL;
#endif

    if (html->dw) return;

    widget = a_Dw_page_new ();
    page = DW_PAGE (widget);
    html->dw = html->stack[0].page = widget;

#ifdef JS_SUPPORT
    jsobj = g_malloc0(sizeof(jsobject));
    if ( jsobj ) {
        jsobj->htmlobj= &(page->container.widget);
        jsobj->jstype = jsdocument;
        jsobj->jsparent = html->dd->jsobj;
        jsobj->children=NULL;
        wjsobj = (jsobject*)html->dd->jsobj;
        if ( wjsobj ) {
            wjsobj->children = g_list_append(wjsobj->children, jsobj);
            page->container.widget.jsobj = jsobj;
            /* init js interpreter */ 
            jsinterpreter= g_malloc0(sizeof(jsscript_interpreter));
            if ( jsinterpreter ) {
                jsinterpreter->jsobj = jsobj;
                if ( !js_get_interpreter(jsinterpreter, wjsobj) ) {
                    /* init js interpreter succeed */
                    fprintf(stderr, "init js engine error\n");
                }
            }
            jsinterpreter->inuse = 2;    /* parsering... */
            jsinterpreter->valid = 1;
            jsinterpreter->point = html;

            jsobj->jsinterpreter = jsinterpreter;

            html->dd->jsinterpreter = jsinterpreter;

            js_setdftdocobj((int)html->dd, jsobj);

        } else {
            g_free(jsobj);
        }
    }

#endif
    if ( !html->dd || !html->dd->bw )
        return;

    a_Dw_style_init_values (&style_attrs, html->dd->bw->main_window);

    /* Create a dummy font, attribute, and tag for the bottom of the stack. */
    font.name = prefs.vw_fontname; /* Helvetica */
    font.size = Html_level_to_fontsize(FontSizesBase);
    font.weight = 400;
    font.style = DW_STYLE_FONT_STYLE_NORMAL;
    font.charset = html->logfont?html->logfont->charset:DEF_CHARSET;

    style_attrs.font = a_Dw_style_font_new (&font);
    style_attrs.color = a_Dw_style_color_new (prefs.text_color, 
                    html->dd->bw->main_window);
    html->stack[0].style = a_Dw_style_new (&style_attrs, 
                    html->dd->bw->main_window);
    html->stack[0].table_cell_style = NULL;

    /* Handle it when the user clicks on a link */
    Html_connect_signals (html, G_OBJECT(widget));

    /* Destroy the linkblock when the DwPage is destroyed */
    g_signal_connect_swapped (G_OBJECT(widget), "destroy",
                             G_CALLBACK (Html_lb_free),
                             (gpointer)html->linkblock);
}

static void Html_find_charset (mSpiderHtml *html, const char* content)
{
    char *my_con;
    char *charset;
    char fontname [256] = {0};

    if (html->logfont)
        return;

    my_con = g_strdup (content);
    charset = my_con;
    while (*charset) {
        *charset = tolower (*charset);
        charset ++;
    }

    charset = strstr (my_con, "charset");

    if (charset == NULL) {
        /* this is the default charset */
       charset = GetSystemFont (SYSLOGFONT_WCHAR_DEF)->charset;
    }
   else{
        int tmp;

        /* find the charset defined in head of this page */
        charset = strchr (charset, '=');
        charset ++;
        while (*charset == ' ' || *charset == '\t')
            charset ++;

        tmp = strcspn (charset, " \t\n");
        charset [tmp] = '\0';
    }

    if(set_charset != NULL)
    {
        strcpy (fontname, "*-*-rrncnn-*-12-");
        strcat (fontname, set_charset);
        html->logfont = CreateLogFontByName (fontname);
        g_free (my_con);
        return;
    }

    strcpy (fontname, "*-*-rrncnn-*-12-");
    strcat (fontname, charset);
    html->logfont = CreateLogFontByName (fontname);

    g_free (my_con);
}

/*
 * Create and initialize a new mSpiderHtml structure
 */
static mSpiderHtml *Html_new(mSpiderDoc *dd, const mSpiderUrl *url, 
                const char* content_type)
{
    mSpiderHtml *html;
    

    html = g_malloc0 (sizeof (mSpiderHtml));
    html->Start_Buf = NULL;
    html->Start_Ofs = 0;
    html->Buf_Size = 0;
    html->CurrTagOfs = 0;
#if 0
    html->CurrTagIdx = 0;
#endif
    html->OldTagOfs = 0;
    html->OldTagLine = 1;

    html->dw = NULL;
    html->dd = dd;
    html->linkblock = Html_lb_new(dd, url);
    dd->html_block = html->linkblock;

    html->stack_max = 32;
    html->stack_top = 0;
    html->stack = g_malloc0 (html->stack_max * sizeof (mSpiderHtmlState));
    html->stack[0].tag_name = g_strdup("none");
    html->stack[0].style = NULL;
    html->stack[0].table_cell_style = NULL;
    html->stack[0].parse_mode = MSPIDER_HTML_PARSE_MODE_INIT;
    html->stack[0].table_mode = MSPIDER_HTML_TABLE_MODE_NONE;
    html->stack[0].frame_mode = MSPIDER_HTML_FRAME_MODE_NONE;
    html->stack[0].frameset = NULL;
    html->stack[0].cell_text_align_set = FALSE;
    html->stack[0].list_type = HTML_LIST_NONE; /* no <ul> or <ol> open */
    html->stack[0].list_number = 0;
    html->stack[0].tag_idx = -1;
    html->stack[0].page = NULL;
    html->stack[0].table = NULL;
    html->stack[0].ref_list_item = NULL;
    html->stack[0].current_bg_color = prefs.bg_color;
    html->stack[0].hand_over_break = FALSE;

    html->Stash = g_string_new("");
    html->StashSpace = FALSE;

    html->SPCBuf = NULL;

    html->pre_column = 0;
    html->PreFirstChar = FALSE;
    html->PrevWasCR = FALSE;
    html->PrevWasOpenTag = FALSE;
    html->SPCPending = FALSE;
    html->InVisitedLink = FALSE;
    html->ReqTagClose = FALSE;
    html->CloseOneTag = FALSE;
    html->TagSoup = TRUE;
    html->NameVal = NULL;
    html->PreWidth = G_MAXINT;
    html->PreCount = 0;

    html->Num_HTML = html->Num_HEAD = html->Num_BODY = html->Num_TITLE = 0;

    html->InFlags = 0;

    html->attr_data = g_string_sized_new(1024);
    html->logfont = NULL;

    Html_find_charset (html, content_type);

    Html_set_dwpage (html);

    return html;
}

/*
 * Initialize the stash buffer
 */
static void Html_stash_init(mSpiderHtml *html)
{
    html->stack[html->stack_top].parse_mode = MSPIDER_HTML_PARSE_MODE_STASH;
    html->StashSpace = FALSE;
    g_string_truncate(html->Stash, 0);
}

/* Entities list from the HTML 4.01 DTD */
typedef struct {
   char *entity;
   int isocode;
} Ent_t;

#define NumEnt 252
static const Ent_t Entities[NumEnt] = {
   {"AElig",0306}, {"Aacute",0301}, {"Acirc",0302},  {"Agrave",0300},
   {"Alpha",01621},{"Aring",0305},  {"Atilde",0303}, {"Auml",0304},
   {"Beta",01622}, {"Ccedil",0307}, {"Chi",01647},   {"Dagger",020041},
   {"Delta",01624},{"ETH",0320},    {"Eacute",0311}, {"Ecirc",0312},
   {"Egrave",0310},{"Epsilon",01625},{"Eta",01627},  {"Euml",0313},
   {"Gamma",01623},{"Iacute",0315}, {"Icirc",0316},  {"Igrave",0314},
   {"Iota",01631}, {"Iuml",0317},   {"Kappa",01632}, {"Lambda",01633},
   {"Mu",01634},   {"Ntilde",0321}, {"Nu",01635},    {"OElig",0522},
   {"Oacute",0323},{"Ocirc",0324},  {"Ograve",0322}, {"Omega",01651},
   {"Omicron",01637},{"Oslash",0330},{"Otilde",0325},{"Ouml",0326},
   {"Phi",01646},  {"Pi",01640},    {"Prime",020063},{"Psi",01650},
   {"Rho",01641},  {"Scaron",0540}, {"Sigma",01643}, {"THORN",0336},
   {"Tau",01644},  {"Theta",01630}, {"Uacute",0332}, {"Ucirc",0333},
   {"Ugrave",0331},{"Upsilon",01645},{"Uuml",0334},  {"Xi",01636},
   {"Yacute",0335},{"Yuml",0570},   {"Zeta",01626},  {"aacute",0341},
   {"acirc",0342}, {"acute",0264},  {"aelig",0346},  {"agrave",0340},
   {"alefsym",020465},{"alpha",01661},{"amp",38},    {"and",021047},
   {"ang",021040}, {"aring",0345},  {"asymp",021110},{"atilde",0343},
   {"auml",0344},  {"bdquo",020036},{"beta",01662},  {"brvbar",0246},
   {"bull",020042},{"cap",021051},  {"ccedil",0347}, {"cedil",0270},
   {"cent",0242},  {"chi",01707},   {"circ",01306},  {"clubs",023143},
   {"cong",021105},{"copy",0251},   {"crarr",020665},{"cup",021052},
   {"curren",0244},{"dArr",020723}, {"dagger",020040},{"darr",020623},
   {"deg",0260},   {"delta",01664}, {"diams",023146},{"divide",0367},
   {"eacute",0351},{"ecirc",0352},  {"egrave",0350}, {"empty",021005},
   {"emsp",020003},{"ensp",020002}, {"epsilon",01665},{"equiv",021141},
   {"eta",01667},  {"eth",0360},    {"euml",0353},   {"euro",020254},
   {"exist",021003},{"fnof",0622},  {"forall",021000},{"frac12",0275},
   {"frac14",0274},{"frac34",0276}, {"frasl",020104},{"gamma",01663},
   {"ge",021145},  {"gt",62},       {"hArr",020724}, {"harr",020624},
   {"hearts",023145},{"hellip",020046},{"iacute",0355},{"icirc",0356},
   {"iexcl",0241}, {"igrave",0354}, {"image",020421},{"infin",021036},
   {"int",021053}, {"iota",01671},  {"iquest",0277}, {"isin",021010},
   {"iuml",0357},  {"kappa",01672}, {"lArr",020720}, {"lambda",01673},
   {"lang",021451},{"laquo",0253},  {"larr",020620}, {"lceil",021410},
   {"ldquo",020034},{"le",021144},  {"lfloor",021412},{"lowast",021027},
   {"loz",022712}, {"lrm",020016},  {"lsaquo",020071},{"lsquo",020030},
   {"lt",60},      {"macr",0257},   {"mdash",020024},{"micro",0265},
   {"middot",0267},{"minus",021022},{"mu",01674},    {"nabla",021007},
   {"nbsp",32},    {"ndash",020023},{"ne",021140},   {"ni",021013},
   {"not",0254},   {"notin",021011},{"nsub",021204}, {"ntilde",0361},
   {"nu",01675},   {"oacute",0363}, {"ocirc",0364},  {"oelig",0523},
   {"ograve",0362},{"oline",020076},{"omega",01711}, {"omicron",01677},
   {"oplus",021225},{"or",021050},  {"ordf",0252},   {"ordm",0272},
   {"oslash",0370},{"otilde",0365}, {"otimes",021227},{"ouml",0366},
   {"para",0266},  {"part",021002}, {"permil",020060},{"perp",021245},
   {"phi",01706},  {"pi",01700},    {"piv",01726},   {"plusmn",0261},
   {"pound",0243}, {"prime",020062},{"prod",021017}, {"prop",021035},
   {"psi",01710},  {"quot",34},     {"rArr",020722}, {"radic",021032},
   {"rang",021452},{"raquo",0273},  {"rarr",020622}, {"rceil",021411},
   {"rdquo",020035},{"real",020434},{"reg",0256},    {"rfloor",021413},
   {"rho",01701},  {"rlm",020017},  {"rsaquo",020072},{"rsquo",020031},
   {"sbquo",020032},{"scaron",0541},{"sdot",021305}, {"sect",0247},
   {"shy",0255},   {"sigma",01703}, {"sigmaf",01702},{"sim",021074},
   {"spades",023140},{"sub",021202},{"sube",021206}, {"sum",021021},
   {"sup",021203}, {"sup1",0271},   {"sup2",0262},   {"sup3",0263},
   {"supe",021207},{"szlig",0337},  {"tau",01704},   {"there4",021064},
   {"theta",01670},{"thetasym",01721},{"thinsp",020011},{"thorn",0376},
   {"tilde",01334},{"times",0327},  {"trade",020442},{"uArr",020721},
   {"uacute",0372},{"uarr",020621}, {"ucirc",0373},  {"ugrave",0371},
   {"uml",0250},   {"upsih",01722}, {"upsilon",01705},{"uuml",0374},
   {"weierp",020430},{"xi",01676},  {"yacute",0375}, {"yen",0245},
   {"yuml",0377},  {"zeta",01666},  {"zwj",020015},  {"zwnj",020014}
};


/*
 * Comparison function for binary search
 */
static int Html_entity_comp (const void *a, const void *b)
{
    return strcmp(((Ent_t *)a)->entity, ((Ent_t *)b)->entity);
}

/*
 * Binary search of 'key' in entity list
 */
static int Html_entity_search (char *key)
{
    Ent_t *res, EntKey;

    EntKey.entity = key;
    res = bsearch(&EntKey, Entities, NumEnt, sizeof(Ent_t), Html_entity_comp);
    if ( res )
        return (res - Entities);
    return -1;
}

/*
 * Switch a few UCS encodings to latin1.
 */
static int Html_try_ucs2latin1(int isocode)
{
    int ret;
    switch (isocode) {
        case 0x2018:
        case 0x2019: ret = '\''; break;
        case 0x201c:
        case 0x201d: ret = '"'; break;
        case 0x2013:
        case 0x2014: ret = '-'; break;
        case 0x2039: ret = '<'; break;
        case 0x203a: ret = '>'; break;
        case 0x2022: ret = 176; break;
        default:     ret = -1;  break;
    }
    return ret;
}

/*
 * Switch a few 'undefined for HTML' ASCII encodings to latin1.
 */
static gint Html_try_ascii2latin1(gint isocode)
{
   gint ret;
   switch (isocode) {
      case 145:
      case 146: ret = '\''; break;
      case 147:
      case 148: ret = '"'; break;
      case 149: ret = 176; break;
      case 150:
      case 151: ret = '-'; break;
      default:  ret = isocode; break;
   }
   return ret;
}

/*
 * Given an entity, return the ISO-Latin1 character code.
 * (-1 if not a valid entity)
 */
static int Html_parse_entity(mSpiderHtml *html, const char *token,
                               int toksize, int *entsize)
{
    int isocode, i;
    char *tok, *s, c;

    token ++;
    tok = s = toksize ? g_strndup(token, (guint)toksize) : g_strdup(token);

    isocode = -1;

   if (*s == '#') {
      /* numeric character reference */
      errno = 0;
      if (*++s == 'x' || *s == 'X') {
         if (isxdigit(*++s)) {
            /* strtol with base 16 accepts leading "0x" - we don't */
            if (*s == '0' && s[1] == 'x') {
               s++;
               isocode = 0; 
            } else {
               isocode = strtol(s, &s, 16);
            }
         }
      } else if (isdigit(*s)) {
         isocode = strtol(s, &s, 10);
      }

      if (!isocode || errno || isocode > 0x7fffffffL) {
         /* this catches null bytes, errors and codes >=2^31 */
         MSG_HTML("numeric character reference out of range\n");
         isocode = -2;
      }

      if (isocode != -1) {
         if (*s == ';')
            s++;
         else if (prefs.show_extra_warnings)
            MSG_HTML("numeric character reference without trailing ';'\n");
      }

   } else if (isalpha(*s)) {
      /* character entity reference */
      while (isalnum(*++s) || strchr(":_.-", *s));
      c = *s;
      *s = 0;

      if ((i = Html_entity_search(tok)) == -1) {
         MSG_HTML("undefined character entity '%s'\n", tok);
         isocode = -3;
      } else
         isocode = Entities[i].isocode;

      if (c == ';')
         s++;
      else if (prefs.show_extra_warnings)
         MSG_HTML("character entity reference without trailing ';'\n");
   }

   *entsize = s-tok+1;
   g_free(tok);

   if (isocode >= 128 && isocode <= 159) {
      MSG_HTML("code positions 128-159 are not defined for ISO Latin-1\n");
      isocode = Html_try_ascii2latin1(isocode);
   } else if (isocode  > 255)
      /* Try a few UCS translations to Latin1 */
      isocode = Html_try_ucs2latin1(isocode);
   else if (isocode == -1 && prefs.show_extra_warnings)
      MSG_HTML("literal '&'\n");

   return isocode;
}

/*
 * Convert all the entities in a token to plain ISO character codes. Takes
 * a token and its length, and returns a newly allocated string.
 */
static char *Html_parse_entities (mSpiderHtml *html, char *token, int toksize)
{
   char *esc_set = "&\xE2\xC2";
   char *new_str;
   int i, j, isocode, entsize;

   new_str = g_strndup(token, toksize);
   if (new_str[strcspn(new_str, esc_set)] == 0)
      return new_str;

   for (i = j = 0; i < toksize; i++) {
      if (token[i] == '&' &&
          (isocode = Html_parse_entity(html, token+i,
                                       toksize-i, &entsize)) >= 0) {
         new_str[j++] = (gchar) isocode;
         i += entsize-1;

      } else if (token[i] == '\xE2' && token[i+1] == '\x80' && i+2 < toksize){
         /* Hack: for parsing some UTF-8 characters into latin1 */
         switch (token[i+2]) {
         case '\x94':
            new_str[j++] = '-';
            new_str[j++] = '-';
            break;
         case '\x98':
         case '\x99':
            new_str[j++] = '\'';
            break;
         case '\x9C':
         case '\x9D':
            new_str[j++] = '"';
            break;
         case '\xA2':
            new_str[j++] = '*';
            new_str[j++] = ' ';
            break;
         default: /* unhandled */
            new_str[j++] = '\xE2';
            break;
         }
         i += 2;

      } else if (token[i] == '\xC2' && token[i+1] == '\xA0') {
         /* Hack: for parsing some UTF-8 characters into latin1 */
         new_str[j++] = ' ';
         ++i;

      } else {
         new_str[j++] = token[i];
      }
   }
   new_str[j] = '\0';
   return new_str;
}

/*
 * Parse spaces
 *
 */
static void Html_process_space(mSpiderHtml *html, char *space, int spacesize)
{
    int i, offset;
    mSpiderHtmlParseMode parse_mode = html->stack[html->stack_top].parse_mode;

    if ( parse_mode == MSPIDER_HTML_PARSE_MODE_STASH ) {
        html->StashSpace = (html->Stash->len > 0);
        html->SPCPending = FALSE;
    }
    else if ( parse_mode == MSPIDER_HTML_PARSE_MODE_VERBATIM ) {
        char *Pword = g_strndup (space, spacesize);
        g_string_append (html->Stash, Pword);
        g_free(Pword);
        html->SPCPending = FALSE;
    }
    else if ( parse_mode == MSPIDER_HTML_PARSE_MODE_PRE ) {
        /* re-scan the string for characters that cause line breaks */
        for (i = 0; i < spacesize; i++) {
            /* Support for "\r", "\n" and "\r\n" line breaks (skips the first) */
            if (!html->PreFirstChar &&
                    (space[i] == '\r' || (space[i] == '\n' && !html->PrevWasCR))) {
                a_Dw_page_add_linebreak(DW_PAGE (html->dw),
                                        html->stack[(html)->stack_top].style);
                html->pre_column = 0;
            }
            html->PreFirstChar = FALSE;

            /* cr and lf should not be rendered -- they appear as a break */
            switch (space[i]) {
            case '\r':
            case '\n':
                break;
            case '\t':
                if (prefs.show_extra_warnings)
                    MSG_HTML("TAB character inside <PRE>\n");
                offset = TAB_SIZE - html->pre_column % TAB_SIZE;
                a_Dw_page_add_text(DW_PAGE (html->dw),
                                   g_strnfill(offset, ' '),
                                   html->stack[html->stack_top].style);
                html->pre_column += offset;
                break;
            default:
                a_Dw_page_add_text(DW_PAGE (html->dw),
                                g_strndup(space + i, 1),
                               html->stack[html->stack_top].style);
                html->pre_column++;
                break;
            }

            html->PrevWasCR = (space[i] == '\r');
        }
        html->SPCPending = FALSE;
    }
    else {
      if (SGML_SPCDEL && html->PrevWasOpenTag) {
         /* SGML_SPCDEL ignores white space inmediately after an open tag */
         html->SPCPending = FALSE;
      } else {
         g_free(html->SPCBuf);
         html->SPCBuf = g_strndup(space, spacesize);
         html->SPCPending = TRUE;
      }

        if ( parse_mode == MSPIDER_HTML_PARSE_MODE_STASH_AND_BODY )
            html->StashSpace = (html->Stash->len > 0);
    }
}

/*
 * Handles putting the word into its proper place
 *  > STASH and VERBATIM --> html->Stash
 *  > otherwise it goes through a_Dw_page_add_text()
 *
 * Entities are parsed (or not) according to parse_mode.
 */
static void Html_process_word(mSpiderHtml *html, char *word, int size)
{
    int i, start;
    char *Pword;
    mSpiderHtmlParseMode parse_mode = html->stack[html->stack_top].parse_mode;

    if (parse_mode == MSPIDER_HTML_PARSE_MODE_STASH ||
            parse_mode == MSPIDER_HTML_PARSE_MODE_STASH_AND_BODY) {
        if ( html->StashSpace ) {
            g_string_append_c(html->Stash, ' ');
            html->StashSpace = FALSE;
        }
        Pword = Html_parse_entities(html, word, size);
        g_string_append(html->Stash, Pword);
        g_free(Pword);

    }
    else if (parse_mode == MSPIDER_HTML_PARSE_MODE_VERBATIM) {
        /* word goes in untouched, it is not processed here. */
        Pword = g_strndup(word, size);
        g_string_append(html->Stash, Pword);
        g_free(Pword);
    }

    if (parse_mode == MSPIDER_HTML_PARSE_MODE_STASH  ||
            parse_mode == MSPIDER_HTML_PARSE_MODE_VERBATIM) {
    }
    else if ( parse_mode == MSPIDER_HTML_PARSE_MODE_PRE ) {
        /* all this overhead is to catch white-space entities */
        
        Pword = Html_parse_entities(html, word, size);
        for (start = i = 0; Pword[i]; start = i) {
            if (isspace(Pword[i])) {
                while (Pword[++i] && isspace(Pword[i]));
                Html_process_space(html, Pword + start, i - start);
            }
            else {
                while (Pword[++i] && !isspace(Pword[i]));

                    if ((html->PreCount < html->PreWidth) &&
                             (html->PreCount + (i-start) < html->PreWidth)){
                        html->PreCount += (i-start);
                    } else {
                        /*FIXME: if the word is end of '\n', it will be linebreak twice!*/
                        a_Dw_page_add_linebreak (DW_PAGE (html->dw), 
                                    html->stack[(html)->stack_top].style);
                        html->PreCount = (i-start);
                    }

                    
                a_Dw_page_add_text(DW_PAGE (html->dw),
                                g_strndup(Pword + start, i - start),
                                html->stack[html->stack_top].style);
                html->pre_column += i - start;
                html->PreFirstChar = FALSE;
            }
        }
        g_free(Pword);
    }
    else {
      if (html->SPCPending && (!SGML_SPCDEL || !html->PrevWasOpenTag))
         /* SGML_SPCDEL ignores space after an open tag */
         a_Dw_page_add_space(DW_PAGE (html->dw),
                             html->stack[html->stack_top].style);

      /* actually white-space entities inside the word could be
       * collapsed (except &nbsp;), but that's too much overhead
       * for a very rare case of ill-formed HTML  --Jcid */

      Pword = Html_parse_entities(html, word, size);
      g_strdelimit(Pword, "\t\f\n\r", ' ');
      a_Dw_page_add_text(DW_PAGE (html->dw),
                         Pword,
                         html->stack[html->stack_top].style);
    }

   html->PrevWasOpenTag = FALSE;
   html->SPCPending = FALSE;
}

/*
 * Does the tag in tagstr (e.g. "p") match the tag in the tag, tagsize
 * structure, with the initial < skipped over (e.g. "P align=center>")
 */
static gboolean Html_match_tag(const char *tagstr, char *tag, int tagsize)
{
    int i;

    for (i = 0; i < tagsize && tagstr[i] != '\0'; i++) {
        if (tolower(tagstr[i]) != tolower(tag[i]))
            return FALSE;
    }
    /* The test for '/' is for xml compatibility: "empty/>" will be matched. */
    if (i < tagsize && (isspace(tag[i]) || tag[i] == '>' || tag[i] == '/'))
        return TRUE;
    return FALSE;
}

/*
 * This function is called by Html_cleanup_tag and Html_pop_tag, to
 * handle nested DwPage widgets.
 */
static void Html_eventually_pop_dw(mSpiderHtml *html)
{
    /* This function is called after popping from the stack, so the
     * relevant hand_over_break is at html->stack_top + 1. */
    if (html->dw != html->stack[html->stack_top].page) {
        if (html->stack[html->stack_top + 1].hand_over_break)
            a_Dw_page_hand_over_break(DW_PAGE(html->dw),
                                    html->stack[(html)->stack_top].style);
        a_Dw_page_flush(DW_PAGE(html->dw));
        html->dw = html->stack[html->stack_top].page;
    }
}

/*
 * Push the tag (copying attributes from the top of the stack)
 */
static void Html_push_tag(mSpiderHtml *html, int tag_idx)
{
    char *tagstr;
    int n_items;

    /* Save the element's name (no parameters) into tagstr. */
    tagstr = g_strdup(Tags[tag_idx].name);

    n_items = html->stack_top + 1;
    a_List_add(html->stack, n_items, html->stack_max);
    /* We'll copy the former stack item and just change the tag and its index
     * instead of copying all fields except for tag.  --Jcid */
    html->stack[n_items] = html->stack[n_items - 1];
    html->stack[n_items].tag_name = tagstr;
    html->stack[n_items].tag_idx = tag_idx;
    html->stack_top = n_items;
    /* proper memory management, may be unref'd later */
    a_Dw_style_ref (html->stack[html->stack_top].style);
    if (html->stack[html->stack_top].table_cell_style)
        a_Dw_style_ref (html->stack[html->stack_top].table_cell_style);
    html->dw = html->stack[html->stack_top].page;
}

/*
 * Push the tag (used to force en element with optional open into the stack)
 * Note: now it's the same as Html_push_tag(), but things may change...
 */
static void Html_force_push_tag(mSpiderHtml *html, gint tag_idx)
{
   Html_push_tag(html, tag_idx);
}
/*
 * Pop the top tag in the stack
 */
static void Html_real_pop_tag(mSpiderHtml *html)
{
   a_Dw_style_unref (html->stack[html->stack_top].style);
   if (html->stack[html->stack_top].table_cell_style)
      a_Dw_style_unref (html->stack[html->stack_top].table_cell_style);
   g_free(html->stack[html->stack_top--].tag_name);
   Html_eventually_pop_dw(html);
}

/*
 * Default close function for tags.
 * (conditional cleanup of the stack)
 * There're several ways of doing it. Considering the HTML 4.01 spec
 * which defines optional close tags, and the will to deliver useful diagnose
 * messages for bad-formed HTML, it'll go as follows:
 *   1.- Search the stack for the first tag that requires a close tag.
 *   2.- If it matches, clean all the optional-close tags in between.
 *   3.- Cleanup the matching tag. (on error, give a warning message)
 *
 * If 'w3c_mode' is NOT enabled:
 *   1.- Search the stack for a matching tag based on tag level.
 *   2.- If it exists, clean all the tags in between.
 *   3.- Cleanup the matching tag. (on error, give a warning message)
 */
static void Html_tag_cleanup_at_close(mSpiderHtml *html, gint TagIdx)
{
   gint w3c_mode = !prefs.w3c_plus_heuristics;
   gint stack_idx, cmp = 1;
   gint new_idx = TagIdx;

   if (html->CloseOneTag) {
      Html_real_pop_tag(html);
      html->CloseOneTag = FALSE;
      return;
   }

   /* Look for the candidate tag to close */
   stack_idx = html->stack_top;
   while (stack_idx &&
          (cmp = (new_idx != html->stack[stack_idx].tag_idx)) &&
          ((w3c_mode &&
            Tags[html->stack[stack_idx].tag_idx].EndTag == 'O') ||
           (!w3c_mode &&
            Tags[html->stack[stack_idx].tag_idx].TagLevel <
            Tags[new_idx].TagLevel))) {
      --stack_idx;
   }

   /* clean, up to the matching tag */
   if (cmp == 0 && stack_idx > 0) {
      /* There's a valid matching tag in the stack */
      while (html->stack_top >= stack_idx) {
         gint toptag_idx = html->stack[html->stack_top].tag_idx;
         /* Warn when we decide to close an open tag (for !w3c_mode) */
         if (html->stack_top > stack_idx &&
             Tags[toptag_idx].EndTag != 'O')
            MSG_HTML("  - forcing close of open tag: <%s>\n",
                     Tags[toptag_idx].name);

         /* Close this and only this tag */
         html->CloseOneTag = TRUE;
         Tags[toptag_idx].close (html, toptag_idx);
      }

   } else {
      MSG_HTML("unexpected closing tag: </%s>. -- expected </%s>\n",
               Tags[new_idx].name, html->stack[stack_idx].tag_name);
   }
}

#if 0
/*
 * Remove the stack's topmost tag (only if it matches)
 * If it matches, TRUE is returned.
 */
static gboolean Html_cleanup_tag(mSpiderHtml *html, char *tag)
{
    if (html->stack_top &&
            Html_match_tag(html->stack[html->stack_top].tag, tag, strlen(tag))) {
        a_Dw_style_unref (html->stack[html->stack_top].style);
        if (html->stack[html->stack_top].table_cell_style)
            a_Dw_style_unref (html->stack[html->stack_top].table_cell_style);
        g_free(html->stack[html->stack_top--].tag);
        Html_eventually_pop_dw(html);
        return TRUE;
    }
    else
        return FALSE;
}
#endif

/*
 * Cleanup (conditional), and Pop the tag (if it matches)
 */
static void Html_pop_tag(mSpiderHtml *html, gint TagIdx)
{
   Html_tag_cleanup_at_close(html, TagIdx);
}

#if 0
/*
 * Do a paragraph break and push the tag. This pops unclosed <p> tags
 * off the stack, if there are any.
 */
static void Html_par_push_tag (mSpiderHtml *html, char *tag, int tagsize)
{
    Html_push_tag (html, tag, tagsize);
    a_Dw_page_add_parbreak (DW_PAGE (html->dw), 9,
                          html->stack[(html)->stack_top].style);
}
#endif

/*
 * Some parsing routines.
 */

/*
 * Used by Html_parse_length and Html_parse_multi_length_list.
 */
static DwStyleLength Html_parse_length_or_multi_length (const char *attr,
                                                        char **endptr)
{
    DwStyleLength l;
    double v;
    char *end;

    v = (double)strtol (attr, &end, 10);
    switch (*end) {
    case '%':
        end++;
        l = DW_STYLE_CREATE_PER_LENGTH (v / 100);
        break;

    case '*':
        end++;
        l = DW_STYLE_CREATE_REL_LENGTH (v);
        break;

    default:
        l = DW_STYLE_CREATE_ABS_LENGTH ((int)v);
        break;
    }

    if (endptr)
        *endptr = end;
    return l;
}


/*
 * Returns a length or a percentage, or DW_STYLE_UNDEF_LENGTH in case
 * of an error, or if attr is NULL.
 */
DwStyleLength Html_parse_length (mSpiderHtml *html, const char *attr)
{
    DwStyleLength l;
    char *end;

    l = Html_parse_length_or_multi_length (attr, &end);
    if (DW_STYLE_IS_REL_LENGTH (l))
        /* not allowed as &Length; */
        return DW_STYLE_LENGTH_AUTO;
    else {
        /* allow only whitespaces */
        if (*end && !isspace (*end)) {
            MSG_HTML("Garbage after length: %s\n", attr);
            return DW_STYLE_LENGTH_AUTO;
        }
    }

    return l;
}

/*
 * Returns a vector of lenghts/percentages. The caller has to g_free the
 * result when it is not longer used.
 */
/*
static DwStyleLength *Html_parse_multi_length_list (const char *attr)
{
   DwStyleLength *l;
   int n, max_n;
   char *end;

   n = 0;
   max_n = 8;
   l = g_malloc0 (max_n, sizeof (DwStyleLength));

   while (TRUE) {
      l[n] = Html_parse_length_or_multi_length (attr, &end);
      n++;
      a_List_add (l, n, max_n);

      while (isspace (*end))
         end++;
      if (*end == ',')
         attr = end + 1;
      else
         // error or end
         break;
   }

   l[n] = DW_STYLE_LENGTH_AUTO;
   return l;
}
*/

/*
 * Parse a color attribute.
 * Return value: parsed color, or default_color (+ error msg) on error.
 */
static gint32 Html_color_parse(mSpiderHtml *html, const char *subtag, gint32 default_color)
{
    int err = 1;
    gint32 color = a_Color_parse (subtag, default_color, &err);

    if (err) {
        MSG_HTML("color is not in \"#RRGGBB\" format\n");
    }
    return color;
}

/*
 * Check that 'val' is composed of characters inside [A-Za-z0-9:_.-]
 * Note: ID can't have entities, but this check is enough (no '&').
 * Return value: 1 if OK, 0 otherwise.
 */
static gint
 Html_check_name_val(mSpiderHtml *html, const char *val, const char *attrname)
{
   gint i;

   for (i = 0; val[i]; ++i)
      if (!(isalnum(val[i]) || strchr(":_.-", val[i])))
         break;

   if (val[i] || !isalpha(val[0]))
      MSG_HTML("'%s' value is not of the form "
               "[A-Za-z][A-Za-z0-9:_.-]*\n", attrname);

   return !(val[i]);
}

/*
 * Handle open HTML element
 */
static void Html_tag_open_html(mSpiderHtml *html, char *tag, gint tagsize)
{
   if (!(html->InFlags & IN_HTML))
      html->InFlags |= IN_HTML;
   ++html->Num_HTML;

   if (html->Num_HTML > 1) {
      MSG_HTML("HTML element was already open\n");
   }
}

/*
 * Handle close HTML element
 */
static void Html_tag_close_html(mSpiderHtml *html, gint TagIdx)
{
   /* todo: may add some checks here */
    DwMgWidget* mgwidget = NULL;
    if (html->linkblock != NULL && html->linkblock->forms != NULL
            &&html->linkblock->forms[0].inputs != NULL)
    {
        mgwidget = (DwMgWidget*)(html->linkblock->forms[0].inputs[0].widget);
        if (mgwidget && mgwidget->window != -1)
        {
           ; //SendMessage(mgwidget->window, MSG_SETFOCUS, 0, 0);
            //SendMessage(mgwidget->window, MSG_MOUSEACTIVE, 0, 0);
        }
    }

   html->InFlags &= ~IN_HTML;
   Html_pop_tag(html, TagIdx);
   stop_flag = 0;
}

/*
 * Handle open HEAD element
 */
static void Html_tag_open_head(mSpiderHtml *html, char *tag, int tagsize)
{
   if (html->InFlags & IN_BODY) {
      MSG_HTML("HEAD element must go before the BODY section\n");
      html->ReqTagClose = TRUE;
      return;
   }

   if (!(html->InFlags & IN_HEAD))
      html->InFlags |= IN_HEAD;
   ++html->Num_HEAD;

   if (html->Num_HEAD > 1) {
      MSG_HTML("HEAD element was already open\n");
   }
}

/*
 * Handle close HEAD element
 */
static void Html_tag_close_head(mSpiderHtml *html, int TagIdx)
{
   if (html->InFlags & IN_HEAD) {
      if (html->Num_TITLE == 0)
         MSG_HTML("HEAD section lacks the TITLE element\n");
   
      html->InFlags &= ~IN_HEAD;
   }
   Html_pop_tag(html, TagIdx);
}

/*
 * Handle open TITLE
 * calls stash init, where the title string will be stored
 */
static void Html_tag_open_title(mSpiderHtml *html, char *tag, int tagsize)
{
    ++html->Num_TITLE;
    Html_stash_init(html);
}

#define EMSPIDER_ISMENUPAGE "Is emSpider menu page"
/*
 * Handle close TITLE
 * set page-title in the browser window and in the history.
 */
static void Html_tag_close_title(mSpiderHtml *html, int TagIdx)
{
    if (html->InFlags & IN_HEAD) {
        /* title is only valid inside HEAD */
        if (!strcmp(html->Stash->str, EMSPIDER_ISMENUPAGE))
            DW_PAGE(html->dw)->is_menupage = TRUE;
        else
            DW_PAGE(html->dw)->is_menupage = FALSE;
#ifdef JS_SUPPORT
        if (html->linkblock->dd->pagetitle) {
            g_free (html->linkblock->dd->pagetitle);
        }
        html->linkblock->dd->pagetitle = g_strdup(html->Stash->str);
#endif 
        a_Doc_title_set(html->linkblock->dd, html->Stash->str);
        if ( NAV_TOP_E(html->linkblock->dd) )
            a_History_set_title(NAV_TOP(html->linkblock->dd), html->Stash->str);
    }
    else {
        MSG_HTML("the TITLE element must be inside the HEAD section\n");
    }
    Html_pop_tag(html, TagIdx);
}

/*
 * Handle open SCRIPT
 * initializes stash, where the embedded code will be stored.
 * MODE_VERBATIM is used because MODE_STASH catches entities.
 */
static void Html_tag_open_script(mSpiderHtml *html, char *tag, int tagsize)
{
#ifdef JS_SUPPORT
    char *attr = NULL;
    jsobject *jsobj = NULL;
    jsscript_interpreter *interpreter = NULL;
#endif

    Html_stash_init(html);
    html->stack[html->stack_top].parse_mode = MSPIDER_HTML_PARSE_MODE_VERBATIM;

#ifdef JS_SUPPORT
    jsobj = js_getdftdocobj((int)html->dd);
    if ( !jsobj || !(interpreter = jsobj->jsinterpreter) ) {
        return;
    }

    /* download *.js file */
    attr = (char*)Html_get_attr(html, tag, tagsize, "src");
    if ( attr ) {
        interpreter->dealing=0;
        attr = correcturl(html->dd, attr);
        GetJSFile(attr, html->dd);
    }
#endif
}

/*
 * Handle close SCRIPT
 */
static void Html_tag_close_script(mSpiderHtml *html, int TagIdx)
{
   static int i = 0;
#ifdef JS_SUPPORT
    jsstring code;
    char *jsstr = NULL;
    jsobject *jsobj = NULL;
    jsscript_interpreter *interpreter = NULL;

#endif
    /* eventually the stash will be sent to an interpreter for parsing */
    Html_pop_tag(html, TagIdx);
    if ((i = ~i))
        return;

#ifdef JS_SUPPORT
    jsobj = js_getdftdocobj((int)html->dd);
    if ( !jsobj || !(interpreter = jsobj->jsinterpreter) ) {
        return;
    }

        if ( interpreter->inuse ) {
            jsstr = g_strdup(html->Stash->str);
            code.source = jsstr;
            code.length = strlen(jsstr);
            js_eval(interpreter, &code);
            free(jsstr);
        }
#endif
}

/*
 * Handle open STYLE
 * store the contents to the stash where (in the future) the style
 * sheet interpreter can get it.
 */
static void Html_tag_open_style(mSpiderHtml *html, char *tag, int tagsize)
{
    Html_stash_init(html);
    html->stack[html->stack_top].parse_mode = MSPIDER_HTML_PARSE_MODE_VERBATIM;
}

/*
 * Handle close STYLE
 */
static void Html_tag_close_style(mSpiderHtml *html, int TagIdx)
{
    /* eventually the stash will be sent to an interpreter for parsing */
    Html_pop_tag(html, TagIdx);
}

/*
 * <BODY>
 */
static void Html_tag_open_body(mSpiderHtml *html, char *tag, int tagsize)
{
    const char *attrbuf;
    DwPage *page;
    DwStyle style_attrs, *style;
    gint32 color = 0xffffff;
    mSpiderUrl* new_url;
    mSpiderImage * Image;
#ifdef ENABLE_LINKTRAVE
    DwWidget * widget;
    LinktraveManage *linktrave_manage;
#endif    
#ifdef JS_SUPPORT
    jsobject *jsobj = NULL;
#endif
   /* if document contains frameset, ignore body */
   if (html->InFlags & IN_FRAMESET) return;
   /* in body, so hide/remove frameset and show docwin */
   if (html->dd->frameset && DW_IS_WIDGET(html->dd->frameset)) {
      g_signal_emit_by_name (DW_WIDGET(html->dd->frameset),"destroy",0);
#ifdef ENABLE_LINKTRAVE
      linktrave_manage =(LinktraveManage*)html->dd->bw->linktrave_manage;
      widget = linktrave_manage->linktrave_state->widget;
     
      g_free (linktrave_manage->linktrave_state->ddary);
       linktrave_manage->stack_top--; 
       a_Linktrave_state_destroy (linktrave_manage->linktrave_state);
       linktrave_manage->linktrave_state = 
            linktrave_manage->stack[linktrave_manage->stack_top];
        
       a_Linktrave_state_set_widget (linktrave_manage->linktrave_state, widget);
       linktrave_manage->link_number_focus = -2; 
#endif
     html->dd->frameset = NULL;
     ShowWindow(html->dd->docwin , SW_SHOW);
   }

   if (!(html->InFlags & IN_BODY))
      html->InFlags |= IN_BODY;
   ++html->Num_BODY;

   if (html->Num_BODY > 1) {
      MSG_HTML("BODY element was already open\n");
      return;
   }
   if (html->InFlags & IN_HEAD) {
      /* if we're here, it's bad XHTML, no need to recover */
      MSG_HTML("unclosed HEAD element\n");
   }

    page = DW_PAGE (html->dw);

    /* HEAD to BODY section transition was done by Html_test_body_section() */
    if (!prefs.force_my_colors) {
        if ((attrbuf = Html_get_attr(html, tag, tagsize, "bgcolor"))) {
            color = Html_color_parse(html, attrbuf, prefs.bg_color);
            if ( (color == 0xffffff && !prefs.allow_white_bg) ||
                    prefs.force_my_colors )
                color = prefs.bg_color;
        }
        else
            color = prefs.bg_color;

        style_attrs = *(DwStyle*)html->dw->style;
        style_attrs.background_color =
                a_Dw_style_color_new (color, html->dd->bw->main_window);
        style = a_Dw_style_new (&style_attrs, html->dd->bw->main_window);
        a_Dw_widget_set_style (html->dw, style);
        a_Dw_style_unref (style);
        html->stack[html->stack_top].current_bg_color = color;

        if ((attrbuf = Html_get_attr(html, tag, tagsize, "background"))) {
            new_url = a_Url_new(attrbuf, URL_STR_(html->linkblock->base_url), 0, 0, 0);
            if ((Image = a_Image_new_for_background (0, 0)))
            {
                style_attrs = *html->dw->style;
                style_attrs.background_image = a_Dw_style_bgimage_new ((DwImage*)Image->dw);
                style = a_Dw_style_new (&style_attrs, html->dd->bw->main_window);
                a_Dw_widget_set_style (html->dw, style);
                a_Dw_style_unref (style);

                Html_load_image (html, new_url, Image);
            }
            a_Url_free(new_url);
        }

        if ((attrbuf = Html_get_attr(html, tag, tagsize, "text"))) {
            color = Html_color_parse(html, attrbuf, prefs.text_color);
            HTML_SET_TOP_ATTR (html, color,
                a_Dw_style_color_new (color, html->dd->bw->main_window));
        }

        if ((attrbuf = Html_get_attr(html, tag, tagsize, "link")))
            html->linkblock->link_color = Html_color_parse(html, attrbuf,
                                                            prefs.link_color);

        if ((attrbuf = Html_get_attr(html, tag, tagsize, "vlink")))
            html->linkblock->visited_color =
                Html_color_parse(html, attrbuf, prefs.visited_color);

        if (prefs.contrast_visited_color &&
            html->linkblock->link_color == html->linkblock->visited_color) {
            /* get a color that has a "safe distance" from text, link and bg */
            html->linkblock->visited_color =
                a_Color_vc(html->stack[html->stack_top].style->color->color_val,
                        html->linkblock->link_color,
                        html->stack[html->stack_top].current_bg_color);
        }
    }

    html->stack[html->stack_top].parse_mode = MSPIDER_HTML_PARSE_MODE_BODY;

#ifdef JS_SUPPORT
    attrbuf = Html_get_attr(html, tag, tagsize, "onLoad");
    if ( attrbuf && (strlen(attrbuf) != 0) ) {
        jsobj = js_getdftdocobj((int)html->dd);
        if ( jsobj ) {
            jsobj->onload = g_strdup(attrbuf);
            jsobj->is_onload = 1;
        }
    }

    attrbuf = Html_get_attr(html, tag, tagsize, "onUnload");
    if ( attrbuf && (strlen(attrbuf) != 0) ) {
        jsobj = js_getdftdocobj((int)html->dd);
        if ( jsobj ) {
            jsobj->onunload = g_strdup(attrbuf);
        }
    }
#endif
}

/*
 * BODY
 */
static void Html_tag_close_body(mSpiderHtml *html, gint TagIdx)
{
   html->InFlags &= ~IN_BODY;
   Html_pop_tag(html, TagIdx);
}

/*
 * <P>
 * todo: what's the point between adding the parbreak before and
 *       after the push?
 * todo: <P> should be closed upon openning a 'block' element.
 */
static void Html_tag_open_p(mSpiderHtml *html, char *tag, int tagsize)
{
    a_Dw_page_add_parbreak(DW_PAGE (html->dw), 9,
                          html->stack[(html)->stack_top].style);
    Html_tag_set_align_attr (html, tag, tagsize);
}

/*
 * <TABLE>
 */
static void Html_tag_open_table(mSpiderHtml *html, char *tag, int tagsize)
{
#ifdef USE_TABLES
    DwWidget *table;
    mSpiderUrl* new_url;
    mSpiderImage * Image;
    DwStyle style_attrs, *tstyle, *old_style;
    const char *attrbuf;
    int border = 0, cellspacing = 1, cellpadding = 2, bgcolor;
#endif

    a_Dw_page_add_parbreak(DW_PAGE (html->dw), 0,
                          html->stack[(html)->stack_top].style);

#ifdef USE_TABLES
    if ((attrbuf = Html_get_attr(html, tag, tagsize, "border")))
        border = isdigit(attrbuf[0]) ? strtol (attrbuf, NULL, 10) : 1;
    if ((attrbuf = Html_get_attr(html, tag, tagsize, "cellspacing")))
        cellspacing = strtol (attrbuf, NULL, 10);
    if ((attrbuf = Html_get_attr(html, tag, tagsize, "cellpadding")))
        cellpadding = strtol (attrbuf, NULL, 10);

    /* The style for the table */
    style_attrs = *html->stack[html->stack_top].style;

    /* When mspider was started with the --debug-rendering option, there
     * is always a border around the table. */
    if (mspider_dbg_rendering)
        a_Dw_style_box_set_val (&style_attrs.border_width, MIN (border, 1));
    else
        a_Dw_style_box_set_val (&style_attrs.border_width, border);

    a_Dw_style_box_set_border_color (&style_attrs,
        a_Dw_style_shaded_color_new
            (html->stack[html->stack_top].current_bg_color, html->dd->bw->main_window));
    a_Dw_style_box_set_border_style (&style_attrs, DW_STYLE_BORDER_OUTSET);
    style_attrs.border_spacing = cellspacing;

#ifndef AUTOFOLD_SUPPORT
    if ((attrbuf = Html_get_attr(html, tag, tagsize, "width")))
        style_attrs.width = Html_parse_length (html, attrbuf);

    if ((attrbuf = Html_get_attr(html, tag, tagsize, "height")))
        style_attrs.height = Html_parse_length (html, attrbuf);
#endif

    if ((attrbuf = Html_get_attr(html, tag, tagsize, "align"))) {
        if (g_strcasecmp (attrbuf, "left") == 0)
            style_attrs.text_align = DW_STYLE_TEXT_ALIGN_LEFT;
        else if (g_strcasecmp (attrbuf, "right") == 0)
            style_attrs.text_align = DW_STYLE_TEXT_ALIGN_RIGHT;
        else if (g_strcasecmp (attrbuf, "center") == 0)
            style_attrs.text_align = DW_STYLE_TEXT_ALIGN_CENTER;
    }

    if (!prefs.force_my_colors &&
            (attrbuf = Html_get_attr(html, tag, tagsize, "bgcolor"))) {
        bgcolor = Html_color_parse(html, attrbuf, -1);
        if (bgcolor != -1) {
            if (bgcolor == 0xffffff && !prefs.allow_white_bg)
                bgcolor = prefs.bg_color;
            html->stack[html->stack_top].current_bg_color = bgcolor;
            style_attrs.background_color =
                a_Dw_style_color_new (bgcolor, html->dd->bw->main_window);
        }
    }

    attrbuf = Html_get_attr(html, tag, tagsize, "background");
    if (attrbuf)
    {
        new_url = a_Url_new(attrbuf, URL_STR_(html->linkblock->base_url), 0, 0, 0);
        if ((Image = a_Image_new_for_background (0, 0)))
        {
            style_attrs.background_image = a_Dw_style_bgimage_new ((DwImage*)Image->dw);
            Html_load_image(html, new_url, Image);
        }
        a_Url_free(new_url);
    }

    tstyle = a_Dw_style_new (&style_attrs, html->dd->bw->main_window);

    /* The style for the cells */
    style_attrs = *html->stack[html->stack_top].style;
    /* When mspider was started with the --debug-rendering option, there
        * is always a border around the cells. */
    if (mspider_dbg_rendering)
        a_Dw_style_box_set_val (&style_attrs.border_width, 1);
    else
        a_Dw_style_box_set_val (&style_attrs.border_width, border ? 1 : 0);

    a_Dw_style_box_set_val (&style_attrs.padding, cellpadding);
    a_Dw_style_box_set_border_color (&style_attrs, tstyle->border_color.top);
    a_Dw_style_box_set_border_style (&style_attrs, DW_STYLE_BORDER_INSET);

    old_style = html->stack[html->stack_top].table_cell_style;
    html->stack[html->stack_top].table_cell_style =
        a_Dw_style_new (&style_attrs, html->dd->bw->main_window);
    if (old_style)
        a_Dw_style_unref (old_style);

    table = a_Dw_table_new ();
    a_Dw_page_add_widget (DW_PAGE (html->dw), table, tstyle);
    a_Dw_style_unref (tstyle);

    html->stack[html->stack_top].table_mode = MSPIDER_HTML_TABLE_MODE_TOP;
    html->stack[html->stack_top].cell_text_align_set = FALSE;
    html->stack[html->stack_top].table = table;
#endif

#ifdef AUTOFOLD_SUPPORT
    tabstatck[intable++] = border;
    if ( intable > 0x3f ) {
        intable = 0x3f;
    }
#endif
}

/*
 * used by <TD> and <TH>
 */
static void Html_tag_open_table_cell(mSpiderHtml *html, char *tag, int tagsize,
                                     DwStyleTextAlignType text_align)
{
#ifdef USE_TABLES
    DwWidget *col_page;
    int colspan = 1, rowspan = 1;
    const char *attrbuf;
    DwStyle style_attrs, *style, *old_style;
    gint32 bgcolor;
    gboolean new_style;

    switch (html->stack[html->stack_top].table_mode) {
    case MSPIDER_HTML_TABLE_MODE_NONE:
        MSG_HTML("<td> or <th> outside <table>\n");
        return;

    case MSPIDER_HTML_TABLE_MODE_TOP:
        MSG_HTML("<td> or <th> outside <tr>\n");
        /* a_Dw_table_add_cell takes care that mspider does not crash. */
        /* continues */
    case MSPIDER_HTML_TABLE_MODE_TR:
    case MSPIDER_HTML_TABLE_MODE_TD:
        /* todo: check errors? */
        if ((attrbuf = Html_get_attr(html, tag, tagsize, "colspan")))
            colspan = strtol (attrbuf, NULL, 10);
        if ((attrbuf = Html_get_attr(html, tag, tagsize, "rowspan")))
            rowspan = strtol (attrbuf, NULL, 10);

        /* text style */
        old_style = html->stack[html->stack_top].style;
        style_attrs = *old_style;
        if (!html->stack[html->stack_top].cell_text_align_set)
            style_attrs.text_align = text_align;
        if (Html_get_attr(html, tag, tagsize, "nowrap"))
            style_attrs.white_space = DW_STYLE_WHITE_SPACE_NOWRAP;
        else
            style_attrs.white_space = DW_STYLE_WHITE_SPACE_NORMAL;

        html->stack[html->stack_top].style =
            a_Dw_style_new (&style_attrs, html->dd->bw->main_window);
        a_Dw_style_unref (old_style);
        Html_tag_set_align_attr (html, tag, tagsize);

        /* cell style */
        style_attrs = *html->stack[html->stack_top].table_cell_style;
        new_style = FALSE;

#ifndef AUTOFOLD_SUPPORT
        if ((attrbuf = Html_get_attr(html, tag, tagsize, "width"))) {
            style_attrs.width = Html_parse_length (html, attrbuf);
            new_style = TRUE;
        }
#endif

        if (Html_tag_set_valign_attr (html, tag, tagsize, &style_attrs))
            new_style = TRUE;

        if (!prefs.force_my_colors &&
                (attrbuf = Html_get_attr(html, tag, tagsize, "bgcolor"))) {
            bgcolor = Html_color_parse(html, attrbuf, -1);
            if (bgcolor != -1) {
                if (bgcolor == 0xffffff && !prefs.allow_white_bg)
                    bgcolor = prefs.bg_color;

                new_style = TRUE;
                style_attrs.background_color =
                    a_Dw_style_color_new (bgcolor, html->dd->bw->main_window);
                html->stack[html->stack_top].current_bg_color = bgcolor;
            }
        }

        if (html->stack[html->stack_top].style->text_align
                == DW_STYLE_TEXT_ALIGN_STRING)
            col_page = a_Dw_table_cell_new (a_Dw_table_get_cell_ref
                            (DW_TABLE (html->stack[html->stack_top].table)));
        else
            col_page = a_Dw_page_new ();

        if (new_style) {
            style = a_Dw_style_new (&style_attrs, html->dd->bw->main_window);
            a_Dw_widget_set_style (col_page, style);
            a_Dw_style_unref (style);
        }
        else
            a_Dw_widget_set_style (col_page,
                                    html->stack[html->stack_top].table_cell_style);

        a_Dw_table_add_cell (DW_TABLE (html->stack[html->stack_top].table),
                        col_page, colspan, rowspan);
        html->stack[html->stack_top].page = html->dw = col_page;

        /* Handle it when the user clicks on a link */
        Html_connect_signals(html, G_OBJECT(col_page));
        break;

    default:
        /* compiler happiness */
        break;
    }

    html->stack[html->stack_top].table_mode = MSPIDER_HTML_TABLE_MODE_TD;
#endif
}


/*
 * <TD>
 */
static void Html_tag_open_td(mSpiderHtml *html, char *tag, int tagsize)
{
    const char * attrbuf;
    DwStyle style_attrs, *style, *old_style;
    mSpiderUrl* new_url;
    mSpiderImage * Image;
    gboolean have_style;

    have_style = FALSE;

    switch (html->stack[html->stack_top].table_mode) {
        case MSPIDER_HTML_TABLE_MODE_NONE:
            MSG_HTML("<td> or <th> outside <table>\n");
            return;
        case MSPIDER_HTML_TABLE_MODE_TR:
        case MSPIDER_HTML_TABLE_MODE_TD:
            style_attrs = *html->stack[html->stack_top].table_cell_style;

            if ((attrbuf = Html_get_attr(html, tag, tagsize, "width")))
                style_attrs.width = Html_parse_length (html, attrbuf);

            if ((attrbuf = Html_get_attr(html, tag, tagsize, "height")))
                style_attrs.height = Html_parse_length (html, attrbuf);

            attrbuf = Html_get_attr(html, tag, tagsize, "background");
            if (attrbuf)
            {
                new_url = a_Url_new(attrbuf, URL_STR_(html->linkblock->base_url), 0, 0, 0);
                if ((Image = a_Image_new_for_background (0, 0))) {
                    style_attrs.background_image = a_Dw_style_bgimage_new ((DwImage*)Image->dw);
            	    Html_load_image(html, new_url, Image);
                }
                a_Url_free(new_url);
            } 

            style = a_Dw_style_new (&style_attrs, html->dd->bw->main_window);
            old_style = html->stack[html->stack_top].table_cell_style;
            html->stack[html->stack_top].table_cell_style = style;
            have_style = TRUE;
        break;
    default:
        break;
    }


        Html_tag_open_table_cell (html, tag, tagsize, DW_STYLE_TEXT_ALIGN_LEFT);

    if (have_style) {
        html->stack[html->stack_top].table_cell_style = old_style;
        a_Dw_style_unref (style);
    }
}


/*
 * <TH>
 */
static void Html_tag_open_th(mSpiderHtml *html, char *tag, int tagsize)
{
#if 0
    Html_cleanup_tag(html, "td>");
    Html_cleanup_tag(html, "th>");

    Html_push_tag(html, tag, tagsize);
#endif
    Html_set_top_font(html, NULL, 0, 1, 1);
    Html_tag_open_table_cell (html, tag, tagsize, DW_STYLE_TEXT_ALIGN_CENTER);
}


/*
 * <TR>
 */
static void Html_tag_open_tr(mSpiderHtml *html, char *tag, int tagsize)
{
    const char *attrbuf;
    DwStyle style_attrs, *style, *old_style;
    guint32 bgcolor;

#if 0
    Html_cleanup_tag(html, "td>");
    Html_cleanup_tag(html, "th>");
    Html_cleanup_tag(html, "tr>");

    Html_push_tag(html, tag, tagsize);
#endif

#ifdef USE_TABLES
    switch (html->stack[html->stack_top].table_mode) {
    case MSPIDER_HTML_TABLE_MODE_NONE:
        _MSG("Invalid HTML syntax: <tr> outside <table>\n");
        return;

    case MSPIDER_HTML_TABLE_MODE_TOP:
    case MSPIDER_HTML_TABLE_MODE_TR:
    case MSPIDER_HTML_TABLE_MODE_TD:
        style = NULL;

        if (!prefs.force_my_colors &&
                (attrbuf = Html_get_attr(html, tag, tagsize, "bgcolor"))) {
            bgcolor = Html_color_parse(html, attrbuf, -1);
            if (bgcolor != -1) {
                if (bgcolor == 0xffffff && !prefs.allow_white_bg)
                    bgcolor = prefs.bg_color;

                style_attrs = *html->stack[html->stack_top].style;
                style_attrs.background_color =
                    a_Dw_style_color_new (bgcolor, html->dd->bw->main_window);
                style =
                    a_Dw_style_new (&style_attrs, html->dd->bw->main_window);
                html->stack[html->stack_top].current_bg_color = bgcolor;
            }
        }


        a_Dw_table_add_row (DW_TABLE (html->stack[html->stack_top].table), style);
        if (style)
            a_Dw_style_unref (style);

        if (Html_get_attr (html, tag, tagsize, "align")) {
            html->stack[html->stack_top].cell_text_align_set = TRUE;
            Html_tag_set_align_attr (html, tag, tagsize);
        }

        style_attrs = *html->stack[html->stack_top].table_cell_style;
        if (Html_tag_set_valign_attr (html, tag, tagsize, &style_attrs)) {
            old_style = html->stack[html->stack_top].table_cell_style;
            html->stack[html->stack_top].table_cell_style =
                a_Dw_style_new (&style_attrs, html->dd->bw->main_window);
            a_Dw_style_unref (old_style);
        } else
        break;

    default:
        break;
    }

    html->stack[html->stack_top].table_mode = MSPIDER_HTML_TABLE_MODE_TR;
#else
    a_Dw_page_add_parbreak (DW_PAGE (html->dw), 0,
                          html->stack[(html)->stack_top].style);
#endif
}

static void Html_tag_open_noframes (mSpiderHtml *html, gchar *tag, gint tagsize)
{
   if (html->stack[html->stack_top].frame_mode == MSPIDER_HTML_FRAME_MODE_NONE)
      MSG_HTML("<noframes> outside of <frameset>!!!\n");
   /* This code will allow the misuse of <noframes> which often exists. */
   html->stack[html->stack_top].frame_mode = MSPIDER_HTML_FRAME_MODE_NOFRAMES;
}

/* Warning : this is illegal tag not existing! */
static void Html_tag_open_noframe (mSpiderHtml *html, gchar *tag, gint tagsize)
{
   MSG_HTML("<noframe> is illegal tag !!! use <noframes>.\n");
   Html_tag_open_noframes (html, tag, tagsize);
}
/*
 * <FRAME>
 * todo: This is just a temporary fix while real frame support
 *       isn't finished. Imitates lynx/w3m's frames.
 */
static void Html_tag_open_frame (mSpiderHtml *html, char *tag, int tagsize)
{
   const char *attrbuf;
   mSpiderUrl *url;
   DwWidget *widget;
   DwStyle style_attrs;
   gboolean frameborder, noresize;
   guint marginwidth, marginheight;
   mSpiderIFrameScrollType scrolling;
   DwFrameset *frameset;
   RECT *rc;
#ifdef ENABLE_LINKTRAVE
    mSpiderDoc * dd;
    LinktraveManage *linktrave_manage;
#endif
   /* no link == return */
   if ( !(attrbuf = Html_get_attr(html, tag, tagsize, "src"))) 
      return;
   if (strlen(attrbuf) == 0) 
      return;
   if (!(url = Html_url_new(html, attrbuf, NULL, 0, 0, 0, 0))) 
      return;
   

   if ((attrbuf = Html_get_attr(html, tag, tagsize, "frameborder")))
     frameborder = (strtol (attrbuf, NULL, 10) == 0 ? FALSE : TRUE);
   else
     frameborder = TRUE;

   if ((attrbuf = Html_get_attr(html, tag, tagsize, "noresize")))
     noresize = TRUE;
   else
     noresize = FALSE;

   /* if margins have not been set explicitly, use defaults */
   if ((attrbuf = Html_get_attr(html, tag, tagsize, "marginwidth")))
     marginwidth = strtol (attrbuf, NULL, 10);
   else
     marginwidth = 5;

   if ((attrbuf = Html_get_attr(html, tag, tagsize, "marginheight")))
     marginheight = strtol (attrbuf, NULL, 10);
   else
     marginheight = 5;

   if ((attrbuf = Html_get_attr(html, tag, tagsize, "scrolling")))
     scrolling = (!g_strcasecmp(attrbuf, "no") ? IFrameScroll_NO :
          (!g_strcasecmp(attrbuf, "yes") ? IFrameScroll_YES : IFrameScroll_AUTOMATIC));
   else
     scrolling = IFrameScroll_AUTOMATIC;

  // DEBUG_MSG(DEBUG_EVENT, "      URL %s\n", attrbuf);

   switch(html->stack[html->stack_top].frame_mode) {
   case MSPIDER_HTML_FRAME_MODE_NONE:
     MSG_HTML("<frame> outside of <frameset>\n");
     a_Url_free (url);
     return;
   case MSPIDER_HTML_FRAME_MODE_NOFRAMES:
     MSG_HTML("<frame> inside of <noframes>\n");
     a_Url_free (url);
     return;
   case MSPIDER_HTML_FRAME_MODE_IFRAME:
     MSG_HTML("<frame> inside of <iframe>\n");
     a_Url_free (url);
     return;
   case MSPIDER_HTML_FRAME_MODE_FRAMESET:
     
     /* set marginwidth & height */
     style_attrs.margin.left = style_attrs.margin.right = marginwidth;
     style_attrs.margin.top = style_attrs.margin.bottom = marginheight;
     if(!frameborder) {
       a_Dw_style_box_set_val(&(style_attrs.border_width), 0);
       a_Dw_style_box_set_border_style(&style_attrs, DW_STYLE_BORDER_NONE);
     }
    
     attrbuf = Html_get_attr(html, tag, tagsize, "name");
     frameset =DW_FRAMESET(html->stack[html->stack_top].frameset);
     rc = g_slist_nth_data(frameset->children_area , frameset->area_used);

    if ( !rc ) {
        a_Url_free (url);
        return; 
    }

     widget = a_Dw_MgWidget_mspider_new (html->dd, (char *)attrbuf , frameset->current_frame,
                                   url, rc->left,rc->top, rc->right, rc->bottom,
                                   scrolling, frameborder , frameset);

      a_Dw_container_add(DW_CONTAINER(frameset) , DW_WIDGET(widget));
      
      frameset->area_used++;
#ifdef ENABLE_LINKTRAVE
    linktrave_manage =(LinktraveManage*)html->dd->bw->linktrave_manage;
    dd = (mSpiderDoc*) GetWindowAdditionalData2(widget->hwnd);
    linktrave_manage->linktrave_state->ddary[linktrave_manage->linktrave_state->nr_dd] = dd;

     linktrave_manage->linktrave_state->nr_dd++;
     a_List_add (linktrave_manage->linktrave_state->ddary, 
                 linktrave_manage->linktrave_state->nr_dd,
                 linktrave_manage->linktrave_state->max_dd);
#endif
      ShowWindow(DW_WIDGET(widget)->hwnd , SW_SHOW);
     break;
     
   default:
     break;
   }
   a_Url_free (url);
}

/*
 * <IFRAME>
 */
static void Html_tag_open_iframe (mSpiderHtml *html, char *tag, int tagsize)
{
    const char *attrbuf;
    mSpiderUrl *url;
    gchar *width_ptr, *height_ptr;
    DwPage *page;
    DwStyle style_attrs, *estyle;
    DwWidget *widget;
    gboolean frameborder;
    guint marginwidth, marginheight;
    mSpiderIFrameScrollType scrolling;

    Html_tag_set_align_attr (html, tag, tagsize);

    html->stack[html->stack_top].parse_mode = MSPIDER_HTML_PARSE_MODE_VERBATIM;
    html->stack[html->stack_top].frame_mode = MSPIDER_HTML_FRAME_MODE_IFRAME;

    if (!(attrbuf = Html_get_attr(html, tag, tagsize, "src")) ||
            attrbuf[0] == '\0')
        return;

    if (!(url = Html_url_new(html, attrbuf, NULL, 0, 0, 0, 0)))
        return;

    page = DW_PAGE(html->dw);
    style_attrs = *(html->stack[html->stack_top].style);

    /* use default for width (300) and height (150) if not specified */
    if ((attrbuf = Html_get_attr(html, tag, tagsize, "width")))
      width_ptr = g_strdup(attrbuf);
    else
      width_ptr = g_strdup("300");

    if ((attrbuf = Html_get_attr(html, tag, tagsize, "height")))
      height_ptr = g_strdup(attrbuf);
    else
      height_ptr = g_strdup("150");

    if ((attrbuf = Html_get_attr(html, tag, tagsize, "frameborder")))
      frameborder = (strtol (attrbuf, NULL, 10) == 0 ? FALSE : TRUE);
    else
      frameborder = TRUE;

    if ((attrbuf = Html_get_attr(html, tag, tagsize, "marginwidth")))
      marginwidth = strtol (attrbuf, NULL, 10);
    else
      marginwidth = 0;

    if ((attrbuf = Html_get_attr(html, tag, tagsize, "marginheight")))
      marginheight = strtol (attrbuf, NULL, 10);
    else
      marginheight = 0;

    if ((attrbuf = Html_get_attr(html, tag, tagsize, "scrolling")))
      scrolling = (!g_strcasecmp(attrbuf, "no") ? IFrameScroll_NO:
           (!g_strcasecmp(attrbuf, "yes") ? IFrameScroll_YES :
           IFrameScroll_AUTOMATIC));
    else
      scrolling = IFrameScroll_AUTOMATIC;

    attrbuf = Html_get_attr(html, tag, tagsize, "name");

     /* set margins */
    style_attrs.margin.left = style_attrs.margin.right = marginwidth;
    style_attrs.margin.top = style_attrs.margin.bottom = marginheight;

    if (!frameborder)
        a_Dw_style_box_set_val (&style_attrs.border_width, frameborder);

    estyle = a_Dw_style_new (&style_attrs, html->dd->bw->main_window);

    page->num_iframe ++;
    
    a_Url_set_referer(url, html->linkblock->base_url);
    
    widget = a_Dw_MgWidget_frame_new (html->dd, (gchar *)attrbuf, 
                    page->num_iframe, url,
                    0, 0,
                    strtol (width_ptr, NULL, 10),
                    strtol (height_ptr, NULL, 10),
                    scrolling, frameborder);
    
    Html_add_widget (html, widget, width_ptr, height_ptr, estyle);

    a_Dw_style_unref (estyle);
    a_Url_free (url);
    g_free (width_ptr);
    g_free (height_ptr);
}

/*
 * <FRAMESET>
 * todo: This is just a temporary fix while real frame support
 *       isn't finished. Imitates lynx/w3m's frames.
 */
static void Html_tag_open_frameset (mSpiderHtml *html, char *tag, int tagsize)
{
   const char *attrbuf;
   DwWidget *frameset;
   gchar *rows, *cols;
   DwFrameset *prv_frameset;

#ifdef ENABLE_LINKTRAVE
    LinktraveManage *linktrave_manage;
    linktrave_manage =(LinktraveManage*)html->dd->bw->linktrave_manage;
#endif
   frameset = NULL;
   prv_frameset = NULL;
#if 0
   /* first, see if this frameset tag is in the right place... */
   if(html->stack[html->stack_top].parse_mode != MSPIDER_HTML_PARSE_MODE_INIT) {
     MSG_HTML("incorrectly placed <frameset>\n");
     return;
   }
#endif
   /* get frameset attributes */
   if ((attrbuf = Html_get_attr(html, tag, tagsize, "rows")))
     rows = g_strdup(attrbuf);
   else
     rows = NULL;
   if ((attrbuf = Html_get_attr(html, tag, tagsize, "cols")))
     cols = g_strdup(attrbuf);
   else
     cols = NULL;

#ifdef AUTOFOLD_SUPPORT
    if ( cols ) {
        rows = cols;
        cols = NULL;
    }
#endif
   switch(html->stack[html->stack_top].frame_mode) {
   case MSPIDER_HTML_FRAME_MODE_NOFRAMES:
     MSG_HTML("<frameset> inside of <noframes>\n");
     g_free(rows);
     g_free(cols);
     return;
   case MSPIDER_HTML_FRAME_MODE_IFRAME:
     MSG_HTML("<frameset> inside of <iframe>\n");
     g_free(rows);
     g_free(cols);
     return;
   case MSPIDER_HTML_FRAME_MODE_NONE:
    /*  'root' frameset, so add the frameset to the dd */
     if(html->dd->frameset) {
      g_signal_emit_by_name (DW_WIDGET(html->dd->frameset),"destroy",0);
#ifdef ENABLE_LINKTRAVE
      g_free (linktrave_manage->linktrave_state->ddary);
       linktrave_manage->stack_top--; 
       a_Linktrave_state_destroy (linktrave_manage->linktrave_state);
       linktrave_manage->linktrave_state = 
            linktrave_manage->stack[linktrave_manage->stack_top];
       linktrave_manage->link_number_focus = linktrave_manage->linktrave_state->link_no; 
#endif
     }
     frameset = a_Dw_frameset_new(rows, cols);
     html->dd->frameset = frameset;
     a_Dw_container_add(DW_CONTAINER(DW_WIDGET(DW_PAGE(html->dw))) ,DW_WIDGET(frameset));
     
     DW_FRAMESET(frameset)->top_level = TRUE;
     DW_WIDGET(frameset)->add_data = (DWORD)html->dd->bw;

#ifdef ENABLE_LINKTRAVE
     linktrave_manage->stack_top++;
     linktrave_manage->linktrave_state = a_Linktrave_state_new();
     a_List_add (linktrave_manage->stack, linktrave_manage->stack_top,
                                         linktrave_manage->stack_max); 
     linktrave_manage->stack[linktrave_manage->stack_top] =
                             linktrave_manage->linktrave_state;
     linktrave_manage->linktrave_state->mode = LSM_MULIT;
     linktrave_manage->linktrave_state->ddary =
                 g_new0 (mSpiderDoc* , linktrave_manage->linktrave_state->max_dd);
     linktrave_manage->linktrave_state->cur_set = frameset; 
     linktrave_manage->link_number_focus = -2;
#endif
     /* hide the docwin, show the frameset */     
     ShowWindow(html->dd->docwin , SW_HIDE);
     ShowWindow(DW_WIDGET(html->dd->frameset)->hwnd , SW_SHOW);
     
      g_signal_emit_by_name (DW_WIDGET(html->dd->frameset),"realize",0);
     break;
   case MSPIDER_HTML_FRAME_MODE_FRAMESET:
     /* nested frameset */
     frameset = a_Dw_frameset_new(rows, cols);
     
     prv_frameset =DW_FRAMESET(html->stack[html->stack_top].frameset);
     DW_WIDGET(frameset)->add_data =
        (DWORD) g_slist_nth_data(prv_frameset->children_area , prv_frameset->area_used);

     DW_WIDGET(frameset)->parent = DW_WIDGET(prv_frameset);
     a_Dw_container_add(DW_CONTAINER(prv_frameset) , DW_WIDGET(frameset));
     prv_frameset->area_used++;
     
     ShowWindow(DW_WIDGET(frameset)->hwnd , SW_SHOW);
      g_signal_emit_by_name (DW_WIDGET(frameset),"realize",0);
     break;
   default:
     break;
   }
     
   g_free(rows);
   g_free(cols);


   /* once set, this flag does not get reset, even when the frameset tag
    * is closed. This is intentional. */
   html->InFlags |= IN_FRAMESET;

   /* put the current frameset state on the stack */
   html->stack[html->stack_top].frame_mode = MSPIDER_HTML_FRAME_MODE_FRAMESET;
   html->stack[html->stack_top].frameset = frameset;
}

/*
 * <H1> | <H2> | <H3> | <H4> | <H5> | <H6>
 */
static void Html_tag_open_h(mSpiderHtml *html, char *tag, int tagsize)
{
#if 0
    Html_par_push_tag(html, tag, tagsize);
#endif
   a_Dw_page_add_parbreak(DW_PAGE (html->dw), 9,
                          html->stack[(html)->stack_top].style);

    /* todo: combining these two would be slightly faster */
    Html_set_top_font(html, prefs.vw_fontname,
                        Html_level_to_fontsize(FontSizesNum - (tag[2] - '0')),
                        1, 3);
    Html_tag_set_align_attr (html, tag, tagsize);

    /* First finalize unclosed H tags (we test if already named anyway) */
#if 0
    a_Menu_pagemarks_set_text(html->bw, html->Stash->str);
    a_Menu_pagemarks_add(html->bw, DW_PAGE (html->dw),
                        html->stack[html->stack_top].style, (tag[2] - '0'));
#endif
    Html_stash_init(html);
    html->stack[html->stack_top].parse_mode =
            MSPIDER_HTML_PARSE_MODE_STASH_AND_BODY;
}

/*
 * Handle close: <H1> | <H2> | <H3> | <H4> | <H5> | <H6>
 */
static void Html_tag_close_h(mSpiderHtml *html, int TagIdx)
{
#if 0
    a_Menu_pagemarks_set_text(html->bw, html->Stash->str);
#endif
    a_Dw_page_add_parbreak(DW_PAGE (html->dw), 9,
                            html->stack[(html)->stack_top].style);
    Html_pop_tag(html, TagIdx);
}

/*
 * <BIG> | <SMALL>
 */
static void Html_tag_open_big_small(mSpiderHtml *html, char *tag, int tagsize)
{
    int level;

    level =
            Html_fontsize_to_level(html->stack[html->stack_top].style->font->size) +
            ((g_strncasecmp(tag+1, "big", 3)) ? -2 : 2);
    Html_set_top_font(html, NULL, Html_level_to_fontsize(level), 0, 0);
}

/*
 * <BR>
 */
static void Html_tag_open_br(mSpiderHtml *html, char *tag, int tagsize)
{
    a_Dw_page_add_linebreak(DW_PAGE (html->dw),
                           html->stack[(html)->stack_top].style);
}

/*
 * <BUTTON>
 */
static void Html_tag_open_button(mSpiderHtml *html, char *tag, int tagsize)
{
    /*
     * Buttons are rendered on one line, this is (at several levels) a
     * bit simpler. May be changed in the future.
     */
    DwStyle style_attrs, *style;
    DwWidget *button, *page;
    mSpiderHtmlForm *form;
    mSpiderHtmlLB *html_lb;
    mSpiderHtmlInputType inp_type;
    //const char *attrbuf;
    char *name, *value, *type, *disable;
    int mgwidget_id = 0;
    /* Render the button */
    //Html_push_tag(html, tag, tagsize);
    style_attrs = *html->stack[html->stack_top].style;

    a_Dw_style_box_set_val(&style_attrs.margin, 0);
    a_Dw_style_box_set_val(&style_attrs.border_width, 0);
    a_Dw_style_box_set_val(&style_attrs.padding, 0);
    style = a_Dw_style_new (&style_attrs, html->dd->bw->main_window);

    /* The new button is not set button-insensitive, since nested buttons
     * (if they are anyway allowed, todo: search in spec) should all be
     * activatable. */
    value = Html_get_attr_wdef(html, tag, tagsize, "value", NULL);
    disable = Html_get_attr_wdef(html, tag, tagsize, "disabled", "false");

    html_lb = html->linkblock;
    if( html_lb->num_forms - 1 < 0)
        return; 
    form = &(html_lb->forms[html_lb->num_forms - 1]);
    type = Html_get_attr_wdef(html, tag, tagsize, "type", "");
    mgwidget_id = (html_lb->num_forms << 16) | (form->num_inputs);
    button = a_Dw_MgWidget_button_new (html, mgwidget_id, value, (DWORD)html_lb);

    if (g_strcasecmp (disable, "true") == 0)
        EnableWindow (((DwMgWidget*)button)->window, FALSE);
    else
        EnableWindow (((DwMgWidget*)button)->window, TRUE);
        
    a_Dw_page_add_widget (DW_PAGE (html->dw), button, style);
    a_Dw_style_unref (style);

    a_Dw_style_box_set_val(&style_attrs.margin, 5);
    style = a_Dw_style_new (&style_attrs, html->dd->bw->main_window);
    page = a_Dw_page_new ();
    a_Dw_widget_set_style (page, style);
    a_Dw_style_unref (style);
    a_Dw_container_add (DW_CONTAINER (button), page);
    a_Dw_style_box_set_val(&style_attrs.margin, 0);


    html->stack[html->stack_top].page = html->dw = page;

    /* Handle it when the user clicks on a link */
    Html_connect_signals(html, G_OBJECT(page));

    /* Connect it to the form */

    if (g_strcasecmp(type, "submit") == 0)
        inp_type = MSPIDER_HTML_INPUT_BUTTON_SUBMIT;
    else if (g_strcasecmp(type, "reset") == 0) 
        inp_type = MSPIDER_HTML_INPUT_BUTTON_RESET;
    else
        return;

    a_Dw_MgWidget_set_notification (((DwMgWidget*)button)->window, inp_type);

    name = Html_get_attr_wdef(html, tag, tagsize, "name", NULL);

    Html_add_input(form, inp_type, (DwMgWidget*)button, name, value,
                NULL, FALSE);

    g_free(type);
    g_free(name);
    g_free(value);
    g_free(disable);
}

/*
 * <Flash>
 */
#ifdef ENABLE_FLASH
static void Html_tag_open_flash(mSpiderHtml *html, char *tag, int tagsize)
{
    DwStyle style_attrs, *style;
    DwWidget *flash, *page;
    mSpiderHtmlLB *html_lb;
    int width=0,height=0;
    const char *attrbuf;
    HWND hwnd = HWND_INVALID; 
    mSpiderUrl *baseurl = NULL;
    mSpiderHtmlLB *lb = NULL;
    char *base;
    char *temp2;
    char *temp3;

    temp2=(char *)malloc(100); 
    temp3=(char *)malloc(100); 
    lb =html->dd->html_block;
    baseurl=lb->base_url;
    base = baseurl->url_string->str;
     if(flashflag<0)
     {
         return;
     }
     flashflag++;

    if (!(attrbuf = Html_get_attr(html, tag, tagsize, "type"))) 
       return;

    if(strcmp(attrbuf,"application/x-shockwave-flash")!=0)
        return;

    style_attrs = *html->stack[html->stack_top].style;

    if ((attrbuf = Html_get_attr(html, tag, tagsize, "width")))
        width = strtol(attrbuf, NULL, 10);

    if ((attrbuf = Html_get_attr(html, tag, tagsize, "height")))
        height = strtol(attrbuf, NULL, 10);
    if(width==0||height==0)
        return;

    page =(DwWidget*)DW_PAGE (html->dw);

    //printf("the flash width and height is %d and %d\n",width,height);

    a_Dw_style_box_set_val(&style_attrs.margin, 0);
    a_Dw_style_box_set_val(&style_attrs.border_width, 0);
    a_Dw_style_box_set_val(&style_attrs.padding, 0);
    style = a_Dw_style_new (&style_attrs, html->dd->bw->main_window);

    html_lb = html->linkblock;

    flash = a_Dw_MgWidget_flash_new (html, 0, width, height, (DWORD)html_lb, &hwnd);

    if (!(attrbuf = Html_get_attr(html, tag, tagsize, "src"))) 
       return;
    attrbuf = correcturl(html->dd, attrbuf);
    GetFlashFile((char*)attrbuf, hwnd);


    a_Dw_page_add_widget (DW_PAGE (html->dw), flash, style);

    a_Dw_style_unref (style);

    style = a_Dw_style_new (&style_attrs, html->dd->bw->main_window);

    a_Dw_widget_set_style (page, style);

    a_Dw_style_unref (style);

    a_Dw_container_add (DW_CONTAINER (flash), page);

    //a_Dw_style_box_set_val(&style_attrs.margin, 0);

    html->stack[html->stack_top].page = html->dw = page;

    /* Handle it when the user clicks on a link */
    //Html_connect_signals(html, G_OBJECT(flash));
}
#endif

static void Html_tag_open_font(mSpiderHtml *html, char *tag, int tagsize)
{
    DwStyle style_attrs, *old_style;
    DwStyleFont font;
    const char *attrbuf;
    int size;
    gint32 color;

    if (!prefs.force_my_colors) {
        old_style = html->stack[html->stack_top].style;
        style_attrs = *old_style;

        if ((attrbuf = Html_get_attr(html, tag, tagsize, "color"))) {
         if (prefs.contrast_visited_color && html->InVisitedLink) {
            color = html->linkblock->visited_color;
         } else { 
            /* use the tag-specified color */
            color = Html_color_parse(
                       html, attrbuf, style_attrs.color->color_val);
            style_attrs.color = a_Dw_style_color_new
               (color, html->dd->bw->main_window);
         }
        }

    font = *( style_attrs.font );

    size = 0;
    if ((attrbuf = Html_get_attr(html, tag, tagsize, "size"))){
        size = strtol(attrbuf, NULL, 10);    
        font.size = (int)((FontSizes[0]+ size*2)*prefs.font_factor+0.5);
    }

    if ((attrbuf = Html_get_attr(html, tag, tagsize, "face"))) {
        font.name = attrbuf;
    }
    style_attrs.font = a_Dw_style_font_new_from_list (&font, font.name,
                        html->logfont? html->logfont->charset:DEF_CHARSET);

        html->stack[html->stack_top].style =
            a_Dw_style_new (&style_attrs, html->dd->bw->main_window);
        a_Dw_style_unref (old_style);
    }

}

#ifdef _TOOL_TIP_
/*
 * <ABBR>
 */
static void Html_tag_open_abbr(mSpiderHtml *html, char *tag, gint tagsize)
{
   DwTooltip *tooltip;
   const char *attrbuf;

   if ((attrbuf = Html_get_attr(html, tag, tagsize, "title"))) {
      tooltip = a_Dw_tooltip_new_no_ref(attrbuf);
      HTML_SET_TOP_ATTR(html, x_tooltip, tooltip);
   }
}
#endif

/*
 * <B>
 */
static void Html_tag_open_b(mSpiderHtml *html, char *tag, int tagsize)
{
    //Html_push_tag(html, tag, tagsize);
    Html_set_top_font(html, NULL, 0, 1, 1);
}

/*
 * <STRONG>
 */
static void Html_tag_open_strong(mSpiderHtml *html, char *tag, int tagsize)
{
    //Html_push_tag(html, tag, tagsize);
    Html_set_top_font(html, NULL, 0, 1, 1);
}

/*
 * <I>
 */
static void Html_tag_open_i(mSpiderHtml *html, char *tag, int tagsize)
{
    //Html_push_tag(html, tag, tagsize);
    Html_set_top_font(html, NULL, 0, 2, 2);
}

/*
 * <EM>
 */
static void Html_tag_open_em(mSpiderHtml *html, char *tag, int tagsize)
{
    //Html_push_tag(html, tag, tagsize);
    Html_set_top_font(html, NULL, 0, 2, 2);
}

/*
 * <CITE>
 */
static void Html_tag_open_cite(mSpiderHtml *html, char *tag, int tagsize)
{
    //Html_push_tag(html, tag, tagsize);
    Html_set_top_font(html, NULL, 0, 2, 2);
}

/*
 * <CENTER>
 */
static void Html_tag_open_center(mSpiderHtml *html, char *tag, int tagsize)
{
    a_Dw_page_add_parbreak(DW_PAGE (html->dw), 0,
                            html->stack[(html)->stack_top].style);
    //Html_push_tag(html, tag, tagsize);
    HTML_SET_TOP_ATTR(html, text_align, DW_STYLE_TEXT_ALIGN_CENTER);
}

/*
 * <ADDRESS>
 */
static void Html_tag_open_address(mSpiderHtml *html, char *tag, gint tagsize)
{
   a_Dw_page_add_parbreak(DW_PAGE (html->dw), 9,
                          html->stack[(html)->stack_top].style);
   Html_set_top_font(html, NULL, 0, 2, 2);
}

/*
 * <TT>
 */
static void Html_tag_open_tt(mSpiderHtml *html, char *tag, int tagsize)
{
    //Html_push_tag(html, tag, tagsize);
    Html_set_top_font(html, prefs.fw_fontname, 0, 0, 0);
}

/*
 * Read image associated tag attributes,
 * create new image and add it to the html page (if add is TRUE).
 */
static mSpiderImage *Html_add_new_image(mSpiderHtml *html, char *tag,
                                      int tagsize, DwStyle *style_attrs,
                                      gboolean add)
{
    mSpiderImage *Image;
    char *width_ptr=NULL, *height_ptr=NULL, *title_ptr, *alt_ptr, *align_ptr;
    const char *attrbuf;
    int space;

    width_ptr = NULL;
    if ((attrbuf = Html_get_attr(html, tag, tagsize, "width")))
        width_ptr = g_strdup(attrbuf);

    height_ptr = NULL;
    if ((attrbuf = Html_get_attr(html, tag, tagsize, "height")))
        height_ptr = g_strdup(attrbuf);
        
    title_ptr = NULL;
    if ((attrbuf = Html_get_attr(html, tag, tagsize, "title")))
        title_ptr = g_strdup(attrbuf);

    alt_ptr = NULL;
    if ((attrbuf = Html_get_attr(html, tag, tagsize, "alt")))
        alt_ptr = g_strdup(attrbuf);

    align_ptr = NULL;
    if ((attrbuf = Html_get_attr(html, tag, tagsize, "align"))) {
        align_ptr = g_strdup(attrbuf);
#if 0
        if (g_strcasecmp (align_ptr, "left") == 0)
            style_attrs->text_align = DW_STYLE_TEXT_ALIGN_LEFT;
        else if (g_strcasecmp (align_ptr, "right") == 0)
            style_attrs->text_align = DW_STYLE_TEXT_ALIGN_RIGHT;
        else if (g_strcasecmp (align_ptr, "top") == 0)
            style_attrs->valign = DW_STYLE_VALIGN_TOP;
        else if (g_strcasecmp (align_ptr, "bottom") == 0)
            style_attrs->valign = DW_STYLE_VALIGN_BOTTOM;
        else if (g_strcasecmp (align_ptr, "middle") == 0)
            style_attrs->valign = DW_STYLE_VALIGN_MIDDLE;
#endif
    }

    if ((width_ptr && !height_ptr) || (height_ptr && !width_ptr))
        MSG_HTML("IMG tag only specifies <%s>\n",
                (width_ptr) ? "width" : "height");

    /* Spacing to the left and right */
    if ((attrbuf = Html_get_attr(html, tag, tagsize, "hspace"))) {
        space = strtol(attrbuf, NULL, 10);

        if (space > 0)
            style_attrs->margin.left = style_attrs->margin.right = space;
    }

    /* Spacing at the top and bottom */
    if ((attrbuf = Html_get_attr(html, tag, tagsize, "vspace"))) {
        space = strtol(attrbuf, NULL, 10);

        if (space > 0)
            style_attrs->margin.top = style_attrs->margin.bottom = space;
    }

    /* Add a new image widget to this page */
    if ((Image = a_Image_new(0, 0, alt_ptr,
                                html->stack[html->stack_top].current_bg_color)))
        if (add)
            Html_add_widget(html, DW_WIDGET(Image->dw), width_ptr, height_ptr,
                            style_attrs);

    if(width_ptr)
        g_free(width_ptr);
    if(height_ptr)
        g_free(height_ptr);
    g_free(title_ptr);
    g_free(alt_ptr);
    g_free(align_ptr);
    return Image;
}

/*
 * Tell cache to retrieve image
 */
static void Html_load_image(mSpiderHtml *html, mSpiderUrl *url, mSpiderImage *Image)
{
    mSpiderWeb *Web;
    int ClientKey;

   a_Url_set_referer(url, html->linkblock->base_url);
   /* Fill a Web structure for the cache query */
   if (URL_FLAGS(html->linkblock->base_url) && URL_MustCache)
       URL_FLAGS(url) |= URL_MustCache; /* cache page, cache images */
   URL_FLAGS(url) |= URL_IsImage;
    /* Fill a Web structure for the cache query */
    Web = a_Web_new(url);
    Web->dd = html->dd;
    Web->Image = Image;
    Web->flags |= WEB_Image;
    /* Request image data from the cache */
    if ((ClientKey = a_Cache_open_url (Web, NULL, NULL)) != 0) {
        Image->dw->client_id = ClientKey;
        a_Doc_add_client (html->dd, ClientKey, 0);
        a_Doc_add_url (html->dd, url, WEB_Image);
    }
}
/*
 * Try to reload a image from the url that was given, use the image structure that has 
 * be constructed!
 */
void Html_reload_image(mSpiderDoc *HtmlImagedoc, mSpiderUrl *url,mSpiderImage *Image)
{
    mSpiderWeb *Web;
    int ClientKey;
   a_Url_set_referer(url, ((mSpiderHtmlLB*)HtmlImagedoc->html_block)->base_url);
   /* Fill a Web structure for the cache query */
   if (URL_FLAGS(((mSpiderHtmlLB*)HtmlImagedoc->html_block)->base_url) && URL_MustCache)
       URL_FLAGS(url) |= URL_MustCache; /* cache page, cache images */
   URL_FLAGS(url) |= URL_IsImage;
    /* Fill a Web structure for the cache query */
    Web = a_Web_new(url);
    Web->dd = HtmlImagedoc;
    Web->Image = Image;
    Web->flags |= WEB_Image;
    /* Request image data from the cache */
    if ((ClientKey = a_Cache_open_url (Web, NULL, NULL)) != 0) {
        Image->dw->client_id = ClientKey;
        a_Doc_add_client (HtmlImagedoc, ClientKey, 0);
        a_Doc_add_url (HtmlImagedoc, url, WEB_Image);
    }
}
/*
 * Create a new Image struct and request the image-url to the cache
 * (If it either hits or misses, is not relevant here; that's up to the
 *  cache functions)
 */
static void Html_tag_open_img(mSpiderHtml *html, char *tag, int tagsize)
{
    mSpiderImage *Image;
    mSpiderUrl *url, *usemap_url,*jsurl;
    DwPage *page;
    DwStyle style_attrs;
    const char *attrbuf;
    int border;
    
#ifdef JS_SUPPORT
    jsobject *jsobj = NULL;
    jsobject *pobj = NULL;
    jsscript_interpreter *interpreter = NULL;
    JSContext *ctx = NULL;
    JSObject *docobj = NULL;
    char *java = NULL;
    const char *onclick;
#endif
    
    /* This avoids loading images. Useful for viewing suspicious HTML email. */
    if (URL_FLAGS(html->linkblock->base_url) & URL_SpamSafe)
        return;

    if (!(attrbuf = Html_get_attr(html, tag, tagsize, "src")) 
            || !(url = Html_url_new(html, attrbuf, NULL, 0, 0, 0, 0)))
        return;

    if (a_Url_get_ccc_funct(url) == NULL) {/* unsupported*/
        a_Url_free(url);
        return;
    }

    page = DW_PAGE (html->dw);

    usemap_url = NULL;
    if ((attrbuf = Html_get_attr(html, tag, tagsize, "usemap")))
        /* todo: usemap URLs outside of the document are not used. */
        usemap_url = Html_url_new(html, attrbuf, NULL, 0, 0, 0, 0);

    style_attrs = *html->stack[html->stack_top].style;
#ifdef JS_SUPPORT 
    onclick = Html_get_attr(html, tag, tagsize, "onclick");
    if(onclick != NULL && html->stack[html->stack_top].style->x_link == -1)
    {
        java = g_malloc0 (strlen(onclick)+12);
        strcpy (java , "javascript:");
        strcat(java,onclick);
        jsurl = Html_url_new(html, java, NULL, 0, 0, 0, 0);
        style_attrs.x_link = Html_set_new_link(html, &jsurl);
        g_free (java);
    }
#endif

   if (html->stack[html->stack_top].style->x_link != -1 ||
        usemap_url != NULL) {
        /* Images within links */
        border = 1;
   } else
        border = 0;

        if ((attrbuf = Html_get_attr(html, tag, tagsize, "border")))
            border = strtol (attrbuf, NULL, 10);

        if (html->stack[html->stack_top].style->x_link == -1 )
            /* In this case we can use the text color */
            a_Dw_style_box_set_border_color (&style_attrs,
                    a_Dw_style_shaded_color_new (style_attrs.color->color_val, html->dd->bw->main_window));
        else
            a_Dw_style_box_set_border_color (&style_attrs,
                a_Dw_style_shaded_color_new (html->linkblock->link_color, html->dd->bw->main_window));

        a_Dw_style_box_set_border_style (&style_attrs, DW_STYLE_BORDER_SOLID);
        a_Dw_style_box_set_val (&style_attrs.border_width, border);

    Image = Html_add_new_image(html, tag, tagsize, &style_attrs, TRUE);
    Html_connect_signals(html, G_OBJECT(Image->dw));

    /* Image maps */
    if (Html_get_attr(html, tag, tagsize, "ismap")) {
        /* BUG: if several ISMAP images follow each other without
        * being separated with a word, only the first one is ISMAPed
        */
        a_Dw_image_set_ismap (Image->dw);
        _MSG("  Html_tag_open_img: server-side map (ISMAP)\n");
    } else if (html->stack[html->stack_top].style->x_link != -1 &&
                usemap_url == NULL )
        /* For simple links, we have to suppress the "image_pressed" signal.
        * This is overridden for USEMAP images. */
        a_Dw_widget_set_button_sensitive (DW_WIDGET (Image->dw), FALSE);

#ifdef JS_SUPPORT
    if(onclick != NULL)
        a_Dw_widget_set_button_sensitive (DW_WIDGET (Image->dw), FALSE);
#endif    

    if (usemap_url) {
        a_Dw_image_set_usemap (Image->dw, &html->linkblock->maps, usemap_url);
        a_Url_free (usemap_url);
    } 

    Html_load_image(html, url, Image);
#ifdef JS_SUPPORT
    /* vitrual form object */

    pobj = js_getdftdocobj((int)html->dd);
    if ( pobj ) {
        interpreter = pobj->jsinterpreter;

        if ( interpreter ) {
            ctx = (JSContext*)interpreter->backend_data;
            docobj = pobj->smobj;
        }

        jsobj = js_objnew(DW_WIDGET(Image->dw), jsimage, pobj, html, tag, tagsize);
        jsobj->pvar1 = Image->dw;
        jsobj->pvar2 = page;
        jsobj->pvar3 = html->dd;
        if ( jsobj ) {
            /* common */
            pobj->children = g_list_append((GList*)pobj->children, jsobj);
            html->linkblock->images = g_list_append((GList*)html->linkblock->images, jsobj);
        }
    }
#endif
    a_Url_free(url);
}

/*
 * <map>
 */
static void Html_tag_open_map(mSpiderHtml *html, char *tag, int tagsize)
{
    char *hash_name;
    const char *attrbuf;
    mSpiderUrl *url;

    //Html_push_tag(html, tag, tagsize);
    if (html->InFlags & IN_MAP) {
        MSG_HTML("nested <map>\n");
    } else {
        if ((attrbuf = Html_get_attr(html, tag, tagsize, "name"))) {
            hash_name = g_strdup_printf("#%s", attrbuf);
            url = Html_url_new(html, hash_name, NULL, 0, 0, 0, 0);
            a_Dw_image_map_list_add_map (&html->linkblock->maps, url);
            a_Url_free (url);
            g_free(hash_name);
        }
        html->InFlags |= IN_MAP;
    }
}

/*
 * Handle close <MAP>
 */
static void Html_tag_close_map(mSpiderHtml *html, int TagIdx)
{
    html->InFlags &= ~IN_MAP;
    Html_pop_tag(html, TagIdx);
}

/*
 * Read coords in a string and fill a POINT array
 */
static int Html_read_coords(mSpiderHtml *html, const char *str, POINT *array)
{
    int i, toggle, pending, coord;
    const char *tail = str;
    char *newtail = NULL;

    i = 0;
    toggle = 0;
    pending = 1;
    while ( pending ) {
        coord = strtol(tail, &newtail, 10);
        if (toggle) {
            array[i].y = coord;
            array[++i].x = 0;
            toggle = 0;
        } else {
            array[i].x = coord;
            array[i].y = -1;
            toggle = 1;
        }
        if (!*newtail || (coord == 0 && newtail == tail)) {
            pending = 0;
        }
        else {
            if (*newtail != ',') {
                MSG_HTML("usemap coords MUST be separated with ','\n");
            }
            tail = newtail + 1;
        }
   }

   return i;
}

/*
 * <AREA>
 */
static void Html_tag_open_area(mSpiderHtml *html, char *tag, int tagsize)
{
    /* todo: point must be a dynamic array */
    POINT point[1024];
    mSpiderUrl* url;
    const char *attrbuf;
    int type = DW_IMAGE_MAP_SHAPE_RECT;
    int nbpoints, link = -1;

    if ( (attrbuf = Html_get_attr(html, tag, tagsize, "shape")) ) {
        if ( g_strcasecmp(attrbuf, "rect") == 0 )
            type = DW_IMAGE_MAP_SHAPE_RECT;
        else if ( g_strcasecmp(attrbuf, "circle") == 0 )
            type = DW_IMAGE_MAP_SHAPE_CIRCLE;
        else if ( g_strncasecmp(attrbuf, "poly", 4) == 0 )
            type = DW_IMAGE_MAP_SHAPE_POLY;
        else
            type = DW_IMAGE_MAP_SHAPE_RECT;
    }
    /* todo: add support for coords in % */
    if ( (attrbuf = Html_get_attr(html, tag, tagsize, "coords")) ) {
        /* Is this a valid poly ?
         * rect = x0,y0,x1,y1               => 2
         * circle = x,y,r                   => 2
         * poly = x0,y0,x1,y1,x2,y2 minimum => 3 */
        nbpoints = Html_read_coords(html, attrbuf, point);
    }
    else
        return;

    if ( Html_get_attr(html, tag, tagsize, "nohref") ) {
        link = -1;
        _MSG("nohref");
    }

    if ((attrbuf = Html_get_attr(html, tag, tagsize, "href"))) {
        url = Html_url_new(html, attrbuf, NULL, 0, 0, 0, 0);
        if (url == NULL) return;
        if ((attrbuf = Html_get_attr(html, tag, tagsize, "alt")))
            a_Url_set_alt(url, attrbuf);

        if ((attrbuf = Html_get_attr(html, tag, tagsize, "target")))
            a_Url_set_target(url, attrbuf);
        else if (URL_TARGET_(html->linkblock->base_url))
            a_Url_set_target(url, URL_TARGET_(html->linkblock->base_url));

        link = Html_set_new_link(html, &url);
    }

    a_Dw_image_map_list_add_shape(&html->linkblock->maps, type, link,
                                 point, nbpoints);
}


/*
 * Test and extract the link from a javascript instruction.
 */
static const char* Html_get_javascript_link(mSpiderHtml *html)
{
   size_t i;
   char ch, *p1, *p2;
   GString *Buf = html->attr_data;

   if (g_strncasecmp("javascript", Buf->str, 10) == 0) {
      i = strcspn(Buf->str, "'\"");
      ch = Buf->str[i];
      if ((ch == '"' || ch == '\'') &&
          (p2 = strchr(Buf->str + i + 1 , ch))) {
         p1 = Buf->str + i;
         MSG_HTML("link depends on javascript()\n");
         g_string_truncate(Buf, p2 - Buf->str);
         g_string_erase(Buf, 0, p1 - Buf->str + 1);
      }
   }
   return Buf->str;
}

/*
 * Register an anchor for this page.
 */
static void Html_add_anchor(mSpiderHtml *html, const char *name)
{
   _MSG("Registering ANCHOR: %s\n", name);
   if (!a_Dw_page_add_anchor(DW_PAGE(html->dw), name,
                             html->stack[html->stack_top].style))
      MSG_HTML("Anchor names must be unique within the document\n");
   /* According to Sec. 12.2.1 of the HTML 4.01 spec, "anchor names that
    * differ only in case may not appear in the same document", but
    * "comparisons between fragment identifiers and anchor names must be
    * done by exact (case-sensitive) match." We ignore the case issue and
    * always test for exact matches. Moreover, what does uppercase mean
    * for Unicode characters outside the ASCII range? */
}

/*
 * <A>
 */
static void Html_tag_open_a(mSpiderHtml *html, char *tag, int tagsize)
{
    DwPage *page;
    DwStyle style_attrs, *old_style;
    mSpiderUrl *url;
    const char *attrbuf;

    page = DW_PAGE (html->dw);
    /* todo: add support for MAP with A HREF */
    Html_tag_open_area(html, tag, tagsize);

    if ( (attrbuf = Html_get_attr(html, tag, tagsize, "href"))) {
        /* if it's a javascript link, extract the reference. */
        if (tolower(attrbuf[0]) == 'j'&&(strstr(attrbuf,"javascript:")==0))
            attrbuf = Html_get_javascript_link(html);

        url = Html_url_new(html, attrbuf, NULL, 0, 0, 0, 0);
        if (url == NULL) return;

        if ((attrbuf = Html_get_attr(html, tag, tagsize, "target"))) {
            a_Url_set_target(url, attrbuf);
        }
        else if (URL_TARGET_(html->linkblock->base_url))
            a_Url_set_target(url, URL_TARGET_(html->linkblock->base_url));

        old_style = html->stack[html->stack_top].style;
        style_attrs = *old_style;

        if (a_Url_does_visited (url)) {
            html->InVisitedLink = TRUE;
            style_attrs.color = a_Dw_style_color_new
                (html->linkblock->visited_color, html->dd->bw->main_window);
        }
        else {
            style_attrs.color = a_Dw_style_color_new 
                (html->linkblock->link_color, html->dd->bw->main_window);
        }

        style_attrs.text_decoration |= DW_STYLE_TEXT_DECORATION_UNDERLINE;
        style_attrs.x_link = Html_set_new_link(html, &url);

        html->stack[html->stack_top].style =
            a_Dw_style_new (&style_attrs, html->dd->bw->main_window);
        a_Dw_style_unref (old_style);
    }
    if ( (attrbuf = Html_get_attr(html, tag, tagsize, "name"))) {
      if (prefs.show_extra_warnings)
         Html_check_name_val(html, attrbuf, "name");
      /* html->NameVal is freed in Html_process_tag */
      html->NameVal = a_Url_decode_hex_str(attrbuf);
      Html_add_anchor(html, html->NameVal);
#if 0
        a_Dw_page_add_anchor(page, attrbuf, html->stack[html->stack_top].style);
        _MSG("Registering ANCHOR: %s\n", attrbuf);
#endif
    }
}

/*
 * <A> close function
 */
static void Html_tag_close_a(mSpiderHtml *html, int TagIdx)
{
    html->InVisitedLink = FALSE;
    Html_pop_tag(html, TagIdx);
}

/*
 * Insert underlined text in the page.
 */
static void Html_tag_open_u(mSpiderHtml *html, char *tag, int tagsize)
{
    DwStyle *style;
    DwStyle style_attrs;

    //Html_push_tag(html, tag, tagsize);

    style = html->stack[html->stack_top].style;
    style_attrs = *style;
    style_attrs.text_decoration |= DW_STYLE_TEXT_DECORATION_UNDERLINE;
    html->stack[html->stack_top].style =
        a_Dw_style_new(&style_attrs, html->dd->bw->main_window);
    a_Dw_style_unref(style);
}

/*
 * Insert strike-through text. Used by <S>, <STRIKE> and <DEL>.
 */
static void Html_tag_open_strike(mSpiderHtml *html, char *tag, int tagsize)
{
    DwStyle *style;
    DwStyle style_attrs;

    //Html_push_tag(html, tag, tagsize);

    style = html->stack[html->stack_top].style;
    style_attrs = *style;
    style_attrs.text_decoration |= DW_STYLE_TEXT_DECORATION_LINE_THROUGH;
    html->stack[html->stack_top].style =
            a_Dw_style_new(&style_attrs, html->dd->bw->main_window);
    a_Dw_style_unref(style);
}

/*
 * Insert underline text. Used by <ins>.
 */
static void Html_tag_open_underline(mSpiderHtml *html, char *tag, int tagsize)
{
    DwStyle *style;
    DwStyle style_attrs;

    //Html_push_tag(html, tag, tagsize);

    style = html->stack[html->stack_top].style;
    style_attrs = *style;
    style_attrs.text_decoration |= DW_STYLE_TEXT_DECORATION_UNDERLINE;
    html->stack[html->stack_top].style =
            a_Dw_style_new(&style_attrs, html->dd->bw->main_window);
    a_Dw_style_unref(style);
}

/*
 * <BLOCKQUOTE>
 */
static void Html_tag_open_blockquote(mSpiderHtml *html, char *tag, int tagsize)
{
    //Html_par_push_tag(html, tag, tagsize);
   a_Dw_page_add_parbreak(DW_PAGE (html->dw), 9,
                          html->stack[(html)->stack_top].style);
    Html_add_indented(html, 40, 40, 9);
}

/*
 * Handle the <UL> tag.
 */
static void Html_tag_open_ul(mSpiderHtml *html, char *tag, int tagsize)
{
    const char *attrbuf;
    DwStyleListStyleType list_style_type;

   a_Dw_page_add_parbreak(DW_PAGE (html->dw), 9,
                          html->stack[(html)->stack_top].style);
    Html_add_indented(html, 40, 0, 9);

    if ((attrbuf = Html_get_attr(html, tag, tagsize, "type"))) {
        /* list_style_type explicitly defined */
        if (g_strncasecmp(attrbuf, "disc", 4) == 0)
            list_style_type = DW_STYLE_LIST_STYLE_TYPE_DISC;
        else if (g_strncasecmp(attrbuf, "circle", 6) == 0)
            list_style_type = DW_STYLE_LIST_STYLE_TYPE_CIRCLE;
        else if (g_strncasecmp(attrbuf, "square", 6) == 0)
            list_style_type = DW_STYLE_LIST_STYLE_TYPE_SQUARE;
        else
            /* invalid value */
            list_style_type = DW_STYLE_LIST_STYLE_TYPE_DISC;
    }
    else {
        if (html->stack[html->stack_top].list_type == HTML_LIST_UNORDERED) {
            /* Nested <UL>'s. */
            /* --EG :: I changed the behavior here : types are cycling instead of
            * being forced to square. It's easier for mixed lists level counting.
            */
            switch (html->stack[html->stack_top].style->list_style_type) {
            case DW_STYLE_LIST_STYLE_TYPE_DISC:
                list_style_type = DW_STYLE_LIST_STYLE_TYPE_CIRCLE;
                break;
            case DW_STYLE_LIST_STYLE_TYPE_CIRCLE:
                list_style_type = DW_STYLE_LIST_STYLE_TYPE_SQUARE;
                break;
            case DW_STYLE_LIST_STYLE_TYPE_SQUARE:
            default: /* this is actually a bug */
                list_style_type = DW_STYLE_LIST_STYLE_TYPE_DISC;
                break;
            }
        }
        else {
            /* Either first <UL>, or a <OL> before. */
            list_style_type = DW_STYLE_LIST_STYLE_TYPE_DISC;
        }
    }

    HTML_SET_TOP_ATTR(html, list_style_type, list_style_type);
    html->stack[html->stack_top].list_type = HTML_LIST_UNORDERED;

    html->stack[html->stack_top].list_number = 0;
    html->stack[html->stack_top].ref_list_item = NULL;
}

/*
 * Handle the <MENU> tag.
 * (Deprecated and almost the same as <UL>)
 */
static void Html_tag_open_menu(mSpiderHtml *html, char *tag, gint tagsize)
{
   DwStyleListStyleType list_style_type = DW_STYLE_LIST_STYLE_TYPE_DISC;

   a_Dw_page_add_parbreak(DW_PAGE (html->dw), 9,
                          html->stack[(html)->stack_top].style);
   Html_add_indented(html, 40, 0, 9);
   HTML_SET_TOP_ATTR(html, list_style_type, list_style_type);
   html->stack[html->stack_top].list_type = HTML_LIST_UNORDERED;
   html->stack[html->stack_top].list_number = 0;
   html->stack[html->stack_top].ref_list_item = NULL;

   if (prefs.show_extra_warnings)
      MSG_HTML("it is strongly recommended using <UL> instead of <MENU>\n");
}

/*
 * Handle the <OL> tag.
 */
static void Html_tag_open_ol(mSpiderHtml *html, char *tag, int tagsize)
{
    const char *attrbuf;
    DwStyleListStyleType list_style_type;

   a_Dw_page_add_parbreak(DW_PAGE (html->dw), 9,
                          html->stack[(html)->stack_top].style);
   // Html_par_push_tag(html, tag, tagsize);
    Html_add_indented(html, 40, 0, 9);

    list_style_type = DW_STYLE_LIST_STYLE_TYPE_DECIMAL;

    if ((attrbuf = Html_get_attr(html, tag, tagsize, "type"))) {
        if (*attrbuf == '1')
            list_style_type = DW_STYLE_LIST_STYLE_TYPE_DECIMAL;
        else if (*attrbuf == 'a')
            list_style_type = DW_STYLE_LIST_STYLE_TYPE_LOWER_ALPHA;
        else if (*attrbuf == 'A')
            list_style_type = DW_STYLE_LIST_STYLE_TYPE_UPPER_ALPHA;
        else if (*attrbuf == 'i')
            list_style_type = DW_STYLE_LIST_STYLE_TYPE_LOWER_ROMAN;
        else if (*attrbuf == 'I')
            list_style_type = DW_STYLE_LIST_STYLE_TYPE_UPPER_ROMAN;
    }

    HTML_SET_TOP_ATTR(html, list_style_type, list_style_type);
    html->stack[html->stack_top].list_type = HTML_LIST_ORDERED;

    if ((attrbuf = Html_get_attr(html, tag, tagsize, "start")))
        html->stack[html->stack_top].list_number = strtol(attrbuf, NULL, 10);
    else
        html->stack[html->stack_top].list_number = 1;
    html->stack[html->stack_top].ref_list_item = NULL;
}

/*
 * Handle the <LI> tag.
 */
static void Html_tag_open_li(mSpiderHtml *html, char *tag, int tagsize)
{
    DwWidget *bullet, *list_item, **ref_list_item;
    char str[64];
    const char *attrbuf;
    int *list_number;

    //Html_cleanup_tag(html, "li>");

    /* This is necessary, because the tag is pushed next. */
    list_number = &html->stack[html->stack_top - 1].list_number;
    ref_list_item = &html->stack[html->stack_top - 1].ref_list_item;

    //Html_push_tag(html, tag, tagsize);
    a_Dw_page_add_parbreak(DW_PAGE (html->dw), 0,
                            html->stack[(html)->stack_top].style);

    switch (html->stack[html->stack_top].list_type) {
    case HTML_LIST_NONE:
        MSG_HTML("<li> outside <ul> or <ol>\n");
        list_item = a_Dw_list_item_new(NULL);
        Html_add_indented_widget(html, list_item, 0, 0, 0 /* or 1 */);
        bullet = a_Dw_bullet_new();
        a_Dw_list_item_init_with_widget(DW_LIST_ITEM(html->dw), bullet,
                                        html->stack[html->stack_top].style);
      break;

    default:
        list_item = a_Dw_list_item_new((DwListItem*)*ref_list_item);
        Html_add_indented_widget(html, list_item, 0, 0, 0 /* or 1 */);
        *ref_list_item = list_item;

        switch (html->stack[html->stack_top].list_type) {
        case HTML_LIST_UNORDERED:
            bullet = a_Dw_bullet_new();
            a_Dw_list_item_init_with_widget(DW_LIST_ITEM(html->dw), bullet,
                                            html->stack[html->stack_top].style);
            break;

        case HTML_LIST_ORDERED:
            if ((attrbuf = Html_get_attr(html, tag, tagsize, "value")))
                *list_number = strtol(attrbuf, NULL, 10);
            a_Dw_style_numtostr
                (*list_number, str, 64,
                    html->stack[html->stack_top].style->list_style_type);
            (*list_number)++;
            a_Dw_list_item_init_with_text(DW_LIST_ITEM (html->dw), g_strdup(str),
                                        html->stack[html->stack_top].style);

        case HTML_LIST_NONE:
            /* for making pedantic compilers happy */
            break;
        }
    }
}

/*
 * <HR>
 */
static void Html_tag_open_hr(mSpiderHtml *html, char *tag, int tagsize)
{
    DwWidget *hruler;
    DwStyle style_attrs;
    char *width_ptr;
    const char *attrbuf;
    guint32 size = 0;

   width_ptr = Html_get_attr_wdef(html, tag, tagsize, "width", "100%");
#if 0
    if ((attrbuf = Html_get_attr(html, tag, tagsize, "width")))
        width_ptr = g_strdup(attrbuf);
    else
        width_ptr = g_strdup("100%");
#endif

    style_attrs = *html->stack[html->stack_top].style;

    if ((attrbuf = Html_get_attr(html, tag, tagsize, "size")))
        size = strtol(attrbuf, NULL, 10);

    if ((attrbuf = Html_get_attr(html, tag, tagsize, "align"))) {
        if (g_strcasecmp (attrbuf, "left") == 0)
            style_attrs.text_align = DW_STYLE_TEXT_ALIGN_LEFT;
        else if (g_strcasecmp (attrbuf, "right") == 0)
            style_attrs.text_align = DW_STYLE_TEXT_ALIGN_RIGHT;
        else if (g_strcasecmp (attrbuf, "center") == 0)
            style_attrs.text_align = DW_STYLE_TEXT_ALIGN_CENTER;
    }

    /* todo: evaluate attribute */
    if (Html_get_attr(html, tag, tagsize, "noshade")) {
        a_Dw_style_box_set_border_style (&style_attrs, DW_STYLE_BORDER_SOLID);
        a_Dw_style_box_set_border_color (&style_attrs,
            a_Dw_style_shaded_color_new (style_attrs.color->color_val, html->dd->bw->main_window));
        if (size < 1)
            size = 1;
    } else {
        a_Dw_style_box_set_border_style (&style_attrs, DW_STYLE_BORDER_INSET);
        a_Dw_style_box_set_border_color (&style_attrs,
            a_Dw_style_shaded_color_new (
                html->stack[html->stack_top].current_bg_color, 
                html->dd->bw->main_window));
        if (size < 2)
            size = 2;
    }

    style_attrs.border_width.top =
        style_attrs.border_width.left = (size + 1) / 2;
    style_attrs.border_width.bottom =
        style_attrs.border_width.right = size / 2;

    a_Dw_page_add_parbreak (DW_PAGE (html->dw), 5,
                            html->stack[(html)->stack_top].style);
    hruler = a_Dw_hruler_new ();
    Html_add_widget(html, hruler, width_ptr, NULL, &style_attrs);
    a_Dw_page_add_parbreak (DW_PAGE (html->dw), 5,
                            html->stack[(html)->stack_top].style);
    g_free(width_ptr);
}

/*
 * <DL>
 */
static void Html_tag_open_dl(mSpiderHtml *html, char *tag, int tagsize)
{
    /* may want to actually do some stuff here. */
   a_Dw_page_add_parbreak(DW_PAGE (html->dw), 9,
                          html->stack[(html)->stack_top].style);
   // Html_par_push_tag(html, tag, tagsize);
}

/*
 * <DT>
 */
static void Html_tag_open_dt(mSpiderHtml *html, char *tag, int tagsize)
{
#if 0
    Html_cleanup_tag(html, "dd>");
    Html_cleanup_tag(html, "dt>");
    Html_par_push_tag(html, tag, tagsize);
#endif
   a_Dw_page_add_parbreak(DW_PAGE (html->dw), 9,
                          html->stack[(html)->stack_top].style);
    Html_set_top_font(html, NULL, 0, 1, 1);
}

/*
 * <DD>
 */
static void Html_tag_open_dd(mSpiderHtml *html, char *tag, int tagsize)
{
#if 0
    Html_cleanup_tag(html, "dd>");
    Html_cleanup_tag(html, "dt>");

    Html_par_push_tag(html, tag, tagsize);
#endif
   a_Dw_page_add_parbreak(DW_PAGE (html->dw), 9,
                          html->stack[(html)->stack_top].style);
    Html_add_indented(html, 40, 40, 9);
}

/*
 * <PRE>
 */
static void Html_tag_open_pre(mSpiderHtml *html, char *tag, int tagsize)
{
   const char * attrbuf;
   a_Dw_page_add_parbreak(DW_PAGE (html->dw), 9,
                          html->stack[(html)->stack_top].style);
    Html_set_top_font(html, prefs.fw_fontname, 0, 0, 0);

    /* Is the placement of this statement right? */
    if ((attrbuf = Html_get_attr(html, tag, tagsize, "width"))) 
        html->PreWidth = strtol(attrbuf, NULL, 10);

    html->stack[html->stack_top].parse_mode = MSPIDER_HTML_PARSE_MODE_PRE;
    HTML_SET_TOP_ATTR (html, white_space, DW_STYLE_WHITE_SPACE_PRE);
    html->pre_column = 0;
    html->PreFirstChar = TRUE;
    html->InFlags |= IN_PRE;
}

/*
 * Custom close for <PRE>
 */
static void Html_tag_close_pre(mSpiderHtml *html, gint TagIdx)
{
   html->InFlags &= ~IN_PRE;
   html->PreWidth = G_MAXINT;
   html->PreCount = 0;

   a_Dw_page_add_parbreak(DW_PAGE (html->dw), 9,
                          html->stack[(html)->stack_top].style);
   Html_pop_tag(html, TagIdx);
}

/*
 * Check whether a tag is in the "excluding" element set for PRE
 * Excl. Set = {IMG, OBJECT, APPLET, BIG, SMALL, SUB, SUP, FONT, BASEFONT}
 */
static gint Html_tag_pre_excludes(gint tag_idx)
{
   char *es_set[] = {"img", "object", "applet", "big", "small", "sub", "sup",
                     "font", "basefont", NULL};
   static gint ei_set[10], i;

   /* initialize array */
   if (!ei_set[0])
      for (i = 0; es_set[i]; ++i)
         ei_set[i] = Html_tag_index(es_set[i]);

   for (i = 0; ei_set[i]; ++i)
      if (tag_idx == ei_set[i])
         return 1;
   return 0;
}

/*
 * Handle <FORM> tag
 */
static void Html_tag_open_form(mSpiderHtml *html, char *tag, int tagsize)
{
    mSpiderUrl *action;
    mSpiderHtmlMethod method;
    mSpiderHtmlEnc enc;
    const char *attrbuf;
#ifdef JS_SUPPORT
    int ret;
    jsobject *jsobj = NULL;
    jsobject *pobj = NULL;
    jsscript_interpreter *interpreter = NULL;
    JSContext *ctx = NULL;
    JSObject *docobj = NULL;
#endif
   a_Dw_page_add_parbreak(DW_PAGE (html->dw), 9,
                          html->stack[(html)->stack_top].style);

    if (html->InFlags & IN_FORM) {
        MSG_HTML("nested forms\n");
#ifndef JS_SUPPORT
        return;
#endif
    }
    html->InFlags |= IN_FORM;

    method = MSPIDER_HTML_METHOD_GET;
    if ((attrbuf = Html_get_attr(html, tag, tagsize, "method"))) {
        if (!g_strcasecmp(attrbuf, "post"))
            method = MSPIDER_HTML_METHOD_POST;
        /* todo: maybe deal with unknown methods? */
    }
    if ((attrbuf = Html_get_attr(html, tag, tagsize, "action")))
        action = Html_url_new(html, attrbuf, NULL, 0, 0, 0, 0);
    else
        action = a_Url_dup(html->linkblock->base_url);
    
    
   if ((attrbuf = Html_get_attr(html, tag, tagsize, "target")))
     a_Url_set_target(action, attrbuf);
   else if (URL_TARGET_(html->linkblock->base_url))
     a_Url_set_target(action, URL_TARGET_(html->linkblock->base_url));

    enc = MSPIDER_HTML_ENC_URLENCODING;
    if ( (attrbuf = Html_get_attr(html, tag, tagsize, "encoding")) ) {
        /* todo: maybe deal with unknown encodings? */
    }

#ifdef JS_SUPPORT
    ret = Html_form_new(html->linkblock, method, action, enc);
    lastformobj = NULL;
    inputid = 0;
    /* vitrual form object */
    pobj = js_getdftdocobj((int)html->dd);
    if ( !pobj ) {
        return;
    }
    interpreter = pobj->jsinterpreter;
    if ( interpreter ) {
        ctx = (JSContext*)interpreter->backend_data;
        docobj = pobj->smobj;
    }

    jsobj = js_objnew(NULL /* form object has no widget */, jsform, pobj,
        html, tag, tagsize);
    if ( jsobj ) {
        /* common */
        jsobj->FormID = (void*)formid++;
        jsobj->pvar1 = (void*)ret;    /* form id */
        pobj->children = g_list_append((GList*)pobj->children, jsobj);
        lastformobj = jsobj;
        
        html->linkblock->forms[ret].jsobj = jsobj;

        /* create form's JSObject */
        if ( ctx ) {
            get_form_object(ctx, docobj, jsobj);
        }
    }
#else    
    Html_form_new(html->linkblock, method, action, enc);
#endif

    a_Url_free(action);
}

static void Html_tag_close_form(mSpiderHtml *html, int TagIdx)
{
    static char *SubmitTag =
        "<input type='submit' value='?Submit?' alt='mspider-generated-button'>";
    mSpiderHtmlForm *form;
    int i;
    
#ifdef JS_SUPPORT
    if ( lastformobj ) {
        lastformobj->InputID = (void*)inputid;
        inputid = 0;
    }
#endif
    if (html->InFlags & IN_FORM) {
        form = &(html->linkblock->forms[html->linkblock->num_forms - 1]);

        /* If we don't have a submit button and the user desires one,
            lets add a custom one */
        if (form->num_submit_buttons == 0) {
            MSG_HTML("FORM lacks a Submit button\n");
            if (prefs.generate_submit) {
                MSG_HTML(" (added a submit button internally)\n");
                Html_tag_open_input(html, SubmitTag, strlen(SubmitTag));
                form->num_submit_buttons = 0;
            }
        }

        /* Make buttons sensitive again */
        for (i = 0; i < form->num_inputs; i++) {
            if (form->inputs[i].type == MSPIDER_HTML_INPUT_SUBMIT ||
                    form->inputs[i].type == MSPIDER_HTML_INPUT_RESET) {
                a_Dw_widget_set_button_sensitive (form->inputs[i].widget, TRUE);
            }
            else if (form->inputs[i].type == MSPIDER_HTML_INPUT_IMAGE ||
                        form->inputs[i].type == MSPIDER_HTML_INPUT_BUTTON_SUBMIT ||
                        form->inputs[i].type == MSPIDER_HTML_INPUT_BUTTON_RESET) {
                a_Dw_button_set_sensitive(DW_BUTTON(form->inputs[i].widget), TRUE);
            }
        }
    }
    html->InFlags &= ~IN_FORM;
    html->InFlags &= ~IN_SELECT;
    html->InFlags &= ~IN_TEXTAREA;
    Html_pop_tag(html, TagIdx);
}

/*
 * Handle <META>
 * We do not support http-equiv=refresh because it's non standard,
 * (the HTML 4.01 SPEC recommends explicitily to avoid it), and it
 * can be easily abused!
 *
 * todo: Note that we're sending HTML while still IN_HEAD. The proper
 * solution is to stack it until IN_BODY, but as currently mspider doesn't
 * check that and starts rendering from the first HTML stream, is OK.
 */

static void Html_tag_open_meta(mSpiderHtml *html, char *tag, int tagsize)
{
    const char *meta_template =
"<table width='100%%'><tr><td bgcolor='#ee0000'>Warning:</td>\n"
" <td bgcolor='#8899aa' width='100%%'>\n"
" This page uses the NON-STANDARD meta refresh tag.<br> The HTML 4.01 SPEC\n"
" (sec 7.4.4) recommends explicitly to avoid it.</td></tr>\n"
" <tr><td bgcolor='#a0a0a0' colspan='2'>The author wanted you to go\n"
" <a href='%s'>here</a>%s</td></tr></table><br>\n";

    const char *equiv, *content;
    char *html_msg, delay_str[64];
    int delay;
    /* only valid inside HEAD */
    if (!(html->InFlags & IN_HEAD)) {
        MSG_HTML("META elements must be inside the HEAD section\n");
        return;
    }

    if ((equiv = Html_get_attr(html, tag, tagsize, "http-equiv")) &&
       !g_strcasecmp(equiv, "refresh") &&
                (content = Html_get_attr(html, tag, tagsize, "content"))) {

            /* Get delay, if present, and make a message with it */
            if ((delay = strtol(content, NULL, 0)))
                snprintf(delay_str, 64, " after %d second%s.",
                        delay, (delay > 1) ? "s" : "");
            else
                sprintf(delay_str, ".");

            /* Skip to anything after "URL=" */
            while (*content && *(content++) != '=');

            html_msg = g_strdup_printf (meta_template, content, delay_str);
            if (html_msg) {
                mSpiderHtmlProcessingState SaveFlags = html->InFlags;

                html->InFlags = IN_BODY;
                html->TagSoup = FALSE;
                Html_write_raw(html, html_msg, strlen(html_msg), 0);
                html->TagSoup = TRUE;
                html->InFlags = SaveFlags;
            }
            g_free(html_msg);
        }
    
}

/*
 * Reset the input widget to the initial value.
 */
void Html_reset_input(mSpiderHtmlInput *input)
{
    int i;
    HWND hwnd;

    if(input->type ==MSPIDER_HTML_INPUT_HIDDEN || input->type == MSPIDER_HTML_INPUT_FILE) /* FIXME */
        return;

    hwnd = ((DwMgWidget*)(input->widget))->window;

    switch (input->type) 
    {
    case MSPIDER_HTML_INPUT_TEXT:
    case MSPIDER_HTML_INPUT_TEXTAREA:
    case MSPIDER_HTML_INPUT_PASSWORD:
        SetWindowText(hwnd, input->init_str); 
        break;
    case MSPIDER_HTML_INPUT_CHECKBOX:
        if(input->init_val)
            SendMessage(hwnd, BM_SETCHECK, BST_CHECKED, 0);
        else
            SendMessage(hwnd, BM_SETCHECK, BST_UNCHECKED, 0);
        break;
    case MSPIDER_HTML_INPUT_RADIO:
        if(input->init_val)
            SendMessage(hwnd, BM_SETCHECK, BST_CHECKED, 0);
        else
            SendMessage(hwnd, BM_SETCHECK, BST_UNCHECKED, 0);
        break;
    case MSPIDER_HTML_INPUT_SELECT:
        if (input->select != NULL) {
            /* this is in reverse order so that, in case more than one was
             * selected, we get the last one, which is consistent with handling
             * of multiple selected options in the layout code. */
            for (i = input->select->num_options - 1; i >= 0; i--) {
                if (input->select->options[i].init_val) {
                    SendMessage(((DwMgWidget*)input->widget)->window,
                            CB_SETCURSEL, i, 0);
                    break;
                }
            }
        }
        break;
    case MSPIDER_HTML_INPUT_SEL_LIST:
        if (!input->select)
            break;
        for (i = 0; i < input->select->num_options; i++) {
            if (input->select->options[i].init_val) 
                SendMessage(((DwMgWidget*)input->widget)->window, LB_SETSEL, 1, (LPARAM)i);
            else 
            {
                if (SendMessage(((DwMgWidget*)input->widget)->window, LB_GETSEL, i, 0))
                    SendMessage(((DwMgWidget*)input->widget)->window, LB_SETSEL, 0, (LPARAM)i);
            }
        }
        break;
    default:
        break;
    }
}


/*
 * Add a new input to the form data structure, setting the initial
 * values.
 */
static int Html_add_input(mSpiderHtmlForm *form,
                           mSpiderHtmlInputType type,
                           DwMgWidget *widget,
                           const char *name,
                           const char *init_str,
                           mSpiderHtmlSelect *select,
                           gboolean init_val)
{
    mSpiderHtmlInput *input;

    _MSG("name=[%s] init_str=[%s] init_val=[%d]\n",
            name, init_str, init_val);
    a_List_add(form->inputs, form->num_inputs, form->num_inputs_max);
    input = &(form->inputs[form->num_inputs]);
    input->type = type;
    input->widget = widget;
    input->name = (name) ? g_strdup(name) : NULL;
    input->init_str = (init_str) ? g_strdup(init_str) : NULL;
    input->select = select;
    input->init_val = init_val;
    Html_reset_input(input);

    /* some stats */
    if (type == MSPIDER_HTML_INPUT_PASSWORD ||
            type == MSPIDER_HTML_INPUT_TEXT ||
            type == MSPIDER_HTML_INPUT_TEXTAREA) {
        form->num_entry_fields++;
    }
    else if (type == MSPIDER_HTML_INPUT_SUBMIT ||
            type == MSPIDER_HTML_INPUT_BUTTON_SUBMIT ||
            type == MSPIDER_HTML_INPUT_IMAGE) {
        form->num_submit_buttons++;
    }
    form->num_inputs++;


    return form->num_inputs-1;
}


/*
 * Given a DwWidget, find the form that contains it.
 * Return value: the form if successful, NULL otherwise.
 */
static mSpiderHtmlForm* Html_find_form(DwWidget *reset, mSpiderHtmlLB *html_lb)
{
    int form_index;
    int input_index;
    mSpiderHtmlForm *form;

    for (form_index = 0; form_index < html_lb->num_forms; form_index++) {
        form = &(html_lb->forms[form_index]);
        for (input_index = 0; input_index < form->num_inputs; input_index++) {
            if (form->inputs[input_index].widget == reset) {
                return form;
            }
        }
    }
    return NULL;
}

/*
 * Reset all inputs in the form containing reset to their initial values.
 * In general, reset is the reset button for the form.
 */
void Html_reset_form(HWND hwnd, int id, int nc, DWORD add_data)
{
   gint j = 0;
   gint form_id;
   mSpiderHtmlForm *form;
   mSpiderHtmlLB* html_lb;
            
   form_id = HIWORD(id) - 1;
   html_lb = (mSpiderHtmlLB*)GetWindowAdditionalData (hwnd);

   form = &html_lb->forms[form_id];

#ifdef JS_SUPPORT
    /* onReset */
    js_onformevent(form->jsobj, 0/*reset*/);
#endif

   for ( j = 0; j < form->num_inputs; j++)
   {
        Html_reset_input(&(form->inputs[j]));
   }
}

/*
 * Urlencode 'val' and append it to 'str'
 * -RL :: According to the RFC 1738, only alphanumerics, the special
 *        characters "$-_.+!*'(),", and reserved characters ";/?:@=&" used
 *        for their *reserved purposes* may be used unencoded within a URL.
 * We'll escape everything but alphanumeric and "-_.*" (as lynx).  --Jcid
 */

static void Html_urlencode_append(GString *str, const char *val)
{
    int i;
    static const char *verbatim = "-_.*";
    static const char *hex = "0123456789ABCDEF";

    if ( val == NULL )
        return;
    for (i = 0; val[i] != '\0'; i++) {
        if (val[i] == ' ') {
            g_string_append_c(str, '+');
        }
        else if (isalnum (val[i]) || strchr(verbatim, val[i])) {
            g_string_append_c(str, val[i]);
        }
        else if (val[i] == '\n') {
            g_string_append(str, "%0D%0A");
        }
        else {
            g_string_append_c(str, '%');
            g_string_append_c(str, hex[(val[i] >> 4) & 15]);
            g_string_append_c(str, hex[val[i] & 15]);
        }
    }
}


/*
 * Append a name-value pair to an existing url.
 * (name and value are urlencoded before appending them)
 */

void
Html_append_input(GString *url, const char *name, const char *value)
{
    if (name != NULL) {
        Html_urlencode_append(url, name);
        g_string_append_c(url, '=');
        Html_urlencode_append(url, value);
        g_string_append_c(url, '&');
   }
}

#if 0
/*
 * Append a image button click position to an existing url.
 */
static void Html_append_clickpos(GString *url, const char *name, int x, int y)
{
    if (name) {
        Html_urlencode_append(url, name);
        g_string_sprintfa(url, ".x=%d&", x);
        Html_urlencode_append(url, name);
        g_string_sprintfa(url, ".y=%d&", y);
    }
    else
        g_string_sprintfa(url, "x=%d&y=%d&", x, y);
}
#endif

/*
 * Submit the form containing the submit input by making a new query URL
 * and sending it with a_Nav_push.
 * (Called by GTK+)
 * click_x and click_y are used only by input images and are set only when
 * called by Html_image_clicked. GTK+ does NOT give these arguments.
 */

void Html_submit_form(mSpiderHtmlLB* html_lb, mSpiderHtmlForm* form, 
                        DwWidget* submit, mSpiderDoc *doc)
{
   gint input_index;
   int i;
   mSpiderHtmlInput *input;
   mSpiderUrl *new_url;
   gchar *url_str, *action_str, *p;
   char *title_str;
   int  title_len;

   mSpiderDoc *name_dd;

   name_dd = NULL;
#ifdef JS_SUPPORT
    /* onSubmit */
    js_onformevent(form->jsobj, 1/*submit*/);
#endif

   if ((form->method == MSPIDER_HTML_METHOD_GET) ||
       (form->method == MSPIDER_HTML_METHOD_POST)) 
   {
      GString *DataStr = g_string_sized_new(4096);

      DEBUG_MSG(3,"Html_submit_form form->action=%s\n",URL_STR_(form->action));

      for (input_index = 0; input_index < form->num_inputs; input_index++) 
      {
         input = &(form->inputs[input_index]);
         switch (input->type) 
         {
         case MSPIDER_HTML_INPUT_TEXT:
         case MSPIDER_HTML_INPUT_PASSWORD:
            title_len = SendMessage(((DwMgWidget*)input->widget)->window, 
                            MSG_GETTEXTLENGTH, 0, 0); 
            title_str = (char*)g_malloc(title_len + 1);
            if (title_str) {
                GetWindowText(((DwMgWidget*)input->widget)->window, 
                            title_str, title_len); 
                Html_append_input(DataStr, input->name, title_str);  
                g_free (title_str);
            }
            break;

         case MSPIDER_HTML_INPUT_CHECKBOX:
         case MSPIDER_HTML_INPUT_RADIO:
            if (SendMessage(((DwMgWidget*)input->widget)->window, 
                            BM_GETCHECK, 0, 0) == BST_CHECKED &&
                        input->name != NULL && input->init_str != NULL) 
            {
               Html_append_input(DataStr, input->name, input->init_str);
            }
            break;

         case MSPIDER_HTML_INPUT_HIDDEN:
            Html_append_input(DataStr, input->name, input->init_str);
            break;

         case MSPIDER_HTML_INPUT_SELECT:
            i =  SendMessage(((DwMgWidget*)input->widget)->window, CB_GETCURSEL, 0, 0);    
            if ( i >= 0 ) {
                Html_append_input(DataStr, input->name, input->select->options[i].value);
            }
            break;

         case MSPIDER_HTML_INPUT_SEL_LIST:
            for (i = 0; i < input->select->num_options; i++) 
            {
                if (SendMessage(((DwMgWidget*)input->widget)->window, 
                                        LB_GETSEL, i, 0))
                  Html_append_input(DataStr, input->name,
                                    input->select->options[i].value);
            }
            break;

         case MSPIDER_HTML_INPUT_TEXTAREA:
            title_len = SendMessage(((DwMgWidget*)input->widget)->window, 
                            MSG_GETTEXTLENGTH, 0, 0); 
            title_str = (char*)g_malloc(title_len + 1);
            if (title_str) {
                GetWindowText(((DwMgWidget*)input->widget)->window, 
                                title_str, title_len); 
                Html_append_input(DataStr, input->name, title_str);
                g_free (title_str);
            }
            break;
         case MSPIDER_HTML_INPUT_INDEX:
            //Html_urlencode_append(DataStr,title_str);
            break;
         case MSPIDER_HTML_INPUT_IMAGE:
            if (input->widget == submit) {
               Html_append_input(DataStr, input->name, input->init_str);
               //Html_append_clickpos(DataStr, input->name, click_x, click_y);
            }
            break;
         case MSPIDER_HTML_INPUT_SUBMIT:
         case MSPIDER_HTML_INPUT_BUTTON_SUBMIT:
            /* Only the button that triggered the submit. */
            if ( submit ) {
                if ( ((DwMgWidget*)(input->widget))->window ==
                        ((DwMgWidget*)submit)->window && form->num_submit_buttons > 0 )
                   Html_append_input(DataStr, input->name, input->init_str);
            }
            break;
         default:
            break;
         } /* switch */
      } /* for (inputs) */
      if ( DataStr->str[DataStr->len - 1] == '&' )
         g_string_truncate(DataStr, DataStr->len - 1);

      /* form->action was previously resolved against base URL */
      action_str = g_strdup(URL_STR(form->action));
      if (form->method == MSPIDER_HTML_METHOD_POST) {
         new_url = a_Url_new(action_str, NULL, 0, 0, 0);
         a_Url_set_data(new_url, DataStr->str);
         a_Url_set_flags(new_url, URL_FLAGS(new_url) | URL_Post);
      } 
      else {
         /* remove <fragment> and <query> sections if present */
         if ((p = strchr(action_str, '#')))
            *p = 0;
         if ((p = strchr(action_str, '?')))
            *p = 0;

         url_str = g_strconcat(action_str, "?", DataStr->str, NULL);
         new_url = a_Url_new(url_str, NULL, 0, 0, 0);
         a_Url_set_flags(new_url, URL_FLAGS(new_url) | URL_Get);
         g_free(url_str);
      }

      a_Url_set_referer(new_url, html_lb->base_url);
        
      if ( doc ) {
            a_Nav_push(doc, new_url);
      } else {
          if (URL_TARGET_(form->action)) {
             a_Url_set_target(new_url, (gchar *) URL_TARGET_(form->action));
             name_dd = a_Doc_get_by_name(html_lb->dd, (gchar *) URL_TARGET_(new_url));
          }

          if (name_dd)
            a_Nav_push(name_dd, new_url);
          else
              a_Nav_push(html_lb->dd, new_url);
      }
      g_free(action_str);

      g_string_free(DataStr, TRUE);
      a_Url_free(new_url);
    } 
    else {
      //printf("Html_submit_form: Method unknown\n");
   }

   /* now, make the rendered area have its focus back */
    // gtk_widget_grab_focus(GTK_BIN(html_lb->bw->docwin)->child);
}


/*
 * Call submit form, when input image has been clicked
 */

static void Html_image_clicked(DwWidget *widget, int x, int y,
                               mSpiderHtmlLB *html_lb)
{
    mSpiderHtmlForm* form;
    form = Html_find_form(widget, html_lb);
    if (form != NULL)
        Html_submit_form(html_lb, form, widget, NULL);
}

/*
 * Create input image for the form
 */
static DwWidget *Html_input_image(mSpiderHtml *html, char *tag, int tagsize,
                                  mSpiderHtmlLB *html_lb, mSpiderUrl *action)
{
    mSpiderImage *Image;
    DwWidget *button;
    mSpiderUrl *url = NULL;
    DwStyle style_attrs;
    const char *attrbuf;

    if ((attrbuf = Html_get_attr(html, tag, tagsize, "src")) &&
            (url = Html_url_new(html, attrbuf, NULL, 0, 0, 0, 0))) {
        button = a_Dw_button_new (0, FALSE);

        a_Dw_page_add_widget (DW_PAGE (html->dw), button,
                            html->stack[html->stack_top].style);

        g_signal_connect(G_OBJECT(button), "clicked_at",
                         G_CALLBACK(Html_image_clicked), (gpointer)html_lb);

        a_Dw_button_set_sensitive(DW_BUTTON(button), FALSE);

        /* create new image and add it to the button */
        if ((Image = Html_add_new_image(html, tag, tagsize, &style_attrs,
                                      FALSE))) 
        {
            /* By suppressing the "image_pressed" signal, the events are sent
             * to the parent DwButton */
            a_Dw_widget_set_button_sensitive (DW_WIDGET (Image->dw), FALSE);
            a_Dw_widget_set_style(DW_WIDGET(Image->dw),
                                html->stack[html->stack_top].style);
            a_Dw_container_add(DW_CONTAINER(button), DW_WIDGET(Image->dw));
            // TODO: a_Dw_widget_set_cursor(DW_WIDGET(Image->dw), Dw_cursor_hand);
            Html_load_image(html, url, Image);
            a_Url_free(url);
            return button;
        }
    }

    DEBUG_MSG(10, "Html_input_image: unable to create image submit.\n");
    a_Url_free(url);
    return NULL;
}

/*
 * Add a new input to current form
 */
static void Html_tag_open_input(mSpiderHtml *html, char *tag, int tagsize)
{
    int inpid;
    mSpiderHtmlForm *form;
    mSpiderHtmlLB *html_lb;
    DwWidget *widget = NULL;
    mSpiderHtmlInputType inp_type = MSPIDER_HTML_INPUT_TEXT;
    char *value, *name, *type, *init_str,*id;
    const char *attrbuf, *label;
    char *disable;
    gboolean init_val = FALSE;
#ifdef JS_SUPPORT
    int input_type; 
    jsobject *jsobj = NULL;
    jsobject *pobj = NULL;
    jsobject *tmpobj = NULL;
    jsscript_interpreter *interpreter = NULL;
    JSContext *ctx = NULL;
    JSObject *formobj = NULL;
#endif

    int mgwidget_id = 0;
    int input_index;
    int create_group = 1;

    DWORD entry_styles = WS_NONE;
    int entry_size = 10;
    int entry_maxlen = -1;

    if (!(html->InFlags & IN_FORM)) {
        MSG_HTML("input camp outside <form>\n");
#ifndef JS_SUPPORT
        return;
#endif
    }

    html_lb = html->linkblock;
#ifdef JS_SUPPORT
    if ( html_lb->num_forms == 0) {
        /* if this page have not a form them create a virtual one */
        Html_tag_open_form(html, vform, strlen(vform));
    }
    if ( html_lb->num_forms == 0) {
        return;
    }
#endif
    
    form = &(html_lb->forms[html_lb->num_forms - 1]);
#ifdef JS_SUPPORT
    if ( !form ) {
        return;
    }
#endif

    mgwidget_id = (html_lb->num_forms << 16) | (form->num_inputs);

    /* Get 'value', 'name' and 'type' */
    value = Html_get_attr_wdef(html, tag, tagsize, "value", NULL);
    name = Html_get_attr_wdef(html, tag, tagsize, "name", NULL);
    type = Html_get_attr_wdef(html, tag, tagsize, "type", "");
    id = Html_get_attr_wdef(html, tag, tagsize, "id", NULL);
    disable = Html_get_attr_wdef(html, tag, tagsize, "disabled", "false");

    /* Readonly flag of an entry */
    entry_styles |= Html_get_attr(html, tag, tagsize, "readonly")?ES_READONLY:0;

    /* Width of an entry */
    if ((attrbuf = Html_get_attr(html, tag, tagsize, "size")))
        entry_size = strtol(attrbuf, NULL, 10);

    /* Maximum length of the text in an entry */
    if ((attrbuf = Html_get_attr(html, tag, tagsize, "maxlength")))
        entry_maxlen = strtol(attrbuf, NULL, 10);

   
    init_str = NULL;
    if (!g_strcasecmp(type, "password")) 
    {

 
        inp_type = MSPIDER_HTML_INPUT_PASSWORD;
        widget = a_Dw_MgWidget_entry_new(html, mgwidget_id, value, 
                                        entry_size, entry_maxlen,
                                           entry_styles | ES_PASSWORD, (DWORD)html_lb);        
        if (value)
            init_str = g_strdup(Html_get_attr(html, tag, tagsize, "value"));
    }
    else if (!g_strcasecmp(type, "checkbox")) {
        inp_type = MSPIDER_HTML_INPUT_CHECKBOX;

        init_val = (Html_get_attr(html, tag, tagsize, "checked") != NULL);

        widget = a_Dw_MgWidget_check_button__new (html, mgwidget_id,
                                     "", (DWORD)html_lb, init_val);
        init_str = (value) ? value : g_strdup("on");
    }
    else if (!g_strcasecmp(type, "radio")) {
        inp_type = MSPIDER_HTML_INPUT_RADIO;

        for (input_index = 0; input_index < form->num_inputs; input_index++) {
            if (form->inputs[input_index].type == MSPIDER_HTML_INPUT_RADIO &&
                    (form->inputs[input_index].name &&
                    !g_strcasecmp(form->inputs[input_index].name, name)) ){
                create_group = 0;
                break;
            }
        }
       init_val = (Html_get_attr(html, tag, tagsize, "checked") != NULL);

       widget = a_Dw_MgWidget_radio_button_new (html, mgwidget_id,
                                   "", (DWORD)html_lb, create_group, init_val);

       init_str = (value) ? value : NULL;
    }
    else if (!g_strcasecmp(type, "hidden")) {
        inp_type = MSPIDER_HTML_INPUT_HIDDEN;
        if (value)
            init_str = g_strdup(Html_get_attr(html, tag, tagsize, "value"));
    }
    else if (!g_strcasecmp(type, "submit")) {
        inp_type = MSPIDER_HTML_INPUT_SUBMIT;
        init_str = (value) ? value : g_strdup("submit");
        widget = a_Dw_MgWidget_button_new (html, mgwidget_id, init_str, (DWORD)html_lb);
        form->hwnd_submit = ((DwMgWidget*)widget)->window;
        a_Dw_MgWidget_set_notification (((DwMgWidget*)widget)->window, inp_type);
    }
    else if (!g_strcasecmp(type, "reset")) {
        inp_type = MSPIDER_HTML_INPUT_RESET;
        init_str = (value) ? value : g_strdup("Reset");
        widget = a_Dw_MgWidget_button_new(html, mgwidget_id, init_str, (DWORD)html_lb);
        a_Dw_MgWidget_set_notification (((DwMgWidget*)widget)->window, inp_type);
    }
    else if (!g_strcasecmp(type, "image")) {
        if (URL_FLAGS(html->linkblock->base_url) & URL_SpamSafe) {
            /* Don't request the image, make a text submit button instead */
            inp_type = MSPIDER_HTML_INPUT_SUBMIT;
            attrbuf = Html_get_attr(html, tag, tagsize, "alt");
            label = attrbuf ? attrbuf : value ? value : name ? name : "Submit";
            init_str = g_strdup(label);
            widget = a_Dw_MgWidget_button_new (html, mgwidget_id, init_str, (DWORD)html_lb);
            a_Dw_MgWidget_set_notification (((DwMgWidget*)widget)->window, inp_type);
        }
        else {
            inp_type = MSPIDER_HTML_INPUT_IMAGE;
            /* use a dw_image widget */
            widget = (DwWidget*) Html_input_image(html, tag, tagsize,
                                                html_lb, form->action);
            init_str = value;
        }
    }
    else if (!g_strcasecmp(type, "file")) {
        /* todo: implement it! */
        inp_type = MSPIDER_HTML_INPUT_FILE;
        init_str = (value) ? value : NULL;
        MSPIDER_MSG("An input of the type \"file\" wasn't rendered!\n");
    }
    else if (!g_strcasecmp(type, "button")) 
    {
        inp_type = MSPIDER_HTML_INPUT_BUTTON;
        if ( !value ) {
            value = g_strdup(" ");    /* default button value */
        }
        init_str = value;
        widget = a_Dw_MgWidget_button_new (html, mgwidget_id, init_str, (DWORD)html_lb);
    }
    else 
    {
        /* Text input, which also is the default */

        inp_type = MSPIDER_HTML_INPUT_TEXT;
        init_str = (value) ? value : NULL;
        widget = a_Dw_MgWidget_entry_new (html, mgwidget_id, init_str, 
                        entry_size, entry_maxlen,
                           entry_styles, (DWORD)html_lb);        
    }

    if (widget) {
        if (g_strcasecmp (disable, "true") == 0)
            EnableWindow (((DwMgWidget*)widget)->window, FALSE);
        else
            EnableWindow (((DwMgWidget*)widget)->window, TRUE);
    }
#ifdef JS_SUPPORT
    input_type = jstnone;
    switch ( inp_type ) {
    case MSPIDER_HTML_INPUT_TEXT:
        input_type = jstext;
        break;
    case MSPIDER_HTML_INPUT_PASSWORD:
        input_type = jspassword;
        break;
    case MSPIDER_HTML_INPUT_CHECKBOX:
        input_type = jscheckbox;
        break;
    case MSPIDER_HTML_INPUT_RADIO:
        input_type=jsradio;
        break;
    case MSPIDER_HTML_INPUT_IMAGE:
        break;
    case MSPIDER_HTML_INPUT_FILE:
        break;
    case MSPIDER_HTML_INPUT_BUTTON:
        input_type = jsbutton;
        break;
    case MSPIDER_HTML_INPUT_HIDDEN:
        input_type = jshidden;
        break;
    case MSPIDER_HTML_INPUT_SUBMIT:
        input_type = jssubmit;
        break;
    case MSPIDER_HTML_INPUT_RESET:
        input_type = jsreset;
        break;
    case MSPIDER_HTML_INPUT_BUTTON_SUBMIT:
        break;
    case MSPIDER_HTML_INPUT_BUTTON_RESET:
        break;
    case MSPIDER_HTML_INPUT_SELECT:
        break;
    case MSPIDER_HTML_INPUT_SEL_LIST:
        break;
    case MSPIDER_HTML_INPUT_TEXTAREA:
        break;
    case MSPIDER_HTML_INPUT_INDEX:
        break;
    default:
        break;
    }

    if ( input_type != jstnone ) {
        pobj = lastformobj;
        if ( pobj ) {
            jsobj = js_objnew(widget, input_type, pobj, html, tag, tagsize);
            if ( jsobj ) {
                /* comon */
                if ( pobj ) {
                    jsobj->InputID = (void*)inputid++;
                    pobj->children = 
                        g_list_append(pobj->children, jsobj);
                }

                /* create input object's JSObject */
                tmpobj = jsobj;
                while ( tmpobj && (tmpobj->jstype != jsdocument ) )    {
                    tmpobj = (jsobject*)tmpobj->jsparent;
                }
                if ( tmpobj ) {
                    interpreter = tmpobj->jsinterpreter;
                    if ( interpreter ) {
                        ctx = (JSContext*)interpreter->backend_data;
                        formobj = pobj->smobj;
                        getinputsmobj(ctx, formobj, jsobj);
                    }
                }

                if ( widget ) {
                    widget->jsobj = jsobj;
                    jsobj->oldproc = SetNotificationCallback(((DwMgWidget*)widget)->window, 
                        js_eventprocess);
                }
            }
        }
    }
#endif

    inpid = Html_add_input(form, inp_type, (DwMgWidget*)widget, name,
                    (init_str) ? init_str : "", NULL, init_val);

#ifdef JS_SUPPORT
    switch ( input_type ) {
    case jshidden:
        jsobj->pvar1 = (void*)inpid;
        break;
    case jspassword:
        jsobj->pvar1 = DW_PAGE (html->dw);
        break;
    case jstext:
        jsobj->pvar1 = DW_PAGE (html->dw);
        break;
    case jsbutton:
        jsobj->pvar1 = DW_PAGE (html->dw);
        break;


    default:
        break;
    }
#endif

    if(widget != NULL && inp_type != MSPIDER_HTML_INPUT_IMAGE) {
        if (inp_type == MSPIDER_HTML_INPUT_TEXT ||
            inp_type == MSPIDER_HTML_INPUT_PASSWORD) {
        }
        a_Dw_page_add_widget(DW_PAGE (html->dw), (DwWidget*)widget,
                            html->stack[html->stack_top].style);
    }

    g_free(type);
    g_free(id);
    g_free(name);
    if (init_str != value)
        g_free(init_str);
    g_free(value);
    g_free(disable);
}

/*
 * The ISINDEX tag is just a deprecated form of <INPUT type=text> with
 * implied FORM, afaics.
 */
static void Html_tag_open_isindex(mSpiderHtml *html, char *tag, int tagsize)
{
    mSpiderHtmlForm *form;
    mSpiderHtmlLB *html_lb;
    mSpiderUrl *action;
    const char *attrbuf;

    html_lb = html->linkblock;

    if ((attrbuf = Html_get_attr(html, tag, tagsize, "action")))
        action = Html_url_new(html, attrbuf, NULL, 0, 0, 0, 0);
    else
        action = a_Url_dup(html->linkblock->base_url);

    Html_form_new(html->linkblock, MSPIDER_HTML_METHOD_GET, action,
                    MSPIDER_HTML_ENC_URLENCODING);

    form = &(html_lb->forms[html_lb->num_forms - 1]);

    a_Dw_page_add_parbreak(DW_PAGE (html->dw), 9,
                            html->stack[(html)->stack_top].style);

    if ((attrbuf = Html_get_attr(html, tag, tagsize, "prompt")))
        a_Dw_page_add_text(DW_PAGE (html->dw), g_strdup(attrbuf),
                         html->stack[html->stack_top].style);

    a_Url_free(action);
}

/*
 * Close  textarea
 * (TEXTAREA is parsed in VERBATIM mode, and entities are handled here)
 */
static void Html_tag_close_textarea(mSpiderHtml *html, int TagIdx)
{
    mSpiderHtmlLB *html_lb = html->linkblock;
    char *str;
    mSpiderHtmlForm *form;
    int i;

    if ((html->InFlags & IN_FORM) &&
         (html->InFlags & IN_TEXTAREA))
    {

    /* Remove the line ending that follows the opening tag */
    if (html->Stash->str[0] == '\r')
        html->Stash = g_string_erase(html->Stash, 0, 1);
    if (html->Stash->str[0] == '\n')
        html->Stash = g_string_erase(html->Stash, 0, 1);

    /* As the spec recommends to canonicalize line endings, it is safe
     * to replace '\r' with '\n'. It will be canonicalized anyway! */
    for (i = 0; i < html->Stash->len; ++i) {
        if (html->Stash->str[i] == '\r') {
            if (html->Stash->str[i + 1] == '\n')
                g_string_erase(html->Stash, i, 1);
            else
                html->Stash->str[i] = '\n';
        }
    }

    /* The HTML3.2 spec says it can have "text and character entities". */
        str = Html_parse_entities(html, html->Stash->str, html->Stash->len);

        form = &(html_lb->forms[html_lb->num_forms - 1]);
        form->inputs[form->num_inputs - 1].init_str = str;
        SendMessage(((DwMgWidget*)(form->inputs[form->num_inputs -1].widget))->window, MSG_SETTEXT, 0, (LPARAM)str);

        html->InFlags &= ~IN_TEXTAREA;
    }
    Html_pop_tag(html, TagIdx);
}

/*
 * The textarea tag
 * (todo: It doesn't support wrapping).
 */
static void Html_tag_open_textarea(mSpiderHtml *html, char *tag, int tagsize)
{
    mSpiderHtmlLB *html_lb;
    mSpiderHtmlForm *form;
    char *name;
    char *value;
    char *disable;
    char *init_str = NULL;
    const char *attrbuf;
    DwWidget *widget = NULL;
    int mgwidget_id = 0;
    int cols, rows;
#ifdef JS_SUPPORT
    jsobject *pobj = NULL;
    jsobject *jsobj = NULL;
    jsobject *tmpobj = NULL;
    jsscript_interpreter *interpreter= NULL;
    JSContext *ctx = NULL;
    JSObject *formobj = NULL;

    DwPage *page;
    page = DW_PAGE (html->dw);
#endif

    if (!(html->InFlags & IN_FORM)) {
        MSG_HTML("<textarea> outside <form>\n");
#ifndef JS_SUPPORT
        return;
#endif
    }
    if (html->InFlags & IN_TEXTAREA) {
        MSG_HTML("nested <textarea>\n");
        return;
    }

    html->InFlags |= IN_TEXTAREA;

    html_lb = html->linkblock;
#ifdef JS_SUPPORT
    if ( html_lb->num_forms == 0) {
        /* if this page have not a form them create a virtual one */
        Html_tag_open_form(html, vform, strlen(vform));
    }
    if ( html_lb->num_forms == 0) {
        return;
    }
#endif

    form = &(html_lb->forms[html_lb->num_forms - 1]);

    mgwidget_id = (html_lb->num_forms << 16) | (form->num_inputs);
    //Html_push_tag(html, tag, tagsize);
    Html_stash_init(html);
    html->stack[html->stack_top].parse_mode = MSPIDER_HTML_PARSE_MODE_VERBATIM;

    value = Html_get_attr_wdef(html, tag, tagsize, "value", NULL);
    disable = Html_get_attr_wdef(html, tag, tagsize, "disabled", "false");

    cols = 10;
    if ((attrbuf = Html_get_attr(html, tag, tagsize, "cols")))
        cols = strtol(attrbuf, NULL, 10);
    rows = 10;
    if ((attrbuf = Html_get_attr(html, tag, tagsize, "rows")))
        rows = strtol(attrbuf, NULL, 10);

    name = NULL;
    if ((attrbuf = Html_get_attr(html, tag, tagsize, "name")))
        name = g_strdup(attrbuf);

    widget = a_Dw_MgWidget_textarea_new (html, mgwidget_id, "", rows,
                                      cols, -1, (DWORD)html_lb);

    if (g_strcasecmp (disable, "true") == 0)
        EnableWindow (((DwMgWidget*)widget)->window, FALSE);
    else
        EnableWindow (((DwMgWidget*)widget)->window, TRUE);

    /* If the attribute readonly isn't specified we make the textarea
     * editable. If readonly is set we don't have to do anything.
     */
    if (Html_get_attr(html, tag, tagsize, "readonly"))
        SendMessage(((DwMgWidget*)widget)->window, EM_SETREADONLY, TRUE, 0);

    if (value){
        init_str = g_strdup(Html_get_attr(html, tag, tagsize, "value"));
        g_free(value);
    }

    Html_add_input(form, MSPIDER_HTML_INPUT_TEXTAREA,
                    (DwMgWidget*)widget, name, (init_str) ? init_str : "", NULL, FALSE);

#ifdef JS_SUPPORT
    pobj = lastformobj;
    if ( pobj ) {
        jsobj = js_objnew(widget, jstextarea, pobj, html, tag, tagsize);
        if ( jsobj ) {
            /* common */
            if (pobj){
                jsobj->InputID = (void*)inputid++;
                jsobj->pvar1 = page;
                pobj->children = 
                    g_list_append(pobj->children, jsobj);
            }

            /* create input object's JSObject */
            tmpobj = jsobj;
            while ( tmpobj && (tmpobj->jstype != jsdocument ) )    {
                tmpobj = (jsobject*)tmpobj->jsparent;
            }
            if ( tmpobj ) {
                interpreter = tmpobj->jsinterpreter;
                if ( interpreter ) {
                    ctx = (JSContext*)interpreter->backend_data;
                    formobj = pobj->smobj;
                    getinputsmobj(ctx, formobj, jsobj);
                }
            }

            /* event */
            if (widget) {
                widget->jsobj = jsobj;
                jsobj->oldproc = SetNotificationCallback(((DwMgWidget*)widget)->window, 
                js_eventprocess);
            }
        }
    }
#endif
    g_free(name);
    g_free(disable);
    if (init_str)
        g_free(init_str);

    a_Dw_page_add_widget(DW_PAGE (html->dw), widget,
                        html->stack[html->stack_top].style);
}

/*
 * ?
 */
static void Html_option_finish(mSpiderHtml *html)
{
    mSpiderHtmlForm *form;
    mSpiderHtmlInput *input;
    mSpiderHtmlSelect *select;
    char *str;
    int i;
#if JS_SUPPORT
    jsobject *jsobj = NULL;
    jsscript_interpreter *interpreter = NULL;
    jsobj = js_getdftdocobj((int)html->dd);
    interpreter = (jsscript_interpreter*)jsobj->jsinterpreter;
#endif

    if (html->Stash->str[0] == '\r')
        html->Stash = g_string_erase(html->Stash, 0, 1);
    if (html->Stash->str[0] == '\n')
        html->Stash = g_string_erase(html->Stash, 0, 1);

    /* As the spec recommends to canonicalize line endings, it is safe
     * to replace '\r' with '\n'. It will be canonicalized anyway! */
    for (i = 0; i < html->Stash->len; ++i) 
    {
        if (html->Stash->str[i] == '\r') 
        {
            if (html->Stash->str[i + 1] == '\n')
                g_string_erase(html->Stash, i, 1);
            else
                html->Stash->str[i] = '\n';
        }
    }

    /* The HTML3.2 spec says it can have "text and character entities". */
   str = Html_parse_entities(html, html->Stash->str, html->Stash->len);
  
   /* we need not check this flag, because the form have value */
   /* before calling Html_option_finish, form has value */
#if 0
    if (!(html->InFlags & IN_FORM))
        return;
#endif

    form = &(html->linkblock->forms[html->linkblock->num_forms - 1]);
    input = &(form->inputs[form->num_inputs - 1]);
    if ( !(input->select) || input->select->num_options <= 0)
        return;

    select = input->select;

    if (select->options[select->num_options - 1].value == NULL )
    {
        select->options[select->num_options - 1].value =
            g_strdup(html->Stash->str);
    }
    select->options[select->num_options - 1].menuitem = str;
}

/*
 * <SELECT>
 */
/* The select tag is quite tricky, because of gorpy html syntax. */
static void Html_tag_open_select(mSpiderHtml *html, char *tag, int tagsize)
{
    mSpiderHtmlForm *form;
    mSpiderHtmlLB *html_lb;
    mSpiderHtmlSelect *Select;
    DwWidget *widget = NULL;
    mSpiderHtmlInputType type;
    int mgwidget_id;
    char *name;
    char *disable;
    const char *attrbuf;
    int size, multi;
    int ret;
#ifdef JS_SUPPORT
    jsobject *pobj = NULL;
    jsobject *jsobj = NULL;
    jsobject *tmpobj = NULL;
    jsscript_interpreter *interpreter= NULL;
    JSContext *ctx = NULL;
    JSObject *formobj = NULL;
#endif

    if (!(html->InFlags & IN_FORM)) {
        MSG_HTML("<select> outside <form>\n");
#ifndef JS_SUPPORT
        return;
#endif
    }
    if (html->InFlags & IN_SELECT) {
        MSG_HTML("nested <select>\n");
        return;
    }

    html->InFlags |= IN_SELECT;
    html_lb = html->linkblock;
#ifdef JS_SUPPORT
    if ( html_lb->num_forms == 0) {
        /* if this page have not a form them create a virtual one */
        Html_tag_open_form(html, vform, strlen(vform));
    }
    if ( html_lb->num_forms == 0) {
        return;
    }
#endif
    form = &(html_lb->forms[html_lb->num_forms - 1]);

    mgwidget_id = (html_lb->num_forms << 16) | (form->num_inputs);

    name = NULL;
    if ((attrbuf = Html_get_attr(html, tag, tagsize, "name")))
        name = g_strdup(attrbuf);

    disable = Html_get_attr_wdef(html, tag, tagsize, "disabled", "false");
    size = 0;
    if ((attrbuf = Html_get_attr(html, tag, tagsize, "size")))
        size = strtol(attrbuf, NULL, 10);



    if ((attrbuf = Html_get_attr(html, tag, tagsize, "multiple"))){
        if (g_strcasecmp (attrbuf, "true") == 0)
            multi = 1;
        else
            multi = 0;
    }else
        multi = 0;
    
    if (size < 1)
        size = multi ? 10 : 1;

    if (size == 1) 
    {
         widget = a_Dw_MgWidget_combobox_new (html, mgwidget_id, NULL, (DWORD)html_lb);
         type = MSPIDER_HTML_INPUT_SELECT;
    } else {
        widget = a_Dw_MgWidget_listbox_new (html, mgwidget_id, size, NULL, (DWORD)html_lb, multi);
        type = MSPIDER_HTML_INPUT_SEL_LIST;
    }

    if (g_strcasecmp (disable, "true") == 0)
        EnableWindow (((DwMgWidget*)widget)->window, FALSE);
    else
        EnableWindow (((DwMgWidget*)widget)->window, TRUE);

    Select = g_new(mSpiderHtmlSelect, 1);
    Select->menu = NULL;
    Select->size = size;
    Select->num_options = 0;
    Select->num_options_max = 8;
    Select->options = g_new0 (mSpiderHtmlOption, Select->num_options_max);

    ret = Html_add_input(form, type, (DwMgWidget*)widget, name, NULL, Select, FALSE);
#ifdef JS_SUPPORT
    pobj = lastformobj;
    if ( pobj ) {
        lastselobj = NULL;
        jsobj = js_objnew(widget, jsselect, pobj, html, tag, tagsize);
        if ( jsobj ) {
            /* common */
            jsobj->pvar1 = (void*)0;    /* opt num */
            jsobj->pvar2 = (void*)ret;    /* input id */
            jsobj->pvar3 = (void*)type;    /* input type */
            pobj->children = 
                g_list_append(pobj->children, jsobj);
            widget->jsobj = jsobj;
            lastselobj = jsobj;

            /* create input object's JSObject */
            tmpobj = jsobj;
            while ( tmpobj && (tmpobj->jstype != jsdocument ) )    {
                tmpobj = (jsobject*)tmpobj->jsparent;
            }
            if ( tmpobj ) {
                interpreter = tmpobj->jsinterpreter;
                if ( interpreter ) {
                    ctx = (JSContext*)interpreter->backend_data;
                    formobj = pobj->smobj;
                    getinputsmobj(ctx, formobj, jsobj);
                }
            }

            /* event */
            jsobj->oldproc = SetNotificationCallback(((DwMgWidget*)widget)->window, 
                js_eventprocess);
        }
    }
#endif
    Html_stash_init(html);

    g_free(name);
    g_free(disable);
}

/*
 * ?
 */
static void Html_tag_close_select(mSpiderHtml *html, int TagIdx)
{
    mSpiderHtmlForm *form;
    mSpiderHtmlInput *input;
    mSpiderHtmlLB *html_lb;
    int i;


    if (html->InFlags & IN_SELECT) {
        html->InFlags &= ~IN_SELECT;
        html_lb = html->linkblock;

        form = &(html_lb->forms[html_lb->num_forms - 1]);
        input = &(form->inputs[form->num_inputs - 1]);
        Html_option_finish(html);

        if (input->type == MSPIDER_HTML_INPUT_SELECT)
        {
            for (i = 0; i < input->select->num_options; ++i)
            {
                if(input->select->options[i].init_val)
                    a_Dw_MgWidget_combobox_add_item
                        ((DwMgWidget*)(input->widget), i, input->select->options[i].menuitem);
                else
                    a_Dw_MgWidget_combobox_add_item
                      ((DwMgWidget*)(input->widget), -1, input->select->options[i].menuitem);
            }
            SetWindowText(((DwMgWidget*)(input->widget))->window, input->select->options[0].menuitem);
            a_Dw_page_add_widget(DW_PAGE (html->dw), input->widget,
                                html->stack[html->stack_top].style);
        }
        else if (input->type == MSPIDER_HTML_INPUT_SEL_LIST)
        {
            for (i = 0; i < input->select->num_options; ++i)
            {
                if(input->select->options[i].init_val)
                    a_Dw_MgWidget_listbox_add_item
                        ((DwMgWidget*)(input->widget), i, input->select->options[i].menuitem);
                else
                    a_Dw_MgWidget_listbox_add_item
                        ((DwMgWidget*)(input->widget), -1, input->select->options[i].menuitem);
            }
            SetWindowText(((DwMgWidget*)(input->widget))->window, input->select->options[0].menuitem);
            a_Dw_page_add_widget(DW_PAGE (html->dw), input->widget,
                                html->stack[html->stack_top].style);
        }
    }
    Html_pop_tag(html, TagIdx);
}

/*
 * ?
 */
static void Html_tag_open_option(mSpiderHtml *html, char *tag, int tagsize)
{
    mSpiderHtmlForm *form;
    mSpiderHtmlInput *input;
    mSpiderHtmlLB *html_lb;
    int no;
#ifdef JS_SUPPORT
    jsobject *jsobj = NULL;        
    jsobject *pobj = NULL;
#endif

    if (!(html->InFlags & IN_SELECT))
        return;

    html_lb = html->linkblock;

    form = &(html_lb->forms[html_lb->num_forms - 1]);
    input = &(form->inputs[form->num_inputs - 1]);
    if (input->type == MSPIDER_HTML_INPUT_SELECT ||
        input->type == MSPIDER_HTML_INPUT_SEL_LIST) {
        Html_option_finish(html);
        no = input->select->num_options;
        a_List_add(input->select->options, no, input->select->num_options_max);
        input->select->options[no].menuitem = NULL;
        input->select->options[no].value = Html_get_attr_wdef(html, tag, tagsize,
                                                               "value", NULL);
        input->select->options[no].init_val =
            (Html_get_attr(html, tag, tagsize, "selected") != NULL);
        {
            input->select->num_options++;
        }
#ifdef JS_SUPPORT
        pobj = lastselobj;
        if ( pobj ) {
            jsobj = js_objnew(NULL, jsoption, pobj, html, tag, tagsize);
            if ( jsobj ) {
                /* common */
                jsobj->pvar1 = pobj->pvar1;    /* index */
                pobj->pvar1 ++;    /* opt nums */
                pobj->children = 
                    g_list_append(pobj->children, jsobj);
            }
        }
#endif
    }
    Html_stash_init(html);
}

/*
 * Set the Document Base URI
 */
static void Html_tag_open_base(mSpiderHtml *html, char *tag, int tagsize)
{
    const char *attrbuf;
    mSpiderUrl *BaseUrl;

    if (html->InFlags & IN_HEAD) {
        if ((attrbuf = Html_get_attr(html, tag, tagsize, "href"))) {
            BaseUrl = Html_url_new(html, attrbuf, "", 0, 0, 0, 1);
            if (URL_SCHEME_(BaseUrl)) {
                /* Pass the URL_SpamSafe flag to the new base url */
                a_Url_set_flags(BaseUrl, 
                    URL_FLAGS(html->linkblock->base_url) & URL_SpamSafe);
                a_Url_free(html->linkblock->base_url);
                html->linkblock->base_url = BaseUrl;
            } else {
                MSG_HTML("base URI is relative (it MUST be absolute)\n");
                a_Url_free(BaseUrl);
            }
        }
        if ((attrbuf = Html_get_attr(html, tag, tagsize, "target")))
	       a_Url_set_target(html->linkblock->base_url, attrbuf);
    }
    else {
        MSG_HTML("the BASE element must appear in the HEAD section\n");
    }
}

/*
 * <CODE>
 */
static void Html_tag_open_code(mSpiderHtml *html, char *tag, int tagsize)
{
    //Html_push_tag(html, tag, tagsize);
    Html_set_top_font(html, prefs.fw_fontname, 0, 0, 0);
}

/*
 * <DFN>
 */
static void Html_tag_open_dfn(mSpiderHtml *html, char *tag, int tagsize)
{
    //Html_push_tag(html, tag, tagsize);
    Html_set_top_font(html, NULL, 0, 2, 3);
}

/*
 * <KBD>
 */
static void Html_tag_open_kbd(mSpiderHtml *html, char *tag, int tagsize)
{
    //Html_push_tag(html, tag, tagsize);
    Html_set_top_font(html, prefs.fw_fontname, 0, 0, 0);
}

static void Html_tag_open_samp(mSpiderHtml *html, char *tag, int tagsize)
{
    //Html_push_tag(html, tag, tagsize);
    Html_set_top_font(html, prefs.fw_fontname, 0, 0, 0);
}

/*
 * <VAR>
 */
static void Html_tag_open_var(mSpiderHtml *html, char *tag, int tagsize)
{
    //Html_push_tag(html, tag, tagsize);
    Html_set_top_font(html, NULL, 0, 2, 2);
}

static void Html_tag_open_sub(mSpiderHtml *html, char *tag, int tagsize)
{
    //Html_push_tag(html, tag, tagsize);
    HTML_SET_TOP_ATTR (html, valign, DW_STYLE_VALIGN_SUB);
}

static void Html_tag_open_sup(mSpiderHtml *html, char *tag, int tagsize)
{
    //Html_push_tag(html, tag, tagsize);
    HTML_SET_TOP_ATTR (html, valign, DW_STYLE_VALIGN_SUPER);
}

/*
 * <DIV> (todo: make a complete implementation)
 */
static void Html_tag_open_div(mSpiderHtml *html, char *tag, int tagsize)
{
    a_Dw_page_add_parbreak(DW_PAGE (html->dw), 0,
                            html->stack[(html)->stack_top].style);

    //Html_push_tag(html, tag, tagsize);
    Html_tag_set_align_attr (html, tag, tagsize);

}

/*
 * </DIV>, also used for </CENTER>
 */
static void Html_tag_close_div(mSpiderHtml *html, int TagIdx)
{
    a_Dw_page_add_parbreak(DW_PAGE (html->dw), 0,
                          html->stack[(html)->stack_top].style);
    Html_pop_tag(html, TagIdx);
}

/*
 * </table>
 */
static void Html_tag_close_table(mSpiderHtml *html, int TagIdx)
{
#ifdef AUTOFOLD_SUPPORT
    tabstatck[intable--] = 0;
    if ( intable < 0 ) {
        intable = 0;
    }
#endif
    a_Dw_page_add_parbreak(DW_PAGE (html->dw), 0,
                          html->stack[(html)->stack_top].style);
    Html_pop_tag(html, TagIdx);
}

/*
 * Default close for most tags - just pop the stack.
 */
static void Html_tag_close_default(mSpiderHtml *html, int TagIdx)
{
    Html_pop_tag(html, TagIdx);
}

/*
 * Default close for paragraph tags - pop the stack and break.
 */
static void Html_tag_close_par(mSpiderHtml *html, int TagIdx)
{
    a_Dw_page_add_parbreak(DW_PAGE (html->dw), 9,
                           html->stack[(html)->stack_top].style);
    Html_pop_tag(html, TagIdx);
}

#if 0
static void Html_tag_open_marquee(mSpiderHtml *html, char *tag, int tagsize)
{
    DwStyleFontMove font_move_style;
    DwStyle style_attrs, *tstyle, *old_style;
    const char *attrbuf;
    HWND hwnd_main;
    hwnd_main = html->dd->docwin;
    char *str = NULL;
    char *marquee_str;


    str = strstr(tag, "</marquee>");
    tag = str + 10;

    return; 

#if 0
    marquee_str = (char*) malloc(str + 10 - tag + 1);
    memset(marquee_str, '0', sizeof(marquee_str));
    strncpy(marquee_str, tag, str - tag + 11 ); 
    printf("marquee_str ========= (%s) \n", marquee_str);
    free(marquee_str);

    if ((attrbuf = Html_get_attr(html, tag, tagsize, "direction"))) 
    {
        if (g_strcasecmp (attrbuf, "left") == 0)
            font_move_style.direction = FONT_MOVE_LEFT;
        else if (g_strcasecmp (attrbuf, "right") == 0)
            font_move_style.direction = FONT_MOVE_RIGHT;
        else if (g_strcasecmp (attrbuf, "up") == 0)
            font_move_style.direction = FONT_MOVE_UP;
        else if (g_strcasecmp (attrbuf, "down") == 0)
            font_move_style.direction = FONT_MOVE_DOWN;
    }
    else
        font_move_style.direction = FONT_MOVE_LEFT;

    if ((attrbuf = Html_get_attr(html, tag, tagsize, "behavior"))) 
    {

        if (g_strcasecmp (attrbuf, "scroll") == 0)
            font_move_style.direction = FONT_BEHAVIOR_SCROLL;
        else if (g_strcasecmp (attrbuf, "slide") == 0)
            font_move_style.direction = FONT_BEHAVIOR_SLIDE;
        else if (g_strcasecmp (attrbuf, "alternate") == 0)
            font_move_style.direction = FONT_BEHAVIOR_ALTERNATE;
    }
    else
        font_move_style.direction = FONT_BEHAVIOR_SCROLL;


    if ((attrbuf = Html_get_attr(html, tag, tagsize, "loop")))
    {
        font_move_style.loop = isdigit(attrbuf[0]) ? strtol (attrbuf, NULL, 10) : 0;
    }
    else 
        font_move_style.loop = 0;

    if ((attrbuf = Html_get_attr(html, tag, tagsize, "scrollamount")))
    {
        font_move_style.move_speed = isdigit(attrbuf[0]) ? strtol (attrbuf, NULL, 10) : 0;
    }
    else
        font_move_style.move_speed = 0;

    if ((attrbuf = Html_get_attr(html, tag, tagsize, "scrolldelay")))
    {
        font_move_style.scroll_delay = isdigit(attrbuf[0]) ? strtol (attrbuf, NULL, 10) : 0;
    } 
    else
        font_move_style.scroll_delay = 0;

    if ((attrbuf = Html_get_attr(html, tag, tagsize, "bgcolor")))
    {
        font_move_style.bgcolor = PIXEL_lightgray ;
        //font_move_style.bgcolor = isdigit(attrbuf[0]) ? strtol (attrbuf, NULL, 10) : 1;
    }
    else
        font_move_style.bgcolor = PIXEL_lightgray;

    if ((attrbuf = Html_get_attr(html, tag, tagsize, "width")))
    {
        font_move_style.width = isdigit(attrbuf[0]) ? strtol (attrbuf, NULL, 10) : 1;
    }
    else
        font_move_style.width = 0;

    if ((attrbuf = Html_get_attr(html, tag, tagsize, "height")))
    {
        font_move_style.height = isdigit(attrbuf[0]) ? strtol (attrbuf, NULL, 10) : 1;
    }
    else
        font_move_style.height = 0;

    printf(" direction = %d \n", font_move_style.direction);
    printf(" behavior = %d \n", font_move_style.behavior);
    printf(" loop = %d \n", font_move_style.loop);
    printf(" move_speed = %d \n", font_move_style.move_speed);
    printf(" scroll_delay = %d \n", font_move_style.scroll_delay);
    printf(" bgcolor = %d \n", font_move_style.bgcolor);
    printf(" width = %d \n", font_move_style.width);
    printf(" height = %d \n", font_move_style.height);

#if 0
    /* The style for the table */
    style_attrs = *html->stack[html->stack_top].style;

    /* When mspider was started with the --debug-rendering option, there
     * is always a border around the table. */
    if (mspider_dbg_rendering)
        a_Dw_style_box_set_val (&style_attrs.border_width, MIN (border, 1));
    else
        a_Dw_style_box_set_val (&style_attrs.border_width, border);

    a_Dw_style_box_set_border_color (&style_attrs,
        a_Dw_style_shaded_color_new
            (html->stack[html->stack_top].current_bg_color, html->dd->bw->main_window));
    a_Dw_style_box_set_border_style (&style_attrs, DW_STYLE_BORDER_OUTSET);
    style_attrs.border_spacing = cellspacing;

    if ((attrbuf = Html_get_attr(html, tag, tagsize, "width")))
        style_attrs.width = Html_parse_length (html, attrbuf);

    if ((attrbuf = Html_get_attr(html, tag, tagsize, "align"))) {
        if (g_strcasecmp (attrbuf, "left") == 0)
            style_attrs.text_align = DW_STYLE_TEXT_ALIGN_LEFT;
        else if (g_strcasecmp (attrbuf, "right") == 0)
            style_attrs.text_align = DW_STYLE_TEXT_ALIGN_RIGHT;
        else if (g_strcasecmp (attrbuf, "center") == 0)
            style_attrs.text_align = DW_STYLE_TEXT_ALIGN_CENTER;
    }

    if (!prefs.force_my_colors &&
            (attrbuf = Html_get_attr(html, tag, tagsize, "bgcolor"))) {
        bgcolor = Html_color_parse(html, attrbuf, -1);
        if (bgcolor != -1) {
            if (bgcolor == 0xffffff && !prefs.allow_white_bg)
                bgcolor = prefs.bg_color;
            html->stack[html->stack_top].current_bg_color = bgcolor;
            style_attrs.background_color =
                a_Dw_style_color_new (bgcolor, html->dd->bw->main_window);
        }
    }

    tstyle = a_Dw_style_new (&style_attrs, html->dd->bw->main_window);

    /* The style for the cells */
    style_attrs = *html->stack[html->stack_top].style;
    /* When mspider was started with the --debug-rendering option, there
       * is always a border around the cells. */
    if (mspider_dbg_rendering)
        a_Dw_style_box_set_val (&style_attrs.border_width, 1);
    else
        a_Dw_style_box_set_val (&style_attrs.border_width, border ? 1 : 0);

    a_Dw_style_box_set_val (&style_attrs.padding, cellpadding);
    a_Dw_style_box_set_border_color (&style_attrs, tstyle->border_color.top);
    a_Dw_style_box_set_border_style (&style_attrs, DW_STYLE_BORDER_INSET);

    old_style = html->stack[html->stack_top].table_cell_style;
    html->stack[html->stack_top].table_cell_style =
        a_Dw_style_new (&style_attrs, html->dd->bw->main_window);
    if (old_style)
        a_Dw_style_unref (old_style);

    table = a_Dw_table_new ();
    a_Dw_page_add_widget (DW_PAGE (html->dw), table, tstyle);
    a_Dw_style_unref (tstyle);

    html->stack[html->stack_top].table_mode = MSPIDER_HTML_TABLE_MODE_TOP;
    html->stack[html->stack_top].cell_text_align_set = FALSE;
    html->stack[html->stack_top].table = table;
#endif

#endif
} 
#endif

/*
 * Function index for the open and close functions for each tag
 * (Alphabetically sorted for a binary search)
 */
static const TagInfo Tags[] = {
 {"a", B8(010101),'R',2, Html_tag_open_a, Html_tag_close_a},
#ifdef _TOOL_TIP_
 {"abbr", B8(010101),'R',2, Html_tag_open_abbr, Html_tag_close_default},
#endif
 /* acronym 010101 */
 {"address", B8(010110),'R',2, Html_tag_open_address, Html_tag_close_par},
 {"area", B8(010001),'F',0, Html_tag_open_area, Html_tag_close_default},
 {"b", B8(010101),'R',2, Html_tag_open_b, Html_tag_close_default},
 {"base", B8(100001),'F',0, Html_tag_open_base, Html_tag_close_default},
 /* basefont 010001 */
 /* bdo 010101 */
 {"big", B8(010101),'R',2, Html_tag_open_big_small, Html_tag_close_default},
 {"blockquote", B8(011110),'R',2,Html_tag_open_blockquote,Html_tag_close_par},
 {"body", B8(011110),'O',7, Html_tag_open_body, Html_tag_close_body},
 {"br", B8(010001),'F',0, Html_tag_open_br, Html_tag_close_default},
 {"button", B8(011101),'R',2, Html_tag_open_button, Html_tag_close_default},
 /* caption */
 {"center", B8(011110),'R',2, Html_tag_open_center, Html_tag_close_div},
 {"cite", B8(010101),'R',2, Html_tag_open_cite, Html_tag_close_default},
 {"code", B8(010101),'R',2, Html_tag_open_code, Html_tag_close_default},
 /* col 010010 'F' */
 /* colgroup */
 {"dd", B8(011110),'O',1, Html_tag_open_dd, Html_tag_close_par},
 {"del", B8(011101),'R',2, Html_tag_open_strike, Html_tag_close_default},
 {"dfn", B8(010101),'R',2, Html_tag_open_dfn, Html_tag_close_default},
 /* dir 011010 */
 /* todo: complete <div> support! */
 {"div", B8(011110),'R',2, Html_tag_open_div, Html_tag_close_div},
 {"dl", B8(011010),'R',2, Html_tag_open_dl, Html_tag_close_par},
 {"dt", B8(010110),'O',1, Html_tag_open_dt, Html_tag_close_par},
 {"em", B8(010101),'R',2, Html_tag_open_em, Html_tag_close_default},
#ifdef ENABLE_FLASH
 {"embed", B8(010001),'F',0, Html_tag_open_flash, Html_tag_close_default},
#endif
 /* fieldset */
 {"font", B8(010101),'R',2, Html_tag_open_font, Html_tag_close_default},
 {"form", B8(011110),'R',2, Html_tag_open_form, Html_tag_close_form},
 {"frame", B8(010010),'F',0, Html_tag_open_frame, Html_tag_close_default},
 {"frameset", B8(011110),'R',2,Html_tag_open_frameset, Html_tag_close_default},
 {"h1", B8(010110),'R',2, Html_tag_open_h, Html_tag_close_h},
 {"h2", B8(010110),'R',2, Html_tag_open_h, Html_tag_close_h},
 {"h3", B8(010110),'R',2, Html_tag_open_h, Html_tag_close_h},
 {"h4", B8(010110),'R',2, Html_tag_open_h, Html_tag_close_h},
 {"h5", B8(010110),'R',2, Html_tag_open_h, Html_tag_close_h},
 {"h6", B8(010110),'R',2, Html_tag_open_h, Html_tag_close_h},
 {"head", B8(101101),'O',1, Html_tag_open_head, Html_tag_close_head},
 {"hr", B8(010010),'F',0, Html_tag_open_hr, Html_tag_close_default},
 {"html", B8(001110),'O',8, Html_tag_open_html, Html_tag_close_html},
 {"i", B8(010101),'R',2, Html_tag_open_i, Html_tag_close_default},
 {"iframe", B8(011110),'R',2, Html_tag_open_iframe, Html_tag_close_default},
 {"img", B8(010001),'F',0, Html_tag_open_img, Html_tag_close_default},
 {"input", B8(010001),'F',0, Html_tag_open_input, Html_tag_close_default},
 /* ins */
 {"ins", B8(011101),'R',2, Html_tag_open_underline, Html_tag_close_default},
 {"isindex", B8(110001),'F',0, Html_tag_open_isindex, Html_tag_close_default},
 {"kbd", B8(010101),'R',2, Html_tag_open_kbd, Html_tag_close_default},
 /* label 010101 */
 /* legend 01?? */
 {"li", B8(011110),'O',1, Html_tag_open_li, Html_tag_close_default},
 /* link 100000 'F' */
 {"map", B8(011001),'R',2, Html_tag_open_map, Html_tag_close_map},
// {"marquee", B8(001011),'R',2, Html_tag_open_marquee, Html_tag_close_default},
 /* menu 1010 -- todo: not exactly 1010, it can contain LI and inline */
 {"menu", B8(011010),'R',2, Html_tag_open_menu, Html_tag_close_par},
 {"meta", B8(100001),'F',0, Html_tag_open_meta, Html_tag_close_default},
 {"noframe", B8(001011),'R',2, Html_tag_open_noframe,  Html_tag_close_default}, /* Illegal tag */
 {"noframes", B8(001011),'R',2, Html_tag_open_noframes, Html_tag_close_default},
   /* noscript 1011 */
 /* object 11xxxx */
 {"ol", B8(011010),'R',2, Html_tag_open_ol, Html_tag_close_par},
 /* optgroup */
 {"option", B8(010001),'O',1, Html_tag_open_option, Html_tag_close_default},
 {"p", B8(010110),'O',1, Html_tag_open_p, Html_tag_close_par},
 /* param 010001 'F' */
 {"pre", B8(010110),'R',2, Html_tag_open_pre, Html_tag_close_pre},
 /* q 010101 */
 {"s", B8(010101),'R',2, Html_tag_open_strike, Html_tag_close_default},
 {"samp", B8(010101),'R',2, Html_tag_open_samp, Html_tag_close_default},
 {"script", B8(111001),'R',2, Html_tag_open_script, Html_tag_close_script},
 {"select", B8(011001),'R',2, Html_tag_open_select, Html_tag_close_select},
 {"small", B8(010101),'R',2, Html_tag_open_big_small, Html_tag_close_default},
 /* span 0101 */
 {"strike", B8(010101),'R',2, Html_tag_open_strike, Html_tag_close_default},
 {"strong", B8(010101),'R',2, Html_tag_open_strong, Html_tag_close_default},
 {"style", B8(100101),'R',2, Html_tag_open_style, Html_tag_close_style},
 {"sub", B8(010101),'R',2, Html_tag_open_sub, Html_tag_close_default},
 {"sup", B8(010101),'R',2, Html_tag_open_sup, Html_tag_close_default},
 {"table", B8(011010),'R',5, Html_tag_open_table, Html_tag_close_table},
 /* tbody */
 {"td", B8(011110),'O',3, Html_tag_open_td, Html_tag_close_default},
 {"textarea", B8(010101),'R',2,Html_tag_open_textarea,Html_tag_close_textarea},
 /* tfoot */
 {"th", B8(011110),'O',1, Html_tag_open_th, Html_tag_close_default},
 /* thead */
 {"title", B8(100101),'R',2, Html_tag_open_title, Html_tag_close_title},
 {"tr", B8(011010),'O',4, Html_tag_open_tr, Html_tag_close_default},
 {"tt", B8(010101),'R',2, Html_tag_open_tt, Html_tag_close_default},
 {"u", B8(010101),'R',2, Html_tag_open_u, Html_tag_close_default},
 {"ul", B8(011010),'R',2, Html_tag_open_ul, Html_tag_close_par},
 {"var", B8(010101),'R',2, Html_tag_open_var, Html_tag_close_default},
};
#define NTAGS (sizeof(Tags)/sizeof(Tags[0]))

/*
 * Compares tag from buffer ('/' or '>' or space-ended string) [p1]
 * with tag from taglist (lowercase, zero ended string) [p2]
 * Return value: as strcmp()
 */
static int Html_tag_compare(char *p1, char *p2)
{
    while ( *p2 ) {
       if ( tolower(*p1) != *p2 )
          return(tolower(*p1) - *p2);
       ++p1;
       ++p2;
    }
    return !strchr(" >/\n\r\t", *p1);
}

/*
 * Get 'tag' index
 * return -1 if tag is not handled yet
 */
static int Html_tag_index(char *tag)
{
    int low, high, mid, cond;

    /* Binary search */
    low = 0;
    high = NTAGS - 1;          /* Last tag index */
    while (low <= high) {
        mid = (low + high) / 2;
        if ((cond = Html_tag_compare(tag, Tags[mid].name)) < 0 )
            high = mid - 1;
        else if (cond > 0)
            low = mid + 1;
        else
            return mid;
    }
    return -1;
}

#if 0
/*
 * The HEAD element is optional. This function tests
 * when to switch to the BODY section.
 */
static void Html_test_body_section(mSpiderHtml *html)
{
    char *tag_str = Html_tags_get_name(html->CurrTagIdx);

    if (strcmp(tag_str, "title") && strcmp(tag_str, "base") &&
            strcmp(tag_str, "meta") && strcmp(tag_str, "link") &&
            strcmp(tag_str, "object") && strcmp(tag_str, "script") &&
            strcmp(tag_str, "style") && strcmp(tag_str, "head")) {
        /* tag does not belong inside HEAD, switch to BODY section */
        Html_cleanup_tag(html, "head>");
        html->InFlags &= ~IN_HEAD;
        html->InFlags |= IN_BODY;
    }
}
#endif

/*
 * For elements with optional close, check whether is time to close.
 * Return value: (1: Close, 0: Don't close)
 * --tuned for speed.
 */
static gint Html_needs_optional_close(gint old_idx, gint cur_idx)
{
   static gint i_P = -1, i_LI, i_TD, i_TR, i_TH, i_DD, i_DT, i_OPTION;
               // i_THEAD, i_TFOOT, i_COLGROUP;

   if (i_P == -1) {
    /* initialize the indexes of elements with optional close */
    i_P  = Html_tag_index("p"),
    i_LI = Html_tag_index("li"),
    i_TD = Html_tag_index("td"),
    i_TR = Html_tag_index("tr"),
    i_TH = Html_tag_index("th"),
    i_DD = Html_tag_index("dd"),
    i_DT = Html_tag_index("dt"),
    i_OPTION = Html_tag_index("option");
    // i_THEAD = Html_tag_index("thead");
    // i_TFOOT = Html_tag_index("tfoot");
    // i_COLGROUP = Html_tag_index("colgroup");
   }

   if (old_idx == i_P || old_idx == i_DT) {
      /* P and DT are closed by block elements */
      return (Tags[cur_idx].Flags & 2);
   } else if (old_idx == i_LI) {
      /* LI closes LI */
      return (cur_idx == i_LI);
   } else if (old_idx == i_TD || old_idx == i_TH) {
      /* TD and TH are closed by TD, TH and TR */
      return (cur_idx == i_TD || cur_idx == i_TH || cur_idx == i_TR);
   } else if (old_idx == i_TR) {
      /* TR closes TR */
      return (cur_idx == i_TR);
   } else if (old_idx ==  i_DD) {
      /* DD is closed by DD and DT */
      return (cur_idx == i_DD || cur_idx == i_DT);
   } else if (old_idx ==  i_OPTION) {
      return 1;  // OPTION always needs close
   }

   /* HTML, HEAD, BODY are handled by Html_test_section(), not here. */
   /* todo: TBODY is pending */
   return 0;
}

/*
 * Conditional cleanup of the stack (at open time).
 * - This helps catching block elements inside inline containers (a BUG).
 * - It also closes elements with "optional" close tag.
 *
 * This function is called when opening a block element or <OPTION>.
 *
 * It searches the stack closing open inline containers, and closing
 * elements with optional close tag when necessary.
 *
 * Note: OPTION is the only non-block element with an optional close.
 */
static void Html_stack_cleanup_at_open(mSpiderHtml *html, gint new_idx)
{
   /* We know that the element we're about to push is a block element.
    * (except for OPTION, which is an empty inline, so is closed anyway)
    * Notes:
    *   Its 'tag' is not yet pushed into the stack,
    *   'new_idx' is its index inside Tags[].
    */

   if (!html->TagSoup)
      return;

   while (html->stack_top) {
      gint oldtag_idx = html->stack[html->stack_top].tag_idx;

      if (Tags[oldtag_idx].EndTag == 'O') {    // Element with optional close
         if (!Html_needs_optional_close(oldtag_idx, new_idx))
            break;
      } else if (Tags[oldtag_idx].Flags & 8) { // Block container
         break;
      }

      /* we have an inline (or empty) container... */
      if (Tags[oldtag_idx].EndTag == 'R') {
         MSG_HTML("<%s> is not allowed to contain <%s>. -- closing <%s>\n",
                  Tags[oldtag_idx].name, Tags[new_idx].name,
                  Tags[oldtag_idx].name);
      }

      /* Workaround for Apache and its bad HTML directory listings... */
      if ((html->InFlags & IN_PRE) &&
          strcmp(Tags[new_idx].name, "hr") == 0)
         break;

      /* This call closes the top tag only. */
      Html_tag_cleanup_at_close(html, oldtag_idx);
   }
}

/*
 * HTML, HEAD and BODY elements have optional open and close tags.
 * Handle this "magic" here.
 */
static void Html_test_section(mSpiderHtml *html, gint new_idx, gint IsCloseTag)
{
   gchar *tag;
   gint tag_idx;

   if (!(html->InFlags & IN_HTML)) {
      tag = "<html>";
      tag_idx = Html_tag_index(tag + 1);
      if (tag_idx != new_idx || IsCloseTag) {
         /* implicit open */
         Html_force_push_tag(html, tag_idx);
         Tags[tag_idx].open (html, tag, strlen(tag));
      }
   }

   if (Tags[new_idx].Flags & 32) {
      /* head element */
      if (!(html->InFlags & IN_HEAD)) {
         tag = "<head>";
         tag_idx = Html_tag_index(tag + 1);
         if (tag_idx != new_idx || IsCloseTag) {
            /* implicit open of the head element */
            Html_force_push_tag(html, tag_idx);
            Tags[tag_idx].open (html, tag, strlen(tag));
         }
      }

   } else if (Tags[new_idx].Flags & 16) {
      /* body element */
      if (html->InFlags & IN_HEAD) {
         tag = "</head>";
         tag_idx = Html_tag_index(tag + 2);
         Tags[tag_idx].close (html, tag_idx);
      }
      tag = "<body>";
      tag_idx = Html_tag_index(tag + 1);
      if (tag_idx != new_idx || IsCloseTag) {
         /* implicit open */
         Html_force_push_tag(html, tag_idx);
         Tags[tag_idx].open (html, tag, strlen(tag));
      }
   }
}

/*
 * Process a tag, given as 'tag' and 'tagsize'.
 * ('tag' must include the enclosing angle brackets)
 * This function calls the right open or close function for the tag.
 */
static void Html_process_tag(mSpiderHtml *html, char *tag, int tagsize)
{
    int ci, ni;       /* current and new tags */
    const char *attrbuf;
    char *start = tag + 1;       /* discard the '<' */
    int IsCloseTag = (*start == '/');
   /* Handle HTML, HEAD and BODY. Elements with optional open and close */
   ni = Html_tag_index(start + IsCloseTag);
   if (ni != -1 && !(html->InFlags & IN_BODY) /* && parsing HTML */)
      Html_test_section(html, ni, IsCloseTag);

   /* White space handling */
   if (html->SPCPending && (!SGML_SPCDEL || !IsCloseTag))
      /* SGML_SPCDEL requires space pending and open tag */
      a_Dw_page_add_space(DW_PAGE (html->dw),
                          html->stack[html->stack_top].style);
   html->SPCPending = FALSE;

   /* Tag processing */
   ci = html->stack[html->stack_top].tag_idx;
   if (ni != -1) {

      if (!IsCloseTag) {
         /* Open function */

         /* Cleanup when opening a block element, or
          * when openning over an element with optional close */
         if (Tags[ni].Flags & 2 || (ci != -1 && Tags[ci].EndTag == 'O'))
            Html_stack_cleanup_at_open(html, ni);

         /* todo: this is only raising a warning, take some defined action.
          * Note: apache uses IMG inside PRE (we could use its "alt"). */
         if ((html->InFlags & IN_PRE) && Html_tag_pre_excludes(ni))
            MSG_HTML("<pre> is not allowed to contain <%s>\n", Tags[ni].name);

         /* Push the tag into the stack */
         Html_push_tag(html, ni);

         /* Call the open function for this tag */
         Tags[ni].open (html, tag, tagsize);

         /* Now parse attributes that can appear on any tag */
         if (tagsize >= 8 &&        /* length of "<t id=i>" */
             (attrbuf = Html_get_attr2(html, tag, tagsize, "id",
                                       HTML_LeftTrim | HTML_RightTrim))) {
            /* According to the SGML declaration of HTML 4, all NAME values
             * occuring outside entities must be converted to uppercase
             * (this is what "NAMECASE GENERAL YES" says). But the HTML 4
             * spec states in Sec. 7.5.2 that anchor ids are case-sensitive.
             * So we don't do it and hope for better specs in the future ...
             */
            Html_check_name_val(html, attrbuf, "id");
            /* We compare the "id" value with the url-decoded "name" value */
            if (!html->NameVal || strcmp(html->NameVal, attrbuf)) {
               if (html->NameVal)
                  MSG_HTML("'id' and 'name' attribute of <a> tag differ\n");
               Html_add_anchor(html, attrbuf);
            }
         }

         /* Reset NameVal */
         if (html->NameVal) {
            g_free(html->NameVal);
            html->NameVal = NULL;
         }

         /* let the parser know this was an open tag */
         html->PrevWasOpenTag = TRUE;

         /* Request inmediate close for elements with forbidden close tag. */
         /* todo: XHTML always requires close tags. A simple implementation
          * of the commented clause below will make it work. */
         if  (/* parsing HTML && */ Tags[ni].EndTag == 'F')
            html->ReqTagClose = TRUE;
      }

      /* Close function: test for </x>, ReqTagClose, <x /> and <x/> */
      if (*start == '/' ||                                      /* </x>    */
          html->ReqTagClose ||                                  /* request */
          (tag[tagsize - 2] == '/' &&                           /* XML:    */
           (isspace(tag[tagsize - 3]) ||                        /*  <x />  */
            (size_t)tagsize == strlen(Tags[ni].name) + 3))) {   /*  <x/>   */

         Tags[ni].close (html, ni);
         /* This was a close tag */
         html->PrevWasOpenTag = FALSE;
         html->ReqTagClose = FALSE;
      }

   } else {
      /* tag not working - just ignore it */
   }
}

/*
 * Get attribute value for 'attrname' and return it.
 *  Tags start with '<' and end with a '>' (Ex: "<P align=center>")
 *  tagsize = strlen(tag) from '<' to '>', inclusive.
 *
 * Returns one of the following:
 *    * The value of the attribute.
 *    * An empty string if the attribute exists but has no value.
 *    * NULL if the attribute doesn't exist.
 */
static const char *Html_get_attr2(mSpiderHtml *html,
                                  const char *tag,
                                  int tagsize,
                                  const char *attrname,
                                  mSpiderHtmlTagParsingFlags flags)
{
    int i, isocode, entsize, Found = 0, delimiter = 0, attr_pos = 0;
    GString *Buf = html->attr_data;
    mSpiderHtmlTagParsingState state = SEEK_ATTR_START;


    if (*attrname == 0) return NULL;

    g_string_truncate(Buf, 0);

    for (i = 1; i < tagsize; ++i) {
        switch (state) {
        case SEEK_ATTR_START:
            if (isspace(tag[i]))
                state = SEEK_TOKEN_START;
            else if (tag[i] == '=') {
                state = SEEK_VALUE_START;}
            break;

        case MATCH_ATTR_NAME:
            if ((Found = (!(attrname[attr_pos]) &&
                        (tag[i] == '=' || isspace(tag[i]) || tag[i] == '>')))) {
                state = SEEK_TOKEN_START;
                --i;
            }
            else {
                if (tolower(tag[i]) != tolower(attrname[attr_pos])) {
                    state = SEEK_ATTR_START;
                }
                attr_pos++;
            }
            break;

        case SEEK_TOKEN_START:
            if (tag[i] == '=') {
                state = SEEK_VALUE_START;
            }
            else if (!isspace(tag[i])) {
                attr_pos = 0;
                state = (Found) ? FINISHED : MATCH_ATTR_NAME;
                --i;
            }
            break;
        case SEEK_VALUE_START:
            if (!isspace(tag[i])) {
                delimiter = (tag[i] == '"' || tag[i] == '\'') ? tag[i] : ' ';
                i -= (delimiter == ' ');
                state = (Found) ? GET_VALUE : SKIP_VALUE;
            }
            break;

        case SKIP_VALUE:
            if ((delimiter == ' ' && isspace(tag[i])) || tag[i] == delimiter)
                state = SEEK_TOKEN_START;
            break;
        case GET_VALUE:
            if ((delimiter == ' ' && (isspace(tag[i]) || tag[i] == '>')) ||
                tag[i] == delimiter) {
                state = FINISHED;
            }
            else if (tag[i] == '&' && (flags & HTML_ParseEntities)) {
                if ((isocode = Html_parse_entity(html, tag+i, tagsize-i, &entsize)) >= 0) {
                    g_string_append_c(Buf, (char) isocode);
                    i += entsize - 1;
                    //while (tag[++i] != ';');
                }
                else {
                    g_string_append_c(Buf, tag[i]);
                }
            }
            else if (tag[i] == '\r' || tag[i] == '\t') {
                g_string_append_c(Buf, ' ');
            }
            else if (tag[i] == '\n') {
                /* ignore */
            }
            else {
                g_string_append_c(Buf, tag[i]);
            }
            break;

        case FINISHED:
            i = tagsize;
            break;
        }
    }

    if (flags & HTML_LeftTrim)
        while (isspace(Buf->str[0]))
            g_string_erase(Buf, 0, 1);
    if (flags & HTML_RightTrim)
        while (Buf->len && isspace(Buf->str[Buf->len - 1]))
            g_string_truncate(Buf, Buf->len - 1);
    return (Found) ? Buf->str : NULL;
}

/*
 * Call Html_get_attr2 telling it to parse entities and strip the result
 */
const char *Html_get_attr(mSpiderHtml *html,
                                 const char *tag,
                                 int tagsize,
                                 const char *attrname)
{
    return Html_get_attr2(html, tag, tagsize, attrname,
                         HTML_LeftTrim | HTML_RightTrim | HTML_ParseEntities);
}

/*
 * Add a widget to the page.
 */
static void Html_add_widget(mSpiderHtml *html,
                            DwWidget *widget,
                            char *width_str,
                            char *height_str,
                            DwStyle *style_attrs)
{
    DwStyle new_style_attrs, *style;

    new_style_attrs = *style_attrs;
    new_style_attrs.width = width_str ?
        Html_parse_length (html, width_str) : DW_STYLE_LENGTH_AUTO;
    new_style_attrs.height = height_str ?
        Html_parse_length (html, height_str) : DW_STYLE_LENGTH_AUTO;

   if ((gulong)new_style_attrs.width*new_style_attrs.height
         > (gulong)(G_MAXLONG >> 1)) {
      new_style_attrs.width = DW_STYLE_LENGTH_AUTO;
      new_style_attrs.height = DW_STYLE_LENGTH_AUTO;
   }
    style = a_Dw_style_new (&new_style_attrs, html->dd->bw->main_window);
    a_Dw_page_add_widget(DW_PAGE (html->dw), widget, style);
    a_Dw_style_unref (style);
}


/*
 * Dispatch the apropriate function for 'Op'
 * This function is a Cache client and gets called whenever new data arrives
 *  Op      : operation to perform.
 *  CbData  : a pointer to a mSpiderHtml structure
 *  Buf     : a pointer to new data
 *  BufSize : new data size (in bytes)
 */
static void Html_callback(int Op, CacheClient_t *Client)
{
#ifdef JS_SUPPORT
    jsobject *jsobj = NULL;
    jsscript_interpreter *interpreter = NULL;
#endif
    if (Op) {
        Html_write(Client->CbData, (char*)Client->Buf, Client->BufSize, 1);
#ifdef JS_SUPPORT
        jsobj = js_getdftdocobj((int)((mSpiderHtml*)Client->CbData)->dd);
        if ( !jsobj || !(interpreter = jsobj->jsinterpreter) ) {
            return;
        }
        
        if ( interpreter->inuse == 2 ) {
            interpreter->inuse = 1;
        }
#endif
        Html_close(Client->CbData, Client->Key);
    }
    else
        Html_write(Client->CbData, (char*)Client->Buf, Client->BufSize, 0);
}

/*
 * Here's where we parse the html and put it into the page structure.
 * Return value: number of bytes parsed
 */
int Html_write_raw(mSpiderHtml *html, char *buf, int bufsize, int Eof)
{
    char ch = 0, *p, *text;
    DwPage *page;
    int token_start, buf_index;

    g_return_val_if_fail ((page = DW_PAGE (html->dw)) != NULL, 0);
   if(bufsize < 0){
      _MSG(_("Html_write_raw : bufsize is negative!\n"));
      bufsize = strlen(buf);
   }

    buf = g_strndup(buf, bufsize);

    /* Now, 'buf' and 'bufsize' define a buffer aligned to start at a token
     * boundary. Iterate through tokens until end of buffer is reached. */
    buf_index = 0;
    token_start = buf_index;
    while (buf_index < bufsize) {
        /* invariant: buf_index == bufsize || token_start == buf_index */

        if (html->stack[html->stack_top].parse_mode ==
                MSPIDER_HTML_PARSE_MODE_VERBATIM) {
            /* Non HTML code here, let's skip until closing tag */
            do {
                char *tag = html->stack[html->stack_top].tag_name;
                buf_index += strcspn(buf + buf_index, "<");
                if (buf_index + (int)strlen(tag) + 3 > bufsize) {
                    buf_index = bufsize;
                }
                else if (strncmp(buf + buf_index, "</", 2) == 0 &&
                        Html_match_tag(tag, buf+buf_index+2, strlen(tag)+1)) {
                    /* copy VERBATIM text into the stash buffer */
                    text = g_strndup(buf + token_start, buf_index - token_start);
                    g_string_append(html->Stash, text);
                    g_free(text);
                    token_start = buf_index;
                    break;
                }
                else
                    ++buf_index;
            } while (buf_index < bufsize);
        }

        if ( isspace(buf[buf_index]) ) {
            /* whitespace: group all available whitespace */
            while (++buf_index < bufsize && isspace(buf[buf_index]));
            Html_process_space(html, buf + token_start, buf_index - token_start);
            token_start = buf_index;
        }
        else if (buf[buf_index] == '<' && (ch = buf[buf_index + 1]) &&
                 (isalpha(ch) || strchr("/!?", ch)) ) {
            /* Tag */
            if (buf_index + 3 < bufsize && !strncmp(buf + buf_index, "<!--", 4)) {
                /* Comment: search for close of comment, skipping over
                 * everything except a matching "-->" tag. */
                while ( (p = memchr(buf + buf_index, '>', bufsize - buf_index)) ){
                    buf_index = p - buf + 1;
                    if ( p[-1] == '-' && p[-2] == '-' ) break;
                }
                if ( p ) {
                    /* Got the whole comment. Let's throw it away! :) */
                    token_start = buf_index;
                }
                else
                    buf_index = bufsize;
            }
#ifdef JS_SUPPORT
            else if ((buf_index + 9 < bufsize) && !g_strncasecmp(buf + buf_index, "<noscript>", 10)) {
                /* Comment: search for close of comment, skipping over
                 * everything except a matching "</noscript>" tag. */
                while ( (p = memchr(buf + buf_index, '>', bufsize - buf_index)) ){
                    buf_index = p - buf + 1;
                    if (!g_strncasecmp(p-10, "</noscript>", 11))
                        break;
                }
                if ( p ) {
                    /* Got the whole comment. Let's throw it away! :) */
                    token_start = buf_index;
                }
                else
                    buf_index = bufsize;
            }
#endif
            else {
                /* Tag: search end of tag (skipping over quoted strings) */
                html->CurrTagOfs = html->Start_Ofs + token_start;

                while ( buf_index < bufsize ) {
                    buf_index++;
                    buf_index += strcspn(buf + buf_index, ">\"'<");
                    if ( (ch = buf[buf_index]) == '>' ) {
                        break;
                    }
#if 0
                    else if ( ch == '"' || ch == '\'' ) {
                        /* Skip over quoted string */
                        buf_index++;
                        buf_index += strcspn(buf + buf_index,
                                            (ch == '"') ? "\"" : "'");
                        if (buf[buf_index] != ch){
                            buf_index += strcspn(buf + buf_index, ">");
                            
                            if ( buf[buf_index] == '>' ) {
                                /* Unterminated string value? Let's look ahead and test:
                                 * (<: unterminated, closing-quote: terminated) */
                                int offset = buf_index + 1;
                                offset += strcspn(buf + offset,"<");
                                if (!buf[offset]) {
                                    buf_index = offset;
                                }else {
                                    MSG_HTML("attribute lacks closing quote\n");
                                    break;
                                }
                            }
                        }
                    }
#else
                    else if ( ch == '"' || ch == '\'' ) {
                        /* Skip over quoted string */
                        buf_index++;
                        buf_index += strcspn(buf + buf_index,
                                            (ch == '"') ? "\">" : "'>");
                        if ( buf[buf_index] == '>' ) {
                            /* Unterminated string value? Let's look ahead and test:
                            * (<: unterminated, closing-quote: terminated) */
                            int offset = buf_index + 1;
                            offset += strcspn(buf + offset,
                                            (ch == '"') ? "\"<" : "'<");
                            if (buf[offset] == ch || !buf[offset]) {
                                buf_index = offset;
                            }
                            else {
                                MSG_HTML("attribute lacks closing quote\n");
                                break;
                            }
                        }
                    }
#endif
                    else if ( ch == '<') {
                          /* unterminated tag detected */
                          p = g_strndup(buf+token_start+1,
                                        strcspn(buf+token_start+1, " <"));
                          MSG_HTML("<%s> element lacks its closing '>'\n", p);
                          g_free(p);
                          --buf_index;
                          break;
                    }
                }
                if (buf_index < bufsize) {
                    buf_index++;
                    Html_process_tag(html, buf + token_start,
                                        buf_index - token_start);
                    token_start = buf_index;
                }
            }
        }
        else {
            /* A Word: search for whitespace or tag open */
            while (++buf_index < bufsize) {
                buf_index += strcspn(buf + buf_index, " <\n\r\t\f\v");
                if ( buf[buf_index] == '<' && (ch = buf[buf_index + 1]) &&
                        !isalpha(ch) && !strchr("/!?", ch))
                    continue;
                break;
            }
            if (buf_index < bufsize || Eof) {
                /* successfully found end of token */
                Html_process_word(html, buf + token_start,
                                buf_index - token_start);
                token_start = buf_index;
            }
        }
    }/*while*/

    a_Dw_page_flush(page);
    g_free(buf);

    return token_start;
}

/*
 * Process the newly arrived html and put it into the page structure.
 * (This function is called by Html_callback whenever there's new data)
 */
static void Html_write(mSpiderHtml *html, char *Buf, int BufSize, int Eof)
{
    DwPage *page;
    char completestr[32];
    int token_start;
    char *buf = Buf + html->Start_Ofs;
    int bufsize = BufSize - html->Start_Ofs;

    if ((page = DW_PAGE (html->dw)) == NULL) return;
    if (BufSize == 0) return;

    html->Start_Buf = Buf;
    html->Buf_Size = BufSize;
    token_start = Html_write_raw(html, buf, bufsize, Eof);
    html->Start_Ofs += token_start;

    if (html->dd) {
        snprintf (completestr, 32, "%s%.1f Kb",
            PBAR_PSTR(prefs.panel_size == 1),
            (float)html->Start_Ofs/1024);
        a_Progressbar_update(html->dd->bw, completestr, 1);
    }
}

/*
 * Finish parsing a HTML page
 * (Free html struct, close the client and update progressbar).
 */
static void Html_close(mSpiderHtml *html, int ClientKey)
{
   int si;
#if 0
#ifdef JS_SUPPORT
    jsstring code;
    jsobject *jsobj = NULL;
    jsscript_interpreter *interpreter = NULL;

    formid=0;
    jsobj = js_getdftdocobj((int)html->dd);
       if ( jsobj && 
        (interpreter = (jsscript_interpreter*)jsobj->jsinterpreter) ) {
        interpreter->valid=0;
        interpreter->point=NULL;
        if ( jsobj->onload ) {
            code.source = jsobj->onload;
            code.length = strlen(jsobj->onload);
            if ( interpreter->inuse ) {
                if ( js_eval(interpreter, &code) ) {
                    g_free(jsobj->onload);
                    jsobj->onload = NULL;
                }
            }
        }
       }
#endif
#endif
   //#if defined (DEBUG_LEVEL) && DEBUG_LEVEL >= 1
   //a_Dw_widget_print_tree (GTK_DW_VIEWPORT(html->dw->viewport)->child);
   //#endif

   /* force the close of elements left open (todo: not for XHTML) */
   while ((si = html->stack_top)) {
      if (html->stack[si].tag_idx != -1) {
         Html_tag_cleanup_at_close(html, html->stack[si].tag_idx);
      }
   }
   g_free(html->stack[0].tag_name); /* "none" */
   a_Dw_style_unref(html->stack[0].style); /* template style */

   g_free(html->stack);

   g_string_free(html->Stash, TRUE);
   g_free(html->SPCBuf);
   g_string_free(html->attr_data, TRUE);

   a_Doc_close_client(html->dd, ClientKey);
#if 0
   /* Remove this client from our active list */
   a_MGD_close_client (html->bw, ClientKey);
#endif

   /* Set progress bar insensitive */
   a_Progressbar_update(html->dd->bw, NULL, 0);

    if (html->logfont)
        DestroyLogFont(html->logfont);

   InvalidateRect(html->dd->docwin, NULL, TRUE);
   g_free(html);
   html = NULL;

}

/*
 * "Html_get_attr with default"
 * Call Html_get_attr() and strdup() the returned string.
 * If the attribute isn't found a copy of 'def' is returned.
 */
static char *Html_get_attr_wdef(mSpiderHtml *html,
                               const char *tag,
                               gint tagsize,
                               const char *attrname,
                               const char *def)
{
   const char *attrbuf = Html_get_attr(html, tag, tagsize, attrname);

   return attrbuf ? g_strdup(attrbuf) : g_strdup(def);
}


#ifdef JS_SUPPORT
static void JSfile_callback(int Op, CacheClient_t *Client)
{
    mSpiderDoc * dd;
    jsstring code;
    jsscript_interpreter *jsinterpreter = NULL;
    jsobject *jsobj = NULL;

      if ( Op == CA_Close ) { 
        code.source = (char*)Client->Buf;
        code.length = Client->BufSize;
        dd = Client->CbData;

        jsobj = js_getdftdocobj((int)dd);
        if ( !jsobj || !(jsinterpreter = jsobj->jsinterpreter) ) {
            return;
        }

          if ( jsinterpreter->inuse ) {
            jsinterpreter->dealing = 1;
            js_eval(jsinterpreter, &code);
            jsinterpreter->dealing = 0;
          }
     }

}

static int GetJSFile(char *jsurl, gpointer data)
{
     mSpiderUrl *url = NULL;
     mSpiderWeb *Web = NULL;
     url = a_Url_new(jsurl, NULL, 0, 0, 0);
     Web = a_Web_new(url);
     Web->flags |= WEB_Download;
     a_Url_free(url);
     return a_Capi_open_url(Web, JSfile_callback, data);
}    
#endif
#ifdef ENABLE_FLASH
static void flash_callback(int Op, CacheClient_t *Client)
{
   char *source;
   int length;
   HWND flash;

   if ( Op == CA_Close )
   {
       source=(char*)Client->Buf;
       length = Client->BufSize;
       flash = (HWND )Client->CbData;
       if (length>=4&&source[3]<=5)
       {
            PrepareFlashPlay (source,length,flash);
       }
   }
}

static int GetFlashFile(char *url,HWND flash)
{
       mSpiderUrl *flashurl = NULL;
       mSpiderWeb *Web = NULL;
     
       flashurl = a_Url_new(url, NULL, 0, 0, 0);
       Web = a_Web_new(flashurl);
       Web->flags |= WEB_Download;
       a_Url_free (flashurl);
    return a_Capi_open_url( Web, flash_callback, flash);
}    
#endif
