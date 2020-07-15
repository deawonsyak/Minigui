
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <minigui/mgconfig.h>
#include <minigui/common.h>
#include <minigui/minigui.h>
#include <minigui/gdi.h>
#include <minigui/window.h>
#include <minigui/control.h>

#include "mgdconfig.h"

#include "mspider.h"
#include "emspider-res.h"

#ifdef _NOUNIX_
#include "resolvLib.h"
#include "netmessage.h"


extern int IPMessageBox(struct intfconfig_s *netms);
char dnsaddress[30];

#define RESOLVER_DOMAIN_SERVER  dnsaddress    /* DNS server IP address */
#define RESOLVER_DOMAIN     "" //    "power.lan.minigui.com"     /* Resolver domain */

#endif
static BITMAP ntb_bmp;

extern long toolbar_datasize;
extern const unsigned char new_toolbar_data[];
extern void create_proxy_window(HWND hParent);

extern int prompt_box(HWND parent, const char *info, const char *definp, char *buf, int buflen);
/* user define data struct*/
typedef struct _window_data
{
       HBW  hbw;
       HWND hwnd_toolbar;
       HWND hwnd_location;
       HWND hwnd_progressbar;
       HWND hwnd_statusbar;
}WINDOWDATA;

static char location_text[256];
static char status_text[256];
static char process_text[256];
static char prompt_text [256];

static char* get_location_text(HWND hMainWnd)
{
    int len;
    HWND hwnd_location = ((WINDOWDATA*)GetWindowAdditionalData(hMainWnd))->hwnd_location;
    len = GetWindowTextLength (hwnd_location);
    memset (location_text, 0 , 256);
    GetWindowText (hwnd_location, location_text, len);
    return location_text;
}
static void set_location_text (HWND hMainWnd, const char * text)
{
    HWND hwnd_location = ((WINDOWDATA*)GetWindowAdditionalData(hMainWnd))->hwnd_location;
    SetWindowText (hwnd_location, text);
}

static char* get_status_text(HWND hMainWnd)
{
    int len;
    HWND hwnd_statusbar = ((WINDOWDATA*)GetWindowAdditionalData(hMainWnd))->hwnd_statusbar;
    len = GetWindowTextLength (hwnd_statusbar);
    memset (status_text, 0 , 256);
    GetWindowText (hwnd_statusbar, status_text, len);
    return status_text;
}
static void set_status_text (HWND hMainWnd, const char * text)
{
    HWND hwnd_statusbar = ((WINDOWDATA*)GetWindowAdditionalData(hMainWnd))->hwnd_statusbar;
    SetWindowText (hwnd_statusbar, text);
}


static char* get_process_text(HWND hMainWnd)
{
    int len;
    HWND hwnd_progressbar = ((WINDOWDATA*)GetWindowAdditionalData(hMainWnd))->hwnd_progressbar;
    len = GetWindowTextLength (hwnd_progressbar);
    memset (process_text, 0 , 256);
    GetWindowText (hwnd_progressbar, process_text, len);
    return process_text;
}
static void set_process_text (HWND hMainWnd, const char * text)
{
    HWND hwnd_progressbar = ((WINDOWDATA*)GetWindowAdditionalData(hMainWnd))->hwnd_progressbar;
    SetWindowText (hwnd_progressbar, text);
}

static void my_messags_box (HWND parent, const char * text, const char * caption)
{
    MessageBox (parent, text, caption, MB_OK);
}

static BOOL my_confirm_box (HWND parent, const char * text, const char * caption)
{
    if (MessageBox (parent, text, caption, MB_OKCANCEL|MB_ICONINFORMATION) == IDOK)
        return TRUE;
    return FALSE;
}
static char * my_prompt_box (HWND parent, const char* text, const char* defaultval, const char* caption)
{
    memset (prompt_text, 0 , sizeof(prompt_text));
    if (prompt_box(parent, text, defaultval, prompt_text, sizeof(prompt_text)))
            return prompt_text;
    return NULL;
}



HWND create_status_window(HWND hwnd_parent, HWND hwnd_progressbar)
{
    HWND ctrl;
    RECT rc_bw, rc;
    int  title_height;
    int  width;
    int  height;

#if MINIGUI_MAJOR_VERSION < 3
    title_height = GetMainWinMetrics(MWM_CAPTIONY);
#else
    title_height = GetWindowElementAttr (hwnd_parent, WE_METRICS_CAPTION);
#endif

    GetWindowRect(hwnd_parent,&rc_bw);

    width = RECTW(rc_bw);
    height = RECTH(rc_bw);

    if (hwnd_progressbar)
    {
        GetWindowRect(hwnd_progressbar, &rc);
        width = width - RECTW(rc); 
    }

    ctrl = CreateWindow (CTRL_STATIC, "", 
                          WS_VISIBLE | SS_LEFT | SS_NOTIFY | WS_BORDER, 
			              IDC_STATUSBAR, 0, 
                          height - STATUSBAR_HEIGHT - title_height,
                          width, STATUSBAR_HEIGHT, hwnd_parent, 0);

    return ctrl;
}

static HWND create_progress_window(HWND hwnd_parent)
{
    HWND ctrl;
    RECT rc_bw;
    int  width;
    int  title_height;

#if MINIGUI_MAJOR_VERSION < 3
    title_height = GetMainWinMetrics(MWM_CAPTIONY);
#else
    title_height = GetWindowElementAttr (hwnd_parent, WE_METRICS_CAPTION);
#endif
    GetWindowRect (hwnd_parent, &rc_bw);

    width = rc_bw.right - rc_bw.left;

    ctrl = CreateWindow (CTRL_STATIC, "", 
                            WS_VISIBLE | SS_CENTER | SS_NOTIFY | WS_BORDER, 
                            IDC_PROGRESSBAR, 
                            width - PROGRESSBAR_WIDTH, 
                            rc_bw.bottom - STATUSBAR_HEIGHT - title_height, 
                            PROGRESSBAR_WIDTH, STATUSBAR_HEIGHT, hwnd_parent, 0);
    return ctrl;
}

static void toolbar_notif_proc (HWND hwnd, int id, int nc, DWORD add_data)
{
    HBW hbw;

    HWND parent = GetParent (hwnd);
    hbw = ((WINDOWDATA*)GetWindowAdditionalData(parent))->hbw;

    switch (nc) {
    case IDC_NAV_BACKWARD:
        mspider_navigate(hbw, NAV_BACKWARD, NULL);
        break;
    case IDC_NAV_FORWARD:
        mspider_navigate(hbw, NAV_FORWARD, NULL);
        break;
    case IDC_NAV_HOME:
        mspider_navigate(hbw, NAV_HOME, NULL);
        break;
    case IDC_NAV_STOP:
        mspider_navigate(hbw, NAV_STOP, NULL);
        break;
    case IDC_NAV_RELOAD:
        mspider_navigate(hbw, NAV_RELOAD, NULL);
        break;
    case IDC_NAV_GOTO:
        mspider_navigate(hbw, NAV_GOTO, get_location_text(parent));
        break;
    case IDC_NAV_PROXY:
        create_proxy_window(parent); 
        break;
    }
}

void location_entry_open_url(HWND hwnd, int id, int nc, DWORD add_data)
{
    HBW hbw;
    int length;
    char* url_string = NULL;

    HWND parent = GetParent (hwnd);
    hbw = ((WINDOWDATA*)GetWindowAdditionalData(parent))->hbw;
    if (nc == EN_ENTER)
    {

        length = SendMessage(hwnd, MSG_GETTEXTLENGTH, 0, 0);  
        url_string = (char*)malloc (length + 1);
        memset (url_string, 0, length+1);
        GetWindowText (hwnd, url_string, length);

        if (url_string)
        {
            mspider_navigate(hbw, NAV_GOTO, url_string);
            free (url_string);
        }
    }
}
HWND create_location_window(HWND hParent, HWND hToolBar)
{
    HWND ctrl;
    RECT rc_bw, rc_tb;
    int x = 2, y = 6, width;

    GetWindowRect (hParent, &rc_bw);

    if (hToolBar)
    {
        GetWindowRect (hToolBar, &rc_tb);
        x += RECTW(rc_tb);
    }

    width = RECTW(rc_bw) - x - 10;

    ctrl = CreateWindowEx (CTRL_SLEDIT, "", 
                           WS_CHILD | WS_BORDER | WS_VISIBLE,
                           0, IDC_LOCATION, x, y, width, LOCATION_HEIGHT,
                           hParent, 0);

    if(ctrl == HWND_INVALID)
        printf ("emSpider: Can not create location bar window.\n");

    SetNotificationCallback (ctrl, location_entry_open_url);

    return ctrl;
}

HWND create_toolbar_window(HWND hParent)
{
    NTBINFO ntb_info;
    NTBITEMINFO ntbii;
    HWND ntb;

    ntb_info.nr_cells = 7;
    ntb_info.w_cell  = 0;
    ntb_info.h_cell  = 0;
    ntb_info.nr_cols = 0;
    ntb_info.image = &ntb_bmp;

    ntb = CreateWindow (CTRL_NEWTOOLBAR,
                    "",
                    WS_CHILD | WS_VISIBLE, 
                    IDC_TOOLBAR,
                    0, 0, 210, 0,
                    hParent,
                    (DWORD) &ntb_info);

    if (ntb == HWND_INVALID)
        return HWND_INVALID;

    SetNotificationCallback (ntb, toolbar_notif_proc);

    memset (&ntbii, 0, sizeof (ntbii));
    ntbii.flags = NTBIF_PUSHBUTTON | NTBIF_DISABLED;
    ntbii.id = IDC_NAV_BACKWARD;
    ntbii.bmp_cell = 0;
    SendMessage(ntb, NTBM_ADDITEM, 0, (LPARAM)&ntbii);

    ntbii.flags = NTBIF_PUSHBUTTON | NTBIF_DISABLED;
    ntbii.id = IDC_NAV_FORWARD;
    ntbii.bmp_cell = 1;
    SendMessage (ntb, NTBM_ADDITEM, 0, (LPARAM)&ntbii);

    ntbii.flags = NTBIF_PUSHBUTTON;
    ntbii.id = IDC_NAV_HOME;
    ntbii.bmp_cell = 2;
    SendMessage (ntb, NTBM_ADDITEM, 0, (LPARAM)&ntbii);

    ntbii.flags = NTBIF_PUSHBUTTON;
    ntbii.id = IDC_NAV_STOP;
    ntbii.bmp_cell = 3;
    SendMessage (ntb, NTBM_ADDITEM, 0, (LPARAM)&ntbii);

    ntbii.flags = NTBIF_PUSHBUTTON;
    ntbii.id = IDC_NAV_RELOAD;
    ntbii.bmp_cell = 4;
    SendMessage (ntb, NTBM_ADDITEM, 0, (LPARAM)&ntbii);

    ntbii.flags = NTBIF_PUSHBUTTON;
    ntbii.id = IDC_NAV_GOTO;
    ntbii.bmp_cell = 5;
    SendMessage (ntb, NTBM_ADDITEM, 0, (LPARAM)&ntbii);

    ntbii.flags = NTBIF_PUSHBUTTON;
    ntbii.id = IDC_NAV_PROXY;
    ntbii.bmp_cell = 6;
    SendMessage (ntb, NTBM_ADDITEM, 0, (LPARAM)&ntbii);

    return ntb;
}

int MGmSpiderMainWinProc(HWND hWnd, int message, WPARAM wParam, LPARAM lParam)
{
    WINDOWDATA * pdata;

    switch(message)
    {
        case MSG_IDLE:
        {
            pdata = (WINDOWDATA*)GetWindowAdditionalData(hWnd);

            if(mspider_bw_canback(pdata->hbw))
                SendMessage (pdata->hwnd_toolbar, NTBM_ENABLEITEM, IDC_NAV_BACKWARD, TRUE);
            else
                SendMessage (pdata->hwnd_toolbar, NTBM_ENABLEITEM, IDC_NAV_BACKWARD, FALSE);

            if(mspider_bw_canforward(pdata->hbw))
                SendMessage (pdata->hwnd_toolbar, NTBM_ENABLEITEM, IDC_NAV_FORWARD, TRUE);
            else
                SendMessage (pdata->hwnd_toolbar, NTBM_ENABLEITEM, IDC_NAV_FORWARD, FALSE);
            if(mspider_bw_canstop(pdata->hbw))
                SendMessage (pdata->hwnd_toolbar, NTBM_ENABLEITEM, IDC_NAV_STOP, TRUE);
            else
                SendMessage (pdata->hwnd_toolbar, NTBM_ENABLEITEM, IDC_NAV_STOP, FALSE);

        }
        break;

        case MSG_CLOSE:
            pdata = (WINDOWDATA*)GetWindowAdditionalData(hWnd);
            mspider_browser_destroy (pdata->hbw);
            free(pdata);
            return 0;
    }


    return DefaultMainWinProc(hWnd, message, wParam, lParam);

}

static void InitCreateInfo (PMAINWINCREATE pCreateInfo, HWND hosting , int x, int y, int w, int h)
{
    pCreateInfo->dwStyle = WS_THINFRAME | WS_CAPTION;
    pCreateInfo->dwExStyle = WS_EX_NONE;
    pCreateInfo->spCaption = "FMSoft mSpider";
    pCreateInfo->hMenu = 0; 
    pCreateInfo->hMenu = 0;
    pCreateInfo->hCursor = GetSystemCursor (IDC_ARROW);
    pCreateInfo->hIcon = 0;
    pCreateInfo->MainWindowProc = MGmSpiderMainWinProc;
    pCreateInfo->lx = x; 
    pCreateInfo->ty = y;
    pCreateInfo->rx = x+w;
    pCreateInfo->by = y+h;
    pCreateInfo->iBkColor = COLOR_lightwhite; 
    pCreateInfo->dwAddData = 0;
    pCreateInfo->hHosting = hosting;
}

HWND my_own_new_bw(HWND hosting, int x, int y , int w, int h, DWORD flags)
{
    HWND hMainWnd;
    MAINWINCREATE CreateInfo;
    gal_pixel pixel;
    RECT crc;
    CB_INFO cb_info;
    WINDOWDATA * pdata;
    BOOL have_tool = FALSE;
    BOOL have_status = FALSE;

    pdata = (WINDOWDATA*)malloc (sizeof(WINDOWDATA));
    pdata->hbw = 0;
    pdata->hwnd_toolbar = 0;
    pdata->hwnd_location = 0;
    pdata->hwnd_progressbar = 0;
    pdata->hwnd_statusbar = 0;

    cb_info.CB_MESSAGE_BOX = my_messags_box;
    cb_info.CB_CONFIRM_BOX = my_confirm_box;
    cb_info.CB_PROMPT_BOX = my_prompt_box;
    cb_info.CB_SET_LOCATION = set_location_text;
    cb_info.CB_GET_LOCATION = get_location_text;
    cb_info.CB_SET_STATUS = set_status_text;
    cb_info.CB_GET_STATUS = get_status_text;
    cb_info.CB_SET_PROGRESS = set_process_text;
    cb_info.CB_GET_PROGRESS = get_process_text;

    /* create main window */
    InitCreateInfo (&CreateInfo, hosting, x, y, w, h);
    hMainWnd = CreateMainWindow (&CreateInfo);
    if (hMainWnd == HWND_INVALID) {
        free(pdata);
        return HWND_INVALID;
    }

    GetClientRect (hMainWnd, &crc);
    
    if (flags & NEW_BW_TOOLBAR) {
        pdata->hwnd_toolbar = create_toolbar_window (hMainWnd);
        have_tool = TRUE;
    }

    if (flags & NEW_BW_LOCATIONBAR) {
        pdata->hwnd_location = create_location_window (hMainWnd, pdata->hwnd_toolbar);
        have_tool = TRUE;
    }

    if (flags & NEW_BW_PROGRESSBAR) {
        pdata->hwnd_progressbar = create_progress_window (hMainWnd);
        have_status = TRUE;
    }

    if (flags & NEW_BW_STATUSBAR) {
        pdata->hwnd_statusbar = create_status_window (hMainWnd, pdata->hwnd_progressbar);
        have_status = TRUE;
    }

    if (have_tool) crc.top += 32;
    if (have_status) crc.bottom -= 18;

    cb_info.rect.x = crc.left;
    cb_info.rect.y = crc.top;
    cb_info.rect.width = crc.right-crc.left;
    cb_info.rect.height = crc.bottom-crc.top;

    pixel = RGB2Pixel(HDC_SCREEN, 212, 208, 200);
    SetWindowBkColor (hMainWnd, pixel);
    InvalidateRect (hMainWnd, NULL, TRUE);

    pdata->hbw = mspider_browser_new(hMainWnd, &cb_info);

    SetWindowAdditionalData (hMainWnd, (DWORD)pdata);

    ShowWindow (hMainWnd, SW_SHOWNORMAL);

    SetFocus(mspider_bw_get_hwnd(pdata->hbw));
    return hMainWnd;
}

#ifdef _NOUNIX_
int StartApp (int args, const char* argv[])
#else
int MiniGUIMain (int argc, const char *argv[])
#endif
{ 
    HWND main_wnd;
    int retval;
    MSPIDER_SETUP_INFO set_info;

#ifdef _NOUNIX_
  struct intfconfig_s c;
  struct ifnet *pIf;


  pIf = ifunit ("elPci0");
  if (pIf == NULL) {
	  MessageBox (HWND_DESKTOP, "Error", "No network interface: elPci0\n", MB_OK);
  }
  else if (IPMessageBox(&c)==1)
  { 
	  strcpy(dnsaddress,c.ns);
	  ifAddrDelete("elPci0", "192.168.1.111");
	  ifAddrAdd("elPci0", c.ip, NULL, 0x0000);

      if (resolvInit(RESOLVER_DOMAIN_SERVER, RESOLVER_DOMAIN, FALSE) != OK)
	        MessageBox (HWND_DESKTOP, "Error", "Failed to configure DNS Server!\n", MB_OK);

	  mRouteAdd(c.ip, c.gw, 0x00000000, 0, 0);
  }

#endif
#ifdef _MGRM_PROCESSES
     if (JoinLayer (NAME_TOPMOST_LAYER, "mSpider", 0, 0) ==
                 INV_LAYER_HANDLE) {
         printf ("JoinLayer: invalid layer handle.\n");
         return -1;
     }
#endif
    /* input method */
#ifdef _IME_GB2312
    GBIMEWindow (HWND_DESKTOP);
#endif


    if ((retval = LoadBitmapFromMem (HDC_SCREEN, &ntb_bmp, &new_toolbar_data, toolbar_datasize, "gif"))) {
        fprintf (stderr, "emSpider: Can not load bitmap for toolbar: %d.\n", retval);
        return -1;
    }

    set_info.charset = "UTF-8";
    set_info.font_factor = 1.0;

    mspider_setup(&set_info);

    main_wnd = mspider_init (HWND_DESKTOP, my_own_new_bw,
                             0, 0, g_rcScr.right, g_rcScr.bottom, argv[1]);
    /* message loop here */
    mspider_enter_event_loop (main_wnd);

    mspider_cleanup (main_wnd);

    UnloadBitmap (&ntb_bmp);

    return 0;
}

#ifndef _LITE_VERSION
#include <minigui/dti.c>
#endif
