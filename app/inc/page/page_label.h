#ifndef __PAGE_LABEL_H__
#define __PAGE_LABEL_H__

#include "libxml/xmlmemory.h"
#include "libxml/parser.h"
#include "widget_manager.h"
#include "page.h"
#include "page_xml.h"

typedef struct page_label_parse{
	char	*name;
	int (*handle)(xmlNodePtr node,page_t *page,void *param);
}page_label_parse_t;

typedef struct page_label_prop{
	page_widget_prop_t	*prop;
	mwidget				*widget;
}page_label_prop_t;

void page_label_prop_destroy(page_label_prop_t *label_prop);

int label_parse(xmlNodePtr label, page_t *page, void *param);
#endif

