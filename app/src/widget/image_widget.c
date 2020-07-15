
#include "image_widget.h"
#include "typeFace.h"
#include "gui_memory.h"

int image_node_paint(uni_gui_node *node, HDC hdc)
{
	int bmp_x, bmp_y;
	mimage *m = (mimage*)node;
	imageData *jpegInfo = &m->image_data;

	if(m->image_data.bkmap == NULL){
		return 0;
	}

	bmp_x = node->show_abs_x - node->start_abs_x;
	bmp_y = node->show_abs_y - node->start_abs_y;

	pthread_mutex_lock(&node->data_mutex);

	if(jpegInfo != NULL && jpegInfo->bkmap != NULL && jpegInfo->bkmap->bmBits != NULL){
		FillBoxWithBitmapPart(hdc, node->show_abs_x, node->show_abs_y,
				node->show_w, node->show_h, node->w, node->h, jpegInfo->bkmap, bmp_x, bmp_y);
	}	
	
	pthread_mutex_unlock(&node->data_mutex);
	
	return 0;
}

void imageData_delete(imageData *data)
{
	if(data){
		data->refer_num--;
		if(data->refer_num <= 0){
			if(data->path){
				widget_free(data->path);
				data->path = NULL;
			}
			if(data->bkmap){			
				unloadBitMap(data->bkmap);
				widget_free(data->bkmap);
				data->bkmap = NULL;
			}
		}
	}
}

void image_widget_destroy(mwidget *node,widget_opt *clas)
{
	mimage *m = (mimage*)node;

	m->opt->lock_data(m);
	imageData_delete(&m->image_data);
	m->opt->unlock_data(m);
		
	clas->parent->destroy(node,clas->parent);
	return;
}



int image_widget_start(mwidget *node)
{
	if(node->screen == NULL){
		return 0;
	}

	mimage *m = (mimage*)node;

	imageData *data = &m->image_data;
	BITMAP *map = (BITMAP*)widget_calloc(1,sizeof(BITMAP));
	if(map == NULL){
		return -1;
	}
	int err_code = LoadBitmapFromFile(HDC_SCREEN,map,data->path);
	if (err_code != ERR_BMP_OK) {
		GUI_ERR("load %s bitmap failed\n",data->path);
		widget_free(map);
		return -1;
	}

	node->opt->lock_data((uni_gui_node*)node);
	if(data->bkmap){
		unloadBitMap(data->bkmap);
		widget_free(data->bkmap);
	}
	data->bkmap = map;
	node->opt->unlock_data((uni_gui_node*)node);

	m->opt->refresh(node);
	return 0;
}

int image_widget_stop(mwidget *node)
{
	if(node == NULL){
		return -1;
	}

	mimage *m = (mimage*)node;
	node->opt->lock_data(node);
	if(m->image_data.bkmap){
		unloadBitMap(m->image_data.bkmap);
		widget_free(m->image_data.bkmap);
		m->image_data.bkmap = NULL;
	}
	node->opt->unlock_data(node);
	return 0;
}

int image_set_picture(mimage *node, char *path)
{
	if(path == NULL || node == NULL){
		GUI_ERR("path: %p,  param = %p", path,node);
		return -1;
	}
	imageData *data = &node->image_data;
	
	if(data->path){
		widget_free(data->path);
	}

	data->path = widget_calloc(strlen(path)+1,1);
	if(data->path == NULL){
		return -1;
	}
	strcpy(data->path,path);

	node->opt->start((mwidget*)node);

	return 0;
}

char *image_widget_get_picture(mimage *node)
{
	if(node->image_data.path == NULL){
		return NULL;
	}
	char *buf = widget_calloc(1,strlen(node->image_data.path)+1);
	strcpy(buf,node->image_data.path);
	return buf;
}

_WIDGET_GENERATE(image,widget)
