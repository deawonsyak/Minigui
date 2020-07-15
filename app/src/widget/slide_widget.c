
#include "slide_widget.h"
#include "screen.h"

static int slide_cala_border(mwidget *node)
{
	if(node == NULL ){
		return -1;
	}
	mwidget *child = node->child;
	if(child == NULL){
		return 0;
	}
	while(child->is_delete == 1){
		child = child->next;
	}

	mslide *m = (mslide*)node;
	struct slideData *slide = &m->data;

	if(slide->first_child_init == 0){
		slide->first_child_location_x = child->x;
		slide->first_child_location_y = child->y;
		slide->first_child_init = 1;
	}

	slide->head_x = child->x;
	slide->head_y = child->y;

	slide->slide_lenx = 0;
	slide->slide_leny = 0;

	mwidget *prev = child;
	child = child->next;
	while(child != NULL){
		if(child->is_delete == 0){
			slide->slide_lenx += child->x;
			slide->slide_leny += child->y;
		}
		prev = child;
		child = child->next;
	}

	slide->slide_w = slide->slide_lenx + prev->w;
	slide->slide_h = slide->slide_leny + prev->h;

	// TODO 滑块小于slide节点的宽度时 怎么处理?
	slide->show_b_xe = 0 - (slide->slide_w - node->w);
	slide->show_b_ye = 0 - (slide->slide_h - node->h);
	slide->show_b_xf = slide->show_b_xe - node->w;
	slide->show_b_yf = slide->show_b_ye - node->h;



	if(slide->mv_dir & SLIDE_NODE_MOVE_DIR_X ){
		slide->end_x = slide->head_x - slide->slide_lenx;
	}
	if(slide->mv_dir & SLIDE_NODE_MOVE_DIR_Y){
		slide->end_y = slide->head_y - slide->slide_leny;
	}
	//GUI_DEBUG(DEBUG_INFO, "slide: head x: %d head y: %d, end x: %d, end y: %d, slide y: %d",slide->head_x, slide->head_y, slide->end_x, slide->end_y, slide->slide_leny);
	return 0;
}

static int slide_location_calc(mwidget *node,slideData *slide)
{
	int cx,cy;
	int endx,endy;

	int speedx = slide->speedx;
	int speedy = slide->speedy;

	char endflag = 0;

	//printf("%s %d startx %d %d\r\n",__func__,__LINE__,slide->total_lenx, slide->startx);

	endx = slide->startx + slide->total_lenx;
	endy = slide->starty + slide->total_leny;

	if (slide->button_status == 1) {	// 滑动中
		slide->active_status = 1;
		if(slide->mv_dir & SLIDE_NODE_MOVE_DIR_X){
			node->x = endx;
		}

		if(slide->mv_dir & SLIDE_NODE_MOVE_DIR_Y){
			node->y = endy;
		}

		if(slide->mv_dir & SLIDE_NODE_MOVE_DIR_X){
			if(slide->end_x < slide->head_x){
				if(node->x < slide->end_x){
					node->x = slide->end_x;
				}else if(node->x > slide->head_x){
					node->x = slide->head_x;
				}
			}else{
				if(node->x > slide->end_x){
					node->x = slide->end_x;
				}else if(node->x < slide->head_x){
					node->x = slide->head_x;
				}
			}
		}

		if(slide->mv_dir & SLIDE_NODE_MOVE_DIR_Y){
			if(slide->end_y < slide->head_y){
				if(node->y < slide->end_y){
					node->y = slide->end_y;
				}else if(node->y > slide->head_y){
					node->y = slide->head_y;
				}
			}else{
				if(node->y > slide->end_y){
					node->y = slide->end_y;
				}else if(node->y < slide->head_y){
					node->y = slide->head_y;
				}
			}
		}
		printf("%s %d y:%d\r\n",__func__,__LINE__,node->y);
	}else{		// 惯性移动

		if((slide->mv_dir & SLIDE_NODE_MOVE_DIR_X) &&
				speedx == 0){
			slide->active_status = 0;
			slide->total_lenx = 0;
			slide->total_leny = 0;
		}else if((slide->mv_dir & SLIDE_NODE_MOVE_DIR_Y) &&
				speedy == 0){
			slide->active_status = 0;
			slide->total_lenx = 0;
			slide->total_leny = 0;
		}

		int coordiate_x = speedx * 2;	// 单次移动长度
		int coordiate_y = speedy * 1;
		char endflagx = 0;
		char endflagy = 0;


		cx = node->x + coordiate_x;
		cy = node->y + coordiate_y;

		//printf("%s %d y:%d\r\n",__func__,__LINE__,node->y);
		//printf("%s %d endy:%d speedy:%d cy:%d \r\n",__func__,__LINE__,endy,slide->speedy,cy);
		//	x边界检测
		if(speedx > 0){
			if(cx > endx){
				cx = endx;
				endflagx = 1;
			}
		}else if(speedx < 0){
			if(cx < endx){
				cx = endx;
				endflagx = 1;
			}
		}

		//printf("%s %d speedy:%d cy:%d endy:%d\r\n",__func__,__LINE__,slide->speedy,cy,endy);
		// y边界检测
		if(speedy > 0){
			if(cy > endy){
				cy = endy;
				endflagy = 1;
			}
		}else if(speedy < 0 ){
			if(cy < endy){
				cy = endy;
				endflagy = 1;
			}
		}


		if(slide->mv_dir & SLIDE_NODE_MOVE_DIR_X){
			if(node->x != cx){
				node->x = cx;
			}
			if(endflagx){
				endflag = 1;
			}
		}

		if(slide->mv_dir & SLIDE_NODE_MOVE_DIR_Y){
			if(node->y != cy){
				node->y = cy;
			}
			if(endflagy){
				endflag = 1;
			}
		}
	}

	// 边界检测, 如果超出边界强制定位到边界
	if(slide->mv_dir & SLIDE_NODE_MOVE_DIR_X){
		if(slide->end_x < slide->head_x){
			if(node->x < slide->end_x){
				node->x = slide->end_x;
			}else if(node->x > slide->head_x){
				node->x = slide->head_x;
			}
		}else{
			if(node->x > slide->end_x){
				node->x = slide->end_x;
			}else if(node->x < slide->head_x){
				node->x = slide->head_x;
			}
		}
	}

	if(slide->mv_dir & SLIDE_NODE_MOVE_DIR_Y){
		if(slide->end_y < slide->head_y){
			if(node->y < slide->end_y){
				node->y = slide->end_y;
				endflag = 1;
			}else if(node->y > slide->head_y){
				node->y = slide->head_y;
				endflag = 1;
			}
		}else{
			if(node->y > slide->end_y){
				node->y = slide->end_y;
				endflag = 1;
			}else if(node->y < slide->head_y){
				node->y = slide->head_y;
				endflag = 1;
			}
		}
	}

	if (endflag) {
		slide->active_status = 0;
		slide->total_lenx = 0;
		slide->total_leny = 0;
	}
	node->opt->refresh(node);
	return 0;

}

static int slide_paint(mwidget *node, HDC hdc)
{

	mslide *m = (mslide*)node;
	slideData*	data = &m->data;

	if (data->active_status || data->button_status) {
		mwidget* child = node->child;
		if (child != NULL) {
			data->speedx = data->speedx * 19 / 20;
			data->speedy = data->speedy * 19 / 20;
			slide_location_calc(child,data);
		}
	}

	return 0;
}

static int slide_start(mwidget *node,void *param)
{
	if(node == NULL){
		return -1;
	}

	slide_cala_border(node);

	return 0;
}

static int slide_stop(mwidget *node,void *param)
{
	mslide *m = (mslide*)node;
	slideData *slide = &m->data;
	slide->total_lenx = 0;
	slide->total_leny = 0;
	return 0;
}

static int slide_set_mv_dir(mslide *m, char dir)
{
	if(m == NULL){
		return -1;
	}
	m->data.mv_dir = dir;
	return 0;
}

static int slide_set_adsorb_point(struct slideData *slide, unsigned char open, int x, int y)
{
	if (slide == NULL){
		return -1;
	}

	slide->adsorb_open = open;

	slide->adsorb_x = x;
	slide->adsorb_y = y;

	return 0;
}

#define MODULUS(x) \
	if(x < 0){ \
		x = 0-x; \
	}
static void slide_adsorb_calc(mslide *m)
{
	slideData *slide = &m->data;

	mwidget *child = m->child;
	mwidget *nearest;

	int spx, spy;		// 当前最近节点 距离吸附点 的位移绝对值
	int sspx, sspy;		// 当前最近节点 距离吸附点 的位移

	int cx= slide->startx + slide->total_lenx;		// 当前节点最终绝对坐标
	int cy = slide->starty + slide->total_leny;

	sspx = spx = cx - slide->adsorb_x;
	sspy = spy = cy - slide->adsorb_y;


	MODULUS(spx);
	MODULUS(spy);
	nearest = child;

	child = child->next;
	while(child){
		cx += child->x;	// 节点最终绝对坐标
		cy += child->y;

		int sx = cx - slide->adsorb_x;	// 节点最终坐标和吸附点距离
		int sy = cy - slide->adsorb_y;
		int ssx = sx;
		int ssy = sy;
		MODULUS(sx);
		MODULUS(sy);

		if(slide->mv_dir == SLIDE_NODE_MOVE_DIR_X){
			if(sx < spx){
				nearest = child;
				spx = sx;
				sspx = ssx;
			}
		}else if(slide->mv_dir == SLIDE_NODE_MOVE_DIR_Y){
			if(sy < spy){
				nearest = child;
				spy = sy;
				sspy = ssy;
			}
		}

		child = child->next;
	}

	if(slide->mv_dir == SLIDE_NODE_MOVE_DIR_X){
		slide->total_lenx -= sspx;
		printf("%s %d sspx:%d total:%d\r\n",__func__,__LINE__,sspx,slide->total_lenx);
		printf("nearest node:%s absx:%d %d, \n",nearest->name,nearest->start_abs_x,nearest->start_abs_y);
	}

	if(slide->mv_dir == SLIDE_NODE_MOVE_DIR_Y){
		slide->total_leny += sspy;
	}


}

mwidget *slide_get_focus(mwidget *node,unsigned int flag, struct gui_coordinate *coor)
{
	mslide *m = (mslide*)node;
	if(flag & SCREEN_FORCE_FLAG_MOVEX){
		if(m->data.mv_dir & SLIDE_NODE_MOVE_DIR_X){
			m->data.button_status = 1;
			m->data.startx = m->child->x;
			m->data.starty = m->child->y;
			m->data.total_lenx = 0;
			m->data.total_leny = 0;
			m->data.speedx = 0;
			m->data.speedy = 0;
			m->focus_lock = 1;
			return node;
		}
	}

	if(flag & SCREEN_FORCE_FLAG_MOVEY){
		if(m->data.mv_dir & SLIDE_NODE_MOVE_DIR_Y){
			m->data.button_status = 1;
			m->data.startx = m->child->x;
			m->data.starty = m->child->y;
			m->data.total_lenx = 0;
			m->data.total_leny = 0;
			m->data.speedx = 0;
			m->data.speedy = 0;
			m->focus_lock = 1;
			return node;
		}
	}

	if(flag & SCREEN_FORCE_FLAG_BUTTONDOWN){
		if((m->data.mv_dir & SLIDE_NODE_MOVE_DIR_X) && (abs(m->data.speedx) > 0)){
			m->data.total_lenx = 0;
			m->data.total_leny = 0;
			m->data.speedx = 0;
			m->data.speedy = 0;
		}

		if((m->data.mv_dir & SLIDE_NODE_MOVE_DIR_Y) && (abs(m->data.speedy) > 0)){
			m->data.total_lenx = 0;
			m->data.total_leny = 0;
			m->data.speedx = 0;
			m->data.speedy = 0;
		}
	}
	

	return m->opt->parent->get_focus(node,flag,coor);
}

int slide_touch_event_handle(mwidget *node, unsigned int flag, void *param)
{
	mslide *m = (mslide*)node;
	struct move_info *mv_info = param;
	int ret = 0;
	if(flag & SCREEN_FORCE_FLAG_MOVEX){
		if(m->data.mv_dir & SLIDE_NODE_MOVE_DIR_X){
			if(m->focus){
				m->data.speedx = mv_info->speedx;
				m->data.total_lenx += mv_info->lengthx;
				m->opt->refresh(m);
				ret = 1;
			}
		}
	}

	if(flag & SCREEN_FORCE_FLAG_MOVEY){
		if(m->data.mv_dir & SLIDE_NODE_MOVE_DIR_Y){
			if(m->focus){
				m->data.speedy = mv_info->speedy;
				m->data.total_leny += mv_info->lengthy;
				m->opt->refresh(m);
				ret = 1;
			}
		}
	}

	if(flag & SCREEN_FORCE_FLAG_BUTTONUP){
		m->data.button_status = 0;
		if(m->data.mv_dir & SLIDE_NODE_MOVE_DIR_X){
			m->data.total_lenx += m->data.speedx*20;
		}
		if(m->data.mv_dir & SLIDE_NODE_MOVE_DIR_Y){
			m->data.speedy = mv_info->speedy;
			m->data.total_leny += m->data.speedy * 60;
			printf("%s %d speedy:%d\r\n",__func__,__LINE__,m->data.speedy);
		}
		slide_adsorb_calc(m);
	}

	return ret;
}

int slide_move_dir_set(mslide *m, char *value){
	if (NULL == value){
		GUI_ERR("slide set gap failed, param is NULL\n");
		return -1;
	}

	char *tmp = value;
	int moveDir = atoi(tmp);
	if ((moveDir > SLIDE_NODE_MOVE_DIR_XY) || (moveDir < SLIDE_NODE_MOVE_DIR_X)){
		GUI_ERR("slide move dir is not in range, param is %s\n",value);
		return -1;	
	}

	m->opt->set_mv_dir(m, moveDir);

	return 0;
}

char *slide_move_dir_get(mslide *m){
	
	char *buf = widget_calloc(1,12);
	sprintf(buf, "%d", m->data.mv_dir);

	return buf;
}

int slide_adsorb_point_set(mslide *m,char * value){
	if (NULL == value){
		GUI_ERR("slide set adsorb point failed, param is NULL\n");
		return -1;
	}

	char *tmp = value;
	int adsorb_open = atoi(tmp);

	tmp = strchr(tmp,',');
	if (NULL == tmp){
		GUI_ERR("slide set adsorb point, adsorb_x is NULL\n");
		return -1;
	}
	tmp++;
	int adsorb_x = atoi(tmp);

	tmp = strchr(tmp,',');
	if (NULL == tmp){
		GUI_ERR("slide set adsorb point, adsorb_y is NULL\n");
		return -1;
	}
	tmp++;
	int adsorb_y = atoi(tmp);

	m->opt->set_adsorb_point(m,adsorb_open,adsorb_x,adsorb_y);

	return 0;
}

char *slide_adsorb_point_get(mslide *m){

	char *buf = widget_calloc(1,36);
	sprintf(buf, "%d,%d,%d", m->data.adsorb_open, m->data.adsorb_x, m->data.adsorb_y);
	
	return buf;
}



_WIDGET_GENERATE(slide,widget)
