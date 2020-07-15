#include "page_manager.h"
#include "page.h"
#include "page_xml.h"
#include "gui_memory.h"
#include "orb_stack.h"
#include "gui_service.h"
#include "page_event.h"
#include "cJSON.h"

static page_manager_t g_page_manager;

static void page_manager_page_delete(page_t *page)
{
	if(page == NULL){
		return;
	}

	if(page == g_page_manager.main){
		return;
	}

	orb_list_del(g_page_manager.pages,page);
	gui_page_destroy(page);

}

static int clean_page_stack(void)
{
	page_t *page = orb_stack_Pop(g_page_manager.page_stack);
	while(page){
		gui_page_stop(page);
		page_manager_page_delete(page);
		page = orb_stack_Pop(g_page_manager.page_stack);
	}

	return 0;

}

static int jump_to_main(gui_event_t *event, void *param)
{
	if(g_page_manager.c_page == g_page_manager.main){
		return 0;
	}

	page_t *page = g_page_manager.c_page;

	g_page_manager.c_page = g_page_manager.main;
	gui_page_start(g_page_manager.c_page);
	gui_page_stop(page);
	page_manager_page_delete(page);

	page = orb_stack_Pop(g_page_manager.page_stack);
	while(page){
		page_manager_page_delete(page);
		page = orb_stack_Pop(g_page_manager.page_stack);
	}

	return 0;
}

static int jump_to_next(gui_event_t *event, void *param)
{
	if(event == NULL){
		return -1;
	}

	cJSON *root = cJSON_Parse(event->value);
	if(root == NULL){
		GUI_ERR("jump to page event value error");
		return -1;
	}

	cJSON *page_file = cJSON_GetObjectItem(root,"page");
	if(page_file == NULL || page_file->type != cJSON_String){
		goto err;
	}

	char page_name[64];

	char *tmp = strrchr(page_file->valuestring,'.');
	strncpy(page_name,page_file->valuestring, tmp - page_file->valuestring);

	page_t *page = page_manager_get_page_by_name(page_name);
	if(page == NULL){
		page = xml_page_load(page_file->valuestring);
		if(page == NULL){
			goto err;
		}
		orb_list_add(g_page_manager.pages,page);
	}

	if(g_page_manager.c_page != g_page_manager.main){
		orb_stack_Push(g_page_manager.page_stack,g_page_manager.c_page);
	}
	g_page_manager.c_page = page;
	gui_page_start(page);

	cJSON_Delete(root);

	return 0;
err:
	if(root){
		cJSON_Delete(root);
	}
	return -1;
}

static int jump_to_page(gui_event_t *event, void *param)
{
	if(event == NULL){
		return -1;
	}

	cJSON *root = cJSON_Parse(event->value);
	if(root == NULL){
		GUI_ERR("jump to page event value error");
		return -1;
	}

	cJSON *page_file = cJSON_GetObjectItem(root,"page");
	if(page_file == NULL || page_file->type != cJSON_String){
		goto err;
	}

	char page_name[64];

	char *tmp = strrchr(page_file->valuestring,'.');
	strncpy(page_name,page_file->valuestring, tmp - page_file->valuestring);

	page_t *page = page_manager_get_page_by_name(page_name);
	if(page == NULL){
		page = xml_page_load(page_file->valuestring);
		if(page == NULL){
			goto err;
		}
		orb_list_add(g_page_manager.pages,page);
	}

	page_t *old_page = g_page_manager.c_page;

	g_page_manager.c_page = page;
	gui_page_start(page);

	cJSON_Delete(root);

	gui_page_stop(old_page);
	if(old_page != g_page_manager.main){	
		page_manager_page_delete(old_page);
	}

	page = orb_stack_Pop(g_page_manager.page_stack);
	while(page){
		page_manager_page_delete(page);
		page = orb_stack_Pop(g_page_manager.page_stack);
	}

	return 0;
err:
	if(root){
		cJSON_Delete(root);
	}
	return -1;
}

static int close_current_page(gui_event_t *event, void *param)
{

	return 0;
}

static int backup_page(gui_event_t *event, void *param)
{
	if(g_page_manager.c_page == g_page_manager.main){
		return -1;
	}

	page_t *c_page = g_page_manager.c_page;

	page_t *page = orb_stack_Pop(g_page_manager.page_stack);
	if(page == NULL){
		page = g_page_manager.main;
	}

	gui_page_start(page);
	gui_page_stop(c_page);
	page_manager_page_delete(c_page);

	g_page_manager.c_page = page;

	return 0;
}

static int page_event_handle(gui_event_t *event, void *param)
{
	script_t *script = NULL;
	if(g_page_manager.c_page){
		// TODO	
		script = g_page_manager.c_page->script;
	}

	char code[256];

	exp_node_t *result = NULL;

	if(script){
		script_change_var_value(script,"Event",event->value);

		sprintf(code,"Event.sessionID = %d",event->session_id);
		result = script_run_code(script,code);
		exp_node_destroy(result);

		sprintf(code,"Event.serial = %d",event->serial);
		result = script_run_code(script,code);
		exp_node_destroy(result);


		result = script_run_code(script,"Print(Event)");
		exp_node_destroy(result);

		result = script_run_code(script,"Event.event");
		char *sevent = exp_node_to_string(result);
		exp_node_destroy(result);
		if(sevent == NULL){
			GUI_ERR("page event name error");
			goto err;
		}

		page_event_t *page_event = page_event_find(g_page_manager.c_page,sevent,NULL);
		if(page_event){
			expression_calc(page_event->exp,script->code_stack);
		}else{
			GUI_ERR("Can't find event %s in page %s",sevent,g_page_manager.c_page->name);
		}

		free(sevent);

	}

	return 0;
err:
	return -1;

}

static int widget_event_handle(gui_event_t *event, void *param)
{
	widget_event_t *wet = event->widget_event;
	if(g_page_manager.c_page == NULL){
		GUI_ERR("current page is null");
		return -1;
	}
	page_t *page = g_page_manager.c_page;

	page_event_t *page_event = page_event_find(page,wet->event,wet->hd);
	if(page_event == NULL){
		GUI_ERR("page %s event %s find fail",page->name,wet->event);
		return -1;
	}

	exp_node_t *result = expression_calc(page_event->exp,page->script->code_stack);
	if(result == NULL){
		GUI_ERR("page %s event %s handle fail",page->name,wet->event);
		return -1;
	}

	exp_node_destroy(result);

	return 0;
}

static struct gui_event_handle_table g_event_table[] = {
	{GUI_EVENT_JUMP_TO_MAIN, jump_to_main,NULL},
	{GUI_EVENT_JUMP_TO_PAGE, jump_to_page,NULL},
	{GUI_EVENT_CLOSE_CURRENT_PAGE, close_current_page,NULL},
	{GUI_EVENT_BACKUP_PAGE, backup_page,NULL},
	{GUI_EVENT_PAGE_EVENT,		page_event_handle,NULL},
	{GUI_EVENT_WIDGET_EVENT,	widget_event_handle,NULL}
};

static struct gui_event_handle_table *find_event_handle(gui_event_e event)
{
	unsigned long i;
	for(i = 0; i < sizeof(g_event_table)/sizeof(g_event_table[0]); i++){
		if(g_event_table[i].event == event){
			return &g_event_table[i];
		}
	}
	return NULL;
}

static void page_manager_event_thread(void *param)
{
	gui_event_t *event;
	while(1){
		event = orb_queue_get(g_page_manager.event_queue,ORB_WAITFOREVER);
		if(event == NULL){
			continue;
		}
		
		struct gui_event_handle_table *event_handle = find_event_handle(event->event);
		if(event_handle){
			event_handle->handle(event,NULL);
		}

		gui_event_destroy(event);
	}
}

int page_manager_init(void)
{
	memset(&g_page_manager,0,sizeof(g_page_manager));	

	g_page_manager.mode_list = orb_list_create();
	g_page_manager.card_list = orb_list_create();
	g_page_manager.pages = orb_list_create();

	g_page_manager.page_stack = orb_stack_create();

	g_page_manager.event_queue = orb_queue_create(10);
	struct thread_param param = {"page_manager",OS_PRIORITY_HIGH,1024};
	orb_thread_create(&g_page_manager.event_loop_thread,&param,page_manager_event_thread,NULL);

	page_t *page = xml_page_load("main.xml");
	g_page_manager.c_page = page;
	g_page_manager.main = page;
	gui_page_start(page);

	orb_list_add(g_page_manager.pages,page);

	page_show(page);

	gui_service_init();

	return 0;
}

void gui_event_destroy(gui_event_t *event)
{
	if(event == NULL){
		return;
	}

	if(event->event == GUI_EVENT_WIDGET_EVENT){
		widget_event_destroy(event->widget_event);
	}else{
		if(event->value){
			gui_free(event->value);
		}
	}
	gui_free(event);
}

int page_manager_send_event(gui_event_e type, int session_id,int serial, void *value)
{
	gui_event_t *event = gui_calloc(1,sizeof(gui_event_t));
	event->event = type;
	event->session_id = session_id;
	event->serial = serial;
	if(type == GUI_EVENT_WIDGET_EVENT){
		event->widget_event = (widget_event_t*)value;
	}else{
		if(value){
			event->value = gui_calloc(1,strlen(value)+1);
			strcpy(event->value,value);
		}
	}

	if(orb_queue_put(g_page_manager.event_queue,event,100)){
		GUI_ERR("gui event put queue fail");
		gui_event_destroy(event);
		return -1;
	}

	return 0;
}

page_t *page_manager_get_page(int page_id)
{
	if(g_page_manager.c_page == NULL){
		return NULL;
	}
	
	if(g_page_manager.c_page->id == page_id){
		return g_page_manager.c_page;
	}

	return NULL;
}

page_t *page_manager_get_page_by_name(char *name)
{
	page_t *page = orb_list_get_first(g_page_manager.pages);
	while(page){
		if(strcmp(page->name,name) == 0){
			return page;
		}
		page = orb_list_get_next(g_page_manager.pages);
	}

	return NULL;
}

page_t *page_manager_get_current_page(void)
{
	return g_page_manager.c_page;
}

void page_manager_test_close_page(void)
{
	if(g_page_manager.c_page || g_page_manager.main == g_page_manager.c_page){
		printf("main page %s close\n",g_page_manager.c_page->name);
		gui_page_stop(g_page_manager.c_page);
		gui_page_destroy(g_page_manager.c_page);
		g_page_manager.c_page = NULL;
		g_page_manager.main = NULL;
	}else{
		GUI_ERR("main page close test fail");
	}
}


void page_manager_show_all_page(void)
{
	page_t *page = orb_list_get_first(g_page_manager.pages);
	int count = 0;
	printf("Pages:\n");
	while(page){
		printf("  %d. %s\n",count++,page->name);
		page = orb_list_get_next(g_page_manager.pages);
	}
	printf("\n");
}

