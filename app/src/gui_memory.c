#include "platform.h"
#include "gui_memory.h"

#ifdef GUI_MEMORY_DEBUG

struct gui_memory_t{
	void	*p;
	int		size;
	char	desc[256];
	struct gui_memory_t *next;
};

static struct gui_memory_t *g_widget_memory = NULL;
static struct gui_memory_t *g_page_memory = NULL;

static void add_memory(struct gui_memory_t **head,struct gui_memory_t *m)
{
	if(*head == NULL){
		*head = m;
		return;
	}

	struct gui_memory_t *n = *head;
	while(n->next){
		n = n->next;
	}
	n->next = m;
}

static void del_memory(struct gui_memory_t **head,void *p)
{
	struct gui_memory_t *m = *head;
	struct gui_memory_t *prev = NULL;
	while(m){
		if(m->p == p){
			if(prev == NULL){
				*head = (*head)->next;
			}else{
				prev->next = m->next;
			}
			free(m);
			return;
		}
		prev = m;
		m = m->next;
	}
}

void *_widget_calloc(int size, int num,char *file, int line)
{
	void *data = calloc(size,num);
	if(data){
		struct gui_memory_t *m = calloc(1,sizeof(struct gui_memory_t));
		m->p = data;
		m->size = size*num;
		sprintf(m->desc,"%s - %d",file,line);
		add_memory(&g_widget_memory,m);
	}
	return data;
}

void _widget_free(void *data)
{
	del_memory(&g_widget_memory,data);
	free(data);
}

void *_page_calloc(int size, int num,char *file, int line)
{
	void *data = calloc(size,num);
	if(data){
		struct gui_memory_t *m = calloc(1,sizeof(struct gui_memory_t));
		m->p = data;
		m->size = size*num;
		sprintf(m->desc,"%s - %d",file,line);
		add_memory(&g_page_memory,m);
	}
	return data;
}

void _page_free(void *data)
{
	del_memory(&g_page_memory,data);
	free(data);
}

void show_widget_memory(void)
{
	struct gui_memory_t *m = g_widget_memory;
	int total = 0;
	int index = 0;
	while(m){
		printf("%-3d. %p size:%d desc:%s\n",index++,m->p,m->size,m->desc);
		total += m->size;
		m = m->next;
	}

	printf("---------------------------------\n");
	printf("memory total: %d\n",total);
}

void show_page_memory(void)
{
	struct gui_memory_t *m = g_page_memory;
	int total = 0;
	int index = 0;
	while(m){
		printf("%-3d. %p size:%d desc:%s\n",index++,m->p,m->size,m->desc);
		total += m->size;
		m = m->next;
	}

	printf("---------------------------------\n");
	printf("memory total: %d\n",total);
}

void *gui_calloc(size_t size,int num)
{
	void *v = calloc(size,num);
	return v;
}

void gui_free(void *v)
{
	free(v);
}

#endif

