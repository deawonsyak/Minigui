
#include "widget_manager.h"
#include "screen.h"

#include "box_widget.h"
#include "rect_widget.h"
#include "image_widget.h"
#include "text_widget.h"
#include "button_widget.h"
#include "svg_widget.h"
#include "slide_widget.h"
#include "slideloop_widget.h"
#include "slidescreen_widget.h"
#include "card_widget.h"
#include "icard_widget.h"
#include "xmlwidget_widget.h"
#include "inputbox_widget.h"
#include "trackbar_widget.h"
#include "table_widget.h"

static struct widget_info_t widgets_info[] = {
	{"widget", create_widget},
	{"box", create_box_widget},
	{"rect", create_rect_widget},
	{"image",create_image_widget},
	{"text",create_text_widget},
	{"button",create_button_widget},
	{"svg",create_svg_widget},
	{"slide",create_slide_widget},
	{"slideloop",create_slideloop_widget},
	{"slidescreen",create_slidescreen_widget},
	{"card",create_card_widget},
	{"icard",create_icard_widget},
	{"xmlwidget", create_xmlwidget_widget},
	{"inputbox", create_inputbox_widget},
	{"trackbar", create_trackbar_widget},
	{"table", create_table_widget}
};

mwidget *create_widget_by_name(char *widget, char *name, int x, int y, int w, int h)
{
	mwidget *m;
	unsigned int i;
	for(i = 0; i < sizeof(widgets_info)/sizeof(widgets_info[0]); i++){
		if(strcmp(widgets_info[i].name, widget) == 0){
			m = widgets_info[i].create(name,x,y,w,h);
			return m;
		}
	}

	mxmlwidget *mxml = orb_list_get_first(g_screen.xmlwidget_list);
	while(mxml){
		if(strcmp(widget,mxml->name) == 0){
			m = mxml->opt->create_widget(mxml,name,x,y,w,h);
			return m;
		}
		mxml = orb_list_get_next(g_screen.xmlwidget_list);
	}

	return NULL;
}
