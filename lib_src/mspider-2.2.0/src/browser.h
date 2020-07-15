/* 
 * Copyright (C) 2005-2006 Feynman Software
 *
 * This program is g_free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#ifndef __BROWSER_H__
#define __BROWSER_H__
#include "mgdconfig.h"
#ifdef _NOUNIX_
#include <common.h>
#include <minigui.h>
#include <gdi.h>
#include <window.h>
#include <control.h>
#else
#include <minigui/common.h>
#include <minigui/minigui.h>
#include <minigui/gdi.h>
#include <minigui/window.h>
#include <minigui/control.h>
#endif
#include "mspider.h"
#include "url.h"
#include "js.h"

typedef struct _BrowserWindow BrowserWindow;

typedef struct _mSpiderDoc mSpiderDoc;

typedef struct {
    mSpiderUrl *Url;   /* URL-key for this cache connection */
    gint Flags;      /* {WEB_RootUrl, WEB_Image, WEB_Download} */
} DdUrls;



/* browser_window contains all widgets to create a single window */
struct _BrowserWindow
{
   /* widgets for the main window */
   HWND main_window;

   RECT client_rc;

   void (*CB_MESSAGE_BOX)(HWND hMainWnd, const char* pszText, const char* pszCaption);
   BOOL (*CB_CONFIRM_BOX)(HWND hMainWnd, const char* pszText, const char* pszCaption);
   char *(*CB_PROMPT_BOX)(HWND hMainWnd, const char* pszText, const char* pszDefault, const char* pszCaption);

    void (*CB_SET_LOCATION) (HWND hMainWnd, const char * plocText);
    char *(*CB_GET_LOCATION) (HWND hMainWnd);
    void (*CB_SET_STATUS)(HWND hMainWnd, const char* status_msg);
    char *(*CB_GET_STATUS)(HWND hMainWnd);
    void (*CB_SET_PROGRESS)(HWND hMainWnd, const char* progress_msg);
    char *(*CB_GET_PROGRESS)(HWND hMainWnd);
#ifdef JS_SUPPORT
	jsobject *jsobj;        /* js object: main window */
#endif	
   /* the container widget, this contains either the tabbrowser
    * or the docwin
    */
   /* the currently displayed document */
   mSpiderDoc *dd;

   /* the findtext state (TRUE if search string has been found
    * in the document or any of its children
    */
   gboolean found;

   /* the list of documents to search through in findtext */
   GList *dd_list;

   /* the current index for dd_list */
   guint dd_list_index;

   /* the url for tab*/
   mSpiderUrl *url;

   /* The id for the idle function that sets button sensitivity. */
   guint sens_idle_id;

#ifdef ENABLE_LINKTRAVE
   void *linktrave_manage;
#endif /* ENABLE_LINKTRAVE */
};

/* mspider_doc contains all data pertaining to a single document */
struct _mSpiderDoc
{
   /* the BrowserWindow this document is currently part of */
   BrowserWindow *bw;

   /* This is the main document widget. (HTML rendering or whatever) */
   HWND docwin;
   
   int   mspider_id;  

   void* html_block;
   void* viewport;

   gint status_is_link;

   /* The initial style for the widget */
   void *style;

   /* for nested documents, points to parent document */
   mSpiderDoc *parent;

   /* This is the frameset widget. */
   void *frameset;

   /* document name (as used in 'target=window_or_frame_name' attribute */
   gchar *name;
#if 0
   /* The "Headings" and "Anchors" menus */
   DwWidget *pagemarks_menu;
   DwWidget *pagemarks_last;
#endif
   /* Current cursor type */
   HCURSOR CursorType;

   /* A list of active cache clients in the window (The primary Key) */
   gint *RootClients;
   gint NumRootClients;
   gint MaxRootClients;

   /* Image Keys for all active connections in the window */
   gint *ImageClients;
   gint NumImageClients;
   gint MaxImageClients;
   /* Number of different images in the page */
   gint NumImages;
   /* Number of different images already loaded */
   gint NumImagesGot;
   /* the current document progress */
   gfloat progress;
   /* 'ready' is true when the root document has finished loading */
   gboolean ready;

   /* List of all Urls requested by this page (and its types) */
   DdUrls *PageUrls;
   gint NumPageUrls;
   gint MaxPageUrls;

   /* mSpider navigation stack (holds indexes to history list) */
   gint *nav_stack;
   gint nav_stack_size;       /* [1 based] */
   gint nav_stack_size_max;
   /* 'nav_stack_ptr' refers to what's being displayed */
   gint nav_stack_ptr;        /* [0 based] */
   /* When the user clicks a link, the URL isn't pushed directly to history;
    * nav_expect_url holds it until the first answer-bytes are got. Only then
    * it is sent to history and referenced in 'nav_stack[++nav_stack_ptr]' */
   mSpiderUrl *nav_expect_url;
   /* 'nav_expecting' is true if the last URL is being loaded for
    * the first time and has not gotten the dw yet. */
   gboolean nav_expecting;

   /* Counter for the number of hops on a redirection. Used to stop
    * redirection loops (accounts for WEB_RootUrl only) */
   gint redirect_level;

   mSpiderUrl *auth_await_url;

   gint is_iframe;

#ifdef JS_SUPPORT
	jsobject *jsobj;	/* js object: window or tab window */
	void* jsinterpreter;    /* js object: interpreter */
	char *pagetitle;
#endif	
};

#endif /* __BROWSER_H__ */

