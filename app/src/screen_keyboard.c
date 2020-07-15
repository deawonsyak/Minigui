#include "screen_keyboard.h"
#include "page_xml.h"
#include "widget_manager.h"
#include "ixmlwidget_widget.h"

static mwidget *g_keyboard = NULL;

int KeyboardOpen(char *name)
{
	if(name == NULL){
		return -1;
	}


	mixmlwidget *keyboard = (mixmlwidget*)create_widget_by_name(name,"keyboard",0,0,0,0);
	if(keyboard == NULL){
		GUI_ERR("Can't find keyboard widget %s",name);
		return -1;
	}

	keyboard->x = keyboard->xmlwidget->x;
	keyboard->y = keyboard->xmlwidget->y;
	keyboard->w = keyboard->xmlwidget->w;
	keyboard->h = keyboard->xmlwidget->h;

	if(add_node_to_toollayer((mwidget*)keyboard)){
		GUI_ERR("Add keyboard to screen fail");
		keyboard->opt->destroy((mwidget*)keyboard,(widget_opt*)keyboard->opt);
		goto err;
	}

	g_keyboard = (mwidget*)keyboard;

err:
	return -1;

}

int KeyboardClose(void)
{
	screen_stop_node(g_keyboard,g_keyboard->screen);
	g_keyboard->opt->destroy(g_keyboard,g_keyboard->opt);
	char key[3] = {0xff,0xff,0};
	ScreenKeyboardMsgSend(key);
	return 0;
}
