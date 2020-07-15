/* * File: auth.c *
 *
 * Copyright (C) 2005-2006 Feynman Software
 *
 * This program is g_free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "mgdconfig.h"
#include "auth.h"
#include "msg.h"
#include "klist.h"
#include "list.h"
#include "cache.h"
#include "web.h"
#include "interface.h"
#include "nav.h"
#include "history.h"
#include "misc.h"

void Auth_confirmed(mSpiderDoc *dd);
void Auth_refused(mSpiderDoc *dd);
void Auth_load_authenticated(mSpiderDoc *dd, mSpiderUrl *url);

typedef struct _Realm Realm;

struct _Realm {
   mSpiderUrl *base_url;
   GString *auth;
};

static Realm *realms = NULL;
static gint num_realms;

void a_Auth_byrealm(GString *auth_realm, mSpiderUrl *NewUrl, mSpiderDoc *dd)
{
   /* we already have a password dialogue open */
   if (dd->auth_await_url)
      return;

   dd->auth_await_url = a_Url_dup(NewUrl);
   auth_realm = g_string_append(auth_realm, "\n");
#if 0
   a_Interface_passwd_dialog(dd->bw, auth_realm->str,
                             Auth_confirmed, dd,
                             Auth_refused, dd);
#endif
}

GString *a_Auth_byurl(mSpiderUrl *n)
{
   gchar *offset;
   int i, longest = -1, len = 0, longlen = 0;
   gchar *ptr;

   if (!n)
      return NULL;

   for (i = 0; i < num_realms; i++) {
      ptr = URL_STR(realms[i].base_url);
      offset = strrchr(ptr, '/');
      if (!offset)
         offset = ptr + strlen(ptr);
      if (strncmp(URL_STR(n), ptr, (char*) offset - (char*) ptr) == 0) {
         len = (gchar *) offset - (gchar *) ptr;
         if (longlen <= len) {
            longlen=len;
            longest=i;
         }
      }
   }
   return longest == -1 ? NULL : realms[longest].auth;
}
void Auth_confirmed(mSpiderDoc *dd)
{
#if 0
   static gint realms_max = 16;
   mSpiderUrl *NewUrl;
   GString *new_auth;
   GtkEntry *uentry,*pentry;
   GString *up = g_string_new("");
   gchar *encoded;

   if (!dd->auth_await_url)
      return;

   NewUrl = dd->auth_await_url;

   /* are we still trying to browse the url that wants authentication? */

   if (a_Nav_stack_size(dd) != 0
         && a_Url_cmp(a_History_get_url(NAV_TOP(dd)), NewUrl) != 0) {
      /* free the url here, maybe ? */
      a_Url_free(NewUrl);
      dd->auth_await_url = NULL;
      return;
   }

   /* fetch and encode authorization */
   uentry = GTK_ENTRY(dd->bw->passwd_dialog_uentry);
   pentry = GTK_ENTRY(dd->bw->passwd_dialog_pentry);
   /* ... */
   g_string_sprintfa(up, "%s:%s", gtk_entry_get_text(uentry),
                     gtk_entry_get_text(pentry));
   new_auth = g_string_new("");
   encoded = a_Misc_encode_base64(up->str);
   g_string_sprintfa(new_auth, "Basic %s", encoded);
   g_free(encoded);
   a_List_add(realms, num_realms, realms_max);
   realms[num_realms].base_url = a_Url_dup(NewUrl);
   realms[num_realms].auth = new_auth;
   num_realms++;
   Auth_load_authenticated(dd, NewUrl);
   dd->auth_await_url = NULL;
#endif
}
void Auth_refused(mSpiderDoc *dd)
{
   mSpiderUrl *NewUrl;

   if (!dd->auth_await_url)
      return;
   NewUrl = dd->auth_await_url;
   g_return_if_fail(NewUrl);
   a_Url_free(NewUrl);
   dd->auth_await_url = NULL;
}

void Auth_load_authenticated(mSpiderDoc *dd, mSpiderUrl *NewUrl)
{
   a_Url_set_flags(NewUrl, URL_FLAGS(NewUrl) | URL_E2EReload | URL_RealmAccess);
   a_Nav_push(dd, NewUrl);
   a_Url_free(NewUrl);
}
  
void a_Auth_freeall(void)
{
}

