#ifndef __MG_WIDGET_H__
#define __MG_WIDGET_H__

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */
#include "dw_widget.h"
#include "html.h"

#define     ADD_LENGTH        10
#define     APPEND_LENGTH     50
#define     MIN_WIDTH         40

#define  X_Y_ADDRESS 1000


#define DW_TYPE_MGWIDGET         (a_Dw_MgWidget_get_type())
#define DW_MGWIDGET(obj)          (G_TYPE_CHECK_INSTANCE_CAST ((obj), DW_TYPE_MGWIDGET, DwMgWidget))
#define DW_MGWIDGET_CLASS(klass)  (G_TYPE_CHECK_CLASS_CAST ((klass), DW_TYPE_MGWIDGET, DwMgWidgetClass))
#define DW_MGWIDGET_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS ((obj), DW_TYPE_MGWIDGET, DwMgWidgetClass))

#define DW_IS_MGWIDGET(obj)       (G_TYPE_CHECK_INSTANCE_TYPE ((obj), DW_TYPE_MGWIDGET))

typedef struct _DwMgWidget DwMgWidget;
typedef struct _DwMgWidgetClass DwMgWidgetClass;

struct _DwMgWidget
{
    DwWidget dw_widget;
    mSpiderHtmlInputType type;
    HWND window;
	int id;
    SIZE size;
};

struct _DwMgWidgetClass
{
   DwWidgetClass parent_class;
   void (*clicked)    (DwMgWidget *mg_widget);
   void (*clicked_at) (DwMgWidget *mg_widget, gint32 x, gint32 y);
};

GType a_Dw_MgWidget_get_type      (void);

DwWidget* a_Dw_MgWidget_button_new (mSpiderHtml* html, int id,
									const char* title, DWORD add_data);
DwWidget* a_Dw_MgWidget_entry_new (mSpiderHtml* html, int id,
								   const char* title, int nr_chars,
								   int chars_limit, DWORD styles,
								   DWORD add_data);
DwWidget* a_Dw_MgWidget_textarea_new (mSpiderHtml* html, int id,
									  const char* title, int height_chars,
									  int width_chars, int chars_limit,
									  DWORD add_data);
DwWidget* a_Dw_MgWidget_radio_button_new (mSpiderHtml* html, int id,
	   									  const char* title, DWORD add_data,
										  int is_group, gboolean is_checked);
DwWidget* a_Dw_MgWidget_check_button__new (mSpiderHtml* html, int id,
	   									   const char* title, DWORD add_data, gboolean is_checked);
DwWidget* a_Dw_MgWidget_combobox_new (mSpiderHtml* html, int id,
	   								 	 const char* title, DWORD add_data);
DwWidget* a_Dw_MgWidget_listbox_new (mSpiderHtml* html, int id, int size,
	   							  const char* title, DWORD add_data, gboolean is_multiple);

DwWidget* a_Widget_button_new (mSpiderHtml* html, int id, DWORD add_data);


DwWidget* a_Dw_MgWidget_mspider_new (mSpiderDoc * parent, gchar * name , int id,
                         const mSpiderUrl* start_url, int x, int y,int width, int height,
                         mSpiderIFrameScrollType scrolling, gboolean frame_border , void * add_data);

DwWidget* a_Dw_MgWidget_frame_new (mSpiderDoc * parent, gchar * name ,int id,
                         const mSpiderUrl* start_url, int x , int y , int width, int height,
                         mSpiderIFrameScrollType scrolling, gboolean frame_border);

void Dw_MgWidget_get_text_metrics (const char* title, int nr_chars,
	   									  int* width, int* height);

DwWidget* a_Dw_MgWidget_flash_new (mSpiderHtml* html, int id,
	   			int width,int height, DWORD add_data,HWND *hwnd);
void a_Dw_MgWidget_combobox_add_item (DwMgWidget* mgwidget, int selected, const char *str);
void a_Dw_MgWidget_listbox_add_item (DwMgWidget* mgwidget, int selected, const char *str);
void a_Dw_MgWidget_set_notification (HWND hwnd, mSpiderHtmlInputType type);
#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __MG_WIDGET_H__ */

