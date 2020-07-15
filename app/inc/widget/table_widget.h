#ifndef __TABLE_WIDGET_H__
#define __TABLE_WIDGET_H__

#include "widget.h"
#include "screen.h"

struct _table_widget;

#define TABLE_MAX_INDEX_COUNT (65535)

enum {
	TABLE_FOCUS_MODE_NONE = 0,
	TABLE_FOCUS_MODE_BK_BTIMAP,
	TABLE_FOCUS_MODE_RECTANGLE,
	TABLE_FOCUS_MODE_ALPHA,
}TableFocusMode;

enum{
	TABLE_MOVE_RULE_EXCHANGE = 0,
	TABLE_MOVE_RULE_ARRAY,
}TableMoveRule;

enum{
	TABLE_INDEX_LIST_UNINIT = 0,
	TABLE_INDEX_LIST_INIT,
}TableIndexListInitStatus;


typedef struct tableData{
	unsigned char  line_count; \
	unsigned char  column_count; \
	unsigned char  cur_table_screen; \
	unsigned char  table_index_list_init_flag; \
	
	int  focus_index; \
	int  table_index_list[TABLE_MAX_INDEX_COUNT]; \

	int  table_focus_node_abs_x; 
	int  table_focus_node_abs_y; 
	int  coor_x; 
	int  coor_y;
	int  table_move_rule;
}TableData;



#define _TABLE_CLASS(clas_name) \
	_WIDGET_CLASS(clas_name) \
	TableData data; \
	HDC hdc; \


#define _table_EVENT_LIST \
	_widget_EVENT_LIST

#define _table_PROPERTYS \
	{"layout",TableLayoutSet,TableLayoutGet}, \
	_widget_PROPERTYS
	

#define _TABLE_OPT(parent) \
	_WIDGET_OPT(parent) \
	void (*SetLayout) (struct _text_widget *m, int line_count, int column_count);
	
#define _table_OPT_BEGIN(clas_name,parent) \
	_WIDGET_OPT_BEGIN(clas_name,parent) \
	.paint = TablePaint, \
	.get_focus = TableGetFocus, \
	.touch_event_handle = TableTouchEventHandle, \
	.SetLayout = TableSetLayout, \


#define _table_CREATE(clas_name,_name,x,y,w,h) \
	_WIDGET_CREATE(clas_name,_name,x,y,w,h)  \
	m->data.focus_index = -1; \
	m->child_paint_flag = 1; \
	m->data.table_index_list_init_flag = TABLE_INDEX_LIST_UNINIT; \
	m->data.table_move_rule = TABLE_MOVE_RULE_EXCHANGE;

	

typedef struct _table_opt{
	_TABLE_OPT(widget)
}table_opt;

typedef struct _table_widget{
	_TABLE_CLASS(table);
}mtable;

mtable *create_table_widget(char *name, int x, int y, int w, int h);

#endif

