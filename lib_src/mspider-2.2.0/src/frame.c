/*
 ** $Id: frame.c,v 1.12 2006-06-03 09:48:07 jpzhang Exp $
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
#ifndef _NOUNIX_
#include <unistd.h>
#endif
#include <minigui/common.h>
#include <minigui/minigui.h>
#include <minigui/gdi.h>
#include <minigui/window.h>
#include <minigui/control.h>
#include "emspider.h"
#include "frame.h"

static int FrameControlProc (HWND hwnd, 
                int message, WPARAM wParam, LPARAM lParam)
{
    return DefaultControlProc (hwnd, message, wParam, lParam);
}


BOOL register_frame_control (void)
{
    WNDCLASS MyClass;

    MyClass.spClassName = CTRL_FRAME;
    MyClass.dwStyle     = WS_NONE;
    MyClass.dwExStyle   = WS_EX_NONE;
    MyClass.hCursor     = GetSystemCursor (IDC_ARROW);
    MyClass.iBkColor    = COLOR_lightwhite;
    MyClass.WinProc     = FrameControlProc;

    return RegisterWindowClass (&MyClass);
}


void unregister_frame_control (void)
{
    UnregisterWindowClass (CTRL_FRAME);
}

