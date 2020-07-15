/*
 * File: dw_viewport.c
 *
 * Copyright (C) 2005-2006 Feynman Software
 * Copyright (C) 2001-2003  Sebastian Geerken <S.Geerken@ping.de>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include "browser.h"
#include "mspider.h"
#include "emspider.h"
#include "msg.h"
#include "dw_viewport.h"
#include "dw_container.h"
#include "list.h"

#define DEBUG_LEVEL 10
#include "debug.h"
#include "msg.h"

typedef struct
{
   gchar* name;
   DwWidget *widget;
   gint32 y;
} DwViewportAnchor;

/**********************
 *                    *
 *  public functions  *
 *                    *
 **********************/

DwViewport*  a_Dw_viewport_new  (HWND hwnd)
{
    RECT rc;

    DwViewport* viewport = g_new0 (DwViewport, 1);

    viewport->hwnd = hwnd;
    
    GetClientRect (hwnd, &rc);
    viewport->draw_resize_idle_id = 0;
    viewport->allocation.x = rc.left;
    viewport->allocation.y = rc.top;
    viewport->allocation.width = rc.right;
    viewport->allocation.height = rc.bottom;
    viewport->anchors_table = g_hash_table_new (g_str_hash, g_str_equal);

#ifdef ENABLE_FINDTEXT
    viewport->findtext_state = a_Findtext_state_new ();
#endif/* ENABLE_FINDTEXT */
    viewport->draw_areas = NULL;
    viewport->num_draw_areas = 0;
    viewport->num_draw_areas_max = 4; 

    return viewport;
}

static gboolean Dw_viewport_destroy_anchor (gpointer key,
                                                gpointer value,
                                                gpointer user_data)
{
   g_free (value);
   return TRUE;
}


void a_Dw_viewport_destroy (DwViewport* viewport)
{
    g_hash_table_foreach_remove (viewport->anchors_table,
                                 Dw_viewport_destroy_anchor, NULL);

    g_hash_table_destroy (viewport->anchors_table);

#ifdef ENABLE_FINDTEXT
    a_Findtext_state_destroy (viewport->findtext_state);
#endif/* ENABLE_FINDTEXT */
    Dw_viewport_remove_anchor (viewport);

    


   if (viewport->draw_resize_idle_id != 0)
        g_idle_remove_by_data (viewport);

    viewport->num_draw_areas = 0;

    g_free (viewport->draw_areas);

    g_free (viewport);

}

/*
 * Used by Dw_viewport_calc_size.
 */
static void Dw_viewport_calc_child_size (DwViewport *viewport,
                                             gint32 child_width,
                                             gint32 child_height,
                                             DwRequisition *child_requisition)
{
   if (child_width < 0) child_width = 0;
   if (child_height < 0) child_height = 0;

   DEBUG_MSG (2, "   width = %d, height = %d ...\n",
              child_width, child_height);

   a_Dw_widget_set_width (viewport->child, child_width);
   a_Dw_widget_set_ascent (viewport->child, child_height);
   a_Dw_widget_set_descent (viewport->child, 0);

   a_Dw_widget_size_request (viewport->child, child_requisition);
}


static void Dw_viewport_set_scrollbars (DwViewport* viewport,
                            gint32 width,
                            gint32 height)
{
    SCROLLINFO si;

    if (width <= viewport->allocation.width) {
//        SetScrollPos (viewport->hwnd, SB_HORZ, 0);
        si.fMask = SIF_RANGE | SIF_PAGE | SIF_POS;
        si.nMax = 100;
        si.nMin = 0;
        si.nPage = 101;
        si.nPos = 0;
//        EnableScrollBar (viewport->hwnd, SB_HORZ, TRUE);
        SetScrollInfo (viewport->hwnd, SB_HORZ, &si, TRUE);
        EnableScrollBar (viewport->hwnd, SB_HORZ, FALSE);
    }
    else {
        si.fMask = SIF_RANGE | SIF_PAGE | SIF_POS;
        si.nMax = width;
        si.nMin = 0;
        si.nPage = viewport->allocation.width;
        si.nPos = viewport->world_x;
        EnableScrollBar (viewport->hwnd, SB_HORZ, TRUE);
        SetScrollInfo (viewport->hwnd, SB_HORZ, &si, TRUE);
    }

    if (height <= viewport->allocation.height) {
//        SetScrollPos (viewport->hwnd, SB_VERT, 0);
        si.fMask = SIF_RANGE | SIF_PAGE | SIF_POS;
        si.nMax = 100;
        si.nMin = 0;
        si.nPage = 101;
        si.nPos = 0;
//        EnableScrollBar (viewport->hwnd, SB_VERT, TRUE);
        SetScrollInfo (viewport->hwnd, SB_VERT, &si, TRUE);
        EnableScrollBar (viewport->hwnd, SB_VERT, FALSE);
    }
    else {
        si.fMask = SIF_RANGE | SIF_PAGE | SIF_POS;
        si.nMax = height;
        si.nMin = 0;
        si.nPage = viewport->allocation.height;
        si.nPos = viewport->world_y;
        EnableScrollBar (viewport->hwnd, SB_VERT, TRUE);
        SetScrollInfo (viewport->hwnd, SB_VERT, &si, TRUE);
    }
}

/*
 * Calculate the size of the scrolled area and allocate the top-level
 * widget. This function is called when the top-level Dw widget has
 * changed its size etc.
 */
void Dw_viewport_calc_size (DwViewport *viewport)
{
   RECT rc_viewport;
   DwRequisition child_requisition;
   DwAllocation child_allocation;

   gint max_width, max_height;

   if (viewport->calc_size_blocked)
      return;

   viewport->calc_size_blocked = TRUE;

   if (viewport->child) {
      /*
       * Determine the size hints for the Dw widget. This is a bit
       * tricky, because you must know if scrollbars are visible or
       * not, which depends on the size of the Dw widget, which then
       * depends on the hints. The idea is to test several
       * configurations, there are four of them, from combining the
       * cases horizontal/vertical scrollbar visible/invisible.
       *
       * For optimization, the horizontal scrollbar is currently not
       * regarded, the height hint is always the same, as if the
       * scrollbar was allways visible. In future, this may be
       * implemented correctly, by using the minimal width to optimize
       * most cases. (Minimal widths will also be used by tables.)
       *
       * Furthermore, the last result (vertical scrollbar visible or
       * not) is stored in the viewport, and tested first. This will
       * make a second test only necessary when the visibility
       * switches, which normally happens only once when filling the
       * page with text. (Actually, this assumes that the page size is
       * always *growing*, but this is nevertheless true in mspider.)
       */

      max_width = viewport->allocation.width;
      max_height = viewport->allocation.height;


      DEBUG_MSG (2, "------------------------------------------------->\n");
      DEBUG_MSG (2, "Dw_viewport_calc_size: %d x %d -> %d x %d\n",
                 viewport->allocation.width, viewport->allocation.height,
                 max_width, max_height);

      Dw_viewport_calc_child_size (viewport, max_width,
                                          max_height,
                                          &child_requisition);

      child_allocation.x = 0;
      child_allocation.y = 0;
      child_allocation.width = child_requisition.width;
      child_allocation.ascent = child_requisition.ascent;
      child_allocation.descent = child_requisition.descent;
      a_Dw_widget_size_allocate (viewport->child, &child_allocation);


      DEBUG_MSG (1, "Setting size to %d x %d\n",
                 child_requisition.width,
                 child_requisition.ascent + child_requisition.descent);

      DEBUG_MSG (2, "<-------------------------------------------------\n");

      Dw_viewport_set_scrollbars (viewport, child_allocation.width, 
                    child_requisition.ascent + child_requisition.descent);
#if 0
     rc_viewport.left = child_allocation.x;
     rc_viewport.top = child_allocation.y;
     rc_viewport.right = child_allocation.x + child_allocation.width;
     rc_viewport.bottom = child_allocation.y + child_allocation.ascent + child_allocation.descent;
#else
     GetWindowRect(viewport->child->hwnd, &rc_viewport);
#endif   
     InvalidateRect (viewport->child->hwnd, &rc_viewport, FALSE);
   } else {
      viewport->hscrollbar_used = FALSE;
      viewport->vscrollbar_used = FALSE;
   }

   Dw_viewport_update_anchor (viewport);

   viewport->calc_size_blocked = FALSE;
}

/*
 * Set the top-level Dw widget.
 * If there is already one, you must destroy it before, otherwise the
 * function will fail.
 */
void a_Dw_viewport_add_dw (DwViewport *viewport, DwWidget *widget)
{
#ifdef ENABLE_LINKTRAVE
    LinktraveManage *linktrave_manage;
    mSpiderDoc* dd = (mSpiderDoc*) GetWindowAdditionalData2 (viewport->hwnd);
#endif


    if (viewport->child != NULL)
        g_signal_emit_by_name (G_OBJECT(viewport->child), "destroy", 0);

    viewport->world_x = viewport->world_y = 0;

    viewport->child = widget;
    DBG_OBJ_ASSOC(widget, viewport);

    widget->parent = NULL;
    widget->viewport = viewport;
#if 0
   widget->window = viewport->back_pixmap;
#endif

    a_Dw_widget_realize (widget);
    Dw_viewport_calc_size (viewport);
    Dw_viewport_remove_anchor (viewport);
#ifdef ENABLE_FINDTEXT
   a_Findtext_state_set_widget (viewport->findtext_state, widget);
#endif/* ENABLE_FINDTEXT */

#ifdef ENABLE_LINKTRAVE
   linktrave_manage =(LinktraveManage*)dd->bw->linktrave_manage ;

   if (linktrave_manage->top_dd == dd) { 
         a_Linktrave_state_set_widget (linktrave_manage->linktrave_state, widget);
        linktrave_manage->link_number_focus = -2;
    }
#endif /* ENABLE_LINKTRAVE */
}

/**************************************************
 *                                                *
 *  Functions used by DwViewport and DwWidget  *
 *                                                *
 **************************************************/

/*
 * This function only *recognizes* that the top-level Dw widget is to
 * be removed. It is called by Dw_widget_destroy.
 * Don't use this function directly!
 */
void Dw_viewport_remove_dw (DwViewport *viewport)
{
   gint num_anchors_left;
   
    if (NULL == viewport)
        return;
   /* Test, that all anchors have been removed properly. */
   num_anchors_left = g_hash_table_size (viewport->anchors_table);
   /* g_assert (num_anchors_left == 0); */
   if (num_anchors_left != 0)
        g_warning ("%d anchors left", num_anchors_left);


   Dw_viewport_remove_anchor (viewport);

   viewport->child = NULL;
   Dw_viewport_calc_size (viewport);
}

/* used by Dw_viewport_widget_at_point */
typedef struct
{
   gint32 x;
   gint32 y;
   DwWidget *widget;
} WidgetAtPointData;

/* used by Dw_viewport_widget_at_point */
static void Dw_viewport_widget_at_point_callback (DwWidget *widget,
                                                      gpointer data)
{
   WidgetAtPointData *callback_data;

   callback_data = (WidgetAtPointData*) data;
   DEBUG_MSG (1, "  Checking %p ...\n", widget);

   if (/* As a special exception, for the top-level widget, not the
        * allocation is regarded, but the whole viewport. This makes
        * selections more useful, since so the user can start the
        * selection outside of the allocation. */
       widget->parent == NULL ||
       /* Otherwise, check whether pointer is in the allocation. */
       (callback_data->x >= widget->allocation.x &&
        callback_data->y >= widget->allocation.y &&
        callback_data->x < widget->allocation.x + widget->allocation.width &&
        callback_data->y < widget->allocation.y + DW_WIDGET_HEIGHT(widget))) {
      DEBUG_MSG (1, "    yes\n");
      if (DW_IS_CONTAINER (widget))
         a_Dw_container_forall (DW_CONTAINER (widget),
                                Dw_viewport_widget_at_point_callback,
                                data);

      if (callback_data->widget == NULL)
         callback_data->widget = widget;
   }
}

/*
 * Return the widget at point (x, y) (world coordinates).
 */
DwWidget* Dw_viewport_widget_at_point (DwViewport *viewport,
                                           gint32 x,
                                           gint32 y)
{
   WidgetAtPointData callback_data;

   callback_data.x = x;
   callback_data.y = y;
   callback_data.widget = NULL;

   if (viewport->child)
      Dw_viewport_widget_at_point_callback (viewport->child,
                                                &callback_data);

   return callback_data.widget;
}

/*************
 *           *
 *  Anchors  *
 *           *
 *************/

/*
 * todo: Currently, no horizontal scrolling is done. This is generally
 * possible, DW_HPOS_INTO_VIEW should be used for this, but it is
 * rather complicated to determine the width of an anchor. This would
 * be the lenght of the region between <a> and </a>, the page widget
 * would have to have two kinds of content types (opening and closing
 * anchor), and some changes in the HTML parser are necessary.
 */

/*
 * Add an anchor, and assign a position for it. For all widgets
 * directly or indirectly assigned to a viewports, anchors must be
 * unique, this is tested. "name" is copied, so no strdup is
 * neccessary for the caller.
 *
 * Return the copy on success, or NULL, when this anchor had already
 * been added to the widget tree.
 *
 * The viewport gets the responsibility to free "name".
 */
gchar* p_Dw_viewport_add_anchor (DwWidget *widget,
                                     const gchar *name,
                                     gint32 y)
{
   DwViewport *viewport;
   DwViewportAnchor *anchor;

   _MSG("new anchor %p/'%s' -> %d\n", widget, name, y);

   g_return_val_if_fail (widget->viewport != NULL, NULL);
   viewport = DW_VIEWPORT (widget->viewport);

   if (g_hash_table_lookup_extended (viewport->anchors_table, name,NULL,NULL))
      /* Anchor does already exist. */
      return NULL;
   else {
      anchor = g_new (DwViewportAnchor, 1);
      anchor->name = g_strdup (name);
      anchor->widget = widget;
      anchor->y = y;
      
      g_hash_table_insert (viewport->anchors_table, anchor->name, anchor);
      Dw_viewport_update_anchor (viewport);
      
      return anchor->name;
   }
}


/*
 * Assign a position for an already existing anchor.
 */
void p_Dw_viewport_change_anchor (DwWidget *widget,
                                      gchar *name,
                                      gint32 y)
{
   DwViewport *viewport;
   DwViewportAnchor *anchor;
   gpointer tmp_anchor;
   gboolean exists;

   _MSG("changing anchor %p/'%s' -> %d\n", widget, name, y);

   g_return_if_fail (widget->viewport != NULL);
   viewport = DW_VIEWPORT (widget->viewport);

   exists =
      g_hash_table_lookup_extended (viewport->anchors_table, name, NULL,
                                    &tmp_anchor);
   g_return_if_fail(exists);

   anchor = tmp_anchor;
   g_return_if_fail(anchor->widget == widget);

   anchor->y = y;

   Dw_viewport_update_anchor (viewport);
}


/*
 * Remove an anchor from the table in the viewport. Notice that "name"
 * is freed here.
 */
void p_Dw_viewport_remove_anchor (DwWidget *widget,
                                      gchar *name)
{
   DwViewport *viewport;
   DwViewportAnchor *anchor;
   gpointer tmp_anchor;
   gboolean exists;

   _MSG("removing anchor %p/'%s'\n", widget, name);

   g_return_if_fail (widget->viewport != NULL);
   viewport = DW_VIEWPORT (widget->viewport);

   exists = 
      g_hash_table_lookup_extended (viewport->anchors_table, name, NULL,
                                    &tmp_anchor);
   g_return_if_fail(exists);

   anchor = tmp_anchor;
   g_return_if_fail(anchor->widget == widget);
   
   g_hash_table_remove (viewport->anchors_table, name);
   g_free (anchor->name);
   g_free (anchor);
}


/*
 * Called when possibly the scroll position has to be changed because
 * of anchors.
 */
void Dw_viewport_update_anchor (DwViewport *viewport)
{
   DwViewportAnchor *anchor;

   if (viewport->anchor &&
       g_hash_table_lookup_extended (viewport->anchors_table, viewport->anchor,
                                     NULL, (gpointer)&anchor)) {
       Dw_viewport_scroll_to (viewport, DW_HPOS_NO_CHANGE, DW_VPOS_TOP,
                                 0, anchor->y + anchor->widget->allocation.y,
                                 0, 0);
   }
}

/*
 * Sets the anchor to scroll to.
 */
void a_Dw_viewport_set_anchor (DwViewport *viewport,
                                   const gchar *anchor)
{
   Dw_viewport_remove_anchor (viewport);

   if (anchor) {
      viewport->anchor = g_strdup (anchor);
      Dw_viewport_update_anchor (viewport);
   } else {
      viewport->anchor = NULL;
      viewport->world_y = 0;
      SetScrollPos (viewport->hwnd, SB_VERT, 0);
   }
}

/*
 * Used by Dw_viewport_update_anchor_idle.
 */
static gboolean Dw_viewport_calc_into (gint32 requested_value,
                                           gint32 requested_size,
                                           gint32 current_value,
                                           gint32 size,
                                           gint32 *return_value)
{
   if (requested_size > size) {
      /* The viewport size is smaller than the size of the region which will
       * be shown. If the region is already visible, do not change the
       * position. Otherwise, show the left/upper border, this is most likely
       * what is needed. */
      if (current_value >= requested_value &&
          current_value + size < requested_value + requested_size)
         return FALSE;
      else
         requested_size = size;
   }

   if (requested_value < current_value) {
      *return_value = requested_value;
      return TRUE;
   } else if (requested_value + requested_size > current_value + size) {
      *return_value = requested_value - size + requested_size;
      return TRUE;
   } else
      return FALSE;
}

/*
 * See Dw_viewport_scroll_to.
 */
static gint Dw_viewport_update_anchor_idle (DwViewport *viewport)
{
   gint32 vp_width, vp_height, x = 0, y = 0;
   gboolean change_x, change_y;

   vp_width = viewport->allocation.width;
   vp_height = viewport->allocation.height;

   change_x = TRUE;
   switch (viewport->anchor_pos.hpos) {
   case DW_HPOS_LEFT:
      x = viewport->anchor_pos.x;
      break;
   case DW_HPOS_CENTER:
      x = viewport->anchor_pos.x - (vp_width - viewport->anchor_pos.width) / 2;
      break;
   case DW_HPOS_RIGHT:
      x = viewport->anchor_pos.x - (vp_width - viewport->anchor_pos.width);
      break;
   case DW_HPOS_INTO_VIEW:
      change_x = Dw_viewport_calc_into (viewport->anchor_pos.x,
                                            viewport->anchor_pos.width,
                                            viewport->world_x, vp_width, &x);
      break;
   case DW_HPOS_NO_CHANGE:
      change_x = FALSE;
      break;
   }

   change_y = TRUE;
   switch (viewport->anchor_pos.vpos) {
   case DW_VPOS_TOP:
      y = viewport->anchor_pos.y;
      break;
   case DW_VPOS_CENTER:
      y = viewport->anchor_pos.y -
         (vp_height - viewport->anchor_pos.height) / 2;
      break;
   case DW_VPOS_BOTTOM:
      y = viewport->anchor_pos.y - (vp_height - viewport->anchor_pos.height);
      break;
   case DW_VPOS_INTO_VIEW:
      change_y = Dw_viewport_calc_into (viewport->anchor_pos.y,
                                            viewport->anchor_pos.height,
                                            viewport->world_y, vp_height, &y);
      break;
   case DW_VPOS_NO_CHANGE:
      change_y = FALSE;
      break;
   }

   if (change_x) {
      MGD_scroll_to_x (viewport, x);
   }

   if (change_y) {
      MGD_scroll_to_y (viewport, y);
   }

   return FALSE;
}

/*
 * Sets the position to scroll to. The current anchor will be removed.
 */
void a_Dw_viewport_set_scrolling_position (DwViewport *viewport,
                                               gint32 x,
                                               gint32 y)
{
   Dw_viewport_remove_anchor (viewport);
   Dw_viewport_scroll_to (viewport, DW_HPOS_LEFT, DW_VPOS_TOP, x, y, 0, 0);
}

/*
 * Scrolls the viewport, so that the region [x, y, width, height] (world
 * coordinates) is seen, according to hpos and vpos.
 *
 * The actual scrolling is done in an idle function.
 */
void Dw_viewport_scroll_to (DwViewport *viewport,
                                DwHPosition hpos,
                                DwVPosition vpos,
                                gint32 x,
                                gint32 y,
                                gint32 width,
                                gint32 height)
{
   viewport->anchor_pos.hpos = hpos;
   viewport->anchor_pos.vpos = vpos;
   viewport->anchor_pos.x = x;
   viewport->anchor_pos.y = y;
   viewport->anchor_pos.width = width;
   viewport->anchor_pos.height = height;

   DBG_OBJ_SET_NUM (viewport, "anchor_pos.hpos", viewport->anchor_pos.hpos);
   DBG_OBJ_SET_NUM (viewport, "anchor_pos.vpos", viewport->anchor_pos.vpos);
   DBG_OBJ_SET_NUM (viewport, "anchor_pos.x", viewport->anchor_pos.x);
   DBG_OBJ_SET_NUM (viewport, "anchor_pos.y", viewport->anchor_pos.y);
   DBG_OBJ_SET_NUM (viewport, "anchor_pos.width", viewport->anchor_pos.width);
   DBG_OBJ_SET_NUM (viewport, "anchor_pos.height",
                    viewport->anchor_pos.height);

   Dw_viewport_update_anchor_idle (viewport);
}

/*
 * Remove anchor and idle function.
 */
void Dw_viewport_remove_anchor (DwViewport *viewport)
{
   if (viewport->anchor) {
      g_free (viewport->anchor);
      viewport->anchor = NULL;
   }
}


static gint Dw_viewport_draw_resize_idle (gpointer data)
{
   DwViewport *viewport;
   DwRectangle viewport_area, world_area;
   RECT redraw_area;
   int i;

   viewport = DW_VIEWPORT (data);

   switch (viewport->draw_resize_action) {
   case DW_VIEWPORT_DRAW:
   for (i = 0; i < viewport->num_draw_areas; i++) {

      viewport_area.x = viewport->world_x;
      viewport_area.y = viewport->world_y;
      viewport_area.width = viewport->allocation.width;
      viewport_area.height = viewport->allocation.height;

      if (p_Dw_rectangle_intersect (&viewport->draw_areas[i],
                                    &viewport_area, &world_area)) {
         redraw_area.left = world_area.x - viewport_area.x;
         redraw_area.top = world_area.y - viewport_area.y;
         redraw_area.right = redraw_area.left + world_area.width;
         redraw_area.bottom = redraw_area.top + world_area.height;
         InvalidateRect (viewport->hwnd, &redraw_area, TRUE);
      }
   }

      /* No more areas to be drawn. */
      viewport->num_draw_areas = 0;
      break;

   case DW_VIEWPORT_RESIZE:
      Dw_viewport_calc_size (viewport);
      viewport->draw_resize_action = DW_VIEWPORT_DRAW;
      break;
   }

   viewport->draw_resize_idle_id = 0;
   return FALSE;
}


/*
 * Drawing and resizing is done in this idle function.
 */
gint a_Dw_viewport_draw_resize (DwViewport *viewport)
{
   DwRectangle viewport_area, world_area;
   RECT redraw_area;
   int i;

   for (i = 0; i < viewport->num_draw_areas; i++) {

      viewport_area.x = viewport->world_x;
      viewport_area.y = viewport->world_y;
      viewport_area.width = viewport->allocation.width;
      viewport_area.height = viewport->allocation.height;

      if (p_Dw_rectangle_intersect (&viewport->draw_areas[i],
                                    &viewport_area, &world_area)) {
         redraw_area.left = world_area.x - viewport_area.x;
         redraw_area.top = world_area.y - viewport_area.y;
         redraw_area.right = redraw_area.left + world_area.width;
         redraw_area.bottom = redraw_area.top + world_area.height;
         InvalidateRect (viewport->hwnd, &redraw_area, FALSE);
      }
   }

   /* No more areas to be drawn. */
   viewport->num_draw_areas = 0;

   if (viewport->draw_resize_action == DW_VIEWPORT_RESIZE) {
      Dw_viewport_calc_size (viewport);
      viewport->draw_resize_action = DW_VIEWPORT_DRAW;
   }

   return 0;
}

/*
 * Queue an area for drawing. This function is called by
 * p_Dw_widget_queue_draw_area. x and y are passed in world coordinates.
 */
void Dw_viewport_queue_draw (DwViewport *viewport,
                                 gint32 x,
                                 gint32 y,
                                 gint32 width,
                                 gint32 height)
{
    DwRectangle area;
    int i;

	if (!viewport || width <= 0 || height <= 0) {
		return;
	}

   if (viewport->draw_resize_idle_id == 0) {
      viewport->draw_resize_action = DW_VIEWPORT_DRAW;
      viewport->draw_resize_idle_id =
         g_idle_add_full (G_PRIORITY_HIGH_IDLE,
			 Dw_viewport_draw_resize_idle, (gpointer)viewport, NULL);
   } else if (viewport->draw_resize_action == DW_VIEWPORT_RESIZE)
      /* Drawing is always overridden by resizing. */
      return;

    area.x = x;
    area.y = y;
    area.width = width;
    area.height = height;

   /* First, try to keep the list as clean as possible. Check whether other
    * rectangles interfer with this one in some way. */
   /* An idea for optimization: The list could be sorted, and so the part of
    * the list we have to consider here, may be reduced, the start may be
    * found via linear search. However, this probably makes balanced binary
    * trees necessary, since moving elements within the array may be quite
    * time-consuming.
    */

   for (i = 0; i < viewport->num_draw_areas; i++) {
      if (p_Dw_rectangle_is_subset (&area, &viewport->draw_areas[i]))
         /* First case: area is a subset of an already queued rectangle
          * -> nothing to do. */
         return;
      else if (p_Dw_rectangle_is_subset (&viewport->draw_areas[i], &area)) {
         /* Second case: area is a subset of an already queued rectangle
          * -> replace the other one with area. */
         viewport->draw_areas[i] = area;
         return;
      }
      /* Maybe some more tests: if both areas may exactly be combined to a
       * rectangle? Very unlikely case ... */
   }

   /* No interference: add  the new area to the list. */
   viewport->num_draw_areas++;
   a_List_add (viewport->draw_areas, viewport->num_draw_areas,
               viewport->num_draw_areas_max);
   viewport->draw_areas[viewport->num_draw_areas - 1] = area;

}

/*
 * Start the resizing idle. This function is called by
 * p_Dw_widget_queue_resize, after the appropriate attributes have been set in
 * the widgets, where necessary.
 */
void Dw_viewport_queue_resize (DwViewport *viewport)
{

   /* Resizing always overrides drawing. */
   viewport->draw_resize_action = DW_VIEWPORT_RESIZE;
   viewport->num_draw_areas = 0;

   if (viewport->draw_resize_idle_id == 0)
      viewport->draw_resize_idle_id =
         g_idle_add_full (G_PRIORITY_HIGH_IDLE,
			 Dw_viewport_draw_resize_idle, (gpointer)viewport, NULL);
}

/*
 * Return the DwWidget which is at position (vx, vy) in viewport coordinates.
 */
DwWidget* a_Dw_viewport_widget_at_viewport_point (DwViewport *viewport,
                                                      gint32 vx,
                                                      gint32 vy)
{
   gint32 world_x, world_y;

   if (viewport->child) {
      world_x = vx + viewport->world_x;
      world_y = vy + viewport->world_y;
      return Dw_viewport_widget_at_point (viewport, world_x, world_y);
   } else
      return NULL;   
}

