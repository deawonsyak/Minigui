/* 
 * Copyright (C) 2005-2006 Feynman Software
 *
 * This program is g_free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */
#ifndef __INTERFACE_H__
#define __INTERFACE_H__

#include "browser.h"

#include <minigui/common.h>
#include <minigui/minigui.h>
#include <minigui/gdi.h>
#include <minigui/window.h>
#include <minigui/control.h>
typedef enum {
    WITH_NONE        = 1 << 0,
    WITH_LOCATION    = 1 << 1,
    WITH_MENUBAR     = 1 << 2,
    WITH_TOOLBAR     = 1 << 3,
    WITH_STATUSBAR   = 1 << 4,
    WITH_PROGRESSBAR = 1 << 5,
} BrowserWindowType;

void a_Interface_init(void);
void a_Interface_stop(mSpiderDoc *dd);
void a_Interface_clean(mSpiderDoc *dd);
void a_Interface_quit_all(void);

void a_Interface_add_client(mSpiderDoc *dd, gint Key, gint Root);
void a_Interface_remove_client(mSpiderDoc *dd, gint ClientKey);
void a_Interface_add_url(mSpiderDoc *dd, const mSpiderUrl *Url, gint Flags);
void a_Interface_close_client(mSpiderDoc *dd, gint ClientKey);

void a_Interface_msg(mSpiderDoc *dd, const char *format, ... );
void a_Interface_bug_meter_update(mSpiderDoc *dd, gint num_err);
void a_Interface_open_url_string(gchar *text, mSpiderDoc *dd);


void a_Interface_set_page_title(mSpiderDoc *dd, char *title);
void a_Interface_set_location_text(mSpiderDoc *dd, char *text);
gchar *a_Interface_get_location_text(mSpiderDoc *dd);
void a_Interface_reset_progress_bars(mSpiderDoc *dd);
void a_Interface_set_cursor (mSpiderDoc *dd, HCURSOR CursorType);

gboolean a_Interface_quit(BrowserWindow *bw);


#if 0
void a_Interface_set_button_sens(mSpiderDoc *dd);
#endif

extern void a_Dns_init (void);
extern void a_Dns_freeall(void);

extern void a_Prefs_init(void);
extern void a_Prefs_freeall(void);

extern gint a_Http_init(void);
extern void a_Http_freeall(void);

extern void a_Mime_init(void);

extern void a_Cache_init(void);
extern void a_Cache_freeall(void);

extern void a_Dicache_init (void);
extern void a_Dicache_freeall(void);

extern void a_Dw_style_init    (void);
extern void a_Dw_style_freeall (void);

extern void a_History_free (void);
#if 0
void a_Interface_scroll_popup(GtkWidget *widget);
void a_Interface_question_dialog(
        mSpiderDoc *dd, gchar *QuestionTxt,
        GtkSignalFunc OkCallback, void *OkCbData,
        GtkSignalFunc CancelCallback, void *CancelCbData);
void a_Interface_message_window(const char *title, const char *format, ... );
void a_Interface_text_window (GtkWidget **text_widget,
                              gchar *title, gchar *wm_class,
                              gchar *buf, gint buf_size,
                              gint xsize_max, gint ysize_max);

void a_Interface_set_nice_window_pos(GtkWidget *win1, GtkWidget *win2);
#endif


#endif /* __INTERFACE_H__ */
