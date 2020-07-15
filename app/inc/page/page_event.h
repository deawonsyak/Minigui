#ifndef __PAGE_EVENT_H__
#define __PAGE_EVENT_H__

#include "page.h"

page_event_t *page_event_create(char *event);

/********************************************************
 *	function	page_event_add
 *		把event添加到page中
 *
 * ******************************************************/
int page_event_add(page_t *page, char *event, mwidget *m, exp_node_t *exp);

/********************************************************
 *
 * ******************************************************/
void page_event_destroy(page_event_t *event);

page_event_t *page_event_find(page_t *page, char *event, mwidget *m);
#endif

