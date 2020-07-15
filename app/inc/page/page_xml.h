#ifndef __PAGE_XML_H__
#define __PAGE_XML_H__
#include "libxml/xmlmemory.h"
#include "libxml/parser.h"
#include "page.h"
#include "script.h"

typedef enum property_type{
	PAGE_XML_PROPERTY_TYPE_STRING,
	PAGE_XML_PROPERTY_TYPE_VAR,
}property_type_e;

typedef struct page_xml_property{
	property_type_e type;
	char *name;
	char *value;
	struct page_xml_property *next;
}page_xml_prop_t;

typedef struct page_widget_property{
	char		*name;
	exp_node_t	*exp;
	struct page_widget_property *next;
}page_widget_prop_t;

struct page_xml_label{
	char	*name;
	int (*handle)(xmlNodePtr node,page_t *page,void *param);
};

/********************************************************
 *	function	xml_page_load
 *		从xml文件加载页面
 *	@file 
 *		文件名
 *
 *	return
 *		成功返回 页面指针
 *		失败返回 NULL
 *
 * ******************************************************/
page_t *xml_page_load(char *file);

/********************************************************
 *	function	xml_keyboard_load
 *		从xml文件加载键盘
 *	@file
 *		xml文件名
 *
 *	return 
 *		成功返回键盘控件指针
 *		失败返回NULL
 *
 * ******************************************************/
mwidget *xml_keyboard_load(char *file);

page_xml_prop_t *xml_node_property_parse(xmlNodePtr node);
void page_xml_prop_list_destroy(page_xml_prop_t *pro);
page_widget_prop_t *page_xml_prop_text_parse(xmlNodePtr node,page_t *page);

void page_widget_prop_destroy(page_widget_prop_t *prop);
char *xml_node_property_get(page_xml_prop_t *prop, char *name);
#endif
