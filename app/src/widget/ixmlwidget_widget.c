
#include "ixmlwidget_widget.h"
#include "screen.h"
#include "page_event.h"
#include "widget_manager.h"

int ixmlwidget_paint(mwidget *node, HDC hdc)
{
	mixmlwidget *m = (mixmlwidget*)node;

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

void ixmlwidget_destroy(mwidget *node, widget_opt *clas)
{
	mixmlwidget *m = (mixmlwidget*)node;
	if(m->hdc){
		screen_destroy_hdc(m->hdc);
		m->hdc = NULL;
	}

	if(m->widget_tree){
		m->widget_tree->opt->destroy(m->widget_tree,m->widget_tree->opt);
		m->widget_tree = NULL;
	}

	if(m->event_list){
		page_event_t *event = orb_list_del_first(m->event_list);
		while(event){
			page_event_destroy(event);
			event = orb_list_del_first(m->event_list);
		}
		orb_list_destroy(m->event_list);
	}

	if(m->script){
		script_destroy(m->script);
		m->script = NULL;
	}


	m->xmlwidget->opt->lock_data((mwidget*)m->xmlwidget);
	m->xmlwidget->used_num--;
	m->xmlwidget->opt->unlock_data((mwidget*)m->xmlwidget);

	clas->parent->destroy(node,clas->parent);
}

static int widget_event_cbk(widget_event_t *event, void *param)
{
	mixmlwidget *m = param;

	page_event_t *page_event = orb_list_get_first(m->event_list);
	while(page_event){
		if(page_event->widget == event->hd && strcmp(page_event->name,event->event) == 0){
			break;
		}
		page_event = orb_list_get_next(m->event_list);
	}

	if(page_event == NULL){
		return -1;
	}

	//show_script(m->script);
	exp_node_t *result = expression_calc(page_event->exp,m->script->code_stack);
	if(result == NULL){
		GUI_ERR("expression calc fail");
		return -1;
	}

	exp_node_destroy(result);

	return 0;
}

int IxmlwidgetStart(mwidget *node)
{
	mixmlwidget *m = (mixmlwidget*)node;
	if(node->screen == NULL){
		return 0;
	}

	m->widget_tree->deep = m->deep + 1;
	screen_start_node(m->widget_tree,m->screen);

	m->event_cbk = screen_register_widget_event_cbk(widget_event_cbk,m,SCREEN_EVENT_CBK_PRIORITY_HIGH);
	
	//gui_node_show(m->widget_tree);

	return 0;
}

int IxmlwidgetStop(mwidget *node)
{
	mixmlwidget *m = (mixmlwidget*)node;

	if(node->screen == NULL){
		return 0;
	}

	screen_unreister_widget_event_cbk(m->event_cbk);
	m->event_cbk = NULL;
	screen_stop_node(m->widget_tree,node->screen);
	
	return 0;
}

mwidget *IxmlwidgetGetFocus(mwidget *node, unsigned int flag, struct gui_coordinate *coor)
{
	mixmlwidget *m = (mixmlwidget*)node;

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

int IxmlwidgetTouchEventHandle(mwidget *node, unsigned int flag, struct move_info *mv_info)
{
	mixmlwidget *m = (mixmlwidget*)node;
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

int IxmlwidgetActiveEvent(mwidget *node, char *event)
{
	if(node == NULL || event == NULL){
		return -1;
	}

	mixmlwidget *m = (mixmlwidget*)node;

	if(m->opt->parent->activate_event(node,event) == 0){
		return 0;
	}

	struct user_event *user = m->xmlwidget->user_event;
	int i = 0;
	while(user){
		if(strcmp(user->name,event) == 0){
			m->user_event_flag |= 1<<i;
			return 0;
		}
		user = user->next;
	}

	return -1;
}

_WIDGET_GENERATE(ixmlwidget,widget)
