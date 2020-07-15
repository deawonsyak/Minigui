#include "page.h"
#include "page_manager.h"
#include "script.h"
#include "screen.h"

#include "gui_service.h"


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

	page_t *page = param;

	mwidget *m = gui_node_get_node_by_name(page->widget_tree,str_widget_name);
	if(m == NULL){
		GUI_ERR("Can't find widget %s in page %s",str_widget_name,page->name);
		goto err;
	}

	char* ret = m->opt->get_property(m,str_property);
	if(ret == NULL){
		GUI_ERR("Get property fail");
		type = EXP_NODE_TYPE_CONST_NULL;
	}
	
err:
	if(_widget_name){
		exp_node_destroy(_widget_name);
	}
	if(_property){
		exp_node_destroy(_property);
	}

	if(str_property)
		free(str_property);
	if(str_widget_name)
		free(str_widget_name);
	return exp_node_create(type, (void *)ret);
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

	page_t *page = param;

	mwidget *m = gui_node_get_node_by_name(page->widget_tree,str_widget_name);
	if(m == NULL){
		GUI_ERR("Can't find widget %s in page %s",str_widget_name,page->name);
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

static exp_node_t *send_service_event(TFly_list_t *args, script_stack_t *block_stack,void *param)
{
	if(args == NULL || block_stack == NULL || param == NULL){
		GUI_ERR("Param error!");
		return NULL;
	}

	page_t *page = param;

	exp_node_t *event = TFly_list_get_first(args);
	exp_node_t *value = TFly_list_get_next(args);

	exp_node_t *_event = expression_calc(event,block_stack);
	exp_node_t *_value = expression_calc(value,block_stack);

	char *tmp = NULL;

	if(_event->type != EXP_NODE_TYPE_CONST_STRING){
		goto err;
	}

	char event_name[128];
	sprintf(event_name,"page.%s",_event->string);
	tmp = exp_node_to_string(_value);


	gui_service_event_send(event_name,0,0,tmp);

err:

	if(_event){
		exp_node_destroy(_event);
	}

	if(_value){
		exp_node_destroy(_value);
	}

	if(tmp){
		free(tmp);
	}

	return exp_node_create(EXP_NODE_TYPE_CONST_NULL,NULL);
}

static exp_node_t *jump_to_page_func(TFly_list_t *args, script_stack_t *block_stack,void *param)
{
	if(args == NULL || block_stack == NULL || param == NULL){
		GUI_ERR("Param error!");
		return NULL;
	}

	exp_node_t *page_file = TFly_list_get_first(args);

	if(page_file == NULL){
		return NULL;
	}

	exp_node_t *value = TFly_list_get_next(args);

	exp_node_t *_page_file = expression_calc(page_file,block_stack);

	if(_page_file == NULL){
		goto err;
	}

	char *page = exp_node_to_string(_page_file);
	char *v = NULL;
	exp_node_t *_value = NULL;
	if(value){
		_value = expression_calc(value,block_stack);
		v = exp_node_to_string(_value);
	}


	exp_var_t *var = exp_var_parse(NULL,"{}");
	exp_var_struct_add_parse(var,"page",page);
	exp_var_struct_add_parse(var,"value",v);

	char *data = exp_var_print(var);

	page_manager_send_event(GUI_EVENT_JUMP_TO_PAGE,0,0,data);

	if(v){
		free(v);
	}

	if(data){
		free(data);
	}

	exp_node_destroy(_page_file);
	exp_node_destroy(_value);
	exp_var_destroy(var);


	return exp_node_create(EXP_NODE_TYPE_CONST_NULL,NULL);

err:
	return NULL;
}

static exp_node_t *jump_to_next_func(TFly_list_t *args, script_stack_t *block_stack,void *param)
{
	if(args == NULL || block_stack == NULL || param == NULL){
		GUI_ERR("Param error!");
		return NULL;
	}

	exp_node_t *page_file = TFly_list_get_first(args);

	if(page_file == NULL){
		return NULL;
	}

	exp_node_t *value = TFly_list_get_next(args);

	exp_node_t *_page_file = expression_calc(page_file,block_stack);

	if(_page_file == NULL){
		goto err;
	}

	char *page = exp_node_to_string(_page_file);
	char *v = NULL;
	exp_node_t *_value = NULL;
	if(value){
		_value = expression_calc(value,block_stack);
		v = exp_node_to_string(_value);
	}


	exp_var_t *var = exp_var_parse(NULL,"{}");
	exp_var_struct_add_parse(var,"page",page);
	exp_var_struct_add_parse(var,"value",v);

	char *data = exp_var_print(var);

	page_manager_send_event(GUI_EVENT_JUMP_TO_NEXT,0,0,data);

	if(v){
		free(v);
	}

	if(data){
		free(data);
	}

	exp_node_destroy(_page_file);
	exp_node_destroy(_value);
	exp_var_destroy(var);


	return exp_node_create(EXP_NODE_TYPE_CONST_NULL,NULL);

err:
	return NULL;
}

static exp_node_t *backup_page(TFly_list_t *args, script_stack_t *block_stack,void *param)
{
	if(args == NULL || block_stack == NULL || param == NULL){
		GUI_ERR("Param error!");
		return NULL;
	}

	page_manager_send_event(GUI_EVENT_BACKUP_PAGE,0,0,NULL);

	return exp_node_create(EXP_NODE_TYPE_CONST_NULL,NULL);
}

static exp_node_t *jump_to_main_func(TFly_list_t *args, script_stack_t *block_stack,void *param)
{
	if(args == NULL || block_stack == NULL || param == NULL){
		GUI_ERR("Param error!");
		return NULL;
	}

	page_manager_send_event(GUI_EVENT_JUMP_TO_MAIN,0,0,NULL);

	return exp_node_create(EXP_NODE_TYPE_CONST_NULL,NULL);
}

int page_order_init(page_t *page)
{
	script_c_func_register(page->script,set_property,"set_property",page);
	script_c_func_register(page->script,get_property,"get_property",page);
	script_c_func_register(page->script,send_service_event,"send_service_event",page);
	script_c_func_register(page->script,jump_to_page_func,"jump_to_page",page);
	script_c_func_register(page->script,jump_to_next_func,"jump_to_next",page);
	script_c_func_register(page->script,jump_to_main_func,"jump_to_main",page);
	script_c_func_register(page->script,backup_page,"backup_page",page);

	return 0;
}

