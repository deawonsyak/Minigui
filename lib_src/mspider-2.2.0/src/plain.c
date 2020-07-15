/*
 * File: plain.c
 *
 * Copyright (C) 2005-2006 Feynman Software
 * Copyright (C) 1997 Raph Levien <raph@acm.org>
 * Copyright (C) 1999 James McCollough <jamesm@gtwn.net>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

/*
 * Module for decoding a text/plain object into a gtk_page widget.
 */

#include <string.h>     /* for memcpy and memmove */

#include "prefs.h"
#include "dw_page.h"
#include "cache.h"
#include "browser.h"
#include "web.h"
#include "interface.h"
#include "progressbar.h"
#include "misc.h"
#include "history.h"
#include "nav.h"


typedef struct _mSpiderPlain {
   DwWidget *dw;
   size_t Start_Ofs;    /* Offset of where to start reading next */
   DwStyle *style;
   mSpiderDoc *dd;
   gint state;
} mSpiderPlain;

/* FSM states */
enum {
   ST_SeekingEol,
   ST_Eol,
   ST_Eof
};

/*
 * Forward declarations
 */
static void Plain_write(mSpiderPlain *plain, void *Buf, guint BufSize, gint Eof);
static void Plain_callback(int Op, CacheClient_t *Client);

/* exported function */
DwWidget* a_Plain_text(const char *type, void *P, CA_Callback_t *Call,
                       void **Data);

#if 0
/*
 * Popup the page menu ("button_press_event" callback of the viewport)
 */
static int Plain_page_menu(GtkWidget *viewport, GdkEventButton *event,
                           BrowserWindow *bw)
{
   if (event->button == 3) {
      a_Menu_popup_set_url(bw, a_History_get_url(NAV_TOP(bw)));
      gtk_menu_popup(GTK_MENU(bw->menu_popup.over_page), NULL, NULL,
                     NULL, NULL, event->button, event->time);
      return TRUE;
   } else
      return FALSE;
}
#endif

/*
 * Create and initialize a new mSpiderPlain structure.
 */
static mSpiderPlain *Plain_new(mSpiderDoc *dd)
{
   mSpiderPlain *plain;
   DwPage *page;
   DwStyle style_attrs;
   DwStyleFont font_attrs;

   plain = g_new(mSpiderPlain, 1);
   plain->state = ST_SeekingEol;
   plain->Start_Ofs = 0;
   plain->dd = dd;
   plain->dw = a_Dw_page_new();
   page = (DwPage *) plain->dw;

   /* Create the font and attribute for the page. */
   font_attrs.name = prefs.fw_fontname;
   font_attrs.size = (int)(12.0 * prefs.font_factor + 0.5);
   font_attrs.weight = 400;
   font_attrs.style = DW_STYLE_FONT_STYLE_NORMAL;
   font_attrs.charset = DEF_CHARSET;

   a_Dw_style_init_values (&style_attrs, plain->dd->bw->main_window);
   style_attrs.font = a_Dw_style_font_new (&font_attrs);
   style_attrs.color = a_Dw_style_color_new (prefs.text_color,
                                             plain->dd->bw->main_window);
   plain->style = a_Dw_style_new (&style_attrs,
                                  plain->dd->bw->main_window);

#if 0
   /* The context menu */
   gtk_signal_connect_while_alive
      (GTK_OBJECT(GTK_BIN(plain->bw->docwin)->child),"button_press_event",
       GTK_SIGNAL_FUNC(Plain_page_menu), (gpointer)plain->bw,
       GTK_OBJECT (page));
#endif

   return plain;
}

/*
 * Set callback function and callback data for "text/" MIME major-type.
 */
DwWidget* a_Plain_text(const char *type, void *P, CA_Callback_t *Call,
                       void **Data)
{
   mSpiderWeb *web = P;
   mSpiderPlain *plain = Plain_new(web->dd);

   *Call = (CA_Callback_t)Plain_callback;
   *Data = (void*)plain;

   return plain->dw;
}

/*
 * This function is a cache client
 */
static void Plain_callback(int Op, CacheClient_t *Client)
{
   mSpiderPlain *plain= Client->CbData;
   DwPage *page = (DwPage *)plain->dw;

   if ( Op ) {
      /* Do the last line: */
      if (plain->Start_Ofs < Client->BufSize)
         Plain_write(plain, Client->Buf, Client->BufSize, 1);
      /* remove this client from our active list */
      a_Doc_close_client(plain->dd, Client->Key);
      /* set progress bar insensitive */
      a_Progressbar_update(plain->dd->bw, NULL, 0);

      a_Dw_style_unref (plain->style);
      g_free(plain);

   } else {
      Plain_write(plain, Client->Buf, Client->BufSize, 0);
   }

   a_Dw_page_flush(page);
}

/*
 * Here we parse plain text and put it into the page structure.
 * (This function is called by Plain_callback whenever there's new data)
 */
static void Plain_write(mSpiderPlain *plain, void *Buf, guint BufSize, gint Eof)
{
   DwPage *page = (DwPage *)plain->dw;
   char *Start;
   char *data;
   guint i, len, MaxBytes;

   Start = (char*)Buf + plain->Start_Ofs;
   MaxBytes = BufSize - plain->Start_Ofs;
   i = len = 0;
   while ( i < MaxBytes ) {
      switch ( plain->state ) {
      case ST_SeekingEol:
         if ( Start[i] == '\n' || Start[i] == '\r' )
            plain->state = ST_Eol;
         else {
            ++i; ++len;
         }
         break;
      case ST_Eol:
         data = g_strndup(Start + i - len, len);
         a_Dw_page_add_text(page, a_Misc_expand_tabs(data), plain->style);
         g_free(data);
         a_Dw_page_add_parbreak(page, 0, plain->style);
         if ( Start[i] == '\r' && Start[i + 1] == '\n' ) ++i;
         if ( i < MaxBytes ) ++i;
         plain->state = ST_SeekingEol;
         len = 0;
         break;
      }
   }
   plain->Start_Ofs += i - len;
   if ( Eof && len ) {
      data = g_strndup(Start + i - len, len);
      a_Dw_page_add_text(page, a_Misc_expand_tabs(data), plain->style);
      g_free(data);
      a_Dw_page_add_parbreak(page, 0, plain->style);
      plain->Start_Ofs += len;
   }

   InvalidateRect(plain->dd->docwin, NULL, TRUE);

   if ( plain->dd ) {
      gchar completestr[32];
      g_snprintf(
         completestr, 32, "%s%.1f Kb",
         PBAR_PSTR(prefs.panel_size == 1),
         (float)plain->Start_Ofs/1024);
      a_Progressbar_update(plain->dd->bw, completestr, 1);
   }
}
