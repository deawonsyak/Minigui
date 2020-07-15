/*
 * File: dw_bullet.c
 *
 * Copyright (C) 2005-2006 Feynman Software
 * Copyright (C) 1997       Raph Levien <raph@acm.org>
 * Copyright (C) 1999       Luca Rota <drake@freemail.it>
 * Copyright (C) 2001-2003  Sebastian Geerken <s.geerken@ping.de>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

/*
 * Bullets are drawn 1/5 of an x-height above the baseline, and are
 * 4/5 of an x-height wide and high.
 */

#include "dw_bullet.h"
#include "dw_viewport.h"

static void        Dw_bullet_init               (DwBullet *bullet);
static void        Dw_bullet_class_init         (DwBulletClass *klass);
static void        Dw_bullet_size_request       (DwWidget *widget,
                                                 DwRequisition *requisition);
static void        Dw_bullet_draw               (DwWidget *widget,
                                                 HDC hdc,
                                                 DwRectangle *area);
static DwIterator* Dw_bullet_iterator           (DwWidget *widget,
                                                gint32 mask,
                                                gboolean at_end);
static void        Dw_bullet_iterator_highlight (DwIterator *it,
                                                 gint start,
                                                 gint end,
                                                 DwHighlightLayer layer);



GType a_Dw_bullet_get_type (void)
{
   static GType type = 0;

   if (!type) {
      static const GTypeInfo info = {
         sizeof (DwBulletClass),
         (GBaseInitFunc) NULL,
         (GBaseFinalizeFunc) NULL,
         (GClassInitFunc) Dw_bullet_class_init,
         (GClassFinalizeFunc) NULL,
         (gconstpointer) NULL,
         sizeof (DwBullet),
         0,
         (GInstanceInitFunc) Dw_bullet_init
      };

      type = g_type_register_static (DW_TYPE_WIDGET, "DwBullet", &info, 0);
   }

   return type;
}


DwWidget* a_Dw_bullet_new (void)
{
   return DW_WIDGET (g_object_new (DW_TYPE_BULLET, NULL));
}


static void Dw_bullet_init (DwBullet *bullet)
{
   int i;
   for (i = 0; i < DW_HIGHLIGHT_NUM_LAYERS; i++)
      bullet->selected[i] = FALSE;
}


static void Dw_bullet_class_init (DwBulletClass *klass)
{
   DwWidgetClass *widget_class;

   widget_class = (DwWidgetClass*)klass;
   widget_class->size_request = Dw_bullet_size_request;
   widget_class->draw = Dw_bullet_draw;
   widget_class->iterator = Dw_bullet_iterator;
}


static void Dw_bullet_size_request (DwWidget *widget,
                                    DwRequisition *requisition)
{
   requisition->width = MAX (widget->style->font->space_width, 5);
   requisition->ascent = MAX (widget->style->font->x_height, 1);
   requisition->descent = 0;
}


static void Dw_bullet_draw (DwWidget *widget, HDC hdc, DwRectangle *area)
{
   gint32 x0, y0, x, y;
   DwStyleColor *bg_color;
   gint32 l;
   int i;
   gboolean selected = FALSE;

   for (i = 0; i < DW_HIGHLIGHT_NUM_LAYERS && !selected; i++)
      selected = DW_BULLET(widget)->selected[i];

   l = MIN (widget->allocation.width, widget->allocation.ascent);
   x = x0 = p_Dw_widget_x_world_to_viewport (widget, widget->allocation.x);
   y = y0 = p_Dw_widget_y_world_to_viewport (widget, widget->allocation.y);

   if (selected) {
      bg_color = p_Dw_widget_get_bg_color (widget);
      SetBrushColor (hdc, bg_color->inverse_pixel);
      FillBox (hdc, x0, y0, 
                      widget->allocation.width, widget->allocation.ascent);
   }

   SetBrushColor (hdc, selected ? widget->style->color->inverse_pixel : 
                       widget->style->color->pixel);
   SetPenColor (hdc, selected ? widget->style->color->inverse_pixel : 
                       widget->style->color->pixel);

   x += (widget->allocation.width - l) / 2;
   y += (widget->allocation.ascent - l) / 2;

   switch (widget->style->list_style_type) {
   case DW_STYLE_LIST_STYLE_TYPE_DISC:
      FillCircle (hdc, x + l/2, y + l/2, l/2);
      break;
   case DW_STYLE_LIST_STYLE_TYPE_CIRCLE:
      Circle (hdc, x + l/2, y + l/2, l/2);
      break;
   case DW_STYLE_LIST_STYLE_TYPE_SQUARE:
      Rectangle (hdc, x + l/4, y + l/4, x + 3*l/4, y + 3*l/4);
      break;
   default:
      break;
   }
}

static DwIterator *Dw_bullet_iterator (DwWidget *widget,
                                        gint32 mask,
                                        gboolean at_end)
{
   DwIterator *it;
   gchar *text;

   switch (widget->style->list_style_type) {
   case DW_STYLE_LIST_STYLE_TYPE_DISC:
      text = "*";
      break;
   case DW_STYLE_LIST_STYLE_TYPE_CIRCLE:
      text = "o";
      break;
   case DW_STYLE_LIST_STYLE_TYPE_SQUARE:
      text = "-";
      break;
   default:
      text = "?";
      break;
   }

   it = p_Dw_widget_text_iterator (widget, mask, at_end, text);
   if (it)
     it->highlight = Dw_bullet_iterator_highlight;
   return it;
}

static void Dw_bullet_iterator_highlight (DwIterator *it,
                                          gint start,
                                          gint end,
                                          DwHighlightLayer layer)
{
   if (it->content.type == DW_CONTENT_TEXT) {
      DW_BULLET(it->widget)->selected[layer] = (start == 0 && end >= 1);
      p_Dw_widget_queue_draw (it->widget);
   }
}
