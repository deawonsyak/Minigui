#include "page_cli.h"
#include "page.h"
#include "page_manager.h"


int page_show_cli(int argc, char **argv)
{
	char *page_name = NULL;
	if(argc >= 2){
		page_name = argv[1];
	}

	page_t *page = NULL;
	if(page_name){
		if(strcmp(page_name,"all") == 0){
			page_manager_show_all_page();
			return 0;
		}

		page = page_manager_get_page_by_name(page_name);
		if(page == NULL){
			printf("Can't find page %s\n",page_name);
		}
	}else{
		page = page_manager_get_current_page();
		if(page == NULL){
			printf("Current page is NULL\n");
		}
	}

	if(page){
		page_show(page);
	}

	return 0;
}

int page_close_cli(int argc, char **argv)
{

	page_manager_test_close_page();
	return 0;
}

static int page_memory_show(int argc, char **argv)
{
	show_page_memory();
	return 0;
}



int page_cli_init(void)
{
	orb_cli_cmd_register("page_show",page_show_cli,"page_show [page_name]");
	orb_cli_cmd_register("page_memory_show",page_memory_show,NULL);
	orb_cli_cmd_register("page_close_test",page_close_cli,NULL);
	return 0;
}

