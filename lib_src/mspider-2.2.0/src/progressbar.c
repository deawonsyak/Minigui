/*
 * File: progressbar.c
 *
 * Copyright (C) 2005-2006 Feynman Software
 * Copyright (C) 2004 Jorge Arellano Cid <jcid@mspider.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include <stdio.h>

#include "browser.h"
#include "progressbar.h"


/*
 * The progressbar is basically a GtkFrame with a text label.
 */
void* a_Progressbar_new(void)
{
   printf ("a_Progressbar_new called.\n");
   return NULL;
}

/*
 * Update the specified progress bar.
 *  updatestr : String to display within the bar (NULL is ignored)
 *  sens      : sensitivity (0 = set insensitive, 1 = set sensitive)
 */
void a_Progressbar_update (BrowserWindow* bw, const char *updatestr, gint sens)
{
   if (updatestr && bw->CB_SET_PROGRESS)
        (*(bw->CB_SET_PROGRESS))(bw->main_window, updatestr);
}

