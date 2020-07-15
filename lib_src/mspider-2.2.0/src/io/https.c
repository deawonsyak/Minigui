/*
 * File: https.c
 *
 * Copyright (C) 2005-2006 Feynman Software
 * Copyright (C) 2000, 2001 Jorge Arellano Cid <jcid@inf.utfsm.cl>
 * Copyright (C) 2002       Jonathan P Springer <jonathan.springer@verizon.net>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc., 59
 * Temple Place, Suite 330, Boston, MA 02111-1307 USA
 *
 * In addition, as a special exception, Jorge Arellano Cid and Jonathan
 * Springer give permission to link the code of this program with the OpenSSL
 * library (or modified versions of OpenSSL that use the same license as
 * OpenSSL), and distribute linked combinations including the two.  You must
 * obey the GNU General Public License in all respects for all of the code
 * used other than OpenSSL.  If you modify this file, you may extend this
 * exception to your version of the file, but you are not obligated to do so.
 * If you do not wish to do so, delete this exception from your version.  
 *
 */

/*
 * This program exploits code originally published under the OpenSSL demos/bio
 * directory.  I am uncertain as to the copyright status of this code, as no
 * notice was included.  If you own the copyright on this code and object to
 * its use in this GPL program, please contact me.  -- JPS
 */

/*
 * HTTPS connect functions
 */

#include <mgdconfig.h>

#ifdef ENABLE_SSL

#include <unistd.h>
#include <errno.h>              /* for errno */
#include <string.h>             /* for strstr */
#include <stdlib.h>
#include <signal.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <sys/socket.h>         /* for lots of socket stuff */
#include <netinet/in.h>         /* for ntohl and stuff */
#include <openssl/ssl.h>
#include <openssl/err.h>

#include <glib.h>

#include "../klist.h"
#include "../dns.h"
#include "../cache.h"
#include "../emspider.h"
#include "../web.h"
#include "url_io.h"
#include "DBIO.h"
#include "interface.h"

extern char *Http_query(const mSpiderUrl *url, gboolean use_proxy);


/* Used to send a message to the bw's status bar */
#define BW_MSG(web, root, fmt...) \
   (a_Web_valid(web) && (!(root) || (web)->flags & WEB_RootUrl)) ? \
   a_Interface_msg((web)->dd, fmt) : (root)

#define DEBUG_LEVEL 5
#include "../debug.h"


/* 'Url' and 'web' are just references (no need to deallocate them here). */
typedef struct {
  BIO* bio;
  SSL* ssl;
  const mSpiderUrl *Url;    /* reference to original URL */
  mSpiderWeb *web;          /* reference to client's web structure */
  guint32 ip_addr;        /* Holds the DNS answer */
  gint Err;               /* Holds the errno of the connect() call */
  ChainLink *Info;        /* Used for CCC asynchronous operations */
} SSLData_t;


/*
 * Local data
 */
static Klist_t *ValidSSLs = NULL;  /* Active sockets list. It holds pointers to
				    * SSLData_t structures. */

static gboolean Https_init = FALSE;

static SSL_CTX* ssl_ctx;

/*
 * Forward declarations
 */
static void Https_send_query(ChainLink *Info, SSLData_t *S);
static void Https_expect_answer(SSLData_t *S);

/*
 *  If it hasn't been initialized already, initialize
 *  the SSL-specific stuff.
 */
void Https_SSL_init(void)
{
  if (!Https_init) {
    SSL_load_error_strings();
    OpenSSL_add_ssl_algorithms();
    ssl_ctx = SSL_CTX_new(SSLv23_client_method());
    Https_init = TRUE;
  }
}
    
/*
 * Create and init a new SSLData_t struct, insert into ValidSSLs,
 * and return a primary key for it.
 */
gint Https_SSL_new(void)
{
  SSLData_t *S = g_new0(SSLData_t, 1);
  return a_Klist_insert(&ValidSSLs, (gpointer)S);
}

/*
 * Free SSLData_t struct
 */
void Https_SSL_free(gint SKey)
{
  SSLData_t *S;

  if ((S = a_Klist_get_data(ValidSSLs, SKey))) {
    a_Klist_remove(ValidSSLs, SKey);
    if (S->bio) BIO_free_all(S->bio);
    g_free(S);
  }
}

/*
 * This function is called after the socket has been successfuly connected,
 * or upon an error condition on the connecting process.
 * Task: use the socket to send the HTTP-query and expect its answer
 */
static gboolean Https_Callback_Use(gpointer data)
{
  ChainLink *Info;
  SSLData_t *S;
  gint SKey = (gint) data;
  gchar *msg_enc;

  DEBUG_MSG(3, "Https_Callback_Use\n");

  /* This check is required because glib may asynchronously
   * call this function with data that's no longer used  --Jcid   */
  if ( !(S = a_Klist_get_data(ValidSSLs, SKey)) ) return FALSE;

  Info = S->Info;
  if ( S->Err ) {
    DEBUG_MSG(3, "--Error detected\n");
    DEBUG_MSG(3, "%d:%s:%s:%s\n", S->Err, 
	      ERR_lib_error_string(S->Err), 
	      ERR_func_error_string(S->Err),
	      ERR_reason_error_string(S->Err));
	/*msg_enc = a_I18n_locale_to_MSPIDER_CHARSET("ERROR: unable to connect to remote host");*/
    msg_enc = g_malloc(64);/*change by zjp*/
    strcpy(msg_enc , "ERROR: unable to connect to remote host");/*change by zjp*/
    BW_MSG(S->web, 0, msg_enc);  
    PAGE_MSG (S->web, ERR_NETCONNECT);
	g_free(msg_enc);
    a_Chain_fcb(OpAbort, Info, NULL, NULL);
    Https_SSL_free(SKey);
  } else {
    DEBUG_MSG(3, "--Connection established\n");
    Https_send_query(Info, S);
    Https_expect_answer(S);
  }
  return FALSE;
}

gboolean Https_Callback_Connect(gpointer data) {

  gint SKey = (gint) data;
  SSLData_t *S;
  gchar *msg_enc;

  /* This check is required because glib may asynchronously
   * call this function with data that's no longer used  --Jcid   */
  if ( !(S = a_Klist_get_data(ValidSSLs, SKey)) ) return FALSE;
  
  S->Err = 0;
  if (BIO_do_connect(S->bio) != 1) {
    if (BIO_should_retry(S->bio)) return TRUE;
    S->Err = ERR_get_error();
    DEBUG_MSG(3, "%d:%s:%s:%s\n", S->Err, 
	      ERR_lib_error_string(S->Err), 
	      ERR_func_error_string(S->Err),
	      ERR_reason_error_string(S->Err));
/*	msg_enc = a_I18n_locale_to_MSPIDER_CHARSET(_("ERROR: unable to connect to remote host"));*/
    msg_enc = g_malloc(64);
    strcpy(msg_enc , "ERROR: unable to connect to remote host");
    BW_MSG(S->web, 0, msg_enc); 
	g_free(msg_enc);
    return FALSE;
  }
  g_idle_add_full(G_PRIORITY_HIGH_IDLE, Https_Callback_Use, data, NULL);
  //g_idle_add(Https_Callback_Use, data);
  return FALSE;
}

/*
 * This function gets called after the DNS succeeds solving a hostname.
 * Task: Finish socket setup and start connecting the socket.
 * Return value: 0 on success;  -1 on error.
*/
static int Https_connect_SSL(ChainLink *Info)
{
  SSLData_t *S = a_Klist_get_data(ValidSSLs, (gint)Info->LocalKey);

  BIO *ssl_bio;
  union {
    guint32 i;
    guchar c[4];
  } U;
  gchar *tmp_str;
  int port = MSPIDER_URL_HTTPS_PORT;

  /* TODO - Better error checking */

  /* Initialize some SSL stuff if this is our first connection */
  Https_SSL_init();

  /* Create an SSL structure */
  S->ssl = SSL_new(ssl_ctx);
  SSL_set_connect_state(S->ssl);

  /* Create a BIO structure */
  ssl_bio = BIO_new(BIO_f_ssl());
  BIO_set_ssl(ssl_bio, S->ssl, BIO_CLOSE);

  /* Create and configure the connection BIO */
  S->bio = BIO_new(BIO_s_connect());

#if 0
  U.i = htonl(S->ip_addr);
#else
  U.i = S->ip_addr; /* should already be in the right order */
#endif
  tmp_str = g_strdup_printf("%u.%u.%u.%u", U.c[0], U.c[1], U.c[2], U.c[3]);
  BIO_set_conn_hostname(S->bio, tmp_str);
  g_free(tmp_str);
  BIO_set_conn_int_port(S->bio, &port);

  BIO_set_nbio(S->bio, 1); /* non-blocking ON */

  /* Chain the socket & SSL BIOs together */
  S->bio = BIO_push(ssl_bio, S->bio); /* chain this with the SSL BIO */

  /* And set up a looping idle event to connect it... */
  g_idle_add_full(G_PRIORITY_HIGH_IDLE, Https_Callback_Connect, (gpointer) Info->LocalKey, NULL);
  //g_idle_add(Https_Callback_Connect, (gpointer) Info->LocalKey);

  return 0; /* Success */
}

/*
 * Create and submit the HTTP query to the IO engine
 */
static void Https_send_query(ChainLink *Info, SSLData_t *S)
{
  IOData_t *io;
  gchar *query, *msg_enc;
  void *link;

  /* Create the query */
  query = a_Http_make_query_str(S->Url, FALSE);

  /* send query */
  /*msg_enc = a_I18n_locale_to_MSPIDER_CHARSET(_("Sending query to %s..."));*/
  msg_enc = g_malloc(64);
  strcpy(msg_enc ,"Sending query to host!");/*change by zjp*/
  BW_MSG(S->web, 1, msg_enc, URL_HOST(S->Url));
  g_free(msg_enc);
  io = a_DBIO_new(S->bio);
  io->Op = IOWrite;
  a_IO_set_buf(io, query, strlen(query));
  io->Flags |= IOFlag_FreeIOBuf;
  io->ExtData = NULL;
  link = a_Chain_link_new(Info, a_Https_ccc, BCK, a_DBIO_ccc,2,2);
  a_DBIO_ccc(OpStart, 1,0, link, io, NULL);
}

/*
 * Expect the HTTP query's answer
 */
static void Https_expect_answer(SSLData_t *S)
{
  IOData_t *io2;

  /* receive answer */
  io2 = a_DBIO_new(S->bio);
  io2->Op = IORead;
  a_IO_set_buf(io2,g_malloc(IOBufLen_Http),IOBufLen_Http);
  io2->Flags |= IOFlag_FreeIOBuf;
  io2->ExtData = (void *) S->Url;
  a_DBIO_ccc(OpStart, 2, 0,a_Chain_new(), io2, NULL);
}

/*
 * Asynchronously create a new http connection for 'Url'
 * We'll set some socket parameters; the rest will be set later
 * when the IP is known.
 * ( Data = Requested Url; ExtraData = Web structure )
 * Return value: 0 on success, -1 otherwise
 */
gint Https_get(ChainLink *Info, void *Data, void *ExtraData)
{
  void *link;
  const mSpiderUrl *Url = Data;
  SSLData_t *S = a_Klist_get_data(ValidSSLs, (gint)Info->LocalKey);
  gchar hostname[256], *Host = hostname, *msg_enc;

  /* Reference Info data */
  S->Info = Info;
  /* Reference Web data */
  S->web = ExtraData;

  Host = (gchar *)URL_HOST(Url);
  
  /* Set more socket parameters */
  S->Url = Url;

  /* Let the user know what we'll do */
  /*msg_enc = a_I18n_locale_to_MSPIDER_CHARSET(_("DNS solving %s"));*/
  msg_enc = g_malloc(32);
  strcpy(msg_enc ," DNS solving");
  BW_MSG(S->web, 1, msg_enc, URL_HOST(S->Url)); 
  g_free(msg_enc);

  /* Let the DNS engine solve the hostname, and when done,
   * we'll try to connect the socket */
  link = a_Chain_link_new(Info, a_Https_ccc, BCK, a_Dns_ccc, 1, 1);
  a_Chain_bcb(OpStart,Info, Host,NULL);
  
  return 0;
}

/*
 * CCC function for the HTTP module
 */
void
a_Https_ccc(int Op, int Branch, int Dir, ChainLink *Info, void *Data, void *ExtraData)
{
  gint SKey = (gint)Info->LocalKey;
  SSLData_t *S = a_Klist_get_data(ValidSSLs, SKey);
  gchar *msg_enc;

  if ( Branch == 1 ) {
    /* DNS query branch */
      switch (Op) {  
      case OpStart:
	SKey = Https_SSL_new();
	Info->LocalKey = (void *) SKey;
	if (Https_get(Info, Data, ExtraData) < 0) {
	  DEBUG_MSG(2, " HTTP: new abort handler! #2\n");
	  S = a_Klist_get_data(ValidSSLs, SKey);
	  /*msg_enc = a_I18n_locale_to_MSPIDER_CHARSET("ERROR: %s");*/
      msg_enc = g_malloc(32);/*add by zjp*/
      strcpy(msg_enc , "ERROR:HTTPS Get!"); /*change by zjp*/
	  BW_MSG(S->web, 1, msg_enc, g_strerror(S->Err)); 
	  g_free(msg_enc);
	  Https_SSL_free(SKey);
	  a_Chain_fcb(OpAbort, Info, NULL, NULL);
	}
      break;
      case OpSend:
	/* Successful DNS answer; save the IP */
	if (S)
	  {
	    mSpiderHost *dh=(mSpiderHost *)(((GSList *)Data)->data);
	    S->ip_addr = *(int *)dh->data;
	  }
	break;
      case OpEnd:
      if (S) {
	/* Unlink DNS_Info */
	a_Chain_del_link(Info, BCK);
	/* start connecting the socket */
	if (Https_connect_SSL(Info) < 0) {
	  DEBUG_MSG(2, " HTTP: new abort handler! #1\n");
	  /*msg_enc = a_I18n_locale_to_MSPIDER_CHARSET(_("ERROR: %s")); change by zjp*/
      msg_enc = g_malloc(32);
      strcpy(msg_enc , "ERROR:");
	  BW_MSG(S->web, 1, msg_enc, g_strerror(S->Err));
	  g_free(msg_enc);
	  Https_SSL_free(SKey);
	  a_Chain_fcb(OpAbort, Info, NULL, NULL);
	}
      }
      break;
    case OpAbort:
      /* DNS wasn't able to resolve the hostname */
      if (S) {
        /* Unlink DNS_Info */
        a_Chain_del_link(Info, BCK);
        /*msg_enc = a_I18n_locale_to_MSPIDER_CHARSET(_("ERROR: Dns can't solve %s")); change by zjp*/
        msg_enc = g_malloc(32);
        strcpy(msg_enc ,"ERROR: Dns can't solve");
        BW_MSG(S->web, 0, msg_enc, URL_HOST(S->Url)); 
        
        g_free(msg_enc);
        BIO_free_all(S->bio);
        Https_SSL_free(SKey);
        /* send abort message to higher-level functions */
        a_Chain_fcb(OpAbort, Info, NULL, NULL);
        PAGE_MSG (S->web, ERR_DNSSOLVE);
      }
      break;
      
    }
  
  } else if ( Branch == 2 ) {
    /* IO send-query branch */
    switch (Op) {
    case OpStart:
      /* LocalKey was set by branch 1 */
      break;
    case OpEnd:
      /* finished sending the HTTP query */
      if (S) {
		 /* msg_enc = a_I18n_locale_to_MSPIDER_CHARSET(_("Query sent, waiting for reply..."));*/
          msg_enc = g_malloc(64);
          strcpy(msg_enc , "Query sent, waiting for reply...");
          BW_MSG(S->web, 1, msg_enc); 
          g_free(msg_enc);
          a_Chain_del_link(Info, BCK);
          a_Chain_fcb(OpEnd, Info, NULL, NULL);
      }
      break;
    case OpAbort:
      /* something bad happened... */
      /* unlink IO_Info */
      if (S) {
        a_Chain_del_link(Info, BCK);
        a_Chain_fcb(OpAbort, Info, NULL, NULL);
        Https_SSL_free(SKey);
      }
      break;
    }

  } else if ( Branch == -1 ) {
    /* Backwards abort */
    switch (Op) {
    case OpAbort:
      /* something bad happened... */
      DEBUG_MSG(2, "Http: OpAbort [-1]\n");
      Https_SSL_free(SKey);
      a_Chain_bcb(OpAbort, Info, NULL, NULL);
      g_free(Info);
      break;
    }
  }
}



/*
 * Deallocate memory used by http module
 * (Call this one at exit time)
 */
void a_Https_freeall(void)
{
  if (ssl_ctx) SSL_CTX_free(ssl_ctx);
  a_Klist_free(&ValidSSLs);
}

#endif

