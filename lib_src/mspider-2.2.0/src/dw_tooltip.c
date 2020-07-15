/* * File: dw_tooltip.c *
 *
 * Copyright (C) 2005-2006 Feynman Software
 *
 * This program is g_free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

/*
 * A few notes:
 *
 *    - Currently, a window is created every time before it is shown, and
 *      destroyed, before it is hidden. This saves (probably?) some
 *      memory, but can simply be changed. An alternative is having a
 *      global window for all tooltips.
 *
 *    - Tooltips are positioned near the pointer, as opposed to Gtk+
 *      tooltips, which are positioned near the widget.
 *
 * Sebastian
 */

#include "dw_tooltip.h"
#include "mgwidget.h"

#define IDC_TOOLTIP 100
/* The amount of space around the text, including the border. */
#define PADDING 4

/* The difference between pointer position and upper left corner of the
 * tooltip. */
#define DIFF 15


extern int mouse_x_position; 
extern int mouse_y_position;

static gboolean Dw_tooltip_draw (DwTooltip *tooltip);

static DwTooltip *Dw_tooltip_new0 (const char *text, gint ref_count)
{
   DwTooltip *tooltip;

   tooltip = g_new (DwTooltip, 1);
   tooltip->ref_count = ref_count;
   tooltip->hwnd_window = HWND_INVALID;
   tooltip->timeout_id = 0;
   tooltip->text = g_strdup (text);
   return tooltip;
}

/*
 * Create a new tooltip, with ref_count set to 1.
 */
DwTooltip* a_Dw_tooltip_new (const gchar *text)
{
   return Dw_tooltip_new0(text, 1);
}

/*
 * Create a new tooltip, with ref_count set to 0. Tyically used for DwStyle.
 */
DwTooltip* a_Dw_tooltip_new_no_ref (const gchar *text)
{
   return Dw_tooltip_new0(text, 0);
}


/*
 * Destroy the tooltip. Used by Dw_tooltip_unref.
 */
void Dw_tooltip_destroy (DwTooltip *tooltip)
{
   a_Dw_tooltip_on_leave (tooltip);
   g_free (tooltip->text);
   g_free (tooltip);
}


/*
 * Call this function if the pointer has entered the widget/word.
 */
void a_Dw_tooltip_on_enter (DwTooltip *tooltip)
{
   a_Dw_tooltip_on_leave (tooltip);
   tooltip->timeout_id = g_timeout_add (500, (GSourceFunc)Dw_tooltip_draw,
                                          tooltip);
}


/*
 * Call this function if the pointer has left the widget/word.
 */
void a_Dw_tooltip_on_leave (DwTooltip *tooltip)
{
   if (tooltip->timeout_id != 0) {
      g_source_remove (tooltip->timeout_id);
      tooltip->timeout_id = 0;
   }

   if (tooltip->hwnd_window!= HWND_INVALID) {
      DestroyWindow(tooltip->hwnd_window);
      tooltip->hwnd_window= HWND_INVALID;
   }
}


/*
 * Call this function if the pointer has moved within the widget/word.
 */
void a_Dw_tooltip_on_motion (DwTooltip *tooltip)
{
   a_Dw_tooltip_on_enter (tooltip);
}

/*
 *  Draw the tooltip. Called as a timeout function.
 */
static gboolean Dw_tooltip_draw (DwTooltip *tooltip)
{
    int width, height;


    Dw_MgWidget_get_text_metrics (tooltip->text, strlen(tooltip->text), &width, &height);
    
    tooltip->hwnd_window = CreateWindow (CTRL_STATIC, tooltip->text, 
                          WS_VISIBLE | SS_CENTER, 
			              IDC_TOOLTIP, mouse_x_position + DIFF, 
                          mouse_y_position + DIFF,
                          width + ADD_LENGTH, height + ADD_LENGTH, GetActiveWindow(), 0);
    return FALSE;
}



