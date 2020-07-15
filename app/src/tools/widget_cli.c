
#include "widget_cli.h"
#include "orb_cli.h"
#include "screen.h"
#include "gui_memory.h"
#include "ixmlwidget_widget.h"
#include "card_widget.h"

static int screen_widget_tree_show(int argc, char **argv)
{
	if(g_screen.node == NULL){
		printf("Screen node is NULL");
		return 0;
	}
	gui_node_show(g_screen.node);
	return 0;
}

static int screen_clean_cli(int argc, char **argv)
{
	if(g_screen.node == NULL){
		return 0;
	}

	mwidget *m = g_screen.node;
	g_screen.node = NULL;

	screen_stop_node(m,&g_screen);

	m->opt->destroy(m,m->opt);
	
	return 0;
}

static int widget_memory_show(int argc, char **argv)
{
	show_widget_memory();
	return 0;
}

static void _widget_show(mwidget *node, int argc, char **argv)
{
	if(node == NULL || argv == NULL || argc < 1){
		return;
	}

	mwidget *m = NULL;
	if(strcmp(node->opt->class_name,"ixmlwidget") == 0){
		mixmlwidget *mi = (mixmlwidget*)node;
		m = mi->widget_tree;
	}

	if(m){
		m = gui_node_get_node_by_name(m,argv[0]);
		argc--;
		if(argc == 0){
			if(m) m->opt->show(m);
		}else{
			_widget_show(m,argc,&argv[1]);
		}
	}
}
static int widget_show_cli(int argc, char **argv)
{
	if(argc < 3){
		return -1;
	}
	
	mwidget *m = g_screen.node;
	if(strcmp(argv[1],"tool_layer") == 0){
		m = g_screen.tool_layer;
	}else if(strcmp(argv[1],"suspention") == 0){
		m = g_screen.suspention;
	}else if(strcmp(argv[1],"screen") == 0){
		m = g_screen.node;
	}

	if(m){
		m = gui_node_get_node_by_name(m,argv[2]);
		if(m){
			argc -= 3;
			if(argc == 0){
				m->opt->show(m);
			}else{
				_widget_show(m,argc,&argv[3]);
			}
		}
	}else{
		printf("Can't find widget %s\n",argv[2]);
	}

	return 0;
}

static int widget_test_cli(int argc, char **argv)
{
	
	return 0;
}


int widget_cli_init(void)
{
	orb_cli_cmd_register("screen_widget_tree_show",screen_widget_tree_show,NULL);
	orb_cli_cmd_register("screen_clean",screen_clean_cli,NULL);
	orb_cli_cmd_register("widget_memory_show",widget_memory_show,NULL);
	orb_cli_cmd_register("widget_show",widget_show_cli,"widget_show [screen/tool_layer/suspention] name");
	return 0;
}

