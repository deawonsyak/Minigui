#include <string.h>
#include "screen.h"
#include "uni_gui_node.h"

#include <minigui/common.h>
#include <minigui/minigui.h>
#include <minigui/gdi.h>
#include <minigui/window.h>
#include <minigui/control.h>

#include "xmlwidget_widget.h"

int screenProc(HWND hwnd, int message, WPARAM wParam, LPARAM lParam);

static int screenSetBkpho(void *hd,char *photo_path)
{
	Screen *screen = (Screen*)hd;
	if(hd == NULL || photo_path == NULL){
		return -1;
	}
	if(screen->bk_pho != NULL){
		free(screen->bk_pho);
		screen->bk_pho = NULL;
	}
	screen->bk_pho = malloc(strlen(photo_path)+1);
	memset(screen->bk_pho,0,strlen(photo_path)+1);
	strcpy(screen->bk_pho,photo_path);
	return 0;
}

static int screenSetBackground(void *hd,char type,void *param)
{
	if(hd == NULL || param == NULL){
		return -1;
	}
	Screen *screen = (Screen*)hd;

	if(type == SCREEN_BACKGROUND_TYPE_COLOR){
		screen->bgColor = *(int*)param;
		if(screen->bkmap != NULL){
			UnloadBitmap(screen->bkmap);
			free(screen->bkmap);
			screen->bkmap = NULL;
		}
		if(screen->bk_pho != NULL){
			free(screen->bk_pho);
			screen->bk_pho = NULL;
		}
	}else if(type == SCREEN_BACKGROUND_TYPE_PHOTO){
		char *photo_path = (char*)param;
		if(screen->bk_pho != NULL){
			free(screen->bk_pho);
			screen->bk_pho = NULL;
		}
		screen->bk_pho = malloc(strlen(photo_path)+1);
		memset(screen->bk_pho,0,strlen(photo_path)+1);
		strcpy(screen->bk_pho,photo_path);
		//LoadBitmapFromFile(screen,screen->bkmap,photo_path);
	}
	return 0;
}

static void screen_widget_event_handle(void *param)
{
	GUI_INFO("screen widget event thread start");
	Screen *screen = param;
	widget_event_t *event;
	int ret = -1;
	while(1){
		event = screen->widget_event_queue->get(screen->widget_event_queue,-1);
		if(event == NULL){
			continue;
		}
		mwidget *m = event->hd;
		GUI_DEBUG("Recv widget event %s node:%s",event->event,m->name);

		screen_widget_event_cbk_t *wcbk = orb_list_get_first(screen->widget_event_cbk_list);
		while(wcbk){
			if(wcbk->cbk){
				ret = wcbk->cbk(event,wcbk->param);
				if(ret == 0){
					break;
				}
			}
			wcbk = orb_list_get_next(screen->widget_event_cbk_list);
		}

		if(ret != 0 && screen->widget_event_handle_cbk){
			screen->widget_event_handle_cbk(event,screen->widget_event_handle_param);
		}
		
		widget_event_destroy(event);
	}
}

BOOL initScreen(Screen *screen)
{
	screen->setBkpho = screenSetBkpho;
	screen->setBackground = screenSetBackground;
	screen->node = NULL;
	WNDCLASS MyClass;
	MyClass.spClassName = screen->name;
	MyClass.dwStyle = WS_NONE;
	MyClass.dwExStyle = WS_EX_NONE;
	MyClass.hCursor = 0;
	MyClass.iBkColor = PIXEL_lightgray;
	MyClass.WinProc = screenProc;

	pthread_mutex_init(&screen->mutex,NULL);


	screen->widget_event_queue = orb_queue_create(10);
	screen->widget_event_cbk_list = orb_list_create();
	screen->xmlwidget_list = orb_list_create();
	orb_mutex_init(&screen->widget_event_cbk_mutex);

	struct thread_param param = {"screen_event",OS_PRIORITY_HIGH,1024};
	orb_thread_t td;
	orb_thread_create(&td,&param,screen_widget_event_handle,screen);


	return RegisterWindowClass(&MyClass);
}

static int sort_array(int *array, int maxNum)
{
	if(NULL == array){
		return -1;
	}

	int tmp = 0;
	int i = 0;
	int j = 0;
	
	for (i = 0; i < maxNum - 1; i++){
		for (j = i+1; j < maxNum ; j++){
			if (abs(array[i]) < abs(array[j]))
			{
				tmp = array[i];
				array[i] = array[j];
				array[j] = tmp;
			}
		}
	}

	return 0;
}

static int _screen_calc_speed(Screen *screen,struct move_info *mv_info, int ticks)
{
	int av_x = 0,av_y = 0;

	if(ticks > 0){
		av_x = mv_info->lengthx/ticks;
		av_y = mv_info->lengthy/ticks;
	}

	screen->movelen[screen->movelen_index].x = av_x;
	screen->movelen[screen->movelen_index].y = av_y;
	screen->movelen_index++;
	if(screen->movelen_index >= SCREEN_MOVELEN_NUM){
		screen->movelen_index = 0;
	}

	int moveXLen[SCREEN_MOVELEN_NUM] = {0};
	int moveYLen[SCREEN_MOVELEN_NUM] = {0};
	for (int i = 0; i < SCREEN_MOVELEN_NUM; i++){
		moveXLen[i] = screen->movelen[i].x;
		moveYLen[i] = screen->movelen[i].y;
	}

	sort_array(moveXLen, SCREEN_MOVELEN_NUM);
	sort_array(moveYLen, SCREEN_MOVELEN_NUM);

	int averageNum = 0; 
	if ((SCREEN_MOVELEN_AVERAGE >= SCREEN_MOVELEN_NUM) || (SCREEN_MOVELEN_AVERAGE <= 0)){
		averageNum = SCREEN_MOVELEN_NUM;
	}else{
		averageNum = SCREEN_MOVELEN_AVERAGE;
	}

	av_x = 0;
	av_y = 0;

	/*
	printf("\n============================\n");
	for(int i = 0; i < SCREEN_MOVELEN_NUM; i++){
		printf("Screen move speed[%d].x = %d  y = %d\n", i, screen->movelen[i].x, screen->movelen[i].y);
	} 
	printf("\n============================\n");
	*/

	//The speed is uneven, take the average value of the maximum three items, the sliding is smoother
	for(int i = 0; i < averageNum; i++){
		
		av_x += moveXLen[i];
		av_y += moveYLen[i];
	} 

	av_x = av_x/averageNum;
	av_y = av_y/averageNum;

	//printf("averageNum = %d av_x = %d  av_y = %d\n", averageNum, av_x, av_y);

	mv_info->speedx = av_x;
	mv_info->speedy = av_y;

#if 0
	if(av_x < -40){
		mv_info->speedx = -7;
	}else if(av_x < -20){
		mv_info->speedx = -4;
	}else if(av_x < -2){
		mv_info->speedx = -1;
	}else if(av_x < 2){
		mv_info->speedx = 0;
	}else if(av_x < 20){
		mv_info->speedx = 1;
	}else if(av_x < 40){
		mv_info->speedx = 4;
	}else{
		mv_info->speedx = 7;
	}

	if(av_y < -40){
		mv_info->speedy = -7;
	}else if(av_y < -20){
		mv_info->speedy = -4;
	}else if(av_y < -2){
		mv_info->speedy = -1;
	}else if(av_y < 2){
		mv_info->speedy = 0;
	}else if(av_y < 20){
		mv_info->speedy = 1;
	}else if(av_y < 40){
		mv_info->speedy = 4;
	}else{
		mv_info->speedy = 7;
	}
#endif

	//printf(" calc speed x:%d y:%d, av_x:%d av_y:%d\n",screen->move_speedx,screen->move_speedy,av_x,av_y);

	return 0;
}

#define MSG_USER_KEY 0x81
int ScreenKeyboardMsgSend(char *key)
{
	if(key == NULL){
		GUI_ERR("Param error");
		return -1;
	}

	char *buf = widget_calloc(1,strlen(key)+1);
	strcpy(buf,key);
	SendMessage(g_screen.hwnd,MSG_USER_KEY,0,(LPARAM)buf);
	return 0;
}

static uint64_t screen_timer_tick = 0;

int screenProc(HWND hwnd, int message, WPARAM wParam, LPARAM lParam)
{
	Screen *screen = (Screen*)GetWindowAdditionalData(hwnd);
	struct move_info mv_info;
	switch (message){
		case MSG_CREATE: 
			screen->hwnd = hwnd;
			screen->timer_count = -1;
			SetTimer(hwnd,SCREEN_TIMER_ID,SCREEN_TIMER_FREQ);
			screen->bgColor = PIXEL_transparent;			//这个必须要有！！！
			SetWindowBkColor(screen->hwnd,screen->bgColor);	//这个必须要有！！！
			
			break;
		case MSG_PAINT:
			{
				//printf("%s %d MSG_PAINT\r\n",__func__,__LINE__);
				struct timeval tv,tv1;
				gettimeofday(&tv,NULL);
				HDC hdc = BeginPaint(hwnd);
				pthread_mutex_lock(&screen->mutex);

				// 绘制页面层
				if(screen->node != NULL ){
					gui_node_paint(screen->node,hdc);
				}

				// 绘制工具层
				if(screen->tool_layer != NULL){
					gui_node_paint(screen->tool_layer,hdc);
				}

				// 绘制弹框层
				if(screen->suspention != NULL){
					gui_node_paint(screen->suspention,hdc);
				}

				pthread_mutex_unlock(&screen->mutex);


				//gettimeofday(&tv1,NULL);
				//printf("**screenProc MSG_PAINT time %lds %ldus\n",tv1.tv_sec-tv.tv_sec,tv1.tv_usec-tv.tv_usec);
				//gui_node_show(screen->node);
				EndPaint(hwnd, hdc);
				gettimeofday(&tv1,NULL);
				//printf("screenProc MSG_PAINT time %lds %ldus\n",tv1.tv_sec-tv.tv_sec,tv1.tv_usec-tv.tv_usec);
			}
			break;
		case MSG_TIMER:
			{
				screen_timer_tick++;
				//printf("screenProc MSG_TIMER %p count:%d\n",screen,screen->timer_count);
				//screen->timer_count = -1;
				
				if(screen->longpress_flag == 0 && screen->mouse_status == SCREEN_MOUSE_STATUS_DOWN){
					uint64_t gap_tick = 2500/SCREEN_TIMER_FREQ/10;
					if(screen_timer_tick - screen->buttondown_tick > gap_tick ){
						mv_info.coor.x = screen->buttondown_coor.x;
						mv_info.coor.y = screen->buttondown_coor.y;
						if (NULL != screen->focus_node){
							WIDGET_TOUCH_EVENT_HANDLE(screen->focus_node,SCREEN_FORCE_FLAG_LONGPRESS,&mv_info);
						}
						screen->longpress_flag = 1;
					}
				}
				
				if(screen->timer_count == 0){
					return 0;
				}
				if(screen->timer_count > 0){
					screen->timer_count--;
					if(screen->timer_count == 0){
					}
				}

				if(screen->mv_start){
					if(screen->last_move_tick < screen_timer_tick - 5){
						if(screen->focus_node){
							WIDGET_TOUCH_EVENT_HANDLE(screen->focus_node,SCREEN_FORCE_FLAG_MOVESTOP,NULL);
							screen->mv_start = 0;
							memset(screen->movelen,0,sizeof(screen->movelen));
						}
					}
				}

				SendMessage(hwnd,MSG_SCREEN_PAINT,0,(LPARAM)screen);
				//InvalidateRect(hwnd,NULL,TRUE);
				InvalidateRect(hwnd,NULL,FALSE);
			}
			return 0;
		case MSG_LBUTTONDOWN:
			{
				//GUI_DEBUG(DEBUG_NORMAL,"screenProc MSG_LBUTTONDOWN\n");
				if(screen->node == NULL){
					return 0;
				}
				int x = LOWORD(lParam);
				int y = HIWORD(lParam);
				//printf("%s %d x:%d y:%d\r\n",__func__,__LINE__,x,y);
				screen->mouse_status = SCREEN_MOUSE_STATUS_DOWN;
				//gettimeofday(&screen->tv,NULL);
				screen->buttondown_coor.x = x;
				screen->buttondown_coor.y = y;
				screen->mousemove_coor.x = x;
				screen->mousemove_coor.y = y;
				memset(screen->movelen,0,sizeof(screen->movelen));
				screen->movelen_index = 0;
				screen->last_move_tick = screen_timer_tick;
				screen->buttondown_tick = screen_timer_tick;
				screen->longpress_flag = 0;

				screen->mv_start = 0;

				mv_info.coor.x = x;
				mv_info.coor.y = y;



				pthread_mutex_lock(&screen->mutex);

				screen_focus_check(screen,SCREEN_FORCE_FLAG_BUTTONDOWN,&mv_info.coor);
				if(screen->focus_node){
					WIDGET_TOUCH_EVENT_HANDLE(screen->focus_node,SCREEN_FORCE_FLAG_BUTTONDOWN,&mv_info);
				}

				pthread_mutex_unlock(&screen->mutex);
			}
			return 0;
		case MSG_LBUTTONUP:
			{
				//GUI_DEBUG(DEBUG_NORMAL,"screenProc MSG_LBUTTONUP speed:%d %d\n",screen->move_speedx,screen->move_speedy);
				screen->buttondown_node = NULL;
				if(screen->node == NULL){
					return 0;
				}
				int x = LOWORD(lParam);
				int y = HIWORD(lParam);
				screen->buttondup_coor.x = x;
				screen->buttondup_coor.y = y;
				screen->mouse_status = SCREEN_MOUSE_STATUS_UP;

				mv_info.coor.x = x;
				mv_info.coor.y = y;
				mv_info.lengthx = x - screen->mousemove_coor.x;
				mv_info.lengthy = y - screen->mousemove_coor.y;

				// 计算抬起时的滑动速度
				int ticks = screen_timer_tick - screen->last_move_tick;
				
				_screen_calc_speed(screen,&mv_info,ticks);


				int ret = 0;
				pthread_mutex_lock(&screen->mutex);
				if(screen->focus_node){
					struct gui_coordinate cr;
					cr = screen->buttondown_coor;
					if(point_in_widget_check_2(screen->focus_node,&cr)){
						ret = WIDGET_TOUCH_EVENT_HANDLE(screen->focus_node,SCREEN_FORCE_FLAG_CLICK|SCREEN_FORCE_FLAG_BUTTONUP,&mv_info);
					}else{
						printf("%s %d focusnode up not:%s\r\n",__func__,__LINE__,screen->focus_node->name);
					}
					screen->focus_node->opt->release_focus(screen->focus_node);
					screen->focus_node = NULL;
				}

				if(ret == 0){
					screen_focus_check(screen,SCREEN_FORCE_FLAG_BUTTONUP,&mv_info.coor);
					if(screen->focus_node){
						WIDGET_TOUCH_EVENT_HANDLE(screen->focus_node,SCREEN_FORCE_FLAG_BUTTONUP,&mv_info);
					}
				}

				pthread_mutex_unlock(&screen->mutex);
			}
			return 0;
		case MSG_MOUSEMOVE:
			{
				int x = LOWORD(lParam);
				int y = HIWORD(lParam);
				//GUI_DEBUG("screenProc MSG_MOUSEMOVE x:%d y:%d mouse_status:%d\n",x,y, screen->mouse_status);
				if(screen->mouse_status != SCREEN_MOUSE_STATUS_DOWN){	// 检测是否按下
					return 0;
				}

				int ticks = screen_timer_tick - screen->last_move_tick;
				if(ticks <= 0){		// 限制move事件周期必须大于页面刷新周期
					return 0;
				}

				// 检测滑动范围， 如果小于2 pixel则判定为无效事件 不处理
				if(POINT_RANGE_CHECK(x,y,screen->mousemove_coor.x,screen->mousemove_coor.y,3)){
					return 0;
				}

				mv_info.coor.x = x;
				mv_info.coor.y = y;

				// 检测滑动事件是否在焦点控件中， 如果不在给焦点控件发送MOVEOUT事件
				pthread_mutex_lock(&screen->mutex);
				if(screen->focus_node){
					struct gui_coordinate cr;
					cr = mv_info.coor;
					if(point_in_widget_check_2(screen->focus_node,&cr)){
						WIDGET_TOUCH_EVENT_HANDLE(screen->focus_node,SCREEN_FORCE_FLAG_MOVEOUT,&mv_info);
					}
				}
				pthread_mutex_unlock(&screen->mutex);


				screen->mv_start = 1;
				int move_lengthx = x - screen->mousemove_coor.x;
				int move_lengthy = y - screen->mousemove_coor.y;
				screen->mousemove_coor.x = x;
				screen->mousemove_coor.y = y;


				unsigned int flag = 0;
				if(move_lengthx > 2 || move_lengthx < -2){
					flag |= SCREEN_FORCE_FLAG_MOVEX;
				}

				if(move_lengthy > 2 || move_lengthy < -2){
					flag |= SCREEN_FORCE_FLAG_MOVEY;
				}


				mv_info.lengthx = move_lengthx;
				mv_info.lengthy = move_lengthy;
				mv_info.coor.x = x;
				mv_info.coor.y = y;

				_screen_calc_speed(screen,&mv_info,ticks);

				int ret = 0;
				if(screen->focus_node){
					ret = WIDGET_TOUCH_EVENT_HANDLE(screen->focus_node,flag,&mv_info);
				}

				if(ret == 0){
					screen_focus_check(screen,flag,&mv_info.coor);
					if(screen->focus_node){
						WIDGET_TOUCH_EVENT_HANDLE(screen->focus_node,flag,&mv_info);
					}
				}

				screen->last_move_tick = screen_timer_tick;
			}
			return 0;
		case MSG_USER_KEY:
			{
				char *key = (char*)lParam;

				printf("MSG_USER_KEY key:%d\n",key[0]);
				pthread_mutex_lock(&screen->mutex);
				if(screen->keyboard_focus_node){
					screen->node->opt->keyboard_event_handle(screen->node,key);		
				}

				widget_free(key);
				pthread_mutex_unlock(&screen->mutex);

				return 0;
			}
		case MSG_DESTROY:
			return 0;
		case MSG_CLOSE:
			DestroyMainWindow(hwnd);
			PostQuitMessage(hwnd);
			return 0;
	}
	return DefaultMainWinProc(hwnd, message, wParam, lParam);
}

int screen_start_node(mwidget *node, Screen *screen)
{
	if(node == NULL){
		return -1;
	}

	node->screen = screen;
	if(node->parent != NULL){
		node->deep = node->parent->deep + 1;
		node->screen_level = node->parent->screen_level;
	}

	if(node->opt->start != NULL){
		node->opt->start(node);
	}

	if(node->child != NULL){
		screen_start_node(node->child,screen);
	}
	if(node->next != NULL){
		screen_start_node(node->next,screen);
	}

	return 0;
}

int screen_stop_node(mwidget *node, Screen *screen)
{
	if(node == NULL){
		return -1;
	}

	if(node->screen != NULL){
		remove_node_from_screen(node);
	}

	node->screen = NULL;
	if(node->opt->stop){
		node->opt->stop(node);
	}

	if(node->child != NULL){
		screen_stop_node(node->child,screen);
	}

	if(node->next != NULL){
		screen_stop_node(node->next,screen);
	}

	return 0;
}

int add_node_to_screen(mwidget *newnode)
{
	Screen *screen = &g_screen;
	if(screen->node == newnode){
		GUI_ERR("Newnode is same of screen node");
		return -1;
	}
	newnode->screen = screen;
	newnode->deep = 1;
	newnode->screen_level = SCREEN_LEVEL_NODE;
	mwidget *oldnode = screen->node;
	pthread_mutex_lock(&screen->mutex);
	screen_start_node(newnode,screen);
	screen->focus_node = NULL;
	screen->node = newnode;
	pthread_mutex_unlock(&screen->mutex);

	//gui_node_show(screen->node);
	gui_node_paint_enable(screen->node,1);

	if(oldnode){
		screen_stop_node(oldnode,screen);
	}

	return 0;
}

int add_node_to_toollayer(mwidget *newnode)
{
	Screen *screen = &g_screen;
	if(screen->tool_layer == newnode){
		GUI_ERR("Newnode is same of screen node");
		return -1;
	}
	newnode->screen = screen;
	newnode->deep = 1;
	newnode->screen_level = SCREEN_LEVEL_TOOL_LAYER;
	mwidget *oldnode = screen->tool_layer;
	pthread_mutex_lock(&screen->mutex);
	screen_start_node(newnode,screen);
	screen->tool_layer = newnode;
	pthread_mutex_unlock(&screen->mutex);

	gui_node_paint_enable(screen->tool_layer,1);

	if(oldnode){
		screen_stop_node(oldnode,screen);
	}

	return 0;
}

int add_node_to_screen_suspention(mwidget *node, Screen *screen)
{
	node->screen = screen;
	node->screen_level = SCREEN_LEVEL_SUSPENTION;
	screen_start_node(node,screen);
	//pthread_mutex_lock(&screen->mutex);
	//lock_flag = pthread_mutex_trylock(&screen->mutex);
	if(screen->suspention != NULL){
		node->next = screen->suspention;
	}
	screen->suspention = node;

	//gui_node_show(screen->suspention);
	//pthread_mutex_unlock(&screen->mutex);
	//if(lock_flag==0){
		//pthread_mutex_unlock(&screen->mutex);
	//}

	gui_node_paint_enable(node,2);

	return 0;
}

mwidget *remove_node_from_screen_suspention(mwidget *node, Screen *screen)
{

	pthread_mutex_lock(&screen->mutex);
	mwidget *next = screen->suspention,*prev = NULL;;
	while(next){
		if(next == node){
			if(prev == NULL){
				screen->suspention = node->next;
			}else{
				prev->next = node->next;
			}
			node->next = NULL;
			node->screen = NULL;
			node->screen_level = SCREEN_LEVEL_NONE;
			break;
		}
		next = next->next;
	}
	//gui_node_show(screen->suspention);
	pthread_mutex_unlock(&screen->mutex);
	return NULL;

}

int remove_node_from_screen(mwidget *node)
{
	if(node == NULL ){
		return -1;
	}

	Screen *screen = &g_screen;

#if 0
	mwidget *tmp, *prev = NULL;
	switch(node->screen_level){
		case SCREEN_LEVEL_NODE:
			tmp = screen->node;
			while(tmp){
				if(tmp == node){
					if(tmp == screen->node){
						screen->node = tmp->next;
					}else{
						prev->next = tmp->next;
					}
					tmp->next = NULL;
					tmp->screen = NULL;
					tmp->screen_level = SCREEN_LEVEL_NONE;
				}
				prev = tmp;
				tmp = tmp->next;
			}
			break;
		case SCREEN_LEVEL_SUSPENTION:
			tmp = screen->suspention;
			while(tmp){
				if(tmp == node){
					if(tmp == screen->suspention){
						screen->suspention = tmp->next;
					}else{
						prev->next = tmp->next;
					}
					tmp->next = NULL;
					tmp->screen = NULL;
					tmp->screen_level = SCREEN_LEVEL_NONE;
				}
				prev = tmp;
				tmp = tmp->next;
			}
			break;
	}
#endif

	pthread_mutex_lock(&screen->mutex);
	switch(node->screen_level){
		case SCREEN_LEVEL_NODE:
			if(screen->node == node){
				screen->node = NULL;
			}
			break;
		case SCREEN_LEVEL_TOOL_LAYER:
			if(screen->tool_layer == node){
				screen->tool_layer = NULL;
			}
			break;
		case SCREEN_LEVEL_SUSPENTION:
			if(screen->suspention == NULL){
				screen->suspention = NULL;
			}
			break;
	}
	pthread_mutex_unlock(&screen->mutex);


	return 0;
}

Screen *uni_get_gui_node_screen(mwidget *node)
{
	if(node == NULL){
		return NULL;
	}

	mwidget *parent = node;

	while(parent->parent != NULL){
		parent = parent->parent;
	}
	return parent->screen;
}

HDC screen_create_hdc(int w, int h)
{
	HDC phdc = GetClientDC(g_screen.hwnd);
	HDC hdc;
	hdc = CreateCompatibleDCEx(phdc,w,h);
	ReleaseDC(phdc);
	
	return hdc;
}

void screen_destroy_hdc(HDC hdc)
{
	DeleteMemDC(hdc);
}


int screen_register_widget_event_handle( int (*widget_event_handle_cbk)(widget_event_t *event,void *param),void *param)
{
	g_screen.widget_event_handle_cbk = widget_event_handle_cbk;
	g_screen.widget_event_handle_param = param;
	return 0;
}

screen_widget_event_cbk_t *screen_register_widget_event_cbk( int (*widget_event_handle_cbk)(widget_event_t *event,void *param),void *param,screen_event_cbk_priority_e priority)
{
	screen_widget_event_cbk_t *sw_cbk = widget_calloc(1,sizeof(*sw_cbk));	
	sw_cbk->cbk = widget_event_handle_cbk;
	sw_cbk->param = param;
	sw_cbk->priority = priority;

	screen_widget_event_cbk_t *tmp_cbk = orb_list_get_first(g_screen.widget_event_cbk_list);
	while(tmp_cbk){
		if(priority >= tmp_cbk->priority){
			orb_list_insert_prev(g_screen.widget_event_cbk_list,sw_cbk,tmp_cbk);
			break;
		}
		tmp_cbk = orb_list_get_next(g_screen.widget_event_cbk_list);
	}

	if(tmp_cbk == NULL){
		orb_list_add(g_screen.widget_event_cbk_list,sw_cbk);
	}

	return sw_cbk;
}

int screen_unreister_widget_event_cbk(screen_widget_event_cbk_t *cbk)
{
	orb_list_del(g_screen.widget_event_cbk_list,cbk);
	return 0;
}

int screen_add_xmlwidget(struct _xmlwidget_widget *m)
{
	if(m == NULL){
		return -1;
	}

	orb_list_add(g_screen.xmlwidget_list,m);
	return 0;
}

int screen_del_xmlwidget(struct _xmlwidget_widget *m)
{
	if(m == NULL){
		return -1;
	}

	orb_list_add(g_screen.xmlwidget_list,m);
	return 0;
}



