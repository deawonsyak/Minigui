#ifndef __GUI_SERVICE_H__
#define __GUI_SERVICE_H__

#include "vicenter_service.h"
#include "page_manager.h"


/********************************************************
 *	function	gui_service_init
 *		初始化 gui service
 *		
 * ******************************************************/
int gui_service_init(void);

/********************************************************
 *	function	gui_service_event_send
 *		发送GUI event到vicenter
 *
 * ******************************************************/
int gui_service_event_send(char *event_name, int sessionID, int serial, char *value);

#endif

