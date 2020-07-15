#ifndef __HTML_H__
#define __HTML_H__

#include "url.h"
#include "dw_widget.h"
#include "dw_image.h"
#include "dw_style.h"
#include "browser.h"
#include "image.h"

#include <minigui/common.h>
#include <minigui/minigui.h>
#include <minigui/gdi.h>
#include <minigui/window.h>
#include <minigui/control.h>
#include "js.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* First, the html linkblock. For now, this mostly has forms, although
   pointers to actual links will go here soon, if for no other reason
   than to implement history-sensitive link colors. Also, it seems
   likely that imagemaps will go here. */

typedef struct _mSpiderHtmlLB      mSpiderHtmlLB;

typedef struct _mSpiderHtml        mSpiderHtml;
typedef struct _mSpiderHtmlClass   mSpiderHtmlClass;
typedef struct _mSpiderHtmlState   mSpiderHtmlState;
typedef struct _mSpiderHtmlForm    mSpiderHtmlForm;
typedef struct _mSpiderHtmlOption  mSpiderHtmlOption;
typedef struct _mSpiderHtmlSelect  mSpiderHtmlSelect;
typedef struct _mSpiderHtmlInput   mSpiderHtmlInput;

struct _mSpiderHtmlLB {
   mSpiderDoc *dd;
   mSpiderUrl *base_url;
 
   mSpiderHtmlForm *forms;
   int num_forms;
   int num_forms_max;
 
   mSpiderUrl **links;
   int num_links;
   int num_links_max;
 
   DwImageMapList maps;
 
   guint32 link_color;
   guint32 visited_color;

   int num_page_bugs;
   GString* page_bugs;
#ifdef JS_SUPPORT
   GList *images;
#endif

};


typedef enum {
   MSPIDER_HTML_PARSE_MODE_INIT,
   MSPIDER_HTML_PARSE_MODE_STASH,
   MSPIDER_HTML_PARSE_MODE_STASH_AND_BODY,
   MSPIDER_HTML_PARSE_MODE_VERBATIM,
   MSPIDER_HTML_PARSE_MODE_BODY,
   MSPIDER_HTML_PARSE_MODE_PRE
} mSpiderHtmlParseMode;

typedef enum {
   SEEK_ATTR_START,
   MATCH_ATTR_NAME,
   SEEK_TOKEN_START,
   SEEK_VALUE_START,
   SKIP_VALUE,
   GET_VALUE,
   FINISHED
} mSpiderHtmlTagParsingState;

typedef enum {
   HTML_LeftTrim      = 1 << 0,
   HTML_RightTrim     = 1 << 1,
   HTML_ParseEntities = 1 << 2
} mSpiderHtmlTagParsingFlags;

typedef enum {
   MSPIDER_HTML_TABLE_MODE_NONE,  /* no table at all */
   MSPIDER_HTML_TABLE_MODE_TOP,   /* outside of <tr> */
   MSPIDER_HTML_TABLE_MODE_TR,    /* inside of <tr>, outside of <td> */
   MSPIDER_HTML_TABLE_MODE_TD     /* inside of <td> */
} mSpiderHtmlTableMode;


typedef enum {
   IN_HTML        = 1 << 0,
   IN_HEAD        = 1 << 1,
   IN_BODY        = 1 << 2,
   IN_FORM        = 1 << 3,
   IN_SELECT      = 1 << 4,
   IN_TEXTAREA    = 1 << 5,
   IN_MAP         = 1 << 6,
   IN_PRE		  = 1 << 7,
   IN_BUTTON	  = 1 << 8,
   IN_FRAMESET    = 1 << 9
} mSpiderHtmlProcessingState;

typedef enum {
    IFrameScroll_AUTOMATIC,
    IFrameScroll_YES,
    IFrameScroll_NO,
} mSpiderIFrameScrollType;

typedef enum {
   MSPIDER_HTML_FRAME_MODE_NONE,     /* no frameset at all */
   MSPIDER_HTML_FRAME_MODE_IFRAME,   /* inside of <iframe> */
   MSPIDER_HTML_FRAME_MODE_FRAMESET, /* inside of <frameset> */
   MSPIDER_HTML_FRAME_MODE_NOFRAMES  /* inside of <noframes> */
} mSpiderHtmlFrameMode;

struct _mSpiderHtmlState {
   char *tag_name;
   DwStyle *style;
   DwStyle *table_cell_style;
   mSpiderHtmlParseMode parse_mode;
   mSpiderHtmlTableMode table_mode;
   mSpiderHtmlFrameMode frame_mode;
   DwWidget *frameset;
   gboolean cell_text_align_set;
   enum { HTML_LIST_NONE, HTML_LIST_UNORDERED, HTML_LIST_ORDERED } list_type;
   int list_number;
 
   /* TagInfo index for the tag that's being processed */
   int tag_idx;
 
   DwWidget *page, *table;

   /* This is used to align list items (especially in enumerated lists) */
   DwWidget *ref_list_item;

   /* This makes image processing faster than a function
      a_Dw_widget_get_background_color. */
   guint32 current_bg_color;

   /* This is used for list items etc; if it is set to TRUE, breaks
      have to be "handed over" (see Html_add_indented and
      Html_eventually_pop_dw). */
   gboolean hand_over_break;
};

typedef enum {
   MSPIDER_HTML_METHOD_UNKNOWN,
   MSPIDER_HTML_METHOD_GET,
   MSPIDER_HTML_METHOD_POST
} mSpiderHtmlMethod;

typedef enum {
   MSPIDER_HTML_ENC_URLENCODING
} mSpiderHtmlEnc;

struct _mSpiderHtmlForm {
   mSpiderHtmlMethod method;
   mSpiderUrl *action;
   mSpiderHtmlEnc enc;
 
   mSpiderHtmlInput *inputs;
   int num_inputs;
   int num_inputs_max;
   int num_entry_fields;
   int num_submit_buttons;
   HWND hwnd_submit;
#ifdef JS_SUPPORT
	jsobject *jsobj;
#endif
};

struct _mSpiderHtmlOption {
   char*menuitem;
   char *value;
   void *defaultopt;
   gboolean init_val;
};

struct _mSpiderHtmlSelect {
   void *menu;
   int size;
 
   mSpiderHtmlOption *options;
   int num_options;
   int num_options_max;
};

typedef enum {
   MSPIDER_HTML_INPUT_TEXT,
   MSPIDER_HTML_INPUT_PASSWORD,
   MSPIDER_HTML_INPUT_CHECKBOX,
   MSPIDER_HTML_INPUT_RADIO,
   MSPIDER_HTML_INPUT_IMAGE,
   MSPIDER_HTML_INPUT_FILE,
   MSPIDER_HTML_INPUT_BUTTON,
   MSPIDER_HTML_INPUT_HIDDEN,
   MSPIDER_HTML_INPUT_SUBMIT,
   MSPIDER_HTML_INPUT_RESET,
   MSPIDER_HTML_INPUT_BUTTON_SUBMIT,
   MSPIDER_HTML_INPUT_BUTTON_RESET,
   MSPIDER_HTML_INPUT_SELECT,
   MSPIDER_HTML_INPUT_SEL_LIST,
   MSPIDER_HTML_INPUT_TEXTAREA,
   MSPIDER_HTML_INPUT_INDEX
} mSpiderHtmlInputType;

struct _mSpiderHtmlInput {
   mSpiderHtmlInputType type;
   void *widget;      /* May be a DwMgWidget or a DwWidget. */
   char *name;
   char *init_str;    /* note: some overloading - for buttons, init_str
                         is simply the value of the button; for text
                         entries, it is the initial value */
   mSpiderHtmlSelect *select;
   gboolean init_val; /* only meaningful for buttons */
};

struct _mSpiderHtml {
   DwWidget *dw;          /* this is duplicated in the stack (page) */
 
   mSpiderHtmlLB *linkblock;
   char *Start_Buf;
   size_t Start_Ofs;
   size_t Buf_Size;
   size_t CurrTagOfs;
   size_t OldTagOfs, OldTagLine;
 
   mSpiderHtmlState *stack;
   int stack_top;        /* Index to the top of the stack [0 based] */
   int stack_max;
 
   mSpiderHtmlProcessingState InFlags; /* tracks which tags we are in */
 
   GString *Stash;
   gboolean StashSpace;

   gchar *SPCBuf;           /* Buffer for white space */
 
   int pre_column;          /* current column, used in PRE tags with tabs */
   gboolean PreFirstChar;   /* used to skip the first CR or CRLF in PRE tags*/
   gboolean PrevWasCR;      /* Flag to help parsing of "\r\n" in PRE tags */
   gboolean PrevWasOpenTag; /* Flag to help deferred parsing of white space */
   gboolean SPCPending;     /* Flag to help deferred parsing of white space */
   gboolean InVisitedLink;  /* used to 'contrast_visited_colors' */
   gboolean ReqTagClose;    /* Flag to help handling bad-formed HTML */
   gboolean CloseOneTag;    /* Flag to help Html_tag_cleanup_at_close() */
   gboolean TagSoup;        /* Flag to enable the parser's cleanup functions */
   gchar *NameVal;          /* used for validation of "NAME" and "ID" in <A> */
   gint PreWidth;           /* used for width attribute*/
   gint PreCount;           /* used for width attribute*/
 
   /* element counters: used for validation purposes */
   guchar Num_HTML, Num_HEAD, Num_BODY, Num_TITLE;

   GString *attr_data;
 
   PLOGFONT logfont; /* the logical font used to parse the word */

   mSpiderDoc *dd;
};

typedef enum {
        FONT_MOVE_LEFT,
        FONT_MOVE_RIGHT, 
        FONT_MOVE_UP,
        FONT_MOVE_DOWN
} FontScrollDirection;

typedef enum {
    FONT_BEHAVIOR_SCROLL,    /* loop scroll */
    FONT_BEHAVIOR_SLIDE,     /* only move one time */
    FONT_BEHAVIOR_ALTERNATE  /* move forward and backward */
} FontMoveBehavior;

typedef struct _DwStyleFontMove{
    FontScrollDirection direction;
    FontMoveBehavior behavior;
    int loop;         /* infinite:infinite loop £¬or specify the number of loop */
    long int move_speed; 
    int scroll_delay; /* delay time */
    int bgcolor;
    int width;        /* the width of movement district */
    int height;       /* the height of movement district */
} DwStyleFontMove;


void Html_submit_form(mSpiderHtmlLB* html_lb, mSpiderHtmlForm* form,
                      DwWidget* submit, mSpiderDoc *doc);
void Html_reset_form(HWND hwnd, int id, int nc, DWORD add_data);

void Html_reset_input(mSpiderHtmlInput *input);

void Html_reload_image(mSpiderDoc *HtmlImagedoc, mSpiderUrl *url,mSpiderImage *Image);

DwStyleLength Html_parse_length (mSpiderHtml *html, const char *attr);

gboolean Html_link_clicked (DwWidget *widget, int link, int x, int y,
                                  DWORD flags, mSpiderHtmlLB *lb);

#ifdef __cplusplus
}
#endif /* __cplusplus */


#endif /* __HTML_H__ */
