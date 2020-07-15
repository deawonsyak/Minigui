#include "page.h"

page_event_t *page_event_create(char *event)
{
	if(event == NULL){
		return NULL;
	}
	page_event_t *pevent = page_calloc(1,sizeof(page_event_t) + strlen(event) + 1);
	pevent->name = (char*)pevent + sizeof(page_event_t);
	strcpy(pevent->name,event);
	return pevent;
}

void page_event_destroy(page_event_t *event)
{
	if(event){
		page_free(event);
	}
}

page_event_t *page_event_find(page_t *page, char *event, mwidget *m)
{
	if(page == NULL || event == NULL){
		return NULL;
	}

	page_event_t *pevent = orb_list_get_first(page->event_list);
	while(pevent){
		if(strcmp(pevent->name,event) == 0 && m == pevent->widget){
			return pevent;
		}
		pevent = orb_list_get_next(page->event_list);
	}
	return NULL;
}

int page_event_add(page_t *page, char *event, mwidget *m, exp_node_t *exp)
{
	if(page == NULL || event == NULL || exp == NULL){
		return -1;
	}

	page_event_t *pevent = page_event_create(event);

	pevent->exp = exp;
	pevent->widget = m;

	orb_list_add(page->event_list,pevent);

	return 0;
}
