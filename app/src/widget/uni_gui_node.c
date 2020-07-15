#include "uni_gui_node.h"
#include "screen.h"
#include "orb_stack.h"
#include "ixmlwidget_widget.h"

struct node_del_list *del_list = NULL;

int gui_node_location_get(mwidget *node)
{
	if(node == NULL){
		return -1;
	}
	mwidget *prev;

	char type = node->location_type;
	if(type == GUI_LOCATION_TYPE_PREV_BROTHERS){
		prev = gui_node_get_prev_node(node);
		if(prev == NULL){
			type = GUI_LOCATION_TYPE_PARENT;
		}

	}

	switch(type){
		case GUI_LOCATION_TYPE_PARENT:
			if(node->parent == NULL ){
				node->start_abs_x = node->x;
				node->start_abs_y = node->y;
				node->show_w = node->w;
				node->show_h = node->h;
				node->show_abs_x = node->x;
				node->show_abs_y = node->y;
			}else if(node->parent->child_paint_flag == 1){	// 
				node->start_abs_x = node->x;
				node->start_abs_y = node->y;
				node->show_w = node->w;
				node->show_h = node->h;
				node->show_abs_x = node->x;
				node->show_abs_y = node->y;

			}else{
				
				node->start_abs_x = node->parent->start_abs_x + node->x;	
				node->start_abs_y = node->parent->start_abs_y + node->y;
				if(node->start_abs_x + node->w <= node->parent->show_abs_x || node->start_abs_x >= node->parent->show_abs_x + node->parent->show_w){
					node->show_abs_x = -1;
					node->show_abs_y = -1;
					node->show_w = 0;
					node->show_h = 0;
					return 0;
				}else if(node->start_abs_x + node->w > node->parent->show_abs_x && node->start_abs_x + node->w < node->parent->show_abs_x + node->parent->show_w){	
					if(node->start_abs_x >= node->parent->show_abs_x){
						node->show_w = node->w;
					}else{
						node->show_w = node->start_abs_x + node->w - node->parent->show_abs_x;
					}
				}else{
					if(node->start_abs_x >= node->parent->show_abs_x){
						node->show_w = node->parent->show_abs_x + node->parent->show_w - node->start_abs_x;
					}else{
						node->show_w = node->parent->show_w;
					}
				}

				if(node->start_abs_y + node->h <= node->parent->show_abs_y || node->start_abs_y >= node->parent->show_abs_y + node->parent->show_h){
					node->show_abs_x = -1;
					node->show_abs_y = -1;
					node->show_w = 0;
					node->show_h = 0;
					return 0;
				}else if(node->start_abs_y + node->h > node->parent->show_abs_y && node->start_abs_y + node->h < node->parent->show_abs_y + node->parent->show_h){
					if(node->start_abs_y >= node->parent->show_abs_y){
						node->show_h = node->h;
					}else{
						node->show_h = node->start_abs_y + node->h - node->parent->show_abs_y;
					}
				}else{
					if(node->start_abs_y >= node->parent->show_abs_y){
						node->show_h = node->parent->show_abs_y + node->parent->show_h - node->start_abs_y;
					}else{
						node->show_h =node->parent->show_h;
					}
				}

				node->show_abs_x = node->parent->show_abs_x > node->start_abs_x ? node->parent->show_abs_x: node->start_abs_x;
				node->show_abs_y = node->parent->show_abs_y > node->start_abs_y ? node->parent->show_abs_y: node->start_abs_y;
			}
			break;
		case GUI_LOCATION_TYPE_PREV_BROTHERS:
			if(prev != NULL){
				node->start_abs_x = prev->start_abs_x + node->x;
				node->start_abs_y = prev->start_abs_y + node->y;
			}

			if(node->parent == NULL){
				node->start_abs_x = node->x;
				node->start_abs_y = node->y;
				node->show_w = node->w;
				node->show_h = node->h;
				node->show_abs_x = node->x;
				node->show_abs_y = node->y;
			}else{	
				if(prev == NULL){
					node->start_abs_x = node->parent->start_abs_x + node->x;
					node->start_abs_y = node->parent->start_abs_y + node->y;
				}

				if(node->start_abs_x + node->w <= node->parent->show_abs_x || node->start_abs_x >= node->parent->show_abs_x + node->parent->show_w){
					//					printf("[%s %d], parentX:[%d], parentY: [%d] parentshowW: [%d], parentshowh: [%d]\n",__FILE__,__LINE__, node->parent->start_abs_x, node->parent->start_abs_y, node->parent->show_w, node->parent->show_h);
					//					printf("[%s %d], nodeX:[%d], nodeY: [%d] nodeW: [%d], nodeH: [%d]\n",__FILE__,__LINE__, node->x, node->y, node->w, node->h);
					node->show_abs_x = -1;
					node->show_abs_y = -1;
					node->show_w = 0;
					node->show_h = 0;
					return 0;
				}else if(node->start_abs_x + node->w > node->parent->show_abs_x && node->start_abs_x + node->w < node->parent->show_abs_x + node->parent->show_w){	
					if(node->start_abs_x >= node->parent->show_abs_x){
						node->show_w = node->w;
					}else{
						node->show_w = node->start_abs_x + node->w - node->parent->show_abs_x;
					}
				}else{
					if(node->start_abs_x >= node->parent->show_abs_x){
						node->show_w = node->parent->show_abs_x + node->parent->show_w - node->start_abs_x;
					}else{
						node->show_w = node->parent->show_w;
					}
				}

				if(node->start_abs_y + node->h <= node->parent->show_abs_y || node->start_abs_y >= node->parent->show_abs_y + node->parent->show_h){
					node->show_abs_x = -1;
					node->show_abs_y = -1;
					node->show_w = 0;
					node->show_h = 0;
					//					printf("[%s %d], stX:[%d], noW: [%d] PabX: [%d], PH: [%d]\n",__FILE__,__LINE__, node->start_abs_y, node->h, node->parent->show_abs_y, node->parent->show_h);
					return 0;
				}else if(node->start_abs_y + node->h > node->parent->show_abs_y && node->start_abs_y + node->h < node->parent->show_abs_y + node->parent->show_h){
					if(node->start_abs_y >= node->parent->show_abs_y){
						node->show_h = node->h;
					}else{
						node->show_h = node->start_abs_y + node->h - node->parent->show_abs_y;
					}
				}else{
					if(node->start_abs_y >= node->parent->show_abs_y){
						node->show_h = node->parent->show_abs_y + node->parent->show_h - node->start_abs_y;
					}else{
						node->show_h =node->parent->show_h;
					}
				}

				node->show_abs_x = node->parent->show_abs_x > node->start_abs_x ? node->parent->show_abs_x: node->start_abs_x;
				node->show_abs_y = node->parent->show_abs_y > node->start_abs_y ? node->parent->show_abs_y: node->start_abs_y;
				//				printf("[%s %d], PX:[%d], PY: [%d] PW: [%d], PH: [%d]\n",__FILE__,__LINE__, node->parent->x, node->parent->y, node->parent->w, node->parent->h);
			}
			break;
		case GUI_LOCATION_TYPE_ABSOLUTE:
			break;

	}

	//printf("[%s %d] node:%s type:%d show: x:%d y:%d w:%d h:%d\n",__func__,__LINE__,node->name,node->location_type,node->show_abs_x,node->show_abs_y,node->show_w,node->show_h);
	return 0;

}

int gui_node_paint_enable(mwidget *node,int count)
{
	Screen *screen =  &g_screen;
	if(screen->timer_count < count || count == -1){
		screen->timer_count = count;
	}

	node->need_paint = 1;

	if( !IsTimerInstalled(screen->hwnd,SCREEN_TIMER_ID)){
		SetTimer(screen->hwnd,SCREEN_TIMER_ID,SCREEN_TIMER_FREQ);
	}

	return 0;
}

/********************************************************
 *	倒序检测
 * ******************************************************/
int point_in_widget_check_2(mwidget *m, struct gui_coordinate *coor)
{
	if(m == NULL || coor == NULL){
		return 0;
	}

	if(m->hide){
		return 0;
	}

	mwidget *parent = m->parent;
	if(parent){
		if(!point_in_widget_check_2(parent,coor)){
			return 0;
		}
	}

	if(POINT_IN_RANGE(coor->x,coor->y,m->show_abs_x,m->show_abs_y,m->show_w,m->show_h)){
		if(m->child_paint_flag){
			coor->x = coor->x - m->show_abs_x;
			coor->y = coor->y - m->show_abs_y;
		}
		return 1;	
	}

	return 0;
}

/********************************************************
 *	顺序检测
 * ******************************************************/
static int point_in_widget_check(mwidget *m, struct gui_coordinate *coor)
{
	int cx = coor->x;
	int cy = coor->y;

	if(m == NULL || coor == NULL){
		return 0;
	}

	if(m->hide){
		return 0;
	}

	return POINT_IN_RANGE(cx,cy,m->show_abs_x,m->show_abs_y,m->show_w,m->show_h);
}

void node_push_stack(mwidget* node, void* param, orb_stack_t* node_stack)
{
	if (!node)
		return ;

	struct gui_coordinate *coor = (struct gui_coordinate*)param;
	mwidget *child = node->child;
	mwidget *next = node->next;

	if(point_in_widget_check(node,coor)){
		orb_stack_Push(node_stack,node);
		if(child){
			if(node->child_paint_flag){
				struct gui_coordinate cr;
				cr.x = coor->x - node->show_abs_x;
				cr.y = coor->y - node->show_abs_y;
				node_push_stack(child, &cr, node_stack);
			}else{
				node_push_stack(child, param, node_stack);
			}
		}
	}

	if (next) {
		node_push_stack(next, param, node_stack);
	}
}

mwidget *widget_tree_focus_check(mwidget *node, unsigned int flag, struct gui_coordinate *coor)
{
	orb_stack_t*	node_stack = NULL;
	mwidget*	ack_node = NULL;
	mwidget*	focus_node = NULL;

	node_stack = orb_stack_create();

	node_push_stack(node, coor, node_stack);

	while(1){
		ack_node = (mwidget*)orb_stack_Pop(node_stack);
		if(ack_node == NULL){
			break;
		}

		focus_node = ack_node->opt->get_focus(ack_node,flag,coor);
		if(focus_node){
			break;
		}
	}

	while(orb_stack_Top(node_stack)){
		orb_stack_Pop(node_stack);
	}

	//销毁栈
	orb_stack_destroy(node_stack);

	return focus_node;

}

int screen_focus_check(Screen *screen,unsigned int flag, struct gui_coordinate *coor)
{
	if (!screen){
		return -1;
	}

	mwidget	*focus_node = widget_tree_focus_check(screen->suspention,flag,coor);

	if(focus_node == NULL){
		focus_node = widget_tree_focus_check(screen->tool_layer,flag,coor);
		if(focus_node == NULL){
			focus_node = widget_tree_focus_check(screen->node,flag,coor);
		}
	}

	if(focus_node){
		if(screen->focus_node && focus_node != screen->focus_node){
			// 焦点控件被锁定 只能由控件自己释放焦点
			if(screen->focus_node->focus_lock == 1){	
				return 0;
			}
			// 释放当前屏幕焦点
			screen->focus_node->opt->release_focus(screen->focus_node);
			screen->focus_node = NULL;
		}
		screen->focus_node = focus_node;
		focus_node->focus = 1;
	}

	return 0;
}

int ScreenKeyboardFocusSet(Screen *screen, mwidget *m)
{
	if(screen == NULL){
		return -1;
	}

	if(screen->keyboard_focus_node == m){
		return 0;
	}

	if(screen->keyboard_focus_node){
		if(screen->keyboard_focus_node->opt->release_keyboard_focus)
			screen->keyboard_focus_node->opt->release_keyboard_focus(screen->keyboard_focus_node);
		screen->keyboard_focus_node = NULL;
	}

	screen->keyboard_focus_node = m;

	return 0;

}


/********************************************************
 *	function _node_paint
 *		递归调用 paint 函数
 *
 * ******************************************************/
static int _node_paint(mwidget *node,HDC hdc)
{
	if(node == NULL){
		return -1;
	}
	mwidget *child = node->child;
	mwidget *next = node->next;


	if(child != NULL && node->hide == 0 && node->child_paint_flag == 0){
		gui_node_paint(child,hdc);
	}
	if(next != NULL ){
		gui_node_paint(next,hdc);
	}

	return 0;
}

int gui_node_paint(mwidget *node,HDC hdc)
{
	if(node == NULL){
		GUI_ERR("node is NULL");
		return -1;
	}
	pthread_mutex_lock(&node->node_mutex);
	node->opt->get_location(node);
	if(node->opt->paint != NULL && node->hide == 0){
		node->opt->paint(node,hdc);
	}
	_node_paint(node,hdc);

	pthread_mutex_unlock(&node->node_mutex);

	return 0;
}

void gui_node_data_lock(mwidget *node)
{
	if(node == NULL){
		return;
	}
	pthread_mutex_lock(&node->data_mutex);
}
void gui_node_data_unlock(mwidget *node)
{
	if(node == NULL){
		return;
	}
	pthread_mutex_unlock(&node->data_mutex);
}

int gui_node_get_focus(mwidget *node)
{
	if(node == NULL || node->screen == NULL){
		return -1;
	}
	Screen *screen = (Screen *)node->screen;
	if(screen->focus == 1){
		return -1;
	}
	screen->focus = 1;
	node->focus = 1;
	return 0;
}

int gui_node_release_focus(mwidget *node)
{
	if(node == NULL || node->screen == NULL){
		return -1;
	}
	Screen *screen = (Screen *)node->screen;
	if(node->focus == 0){
		return -1;
	}
	screen->focus = 0;
	node->focus = 0;
	GUI_DEBUG("screen focus: [%d], node focus: [%d]\n",screen->focus, node->focus);
	return 0;
}

int gui_node_set_location_type(mwidget *node,enum location_type type)
{
	node->location_type = type;
	return 0;
}

void updata_node_deep(mwidget *node)
{
	mwidget *next,*child;
	next = node->next;
	child = node->child;


	if(next != NULL){
		next->deep = node->deep;
		updata_node_deep(next);
	}

	if(child != NULL){
		child->deep = node->deep;
		updata_node_deep(child);
	}
}

/********************************************************
 *	添加节点到parent的child节点末尾
 *
 * ******************************************************/
int gui_node_add_child_node_to_end(void *_childnode, void *_parent)
{
	if(_childnode == NULL || _parent == NULL){
		return -1;
	}

	mwidget *parent = _parent;
	mwidget *childnode = _childnode;

	mwidget *node;
	pthread_mutex_lock(&parent->node_mutex);
	if(parent->child == NULL){
		parent->child = childnode;
		pthread_mutex_unlock(&parent->node_mutex);
		goto exit;
	}

	node = parent->child;
	while(node->next != NULL){
		node = node->next;
	}
	node->next = childnode;
	pthread_mutex_unlock(&parent->node_mutex);

exit:
	pthread_mutex_lock(&childnode->node_mutex);
	childnode->parent = parent;
	childnode->screen = parent->screen;
	childnode->deep = parent->deep + 1;
	updata_node_deep(childnode);
	if(childnode->screen != NULL){
		screen_start_node(childnode,childnode->screen);
	}
	pthread_mutex_unlock(&childnode->node_mutex);
	return 0;
}

int gui_node_add_child_node_to_start(mwidget *childnode, mwidget *parent)
{
	if(childnode == NULL || parent == NULL){
		return -1;
	}

	mwidget *node;
	mwidget tmpnode;

	pthread_mutex_lock(&parent->node_mutex);

	if(parent->child == NULL){
		parent->child = childnode;
		goto exit;
	}

	memset(&tmpnode, 0, sizeof(mwidget));
	node = parent->child;
#if 1
	tmpnode.x = node->x;
	tmpnode.y = node->y;
	tmpnode.location_type = node->location_type;

	node->x = childnode->x;
	node->y = childnode->y;
	node->location_type = childnode->location_type;

	childnode->x = tmpnode.x;
	childnode->y = tmpnode.y;
	childnode->location_type = tmpnode.location_type;
#endif

	parent->child = childnode;
	childnode->next = node;

exit:
	childnode->parent = parent;
	childnode->screen = parent->screen;
	childnode->deep = parent->deep + 1;
	updata_node_deep(childnode);
	if(childnode->screen != NULL){
		//		printf("[%s %d]\n",__func__,__LINE__);
		screen_start_node(childnode,childnode->screen);
	}
	//pthread_mutex_unlock(&childnode->node_mutex);
	pthread_mutex_unlock(&parent->node_mutex);

	return 0;
}

mwidget *gui_node_get_node_by_name(mwidget *node,const char *name)
{

	if(node == NULL){
		GUI_ERR("Param error");
		return NULL;
	}
	mwidget *m = node->opt->get_widget(node,(char*)name);
	if(m){
		return m;
	}

	mwidget *child,*next;
	child = node->child;
	if(child != NULL){
		m = gui_node_get_node_by_name(child,name);
		if(m){
			return m;
		}
	}

	next = node->next;
	if(next != NULL){
		return gui_node_get_node_by_name(next,name);
	}

	return NULL;
}


mwidget *gui_node_get_prev_node(mwidget *node)
{
	mwidget *prev = NULL,*parent = NULL,*head = NULL;
	Screen *screen;

	parent = node->parent;
	if(parent == NULL){
		screen = uni_get_gui_node_screen(node);
		if(screen){
			head = screen->node;
		}else{
			return NULL;
		}
	}else{
		head = parent->child;
		if(head == NULL){
			return NULL;
		}
	}

	if(head == node){
		return NULL;
	}

	prev = head;
	while( prev->next != NULL){
		if(prev->next == node){
			return prev;
		}
		prev = prev->next;
	}
	return NULL;

}

static void gui_show(mwidget *node)
{
	if(node == NULL){
		printf(" Node is NULL\n");
		return;
	}

	for(int i = 0; i < node->deep; i++){
		printf("  ");
	}
	if(strcmp(node->opt->class_name,"ixmlwidget") == 0){
		mixmlwidget *m = (mixmlwidget*)node;
		printf("%s name: %s deep:%d absx:%d absy:%d x: %d y: %d w:%d h:%d, showX:%d, showY:%d, showW:%d,showH:%d\n",m->xmlwidget->name,node->name,node->deep,
				node->start_abs_x,node->start_abs_y,node->x, node->y, node->w,node->h, node->show_abs_x, node->show_abs_y,node->show_w,node->show_h);
		gui_show(m->widget_tree);
		
	}else{
		printf("%s name: %s deep:%d absx:%d absy:%d x: %d y: %d w:%d h:%d, showX:%d, showY:%d, showW:%d,showH:%d\n",node->opt->class_name,node->name,node->deep,
				node->start_abs_x,node->start_abs_y,node->x, node->y, node->w,node->h, node->show_abs_x, node->show_abs_y,node->show_w,node->show_h);
	}
	mwidget *child,*next;
	child = node->child;
	if(child != NULL){
		gui_show(child);
	}

	next = node->next;
	if(next != NULL){
		gui_show(next);
	}
}


void gui_node_show(mwidget *node)
{
	if(node == NULL){
		return;
	}
	printf("Gui Node Show:\n");
	gui_show(node);
}


