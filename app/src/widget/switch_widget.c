
#include "switch_widget.h"
#include "screen.h"

static void toggleswitch_update(mtoggleswitch *m)
{
	int x0,y0,x1,y1;

	if(m->hdc){
		screen_destroy_hdc(m->hdc);
		m->hdc = screen_create_hdc(m->w,m->h);
	}

#if 0
	SetPenWidth(m->hdc,3);
	SetPenColor(m->hdc,0xffffffff);
	x0 = m->h/2;
	y0 = 0;
	x1 = (m->w - m->h)+x0;
	y1 = 0;
	LineEx(m->hdc,x0,y0,x1,y1);
	SetPenWidth(m->hdc,2);
	ArcEx(m->hdc,x1-m->h/2,y1,m->h,m->h,270*64,180*64);
	x0 = x1;
	y0 = m->h;
	x1 = m->h/2;
	y1 = m->h;
	SetPenWidth(m->hdc,3);
	LineEx(m->hdc,x0,y0,x1,y1);
	SetPenWidth(m->hdc,2);
	ArcEx(m->hdc,0,0,m->h,m->h,90*64,180*64);
#endif

	if(m->button_status < 4){

		SetPenColor(m->hdc, 0xff505050);
		SetPenCapStyle(m->hdc, PT_CAP_ROUND);
		SetPenJoinStyle(m->hdc, PT_JOIN_ROUND);
		x0 = 13 + m->button_status*8;
		y0 = 16;
		int len = 16 - m->button_status*2;
		int width = 7-m->button_status;
		SetPenWidth(m->hdc,width);
		LineEx(m->hdc,x0,y0,x0+len,y0);
		FillBoxWithBitmap(m->hdc,0,0,m->w,m->h,m->on_map);
	}else{
		FillBoxWithBitmap(m->hdc,0,0,m->w,m->h,m->off_map);
	}

	if(m->button_status > 0){
		gal_pixel color = (0xFFA522) | 0xff000000;
		HGRAPHICS gpc = MGPlusGraphicCreateFromDC(m->hdc);
		HBRUSH brush = MGPlusBrushCreate (MP_BRUSH_TYPE_SOLIDCOLOR);
		MGPlusSetSolidBrushColor (brush, color);
		x0 = m->w - m->h/2;
		y0 = m->h/2;
		int r;
		if(m->button_status == 4){
			r = m->h/2-5;
		}else{
			r = (m->h/2-5)/4*m->button_status;

		}
		MGPlusFillEllipseI(gpc,	brush, x0,y0,r,r);
		MGPlusGraphicSave (gpc, m->hdc, 0, 0, 0, 0, 0, 0);
		MGPlusGraphicDelete (gpc);
		MGPlusBrushDelete(brush);

	}
}

int toggleswitch_paint(mwidget *node, HDC hdc)
{
	mtoggleswitch *m = (mtoggleswitch*)node;

	if(m->start_move){
		if(m->button_flag == 0){ // off
			m->button_status--;
			toggleswitch_update(m);
			if(m->button_status <= 0){
				m->start_move = 0;
			}else{
				m->opt->refresh(m);
			}
		}else{
			m->button_status++;
			toggleswitch_update(m);
			if(m->button_status >= 4){
				m->start_move = 0;
			}else{
				m->opt->refresh(m);
			}
		}
	}

	return m->opt->parent->paint(m,hdc);
}

mwidget *toggleswitch_get_focus(mwidget *node, unsigned int flag, struct gui_coordinate *coor)
{
	if(flag & SCREEN_FORCE_FLAG_BUTTONDOWN){
		return node;
	}
	return NULL;
}

int toggleswitch_touch_event_handle(mwidget *node, unsigned int flag, struct move_info *mv_info)
{
	mtoggleswitch *m = (mtoggleswitch*)node;
	if(flag & SCREEN_FORCE_FLAG_CLICK){
		m->button_flag = !m->button_flag;
		m->start_move = 1;
		m->opt->refresh(m);
		m->opt->event_send(node,"onClick");
		return 1;
	}

	return 0;
}

_WIDGET_GENERATE(toggleswitch,svg)
