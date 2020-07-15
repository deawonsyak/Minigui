/*
 * File: about.c
 *
 * Copyright (C) 2005-2006 Feynman Software
 * Copyright (C) 1997 Raph Levien <raph@acm.org>
 * Copyright (C) 1999, 2001 Jorge Arellano Cid <jcid@mspider.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include "nav.h"
#include "../web.h"
#include "../msg.h"

#include "io.h"
#include "url_io.h"

typedef struct _SplashInfo SplashInfo_t;

struct _SplashInfo {
   gint FD_Read;
   gint FD_Write;
};


/*
 * HTML text for startup screen
 */
static char *Splash="";
static char *NetErr=
"Content-type: text/html\n"
"\n"
"<!DOCTYPE HTML PUBLIC '-//W3C//DTD HTML 4.01 Transitional//EN'>\n"
"<html>\n"
"<head><title>mSpider error page</title></head>\n"
"<body>\n"
"<h1>Unable to connect to remote host </h1>\n"
"Unable to connect to remote host, please check the network status!</p>\n"
"</p>\n"
"<i>mspider message</i>\n"
"</p>\n"
"</body>\n"
"</html>\n";
static char *DnsErr=
"Content-type: text/html\n"
"\n"
"<!DOCTYPE HTML PUBLIC '-//W3C//DTD HTML 4.01 Transitional//EN'>\n"
"<html>\n"
"<head><title>mSpider error page</title></head>\n"
"<body>\n"
"<h1>Dns can't solve the host address</h1>\n"
"Dns can't solve the host, please check the network status!</p>\n"
"</p>\n"
"<i>mspider message</i>\n"
"</p>\n"
"</body>\n"
"</html>\n";
static char MGSplash [] =
"Content-type: text/html\n"
"\n"
"<!DOCTYPE HTML PUBLIC '-//W3C//DTD HTML 4.01 Transitional//EN'>\n"
"<html>\n"
"<head>\n"
"<title>Splash screen for MiniGUI V1.6.8 or later</title>\n"
"</head>\n"
"<body bgcolor='#778899' text='#000000' link='#000000' vlink='#000000'>\n"
"\n"
"\n"
"<!--                          -->\n"
"<!--   the head of the page   -->\n"
"<!--                          -->\n"
"\n"
"<table width='100%' border='0' cellspacing='1' cellpadding='3'>\n"
" <tr><td>\n"
"   <table border='1' cellspacing='1' cellpadding='0'>\n"
"    <tr>\n"
"    <td bgcolor='#000000'>\n"
"     <table width='100%' border='0' bgcolor='#ffffff'>\n"
"     <tr>\n"
"       <td valign='top' align='left'>\n"
"         <h2>Welcome to the world of MiniGUI!</h2>\n"
"       </td>\n"
"     </tr>\n"
"     </table>\n"
"    </tr>\n"
"   </table>\n"
" </td></tr>\n"
"</table>\n"
"\n"
"<!-- a small horizontal spacer -->\n"
"<br>\n"
"\n"
"\n"
"<table border='0' cellspacing='0' cellpadding='0' width='284' bgcolor='#000000'>\n"
"<tr>\n"
"  <td>\n"
"    <table width='100%' border='0' cellspacing='1' cellpadding='3'>\n"
"    <tr>\n"
"      <td bgcolor='#CCCCCC'>\n"
"        What's MiniGUI\n"
"      </td>\n"
"    </tr>\n"
"    <tr>\n"
"      <td bgcolor='#FFFFFF'>\n"
"<p>\n"
"    MiniGUI is a GPL'd free software project, led by Beijing Feynman Software Co., Ltd.\n"
"    It aims to provide a fast, stable, lightweight, and cross-platform\n"
"    Graphics User Interface support system, which is especially fit for\n"
"    real-time embedded systems based-on Linux/uClinux, eCos, uC/OS-II, and\n"
"    other traditional RTOSes, such as VxWorks.\n"
"</p>\n"
"\n"
"<p>\n"
"    MiniGUI defines a set of light windowing and GDI (Grahpics Device Interface) \n"
"    APIs for applications.  By using them, an application can create multiple \n"
"    windows and/or controls, and can draw in these windows/controls without \n"
"    interfering the others.\n"
"</p>\n"
"\n"
"<p>\n"
"    MiniGUI is composed of three libiraries: libminigui (source is in src/),\n"
"    libmgext (ext/), and libvcongui (vcongui/). Libminigui is the core library,\n"
"    which provides windowing and graphics interfaces as well as standard controls.\n"
"    Libmgext is an extension library of libminigui and provides some\n"
"    useful controls and convenient user interface functions, such as\n"
"    `Open File Dialog Box'. Libvcongui provides a virtual console window\n"
"    in which you can run programs in character mode.\n"
"</p>\n"
"\n"
"<p>\n"
"    You can configure and compile MiniGUI as one of three run-time modes:\n"
"</p>\n"
"    \n"
"<ul>\n"
"    <li>\n"
"      `MiniGUI-Threads': A program running on MiniGUI-Threads can create\n"
"      multiple cascaded windows in different threads, and all the windows\n"
"      belong to a single process. MiniGUI-Threads is fit for some real-time\n"
"      systems on Linux/uClinux, eCos, uC/OS-II, VxWorks, and so on.\n"
"    </li>\n"
"    \n"
"    <li>\n"
"      `MiniGUI-Lite': A program running on MiniGUI-Lite is an independent\n"
"      process, which can also create multiple windows. MiniGUI-Lite is fit for\n"
"      some complex embedded systems, such as PDAs, Thin-Clients or STBs.\n"
"      This mode is useful for full-featured UNIX-like operating systems.\n"
"    </li>\n"
"\n"
"    <li>\n"
"      `MiniGUI-Standalone': A single process version of MiniGUI. This mode\n"
"      is useful for some systems which lack of PThread support, like some \n"
"      uClinux system.\n"
"    </li>\n"
"\n"
"</ul>\n"
"\n"
"<p>\n"
"    Currently, the latest stable release of MiniGUI is version 1.6.1, the \n"
"    latest public release (the freely downloadable release) is version 1.3.3. \n"
"    And the developing version of MiniGUI is version 2.0.x (in the works).  \n"
"</p>\n"
"\n"
"<br>\n"
"<br>\n"
"\n"
"<table cellspacing=0 cellpadding=2 border=0 bgcolor=#C0C0C0>\n"
"    <tr><td>\n"
"<p>\n"
"    <b>NOTE</b>: The edition you downloaded freely from our site may only be used to\n"
"    develop GPL (non-proprietary) Software. The terms of GPL explains\n"
"    what you may or may not do with the free edition.\n"
"</p>\n"
"\n"
"<p>\n"
"    If you are using MiniGUI for developing commercial, proprietary, or other\n"
"    software not covered by the terms listed in GPL, you\n"
"    must have a commercial license for MiniGUI. Please visit\n"
"</p>\n"
"<br>\n"
"<br>\n"
"<code>\n"
"    http://www.fmsoft.cn/product/index.shtml"
"</code>\n"
"<p>"
" for how to obtain this.\n"
"</p>\n"
"\n"
"<p>\n"
"    If you are interested in the commercial MiniGUI licensing, please write\n"
"    to sales@minigui.com. Other mail can be sent to info@minigui.com.\n"
"</p>\n"
"\n"
"<p>\n"
"    More information about our licensing policy for MiniGUI, please visit</p>\n"
"<br>\n"
"<br>\n"
"<code>\n"
"    \n"
"    http://www.fmsoft.cn/product/licensing.shtml.\n"
"</code>\n"
"</p>\n"
"      </td>\n"
"    </tr>\n"
"</table>\n"
"  </td>\n"
"</tr>\n"
"</table>\n"
"</table>\n"
"\n"
"<br>\n"
"\n"
"<table border='0' cellspacing='0' cellpadding='0' width='284' bgcolor='#000000'>\n"
"<tr>\n"
"  <td>\n"
"    <table width='100%' border='0' cellspacing='1' cellpadding='3'>\n"
"    <tr>\n"
"      <td colspan='1' bgcolor='#CCCCCC'>\n"
"        Fetures of MiniGUI V1.6.x\n"
"      </td>\n"
"    </tr>\n"
"    <tr>\n"
"      <td bgcolor='#FFFFFF'>\n"
"<p>\n"
"    MiniGUI version 1.6 offers the following new features:\n"
"</p>\n"
"\n"
"<ol>\n"
"    <li>\n"
"    Provide support for VxWorks and ThreadX operating systems. MiniGUI now\n"
"    can run on Linux/uClinux, eCos, uC/OS-II, VxWorks, ThreadX operating systems\n"
"    and Win32 platform.\n"
"    </li>\n"
"    \n"
"    <li>\n"
"    More controls. Transparent style, ScrollView and ScrollWnd controls,\n"
"    GridView control, IconView control, Animation control, and enhanced \n"
"    SLEdit and MLEdit control.\n"
"    </li>\n"
"\n"
"    <li>\n"
"    Enhanced GDI APIs providing support for advanced 2D graphics functions,\n"
"    lick pen-width, pen-style, and brush-pattern.\n"
"    </li>\n"
"\n"
"    <li>\n"
"    Provides support for more development boards, including TI DM270, TI DM320, \n"
"    Intel DMG, FRV, Agere T8307, ADI BF 533, Sigma Designs RealMagic EM85xx, \n"
"    HH2410, ATMEL9200 and so on.\n"
"    </li>\n"
"\n"
"    <li>\n"
"    Many bugs fixed.\n"
"    </li>\n"
"</ol>\n"
"\n"
"<p><em>\n"
"By now, MiniGUI version 1.6.x is only released through Feynman Software's MiniGUI-VAR product and commercial license.\n"
"</em></p>\n"
"      </td>\n"
"    </tr>\n"
"    </table>\n"
"  </td>\n"
"</tr>\n"
"</table>\n"
"\n"
"<!-- a small horizontal spacer -->\n"
"<br>\n"
"\n"
"\n"
"<!--               -->\n"
"<!--   footnotes   -->\n"
"<!--               -->\n"
"\n"
"<br>\n"
"<center>\n"
"<hr size='2'>\n"
"<address>\n"
"Copyright &copy; 2002~2005, Beijing Feynman Software Technology Co., Ltd.<br>\n"
"http://www.fmsoft.cn\n"
"</address>\n"
"<hr size='2'>\n"
"</center>\n"
"</body>\n"
"</html>\n";



/*
 * Send the splash screen through the IO using a pipe.
 */
static gint About_send_splash(ChainLink *Info, mSpiderUrl *Url, int which)
{
   gint SplashPipe[2];
   IOData_t *io1;
   SplashInfo_t *SpInfo;

   if (pipe(SplashPipe))
      return -1;

   SpInfo = g_new(SplashInfo_t, 1);
   SpInfo->FD_Read  = SplashPipe[0];
   SpInfo->FD_Write = SplashPipe[1];
   Info->LocalKey = SpInfo;

   /* send splash */
   io1 = a_IO_new(IOWrite, SpInfo->FD_Write);
   switch (which) {
     case 1:
        a_IO_set_buf(io1, MGSplash, strlen(MGSplash));
        break;
     case 2:
        a_IO_set_buf(io1, DnsErr, strlen(DnsErr));
        break;
     case 3:
        a_IO_set_buf(io1, NetErr, strlen(NetErr));
        break;
     default:
        a_IO_set_buf(io1, Splash, strlen(Splash));
        break;
   }

   io1->Flags |= (IOFlag_ForceClose + IOFlag_SingleWrite);
   
   a_Chain_link_new(Info, a_About_ccc, BCK, a_IO_ccc, 1, 1);

   a_Chain_bcb(OpStart, Info, io1, NULL);
   a_Chain_bcb(OpSend, Info, io1, NULL);

   /* Tell the cache to receive answer */
   a_Chain_fcb(OpSend, Info, &SpInfo->FD_Read, NULL);
   return SpInfo->FD_Read;
}

/*
 * Push the right URL for each supported "about"
 * ( Data1 = Requested URL; Data2 = Web structure )
 */
static gint About_get(ChainLink *Info, void *Data1, void *Data2)
{
   char *loc;
   const char *tail;
   mSpiderUrl *Url = Data1;
   mSpiderWeb *web = Data2;
   mSpiderUrl *LocUrl;

   /* Don't allow the "about:" method for non-root URLs */
   if (!(web->flags & WEB_RootUrl))
      return -1;

   tail = URL_PATH(Url);

   if (!strcmp(tail, "splash")) {
      return About_send_splash (Info, Url, 0);
   }
   else if (!strcmp(tail, "minigui")) {
      return About_send_splash (Info, Url, 1);
   }
   else if (!strcmp(tail, "dnserr")) {
      return About_send_splash (Info, Url, 2);
   }
   else if (!strcmp(tail, "networkerr")){
      return About_send_splash (Info, Url, 3);
   }
   else if (!strcmp(tail, "blank")){
      return About_send_splash (Info, Url, 0);
   }

   loc = "http://www.minigui.com/";

   LocUrl = a_Url_new(loc, NULL, 0, 0, 0);

   a_Nav_push(web->dd, LocUrl);

   a_Url_free(LocUrl);
   return -1;
}

/*
 * CCC function for the ABOUT module
 */
void a_About_ccc(int Op, int Branch, int Dir, ChainLink *Info,
                 void *Data1, void *Data2)
{
   int FD;

   a_Chain_debug_msg("a_About_ccc", Op, Branch, Dir);

   if ( Branch == 1 ) {
      /* Start about method */
      if (Dir == BCK) {
         switch (Op) {
         case OpStart:
            /* (Data1 = Url;  Data2 = Web) */
            // Info->LocalKey gets set in About_get
            if ((FD = About_get(Info, Data1, Data2)) == -1)
               a_Chain_fcb(OpAbort, Info, NULL, NULL);
            break;
         case OpAbort:
            a_Chain_bcb(OpAbort, Info, NULL, NULL);
            g_free(Info->LocalKey);
            g_free(Info);
            break;
         }
      } else {  /* FWD */
         switch (Op) {
         case OpSend:
            /* This means the sending framework was set OK */
            FD = ((SplashInfo_t *)Info->LocalKey)->FD_Read;
            a_Chain_fcb(OpSend, Info, &FD, NULL);
            break;
         case OpEnd:
            /* Everything sent! */
            a_Chain_del_link(Info, BCK);
            g_free(Info->LocalKey);
            a_Chain_fcb(OpEnd, Info, NULL, NULL);
            break;
         case OpAbort:
            g_free(Info->LocalKey);
            a_Chain_fcb(OpAbort, Info, NULL, NULL);
            break;
         }
      }
   }
}

