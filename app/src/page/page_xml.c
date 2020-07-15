#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "libxml/xmlmemory.h"
#include "libxml/parser.h"
#include "page.h"
#include "orb_stack.h"
#include "widget_manager.h"
#include "screen.h"
#include "page_xml.h"
#include "page_label.h"
#include "script_string.h"

int show_xmlnode(xmlNodePtr node,int deep)
{
	char *buf;
	if(deep > 0){
		buf = page_calloc(1,deep*4);
		for(int i = 0; i < deep; i++){
			sprintf(buf,"%s  ",buf);
		}
	}else{
		buf = "";
	}

	while(node){
		if(node->type == XML_TEXT_NODE){
			printf("%scontent:%s\n",buf,node->content);
		}else{
			printf("%snode:%s type:%d %s\n",buf,node->name,node->type,node->content);
		}

		xmlAttrPtr ptr = node->properties;
		while(ptr){
			printf("%s  property:%s=%s\n",buf,ptr->name,ptr->children->content);
			ptr = ptr->next;
		}
		if(node->children){
			show_xmlnode(node->children,deep+1);
		}
		node = node->next;
	}

	return 0;
}

static page_xml_prop_t *page_xml_prop_create(char *name, char *value)
{
	if(name == NULL || value == NULL){
		return NULL;
	}
	int name_len = strlen(name) + 1;
	int value_len = strlen(value) + 1;
	int len = sizeof(page_xml_prop_t) + name_len + value_len;


	page_xml_prop_t *pro = page_calloc(1,len);
	pro->name = (char*)pro + sizeof(page_xml_prop_t);
	pro->value = pro->name + name_len;
	strcpy(pro->name,name);
	strcpy(pro->value,value);

	return pro;
}

static void page_xml_prop_destroy(page_xml_prop_t *pro)
{
	page_free(pro);
}

void page_xml_prop_list_destroy(page_xml_prop_t *pro)
{
	page_xml_prop_t *next,*tmp;
	tmp = pro;
	while(tmp){
		next = tmp->next;
		page_xml_prop_destroy(tmp);
		tmp = next;
	}
}

static page_widget_prop_t *page_widget_prop_create(char *name)
{
	if(name == NULL){
		return NULL;
	}

	page_widget_prop_t *prop = page_calloc(1,sizeof(*prop) + strlen(name) + 1);

	prop->name = (char*)prop + sizeof(*prop);
	strcpy(prop->name,name);

	return prop;
}

void page_widget_prop_destroy(page_widget_prop_t *prop)
{
	if(prop){
		if(prop->exp){
			exp_node_list_destroy(prop->exp);
		}
		page_free(prop);
	}
}

void page_label_prop_destroy(page_label_prop_t *label_prop)
{
	if(label_prop == NULL){
		return;
	}

	page_widget_prop_t *wp = label_prop->prop;
	page_widget_prop_t *next;
	while(wp){
		next = wp->next;
		page_widget_prop_destroy(wp);
		wp = next;
	}
	page_free(label_prop);
}

void show_xml_prop(page_xml_prop_t *pro)
{
	page_xml_prop_t *tmp = pro;
	printf("page_xml_prop_t show:\n");
	while(tmp){
		printf("  %s = %s\n",tmp->name,tmp->value);
		tmp = tmp->next;
	}
}

page_widget_prop_t *page_xml_prop_text_parse(xmlNodePtr node,page_t *page)
{

	page_widget_prop_t *head = NULL;
	page_widget_prop_t *cur = NULL;
	page_widget_prop_t *new = NULL;

	xmlNodePtr text = node->children;
	while(text){
		if(text->type != XML_TEXT_NODE){
			text = text->next;
			continue;
		}

		xmlChar *str = text->content;

		if(str == NULL){
			text = text->next;
			continue;
		}


		exp_string_t *lines = exp_lines_decode(str);
		exp_string_t *line = lines;

		while(line){
			exp_string_t *strings = exp_string_decode(line->str);

			//show_exp_string_t(strings);

			exp_string_t *tmp = strings;
			exp_string_t *next = tmp->next;
			if(next == NULL){
				GUI_ERR("xml property parse error: no =");
				break;
			}

			exp_string_t *value = next->next;
			if(value == NULL ||strcmp(next->str,"=") != 0){
				GUI_ERR("xml property parse error no value");
				break;
			}

			exp_node_t *exp = expression_decode(value,page->script);
			if(exp == NULL){
				GUI_ERR("property expression parse fail");
				goto err;
			}

			new = page_widget_prop_create(tmp->str);
			new->exp = exp;
			if(head == NULL){
				head = new;
			}else{
				cur->next = new;
			}
			cur = new;

			exp_string_destroy(strings);
			line = line->next;
		}
		exp_string_destroy(lines);

		text = text->next;
	}

	return head;

err:
	//TODO error handle
	return NULL;
}

page_xml_prop_t *xml_node_property_parse(xmlNodePtr node)
{
	page_xml_prop_t *head = NULL;
	page_xml_prop_t *cur = NULL;

	xmlAttrPtr pro = node->properties;
	while(pro){
		page_xml_prop_t *tmp;
		tmp = page_xml_prop_create(pro->name,pro->children->content);
		if(tmp){
			if(head == NULL){
				head = tmp;
			}else{
				cur->next = tmp;
			}
			cur = tmp;
		}

		pro = pro->next;
	}

	//show_xml_prop(head);
	return head;
}

char *xml_node_property_get(page_xml_prop_t *prop, char *name)
{
	page_xml_prop_t *tmp = prop;
	while(tmp){
		if(strcmp(tmp->name,name) == 0){
			return tmp->value;
		}
		tmp = tmp->next;
	}
	return NULL;
}


page_t *xml_page_load(char *file)
{
	xmlDocPtr doc;
	doc = xmlParseFile(file);

	xmlNodePtr cur;

	cur = xmlDocGetRootElement(doc);
	xmlChar *value = xmlGetProp(cur,"name");
	if(value == NULL){
		GUI_ERR("xml load error: No name property in page");
		return NULL;
	}

	page_t *page = gui_page_create(value);
	if(page == NULL){
		GUI_ERR("xml load error: Create page fail");
		return NULL;
	}

	cur = cur->children;
	while(cur){
		if(cur->type == XML_ELEMENT_NODE){
			if(label_parse(cur,page,NULL)){
				goto err;
			}
		}
		cur = cur->next;
	}

	xmlFreeDoc(doc);

	gui_node_show(page->widget_tree);
	return page;

err:
	// TODO	release
	GUI_ERR("xml page load fail");
	return NULL;
}

