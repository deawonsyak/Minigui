#ifndef __PAGE_MODE_H__
#define __PAGE_MODE_H__

#include "page.h"
#include "page_xml.h"



typedef enum _mode_property_type{
	PAGE_MODE_PROPERTY_VALUE_TYPE_STRING,
	PAGE_MODE_PROPERTY_VALUE_TYPE_EXP,
}mode_property_type_e;

typedef struct mode_property{
	mode_property_type_e value_type;
	char *name;
	union{
		char		*value;
		page_widget_prop_t *exp_prop;
	};
}page_mode_prop_t;

typedef struct page_mode_node{
	page_mode_prop_t *widget;

	orb_list_t *propertys;

	struct page_mode_node *child;
	struct page_mode_node *next;
}page_mode_node_t;

/********************************************************
 *	Data page_mode_t
 *		页面显示模板， 用于构建统一样式的显示模块
 *
 * ******************************************************/
typedef struct _page_mode{
	char *mode_name;
	page_mode_node_t nodes;		// 节点头
}page_mode_t;

page_mode_t *page_mode_create(char *name);

void page_mode_destory(page_mode_t *mode);

page_mode_node_t *page_mode_node_create(void);

void page_mode_show(page_mode_t *mode,int deep);

#endif

