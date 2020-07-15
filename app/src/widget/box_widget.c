
#include "box_widget.h"

static int box_check_rotate_scale_need(mbox* m){
	
	int rotate_scale_need = BOX_NOT_NEED_ROTATE_SCALE;

	//need roate
	if (m->rotate % 360 != 0){
		rotate_scale_need = BOX_NEED_ROTATE_SCALE;
	}

	//width need scale
	if ((m->scale_width >= 0) && (m->scale_width != m->w)){
		rotate_scale_need = BOX_NEED_ROTATE_SCALE;
	}

	//height need scale
	if ((m->scale_height >= 0) && (m->scale_height != m->w)){
		rotate_scale_need = BOX_NEED_ROTATE_SCALE;
	}

	//GUI_DEBUG("box_check_rotate_scale_need rotate = %d scale_width = %d scale_height = %d rotate_scale_need:%d\n", m->rotate, m->scale_width, m->scale_height,rotate_scale_need);

	return rotate_scale_need;
}


static int box_paint(mwidget *node,HDC hdc)
{
	mbox *m = (mbox*)node;
	
	if(node->need_paint || m->hdc == NULL){
		if(m->hdc){
			screen_destroy_hdc(m->hdc);
		}
		m->hdc = screen_create_hdc(node->w,node->h);
		node->need_paint = 0;
		gui_node_paint(m->child,m->hdc);
	}

 
	if (BOX_NEED_ROTATE_SCALE == box_check_rotate_scale_need(m)){
		BITMAP *tmp_bitmap = NULL;
		tmp_bitmap = (BITMAP*)widget_calloc(1,sizeof(BITMAP));

		GetBitmapFromDC(m->hdc, 0, 0,m->w, m->h, tmp_bitmap);

		//使用缩放矩形对角线为边长绘制矩形hdc,如此矩形不管什么角度都可以显示完全
		int tmp_width = sqrt(m->scale_width * m->scale_width + m->scale_height * m->scale_height) + 1;
		HDC tmp_hdc = screen_create_hdc(tmp_width,tmp_width);

		//矩形与矩形HDC的中心点是重合的，计算原矩形相对于矩形HDC的x,y坐标
		int sx = (tmp_width - m->w) / 2;
		int sy = (tmp_width - m->h) / 2;

		RotateScaledBitmap(tmp_hdc, tmp_bitmap, sx, sy, m->rotate * BOX_ROTATION_ANGLE_DEGREE, m->scale_width, m->scale_height);

		if(m->alpha_flag){
			SetMemDCAlpha(tmp_hdc,MEMDC_FLAG_SRCALPHA,m->alpha);
		}

		//计算矩形HDC相对于控件原显示位置的对应显示位置
		int show_x = node->show_abs_x - sx;
		int show_y = node->show_abs_y - sy;

		BitBlt(tmp_hdc,0,0,tmp_width,tmp_width,hdc,show_x,show_y,0);

		if(tmp_hdc){
			screen_destroy_hdc(tmp_hdc);
		}

		unloadBitMap(tmp_bitmap);
		widget_free(tmp_bitmap);
	
	}else{
		if(node->show_abs_x >= 0){
			int sx = node->show_abs_x - node->start_abs_x;
			int sy = node->show_abs_y - node->start_abs_y;
			if(m->alpha_flag){
				SetMemDCAlpha(m->hdc,MEMDC_FLAG_SRCALPHA,m->alpha);
			}
			
			BitBlt(m->hdc,sx,sy,node->show_w,node->show_h,hdc,node->show_abs_x,node->show_abs_y,0);
		}
	}

	if(m->auto_rotate){
		m->rotate += m->rotate_speed;
		if(m->rotate >= 360){
			m->rotate = 0;
		}

		//printf("auto_rotate = %d rotate_speed = %d rotate = %d\n", m->auto_rotate, m->rotate_speed, m->rotate);
		m->opt->refresh(m);
	}


	return 0;
}


int box_set_alpha(mbox *m, int enable, unsigned char alpha)
{
	m->alpha_flag = enable;
	m->alpha = alpha;

	m->opt->refresh(m);
	return 0;
}

int box_alpha_set(mbox *m, char *value)
{
	unsigned char alpha = strtol(value,NULL,0);
	if(alpha == 0xff){
		m->alpha_flag = 0;
	}else{
		m->alpha_flag = 1;
	}
	m->alpha = alpha;

	m->opt->refresh(m);
	return 0;
}

char *box_alpha_get(mbox *m)
{
	char *buf = widget_calloc(1,6);
	if(m->alpha_flag == 0){
		sprintf(buf,"0xff");
	}else{
		sprintf(buf,"0x%02x",m->alpha);
	}
	return buf;
}

int box_set_rotate(mbox *m, int rotate)
{
	m->rotate = rotate;

	m->opt->refresh(m);
	return 0;
}


int box_rotate_set(mbox *m, char *value){
	if (NULL == value){
		GUI_ERR("box rotate set failed, param is NULL\n");
		return -1;
	}

	m->rotate = atoi(value);

	m->opt->refresh(m);
}

char *box_rotate_get(mbox *m){
	char *buf = widget_calloc(1,12);
	sprintf(buf, "%d", m->rotate);

	return buf;
}

int box_set_scale(mbox *m, int scale_width, int scale_height)
{
	GUI_DEBUG("box_set_scale# scale_width:%d scale_height:%d\n", scale_width, scale_height);
	m->scale_width = scale_width;
	m->scale_height = scale_height;

	m->opt->refresh(m);
	return 0;
}


int box_scale_set(mbox *m, char *value){
	if (NULL == value){
		GUI_ERR("box scale set failed, param is NULL\n");
		return -1;
	}

	char *tmp = value;
	

	m->scale_width = atoi(tmp);
	tmp = strchr(tmp, ",");
	if (NULL == tmp){
		GUI_ERR("box scale height is NULL, value=%s\n", value);
		return -1;
	}
	tmp++;
	
	m->scale_height = atoi(tmp);

	//GUI_ERR("box_set_scale# scale_width:%d scale_height:%d\n", m->scale_width, m->scale_height);


	m->opt->refresh(m);
	return 0;
}

char *box_scale_get(mbox *m){
	char *buf = widget_calloc(1,24);
	sprintf(buf, "%d,%d", m->scale_width, m->scale_height);

	return buf;
}



int box_set_auto_rotate(mbox *m, int enable, int rotate_speed)
{
	m->auto_rotate = enable;
	m->rotate_speed = rotate_speed;

	m->opt->refresh((mwidget*)m);
	return 0;
}



int box_auto_rotate_set(mbox *m, char *value){
	if (NULL == value){
		GUI_ERR("box auto rotate set failed, param is NULL\n");
		return -1;
	}

	char *tmp = value;

	m->auto_rotate = atoi(tmp);
	tmp = strchr(tmp, ",");
	if (NULL == tmp){
		GUI_ERR("box rotate speed is NULL, value=%s\n", value);
		return -1;
	}
	tmp++;
	
	m->rotate_speed = atoi(tmp);

	m->opt->refresh(m);
	return 0;
}

char *box_auto_rotate_get(mbox *m){
	char *buf = widget_calloc(1,24);
	sprintf(buf, "%d,%d", m->auto_rotate, m->rotate_speed);

	return buf;
}

void box_destroy(mwidget *node,widget_opt *clas)
{
	mbox *m = (mbox*)node;
	if(m->hdc){
		screen_destroy_hdc(m->hdc);
		m->hdc = NULL;
	}

	clas->parent->destroy(node,clas->parent);
}

_WIDGET_GENERATE(box,widget)

