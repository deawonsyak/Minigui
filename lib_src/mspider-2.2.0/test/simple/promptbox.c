
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


#define IDC_INPUT	1000
static char *inputboxbuf;
static int inputboxbuflen;
static int InputBoxProc(HWND hDlg, int message, WPARAM wParam, LPARAM lParam)
{
    if ( message == MSG_COMMAND ) {
        if ( wParam == IDOK ) {
            GetWindowText (GetDlgItem (hDlg, IDC_INPUT), inputboxbuf, inputboxbuflen);
            EndDialog (hDlg, 1);
        }else if (wParam == IDCANCEL){
            EndDialog (hDlg, 0);
        } 
    }
    
    return DefaultDialogProc (hDlg, message, wParam, lParam);
}


int prompt_box(HWND parent, const char *info, const char *definp, char *buf, int buflen)
{
	static DLGTEMPLATE DlgBoxInputLen =
	{
    	WS_BORDER | WS_CAPTION, 
	    WS_EX_NONE,
    	0, 0, 300, 120, 
		"Javascript",
	    0, 0,
    	4, NULL,
	    0
	};

	static CTRLDATA CtrlInputLen [] =
	{ 
    	{
        	CTRL_STATIC,
	        WS_VISIBLE | SS_SIMPLE,
    	    20, 10, 260, 20, 
        	IDC_STATIC, 
	        "InputBox",
    	    0
	    },
    	{
	        CTRL_EDIT,
    	    WS_VISIBLE | WS_TABSTOP | WS_BORDER,
        	20, 40, 260, 20,
	        IDC_INPUT,
    	    NULL,
	        0
    	},
	    {
    	    CTRL_BUTTON,
        	WS_TABSTOP | WS_VISIBLE | BS_DEFPUSHBUTTON, 
	        50, 70, 60, 20,
    	    IDOK, 
        	"Ok",
	        0
    	},
	    {
    	    CTRL_BUTTON,
        	WS_TABSTOP | WS_VISIBLE | BS_DEFPUSHBUTTON, 
	        150, 70, 60, 20,
    	    IDCANCEL, 
        	"Cancel",
	        0
    	}
	};

	CtrlInputLen[0].caption = info;
	CtrlInputLen[1].caption = definp;
    DlgBoxInputLen.controls = CtrlInputLen;
	inputboxbuf = buf;
	inputboxbuflen = buflen;

    return DialogBoxIndirectParam (&DlgBoxInputLen, parent, InputBoxProc, 0);
}

