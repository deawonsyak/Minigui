/*
 * File: web.c
 *
 * Copyright (C) 2005-2006 Feynman Software
 * Copyright 2000 Jorge Arellano Cid <jcid@mspider.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include <stdio.h>
#include <stdlib.h>
#include <glib.h>

#include "msg.h"
#include "browser.h"
#include "prefs.h"
#include "interface.h"
#include "nav.h"
#include "dw_widget.h"
#include "dw_viewport.h"
#include "web.h"
#include "dw_style.h"
#include "io/io.h"
#include "io/mime.h"

#if 0
#include "dw_gtk_scrolled_window.h"
#include "dw_embed_gtk.h"
#endif

#define DEBUG_LEVEL 5
#include "debug.h"

/*
 * Local data
 */
static GSList *ValidWebs = NULL;  /* Active web structures list; it holds
                                   * pointers to mSpiderWeb structures. */

/*
 * Given the MIME content type, and a fd to read it from,
 * this function connects the proper MIME viewer to it.
 */
gint a_Web_dispatch_by_type (const char *Type, mSpiderWeb *Web,
                             CA_Callback_t *Call, void **Data)
{
   DwWidget *dw = NULL;
   DwStyle style_attrs, *style;
   DwStyleFont font;

   DEBUG_MSG(1, "a_Web_dispatch_by_type\n");

   g_return_val_if_fail(Web->dd != NULL, -1);

   if (Web->flags & WEB_RootUrl) 
   {
      /* We have RootUrl! */
      dw = a_Mime_set_viewer(Type, Web, Call, Data);
      if (dw == NULL)
         return -1;

      /* Set a style for the widget */
      if (dw->style == NULL) {
        font.name = prefs.vw_fontname; /* must be defined */
        font.size = (int)(12.0 * prefs.font_factor+0.5);
        font.weight = 400;
        font.style = DW_STYLE_FONT_STYLE_NORMAL;
        font.charset = DEF_CHARSET;
        a_Dw_style_init_values (&style_attrs, Web->dd->docwin);
        a_Dw_style_box_set_val (&style_attrs.margin, 5);
        style_attrs.font = a_Dw_style_font_new (&font);
        style_attrs.color =
            a_Dw_style_color_new (prefs.text_color, Web->dd->docwin);
        style_attrs.background_color =
            a_Dw_style_color_new (prefs.bg_color, Web->dd->docwin);
        style = a_Dw_style_new (&style_attrs, Web->dd->docwin);
        a_Dw_widget_set_style (dw, style);
        a_Dw_style_unref (style);
      }

      a_Dw_viewport_add_dw (Web->dd->viewport, dw);
#if 0

      a_Dw_gtk_scrolled_window_set_dw(
         GTK_DW_SCROLLED_WINDOW(Web->bw->docwin), dw);
#endif
      if (Web->url != NULL)
      {
          if (URL_POSX(Web->url) || URL_POSY(Web->url)) {
#if 0
            a_Dw_gtk_scrolled_window_set_scrolling_position(
               GTK_DW_SCROLLED_WINDOW(Web->bw->docwin),
               URL_POSX(Web->url), URL_POSY(Web->url));
#endif
          } 
          else {
            gchar *pf = a_Url_decode_hex_str(URL_FRAGMENT_(Web->url));
#if 0
            a_Dw_gtk_scrolled_window_set_anchor(
               GTK_DW_SCROLLED_WINDOW(Web->bw->docwin), pf);
#endif
            g_free(pf);
          }
      }

      if (NULL == Web->dd->parent)/*top level mSpiderDoc */
      {
      /* Clear the title bar for pages without a <TITLE> tag */
        a_Interface_set_page_title(Web->dd, "");
        a_Interface_set_location_text(Web->dd, URL_STR(Web->url));
        a_Interface_reset_progress_bars(Web->dd);
      }
      /* Reset the bug meter */
#if 0
      a_Interface_bug_meter_update(Web->bw, 0);
#endif
      /* Let the Nav module know... */
      a_Nav_expect_done(Web->dd);
   } else {
      /* A non-RootUrl. At this moment we only handle image-children */
      if (!g_strncasecmp(Type, "image/", 6))
         dw = a_Mime_set_viewer(Type, Web, Call, Data);
   }

   if (!dw) {
      _MSG_HTTP("unhandled MIME type: \"%s\"\n", Type);
   }
   return (dw ? 1 : -1);
}

/*
 * Allocate and set safe values for a mSpiderWeb structure
 */
mSpiderWeb* a_Web_new(const mSpiderUrl *url)
{
   mSpiderWeb *web= g_new(mSpiderWeb, 1);

   web->url = a_Url_dup(url);
   web->dd = NULL;
   web->flags = 0;
   web->Image = NULL;
   web->page = NULL;
   web->stream  = NULL;
   web->SavedBytes = 0;
   //web->refcount = 1;

   ValidWebs = g_slist_append(ValidWebs, (gpointer)web);
   return web;
}
#if 0
mSpiderWeb* a_Web_ref (mSpiderWeb *web)
{
   if (!web) return NULL;
    web->refcount++;
    return web;
}
#endif
/*
 * Validate a mSpiderWeb pointer
 */
gint a_Web_valid(mSpiderWeb *web)
{
   return (g_slist_find(ValidWebs, web) != NULL);
}

/*
 * Deallocate a mSpiderWeb structure
 */
void a_Web_free(mSpiderWeb *web)
{
   if (!web) return;
   //if ((--(web->refcount)) != 0)
     //  return;
  // printf("the web is freed!\n");
   if (web->url) {
      a_Url_free(web->url);
   }
   if (web->Image)
#if 1
      a_Image_unref(web->Image);
     //printf("a_Image_unref is called !\n");
#else
//MSG("a_Image_unref is called!\n");
#endif
   ValidWebs = g_slist_remove(ValidWebs, (gpointer)web);
   g_free(web);
}

