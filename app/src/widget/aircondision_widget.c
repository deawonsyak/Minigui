
#include "aircondision_widget.h"
#include "screen.h"

int aircondision_paint(mwidget *node, HDC hdc)
{

	return 0;
}

int aircondision_set_select_color(maircondision *m, unsigned int rgb)
{
	m->select_color = rgb;

	m->opt->refresh(m);
	return 0;
}

void aircondision_update(maircondision *m)
{
	if(m->hdc){
		screen_destroy_hdc(m->hdc);
		m->hdc = screen_create_hdc(m->w,m->h);
	}
	//printf("%s %d index:%d\r\n",__func__,__LINE__,m->select_index);
	m->opt->graphicClear(m);
	m->opt->setPenWidth(m,2);
	int step_pixel = (m->w-10) / (m->steps);
	int i;
	for(i = 0; i < m->steps; i++){
		int x0 = (i+1)*step_pixel;
		int y0 = 15;

		if(i < m->select_index){
			m->opt->setPenColor(m,m->select_color,0xff);
		}else if(i == m->select_index){
			m->opt->setPenColor(m,m->select_color,0xff);
			m->opt->setSolidBrushColor(m,m->select_color,0xff);
			POINT point[5];
			point[0].x = x0 - 5;
			point[0].y = 0;
			point[1].x = x0 + 5;
			point[1].y = 0;
			point[2].x = x0;
			point[2].y = 10;
			m->opt->fillPolygon(m,point,3, 0);
		}else{
			m->opt->setPenColor(m,m->blank_color,0xff);
		}
		
		m->opt->line(m,x0,y0,x0,y0+40);
	}

	m->opt->graphicDisplay(m);
}

int aircondision_set_select_index(maircondision *m, short index)
{
	m->select_index = index;	

	aircondision_update(m);
	m->opt->refresh(m);

	return 0;
}

mwidget *aircondision_get_focus(mwidget *node, unsigned int flag, struct gui_coordinate *coor)
{
	if(flag & SCREEN_FORCE_FLAG_BUTTONDOWN){
		return node;
	}

	return NULL;
}

int aircondision_touch_event_handle(mwidget *node, unsigned int flag, struct move_info *mv_info)
{
	maircondision *m = (maircondision*)node;

	int index;

	POINT coor;
	coor.x = mv_info->coor.x;
	coor.y = mv_info->coor.y;

	int step_pixel = (m->w-2) / (m->steps-1);
	
	if(	(flag & SCREEN_FORCE_FLAG_BUTTONDOWN) ){
		index = (coor.x - node->start_abs_x)/step_pixel;
		if(index != m->select_index){
			if(index < m->steps && index >= 0){
				m->opt->set_select_index(m,index);
				m->opt->event_send(m,"selectIndex");
			}
		}
	}

	if(	(flag & SCREEN_FORCE_FLAG_MOVEX) ){
		index = (coor.x - node->start_abs_x)/step_pixel;
		if(index != m->select_index){
			if(index < m->steps && index >= 0){
				m->opt->set_select_index(m,index);
				m->opt->event_send(m,"selectIndex");
			}
		}
		return 1;
	}

	if(	(flag & SCREEN_FORCE_FLAG_BUTTONUP) ){

	}

	return 0;
}


_WIDGET_GENERATE(aircondision,svg)
