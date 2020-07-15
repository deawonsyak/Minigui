

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

#include "mspider.h"
#include "emspider-res.h"

#define IDC_PROXY_ADDRESS  601
#define IDC_PROXY_PORT     602
#define IDC_PROXY_NAME     603
#define IDC_PROXY_PASSWORD 604

extern long interface_datasize;
extern const unsigned char interface_data[]; 
static BITMAP bkgnd;

extern long ok_datasize;
extern long cancel_datasize;
extern const unsigned char ok_data[];
extern const unsigned char cancel_data[];
static BITMAP bok;
static BITMAP bcancel;

static DLGTEMPLATE PopWinData =
{
     WS_CAPTION | WS_THINFRAME,
     WS_EX_NONE, 0, 0 , 318, 217, "proxy", 0, 0, 6, NULL, 0
};

static CTRLDATA CtrlInitProgress [] =
{
    {
        CTRL_SLEDIT,
        WS_VISIBLE | WS_TABSTOP ,
        97, 22, 191, 23,
        IDC_PROXY_ADDRESS,
        NULL,
        0
    },
    {
        CTRL_SLEDIT,
        WS_VISIBLE | WS_TABSTOP ,
        97, 51, 191, 23,
        IDC_PROXY_PORT,
        NULL,
        0
    },
    {
        CTRL_SLEDIT,
        WS_VISIBLE | WS_TABSTOP ,
        97, 96, 191, 23,
        IDC_PROXY_NAME,
        NULL,
        0
    },
    {
        CTRL_SLEDIT,
        WS_VISIBLE | WS_TABSTOP | ES_PASSWORD ,
        97, 130, 191, 23,
        IDC_PROXY_PASSWORD,
        NULL,
        0
    },
    {
        CTRL_BUTTON,
        WS_TABSTOP | WS_VISIBLE | BS_DEFPUSHBUTTON | BS_BITMAP, 
        51, 175, 92, 25,
        IDOK, 
#ifdef _LANG_ZHCN
        "确定",
#else
        "Ok",
#endif
        (DWORD)&bok
    },
    {
        CTRL_BUTTON,
        WS_TABSTOP | WS_VISIBLE | BS_DEFPUSHBUTTON | BS_BITMAP, 
        177, 175, 92, 25,
        IDCANCEL, 
#ifdef _LANG_ZHCN
        "取消",
#else
        "Cancel",
#endif
        (DWORD)&bcancel
    }
};
static int ProxyBoxProc (HWND hDlg, int message, WPARAM wParam, LPARAM lParam)
{
    
    char buf[16];
    RECT rc;
    PROXY_INFO *proxy_info = NULL;
    int retval = 0;
    int len = 0;


    switch (message) {
    case MSG_INITDIALOG:
        {
            if ((retval = LoadBitmapFromMem (HDC_SCREEN, &bkgnd, &interface_data, interface_datasize, "png"))) 
            {
                fprintf (stderr, "emSpider: Can not load bitmap for dialog: %d.\n", retval);
                return -1;
            }
            if ((retval = LoadBitmapFromMem (HDC_SCREEN, &bok, &ok_data, ok_datasize, "png"))) 
            {
                fprintf (stderr, "emSpider: Can not load bitmap for dialog: %d.\n", retval);
                return -1;
            }
            if ((retval = LoadBitmapFromMem(HDC_SCREEN, &bcancel, &cancel_data, cancel_datasize, "png"))) 
            {
                fprintf (stderr, "emSpider: Can not load bitmap for dialog: %d.\n", retval);
                return -1;
            }

            GetWindowRect ((HWND)(lParam), &rc);
            MoveWindow (hDlg , (rc.right-rc.left)/2-160 , (rc.bottom-rc.top)/2-120 , 320 , 240 , TRUE);

#if 0
            if(HTTP_Proxy) 
            {
                SetWindowText(GetDlgItem (hDlg, IDC_PROXY), HTTP_Proxy->url_string->str);
                SetWindowText(GetDlgItem (hDlg, IDC_PORT), buf);
            }
#endif
        }
        return 1;
        
    case MSG_ERASEBKGND:
    {
        HDC hdc = (HDC)wParam;
        const RECT* clip = (const RECT*) lParam;
        BOOL fGetDC = FALSE;
        RECT rcTemp;
                
        if (hdc == 0) {
            hdc = GetClientDC (hDlg);
            fGetDC = TRUE;
        }       
                
        if (clip) {
            rcTemp = *clip;
            ScreenToClient (hDlg, &rcTemp.left, &rcTemp.top);
            ScreenToClient (hDlg, &rcTemp.right, &rcTemp.bottom);
            IncludeClipRect (hdc, &rcTemp);
        }
        FillBoxWithBitmap (hdc, 0, 0, 0, 0, &bkgnd);

        if (fGetDC)
            ReleaseDC (hdc);
        return 0;
    }

    case MSG_COMMAND:
        switch (wParam) {
        case IDOK: 
        {
            proxy_info = (PROXY_INFO*)malloc(sizeof(PROXY_INFO));
            memset(proxy_info, 0, sizeof(PROXY_INFO));
            proxy_info->address = NULL;
            proxy_info->username = NULL;
            proxy_info->password = NULL;

            len = GetWindowTextLength (GetDlgItem (hDlg, IDC_PROXY_ADDRESS));
            if (len > 0) 
            {
                proxy_info->address = (char*) malloc( len + 1);
                memset(proxy_info->address, 0, len+1);
                GetWindowText (GetDlgItem (hDlg, IDC_PROXY_ADDRESS), proxy_info->address, len);
                proxy_info->address[len + 1] = '\0';
                len = 0;
            }

            len = GetWindowTextLength (GetDlgItem (hDlg, IDC_PROXY_NAME));
            if (len > 0) 
            {
                proxy_info->username = (char*) malloc( len + 1);
                memset(proxy_info->username, 0, len+1);
                GetWindowText (GetDlgItem (hDlg, IDC_PROXY_NAME), proxy_info->username, len);
                proxy_info->username[len + 1] = '\0';
                len = 0;
            }

            len = GetWindowTextLength (GetDlgItem (hDlg, IDC_PROXY_PASSWORD));
            if (len > 0) 
            {
                proxy_info->password = (char*) malloc( len + 1);
                memset(proxy_info->password, 0, len+1);
                GetWindowText (GetDlgItem (hDlg, IDC_PROXY_PASSWORD), proxy_info->password, len);
                proxy_info->password[len + 1] = '\0';
                len = 0;
            }

            len = GetWindowTextLength (GetDlgItem (hDlg, IDC_PROXY_PORT));
            if (len > 0) 
            {
                memset(buf, 0, sizeof(buf));
                GetWindowText (GetDlgItem (hDlg, IDC_PROXY_PORT), buf, len);
                proxy_info->port = atoi(buf);
            }

            if(mspider_set_proxy(proxy_info) != 0)
                fprintf(stderr, "set proxy failure\n");

            if(proxy_info->address != NULL)
                free(proxy_info->address);
            if(proxy_info->username != NULL)
                free(proxy_info->username);
            if(proxy_info->password != NULL)
                free(proxy_info->password);
            free(proxy_info);

        }    
        case IDCANCEL:
            EndDialog (hDlg, wParam);
            break;
        }
        break;

    case MSG_CLOSE:
        EndDialog (hDlg, IDCANCEL);
        break;
    }
    
    return DefaultDialogProc (hDlg, message, wParam, lParam);
}

void create_proxy_window(HWND hParent)
{
      PopWinData.controls = CtrlInitProgress;
      DialogBoxIndirectParam (&PopWinData, hParent, ProxyBoxProc, (DWORD)hParent);
}

