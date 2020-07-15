#ifndef __XMLWIDGET_WIDGET_H__
#define __XMLWIDGET_WIDGET_H__

#include "widget.h"
#include "screen.h"
#include "script.h"
#include "page.h"

struct _xmlwidget_opt;
struct _xmlwidget_widget;

struct user_event{
	char *name;
	struct user_event *next;
};

#define _XMLWIDGET_CLASS(clas_name) \
	_WIDGET_CLASS(clas_name) \
	mwidget *widget_tree; \
	mwidget *focus_node; \
	screen_widget_event_cbk_t *event_cbk; \
	page_t *page; \
	int		used_num; \
	struct user_event *user_event;


#define _xmlwidget_EVENT_LIST \
	_widget_EVENT_LIST

#define _xmlwidget_PROPERTYS \
	_widget_PROPERTYS

#define _xmlwidget_OPT(parent) \
	_WIDGET_OPT(parent) \
	mwidget *(*create_widget)(struct _xmlwidget_widget *m,char *name, int x, int y, int w, int h);\
	int (*user_event_register)(struct _xmlwidget_widget *m, char *event); \
	int (*user_event_check)(struct _xmlwidget_widget *m, char *event, unsigned int flag);


#define _xmlwidget_OPT_BEGIN(clas_name,parent) \
	_WIDGET_OPT_BEGIN(clas_name,parent) \
	.destroy = xmlwidget_destroy, \
	.create_widget = XmlwidgetCreateWidget, \
	.user_event_register =  XmlwidgetUserEventRegister,\
	.user_event_check =  XmlwidgetUserEventIndexGet, \

#define _xmlwidget_CREATE(clas_name,_name,x,y,w,h) \
	_WIDGET_CREATE(clas_name,_name,x,y,w,h) \
	m->child_paint_flag = 1; \
	char buf[64] = {0}; \
	sprintf(buf,"%s_%s",#clas_name,_name); \
	m->page = gui_page_create(buf); \
	m->widget_tree = create_card_widget(buf,x,y,w,h); \
	XmlwidgetScriptFuncInit(m->page->script,m);

typedef struct _xmlwidget_opt{
	_xmlwidget_OPT(widget)
}xmlwidget_opt;

typedef struct _xmlwidget_widget{
	_XMLWIDGET_CLASS(xmlwidget)
}mxmlwidget;

mxmlwidget *create_xmlwidget_widget(char *name, int x, int y, int w, int h);

#endif

