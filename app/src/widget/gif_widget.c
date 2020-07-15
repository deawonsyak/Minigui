
#include "gif_widget.h"
#include "screen.h"

static int gif_paint(mwidget *node,HDC hdc)
{
	mgif *m = (mgif*)node;
	node->opt->lock_data(node);
	if(m->gif == NULL){
		printf("gif is null\n");
		node->opt->unlock_data(node);
		return 0;
	}

	static int a = 0;
	if(a++ < 0){
		node->opt->unlock_data(node);
		return 0;
	}
	a = 0;

	ANIMATIONFRAME  *frame = NULL;
	frame = m->gif->get_current_frame(m->gif);
	if(frame == NULL){
		printf("frame is null\n");
		node->opt->unlock_data(node);
		m->opt->refresh(m);
		return 0;
	}
	if(node->show_abs_x >= 0){
		int sx = node->show_abs_x - node->start_abs_x;
		int sy = node->show_abs_y - node->start_abs_y;

		BitBlt(frame->mem_dc,0,0,frame->width,frame->height,m->hdc,frame->off_x,frame->off_y,0);
		//StretchBlt(frame->mem_dc,0,0,frame->width,frame->height,m->hdc,frame->off_x,frame->off_y,frame->width,frame->height,0);
		BitBlt(m->hdc,sx,sy,node->show_w,node->show_h,hdc,node->show_abs_x,node->show_abs_y,0);
		//BitBlt(frame->mem_dc,0,0,frame->width,frame->height,hdc,frame->off_x,frame->off_y,0);
	}

	node->opt->unlock_data(node);
	m->opt->refresh(m);
	return 0;
}

static void gif_destroy(mwidget *node,widget_opt *clas)
{
	mgif *m = (mgif*)node;

	if(node == NULL || node->screen){
		return;
	}

	gif_data_destroy(m->gif);
	m->gif = NULL;

	screen_destroy_hdc(m->hdc);
	m->hdc = NULL;

	if(m->path){
		widget_free(m->path);
		m->path = NULL;
	}
	
	clas->parent->destroy(node,clas);
}

int gif_set_path(mgif *m,char *path)
{
	if(m == NULL || path == NULL){
		return -1;
	}

	if(m->path){
		widget_free(m->path);
	}

	m->path = widget_calloc(1,strlen(path)+1);
	strcpy(m->path, path);

	if(m->gif){
		gif_data_destroy(m->gif);
		m->gif = NULL;
	}

	m->gif = CreateGifData(path,3);
	m->opt->refresh(m);


	return 0;

}


_WIDGET_GENERATE(gif,widget)
