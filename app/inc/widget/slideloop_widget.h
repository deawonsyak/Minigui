#ifndef __SLIDELOOP_WIDGET_H__
#define __SLIDELOOP_WIDGET_H__

#include "widget.h"


enum {
	SLIDELOOP_NODE_MOVE_DIR_X = 0,
	SLIDELOOP_NODE_MOVE_DIR_Y,
};

typedef struct slideloopData{
	unsigned char		mv_start:1;
	unsigned char		mv_dir:1;	// 0: x; 1: y
	unsigned char		init_flag:1;
	unsigned char		active_status:1;	// 控件需要移动时为1
	unsigned char		button_status:1;	// 控件聚焦时为1
	unsigned char		calc_flag:1;	

	int					gap;		// 子节点间距

	int					startx;		// 开始滑动时 第一个子节点的位置
	int					starty;

	int					speed;
	int					speedy;

	int					total_len;		// 总位移 往反方向滑动这个值会减小
	int					move_len;		// 当前移动长度


	int					slide_len;	// 滑块总长度

	int					show_b_1;
	int					show_b_e;	// 显示区域B 的尾部坐标
	int					show_b_f;	// 显示区域B 的头部坐标

	unsigned char 		adsorb_open;	//0:关闭吸附，1：打开吸附
	int 				adsorb_x;		//吸附点坐标,相对于父节点
	int 				adsorb_y;
}slideloopData;

struct _slideloop_widget;

#define _SLIDELOOP_CLASS(clas_name) \
	_WIDGET_CLASS(slideloop) \
	slideloopData data;

#define _slideloop_EVENT_LIST \
	_widget_EVENT_LIST


#define _slideloop_PROPERTYS \
	{"moveDir",slideloop_move_dir_set,slideloop_move_dir_get}, \
	{"gap",slideloop_gap_set,slideloop_gap_get}, \
	{"adsorbPoint",slideloop_adsorb_point_set,slideloop_adsorb_point_get}, \
	_widget_PROPERTYS


#define _SLIDELOOP_OPT(parent) \
	_WIDGET_OPT(parent) \
	int (*set_mv_dir)(struct _slideloop_widget *m,char ); \
	int (*set_gap)(struct _slideloop_widget *m, int gap); \
	int (*set_adsorb_point)(struct _slideloop_widget *m, unsigned char open, int x,int y); 

#define _slideloop_OPT_BEGIN(clas_name,parent) \
	_WIDGET_OPT_BEGIN(clas_name,parent) \
	.paint = slideloop_paint, \
	.start = slideloop_start, \
	.set_mv_dir = slideloop_set_mv_dir, \
	.set_gap = slideloop_set_gap, \
	.set_adsorb_point = slideloop_set_adsorb_point, \
	.get_focus = slideloop_get_focus, \
	.touch_event_handle = slideloop_touch_event_handle,

#define _slideloop_CREATE(clas_name,_name,x,y,w,h) \
	_WIDGET_CREATE(clas_name,_name,x,y,w,h) \

typedef struct _slideloop_opt{
	_SLIDELOOP_OPT(widget)
}slideloop_opt;

typedef struct _slideloop_widget{
	_SLIDELOOP_CLASS(slideloop)
}mslideloop;

mslideloop *create_slideloop_widget(char *name, int x, int y, int w, int h);

#endif
