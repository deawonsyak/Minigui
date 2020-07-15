#ifndef __DOC_H__
#define __DOC_H__

#include "browser.h"   /* for mSpiderDoc */

/* used to set default margin for documents */
#define DOC_DEFAULT_MARGIN 5
#define DOC_DEFAULT_FONT_SIZE 12.0

/* macros for documents */
#define DD_HAS_FOCUS(dd,bw) bw->dd == dd
void a_Doc_init(void);
void a_Doc_stop(mSpiderDoc *dd);
void a_Doc_clean(mSpiderDoc *dd);
void a_Doc_title_set(mSpiderDoc *dd, gchar *title);
void a_Doc_location_set(mSpiderDoc *dd, gchar *location);
void a_Doc_remove_client(mSpiderDoc *dd, gint ClientKey);
void a_Doc_close_client(mSpiderDoc *dd, gint ClientKey);
void a_Doc_add_client(mSpiderDoc *dd, gint Key, gint Root);
void a_Doc_add_url(mSpiderDoc *dd, const mSpiderUrl *Url, gint Flags);
void a_Doc_destroy(mSpiderDoc *dd);
void a_Doc_set_name(mSpiderDoc *dd, gchar *name);
mSpiderDoc * a_Doc_get_by_name(mSpiderDoc *dd, gchar *name);
void a_Doc_set_parent(mSpiderDoc *dd, mSpiderDoc *parent);
void a_Doc_set_browserwindow(mSpiderDoc *dd, BrowserWindow *bw);
void a_Doc_progress_update(mSpiderDoc *dd);
GList * a_Doc_get_visible_children(mSpiderDoc *dd);
mSpiderDoc * a_Doc_get_by_docwin(HWND  docwin);
mSpiderDoc * a_Doc_get_parent(mSpiderDoc *dd);
mSpiderDoc * a_Doc_get_root(mSpiderDoc *dd);
mSpiderDoc * a_Doc_new(void);
gboolean a_Doc_CreateEx(mSpiderDoc * dd, 
                HWND parent_hwnd, guint flags, 
                gint x, gint y, gint cx, gint cy, 
                gint ctrlId, gchar * startpage);
char* correcturl(mSpiderDoc *dd, const char *url);
void a_Doc_freeall(void);
#endif /* __DOC_H__ */
