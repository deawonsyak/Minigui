#ifndef __CARD_WIDGET_H__
#define __CARD_WIDGET_H__

#include "widget.h"
#include "orb_list.h"

struct _card_widget;

#define _CARD_CLASS(clas_name) \
	_WIDGET_CLASS(clas_name) \
	orb_list_t *icard_list;
	

#define _card_EVENT_LIST \
	_widget_EVENT_LIST

#define _card_PROPERTYS \
	_widget_PROPERTYS

#define _card_OPT(parent) \
	_WIDGET_OPT(parent) \
	int (*add_to_icard)(struct _card_widget *m, mwidget *icard); \
	int (*del_from_icard)(struct _card_widget *m, mwidget *icard);


#define _card_OPT_BEGIN(clas_name,parent) \
	_WIDGET_OPT_BEGIN(clas_name,parent)\
	.refresh = card_refresh, \
	.add_to_icard = card_add_to_icard, \
	.del_from_icard = card_del_from_icard,


#define _card_CREATE(clas_name,_name,x,y,w,h) \
	_WIDGET_CREATE(clas_name,_name,x,y,w,h) \
	m->icard_list = orb_list_create(); \

typedef struct _card_opt{
	_card_OPT(widget)
}card_opt;

typedef struct _card_widget{
	_CARD_CLASS(card)
}mcard;

mcard *create_card_widget(char *name, int x, int y, int w, int h);

#endif

