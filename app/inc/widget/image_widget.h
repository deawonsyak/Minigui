#ifndef __IMAGE_WIDGET_H__
#define __IMAGE_WIDGET_H__
#include <minigui/common.h>
#include <minigui/minigui.h>
#include <minigui/gdi.h>
#include <minigui/window.h>
#include <minigui/control.h>
#include <pthread.h>

#include "uni_gui_node.h"
#include "widget.h"

typedef struct image_data{
	char			*path; 
	BITMAP			*bkmap; 
	int				refer_num;
	unsigned char	auto_rotate:1;
	short			rotate_speed;
	short			rotate;
}imageData;

struct _image_widget;

#define _IMAGE_CLASS(clas_name) \
	_WIDGET_CLASS(clas_name) \
	imageData	image_data;

#define _image_EVENT_LIST \
	_widget_EVENT_LIST

#define _image_PROPERTYS \
	{"image",image_set_picture,image_widget_get_picture}, \
	_widget_PROPERTYS

#define _IMAGE_OPT(parent) \
	_WIDGET_OPT(parent) \
	int (*set_picture)(struct _image_widget *node,char *path); \



#define _IMAGE_OPT_BEGIN(clas_name,parent) \
	_WIDGET_OPT_BEGIN(clas_name,parent) \
	.set_picture = image_set_picture, \
	.paint = image_node_paint, \
	.destroy = image_widget_destroy, \
	.start = image_widget_start, \
	.stop = image_widget_stop, \

	
#define _image_OPT_BEGIN(clas_name,parent) _IMAGE_OPT_BEGIN(clas_name,parent) \


#define _IMAGE_CREATE(clas_name,_name,x,y,w,h) \
	_WIDGET_CREATE(clas_name,_name,x,y,w,h) \
	
#define _image_CREATE(clas_name,_name,x,y,w,h)  _IMAGE_CREATE(clas_name,_name,x,y,w,h) \

typedef struct _image_opt{
	_IMAGE_OPT(widget)
}image_opt;

typedef struct _image_widget{
	_IMAGE_CLASS(image)
}mimage;

mimage *create_image_widget(char *name, int x, int y, int w, int h);

char *image_widget_get_picture(mimage *node);
int image_set_picture(mimage *node, char *path);



#endif

