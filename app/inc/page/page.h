#ifndef __PAGE_H__
#define __PAGE_H__

#include "widget.h"
#include "platform.h"
#include "orb_queue.h"
#include "orb_list.h"
#include "script.h"
#include "gui_memory.h"


typedef struct page_event{
	char		*name;
	mwidget		*widget;
	exp_node_t	*exp;
}page_event_t;


/********************************************************
 *	Data page_card_t
 *		卡片结构体. 相同模块。
 *
 * ******************************************************/
typedef struct _page_card{
	char	*name;
	mwidget *widget_tree;
}page_card_t;

typedef struct _page{
	char		name[32];			// 页面名称
	int			id;					// 页面ID
	int			lable_index;		// 用于生成控件名称
	mwidget		*widget_tree;		// 页面显示控件树

	orb_queue		*event_queue;		// 事件队列
	orb_thread_t	event_loop_thread;	// 事件循环线程句柄

	script_t			*script;		// 页面脚本

	orb_list_t			*event_list;		// 页面事件列表。 元素类型为 page_event_t

	orb_list_t			*card_list;			// 卡片列表

	orb_list_t			*modes;				// 模板列表

	orb_list_t			*label_props;		// 标签控件属性列表，在页面启动时设置。
}page_t;

page_card_t *page_card_create(char *name);
page_card_t *page_card_find(page_t *page,char *name);
void page_card_destroy(page_card_t *card);

page_t *gui_page_create(char *name);
int gui_page_destroy(page_t *page);

void page_show(page_t *page);

int gui_page_start(page_t *page);

int gui_page_stop(page_t *page);
#endif

