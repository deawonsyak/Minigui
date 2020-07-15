
#include "slidescreen_widget.h"

static int slide_cala_border(mwidget *node)
{
	if(node == NULL ){
		return -1;
	}

	mwidget *child = node->child;
	if(child == NULL){
		return 0;
	}

	mslidescreen *m = (mslidescreen*)node;
	struct slidescreenData *slide = &m->data;

	slide->slide_len = 0;


	mwidget *prev = child;
	while(child != NULL){

		if(slide->mv_dir == SLIDESCREEN_NODE_MOVE_DIR_X){
			slide->slide_len += child->w + slide->gap;
		}else{
			slide->slide_len += child->h + slide->gap;

		}
		prev = child;
		child = child->next;
	}


	// TODO 滑块小于slide节点的宽度时 怎么处理?
	
	if(slide->mv_dir == SLIDESCREEN_NODE_MOVE_DIR_X){
		slide->show_b_1 = node->w - slide->slide_len;
		slide->show_b_f = slide->slide_len;
		slide->show_b_e = slide->slide_len + node->w;
	}else{
		slide->show_b_1 = node->h - slide->slide_len;
		slide->show_b_f = slide->slide_len;
		slide->show_b_e = slide->slide_len + node->h;
	}


	return 0;
}

static int child_coor_calc(mslidescreen *m)
{
	mwidget *child = m->child->next;
	slidescreenData *slide = &m->data;

	mwidget *prev = m->child;
	while(child){
		if(slide->mv_dir == SLIDESCREEN_NODE_MOVE_DIR_X){
			child->x = prev->x + prev->w + m->data.gap;
			child->y = prev->y;
			if(child->x >= slide->show_b_f - child->w){
				child->x -= slide->slide_len;
			}
		}else{
			child->y = prev->y + prev->h + m->data.gap;
			child->x = prev->x;
			if(child->y >= slide->show_b_f - child->h){
				child->y -= slide->slide_len;
			}
		}

		//printf("%s:child_x:%d _y:%d, gap:%d, show_b_f:%d w:%d h:%d\n", __func__, child->x, child->y,m->data.gap, slide->show_b_f, child->w, child->h);

		prev = child;
		child = child->next;
	}

	return 0;
}


static int end_point_calc(mslidescreen *m)
{
	slidescreenData *slide = &m->data;
	int end = 0;

	if(slide->mv_dir == SLIDESCREEN_NODE_MOVE_DIR_X){
		end = slide->startx + slide->total_len;
		while(end < m->data.show_b_1){
			end += slide->slide_len;
		}

		while( end >= m->w){
			end -= slide->slide_len;
		}
	}else if(slide->mv_dir == SLIDESCREEN_NODE_MOVE_DIR_Y){
		end = slide->starty + slide->total_len;
		while(end < m->data.show_b_1){
			slide->total_len += slide->slide_len;
			end = slide->starty + slide->total_len;
		}

		while( end >= m->h){
			slide->total_len -= slide->slide_len;
			end = slide->starty + slide->total_len;
		}
	}

	return end;

}

static int slidescreen_coor_get(mslidescreen *m, int mv_len)
{
	int cc;

	//printf("%s:child_x:%d child_y:%d show_b_l:%d\n", __func__,m->child->x, m->child->y, m->data.show_b_1);
	if(m->data.mv_dir == SLIDESCREEN_NODE_MOVE_DIR_X){
		cc = m->child->x + mv_len;
		while( cc >= m->w){
			cc -= m->data.slide_len;
		}
	}else{
		cc = m->child->y + mv_len;
		while( cc >= m->h){
			cc -= m->data.slide_len;
		}
	}

	while(cc < m->data.show_b_1){
		cc += m->data.slide_len;
	}

	//printf("%s:slide_len:%d w:%d h:%d cc:%d\n", __func__, m->data.slide_len, m->w, m->h, cc);
	return cc;
}

static int slide_location_calc(mslidescreen *m)
{
	slidescreenData *slide = &m->data;

	struct timeval tv;
	gettimeofday(&tv,NULL);

	int end = end_point_calc(m);

	if (slide->button_status == 1) {	// 滑动中
		slide->active_status = 1;
		if(slide->mv_dir == SLIDESCREEN_NODE_MOVE_DIR_X){	// X 
			m->child->x = end;
		}else{		// Y
			m->child->y = end;
		}
		child_coor_calc(m);
	}else{		// 惯性移动
		int mv_len = slide->speed * 3;	// 单次移动长度

		slide->move_len += mv_len;

		int cs = slidescreen_coor_get(m,mv_len);	// 移动后坐标

		char endflag = 0;

		// 边界检测
		if(slide->speed > 0){
			if(slide->move_len > slide->total_len){
				endflag = 1;
				cs = end;
			}
		}else if(slide->speed < 0){
			if(slide->move_len < slide->total_len){
				endflag = 1;
				cs = end;
			}
		}

		if(slide->mv_dir == SLIDESCREEN_NODE_MOVE_DIR_X){
			if(m->child->x != cs){
				m->child->x = cs;
			}
		}

		if(slide->mv_dir == SLIDESCREEN_NODE_MOVE_DIR_Y){
			if(m->child->y != cs){
				m->child->y = cs;
			}
		}

		child_coor_calc(m);
	


		if (endflag) {
			slide->active_status = 0;
			slide->total_len = 0;
		}

	}
	m->opt->refresh(m);
	return 0;
}

static int slidescreen_paint(mwidget *node, HDC hdc)
{
	mslidescreen *m = (mslidescreen*)node;
    slidescreenData*	data = &m->data;
    
	if (data->active_status || data->button_status) {
		mwidget* child = node->child;
		if (child != NULL) {
			slide_location_calc(m);
		}
	}
	
	return 0;
}

static int slidescreen_start(mwidget *node,void *param)
{
	if(node == NULL){
		return -1;
	}

	slide_cala_border(node);
	child_coor_calc((mslidescreen*)node);

	return 0;
}

static int slidescreen_set_mv_dir(mslidescreen *m, char dir)
{
	if(m == NULL){
		return -1;
	}
	m->data.mv_dir = dir;
	return 0;
}

int slidescreen_set_gap(mslidescreen *m, int gap)
{
	m->data.gap = gap;
	return 0;
}

mwidget *slidescreen_get_focus(mwidget *node,unsigned int flag, struct gui_coordinate *coor)
{
	
	mslidescreen *m = (mslidescreen*)node;
	if(flag & SCREEN_FORCE_FLAG_MOVEX){
		if(m->data.mv_dir == SLIDESCREEN_NODE_MOVE_DIR_X){
			m->data.button_status = 1;
			m->data.startx = m->child->x;
			m->data.starty = m->child->y;
			m->data.total_len = 0;
			m->data.speed = 0;
			m->data.speedy = 0;
			m->data.move_len = 0;
			m->focus_lock = 1;
			return node;
		}
	}

	if(flag & SCREEN_FORCE_FLAG_MOVEY){
		if(m->data.mv_dir == SLIDESCREEN_NODE_MOVE_DIR_Y){
			m->focus_lock = 1;
			return node;
		}
	}

	return m->opt->parent->get_focus(node,flag,coor);
}

static int slidescreen_set_adsorb_point(mslidescreen *m, unsigned char open, int x, int y)
{
	slidescreenData *slide = &m->data;
	if (slide == NULL)
	{
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
static void slidescreen_adsorb_calc(mslidescreen *m)
{
	slidescreenData *slide = &m->data;

	mwidget *child = m->child;
	mwidget *nearest;

	int sp;			// 当前最近节点 距离吸附点 的位移绝对值
	int ssp;		// 当前最近节点 距离吸附点 的位移

	int end = end_point_calc(m);

	int cc = end;		// 当前节点最终绝对坐标

	if(slide->mv_dir == SLIDESCREEN_NODE_MOVE_DIR_X){
		ssp = sp = cc - slide->adsorb_x;
		MODULUS(sp)
		nearest = child;
		mwidget *prev = child;
		child = child->next;	
		while(child){
			cc = cc + prev->w + slide->gap;
			if(cc >= slide->show_b_f - child->w){
				cc -= slide->slide_len;
			}

			int sx = cc - slide->adsorb_x;	// 当前节点距离吸附点的位移
			int ssx = sx;

			MODULUS(sx);
			if(sx < sp){
				nearest = child;
				sp = sx;
				ssp = ssx;
			}
			prev = child;
			child = child->next;
		}
	}else{		// Y
		ssp = sp = cc - slide->adsorb_y;
		nearest = child;
		mwidget *prev = child;
		child = child->next;	
		while(child){
			cc = prev->y + prev->h + slide->gap;
			if(cc >= slide->show_b_f - child->w){
				cc -= slide->slide_len;
			}

			int sx = cc - slide->adsorb_y;
			int ssx = sx;

			MODULUS(sx);
			if(sx < sp){
				nearest = child;
				sp = sx;
				ssp = ssx;
			}
			prev = child;
			child = child->next;
		}
	}

	slide->total_len -= ssp;

	if(slide->speed == 0){
		if(ssp > 0){
			slide->speed = 20;
		}else{
			slide->speed = -20;
		}
	}else if(slide->speed > 0){
		if(slide->move_len > slide->total_len){
			slide->speed = 0-slide->speed;
		}
	}else{
		if(slide->move_len < slide->total_len){
			slide->speed = 0-slide->speed;
		}
	}

	//speed too small, move soo slow
	if (abs(slide->speed) < 20){
		slide->speed = 20 * slide->speed / abs(slide->speed);
	}

}
int slidescreen_touch_event_handle(mwidget *node, unsigned int flag, void *param)
{
	mslidescreen *m = (mslidescreen*)node;
	struct move_info *mv_info = param;
	int ret = 0;
	if(flag & SCREEN_FORCE_FLAG_MOVEX){
		if(m->data.mv_dir == SLIDESCREEN_NODE_MOVE_DIR_X){
			if(m->focus){
				m->data.speed = mv_info->speedx;
				m->data.total_len += mv_info->lengthx;
				//GUI_DEBUG("DIR_X total_len:%d mv_info->lengthx=%d\n", m->data.total_len, mv_info->lengthx);
				m->data.move_len += mv_info->lengthx;
				m->opt->refresh(node);
				ret = 1;
			}
			return 1;
		}
	}

	if(flag & SCREEN_FORCE_FLAG_MOVEY){
		if(m->data.mv_dir == SLIDESCREEN_NODE_MOVE_DIR_Y){
			m->data.button_status = 1;
			m->data.speedy = mv_info->speedy;
			m->data.total_len += mv_info->lengthy;
			//GUI_DEBUG("DIR_Y total_len:%d mv_info->lengthx=%d\n", m->data.total_len, mv_info->lengthy);
			m->opt->refresh(node);
			ret = 1;
			return 1;
		}
	}

	if(flag & SCREEN_FORCE_FLAG_BUTTONUP){
		m->data.button_status = 0;
		m->data.speed = mv_info->speedx;
		m->data.total_len += m->data.speed* 10;
		//GUI_DEBUG("total_len:%d speed = %d\n", m->data.total_len, m->data.speed);
		if ((abs(m->data.total_len) > m->w) || (abs(m->data.speed) >= SLIDESCREEN_CHANGE_SCREEN_SPEED)){
			m->data.total_len = m->data.total_len / abs(m->data.total_len) * m->w;
		}
		
		if(m->data.adsorb_open){
			slidescreen_adsorb_calc(m);
		}
	}

	return ret;
}

int slidescreen_move_dir_set(mslidescreen * m,char * value){
	if (NULL == value){
		GUI_ERR("slidescreen set move dir failed, param is NULL\n");
		return -1;
	}

	char *tmp = value;
	int moveDir = atoi(tmp);
	if ((moveDir > SLIDESCREEN_NODE_MOVE_DIR_Y) || (moveDir < SLIDESCREEN_NODE_MOVE_DIR_X)){
		GUI_ERR("slidescreen move dir is not in range, param is %s\n",value);
		return -1;	
	}

	m->opt->set_mv_dir(m, moveDir);

	return 0;
}

char *slidescreen_move_dir_get(mslidescreen *m){
	char *buf = widget_calloc(1,12);
	sprintf(buf, "%d", m->data.mv_dir);

	return buf;
}

int slidescreen_gap_set(mslidescreen * m,char * value){
	if (NULL == value){
		GUI_ERR("slidescreen set gap failed, param is NULL\n");
		return -1;
	}

	char *tmp = value;
	int gap = atoi(tmp);
	m->opt->set_gap(m,gap);

	return 0;
}

char *slidescreen_gap_get(mslidescreen *m){
	char *buf = widget_calloc(1,12);
	sprintf(buf, "%d", m->data.gap);

	return buf;
}

int slidescreen_adsorb_point_set(mslidescreen *m,char * value){
	if (NULL == value){
		GUI_ERR("slidescreen set adsorb point failed, param is NULL\n");
		return -1;
	}

	char *tmp = value;
	int adsorb_open = atoi(tmp);

	tmp = strchr(tmp,',');
	if (NULL == tmp){
		GUI_ERR("slidescreen set adsorb point, adsorb_x is NULL\n");
		return -1;
	}
	tmp++;
	int adsorb_x = atoi(tmp);

	tmp = strchr(tmp,',');
	if (NULL == tmp){
		GUI_ERR("slidescreen set adsorb point, adsorb_y is NULL\n");
		return -1;
	}
	tmp++;
	int adsorb_y = atoi(tmp);

	m->opt->set_adsorb_point(m,adsorb_open,adsorb_x,adsorb_y);

	return 0;
}

char *slidescreen_adsorb_point_get(mslidescreen *m){
	char *buf = widget_calloc(1,36);
	sprintf(buf, "%d,%d,%d", m->data.adsorb_open, m->data.adsorb_x, m->data.adsorb_y);
	
	return buf;
}





_WIDGET_GENERATE(slidescreen,widget)
