#ifndef __DW_VIEWPORT_H__
#define __DW_VIEWPORT_H__

#include "dw.h"
#include "dw_style.h"
#include "dw_widget.h"
#include "findtext.h"
#include "linktrave.h"
#include "web.h"
#include "browser.h"
#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#define DW_VIEWPORT(obj)        ((DwViewport*)(obj))

typedef struct _MGAllocation MGAllocation;
typedef struct _DwViewport   DwViewport;

struct _MGAllocation
{
   gint32 x;
   gint32 y;
   gint32 width;
   gint32 height;
};

typedef enum {
   DW_VIEWPORT_DRAW,
   DW_VIEWPORT_RESIZE
} DwViewportDrawResizeAction;

struct _DwViewport
{
   HWND hwnd;

   /* the current allocation: size and position, always relative to the
    * scrolled area! */
   MGAllocation allocation;

   gint32 world_x, world_y;

   DwWidget *child;
   DwWidget *last_entered;
   gboolean hscrollbar_used, vscrollbar_used, calc_size_blocked;

   /* updated by Dw_viewport_motion_notify */
   gdouble mouse_x, mouse_y;
#ifdef ENABLE_FINDTEXT
   FindtextState *findtext_state;
#endif

   gchar *anchor;
   DwRectPosition anchor_pos;

#if 0
   Selection *selection;
#endif

   /* Anchors of the widget tree.
    * Key: gchar*, has to be stored elsewhere
    * Value: an instance of DwViewportAnchor (in .c file) */
   GHashTable *anchors_table;

   /* Queue of draw and resize requests. */
   gint draw_resize_idle_id;
   DwViewportDrawResizeAction draw_resize_action;

   /* What has to be redrawn. DwRectangle's are in world coordinates. */
   DwRectangle *draw_areas;
   gint num_draw_areas;
   gint num_draw_areas_max; /* number allocated */

};

GType        a_Dw_viewport_get_type          (void);

DwViewport*  a_Dw_viewport_new               (HWND hwnd);

void         a_Dw_viewport_destroy           (DwViewport* viewport);

void         a_Dw_viewport_add_dw          (DwViewport *viewport,
                                                  DwWidget *widget);

void         a_Dw_viewport_set_anchor      (DwViewport *viewport,
                                                  const gchar *anchor);
void         a_Dw_viewport_set_scrolling_position (DwViewport
                                                         *viewport,
                                                         gint32 x,
                                                         gint32 y);

DwWidget*    a_Dw_viewport_widget_at_viewport_point (DwViewport *viewport,
                                                           gint32 vx,
                                                           gint32 vy);

gint         a_Dw_viewport_draw_resize (DwViewport *viewport);

gchar*       p_Dw_viewport_add_anchor    (DwWidget *widget,
                                          const gchar *name,
                                           gint32 y);

void         p_Dw_viewport_change_anchor (DwWidget *widget,
                                          gchar *name,
                                          gint32 y);
void         p_Dw_viewport_remove_anchor (DwWidget *widget,
                                          gchar *name);

void         Dw_viewport_remove_dw         (DwViewport *viewport);
void         Dw_viewport_calc_size         (DwViewport *viewport);

DwWidget*    Dw_viewport_widget_at_point (DwViewport *viewport,
                                                gint32 x,
                                                gint32 y);

void         Dw_viewport_update_anchor    (DwViewport *viewport);
void         Dw_viewport_scroll_to        (DwViewport *viewport,
                                                 DwHPosition hpos,
                                                 DwVPosition vpos,
                                                 gint32 x,
                                                 gint32 y,
                                                 gint32 width,
                                                 gint32 height);
void           Dw_viewport_remove_anchor    (DwViewport *viewport);
void           Dw_viewport_queue_draw       (DwViewport *viewport,
                                                 gint32 x,
                                                 gint32 y,
                                                 gint32 width,
                                                 gint32 height);
void           Dw_viewport_queue_resize     (DwViewport *viewport);


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __DW_VIEWPORT_H__ */

