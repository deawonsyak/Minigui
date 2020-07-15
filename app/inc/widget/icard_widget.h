#ifndef __ICARD_WIDGET_H__
#define __ICARD_WIDGET_H__

#include "widget.h"

#define _ICARD_CLASS(clas_name) \
	_WIDGET_CLASS(clas_name) \
	HDC hdc; \
	mwidget *widget_tree; \
	mwidget *focus_node;
	

#define _icard_EVENT_LIST \
	_widget_EVENT_LIST

#define _icard_PROPERTYS \
	_widget_PROPERTYS

#define _icard_OPT(parent) \
	_WIDGET_OPT(parent) \
	int (*set_widget_tree)(struct _icard_widget *m, mwidget *tree);

#define _icard_OPT_BEGIN(clas_name,parent) \
	_WIDGET_OPT_BEGIN(clas_name,parent)\
	.paint = icard_paint, \
	.destroy = icard_destroy, \
	.start = icard_start, \
	.stop = icard_stop, \
	.get_focus = icard_get_focus, \
	.get_widget = icard_get_widget, \
	.touch_event_handle = icard_touch_event_handle, \
	.set_widget_tree = icard_set_widget_tree,


#define _icard_CREATE(clas_name,_name,x,y,w,h) \
	_WIDGET_CREATE(clas_name,_name,x,y,w,h) \
	m->child_paint_flag = 1;

typedef struct _icard_opt{
	_icard_OPT(widget)
}icard_opt;

typedef struct _icard_widget{
	_ICARD_CLASS(icard)
}micard;

micard *create_icard_widget(char *name, int x, int y, int w, int h);

#endif

