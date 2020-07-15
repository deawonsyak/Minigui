#ifndef __DW_TOOLTIP_H__
#define __DW_TOOLTIP_H__

#include <glib.h>
#include "mgdconfig.h"
#ifdef _NOUNIX_
#include <common.h>
#include <minigui.h>
#include <gdi.h>
#include <window.h>
#include <mgutils/mgutils.h>
#else
#include <minigui/common.h>
#include <minigui/minigui.h>
#include <minigui/gdi.h>
#include <minigui/window.h>
#include <mgutils/mgutils.h>
#endif

typedef struct _DwTooltip DwTooltip;

struct _DwTooltip
{
   HWND hwnd_window;
   gchar *text;
   guint timeout_id;
   gint ref_count;
};

DwTooltip* a_Dw_tooltip_new        (const gchar *text);
DwTooltip* a_Dw_tooltip_new_no_ref (const gchar *text);

void       a_Dw_tooltip_on_enter   (DwTooltip *tooltip);
void       a_Dw_tooltip_on_leave   (DwTooltip *tooltip);
void       a_Dw_tooltip_on_motion  (DwTooltip *tooltip);

#define a_Dw_tooltip_ref(tooltip)   ((tooltip)->ref_count++)
#define a_Dw_tooltip_unref(tooltip) if (--((tooltip)->ref_count) == 0) \
                                       Dw_tooltip_destroy (tooltip)

/* Don't use this function directly! */
void Dw_tooltip_destroy (DwTooltip *tooltip);

#endif /* __DW_TOOLTIP_H__ */
