

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <minigui/common.h>
#include <minigui/minigui.h>
#include <minigui/gdi.h>
#include <minigui/window.h>
#include <minigui/control.h>

#include "emspider.h"
#include "interface.h"
#include "browser.h"
#include "p_window.h"
#include "mspider.h"


extern ON_NEW_BW g_on_new_bw;
extern HBW mspider_window_get_hbw (HWND hMainWnd);

void* a_Pop_Window (HWND hParentWnd, const char * name, unsigned int flags,
                                 int x, int y, int w, int h, const char * url)
{
    HWND hwnd;
    mSpiderDoc * dd = NULL;

    hwnd = g_on_new_bw(hParentWnd, x, y, w, h, flags);

    if (hwnd) {
        dd =((BrowserWindow*)mspider_window_get_hbw (hwnd))->dd;
        if (url)
            a_Interface_open_url_string (url, dd);
    }
 
    return dd; 
}
