#ifndef __GUI_MEMORY_H__
#define __GUI_MEMORY_H__

#include "platform.h"

#define GUI_MEMORY_DEBUG

#ifdef GUI_MEMORY_DEBUG

void *_widget_calloc(int size, int num,char *file, int line);
void _widget_free(void *data);

void *_page_calloc(int size, int num,char *file, int line);
void _page_free(void *data);

void *gui_calloc(size_t size,int num);
void gui_free(void *v);

#define widget_calloc(size,num) _widget_calloc(size,num,__FILE__,__LINE__)
#define widget_free(v) _widget_free(v)
void show_widget_memory(void);

#define page_calloc(size,num) _page_calloc(size,num,__FILE__,__LINE__)
#define page_malloc(size) _page_calloc(1,size,__FILE__,__LINE__)
#define page_free(v) _page_free(v)
void show_page_memory(void);


#else

#define gui_calloc(size,num) calloc(size,num)
#define gui_free(v) free(v)

#define widget_calloc(size,num) calloc(size,num)
#define widget_malloc(size) malloc(size)
#define widget_free(v) free(v)

#define page_calloc(size,num) calloc(size,num)
#define page_malloc(size) malloc(size)
#define page_free(v) free(v)

#define show_widget_memory()

#endif


#endif

