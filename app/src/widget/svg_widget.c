
#include "svg_widget.h"
#include "screen.h"

int svg_paint(mwidget *node, HDC hdc)
{
	msvg *m = (msvg*)node;
	m->opt->lock_data(m);
	if(node->show_abs_x >= 0){
		int sx = node->show_abs_x - node->start_abs_x;
		int sy = node->show_abs_y - node->start_abs_y;


		BitBlt(m->hdc,sx,sy,node->show_w,node->show_h,hdc,node->show_abs_x,node->show_abs_y,0);
	}
	m->opt->unlock_data(m);

	return 0;
}

void svg_destroy(mwidget *node,widget_opt *clas)
{
	msvg *m = (msvg*)node;

	if(m == NULL){
		return;
	}

	if(m->hdc){
		screen_destroy_hdc(m->hdc);
		m->hdc = NULL;
	}

	if(m->destroy_flag == 1){
		if(m->pen){
			MGPlusPenDelete(m->pen);
		}

		if(m->solidBrush){
			MGPlusBrushDelete(m->solidBrush);
		}

		if(m->hatchFillBrush){
			MGPlusBrushDelete(m->hatchFillBrush);
		}

		if(m->textureFillBrush){
			MGPlusBrushDelete(m->textureFillBrush);
		}

		if(m->pathgradientBrush){
			MGPlusBrushDelete(m->pathgradientBrush);
		}

		if(m->lineargradientBrush){
			MGPlusBrushDelete(m->lineargradientBrush);	
		}

		if(m->graphics){
			MGPlusGraphicDelete(m->graphics);
		}
		
		m->destroy_flag = 0;
	}

	clas->parent->destroy(m,clas->parent);
}

int mgplus_test(msvg *m)
{
#if 0
	gal_pixel color = 0xff0000ff;	
	HPEN pen = MGPlusPenCreate (2,color);

	HGRAPHICS gpc = MGPlusGraphicCreateFromDC(m->hdc);

	HBRUSH brush = MGPlusBrushCreate (MP_BRUSH_TYPE_SOLIDCOLOR);
	MGPlusSetSolidBrushColor (brush, 0xFFffFFFF);
	MGPlusFillEllipseI(gpc,	brush, 30,30,20,20);
	//MGPlusFillEllipse(gpc,	brush, 80,30,20,20);
	//MGPlusGraphicSave (gpc, m->hdc, 0, 0, 0, 0, 0, 0);
	
	HFONT hfont = MGPlusCreateFont("res/font/PingFangRegular.ttf",0,MP_GLYPH_REN_OUTLINE,60,60,TRUE);
	if(hfont == 0){
		printf("%s %d error\r\n",__func__,__LINE__);
	}
	GLYPHMETRICS metrics;
	GLYPHDATA glyph_data = {0};
	HPATH hpath;
	char *text = "hello 你好";
	//char *text = "hello";
	int x = 60;
	int y = 60;
	for(int i = 0; i < strlen(text); i++){
		MGPlusGetGlyphOutline(hfont,text[i],&metrics,&glyph_data);
		MGPlusDrawGlyph(gpc,hfont, x, y, &glyph_data, 0x5f00ffff);

		//hpath = MGPlusGetGlyphPath(x,y,&glyph_data);
		//if(hpath){
			//MGPlusFillPath(gpc,brush,hpath);
		//}
		x += metrics.adv_x;
		y += metrics.adv_y;
	}
	MGPlusGraphicSave (gpc, m->hdc, 0, 0, 0, 0, 0, 0);
	

	MGPlusPenDelete (pen);
	MGPlusGraphicDelete (gpc);
	MGPlusBrushDelete(brush);
#else

	PLOGFONT font = CreateLogFont("ttf", "PingFangRegular", "UTF-8",		//注意：字符集一定要选择UTF-8
				0, FONT_SLANT_ROMAN,
				FONT_FLIP_NIL, FONT_OTHER_AUTOSCALE,
				FONT_UNDERLINE_NONE, FONT_STRUCKOUT_NONE, 30, 0);


	Glyph32 gly;
	Glyph32 *pgly = NULL;
	GLYPHMAPINFO glp_map;
	GLYPHMAPINFO *pgly_map=NULL;
	printf("%s %d\r\n",__func__,__LINE__);
	BIDIGetTextLogicalGlyphs(font,"111",3,&pgly,&pgly_map);
	printf("%s %d %p %p\r\n",__func__,__LINE__,pgly,pgly_map);
	int x,y;
	SetPenColor(m->hdc, 0xff00ff00);
	SetBrushColor(m->hdc, 0x000000);
	//DrawGlyph(m->hdc,0,0,*pgly,&x,&y);
	DrawGlyphString(m->hdc,0,0,pgly,10,&x,&y);
	printf("%s %d\r\n",__func__,__LINE__);
#endif



	return 0;

}

int checkGraphics(msvg *m)
{
	if (0 == m->graphics){
		m->graphics = MGPlusGraphicCreateFromDC(m->hdc);
		if (0 ==  m->graphics){
			return -1;
		}

		//printf("%s:recreate graphic from dc\n",__func__);
	}

	return 0;
}

int checkPen(msvg *m)
{
	if (0 == m->pen){
		m->pen = MGPlusPenCreate(_SVG_DEFAULT_PEN_WIDTH, _SVG_DEFAULT_PEN_ARGB);
		if (0 ==  m->pen){
			return -1;
		}
		//printf("%s:recreate pen use default pen width and rgba\n",__func__);
		
		return 1;
	}

	return 0;
}


int GetBrushFromType(msvg *m, int brushType, HBRUSH *pBrush)
{
	int type = MP_BRUSH_TYPE_SOLIDCOLOR;

	switch(brushType){
		case MP_BRUSH_TYPE_SOLIDCOLOR:
		{
			type = MP_BRUSH_TYPE_SOLIDCOLOR;
			*pBrush = m->solidBrush;
			break;
		}
		case MP_BRUSH_TYPE_HATCHFILL:
		{
			type = MP_BRUSH_TYPE_HATCHFILL;
			*pBrush = m->hatchFillBrush;
			break;
		}
		case MP_BRUSH_TYPE_TEXTUREFILL:
		{
			type = MP_BRUSH_TYPE_TEXTUREFILL;
			*pBrush = m->textureFillBrush;
			break;
		}

		case MP_BRUSH_TYPE_PATHGRADIENT:
		{
			type = MP_BRUSH_TYPE_PATHGRADIENT;
			*pBrush = m->pathgradientBrush;
			break;
		}

		case MP_BRUSH_TYPE_LINEARGRADIENT:
		{
			type = MP_BRUSH_TYPE_LINEARGRADIENT;
			*pBrush = m->lineargradientBrush;
			break;
		}	
		
		default:
		{
			type = MP_BRUSH_TYPE_SOLIDCOLOR;
			*pBrush = m->solidBrush;
			break;
		}
	
	}

	return type;
}

int _graphicClear(msvg *m){
	if(m == NULL){
		return -1;
	}

	if (checkGraphics(m) < 0){
		GUI_ERR("%s:graphic recreate failed\n",__func__);
		return -1;
	}

	MGPlusGraphicClear(m->graphics,_SVG_DEFAULT_CLEAR_GRAPHICS_ARGB);
}


int _graphicDisplay(msvg *m){
	if(m == NULL){
		return -1;
	}

	if (checkGraphics(m) < 0){
		GUI_ERR("%s:graphic recreate failed\n",__func__);
		return -1;
	}

	MGPlusGraphicSave (m->graphics, m->hdc, 0, 0, 0, 0, 0, 0);
	m->opt->refresh((mwidget*)m);
}


int _line(msvg *m, int x0, int y0,int x1,int y1)
{
	if(m == NULL){
		return -1;
	}
	
	if (checkGraphics(m) < 0){
		GUI_ERR("%s:graphic recreate failed\n",__func__);
		return -1;
	}

	if (checkPen(m) < 0){
		GUI_ERR("%s:pen recreate failed\n",__func__);
		return -1;
	}
	
	MGPlusDrawLineI(m->graphics, m->pen, x0,y0,x1,y1);

	if (m->displayFlag){
		MGPlusGraphicSave (m->graphics, m->hdc, 0, 0, 0, 0, 0, 0);
	}
	
	m->opt->refresh((mwidget*)m);
	return 0;
}


int _fillPolygon(msvg *m, POINT *points, int num, int fillType)
{
	if(m == NULL){
		return -1;
	}

	if (num < 3)
	{
		GUI_ERR("fill polygon points num < 3(mix polygon point), num = %d\n", num);
		return -1;
	}

	if (checkGraphics(m) < 0){
		GUI_ERR("%s:graphic recreate failed\n",__func__);
		return -1;
	}

	HBRUSH fillbrush = 0;
	MPBrushType brushType = GetBrushFromType(m, fillType, &fillbrush);
	if (0 ==  fillbrush){
		fillbrush = MGPlusBrushCreate(brushType);
		if (0 ==  fillbrush){
			GUI_ERR("%s:fill brush recreate failed\n",__func__);
			return -1;
		}
	}

	HPATH path = MGPlusPathCreate(MP_PATH_FILL_MODE_WINDING);
	if (0 == path){
		GUI_ERR("%s:mgplus path create failed\n",__func__);
		return -1;
	}

	for(int index= 0; index < (num -1); index++){
		MGPlusPathAddLine(path, points[index].x, points[index].y, points[index+1].x, points[index+1].y);
	}
	MGPlusPathAddLine(path,points[num -1].x,points[num -1].y,points[0].x,points[0].y);

	MGPlusFillPath(m->graphics,fillbrush,path);

	MGPlusPathDelete(path);

	if (m->displayFlag){
		MGPlusGraphicSave (m->graphics, m->hdc, 0, 0, 0, 0, 0, 0);
	}

	m->opt->refresh((mwidget*)m);
	return 0;
}

int _circle(msvg *m, int x, int y, int r)
{
	if(m == NULL){
		return -1;
	}

	if (checkGraphics(m) < 0){
		GUI_ERR("%s:graphic recreate failed\n",__func__);
		return -1;
	}

	if (checkPen(m) < 0){
		GUI_ERR("%s:pen recreate failed\n",__func__);
		return -1;
	}

	/* 该接口支持绘制椭圆,根据,rx,ry      控制, rx,ry相同既是圆形      */
	int ret = MGPlusDrawEllipseI(m->graphics, m->pen, x, y, r, r); 
	if (MP_OK != ret)
	{
		GUI_ERR("%s: draw circle failed, errcode = %d\n", __func__, ret);
		return -1;
	}

	if (m->displayFlag){
		MGPlusGraphicSave (m->graphics, m->hdc, 0, 0, 0, 0, 0, 0);
	}

	m->opt->refresh((mwidget*)m);
	return 0;

}

int _fillCircle(msvg *m, int x, int y, int r, int fillType)
{
	if(m == NULL){
		return -1;
	}

	if (checkGraphics(m) < 0){
		GUI_ERR("%s:graphic recreate failed\n",__func__);
		return -1;
	}


	HBRUSH fillbrush = 0;
	MPBrushType brushType = GetBrushFromType(m, fillType, &fillbrush);
	if (0 ==  fillbrush){
		fillbrush = MGPlusBrushCreate(brushType);
		if (0 ==  fillbrush){
			GUI_ERR("%s:fill brush recreate failed\n",__func__);
			return -1;
		}
	}

	GUI_DEBUG("MGPlusFillEllipseI:%d %d %d %d %d %d\n", m->graphics, fillbrush, x, y, r,r);
	/* 该接口支持绘制椭圆,根据,rx,ry      控制, rx,ry相同既是圆形      */
	int ret = MGPlusFillEllipseI(m->graphics, fillbrush, x, y, r, r); 
	if (MP_OK != ret)
	{
		GUI_ERR("%s: fill circle failed, errcode = %d\n", __func__, ret);
		return -1;
	}

	if (m->displayFlag){
		MGPlusGraphicSave (m->graphics, m->hdc, 0, 0, 0, 0, 0, 0);
	}

	m->opt->refresh((mwidget*)m);
	return 0;
}

int _circleArc(msvg *m, int sx, int sy, int r, int ang1, int ang2)
{
	if(m == NULL){
		return -1;
	}

	if (checkGraphics(m) < 0){
		GUI_ERR("%s:graphic recreate failed\n",__func__);
		return -1;
	}

	if (checkPen(m) < 0){
		GUI_ERR("%s:pen recreate failed\n",__func__);
		return -1;
	}

	
	int w = r*2;
	int h = r*2;
	int x = sx - r;
	int y = sy - r;


	int ret = MGPlusDrawArcI(m->graphics, m->pen, x, y, w, h, ang1, ang2);
	if (MP_OK != ret)
	{
		GUI_ERR("%s: draw arc failed, errcode = %d\n", __func__, ret);
		return -1;
	}

	if (m->displayFlag){
		MGPlusGraphicSave (m->graphics, m->hdc, 0, 0, 0, 0, 0, 0);
	}

	m->opt->refresh((mwidget*)m);

	return 0;
}

int _arcEx(msvg *m, int x, int y, int w, int h, int ang1, int ang2)
{
	if(m == NULL){
		return -1;
	}

	if (checkGraphics(m) < 0){
		GUI_ERR("%s:graphic recreate failed\n",__func__);
		return -1;
	}

	if (checkPen(m) < 0){
		GUI_ERR("%s:pen recreate failed\n",__func__);
		return -1;
	}


	int ret = MGPlusDrawArcI(m->graphics, m->pen, x, y, w, h, ang1, ang2);
	if (MP_OK != ret)
	{
		GUI_ERR("%s: draw arc failed, errcode = %d\n", __func__, ret);
		return -1;
	}

	if (m->displayFlag){
		MGPlusGraphicSave (m->graphics, m->hdc, 0, 0, 0, 0, 0, 0);
	}

	m->opt->refresh((mwidget*)m);

	return 0;
}


int _fillArc(msvg *m, int x, int y, int w, int h, int ang1, int ang2, int fillType)
{
	if(m == NULL){
		return -1;
	}

	if (checkGraphics(m) < 0){
		GUI_ERR("%s:graphic recreate failed\n",__func__);
		return -1;
	}


	HBRUSH fillbrush = 0;
	MPBrushType brushType = GetBrushFromType(m, fillType, &fillbrush);
	if (0 ==  fillbrush){
		fillbrush = MGPlusBrushCreate(brushType);
		if (0 ==  fillbrush){
			GUI_ERR("%s:fill brush recreate failed\n",__func__);
			return -1;
		}
	}

	int ret = MGPlusFillArcI (m->graphics, fillbrush, x, y, w, h, ang1, ang2);
	if (MP_OK != ret)
	{
		GUI_ERR("%s: fill arc failed, errcode = %d\n", __func__, ret);
		return -1;
	}

	if (m->displayFlag){
		MGPlusGraphicSave (m->graphics, m->hdc, 0, 0, 0, 0, 0, 0);
	}

	m->opt->refresh((mwidget*)m);

	return 0;
}


int _setPenColor(msvg *m, unsigned int rgb, unsigned char alpha)
{
	if(m == NULL){
		return -1;
	}

	if (checkPen(m) < 0){
		GUI_ERR("%s:pen recreate failed\n",__func__);
		return -1;
	}

	ARGB argb = (rgb | (alpha << 24));

	int ret = MGPlusPenSetColor(m->pen, argb);
	if (MP_OK != ret)
	{
		GUI_ERR("%s: set pen color failed, errcode = %d\n", __func__, ret);
		return -1;
	}

	return 0;
}

int _setPenWidth(msvg *m, int width)
{
	if(m == NULL){
		return -1;
	}

	if (checkPen(m) < 0){
		GUI_ERR("%s:pen recreate failed\n",__func__);
		return -1;
	}	

	int ret = MGPlusPenSetWidth(m->pen, width);
	if (MP_OK != ret)
	{
		GUI_ERR("%s: set pen width failed, errcode = %d\n", __func__, ret);
		return -1;
	}
	return 0;
}

int _setSolidBrushColor(msvg *m, unsigned int rgb, unsigned char alpha)
{

	if(m == NULL){
		return -1;
	}


	if (0 ==  m->solidBrush){
		m->solidBrush = MGPlusBrushCreate(MP_BRUSH_TYPE_SOLIDCOLOR);
		if (0 ==  m->solidBrush){
			GUI_ERR("%s:solid brush recreate failed\n",__func__);
			return -1;
		}
	}

	ARGB argb = (rgb | (alpha << 24));

	int ret = MGPlusSetSolidBrushColor(m->solidBrush, argb);
	if (MP_OK != ret)
	{
		GUI_ERR("%s: set solid brush color failed, errcode = %d\n", __func__, ret);
		return -1;
	}

	return 0;
}

//目前设置纹理后，显示无效果
int _setTextureBrushImage(msvg *m, char* path)
{
	if(m == NULL){
		return -1;
	}

	if (0 ==  m->textureFillBrush){
		m->textureFillBrush = MGPlusBrushCreate(MP_BRUSH_TYPE_TEXTUREFILL);
		if (0 ==  m->textureFillBrush){
			GUI_ERR("%s:solid texture fill recreate failed\n",__func__);
			return -1;
		}
	}

	if (checkGraphics(m) < 0){
		GUI_ERR("%s:graphic recreate failed\n",__func__);
		return -1;
	}



	BITMAP *brushImg = NULL;
	if(path){
		int err_code = MGPlusGraphicLoadBitmapFromFile(m->graphics,0,path);
		if (err_code != ERR_BMP_OK) {
			GUI_ERR("%s: load %s bitmap failed\n",__func__,path);
			MGPlusGraphicUnLoadBitmap(m->graphics, 0);
			return -1;
		}
	}
	PBITMAP pBitmap = MGPlusGraphicGetBitmap(m->graphics, 0);

	int ret = MGPlusSetTextureBrushImage(m->textureFillBrush, pBitmap);
	if (MP_OK != ret)
	{
		GUI_ERR("%s: set texture brush image failed, errcode = %d\n", __func__, ret);
		return -1;
	}

	//MGPlusGraphicUnLoadBitmap(m->graphics, 0);
	
	
	return 0;
}


int _setPathGradientBrushCenterPoint(msvg *m,int x, int y)
{
	if(m == NULL){
		return -1;
	}

	if (0 ==  m->pathgradientBrush){
		m->pathgradientBrush = MGPlusBrushCreate(MP_BRUSH_TYPE_PATHGRADIENT);
		if (0 ==  m->pathgradientBrush){
			GUI_ERR("%s:path gradient brush recreate failed\n",__func__);
			return -1;
		}
	}

	MPPOINT point;
	memset(&point, 0, sizeof(MPPOINT));
	point.x = x;
	point.y = y;

	int ret = MGPlusSetPathGradientBrushCenterPoint(m->pathgradientBrush, &point);
	if (MP_OK != ret)
	{
		GUI_ERR("%s: set path gradient brush center point failed, errcode = %d\n", __func__, ret);
		return -1;
	}	

	return 0;
}


int _setPathGradientBrushCenterColor(msvg *m,unsigned int rgb, unsigned char alpha)
{
	if(m == NULL){
		return -1;
	}

	if (0 ==  m->pathgradientBrush){
		m->pathgradientBrush = MGPlusBrushCreate(MP_BRUSH_TYPE_PATHGRADIENT);
		if (0 ==  m->pathgradientBrush){
			GUI_ERR("%s:path gradient brush recreate failed\n",__func__);
			return -1;
		}
	}	

	ARGB argb = (rgb | (alpha << 24));

	int ret = MGPlusSetPathGradientBrushCenterColor(m->pathgradientBrush, argb);
	if (MP_OK != ret)
	{
		GUI_ERR("%s: set path gradient brush center color failed, errcode = %d\n", __func__, ret);
		return -1;
	}		

	return 0;
}

/***************************************************************
**函数名:_setPathGradientBrushSurroundColor
**描述:路径梯度画刷层次颜色及层次数设置
**参数:
**    m:  控件指针
**    rgb: 层次颜色rgb值
**    alpha: 层次颜色透明度
**    surroundCount: 路径梯度画刷层次总数
**返回值:
**    -1: 设置失败
**     0: 设置成功
**附加:与_setPathGradientBrushCenterColor配合使用
***************************************************************/
int _setPathGradientBrushSurroundColor(msvg *m, unsigned int rgb, unsigned char alpha, int surroundCount)
{
	if(m == NULL){
		return -1;
	}

	if (0 ==  m->pathgradientBrush){
		m->pathgradientBrush = MGPlusBrushCreate(MP_BRUSH_TYPE_PATHGRADIENT);
		if (0 ==  m->pathgradientBrush){
			GUI_ERR("%s:path gradient brush recreate failed\n",__func__);
			return -1;
		}
	}	

	ARGB argb = (rgb | (alpha << 24));

	int ret = MGPlusSetPathGradientBrushSurroundColors(m->pathgradientBrush, &argb, surroundCount);
	if (MP_OK != ret)
	{
		GUI_ERR("%s: set path gradient brush surround color failed, errcode = %d\n", __func__, ret);
		return -1;
	}		

	return 0;
}

int _setPathGradientBrushSurroundRect(msvg * m, int x, int y, int w, int h)
{
	if(m == NULL){
		return -1;
	}

	if (0 ==  m->pathgradientBrush){
		m->pathgradientBrush = MGPlusBrushCreate(MP_BRUSH_TYPE_PATHGRADIENT);
		if (0 ==  m->pathgradientBrush){
			GUI_ERR("%s:path gradient brush recreate failed\n",__func__);
			return -1;
		}
	}	

	RECT rect;	
	memset(&rect, 0, sizeof(RECT));
	rect.left = x;
	rect.top = y;
	rect.right = x + w;
	rect.bottom = y + h;
	
	int ret = MGPlusSetPathGradientBrushSurroundRect(m->pathgradientBrush, &rect);
	if (MP_OK != ret)
	{
		GUI_ERR("%s: set path gradient brush surround rect failed, errcode = %d\n", __func__, ret);
		return -1;
	}		

	return 0;
}


int _setLinearGradientBrushMode(msvg * m, unsigned int mode)
{
	if(m == NULL){
		return -1;
	}

	if (0 ==  m->lineargradientBrush){
		m->lineargradientBrush = MGPlusBrushCreate(MP_BRUSH_TYPE_LINEARGRADIENT);
		if (0 ==  m->lineargradientBrush){
			GUI_ERR("%s:linear gradient brush recreate failed\n",__func__);
			return -1;
		}
	}	

	MPLinearGradientMode lgMode = MP_LINEAR_GRADIENT_MODE_HORIZONTAL;
	switch(mode){
		case 0:
			lgMode = MP_LINEAR_GRADIENT_MODE_HORIZONTAL;	/* gradient horizontal.*/
			break;

		case 1: 
			lgMode = MP_LINEAR_GRADIENT_MODE_VERTICAL;	/* gradient vertica.*/
			break;
		case 2:
			lgMode = MP_LINEAR_GRADIENT_MODE_FORWARDDIAGONAL;	/* gradient forwarddiagonal.*/
			break;
		case 4: 
			lgMode = MP_LINEAR_GRADIENT_MODE_BACKWARDDIAGONAL;	/* gradient backwarddiagonal.*/
			break;
		default:
			lgMode = MP_LINEAR_GRADIENT_MODE_HORIZONTAL;	/* gradient horizontal.*/
			break;
	}


	int ret = MGPlusSetLinearGradientBrushMode(m->lineargradientBrush, lgMode);
	if (MP_OK != ret)
	{
		GUI_ERR("%s: set linear gradient brush mode failed, errcode = %d\n", __func__, ret);
		return -1;
	}		


	return 0;
}


int _setLinearGradientBrushRect(msvg * m, int x, int y, int w, int h)
{
	if(m == NULL){
		return -1;
	}

	if (0 ==  m->lineargradientBrush){
		m->lineargradientBrush = MGPlusBrushCreate(MP_BRUSH_TYPE_LINEARGRADIENT);
		if (0 ==  m->lineargradientBrush){
			GUI_ERR("%s:linear gradient brush recreate failed\n",__func__);
			return -1;
		}
	}	

	RECT rect;	
	memset(&rect, 0, sizeof(RECT));
	rect.left = x;
	rect.top = y;
	rect.right = x + w;
	rect.bottom = y + h;

	int ret = MGPlusSetLinearGradientBrushRect(m->lineargradientBrush, &rect);
	if (MP_OK != ret)
	{
		GUI_ERR("%s: set linear gradient brush rect failed, errcode = %d\n", __func__, ret);
		return -1;
	}		

}


int _setLinearGradientBrushColor(msvg * m, unsigned int rgb, unsigned char alpha, int gradientCount)
{
	if(m == NULL){
		return -1;
	}

	if (0 ==  m->lineargradientBrush){
		m->lineargradientBrush = MGPlusBrushCreate(MP_BRUSH_TYPE_LINEARGRADIENT);
		if (0 ==  m->lineargradientBrush){
			GUI_ERR("%s:linear gradient brush recreate failed\n",__func__);
			return -1;
		}
	}

	ARGB argb = (rgb | (alpha << 24));

	int ret = MGPlusSetLinearGradientBrushColors(m->lineargradientBrush, &argb, gradientCount);
	if (MP_OK != ret)
	{
		GUI_ERR("%s: set linear gradient brush color failed, errcode = %d\n", __func__, ret);
		return -1;
	}

	return 0;
}


int svg_line_set(msvg * m,char * value){
	if (NULL == value){
		GUI_ERR("svg draw line failed, param is NULL\n");
		return -1;
	}

	char *tmp = value;
	
	int penWidth = atoi(tmp);
	m->opt->setPenWidth(m, penWidth);

	tmp = strchr(tmp,',');
	if (NULL == tmp){
		GUI_ERR("svg draw line failed, pencolor is NULL\n");
		return -1;
	}
	tmp++;
	unsigned int rgb = strtoul(tmp, 0, 16);
	
	tmp = strchr(tmp,',');
	if (NULL == tmp){
		GUI_ERR("svg draw line failed, pencolor is NULL\n");
		return -1;
	}
	tmp++;
	unsigned int alpha = strtoul(tmp, 0, 16);
	m->opt->setPenColor(m, rgb, alpha);

	tmp = strchr(tmp,',');
	if (NULL == tmp){
		GUI_ERR("svg draw line failed, startx is NULL, param=%s\n", value);
		return -1;
	}
	tmp++;
	int start_x = atoi(tmp);
	
	tmp = strchr(tmp,',');
	if (NULL == tmp){
		GUI_ERR("svg draw line failed, starty is NULL, param=%s\n", value);
		return -1;
	}
	tmp++;
	
	int start_y = atoi(tmp);
	
	tmp = strchr(tmp,',');
	if (NULL == tmp){
		GUI_ERR("svg draw line failed, endx is NULL, param=%s\n", value);
		return -1;
	}
	tmp++;
	int end_x = atoi(tmp);
	
	tmp = strchr(tmp,',');
	if (NULL == tmp){
		GUI_ERR("svg draw line failed, endy is NULL, param=%s\n", value);
		return -1;
	}
	tmp++;
	
	int end_y = atoi(tmp);
	
	GUI_DEBUG("line width = %d pencolor = 0x%02x alpha = 0x%02x x0 = %d, y0 = %d x1 = %d y1= %d\n", 
	penWidth, rgb, alpha, start_x, start_y, end_x, 
	end_y);
	
	m->opt->line(m, start_x, start_y, end_x, end_y);

	return 0;
}

char *svg_line_get(msvg *m){
	return NULL;
}

int svg_fill_polygon_set(msvg * m,char * value){
	if (NULL == value){
		GUI_ERR("svg fill polygon failed, param is NULL\n");
		return -1;
	}

	char *tmp = value;
	unsigned int rgb = strtoul(tmp, 0, 16);
	
	tmp = strchr(tmp,',');
	if (NULL == tmp){
		GUI_ERR("svg draw line failed, pencolor is NULL\n");
		return -1;
	}
	tmp++;
	unsigned int alpha = strtoul(tmp, 0, 16);
	m->opt->setSolidBrushColor(m, rgb, alpha);	

	tmp = strchr(tmp,',');
	if (NULL == tmp){
		GUI_ERR("svg fill polygon failed, points count is NULL, param=%s\n", value);
		return -1;
	}
	tmp++;

	int polygonNum = atoi(tmp);
	if(polygonNum <= 0)
	{
		GUI_ERR("svg fill polygon failed, points count is error, param=%s\n", value);
		return -1;
	}
	POINT *points = (POINT *)calloc(1, sizeof(POINT) * polygonNum);
	memset(points, 0, sizeof(POINT) * polygonNum);
	for (int i = 0; i < polygonNum; i++){
		tmp = strchr(tmp,',');
		if (NULL == tmp){
			GUI_ERR("svg fill polygon failed, points x is not match  counts, param=%s\n", value);
			return -1;
		}
		tmp++;	

		points[i].x = atoi(tmp);

		tmp = strchr(tmp,',');
		if (NULL == tmp){
			GUI_ERR("svg fill polygon failed, points y is not match  counts, param=%s\n", value);
			return -1;
		}
		tmp++;	
		
		points[i].y = atoi(tmp);
	}
	
	m->opt->fillPolygon (m, points, polygonNum, 0);

}

char *svg_fill_polygon_get(msvg *m){
	return NULL;
}


int svg_circle_set(msvg * m,char * value){
	if (NULL == value){
		GUI_ERR("svg draw circle failed, param is NULL\n");
		return -1;
	}

	char *tmp = value;
	
	int penWidth = atoi(tmp);
	m->opt->setPenWidth(m, penWidth);

	tmp = strchr(tmp,',');
	if (NULL == tmp){
		GUI_ERR("svg draw line failed, pencolor is NULL\n");
		return -1;
	}
	tmp++;
	unsigned int rgb = strtoul(tmp, 0, 16);
	
	tmp = strchr(tmp,',');
	if (NULL == tmp){
		GUI_ERR("svg draw line failed, pencolor is NULL\n");
		return -1;
	}
	tmp++;
	unsigned int alpha = strtoul(tmp, 0, 16);
	m->opt->setPenColor(m, rgb, alpha);

	tmp = strchr(tmp,',');
	if (NULL == tmp){
		GUI_ERR("svg draw circle failed, x is NULL, param=%s\n", value);
		return -1;
	}
	tmp++;

	int x = atoi(tmp);

	tmp = strchr(tmp,',');
	if (NULL == tmp){
		GUI_ERR("svg draw circle failed, y is NULL, param=%s\n", value);
		return -1;
	}
	tmp++;

	int y = atoi(tmp);

	tmp = strchr(tmp,',');
	if (NULL == tmp){
		GUI_ERR("svg draw circle failed, r is NULL, param=%s\n", value);
		return -1;
	}
	tmp++;

	int r = atoi(tmp);

	m->opt->circle(m, x, y, r);
}

char *svg_circle_get(msvg * m){
	return NULL;
}

int svg_solid_circlar_set(msvg * m,char * value){
	if (NULL == value){
		GUI_ERR("svg solid circle failed, param is NULL\n");
		return -1;
	}

	char *tmp = value;
	
	unsigned int rgb = strtoul(tmp, 0, 16);
	
	tmp = strchr(tmp,',');
	if (NULL == tmp){
		GUI_ERR("svg draw line failed, pencolor is NULL\n");
		return -1;
	}
	tmp++;
	unsigned int alpha = strtoul(tmp, 0, 16);
	m->opt->setSolidBrushColor(m, rgb, alpha);	
	
	tmp = strchr(tmp,',');
	if (NULL == tmp){
		GUI_ERR("svg solid circle failed, x is NULL, param=%s\n", value);
		return -1;
	}
	tmp++;

	int x = atoi(tmp);

	tmp = strchr(tmp,',');
	if (NULL == tmp){
		GUI_ERR("svg solid circle failed, y is NULL, param=%s\n", value);
		return -1;
	}
	tmp++;

	int y = atoi(tmp);

	tmp = strchr(tmp,',');
	if (NULL == tmp){
		GUI_ERR("svg solid circle failed, r is NULL, param=%s\n", value);
		return -1;
	}
	tmp++;

	int r = atoi(tmp);

	m->opt->fillCircle(m, x, y, r, 0);	
}

char *svg_solid_circlar_get(msvg * m){
	return NULL;
}



int svg_circle_arc_set(msvg *m, char *value){
	if (NULL == value){
		GUI_ERR("svg draw circle failed, param is NULL\n");
		return -1;
	}

	char *tmp = value;
	
	int penWidth = atoi(tmp);
	m->opt->setPenWidth(m, penWidth);

	tmp = strchr(tmp,',');
	if (NULL == tmp){
		GUI_ERR("svg draw line failed, pencolor is NULL\n");
		return -1;
	}
	tmp++;
	unsigned int rgb = strtoul(tmp, 0, 16);
	
	tmp = strchr(tmp,',');
	if (NULL == tmp){
		GUI_ERR("svg draw line failed, pencolor is NULL\n");
		return -1;
	}
	tmp++;
	unsigned int alpha = strtoul(tmp, 0, 16);
	m->opt->setPenColor(m, rgb, alpha);
	
	tmp = strchr(tmp,',');
	if (NULL == tmp){
		GUI_ERR("svg draw circle failed, x is NULL, param=%s\n", value);
		return -1;
	}
	tmp++;

	int x = atoi(tmp);

	tmp = strchr(tmp,',');
	if (NULL == tmp){
		GUI_ERR("svg draw circle failed, y is NULL, param=%s\n", value);
		return -1;
	}
	tmp++;

	int y = atoi(tmp);	

	tmp = strchr(tmp,',');
	if (NULL == tmp){
		GUI_ERR("svg draw circle failed, r is NULL, param=%s\n", value);
		return -1;
	}
	tmp++;

	int r = atoi(tmp);

	tmp = strchr(tmp,',');
	if (NULL == tmp){
		GUI_ERR("svg draw circle failed, ang1 is NULL, param=%s\n", value);
		return -1;
	}
	tmp++;

	int ang1 = atoi(tmp);

	tmp = strchr(tmp,',');
	if (NULL == tmp){
		GUI_ERR("svg draw circle failed, ang2 is NULL, param=%s\n", value);
		return -1;
	}
	tmp++;

	int ang2 = atoi(tmp);

	GUI_DEBUG("circleArc penWidth:%d,color:0x%02x,alpha=0x%02x,x:%d,y:%d,r:%d,ang1:%d,ang2:%d\n",
		penWidth,rgb, alpha,x,y,r,ang1, ang2);
	m->opt->circleArc(m,x,y,r,ang1, ang2);

	return 0;
}

char *svg_circle_arc_get(msvg *m){
	return NULL;
}

int svg_arc_set(msvg *m, char *value){
	if (NULL == value){
		GUI_ERR("svg draw arc failed, param is NULL\n");
		return -1;
	}

	char *tmp = value;
	
	int penWidth = atoi(tmp);
	m->opt->setPenWidth(m, penWidth);

	tmp = strchr(tmp,',');
	if (NULL == tmp){
		GUI_ERR("svg draw line failed, pencolor is NULL\n");
		return -1;
	}
	tmp++;
	unsigned int rgb = strtoul(tmp, 0, 16);
	
	tmp = strchr(tmp,',');
	if (NULL == tmp){
		GUI_ERR("svg draw line failed, pencolor is NULL\n");
		return -1;
	}
	tmp++;
	unsigned int alpha = strtoul(tmp, 0, 16);
	m->opt->setPenColor(m, rgb, alpha);
	
	tmp = strchr(tmp,',');
	if (NULL == tmp){
		GUI_ERR("svg draw arc failed, x is NULL, param=%s\n", value);
		return -1;
	}
	tmp++;

	int x = atoi(tmp);

	tmp = strchr(tmp,',');
	if (NULL == tmp){
		GUI_ERR("svg draw arc failed, y is NULL, param=%s\n", value);
		return -1;
	}
	tmp++;

	int y = atoi(tmp);	

	tmp = strchr(tmp,',');
	if (NULL == tmp){
		GUI_ERR("svg draw arc failed, w is NULL, param=%s\n", value);
		return -1;
	}
	tmp++;
	int w = atoi(tmp);

	tmp = strchr(tmp,',');
	if (NULL == tmp){
		GUI_ERR("svg draw arc failed, h is NULL, param=%s\n", value);
		return -1;
	}
	tmp++;
	
	int h = atoi(tmp);

	tmp = strchr(tmp,',');
	if (NULL == tmp){
		GUI_ERR("svg draw arc failed, ang1 is NULL, param=%s\n", value);
		return -1;
	}
	tmp++;

	int ang1 = atoi(tmp);

	tmp = strchr(tmp,',');
	if (NULL == tmp){
		GUI_ERR("svg draw arc failed, ang2 is NULL, param=%s\n", value);
		return -1;
	}
	tmp++;

	int ang2 = atoi(tmp);

	GUI_DEBUG("circleArc penWidth:%d,penColor:0x%02x,alpha:0x%02x,x:%d,y:%d,w:%d,h:%d,ang1:%d,ang2:%d\n",
		penWidth,rgb,alpha,x,y,w,h,ang1, ang2);
	m->opt->arcEx(m,x,y,w,h,ang1, ang2);

	return 0;
}

char *svg_arc_get(msvg *m){
	return NULL;
}

int svg_solid_fill_arc_set(msvg *m, char *value){
	if (NULL == value){
		GUI_ERR("svg solid fill arc failed, param is NULL\n");
		return -1;
	}

	char *tmp = value;

	unsigned int rgb = strtoul(tmp, 0, 16);
	
	tmp = strchr(tmp,',');
	if (NULL == tmp){
		GUI_ERR("svg draw line failed, pencolor is NULL\n");
		return -1;
	}
	tmp++;
	unsigned int alpha = strtoul(tmp, 0, 16);
	m->opt->setSolidBrushColor(m, rgb, alpha);

	tmp = strchr(tmp,',');
	if (NULL == tmp){
		GUI_ERR("svg solid fill arc failed, x is NULL, param=%s\n", value);
		return -1;
	}
	tmp++;

	int x = atoi(tmp);

	tmp = strchr(tmp,',');
	if (NULL == tmp){
		GUI_ERR("svg solid fill arc failed, y is NULL, param=%s\n", value);
		return -1;
	}
	tmp++;

	int y = atoi(tmp);	

	tmp = strchr(tmp,',');
	if (NULL == tmp){
		GUI_ERR("svg solid fill arc failed, w is NULL, param=%s\n", value);
		return -1;
	}
	tmp++;
	int w = atoi(tmp);

	tmp = strchr(tmp,',');
	if (NULL == tmp){
		GUI_ERR("svg solid fill arc failed, h is NULL, param=%s\n", value);
		return -1;
	}
	tmp++;
	
	int h = atoi(tmp);

	tmp = strchr(tmp,',');
	if (NULL == tmp){
		GUI_ERR("svg solid fill arc failed, ang1 is NULL, param=%s\n", value);
		return -1;
	}
	tmp++;

	int ang1 = atoi(tmp);

	tmp = strchr(tmp,',');
	if (NULL == tmp){
		GUI_ERR("svg solid fill arc failed, ang2 is NULL, param=%s\n", value);
		return -1;
	}
	tmp++;

	int ang2 = atoi(tmp);

	GUI_DEBUG("circleArc brushColor:0x%02x,alpha:0x%02x,x:%d,y:%d,w:%d,h:%d,ang1:%d,ang2:%d\n",
		rgb,alpha,x,y,w,h,ang1, ang2);
	m->opt->fillArc(m,x,y,w,h,ang1, ang2, 0);

	return 0;
}

char *svg_solid_fill_arc_get(msvg *m){
	return NULL;
}

int svg_display_flag_set(msvg *m, char *value){
	if (NULL == value){
		GUI_ERR("svg set display failed, param is NULL\n");
		return -1;
	}

	char *tmp = value;

	int displayFlag = atoi(tmp);	
	m->displayFlag = displayFlag;
	if (m->displayFlag){
		m->opt->graphicDisplay(m);
	}	
}

char *svg_display_flag_get(msvg *m){
	char *buf = widget_calloc(1,12);
	sprintf(buf, "%d", m->displayFlag);

	return buf;
}



_WIDGET_GENERATE(svg,widget)
