#ifndef __TEXT_WIDGET_H__
#define __TEXT_WIDGET_H__

#include <minigui/common.h>
#include <minigui/minigui.h>
#include <minigui/gdi.h>
#include <minigui/window.h>
#include <minigui/control.h>

#include "widget.h"

struct _text_widget;
#define ALIGN_TO_LEFT 0x01
#define ALIGN_TO_RIGHT 0x02
#define ALIGN_TO_TOP 0x04
#define ALIGN_TO_BOTTOM 0x08
#define ALIGN_TO_CENTER 0x10

enum {
	TEXT_FONT_TYPE_PINGFANGREGULAR = 0,
	TEXT_FONT_TYPE_ROBOTOREGULAR,
	TEXT_FONT_TYPE_STHeitiSCMedium,
};


enum {
	TEXT_PROPERTY_DISABLE = 0,
	TEXT_PROPERTY_ENABLE,
};


#define TEXT_DEFAULT_FONT_TYPE (TEXT_FONT_TYPE_PINGFANGREGULAR)
#define TEXT_DEFAULT_TEXT_SIZE (20)
#define TEXT_DEFAULT_TEXT_COLOR (0xffffff)
#define TEXT_DEFAULT_TEXT_ALPHA (0xff)
#define TEXT_DEFAULT_TEXT_BKCOLOR (0x000000)
#define TEXT_DEFAULT_TRANSPARENTBK_FLAG (TEXT_PROPERTY_ENABLE)
#define TEXT_DEFAULT_TEXT_ALIGN (ALIGN_TO_LEFT)
#define TEXT_DEFAULT_TEXT_UNDERLINE (TEXT_PROPERTY_DISABLE)


typedef struct text_style{
	int				refer_num;

	gal_pixel	 	textColor;
	gal_pixel	 	textBKColor;
	unsigned char   textAlpha;
	char            reverse[3];
	int				textSize;
	char			weight;
	unsigned char	transparentBKFlag;  //1为设置为透明色
	unsigned char	underlineFlag;		//添加下划线：1
	unsigned char	fontType;
	unsigned int	textAlign;
	
	PLOGFONT     	textFONT;

	struct text_style *next;
}textStyle;

extern void textStyle_delete(textStyle *style);

typedef struct style_info{
	char styleStr[32];
	int styleType;
}StyleInfo;


#define _TEXT_CLASS(clas_name) \
	_WIDGET_CLASS(clas_name) \
	textStyle *style; \
	char *text;

#define _text_EVENT_LIST \
	_widget_EVENT_LIST

#define _text_PROPERTYS \
	{"text",text_text_set,text_text_get}, \
	{"style",text_style_set,text_style_get}, \
	{"fontType",text_font_set,text_font_get}, \
	{"textSize",text_size_set,text_size_get}, \
	{"textAlign",text_align_set,text_align_get}, \
	{"textColor",text_color_set,text_color_get}, \
	{"textAlpha",text_alpha_set,text_alpha_get}, \
	{"transparentBK",text_transparent_back_flag_set,text_transparent_back_flag_get}, \
	{"textBKColor",text_back_color_set,text_back_color_get}, \
	{"underline",text_underline_set,text_underline_get}, \
	_widget_PROPERTYS

#define _TEXT_OPT(parent) \
	_WIDGET_OPT(parent) \
	void (*change_text)(struct _text_widget *m, char *text); \
	void (*set_style)(struct _text_widget *m, textStyle *style); \
	void (*set_default_style) (struct _text_widget *m);

#define _text_OPT_BEGIN(clas_name,parent) \
	_WIDGET_OPT_BEGIN(clas_name,parent) \
	.paint = text_node_paint, \
	.destroy = text_destroy, \
	.change_text = change_text,\
	.set_style = set_style, \
	.set_default_style = set_default_style,

#define _text_CREATE(clas_name,_name,x,y,w,h) \
	_WIDGET_CREATE(clas_name,_name,x,y,w,h) \
	m->opt->set_default_style(m);

typedef struct _text_opt{
	_TEXT_OPT(widget)
}text_opt;

typedef struct _text_widget{
	_TEXT_CLASS(text)
}mtext;

mtext *create_text_widget(char *name, int x, int y, int w, int h);
#endif

