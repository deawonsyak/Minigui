/* 
** $Id: emspider.c,v 1.61 2007-09-14 02:27:05 jpzhang Exp $
**
** Copyright (C) 2005-2006 Feynman Software.
**
** License: GPL
*/

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>

#include "mgdconfig.h"

#ifdef _NOUNIX_
#include <usrLib.h>
#else
#include <unistd.h>
#endif

#include <minigui/common.h>
#include <minigui/minigui.h>
#include <minigui/gdi.h>
#include <minigui/window.h>
#include <minigui/control.h>
#include "emspider.h"

#include "cache.h"
#include "html.h"
#include "web.h"
#include "prefs.h"
#include "history.h"
#include "nav.h"
#include "mspider.h"
#include "frame.h"
#include "dns.h"
#include "dicache.h"
#include "io/mime.h"
#include "io/url_io.h"
#include "interface.h"
#include "cookies.h"
#include "doc.h"
#include "url.h"
#include "js.h"
#include "debug.h"
#include "mflash.h"


extern gchar *a_Misc_prepend_user_home(const char *file);

extern BLOCKHEAP __msp_cliprc_heap;

void page_error(mSpiderDoc *dd, int errop)
{
#ifndef _NOUNIX_
    char burl[64];
    if (errop == ERR_DNSSOLVE){
        sprintf (burl, "about:dnserr"); 
        a_Interface_open_url_string(burl, dd);
    }else if (errop == ERR_NETCONNECT) {
        sprintf (burl, "about:networkerr"); 
        a_Interface_open_url_string(burl, dd);
    }
#else
    if (errop == ERR_DNSSOLVE){
    	MessageBox (dd->docwin , "Dns can't solve the host address!\n", "Error Message", MB_OK);
    }else if (errop == ERR_NETCONNECT) {
    	MessageBox (dd->docwin , "Unable connect to the host address!\n", "Error Message", MB_OK);
    }
#endif
}

static void reset_content (mSpiderDoc* dd)
{
    DwViewport* viewport = (DwViewport*)dd->viewport;

    if (viewport == NULL || viewport->child == NULL) {
        return;
    }

    g_signal_emit_by_name (G_OBJECT(viewport->child), "destroy", 0);

}

int MGD_open_url (HWND hwnd, const char* url_str)
{
    mSpiderDoc* dd = (mSpiderDoc*) GetWindowAdditionalData2 (hwnd);
    DwViewport* viewport = (DwViewport*)dd->viewport;

    if (viewport == NULL)
        return URL_ERROR;

    a_Interface_open_url_string ((gchar*)url_str, dd);

    return 0;
}

int MGD_open_url_ismap (HWND hwnd, const mSpiderUrl* url_ismap)
{
    int ret;

    mSpiderUrl* url = a_Url_dup (url_ismap);

    if (url->ismap_url_len)
        g_string_truncate (url->url_string, url->ismap_url_len);

    ret = MGD_open_url (hwnd, url->url_string->str);

    a_Url_free (url);

    return ret;
}

static void on_realized (HWND hwnd)
{

}

static void on_paint (HWND hwnd, HDC hdc)
{
    RECT parent_rc, child_rc, ints_rc;
    DwRectangle area;
    mSpiderDoc* dd = NULL;
    DwViewport* viewport = NULL;

    dd = (mSpiderDoc*) GetWindowAdditionalData2 (hwnd);
    if(dd == NULL)
        return;
    viewport = (DwViewport*)dd->viewport;
    if (viewport == NULL || viewport->child == NULL) {
        return;
    }

    GetClientRect (hwnd, &parent_rc);

    parent_rc.left += viewport->world_x;
    parent_rc.top += viewport->world_y;
    parent_rc.right += viewport->world_x;
    parent_rc.bottom += viewport->world_y;

    child_rc.left = viewport->child->allocation.x;
    child_rc.top = viewport->child->allocation.y;
    child_rc.right = viewport->child->allocation.width + child_rc.left;
    child_rc.bottom = DW_WIDGET_HEIGHT(viewport->child) + child_rc.top;

    if (IntersectRect (&ints_rc, &parent_rc, &child_rc)) {
        ints_rc.left -= viewport->child->allocation.x;
        ints_rc.right -= viewport->child->allocation.x;
        ints_rc.top -= viewport->child->allocation.y;
        ints_rc.bottom -= viewport->child->allocation.y;

        area.x = ints_rc.left;
        area.y = ints_rc.top;
        area.width = ints_rc.right - ints_rc.left;
        area.height = ints_rc.bottom - ints_rc.top;

        a_Dw_widget_draw (viewport->child, hdc , &area);
    }

}

static void on_destroy (HWND hwnd)
{
    mSpiderDoc* dd = (mSpiderDoc*) GetWindowAdditionalData2 (hwnd);
    DwViewport* viewport = (DwViewport*)dd->viewport;
    if (viewport == NULL)
        return;

    reset_content (dd);

    a_Doc_destroy(dd); 
}

#define SCROLL_LINE     10
#define SCROLL_PAGE     100

static void on_hscrollbar (HWND hwnd, int event, int param)
{
    int scroll = 0;
    gint32 world_width;
    mSpiderDoc* dd = (mSpiderDoc*) GetWindowAdditionalData2 (hwnd);
    DwViewport* viewport = (DwViewport*)dd->viewport;

    if (viewport == NULL || viewport->child == NULL) {
        return;
    }

    world_width = viewport->child->allocation.width;

    switch (event) {
    case SB_LINERIGHT:
        if (viewport->world_x + viewport->allocation.width < world_width) {
            scroll = world_width - viewport->world_x - viewport->allocation.width;
            if (scroll > SCROLL_LINE) scroll = SCROLL_LINE;
        }
        break;
                
    case SB_LINELEFT:
        if (viewport->world_x > 0) {
            scroll = -viewport->world_x;
            if (scroll < -SCROLL_LINE) scroll = -SCROLL_LINE;
        }
        break;
                
    case SB_PAGERIGHT:
        if (viewport->world_x + viewport->allocation.width < world_width) {
            scroll = world_width - viewport->world_x - viewport->allocation.width;
            if (scroll > SCROLL_PAGE) scroll = SCROLL_PAGE;
        }
        break;

    case SB_PAGELEFT:
        if (viewport->world_x > 0) {
            scroll = -viewport->world_x;
            if (scroll < -SCROLL_PAGE) scroll = -SCROLL_PAGE;
        }
        break;

    case SB_THUMBPOSITION:
		UpdateWindow(hwnd, TRUE);
		break;

    case SB_THUMBTRACK:
        scroll = param - viewport->world_x;
        if ( viewport->world_x + viewport->allocation.width+ scroll > world_width) {
            scroll = world_width - viewport->world_x - viewport->allocation.width;
        }
        break;
    }

    if (scroll) {
        viewport->world_x = viewport->world_x + scroll;
        SetScrollPos (hwnd, SB_HORZ, viewport->world_x);
        ScrollWindow (hwnd, -scroll, 0, NULL, NULL);
    }
}

void MGD_scroll_to_x (DwViewport *viewport, int x)
{
    int world_width;
    int scroll;

    if (viewport == NULL || viewport->child == NULL) {
        return;
    }

    world_width = viewport->child->allocation.width;

    if (x < 0)
        x = 0;
    if (x + viewport->allocation.width > world_width)
        x = world_width - viewport->allocation.width;

    scroll = x - viewport->world_x;
    if (scroll) {
        viewport->world_x = x;
        SetScrollPos (viewport->hwnd, SB_HORZ, x);
        ScrollWindow (viewport->hwnd, -scroll, 0, NULL, NULL);
    }
}

static void on_vscrollbar (HWND hwnd, int event, int param)
{
    int scroll = 0;
    gint32 world_height;
    mSpiderDoc* dd = (mSpiderDoc*) GetWindowAdditionalData2 (hwnd);
    DwViewport* viewport = (DwViewport*)dd->viewport;

    if (viewport == NULL || viewport->child == NULL) {
        return;
    }

    world_height = viewport->child->allocation.ascent 
            + viewport->child->allocation.descent;

    switch (event) {
    case SB_LINEDOWN:
        if (viewport->world_y + viewport->allocation.height < world_height) {
            scroll = world_height - viewport->world_y 
                    - viewport->allocation.height;
            if (scroll > SCROLL_LINE) scroll = SCROLL_LINE;
        }
        break;
                
    case SB_LINEUP:
        if (viewport->world_y > 0) {
            scroll = -viewport->world_y;
            if (scroll < -SCROLL_LINE) scroll = -SCROLL_LINE;
        }
        break;
                
    case SB_PAGEDOWN:
        if (viewport->world_y + viewport->allocation.height < world_height) {
            scroll = world_height - viewport->world_y 
                    - viewport->allocation.height;
            if (scroll > SCROLL_PAGE) scroll = SCROLL_PAGE;
        }
        break;

    case SB_PAGEUP:
        if (viewport->world_y > 0) {
            scroll = -viewport->world_y;
            if (scroll < -SCROLL_PAGE) scroll = -SCROLL_PAGE;
        }
        break;

    case SB_THUMBPOSITION:
		UpdateWindow(hwnd, TRUE);
		break;

    case SB_THUMBTRACK:
        scroll = param - viewport->world_y;
        if ( viewport->world_y + viewport->allocation.height + scroll > world_height ) {
           scroll = world_height - viewport->world_y - viewport->allocation.height;
        }
        break;
    }

    if (scroll) {
        viewport->world_y = viewport->world_y + scroll;
        SetScrollPos (hwnd, SB_VERT, viewport->world_y);
        ScrollWindow (hwnd, 0, -scroll, NULL, NULL);
    }
}

void MGD_scroll_to_y (DwViewport *viewport, int y)
{
    gint32 world_height;
    int scroll;

    if (viewport == NULL || viewport->child == NULL) {
        return;
    }

    world_height = viewport->child->allocation.ascent 
            + viewport->child->allocation.descent;

    if (y < 0)
        y = 0;
    if (y + viewport->allocation.height > world_height)
        y = world_height - viewport->allocation.height;

    scroll = y - viewport->world_y;
    if (scroll) {
        viewport->world_y = y;
        SetScrollPos (viewport->hwnd, SB_VERT, y);
        ScrollWindow (viewport->hwnd, 0, -scroll, NULL, NULL);
    }
}

static BOOL on_mouse_event (HWND hwnd, int x, int y, DWORD flags)
{
    DwWidget *dw_widget;
    gint32 world_x, world_y;
    mSpiderDoc* dd = (mSpiderDoc*) GetWindowAdditionalData2 (hwnd);
    DwViewport* viewport = (DwViewport*)dd->viewport;

    if (viewport == NULL || viewport->child == NULL) {
        return FALSE;
    }

    world_x = x + viewport->world_x;
    world_y = y + viewport->world_y;
    dw_widget = Dw_viewport_widget_at_point (viewport, world_x, world_y);

    return Dw_widget_mouse_event (dw_widget, viewport, world_x, world_y, flags);
}

static int navigation (HWND hwnd, int message)
{
    mSpiderDoc* dd = (mSpiderDoc*) GetWindowAdditionalData2 (hwnd);
    DwViewport* viewport = (DwViewport*)dd->viewport;

    if (viewport == NULL) {
        return URL_ERROR;
    }

    if (message == MGD_NAV_BACKWARD)
        a_Nav_back (dd);
    else
        a_Nav_forw (dd);

    return 0;
}

static void on_keydown (HWND hwnd, int scancode, DWORD ks)
{
    mSpiderDoc* dd = (mSpiderDoc*) GetWindowAdditionalData2 (hwnd);
#ifdef ENABLE_LINKTRAVE
   mSpiderDoc * ldd;
#endif
    DwViewport* viewport = (DwViewport*)dd->viewport;
    mSpiderHtmlLB* lb= (mSpiderHtmlLB*) dd->html_block;
    mSpiderHtmlForm* form;
    long world_height = 0;

#ifdef ENABLE_LINKTRAVE
    mSpiderDoc *named_dd;
    gchar * target;
    LinktraveManage *linktrave_manage;
#ifdef JS_SUPPORT
    char *urlstring;
   jsstring code;
   char *jsstr = NULL;
   jsobject *jsobj = NULL;
   jsscript_interpreter *interpreter = NULL;
#endif
#endif
    if (viewport == NULL || viewport->child == NULL) {
        return;
    }


    world_height = viewport->child->allocation.ascent 
            + viewport->child->allocation.descent;

#ifdef ENABLE_LINKTRAVE
    linktrave_manage =(LinktraveManage*)dd->bw->linktrave_manage ;
#endif
    switch (scancode) {
    case SCANCODE_HOME:
        MGD_scroll_to_x (viewport, 0);
        MGD_scroll_to_y (viewport, 0);
        break;

    case SCANCODE_END:
        MGD_scroll_to_y (viewport, world_height); 
        break;

     /* The following is for chang xin jia company */
    case SCANCODE_F1:
        on_vscrollbar (hwnd, SB_LINEUP, 0);
        break;

    case SCANCODE_F2:
        on_vscrollbar (hwnd, SB_LINEDOWN, 0);
        break;

    case SCANCODE_F3:
        on_hscrollbar (hwnd, SB_LINELEFT, 0);
        break;

    case SCANCODE_F4:
        on_hscrollbar (hwnd, SB_LINERIGHT, 0);
        break;

	case SCANCODE_F5:	//refresh
    	UpdateWindow(hwnd, TRUE);
		break;

	case SCANCODE_F6:
		break;
		
	case SCANCODE_F7:	//back
        navigation (hwnd, MGD_NAV_BACKWARD);
		break;

	case SCANCODE_F8:	//fwd
        navigation (hwnd, MGD_NAV_FORWARD);
		break;
     /* end for chang xin jia company */

    case SCANCODE_CURSORBLOCKDOWN:
        on_vscrollbar (hwnd, SB_LINEDOWN, 0);
        break;

    case SCANCODE_CURSORBLOCKUP:
        on_vscrollbar (hwnd, SB_LINEUP, 0);
        break;

    case SCANCODE_CURSORBLOCKLEFT:
#ifdef ENABLE_LINKTRAVE
        linktrave_manage->link_number_focus--; 

         while (!a_Linktrave_step (linktrave_manage->linktrave_state,
                  linktrave_manage->link_number_focus, LTS_BACKWARD))
         {
                if (linktrave_manage->stack_top == 0)
                {
                    /*top level widget*/
                    if (linktrave_manage->link_number_focus < 0 )
                        linktrave_manage->link_number_focus = lb->num_links; /* loop */
                    break;
                }
                else if (linktrave_manage->linktrave_state->mode == LSM_MULIT)
                {
                    linktrave_manage->linktrave_state->cur_dd--;                     
                    if (linktrave_manage->linktrave_state->cur_dd < 0 )
                        linktrave_manage->linktrave_state->cur_dd = linktrave_manage->linktrave_state->nr_dd -1;

                    ldd = linktrave_manage->linktrave_state->ddary[linktrave_manage->linktrave_state->cur_dd]; 
                     a_Linktrave_state_set_widget (linktrave_manage->linktrave_state,
                                                 ((DwViewport*)(ldd->viewport))->child);
                    linktrave_manage->linktrave_state->link_block = ldd->html_block; 
                    linktrave_manage->link_number_focus = ((mSpiderHtmlLB*)(ldd->html_block))->num_links-1;

                    SendMessage (ldd->docwin, MSG_SETFOCUS, 0, 0);                    
                   continue;
                }
                else /* LSM_SINGLE mode */
                {
                   linktrave_manage->stack_top--; 
                   a_Linktrave_state_destroy (linktrave_manage->linktrave_state);
                   linktrave_manage->linktrave_state = 
                        linktrave_manage->stack[linktrave_manage->stack_top];
                   linktrave_manage->link_number_focus = linktrave_manage->linktrave_state->link_no; 
                   linktrave_manage->link_number_focus--; 
                   continue;
                }
         }
#endif
        break;

    case SCANCODE_CURSORBLOCKRIGHT:

#ifdef ENABLE_LINKTRAVE
        linktrave_manage->link_number_focus++; 

         while (!a_Linktrave_step (linktrave_manage->linktrave_state,
                  linktrave_manage->link_number_focus, LTS_FORWARD))
         {
                if (linktrave_manage->stack_top == 0)
                {
                    /*top level widget*/
                    if (linktrave_manage->link_number_focus >= lb->num_links)
                        linktrave_manage->link_number_focus = -1; /* loop */
                    break;
                }
                else if (linktrave_manage->linktrave_state->mode == LSM_MULIT)
                {
                    linktrave_manage->linktrave_state->cur_dd++;                     
                    if (linktrave_manage->linktrave_state->cur_dd > (linktrave_manage->linktrave_state->nr_dd -1))
                        linktrave_manage->linktrave_state->cur_dd = 0;

                    ldd = linktrave_manage->linktrave_state->ddary[linktrave_manage->linktrave_state->cur_dd]; 
                     a_Linktrave_state_set_widget (linktrave_manage->linktrave_state,
                                                 ((DwViewport*)(ldd->viewport))->child);
                    linktrave_manage->linktrave_state->link_block = ldd->html_block; 
                    linktrave_manage->link_number_focus = 0;

                    SendMessage (ldd->docwin, MSG_SETFOCUS, 0, 0);                    
                   continue;
                }
                else /* LSM_SINGLE mode */
                {
                   linktrave_manage->stack_top--; 
                   a_Linktrave_state_destroy (linktrave_manage->linktrave_state);
                   linktrave_manage->linktrave_state = 
                        linktrave_manage->stack[linktrave_manage->stack_top];
                   linktrave_manage->link_number_focus = linktrave_manage->linktrave_state->link_no; 
                   linktrave_manage->link_number_focus++; 
                   continue;
                }
         }
#endif
        break;
    case SCANCODE_PAGEDOWN:
        if (ks & KS_ALT)
            on_hscrollbar (hwnd, SB_PAGERIGHT, 0);
        else
            on_vscrollbar (hwnd, SB_PAGEDOWN, 0);
        break;

    case SCANCODE_PAGEUP:
        if (ks & KS_ALT)
            on_hscrollbar (hwnd, SB_PAGELEFT, 0);
        else
            on_vscrollbar (hwnd, SB_PAGEUP, 0);
        break;

    case SCANCODE_F:
        if (ks & KS_CTRL)
            navigation (hwnd, MGD_NAV_FORWARD);
        break;

    case SCANCODE_B:
        if (ks & KS_CTRL)
            navigation (hwnd, MGD_NAV_BACKWARD);
        break;

    case SCANCODE_ENTER:

#ifdef ENABLE_LINKTRAVE
        if (lb->links == NULL)
            return;

        if (linktrave_manage->linktrave_state->link_block)
            lb = linktrave_manage->linktrave_state->link_block;        

        a_Linktrave_reset(linktrave_manage->linktrave_state);
               
#ifdef JS_SUPPORT
           urlstring=a_Url_str(lb->links[linktrave_manage->link_number_focus]);
          if(strstr(urlstring,"javascript:")!=0)
          { 
            jsobj = lb->dd->jsobj;
            jsobj = js_getdftdocobj((int)lb->dd);
            if ( !jsobj || !(interpreter = jsobj->jsinterpreter) )
            {
                return;
            }
            if ( interpreter && interpreter->inuse ) 
            {
                jsstr = g_strdup(urlstring);
                code.source = jsstr;
                code.length = strlen(jsstr);
                js_eval(interpreter, &code);
                free(jsstr);
            }
            return;
          }
#endif
       if((target = (gchar *) URL_TARGET_(lb->links[linktrave_manage->link_number_focus])))
       {
          if ((named_dd = a_Doc_get_by_name(dd, (gchar *) target)))
             a_Nav_push(named_dd, lb->links[linktrave_manage->link_number_focus]);
          else 
          {
                if (dd->is_iframe)
                    a_Nav_push(a_Doc_get_root(dd), lb->links[linktrave_manage->link_number_focus]);
                else
                    a_Nav_push(dd, lb->links[linktrave_manage->link_number_focus]);
          }
       }
       else
       {
        if (dd->is_iframe)
           a_Nav_push(a_Doc_get_root(dd), lb->links[linktrave_manage->link_number_focus]);
        else
           a_Nav_push(dd, lb->links[linktrave_manage->link_number_focus]);
       }
               
        UpdateWindow(hwnd, TRUE);
#endif
        break;

    case SCANCODE_F9:
        form = &lb->forms[0];
        SendMessage(form->hwnd_submit, BM_CLICK, 0, 0);
        break;

    case SCANCODE_F10:
        form = &lb->forms[1];
        SendMessage(form->hwnd_submit, BM_CLICK, 0, 0);
        break;

    case SCANCODE_F11:
        form = &lb->forms[2];
        SendMessage(form->hwnd_submit, BM_CLICK, 0, 0);
        break;

    case SCANCODE_F12:
        form = &lb->forms[3];
        SendMessage(form->hwnd_submit, BM_CLICK, 0, 0);
        break;

    default:
        break;
    }
}

/*
* It is the same as static gboolean Dw_viewport_calc_into function.
*/
static gboolean Rect_calc_into (gint32 requested_value,
                                           gint32 requested_size,
                                           gint32 current_value,
                                           gint32 size,
                                           gint32 *return_value)
{
   if (requested_size > size) {
      /* The viewport size is smaller than the size of the region which will
       * be shown. If the region is already visible, do not change the
       * position. Otherwise, show the left/upper border, this is most likely
       * what is needed. */
      if (current_value >= requested_value &&
          current_value + size < requested_value + requested_size)
         return FALSE;
      else
         requested_size = size;
   }

   if (requested_value < current_value) {
      *return_value = requested_value;
      return TRUE;
   } else if (requested_value + requested_size > current_value + size) {
      *return_value = requested_value - size + requested_size;
      return TRUE;
   } else
      return FALSE;
}

static BOOL focus_on_child (HWND hwnd, int scancode, DWORD ks)
{
    HWND hCurFocus;
    HWND hNewFocus;
    hCurFocus = GetFocusChild (hwnd);

    if (scancode != SCANCODE_TAB) {
        if (!hCurFocus)
            return FALSE;

        if (SendMessage (hCurFocus, MSG_GETDLGCODE, 0, 0L) & DLGC_WANTALLKEYS)
            return TRUE;
    }

    switch (scancode)
    {
        case SCANCODE_CURSORBLOCKDOWN:
        case SCANCODE_CURSORBLOCKUP:
        case SCANCODE_CURSORBLOCKRIGHT:
        case SCANCODE_CURSORBLOCKLEFT:
        {
            if (hCurFocus && SendMessage (hCurFocus, 
                                    MSG_GETDLGCODE, 0, 0L) & DLGC_WANTARROWS)
                return TRUE;

            if (scancode == SCANCODE_CURSORBLOCKDOWN
                    || scancode == SCANCODE_CURSORBLOCKRIGHT)
            {
                hNewFocus = GetNextDlgGroupItem (hwnd, hCurFocus, FALSE);
            }
            else
            {
                hNewFocus = GetNextDlgGroupItem (hwnd, hCurFocus, TRUE);
            }
            
            if (hNewFocus != hCurFocus) {
                if (SendMessage (hCurFocus, MSG_GETDLGCODE, 0, 0L) 
                                & DLGC_STATIC)
                    break;


                if (SendMessage (hNewFocus, MSG_GETDLGCODE, 0, 0L)
                        & DLGC_RADIOBUTTON) {
                    mSpiderDoc* dd = (mSpiderDoc*) GetWindowAdditionalData2 (hwnd);
                    DwViewport* viewport = (DwViewport*)dd->viewport;
                    if (viewport == NULL || viewport->child == NULL) {
                        return TRUE;
                    }

                    RECT rect;
                    gint32 vp_width, vp_height, x = 0, y = 0;
                    GetWindowRect( hNewFocus, &rect);
                    gint32 posx = rect.left + viewport->world_x, posy = rect.top + viewport->world_y;
                    gint32 width = rect.right - rect.left, height = rect.bottom - rect.top;

                    vp_width = viewport->allocation.width;
                    vp_height = viewport->allocation.height;

                    gboolean change_x, change_y;
                    change_x = TRUE;
                    change_x = Rect_calc_into (posx, width, viewport->world_x, vp_width, &x);
                    change_y = TRUE;
                    change_y = Rect_calc_into (posy, height, viewport->world_y, vp_height, &y);

                    if (change_x) {
                        MGD_scroll_to_x (viewport, x);
                    }

                    if (change_y) {
                        MGD_scroll_to_y (viewport, y);
                    }
                    SetFocus (hNewFocus);
                    SendMessage (hNewFocus, BM_CLICK, 0, 0L);
                    ExcludeWindowStyle (hCurFocus, WS_TABSTOP);
                    IncludeWindowStyle (hNewFocus, WS_TABSTOP);
                }
            }
        }
        break;
        case SCANCODE_TAB:
        {
            if (ks & KS_SHIFT)
                hNewFocus = GetNextDlgTabItem (hwnd, hCurFocus, TRUE);
            else
                hNewFocus = GetNextDlgTabItem (hwnd, hCurFocus, FALSE);

            if (hNewFocus != hCurFocus) {
                mSpiderDoc* dd = (mSpiderDoc*) GetWindowAdditionalData2 (hwnd);
                DwViewport* viewport = (DwViewport*)dd->viewport;
                if (viewport == NULL || viewport->child == NULL) {
                    return TRUE;
                }

                RECT rect;
                gint32 vp_width, vp_height, x = 0, y = 0;
                GetWindowRect( hNewFocus, &rect);
                gint32 posx = rect.left + viewport->world_x, posy = rect.top + viewport->world_y;
                gint32 width = rect.right - rect.left, height = rect.bottom - rect.top;

                vp_width = viewport->allocation.width;
                vp_height = viewport->allocation.height;

                gboolean change_x, change_y;
                change_x = TRUE;
                change_x = Rect_calc_into (posx, width, viewport->world_x, vp_width, &x);
                change_y = TRUE;
                change_y = Rect_calc_into (posy, height, viewport->world_y, vp_height, &y);

                if (change_x) {
                    MGD_scroll_to_x (viewport, x);
                }

                if (change_y) {
                    MGD_scroll_to_y (viewport, y);
                }

                SetFocus (hNewFocus);
            }
            break;
        }
    }
    return TRUE;
}

static int emSpiderControlProc (HWND hwnd, int message, WPARAM wParam, LPARAM lParam)
{
    HDC hdc;
    int scan_code = LOWORD(wParam); 

    switch (message) {
    case MSG_CREATE:

        return 0;

    case MSG_PAINT:
        hdc = BeginPaint (hwnd);
        on_paint (hwnd, hdc);
        EndPaint (hwnd, hdc);
        return 0;

    case MSG_HSCROLL:
        on_hscrollbar (hwnd, wParam, lParam);
        break;

    case MSG_VSCROLL:
        on_vscrollbar (hwnd, wParam, lParam);
        break;

    case MSG_KEYDOWN:
        if ( ((scan_code >= SCANCODE_F1) && (scan_code <= SCANCODE_F10)) 
                        || scan_code == SCANCODE_F11 || scan_code == SCANCODE_F12
                        || !focus_on_child (hwnd, scan_code, lParam))
            on_keydown (hwnd, scan_code ,lParam);
        break;

    case MSG_LBUTTONUP:
        SetFocus (hwnd);
    case MSG_MOUSEMOVE:
    case MSG_LBUTTONDOWN:
        on_mouse_event (hwnd, LOSWORD (lParam), HISWORD (lParam), 
                    MAKELONG (wParam, message));
        break;

    case MSG_DESTROY:
        on_destroy (hwnd);
        break;

    case MGD_REALIZED:
        on_realized (hwnd);
        break;
    case MGD_OPENURL:
    case MSG_BROWSER_GOTOPAGE:
        return MGD_open_url (hwnd, (const char*)lParam);

#if 0
    case MGD_NAV_BACKWARD:
    case MGD_NAV_FORWARD:
        return navigation (hwnd, message);
#endif

    case MGD_RESETCONTENT:
        reset_content ((mSpiderDoc*) GetWindowAdditionalData2 (hwnd));
        return 0;

    case MGD_RESIZE:
        {
            mSpiderDoc* dd = (mSpiderDoc*)GetWindowAdditionalData2 (hwnd);
            return a_Dw_viewport_draw_resize ((DwViewport*)dd->viewport);
        }
    }

    return DefaultControlProc (hwnd, message, wParam, lParam);
}

static BOOL register_emspider_control (void)
{
    WNDCLASS MyClass;

    MyClass.spClassName = CTRL_MSPIDER;
    MyClass.dwStyle     = WS_NONE;
    MyClass.dwExStyle   = WS_EX_NONE;
    MyClass.hCursor     = GetSystemCursor (IDC_ARROW);
    MyClass.iBkColor    = COLOR_lightwhite;
    MyClass.WinProc     = emSpiderControlProc;

    return RegisterWindowClass (&MyClass);
}

static void unregister_emspider_control (void)
{
    UnregisterWindowClass (CTRL_MSPIDER);
}

/*
 * Check if '~/.mspider' directory exists.
 * If not, try to create it.
 */
static void mSpider_check_home_dir(char *dir)
{
   struct stat st;

   if ( stat(dir, &st) == -1 ) {
#ifdef _NOUNIX_
      if ( errno == ENOENT && mkdir(dir) < 0 ) 
#else
      if ( errno == ENOENT && mkdir(dir, 0700) < 0 ) 
#endif
      {
         DEBUG_MSG("mSpider: error creating directory %s: %s\n", dir, g_strerror(errno));
      } 
      else
         DEBUG_MSG("mSpider: error reading %s: %s\n", dir, g_strerror(errno));
   }
}

BOOL emspider_init (void)
{
    gchar *dir;

    /* check that ~/.mspider exists, create it if it doesn't */
    dir = a_Misc_prepend_user_home(".mspider");
    mSpider_check_home_dir(dir);
    g_free(dir);

   /* Init the block ragen heap*/ 
   InitFreeClipRectList (&__msp_cliprc_heap, 100);

    a_Bw_map_init ();
    a_Prefs_init ();
    a_Dns_init ();
    a_Http_init ();
#ifdef ENABLE_SSL
   // a_Https_SSL_init ();
#endif
    a_Mime_init ();
    a_Cache_init ();
    a_Dicache_init ();
    a_Dw_style_init ();
    a_Interface_init ();
#ifdef ENABLE_COOKIES
    a_Cookies_init ();
#endif
    a_Doc_init ();

#ifdef JS_SUPPORT
	js_init();
#endif
    
    if (!register_emspider_control ()) {
        return FALSE;
    }

    if (!register_frame_control ()) {
        return FALSE;
    }
#ifdef ENABLE_FLASH
    if (!RegisterMyFlashPlayer()) {
        return FALSE;
    }
#endif
    return TRUE;
}

void emspider_cleanup (void)
{
    unregister_frame_control ();
    unregister_emspider_control ();
#ifdef JS_SUPPORT
	js_done();
#endif
#ifdef ENABLE_FLASH
   UnregisterFlashControl ();
#endif

    a_Dicache_freeall ();
    a_Cache_freeall ();
#ifdef ENABLE_SSL
    a_Https_freeall ();    
#endif

    a_Http_freeall ();
    a_Dns_freeall ();

#ifdef ENABLE_COOKIES
    a_Cookies_freeall ();
#endif

    a_Dw_style_freeall ();
    a_Prefs_freeall ();
    a_History_free ();
    a_Doc_freeall();
    a_Interface_quit_all();
    a_Bw_map_free ();
    DestroyBlockDataHeap (&__msp_cliprc_heap);
}

