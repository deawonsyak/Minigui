#ifndef __BUTTON_WIDGET_H__
#define __BUTTON_WIDGET_H__

#include <minigui/common.h>
#include <minigui/minigui.h>
#include <minigui/gdi.h>
#include <minigui/window.h>
#include <minigui/control.h>

#include "widget.h"

enum{
	BUTTON_STATUS_UP,
	BUTTON_STATUS_DOWN,
};

typedef struct _button_data{
	int			refer_num;
	char *up_path;
	char *down_path;
	BITMAP *up_map;
	BITMAP *down_map;

}buttonData;

struct _button_widget;

#define _BUTTON_CLASS(cla_name) \
	_WIDGET_CLASS(cla_name) \
	buttonData *data; \
	/* 按钮状态 */	\
	unsigned char button_status;  \


#define _BUTTON_OPT(_parent) \
	_WIDGET_OPT(_parent) \
	int (*image_set)(struct _button_widget *m, char *up_path, char *down_path);
	
#define _button_EVENT_LIST \
	_widget_EVENT_LIST \

#define _button_PROPERTYS \
	{"upImage",button_up_image_set,button_up_image_get}, \
	{"downImage",button_down_image_set,button_down_image_get},\
	_widget_PROPERTYS \
	
#define _BUTTON_OPT_BEGIN(clas_name,parent) \
	_WIDGET_OPT_BEGIN(clas_name,parent) \
	.paint = button_node_paint, \
	.destroy = button_destroy, \
	.image_set = button_image_set, \
	.touch_event_handle = button_touch_event_handle, \
	.get_focus = button_get_focus, \
	.release_focus = button_release_focus, \
	.start = button_widget_start, \
	.stop = button_widget_stop,

#define _button_OPT_BEGIN(clas_name,parent)  _BUTTON_OPT_BEGIN(clas_name,parent) \

#define _BUTTON_CREATE(clas_name,_name,x,y,w,h) \
	_WIDGET_CREATE(clas_name,_name,x,y,w,h) \

#define _button_CREATE(clas_name,_name,x,y,w,h)  _BUTTON_CREATE(clas_name,_name,x,y,w,h) \

typedef struct _button_opt{
	_BUTTON_OPT(widget)
}button_opt;

typedef struct _button_widget{
	_BUTTON_CLASS(button)
}mbutton;

mbutton *create_button_widget(char *name, int x, int y, int w, int h);
int button_node_paint(mwidget *node, HDC hdc,int x,int y);
int button_widget_start(mwidget *node);

/********************************************************
 *	屬性函數
 * ******************************************************/
int button_down_image_set(mbutton *m, char *down_path);
int button_up_image_set(mbutton *m, char *up_path);
char *button_down_image_get(mbutton *m);
char *button_up_image_get(mbutton *m);

#endif
