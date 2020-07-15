/*
 * File: dw_widget.c
 *
 * Copyright (C) 2005-2006 Feynman Software
 * Copyright (C) 2001-2003  Sebastian Geerken <S.Geerken@ping.de>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include <stdio.h>
#include <string.h>
#include "mgdconfig.h"

#include "msg.h"
#include "dw_widget.h"
#include "dw_marshal.h"
#include "dw_container.h"
#include "dw_viewport.h"
#include "dw_ext_iterator.h"

#define DEBUG_SIZE   0
#define DEBUG_ALLOC  2

#define DEBUG_LEVEL 10
#include "debug.h"
#ifndef _NOUNIX_
#include "js.h"
#endif


static void Dw_widget_init                (DwWidget *widget);
static void Dw_widget_class_init          (DwWidgetClass *klass);

static void Dw_widget_finalize            (GObject *object);

static void Dw_widget_real_destroy        (DwWidget *widget);

static void Dw_widget_real_size_request   (DwWidget *widget,
                                           DwRequisition *requisition);
static void Dw_widget_real_get_extremes   (DwWidget *widget,
                                           DwExtremes *extremes);

static void Dw_widget_update_cursor       (DwWidget *widget);


enum
{
   SIZE_REQUEST,
   SIZE_ALLOCATE,
   SET_WIDTH,
   SET_ASCENT,
   SET_DESCENT,
   DRAW,
   REALIZE,
   UNREALIZE,
   DESTROY,
   BUTTON_PRESS_EVENT,
   BUTTON_RELEASE_EVENT,
   MOTION_NOTIFY_EVENT,
   ENTER_NOTIFY_EVENT,
   LEAVE_NOTIFY_EVENT,
   LAST_SIGNAL
};


static GObjectClass *parent_class;
static guint widget_signals[LAST_SIGNAL] = { 0 };

/*
 * Standard GObject function
 */
GType a_Dw_widget_get_type (void)
{
   static GType type = 0;

   if (!type) {
      static const GTypeInfo info = {
         sizeof (DwWidgetClass),
         (GBaseInitFunc) NULL,
         (GBaseFinalizeFunc) NULL,
         (GClassInitFunc) Dw_widget_class_init,
         (GClassFinalizeFunc) NULL,
         (gconstpointer) NULL,
         sizeof (DwWidget),
         0,
         (GInstanceInitFunc) Dw_widget_init
      };

      type = g_type_register_static (G_TYPE_OBJECT, "DwWidget", &info, 0);
   }

   return type;
}


/*
 * Standard GObject function
 */
static void Dw_widget_init (DwWidget *widget)
{
   widget->flags = DW_NEEDS_RESIZE | DW_EXTREMES_CHANGED | DW_HAS_CONTENT;
   widget->parent = NULL;
   widget->viewport = NULL;

   widget->allocation.x = -1;
   widget->allocation.y = -1;
   widget->allocation.width = 1;
   widget->allocation.ascent = 1;
   widget->allocation.descent = 0;

   widget->cursor = 0;
   widget->style = NULL;
   widget->bg_color = NULL;
   widget->background_image = NULL;
   widget->button_sensitive = TRUE;
   widget->button_sensitive_set = FALSE;
   widget->add_data = 0;

#ifdef JS_SUPPORT
	widget->jsobj = NULL;
#endif
}


/*
 * Standard GObject function
 */
static void Dw_widget_class_init (DwWidgetClass *klass)
{
   GObjectClass *object_class;

   parent_class = g_type_class_peek_parent (klass);

   object_class = G_OBJECT_CLASS (klass);

   widget_signals[SIZE_REQUEST] =
      g_signal_new ("size_request",
                      G_OBJECT_CLASS_TYPE (klass),
                      G_SIGNAL_RUN_FIRST,
                      G_STRUCT_OFFSET (DwWidgetClass, size_request),
                      NULL, NULL,
                      g_cclosure_marshal_VOID__POINTER,
                      G_TYPE_NONE,
                      1, G_TYPE_POINTER);
   widget_signals[SIZE_ALLOCATE] =
      g_signal_new ("size_allocate",
                      G_OBJECT_CLASS_TYPE (klass),
                      G_SIGNAL_RUN_FIRST,
                      G_STRUCT_OFFSET (DwWidgetClass, size_allocate),
                      NULL, NULL,
                      g_cclosure_marshal_VOID__POINTER,
                      G_TYPE_NONE,
                      1, G_TYPE_POINTER);
   widget_signals[SET_WIDTH] =
      g_signal_new ("set_width",
                      G_OBJECT_CLASS_TYPE (klass),
                      G_SIGNAL_RUN_FIRST,
                      G_STRUCT_OFFSET (DwWidgetClass, set_width),
                      NULL, NULL,
                      g_cclosure_marshal_VOID__INT,
                      G_TYPE_NONE,
                      1, G_TYPE_UINT);
   widget_signals[SET_ASCENT] =
      g_signal_new ("set_ascent",
                      G_OBJECT_CLASS_TYPE (klass),
                      G_SIGNAL_RUN_FIRST,
                      G_STRUCT_OFFSET (DwWidgetClass, set_ascent),
                      NULL, NULL,
                      g_cclosure_marshal_VOID__INT,
                      G_TYPE_NONE,
                      1, G_TYPE_UINT);
   widget_signals[SET_DESCENT] =
      g_signal_new ("set_descent",
                      G_OBJECT_CLASS_TYPE (klass),
                      G_SIGNAL_RUN_FIRST,
                      G_STRUCT_OFFSET (DwWidgetClass, set_descent),
                      NULL, NULL,
                      g_cclosure_marshal_VOID__INT,
                      G_TYPE_NONE,
                      1, G_TYPE_UINT);
   widget_signals[DRAW] =
      g_signal_new ("draw",
                      G_OBJECT_CLASS_TYPE (klass),
                      G_SIGNAL_RUN_FIRST,
                      G_STRUCT_OFFSET (DwWidgetClass, draw),
                      NULL, NULL,
                      g_cclosure_marshal_VOID__UINT_POINTER,
                      G_TYPE_NONE,
                      2, G_TYPE_UINT, G_TYPE_POINTER);
   widget_signals[REALIZE] =
      g_signal_new ("realize",
                      G_OBJECT_CLASS_TYPE (klass),
                      G_SIGNAL_RUN_FIRST,
                      G_STRUCT_OFFSET (DwWidgetClass, realize),
                      NULL, NULL,
                      g_cclosure_marshal_VOID__VOID,
                      G_TYPE_NONE, 0);
   widget_signals[UNREALIZE] =
      g_signal_new ("unrealize",
                      G_OBJECT_CLASS_TYPE (klass),
                      G_SIGNAL_RUN_FIRST,
                      G_STRUCT_OFFSET (DwWidgetClass, unrealize),
                      NULL, NULL,
                      g_cclosure_marshal_VOID__VOID,
                      G_TYPE_NONE, 0);
  widget_signals[DESTROY] =
     g_signal_new ("destroy",
                      G_OBJECT_CLASS_TYPE (klass),
                      G_SIGNAL_RUN_CLEANUP | G_SIGNAL_NO_RECURSE | G_SIGNAL_NO_HOOKS,
                      G_STRUCT_OFFSET (DwWidgetClass, destroy),
                      NULL, NULL,
                      g_cclosure_marshal_VOID__VOID,
                      G_TYPE_NONE, 0);
  widget_signals[BUTTON_PRESS_EVENT] =
     g_signal_new ("button_press_event",
                     G_OBJECT_CLASS_TYPE (klass),
                     G_SIGNAL_RUN_LAST,
                     G_STRUCT_OFFSET (DwWidgetClass, button_press_event),
                     NULL, NULL,
                     p_Dw_marshal_BOOL__INT_INT_UINT,
                     G_TYPE_BOOLEAN,
                     3, G_TYPE_INT, G_TYPE_INT, G_TYPE_UINT);
  widget_signals[BUTTON_RELEASE_EVENT] =
     g_signal_new ("button_release_event",
                     G_OBJECT_CLASS_TYPE (klass),
                     G_SIGNAL_RUN_LAST,
                     G_STRUCT_OFFSET (DwWidgetClass, button_release_event),
                      NULL, NULL,
                     p_Dw_marshal_BOOL__INT_INT_UINT,
                     G_TYPE_BOOLEAN,
                     3, G_TYPE_INT, G_TYPE_INT, G_TYPE_UINT);
  widget_signals[MOTION_NOTIFY_EVENT] =
     g_signal_new ("motion_notify_event",
                      G_OBJECT_CLASS_TYPE (klass),
                      G_SIGNAL_RUN_LAST,
                      G_STRUCT_OFFSET (DwWidgetClass, motion_notify_event),
                      NULL, NULL,
                      p_Dw_marshal_BOOL__INT_INT_UINT,
                      G_TYPE_BOOLEAN,
                      3, G_TYPE_INT, G_TYPE_INT, G_TYPE_UINT);
  widget_signals[ENTER_NOTIFY_EVENT] =
     g_signal_new ("enter_notify_event",
                     G_OBJECT_CLASS_TYPE (klass),
                     G_SIGNAL_RUN_LAST,
                     G_STRUCT_OFFSET (DwWidgetClass, enter_notify_event),
                      NULL, NULL,
                     p_Dw_marshal_BOOL__POINTER_POINTER,
                     G_TYPE_BOOLEAN,
                     2, G_TYPE_POINTER, G_TYPE_POINTER);
  widget_signals[LEAVE_NOTIFY_EVENT] =
     g_signal_new ("leave_notify_event",
                     G_OBJECT_CLASS_TYPE (klass),
                     G_SIGNAL_RUN_LAST,
                     G_STRUCT_OFFSET (DwWidgetClass, leave_notify_event),
                      NULL, NULL,
                     p_Dw_marshal_BOOL__POINTER_POINTER,
                     G_TYPE_BOOLEAN,
                     2, G_TYPE_POINTER, G_TYPE_POINTER);

   object_class->finalize = Dw_widget_finalize;

   klass->size_request = Dw_widget_real_size_request;
   klass->get_extremes = Dw_widget_real_get_extremes;
   klass->size_allocate = NULL;
   klass->mark_size_change = NULL;
   klass->mark_extremes_change = NULL;
   klass->set_width = NULL;
   klass->set_ascent = NULL;
   klass->set_descent = NULL;
   klass->draw = NULL;
   klass->realize = NULL;
   klass->unrealize = NULL;
   klass->destroy = Dw_widget_real_destroy;
   klass->button_press_event = NULL;
   klass->button_release_event = NULL;
   klass->motion_notify_event = NULL;
   klass->enter_notify_event = NULL;
   klass->leave_notify_event = NULL;
   klass->iterator = NULL;
}



/*
 * Standard GObject function
 */
static void Dw_widget_finalize (GObject *object)
{
    DwWidget *widget;

    widget = DW_WIDGET (object);

    g_return_if_fail (widget != NULL);
    g_return_if_fail (DW_IS_WIDGET (widget));

    /* The widget the pointer is in? */
    if (widget->viewport != NULL &&
        widget == DW_VIEWPORT(widget->viewport)->last_entered)
        /* todo: perhaps call the leave_notify function? */
        DW_VIEWPORT(widget->viewport)->last_entered = NULL;

    if (widget->style)
        a_Dw_style_unref (widget->style);

    if (widget->parent)
        Dw_container_remove (DW_CONTAINER (widget->parent), widget);
    else
        Dw_viewport_remove_dw (DW_VIEWPORT (widget->viewport));

    parent_class->finalize (object);
}

static void Dw_widget_real_destroy (DwWidget *widget)
{
    _MSG ("Dw_widget_real_destroy called for `%s'\n",
              g_type_name (G_OBJECT_TYPE (widget)));

    a_Dw_widget_unrealize (widget);

    g_signal_handlers_destroy (G_OBJECT (widget));

    g_object_unref (G_OBJECT (widget));
}

/*
 * Standard Dw function
 */
static void Dw_widget_real_size_request (DwWidget *widget,
                                         DwRequisition *requisition)
{
   g_warning ("DwWidget::size_request not implemented for `%s'",
              g_type_name (G_OBJECT_TYPE (widget)));

   /* return random size to prevent crashes*/
   requisition->width = 50;
   requisition->ascent = 50;
   requisition->descent = 50;
}


/*
 * Standard Dw function
 */
static void Dw_widget_real_get_extremes (DwWidget *widget,
                                         DwExtremes *extremes)
{
   /* Simply return the requisition width */
   DwRequisition requisition;

   a_Dw_widget_size_request (widget, &requisition);
   extremes->min_width = extremes->max_width = requisition.width;
}


/*
 * This function is a wrapper for DwWidget::size_request; it calls
 * this method only when needed.
 */
void a_Dw_widget_size_request (DwWidget *widget,
                               DwRequisition *requisition)
{
   if (DW_WIDGET_NEEDS_RESIZE (widget)) {
      /* todo: check requisition == &(widget->requisition) and do what? */

      g_signal_emit (G_OBJECT (widget), widget_signals[SIZE_REQUEST], 0,
                       requisition);
      widget->requisition = *requisition;
      DW_WIDGET_UNSET_FLAGS (widget, DW_NEEDS_RESIZE);

      DBG_OBJ_SET_NUM (widget, "requisition.width", widget->requisition.width);
      DBG_OBJ_SET_NUM (widget, "requisition.ascent",
                       widget->requisition.ascent);
      DBG_OBJ_SET_NUM (widget, "requisition.descent",
                       widget->requisition.descent);
   } else
      *requisition = widget->requisition;
}

/*
 * Wrapper for DwWidget::get_extremes.
 */
void a_Dw_widget_get_extremes (DwWidget *widget,
                               DwExtremes *extremes)
{
   DwWidgetClass *klass;

   if (DW_WIDGET_EXTREMES_CHANGED (widget)) {
      klass =  DW_WIDGET_GET_CLASS (widget);
      (* (klass->get_extremes)) (widget, extremes);
      widget->extremes = *extremes;
      DW_WIDGET_UNSET_FLAGS (widget, DW_EXTREMES_CHANGED);

      DBG_OBJ_SET_NUM (widget, "extremes.min_width",
                       widget->extremes.min_width);
      DBG_OBJ_SET_NUM (widget, "extremes.max_width",
                       widget->extremes.max_width);
   } else
      *extremes = widget->extremes;
}


/*
 * Wrapper for DwWidget::size_allocate, only called when needed.
 */
void a_Dw_widget_size_allocate  (DwWidget *widget,
                                 DwAllocation *allocation)
{
   if (DW_WIDGET_NEEDS_ALLOCATE (widget) ||
       allocation->x != widget->allocation.x ||
       allocation->y != widget->allocation.y ||
       allocation->width != widget->allocation.width ||
       allocation->ascent != widget->allocation.ascent ||
       allocation->descent != widget->allocation.descent) {

      DEBUG_MSG (DEBUG_ALLOC,
                 "a %stop-level %s with parent_ref = %d is newly allocated "
                 "from %d, %d, %d x %d x %d ...\n",
                 widget->parent ? "non-" : "",
                 g_type_name (G_OBJECT_TYPE (widget)), widget->parent_ref,
                 widget->allocation.x, widget->allocation.y,
                 widget->allocation.width, widget->allocation.ascent,
                 widget->allocation.descent);

      g_signal_emit (G_OBJECT (widget), widget_signals[SIZE_ALLOCATE], 0,
                       allocation);

      DEBUG_MSG (DEBUG_ALLOC, "... to %d, %d, %d x %d x %d\n",
                 widget->allocation.x, widget->allocation.y,
                 widget->allocation.width, widget->allocation.ascent,
                 widget->allocation.descent);

      widget->allocation = *allocation;
      DW_WIDGET_UNSET_FLAGS (widget, DW_NEEDS_ALLOCATE);

      DBG_OBJ_SET_NUM (widget, "allocation.x", widget->allocation.x);
      DBG_OBJ_SET_NUM (widget, "allocation.y", widget->allocation.y);
      DBG_OBJ_SET_NUM (widget, "allocation.width", widget->allocation.width);
      DBG_OBJ_SET_NUM (widget, "allocation.ascent", widget->allocation.ascent);
      DBG_OBJ_SET_NUM (widget, "allocation.descent",
                       widget->allocation.descent);

   }

   /*DW_WIDGET_UNSET_FLAGS (widget, DW_NEEDS_RESIZE);*/
}


void a_Dw_widget_set_width (DwWidget *widget,
                            gint32 width)
{
   g_signal_emit (G_OBJECT (widget), widget_signals[SET_WIDTH], 0, width);
}


void a_Dw_widget_set_ascent (DwWidget *widget,
                             gint32 ascent)
{
   g_signal_emit (G_OBJECT (widget), widget_signals[SET_ASCENT], 0, ascent);
}


void a_Dw_widget_set_descent (DwWidget *widget,
                              gint32 descent)
{
   g_signal_emit (G_OBJECT (widget), widget_signals[SET_DESCENT], 0, descent);
}

typedef struct _DrawInfo
{
    HDC hdc;
    DwRectangle *area;
} DrawInfo;

void a_Dw_widget_draw (DwWidget *widget,
                       HDC hdc,
                       DwRectangle *area)
{
    g_signal_emit (G_OBJECT (widget), widget_signals[DRAW], 0, hdc, area);
}

void a_Dw_widget_realize (DwWidget *widget)
{
   if (!DW_WIDGET_REALIZED (widget)) {
       g_signal_emit (G_OBJECT (widget), widget_signals[REALIZE], 0);
       DW_WIDGET_SET_FLAGS (widget, DW_REALIZED);

       if (DW_IS_CONTAINER (widget))
           a_Dw_container_forall (DW_CONTAINER (widget),
                   (DwCallback) a_Dw_widget_realize,
                   NULL);

       Dw_widget_update_cursor (widget);
       HDC hdc;
       if (widget->viewport)
           hdc =  GetClientDC(widget->viewport->hwnd); 

       if (widget->parent == NULL && widget->style->background_color != NULL) {
           SetBrushColor (hdc, widget->style->background_color->pixel);
           FillBox ( hdc, widget->allocation.x, widget->allocation.y, 
                   widget->allocation.width, widget->allocation.ascent + widget->allocation.descent);
           /*SetWindowBkColor (widget->viewport->hwnd, 
             widget->style->background_color->pixel);*/
       } else {
           if (widget->parent->style != NULL && widget->parent->style->background_color != NULL) {
               SetBrushColor ( hdc, widget->parent->style->background_color->pixel);
               FillBox ( hdc, widget->allocation.x, widget->allocation.y, 
                       widget->allocation.width, widget->allocation.ascent + widget->allocation.descent);
               /*SetWindowBkColor (widget->viewport->hwnd, 
                 widget->parent->style->background_color->pixel);*/
           } else {
               SetBrushColor ( hdc, PIXEL_lightwhite);
               FillBox ( hdc, widget->allocation.x, widget->allocation.y, 
                       widget->allocation.width, widget->allocation.ascent + widget->allocation.descent);
               // SetWindowBkColor (widget->viewport->hwnd, PIXEL_lightwhite);
           }
       }
       if (widget->viewport)
           ReleaseDC( hdc );
   }
}


void a_Dw_widget_unrealize (DwWidget *widget)
{
   if (DW_WIDGET_REALIZED (widget)) {
      a_Dw_widget_set_cursor (widget, 0);
      g_signal_emit (G_OBJECT (widget), widget_signals[UNREALIZE], 0);
      DW_WIDGET_UNSET_FLAGS (widget, DW_REALIZED);

      if (DW_IS_CONTAINER (widget))
         a_Dw_container_forall (DW_CONTAINER (widget),
                                (DwCallback) a_Dw_widget_unrealize,
                                NULL);
   }
}


/*
 * Handles a mouse event.
 *
 * This function is called by Dw_viewport_mouse_event, the type of
 * the event is determined by event->type. x and y are world coordinates.
 * widget may be NULL (if the pointer is outside the top-level widget).
 *
 * When event is NULL, GDK_MOTION_NOTIFY is used as the type. This will
 * soon be the case when GDK_MOTION_NOTIFY events are simulated as a
 * result of viewport changes (bug #94)
 */
gboolean Dw_widget_mouse_event (DwWidget *widget,
                            void* dw_viewport,
                            gint32 x,
                            gint32 y,
                            DWORD flags)
{
   gint signal_no;
   gboolean return_val;
   DwWidgetClass *klass;
   DwViewport *viewport = (DwViewport*) dw_viewport;
   DwWidget *ancestor, *w1, *w2, **track;
   gint track_len, i;
   guint message = HIWORD (flags);

#if 0
   DEBUG_MSG(DEBUG_EVENT,
             "------------------------- EVENT -------------------------\n");
#endif

   /* simulate crossing events */
   /* todo: resizing/moving widgets */
   if (widget != viewport->last_entered) {
#if 0
      DEBUG_MSG (DEBUG_EVENT, "----------> crossing event\n");
      DEBUG_MSG (DEBUG_EVENT, "  last = %p, now = %p\n",
                 viewport->last_entered, widget);
#endif

      /* Determine the next common ancestor of the widgets. */
      if (viewport->last_entered == NULL || widget == NULL)
         ancestor = NULL;
      else {
         /* There is probably a faster algorithm. ;-) */
         ancestor = NULL;
         for (w1 = viewport->last_entered; ancestor == NULL && w1 != NULL;
              w1 = w1->parent)
            for (w2 = widget; ancestor == NULL && w2 != NULL; w2 = w2->parent)
               if (w1 == w2)
                  ancestor = w1;
      }

      /* Construct the track:
       * viewport->last_entered => anchestor => (current) widget */

#if 0
      DEBUG_MSG (DEBUG_EVENT, "common ancestor: %s %p\n",
                 (ancestor ?
                  g_type_name (G_OBJECT_TYPE (ancestor)) : "(none)"),
                 ancestor);
#endif

      track_len = 0;
      if (viewport->last_entered)
         for (w1 = viewport->last_entered; w1 != ancestor; w1 = w1->parent)
            track_len++;
      if (ancestor)
         track_len++; /* for the ancestor */
      if (widget)
         for (w1 = widget; w1 != ancestor; w1 = w1->parent)
            track_len++;

      track = g_new (DwWidget*, track_len);
      i = 0;
      if (viewport->last_entered)
         for (w1 = viewport->last_entered; w1 != ancestor; w1 = w1->parent)
            track[i++] = w1;
      if (ancestor)
         track[i++] = ancestor;
      if (widget) {
         i = track_len - 1;
         for (w1 = widget; w1 != ancestor; w1 = w1->parent)
            track[i--] = w1;
      }

      /* Send events to all events on the track */
      /* todo: emit signals */
      for (i = 0; i < track_len; i++) {
         klass = DW_WIDGET_GET_CLASS (track[i]);
         if (i != 0) {
            if (klass->enter_notify_event)
               klass->enter_notify_event (track[i], track[i - 1], flags);
#if 0
            DEBUG_MSG (DEBUG_EVENT, "entering %s %p\n",
                       g_type_name (G_OBJECT_TYPE (track[i])), track[i]);
#endif
         }
         if (i != track_len - 1) {
            if (klass->leave_notify_event)
               klass->leave_notify_event (track[i], track[i + 1], flags);
#if 0
            DEBUG_MSG (DEBUG_EVENT, "leaving %s %p\n",
                       g_type_name (G_OBJECT_TYPE (track[i])), track[i]);
#endif
         }
      }

#if 0
      DEBUG_MSG (DEBUG_EVENT, "<----------\n");
#endif

      g_free (track);
      viewport->last_entered = widget;
      if (widget)
         Dw_widget_update_cursor (widget);
      else
         SetWindowCursor (viewport->hwnd, GetSystemCursor (IDC_ARROW));
   }

   /* other events */
   while (widget) {
      switch (message) {
      case MSG_LBUTTONDOWN:
         if (widget->button_sensitive)
            signal_no = widget_signals[BUTTON_PRESS_EVENT];
         else
            signal_no = -1;
         break;

      case MSG_LBUTTONUP:
         if (widget->button_sensitive)
            signal_no = widget_signals[BUTTON_RELEASE_EVENT];
         else
            signal_no = -1;
         break;

      case MSG_MOUSEMOVE:
         signal_no = widget_signals[MOTION_NOTIFY_EVENT];
         break;

      default:
         signal_no = -1;
         break;
      }

#if 0
      DEBUG_MSG (DEBUG_EVENT, "Sending %s event to %p, a %s.\n",
                 (message == MSG_MOUSEMOVE ? "motion notify" :
                  message == MSG_LBUTTONUP ? "button release" : "button press"),
                 widget, g_type_name (G_OBJECT_TYPE (widget)));
#endif

      if (signal_no != -1) {
         return_val = FALSE;
         g_signal_emit (G_OBJECT (widget), signal_no, 0,
                          x - widget->allocation.x, y - widget->allocation.y,
                          flags, &return_val);
         if (return_val) {
#if 0
            DEBUG_MSG (DEBUG_EVENT, "-> Processed.\n");
#endif
            return TRUE;
         }
      }

      widget = widget->parent;
   }

#if 0
   DEBUG_MSG (DEBUG_EVENT, "-> Not processed at all.\n");
#endif
   return FALSE;
}


/*
 *  Change the style of a widget. The old style is automatically
 *  unreferred, the new is referred. If this call causes the widget to
 *  change its size, Dw_widget_queue_resize is called.
 */
void a_Dw_widget_set_style (DwWidget *widget,
                            DwStyle *style)
{
   gboolean size_changed;

   if (widget->style) {
      a_Dw_style_unref (widget->style);
      size_changed = a_Dw_style_size_diffs (widget->style, style);
   } else
      size_changed = TRUE;

   a_Dw_style_ref (style);
   widget->style = style;
/* background color */
   if (widget->parent == NULL && DW_WIDGET_REALIZED (widget)) {
        if (widget->style->background_color != NULL)
            SetWindowBkColor (widget->viewport->hwnd, 
                      widget->style->background_color->pixel);
        else
            SetWindowBkColor (widget->viewport->hwnd, PIXEL_lightwhite);
   }

   if (DW_WIDGET_REALIZED (widget)) {
      if (size_changed)
         p_Dw_widget_queue_resize (widget, 0, TRUE);
      else
         p_Dw_widget_queue_draw (widget);
   }
}


/*
 * Set the cursor of the viewport.
 * Called from several other functions.
 */
static void Dw_widget_update_cursor (DwWidget *widget)
{
   DwViewport *viewport = widget->viewport;
   DwWidget *cursor_widget;

   if (viewport) {
      /* Search cursor to use, going up from last_entered (not from widget!).
       */
      cursor_widget = viewport->last_entered;
      while (cursor_widget && cursor_widget->cursor == 0)
         cursor_widget = cursor_widget->parent;

      if (cursor_widget)
         SetWindowCursor (viewport->hwnd, cursor_widget->cursor);
      else
         SetWindowCursor (viewport->hwnd, GetSystemCursor (IDC_ARROW));
   }
}

/*
 * Set the cursor for a DwWidget. cursor has to be stored elsewhere, it
 * is not copied (and not destroyed). If cursor is NULL, the cursor of
 * the parent widget is used.
 */
void a_Dw_widget_set_cursor (DwWidget *widget,
                             HCURSOR cursor)
{
   widget->cursor = cursor;
   if (DW_WIDGET_REALIZED (widget))
      Dw_widget_update_cursor (widget);
}


/*
 * If this function is called with button_sensitive == FALSE, the widget will
 * never receive button press/release events, instead they are sent to the
 * parent widgets. This attribute is inherited from the parent, if this
 * function is never called.
 *
 * TODO: A bit hackish, this is only needed for disabling selection
 * within <BUTTON>'s, and so make the button accessible at all.
 * It would be better to handle this problem in a way links are handled,
 * but this case is much more complicated, since a button is more complex
 * than a link.
 *
 * NOTE: This may be replaced by somehow using signals for events. 
 */
void a_Dw_widget_set_button_sensitive (DwWidget *widget,
                                       gboolean button_sensitive)
{
   widget->button_sensitive = button_sensitive;
   widget->button_sensitive_set = TRUE;
}


/*
 * ...
 */
DwWidget *a_Dw_widget_get_toplevel (DwWidget *widget)
{
   while (widget->parent)
      widget = widget->parent;

   return widget;
}

/*
 * Scrolls the viewport, so that the region [x, y, width, height] (widget
 * coordinates) is seen, according to hpos and vpos.
 */
void a_Dw_widget_scroll_to (DwWidget *widget,
                            DwHPosition hpos,
                            DwVPosition vpos,
                            gint32 x,
                            gint32 y,
                            gint32 width,
                            gint32 height)
{
   Dw_viewport_scroll_to ((DwViewport*) widget->viewport,
                              hpos, vpos,
                              x + widget->allocation.x,
                              y + widget->allocation.y,
                              width, height);
}

/*
 * Retreive an iterator pointing before the first content element
 * of the widget.
 */
DwIterator* a_Dw_widget_iterator (DwWidget *widget,
                                  gint mask,
                                  gboolean at_end)
{
   DwWidgetClass *klass =  DW_WIDGET_GET_CLASS (widget);
   if (klass && klass->iterator)
      return klass->iterator(widget, mask, at_end);
   else
      return NULL;
}

/*
 * ...
 */
DwIterator* p_Dw_iterator_clone_std (DwIterator *it)
{
   DwIterator *it2 = g_new (DwIterator, 1);
   *it2 = *it;
   return it2;
}

/*
 * ...
 */
void p_Dw_iterator_free_std (DwIterator *it)
{
   g_free (it);
}

/*
 * ...
 */
void p_Dw_iterator_highlight_std (DwIterator *it,
                                  gint start,
                                  gint end,
                                  DwHighlightLayer layer)
{
   /* do nothing */
}

void p_Dw_iterator_get_allocation_std (DwIterator *it,
                                       gint start,
                                       gint end,
                                       DwAllocation *allocation)
{
   /* for widgets containing nothing that widgets. */
   g_return_if_fail (it->content.type == DW_CONTENT_WIDGET);

   *allocation = it->content.data.widget->allocation;
}

/*
 * Scrolls the viewport, so that the region between it1 and it2 is
 * seen, according to hpos and vpos. The parameters start and end have
 * the same meaning as in DwIterator::get_allocation(); start refers
 * to it1, while end ferers to it2.
 * 
 * If it1 and it2 point to the same location (see code below), only
 * it1 is regarded, and both start and end refer to it.
 */
void a_Dw_iterator_scroll_to (DwIterator *it1,
                              DwIterator *it2,
                              gint start,
                              gint end,
                              DwHPosition hpos,
                              DwVPosition vpos)
{
#if defined (ENABLE_FINDTEXT) || defined (ENABLE_LINKTRAVE)
   DwAllocation alloc1, alloc2, alloc;
   gint32 x1, x2, y1, y2, vp_width, vp_height;
   DwExtIterator *eit1, *eit2, *eit;
   gint cur_start, cur_end, cmp;
   gboolean at_start;

   DBG_MSG (it1->widget->viewport, "scrolling", 0, "a_Dw_iterator_scroll_to");
   DBG_MSG_START (it1->widget->viewport);

   if (it1 == it2 ||
       (it1->widget == it2->widget && a_Dw_iterator_compare (it1, it2) == 0)) {
      a_Dw_iterator_get_allocation (it1, start, end, &alloc);
      Dw_viewport_scroll_to (DW_VIEWPORT (it1->widget->viewport),
                                 hpos, vpos, alloc.x, alloc.y,
                                 alloc.width, alloc.ascent + alloc.descent);
   } else {
      /* First, determine the rectangle all iterators from it1 and it2
       * allocate, i.e. the smallest rectangle containing all allocations of
       * these iterators. */
      eit1 = a_Dw_ext_iterator_new (it1);
      eit2 = a_Dw_ext_iterator_new (it2);

      x1 = DW_INFINITY;
      x2 = - DW_INFINITY;
      y1 = DW_INFINITY;
      y2 = - DW_INFINITY;

      DBG_MSG_START (it1->widget->viewport);
      for (eit = a_Dw_ext_iterator_clone (eit1), at_start = TRUE;
           (cmp = a_Dw_ext_iterator_compare (eit, eit2)) <= 0;
           a_Dw_ext_iterator_next (eit), at_start = FALSE) {
         if (at_start)
            cur_start = start;
         else
            cur_start = 0;

         if (cmp == 0)
            cur_end = end;
         else
            cur_end = DW_INFINITY;

         a_Dw_ext_iterator_get_allocation (eit, cur_start, cur_end, &alloc);
         DBG_MSGF (it1->widget->viewport, "scrolling", 0,
                   "allocation of %s, from %d to %d: [%d, %d, %d x %d x %d]\n",
                   a_Dw_content_html (&eit->content), cur_start, cur_end,
                   alloc.x, alloc.y, alloc.width, alloc.ascent, alloc.descent);
         x1 = MIN (x1, alloc.x);
         x2 = MAX (x2, alloc.x + alloc.width);
         y1 = MIN (y1, alloc.y);
         y2 = MAX (y2, alloc.y + alloc.ascent + alloc.descent);
         DBG_MSGF (it1->widget->viewport, "scrolling", 0,
                   "result is [%d, %d, %d x %d]", x1, y1, x2 - x1, y2 - y1);
      }
      DBG_MSG_END (it1->widget->viewport);

      DBG_MSGF (it1->widget->viewport, "scrolling", 0,
                "1. region is [%d, %d, %d x %d]", x1, y1, x2 - x1, y2 - y1);

      a_Dw_iterator_get_allocation (it1, start, DW_INFINITY, &alloc1);
      a_Dw_iterator_get_allocation (it2, 0, end, &alloc2);

      DBG_MSGF (it1->widget->viewport, "scrolling", 0,
                "alloc1 = [%d, %d, %d x %d x %d]",
                alloc1.x, alloc1.y,
                alloc1.width, alloc1.ascent, alloc1.descent);
      DBG_MSGF (it2->widget->viewport, "scrolling", 0,
                "alloc2 = [%d, %d, %d x %d x %d]",
                alloc2.x, alloc2.y,
                alloc2.width, alloc2.ascent, alloc2.descent);

      if (alloc1.x > alloc2.x) {
         /*
          * This is due to a line break within the region. When the line is
          * longer than the viewport, and the region is actually quite short,
          * the user would not see anything of the region, as in this figure
          * (with region marked as "#"):
          *
          *            +----------+   ,-- alloc1
          *            |          |   V
          *            |          |  ### ###
          *   ### ###  |          |
          *        ^   |          | <-- viewport
          *        |   +----------+
          *        `-- alloc2
          *   |----------------------------|
          *               width
          *
          * Therefor, we the region smaller, so that the region will be
          * displayed like this:
          *
          *                           ,-- alloc1
          *                      +----|-----+
          *                      |    V     |
          *                      |   ### ###|
          *   ### ###            |          |
          *        ^             |          | <-- viewport
          *        `-- alloc2    +----------+
          *                      |----------|
          *                         width
          *
          * todo: Changes in the viewport size, until the idle function is
          * called, are not regarded.
          */

         vp_width = it1->widget->viewport->allocation.width;
         DBG_MSGF (it2->widget->viewport, "scrolling", 0,
                   "vp_width = %d", vp_width);
         if (x2 - x1 > vp_width) {
            x1 = x2 - vp_width;
            x2 = x1 + vp_width;
         }
      }

      if (alloc1.y > alloc2.y) {
         /* This is similar to the case above, e.g. if the region ends in
          * another table column. */
         vp_height = it1->widget->viewport->allocation.height;
         DBG_MSGF (it2->widget->viewport, "scrolling", 0,
                   "vp_height = %d", vp_height);
         if (y2 - y1 > vp_height) {
            y1 = y2 - vp_height;
            y2 = y1 + vp_height;
         }
      }

      DBG_MSGF (it1->widget->viewport, "scrolling", 0,
                "2. region is [%d, %d, %d x %d]",  x1, y1, x2 - x1, y2 - y1);

      Dw_viewport_scroll_to (DW_VIEWPORT (it1->widget->viewport),
                                 hpos, vpos,  x1, y1, x2 - x1, y2 - y1);
   }

   DBG_MSG_END (it1->widget->viewport);
#endif
}

/*
 * For DwIteratorInt.
 */
DwIterator* p_Dw_iterator_clone_std_int (DwIterator *it)
{
   DwIteratorInt *it2 = g_new (DwIteratorInt, 1);
   *it2 = *(DwIteratorInt*)it;
   return (DwIterator*)it2;
}

/*
 * For DwIteratorInt.
 */
gint p_Dw_iterator_compare_std_int (DwIterator *it1,
                                    DwIterator *it2)
{
   DwIteratorInt *ii1 = (DwIteratorInt*)it1;
   DwIteratorInt *ii2 = (DwIteratorInt*)it2;

   g_return_val_if_fail (it1->widget == it2->widget, 0);

   if (ii1->pos == ii2->pos)
      return 0;
   if (ii1->pos < ii2->pos)
      return -1;
   else
      return +1;
}

/*
 * This function returns a descriptive text for a piece of content,
 * useful for debugging.
 * NOTE: This function is not very reliable, and should really only
 * used for non-critical tasks like debugging.
 */
gchar *a_Dw_content_text (DwContent *content)
{
   /* We cycle through several buffers, so that printf should have
    * no problems. */
#define BUF_NUM     5
#define BUF_SIZE 2048
   static gchar buf[BUF_SIZE * BUF_NUM];
   static int cur_buf = 0;
   gchar *ptr;

   ptr = buf + cur_buf * BUF_SIZE;
   cur_buf = (cur_buf + 1) % BUF_NUM;

   switch (content->type) {
   case DW_CONTENT_START:
      sprintf (ptr, "<start>");
      break;
   case DW_CONTENT_END:
      sprintf (ptr, "<end>");
      break;
   case DW_CONTENT_TEXT:
      sprintf (ptr, "\"%s\"", content->data.text);
      break;
   case DW_CONTENT_WIDGET:
      sprintf (ptr, "the %s %p",
               g_type_name (G_OBJECT_TYPE (content->data.widget)),
               content->data.widget);
      break;
   case DW_CONTENT_ANCHOR:
      sprintf (ptr, "#%s", content->data.anchor);
      break;
   case DW_CONTENT_BREAK:
      sprintf (ptr, "<break(%d)>", content->data.break_space);
      break;
   default:
      sprintf (ptr, "<unknown %d>", content->type);
      break;
   }

   return ptr;
}

/*
 * Like a_Dw_content_text, but returns HTML.
 * NOTE: This function is not very reliable, and should really only
 * used for non-critical tasks like debugging.
 */
gchar* a_Dw_content_html (DwContent *content)
{
   /* We cycle through several buffers, so that printf should have
    * no problems. */
#define BUF_NUM     5
#define BUF_SIZE 2048
   static gchar buf[BUF_SIZE * BUF_NUM];
   static int cur_buf = 0;
   gchar *ptr, *ptr1, *ptr2;

   ptr = buf + cur_buf * BUF_SIZE;
   cur_buf = (cur_buf + 1) % BUF_NUM;

   switch (content->type) {
   case DW_CONTENT_START:
      sprintf (ptr, "<i>&lt;start&gt;</i>");
      break;
   case DW_CONTENT_END:
      sprintf (ptr, "<i>&lt;end&gt;</i>");
      break;
   case DW_CONTENT_TEXT:
      *ptr = '"';
      ptr2 = ptr + 1;

      for (ptr1 = content->data.text; *ptr1; ptr1++) {
         switch (*ptr1) {
         case '<':
            strcpy (ptr2, "&lt;");
            ptr2 += 4;
            break;
         case '>':
            strcpy (ptr2, "&gt;");
            ptr2 += 4;
            break;
         case '&':
            strcpy (ptr2, "&amp;");
            ptr2 += 5;
            break;
         default:
            *ptr2 = *ptr1;
            ptr2++;
         }
      }

      ptr2[0] = '"';
      ptr2[1] = 0;
      break;
   case DW_CONTENT_WIDGET:
      sprintf (ptr, "the %s %p",
               g_type_name (G_OBJECT_TYPE (content->data.widget)),
               content->data.widget);
      break;
   case DW_CONTENT_ANCHOR:
      sprintf (ptr, "#%s", content->data.anchor);
      break;
   case DW_CONTENT_BREAK:
      sprintf (ptr, "<i>&lt;break(%d)&gt;</i>", content->data.break_space);
      break;
   default:
      sprintf (ptr, "<i>&lt;unknown %d&gt;</i>", content->type);
      break;
   }

   return ptr;
}

/*
 * This function returns a descriptive text for an iterator, useful
 * for debugging.
 * NOTE: This function is not very reliable, and should really only
 * used for non-critical tasks like debugging.
 */
gchar *a_Dw_iterator_text (DwIterator *it)
{
   /* We cycle through several buffers, so that printf should have
    * no problems. */
#define BUF_NUM     5
#define BUF_SIZE 2048
   static gchar buf[BUF_SIZE * BUF_NUM];
   static int cur_buf = 0;
   gchar *ptr;

   ptr = buf + cur_buf * BUF_SIZE;
   cur_buf = (cur_buf + 1) % BUF_NUM;

   if (it)
      sprintf (ptr, "[%s in the %s %p]",
               a_Dw_content_text (&it->content),
               g_type_name (G_OBJECT_TYPE (it->widget)), it->widget);
   else
      strcpy (ptr, "[NULL]");

   return ptr;
}

/*
 * This function prints the contents of a whole widget tree.
 * NOTE: This function is not very reliable, and should really only
 * used for non-critical tasks like debugging.
 */
static void Dw_widget_print_tree0 (DwWidget *widget, int indent)
{
   DwIterator *it;

   MSPIDER_MSG ("%*sthe %s %p, contains:\n",
        indent, "", g_type_name (G_OBJECT_TYPE (widget)), widget);
   it = a_Dw_widget_iterator (widget, 0xff, FALSE);
   while (a_Dw_iterator_next (it)) {
      if (it->content.type == DW_CONTENT_WIDGET)
         Dw_widget_print_tree0 (it->content.data.widget, indent + 3);
      else
         MSPIDER_MSG ("%*s%s\n", indent + 3, "", a_Dw_content_text (&it->content));
   }
}
   
void a_Dw_widget_print_tree (DwWidget *widget)
{
   MSPIDER_MSG ("--- START OF WIDGET TREE ---\n");
   Dw_widget_print_tree0 (widget, 0);
   MSPIDER_MSG ("--- END OF WIDGET TREE ---\n");
}


/*
 * The following is a standard implementation for iterators containing
 * exactly one piece of text.
 */

DwIterator* p_Dw_widget_text_iterator (DwWidget *widget,
                                       gint32 mask,
                                       gboolean at_end,
                                       gchar *text)
{
   DwIteratorText *it = NULL;

   if (mask & DW_CONTENT_TEXT) {
      it = g_new (DwIteratorText, 1);
      it->it.widget = widget;
      it->it.mask = mask;
      it->it.content.type = (at_end ? DW_CONTENT_END : DW_CONTENT_START);
      it->it.next = p_Dw_iterator_text_next;
      it->it.prev = p_Dw_iterator_text_prev;
      it->it.clone = p_Dw_iterator_text_clone;
      it->it.compare = p_Dw_iterator_text_compare;
      it->it.free = p_Dw_iterator_free_std;
      it->it.highlight = p_Dw_iterator_highlight_std;
      it->it.get_allocation =p_Dw_iterator_text_get_allocation;// p_Dw_iterator_get_allocation_std;
      it->text = text;
   } else
      it = NULL;

   return (DwIterator*)it;
}

gboolean p_Dw_iterator_text_next (DwIterator *it)
{
   if (it->content.type == DW_CONTENT_START) {
      it->content.type = DW_CONTENT_TEXT;
      it->content.data.text = ((DwIteratorText*)it)->text;
      return TRUE;
   } else {
      it->content.type = DW_CONTENT_END;
      return FALSE;
   }
}

gboolean p_Dw_iterator_text_prev (DwIterator *it)
{
   if (it->content.type == DW_CONTENT_END) {
      it->content.type = DW_CONTENT_TEXT;
      it->content.data.text = ((DwIteratorText*)it)->text;
      return TRUE;
   } else {
      it->content.type = DW_CONTENT_START;
      return FALSE;
   }
}

DwIterator* p_Dw_iterator_text_clone (DwIterator *it)
{
   DwIteratorText *it2 = g_new (DwIteratorText, 1);
   *it2 = *(DwIteratorText*)it;
   return (DwIterator*)it2;
}

gint p_Dw_iterator_text_compare (DwIterator *it1,
                                 DwIterator *it2)
{
   if (it1->content.type == it2->content.type)
      return 0;

   switch (it1->content.type) {
   case DW_CONTENT_START:
      return -1;
   case DW_CONTENT_TEXT:
      if (it2->content.type == DW_CONTENT_START)
         return +1;
      else
         return -1;
   case DW_CONTENT_END:
      return +1;
   default:
      return 0;
   }
}


void p_Dw_iterator_text_get_allocation (DwIterator *it,
                                        gint start,
                                        gint end,
                                        DwAllocation *allocation)
{
   /*
    * Return the allocation of the widget. This is a bit incorrect, since
    * start and end are not regarded, but should be correct enough for most
    * purposes.
    */
   *allocation = it->widget->allocation;
}

/*
 * Calculates the intersection of widget->allocation and area, returned in
 * intersection (in widget coordinates!). Typically used by containers when
 * drawing their children. Returns whether intersection is not empty.
 */
gint p_Dw_widget_intersect (DwWidget *widget,
                            DwRectangle *area,
                            DwRectangle *intersection)
{
#if 1
   DwRectangle parent_area, child_area;

   parent_area = *area;
   parent_area.x += widget->parent->allocation.x;
   parent_area.y += widget->parent->allocation.y;

   child_area.x = widget->allocation.x;
   child_area.y = widget->allocation.y;
   child_area.width = widget->allocation.width;
   child_area.height = DW_WIDGET_HEIGHT(widget);

   if (p_Dw_rectangle_intersect (&parent_area, &child_area, intersection)) {
      intersection->x -= widget->allocation.x;
      intersection->y -= widget->allocation.y;
      return TRUE;
   } else
      return FALSE;
#else
   intersection->x = 0;
   intersection->y = 0;
   intersection->width = widget->allocation.width;
   intersection->height = DW_WIDGET_HEIGHT(widget);

   return TRUE;
#endif
}


void p_Dw_widget_set_parent (DwWidget *widget,
                             DwWidget *parent)
{
#if 0
   g_object_ref (G_OBJECT (widget));
   gtk_object_sink (G_OBJECT (widget));
#endif
   widget->parent = parent;
   widget->viewport = parent->viewport;


   if (!widget->button_sensitive_set)
      widget->button_sensitive = parent->button_sensitive;

   DBG_OBJ_ASSOC (widget, parent);

   if (DW_WIDGET_REALIZED (parent))
      a_Dw_widget_realize (widget);
}


/*
 * Converting between coordinates.
 */

gint32 p_Dw_widget_x_viewport_to_world (DwWidget *widget,
                                        gint16 viewport_x)
{
   g_return_val_if_fail (widget && widget->viewport, viewport_x);
   return viewport_x + widget->viewport->world_x;
}


gint32 p_Dw_widget_y_viewport_to_world (DwWidget *widget,
                                        gint16 viewport_y)
{
   g_return_val_if_fail (widget && widget->viewport, viewport_y);
   return viewport_y + widget->viewport->world_y;
}


gint16 p_Dw_widget_x_world_to_viewport (DwWidget *widget,
                                        gint32 world_x)
{
   g_return_val_if_fail (widget && widget->viewport, world_x);

   return world_x - widget->viewport->world_x;
}


gint16 p_Dw_widget_y_world_to_viewport (DwWidget *widget,
                                        gint32 world_y)
{
   g_return_val_if_fail (widget && widget->viewport, world_y);
   return world_y - widget->viewport->world_y;
}


/*
 * Calculate the intersection of (x, y, width, height) (widget
 * coordinates) and the current viewport area. gdk_intersection has
 * (of course) viewport coordinates, the return value is TRUE if the
 * intersection is not empty.
 */
static gboolean Dw_widget_intersect_viewport (DwWidget *widget,
                                              gint32 x,
                                              gint32 y,
                                              gint32 width,
                                              gint32 height,
                                              DwRectangle *gdk_intersection)
{
   DwRectangle widget_area, viewport_area, intersection;

   g_return_val_if_fail (widget && widget->viewport, FALSE);

   widget_area.x = widget->allocation.x + x;
   widget_area.y = widget->allocation.y + y;
   widget_area.width = width;
   widget_area.height = height;

   viewport_area.x = widget->viewport->world_x;
   viewport_area.y = widget->viewport->world_y;
   viewport_area.width = widget->viewport->allocation.width;
   viewport_area.height = widget->viewport->allocation.height;

   if (p_Dw_rectangle_intersect (&widget_area, &viewport_area,
                                 &intersection)) {
      gdk_intersection->x = intersection.x - widget->viewport->world_x;
      gdk_intersection->y = intersection.y - widget->viewport->world_y;
      gdk_intersection->width = intersection.width;
      gdk_intersection->height = intersection.height;
      return TRUE;
   } else
      return FALSE;
}


/*
 * ...
 */
void p_Dw_widget_queue_draw (DwWidget *widget)
{
   p_Dw_widget_queue_draw_area (widget, 0, 0, widget->allocation.width,
                                DW_WIDGET_HEIGHT(widget));
}


/*
 * ...
 */
void p_Dw_widget_queue_draw_area (DwWidget *widget,
                                  gint32 x,
                                  gint32 y,
                                  gint32 width,
                                  gint32 height)
{
   /* todo: maybe only the intersection? */
   Dw_viewport_queue_draw ((DwViewport*) widget->viewport,
                               x + widget->allocation.x,
                               y + widget->allocation.y, width, height);
}


/*
 * Resizing of Widgets.
 * The interface was adopted by Gtk+, but the implementation is far simpler,
 * since Gtk+ handles a lot of cases which are irrelevant to Dw.
 */

/*
 * This function should be called, if the widget changed its size.
 */
void p_Dw_widget_queue_resize (DwWidget *widget,
                               gint ref,
                               gboolean extremes_changed)
{
   DwWidget *widget2, *child;
   DwWidgetClass *klass;


   klass =  DW_WIDGET_GET_CLASS (widget);
   DW_WIDGET_SET_FLAGS (widget, DW_NEEDS_RESIZE);
   if(klass)
   {
   if (klass->mark_size_change)
      klass->mark_size_change (widget, ref);
   }

   if (extremes_changed) {
      DW_WIDGET_SET_FLAGS (widget, DW_EXTREMES_CHANGED);
      if (klass && klass->mark_extremes_change)
         klass->mark_extremes_change (widget, ref);
   }

   for (widget2 = widget->parent, child = widget;
        widget2;
        child = widget2, widget2 = widget2->parent) {
      klass =  DW_WIDGET_GET_CLASS (widget2);
      DW_WIDGET_SET_FLAGS (widget2, DW_NEEDS_RESIZE);
      if (klass->mark_size_change)
         klass->mark_size_change (widget2, child->parent_ref);
      DW_WIDGET_SET_FLAGS (widget2, DW_NEEDS_ALLOCATE);


      if (extremes_changed) {
         DW_WIDGET_SET_FLAGS (widget2, DW_EXTREMES_CHANGED);
         if (klass->mark_extremes_change)
            klass->mark_extremes_change (widget2, child->parent_ref);
      }
   }

   if (widget->viewport) {
      Dw_viewport_queue_resize ((DwViewport*) widget->viewport);
   }
}

/*
 * If a widget might draw outside of its allocation, this function should be
 * called at the beginning of the draw method. It will cause drawings be done
 * in a temporary pixmap, which is later copied into the backing pixmap, so
 * that drawings outside of the allocation are later discarded.
 *
 * Clipping causes a bit overhead due to the second copying, so it should
 * only used when neccessary.
 */
void p_Dw_widget_will_clip (DwWidget *widget)
{
#if 0
   DwWidget *widget2;

   widget->clip_pixmap =
      gdk_pixmap_new (widget->viewport->window,
                      widget->viewport->allocation.width,
                      widget->viewport->allocation.height,
                      GTK_DW_VIEWPORT(widget->viewport)->depth);
   
   /* Determine the effective background color. (There is a defined background
    * at least for the top-level widget.) */
   for (widget2 = widget;
        widget2 != NULL && widget2->style->background_color == NULL;
        widget2 = widget2->parent)
      ;

   g_return_if_fail (widget2 != NULL);

   gdk_draw_rectangle (widget->clip_pixmap,
                       widget2->style->background_color->gc,
                       TRUE, 0, 0,
                       widget->viewport->allocation.width,
                       widget->viewport->allocation.height);
#endif
}


/*
 * Set the background "behind" the widget, if it is not the background of the
 * parent widget, e.g. the background of a table row.
 */
void p_Dw_widget_set_bg_color (DwWidget *widget,
                               DwStyleColor *color)
{
   widget->bg_color = color;
}

/*
 * Get the actual background of a widget.
 */
DwStyleColor* p_Dw_widget_get_bg_color (DwWidget *widget)
{
   while (widget != NULL) {
      if (widget->style->background_color)
         return widget->style->background_color;
      if (widget->bg_color)
         return widget->bg_color;

      widget = widget->parent;
   }

   return NULL;
}

/*
 * Get the actual background image of a widget.
 */
DwStyleBgImage* p_Dw_widget_get_bg_image (DwWidget *widget)
{
   while (widget != NULL) {
      if (widget->style->background_image)
         return widget->style->background_image;
      if (widget->background_image)
         return widget->background_image;
      widget = widget->parent;
   }

   return NULL;
}


/*
 * Draw borders and background of a widget part, which allocation is
 * given by (x, y, width, height) (widget coordinates).
 */
void p_Dw_widget_draw_box (DwWidget *widget,
                           HDC hdc,
                           DwStyle *style,
                           DwRectangle *area,
                           gint32 x,
                           gint32 y,
                           gint32 width,
                           gint32 height,
                           gboolean inverse)
{
   DwRectangle gdk_area;
   gint32 vx, vy;

   if (Dw_widget_intersect_viewport (widget, area->x, area->y,
                                     area->width, area->height, &gdk_area)) {
      vx = p_Dw_widget_x_viewport_to_world (widget, 0);
      vy = p_Dw_widget_y_viewport_to_world (widget, 0);

      p_Dw_style_draw_border (hdc, &gdk_area,
                              vx, vy,
                              widget->allocation.x + x,
                              widget->allocation.y + y,
                              width, height,
                              style, inverse);

      if (style->background_color)
         p_Dw_style_draw_background (hdc, &gdk_area,
                                     vx, vy,
                                     widget->allocation.x + x,
                                     widget->allocation.y + y,
                                     width, height,
                                     style, inverse);
   }
}


/*
 * Draw borders and background of a widget.
 */
void p_Dw_widget_draw_widget_box (DwWidget *widget,
                                  HDC hdc,
                                  DwRectangle *area,
                                  gboolean inverse)
{
   DwRectangle gdk_area;
   gint32 vx, vy;

   if (Dw_widget_intersect_viewport (widget, area->x, area->y,
                                     area->width, area->height, &gdk_area)) {
      vx = p_Dw_widget_x_viewport_to_world (widget, 0);
      vy = p_Dw_widget_y_viewport_to_world (widget, 0);

      p_Dw_style_draw_border (hdc, &gdk_area,
                              vx, vy,
                              widget->allocation.x,
                              widget->allocation.y,
                              widget->allocation.width,
                              DW_WIDGET_HEIGHT(widget),
                              widget->style, inverse);


      /* - Toplevel widget background colors are set as viewport
       *   background color. This is not crucial for the rendering, but
       *   looks a bit nicer when scrolling. Furthermore, the viewport
       *   does anything else in this case.
       *
       * - Since widgets are always drawn from top to bottom, it is
       *   *not* necessary to draw the background if
       *   widget->style->background_color is NULL (shining through).
       */
      if (widget->parent && widget->style->background_color)
         p_Dw_style_draw_background (hdc, &gdk_area,
                                     vx, vy,
                                     widget->allocation.x,
                                     widget->allocation.y,
                                     widget->allocation.width,
                                     DW_WIDGET_HEIGHT(widget),
                                     widget->style, inverse);

      if (widget->style->background_image)
         p_Dw_style_draw_background_image (hdc, &gdk_area,
                                           vx, vy,
                                           widget->allocation.x,
                                           widget->allocation.y,
                                           widget->allocation.width,
                                           DW_WIDGET_HEIGHT(widget),
                                           widget->style, widget);
   }
}

/*
 * This function is used by some widgets, when they are selected (as a whole).
 *
 * todo: This could be accelerated by using clipping bitmaps. Two important
 * issues:
 *
 *     (i) There should always been a pixel in the upper-left corner of the
 *         *widget*, so probably two different clipping bitmaps have to be
 *         used (10/01 and 01/10).
 *
 *    (ii) Should a new GC always be created?
 */
void p_Dw_widget_draw_selected (DwWidget *widget,
                                HDC hdc,
                                DwRectangle *area)
{
   DwRectangle gdk_area;
   /* All coordinates are widget coordinates. */
   gint32 x, y, startxa, startya, startxb, startyb, endx, endy, ix, iy;
   gint32 dx, dy; /* the difference between viewport and widget */
   DwStyleColor *bg_color;

   if (Dw_widget_intersect_viewport (widget, area->x, area->y,
                                     area->width, area->height, &gdk_area)) {
      /* Calculate from where to start the respective drawing loops below.
       * There should always been a pixel in the upper-left corner of the
       * *widget*, so the start depends on whether the drawing area (in widget
       * coordinates) has even or odd offsets. */

      /* the intersection in widget coordinates */
      ix = p_Dw_widget_x_viewport_to_world (widget, gdk_area.x) -
         widget->allocation.x;
      iy = p_Dw_widget_y_viewport_to_world (widget, gdk_area.y) -
         widget->allocation.y;

      if (ix % 2 == 0) {
         startxa = ix;
         startxb = ix + 1;
      } else {
         startxa = ix + 1;
         startxb = ix;
      }

      if (iy % 2 == 0) {
         startya = iy;
         startyb = iy + 1;
      } else {
         startya = iy + 1;
         startyb = iy;
      }

      dx = p_Dw_widget_x_world_to_viewport (widget, widget->allocation.x);
      dy = p_Dw_widget_y_world_to_viewport (widget, widget->allocation.y);
      endx = ix + gdk_area.width;
      endy = iy + gdk_area.height;

      bg_color = p_Dw_widget_get_bg_color (widget);

      for (x = startxa; x < endx; x += 2)
         for (y = startya; y < endy; y += 2) {
            if (bg_color)
                SetPixel (hdc, x + dx, y + dy, RGB2Pixel (hdc, 
                                         bg_color->inverse_color.r, 
                                         bg_color->inverse_color.g, 
                                         bg_color->inverse_color.b));
         }

      for (x = startxb; x < endx; x += 2)
         for (y = startyb; y < endy; y += 2) {
            if (bg_color)
                SetPixel (hdc, x + dx, y + dy, RGB2Pixel (hdc, 
                                         bg_color->inverse_color.r, 
                                         bg_color->inverse_color.g, 
                                         bg_color->inverse_color.b));
         }
   }
}

BOOL
p_Dw_widget_update_area (DwWidget *widget,
                                    const DwAllocation *allocation , BOOL focus)
{
   RECT rc_old, rc_new;
   DwViewport* viewport = widget->viewport;

   if (NULL == viewport)
       return FALSE;

   SetRect (&rc_old, widget->allocation.x, widget->allocation.y,
        widget->allocation.x + widget->allocation.width,
        widget->allocation.y
        + widget->allocation.ascent + widget->allocation.descent);

   SetRect (&rc_new, allocation->x, allocation->y,
        allocation->x + allocation->width,
        allocation->y + allocation->ascent + allocation->descent);

   if (!EqualRect (&rc_old, &rc_new) || focus) {
        OffsetRect (&rc_old, -viewport->world_x, -viewport->world_y);
        OffsetRect (&rc_new, -viewport->world_x, -viewport->world_y);
        InvalidateRect (viewport->hwnd, &rc_old, TRUE);
#if 1
        rc_new.right = viewport->allocation.width;
        rc_new.bottom = viewport->allocation.height;
#endif
        InvalidateRect (viewport->hwnd, &rc_new, TRUE);
        return TRUE;
   }

   return FALSE;
}

