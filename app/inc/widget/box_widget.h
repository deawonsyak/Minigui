#ifndef __BOX_WIDGET_H__
#define __BOX_WIDGET_H__

#include "widget.h"
#include "screen.h"
#include "typeFace.h"


struct _box_widget;

#define BOX_ROTATION_ANGLE_DEGREE (64)

enum{
	BOX_NOT_NEED_ROTATE_SCALE = 0,
	BOX_NEED_ROTATE_SCALE
};



#define _BOX_CLASS(clas_name) \
	_WIDGET_CLASS(clas_name) \
	HDC hdc;  \
	unsigned char alpha; \
	unsigned char alpha_flag:1; \
	int scale_width; \
	int scale_height;  \
	unsigned char	auto_rotate:1; \
	short			rotate_speed; \
	short			rotate;

#define _box_EVENT_LIST \
	_widget_EVENT_LIST

#define _box_PROPERTYS \
	{"alpha",box_alpha_set,box_alpha_get}, \
	{"rotate",box_rotate_set, box_rotate_get}, \
	{"scale", box_scale_set, box_scale_get}, \
	{"autoRotate", box_auto_rotate_set, box_auto_rotate_get}, \
	_widget_PROPERTYS
	
#define _BOX_OPT(parent) \
	_WIDGET_OPT(parent) \
	int (*set_alpha)(struct _box_widget *m, int enable, unsigned char alpha); \
	int (*set_rotate)(struct _box_widget *m, int rotate); \
	int (*set_scale)(struct _box_widget *m, unsigned int scale_width, unsigned int scale_height); \
	int (*set_auto_rotate)(struct _image_widget *node, int enable, int rotate_speed);

	
#define _BOX_OPT_BEGIN(clas_name,parent) \
	_WIDGET_OPT_BEGIN(clas_name,parent) \
	.paint = box_paint, \
	.destroy = box_destroy, \
	.set_alpha = box_set_alpha, \
	.set_rotate = box_set_rotate, \
	.set_scale = box_set_scale, \
	.set_auto_rotate = box_set_auto_rotate,

#define _box_OPT_BEGIN(clas_name,parent) _BOX_OPT_BEGIN(clas_name,parent)

#define _BOX_CREATE(clas_name,_name,x,y,w,h) \
	_WIDGET_CREATE(clas_name,_name,x,y,w,h) \
	m->child_paint_flag = 1; \
	m->scale_width = -1; \
	m->scale_height = -1; \

#define _box_CREATE(clas_name,_name,x,y,w,h) _BOX_CREATE(clas_name,_name,x,y,w,h)

typedef struct _box_opt{
	_BOX_OPT(widget)
}box_opt;

typedef struct _box_widget{
	_BOX_CLASS(box)
}mbox;

mbox *create_box_widget(char *name, int x, int y, int w, int h);
void box_destroy(mwidget *node,widget_opt *clas);

// 控件屬性函數
int box_alpha_set(mbox *m, char *value);
char *box_alpha_get(mbox *m);

#endif


