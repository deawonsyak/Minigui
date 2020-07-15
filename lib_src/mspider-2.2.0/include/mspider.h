
#ifndef _MSPIDER_H
    #define _MSPIDER_H


typedef enum {
    NEW_BW_STATUSBAR        = 1 << 0,
    NEW_BW_PROGRESSBAR      = 1 << 1, 
    NEW_BW_TOOLBAR          = 1 << 2,
    NEW_BW_LOCATIONBAR      = 1 << 3,
    NEW_BW_MENUBAR          = 1 << 4
} BW_STYLE; 

typedef struct  _DOC_RECT{
    int x;
    int y;
    int width;
    int height;
} DOC_RECT;

typedef struct _PROXY_INFO{
    char* address;   /* proxy address */
    char* username; /* user name */
    char* password;  /* proxy password */
    int   port;      /* proxy port */
} PROXY_INFO;

typedef struct _MSPIDER_SETUP_INFO{
    char* charset;  /* charset in mspider */
    float font_factor; /* factor of font in mspider */
} MSPIDER_SETUP_INFO;

typedef HWND (*ON_NEW_BW)(HWND hosting, int x, int y, int w, int h, DWORD flags);

typedef unsigned long  HBW; /*handle og browser window*/

#define INVALID_HBW     0

typedef struct _CB_INFO{
    DOC_RECT rect;
    void (*CB_MESSAGE_BOX)(HWND hMainWnd, const char* pszText, const char* pszCaption);
    BOOL (*CB_CONFIRM_BOX)(HWND hMainWnd, const char* pszText, const char* pszCaption);
    char *(*CB_PROMPT_BOX)(HWND hMainWnd, const char* pszText, const char* pszDefault, const char* pszCaption);
    void (*CB_SET_LOCATION) (HWND hMainWnd, const char * plocText);
    char *(*CB_GET_LOCATION) (HWND hMainWnd);
    void (*CB_SET_STATUS)(HWND hMainWnd, const char* status_msg);
    char *(*CB_GET_STATUS)(HWND hMainWnd);
    void (*CB_SET_PROGRESS)(HWND hMainWnd, const char* progress_msg);
    char *(*CB_GET_PROGRESS)(HWND hMainWnd);
} CB_INFO;

typedef enum {
    NAV_BACKWARD,
    NAV_FORWARD,
    NAV_HOME,
    NAV_GOTO,
    NAV_RELOAD,
    NAV_STOP
} OPERATION;


BOOL mspider_navigate(HBW hbw, int op, void* data);

BOOL mspider_setup(const MSPIDER_SETUP_INFO* set_info);
BOOL mspider_set_proxy(const PROXY_INFO * proxy_info);

HWND mspider_init(HWND hosting, ON_NEW_BW on_new_bw, 
                    int xpos, int ypos, int width, int height, const char *homeurl);
void mspider_enter_event_loop (HWND hMainWnd);
void mspider_cleanup(HWND hMainWnd);

HBW mspider_browser_new(HWND hMainWindow, const CB_INFO* cb_info);
void mspider_browser_destroy(HBW hbw);

HWND mspider_bw_get_hwnd (HBW hbw);

BOOL mspider_bw_canback(HBW hbw);
BOOL mspider_bw_canforward(HBW hbw);
BOOL mspider_bw_canstop(HBW hbw);

#endif
