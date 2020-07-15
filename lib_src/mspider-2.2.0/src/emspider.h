/**
 * \file emspider.h
 * \author Wei Yongming <ymwei@minigui.org>
 * \date 2004/05/31
 * 
 * This file includes API for mSpider on MiniGUI.
 *
 \verbatim
    Copyright (C) 2004~2006 Feynman Software.

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

 \endverbatim
 */

/* 
 * $Id: emspider.h,v 1.2 2006-06-09 03:43:11 jpzhang Exp $
 *
 *             mSpider: a embedded HTML browser for MiniGUI.
 *
 *             Copyright (C) 2004 - 2006 Feynman Software.
 */

#ifndef __MGMSPIDER_H__
#define __MGMSPIDER_H__

#include <glib.h>

#include "dw_viewport.h"
#define CTRL_MSPIDER "mspider"
#define CTRL_FRAME "frame" 
#define CTRL_MARQUEE "marquee" 
#define CTRL_FLASH "flashplayer"

#define DIGITAL_LINK_NUMBER 10

#define URL_OK              0
#define URL_ERROR           1
#define URL_INVALID         2
#define URL_NOTSUPPORTED    3

#define MGD_REALIZED          0xFA00

#define MGD_OPENURL           0xFA01

#define MGD_RESETCONTENT      0xFA02

#define MGD_NAV_BACKWARD      0xFA03
#define MGD_NAV_FORWARD       0xFA04
#define MGD_NAV_HOME          0xFA05

#define MGD_RESIZE            0xFA06
#define MSG_BROWSER_GOTOPAGE  0xFA07

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

BOOL emspider_init (void);
void emspider_cleanup (void);

int MGD_open_url_ismap (HWND hwnd, const mSpiderUrl* url_ismap);
int MGD_open_url (HWND hwnd, const char* url_str);
void MGD_scroll_to_x (DwViewport *viewport, int x);
void MGD_scroll_to_y (DwViewport *viewport, int y);

#define ERR_DNSSOLVE       1
#define ERR_NETCONNECT     2

void page_error(mSpiderDoc *dd, int errop);


#define PAGE_MSG(web, Op) \
   (a_Web_valid(web) && ((web)->flags & WEB_RootUrl)) ? \
   page_error((web)->dd, Op) : 0
#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __MGMSPIDER_H__ */

