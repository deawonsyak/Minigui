/*
 * File: dw_hruler.c
 *
 * Copyright (C) 2005-2006 Feynman Software
 * Copyright (C) 2000 Jorge Arellano Cid <jcid@mspider.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

/*
 * This is really an empty widget, the HTML parser puts a border
 * around it, and drawing is done in Dw_widget_draw_widget_box. The
 * only remarkable point is that the DW_HAS_CONTENT flag is
 * cleared.
 */

#include "dw_hruler.h"
#include "dw_viewport.h"

static void        Dw_hruler_init               (DwHruler *hruler);
static void        Dw_hruler_class_init         (DwHrulerClass *klass);

static void        Dw_hruler_size_request       (DwWidget *widget,
                                                 DwRequisition *requisition);
static void        Dw_hruler_draw               (DwWidget *widget,
                                                 HDC hdc,
                                                 DwRectangle *area);
static DwIterator* Dw_hruler_iterator           (DwWidget *widget,
                                                 gint32 mask,
                                                 gboolean at_end);
static void        Dw_hruler_iterator_highlight (DwIterator *it,
                                                 gint start,
                                                 gint end,
                                                 DwHighlightLayer layer);


GType a_Dw_hruler_get_type (void)
{
   static GType type = 0;

   if (!type) {
      static const GTypeInfo info = {
         sizeof (DwHrulerClass),
         (GBaseInitFunc) NULL,
         (GBaseFinalizeFunc) NULL,
         (GClassInitFunc) Dw_hruler_class_init,
         (GClassFinalizeFunc) NULL,
         (gconstpointer) NULL,
         sizeof (DwHruler),
         0,
         (GInstanceInitFunc) Dw_hruler_init
      };

      type = g_type_register_static (DW_TYPE_WIDGET, "DwHruler", &info, 0);
   }

   return type;
}


DwWidget* a_Dw_hruler_new (void)
{
   return DW_WIDGET (g_object_new (DW_TYPE_HRULER, NULL));
}


static void Dw_hruler_init (DwHruler *hruler)
{
   int i;
   DW_WIDGET_UNSET_FLAGS (hruler, DW_HAS_CONTENT);
   for (i = 0; i < DW_HIGHLIGHT_NUM_LAYERS; i++)
      hruler->selected[i] = FALSE;
}


static void Dw_hruler_class_init (DwHrulerClass *klass)
{
   GObjectClass *object_class;
   DwWidgetClass *widget_class;

   object_class = G_OBJECT_CLASS (klass);

   widget_class = (DwWidgetClass*)klass;
   widget_class->size_request = Dw_hruler_size_request;
   widget_class->draw = Dw_hruler_draw;
   widget_class->iterator = Dw_hruler_iterator;
}


static void Dw_hruler_size_request (DwWidget *widget,
                                    DwRequisition *requisition)
{
   requisition->width = p_Dw_style_box_diff_width (widget->style);
   requisition->ascent = p_Dw_style_box_diff_height (widget->style);
   requisition->descent = 0;
}


static void Dw_hruler_draw (DwWidget *widget,
                            HDC hdc,
                            DwRectangle *area)
{
   int i;
   gboolean selected = FALSE;

   for (i = 0; i < DW_HIGHLIGHT_NUM_LAYERS && !selected; i++)
      selected = DW_HRULER(widget)->selected[i];
   p_Dw_widget_draw_widget_box (widget, hdc, area, selected);
   if (selected)
      p_Dw_widget_draw_selected (widget, hdc, area);
}


static DwIterator *Dw_hruler_iterator (DwWidget *widget,
                                       gint32 mask,
                                       gboolean at_end)
{
   DwIterator *it;

   it = p_Dw_widget_text_iterator (widget, mask, at_end,
                                   "-----------------------------------"
                                   "-----------------------------------");
   if (it)
     it->highlight = Dw_hruler_iterator_highlight;
   return it;
}

static void Dw_hruler_iterator_highlight (DwIterator *it,
                                          gint start,
                                          gint end,
                                          DwHighlightLayer layer)
{
   if (it->content.type == DW_CONTENT_TEXT) {
      DW_HRULER(it->widget)->selected[layer] = (start == 0 && end >= 1);
      p_Dw_widget_queue_draw (it->widget);
   }
}
