#ifndef __UNI_SCREEN_H__
#define __UNI_SCREEN_H__
#include <minigui/common.h>
#include <minigui/minigui.h>
#include <minigui/gdi.h>
#include <minigui/window.h>
#include <minigui/control.h>
#include <sys/time.h>
#include <pthread.h>
#include <semaphore.h>

#include "uni_gui_node.h"

#include "platform.h"
#include "orb_queue.h"
#include "orb_list.h"

#define MSG_SCREEN_ERASEBKGND 0x10000
#define MSG_SCREEN_PAINT		0x10001

#define SCREEN_TIMER_ID 0x11
#define SCREEN_TIMER_FREQ 2

#define SCREEN_MOVELEN_NUM 5
#define SCREEN_MOVELEN_AVERAGE 2

enum{
	SCREEN_BACKGROUND_TYPE_COLOR,
	SCREEN_BACKGROUND_TYPE_PHOTO,
};

enum {
	SCREEN_LEVEL_NONE = 0,
	SCREEN_LEVEL_NODE,
	SCREEN_LEVEL_TOOL_LAYER,
	SCREEN_LEVEL_SUSPENTION,
};

enum{
	SCREEN_MOUSE_STATUS_UP,
	SCREEN_MOUSE_STATUS_DOWN,
};

typedef enum{
	SCREEN_EVENT_CBK_PRIORITY_NORMAL,	
	SCREEN_EVENT_CBK_PRIORITY_HIGH,
	SCREEN_EVENT_CBK_PRIORITY_HIGHEST
}screen_event_cbk_priority_e;

typedef struct{
	screen_event_cbk_priority_e priority;
	int (*cbk)(widget_event_t *event,void *param);
	void *param;
}screen_widget_event_cbk_t;

typedef struct Screen {
	int x;
	int y;
	int	w;
	int h;
	int index;

	struct gui_coordinate buttondown_coor;
	struct gui_coordinate buttondup_coor;
	struct gui_coordinate mousemove_coor;
	struct gui_coordinate prev_mousemove_coor;

	struct gui_coordinate movelen[SCREEN_MOVELEN_NUM];
	unsigned char movelen_index;

	uint64_t last_move_tick;	
	uint64_t buttondown_tick;	// 记录按下时的tick

	unsigned char	mv_start:1;
	unsigned char	mouse_status:2;		// 0: up, 1: down
	unsigned char 	focus:1;
	unsigned char	longpress_flag:1;	// 长按事件触发标志.

	struct timeval	tv;
	pthread_t		pt;
	sem_t			sem;
	pthread_mutex_t mutex;

	int			timer_count;
	HWND		hwnd;
	char		name[64];
	gal_pixel	bgColor;
	char		*bk_pho;
	BITMAP		*bkmap;

	SetBkpho		setBkpho;
	SetBackground	setBackground;
	TIMERPROC		timeproc;

	orb_queue *widget_event_queue;
	
	// 控件事件回调函数
	int (*widget_event_handle_cbk)(widget_event_t *event,void *param);
	void *widget_event_handle_param;

	orb_list_t *widget_event_cbk_list;	// 组合控件的子控件事件回调函数列表
	orb_mutex_t widget_event_cbk_mutex;	// 控件事件回调锁

	orb_list_t *xmlwidget_list;		// xml自定义控件列表

	mwidget *buttondown_node;
	mwidget *focus_node;			// 触摸事件焦点控件
	mwidget *keyboard_focus_node;	// 键盘事件焦点控件

	mwidget *node;					// 主屏层 - 页面显示在这层
	mwidget *tool_layer;			// 工具层 - 键盘等工具显示在这层
	mwidget *suspention;			// 弹窗层 - 提示框，确认框显示在这层

}Screen;

extern Screen g_screen;

int screen_start_node(uni_gui_node *node, Screen *screen);
int screen_stop_node(mwidget *node, Screen *screen);

/********************************************************
 *	function	ad_node_to_screen
 *		添加控件树到屏幕层
 *
 *	@newnode
 *		被添加的控件树
 *
 * ******************************************************/
int add_node_to_screen(mwidget *newnode);

/********************************************************
 *
 * ******************************************************/
int add_node_to_toollayer(mwidget *newnode);

Screen *uni_get_gui_node_screen(uni_gui_node *node);
int add_node_to_screen_suspention(uni_gui_node *node, Screen *screen);
uni_gui_node *remove_node_from_screen_suspention(uni_gui_node *node, Screen *screen);
int add_node_to_screen_popup(uni_gui_node *node, Screen *screen);
uni_gui_node *remove_node_from_screen_popup(uni_gui_node *node, Screen *screen);
uni_gui_node *change_popup_to_screen(uni_gui_node *newnode,Screen *screen);
void delete_node_from_screen_popup(uni_gui_node *node, Screen *screen);
int remove_node_from_screen(uni_gui_node *node);

BOOL initScreen(Screen *screen);
void unloadBitMap(BITMAP *bmp);

HDC screen_create_hdc(int w, int h);
void screen_destroy_hdc(HDC hdc);
int screen_touch_event_handle(mwidget *node,unsigned int flag, void *param);

mwidget *widget_tree_focus_check(mwidget *node, unsigned int flag, struct gui_coordinate *coor);

/********************************************************
 *	function screen_focus_check
 *		焦点检测函数
 *		
 *
 * ******************************************************/
int screen_focus_check(Screen *screen,unsigned int flag, struct gui_coordinate *coor);

/********************************************************
 *	function	point_in_widget_check_2
 *		检测坐标点是否在控件区域内
 * ******************************************************/
int point_in_widget_check_2(mwidget *m, struct gui_coordinate *coor);

/********************************************************
 *	function screen_register_widget_event_handle
 *		注册控件事件处理函数
 *
 * ******************************************************/
int screen_register_widget_event_handle( int (*widget_event_handle_cbk)(widget_event_t *event,void *param),void *param);

int screen_unreister_widget_event_cbk(screen_widget_event_cbk_t *cbk);
screen_widget_event_cbk_t *screen_register_widget_event_cbk( int (*widget_event_handle_cbk)(widget_event_t *event,void *param),void *param,screen_event_cbk_priority_e priority);

struct _xmlwidget_widget;
/********************************************************
 *
 * ******************************************************/
int screen_add_xmlwidget(struct _xmlwidget_widget *m);

/********************************************************
 *
 * ******************************************************/
int screen_del_xmlwidget(struct _xmlwidget_widget *m);

/********************************************************
 *	function	KeyboardOpen
 *		启动键盘
 *
 * ******************************************************/
int KeyboardOpen(char *name);

/********************************************************
 *	function	KeyboardClose
 *		关闭键盘
 * ******************************************************/
int KeyboardClose(void);

/********************************************************
 *	function	ScreenKeyboardMsgSend
 *		发送键盘事件到screen
 *
 * ******************************************************/
int ScreenKeyboardMsgSend(char *key);

/********************************************************
 *	function	ScreenKeyboardFocusSet
 *		设置屏幕键盘事件焦点控件
 *
 * ******************************************************/
int ScreenKeyboardFocusSet(Screen *screen, mwidget *m);

#endif
