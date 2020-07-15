#include "icard_widget.h"
#include "screen.h"


int icard_paint(mwidget *node,HDC hdc)
{
	micard *m = (micard*)node;

	if(node->need_paint || m->hdc == NULL){
		if(m->hdc){
			screen_destroy_hdc(m->hdc);
		}
		m->hdc = screen_create_hdc(node->w,node->h);
		node->need_paint = 0;
		if(m->widget_tree){
			gui_node_paint(m->widget_tree,m->hdc);
		}
	}

	if(node->show_abs_x >= 0){
		int sx = node->show_abs_x - node->start_abs_x;
		int sy = node->show_abs_y - node->start_abs_y;

		BitBlt(m->hdc,sx,sy,node->show_w,node->show_h,hdc,node->show_abs_x,node->show_abs_y,0);
	}

	return 0;
}

void icard_destroy(mwidget *node,widget_opt *clas)
{
	micard *m = (micard*)node;
	screen_destroy_hdc(m->hdc);
	m->widget_tree = NULL;
	clas->parent->destroy(node,clas->parent);
}

int icard_start(mwidget *node,void *param)
{
	micard *m = (micard*)node;

	if(node->screen == NULL){
		return 0;
	}

	screen_start_node(m->widget_tree,node->screen);
	return 0;
}

int icard_stop(mwidget *node,void *param)
{
	micard *m = (micard*)node;

	if(node->screen == NULL){
		return 0;
	}

	screen_stop_node(m->widget_tree,node->screen);
	return 0;
}

mwidget *icard_get_widget(mwidget *node, char *name)
{
	micard *m = (micard*)node;
	mwidget *c = gui_node_get_node_by_name(m->widget_tree,name);
	if(c){
		return c;
	}
	return m->opt->parent->get_widget(node,name);
}

int icard_set_widget_tree(micard *m, mwidget *tree)
{
	if(m == NULL){
		return -1;
	}

	m->opt->lock_data(m);
	m->child = tree;
	m->opt->unlock_data(m);
	m->opt->refresh(m);
	return 0;
}

mwidget *icard_get_focus(mwidget *node, unsigned int flag, struct gui_coordinate *coor)
{
	micard *m = (micard*)node;

	struct gui_coordinate cr;
	cr.x = coor->x - node->show_abs_x;
	cr.y = coor->y - node->show_abs_y;

	if(m->widget_tree){
		mwidget *focus = widget_tree_focus_check(m->widget_tree,flag,&cr);
		if(focus){
			m->focus_node = focus;
			return node;
		}
	}

	m->opt->parent->get_focus(node,flag,coor);

	return NULL;
}

int icard_touch_event_handle(mwidget *node, unsigned int flag, struct move_info *mv_info)
{
	micard *m = (micard*)node;
	int ret = 0;

	struct move_info mv = *mv_info;
	mv.coor.x = mv_info->coor.x - node->show_abs_x;
	mv.coor.y = mv_info->coor.y - node->show_abs_y;

	if(m->focus_node){
		ret = m->focus_node->opt->touch_event_handle(m->focus_node,flag,&mv);
		if(ret){
			return ret;
		}
	}

	ret |= m->opt->parent->touch_event_handle(node,flag,mv_info);

	return ret;
}


_WIDGET_GENERATE(icard,widget)

