/*
 * File: capi.c
 *
 * Copyright (C) 2005-2006 Feynman Software
 * Copyright 2002 Jorge Arellano Cid <jcid@mspider.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

/*
 * Cache API
 * This is the module that manages the cache and starts the CCC chains
 * to get the requests served. Kind of a broker.
 */

#ifndef WIN32
#include <unistd.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "msg.h"
#include "capi.h"
#include "io/url_io.h"
#include "chain.h"
#include "list.h"
#include "interface.h"
#include "history.h"
#include "nav.h"
#include "prefs.h"


#define DEBUG_LEVEL 5
#include "debug.h"
/*
 * Most used function.
 * todo: clean up the ad-hoc bindings with an API that allows dynamic
 *       addition of new plugins.
 */
gint a_Capi_open_url(mSpiderWeb *web, CA_Callback_t Call, void *CbData)
{
   return a_Cache_open_url(web, Call, CbData); 
}

/*
 * Get the cache's buffer for the URL, and its size.
 * Return: 1 cached, 0 not cached.
 */
gint a_Capi_get_buf(const mSpiderUrl *Url, gchar **PBuf, gint *BufSize)
{
   return a_Cache_get_buf(Url, PBuf, BufSize);
}

