#ifndef __INPUTBOX_WIDGET_H__
#define __INPUTBOX_WIDGET_H__

#include "widget.h"
#include "text_widget.h"

typedef struct inputCursor{
	int x;
	int y;
	int w;
	int h;
	int defaultW;
	int defaultH;
	gal_pixel	 	color;
}inputCursor;

enum inputBorder{
	NORMAL_INPUTBORDER,
	SIMPLE_INPUTBORDER
};

typedef struct inputboxData{
	textStyle *inputTextStyle;
	char *inputText;		//input data
	int inputPoint;			//the number in inputText[] for text add to
	int maxLen;		//buffer length
	int dataLen;	//current data length	
	
	textStyle *tipsStyle;
	char *tipsText;

	inputCursor *cursor;
	int oldLeftLen;		//每次画完文本，记录文本宽度，下一次更新光标x坐标时需要参考这个值
	int oldCursorX;		//每次画完文本，记录光标位置，下一次更新光标x坐标时需要参考这个值
	int touchX;			
	char touchEventFlag:1;	//点击事件发生时置1，否则为0
	char cursorHideFlag:1;	//光标超出显示区域时置1，否则为0
	enum inputBorder border;
}inputboxData;

struct _inputbox_widget;

void create_inputbox_data(struct _inputbox_widget *m);

#define _INPUTBOX_CLASS(clas_name) \
	_WIDGET_CLASS(clas_name) \
	inputboxData *inputbox;		\


#define _inputbox_EVENT_LIST \
	_widget_EVENT_LIST


#define _inputbox_PROPERTYS \
	{"textSize",inputbox_textsize_set,inputbox_textsize_get}, \
	{"fontType",inputbox_font_set,inputbox_font_get}, \
	{"textColor",inputbox_color_set,inputbox_color_get}, \
	{"transparentBK",inputbox_transparent_back_flag_set,inputbox_transparent_back_flag_get}, \
	{"border",inputbox_border_set,inputbox_border_get}, 	\
	{"textBKColor",inputbox_back_color_set,inputbox_back_color_get}, \
	{"tipsText",inputbox_tips_text_set,inputbox_tips_text_get}, \
	{"tipsTextSize",inputbox_tips_textsize_set,inputbox_tips_textsize_get}, \
	{"tipsFontType",inputbox_tips_font_set,inputbox_tips_font_get}, \
	{"tipsTextColor",inputbox_tips_color_set,inputbox_tips_color_get}, \
	{"tipsTransparentBK",inputbox_tips_transparent_back_flag_set,inputbox_tips_transparent_back_flag_get}, \
	{"tipsTextBKColor",inputbox_tips_back_color_set,inputbox_tips_back_color_get}, 	\
	_widget_PROPERTYS

//	{"text",inputbox_text_set,inputbox_text_get}, \		//调试用

#define _INPUTBOX_OPT(parent) \
	_WIDGET_OPT(parent) 		\

#define _inputbox_OPT_BEGIN(clas_name,parent) \
	_WIDGET_OPT_BEGIN(clas_name,parent) \
	.paint = inputbox_node_paint, \
	.destroy = inputbox_destroy, \	
	.touch_event_handle = input_touch_event_handle,	\
	.keyboard_event_handle = inputbox_keyboard_event_handle,


//创建控件，使用默认配置
#define _inputbox_CREATE(clas_name,_name,x,y,w,h) \
	_WIDGET_CREATE(clas_name,_name,x,y,w,h) \
	create_inputbox_data(m);

typedef struct _inputbox_opt{
	_INPUTBOX_OPT(widget)
}inputbox_opt;

typedef struct _inputbox_widget{
	_INPUTBOX_CLASS(inputbox)
}minputbox;

minputbox *create_inputbox_widget(char *name, int x, int y, int w, int h);

#endif
