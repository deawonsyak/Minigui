#include "page_mode.h"

page_mode_node_t *page_mode_node_create(void)
{
	page_mode_node_t *node = page_calloc(1,sizeof(*node));

	node->propertys = orb_list_create();

	return node;
}

static void page_mode_prop_destroy(page_mode_prop_t *prop)
{
	if(prop == NULL){
		return;
	}

	if(prop->name){
		page_free(prop->name);
	}
	if(prop->value_type == PAGE_MODE_PROPERTY_VALUE_TYPE_STRING){
		if(prop->value){
			page_free(prop->value);
		}
	}else{
		if(prop->exp_prop){
			page_widget_prop_destroy(prop->exp_prop);
		}
	}
	page_free(prop);
}
void page_mode_node_destroy(page_mode_node_t *node)
{
	if(node == NULL){
		return;
	}

	if(node->widget){
		page_mode_prop_destroy(node->widget);
	}

	if(node->propertys){
		page_mode_prop_t *prop = orb_list_del_first(node->propertys);
		while(prop){
			page_mode_prop_destroy(prop);	
			prop = orb_list_del_first(node->propertys);
		}
		orb_list_destroy(node->propertys);
	}

	if(node->child){
		page_mode_node_destroy(node->child);
	}

	if(node->next){
		page_mode_node_destroy(node->next);
	}

	page_free(node);

}

page_mode_t *page_mode_create(char *name)
{
	page_mode_t *mode = page_calloc(1,sizeof(*mode) + strlen(name) + 1);
	mode->mode_name = (char*)mode + sizeof(*mode);
	strcpy(mode->mode_name,name);

	return mode;
}

void page_mode_destory(page_mode_t *mode)
{
	if(mode == NULL){
		return;
	}

	//TODO destroy nodes
	
	page_mode_node_destroy(mode->nodes.child);

	page_free(mode);
}


void page_mode_node_show(page_mode_node_t *node,int deep)
{
	char pre[64] = {0};

	for(int i = 0; i < deep; i++){
		sprintf(pre,"%s  ",pre);
	}

	page_mode_node_t *child;

	printf("%s%s\n",pre,node->widget->value);

	page_mode_prop_t *prop = orb_list_get_first(node->propertys);
	while(prop){

		if(prop->value_type == PAGE_MODE_PROPERTY_VALUE_TYPE_STRING){
			printf("%s    property:%s value:%s\n",pre,prop->name,prop->value);
		}else{
			printf("%s    property:%s exp\n",pre,prop->name);
		}
		prop = orb_list_get_next(node->propertys);
	}

	
	child = node->child;
	if(child){
		page_mode_node_show(child,deep+1);
	}

	if(node->next){
		page_mode_node_show(node->next,deep);
	}
}

void page_mode_show(page_mode_t *mode,int deep)
{
	if(mode == NULL){
		return;
	}
	
	char pre[32] = {0};

	for(int i = 0; i < deep; i++){
		sprintf(pre,"%s  ",pre);
	}
	
	printf("%smode %s\n",pre,mode->mode_name);

	page_mode_node_t *mnode = mode->nodes.child;

	page_mode_node_show(mnode,deep+1);

}
