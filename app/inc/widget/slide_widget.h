#ifndef __SLIDE_WIDGET_H__
#define __SLIDE_WIDGET_H__

#include "widget.h"

enum {
	SLIDE_NODE_MOVE_DIR_X = 1,
	SLIDE_NODE_MOVE_DIR_Y,
	SLIDE_NODE_MOVE_DIR_XY,
};

typedef struct slideData{
	unsigned char		mv_start:1;
	unsigned char		mv_dir:2;	// 1: x; 2: y, 3: x and y
	unsigned char		cbk_flag:1;
	unsigned char		init_flag:1;
	unsigned char		first_child_init:1;
	unsigned char		active_status:1;	// 控件需要移动时为1
	unsigned char		button_status:1;	// 控件聚焦时为1

	int					startx;		// 开始滑动时 第一个子节点的位置
	int					starty;

	int					speedx;
	int					speedy;

	int first_child_location_x;		// 第一个子节点的初始位置
	int first_child_location_y;

	int					total_lenx;		// 总位移 往反方向滑动这个值会减小
	int					total_leny;

	int					head_x;		// 子节点可滑动区域起始坐标
	int					head_y;
	int					end_x;		// 子节点可滑动的末尾坐标
	int					end_y;
	int					slide_lenx;		// 子节点可滑动长度
	int					slide_leny;

	int					slide_w;	// 滑块总宽度
	int					slide_h;	// 滑块总高度

	int					show_b_xe;	// 显示区域B 的尾部坐标
	int					show_b_ye;
	int					show_b_xf;	// 显示区域B 的头部坐标
	int					show_b_yf;

	unsigned char 		adsorb_open;	//0:关闭吸附，1：打开吸附
	int 				adsorb_x;		//吸附点坐标,相对于父节点
	int 				adsorb_y;
}slideData;

struct _slide_widget;

#define _SLIDE_CLASS(clas_name) \
	_WIDGET_CLASS(clas_name) \
	slideData data;
	

#define _slide_EVENT_LIST \
	_widget_EVENT_LIST

#define _slide_PROPERTYS \
	{"moveDir",slide_move_dir_set,slide_move_dir_get}, \
	{"adsorbPoint",slide_adsorb_point_set,slide_adsorb_point_get}, \	
	_widget_PROPERTYS

#define _SLIDE_OPT(parent) \
	_WIDGET_OPT(parent) \
	int(*set_mv_dir)(struct _slide_widget *m,char ); \
	int(*set_border_gap)(struct slideData *data, int headx, int heady, int endx, int endy); \
	int (*set_adsorb_point)(struct slideData *data ,unsigned char open, int x,int y);
	

#define _slide_OPT_BEGIN(clas_name,parent) \
	_WIDGET_OPT_BEGIN(clas_name,parent) \
	.paint = slide_paint, \
	.start = slide_start, \
	.stop = slide_stop,\
	.set_mv_dir = slide_set_mv_dir,\
	.set_adsorb_point = slide_set_adsorb_point, \
	.get_focus = slide_get_focus, \
	.touch_event_handle = slide_touch_event_handle,


#define _slide_CREATE(clas_name,_name,x,y,w,h) \
	_WIDGET_CREATE(clas_name,_name,x,y,w,h) \



typedef struct _slide_opt{
	_SLIDE_OPT(widget)
}slide_opt;

typedef struct _slide_widget{
	_SLIDE_CLASS(slide)
}mslide;

mslide *create_slide_widget(char *name, int x, int y, int w, int h);

#endif
