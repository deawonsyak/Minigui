#ifndef __WIDGET_MANAGER_H__
#define __WIDGET_MANAGER_H__

#include "widget.h"

struct widget_info_t{
	char	*name;
	mwidget *(*create)(char *name,int x,int y, int w, int h);
};

mwidget *create_widget_by_name(char *widget, char *name, int x, int y, int w, int h);

#endif
