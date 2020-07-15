/* 
 * Copyright (C) 2005-2006 Feynman Software
 *
 * This program is g_free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#ifndef __COOKIES_H__
#define __COOKIES_H__

#include "mgdconfig.h"
#ifndef ENABLE_COOKIES
# define a_Cookies_get(url)  g_strdup("")
# define a_Cookies_init()    ;
# define a_Cookies_freeall() ;
#else
  char *a_Cookies_get(const mSpiderUrl *request_url);
  void  a_Cookies_set(GList *cookie_string, const mSpiderUrl *set_url);
  void  a_Cookies_init( void );
  void  a_Cookies_freeall( void );
#endif
time_t a_Cookies_create_timestamp(const char *expires);

#endif /* !__COOKIES_H__ */
