
#include "rect_widget.h"

static int rect_paint(mwidget *node,HDC hdc)
{
	mrect *m = (mrect*)node;
	m->opt->lock_data(node);
	if(m->show_abs_x != -1){
		unsigned int color = rgb2pixel(m->bgColor) | m->alpha<<24;
		SetBrushColor(hdc,color);
		if(m->is_roundrect){
			HDC mhdc = screen_create_hdc(m->w,m->h);
			SetBrushColor(mhdc,color);
			SetPenColor(mhdc,color);
			RoundRect(mhdc,0,0, m->w, m->h, m->x_radius, m->y_radius);
			int sx = node->show_abs_x - node->start_abs_x;
			int sy = node->show_abs_y - node->start_abs_y;
			StretchBlt(mhdc,sx,sy,node->show_w,node->show_h,hdc,node->show_abs_x,node->show_abs_y,node->show_w,node->show_h,0);
			screen_destroy_hdc(mhdc);
		}else if(m->is_center_shode){
			HDC mhdc = screen_create_hdc(m->w,m->h);
			SetBrushColor(mhdc,color);
			FillBox(mhdc,0,0,m->w,m->h);
			gal_pixel *pixels = widget_calloc(1, sizeof(gal_pixel)*m->shode_r);
			color_shode(m->shode_start_color,m->shode_stop_color, pixels,m->shode_r);
			int i;
			for(i = 0; i < m->shode_r; i++){
				gal_pixel px = rgb2pixel(pixels[i]) | 0xff000000;
				SetPenColor(mhdc,px);
				Circle(mhdc,m->shode_centerx,m->shode_centery,i);
				Circle(mhdc,m->shode_centerx-1,m->shode_centery,i);
			}

			int sx = node->show_abs_x - node->start_abs_x;
			int sy = node->show_abs_y - node->start_abs_y;
			StretchBlt(mhdc,sx,sy,node->show_w,node->show_h,hdc,node->show_abs_x,node->show_abs_y,node->show_w,node->show_h,0);

			widget_free(pixels);
			screen_destroy_hdc(mhdc);

		}else{
			FillBox(hdc,node->show_abs_x,node->show_abs_y,node->show_w,node->show_h);
		}
	}
	//printf("%s %d %s %d %d - %d %d hdc:%p\r\n",__func__,__LINE__,node->name,node->show_abs_x,node->show_abs_y,node->x, node->y,hdc);
	m->opt->unlock_data(node);
	return 0;
}

static void rect_destroy(mwidget *node)
{
	mrect *m = (mrect*)node;

}

static int rect_set_bkColor(mrect *m ,gal_pixel color,char alpha)
{
	if(m == NULL){
		return -1;
	}

	m->opt->lock_data(m);
	m->bgColor = color;
	m->alpha = alpha;
	m->opt->unlock_data(m);

	m->opt->refresh(m);
	return 0;
}

int rect_bgColor_set(mrect *m, char *value)
{
	if(value == NULL){
		return -1;
	}

	unsigned int color = strtol(value,NULL,0);
	m->opt->lock_data(m);
	m->bgColor = color & 0xffffff;
	m->opt->unlock_data(m);

	m->opt->refresh(m);
	return 0;
}

char *rect_bgColor_get(mrect *m)
{
	char *buf = widget_calloc(1,12);
	unsigned int color = (m->alpha << 24) | m->bgColor;
	sprintf(buf,"0x%08x",color);

	return buf;
}

int rect_alpha_set(mrect *m, void *value)
{
	if(m == NULL || value == NULL){
		return -1;
	}

	unsigned char alpha = strtol(value,NULL,0);
	m->opt->lock_data(m);
	m->alpha = alpha;
	m->opt->unlock_data(m);

	m->opt->refresh(m);

	return 0;
}

char *rect_alpha_get(mrect *m)
{
	char *buf = widget_calloc(1,12);
	sprintf(buf,"%d",m->alpha);
	return buf;
}

int rect_rounded_set(mrect *m, char *value)
{
	if(m == NULL || value == NULL){
		return -1;
	}

	char *tmp = value;
	int x_radius = strtol(tmp,NULL,0);

	tmp = strchr(tmp,',');
	if(tmp == NULL){
		GUI_ERR("widget rect propert param format error! param:%s",value)
		return -1;
	}
	tmp++;
	int y_radius = strtol(tmp,NULL,0);

	m->opt->lock_data(m);
	if(x_radius == 0 && y_radius == 0){
		m->is_roundrect = 0;
	}else{
		m->is_roundrect = 1;
		m->x_radius = x_radius;
		m->y_radius = y_radius;
	}

	m->opt->unlock_data(m);
	m->opt->refresh(m);

	return 0;
}

char *rect_rounded_get(mrect *m)
{
	if(m == NULL){
		return NULL;
	}

	char *buf = widget_calloc(1,32);
	if(m->is_roundrect){
		strcpy(buf,"0,0");
	}else{
		sprintf(buf,"%d,%d",m->x_radius,m->y_radius);
	}
	
	return buf;
}

int rect_set_center_shode(mrect *m, int enable, int centerx, int centery, int r, gal_pixel start_color, gal_pixel stop_color)
{
	m->is_center_shode = enable;
	m->shode_centerx = centerx;
	m->shode_centery = centery;
	m->shode_r = r;
	m->shode_start_color = start_color;
	m->shode_stop_color = stop_color;

	m->opt->refresh(m);

	return 0;
}

_WIDGET_GENERATE(rect,widget)

