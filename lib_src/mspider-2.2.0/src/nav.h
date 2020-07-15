#ifndef __NAV_H__
#define __NAV_H__

#include "browser.h"
#include "dw_widget.h"    /* for DwWidget */
#include "url.h"

/* useful macros for the navigation stack */
#define NAV_IDX(bw, i)   (bw)->nav_stack[i]
#define NAV_TOP_E(bw)      ((bw)->nav_stack)
#define NAV_TOP(bw)      (bw)->nav_stack[(bw)->nav_stack_ptr]


#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

void a_Nav_init(mSpiderDoc *dd);
void a_Nav_free(mSpiderDoc *dd);

void a_Nav_back(mSpiderDoc *dd);
void a_Nav_forw(mSpiderDoc *dd);
gint a_Nav_stack_ptr(mSpiderDoc *dd);
gint a_Nav_stack_size(mSpiderDoc *dd);
void a_Nav_new_url (mSpiderDoc *dd, mSpiderUrl* url);

void a_Nav_push(mSpiderDoc *dd, const mSpiderUrl *nurl);
void a_Nav_remove_top_url(mSpiderDoc *dd);
void a_Nav_vpush(void *vbw, const mSpiderUrl *url);
void a_Nav_home(mSpiderDoc *dd);
void a_Nav_reload(mSpiderDoc *dd);
void a_Nav_cancel_expect (mSpiderDoc *dd);
void a_Nav_expect_done(mSpiderDoc *dd);
void a_Nav_gotoSUrl(const char* url);

//void a_Nav_jump_callback(GtkWidget *widget, gpointer client_data, gint NewBw);


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __NAV_H__ */


