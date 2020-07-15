
#include "xmlwidget_widget.h"
#include "screen.h"
#include "page_event.h"
#include "page_label.h"
#include "widget_manager.h"
#include "ixmlwidget_widget.h"
#include "card_widget.h"

static exp_node_t *trigger_widget_event(TFly_list_t *args, script_stack_t *block_stack,void *param);
static exp_node_t *set_property(TFly_list_t *args, script_stack_t *block_stack,void *param);
static exp_node_t *get_property(TFly_list_t *args, script_stack_t *block_stack,void *param);
static exp_node_t *send_keyboard_key(TFly_list_t *args, script_stack_t *block_stack,void *param);
static exp_node_t *open_keyboard(TFly_list_t *args, script_stack_t *block_stack,void *param);
static exp_node_t *close_keyboard(TFly_list_t *args, script_stack_t *block_stack,void *param);

void xmlwidget_destroy(mwidget *node, widget_opt *clas)
{
	mxmlwidget *m = (mxmlwidget*)node;
	if(m->used_num > 0){
		GUI_ERR("xmlwidget destroy fail, used_num is not 0")
		return;
	}

	if(m->page){
		gui_page_destroy(m->page);
		m->page = NULL;
	}

	if(m->widget_tree){
		m->widget_tree->opt->destroy(m->widget_tree,m->widget_tree->opt);
		m->widget_tree = NULL;
	}

	if(m->event_cbk){
		screen_unreister_widget_event_cbk(m->event_cbk);
		m->event_cbk = NULL;
	}


	clas->parent->destroy(node,clas->parent);
}

static mwidget *Xmlwidget_widget_copy(mwidget *src,mwidget *parent)
{
	if(src == NULL ){
		return NULL;
	}

	mwidget *m = create_widget_by_name(src->opt->class_name,src->name,src->x,src->y,src->w,src->h);

	int i = 0;
	widget_property_t *prop = NULL;
	while(1){
		if(m->opt->property[i].name == NULL){
			break;
		}
		prop = &m->opt->property[i];
		char *value = src->opt->get_property(src,prop->name);
		m->opt->set_property( (mwidget*)m, prop->name, value);
		widget_free(value);
		i++;
	}
	m->event_active_flag = src->event_active_flag;

	if(src->child){
		Xmlwidget_widget_copy(src->child,m);
	}

	if(parent){
		gui_node_add_child_node_to_end(m,parent);
	}

	if(src->next){
		Xmlwidget_widget_copy(src->next,parent);
	}

	return m;
}

/********************************************************
 *	拷贝事件处理
 *
 * ******************************************************/
static void copy_event_list(mxmlwidget *mxml, mixmlwidget *new)
{
	orb_list_t *src = mxml->page->event_list;
	orb_list_t *dst = new->event_list;

	page_event_t *event = orb_list_get_first(src);
	while(event){
		page_event_t *new_event = page_event_create(event->name);

		// 重新生成表达式
		char func_name[128] = {0};
		sprintf(func_name,"%s()",event->exp->func->s_func->name);
		exp_string_t *exp_str = exp_string_decode(func_name);
		exp_node_t *exp = expression_decode(exp_str,new->script);
		exp_string_destroy(exp_str);
		if(exp == NULL){
			GUI_ERR("expression decode fail");
		}
		new_event->exp = exp;

		new_event->widget = gui_node_get_node_by_name(new->widget_tree,event->widget->name);
		orb_list_add(dst,new_event);
		
		event = orb_list_get_next(src);
	}
}


static void XmlwidgetScriptFuncInit(script_t *script,mwidget *m)
{
	if(m == NULL || script == NULL){
		return;
	}

	script_c_func_register(script,trigger_widget_event,"trigger_widget_event",m);
	script_c_func_register(script,send_keyboard_key,"send_keyboard_key",m);
	script_c_func_register(script,open_keyboard,"open_keyboard",m);
	script_c_func_register(script,open_keyboard,"close_keyboard",m);

}

mixmlwidget *XmlwidgetCreateWidget(mxmlwidget *m,char *name, int x, int y, int w, int h)
{
	mixmlwidget *new = create_ixmlwidget_widget(name,x,y,w,h);

	m->opt->lock_data((mwidget*)m);
	new->widget_tree = Xmlwidget_widget_copy(m->widget_tree,NULL);
	new->widget_tree->w = m->w;
	new->widget_tree->h = m->h;
	mcard *card = (mcard*)new->widget_tree;
	card->opt->add_to_icard(card,(mwidget*)new);

	char buf[64] = {0};
	sprintf(buf,"%s_%s",m->name,new->name);
	new->script = script_script_create();
	XmlwidgetScriptFuncInit(new->script,(mwidget*)new);
	script_c_func_register(new->script,set_property,"set_property",new);
	script_c_func_register(new->script,get_property,"get_property",new);
	script_parse_from_string(new->script,m->page->script->code);

	run_script(new->script);

	//GUI_ERR("m:%p new:%p script:%p\n",m,new,new->script);
	//show_script(new->script);
	//show_script(m->page->script);

	copy_event_list(m,new);

	new->xmlwidget = m;
	m->used_num++;

	m->opt->unlock_data((mwidget*)m);

	//gui_node_show(new);
	return new;
}


int XmlwidgetStart(mwidget *node)
{
	mxmlwidget *m = (mxmlwidget*)node;
	if(node->screen == NULL){
		return 0;
	}
	
	screen_start_node(m->widget_tree,m->screen);

	return 0;
}

int XmlwidgetStop(mwidget *node)
{
	mxmlwidget *m = (mxmlwidget*)node;

	if(node->screen == NULL){
		return 0;
	}

	screen_unreister_widget_event_cbk(m->event_cbk);

	screen_stop_node(m->widget_tree,node->screen);
	
	return 0;
}

int XmlwidgetUserEventRegister(mxmlwidget *m,char *event)
{
	if(m == NULL || event == NULL){
		return -1;
	}

	struct user_event *uevent = m->user_event;
	struct user_event *prev = NULL;
	while(uevent){
		if(strcmp(uevent->name,event) == 0){
			GUI_ERR("User event register fail! event %s is exist in widget:%s",event,m->name);
			return -1;
		}
		prev = uevent;
		uevent = uevent->next;
	}

	struct user_event *new = calloc(1,sizeof(*new) + strlen(event) + 1);

	new->name = (char*)new + sizeof(*new);
	strcpy(new->name,event);
	if(prev == NULL){
		m->user_event = new;
	}else{
		prev->next = new;
	}

	return 0;
}

int XmlwidgetUserEventIndexGet(mxmlwidget *m, char *event,unsigned int flag)
{
	if(m == NULL || event == NULL){
		return -1;
	}

	struct user_event *uevent = m->user_event;
	int i = 0;

	while(uevent){
		if(strcmp(uevent->name,event) == 0){
			if(flag & (1<<i)){
				return 1;
			}
		}
		i++;
		uevent = uevent->next;
	}

	return 0;
}

static exp_node_t *trigger_widget_event(TFly_list_t *args, script_stack_t *block_stack,void *param)
{
	if(args == NULL || block_stack == NULL || param == NULL){
		GUI_ERR("Param error!");
		return NULL;
	}

	mixmlwidget *m = param;
	
	exp_node_t *event_name = TFly_list_get_first(args);

	exp_node_t *_event_name = expression_calc(event_name,block_stack);


	char *value = exp_node_to_string(_event_name);

	int ret = m->xmlwidget->opt->user_event_check(m->xmlwidget,value,m->user_event_flag);
	if(ret == 1){
		m->opt->event_send((mwidget*)m,value);
	}


	if(value){
		free(value);
	}
	exp_node_destroy(_event_name);

	return exp_node_create(EXP_NODE_TYPE_CONST_NULL,NULL);
}

static exp_node_t *set_property(TFly_list_t *args, script_stack_t *block_stack,void *param)
{
	if(args == NULL || block_stack == NULL || param == NULL){
		GUI_ERR("Param error!");
		return NULL;
	}
	exp_node_t *widget_name = TFly_list_get_first(args);
	exp_node_t *property = TFly_list_get_next(args);
	exp_node_t *value = TFly_list_get_next(args);

	exp_node_t *_widget_name = expression_calc(widget_name,block_stack);
	exp_node_t *_property = expression_calc(property,block_stack);
	exp_node_t *_value = expression_calc(value,block_stack);

	char *str_value = exp_node_to_string(_value);
	char *str_widget_name = exp_node_to_string(_widget_name);
	char *str_property = exp_node_to_string(_property);

	mixmlwidget *mi = param;

	mwidget *m = gui_node_get_node_by_name(mi->widget_tree,str_widget_name);
	if(m == NULL){
		GUI_ERR("Can't find widget %s in widget %s",str_widget_name,mi->name);
		goto err;
	}

	int ret = m->opt->set_property(m,str_property,str_value);
	if(ret){
		GUI_ERR("Set property fail");
	}
err:
	if(_widget_name){
		exp_node_destroy(_widget_name);
	}
	if(_property){
		exp_node_destroy(_property);
	}
	if(_value){
		exp_node_destroy(_value);
	}

	if(str_value)
		free(str_value);
	if(str_property)
		free(str_property);
	if(str_widget_name)
		free(str_widget_name);

	return exp_node_create(EXP_NODE_TYPE_CONST_NULL,NULL);
}

static exp_node_t *get_property(TFly_list_t *args, script_stack_t *block_stack,void *param)
{	
	if(args == NULL || block_stack == NULL || param == NULL){
		GUI_ERR("Param error!");
		return NULL;
	}
	exp_node_type_e type = EXP_NODE_TYPE_CONST_STRING;
	exp_node_t *widget_name = TFly_list_get_first(args);
	exp_node_t *property = TFly_list_get_next(args);

	exp_node_t *_widget_name = expression_calc(widget_name,block_stack);
	exp_node_t *_property = expression_calc(property,block_stack);

	char *str_widget_name = exp_node_to_string(_widget_name);
	char *str_property = exp_node_to_string(_property);

	mixmlwidget *page = param;

	mwidget *m = gui_node_get_node_by_name(page->widget_tree,str_widget_name);
	if(m == NULL){
		GUI_ERR("Can't find widget %s in %s",str_widget_name,page->name);
		goto err;
	}

	char* ret = m->opt->get_property(m,str_property);
	if(ret == NULL){
		GUI_ERR("Get property fail");
		type = EXP_NODE_TYPE_CONST_NULL;
	}
	
err:
	if(str_widget_name){
		script_free(str_widget_name);
	}

	if(str_property){
		script_free(str_property);
	}
	
	if(_widget_name){
		exp_node_destroy(_widget_name);
	}

	if(_property){
		exp_node_destroy(_property);
	}

	return exp_node_create(type, (void *)ret);
}

static exp_node_t *send_keyboard_key(TFly_list_t *args, script_stack_t *block_stack,void *param)
{
	if(args == NULL || block_stack == NULL || param == NULL){
		GUI_ERR("Param error!");
		return NULL;
	}

	exp_node_t *key = TFly_list_get_first(args);
	if(key == NULL){
		GUI_ERR("args is too less");
		return exp_node_create(EXP_NODE_TYPE_CONST_NULL,NULL);
	}

	exp_node_t *_key = expression_calc(key,block_stack);
	if(_key == NULL){
		return NULL;
	}

	char *value = exp_node_to_string(_key);

	ScreenKeyboardMsgSend(value);
	script_free(value);
	
	return exp_node_create(EXP_NODE_TYPE_CONST_NULL,NULL);

}

static exp_node_t *open_keyboard(TFly_list_t *args, script_stack_t *block_stack,void *param)
{
	if(args == NULL || block_stack == NULL || param == NULL){
		GUI_ERR("Param error!");
		return NULL;
	}

	exp_node_t *key = TFly_list_get_first(args);
	if(key == NULL){
		GUI_ERR("args is too less");
		return exp_node_create(EXP_NODE_TYPE_CONST_NULL,NULL);
	}

	exp_node_t *_key = expression_calc(key,block_stack);
	if(_key == NULL){
		return NULL;
	}

	char *value = exp_node_to_string(_key);

	KeyboardOpen(value);

	script_free(value);

	return exp_node_create(EXP_NODE_TYPE_CONST_NULL,NULL);
}

static exp_node_t *close_keyboard(TFly_list_t *args, script_stack_t *block_stack,void *param)
{
	if(args == NULL || block_stack == NULL || param == NULL){
		GUI_ERR("Param error!");
		return NULL;
	}

	KeyboardClose();

	return exp_node_create(EXP_NODE_TYPE_CONST_NULL,NULL);
}

mwidget *XmlwidgetGetFocus(mwidget *node, unsigned int flag, struct gui_coordinate *coor)
{
	mxmlwidget *m = (mxmlwidget*)node;

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

int XmlwidgetTouchEventHandle(mwidget *node, unsigned int flag, struct move_info *mv_info)
{
	mxmlwidget *m = (mxmlwidget*)node;
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

_WIDGET_GENERATE(xmlwidget,widget)

