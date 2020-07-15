#include "gui_service.h"

static int gui_cmd_handle(vic_cmd_t *cmd, void *param)
{
	if(cmd == NULL){
		return -1;
	}

	gui_event_e event = (gui_event_e)param;

	page_manager_send_event(event, cmd->head->sessionID,cmd->head->serial,cmd->value);
	return 0;
}


/********************************************************
 *	page:	页面控制命令， 参数为json格式,如下:
 *		{"event":"event name","value":""}
 *
 *	jump_to_page: 调转到指定页, 参数为json格式， 如下:
 *		{"page":"test.xml","value":""}
 *
 *	jump_to_next: 调转到下一页, 参数为json格式， 如下:
 *		{"page":"test.xml","value":""}
 *		
 * ******************************************************/
const static cmd_list_t cmds[] = {
	{"page","page control command",gui_cmd_handle,(void*)GUI_EVENT_PAGE_EVENT},
	{"jump_to_page","跳转到指定页",gui_cmd_handle,(void*)GUI_EVENT_JUMP_TO_PAGE},
	{"jump_to_next","跳转到下一页",gui_cmd_handle,(void*)GUI_EVENT_JUMP_TO_NEXT},
	{"jump_to_main","跳转到首页",gui_cmd_handle,(void*)GUI_EVENT_JUMP_TO_MAIN},
	{"backup","返回上一页",gui_cmd_handle,(void*)GUI_EVENT_BACKUP_PAGE},
	{NULL,NULL,NULL,NULL}
};

/********************************************************
 *	page:	页面触发发的事件,参数由xml页面定义
 *
 *	page_close:	页面关闭事件， 参数为页面名称
 *
 *
 * ******************************************************/
const static event_list_t events[] = {
	{"page","页面事件"},
	{"page_close","页面关闭事件"},
	{"backup","返回上一页"},
	{NULL,NULL}
};

static vic_service_t *g_gui_service = NULL;

int gui_service_init(void)
{
	g_gui_service = orb_service_init("gui",cmds,events);
	if(g_gui_service == NULL){
		GUI_ERR("Service init fail");
		return -1;
	}
	return 0;
}

int gui_service_event_send(char *event_name, int sessionID, int serial, char *value)
{
	return orb_service_send_event(g_gui_service,event_name,sessionID,serial,value);
}
