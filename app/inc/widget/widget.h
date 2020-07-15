#ifndef __WIDGET_H__
#define __WIDGET_H__

#include <minigui/common.h>
#include <minigui/minigui.h>
#include <minigui/gdi.h>
#include <minigui/window.h>
#include <minigui/control.h>

#include <stdio.h>
#include <stdlib.h>

#include <pthread.h>
#include "typeFace.h"
#include "gui_debug.h"
#include "gui_memory.h"


#define SCREEN_FORCE_FLAG_BUTTONDOWN	(1<<0)
#define SCREEN_FORCE_FLAG_BUTTONUP		(1<<1)
#define SCREEN_FORCE_FLAG_CLICK			(1<<2)
#define SCREEN_FORCE_FLAG_MOVEX			(1<<3)
#define SCREEN_FORCE_FLAG_MOVEY			(1<<4)
#define SCREEN_FORCE_FLAG_MOVESTOP		(1<<5)
#define SCREEN_FORCE_FLAG_MOVEOUT		(1<<6)
#define SCREEN_FORCE_FLAG_KEYBOARD		(1<<7)
#define SCREEN_FORCE_FLAG_LONGPRESS		(1<<8)
#define SCREEN_FORCE_FLAG_SLIDE			(1<<9)

#define WIDGET_TOUCH_EVENT_HANDLE(m,flag,coor) \
	m->opt->touch_event_handle(m,flag,coor)

#define MODULUS(x) \
	if(x < 0){ \
		x = 0-x; \
	}

struct _widget;
struct _widget_opt;
typedef struct _widget_opt _void_opt;
struct Screen;

struct gui_coordinate{
	int x;
	int y;
};

struct move_info{
	struct gui_coordinate coor;
	float speedx;
	float speedy;
	int	lengthx;
	int	lengthy;
};


void gui_node_data_lock(struct _widget *node);
void gui_node_data_unlock(struct _widget *node);
int gui_node_get_focus(struct _widget *node);
void widget_show(struct _widget *node);

struct _widget *widget_get_focus(struct _widget *node, unsigned int flag, struct gui_coordinate *coor);
struct _widget *widget_get_widget(struct _widget *node, char *name);
void widget_release_focus(struct _widget *node);
int	widget_touch_event_handle(struct _widget *node, unsigned int flag, struct move_info *mv_info);
struct _widget *widget_keyboard_event_handle(struct _widget *node, char *key);

int gui_node_buttondown_handle(struct _widget *node, void *param);
int gui_node_buttonup_handle(struct _widget *node, void *param);
int gui_node_click_handle(struct _widget *node, void *param);
int gui_node_move_handle(struct _widget *node, void *param);
int gui_node_stop_handle(struct _widget *node, void *param);
int gui_node_location_get(struct _widget *node);
int gui_node_paint_enable(struct _widget *node,int count);

int widget_hide(struct _widget *node,int enable);
void widget_destroy(struct _widget *node,struct _widget_opt *clas);
void widget_refresh(struct _widget *m);
int widget_activate_event(struct _widget *node, char *event);
int widget_property_set(struct _widget *node, char *property, char *value);
char *widget_property_get(struct _widget *node, char *property);

int widget_property_x_set(struct _widget *node,char *value);
char *widget_property_x_get(struct _widget *node);
int widget_property_y_set(struct _widget *node,char *value);
char *widget_property_y_get(struct _widget *node);
int widget_property_w_set(struct _widget *node,char *value);
char *widget_property_w_get(struct _widget *node);
int widget_property_h_set(struct _widget *node,char *value);
char *widget_property_h_get(struct _widget *node);
int widget_property_location_set(struct _widget *node,char *value);
char *widget_property_location_get(struct _widget *node);
char *widget_property_rect_get(struct _widget *node);
int widget_property_rect_set(struct _widget *node,char *value);

int widget_property_hide_set(struct _widget *node, char *value);
char *widget_property_hide_get(struct _widget *node);



//#include "uni_gui_node.h"

enum location_type{
	GUI_LOCATION_TYPE_PARENT,
	GUI_LOCATION_TYPE_PREV_BROTHERS,
	GUI_LOCATION_TYPE_ABSOLUTE,
};

typedef struct _widget_event{
	void *hd;		// 控件句柄
	char *event;	// event 名称
}widget_event_t;

typedef struct widget_property{
	char *name;
	int (*set)(struct _widget *node,char *value);
	char *(*get)(struct _widget *node);
}widget_property_t;

int widget_event_send(struct _widget *m, char *event);
widget_event_t *widget_event_create(char *event, void *hd);
void widget_event_destroy(widget_event_t *event);

#define OPTName(name) \
	name##_opt *opt;

/********************************************************
 *	
 *	child_paint_flag	子节点绘制控制标示
 *		
 * ******************************************************/
#define _WIDGET_CLASS(_name) \
	OPTName(_name) \
	char name[32]; \
	int x; \
	int y; \
	int w; \
	int h; \
	int start_abs_x;	/* start_abs_x	absolute coordinate*/ \
	int start_abs_y;	/* start_abs_y */ \
	int show_w;			/* show_w		(有可能显示的起始绝对位置超过了可显示区域，以下四个值就是在可显示区域内的起始绝对位置) */ \
	int show_h; \
	int show_abs_x;				\
	int show_abs_y;				\
	enum location_type location_type; \
	unsigned char need_paint:1; \
	unsigned char child_paint_flag:1; /* 0: 子节点按正常流程绘制， 1: 子节点不绘制*/\
	unsigned char hide:1; /*1 hide*/ \
	unsigned char init_flag:1; \
	unsigned char start_flag:1; \
	unsigned char stop_flag:1; \
	unsigned char focus:1; \
	unsigned char is_noplay_click_tone:1;	/* 0: play 1: no play */ \
	unsigned char is_delete:1; \
	unsigned char is_destroy:1; \
	unsigned char screen_level:2;  /* 0: not in screen; 1:screen->node; 2: screen->suspenting; 3:screen->popup*/ \
	unsigned char focus_lock:1; \
	unsigned char				deep; \
	pthread_mutex_t data_mutex; \
	struct _widget		*parent; \
	struct _widget		*child; \
	struct _widget		*next; \
	struct Screen	*screen; \
	pthread_mutex_t node_mutex; \
	pthread_mutex_t handle_mutex; \
	unsigned int event_active_flag;

#define WIDGET_EVENT_MAX 32

/********************************************************
 *
 * ******************************************************/
#define _WIDGET_OPT(_parent) \
	_parent##_opt *parent; \
	const char *class_name; \
	const widget_property_t *property;\
	char **event_list; \
	char *(*get_property)(struct _widget *node,char *property); \
	int (*set_property)(struct _widget *node,char *property, char *value);\
	void (*lock_data)(struct _widget *node); \
	void (*unlock_data)(struct _widget *node); \
	int (*paint)(struct _widget *node,HDC hdc); \
	void(*destroy)(struct _widget*, struct _widget_opt *clas); \
	void(*refresh)(struct _widget*); \
	int (*start)(struct _widget*); \
	int (*stop)(struct _widget*); \
	void(*show)(struct _widget*); \
	struct _widget* (*get_focus)(struct _widget *node,unsigned int flag, struct gui_coordinate *coor); \
	struct _widget* (*get_widget)(struct _widget *node,char *name); \
	void(*release_focus)(struct _widget *node); \
	void(*release_keyboard_focus)(struct _widget *node); \
	int (*widget_hide)(struct _widget *node,int hide_id); /* hide_id:1 hide 0show */ \
	int (*get_location)(struct _widget *node); \
	int (*event_send)(struct _widget *m, char *event); \
	int (*activate_event)(struct _widget *m, char *event); \
	int (*touch_event_handle)(struct _widget *m, unsigned int flag, struct move_info *coor); \
	struct _widget* (*keyboard_event_handle)(struct _widget *m, char *key);

/********************************************************
 *
 * ******************************************************/
#define _widget_EVENT_LIST \
	"onStart", \
	"onButtonDown", \
	"onButtonUp", \
	"onClick", \
	"onLongPress", \

/********************************************************
 *
 * ******************************************************/
#define _widget_PROPERTYS \
	{"x",widget_property_x_set,widget_property_x_get}, \
	{"y",widget_property_y_set,widget_property_y_get}, \
	{"w",widget_property_w_set,widget_property_w_get}, \
	{"h",widget_property_h_set,widget_property_h_get}, \
	{"rect",widget_property_rect_set,widget_property_rect_get},\
	{"location",widget_property_location_set,widget_property_location_get}, \
	{"hide",widget_property_hide_set,widget_property_hide_get},


/********************************************************
 *
 * ******************************************************/
#define __WIDGET_OPT_BEGIN(clas_name) \
	.class_name = #clas_name, \
	.destroy = widget_destroy, \
	.refresh = widget_refresh, \
	.show = widget_show, \
	.lock_data = gui_node_data_lock, \
	.unlock_data = gui_node_data_unlock, \
	.get_focus = widget_get_focus, \
	.get_widget = widget_get_widget, \
	.release_focus = widget_release_focus, \
	.widget_hide = widget_hide, \
	.get_location = gui_node_location_get, \
	.event_send = widget_event_send, \
	.activate_event = widget_activate_event, \
	.touch_event_handle = widget_touch_event_handle, \
	.set_property = widget_property_set, \
	.get_property = widget_property_get, \
	.property = g_##clas_name##_property, \
	.event_list = g_##clas_name##_events,


	

#define _WIDGET_OPT_BEGIN(clas_name,_parent) \
	.parent = &gp##_parent##_opt, \
	.class_name = #clas_name, \
	__WIDGET_OPT_BEGIN(clas_name)

#define _widget_OPT_BEGIN(clas_name,_parent)  _WIDGET_OPT_BEGIN(clas_name,_parent) 


#define __WIDGET_GENERATE(clas_name) \
	static widget_property_t g_##clas_name##_property[] = { \
		_##clas_name##_PROPERTYS \
		{NULL,NULL,NULL} \
	}; \
	const static char *g_##clas_name##_events[] = { \
		_##clas_name##_EVENT_LIST \
		NULL \
	};\
	clas_name##_opt gp##clas_name##_opt = { \
		.parent = NULL, \
		__WIDGET_OPT_BEGIN(clas_name) \
	};

/********************************************************
 *	micro	_WIDGET_GENERATE
 *		生成 控件属性 数组
 *		生成 控件opt结构体
 *		生成 控件event 数组
 *		生成 控件创建函数
 *
 *	此宏需要在新增的控件的.c文件中调用
 *
 * ******************************************************/
#define _WIDGET_GENERATE(clas_name,_parent) \
	static widget_property_t g_##clas_name##_property[] = { \
		_##clas_name##_PROPERTYS \
		{NULL,NULL,NULL} \
	}; \
	const static char *g_##clas_name##_events[] = { \
		_##clas_name##_EVENT_LIST \
		NULL \
	};\
	clas_name##_opt gp##clas_name##_opt = { \
		_##clas_name##_OPT_BEGIN(clas_name,_parent) \
	}; \
	m##clas_name *create_##clas_name##_widget(char *name, int x,int y, int w, int h) \
	{ \
		_##clas_name##_CREATE(clas_name,name,x,y,w,h); \
		return m; \
	}

#define _WIDGET_CREATE(clas_name,_name,x,y,w,h) \
	m##clas_name *m = widget_calloc(1,sizeof(m##clas_name)); \
	if(m == NULL){ \
		return NULL; \
	} \
	m->x = x; \
	m->y = y; \
	m->w = w; \
	m->h = h; \
	m->start_abs_x = x; \
	m->start_abs_y = y; \
	m->show_w = w;  \
	m->show_h = h; \
	m->show_abs_x = x; \
	m->show_abs_y = y; \
	if(name){ \
		strncpy(m->name,_name,sizeof(m->name)-1); \
	}\
	pthread_mutex_init(&m->data_mutex,NULL); \
	pthread_mutex_init(&m->node_mutex,NULL); \
	pthread_mutex_init(&m->handle_mutex,NULL); \
	m->opt = &gp##clas_name##_opt; \

#define _widget_CREATE(clas_name,_name,x,y,w,h)  _WIDGET_CREATE(clas_name,_name,x,y,w,h)

typedef struct _widget_opt{
	//_WIDGET_OPT(_widget)
	_WIDGET_OPT(_void)
}widget_opt;

extern widget_opt gpwidget_opt;

typedef struct _widget{
	_WIDGET_CLASS(widget) \
}mwidget,uni_gui_node;

mwidget *create_widget(char *name,int x,int y, int w, int h);

#endif
