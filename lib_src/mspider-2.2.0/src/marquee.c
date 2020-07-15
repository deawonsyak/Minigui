/*
 ** $Id: marquee.c,v 1.5 2006-06-03 09:48:07 jpzhang Exp $
 **
 ** Copyright (C) 2005-2006 Feynman Softwaver.
 **
 ** License GPL
 **
 */

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>

#include "mgdconfig.h"

#ifdef _NOUNIX_
#include <common.h>
#include <minigui.h>
#include <gdi.h>
#include <window.h>
#include <control.h>
#else
#include <unistd.h>
#include <minigui/common.h>
#include <minigui/minigui.h>
#include <minigui/gdi.h>
#include <minigui/window.h>
#include <minigui/control.h>
#endif
#include "emspider.h"
#include "marquee.h"

static int MarqueeControlProc (HWND hwnd, 
                int message, WPARAM wParam, LPARAM lParam)
{
    HDC hdc;
   // int scan_code = LOWORD(wParam); 

    switch (message) {
    case MSG_CREATE:
        return 0;

    case MSG_PAINT:
        hdc = BeginPaint (hwnd);
        EndPaint (hwnd, hdc);
        return 0;

    case MSG_HSCROLL:
        //on_hscrollbar (hwnd, wParam, lParam);
        break;

    case MSG_VSCROLL:
        //on_vscrollbar (hwnd, wParam, lParam);
        break;

    case MSG_KEYDOWN:
        break;

    case MSG_LBUTTONUP:
    case MSG_MOUSEMOVE:
    case MSG_LBUTTONDOWN:
        //on_mouse_event (hwnd, LOSWORD (lParam), HISWORD (lParam), 
         //           MAKELONG (wParam, message));
        break;

    case MSG_DESTROY:
        //on_destroy (hwnd);
        break;

    case MGD_REALIZED:
        //on_realized (hwnd, GetWindowCaption (hwnd));
        break;

    case MGD_OPENURL:
    case MSG_BROWSER_GOTOPAGE:
        //return MGD_open_url (hwnd, (const char*)lParam);
        break;

    case MGD_RESETCONTENT:
        //reset_content ((mSpiderDoc*) GetWindowAdditionalData2 (hwnd));
        return 0;

    case MGD_RESIZE:
        {
            mSpiderDoc* dd = (mSpiderDoc*)GetWindowAdditionalData2 (hwnd);
            return a_Dw_viewport_draw_resize ((DwViewport*)dd->viewport);
        }
    }

    return DefaultControlProc (hwnd, message, wParam, lParam);
}


BOOL register_marquee_control (void)
{
    WNDCLASS MyClass;

    MyClass.spClassName = CTRL_MARQUEE;
    MyClass.dwStyle     = WS_NONE;
    MyClass.dwExStyle   = WS_EX_NONE;
    MyClass.hCursor     = GetSystemCursor (IDC_ARROW);
    MyClass.iBkColor    = COLOR_lightwhite;
    MyClass.WinProc     = MarqueeControlProc;

    return RegisterWindowClass (&MyClass);
}

void unregister_marquee_control(void)
{
    UnregisterWindowClass (CTRL_MARQUEE);
}

