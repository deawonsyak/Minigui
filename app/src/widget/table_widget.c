
#include "table_widget.h"

static int TableGetNodeCount(mwidget *node){
	if (NULL == node){
		GUI_ERR("TableGetNodeCount param node is NULL\n");
		return -1;
	}
	mtable *m = (mtable*)node;

	int count = 0;

	mwidget *tmp_node = m->child;
	while(tmp_node){
		tmp_node = tmp_node->next;
		count++;
	}
	
	return count;
}

static int TableSetDefaultIndexList(mtable *m){
	int node_count = TableGetNodeCount(m);
	for (int index = 0; index < node_count; index++)
	{
		m->data.table_index_list[index] = index;
	}

	return 0;
}


mwidget *TableGetIndexNode(mwidget *node, int list_index){
	if (NULL == node){
		GUI_ERR("TableGetIndexNode param node is NULL\n");
		return NULL;
	}
	mtable *m = (mtable*)node;
	int node_count = TableGetNodeCount(m);

	if ((list_index < 0) || (list_index > node_count)){
		GUI_ERR("TableGetIndexNode list_index(%d) is not in range(0 ~ %d)\n", list_index, node_count);
		return NULL;		
	}
	
	int node_index = m->data.table_index_list[list_index];

	mwidget *tmp_node = m->child;
	while((node_index > 0) && (tmp_node != NULL)){
		tmp_node = tmp_node->next;
		node_index--;
	}

	return tmp_node;
}

static int TableNodeExchange(mwidget *node, int src_node_index, int des_node_index){
	if (NULL == node){
		GUI_ERR("TableNodeExchange param node is NULL\n");
		return -1;
	}
	mtable *m = (mtable*)node;
	int node_count = TableGetNodeCount(m);

	if ((src_node_index < 0) || (src_node_index >= node_count)){
		GUI_ERR("TableNodeExchange src_node_index is not in range(0~%d)\n", src_node_index, node_count);
		return -1;
	}

	if ((des_node_index < 0) || (des_node_index >= node_count)){
		GUI_ERR("TableNodeExchange src_node_index(%d) is not in range(0~%d)\n", des_node_index, node_count);
		return -1;
	}

	unsigned char tmp_table_index = m->data.table_index_list[src_node_index];
	m->data.table_index_list[src_node_index] = m->data.table_index_list[des_node_index];
	m->data.table_index_list[des_node_index] = tmp_table_index;
	m->data.focus_index = des_node_index;

	/*	
	printf("#TableNodeExchange#table_index_list:{");
	for (int index = 0; index < node_count; index++){
		printf(" %d ", m->data.table_index_list[index]);
	}
	printf("}##\n");
	*/

	return 0;
}


static int TableNodeArray(mwidget *node, int src_node_index, int des_node_index){
	if (NULL == node){
		GUI_ERR("TableNodeExchange param node is NULL\n");
		return -1;
	}
	
	mtable *m = (mtable*)node;
	int node_count = TableGetNodeCount(m);

	if ((src_node_index < 0) || (src_node_index >= node_count)){
		GUI_ERR("TableNodeExchange src_node_index is not in range(0~%d)\n", src_node_index, node_count);
		return -1;
	}

	if ((des_node_index < 0)){
		GUI_ERR("TableNodeExchange des_node_index(%d) invalid\n", des_node_index);
		return -1;
	}

	if (des_node_index >= node_count){
		des_node_index = node_count - 1;
	}

	int tmp_src_index = m->data.table_index_list[src_node_index];
	
	//printf("#TableNodeArray# src_node_index = %d des_node_index=%d\n", src_node_index, des_node_index);

	if (src_node_index < des_node_index){	
		for (int index = src_node_index; index < des_node_index; index++){
			m->data.table_index_list[index] = m->data.table_index_list[index + 1];
		}
		m->data.table_index_list[des_node_index] = tmp_src_index;
	}else{
		for (int index = src_node_index; index > des_node_index; index--){
			m->data.table_index_list[index] = m->data.table_index_list[index - 1];
		}
		m->data.table_index_list[des_node_index] = tmp_src_index;
	}

	m->data.focus_index = des_node_index;

	/*
	printf("#TableNodeArray#table_index_list:{");
	for (int index = 0; index < node_count; index++){
		printf(" %d ", m->data.table_index_list[index]);
	}
	printf("}##\n");
	*/

	return 0;
}



static int TableLayoutChange(mwidget *node){
	if (NULL == node){
		GUI_ERR("TableCheckLayoutChange param node is NULL\n");
		return -1;
	}
	mtable *m = (mtable*)node;

	int index_width =  m->w / m->data.line_count;
	int index_height = m->h / m->data.column_count;
	int screen_max_index = m->data.line_count * m->data.column_count;
	int node_count = TableGetNodeCount(m);
	int table_screen_count =  (node_count + screen_max_index  - 1) / screen_max_index;

	mwidget *table_focus_node = TableGetIndexNode(m, m->data.focus_index);
	if (NULL == table_focus_node){
		GUI_ERR("TableMoveFocusNode get focus node failed\n");
		return -1;
	}

	//printf("#cur_table_screen:%d--# table_focus_node->x:%d  chk:%d\n", m->data.cur_table_screen, table_focus_node->x, (0 - index_width / 2));
	if (m->data.cur_table_screen >= 1){
		if (table_focus_node->x <= (0 - index_width / 2)){
			m->data.cur_table_screen--;
			return 1;
		}
	}

	if (m->data.cur_table_screen < (table_screen_count - 1)){
		//printf("#cur_table_screen++# table_focus_node->x:%d  chk:%d\n", table_focus_node->x, (m->w - index_width / 2));
		if (table_focus_node->x >= (m->w - index_width / 2)){
			m->data.cur_table_screen++;
			return 1;
		}
	}

	int chk_line_index_x = table_focus_node->x + index_width / 2;
	int chk_column_index_y = table_focus_node->y + index_height / 2;

	if (chk_line_index_x >= m->w){
		chk_line_index_x = m->w - index_width;
	}

	if (chk_column_index_y >= m->h){
		chk_column_index_y = m->h - index_height;
	}
	
	int tmp_line_index = chk_line_index_x / index_width;
	int tmp_column_index = chk_column_index_y / index_width;

	//printf("TableLayoutChange tmp_line_index = %d tmp_column_index=%d\n", tmp_line_index, tmp_column_index);
	int change_index = tmp_line_index + tmp_column_index * m->data.line_count + m->data.cur_table_screen * screen_max_index;

	//printf("focus_index = %d  change_index = %d\n", m->data.focus_index, change_index);

	if (m->data.focus_index != change_index){
		if(TABLE_MOVE_RULE_ARRAY == m->data.table_move_rule){
			TableNodeArray(m, m->data.focus_index, change_index);
		}else{
			TableNodeExchange(m, m->data.focus_index, change_index);
		}
	}
	
	return 0;
}


int TableMoveFocusNode(mtable *m){

	if (NULL == m){
		GUI_ERR("TableMoveFocusNode param is invalid\n");
		return -1;
	}	
	if (m->data.focus_index < 0){
		GUI_ERR("TableMoveFocusNode focus_index is invalid (%d)\n", m->data.focus_index);
		return -1;
	}
	
	mwidget *table_focus_node = TableGetIndexNode(m, m->data.focus_index);
	if (NULL == table_focus_node){
		GUI_ERR("TableMoveFocusNode get focus node failed\n");
		return -1;
	}
	
	table_focus_node->x = m->data.coor_x - m->data.table_focus_node_abs_x;
	table_focus_node->y = m->data.coor_y - m->data.table_focus_node_abs_y;

	//printf("table_focus_node show x:%d y:%d \n", table_focus_node->x, table_focus_node->y);

}

static int TableLayoutCalc(mwidget *node){
	if (NULL == node){
		GUI_ERR("TableLayoutCalc param node is NULL\n");
		return -1;
	}

	mtable *m = (mtable*)node;
	
	int index = 0;
	int table_screen_num = 0;
	mwidget *child = m->child;

	if ((0 == m->data.line_count) || (0 == m->data.column_count)){
		GUI_ERR("TableLayoutCalc layout failed, line or  column count is 0\n");
		return -1;
	}

	int index_width =  m->w / m->data.line_count;
	int index_height = m->h / m->data.column_count;
	int screen_max_index = m->data.line_count * m->data.column_count;
	int node_count = TableGetNodeCount(m);

	//printf("====line_count = %d column_count = %d node_count=%d===\n", m->data.line_count, m->data.column_count, node_count);

	if (m->data.focus_index >= 0){
		TableMoveFocusNode(m);
		TableLayoutChange(m);
	}

	for (index = 0; index < node_count; index++)
	{
		if (index == m->data.focus_index){
			continue;
		}

		child = TableGetIndexNode(m, index);
		if (NULL == child){
			GUI_ERR("TableMoveFocusNode get child node failed\n");
			return -1;
		}

		table_screen_num = index / screen_max_index;

		//printf("table_screen_num = %d  cur_table_screen = %d\n", table_screen_num, m->data.cur_table_screen);
		child->x = index_width * (index % m->data.line_count) + (table_screen_num - m->data.cur_table_screen) * m->w;
		child->y = index_height * ((index % screen_max_index) / m->data.line_count);
		child->w = index_width;
		child->h = index_height;
		//printf("table index:%d show x:%d y:%d w:%d h:%d\n", index, child->x, child->y,child->w, child->h);
	}

	return 0;
}

static int TableNodePaint(mwidget *node,HDC hdc, mwidget *table_focus_node){
	if (NULL == node){
		GUI_ERR("TableNodePaint node is NULL\n");
		return -1;
	}	

	mtable *m = (mtable*)node;
	if (NULL == table_focus_node){
		gui_node_paint(m->child,m->hdc);
		return 0;
	}

	//focus node paint in before screen
	table_focus_node->hide = 1;
	gui_node_paint(m->child,m->hdc);

	mwidget* tmp_node = m->child;
	while(tmp_node){
		tmp_node->hide = 1;
		tmp_node = tmp_node->next;
	}	
	table_focus_node->hide = 0;
	gui_node_paint(table_focus_node,m->hdc);

	tmp_node = m->child;
	while(tmp_node){
		tmp_node->hide = 0;
		tmp_node = tmp_node->next;
	}	
		
	return 0;
}


static int TableLayoutPaint(mwidget *node,HDC hdc){
	if (NULL == node){
		GUI_ERR("TableLayoutPaint node is NULL\n");
		return -1;
	}	
	
	mtable *m = (mtable*)node;
	mwidget *table_focus_node = NULL;
	if (m->data.focus_index >= 0){
		table_focus_node = TableGetIndexNode(m, m->data.focus_index);
		if (NULL == table_focus_node){
			GUI_ERR("TableLayoutPaint get focus node failed\n");
			return -1;
		}
	}

	if(node->need_paint || m->hdc == NULL){
		if(m->hdc){
			screen_destroy_hdc(m->hdc);
		}
		m->hdc = screen_create_hdc(node->w,node->h);
		node->need_paint = 0;

		TableNodePaint(node, m->hdc, table_focus_node);
	}

	if(node->show_abs_x >= 0){
		int sx = node->show_abs_x - node->start_abs_x;
		int sy = node->show_abs_y - node->start_abs_y;

		BitBlt(m->hdc,sx,sy,node->show_w,node->show_h,hdc,node->show_abs_x,node->show_abs_y,0);
	}
}


static int TablePaint(mwidget *node,HDC hdc){
	if (NULL == node){
		GUI_ERR("TableLayoutPaint node is NULL\n");
		return -1;
	}	
	
	mtable *m = (mtable*)node;
	if (TABLE_INDEX_LIST_UNINIT == m->data.table_index_list_init_flag){
		TableSetDefaultIndexList(m);
		m->data.table_index_list_init_flag = TABLE_INDEX_LIST_INIT;
	}

	TableLayoutCalc(node);

	TableLayoutPaint(node, hdc);
	return 0;
}



static int TableGetFocusIndex(mwidget *node, struct gui_coordinate *coor){
	if ((NULL == node) || (NULL == coor)){
		GUI_ERR("TableGetFocusIndex param is invalid\n");
		return -1;
	}
	mtable *m = (mtable*)node;

	int coor_line_index = coor->x  * m->data.line_count / m->w;
	int coor_column_index = coor->y * m->data.column_count / m->h;
	int screen_max_index = m->data.line_count * m->data.column_count;

	int coor_focus_index = coor_line_index + coor_column_index * m->data.line_count + screen_max_index * m->data.cur_table_screen;

	m->data.table_focus_node_abs_x = coor->x - (coor_line_index * m->w / m->data.line_count);
	m->data.table_focus_node_abs_y = coor->y - (coor_column_index * m->h / m->data.column_count);

	return coor_focus_index;
}

static int TableSetFocusIndexBg(mwidget *node, int focus_index){
	if (NULL == node){
		GUI_ERR("TableGetFocusIndex param is invalid\n");
		return -1;
	}	
	mtable *m = (mtable*)node;

	mwidget *table_focus_node = TableGetIndexNode(m, focus_index);
	if (NULL == table_focus_node){
		GUI_ERR("TableSetFocusIndexBg get focus node failed\n");
		return -1;
	}
	
	//table_focus_node->opt->set_picture(table_focus_node, "res/res_mixpad/01_home/home_bg.png");
	//table_focus_node->opt->refresh((mwidget*)table_focus_node);
}


mwidget *TableGetFocus(mwidget *node,unsigned int flag, struct gui_coordinate *coor)
{
	if ((NULL == node) || (NULL == coor)){
		GUI_ERR("TableGetFocusIndex param is invalid\n");
		return -1;
	}

	mtable *m = (mtable*)node;
	if(flag & SCREEN_FORCE_FLAG_MOVEX){
		return node;
	}

	if(flag & SCREEN_FORCE_FLAG_MOVEY){
		return node;
	}

	if(flag & SCREEN_FORCE_FLAG_BUTTONDOWN){
	}

	if(flag & SCREEN_FORCE_FLAG_BUTTONUP){

	}


	return m->opt->parent->get_focus(node,flag,coor);
}


int TableTouchEventHandle(mwidget *node, unsigned int flag, void *param){
	if ((NULL == node) || (NULL == param)){
		GUI_ERR("TableTouchEventHandle param is invalid\n");
		return -1;
	}
	
	mtable *m = (mtable*)node;
	struct move_info *mv_info = param;
	int ret = 0;

	//GUI_DEBUG("TableTouchEventHandle move flag:0x%x coor.x:%d coor.y:%d\n", flag, mv_info->coor.x, mv_info->coor.y);	
	if((flag & SCREEN_FORCE_FLAG_MOVEX)||(flag & SCREEN_FORCE_FLAG_MOVEY)){
		if (m->data.focus_index >= 0){
			//printf("TableTouchEventHandle move x:%d y:%d\n", mv_info->coor.x, mv_info->coor.y); 
			m->data.coor_x = mv_info->coor.x;
			m->data.coor_y = mv_info->coor.y;
			m->opt->refresh((mwidget*)m);
		}
	}


	if(flag & SCREEN_FORCE_FLAG_BUTTONDOWN){
	}
	

	if(flag & SCREEN_FORCE_FLAG_BUTTONUP){
		if (m->data.focus_index > 0){
			m->data.focus_index = -1;
			m->opt->refresh((mwidget*)m);
		}
	}

	if (flag & SCREEN_FORCE_FLAG_LONGPRESS){
		m->data.focus_index = TableGetFocusIndex(node, &(mv_info->coor));
		//printf("TableGetFocus focus_index = %d\n", m->data.focus_index);
	}
	

	return ret;
}


static void TableDestroy(mwidget *node)
{
	mtable *m = (mtable*)node;
}


int TableSetLayout(mtable* m, int line_count, int column_count){
	if (NULL == m){
		GUI_ERR("TableSetLayout mtable is NULL\n");
		return -1;
	}

	m->data.line_count = line_count;
	m->data.column_count = column_count;

	m->opt->refresh((mwidget*)m);

	return 0;
}



int TableLayoutSet(mtable *m, char *value){
	if (NULL == m){
		GUI_ERR("TableLayoutSet mtable is NULL\n");
		return -1;
	}

	if (NULL == m){

		GUI_ERR("TableLayoutSet set string is NULL\n");
		return -1;
	}
	
	char* tmp = value;
	int line_count  = atoi(tmp);

	tmp = strchr(tmp, ",");
	if (NULL == tmp){
		GUI_ERR("TableLayoutSet layout set failed, column_count is NULL, value:%s\n", value);
		return -1;
	}
	
	tmp++;

	int column_count = atoi(tmp);

	m->opt->SetLayout(m, line_count, column_count);

	return 0;
}

char* TableLayoutGet(mtable *m){
	char* buf = (char *)widget_calloc(1,24);
	sprintf(buf,"%d,%d", m->data.line_count, m->data.column_count);

	return buf;
}




_WIDGET_GENERATE(table,widget)

