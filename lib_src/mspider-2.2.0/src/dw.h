#ifndef __DW_H__
#define __DW_H__

#include <glib.h>
#include <glib-object.h>
#include "mgdconfig.h"
#ifdef _NOUNIX_
#include <common.h>
#include <minigui.h>
#include <gdi.h>
#include <window.h>
#include <control.h>
#else
#include <minigui/common.h>
#include <minigui/minigui.h>
#include <minigui/gdi.h>
#include <minigui/window.h>
#include <minigui/control.h>
#endif
typedef struct _DwRectangle             DwRectangle;

struct _DwRectangle
{
   gint32 x;
   gint32 y;
   gint32 width;
   gint32 height;
};

void     a_Dw_init                (void);
void     a_Dw_freeall             (void);

gboolean p_Dw_rectangle_intersect (DwRectangle *src1,
                                   DwRectangle *src2,
                                   DwRectangle *dest);
gboolean p_Dw_rectangle_is_subset (DwRectangle *rect1,
                                   DwRectangle *rect2);


/* Needed at several points. */
extern HCURSOR Dw_cursor_hand;


#endif /* __DW_H__ */

