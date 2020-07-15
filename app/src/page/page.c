
#include "page.h"
#include "page_event.h"
#include "page_label.h"
#include "page_manager.h"
#include "page_order.h"
#include "page_mode.h"
#include "gui_memory.h"
#include "screen.h"
#include "cJSON.h"

page_t *gui_page_create(char *name)
{
	if(name == NULL){
		return NULL;
	}

	page_t *page = page_calloc(1, sizeof(page_t) );

	strncpy(page->name, name, sizeof(page->name)-1);

	page->event_list = orb_list_create();
	page->event_queue = orb_queue_create(10);
	page->script = script_script_create();

	page->card_list = orb_list_create();
	page->label_props = orb_list_create();
	page->modes = orb_list_create();
	page_order_init(page);

	return page;
}

int gui_page_destroy(page_t *page)
{
	if(page == NULL){
		return 0;
	}

	if(page->event_queue){
		if(!orb_queue_is_empty(page->event_queue)){
			return -1;
		}
		orb_queue_destroy(page->event_queue);
	}

	if(page->card_list){
		page_card_t *card = orb_list_del_first(page->card_list);
		while(card){
			page_card_destroy(card);
			card = orb_list_del_first(page->card_list);
		}
		
		orb_list_destroy(page->card_list);
	}

	if(page->event_list){
		page_event_t *event = orb_list_del_first(page->event_list);
		while(event){
			//TODO exp destroy?
			page_event_destroy(event);
			event = orb_list_del_first(page->event_list);
		}
		orb_list_destroy(page->event_list);
	}

	if(page->widget_tree){
		page->widget_tree->opt->destroy(page->widget_tree,page->widget_tree->opt);
	}

	if(page->script){
		script_destroy(page->script);
	}

	if(page->label_props){
		page_label_prop_t *prop = orb_list_del_first(page->label_props);
		while(prop){
			page_label_prop_destroy(prop);
			prop = orb_list_del_first(page->label_props);
		}
		orb_list_destroy(page->label_props);
	}

	if(page->modes){
		page_mode_t *mode = orb_list_del_first(page->modes);
		while(mode){
			page_mode_destory(mode);
			mode = orb_list_del_first(page->modes);
		}
		orb_list_destroy(page->modes);
	}

	page_free(page);

	return 0;
}

/********************************************************
 *	
 *	gui_event_t format:
 *		{"widget":123,"event":"onClick"}
 *
 * ******************************************************/
static int page_widget_event_handle(widget_event_t *event, void *param)
{
	widget_event_t *new = widget_event_create(event->event,event->hd);
	page_manager_send_event(GUI_EVENT_WIDGET_EVENT,0,0,new);
	return 0;
}

int gui_page_start(page_t *page)
{
	if(page == NULL){
		return -1;
	}

	if(page->widget_tree == NULL){
		return -1;
	}
	
	script_add_global_var(page->script,"Event","{}");
	run_script(page->script);
	
	page_label_prop_t *prop = orb_list_get_first(page->label_props);
	while(prop){
		page_widget_prop_t *wp = prop->prop;
		while(wp){
			exp_node_t *result = expression_calc(wp->exp,page->script->code_stack);
			if(result == NULL){
				// TODO error handle
				GUI_ERR("Property set: expression cala fail");
				return -1;
			}
			char *value = exp_node_to_string(result);
			widget_property_set(prop->widget,wp->name,value);
			if(value){
				page_free(value);
			}
			exp_node_destroy(result);
			wp = wp->next;
		}
		prop = orb_list_get_next(page->label_props);
	}

	int ret = add_node_to_screen(page->widget_tree);

	screen_register_widget_event_handle(page_widget_event_handle,page);


	return ret;
}

int gui_page_stop(page_t *page)
{
	if(page == NULL){
		return -1;
	}

	remove_node_from_screen(page->widget_tree);
	return 0;
}

page_card_t *page_card_create(char *name)
{
	if(name == NULL){
		GUI_ERR("card name is null");
		return NULL;
	}

	page_card_t *card = page_calloc(1,sizeof(*card) + strlen(name) + 1);
	card->name = (char*)card + sizeof(*card);
	strcpy(card->name,name);

	return card;
}

void page_card_destroy(page_card_t *card)
{
	if(card){
		if(card->widget_tree){
			card->widget_tree->opt->destroy(card->widget_tree,card->widget_tree->opt);
		}
		page_free(card);
	}
}

page_card_t *page_card_find(page_t *page,char *name)
{
	if(page == NULL || name == NULL){
		return NULL;
	}

	page_card_t *card = orb_list_get_first(page->card_list);
	while(card){
		if(strcmp(card->name,name) == 0){
			return card;
		}
		card = orb_list_get_next(page->card_list);
	}
	return NULL;
}

void page_show(page_t *page)
{
	if(page == NULL){
		return;
	}

	printf("page name:%s id:%d\n",page->name,page->id);
	printf("  event list:\n");

	page_event_t *event = orb_list_get_first(page->event_list);
	int count = 0;
	while(event){
		if(event->widget){
			printf("    %2d. %s widget %s\n",count++,event->name,event->widget->name);
		}else{
			printf("    %2d. %s page event\n",count++,event->name);
		}
		event = orb_list_get_next(page->event_list);
	}

	printf("  card list:\n");
	page_card_t *card = orb_list_get_first(page->card_list);
	count = 0;
	while(card){
		printf("    %2d. %s\n",count++,card->name);	
		card = orb_list_get_next(page->card_list);
	}

	printf("  mode list:\n");
	page_mode_t *mode = orb_list_get_first(page->modes);
	count = 0;
	while(mode){
		page_mode_show(mode,2);
		mode = orb_list_get_next(page->modes);
	}
}


