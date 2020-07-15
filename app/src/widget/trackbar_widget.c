#include "trackbar_widget.h"
#include "screen.h"

void calculateTrackBarPosition(mtrackbar *node, HDC hdc)
{
	if(node == NULL){
		return -1;
	}
	trackbarData *trackbar = node->trackbar;
	int leftX = trackbar->barCenter - (trackbar->barW>>1);
	if(trackbar->axis == 0){
		trackbar->barX = trackbar->barCenter - (trackbar->barW>>1);
		trackbar->barX = trackbar->barX > node->start_abs_x ? trackbar->barX: node->start_abs_x;	
		trackbar->barY = node->start_abs_x + node->h>>1;
	}
	else{
		trackbar->barY = trackbar->barCenter - (trackbar->barH>>1);
		trackbar->barY = trackbar->barY > node->start_abs_y ? trackbar->barY: node->start_abs_y;	
		trackbar->barX = node->start_abs_x + node->w>>1;
	}
}

static int trackbar_node_paint(mwidget *node,HDC hdc)
{
	if(node == NULL){
		return -1;
	}
	mtrackbar *m = (mtrackbar*)node;
	trackbarData *trackbar = m->trackbar;
	if(trackbar == NULL){
		return -1;
	}
	
	int bmp_x = node->show_abs_x - node->start_abs_x;
	int bmp_y = node->show_abs_y - node->start_abs_y;
	node->opt->lock_data(node);

	calculateTrackBarPosition(m, hdc);
	if(trackbar->trackBitMap != NULL && trackbar->barBitMap != NULL){
		FillBoxWithBitmapPart(hdc, node->show_abs_x, node->show_abs_y,
				node->show_w, node->show_h, node->w, node->h, trackbar->trackBitMap, bmp_x, bmp_y);
		printf("[%s %d] barX: %d, barY: %d, barW: %d, barH: %d\n", __func__, __LINE__, trackbar->barX, trackbar->barY, trackbar->barW, trackbar->barH);
		FillBoxWithBitmapPart(hdc, trackbar->barX, trackbar->barY,
				trackbar->barW, trackbar->barH, trackbar->barW, trackbar->barH, trackbar->barBitMap, 0, 0);
	}	

end:
	node->opt->unlock_data(node);

	return 0;
}

void trackbarDataFree(trackbarData **data)
{
	if(data == NULL || *data == NULL){
		return;
	}
	trackbarData *d = *data;
	if(d->trackPath){
		widget_free(d->trackPath);
		d->trackPath = NULL;
	}
	if(d->barPath){
		widget_free(d->barPath);
		d->barPath = NULL;
	}
	if(d->trackBitMap){
		unloadBitMap(d->trackBitMap);
		widget_free(d->trackBitMap);
		d->trackBitMap = NULL;
	}
	if(d->barBitMap){
		unloadBitMap(d->barBitMap);
		widget_free(d->barBitMap);
		d->barBitMap = NULL;
	}
	free(d);
	*data = NULL;
}

static void trackbar_destroy(mwidget *node,widget_opt *clas)
{
	mtrackbar *m = (mtrackbar*)node;

	m->opt->lock_data(node);
	if(m->trackbar){
		trackbarDataFree(&m->trackbar);
	}
	m->opt->unlock_data(node);
	clas->parent->destroy(node,clas->parent);
}

int trackbar_touch_event_handle(struct _widget *node, unsigned int flag, struct move_info *coor)
{
	printf("[%s %d]trackbar get x: %d, y: %d, flag: %x\n", __func__, __LINE__, coor->coor.x, coor->coor.y, flag);
//	if(flag & SCREEN_FORCE_FLAG_BUTTONDOWN || flag & SCREEN_FORCE_FLAG_MOVEX || flag & SCREEN_FORCE_FLAG_MOVEY){
		mtrackbar *m = (mtrackbar*)node;
		trackbarData *trackbar = m->trackbar;
		trackbar->barCenter = trackbar->axis == 0 ? coor->coor.x: coor->coor.y;		
		printf("[%s %d]center: %d\n", __func__, __LINE__, trackbar->barCenter);
		m->opt->refresh(m);
//	}
	
	return 0;
}

mwidget *trackbar_get_focus(mwidget *node,unsigned int flag, struct gui_coordinate *coor)
{
	node->focus_lock = 1;
	return node;	
}

BITMAP *bitmap_load(const char *path)
{
	BITMAP *map = (BITMAP*)widget_calloc(1,sizeof(BITMAP));
	if(map == NULL){
		return NULL;
	}
	int err_code = LoadBitmapFromFile(HDC_SCREEN,map,path);
	if (err_code != ERR_BMP_OK) {
		GUI_ERR("load %s bitmap failed\n",path);
		widget_free(map);
		return NULL;
	}
	return map;
}

int trackbar_widget_start(mwidget *node)
{
	if(node->screen == NULL){
		return 0;
	}

	mtrackbar *m = (mtrackbar*)node;

	trackbarData *data = m->trackbar;
	
	node->opt->lock_data((uni_gui_node*)node);
	if(data->trackBitMap){
		unloadBitMap(data->trackBitMap);
		widget_free(data->trackBitMap);
	}
	data->trackBitMap = bitmap_load(data->trackPath);
	
	if(data->barBitMap){
		unloadBitMap(data->barBitMap);
		widget_free(data->barBitMap);
	}
	data->barBitMap = bitmap_load(data->barPath);

	node->opt->unlock_data((uni_gui_node*)node);

	m->opt->refresh(node);
	return 0;
}

int trackbar_widget_stop(mwidget *node)
{
	if(node == NULL){
		return -1;
	}

	mtrackbar *m = (mtrackbar*)node;
	node->opt->lock_data(node);
	if(m->trackbar->trackBitMap){
		unloadBitMap(m->trackbar->trackBitMap);
		widget_free(m->trackbar->trackBitMap);
		m->trackbar->trackBitMap = NULL;
	}
	if(m->trackbar->barBitMap){
		unloadBitMap(m->trackbar->barBitMap);
		widget_free(m->trackbar->barBitMap);
		m->trackbar->barBitMap = NULL;
	}
	
	node->opt->unlock_data(node);
	return 0;
}

int trackbar_level_set(mtrackbar *m, char *value)
{
	if(m == NULL || value == NULL){
		return -1;
	}

	m->opt->lock_data((mwidget*)m);
	m->trackbar->level = atoi(value);
	m->opt->unlock_data((mwidget*)m);

	return 0;
}

char *trackbar_level_get(mtrackbar *m)
{
	if(m == NULL || m->trackbar == NULL){
		return NULL;
	}

	m->opt->lock_data(m);
	char *buf = widget_calloc(1,strlen(m->trackbar->level) + 1);
	sprintf(buf, "%d", m->trackbar->level);
	m->opt->unlock_data(m);
	return buf;
}

int trackbar_trackImage_set(mtrackbar *m, char *value)
{
	if(m == NULL || value == NULL){
		return -1;
	}

	m->opt->lock_data((mwidget*)m);
	if(m->trackbar->trackPath){
		widget_free(m->trackbar->trackPath);
	}
	m->trackbar->trackPath = (char *)widget_calloc(1, strlen(value)+1);
	strcpy(m->trackbar->trackPath,value);
	if(m->trackbar->trackBitMap){
		unloadBitMap(m->trackbar->trackBitMap);
		widget_free(m->trackbar->trackBitMap);
	}
	m->trackbar->trackBitMap = bitmap_load(value);
	m->opt->unlock_data((mwidget*)m);
	m->opt->refresh(m);

	return 0;
}

char *trackbar_trackImage_get(mtrackbar *m)
{
	if(m == NULL || m->trackbar == NULL){
		return NULL;
	}

	m->opt->lock_data(m);
	char *buf = widget_calloc(1,strlen(m->trackbar->trackPath) + 1);
	strcpy(buf, m->trackbar->trackPath);
	m->opt->unlock_data(m);
	return buf;
}

int trackbar_barImage_set(mtrackbar *m, char *value)
{
	if(m == NULL || value == NULL){
		return -1;
	}

	m->opt->lock_data((mwidget*)m);
	if(m->trackbar->barPath){
		widget_free(m->trackbar->barPath);
	}
	m->trackbar->barPath = (char *)widget_calloc(1, strlen(value)+1);
	strcpy(m->trackbar->barPath,value);
	if(m->trackbar->barBitMap){
		unloadBitMap(m->trackbar->barBitMap);
		widget_free(m->trackbar->barBitMap);
	}
	m->trackbar->barBitMap = bitmap_load(value);
	m->opt->unlock_data((mwidget*)m);
	m->opt->refresh(m);

	return 0;
}

char *trackbar_barImage_get(mtrackbar *m)
{
	if(m == NULL || m->trackbar == NULL){
		return NULL;
	}

	m->opt->lock_data(m);
	char *buf = widget_calloc(1,strlen(m->trackbar->barPath) + 1);
	strcpy(buf, m->trackbar->barPath);
	m->opt->unlock_data(m);
	return buf;
}

int trackbar_axis_set(mtrackbar *m, char *value)
{
	if(m == NULL || value == NULL){
		return -1;
	}

	m->opt->lock_data((mwidget*)m);
	m->trackbar->axis = atoi(value);
	m->opt->unlock_data((mwidget*)m);

	m->opt->refresh(m);
	return 0;
}

char *trackbar_axis_get(mtrackbar *m)
{
	if(m == NULL || m->trackbar == NULL){
		return NULL;
	}

	m->opt->lock_data(m);
	char *buf = widget_calloc(1,2);
	sprintf(buf, "%d", m->trackbar->axis);
	m->opt->unlock_data(m);
	return buf;
}

int trackbar_barWidth_set(mtrackbar *m, char *value)
{
	if(m == NULL || value == NULL){
		return -1;
	}

	m->opt->lock_data((mwidget*)m);
	m->trackbar->barW = atoi(value);
	m->opt->unlock_data((mwidget*)m);

	m->opt->refresh(m);
	return 0;
}

char *trackbar_barWidth_get(mtrackbar *m)
{
	if(m == NULL || m->trackbar == NULL){
		return NULL;
	}

	m->opt->lock_data(m);
	char *buf = widget_calloc(1,2);
	sprintf(buf, "%d", m->trackbar->barW);
	m->opt->unlock_data(m);
	return buf;
}

int trackbar_barHeight_set(mtrackbar *m, char *value)
{
	if(m == NULL || value == NULL){
		return -1;
	}

	m->opt->lock_data((mwidget*)m);
	m->trackbar->barH = atoi(value);
	m->opt->unlock_data((mwidget*)m);

	m->opt->refresh(m);
	return 0;
}

char *trackbar_barHeight_get(mtrackbar *m)
{
	if(m == NULL || m->trackbar == NULL){
		return NULL;
	}

	m->opt->lock_data(m);
	char *buf = widget_calloc(1,2);
	sprintf(buf, "%d", m->trackbar->barH);
	m->opt->unlock_data(m);
	return buf;
}

void create_trackbar_data(mtrackbar *m)
{
	trackbarData *data = (trackbarData *)widget_calloc(sizeof(trackbarData), 1);
	m->trackbar = data;
}

_WIDGET_GENERATE(trackbar,widget)
