/*
 * File: dw_image.c
 *
 * Copyright (C) 2005 ~ 2006 Feynman Software
 * Copyright (C) 2001 Sebastian Geerken  <S.Geerken@ping.de>,
 *                    Jorge Arellano Cid <jcid@mspider.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include <stdio.h>
#include <string.h>
#include <glib.h>

#include "mgdconfig.h"
#include "dw_widget.h"
#include "dw_image.h"
#include "dw_viewport.h"
#include "dw_marshal.h"
#include "msg.h"
#include "prefs.h"
#include "list.h"
#include "dicache.h"
#include "debug.h"

static void Dw_image_init               (DwImage *image);
static void Dw_image_class_init         (DwImageClass *klass);

static void Dw_image_finalize           (GObject *object);

static void Dw_image_size_request       (DwWidget *widget,
                                         DwRequisition *requisition);
static void Dw_image_size_allocate      (DwWidget *widget,
                                         DwAllocation *allocation);
static void Dw_image_draw               (DwWidget *widget,
                                         HDC hdc,
                                         DwRectangle *area);
static gboolean Dw_image_button_press   (DwWidget *widget,
                                         gint32 x,
                                         gint32 y,
                                         DWORD flags);
static gboolean Dw_image_button_release (DwWidget *widget,
                                         gint32 x,
                                         gint32 y,
                                         DWORD flags);
static gboolean Dw_image_motion_notify  (DwWidget *widget,
                                         gint32 x,
                                         gint32 y,
                                         DWORD flags);
static gboolean Dw_image_enter_notify   (DwWidget *widget,
                                         DwWidget *last_widget,
                                         DWORD flags);
static gboolean Dw_image_leave_notify   (DwWidget *widget,
                                         DwWidget *next_widget,
                                         DWORD flags);
static DwIterator* Dw_image_iterator    (DwWidget *widget,
                                         gint32 mask,
                                         gboolean at_end);
static void Dw_image_iterator_highlight (DwIterator *it,
                                         gint start,
                                         gint end,
                                         DwHighlightLayer layer);

static void Dw_image_find_link          (DwImage *image,
                                         gint x, gint y,
                                         gint *link,
                                         gint *link_x, gint *link_y);
static void Dw_image_scale              (DwImage *image);

static gint Dw_image_map_list_find_link (DwImageMapList *list,
                                         mSpiderUrl *url,
                                         gint x,
                                         gint y);


#define Dw_image_scaled_y(image, y_src) \
   ( (y_src) * ( ((DwWidget*)(image))->allocation.ascent +  \
                 ((DwWidget*)(image))->allocation.descent - \
                 p_Dw_style_box_diff_height ( ((DwWidget*)(image))->style ) ) \
             / ((DwImage*)(image))->height )


enum
{
    LINK_ENTERED,
    LINK_PRESSED,
    LINK_RELEASED,
    LINK_CLICKED,
    LAST_SIGNAL
};

static guint image_signals[LAST_SIGNAL] = { 0 };
static DwWidgetClass *parent_class;
BLOCKHEAP __msp_cliprc_heap;

/*
 * Standard GObject function.
 */
GType a_Dw_image_get_type (void)
{
    static GType type = 0;

    if (!type) {
        static const GTypeInfo info = {
            sizeof (DwImageClass),
            (GBaseInitFunc) NULL,
            (GBaseFinalizeFunc) NULL,
            (GClassInitFunc) Dw_image_class_init,
            (GClassFinalizeFunc) NULL,
            (gconstpointer) NULL,
            sizeof (DwImage),
            0,
            (GInstanceInitFunc) Dw_image_init
        };

        type = g_type_register_static (DW_TYPE_WIDGET, "DwImage", &info, 0);
    }

    return type;
}


/*
 * Standard GObject function.
 */
DwWidget* a_Dw_image_new (DwImageType type, const gchar *alt_text)
{
    GObject *object;
    DwImage *image;

    object = g_object_new (DW_TYPE_IMAGE, NULL);
    image = DW_IMAGE(object);
    image->alt_text = g_strdup (alt_text);
    return DW_WIDGET (object);
}

/*
 * Standard GObject function.
 */
static void Dw_image_init (DwImage *image)
{
    int i;

    image->url = NULL;
    image->width = 0;
    image->height = 0;
    image->alt_text_width = -1; /* not yet calculated */
    image->buffer = NULL;
    image->state = DW_IMAGE_EMPTY;
    image->scaled_buffer = NULL;
    image->alt_text = NULL;

    image->is_widget = TRUE;
    image->client_id = -1;

    image->usemap_url = NULL;
    image->ismap = FALSE;
    for (i = 0; i < DW_HIGHLIGHT_NUM_LAYERS; i++)
        image->selected[i] = FALSE;
    image->Y = 0;
#ifdef ENABLE_ANIMATION
    /* Just for animated Gif */
    image->timer = 0;
    image->cmap = NULL;
    image->elapsed_10ms = 0;
    image->nr_frames = 0;
    image->frames = NULL;
    image->current_frame = NULL;
    image->done_frame = NULL;
#endif
}


/*
 * Standard GObject function.
 */
static void Dw_image_class_init (DwImageClass *klass)
{
    GObjectClass *object_class;
    DwWidgetClass *widget_class;

    parent_class = g_type_class_peek_parent (klass);

    object_class = (GObjectClass*)klass;
    object_class->finalize = Dw_image_finalize;

    image_signals[LINK_ENTERED] =
        g_signal_new ("link_entered",
                        G_OBJECT_CLASS_TYPE (klass),
                        G_SIGNAL_RUN_LAST,
                        G_STRUCT_OFFSET (DwImageClass, link_entered),
                        NULL, NULL,
                        p_Dw_marshal_link_enter,
                        G_TYPE_BOOLEAN,
                        3, G_TYPE_INT, G_TYPE_INT, G_TYPE_INT);
    image_signals[LINK_PRESSED] =
        g_signal_new ("link_pressed",
                        G_OBJECT_CLASS_TYPE (klass),
                        G_SIGNAL_RUN_LAST,
                        G_STRUCT_OFFSET (DwImageClass, link_pressed),
                        NULL, NULL,
                        p_Dw_marshal_link_button,
                        G_TYPE_BOOLEAN,
                        4, G_TYPE_INT, G_TYPE_INT, G_TYPE_INT,
                        G_TYPE_UINT);
    image_signals[LINK_RELEASED] =
        g_signal_new ("link_released",
                        G_OBJECT_CLASS_TYPE (klass),
                        G_SIGNAL_RUN_LAST,
                        G_STRUCT_OFFSET (DwImageClass, link_released),
                        NULL, NULL,
                        p_Dw_marshal_link_button,
                        G_TYPE_BOOLEAN,
                        4, G_TYPE_INT, G_TYPE_INT, G_TYPE_INT,
                        G_TYPE_UINT);
    image_signals[LINK_CLICKED] =
        g_signal_new ("link_clicked",
                        G_OBJECT_CLASS_TYPE (klass),
                        G_SIGNAL_RUN_LAST,
                        G_STRUCT_OFFSET (DwImageClass, link_clicked),
                        NULL, NULL,
                        p_Dw_marshal_link_button,
                        G_TYPE_BOOLEAN,
                        4, G_TYPE_INT, G_TYPE_INT, G_TYPE_INT,
                        G_TYPE_UINT);

    widget_class = (DwWidgetClass*)klass;
    widget_class->size_request = Dw_image_size_request;
    widget_class->size_allocate = Dw_image_size_allocate;
    widget_class->draw = Dw_image_draw;
    widget_class->button_press_event = Dw_image_button_press;
    widget_class->button_release_event = Dw_image_button_release;
    widget_class->motion_notify_event = Dw_image_motion_notify;
    widget_class->enter_notify_event = Dw_image_enter_notify;
    widget_class->leave_notify_event = Dw_image_leave_notify;
    widget_class->iterator = Dw_image_iterator;
}

#ifdef ENABLE_ANIMATION
void destroy_gif_frames (DwImage* image)
{
    AnimationFrame *tmp, *frame;
    frame = image->frames;
    while (frame) {
        tmp = frame->next;
        if (frame->bits)
            g_free (frame->bits);
        if (frame->cmap)
            g_free (frame->cmap);
        g_free (frame);
        frame = tmp;
    }
}
#endif

/*
 * Standard GObject function.
 */
static void Dw_image_finalize (GObject *object)
{
    DwImage *image = DW_IMAGE (object);

    if (image->usemap_url)
        a_Url_free (image->usemap_url);
    if (image->url)
        a_Dicache_unref (image->url, image->version);
    g_free (image->alt_text);
    g_free (image->scaled_buffer);
#ifdef ENABLE_ANIMATION
    if (image->timer)
        g_source_remove (image->timer);
    if (image->cmap)
        g_free (image->cmap);
    if (image->frames)
        destroy_gif_frames (image);
#endif
    G_OBJECT_CLASS(parent_class)->finalize (object);
}


/*
 * Standard Dw function.
 */
static void Dw_image_size_request (DwWidget *widget,
                                   DwRequisition *requisition)
{
    DwImage *image;
    double wscale, hscale;

    image = DW_IMAGE (widget);
   
    if (image->buffer != NULL ||
            (image->alt_text == NULL || image->alt_text[0] == '\0')) {
        requisition->width = image->width;
        requisition->ascent = image->height;
        requisition->descent = 0;
    }
    else {
        SIZE ext;
        FONTMETRICS fm;
        GetFontMetrics (widget->style->font->font, &fm);
   
        SelectFont (HDC_SCREEN, widget->style->font->font);
        if (image->alt_text_width == -1) {
            GetTextExtent (HDC_SCREEN, image->alt_text, -1, &ext);
            image->alt_text_width = ext.cx;
        }

        requisition->width = image->alt_text_width;
        requisition->ascent = fm.ascent;
        requisition->descent = fm.descent;
    }

    if (widget->style->width == DW_STYLE_LENGTH_AUTO) {
        if (!DW_STYLE_IS_ABS_LENGTH (widget->style->height)) {
            requisition->width = requisition->width;
        }
        else if (DW_STYLE_IS_ABS_LENGTH (widget->style->height)){
            /*calculate the width*/
            wscale = (double)((requisition->ascent+ requisition->descent)*100/
                    DW_STYLE_ABS_LENGTH_VAL (widget->style->height));
            requisition->width =(int) ((requisition->width)*100/wscale);
        }
    }

    if (widget->style->height == DW_STYLE_LENGTH_AUTO) {
       if (!DW_STYLE_IS_ABS_LENGTH (widget->style->width)) { 
            requisition->ascent = requisition->ascent;
            requisition->descent = requisition->descent;
        }
        else if (DW_STYLE_IS_ABS_LENGTH (widget->style->width)) {
            /*calculate the height*/
            hscale = (double)((requisition->width)*100/
                    DW_STYLE_ABS_LENGTH_VAL (widget->style->width));
            requisition->ascent =(int)((requisition->ascent)*100/hscale);
            requisition->descent =(int)((requisition->descent)*100/hscale);
        }
    }

    requisition->width += p_Dw_style_box_diff_width (widget->style);
    requisition->ascent += p_Dw_style_box_offset_y (widget->style);
    requisition->descent += p_Dw_style_box_rest_height (widget->style);
}

/*
 * Standard Dw function.
 */
static void Dw_image_size_allocate (DwWidget *widget,
                                    DwAllocation *allocation)
{
    DwImage *image;

    p_Dw_widget_update_area (widget, allocation, FALSE);

    /* if image is moved only */
    if (allocation->width == widget->allocation.width &&
            allocation->ascent + allocation->descent == DW_WIDGET_HEIGHT(widget)) 
        return;

    p_Dw_widget_update_area (widget, allocation, FALSE);
    /* this is also done in a_Dw_widget_size_allocate, but
        Dw_image_scale needs this. */
    widget->allocation = *allocation;

    _MSG ("Dw_image_size_allocate: width: %d, height: %d\n",
                widget->allocation.width,
                DW_WIDGET_HEIGHT (widget));

    image = DW_IMAGE (widget);
    if (image->buffer != NULL && image->width > 0 && image->height > 0)
        Dw_image_scale (image);
}

/*
 * Standard Dw function.
 */
static void Dw_image_draw (DwWidget *widget,
                           HDC hdc,
                           DwRectangle *area)
{
    gint32 vx, vy;
    DwRectangle content, intersection;
    DwImage *image = DW_IMAGE (widget);
    guchar *buffer, *bstart;
    int i;
    gboolean selected = FALSE;
    MYBITMAP my_bmp;
    HDC mem_dc;
    RECT alt_rc;

    if (image->buffer) {
        p_Dw_widget_draw_widget_box (widget, hdc, area, FALSE);

        content.x = p_Dw_style_box_offset_x (widget->style);
        content.y = p_Dw_style_box_offset_y (widget->style);
        content.width = DW_WIDGET_CONTENT_WIDTH(widget);
        content.height = DW_WIDGET_CONTENT_HEIGHT(widget);

        if (p_Dw_rectangle_intersect (area, &content, &intersection)) {
            vx = p_Dw_widget_x_world_to_viewport (widget, widget->allocation.x);
            vy = p_Dw_widget_y_world_to_viewport (widget, widget->allocation.y);

            if (image->scaled_buffer)
                buffer = image->scaled_buffer;
            else
                buffer = image->buffer;

            bstart = buffer + 
                    3 * ( intersection.x - p_Dw_style_box_offset_x (widget->style)+
                    content.width * (intersection.y -
                                   p_Dw_style_box_offset_y (widget->style)));

            my_bmp.flags = MYBMP_TYPE_BGR | MYBMP_FLOW_DOWN | MYBMP_RGBSIZE_3;
            my_bmp.frames = 1;
            my_bmp.depth = 24;
            my_bmp.w = intersection.width;
            my_bmp.h = intersection.height;
            my_bmp.pitch = content.width * 3;
            my_bmp.bits = bstart;

            mem_dc = CreateMemDCFromMyBitmap (&my_bmp, NULL);
            BitBlt (mem_dc, 0, 0, my_bmp.w, my_bmp.h, 
                        hdc, vx + intersection.x, vy + intersection.y, 0);
            DeleteMemDC (mem_dc);
        }
    }
    else {
        if (image->alt_text && image->alt_text[0]) {
            SIZE ext;
            FONTMETRICS fm;

            SelectFont (hdc, widget->style->font->font);
            GetFontMetrics (widget->style->font->font, &fm);
            if (image->alt_text_width == -1) {
                GetTextExtent (hdc, image->alt_text, -1, &ext);
                image->alt_text_width = ext.cx;
            }

            if (widget->allocation.width < image->alt_text_width ||
                    widget->allocation.ascent + widget->allocation.descent
                    < fm.ascent + fm.descent)
                p_Dw_widget_will_clip (widget);

            p_Dw_widget_draw_widget_box (widget, hdc, area, FALSE);

            SetTextColor (hdc, widget->style->color->pixel);

            if(p_Dw_widget_get_bg_color (widget) && p_Dw_widget_get_bg_color (widget)->pixel)
                SetBkColor (hdc, p_Dw_widget_get_bg_color (widget)->pixel);
            
            alt_rc.left = p_Dw_widget_x_world_to_viewport (widget, widget->allocation.x);
            alt_rc.top = p_Dw_widget_y_world_to_viewport (widget, widget->allocation.y) + fm.ascent;
            alt_rc.right = alt_rc.left + DW_WIDGET_CONTENT_WIDTH(widget); 
            alt_rc.bottom = alt_rc.top + DW_WIDGET_CONTENT_HEIGHT(widget);
            
            DrawText(hdc,image->alt_text, -1, &alt_rc,
                    DT_LEFT);
#if 0
            TextOut (hdc, 
                p_Dw_widget_x_world_to_viewport (widget, widget->allocation.x),
                p_Dw_widget_y_world_to_viewport (widget, widget->allocation.y)
                    + fm.ascent,
                image->alt_text);
#endif
        }
    }

    for (i = 0; i < DW_HIGHLIGHT_NUM_LAYERS && !selected; i++)
        selected = image->selected[i];
    if (selected)
        p_Dw_widget_draw_selected (widget, hdc, &intersection);
}

/*
 * Standard Dw function.
 */
static gboolean Dw_image_enter_notify (DwWidget *widget,
                                       DwWidget *last_widget,
                                       DWORD flags)
{
    return FALSE;
}


/*
 * Standard Dw function.
 */
static gboolean Dw_image_leave_notify (DwWidget *widget,
                                       DwWidget *next_widget,
                                       DWORD flags)
{
    DwImage *image = DW_IMAGE (widget);
    gboolean return_val = FALSE;

    if (image->hover_link != -1) {
        image->hover_link = -1;
        g_signal_emit (G_OBJECT (widget), image_signals[LINK_ENTERED], 0,
                       -1, -1, -1, &return_val);
        return return_val;
    }

    return FALSE;
}

/*
 * Standard Dw function.
 */
static gboolean Dw_image_button_press (DwWidget *widget,
                                       gint32 x,
                                       gint32 y,
                                       DWORD flags)
{
    DwImage *image = DW_IMAGE (widget);
    gint link_x, link_y;
    gboolean return_val = FALSE;

    Dw_image_find_link (image, x, y, &image->pressed_link, &link_x, &link_y);

    if (image->pressed_link >= 0)
        g_signal_emit (G_OBJECT (widget), image_signals[LINK_PRESSED], 0,
                        image->pressed_link, link_x, link_y, flags,
                        &return_val);

    return return_val;
}


/*
 * Standard Dw function.
 */
static gboolean Dw_image_button_release (DwWidget *widget,
                                         gint32 x,
                                         gint32 y,
                                         DWORD flags)
{
    DwImage *image = DW_IMAGE (widget);
    gint link_pressed, link_released, link_x, link_y;
    gboolean return_val1 = FALSE, return_val2 = FALSE;

    link_pressed = image->pressed_link;
    Dw_image_find_link (image, x, y, &link_released, &link_x, &link_y);
    image->pressed_link = -1;

    if (link_released >= 0) {
        g_signal_emit (G_OBJECT (widget), image_signals[LINK_RELEASED], 0,
                       link_released, link_x, link_y, flags, &return_val1);
        if (link_pressed == link_released) {
            g_signal_emit (G_OBJECT (widget), image_signals[LINK_CLICKED], 0,
                          link_released, link_x, link_y, flags, &return_val2);
            return return_val1 || return_val2;
        }

        return return_val1;
    }

    return FALSE;
}


/*
 * Standard Dw function.
 */
static gboolean Dw_image_motion_notify (DwWidget *widget,
                                        gint32 x, gint32 y,
                                        DWORD flags)
{
    DwImage *image = DW_IMAGE (widget);
    gint link, link_old, link_x, link_y;
    gboolean return_val = FALSE;

    link_old = image->hover_link;
    Dw_image_find_link (image, x, y, &link, &link_x, &link_y);
    image->hover_link = link;

    /* In case of a ISMAP image, the signal has to be emitted always */
    if (link_x != -1 || link != link_old) {
        g_signal_emit (G_OBJECT (widget), image_signals[LINK_ENTERED], 0,
                       link, link_x, link_y, &return_val);
        return return_val;
    }
    else
        return (link != -1);
}

/*
 * Find a link given a coordinate location relative to the window.
 */
static void Dw_image_find_link (DwImage *image,
                                gint x, gint y,
                                gint *link,
                                gint *link_x, gint *link_y)
{
    DwStyle *style;
    DwWidget *widget = DW_WIDGET (image);

    *link_x = -1;
    *link_y = -1;

    if (image->map_list && image->usemap_url) {
        *link = Dw_image_map_list_find_link (image->map_list, 
                        image->usemap_url, 
                        x - p_Dw_style_box_offset_x (widget->style), 
                        y - p_Dw_style_box_offset_y (widget->style));
        if (*link != -1)
            return;
    }

    if (image->ismap &&
        (style = DW_WIDGET(image)->style) && style->x_link != -1) {
        *link = style->x_link;
        *link_x = x - p_Dw_style_box_offset_x (widget->style);
        *link_y = y - p_Dw_style_box_offset_y (widget->style);
        return;
    }

    *link = -1;
}


/*
 * Set or resize a image.
 */
void a_Dw_image_size (DwImage *image, gint width, gint height)
{
    gint Resize = (image->width != width || image->height != height);

    image->width = width;
    image->height = height;
    if (Resize) 
        p_Dw_widget_queue_resize (DW_WIDGET (image), 0, TRUE);

    if (image->buffer)
        /* if a_Dw_image_set_buffer has been called before */
        Dw_image_scale (image);
}

/*
 * Called after the RGB line buffer has been copied into the full
 * image buffer. Uses for drawing and scaling.
 */
void a_Dw_image_draw_row (DwImage *image,
                          gint Width, gint Height, gint x, gint y)
{
    DwWidget *widget = DW_WIDGET (image);
    gint dy1, dy2;

    _MSG ("a_Dw_image_draw_row: x=%d y=%d\n", x, y);
    g_return_if_fail (image->buffer != NULL);

    if (image->scaled_buffer) {
        Dw_image_scale_row (image, y);

        dy1 = Dw_image_scaled_y (image, y);
        dy2 = Dw_image_scaled_y (image, y + 1);
#if 0
      p_Dw_widget_queue_draw_area(widget,
                                  x,
                                  0 + p_Dw_style_box_offset_y(widget->style),
                                  widget->allocation.width, dy2);
    }else
      p_Dw_widget_queue_draw_area (widget,
                                   x,
                                   0 + p_Dw_style_box_offset_y (widget->style),
                                   widget->allocation.width, y);
#endif
    }
    /* the image is download cpmplate!*/
    if ((image->height -1) == y || image->state != DW_IMAGE_FILLING) {
      //  print_message ("redraw the image due to dw_image %p downloaded completely", widget);
        p_Dw_widget_update_area (widget, &widget->allocation, TRUE);
    }
}

#ifdef ENABLE_ANIMATION
void a_Dw_image_draw_row_ex (DwImage *image,
                          gint Width, gint Height, gint x, gint y)
{
    DwWidget *widget = DW_WIDGET (image);
    gint dy1, dy2;

    g_return_if_fail (image->buffer != NULL);

    if (image->scaled_buffer) {
        Dw_image_scale_row (image, y);

        dy1 = Dw_image_scaled_y (image, y);
        dy2 = Dw_image_scaled_y (image, y + 1);
    } 
   
    if ((image->height -1) == y) {
        p_Dw_widget_update_area (widget, &widget->allocation , TRUE);
    }
}

#endif

/*
 * Set the widget buffer to reference the dicache entry buffer
 */
void a_Dw_image_set_buffer (DwImage *image, guchar *ImageBuffer,
                            mSpiderUrl *url, gint version)
{
    image->buffer = ImageBuffer;
    image->url = url;
    image->version = version;

    if (image->width > 0 && image->height > 0)
        /* if a_Dw_image_set_size has been called before */
        Dw_image_scale (image);
}

/*
 * Sets image as server side image map.
 */
void a_Dw_image_set_ismap (DwImage *image)
{
    image->ismap = TRUE;
}

/*
 * Sets image as client side image map.
 */
void a_Dw_image_set_usemap (DwImage *image,  DwImageMapList *map_list,
                            mSpiderUrl *usemap_url)
{
    image->map_list = map_list;

    if (image->usemap_url)
        a_Url_free (image->usemap_url);
    image->usemap_url = usemap_url ? a_Url_dup (usemap_url) : NULL;
}

/*
 * Scale the whole image: Compare buffer size with allocation, and, if
 * necessary, allocate a second buffer and scale all rows.
 */
static void Dw_image_scale (DwImage *image)
{
    int w, h, y;
    DwWidget *widget;

    if (!DW_WIDGET(image)->viewport)
        return;

    if (image->scaled_buffer) {
        g_free (image->scaled_buffer);
        image->scaled_buffer = NULL;
    }

    widget = DW_WIDGET (image);
    w = DW_WIDGET_CONTENT_WIDTH(widget);
    h = DW_WIDGET_CONTENT_HEIGHT(widget);

    /* Zero or negative sizes? Ignore. */
    if (w <= 0 || h <= 0)
        return;

    if (image->width != w || image->height != h) {
        /* scaled image */
        image->scaled_buffer = g_malloc ((gulong)3 * w * h);
        
        image->state = DW_IMAGE_SCALEING;
        for (y = 0; y < image->height; y++)
            Dw_image_scale_row (image, y);
    }
}


/*
 * Scale one row. y_src is the row in the dicache buffer.
 */
void Dw_image_scale_row (DwImage *image, gint y_src)
{
    DwWidget *widget;
    guchar *src, *dest, *dest1;
    gint w_src, w_dest, x_src, x_dest, y_dest1, y_dest2, y_dest, delta;

    widget = DW_WIDGET (image);
    w_src = image->width;
    w_dest = DW_WIDGET_CONTENT_WIDTH(widget);
    y_dest1 = Dw_image_scaled_y (image, y_src);
    y_dest2 = Dw_image_scaled_y (image, y_src + 1);

    src = image->buffer + 3 * y_src * w_src;

    if (y_dest1 != y_dest2) {
        dest1 = image->scaled_buffer + 3 * y_dest1 * w_dest;

        if (w_src == w_dest)
            memcpy (dest1, src, 3 * (size_t)w_src);
        else if (w_dest > w_src) {
            delta = w_src / 2;
            x_src = 0;
            x_dest = 0;

            while (x_dest < w_dest) {
                memcpy (dest1 + 3 * x_dest, src + 3 * x_src, 3);
                x_dest++;
                delta += w_src;
                while (delta > w_dest) {
                delta -= w_dest;
                x_src++;
                }
            }
        }
        else {
            delta = w_dest / 2;
            x_src = 0;
            x_dest = 0;

            while (x_src < w_src) {
                memcpy (dest1 + 3 * x_dest, src + 3 * x_src, 3);
                x_src++;
                delta += w_dest;
                while (delta > w_src) {
                    delta -= w_src;
                    x_dest++;
                }
            }
        }

        /* The other lines are simply copied. */
        for (y_dest = y_dest1 + 1; y_dest < y_dest2; y_dest++) {
            dest = image->scaled_buffer + 3 * y_dest * w_dest;
            memcpy (dest, dest1, 3 * (size_t)w_dest);
        }
    }
}


/*
 * Image Maps
 */

/*
 * Initialize a DwImageMapList. The memory has to be allocated before.
 */
void a_Dw_image_map_list_init (DwImageMapList *list)
{
    list->num_maps = 0;
    list->num_maps_max = 8;
    list->maps = g_new (DwImageMap, list->num_maps_max);

    list->num_shapes = 0;
    list->num_shapes_max = 8;
    list->shapes = g_new (DwImageMapShape, list->num_shapes_max);
}

/*
 * Free the content of a DwImageMapList. The memory for the list is
 * not freed.
 */
void a_Dw_image_map_list_free (DwImageMapList *list)
{
    gint i;

    for (i = 0; i < list->num_maps; i++)
        a_Url_free (list->maps[i].url);
    g_free (list->maps);

    for (i = 0; i < list->num_shapes; i++) {
        if (list->shapes[i].type == DW_IMAGE_MAP_SHAPE_POLY)
            EmptyClipRgn (&list->shapes[i].data.poly);
    }

    g_free (list->shapes);
}

/*
 * Add a new map to the list. Next added shapes will belong to this
 * map.
 */
void a_Dw_image_map_list_add_map (DwImageMapList *list,
                                  mSpiderUrl *url)
{
    list->num_maps++;
    a_List_add (list->maps, list->num_maps, list->num_maps_max);
    list->maps[list->num_maps - 1].url = a_Url_dup (url);
    list->maps[list->num_maps - 1].start_shape = list->num_shapes;
}


void a_Dw_image_map_list_add_shape (DwImageMapList *list,
                                    gint type,
                                    gint link,
                                    POINT *points,
                                    gint num_points)
{
    gboolean correct_args;
    DwImageMapShape *shape;

    switch (type) {
    case DW_IMAGE_MAP_SHAPE_RECT:
        correct_args = (num_points >= 2);
        break;

    case DW_IMAGE_MAP_SHAPE_CIRCLE:
        correct_args = (num_points >= 1 && points[1].x != 0);
        break;

    case DW_IMAGE_MAP_SHAPE_POLY:
        correct_args = (num_points >= 3);
        break;

    default:
        correct_args = FALSE;
    }

    if (correct_args) {
        list->num_shapes++;
        a_List_add (list->shapes, list->num_shapes, list->num_shapes_max);
        shape = &list->shapes[list->num_shapes - 1];
        shape->type = type;
        shape->link = link;

        switch (type) {
        case DW_IMAGE_MAP_SHAPE_RECT:
            shape->data.rect.left = points[0].x;
            shape->data.rect.top = points[0].y;
            shape->data.rect.right = points[1].x;
            shape->data.rect.bottom = points[1].y;
            break;

        case DW_IMAGE_MAP_SHAPE_CIRCLE:
            shape->data.circle.x = points[0].x;
            shape->data.circle.y = points[0].y;
            shape->data.circle.r2 = points[1].x * points[1].x;
            break;

        case DW_IMAGE_MAP_SHAPE_POLY:
            InitClipRgn (&shape->data.poly, &__msp_cliprc_heap);
            InitPolygonRegion (&shape->data.poly, points, num_points);
            break;
        }
    }
}


/*
 * Find a link in an image map.
 */
static gint Dw_image_map_list_find_link (DwImageMapList *list,
                                         mSpiderUrl *url,
                                         gint x,
                                         gint y)
{
    gint i, j, start, end, dx, dy;
    DwImageMapShape *shape;

    for (i = 0; i < list->num_maps; i++) {
        if (a_Url_cmp (list->maps[i].url, url) == 0 &&
            URL_STRCAMP_EQ(URL_FRAGMENT_(list->maps[i].url),URL_FRAGMENT_(url))){
            /* map found */
            start = list->maps[i].start_shape;
            if (i == list->num_maps - 1)
                end = list->num_shapes;
            else
                end = list->maps[i + 1].start_shape;

            for (j = start; j < end; j++) {
                shape = &list->shapes[j];
                switch (shape->type) {
                case DW_IMAGE_MAP_SHAPE_CIRCLE:
                    dx = shape->data.circle.x - x;
                    dy = shape->data.circle.y - y;
                    if (shape->data.circle.r2 >= (dx*dx + dy*dy))
                        return shape->link;
                    break;

                case DW_IMAGE_MAP_SHAPE_RECT:
                    if (x > shape->data.rect.left &&
                            x < shape->data.rect.right &&
                            y > shape->data.rect.top &&
                            y < shape->data.rect.bottom)
                        return shape->link;
                    break;

                case DW_IMAGE_MAP_SHAPE_POLY:
                    if (PtInRegion (&shape->data.poly, x, y))
                        return shape->link;
                    break;
                }
            }

            /* no shape found */
            return -1;
        }
    }

    /* no map found */
    return -1;
}

static DwIterator *Dw_image_iterator (DwWidget *widget,
                                       gint32 mask,
                                       gboolean at_end)
{
    DwIterator *it;
    DwImage *image = DW_IMAGE (widget);

    if (image->alt_text) {
        it = p_Dw_widget_text_iterator (widget, mask, at_end, image->alt_text);
        if (it)
            it->highlight = Dw_image_iterator_highlight;
        return it;
    }
    else
        return NULL;
}

static void Dw_image_iterator_highlight (DwIterator *it,
                                          gint start,
                                          gint end,
                                          DwHighlightLayer layer)
{

    if (it->content.type == DW_CONTENT_TEXT) {
        /* The whole image is highlighted, as soon as something is selected at
         * all. */
        DW_IMAGE(it->widget)->selected[layer] = (start != -1 && start != end);
        p_Dw_widget_queue_draw (it->widget);
    }
}

