#include "card_widget.h"
#include "screen.h"

void card_refresh(mwidget *node)
{
	mcard *m = (mcard*)node;

	mwidget *parent = orb_list_get_first(m->icard_list);
	while(parent){
		parent->need_paint = 1;
		parent = orb_list_get_next(m->icard_list);
	}
	m->opt->parent->refresh(node);
}

int card_add_to_icard(mcard *m, mwidget *icard)
{
	if(m == NULL || icard == NULL){
		return -1;
	}

	m->opt->lock_data(m);
	orb_list_add(m->icard_list,icard);
	m->opt->unlock_data(m);

	return 0;
}

int card_del_from_icard(mcard *m, mwidget *icard)
{
	if(m == NULL || icard == NULL){
		return -1;
	}

	m->opt->lock_data(m);
	orb_list_del(m->icard_list,icard);
	m->opt->unlock_data(m);

	return 0;
}



_WIDGET_GENERATE(card,widget)

