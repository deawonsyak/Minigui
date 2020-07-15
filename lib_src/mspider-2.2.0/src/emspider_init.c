/*
** $Id: emspider_init.c,v 1.27 2007-09-10 03:23:20 jpzhang Exp $
**
** emspider.c: The main entry of a simple emSpider test program.
**
** Copyright (C) 2005 Feynman Software.
*/

/*
**  This source is free software; you can redistribute it and/or
**  modify it under the terms of the GNU General Public
**  License as published by the Free Software Foundation; either
**  version 2 of the License, or (at your option) any later version.
**
**  This software is distributed in the hope that it will be useful,
**  but WITHOUT ANY WARRANTY; without even the implied warranty of
**  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
**  General Public License for more details.
**
**  You should have received a copy of the GNU General Public
**  License along with this library; if not, write to the Free
**  Software Foundation, Inc., 59 Temple Place - Suite 330, Boston,
**  MA 02111-1307, USA
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <minigui/mgconfig.h>
#include <minigui/common.h>
#include <minigui/minigui.h>
#include <minigui/gdi.h>
#include <minigui/window.h>
#include <minigui/control.h>

#include <unistd.h>

#include "glib.h"
#include "gtype.h"

#include "mgdconfig.h"
#include "interface.h"

#include "url.h"
#include "doc.h"
#include "nav.h"
#include "dw_widget.h"
#include "dw_viewport.h"
#include "browser.h"

#include "emspider.h"
#include "mspider.h"

extern BrowserWindow *current_browser; // this is the only browser
extern GList *browser_window;

extern BrowserWindow* a_BrowserWindow_new (HWND hwnd_parent);
extern gchar *a_Misc_encode_base64(const gchar *in);
extern int stop_flag;


MSPIDER_SETUP_INFO mspider_setup_info;
char* set_charset = NULL;           /* The global charset*/
float set_font_factor = 0 ;         /* The global font factor*/

ON_NEW_BW g_on_new_bw = NULL;


static GMainLoop *main_loop = NULL; /* The main event loop for mspider*/
static char* home_url = NULL;       /* The home page url for mspider*/
static GHashTable *g_hwnd_bw_table = NULL; /* Store the hwnd and bw*/


static gint Bw_hash_equal (gconstpointer v1, gconstpointer v2)
{
    return (v1 == v2);
}

void a_Bw_map_init (void)
{
    g_hwnd_bw_table = g_hash_table_new(g_int_hash, Bw_hash_equal);
}

void a_Bw_map_free (void)
{
   g_hash_table_destroy(g_hwnd_bw_table);
}

void a_Bw_map_insert (HWND hMainWnd, HBW hbw)
{
   g_hash_table_insert(g_hwnd_bw_table, (gpointer)hMainWnd, (gpointer)hbw);
}

void a_Bw_map_remove (HWND hMainWnd)
{
   g_hash_table_remove(g_hwnd_bw_table, (gpointer)hMainWnd);
}

HBW mspider_window_get_hbw (HWND hMainWnd)
{
   return (HBW)g_hash_table_lookup(g_hwnd_bw_table, (gpointer)hMainWnd);
}

static inline int bw_is_valid (HBW hbw)
{
    if (g_list_find(browser_window, (gpointer)hbw))
        return 1;

    return 0;
}

static inline BrowserWindow * HBW2PBW (HBW hbw)
{
    return bw_is_valid(hbw) ? ((BrowserWindow*)hbw) : NULL ;
}


static gboolean pending_msg_callback (gpointer data)
{
    HWND hwnd = (HWND) data;
    MSG msg;
    if (HavePendingMessage (hwnd)) {
        if (!GetMessage (&msg, hwnd)) {
            return FALSE;
        }

        TranslateMessage (&msg);
        DispatchMessage (&msg);
    }
    else
        g_usleep (5000);

    return TRUE;
}

HWND mspider_init(HWND hosting, ON_NEW_BW on_new_bw, 
                    int xpos, int ypos, int width, int height, const char *url)
{
    int len;
    HWND hMainWnd;

    if (NULL == on_new_bw)
        return HWND_INVALID;

    if (url) {
        len = strlen(url);
        home_url = (char*)malloc(len + 1);
        memset(home_url, 0, len+1);
        strcpy(home_url, url);
    }

    g_type_init ();

    if (!emspider_init ()) {
        printf ("emSpider: Can not init emSpider internals.\n");
        return HWND_INVALID;
    }
    g_on_new_bw = on_new_bw; /* Set global funtion of create a new biowser window. */

    hMainWnd = on_new_bw(hosting, xpos, ypos, width, height, NEW_BW_TOOLBAR | NEW_BW_LOCATIONBAR 
            | NEW_BW_STATUSBAR | NEW_BW_PROGRESSBAR | NEW_BW_MENUBAR);

    return hMainWnd;
}


/* update UI information in the MSG_IDLE of main window */
/* hMainWnd: the handle of main window */
void mspider_enter_event_loop (HWND hMainWnd)
{
    if (hMainWnd == HWND_INVALID)
        return;

    main_loop = g_main_loop_new (NULL, FALSE);
    g_idle_add_full (G_PRIORITY_HIGH_IDLE,
                    (GSourceFunc)pending_msg_callback, (void*)hMainWnd, NULL);

    g_main_loop_run (main_loop);
    g_idle_remove_by_data ((void*)hMainWnd);
}

void mspider_browser_destroy(HBW hbw)
{
    HWND hMainWnd;
    BrowserWindow* bw = HBW2PBW(hbw);

    if (!bw) return;

    hMainWnd = bw->main_window;
    a_Bw_map_remove (hMainWnd);
    
    DestroyMainWindow (hMainWnd);
    a_Interface_quit(bw);

    if (bw == current_browser) {
        PostQuitMessage (hMainWnd);
        g_main_loop_quit (main_loop);
    }
}

void  mspider_cleanup(HWND hMainWnd)
{
    if (hMainWnd == HWND_INVALID)
        return;
    g_main_loop_unref (main_loop);
    MainWindowThreadCleanup (hMainWnd);
    if (set_charset)
        free (set_charset);

    if(home_url) {
        free (home_url);
        home_url = NULL;
    }
    emspider_cleanup();
}

/************************************************
* hMainWindow: the handle of the main window
* 
* cb_info: status, progress, message, close information 
* 
* Return Value: 
*
* ***************************************/
HBW mspider_browser_new(HWND hMainWindow, const CB_INFO* cb_info)
{
    HBW hbw;
    BrowserWindow* bw = NULL;
    gal_pixel pixel;

    if ((cb_info == NULL) || (hMainWindow == HWND_INVALID))
		return INVALID_HBW;

    bw = a_BrowserWindow_new (hMainWindow);

	if ( !bw ) 
		return INVALID_HBW;
	
#ifdef JS_SUPPORT

	bw->jsobj = g_malloc0(sizeof(jsobject));
	if ( !(bw->jsobj) ) {
		return INVALID_HBW;
	}

	bw->jsobj->htmlobj = bw;
	bw->jsobj->jstype = jsmainwindow;
#endif

  /*create a new mSpiderDoc object*/  
    bw->dd = a_Doc_new();

    if (current_browser == NULL) 
        current_browser = bw;

    bw->client_rc.left = cb_info->rect.x;
    bw->client_rc.top = cb_info->rect.y;
    bw->client_rc.right = cb_info->rect.x + cb_info->rect.width;
    bw->client_rc.bottom = cb_info->rect.y + cb_info->rect.height;

#ifdef JS_SUPPORT
	if ( !(bw->dd) || !(bw->dd->jsobj) ) {
		return INVALID_HBW;
	}

	bw->dd->jsobj->jstype = jstabwindow;
	bw->dd->jsobj->jsparent = bw->jsobj;
	bw->jsobj->children =
		g_list_append(bw->jsobj->children, bw->dd->jsobj);

#endif

    a_Doc_set_browserwindow(bw->dd , bw);

    a_Doc_CreateEx(bw->dd , hMainWindow , 
             WS_CHILD | WS_VISIBLE | WS_BORDER | WS_VSCROLL | WS_HSCROLL,
             cb_info->rect.x, cb_info->rect.y , cb_info->rect.width , 
             cb_info->rect.height , bw->dd->mspider_id, home_url);

    pixel = RGB2Pixel(HDC_SCREEN, 212, 208, 200);
    SetWindowBkColor (hMainWindow, pixel);
    SetFocus (bw->dd->docwin);

#ifdef ENABLE_LINKTRAVE
    bw->linktrave_manage = a_Create_LinktraveManage();
    ((LinktraveManage*)bw->linktrave_manage)->top_dd = (void*)bw->dd;
#endif

    bw->CB_MESSAGE_BOX = cb_info->CB_MESSAGE_BOX; 
    bw->CB_CONFIRM_BOX = cb_info->CB_CONFIRM_BOX;
    bw->CB_PROMPT_BOX = cb_info->CB_PROMPT_BOX;
    bw->CB_SET_LOCATION = cb_info->CB_SET_LOCATION;
    bw->CB_GET_LOCATION = cb_info->CB_GET_LOCATION;
    bw->CB_SET_STATUS = cb_info->CB_SET_STATUS;
    bw->CB_GET_STATUS = cb_info->CB_GET_STATUS;
    bw->CB_SET_PROGRESS = cb_info->CB_SET_PROGRESS;
    bw->CB_GET_PROGRESS = cb_info->CB_GET_PROGRESS;

    hbw = (HBW)bw;
    a_Bw_map_insert (hMainWindow, hbw);
    return hbw;
}


/****************************************
* hMainWindow: the handle of main window
* 
* hbw: the structure of HBW
* 
* op: Type of operation: forward, backward,
*     home, goto, reload, stop 
*     NAV_PREV, NAV_NEXT, NAV_HOME, 
*     NAV_GOTO, NAV_RELOAD, NAV_STOP
*
* data: additional data such as URL.
*
* Function: navigation
* 
* Return Value: return 1 when success, 0 if failure.
*
* ***************************************/
BOOL mspider_navigate(HBW hbw, int op, void* data)
{
    mSpiderDoc *dd;
    DwViewport* viewport;
    BrowserWindow* bw = HBW2PBW(hbw);

    if (!bw) return FALSE;

    dd = (mSpiderDoc*)bw->dd;
    viewport = (DwViewport*)dd->viewport;

    switch (op)
    {
        case NAV_BACKWARD:
            if(viewport == NULL)
                break;
            a_Nav_back (dd);
            break;

        case NAV_FORWARD:
            if(viewport == NULL)
                break;
            a_Nav_forw (dd);
            break;

        case NAV_HOME:
            a_Interface_open_url_string (home_url, dd);
			printf("#######NAV_HOME:%s\n", home_url);
            break;

        case NAV_GOTO:
            SendMessage (bw->dd->docwin, MGD_OPENURL, 0, (LPARAM)data);
            break;

        case NAV_RELOAD:
            stop_flag = 1;
            a_Nav_reload (dd);
            break;

        case NAV_STOP:
            stop_flag = 0;
            a_Nav_cancel_expect (dd);
            a_Interface_stop (dd);
            break;
    }
    return TRUE;
}

BOOL mspider_bw_canstop(HBW hbw)
{
    if(stop_flag == 1)
        return TRUE;  
    else
        return FALSE; 
}

BOOL mspider_bw_canback(HBW hbw)
{
    gint idx;
    mSpiderDoc *dd = NULL;
    BrowserWindow* bw = HBW2PBW(hbw);

    if (!bw) return FALSE;

    dd = (mSpiderDoc*)bw->dd;
    idx = a_Nav_stack_ptr(dd);
    if ((idx - 1) >= 0 )
        return TRUE;
    else
        return FALSE;
}

BOOL mspider_bw_canforward(HBW hbw)
{
    gint idx;
    mSpiderDoc *dd = NULL;

    BrowserWindow* bw = HBW2PBW(hbw);

    if (!bw) return FALSE;

    dd = (mSpiderDoc*)bw->dd;
    idx = a_Nav_stack_ptr(dd);
    if ((idx+1) < a_Nav_stack_size(dd))
        return TRUE;
    else
        return FALSE;
}

BOOL mspider_setup(const MSPIDER_SETUP_INFO* set_info)
{
    if (set_info == NULL)
        return FALSE;
    
    set_font_factor = set_info->font_factor;

    if(strcmp(set_info->charset, "GB2312-0") == 0 
            || strcmp(set_info->charset, "EUC-KR") == 0
            || strcmp(set_info->charset, "ISO8859-1") == 0
            || strcmp(set_info->charset, "JISX0208-0") == 0
            || strcmp(set_info->charset, "EUC-JP") == 0)
        set_charset = strdup(set_info->charset);
    else
    {
        fprintf(stderr, "set charset failure\n");
        return FALSE;
    }
    return TRUE;
}

extern mSpiderUrl *HTTP_Proxy;
extern gchar *HTTP_Proxy_Auth_base64;

BOOL mspider_set_proxy(const PROXY_INFO * proxy_info)
{
    char *accr = NULL;
    int len = 0;

    if(proxy_info == NULL || proxy_info->address == NULL)
        return FALSE;
    HTTP_Proxy = a_Url_new(proxy_info->address, NULL, 0, 0, 0);
    HTTP_Proxy->port = proxy_info->port;
    len = strlen(proxy_info->username) + strlen(proxy_info->password);
    accr =g_malloc0(len +2);  
    memset(accr, 0, sizeof(accr));
    strcpy(accr, proxy_info->username);
    strcat(accr, ":");
    strcat(accr, proxy_info->password);
    HTTP_Proxy_Auth_base64 = a_Misc_encode_base64(accr);
    g_free (accr);

    return TRUE;
}


HWND mspider_bw_get_hwnd (HBW hbw)
{
    BrowserWindow* bw = HBW2PBW(hbw);
    if (!bw) 
        return HWND_INVALID;

    return bw->dd->docwin;
}



