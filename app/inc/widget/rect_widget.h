#ifndef __RECT_WIDGET_H__
#define __RECT_WIDGET_H__

#include "widget.h"
#include "screen.h"

struct _rect_widget;

enum {
	RECT_FILL_TYPE_SOLID,
	RECT_FILL_TYPE_CENTER_SHADE,
};

#define _RECT_CLASS(clas_name) \
	_WIDGET_CLASS(clas_name) \
	unsigned char	is_roundrect:1; \
	unsigned char	is_center_shode:1; \
	unsigned char	fill_type; \
	unsigned char	alpha; \
	gal_pixel		shode_start_color; \
	gal_pixel		shode_stop_color; \
	short			shode_centerx; \
	short			shode_centery; \
	short			shode_r; \
	gal_pixel		bgColor; \
	short			x_radius; \
	short			y_radius;
	

#define _rect_EVENT_LIST \
	_widget_EVENT_LIST

#define _rect_PROPERTYS \
	{"color", rect_bgColor_set, rect_bgColor_get}, \
	{"alpha", rect_alpha_set, rect_alpha_get}, \
	{"rounded",rect_rounded_set, rect_rounded_get},\
	_widget_PROPERTYS

#define _RECT_OPT(parent) \
	_WIDGET_OPT(parent) \
	int (*setCenterShode)(struct _rect_widget *node, int enable, int centerx, int centery,int r, gal_pixel start_color,gal_pixel stop_color);

#define _rect_OPT_BEGIN(clas_name,parent) \
	_WIDGET_OPT_BEGIN(clas_name,parent) \
	.paint = rect_paint, \
	.setCenterShode = rect_set_center_shode,


#define _rect_CREATE(clas_name,_name,x,y,w,h) \
	_WIDGET_CREATE(clas_name,_name,x,y,w,h) \
	m->alpha = 0xff; \
	m->bgColor = 0xffffff; \
	

typedef struct _rect_opt{
	_RECT_OPT(widget)
}rect_opt;

typedef struct _rect_widget{
	_RECT_CLASS(rect);
}mrect;

mrect *create_rect_widget(char *name, int x, int y, int w, int h);


char *rect_bgColor_get(mrect *m);
int rect_bgColor_set(mrect *m, char *value);
char *rect_rounded_get(mrect *m);
int rect_rounded_set(mrect *m, char *value);

#endif

