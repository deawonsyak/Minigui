#ifndef __AIRCONDISION_WIDGET_H__
#define __AIRCONDISION_WIDGET_H__

#include "svg_widget.h"

struct _aircondision_widget;

#define _AIRCONDISION_CLASS(clas_name) \
	_SVG_CLASS(clas_name) \
	short steps; \
	short select_index; \
	unsigned int select_color; \
	unsigned int blank_color; \
	
	

#define _aircondision_EVENT_LIST \
	_svg_EVENT_LIST \
	"selectIndex", \

#define _aircondision_PROPERTYS \
	_widget_PROPERTYS

#define _AIRCONDISION_OPT(parent) \
	_SVG_OPT(parent) \
	int (*set_select_color)(struct _aircondision_widget *m, unsigned int rgb); \
	int (*set_select_index)(struct _aircondision_widget *m, short index);

#define _aircondision_OPT_BEGIN(clas_name, parent) \
	_svg_OPT_BEGIN(clas_name, parent) \
	.set_select_color = aircondision_set_select_color, \
	.set_select_index = aircondision_set_select_index, \
	.touch_event_handle = aircondision_touch_event_handle, \
	.get_focus = aircondision_get_focus,
	
	
#define _aircondision_CREATE(clas_name,_name, x,y,w,h) \
	_svg_CREATE(clas_name,_name,x,y,w,h) \
	m->blank_color = 0xD8D8D8; \
	m->select_color = 0x4A90E2; \
	m->steps = 21;	


typedef struct _aircondision_opt{
	_AIRCONDISION_OPT(svg)
}aircondision_opt;

typedef struct _aircondision_widget{
	_AIRCONDISION_CLASS(aircondision)
}maircondision;

maircondision *create_aircondision_widget(char *name, int x, int y, int w, int h);

int aircondision_set_select_color(maircondision *m, unsigned int rgb);
int aircondision_set_select_index(maircondision *m, short index);
void aircondision_refresh(maircondision *m);

#endif

