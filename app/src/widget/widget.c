
#include "widget.h"
#include "screen.h"
#include "gui_memory.h"
#include "ixmlwidget_widget.h"

void widget_event_destroy(widget_event_t *event)
{
	if(event == NULL){
		return;
	}

	widget_free(event);
}

widget_event_t *widget_event_create(char *event, void *hd)
{
	widget_event_t *wevent = widget_calloc(1,sizeof(widget_event_t) + strlen(event)+1);
	if(wevent == NULL){
		return NULL;
	}
	wevent->event = (char*)wevent + sizeof(widget_event_t);
	strcpy(wevent->event,event);
	wevent->hd = hd;
	return wevent;
}

int widget_event_send(mwidget *m, char *event)
{
	if(event == NULL || m == NULL){
		return -1;
	}

	if(m->screen == NULL){
		return -2;
	}

	widget_event_t *wevent = widget_event_create(event,m);

	int ret = m->screen->widget_event_queue->put(m->screen->widget_event_queue,wevent,100);
	if(ret){
		widget_event_destroy(wevent);
		return -3;
	}
	
	return 0;
}

int widget_hide(mwidget *node,int enable)
{
	if(node == NULL){
		return -1;
	}
	node->hide = enable;

	return 0;
}

void widget_destroy(mwidget *node, widget_opt *clas)
{
	if(node == NULL || node->screen){
		return;
	}

	pthread_mutex_lock(&node->node_mutex);
	mwidget *tmp = NULL;
	if(node->next){
		tmp = node->next;
		tmp->opt->destroy(tmp,tmp->opt);
		node->next = NULL;
	}

	if(node->child){
		tmp = node->child;
		node->child = NULL;
		tmp->opt->destroy(tmp,tmp->opt);
	}

	pthread_mutex_unlock(&node->node_mutex);

	pthread_mutex_destroy(&node->node_mutex);
	pthread_mutex_destroy(&node->data_mutex);
	pthread_mutex_destroy(&node->handle_mutex);

	widget_free(node);
}

void widget_refresh(mwidget *m)
{
	if(m == NULL || m->screen == NULL){
		return;
	}

	mwidget *parent = m->parent;

	while(parent){
		parent->opt->refresh(parent);
		parent = parent->parent;
	}

	gui_node_paint_enable(m,2);
}

int widget_activate_event(mwidget *node, char *event)
{
	int i;
	int ret = -1;
	for(i = 0; i < WIDGET_EVENT_MAX; i++){
		if(node->opt->event_list[i] == NULL){
			break;
		}
		if(strcmp(event,node->opt->event_list[i]) == 0){
			node->event_active_flag |= (1<<i);
			ret = 0;
			break;
		}
	}
	return ret;
}

void widget_release_focus(mwidget *node)
{
	if(node->focus == 0){
		return;
	}

	node->focus = 0;
	node->focus_lock = 0;
	node->screen->focus_node = NULL;
}

unsigned int widget_event_flag_get(mwidget *node, char *event)
{
	int i;
	for(i = 0; i < WIDGET_EVENT_MAX; i++){
		if(node->opt->event_list[i] == NULL){
			return 0;
		}
		if(strcmp(event,node->opt->event_list[i]) == 0){
			unsigned int flag = 1 << i;
			return flag;
		}
	}

	return 0;
}

mwidget *widget_get_widget(mwidget *node, char *name)
{
	if(node == NULL || name == NULL){
		return NULL;
	}

	if(strcmp(node->name,name) == 0){
		return node;
	}

	return NULL;
}

mwidget *widget_get_focus(mwidget *node, unsigned int flag, struct gui_coordinate *coor)
{
	if(node->screen == NULL){		
		return NULL;
	}

	if(flag & SCREEN_FORCE_FLAG_BUTTONDOWN){
		if((node->event_active_flag & widget_event_flag_get(node,"onClick")) ||
			(node->event_active_flag & widget_event_flag_get(node,"onButtonDown")) ||
			(node->event_active_flag & widget_event_flag_get(node,"onLongPress")) ||
			(node->event_active_flag & widget_event_flag_get(node,"onButtonUp"))){
			return node;
		}
	}

	return NULL;
}

int	widget_touch_event_handle(mwidget *node, unsigned int flag, struct move_info *mv_info)
{
	if(node->screen == NULL){
		return 0;
	}

	//printf("node:%s event_active_flag:0x%x\n",node->name,node->event_active_flag);
	if(	(flag & SCREEN_FORCE_FLAG_BUTTONDOWN) && 
		(node->event_active_flag & widget_event_flag_get(node,"onButtonDown"))){
		node->opt->event_send(node,"onButtonDown");
		return 1;
	}

	if(	(flag & SCREEN_FORCE_FLAG_BUTTONUP) && 
		(node->event_active_flag & widget_event_flag_get(node,"onButtonUp"))){
		node->opt->event_send(node,"onButtonUp");
		return 1;
	}

	if(	(flag & SCREEN_FORCE_FLAG_CLICK) && 
		(node->event_active_flag & widget_event_flag_get(node,"onClick"))){
		node->opt->event_send(node,"onClick");
		return 1;
	}

	if(	(flag & SCREEN_FORCE_FLAG_LONGPRESS) && 
		(node->event_active_flag & widget_event_flag_get(node,"onLongPress"))){
		node->opt->event_send(node,"onLongPress");
		return 1;
	}
	return 0;
}

mwidget *widget_keyboard_event_handle(mwidget *node, char *key)
{
	if(node == NULL || key == NULL){
		return NULL;
	}

	mwidget *m = NULL;

	if(node->child){
		m = node->child->opt->keyboard_event_handle(node->child, key);
		if(m){
			return m;
		}
	}

	if(node->next){
		m = node->next->opt->keyboard_event_handle(node->next,key);
		if(m){
			return m;
		}
	}

	return 0;
}

char *widget_property_get(mwidget *node, char *property)
{
	char *buf = NULL;
	int i = 0;
	widget_property_t *prop = NULL;
	while(1){
		if(node->opt->property[i].name == NULL){
			break;
		}
		if(strcmp(node->opt->property[i].name,property) == 0){
			prop = &node->opt->property[i];
			break;
		}
		i++;
	}
	if(prop){
		buf = prop->get(node);
	}else{
		GUI_WARN("%s is not exist in %s", property, node->name);
	}
	return buf;
}

int widget_property_set(mwidget *node, char *property, char *value)
{
	if(node == NULL || property == NULL || value == NULL){
		return -1;	
	}
	GUI_DEBUG("%s %s set property:%s value:%s",node->opt->class_name,node->name,property,value);
	int i = 0;
	widget_property_t *prop = NULL;
	while(1){
		if(node->opt->property[i].name == NULL){
			break;
		}
		if(strcmp(node->opt->property[i].name,property) == 0){
			prop = &node->opt->property[i];
			break;
		}
		i++;
	}

	if(prop){
		prop->set(node,value);
	}else{
		GUI_WARN("%s is not exist in %s", property, node->name);
	}

	return 0;
}

int widget_property_x_set(mwidget *node,char *value)
{
	if(value == NULL || node == NULL){
		GUI_ERR("set x property param error!");
		return -1;
	}

	node->x = atoi(value);
	return 0;
}

char *widget_property_x_get(mwidget *node)
{
	if(node == NULL){
		return NULL;
	}
	char *buf = widget_calloc(1,6);
	sprintf(buf,"%d",node->x);
	
	return buf;
}

int widget_property_y_set(mwidget *node,char *value)
{
	if(value == NULL || node == NULL){
		GUI_ERR("set y property param error!");
		return -1;
	}

	node->y = atoi(value);
	return 0;
}

char *widget_property_y_get(mwidget *node)
{
	if(node == NULL){
		return NULL;
	}
	char *buf = widget_calloc(1,6);
	sprintf(buf,"%d",node->y);
	return buf;
}

int widget_property_w_set(mwidget *node,char *value)
{
	if(value == NULL || node == NULL){
		GUI_ERR("set w property param error!");
		return -1;
	}

	node->w = atoi(value);
	return 0;
}

char *widget_property_w_get(mwidget *node)
{
	if(node == NULL){
		return NULL;
	}
	char *buf = widget_calloc(1,6);
	sprintf(buf,"%d",node->w);
	return buf;
}
int widget_property_h_set(mwidget *node,char *value)
{
	if(value == NULL || node == NULL){
		GUI_ERR("set h property param error!");
		return -1;
	}

	node->h = atoi(value);
	return 0;
}

char *widget_property_h_get(mwidget *node)
{
	if(node == NULL){
		return NULL;
	}
	char *buf = widget_calloc(1,6);
	sprintf(buf,"%d",node->h);
	return buf;
}

int widget_property_rect_set(mwidget *node,char *value)
{
	if(value == NULL || node == NULL){
		GUI_ERR("set rect property param error!");
		return -1;
	}
	
	char *tmp = value;

	int x = atoi(tmp);
	tmp = strchr(tmp,',');
	if(tmp == NULL){
		GUI_ERR("widget rect propert param format error! param:%s",value)
		return -1;
	}
	tmp++;
	int y = atoi(tmp);

	tmp = strchr(tmp,',');
	if(tmp == NULL){
		GUI_ERR("widget rect propert param format error! param:%s",value)
		return -1;
	}
	tmp++;
	int w = atoi(tmp);
	
	tmp = strchr(tmp,',');
	if(tmp == NULL){
		GUI_ERR("widget rect propert param format error! param:%s",value)
		return -1;
	}
	tmp++;
	int h = atoi(tmp);

	node->x = x;
	node->y = y;
	node->w = w;
	node->h = h;
	return 0;
}

char *widget_property_rect_get(mwidget *node)
{
	if(node == NULL){
		return NULL;
	}
	char *buf = widget_calloc(1,64);
	sprintf(buf,"%d,%d,%d,%d",node->x,node->y,node->w,node->h);
	return buf;
}

int widget_property_location_set(mwidget *node,char *value)
{
	if(value == NULL || node == NULL){
		GUI_ERR("set location property param error!");
		return -1;
	}

	if(strcmp(value,"parent") == 0){
		node->location_type = GUI_LOCATION_TYPE_PARENT;
	}else if(strcmp(value,"brother") == 0){
		node->location_type = GUI_LOCATION_TYPE_PREV_BROTHERS;
	}else if(strcmp(value,"absolute") == 0){
		node->location_type = GUI_LOCATION_TYPE_ABSOLUTE;
	}else{
		GUI_ERR("property location set error! unknow type:%s",value);
		return -1;
	}
	return 0;
}

char *widget_property_location_get(mwidget *node)
{
	if(node == NULL){
		return NULL;
	}
	char *buf = widget_calloc(1,10);
	switch(node->location_type){
		case GUI_LOCATION_TYPE_PARENT:
			strcpy(buf,"parent");
			break;
		case GUI_LOCATION_TYPE_PREV_BROTHERS:
			strcpy(buf,"brother");
			break;
		case GUI_LOCATION_TYPE_ABSOLUTE:
			strcpy(buf,"absolute");
			break;
	}

	return buf;
}

int widget_property_hide_set(mwidget *node, char *value)
{
	if(value == NULL || node == NULL){
		GUI_ERR("set location property param error!");
		return -1;
	}

	if(strcmp(value,"hidden") == 0){
		node->hide = 1;
	}else{
		node->hide = 0;
	}

	node->opt->refresh(node);
	return 0;

}

char *widget_property_hide_get(mwidget *node)
{
	if(node == NULL){
		return NULL;
	}

	char *buf = widget_calloc(1,16);
	if(node->hide){
		strcpy(buf,"hidden");
	}else{
		strcpy(buf,"visible");
	}
	return buf;
}

void widget_show(mwidget *node)
{
	if(node == NULL){
		return;
	}

	mixmlwidget *m = (mixmlwidget*)node;
	if(strcmp(node->opt->class_name,"ixmlwidget") == 0){
		printf("widget class *%s, name: %s\n",m->xmlwidget->name,m->name);
	}else{
		printf("widget class %s, name: %s\n",node->opt->class_name,node->name);
	}

	printf("  show x:%d show y:%d show w:%d show h:%d\n",node->show_abs_x,node->show_abs_y,node->show_w,node->show_h);

	printf("  properties:\n");
	widget_property_t *prop = NULL;

	int i = 0;
	while(1){
		if(node->opt->property[i].name == NULL){
			break;
		}
		prop = &node->opt->property[i];
		char *value = node->opt->get_property(node,prop->name);
		printf("    %2d.%s:  %s\n",i,prop->name,value);
		widget_free(value);
		i++;
	}
}

__WIDGET_GENERATE(widget)

mwidget *create_widget(char *name,int x, int y, int w, int h)
{
	_WIDGET_CREATE(widget,name,x,y,w,h);
	return m;
}

