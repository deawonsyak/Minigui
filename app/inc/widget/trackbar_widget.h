#ifndef __TRACKBAR_WIDGET_H__
#define __TRACKBAR_WIDGET_H__

#include "widget.h"


typedef struct trackbarData{
	int level;
	int curLevel;
	int barCenter;
	char axis;		//horizontal: 0, vertical: 1
	char *trackPath;
	BITMAP *trackBitMap;
	char *barPath;
	BITMAP *barBitMap;
	int barX;
	int barY;
	int barW;
	int barH;
}trackbarData;

struct _trackbar_widget;

void create_trackbar_data(struct _trackbar_widget *m);

#define _TRACKBAR_CLASS(clas_name) \
	_WIDGET_CLASS(clas_name) \
	trackbarData *trackbar;		\


#define _trackbar_EVENT_LIST \
	"levelUpdate",	\
	_widget_EVENT_LIST


#define _trackbar_PROPERTYS \
	{"level",trackbar_level_set,trackbar_level_get}, \
	{"trackImage",trackbar_trackImage_set,trackbar_trackImage_get}, \
	{"barImage",trackbar_barImage_set,trackbar_barImage_get}, \
	{"axis",trackbar_axis_set,trackbar_axis_get}, \
	{"barWidth",trackbar_barWidth_set,trackbar_barWidth_get}, \
	{"barHeight",trackbar_barHeight_set,trackbar_barHeight_get}, \
	_widget_PROPERTYS

//	{"text",trackbar_text_set,trackbar_text_get}, \		//调试用

#define _TRACKBAR_OPT(parent) \
	_WIDGET_OPT(parent) 		\

#define _trackbar_OPT_BEGIN(clas_name,parent) \
	_WIDGET_OPT_BEGIN(clas_name,parent) \
	.paint = trackbar_node_paint, \
	.destroy = trackbar_destroy, \		
	.start = trackbar_widget_start, \
	.stop = trackbar_widget_stop, \
	.get_focus = trackbar_get_focus,	\
	.touch_event_handle = trackbar_touch_event_handle,	\

//创建控件，使用默认配置
#define _trackbar_CREATE(clas_name,_name,x,y,w,h) \
	_WIDGET_CREATE(clas_name,_name,x,y,w,h) \
	create_trackbar_data(m);

typedef struct _trackbar_opt{
	_TRACKBAR_OPT(widget)
}trackbar_opt;

typedef struct _trackbar_widget{
	_TRACKBAR_CLASS(trackbar)
}mtrackbar;

mtrackbar *create_trackbar_widget(char *name, int x, int y, int w, int h);

#endif

