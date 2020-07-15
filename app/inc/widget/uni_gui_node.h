#ifndef __UNI_GUI_NODE_H__
#define __UNI_GUI_NODE_H__
#include <minigui/common.h>
#include <minigui/minigui.h>
#include <minigui/gdi.h>
#include <minigui/window.h>
#include <minigui/control.h>

#include "widget.h"

#define POINT_RANGE_CHECK(c_x,c_y,x,y,range) ((c_x <= x+range)&&(c_x >= x-range) && (c_y <= y+range)&&(c_y >= y-range))

#define POINT_IN_RANGE(cx,cy,x,y,w,h) (cx > x && cx < x + w && cy > y && cy < y+h)


typedef int(*SetBkpho)(void *,char*);

typedef int(*SetBackground)(void *,char,void *);


int gui_node_paint_enable(uni_gui_node *node,int count);
int gui_node_paint(uni_gui_node *node,HDC hdc);


uni_gui_node *create_gui_node(char *name,int x, int y, int w, int h);
int gui_node_set_location_type(uni_gui_node *node,enum location_type type);
//int gui_node_init_to_text(uni_gui_node *node, char *text, gal_pixel color);
//int gui_node_init_to_text(uni_gui_node *node, textData *textDa);
uni_gui_node *gui_node_reference_text(uni_gui_node *node,char *name,int x,int y,int w,int h);

int gui_node_init_to_jpeg(uni_gui_node *node,char *path);
uni_gui_node *gui_node_reference_jpeg(uni_gui_node *node,char *name,int x,int y,int w,int h);

int gui_node_init_to_box(uni_gui_node *node);
int gui_node_init_to_inputbox(uni_gui_node *node,const char *tips);
int gui_node_init_to_slide(uni_gui_node *node);
int gui_node_init_to_button(uni_gui_node *node,char *up_path,char *down_path);
uni_gui_node *gui_node_reference_button(uni_gui_node *node,char *name, int x,int y);
int gui_node_init_to_anim(uni_gui_node *node, char *path);
int gui_node_init_to_gif(uni_gui_node *node,char *path);


void uni_gui_node_delete(uni_gui_node *node);

int gui_node_add_child_node_to_end(void *childnode, void *parent);
int gui_node_add_child_node_to_start(uni_gui_node *childnode, uni_gui_node *parent);
uni_gui_node *gui_node_get_node_by_name(uni_gui_node *node,const char *name);
uni_gui_node *gui_node_get_prev_node(uni_gui_node *node);

uni_gui_node *gui_node_reference_text_new(uni_gui_node *node,char *name,char *text,int x,int y);

void gui_node_show(uni_gui_node *node);
void uni_gui_node_destroy(uni_gui_node *node);
void gui_node_enter_destroy_queue(uni_gui_node *node);

void gui_node_data_lock(uni_gui_node *node);
void gui_node_data_unlock(uni_gui_node *node);
int gui_node_get_focus(uni_gui_node *node);
int gui_node_release_focus(uni_gui_node *node);
int hide_or_show_node_func(uni_gui_node *node,int hide_id);

int gui_node_buttondown_handle(uni_gui_node *node, void *param);
int gui_node_buttonup_handle(uni_gui_node *node, void *param);
int gui_node_click_handle(uni_gui_node *node, void *param);
int gui_node_move_handle(uni_gui_node *node, void *param);
int gui_node_stop_handle(uni_gui_node *node, void *param);
int gui_node_location_get(uni_gui_node *node);
#endif
