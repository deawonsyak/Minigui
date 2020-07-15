#ifndef __IXMLWIDGET_WIDGET_H__
#define __IXMLWIDGET_WIDGET_H__


#include "widget.h"
#include "screen.h"
#include "script.h"
#include "page.h"
#include "xmlwidget_widget.h"


#define _IXMLWIDGET_CLASS(clas_name) \
	_WIDGET_CLASS(clas_name) \
	HDC hdc; \
	mwidget *widget_tree; \
	mwidget *focus_node; \
	mxmlwidget	*xmlwidget; \
	script_t	*script; \
	orb_list_t *event_list; \
	screen_widget_event_cbk_t *event_cbk; \
	unsigned int user_event_flag;
	


#define _ixmlwidget_EVENT_LIST \
	_widget_EVENT_LIST

#define _ixmlwidget_PROPERTYS \
	_widget_PROPERTYS

#define _ixmlwidget_OPT(parent) \
	_WIDGET_OPT(parent) \
	struct _ixmlwidget *(*create_widget)(char *name, int x, int y, int w, int h);

#define _ixmlwidget_OPT_BEGIN(clas_name,parent) \
	_WIDGET_OPT_BEGIN(clas_name,parent) \
	.paint = ixmlwidget_paint, \
	.destroy = ixmlwidget_destroy, \
	.start = IxmlwidgetStart, \
	.stop = IxmlwidgetStop, \
	.get_focus = IxmlwidgetGetFocus, \
	.touch_event_handle = IxmlwidgetTouchEventHandle, \
	.activate_event = IxmlwidgetActiveEvent,

#define _ixmlwidget_CREATE(clas_name,_name,x,y,w,h) \
	_WIDGET_CREATE(clas_name,_name,x,y,w,h) \
	m->child_paint_flag = 1; \
	m->event_list = orb_list_create();\

typedef struct _ixmlwidget_opt{
	_ixmlwidget_OPT(widget)
}ixmlwidget_opt;

typedef struct _ixmlwidget_widget{
	_IXMLWIDGET_CLASS(ixmlwidget)
}mixmlwidget;

mixmlwidget *create_ixmlwidget_widget(char *name, int x, int y, int w, int h);


#endif

