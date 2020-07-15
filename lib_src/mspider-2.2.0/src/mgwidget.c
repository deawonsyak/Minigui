/* * File: mgwidget.c *
 *
 * Copyright (C) 2005-2006 Feynman Software
 *
 * This program is g_free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include "html.h"
#include "nav.h"
#include "debug.h"
#include "emspider.h"
#include "dw_widget.h"
#include "dw_viewport.h"
#include "mgwidget.h"
#include "dw_container.h"
#include "dw_button.h"
#include "dw_marshal.h"
#include "doc.h"
#include "mgdconfig.h"

extern BrowserWindow* a_BrowserWindow_new (HWND hwnd_parent);
extern void Html_submit_form(mSpiderHtmlLB* html_lb, mSpiderHtmlForm* form,
                             DwWidget* widget, mSpiderDoc* doc);
extern void Html_reset_form(HWND hwnd, int id, int nc, DWORD add_data);
static void Dw_MgWidget_init          (DwMgWidget *MgWidget);
static void Dw_MgWidget_class_init    (DwMgWidgetClass *klass);
static void Dw_MgWidget_size_request      (DwWidget *widget,
                                         DwRequisition *requisition);
static void Dw_MgWidget_size_allocate     (DwWidget *widget,
                                         DwAllocation *allocation);
static void Dw_MgWidget_finalize         (GObject *object);
static void Dw_MgWidget_draw              (DwWidget *widget,
                                         HDC hdc,
                                         DwRectangle *area);
static gboolean Dw_MgWidget_press      (DwWidget *widget,
                                             gint32 x,
                                             gint32 y,
                                             DWORD flags);
static gboolean Dw_MgWidget_release    (DwWidget *widget,
                                             gint32 x,
                                             gint32 y,
                                             DWORD flags);

static void Dw_MgWidget_set_width (DwWidget *widget,
                                     gint32 width);

static void Dw_MgWidget_set_ascent (DwWidget *widget,
                                     gint32 ascent);

static void Dw_MgWidget_set_descent (DwWidget *widget,
                                     gint32 descent);
extern int sounddealing;
enum
{
   CLICKED,
   CLICKED_AT,
   LAST_SIGNAL
};
static DwContainerClass *parent_class;
static guint mgwidget_signals[LAST_SIGNAL] = { 0 };

void mg_notif_submit_form (HWND hwnd, int id, int nc, DWORD add_data)
{
    int form_id, control_id;
    mSpiderHtmlLB* html_lb;
    mSpiderHtmlForm* form;
    mSpiderHtmlInput *input;
    if(nc != BN_CLICKED)
        return;
   
    form_id = HIWORD(id) - 1;
    control_id = LOWORD(id);
    html_lb = (mSpiderHtmlLB*)GetWindowAdditionalData (hwnd);
    form = &html_lb->forms[form_id];
    input = &(form->inputs[control_id]);
    Html_submit_form (html_lb, form, input->widget, NULL);
}


/*
 * Return the type of DwButton
 */
GType a_Dw_MgWidget_get_type (void)
{
   static GType type = 0;

   if (!type) {
      static const GTypeInfo info = {
         sizeof (DwMgWidgetClass),
         (GBaseInitFunc) NULL,
         (GBaseFinalizeFunc) NULL,
         (GClassInitFunc) Dw_MgWidget_class_init,
         (GClassFinalizeFunc) NULL,
         (gconstpointer) NULL,
         sizeof (DwMgWidget),
         0,
         (GInstanceInitFunc) Dw_MgWidget_init
      };

      type = g_type_register_static (DW_TYPE_WIDGET, "DwMgWidget", &info, 0);
   }

   return type;
}

/*
 * Standard GObject function.
 */
static void Dw_MgWidget_init (DwMgWidget *mgwidget)
{
   mgwidget->window = -1;
   mgwidget->id = -1;
   mgwidget->size.cx = 0;
   mgwidget->size.cy = 0;
   mgwidget->type = -1;
}

/*
 * Standard GObject function.
 */
static void Dw_MgWidget_class_init (DwMgWidgetClass *klass)
{
   GObjectClass *object_class;
   DwWidgetClass *widget_class;

   object_class = G_OBJECT_CLASS (klass);
   widget_class = (DwWidgetClass*) klass;
   parent_class =  g_type_class_peek_parent (klass);

   mgwidget_signals[CLICKED] =
      g_signal_new ("clicked",
                      G_OBJECT_CLASS_TYPE (klass),
                      G_SIGNAL_RUN_FIRST | G_SIGNAL_ACTION,
                      G_STRUCT_OFFSET (DwMgWidgetClass, clicked),
                      NULL, NULL,
                      g_cclosure_marshal_VOID__VOID,
                      G_TYPE_NONE, 0);
   mgwidget_signals[CLICKED_AT] =
      g_signal_new ("clicked_at",
                      G_OBJECT_CLASS_TYPE (klass),
                      G_SIGNAL_RUN_FIRST | G_SIGNAL_ACTION,
                      G_STRUCT_OFFSET (DwMgWidgetClass, clicked_at),
                      NULL, NULL,
                      p_Dw_marshal_VOID__INT_INT,
                      G_TYPE_NONE,
                      2, G_TYPE_INT, G_TYPE_INT);

   object_class->finalize = Dw_MgWidget_finalize;

   widget_class->size_request = Dw_MgWidget_size_request;
   widget_class->size_allocate = Dw_MgWidget_size_allocate;
   widget_class->set_width = Dw_MgWidget_set_width;
   widget_class->set_ascent = Dw_MgWidget_set_ascent;
   widget_class->set_descent = Dw_MgWidget_set_descent;
   widget_class->draw = Dw_MgWidget_draw;
   widget_class->button_press_event = Dw_MgWidget_press;
   widget_class->button_release_event = Dw_MgWidget_release;

   klass->clicked = NULL;
   klass->clicked_at = NULL;
}


/*
 * Standard Dw function.
 */
static void Dw_MgWidget_size_request (DwWidget *widget,
                                    DwRequisition *requisition)
{
   DwMgWidget *mgwidget = DW_MGWIDGET (widget);
   gint32 avail_width;
   gint32 avail_height;

   if (requisition)
   {
      if (DW_STYLE_IS_ABS_LENGTH (widget->style->width))
         avail_width = DW_STYLE_ABS_LENGTH_VAL (widget->style->width);
      else if (DW_STYLE_IS_PER_LENGTH (widget->style->width))
         avail_width = mgwidget->size.cx
            * MIN (DW_STYLE_PER_LENGTH_VAL (widget->style->width), 1.0);
      else
         avail_width = mgwidget->size.cx;


      if (DW_STYLE_IS_ABS_LENGTH (widget->style->height))
         avail_height = DW_STYLE_ABS_LENGTH_VAL (widget->style->height);
      else if (DW_STYLE_IS_PER_LENGTH (widget->style->height))
         avail_height = mgwidget->size.cy
            * MIN (DW_STYLE_PER_LENGTH_VAL (widget->style->height), 1.0);
      else
         avail_height = mgwidget->size.cy;

	   requisition->width = avail_width;
	   requisition->ascent = avail_height;
	   requisition->descent = 0;
   }
}

/*
 * Standard Dw function.
 */
static void Dw_MgWidget_size_allocate (DwWidget *widget,
                                     DwAllocation *allocation)
{
    DwMgWidget *mgwidget = DW_MGWIDGET (widget);
    DwViewport *viewport = widget->viewport;

    if (allocation->width == 0 ||
            allocation->ascent + allocation->descent == 0)
        ShowWindow (mgwidget->window, SW_HIDE);
    else {
        ShowWindow (mgwidget->window, SW_SHOWNORMAL);

        if (allocation->x != widget->allocation.x ||
                allocation->y != widget->allocation.y ||
                allocation->width != widget->allocation.width ||
                allocation->ascent + allocation->descent != 
                    DW_WIDGET_HEIGHT(widget)) {

	   	    mgwidget->size.cx = allocation->width;
            mgwidget->size.cy = allocation->ascent + allocation->descent;

            MoveWindow (mgwidget->window, 
                allocation->x - viewport->world_x,
                allocation->y - viewport->world_y,
	   	        mgwidget->size.cx, mgwidget->size.cy, TRUE);
        }
    }
}

/*
 * Standard Dw function.
 */
static void Dw_MgWidget_draw (DwWidget *widget,
                            HDC hdc,
                            DwRectangle *area)
{

}

/*
 * Standard Dw function.
 */
static gboolean Dw_MgWidget_press (DwWidget *widget,
                                        gint32 x,
                                        gint32 y,
                                        DWORD flags)
{
   return FALSE;
}

/*
 * Standard Dw function.
 */
static gboolean Dw_MgWidget_release (DwWidget *widget,
                                          gint32 x,
                                          gint32 y,
                                          DWORD flags)
{
   return FALSE;
}


static void Dw_MgWidget_finalize (GObject* object)
{
   DwMgWidget *mgwidget = DW_MGWIDGET (object);
   if (mgwidget->window != -1)
       DestroyWindow (mgwidget->window);
   G_OBJECT_CLASS(parent_class)->finalize (object);
}

void Dw_MgWidget_get_text_metrics (const char* title, int nr_chars,
	   									  int* width, int* height)
{
    PLOGFONT logfont;
    SIZE size;

    logfont = GetSystemFont(SYSLOGFONT_CONTROL);
    SelectFont (HDC_SCREEN, logfont);
    GetTextExtent (HDC_SCREEN, title, nr_chars, &size);

    if (width)
        *width = size.cx;
    if (height)
        *height = size.cy;
}

static void Dw_MgWidget_set_color (HWND hwnd, mSpiderHtml* html)
{
    BYTE r, g, b;

    r = GetRValue(html->stack[0].current_bg_color);
    g = GetGValue(html->stack[0].current_bg_color);
    b = GetBValue(html->stack[0].current_bg_color);

    SetWindowBkColor(hwnd, RGB2Pixel(HDC_SCREEN, r, g, b));
}

static void Dw_MgWidget_set_font (HWND hwnd, mSpiderHtml* html)
{
    SetWindowFont (hwnd, html->stack[0].style->font->font);
}

static DwWidget* button_real_new (mSpiderHtml* html, int id,
                             const char* title, DWORD add_data,
							 DWORD dwstyle)
{
    DwMgWidget *mgwidget;
    mgwidget = DW_MGWIDGET (g_object_new (DW_TYPE_MGWIDGET, NULL));

    Dw_MgWidget_get_text_metrics (title, strlen(title),
		   				&mgwidget->size.cx, &mgwidget->size.cy);
/*FIXME: real control size is needed.*/
    if (mgwidget->size.cx == 0)
        mgwidget->size.cx += 15;

    mgwidget->window = CreateWindowEx (CTRL_BUTTON, title, 
								 dwstyle,
								 0, id, X_Y_ADDRESS, X_Y_ADDRESS, 
                                 mgwidget->size.cx, mgwidget->size.cy,
								 html->dd->docwin, 0);

    SetWindowAdditionalData (mgwidget->window, add_data);
    Dw_MgWidget_set_color (mgwidget->window, html);
    Dw_MgWidget_set_font (mgwidget->window, html);
    mgwidget->id = id;

    return DW_WIDGET(mgwidget);
}

DwWidget*  a_Dw_MgWidget_entry_new (mSpiderHtml* html, int id, const char* title, 
                                    int nr_chars, int chars_limit, DWORD styles,
									DWORD add_data)
{
    DwMgWidget *mgwidget;
    mgwidget = DW_MGWIDGET (g_object_new (DW_TYPE_MGWIDGET, NULL));
    
    Dw_MgWidget_get_text_metrics ("0", 1, 
                    &mgwidget->size.cx, &mgwidget->size.cy);

    mgwidget->size.cx *= nr_chars;
    mgwidget->size.cx += ADD_LENGTH;
    mgwidget->size.cy += ADD_LENGTH;
    if(mgwidget->size.cx < MIN_WIDTH)
        mgwidget->size.cx = MIN_WIDTH;

    styles |= WS_CHILD | WS_BORDER | WS_TABSTOP;

    mgwidget->window = CreateWindowEx (CTRL_SLEDIT, title, 
                                   styles,
                                   0, id, 
                                   X_Y_ADDRESS, X_Y_ADDRESS, 
                                   mgwidget->size.cx, mgwidget->size.cy,
                                   html->dd->docwin, 0);
    SetWindowAdditionalData (mgwidget->window, add_data);

    Dw_MgWidget_set_font (mgwidget->window, html);

    if (chars_limit > 0)
        SendMessage (mgwidget->window, EM_LIMITTEXT, chars_limit, 0L);

    mgwidget->id = id;

    return DW_WIDGET(mgwidget);
}

DwWidget* a_Dw_MgWidget_button_new (mSpiderHtml* html, int id,
	   			const char* title, DWORD add_data)
{
    DwMgWidget *mgwidget;
    DWORD dwstyle;
    mgwidget = DW_MGWIDGET (g_object_new (DW_TYPE_MGWIDGET, NULL));

    dwstyle = WS_CHILD | BS_PUSHBUTTON | BS_NOTIFY | WS_TABSTOP;

    if (title)
        Dw_MgWidget_get_text_metrics (title, strlen(title),
                              &mgwidget->size.cx, &mgwidget->size.cy);
    else
    {
        Dw_MgWidget_get_text_metrics (" ", 1, 
                        &mgwidget->size.cx, &mgwidget->size.cy);
        mgwidget->size.cx = MIN_WIDTH;
    }
    mgwidget->size.cx += ADD_LENGTH;
    mgwidget->size.cy += ADD_LENGTH;


    mgwidget->window = CreateWindowEx (CTRL_BUTTON, title, 
								 dwstyle,
								 0, id, 
                                 X_Y_ADDRESS, X_Y_ADDRESS, 
                                 mgwidget->size.cx, mgwidget->size.cy,
								 html->dd->docwin, 0);

    SetWindowAdditionalData (mgwidget->window, add_data);
    Dw_MgWidget_set_font (mgwidget->window, html);
    mgwidget->id = id;

    return DW_WIDGET(mgwidget);
}

#ifdef ENABLE_FLASH
DwWidget* a_Dw_MgWidget_flash_new (mSpiderHtml* html, int id,
	   			int width,int height, DWORD add_data,HWND *hwnd)
{
#ifdef ENABLE_FLASH
    int i;
    DwMgWidget *mgwidget;
    DWORD dwstyle;
    mgwidget = DW_MGWIDGET (g_object_new (DW_TYPE_MGWIDGET, NULL));
    dwstyle =   WS_CHILD;
    dwstyle |=   WS_VISIBLE;

    mgwidget->size.cx = width;
    mgwidget->size.cy = height;
#ifndef _NOUNIX_
    for(i=0;i<10;i++)
    {
        if(sounddealing=0)
            break;
        usleep(50000);
    }
#endif
    *hwnd = mgwidget->window = CreateWindowEx (CTRL_FLASH,"" , 
				 dwstyle,
				 WS_EX_TRANSPARENT, id, 
                 X_Y_ADDRESS, X_Y_ADDRESS, 
                 mgwidget->size.cx, mgwidget->size.cy,
				 html->dd->docwin, 0);

    SetWindowAdditionalData (mgwidget->window, add_data);
    mgwidget->id = id;

    return DW_WIDGET(mgwidget);
#else
	return NULL;
#endif
}

#endif
DwWidget* a_Dw_MgWidget_textarea_new (mSpiderHtml* html, int id,
	   								  const char* title, int height_chars,
									  int width_chars, int chars_limit,
									  DWORD add_data)
{
    DwMgWidget *mgwidget;
    mgwidget = DW_MGWIDGET (g_object_new (DW_TYPE_MGWIDGET, NULL));

    Dw_MgWidget_get_text_metrics (" ", 1, 
                    &mgwidget->size.cx, &mgwidget->size.cy);
    mgwidget->size.cx *= width_chars;
    mgwidget->size.cy *= height_chars;

    mgwidget->size.cx += APPEND_LENGTH;
    mgwidget->size.cy += APPEND_LENGTH;

    mgwidget->window = CreateWindowEx (CTRL_MEDIT, title, 
                WS_CHILD | WS_TABSTOP | WS_BORDER | 
                WS_VSCROLL | WS_HSCROLL | ES_AUTOWRAP| ES_NOHIDESEL,
				0, id, X_Y_ADDRESS, X_Y_ADDRESS, 
                mgwidget->size.cx, mgwidget->size.cy, 
                html->dd->docwin, 0);

    Dw_MgWidget_set_font (mgwidget->window, html);
    SetWindowAdditionalData (mgwidget->window, add_data);

    if (chars_limit > 0) {
        SendMessage (mgwidget->window, EM_LIMITTEXT, chars_limit, 0L);
    }

    mgwidget->id = id;

    return DW_WIDGET(mgwidget);
}  

DwWidget*  a_Dw_MgWidget_radio_button_new (mSpiderHtml* html, int id, 
                                    const char* title, DWORD add_data,
                                    int is_group, gboolean is_checked)
{
    DWORD dwstyle;
	g_return_val_if_fail (title != NULL, NULL);

    dwstyle = WS_CHILD | BS_AUTORADIOBUTTON | BS_CENTER| BS_NOTIFY | WS_TABSTOP;
    if (is_group)
       dwstyle = dwstyle | WS_GROUP; 
    if (is_checked)
       dwstyle = dwstyle | BS_CHECKED;
    return button_real_new (html, id, title, add_data, dwstyle);
}

DwWidget* a_Dw_MgWidget_check_button__new (mSpiderHtml* html, int id,
                                     const char* title, DWORD add_data, gboolean is_checked)
{
    DWORD dwstyle;
	g_return_val_if_fail (title != NULL, NULL);

    dwstyle = WS_CHILD | BS_AUTOCHECKBOX | BS_CENTER | BS_NOTIFY | WS_TABSTOP;
    if(is_checked)
        dwstyle = dwstyle | BS_CHECKED;
    return button_real_new (html, id, title, add_data, dwstyle);
}

DwWidget* a_Dw_MgWidget_combobox_new (mSpiderHtml* html, int id,
	   								  const char* title, DWORD add_data)
{
    DwMgWidget *mgwidget;
    mgwidget = DW_MGWIDGET (g_object_new (DW_TYPE_MGWIDGET, NULL));

    if(title)
        Dw_MgWidget_get_text_metrics (title, -1,
			   			&mgwidget->size.cx, &mgwidget->size.cy);
    else {
        Dw_MgWidget_get_text_metrics (" ", 1, &mgwidget->size.cx, 
                        &mgwidget->size.cy);
        mgwidget->size.cx = APPEND_LENGTH;
    }
    mgwidget->size.cy += ADD_LENGTH;

    mgwidget->window = CreateWindowEx (CTRL_COMBOBOX, "", 
        WS_CHILD | WS_TABSTOP | CBS_DROPDOWNLIST | CBS_READONLY | CBS_NOTIFY,
        0, id, X_Y_ADDRESS, X_Y_ADDRESS, mgwidget->size.cx, mgwidget->size.cy,
        html->dd->docwin, 0);

    Dw_MgWidget_set_color (mgwidget->window, html);
    Dw_MgWidget_set_font (mgwidget->window, html);
    SetWindowAdditionalData (mgwidget->window, add_data);

    mgwidget->id = id;

    return DW_WIDGET(mgwidget);
}

DwWidget* a_Dw_MgWidget_listbox_new (mSpiderHtml* html, int id, int size,
	   								 const char* title, DWORD add_data, gboolean is_multiple)
{
    DWORD dwstyle;
    DwMgWidget *mgwidget;
    mgwidget = DW_MGWIDGET (g_object_new (DW_TYPE_MGWIDGET, NULL));

    if(title)
        Dw_MgWidget_get_text_metrics (title, strlen(title),
			   					&mgwidget->size.cx, &mgwidget->size.cy);
    else
    {
        Dw_MgWidget_get_text_metrics (" ", 1, 
                        &mgwidget->size.cx, &mgwidget->size.cy);
        mgwidget->size.cx = APPEND_LENGTH;
    }
    mgwidget->size.cy = size ? (size*mgwidget->size.cy) : ADD_LENGTH;

    if (mgwidget->size.cx < APPEND_LENGTH)
        mgwidget->size.cx = APPEND_LENGTH;

    dwstyle = WS_CHILD | WS_TABSTOP | LBS_NOTIFY | WS_VSCROLL | WS_BORDER;

    if (is_multiple)
        dwstyle = dwstyle | LBS_MULTIPLESEL;
    
    mgwidget->window =
        CreateWindowEx (CTRL_LISTBOX, title, dwstyle,
             0, id, X_Y_ADDRESS, X_Y_ADDRESS, 
             mgwidget->size.cx, mgwidget->size.cy,
             html->dd->docwin, 0);

    Dw_MgWidget_set_font (mgwidget->window, html);
    SetWindowAdditionalData (mgwidget->window, add_data);
    mgwidget->id = id;

    return DW_WIDGET(mgwidget);
}

void a_Dw_MgWidget_set_notification (HWND hwnd, mSpiderHtmlInputType type)
{
	if ((type == MSPIDER_HTML_INPUT_SUBMIT) ||
        (type == MSPIDER_HTML_INPUT_BUTTON_SUBMIT))
		SetNotificationCallback (hwnd, mg_notif_submit_form);
	else if ((type == MSPIDER_HTML_INPUT_RESET) ||
            (type == MSPIDER_HTML_INPUT_BUTTON_RESET))
		SetNotificationCallback (hwnd, Html_reset_form);
}

void a_Dw_MgWidget_combobox_add_item (DwMgWidget* mgwidget, int selected, const char *str)
{
    int width = 0;

    if (str != NULL)
    {
        Dw_MgWidget_get_text_metrics (str, -1, &width, NULL);
        width += APPEND_LENGTH;
    }
    else
        width = mgwidget->size.cx; 

    if (width > mgwidget->size.cx) {
        mgwidget->size.cx = width;
    }

	SendMessage (mgwidget->window, CB_ADDSTRING, 0,(LPARAM)str);

    if(selected >= 0)
        SendMessage (mgwidget->window, CB_SETCURSEL, selected, 0);
}

void a_Dw_MgWidget_listbox_add_item (DwMgWidget* mgwidget, int selected, const char *str)
{
    int width;

    Dw_MgWidget_get_text_metrics (str, -1, &width, NULL);
    width += APPEND_LENGTH;
    if (width > mgwidget->size.cx) {
        mgwidget->size.cx = width;
    }

	SendMessage (mgwidget->window, LB_ADDSTRING, 0, (LPARAM)str);

    if(selected >= 0)
        SendMessage (mgwidget->window, LB_SETSEL, 1, selected);
}
#if 0
DwWidget* a_Widget_button_new (mSpiderHtml* html, int id, DWORD add_data)
{
    DwButton* widget ; 
    widget = DW_BUTTON (g_object_new (DW_TYPE_BUTTON, NULL));
    widget->child->hwnd = CreateWindowEx (CTRL_BUTTON, "", 
								 WS_CHILD | BS_DEFPUSHBUTTON | WS_TABSTOP,
								 0, id, 0, 0, 20, 20,
								 html->bw->docwin, add_data);
    return (DwWidget*)widget;
}
#endif

DwWidget* a_Dw_MgWidget_mspider_new (mSpiderDoc * parent, 
                                    gchar * name,
                                    int id,
                                   const mSpiderUrl* start_url,
                                   int x, int y,
                                   int width, int height,
                                   mSpiderIFrameScrollType scrolling,
                                   gboolean frame_border , void * add_data)
{
    DwMgWidget *mgwidget;
    DWORD dwstyle;
    mSpiderDoc* dd;
    HWND parent_hwnd;
    mgwidget = DW_MGWIDGET (g_object_new (DW_TYPE_MGWIDGET, NULL));
    
    parent_hwnd = DW_WIDGET(add_data)->hwnd;

    dwstyle = WS_CHILD;

    if (scrolling == IFrameScroll_YES || scrolling == IFrameScroll_AUTOMATIC)
        dwstyle |= WS_VSCROLL | WS_HSCROLL;
    if (frame_border)
        dwstyle |= WS_BORDER;

        dwstyle |= WS_TABSTOP;

    dd = a_Doc_new ();
    a_Doc_set_parent(dd , parent);
    a_Doc_set_browserwindow(dd , parent->bw); 
    
    if (name)
        a_Doc_set_name(dd , name);

    mgwidget->size.cx = width;
    mgwidget->size.cy = height;
    
    a_Doc_CreateEx(dd , parent_hwnd, dwstyle , x , y , width , height ,id , "");
    
    mgwidget->window = dd->docwin; 
    
    DW_WIDGET(mgwidget)->viewport = dd->viewport;
    DW_WIDGET(mgwidget)->hwnd = mgwidget->window;
    SetWindowAdditionalData2 (mgwidget->window, (DWORD)dd);
    mgwidget->id = id;

    a_Nav_push (dd, start_url);

    return DW_WIDGET(mgwidget);
}

/*
 *  create a frame object
 */
DwWidget* a_Dw_MgWidget_frame_new (mSpiderDoc * parent, 
                                    gchar * name, int id,
                                   const mSpiderUrl* start_url,
                                   int x, int y,
                                   int width, int height,
                                   mSpiderIFrameScrollType scrolling,
                                   gboolean frame_border)
{
    DwMgWidget *mgwidget;
    DWORD dwstyle;
    mSpiderDoc* dd;
    HWND parent_hwnd;
    mgwidget = DW_MGWIDGET (g_object_new (DW_TYPE_MGWIDGET, NULL));

    dwstyle = WS_CHILD;
    parent_hwnd = parent->docwin;

    if (scrolling == IFrameScroll_YES || scrolling == IFrameScroll_AUTOMATIC)
        dwstyle |= WS_VSCROLL | WS_HSCROLL;
    if (frame_border)
        dwstyle |= WS_BORDER;

        dwstyle |= WS_TABSTOP;


    mgwidget->size.cx = width;
    mgwidget->size.cy = height;
    
    dd = a_Doc_new ();
    dd->is_iframe = 1;
    a_Doc_set_parent(dd , parent);
    a_Doc_set_browserwindow(dd , parent->bw); 

    if (name)
        a_Doc_set_name(dd , name);

    a_Doc_CreateEx(dd , parent_hwnd, dwstyle , X_Y_ADDRESS , X_Y_ADDRESS , width , height ,id , "");
    
    mgwidget->window = dd->docwin; 
    
    DW_WIDGET(mgwidget)->viewport = dd->viewport;
    DW_WIDGET(mgwidget)->hwnd = mgwidget->window;
    SetWindowAdditionalData2 (mgwidget->window, (DWORD)dd);
    mgwidget->id = id;

    a_Nav_push (dd, start_url);

    return DW_WIDGET(mgwidget);
}


static void Dw_MgWidget_set_width (DwWidget *widget,
                                     gint32 width)
{
    DwMgWidget * mgwidget;

    mgwidget = DW_MGWIDGET (widget);

    if (mgwidget->size.cx != width) {
        mgwidget->size.cx = width;
        p_Dw_widget_queue_resize (widget, 0, FALSE);
    }
}

static void Dw_MgWidget_set_ascent (DwWidget *widget,
                                     gint32 ascent)
{

    DwMgWidget * mgwidget;
    
    mgwidget = DW_MGWIDGET (widget);

    if (mgwidget->size.cy != ascent) {
        mgwidget->size.cy = ascent;
        p_Dw_widget_queue_resize (widget, 0, FALSE);
    }

}

static void Dw_MgWidget_set_descent (DwWidget *widget,
                                     gint32 descent)
{

}

