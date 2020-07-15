/*
 * File: nav.c
 *
 * Copyright (C) 2005-2006 Feynman Software
 * Copyright (C) 1999 James McCollough <jamesm@gtwn.net>
 * Copyright (C) 2000, 2001, 2002 Jorge Arellano Cid <jcid@mspider.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

/* Support for a navigation stack */

#include <stdio.h>
#include "msg.h"
#include "list.h"
#include "nav.h"
#include "dw_viewport.h"
#include "history.h"
#include "web.h"
#include "prefs.h"
#include "doc.h"

#define DEBUG_LEVEL 3
#include "debug.h"
#include "interface.h"
#include "capi.h"
#include "mspider.h"


extern int stop_flag;

/*
 * Initialize the navigation structure with safe values
 */
void a_Nav_init(mSpiderDoc *dd)
{
   dd->nav_stack_size = 0;
   dd->nav_stack_size_max = 32;
   dd->nav_stack = NULL;
   dd->nav_stack_ptr = -1;
   dd->nav_expecting = FALSE;
   dd->nav_expect_url = NULL;
}

/*
 * Free memory used by this module
 */
void a_Nav_free(mSpiderDoc *dd)
{
   a_Nav_cancel_expect(dd);
   g_free(dd->nav_stack);
}


/* Navigation stack methods ------------------------------------------------ */

/*
 * Return current nav_stack pointer [0 based; -1 = empty]
 */
gint a_Nav_stack_ptr(mSpiderDoc *dd)
{
   return dd->nav_stack_ptr;
}

/*
 * Move the nav_stack pointer
 */
static void Nav_stack_move_ptr(mSpiderDoc *dd, gint offset)
{
   gint nptr;

   g_return_if_fail (dd != NULL);
   if (offset != 0) {
      nptr = dd->nav_stack_ptr + offset;
      g_return_if_fail (nptr >= 0 && nptr < dd->nav_stack_size);
      dd->nav_stack_ptr = nptr;
   }
}

/*
 * Return size of nav_stack [1 based]
 */
gint a_Nav_stack_size(mSpiderDoc *dd)
{
   return dd->nav_stack_size;
}

/*
 * Add an URL-index in the navigation stack.
 */
static void Nav_stack_add(mSpiderDoc *dd, gint idx)
{
   g_return_if_fail (dd != NULL);

   ++dd->nav_stack_ptr;
   if ( dd->nav_stack_ptr == dd->nav_stack_size) {
      a_List_add(dd->nav_stack, dd->nav_stack_size, dd->nav_stack_size_max);
      ++dd->nav_stack_size;
   } else {
      dd->nav_stack_size = dd->nav_stack_ptr + 1;
   }
   dd->nav_stack[dd->nav_stack_ptr] = idx;
}

/*
 * Remove an URL-index from the navigation stack.
 */
static void Nav_stack_remove(mSpiderDoc *dd, gint idx)
{
   gint sz = a_Nav_stack_size(dd);

   g_return_if_fail (dd != NULL && idx >=0 && idx < sz);

   for (  ; idx < sz - 1; ++idx)
      dd->nav_stack[idx] = dd->nav_stack[idx + 1];
   if ( dd->nav_stack_ptr == --dd->nav_stack_size )
      --dd->nav_stack_ptr;
}

/*
 * Remove equal adyacent URLs at the top of the stack.
 * (It may happen with redirections)
 */
static void Nav_stack_clean(mSpiderDoc *dd)
{
   gint i;

   g_return_if_fail (dd != NULL);

   if ((i = a_Nav_stack_size(dd)) >= 2 &&
       dd->nav_stack[i-2] == dd->nav_stack[i-1])
         Nav_stack_remove(dd, i - 1);
}

/* General methods --------------------------------------------------------- */

void a_Nav_new_url (mSpiderDoc *dd, mSpiderUrl* url)
{
   gint idx;

   g_return_if_fail(dd != NULL);

   idx = a_History_add_url (url);
   Nav_stack_add (dd, idx);
}

/*
 * Create a mSpiderWeb structure for 'url' and ask the cache to send it back.
 *  - Also set a few things related to the browser window.
 * This function requests the page's root-URL; images and related stuff
 * are fetched directly by the HTML module.
 */
static void Nav_open_url (mSpiderDoc *dd, const mSpiderUrl *url, gint offset)
{
   mSpiderUrl *old_url = NULL;
   gboolean MustLoad;
   gchar * loc_txt;
   mSpiderWeb *Web;
   gint ClientKey;
   gboolean ForceReload = (URL_FLAGS(url) & URL_E2EReload);

   //a_Interface_set_location_text(dd, URL_STR_(url));
   /* Get the url of the current page */
   if ( a_Nav_stack_ptr(dd) != -1 )
      old_url = a_History_get_url(NAV_TOP(dd));

   Nav_stack_move_ptr(dd, offset);

   /* Page must be reloaded, if old and new url (without anchor) differ */
   MustLoad = ForceReload || !old_url;
   if (old_url){
      MustLoad |= a_Url_cmp(old_url, url);
      
      loc_txt = a_Interface_get_location_text(dd);
    if (loc_txt) {
	    MustLoad |= strcmp(URL_STR(old_url), loc_txt);
    }
   }


   if (MustLoad) {

      a_Doc_stop(dd);
      a_Doc_clean(dd);
#if 0
      a_Menu_pagemarks_new(bw);
#endif
      Web = a_Web_new(url);
      Web->dd = dd;
      Web->flags |= WEB_RootUrl;

      if ((ClientKey = a_Capi_open_url(Web, NULL, NULL)) != 0) {
         a_Doc_add_client(dd, ClientKey, 1);
         a_Doc_add_url(dd, url, WEB_RootUrl);
      }
   }

   /* Jump to #anchor position */
   if (URL_FRAGMENT_(url)) {
      /* todo: push on stack */
      gchar *pf = a_Url_decode_hex_str(URL_FRAGMENT_(url));
      a_Dw_viewport_set_anchor((DwViewport*)dd->viewport, pf); 
      g_free(pf);
   }

}

/*
 * Send the browser back to previous page
 */
void a_Nav_back(mSpiderDoc *dd)
{
   gint idx = a_Nav_stack_ptr(dd);

   a_Nav_cancel_expect(dd);
   if ( --idx >= 0 ){
      a_Interface_msg(dd, "");
      Nav_open_url(dd, a_History_get_url(NAV_IDX(dd,idx)), -1);
   }
}

/*
 * Send the browser to next page in the history list
 */
void a_Nav_forw(mSpiderDoc *dd)
{
   gint idx = a_Nav_stack_ptr(dd);

   a_Nav_cancel_expect(dd);
   if (++idx < a_Nav_stack_size(dd)) {
      a_Interface_msg(dd, "");
      Nav_open_url(dd, a_History_get_url(NAV_IDX(dd,idx)), +1);
   }
}


/*
 * This one does a_Nav_reload's job!
 */
static void Nav_reload(mSpiderDoc *dd)
{
   mSpiderUrl *url, *ReqURL;

   a_Nav_cancel_expect(dd);
   if ( a_Nav_stack_size(dd) ) {
      url = a_History_get_url(NAV_TOP(dd));
      ReqURL = a_Url_dup(a_History_get_url(NAV_TOP(dd)));
      /* Let's make reload be end-to-end */
      a_Url_set_flags(ReqURL, URL_FLAGS(ReqURL) | URL_E2EReload);
      /* This is an explicit reload, so clear the SpamSafe flag */
      a_Url_set_flags(ReqURL, URL_FLAGS(ReqURL) & ~URL_SpamSafe);
      Nav_open_url(dd, ReqURL, 0);
      a_Url_free(ReqURL);
   }
}
/*
 * Cancel the last expected url if present. The responsibility
 * for actually aborting the data stream remains with the caller.
 */
void a_Nav_cancel_expect(mSpiderDoc *dd)
{
   if (dd->nav_expecting) {
      if (dd->nav_expect_url) {
         a_Url_free(dd->nav_expect_url);
         dd->nav_expect_url = NULL;
      }
      dd->nav_expecting = FALSE;
   }
}

/*
 * Implement the RELOAD button functionality.
 * (We haven't defined it yet ;)
 */
void a_Nav_reload(mSpiderDoc *dd)
{
   mSpiderUrl *url;

   a_Nav_cancel_expect(dd);
   if ( a_Nav_stack_size(dd) ) {
      url = a_History_get_url(NAV_TOP(dd));
      if (URL_FLAGS(url) & URL_Post) {
         /* Attempt to repost data, let's confirm... */

      } else {
         Nav_reload(dd);
      }
   }
}
/*
 * We have an answer! Set things accordingly.
 */
void a_Nav_expect_done(mSpiderDoc *dd)
{
   gint idx;
   mSpiderUrl *url;

   g_return_if_fail(dd != NULL);

   if (dd->nav_expecting) {
      url = dd->nav_expect_url;
      /* unset E2EReload before adding this url to history */
      a_Url_set_flags(url, URL_FLAGS(url) & ~URL_E2EReload);
      idx = a_History_add_url(url);
      Nav_stack_add(dd, idx);

      a_Url_free(url);
      dd->nav_expect_url = NULL;
      dd->nav_expecting = FALSE;
   }
   Nav_stack_clean(dd);
}

/*
 * Remove top-URL from the navigation stack.
 * (Used to remove URLs that force redirection)
 */
void a_Nav_remove_top_url(mSpiderDoc *dd)
{
   g_return_if_fail (dd != NULL);

   /* Deallocate the URL a the top of the stack */
   Nav_stack_remove(dd, a_Nav_stack_size(dd) - 1);
}

/*
 * Make 'url' the current browsed page (upon data arrival)
 * - Set bw to expect the URL data
 * - Ask the cache to feed back the requested URL (via Nav_open_url)
 */
void a_Nav_push(mSpiderDoc *dd, const mSpiderUrl *nurl)
{
    mSpiderUrl *url;
    
    g_return_if_fail (dd != NULL);

    url = a_Url_dup(nurl);

    if (dd->nav_expecting && a_Url_cmp(dd->nav_expect_url, url) == 0 &&
        URL_STRCAMP_EQ(URL_FRAGMENT_(dd->nav_expect_url), URL_FRAGMENT_(url)) 
        && !(URL_FLAGS(url) & URL_RealmAccess)) { 
       /*
        * except BASIC Realm
        * we're already expecting that url (most probably a double-click)
        */
       a_Url_free(url);
       return;
    }

    stop_flag = 1;

    if (dd->parent == NULL) { /*top level widget shouldn't blank*/
        g_free((gchar*)url->target);
        url->target = NULL;
    }

    a_Nav_cancel_expect(dd);
    dd->nav_expect_url = a_Url_dup(url);
    dd->nav_expecting = TRUE;
    /* is this is a targeted URL, (re)name the document after it */
    if(URL_TARGET_(url))
      a_Doc_set_name(dd, (gchar *) URL_TARGET_(url));
   
   Nav_open_url (dd, url, 0);
   a_Url_free(url);
}

