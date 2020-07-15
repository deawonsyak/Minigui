/*
 * File: interface.c
 *
 * Copyright (C) 2005-2006 Feynman Software
 * Copyright (C) 1997 Raph Levien <raph@acm.org>
 * Copyright (C) 1999 Sammy Mannaert <nstalkie@tvd.be>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */


#include <stdio.h>
#include <ctype.h>
#include <glib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "mgdconfig.h"
#ifdef _NOUNIX_
#include <sys/times.h>
#else 
#include <sys/time.h>
#endif
#include <fcntl.h>

#include "msg.h"
#include "list.h"
#include "misc.h"
#include "emspider.h"
#include "history.h"
#include "nav.h"
#include "io/url_io.h"
#include "io/io.h"
#include "interface.h"
#include "prefs.h"
#include "url.h"
#include "doc.h"
#include "dw_widget.h"
#include "dw_viewport.h"
#include "dw_container.h"
#include "progressbar.h"
#include "emspider.h"
#include "mgwidget.h"

#include "linktrave.h"

#include "debug.h"
#include "js.h"

#include "mspider.h"

#define DEBUG_LEVEL 0


/*
 * Local Data
 */
/* BrowserWindow holds all the widgets (and perhaps more)
 * for each new_browser.*/
GList *browser_window = NULL;

BrowserWindow *current_browser = NULL; // this is the only browser



/*
 * Initialize global data
 */
void a_Interface_init(void)
{
   browser_window = NULL;
   current_browser = NULL;
}

/*
 * Stop all active connections in the browser window (except downloads)
 */

void a_Interface_stop(mSpiderDoc *dd)
{
   DEBUG_MSG(3, "a_Interface_stop: hi!\n");

   /* Remove root clients */
   while ( dd->NumRootClients ) {
      a_Cache_stop_client(dd->RootClients[0]);
      a_List_remove(dd->RootClients, 0, dd->NumRootClients);
   }
   /* Remove image clients */
   while ( dd->NumImageClients ) {
      a_Cache_stop_client(dd->ImageClients[0]);
      a_List_remove(dd->ImageClients, 0, dd->NumImageClients);
   }
}

/*
 * Empty RootClients, ImageClients and PageUrls lists and
 * reset progress bar data.
 */
void a_Interface_clean(mSpiderDoc *dd)
{
   g_return_if_fail ( dd != NULL );

   while ( dd->NumRootClients )
      a_List_remove(dd->RootClients, 0, dd->NumRootClients);

   while ( dd->NumImageClients )
      a_List_remove(dd->ImageClients, 0, dd->NumImageClients);

   while ( dd->NumPageUrls ) {
      a_Url_free(dd->PageUrls[0].Url);
      a_List_remove(dd->PageUrls, 0, dd->NumPageUrls);
   }

   /* Zero image-progressbar data */
   dd->NumImages = 0;
   dd->NumImagesGot = 0;
}

/*=== Browser Window Interface Updating =====================================*/
/*
 * Remove the cache-client from the bw list
 * (client can be a image or a html page)
 */
void a_Interface_remove_client(mSpiderDoc *dd, gint ClientKey)
{
   gint i;
   gboolean Found = FALSE;

   for ( i = 0; !Found && i < dd->NumRootClients; ++i)
      if ( dd->RootClients[i] == ClientKey ) {
         a_List_remove(dd->RootClients, i, dd->NumRootClients);
         Found = TRUE;
      }

   for ( i = 0; !Found && i < dd->NumImageClients; ++i)
      if ( dd->ImageClients[i] == ClientKey ) {
         a_List_remove(dd->ImageClients, i, dd->NumImageClients);
         dd->NumImagesGot++;
         Found = TRUE;
      }

//   a_Interface_set_button_sens(bw);
}

/*
 * Remove the cache-client from the bw list
 * (client can be a image or a html page)
 */
void a_Interface_close_client(mSpiderDoc *dd, gint ClientKey)
{
   gchar numstr[32]={0};

   a_Interface_remove_client(dd, ClientKey);

   /* --Progress bars stuff-- */
   g_snprintf(numstr, 31, "%s%d of %d", PBAR_ISTR(prefs.panel_size == 1),
              dd->NumImagesGot, dd->NumImages);
   a_Progressbar_update(dd->bw, numstr,
                        (dd->NumImagesGot == dd->NumImages) ? 0 : 1 );
}

/*
 * Add a reference to the cache-client in the browser window's list.
 * This helps us keep track of which are active in the window so that it's
 * possible to abort them.
 * (Root: Flag, whether a Root URL or not)
 */
void a_Interface_add_client(mSpiderDoc *dd, gint Key, gint Root)
{
   gint nc;
   gchar numstr[32];

   g_return_if_fail ( dd != NULL );

   if ( Root ) {
      nc = dd->NumRootClients;
      a_List_add(dd->RootClients, nc, dd->MaxRootClients);
      dd->RootClients[nc] = Key;
      dd->NumRootClients++;
      //a_Interface_set_button_sens(bw);
   } else {
      nc = dd->NumImageClients;
      a_List_add(dd->ImageClients, nc, dd->MaxImageClients);
      dd->ImageClients[nc] = Key;
      dd->NumImageClients++;
      dd->NumImages++;
      //a_Interface_set_button_sens(bw);

      /* --Progress bar stuff-- */
      g_snprintf(numstr, 32, "%s%d of %d", PBAR_ISTR(prefs.panel_size == 1),
                 dd->NumImagesGot, dd->NumImages);
      a_Progressbar_update(dd->bw, numstr, 1);
   }
}

/*
 * Add an URL to the browser window's list.
 * This helps us keep track of page requested URLs so that it's
 * possible to stop, abort and reload them.)
 *   Flags: Chosen from {BW_Root, BW_Image, BW_Download}
 */
void a_Interface_add_url(mSpiderDoc *dd, const mSpiderUrl *Url, gint Flags)
{
   gint nu, i;
   gboolean found = FALSE;

   g_return_if_fail ( dd != NULL && Url != NULL );

   nu = dd->NumPageUrls;
   for ( i = 0; i < nu; i++ ) {
      if ( !a_Url_cmp(Url, dd->PageUrls[i].Url) ) {
         found = TRUE;
         break;
      }
   }
   if ( !found ) {
      a_List_add(dd->PageUrls, nu, dd->MaxPageUrls);
      dd->PageUrls[nu].Url = a_Url_dup(Url);
      dd->PageUrls[nu].Flags = Flags;
      dd->NumPageUrls++;
   }

}


/*
 * Remove a single browser window. This includes all its open childs,
 * freeing all resources associated with them, and exiting gtk
 * if no browser windows are left.
 */
gboolean a_Interface_quit(BrowserWindow *bw)
{
#ifdef ENABLE_LINKTRAVE
   a_Release_LinktraveManage ((LinktraveManage*)bw->linktrave_manage);
#endif

    browser_window = g_list_remove (browser_window, (gpointer)bw); 

#ifdef JS_SUPPORT
	if (bw->jsobj) {
		g_free (bw->jsobj);
        bw->jsobj = NULL;
	}
#endif
   g_free(bw);

   return FALSE;

}

/*
 * Open an url string.
 * The URL is not sent "as is", illegal chars are ripped out,
 * then it's fully parsed by a_Url_new().
 */
void a_Interface_open_url_string(gchar *text, mSpiderDoc *dd)
{

   gchar *new_text;
   mSpiderUrl *url;


   if (text && *text) {
      /* Filter URL string */
      new_text = a_Url_string_strip_delimiters(text);

	  printf("a_Interface_open_url_string new_text:%s\n", new_text);
      url = a_Url_new(new_text, NULL, 0, 0, 0);
      if (url) {
         a_Nav_push(dd, url);
         a_Url_free(url);
      }
      g_free(new_text);
   }

}


/*=== Browser Window Interface Construction =================================*/
/*
 * Clear a text entry
 */
void Interface_entry_clear (mSpiderDoc *dd)
{
/* w l
    if (dd->bw->hwnd_location != -1)
        SendMessage (dd->bw->hwnd_location, MSG_SETTEXT, 0, (LPARAM)"");
*/
    gchar * url;
        
    url =  a_Interface_get_location_text(dd);

    if (url)
    {
            a_Interface_open_url_string (url, dd);
            g_free (url);
    }

}


BrowserWindow* a_BrowserWindow_new (HWND hwnd_parent)
{
    BrowserWindow* bw;

    bw = g_new0 (BrowserWindow, 1);
    browser_window = g_list_append (browser_window, bw);
    bw->main_window = hwnd_parent;
    bw->sens_idle_id = 0;
    bw->dd = NULL;

    return bw;
}

/*
 * Set the title of the browser window to start with "mSpider: "
 * prepended to it.
 */
void a_Interface_set_page_title(mSpiderDoc *dd, char *title)
{
    GString *buf;

    g_return_if_fail (dd != NULL && title != NULL);

    buf = g_string_new ("");
    g_string_sprintfa (buf, "%s - FMSoft mSpider", title);
    SetWindowCaption (dd->bw->main_window, buf->str);
    g_string_free (buf, TRUE);
}

/*
 * Set location entry's text
 */
void a_Interface_set_location_text(mSpiderDoc *dd, char *text)
{
    if (dd->bw->CB_SET_LOCATION)
        (*(dd->bw->CB_SET_LOCATION))(dd->bw->main_window, text);
}

/*
 * Get location entry's text
 */
gchar *a_Interface_get_location_text(mSpiderDoc *dd)
{
    gchar *str = NULL;

    if (dd->bw->CB_GET_LOCATION) {
        str = (*(dd->bw->CB_GET_LOCATION))(dd->bw->main_window);
    }
    return str;
}

/*
 * Reset images and text progress bars
 */
void a_Interface_reset_progress_bars(mSpiderDoc *dd)
{
    a_Progressbar_update(dd->bw, "", 0);
}

/*
 * Set the status string on the bottom of the mspider window.
 */
void a_Interface_msg (mSpiderDoc *dd, const char *format, ... )
{
   static char msg[1024];
   va_list argp;

   if (dd && dd->bw && dd->bw->CB_SET_STATUS) {
      va_start(argp, format);
      vsnprintf(msg, 1024, format, argp);
      va_end(argp);
       (*(dd->bw->CB_SET_STATUS))(dd->bw->main_window, msg);
      dd->status_is_link = 0;
   }
}

/*
 * Update the bug-meter button for detected page errors.
 */
void a_Interface_bug_meter_update(mSpiderDoc *dd, gint num_err)
{
}

/*
 * Close and free every single browser_window (called at exit time)
 */
static void bw_list_foreach (gpointer data, gpointer user_data)
{
    BrowserWindow * bw = (BrowserWindow *)data;
    DestroyMainWindow(bw->main_window);
    a_Interface_quit(bw);
}
 
void a_Interface_quit_all(void)
{
    g_list_foreach (browser_window, bw_list_foreach, NULL);  
    g_list_free (browser_window);
}

/* 
 * Returns a newly allocated string holding a search url generated from 
 * a string of keywords (separarated by blanks) and prefs.search_url.
 * Any '"' or '+' in the keywords are escaped. 
 */
static gchar *Interface_make_search_url(const gchar *str)
{
   gchar *p, *keys, *new_str;

   keys = g_new(gchar, 3*strlen(str)+1);

   for (p = keys; *str; str++) {
      switch(*str) {
      case ' ':
         *p++ = '+';
         break;
      case '"':
         *p++ = '%';
         *p++ = '2';
         *p++ = '2';
         break;
      case '+':
         *p++ = '%';
         *p++ = '2';
         *p++ = 'B';
         break;
      default:
         *p++ = *str;
      }
   }
   *p = '\0';

   new_str = g_strdup_printf(prefs.search_url, keys);
   g_free(keys);

   return new_str;
}

/*
 * Create and show the "Open File" dialog
 */
void a_Interface_openfile_dialog(mSpiderDoc *dd)
{
}


/*
 * Scan Url and return a local-filename suggestion for saving
 */
static char *Interface_make_save_name(const mSpiderUrl *url)
{
   gchar *FileName;

   if ((FileName = strrchr(URL_PATH(url), '/')))
      return g_strndup(++FileName, MIN(strlen(FileName), 64));
   return g_strdup("");
}


/*
 * A general purpose message window.
 */
void a_Interface_message_window(const char *title, const char *format, ... )
{
}


