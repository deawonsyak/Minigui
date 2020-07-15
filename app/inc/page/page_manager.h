#ifndef __PAGE_MANAGER_H_
#define __PAGE_MANAGER_H_

#include "page.h"
#include "orb_list.h"
#include "orb_queue.h"
#include "platform.h"
#include "orb_list.h"
#include "orb_stack.h"

typedef enum {
	GUI_EVENT_JUMP_TO_MAIN,
	GUI_EVENT_JUMP_TO_PAGE,
	GUI_EVENT_JUMP_TO_NEXT,
	GUI_EVENT_CLOSE_CURRENT_PAGE,
	GUI_EVENT_BACKUP_PAGE,
	GUI_EVENT_PAGE_EVENT,
	GUI_EVENT_WIDGET_EVENT,

}gui_event_e;

typedef struct manager_event{
	gui_event_e		event;
	int				session_id;
	int				serial;
	unsigned int	time;
	union{
		widget_event_t	*widget_event;
		char			*value;
	};
}gui_event_t;

typedef int (*gui_event_handle)(gui_event_t *event, void *param);

struct gui_event_handle_table{
	gui_event_e			event;
	gui_event_handle	handle;
	void				*param;
};


typedef struct _page_manager{
	unsigned int	page_index;			// 用于生成pageID
	page_t			*c_page;			// 当前活动页面
	page_t			*main;				// 主页面
	orb_stack_t		*page_stack;		// 页面栈
	orb_list_t		*pages;				// 所有页面链表, 保存了所有的页面

	orb_queue		*event_queue;		// 事件队列
	orb_thread_t	event_loop_thread;

	orb_list_t		*mode_list;			// 模板列表

	orb_list_t		*card_list;			// 卡片列表

	struct gui_event_handle_table *event_table;

}page_manager_t;

int page_manager_jump_to_main(void);
int page_manager_send_event(gui_event_e event, int session_id,int serial, void *value);

void gui_event_destroy(gui_event_t *event);

page_t *page_manager_get_page_by_name(char *name);
page_t *page_manager_get_current_page(void);

void page_manager_show_all_page(void);
#endif
