#ifndef __PREFS_H__
#define __PREFS_H__

#include <glib.h>
#include "url.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#define MSPIDER_START_PAGE "about:splash"
#define MSPIDER_HOME "http://www.minigui.org/"
#define DEF_CHARSET     "gb2312"

#define D_GEOMETRY_DEFAULT_WIDTH   640
#define D_GEOMETRY_DEFAULT_HEIGHT  550
#define D_GEOMETRY_DEFAULT_XPOS  -9999
#define D_GEOMETRY_DEFAULT_YPOS  -9999

#define DW_COLOR_DEFAULT_GREY   0xd6d6d6
#define DW_COLOR_DEFAULT_BLACK  0x000000
#define DW_COLOR_DEFAULT_BLUE   0x0000ff
#define DW_COLOR_DEFAULT_PURPLE 0x800080
#define DW_COLOR_DEFAULT_BGND   0xFFFFFF

#define COMPRESS_VOWELS "aeiouyAEIOUY"
#define COMPRESS_COMMON_PREFIXES "index of ;re: ;fwd: ;www.;welcome to ;the "
/* define enumeration values to be returned */
enum {
   PARSE_OK = 0,
   FILE_NOT_FOUND
};

/* define enumeration values to be returned for specific symbols */
typedef enum {
   DRC_TOKEN_FIRST = G_TOKEN_LAST,
   DRC_TOKEN_GEOMETRY,
   DRC_TOKEN_PROXY,
   DRC_TOKEN_PROXYUSER,
   DRC_TOKEN_NOPROXY,
   DRC_TOKEN_USER_AGENT,
   DRC_TOKEN_SEND_REFERER,
   DRC_TOKEN_LINK_COLOR,
   DRC_TOKEN_VISITED_COLOR,
   DRC_TOKEN_BG_COLOR,
   DRC_TOKEN_ALLOW_WHITE_BG,
   DRC_TOKEN_FORCE_MY_COLORS,
   DRC_TOKEN_CONTRAST_VISITED_COLOR,
   DRC_TOKEN_TEXT_COLOR,
   DRC_TOKEN_USE_OBLIQUE,
   DRC_TOKEN_START_PAGE,
   DRC_TOKEN_HOME,
   DRC_TOKEN_PANEL_SIZE,
   DRC_TOKEN_SMALL_ICONS,
   DRC_TOKEN_FONT_FACTOR,
   DRC_TOKEN_FONT_SIZES,
   DRC_TOKEN_SHOW_TOOLTIP,
   DRC_TOKEN_LIMIT_TEXT_WIDTH,
   DRC_TOKEN_LIMIT_FONT_DECORATION,
   DRC_TOKEN_W3C_PLUS_HEURISTICS,
   DRC_TOKEN_USE_DICACHE,
   DRC_TOKEN_SHOW_BACK,
   DRC_TOKEN_SHOW_FORW,
   DRC_TOKEN_SHOW_HOME,
   DRC_TOKEN_SHOW_RELOAD,
   DRC_TOKEN_SHOW_SAVE,
   DRC_TOKEN_SHOW_STOP,
   DRC_TOKEN_SHOW_BOOKMARKS,
   DRC_TOKEN_SHOW_MENUBAR,
   DRC_TOKEN_SHOW_CLEAR_URL,
   DRC_TOKEN_SHOW_URL,
   DRC_TOKEN_SHOW_SEARCH,
   DRC_TOKEN_SHOW_PROGRESS_BOX,
   DRC_TOKEN_SHOW_POPUP_NAVIGATION,
   DRC_TOKEN_FULLWINDOW_START,
   DRC_TOKEN_TRANSIENT_DIALOGS,
   DRC_TOKEN_FW_FONT,
   DRC_TOKEN_VW_FONT,
   DRC_TOKEN_GENERATE_SUBMIT,
   DRC_TOKEN_ENTERPRESS_FORCES_SUBMIT,
   DRC_TOKEN_SEARCH_URL,
   DRC_TOKEN_SHOW_MSG,
   DRC_TOKEN_SHOW_EXTRA_WARNINGS,
   DRC_TOKEN_QUERY_EXPIRE_TIME,
   DRC_TOKEN_MIN_PAGE_EXPIRE_TIME,
   DRC_TOKEN_MIN_IMAGE_EXPIRE_TIME,
   DRC_TOKEN_LAST
} mSpider_Rc_TokenType;

typedef struct _mSpiderPrefs mSpiderPrefs;

struct _mSpiderPrefs {
   int width;
   int height;
   int xpos;
   int ypos;
   mSpiderUrl *http_proxy;
   char *http_proxyuser;
   char *no_proxy;
   char **no_proxy_vec;
   gchar *user_agent;
   gboolean send_referer;
   mSpiderUrl *start_page;
   mSpiderUrl *home;
   guint32 link_color;
   guint32 visited_color;
   guint32 bg_color;
   guint32 text_color;
   gboolean allow_white_bg;
   gboolean use_oblique;
   gboolean force_my_colors;
   gboolean contrast_visited_color;
   gboolean show_tooltip;
   int panel_size;
   gboolean small_icons;
   gboolean limit_text_width;
   gboolean w3c_plus_heuristics;
   double font_factor;
   gboolean use_dicache;
   gboolean show_back;
   gboolean show_forw;
   gboolean show_home;
   gboolean show_reload;
   gboolean show_save;
   gboolean show_stop;
   gboolean show_bookmarks;
   gboolean show_menubar;
   gboolean show_clear_url;
   gboolean show_url;
   gboolean show_search;
   gboolean show_progress_box;
   gboolean fullwindow_start;
   gboolean transient_dialogs;
   char *vw_fontname;
   char *fw_fontname;
   gboolean generate_submit;
   gboolean enterpress_forces_submit;
   char *search_url;
   gboolean show_msg;
   gboolean show_extra_warnings;
   int query_expire_time;
   int min_page_expire_time;
   int min_image_expire_time;
};

#define MSPIDERRC_SYS "~/.mspiderrc"

/* Global Data */
extern mSpiderPrefs prefs;

void a_Prefs_init(void);
void a_Prefs_freeall(void);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __PREFS_H__ */
