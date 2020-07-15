/*
 * File: cache.c
 *
 * Copyright (C) 2005-2006 Feynman Software
 * Copyright 2000, 2001, 2002, 2003, 2004 Jorge Arellano Cid <jcid@mspider.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#ifndef __NOUNIX__
#include <unistd.h>
#endif
#include <ctype.h>              /* for tolower */
#include <sys/types.h>

#include <sys/stat.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>


#include "mgdconfig.h"
#include "msg.h"
#include "list.h"
#include "io/url_io.h"
#include "io/io.h"
#include "web.h"
#include "dicache.h"
#include "interface.h"
#include "nav.h"
#include "misc.h"
#include "doc.h"
#include "auth.h"
#ifdef ENABLE_COOKIES
#include "cookies.h"
#endif
#define NULLKey 0

#define DEBUG_LEVEL 5
#include "debug.h"

/*
 *  Local data types
 */

typedef struct {
   const mSpiderUrl *Url;      /* Cached Url. Url is used as a primary Key */
   const char *Type;         /* MIME type string */
   GString *Header;          /* HTTP header */
   GString *AuthRealm;        /* Authentication realm */
   const mSpiderUrl *Location; /* New URI for redirects */
   void *Data;               /* Pointer to raw data */
   size_t ValidSize,         /* Actually size of valid range */
          TotalSize,         /* Goal size of the whole data (0 if unknown) */
          BuffSize;          /* Buffer Size for unknown length transfers */
   guint Flags;              /* Look Flag Defines in cache.h */
   IOData_t *io;             /* Pointer to IO data */
   ChainLink *CCCQuery;      /* CCC link for querying branch */
   ChainLink *CCCAnswer;     /* CCC link for answering branch */
   time_t Expires;           /* when this data should be reloaded */
} CacheData_t;


/*
 *  Local data
 */
/* A hash for cached data. Holds pointers to CacheData_t structs */
static GHashTable *CacheHash = NULL;

/* A list for cache clients.
 * Although implemented as a list, we'll call it ClientQueue  --Jcid */
static GSList *ClientQueue = NULL;

/* A list for delayed clients (it holds weak pointers to cache entries,
 * which are used to make deferred calls to Cache_process_queue) */
static GSList *DelayedQueue = NULL;
static guint DelayedQueueIdleId = 0;


/*
 *  Forward declarations
 */
static void Cache_process_queue(CacheData_t *entry);
static void Cache_delayed_process_queue(CacheData_t *entry);
static void Cache_stop_client(gint Key, gint force);

static gint Cache_open_url(mSpiderWeb *Web, CA_Callback_t Call, void *CbData);


/*
 * Determine if two hash entries are equal (used by GHashTable)
 */
static gint Cache_hash_entry_equal(gconstpointer v1, gconstpointer v2)
{
   return (a_Url_cmp((mSpiderUrl *)v1, (mSpiderUrl *)v2) == 0);
}

/*
 * Determine the hash value given the key (used by GHashTable)
 */
static guint Cache_url_hash(gconstpointer key)
{
   const char *p = URL_STR((mSpiderUrl *)key);
   guint h = *p;

   if (h)
      for (p += 1; *p != '\0' && *p != '#'; p++)
         h = (h << 5) - h + *p;

   return h;
}

/*
 * Initialize dicache data
 */
void a_Cache_init(void)
{
   CacheHash = g_hash_table_new(Cache_url_hash, Cache_hash_entry_equal);
}

/* Client operations ------------------------------------------------------ */

/*
 * Make a unique primary-key for cache clients
 */
static gint Cache_client_make_key(void)
{
   static gint ClientKey = 0; /* Provide a primary key for each client */

   if ( ++ClientKey < 0 ) ClientKey = 1;
   return ClientKey;
}

/*
 * Add a client to ClientQueue.
 *  - Every client-camp is just a reference (except 'Web').
 *  - Return a unique number for identifying the client.
 */
static gint Cache_client_enqueue(const mSpiderUrl *Url, mSpiderWeb *Web,
                                 CA_Callback_t Callback, void *CbData)
{
   gint ClientKey;
   CacheClient_t *NewClient;

   NewClient = g_new(CacheClient_t, 1);
   ClientKey = Cache_client_make_key();
   NewClient->Key = ClientKey;
   NewClient->Url = Url;
   NewClient->Buf = NULL;
   NewClient->Callback = Callback;
   NewClient->CbData = CbData;
   NewClient->Web    = Web;

   ClientQueue = g_slist_append(ClientQueue, NewClient);

   return ClientKey;
}

/*
 * Compare function for searching a Client by its key
 */
static gint Cache_client_key_cmp(gconstpointer client, gconstpointer key)
{
   return ( GPOINTER_TO_INT(key) != ((CacheClient_t *)client)->Key );
}

/*
 * Compare function for searching a Client by its URL
 */
static gint Cache_client_url_cmp(gconstpointer client, gconstpointer url)
{
   return a_Url_cmp((mSpiderUrl *)url, ((CacheClient_t *)client)->Url);
}

/*
 * Compare function for searching a Client by hostname
 */
static gint Cache_client_host_cmp(gconstpointer client, gconstpointer hostname)
{
   return g_strcasecmp(URL_HOST(((CacheClient_t *)client)->Url),
                       (gchar *)hostname );
}

/*
 * Remove a client from the queue
 */
static void Cache_client_dequeue(CacheClient_t *Client, gint Key)
{

   GSList *List;

   if (!Client &&
       (List = g_slist_find_custom(ClientQueue, GINT_TO_POINTER(Key),
                                   Cache_client_key_cmp)))
      Client = List->data;

   if ( Client ) {
      ClientQueue = g_slist_remove(ClientQueue, Client);
      a_Web_free(Client->Web);
      g_free(Client);
   }
}


/* Entry operations ------------------------------------------------------- */

/*
 * Set safe values for a new cache entry
 */
static void Cache_entry_init(CacheData_t *NewEntry, const mSpiderUrl *Url)
{
   NewEntry->Url = a_Url_dup(Url);
   NewEntry->Type = NULL;
   NewEntry->Header = g_string_new("");
   NewEntry->Location = NULL;
   NewEntry->Data = NULL;
   NewEntry->ValidSize = 0;
   NewEntry->TotalSize = 0;
   NewEntry->BuffSize = 4096;
   NewEntry->Flags = URL_FLAGS_(Url) & URL_Post ? CA_Expires : 0;
   NewEntry->io = NULL;
   NewEntry->CCCQuery = a_Chain_new();
   NewEntry->CCCAnswer = NULL;
   NewEntry->AuthRealm = NULL;
   NewEntry->Expires = 0;
}

/*
 * Get the data structure for a cached URL (using 'Url' as the search key)
 * If 'Url' isn't cached, return NULL
 */
static CacheData_t *Cache_entry_search(const mSpiderUrl *Url)
{
   return g_hash_table_lookup(CacheHash, Url);
}

/*
 * Allocate and set a new entry in the cache list
 */
static CacheData_t *Cache_entry_add(const mSpiderUrl *Url)
{
   CacheData_t *new_entry = g_new(CacheData_t, 1);

   if (Cache_entry_search(Url))
      DEBUG_MSG(5, "WARNING: Cache_entry_add, leaking an entry.\n");

   Cache_entry_init(new_entry, Url);  /* Set safe values */
   g_hash_table_insert(CacheHash, (gpointer)new_entry->Url, new_entry);

   return new_entry;
}

/*
 *  Free the components of a CacheData_t struct.
 */
static void Cache_entry_free(CacheData_t *entry)
{
   a_Url_free((mSpiderUrl *)entry->Url);
   g_free((gchar *)entry->Type);
   g_string_free(entry->Header, TRUE);
   a_Url_free((mSpiderUrl *)entry->Location);
   g_free(entry->Data);
   g_free(entry);
   /* CCCQuery and CCCAnswer are just references */
}

/*
 * Remove an entry from the CacheList (no CCC-function is called)
 */
static gint Cache_entry_remove_raw(CacheData_t *entry, mSpiderUrl *url)
{
   if (!entry && !(entry = Cache_entry_search(url)))
      return 0;

   /* There MUST NOT be any clients */
   g_return_val_if_fail(
      !g_slist_find_custom(ClientQueue, entry->Url, Cache_client_url_cmp), 0);

   /* remove from DelayedQueue */
   DelayedQueue = g_slist_remove(DelayedQueue, entry);

   /* remove from dicache */
   a_Dicache_invalidate_entry(entry->Url);

   /* remove from cache */
   g_hash_table_remove(CacheHash, entry->Url);
   Cache_entry_free(entry);
   return 1;
}
/*
 * Remove an entry, using the CCC if necessary.
 * (entry SHOULD NOT have clients)
 */
static void Cache_entry_remove(CacheData_t *entry, mSpiderUrl *url)
{
   ChainLink *InfoQuery, *InfoAnswer;

   if (!entry && !(entry = Cache_entry_search(url)))
      return;

   InfoQuery  = entry->CCCQuery;
   InfoAnswer = entry->CCCAnswer;

   if (InfoQuery) {
      DEBUG_MSG(2, "## Aborting CCCQuery\n");
      a_Cache_ccc(OpAbort, 1, BCK, InfoQuery, NULL, NULL);
   } else if (InfoAnswer) {
      DEBUG_MSG(2, "## Aborting CCCAnswer\n");
      a_Cache_ccc(OpAbort, 2, BCK, InfoAnswer, NULL, NULL);
   } else {
      DEBUG_MSG(2, "## Aborting raw2\n");
      Cache_entry_remove_raw(entry, NULL);
   }
}


/* Misc. operations ------------------------------------------------------- */

/*
 * Given an entry (url), remove all its clients (by url or host).
 */
static void Cache_stop_clients(mSpiderUrl *url, gint ByHost)
{
   guint i;
   CacheClient_t *Client;

   for (i = 0; (Client = g_slist_nth_data(ClientQueue, i)); ++i) {
      if ( (ByHost && Cache_client_host_cmp(Client, URL_HOST(url)) == 0) ||
           (!ByHost && Cache_client_url_cmp(Client, url) == 0) ) {
         Cache_stop_client(Client->Key, 0);
         --i;
      }
   }
}

/*
 * Prepare a reload by stopping clients and removing the entry
 *  Return value: 1 if on success, 0 otherwise
 */
static gint Cache_prepare_reload(mSpiderUrl *url)
{
   CacheClient_t *Client;
   mSpiderWeb *ClientWeb;
   guint i;

   for (i = 0; (Client = g_slist_nth_data(ClientQueue, i)); ++i){
      if (Cache_client_url_cmp(Client, url) == 0 &&
          (ClientWeb = Client->Web) && !(ClientWeb->flags & WEB_Download))
         Cache_stop_client(Client->Key, 0);
   }

   if (!g_slist_find_custom(ClientQueue, url, Cache_client_url_cmp)) {
      /* There're no clients for this entry */
      DEBUG_MSG(2, "## No more clients for this entry\n");
      Cache_entry_remove(NULL, url);
      return 1;
   } else {
      MSG("Cache_prepare_reload: ERROR, entry still has clients\n");
   }

   return 0;
}

/* A list for delayed urls  */
static GSList *DelayedUrl = NULL;
static guint DelayedUrlIdleId = 0;

typedef struct DelayUrlInfo {
    mSpiderWeb* Web;
    CA_Callback_t Call;
    void* CbData;
} DUI;

static gint Cache_delayed_url_callback (gpointer data)
{
    DUI* dui;
    int ClientKey;

    if ((dui = (DUI*) g_slist_nth_data (DelayedUrl, 0))) {
        if (MAX_ACTIVE_SOCKETS > a_Http_get_nr_sockets ()) {
            printf ("A chance to revive the delayed url\n");
            ClientKey = Cache_open_url (dui->Web, dui->Call, dui->CbData);
            if (ClientKey != 0) {
                dui->Web->Image->dw->client_id = ClientKey;
                a_Doc_add_client (dui->Web->dd, ClientKey, 0);
                a_Doc_add_url (dui->Web->dd, dui->Web->url, WEB_Image);
            }
            DelayedUrl = g_slist_remove (DelayedUrl, dui);
            g_free (dui);
        }
    }

    if (g_slist_nth_data (DelayedUrl, 0))
        return 1;
    else {
        DelayedUrlIdleId = 0;
        return 0;
    }
}

void a_Cache_destroy_delayed_url (void)
{
    DUI* dui;

    while ((dui = (DUI*) g_slist_nth_data (DelayedUrl, 0))) {
        DelayedUrl = g_slist_remove (DelayedUrl, dui);
        g_free (dui);
    }

    DelayedUrl = NULL;

   	g_idle_remove_by_data ((void*)Cache_delayed_url_callback);
    DelayedUrlIdleId = 0;
}

static void Cache_delayed_url_process (mSpiderWeb *Web, CA_Callback_t Call, void *CbData)
{
   DUI* dui = g_malloc (sizeof (DUI));

   printf ("Reach the max active sockets, delay a url\n");

   if (dui) {
        dui->Web = Web;
        dui->Call = Call;
        dui->CbData = CbData;
   }

   DelayedUrl = g_slist_prepend (DelayedUrl, dui);

   if (DelayedUrlIdleId == 0)
   	    DelayedUrlIdleId = g_idle_add_full (G_PRIORITY_HIGH_IDLE, 
                         (GSourceFunc)Cache_delayed_url_callback, 
                         (void*)Cache_delayed_url_callback, NULL);
}

/*
 * Try finding the url in the cache. If it hits, send the cache contents
 * from there. If it misses, set up a new connection.
 * Return value: A primary key for identifying the client.
 */
static gint Cache_open_url(mSpiderWeb *Web, CA_Callback_t Call, void *CbData)
{
   void *link;
   gint ClientKey, imgOnOtherHost;
   ChainFunction_t cccFunct;
   mSpiderUrl *Url = Web->url;
   CacheData_t *entry = NULL;

   entry = Cache_entry_search(Url);

   imgOnOtherHost = ((URL_FLAGS(Url) & (URL_IsImage | URL_OnOtherHost))
           != (URL_IsImage | URL_OnOtherHost));

   if (entry && 
         (entry->Flags & CA_Expires) &&/*  FIXME: should't Expires ?*/ 
           (!entry->Expires || entry->Expires <= time(NULL)) &&
           !(URL_FLAGS(Url) & URL_MustCache) && imgOnOtherHost &&
           !g_slist_find_custom(ClientQueue, Url, Cache_client_url_cmp)) {
       Cache_entry_remove(entry, Url);
       entry = NULL;
   }

   if ( entry) {
      /* URL is cached: feed our client with cached data */
      ClientKey = Cache_client_enqueue(entry->Url, Web, Call, CbData);
      Cache_delayed_process_queue(entry);
   } 
   else 
   {
      /* URL not cached */

      /* check the number of active sockets to avoid failure when create socket */
      if (Web->flags & WEB_Image && MAX_ACTIVE_SOCKETS == a_Http_get_nr_sockets ()) {
         Cache_delayed_url_process (Web, Call, CbData);
         return 0;
      }

      /* create an entry, send our client to the queue, and open a new connection */
      entry = Cache_entry_add(Url);
      ClientKey = Cache_client_enqueue(entry->Url, Web, Call, CbData);
      a_Cache_ccc(OpStart, 1, BCK, entry->CCCQuery, NULL, (void *)entry->Url);
      cccFunct = a_Url_get_ccc_funct(entry->Url);
      if ( cccFunct ) {
         link = a_Chain_link_new(entry->CCCQuery,
                                 a_Cache_ccc, BCK, cccFunct, 1, 1);
         a_Chain_bcb(OpStart, entry->CCCQuery, (void *)entry->Url, Web);
      } else {
         a_Interface_msg(Web->dd, "ERROR: unsupported protocol");
         a_Cache_ccc(OpAbort, 1, FWD, entry->CCCQuery, NULL, NULL);
         ClientKey = 0; /* aborted */
      }

   }
    
   return ClientKey;

}

/*
 * Try finding the url in the cache. If it hits, send the cache contents
 * from there. If it misses, set up a new connection.
 *
 * - 'Web' is an auxiliar data structure with misc. parameters.
 * - 'Call' is the callback that receives the data
 * - 'CbData' is custom data passed to 'Call'
 *   Note: 'Call' and/or 'CbData' can be NULL, in that case they get set
 *   later by a_Web_dispatch_by_type, based on content/type and 'Web' data.
 *
 * Return value: A primary key for identifying the client.
 */
gint a_Cache_open_url(void *web, CA_Callback_t Call, void *CbData)
{
   gint ClientKey;
   DICacheEntry *DicEntry;
   CacheData_t *entry;
   mSpiderWeb *Web = web;
   mSpiderUrl *Url = Web->url;
   
   
   if (URL_FLAGS(Url) & URL_E2EReload) {
      /* Reload operation */
      Cache_prepare_reload(Url);
   }


   if ( Call ) {
      /* This is a verbatim request */
      ClientKey = Cache_open_url(Web, Call, CbData);
   } 
   else if ( (DicEntry = a_Dicache_get_entry(Url)) &&
               (entry = Cache_entry_search(Url)) ) 
   {
      /* We have it in the Dicache! */
      ClientKey = Cache_client_enqueue(entry->Url, Web, Call, CbData);
      Cache_delayed_process_queue(entry);

   } else {
      /* It can be anything; let's request it and decide what to do
         when we get more info... */
      ClientKey = Cache_open_url(Web, Call, CbData);
   }

   if (g_slist_find_custom(ClientQueue, GINT_TO_POINTER(ClientKey),
                           Cache_client_key_cmp))
      return ClientKey;
   else
      return 0; /* Aborted */
}

/*
 * Get the pointer to the URL document, and its size, from the cache entry.
 * Return: 1 cached, 0 not cached.
 */
gint a_Cache_get_buf(const mSpiderUrl *Url, gchar **PBuf, gint *BufSize)
{
   CacheData_t *entry;

   while ((entry = Cache_entry_search(Url))) {

      /* Test for a redirection loop */
      if (entry->Flags & CA_RedirectLoop) {
         g_warning ("Redirect loop for URL: >%s<\n", URL_STR_(Url));
         break;
      }
      /* Test for a working redirection */
      if (entry && entry->Flags & CA_Redirect && entry->Location
          && !entry->AuthRealm
        ) {
         Url = entry->Location;
      } else
         break;
   }

   *BufSize = (entry) ? entry->ValidSize : 0;
   *PBuf = (entry) ? (gchar *) entry->Data : NULL;
   return (entry ? 1 : 0);
}

/*
 * Extract a single field from the header, allocating and storing the value
 * in 'field'. ('fieldname' must not include the trailing ':')
 * Return a new string with the field-content if found (NULL on error)
 * (This function expects a '\r' stripped header)
 */
static char *Cache_parse_field(const char *header, const char *fieldname)
{
   char *field;
   guint i, j;

   for ( i = 0; header[i]; i++ ) {
      /* Search fieldname */
      for (j = 0; fieldname[j]; j++)
        if ( tolower(fieldname[j]) != tolower(header[i + j]))
           break;
      if ( fieldname[j] ) {
         /* skip to next line */
         for ( i += j; header[i] != '\n'; i++);
         continue;
      }

      i += j;
      while (header[i] == ' ') i++;
      if (header[i] == ':' ) {
        /* Field found! */
        while (header[++i] == ' ');
        for (j = 0; header[i + j] != '\n'; j++);
        field = g_strndup(header + i, j);
        return field;
      }
   }
   return NULL;
}


/*
 * Extract multiple fields from the header.
 */
static GList *Cache_parse_multiple_fields(const char *header,
                                          const char *fieldname)
{
   guint i, j;
   GList *fields = NULL;
   char *field;
   for (i = 0; header[i]; i++) {
      /* Search fieldname */
      for (j = 0; fieldname[j]; j++)
         if (tolower(fieldname[j]) != tolower(header[i + j]))
            break;
      if (fieldname[j]) {
         /* skip to next line */
         for (i += j; header[i] != '\n'; i++);
         continue;
      }

      i += j;
      for ( ; header[i] == ' '; i++);
      if (header[i] == ':' ) {
         /* Field found! */
         while (header[++i] == ' ');
         for (j = 0; header[i + j] != '\n'; j++);
         field = g_strndup(header + i, j);
         fields = g_list_append(fields, field);
      }
   }
   return fields;
}

static void Cache_force_min_expire(CacheData_t *entry, int min, guint stdExp)
{
   time_t t;

   if (!(entry->Flags & CA_Expires) || (URL_FLAGS_(entry->Url) & URL_Post))
      return;
   if (min < 0) {
      entry->Flags &= ~CA_Expires;
      entry->Expires = 0;
      return;
   }
   t = time(0);
   if (entry->Expires && t > entry->Expires + 1000) {
      if (!stdExp)
         entry->Flags &= ~CA_Expires;
      entry->Expires = 0;
      return;
   }
   if (min && (t += min) > entry->Expires) {
      entry->Expires = t;
   }
}
/*
 * Scan, allocate, and set things according to header info.
 * (This function needs the whole header to work)
 */
static void Cache_parse_header(CacheData_t *entry, IOData_t *io, gint HdrLen)
{
   gchar *header = entry->Header->str;
   gchar *Length, *Type, *location_str;
   gchar *CacheControl, *Date, *CC_ptr;
   guint expFlag;
   GList *Cookies;
   gchar *auth_type_realm;

   if ( HdrLen < 12 ) {
      /* Not enough info. */

   } if ( header[9] == '3' && header[10] == '0' ) {
      /* 30x: URL redirection */
      entry->Flags |= CA_Redirect;
      if ( header[11] == '1' )
         entry->Flags |= CA_ForceRedirect;  // 301 Moved Permanently
      else if ( header[11] == '2' )
         entry->Flags |= CA_TempRedirect;   // 302 Temporal Redirect
      else if ( header[11] == '3' )
         entry->Flags |= CA_Expires;        // 303 See Other
      else if ( header[11] == '7' )
         entry->Flags |= CA_Expires;        // 307 Moved Temporarely
      /* TODO: should be here a_Url_free(entry->Location) ? */
     
      location_str = Cache_parse_field(header, "Location");
      entry->Location = a_Url_new(location_str, URL_STR_(entry->Url), 0, 0, 0);
      g_free(location_str);

   } else if ( strncmp(header + 9, "404", 3) == 0 ) {
      entry->Flags |= CA_NotFound;
   } 
    else if ( strncmp(header + 9, "401", 3) == 0 ) {
      auth_type_realm = Cache_parse_field(header,"WWW-Authenticate");
      entry->Flags |= CA_Redirect;
      if (entry->Location)
         a_Url_free((mSpiderUrl*) entry->Location);
      entry->Location = a_Url_dup(entry->Url);
      entry->AuthRealm = g_string_new(auth_type_realm);
   }

   expFlag = entry->Flags & CA_Expires;
   entry->ValidSize = io->Status - HdrLen;
   if ( (Length = Cache_parse_field(header, "Content-Length")) != NULL ) {
      entry->Flags |= CA_GotLength;
      entry->TotalSize = strtol(Length, NULL, 10);
      g_free(Length);
      if (entry->TotalSize < entry->ValidSize)
         entry->TotalSize = 0;
   }

   if ( (CacheControl = Cache_parse_field(header, "Expires")) != NULL ) {
      entry->Flags |= CA_Expires;
      entry->Expires = a_Cookies_create_timestamp(CacheControl);
      g_free(CacheControl);
   }

   if ( (CacheControl = Cache_parse_field(header, "Cache-Control")) != NULL ) {
      CC_ptr = CacheControl;
      do {
         while (*CC_ptr == ' ')
            ++CC_ptr;
         if ( !g_strncasecmp(CacheControl, "no-cache", 8) ) {
            entry->Flags |= CA_Expires;
            entry->Expires = 0;
            break;
         } else if ( !g_strncasecmp(CacheControl, "max-age=", 8) ) {
            char *e;
            long n;

            if ( !(Date = Cache_parse_field(header, "Date")) ) {
               break;
            }
           n = strtoul(CacheControl + 8, &e, 10);
           if (e > CacheControl + 8 && n >= 0) {
               entry->Flags |= CA_Expires;
               entry->Expires = n + a_Cookies_create_timestamp(Date);
            }
            g_free(Date);
            break;
         }
      } while ( (CC_ptr = strchr(CC_ptr, ',')) && *++CC_ptr );
      g_free(CacheControl);
   }
#ifdef ENABLE_COOKIES
   /* BUG: If a server feels like mixing Set-Cookie2 and Set-Cookie
    * responses which aren't identical, then we have a problem. I don't
    * know if that is a real issue though. */
   if ( (Cookies = Cache_parse_multiple_fields(header, "Set-Cookie2")) ||
        (Cookies = Cache_parse_multiple_fields(header, "Set-Cookie")) ) {
      a_Cookies_set(Cookies, entry->Url);
      g_list_foreach(Cookies, (GFunc)g_free, NULL);
      g_list_free(Cookies);
   }
#endif

   if ( entry->TotalSize > 0 && entry->TotalSize >= entry->ValidSize ) {
      entry->Data = g_malloc(entry->TotalSize);
      memcpy(entry->Data, (char*)io->Buf+HdrLen, (size_t)io->Status-HdrLen);
      /* Prepare next read */
      a_IO_set_buf(io, (char *)entry->Data + entry->ValidSize,
                   entry->TotalSize - entry->ValidSize);
   } else {
      /* We don't know the size of the transfer; A lazy server? ;) */
      entry->Data = g_malloc(entry->ValidSize + entry->BuffSize);
      memcpy(entry->Data, (char *)io->Buf+HdrLen, entry->ValidSize);
      /* Prepare next read */
      a_IO_set_buf(io, (char *)entry->Data + entry->ValidSize,
                   entry->BuffSize);
   }

   /* Get Content-Type */
   if ( (Type = Cache_parse_field(header, "Content-Type")) == NULL ) {
     MSG_HTTP("Server didn't send Content-Type in header.\n");
   } else {
      entry->Type = Type;
      entry->Flags |= CA_GotContentType;
   }
   if (URL_FLAGS(entry->Url) & URL_IsImage || (Type && !strncmp(Type, "image/", 5))) {
      Cache_force_min_expire(entry, prefs.min_image_expire_time, expFlag);
   } else if (!(entry->Flags & CA_Expires) &&
              (entry->Location ? entry->Location->query : entry->Url->query) &&
              prefs.query_expire_time >= 0 && (Type && !strncmp(Type, "text/", 5))) {
      entry->Flags |= CA_Expires;
      entry->Expires = time(NULL) + prefs.query_expire_time;
   } else {
      Cache_force_min_expire(entry, prefs.min_page_expire_time, expFlag);
   }
}

/*
 * Consume bytes until the whole header is got (up to a "\r\n\r\n" sequence)
 * (Also strip '\r' chars from header)
 */
static gint Cache_get_header(IOData_t *io, CacheData_t *entry)
{
   gint N, i;
   GString *hdr = entry->Header;
   guchar *data = io->Buf;

   /* Header finishes when N = 2 */
   N = (hdr->len && hdr->str[hdr->len - 1] == '\n');
   for ( i = 0; i < io->Status && N < 2; ++i ) {
      if ( data[i] == '\r' || !data[i] )
         continue;
      N = (data[i] == '\n') ? N + 1 : 0;
      g_string_append_c(hdr, data[i]);
   }

   if ( N == 2 ){
      /* Got whole header */
      _MSG("Header [io_len=%d]\n%s", i, hdr->str);
      entry->Flags |= CA_GotHeader;
      /* Return number of original-header bytes in this io [1 based] */
      return i;
   }
   return 0;
}

/*
 * Receive new data, update the reception buffer (for next read), update the
 * cache, and service the client queue.
 *
 * This function gets called whenever the IO has new data.
 *  'Op' is the operation to perform
 *  'VPtr' is a (void) pointer to the IO control structure
 */
static void Cache_process_io(int Op, void *VPtr)
{
   gint Status, len;
   IOData_t *io = VPtr;
   const mSpiderUrl *Url = io->ExtData;
   CacheData_t *entry = Cache_entry_search(Url);

   /* Assert a valid entry (not aborted) */
   if ( !entry )
      return;

   /* Keep track of this entry's io */
   entry->io = io;

   if ( Op == IOClose ) {
      if (entry->Flags & CA_GotLength && entry->TotalSize != entry->ValidSize){
         MSG_HTTP("Content-Length does NOT match message body,\n"
                  " at: %s\n", URL_STR_(entry->Url));
      }
      entry->Flags |= CA_GotData;
      entry->Flags &= ~CA_Stopped;          /* it may catch up! */
      entry->TotalSize = entry->ValidSize;
      entry->io = NULL;
      entry->CCCAnswer = NULL;
      Cache_process_queue(entry);
      return;
   } else if ( Op == IOAbort ) {
      /* todo: implement Abort
       * (eliminate cache entry and anything related) */
      DEBUG_MSG(5, "Cache_process_io Op = IOAbort; not implemented yet\n");
      entry->io = NULL;
      entry->CCCAnswer = NULL;
      return;
   }

   if ( !(entry->Flags & CA_GotHeader) ) {
      /* Haven't got the whole header yet */
      len = Cache_get_header(io, entry);
      if ( entry->Flags & CA_GotHeader ) {
         /* Let's scan, allocate, and set things according to header info */
         Cache_parse_header(entry, io, len);
         /* Now that we have it parsed, let's update our clients */
         Cache_process_queue(entry);
      }
      return;
   }

   Status = io->Status;
   entry->ValidSize += Status;
   if ( Status < (gint)io->BufSize ) {
      /* An incomplete buffer; update buffer & size */
      a_IO_set_buf(io, (char *)io->Buf + Status, io->BufSize - Status);

   } else if ( Status == (gint)io->BufSize ) {
      /* A full buffer! */
      if ( !entry->TotalSize ) {
         /* We are receiving in small chunks... */
         entry->Data = g_realloc(entry->Data,entry->ValidSize+entry->BuffSize);
         a_IO_set_buf(io, (char *)entry->Data + entry->ValidSize,
                      entry->BuffSize);
      } else {
         /* We have a preallocated buffer! */
         a_IO_set_buf(io, (char *)io->Buf + Status, io->BufSize - Status);
      }
   }
   Cache_process_queue(entry);
}

/*
 * Process redirections (HTTP 30x answers)
 * (This is a work in progress --not finished yet)
 */
static gint Cache_redirect(CacheData_t *entry, gint Flags, mSpiderDoc *dd)
{
   mSpiderUrl *NewUrl;

   _MSG(" Cache_redirect: redirect_level = %d\n", dd->redirect_level);
   
   /* if there's a redirect loop, stop now */
	if ( !dd )
		return 0;
   if (dd->redirect_level >= 5)
      entry->Flags |= CA_RedirectLoop;

   if (entry->Flags & CA_RedirectLoop) {
     a_Interface_msg(dd, "ERROR: redirect loop for: %s", URL_STR_(entry->Url));
     dd->redirect_level = 0;
     return 0;
   }
   if ((entry->Flags & CA_Redirect && entry->Location) &&
       (entry->Flags & CA_ForceRedirect || entry->Flags & CA_TempRedirect ||
        !entry->ValidSize || entry->ValidSize < 1024 )) {

      _MSG(">>>Redirect from: %s\n to %s\n",
           URL_STR_(entry->Url), URL_STR_(entry->Location));
      _MSG("%s", entry->Header->str);

      if (Flags & WEB_RootUrl) {
         /* Redirection of the main page */
         NewUrl = a_Url_new(URL_STR_(entry->Location), URL_STR_(entry->Url),
                            0, 0, 0);
         if (entry->Flags & CA_TempRedirect)
            a_Url_set_flags(NewUrl, URL_FLAGS(NewUrl) | URL_E2EReload);
         if (entry->AuthRealm) {
            a_Auth_byrealm(entry->AuthRealm, NewUrl, dd);
            a_Url_free(NewUrl);
            return 0;
         } 
         a_Nav_remove_top_url(dd);
         a_Nav_push(dd, NewUrl);
         
         a_Url_free(NewUrl);
      } else {
         /* Sub entity redirection (most probably an image) */
         if ( !entry->ValidSize ) {
            DEBUG_MSG(3,">>>Image redirection without entity-content<<<\n");
         } else {
            DEBUG_MSG(3, ">>>Image redirection with entity-content<<<\n");
         }
      }
   }
   return 0;
}

/*
 * Don't process data any further, but let the cache fill the entry.
 * (Currently used to handle WEB_RootUrl redirects,
 *  and to ignore image redirects --Jcid)
 */
void a_Cache_null_client(int Op, CacheClient_t *Client)
{
   mSpiderWeb *Web = Client->Web;

   /* make the stop button insensitive when done */
   if (Op == CA_Close) {
      if (Web->flags & WEB_RootUrl){
         /* Remove this client from our active list */
         a_Doc_close_client(Web->dd, Client->Key);
      }
   }

   /* else ignore */

   return;
}

/*
 * Update cache clients for a single cache-entry
 * Tasks:
 *   - Set the client function (if not already set)
 *   - Look if new data is available and pass it to client functions
 *   - Remove clients when done
 *   - Call redirect handler
 *
 * todo: Implement CA_Abort Op in client callback
 */
static void Cache_process_queue(CacheData_t *entry)
{
   guint i;
   gint st;
   const char *Type = NULL;
   CacheClient_t *Client;
   mSpiderWeb *ClientWeb;
   mSpiderDoc *Client_dd = NULL;
   static gboolean Busy = FALSE;
   gboolean AbortEntry = FALSE;

   if ( Busy )
      DEBUG_MSG(5, "FATAL!:*** >>>> Cache_process_queue Caught busy!!!\n");
   if (!(entry->Flags & CA_GotHeader))  
      return;

   if (!(entry->Flags & CA_GotContentType)) {
      st = a_Misc_get_content_type_from_data (entry->Data, entry->ValidSize, &Type);
      if (st == 0 || entry->Flags & CA_GotData) {
         entry->Type = g_strdup(Type);
         entry->Flags |= CA_GotContentType;
      } else 
         return;  /* i.e., wait for more data */
   }

   Busy = TRUE;
   for ( i = 0; (Client = g_slist_nth_data(ClientQueue, i)); ++i ) {
      if ( Client->Url == entry->Url ) {
         ClientWeb = Client->Web;    /* It was a (void*) */
         Client_dd = ClientWeb->dd;  /* 'bw' in a local var */

         if (ClientWeb->flags & WEB_RootUrl) {
            if (!(entry->Flags & CA_MsgErased)) {
               /* clear the "expecting for reply..." message */
               a_Interface_msg(Client_dd, "");
               entry->Flags |= CA_MsgErased;
            }
            if (entry->Flags & CA_Redirect
                 && !entry->AuthRealm
               ) {
               if (!Client->Callback) {
                  Client->Callback = a_Cache_null_client;
                  Client_dd->redirect_level++;
               }
            } else {
               Client_dd->redirect_level = 0;
            }
         } else {
            /* For non root URLs, ignore redirections and 404 answers */
            if (entry->Flags & CA_Redirect || entry->Flags & CA_NotFound)
               Client->Callback = a_Cache_null_client;
         }

         /* Set the client function */
         if (!Client->Callback) 
         {
            st = a_Web_dispatch_by_type(entry->Type, ClientWeb,
                                        &Client->Callback, &Client->CbData);
            if (st == -1) {
               /* MIME type is not viewable */
               Client->Callback = a_Cache_null_client;
               Client->CbData = NULL;
               if (ClientWeb->flags & WEB_RootUrl) {
                  /* Unhandled MIME type, prepare a download offer... */
                  AbortEntry = TRUE;
                  a_Doc_remove_client(Client_dd, Client->Key);
                  Cache_client_dequeue(Client, NULLKey);
                  --i; /* Keep the index value in the next iteration */
                  continue;
               }
            }
         }

         /* Send data to our client */
         if ( (Client->BufSize = entry->ValidSize) > 0) 
         {
            Client->Buf = (guchar *)entry->Data;
            (Client->Callback)(CA_Send, Client);
         }

         /* Remove client when done */
         if ( (entry->Flags & CA_GotData) ) {
            /* Copy flags to a local var */
            gint flags = ClientWeb->flags;
            /* We finished sending data, let the client know */
            if (!Client->Callback)
               DEBUG_MSG(3, "Client Callback is NULL");
            else
               (Client->Callback)(CA_Close, Client);
            Cache_client_dequeue(Client, NULLKey);
            --i; /* Keep the index value in the next iteration */

            /* call Cache_redirect() from this 'if' to assert one call only. */
            if ( entry->Flags & CA_Redirect )
               Cache_redirect(entry, flags, Client_dd);

            _MSG("Cache_process_queue: NumRootClients=%d sens_idle_id = %d\n",
                 Client_bw->NumRootClients, Client_bw->sens_idle_id);
         }
      }
   } /* for */

   if (AbortEntry) {
      /* Abort the entry, remove it from cache, and offer download.
       * (the dialog is made before 'entry' is freed) */
      //a_Interface_offer_link_download(Client_bw, entry->Url);
      Cache_entry_remove(entry, NULL);
   }

   Busy = FALSE;
   DEBUG_MSG(1, "QueueSize ====> %d\n", g_slist_length(ClientQueue));
}

/*
 * Callback function for Cache_delayed_process_queue.
 */
static gint Cache_delayed_process_queue_callback(gpointer data)
{
   gpointer entry;

   while ((entry = g_slist_nth_data(DelayedQueue, 0))) {
      Cache_process_queue( (CacheData_t *)entry );
      /* note that if Cache_process_queue removes the entry,
       * the following g_slist_remove has no effect. */
      DelayedQueue = g_slist_remove(DelayedQueue, entry);
   }
   DelayedQueueIdleId = 0;
   return FALSE;
}

/*
 * Set a call to Cache_process_queue from the gtk_main cycle.
 */
static void Cache_delayed_process_queue(CacheData_t *entry)
{
   /* there's no need to repeat entries in the queue */
   if (!g_slist_find(DelayedQueue, entry))
      DelayedQueue = g_slist_prepend(DelayedQueue, entry);

   if (DelayedQueueIdleId == 0)
      DelayedQueueIdleId =
         g_idle_add_full (G_PRIORITY_HIGH_IDLE, 
                         (GSourceFunc)Cache_delayed_process_queue_callback, 
                         NULL, NULL);
}


/*
 * Remove a cache client
 * todo: beware of downloads
 */
static void Cache_remove_client_raw(CacheClient_t *Client, gint Key)
{
   Cache_client_dequeue(Client, Key);
}

/*
 * Remove every Interface-client of a single Url.
 * todo: beware of downloads
 * (this is transitory code)
 */
static void Cache_remove_interface_clients(const mSpiderUrl *Url)
{
   guint i;
   mSpiderWeb *Web;
   CacheClient_t *Client;

   for ( i = 0; (Client = g_slist_nth_data(ClientQueue, i)); ++i ) {
      if ( Client->Url == Url ) {
         Web = Client->Web;
         a_Doc_remove_client(Web->dd, Client->Key);
      }
   }
}

/*
 * Remove a client from the client queue
 * todo: notify the dicache and upper layers
 */
static void Cache_stop_client(gint Key, gint force)
{
   CacheClient_t *Client;
   CacheData_t *entry;
   GSList *List;
   mSpiderUrl *url;

   if (!(List = g_slist_find_custom(ClientQueue, GINT_TO_POINTER(Key),
                                    Cache_client_key_cmp))){
      _MSG("WARNING: Cache_stop_client, inexistent client\n");
      return;
   }

   Client = List->data;
   url = (mSpiderUrl *)Client->Url;
   Cache_remove_client_raw(Client, NULLKey);

   if (force &&
       !g_slist_find_custom(ClientQueue, url, Cache_client_url_cmp)) {
      /* it was the last client of this entry */
      if ((entry = Cache_entry_search(url))) {
         if (entry->CCCQuery) {
            a_Cache_ccc(OpAbort, 1, BCK, entry->CCCQuery, NULL, NULL);
         } else if (entry->CCCAnswer) {
            a_Cache_ccc(OpStop, 2, BCK, entry->CCCAnswer, NULL, Client);
         }
      }
   }
}

/*
 * Remove a client from the client queue
 * (It may keep feeding the cache, but nothing else)
 */
void a_Cache_stop_client(gint Key)
{
   Cache_stop_client(Key, 0);
}

/*
 * CCC function for the CACHE module
 */
void a_Cache_ccc(int Op, int Branch, int Dir, ChainLink *Info,
                 void *Data1, void *Data2)
{
   CacheData_t *entry;
   a_Chain_debug_msg("a_Cache_ccc", Op, Branch, Dir);

   if ( Branch == 1 ) {
      if (Dir == BCK) {
         /* Querying branch */
         switch (Op) {
         case OpStart:
            /* Localkey = entry->Url */
            Info->LocalKey = Data2;
            break;
         case OpStop:
            break;
         case OpAbort:
            Cache_entry_remove_raw(NULL, Info->LocalKey);
            a_Chain_bcb(OpAbort, Info, NULL, NULL);
            g_free(Info);
            break;
         }
      } else {  /* FWD */
         switch (Op) {
         case OpSend:
            /* Start the answer-reading chain */
            a_Cache_ccc(OpStart, 2, BCK, a_Chain_new(), Data1, Info->LocalKey);
            break;
         case OpEnd:
            /* unlink HTTP_Info */
            a_Chain_del_link(Info, BCK);
            /* 'entry->CCCQuery' and 'Info' point to the same place! */
            if ((entry = Cache_entry_search(Info->LocalKey)) != NULL)
               entry->CCCQuery = NULL;
            g_free(Info);
            break;
         case OpAbort:
            /* 
            HTTP_Info */
            a_Chain_del_link(Info, BCK);

            /* remove interface client-references of this entry */
            Cache_remove_interface_clients(Info->LocalKey);
            /* remove clients of this entry */
            Cache_stop_clients(Info->LocalKey, 0);
            /* remove the entry */
            Cache_entry_remove_raw(NULL, Info->LocalKey);

            g_free(Info);
            break;
         }
      }

   } 
   else if (Branch == 2) 
   {
      if (Dir == FWD) {
         /* Answering branch */
         switch (Op) {
         case OpStart:
            /* Data2 = entry->Url */
            Info->LocalKey = Data2;
            if ((entry = Cache_entry_search(Info->LocalKey))) {
               entry->CCCAnswer = Info;
            } else {
               /* The cache-entry was removed */
               a_Chain_bcb(OpAbort, Info, NULL, NULL);
               g_free(Info);
            }
            break;
         case OpSend:
            /* Send data */
            if ((entry = Cache_entry_search(Info->LocalKey))) {
               Cache_process_io(IORead, Data1);
            } else {
               a_Chain_bcb(OpAbort, Info, NULL, NULL);
               g_free(Info);
            }
            break;
         case OpEnd:
            /* Unlink HTTP_Info */
            a_Chain_del_link(Info, BCK);
            g_free(Info);
            Cache_process_io(IOClose, Data1);
            break;
         case OpAbort:
            a_Chain_del_link(Info, BCK);
            g_free(Info);
            Cache_process_io(IOAbort, Data1);
            break;
         }
      } else {  /* BCK */
         switch (Op) {
         case OpStart:
            {
            IOData_t *io2;

            Info->LocalKey = Data2;
            if ((entry = Cache_entry_search(Data2)))    // Is Data2 valid?
               entry->CCCAnswer = Info;

            /* Link IO to receive the answer */
            io2 = a_IO_new(IORead, *(int*)Data1);
            a_IO_set_buf(io2, NULL, IOBufLen);
            io2->ExtData = Data2;       // We have it as LocalKey
            a_Chain_link_new(Info, a_Cache_ccc, BCK, a_IO_ccc, 2, 2);
            a_Chain_bcb(OpStart, Info, io2, NULL);
            break;
            }
         case OpStop:
            MSG(" Not implemented\n");
            break;
         case OpAbort:
            Cache_entry_remove_raw(NULL, Info->LocalKey);
            a_Chain_bcb(OpAbort, Info, NULL, NULL);
            g_free(Info);
            break;
         }
      }
   }
}

static gboolean
Cache_remove_hash_entry(gpointer key, gpointer value, gpointer user_data)
{
   Cache_entry_free((CacheData_t *)value);
   return TRUE;
}


/*
 * Memory deallocator (only called at exit time)
 */
void a_Cache_freeall(void)
{
   CacheClient_t *Client;

   /* free the client queue */
   while ( (Client = g_slist_nth_data(ClientQueue, 0)) )
      Cache_client_dequeue(Client, NULLKey);

   /* free the main cache */
   /* Remove every cache entry */
   g_hash_table_foreach_remove(CacheHash, (GHRFunc)Cache_remove_hash_entry,
                               NULL);
   /* Remove the cache hash */
   g_hash_table_destroy(CacheHash);
}
