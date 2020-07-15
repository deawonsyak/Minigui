
#include "screen.h"
#include "typeFace.h"
#include "uni_gui_node.h"
#include "page.h"
#include "page_manager.h"
#include "widget_cli.h"
#include "page_cli.h"
#include "orb_cli.h"


extern int screenProc(HWND hwnd, int message, WPARAM wParam, LPARAM lParam);


Screen g_screen = {
	.x=0,
	.y=0,
	.w=1280,
	.h=720,
	.bgColor= 0xff00ff,
	.name="screen_test"
};

void uniGuiStart(void)
{
    MSG Msg;
    MAINWINCREATE CreateInfo;
	initScreen(&g_screen);
	
    CreateInfo.dwStyle = WS_NONE; 		//WS_CAPTION | WS_BORDER | WS_VISIBLE;
    //CreateInfo.dwStyle = WS_VISIBLE ; 		//WS_CAPTION | WS_BORDER | WS_VISIBLE;
    CreateInfo.dwExStyle = WS_EX_AUTOSECONDARYDC;
    CreateInfo.spCaption = "Orvibo";
    CreateInfo.hMenu = 0;
    //CreateInfo.hCursor =  GetSystemCursor (IDC_ARROW);
    CreateInfo.hIcon = 0;
    CreateInfo.MainWindowProc = screenProc;
    CreateInfo.lx = 0; 
    CreateInfo.ty = 0;
    CreateInfo.rx = g_rcScr.right;//GetGDCapability(HDC_SCREEN, GDCAP_HPIXEL);
    CreateInfo.by = g_rcScr.bottom;//GetGDCapability(HDC_SCREEN, GDCAP_VPIXEL);
    CreateInfo.iBkColor = PIXEL_transparent;
    CreateInfo.dwAddData = &g_screen;
    CreateInfo.dwReserved = 0;
    CreateInfo.hHosting = HWND_DESKTOP;

    HWND hMainWnd = CreateMainWindow (&CreateInfo);
    if (hMainWnd == HWND_INVALID)
        return ;
    ShowWindow (hMainWnd, SW_SHOWNORMAL);

	//widget_test();
	//orb_test();
	page_manager_init();
    while (GetMessage(&Msg, hMainWnd)) {
        TranslateMessage(&Msg);
        DispatchMessage(&Msg);
    }

    MainWindowThreadCleanup (hMainWnd);
}


int MiniGUIMain(int argc, const char* argv[])
{	
	orb_cli_init();
	widget_cli_init();
	page_cli_init();

	uniGuiStart();
	
    return 0;
}




