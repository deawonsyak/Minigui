#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "page_label.h"
#include "page_xml.h"
#include "page_mode.h"
#include "page.h"
#include "screen.h"
#include "script.h"
#include "page_event.h"
#include "card_widget.h"
#include "icard_widget.h"
#include "xmlwidget_widget.h"

int widgets_label_parse(xmlNodePtr node, page_t *page, void *param)
{
	xmlNodePtr cur = node->children;

	page_xml_prop_t *prop = xml_node_property_parse(node);


	char *name = xml_node_property_get(prop,"name");
	if(name == NULL){
		// TODO error handle
		GUI_ERR("ERR: widget label doesn't have name property");
		return -1;
	}

	mwidget *m = create_widget_by_name("widget",name,0,0,0,0);
	if(m == NULL){
		//TODO error handle
		return -1;
	}

	page_xml_prop_t *tmp = prop;
	while(tmp){
		m->opt->set_property(m,tmp->name,tmp->value);
		tmp = tmp->next;
	}

	page_widget_prop_t *w_prop = page_xml_prop_text_parse(node,page);
	if(w_prop){
		page_label_prop_t *l_prop = page_calloc(1,sizeof(*l_prop));
		l_prop->prop = w_prop;
		l_prop->widget = m;
		orb_list_add(page->label_props,l_prop);
	}
	
	page->widget_tree = m;

	while(cur){
		if(cur->type == XML_ELEMENT_NODE){
			if(label_parse(cur,page,page->widget_tree)){
				//TODO error handle
				return -1;
			}
		}
		cur = cur->next;
	}

	page_xml_prop_list_destroy(prop);
	return 0;
}

int widget_label_parse(xmlNodePtr node, page_t *page, void *param)
{
	if(node == NULL || page == NULL || param == NULL){
		return -1;
	}

	char auto_name[32] = {0};
	char *widget_type = (char*)node->name;	

	page_xml_prop_t *prop = xml_node_property_parse(node);

	char *name = xml_node_property_get(prop,"name");
	if(name == NULL){
		sprintf(auto_name,"%s_%d",widget_type,page->lable_index++);
		name = auto_name;
	}
	mwidget *parent = param;
	mwidget *m = create_widget_by_name(widget_type,name,0,0,0,0);
	if(m == NULL){
		//TODO error handle
		GUI_ERR("xml widgets parse error: Can't find widget %s",widget_type);
		return -1;
	}

	page_xml_prop_t *tmp = prop;
	while(tmp){
		m->opt->set_property(m,tmp->name,tmp->value);
		tmp = tmp->next;
	}

	gui_node_add_child_node_to_end(m,parent);

	page_widget_prop_t *w_prop = page_xml_prop_text_parse(node,page);
	if(w_prop){
		page_label_prop_t *l_prop = page_calloc(1,sizeof(*l_prop));
		l_prop->prop = w_prop;
		l_prop->widget = m;
		orb_list_add(page->label_props,l_prop);
	}

	xmlNodePtr cur = node->children;
	while(cur){
		if(cur->type == XML_ELEMENT_NODE){
			if(label_parse(cur,page,m)){
				page_xml_prop_list_destroy(prop);
				return -1;
			}
		}
		cur = cur->next;
	}
	page_xml_prop_list_destroy(prop);
	return 0;
}

int eventcbk_label_parse(xmlNodePtr node, page_t *page, void *param)
{
	if(node == NULL || page == NULL || param == NULL){
		GUI_ERR("Param error!");
		return -1;
	}
	mwidget *m = param;
	char *event = xmlGetProp(node,"event");	
	if(event == NULL){
		GUI_ERR("eventcbk label parse error! Can't find event property");
		return -1;
	}

	// 激活控件事件
	if(m->opt->activate_event(m,event)){
		GUI_ERR("%s widget doesn't have event %s",m->opt->class_name,event);
		return -1;
	}

	
	char buf[128] = {0};
	sprintf(buf,"func %s_%s_cbk(){", m->name,event);
	script_parse_from_string(page->script,buf);

	xmlNodePtr text = node->children;
	while(text){
		if(text->type == XML_TEXT_NODE){
			if(script_parse_from_string(page->script,(char*)text->content)){
				GUI_ERR("expression decode fail");
				return -1;
			}
		}
		text = text->next;
	}
	memset(buf,0,sizeof(buf));
	strcpy(buf,"}");
	script_parse_from_string(page->script,buf);

	//show_script(page->script);

	char func_name[128] = {0};
	sprintf(func_name,"%s_%s_cbk()",m->name,event);
	exp_string_t *exp_str = exp_string_decode(func_name);
	exp_node_t *exp = expression_decode(exp_str,page->script);
	exp_string_destroy(exp_str);

	if(exp == NULL){
		GUI_ERR("expression decode fail");
		return -1;
	}

	page_event_add(page,event,m,exp);

	return 0;
}

int recvevent_label_parse(xmlNodePtr node, page_t *page, void *param)
{
	if(node == NULL || page == NULL){
		GUI_ERR("Param error!");
		return -1;
	}

	char *event = xmlGetProp(node,"event");	
	if(event == NULL){
		GUI_ERR("eventcbk label parse error! Can't find event property");
		return -1;
	}

	
	char buf[128] = {0};
	sprintf(buf,"func %s_cbk(){",event);
	script_parse_from_string(page->script,buf);

	xmlNodePtr text = node->children;
	while(text){
		if(text->type == XML_TEXT_NODE){
			script_parse_from_string(page->script,(char*)text->content);
		}
		text = text->next;
	}
	memset(buf,0,sizeof(buf));
	strcpy(buf,"}");
	script_parse_from_string(page->script,buf);

	//show_script(page->script);

	char func_name[128] = {0};
	sprintf(func_name,"%s_cbk()",event);
	exp_string_t *exp_str = exp_string_decode(func_name);
	exp_node_t *exp = expression_decode(exp_str,page->script);
	exp_string_destroy(exp_str);

	if(exp == NULL){
		GUI_ERR("expression decode fail");
		return -1;
	}

	page_event_add(page,event,NULL,exp);
	
	return 0;
}

int mode_label_parse(xmlNodePtr node, page_t *page, void *param)
{
	if(node == NULL || page == NULL){
		GUI_ERR("Param error!");
		return -1;
	}

	xmlNodePtr cur = node->children;

	page_xml_prop_t *prop = xml_node_property_parse(node);
	if(prop == NULL){
		return -1;
	}

	char *name = xml_node_property_get(prop,"name");

	page_mode_t *mode = page_mode_create(name);
	if(mode == NULL){
		goto err;
	}

	while(cur){
		if(cur->type == XML_ELEMENT_NODE){
			if(label_parse(cur,page,&mode->nodes)){
				goto err;
			}
		}
		cur = cur->next;
	}

	orb_list_add(page->modes,mode);
	page_xml_prop_list_destroy(prop);
	return 0;

err:
	if(prop) page_xml_prop_list_destroy(prop);
	if(mode) page_mode_destory(mode);
	return -1;
	
}

int mnode_label_parse(xmlNodePtr node, page_t *page, void *param)
{
	
	if(node == NULL || page == NULL){
		GUI_ERR("Param error!");
		return -1;
	}

	xmlNodePtr cur = node->children;

	page_xml_prop_t *prop = xml_node_property_parse(node);
	if(prop == NULL){
		return -1;
	}

	page_mode_node_t *head = (page_mode_node_t*)param;

	page_mode_node_t *mnode = page_mode_node_create();

	page_xml_prop_t *tmp = prop;
	while(tmp){
		page_mode_prop_t *mprop = page_calloc(1,sizeof(*mprop));
		mprop->value = page_calloc(1,strlen(tmp->value)+1);
		strcpy(mprop->value, tmp->value);

		if(strcmp(tmp->name,"widget") == 0){
			mnode->widget = mprop;
			tmp = tmp->next;
			continue;
		}

		mprop->name = page_calloc(1,strlen(tmp->name) + 1);
		strcpy(mprop->name,tmp->name);

		orb_list_add(mnode->propertys,mprop);

		tmp = tmp->next;
	}

	page_widget_prop_t *wprop = page_xml_prop_text_parse(node,page);
	page_widget_prop_t *wp = wprop;
	while(wp){
		//TODO Check property is in widget
		
		page_mode_prop_t *mprop = page_calloc(1,sizeof(*mprop));
		mprop->name = page_calloc(1,strlen(wp->name) + 1);
		strcpy(mprop->name,wp->name);

		mprop->value_type = PAGE_MODE_PROPERTY_VALUE_TYPE_EXP;
		mprop->exp_prop = wp;

		orb_list_add(mnode->propertys,mprop);

		wp = wp->next;
	}

	if(head->child == NULL){
		head->child = mnode;
	}else{
		page_mode_node_t *next = head->child;
		while(next->next){
			next = next->next;
		}
		next->next = mnode;
	}

	while(cur){
		if(cur->type == XML_ELEMENT_NODE){
			if(label_parse(cur,page,mnode)){
				//TODO error handle
				goto err;
			}
		}
		cur = cur->next;
	}
	
	page_xml_prop_list_destroy(prop);
	return 0;

err:
	if(prop){
		page_xml_prop_list_destroy(prop);
	}

	return -1;
}

int imode_label_parse(xmlNodePtr node, page_t *page, void *param)
{

}

int card_label_parse(xmlNodePtr node, page_t *page, void *param)
{
	xmlNodePtr cur = node->children;


	page_xml_prop_t *prop = xml_node_property_parse(node);
	//show_xml_prop(prop);

	char *name = xml_node_property_get(prop,"name");
	if(name == NULL){
		// TODO error handle
		GUI_ERR("ERR: widget label doesn't have name property");
		return -1;
	}

	page_card_t *card = page_card_find(page,name);
	if(card != NULL){
		// TODO error handle
		GUI_ERR("Card %s is exist in page %s",name,page->name);
		return -1;
	}

	card = page_card_create(name);
	if(card == NULL){
		// TODO error handle
		GUI_ERR("Create card fail");
		return -1;
	}

	mwidget *m = create_widget_by_name("card",name,0,0,0,0);
	if(m == NULL){
		//TODO error handle
		return -1;
	}

	card->widget_tree = m;

	page_xml_prop_t *tmp = prop;
	while(tmp){
		m->opt->set_property(m,tmp->name,tmp->value);
		tmp = tmp->next;
	}

	page_widget_prop_t *w_prop = page_xml_prop_text_parse(node,page);
	if(w_prop){
		page_label_prop_t *l_prop = page_calloc(1,sizeof(*l_prop));
		l_prop->prop = w_prop;
		l_prop->widget = m;
		orb_list_add(page->label_props,l_prop);
	}
	
	while(cur){
		if(cur->type == XML_ELEMENT_NODE){
			if(label_parse(cur,page,m)){
				//TODO error handle
				return -1;
			}
		}
		cur = cur->next;
	}

	orb_list_add(page->card_list,card);
	page_xml_prop_list_destroy(prop);
	return 0;
}

int icard_label_parse(xmlNodePtr node, page_t *page, void *param)
{
	xmlNodePtr cur = node->children;


	page_xml_prop_t *prop = xml_node_property_parse(node);
	//show_xml_prop(prop);

	char *name = xml_node_property_get(prop,"name");
	if(name == NULL){
		// TODO error handle
		GUI_ERR("ERR: widget label doesn't have name property");
		return -1;
	}

	char *card_name = xml_node_property_get(prop,"card");
	if(card_name == NULL){
		// TODO error handle
		GUI_ERR("Can't find card property");
		return -1;
	}

	page_card_t *card = page_card_find(page,card_name);
	if(card == NULL){
		// TODO error handle
		GUI_ERR("Card %s is not exist in page %s",card_name,page->name);
		return -1;
	}

	micard *m = create_widget_by_name("icard",name,0,0,0,0);
	if(m == NULL){
		//TODO error handle
		return -1;
	}

	m->widget_tree = card->widget_tree;

	mcard *mc = card->widget_tree;
	mc->opt->add_to_icard(mc,m);

	page_xml_prop_t *tmp = prop;
	while(tmp){
		//printf("%s %d name:%s set_property:%p %s\r\n",__func__,__LINE__,tmp->name,m->opt->set_property,m->opt->class_name);
		m->opt->set_property(m,tmp->name,tmp->value);
		tmp = tmp->next;
	}

	page_widget_prop_t *w_prop = page_xml_prop_text_parse(node,page);
	if(w_prop){
		page_label_prop_t *l_prop = page_calloc(1,sizeof(*l_prop));
		l_prop->prop = w_prop;
		l_prop->widget = m;
		orb_list_add(page->label_props,l_prop);
	}

	mwidget *parent = param;
	if(parent){
		gui_node_add_child_node_to_end(m,parent);
	}else{
		// TODO error
	}
	
	while(cur){
		if(cur->type == XML_ELEMENT_NODE){
			label_parse(cur,page,m);
		}
		cur = cur->next;
	}

	page_xml_prop_list_destroy(prop);
	return 0;
}

int include_label_parse(xmlNodePtr node, page_t *page, void *param)
{
	if(node == NULL || page == NULL){
		return -1;
	}

	page_xml_prop_t *prop = xml_node_property_parse(node);

	char *file = xml_node_property_get(prop,"file");
	if(file == NULL){
		// TODO error handle
		GUI_ERR("ERR: include label doesn't have file property");
		return -1;
	}

	xmlDocPtr doc;
	doc = xmlParseFile(file);
	if(doc == NULL){
		GUI_ERR("xml file %s parse fail",file);
		goto err;
	}

	xmlNodePtr cur;

	cur = xmlDocGetRootElement(doc);
	if(cur == NULL){
		GUI_ERR("Can't find root element in xml file %s",file);
		goto err;
	}

	cur = cur->children;
	while(cur){
		if(cur->type == XML_ELEMENT_NODE){
			if(label_parse(cur,page,param)){
				goto err;
			}
		}
		cur = cur->next;
	}

	if(prop) {
		page_xml_prop_list_destroy(prop);
	}
	xmlFreeDoc(doc);
	return 0;

err:
	//TODO 
	if(doc){
		xmlFreeDoc(doc);
	}

	if(prop) {
		page_xml_prop_list_destroy(prop);
	}

	GUI_ERR("xml page load fail");
	return -1;

}

int xmlwidget_label_parse(xmlNodePtr node, page_t *page, void *param)
{
	
	page_xml_prop_t *prop = xml_node_property_parse(node);
	//show_xml_prop(prop);

	char *name = xml_node_property_get(prop,"name");
	if(name == NULL){
		// TODO error handle
		GUI_ERR("ERR: widget label doesn't have name property");
		return -1;
	}

	mxmlwidget *m = create_widget_by_name("xmlwidget",name,0,0,0,0);
	if(m == NULL){
		//TODO error handle
		GUI_ERR("xml widgets parse error: Can't find widget xmlwidget");
		return -1;
	}

	page_xml_prop_t *tmp = prop;
	while(tmp){
		m->opt->set_property(m,tmp->name,tmp->value);
		tmp = tmp->next;
	}
	
	xmlNodePtr cur = node->children;
	while(cur){
		if(cur->type == XML_ELEMENT_NODE){
			if(strcmp(cur->name,"eventDefine") == 0){
				if(label_parse(cur,m->page,m)){
					page_xml_prop_list_destroy(prop);
					return -1;
				}
				cur = cur->next;
				continue;
			}

			if(label_parse(cur,m->page,m->widget_tree)){
				page_xml_prop_list_destroy(prop);
				return -1;
			}
		}
		cur = cur->next;
	}
	page_xml_prop_list_destroy(prop);

	page = m->page;
	run_script(page->script);
	page_label_prop_t *lprop = orb_list_get_first(page->label_props);
	while(lprop){
		page_widget_prop_t *wp = lprop->prop;
		while(wp){
			exp_node_t *result = expression_calc(wp->exp,page->script->code_stack);
			if(result == NULL){
				// TODO error handle
				GUI_ERR("Property set: expression cala fail");
				goto err;
			}
			char *value = exp_node_to_string(result);
			widget_property_set(lprop->widget,wp->name,value);
			if(value){
				page_free(value);
			}
			exp_node_destroy(result);
			wp = wp->next;
		}
		lprop = orb_list_get_next(page->label_props);
	}


	screen_add_xmlwidget(m);

	return 0;
err:
	//TODO
	return -1;
}

int script_label_parse(xmlNodePtr node, page_t *page, void *param)
{
	if(node == NULL || page == NULL){
		return -1;
	}

	xmlNodePtr child = node->children;
	while(child){
		if(child->type == XML_TEXT_NODE){
			script_parse_from_string(page->script,child->content);
		}
		child = child->next;
	}
	//show_script(page->script);
	return 0;
}

int event_define_label_parse(xmlNodePtr node, page_t *page, void *param)
{
	if(node == NULL || page == NULL || param == NULL){
		return -1;
	}
	
	mxmlwidget *m = param;

	char *event = xmlGetProp(node,"event");	
	if(event == NULL){
		GUI_ERR("eventDefine label parse error! Can't find event property");
		return -1;
	}
	
	return m->opt->user_event_register(m,event);
}

page_label_parse_t g_labels[] = {
	// 脚本标签
	{"script", script_label_parse},
	// 模板定义标签
	{"mode",mode_label_parse},
	// 模板元素标签
	{"mnode",mnode_label_parse},
	// 模板加载标签
	{"imode",imode_label_parse},

	// 控件树标签
	{"widgets",widgets_label_parse},
	// 控件标签
	{"img",widget_label_parse},
	{"box",widget_label_parse},
	{"button",widget_label_parse},
	{"rect",widget_label_parse},
	{"text",widget_label_parse},
	{"slide",widget_label_parse},
	{"slideloop",widget_label_parse},
	{"slidescreen",widget_label_parse},
	{"svg",widget_label_parse},

	// 控件事件标签
	{"eventcbk",eventcbk_label_parse},

	{"pageEvent",recvevent_label_parse},

	// 卡片控件加载标签
	{"icard",icard_label_parse},

	// 卡片控件定义标签
	{"card",card_label_parse},

	// 引用标签
	{"include",include_label_parse},

	// xml控件标签 
	{"xmlwidget",xmlwidget_label_parse},
	// xml控件事件注册标签
	{"eventDefine",event_define_label_parse}

};

int label_parse(xmlNodePtr label, page_t *page, void *param)
{
	int len = sizeof(g_labels)/sizeof(g_labels[0]);
	for(int i = 0; i < len; i++){
		if(strcmp(g_labels[i].name,label->name) == 0){
			GUI_DEBUG("label %s parse",label->name);
			return g_labels[i].handle(label,page,param);
		}
	}

	if(widget_label_parse(label,page,param) == 0){
		return 0;
	}

	GUI_ERR("Can't find label %s parse function",label->name);
	return -1;
}

