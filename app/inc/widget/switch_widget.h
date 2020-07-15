#ifndef __SWITCH_WIDGET_H__
#define __SWITCH_WIDGET_H__

#include "svg_widget.h"


struct _toggleswitch_widget;

#define _TOGGLESWITCH_CLASS(clas_name) \
	_SVG_CLASS(clas_name) \
	signed char button_status;  /* 0: on, 1: on-off n: on-off 8: off */ \
	unsigned char start_move:1; \
	unsigned char button_flag:1; /* 0: off, 1: on*/ \
	BITMAP *on_map; \
	BITMAP *off_map;
	


#define _toggleswitch_EVENT_LIST \
	_svg_EVENT_LIST 


#define _toggleswitch_PROPERTYS \
	_widget_PROPERTYS


#define _TOGGLESWITCH_OPT(parent) \
	_SVG_OPT(parent) \

#define _toggleswitch_OPT_BEGIN(clas_name,parent) \
	_svg_OPT_BEGIN(clas_name,parent) \
	.paint = toggleswitch_paint, \
	.get_focus = toggleswitch_get_focus, \
	.touch_event_handle = toggleswitch_touch_event_handle,


#define _toggleswitch_CREATE(clas_name,_name,x,y,w,h) \
	_svg_CREATE(clas_name,_name,x,y,w,h) \
	BITMAP *map = (BITMAP*)calloc(1,sizeof(BITMAP)); \
	int err_code = LoadBitmapFromFile(HDC_SCREEN,map,"res/toggle_on.png"); \
	if (err_code != ERR_BMP_OK) { \
	} \
	m->on_map = map; \
	map = (BITMAP*)calloc(1,sizeof(BITMAP)); \
	err_code = LoadBitmapFromFile(HDC_SCREEN,map,"res/toggle_off.png"); \
	if (err_code != ERR_BMP_OK) { \
	} \
	m->off_map = map; \
	toggleswitch_update(m);

typedef struct _toggleswitch_opt{
	_TOGGLESWITCH_OPT(svg)
}toggleswitch_opt;

typedef struct _switch_widget{
	_TOGGLESWITCH_CLASS(toggleswitch)
}mtoggleswitch;

mtoggleswitch *create_toggleswitch_widget(char *name, int x, int y, int w, int h);


#endif

