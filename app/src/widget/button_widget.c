
#include "button_widget.h"

int button_node_paint(mwidget *node, HDC hdc,int x,int y)
{
	mbutton *m = (mbutton*)node;
	int bmp_x, bmp_y;
	buttonData *data = m->data;
	BITMAP *map;

	bmp_x = node->show_abs_x - node->start_abs_x;
	bmp_y = node->show_abs_y - node->start_abs_y;
	
	pthread_mutex_lock(&node->data_mutex);
	if(data != NULL){
		if(m->button_status == BUTTON_STATUS_UP){
			map = data->up_map;
		}else{
			map = data->down_map;
		}

		if(map != NULL && map->bmBits != NULL){
			FillBoxWithBitmapPart(hdc, node->show_abs_x, node->show_abs_y,
			node->show_w, node->show_h, node->w, node->h, map, bmp_x, bmp_y);
		}	
	}
	pthread_mutex_unlock(&node->data_mutex);
	
	return 0;
}

void button_release_focus(mwidget *node)
{
	mbutton *m = (mbutton*)node;
	m->button_status = BUTTON_STATUS_UP;
	m->opt->refresh(node);

	m->opt->parent->release_focus(node);
}

mwidget *button_get_focus(mwidget *node, unsigned int flag, struct gui_coordinate *coor)
{
	if(flag & SCREEN_FORCE_FLAG_BUTTONDOWN){
		return node;
	}

	return NULL;
}

int button_touch_event_forward(mwidget *node, unsigned int flag, struct move_info *coor){
	if ((NULL == node) || (NULL == coor)){
		GUI_ERR("button_touch_event_forward param is invlid\n");
		return -1;
	}
	mwidget *parent = node->parent;
	while(parent){
		if (0 == memcmp(parent->opt->class_name, "table", strlen("table"))){
			parent->opt->touch_event_handle(parent,flag,coor);
			break;
		}
		parent = parent->parent;
	}
	
	return 0;
}


int button_touch_event_handle(mwidget *node, unsigned int flag, struct move_info *coor)
{
	mbutton *m = (mbutton*)node;
	int ret = 0;

	if(	(flag & SCREEN_FORCE_FLAG_BUTTONDOWN) ){
		printf("%s %d\r\n",__func__,__LINE__);
		m->button_status = BUTTON_STATUS_DOWN;
		m->opt->refresh(node);
		ret = 1;
	}

	if(	(flag & SCREEN_FORCE_FLAG_BUTTONUP) ){
		m->button_status = BUTTON_STATUS_UP;
		m->opt->refresh(node);
		ret = 1;
		button_touch_event_forward(node, flag, coor);
	}

	if ((flag & SCREEN_FORCE_FLAG_LONGPRESS) || (flag & SCREEN_FORCE_FLAG_BUTTONUP)){
		button_touch_event_forward(node, flag, coor);
	}
	
	ret |= m->opt->parent->touch_event_handle(node,flag,coor);

	return ret;
}

static void button_data_destroy(buttonData *data)
{
	if(data == NULL){
		return;
	}

	data->refer_num--;
	if(data->refer_num <= 0){
		if(data->up_path != NULL){
			widget_free(data->up_path);
		}
		if(data->up_map != NULL){
			unloadBitMap(data->up_map);
			widget_free(data->up_map);
		}
		if(data->down_path != NULL){
			widget_free(data->down_path);
		}
		if(data->down_map != NULL){
			unloadBitMap(data->down_map);
			widget_free(data->down_map);
		}
		widget_free(data);
	}
}

static void button_destroy(mwidget *node,widget_opt *clas)
{
	if(node == NULL){
		return;
	}

	mbutton *m = (mbutton*)node;
	buttonData *data = m->data;

	m->opt->lock_data(m);
	if(data){
		button_data_destroy(m->data);
		m->data = NULL;
	}
	m->opt->unlock_data(m);

	clas->parent->destroy(node,clas->parent);
}

int button_widget_start(mwidget *node)
{
	if(node->screen == NULL){
		return 0;
	}

	mbutton *m = (mbutton*)node;
	if(m->data == NULL){
		return 0;
	}

	BITMAP *up_map = NULL;
	if(m->data->up_path){
		up_map = (BITMAP*)widget_calloc(1,sizeof(BITMAP));
		int err_code = LoadBitmapFromFile(HDC_SCREEN,up_map,m->data->up_path);
		if (err_code != ERR_BMP_OK) {
			widget_free(up_map);
			GUI_ERR("load %s bitmap failed\n",m->data->up_path);
			return -1;
		}
	}

	BITMAP *down_map = NULL;
	if(m->data->down_path){
		down_map = (BITMAP*)widget_calloc(1,sizeof(BITMAP));
		int err_code = LoadBitmapFromFile(HDC_SCREEN,down_map,m->data->down_path);
		if (err_code != ERR_BMP_OK) {
			GUI_ERR("load %s bitmap failed\n",m->data->up_path);
			UnloadBitmap(up_map);
			widget_free(up_map);
			widget_free(down_map);
			return -1;
		}
	}
	
	m->opt->lock_data((mwidget*)m);
	if(m->data->up_map){
		UnloadBitmap(m->data->up_map);
		widget_free(m->data->up_map);
	}
	if(m->data->down_map){
		UnloadBitmap(m->data->down_map);
		widget_free(m->data->down_map);
	}
	m->data->up_map = up_map;
	m->data->down_map = down_map;
	m->opt->unlock_data((mwidget*)m);
	m->opt->refresh((mwidget*)m);
	return 0;	
}

int button_widget_stop(mwidget *node)
{
	if(node == NULL){
		return -1;
	}

	mbutton *m = (mbutton*)node;

	m->opt->lock_data(m);
	if(m->data){
		if(m->data->down_map){
			unloadBitMap(m->data->down_map);
			widget_free(m->data->down_map);
			m->data->down_map = NULL;
		}

		if(m->data->up_map){
			unloadBitMap(m->data->up_map);
			widget_free(m->data->up_map);
			m->data->up_map = NULL;
		}
	}
	m->opt->unlock_data(m);

	return 0;
}

int button_up_image_set(mbutton *m, char *up_path)
{	
	if(m->data == NULL){
		m->data = widget_calloc(1,sizeof(buttonData));
	}
	if(m->data->up_path){
		widget_free(m->data->up_path);
	}

	m->data->up_path = widget_calloc(1,strlen(up_path) + 1);
	strcpy(m->data->up_path,up_path);
	m->opt->start((mwidget*)m);

	return 0;
}

int button_down_image_set(mbutton *m, char *down_path)
{
	if(m->data == NULL){
		m->data = widget_calloc(1,sizeof(buttonData));
	}
	if(m->data->down_path){
		widget_free(m->data->down_path);
	}

	m->data->down_path = widget_calloc(1,strlen(down_path) + 1);
	strcpy(m->data->down_path,down_path);
	m->opt->start((mwidget*)m);
	return 0;
}

char *button_down_image_get(mbutton *m)
{
	if(m->data == NULL || m->data->down_path == NULL){
		return NULL;
	}

	char *buf = widget_calloc(1,strlen(m->data->down_path) + 1);
	strcpy(buf,m->data->down_path);
	return buf;
	
}
char *button_up_image_get(mbutton *m)
{
	if(m->data == NULL || m->data->up_path == NULL){
		return NULL;
	}

	char *buf = widget_calloc(1,strlen(m->data->up_path) + 1);
	strcpy(buf,m->data->up_path);
	return buf;
}

static int button_image_set(mbutton *m, char *up_path, char *down_path)
{
	if(m == NULL){
		return -1;
	}

	button_up_image_set(m,up_path);
	button_down_image_set(m,down_path);

	return 0;
}

_WIDGET_GENERATE(button,widget)
