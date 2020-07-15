#ifndef __GIF_WIDGET_H__
#define __GIF_WIDGET_H__

#include "widget.h"
#include "gif_decode.h"

struct _gif_widget;


#define _GIF_CLASS(clas_name) \
	_WIDGET_CLASS(clas_name) \
	HDC hdc; \
	char	*path; \
	gifData *gif;

#define _gif_EVENT_LIST \
	_widget_EVENT_LIST

#define _gif_PROPERTYS \
	_widget_PROPERTYS

#define _GIF_OPT(parent) \
	_WIDGET_OPT(parent) \
	int (*set_path)(struct _gif_widget *m, char *path); \


#define _gif_OPT_BEGIN(clas_name,parent) \
	_WIDGET_OPT_BEGIN(clas_name,parent) \
	.paint = gif_paint, \
	.destroy = gif_destroy, \
	.set_path = gif_set_path,
	


#define _gif_CREATE(clas_name,_name,x,y,w,h) \
	_WIDGET_CREATE(clas_name,_name,x,y,w,h) \
	m->hdc = screen_create_hdc(w,h); \


typedef struct _gif_opt{
	_GIF_OPT(widget)
}gif_opt;

typedef struct _gif_widget{
	_GIF_CLASS(gif)
}mgif;

mgif *create_gif_widget(char *name, int x, int y, int w, int h);

#endif
