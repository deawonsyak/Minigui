#ifndef __SLIDESCREEN_WIDGET_H__
#define __SLIDESCREEN_WIDGET_H__

#include "widget.h"

#define SLIDESCREEN_CHANGE_SCREEN_SPEED (10)

enum {
	SLIDESCREEN_NODE_MOVE_DIR_X = 0,
	SLIDESCREEN_NODE_MOVE_DIR_Y,
};

typedef struct slidescreenData{
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
	int                 umg;			
}slidescreenData;

struct _slidescreen_widget;

#define _SLIDESCREEN_CLASS(clas_name) \
	_WIDGET_CLASS(slidescreen) \
	slidescreenData data;

#define _slidescreen_EVENT_LIST \
	_widget_EVENT_LIST


#define _slidescreen_PROPERTYS \
	{"moveDir",slidescreen_move_dir_set,slidescreen_move_dir_get}, \
	{"gap",slidescreen_gap_set,slidescreen_gap_get}, \
	{"adsorbPoint",slidescreen_adsorb_point_set,slidescreen_adsorb_point_get}, \
	_widget_PROPERTYS


#define _SLIDESCREEN_OPT(parent) \
	_WIDGET_OPT(parent) \
	int (*set_mv_dir)(struct _slidescreen_widget *m,char ); \
	int (*set_gap)(struct _slidescreen_widget *m, int gap); \
	int (*set_adsorb_point)(struct _slidescreen_widget *m, unsigned char open, int x,int y); 

#define _slidescreen_OPT_BEGIN(clas_name,parent) \
	_WIDGET_OPT_BEGIN(clas_name,parent) \
	.paint = slidescreen_paint, \
	.start = slidescreen_start, \
	.set_mv_dir = slidescreen_set_mv_dir, \
	.set_gap = slidescreen_set_gap, \
	.set_adsorb_point = slidescreen_set_adsorb_point, \
	.get_focus = slidescreen_get_focus, \
	.touch_event_handle = slidescreen_touch_event_handle,

#define _slidescreen_CREATE(clas_name,_name,x,y,w,h) \
	_WIDGET_CREATE(clas_name,_name,x,y,w,h) \

typedef struct _slidescreen_opt{
	_SLIDESCREEN_OPT(widget)
}slidescreen_opt;

typedef struct _slidescreen_widget{
	_SLIDESCREEN_CLASS(slidescreen)
}mslidescreen;

mslidescreen *create_slidescreen_widget(char *name, int x, int y, int w, int h);

#endif
