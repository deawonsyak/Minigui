#ifndef __SVG_WIDGET_H__
#define __SVG_WIDGET_H__

#include "widget.h"
#include "mgplus/mgplus.h"
struct _svg_widget;


#define _SVG_DEFAULT_PEN_WIDTH  (1)
#define _SVG_DEFAULT_PEN_ARGB  (0xffffffff)
#define _SVG_DEFAULT_CLEAR_GRAPHICS_ARGB (0x00000000)


#define _SVG_CLASS(clas_name) \
	_WIDGET_CLASS(clas_name) \
	HDC hdc; \
	HPEN 	pen; \
	HGRAPHICS graphics; \
	HBRUSH	solidBrush; \
	HBRUSH	hatchFillBrush; \
	HBRUSH	textureFillBrush; \
	HBRUSH	pathgradientBrush; \
	HBRUSH	lineargradientBrush; \
	unsigned char destroy_flag; \
	unsigned char displayFlag; \
	//unsigned char clear_flag; \
	//unsigned char auto_refresh_flag;
	

#define _svg_EVENT_LIST \
	_widget_EVENT_LIST

#define _svg_PROPERTYS \
	{"line",svg_line_set,svg_line_get}, \
	{"fillPolygon",svg_fill_polygon_set,svg_fill_polygon_get}, \
	{"circle",svg_circle_set,svg_circle_get}, \
	{"solidCircular",svg_solid_circlar_set,svg_solid_circlar_get}, \
	{"circleArc",svg_circle_arc_set,svg_circle_arc_get}, \
	{"arc",svg_arc_set,svg_arc_get}, \
	{"solidFillArc",svg_solid_fill_arc_set,svg_solid_fill_arc_get}, \
	{"displayFlag", svg_display_flag_set, svg_display_flag_get}, \
	_widget_PROPERTYS


#define _SVG_OPT(parent) \
	_WIDGET_OPT(parent) \
	int (*line)(struct _svg_widget *m, int x0, int y0, int x1,int y1); \
	int (*fillPolygon)(struct _svg_widget *m, POINT *points, int num, int fillType); \
	int (*circle)(struct _svg_widget *m, int x, int y, int r); \
	int (*fillCircle)(struct _svg_widget *m, int x, int y, int r, int fillType); \
	int (*circleArc)(struct _svg_widget *m, int sx, int sy, int r, int ang1, int ang2); \
	int (*arcEx)(struct _svg_widget *m, int x, int y, int w, int h, int ang1, int ang2); \
	int (*fillArc)(struct _svg_widget *m, int x, int y, int w, int h, int ang1, int ang2, int fillType); \
	int (*setPenColor)(struct _svg_widget *m, unsigned int rgb, unsigned char alpha); \
	int (*setPenWidth)(struct _svg_widget *m, int width); \
	int (*setSolidBrushColor)(struct _svg_widget *m, unsigned int rgb, unsigned char alpha); \
	int (*setTextureBrushImage)(struct _svg_widget *m, char* path);\
	int (*setPathGradientBrushCenterPoint)(struct _svg_widget *m,int x, int y);\
	int (*setPathGradientBrushCenterColor)(struct _svg_widget *m,unsigned int rgb, unsigned char alpha);\
	int (*setPathGradientBrushSurroundColor)(struct _svg_widget *m, unsigned int rgb, unsigned char alpha, int surroundCount);\
	int (*setPathGradientBrushSurroundRect)(struct _svg_widget * m, int x, int y, int w, int h);\
	int (*setLinearGradientBrushMode)(struct _svg_widget * m, unsigned int mode);\
	int (*setLinearGradientBrushRect)(struct _svg_widget * m, int x, int y, int w, int h);\
	int (*setLinearGradientBrushColor)(struct _svg_widget * m, unsigned int rgb, unsigned char alpha, int gradientCount);\
	int (*graphicDisplay)(struct _svg_widget *m);\
	int (*graphicClear)(struct _svg_widget *m);\
	int (*mgplus_test)(struct _svg_widget *m);

	

#define _svg_OPT_BEGIN(clas_name,parent) \
	_widget_OPT_BEGIN(clas_name,parent) \
	.paint = svg_paint, \
	.destroy = svg_destroy, \
	.line = _line, \
	.fillPolygon = _fillPolygon, \
	.circle = _circle, \
	.fillCircle = _fillCircle, \
	.circleArc = _circleArc, \
	.arcEx = _arcEx, \
	.fillArc = _fillArc, \
	.setPenColor = _setPenColor, \
	.setPenWidth = _setPenWidth, \
	.setSolidBrushColor = _setSolidBrushColor, \
	.setTextureBrushImage = _setTextureBrushImage, \
	.setPathGradientBrushCenterPoint = _setPathGradientBrushCenterPoint, \
	.setPathGradientBrushCenterColor = _setPathGradientBrushCenterColor, \
	.setPathGradientBrushSurroundColor = _setPathGradientBrushSurroundColor,\
	.setPathGradientBrushSurroundRect = _setPathGradientBrushSurroundRect, \
	.setLinearGradientBrushMode = _setLinearGradientBrushMode, \
	.setLinearGradientBrushRect = _setLinearGradientBrushRect, \
	.setLinearGradientBrushColor = _setLinearGradientBrushColor, \
	.graphicDisplay = _graphicDisplay, \
	.graphicClear = _graphicClear, \
	.mgplus_test = mgplus_test,
	

#define _svg_CREATE(clas_name,_name,x,y,w,h) \
	_WIDGET_CREATE(clas_name,_name,x,y,w,h) \
	m->hdc = screen_create_hdc(w,h); \
	m->pen = MGPlusPenCreate(_SVG_DEFAULT_PEN_WIDTH, _SVG_DEFAULT_PEN_ARGB); \
	m->graphics = MGPlusGraphicCreateFromDC(m->hdc); \
	m->solidBrush = MGPlusBrushCreate(MP_BRUSH_TYPE_SOLIDCOLOR); \
	m->hatchFillBrush = MGPlusBrushCreate(MP_BRUSH_TYPE_HATCHFILL); \
	m->textureFillBrush = MGPlusBrushCreate(MP_BRUSH_TYPE_TEXTUREFILL); \
	m->pathgradientBrush = MGPlusBrushCreate(MP_BRUSH_TYPE_PATHGRADIENT); \
	m->lineargradientBrush = MGPlusBrushCreate(MP_BRUSH_TYPE_LINEARGRADIENT); \
	m->destroy_flag = 1; \
	m->displayFlag = 1;

typedef struct _svg_opt{
	_SVG_OPT(widget)	
}svg_opt;

extern svg_opt gpsvg_opt;

typedef struct _svg_widget{
	_SVG_CLASS(svg)
}msvg;


msvg *create_svg_widget(char *name, int x, int y, int w, int h);

int _graphicDisplay(msvg *m);
int _graphicClear(msvg *m);
int _setPenWidth(msvg *m, int width);
int _setPenColor(msvg *m, unsigned int rgb, unsigned char alpha);
int _arcEx(msvg *m, int x, int y, int w, int h, int ang1, int ang2);
int _circleArc(msvg *m, int sx, int sy, int r, int ang1, int ang2);
int _fillArc(msvg *m, int x, int y, int w, int h, int ang1, int ang2, int fillType);
int _fillCircle(msvg *m, int x, int y, int r, int fillType);
int _circle(msvg *m, int x, int y, int r);
int _fillPolygon(msvg *m, POINT *points, int num, int fillType);
int _line(msvg *m, int x0, int y0,int x1,int y1);
int _setSolidBrushColor(msvg *m, unsigned int rgb, unsigned char alpha);
int _setTextureBrushImage(msvg *m, char* path);
int _setPathGradientBrushCenterPoint(msvg *m,int x, int y);
int _setPathGradientBrushCenterColor(msvg *m,unsigned int rgb, unsigned char alpha);
int _setPathGradientBrushSurroundColor(msvg *m, unsigned int rgb, unsigned char alpha, int surroundCount);
int _setPathGradientBrushSurroundRect(msvg * m, int x, int y, int w, int h);
int _setLinearGradientBrushMode(msvg * m, unsigned int mode);
int _setLinearGradientBrushRect(msvg * m, int x, int y, int w, int h);
int _setLinearGradientBrushColor(msvg * m, unsigned int rgb, unsigned char alpha, int gradientCount);
void svg_destroy(mwidget *node,widget_opt *clas);
int svg_paint(mwidget *node, HDC hdc);
int mgplus_test(msvg *m);

#endif

