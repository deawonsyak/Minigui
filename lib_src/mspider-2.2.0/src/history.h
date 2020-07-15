/* 
 * Copyright (C) 2005-2006 Feynman Software
 *
 * This program is g_free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#ifndef __MSPIDER_HISTORY_H__
#define __MSPIDER_HISTORY_H__

#include <glib.h>

#include "url.h"


#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

int a_History_add_url(mSpiderUrl *url);
int a_History_set_title(gint idx, const gchar *title);
mSpiderUrl *a_History_get_url(gint idx);
const gchar *a_History_get_title(gint idx);
const gchar *a_History_get_title_by_url(mSpiderUrl *url, gint force);
void a_History_free(void);


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __MSPIDER_HISTORY_H__ */
