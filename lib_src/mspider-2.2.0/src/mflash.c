/////////////////////////////////////////////////////////////
// Flash Plugin and Player
// Copyright (C) 2005-2006 Feynman Software
// Copyright (C) 1998 Olivier Debon
// 
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
// 
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
// 
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
// 
///////////////////////////////////////////////////////////////
//  Author : Olivier Debon  <odebon@club-internet.fr>
//    Altered by Sunny Chu
//  
#include "mgdconfig.h"

#ifdef ENABLE_FLASH
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

#ifndef _NOUNIX_
#include <sys/time.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#else
#include <systime.h>
#include <timers.h>
#include <sys/times.h>
#endif

#include <signal.h>
#include <pthread.h>
#include <semaphore.h>
#include <sys/wait.h>
#include <sched.h>
#include <minigui/common.h>
#include <minigui/minigui.h>
#include <minigui/gdi.h>
#include <minigui/window.h>
#include "mflash.h"
#include "flash.h"
#include "emspider.h"
#include "browser.h"
#define  WIDTH_BORDER    2       
#define  HEIGHT_CAPTION  16
#define ID_TIMER 100
//  #define printf(fmt,args...)
pthread_mutex_t timer_mutex = PTHREAD_MUTEX_INITIALIZER;
//pthread_mutex_t valid_mutex = PTHREAD_MUTEX_INITIALIZER;
extern pthread_mutex_t count_mutex ;

extern int soundsecond;
extern int tt;
extern int tasknum;
extern int nn;
extern int soundId;  
extern int frameId;  
extern int sounddealing;
extern BrowserWindow* current_browser; 
extern int soundplay;//means the sound is playing!

int soundframe=0;//means this is the soundplaying frame!
int soundonly=0;//just allow one flash is displayed
int tasknum;
static int timer_counter = 0;

int tasknum;
struct _MgContext{
    FlashDisplay fd;
    HWND         hWnd;
    PBITMAP	 bmp;	// Graphic buffer
};
typedef struct _MgContext MgContext;
typedef struct _flashflag flashflag;

typedef struct _sched_param sched_param; 
struct _flashflag {
	int 	flag;
	HWND    hWnd;
    MgContext *xc;
    FlashHandle flashHandle; 
    FlashInfo *fi;
    int sound;
};
struct _sched_param 
{
    int sched_priority;
};


GList *flashlist;

static long FlashGraphicInitMg(FlashHandle fh, HWND hWnd,MgContext *xc,int sound)
{
	HDC hdc;
	BITMAP* bmp;
	RECT rc;

    hdc = GetClientDC(current_browser->dd->docwin);
   
	bmp = (BITMAP*)malloc(sizeof(BITMAP));
	memset(bmp, 0, sizeof(bmp));
	bmp->bmBits = NULL;
	GetClientRect(hWnd, &rc);
	GetBitmapFromDC (hdc,  rc.left,  rc.top,  rc.right-rc.left,  rc.bottom-rc.top, bmp);

	xc->fd.pixels = (char*)bmp->bmBits;
    if(xc->fd.pixels==0)
        return 2;
    xc->fd.width = bmp->bmWidth;
	xc->fd.height = bmp->bmHeight;
	xc->fd.bpl = bmp->bmPitch;
	xc->fd.depth = bmp->bmBitsPerPixel;	
	
	xc->hWnd = hWnd;
	xc->bmp = bmp;
	
	ReleaseDC(hdc);
	return FlashGraphicInit(fh, &xc->fd,sound);
}

static void FlashCopyMg(MgContext *xc)
{
	HDC hdc;

	hdc = GetClientDC(xc->hWnd);
	FillBoxWithBitmap(hdc, 0, 0, xc->fd.width, xc->fd.height, xc->bmp);
	ReleaseDC(hdc);
}

/*
 *	This file is the entry of a very simple Flash Player
 */
//static struct FlashInfo fi;

static void showUrl(char *url, char *target, void *client_data)
{

    a_Interface_open_url_string ((gchar*)url, current_browser->dd);

}

static int task2(int *skip)
{
   if(soundId)
     timer_counter++;
      while(timer_counter > 0) 
		{	
			if((soundId > 0) && (soundId < (frameId -4)))
			{
                if(timer_counter>0)
					timer_counter--;

				break;
			}

            if(soundId>frameId)
                *skip=soundId-frameId;
            else
            {
                if((soundId!=-1)&&(timer_counter>0))
                    *skip = timer_counter -1;
                else
                    *skip=0;
            }
               timer_counter =0;
               return 1;
		}	
  return 0;    
}
BOOL PrepareFlashPlay (char *buffer,long size,HWND hWnd)
{
    int status;
    int speed;
    flashflag *item;
    int i;

    timer_counter = 0;
#ifndef _NOUNIX_
    for(i=0;i<10;i++)
    {
        if(sounddealing=0)
            break;
        usleep(50000);
    }
#endif

    if(buffer[3]>5)   //flash play doesn't support flash6.0 and flash7.0
    {
        printf("mSpider just support flash 5.0\n");
        return FALSE;
    }
    else
    { 
        item = g_malloc0(sizeof(flashflag));
        if ( item ) 
        {
            item->hWnd = hWnd;
            item->flag = 1;
            item-> flashHandle = FlashNew();
            item->xc=malloc(sizeof(MgContext));
            item->fi=malloc(sizeof(FlashInfo));
            if (g_list_length (flashlist)==0)
            {
                item->sound=0;
#ifndef _NOUNIX_
                soundonly=0;
                soundsecond=1;
                nn=0;
                tt=1;
                item->sound=1;
                pthread_mutex_lock(&count_mutex);
                soundId=-1;
                frameId=-1;
                pthread_mutex_unlock(&count_mutex);
#endif
            }
            else
            {
                item->sound=0;
            }
            
                   
            if (item->flashHandle == 0)
            {
                return FALSE;
            }
	    }
        else
        {
            return FALSE;
        }
    

        // Load level 0 movie
        do
        {
           status = FlashParse(item->flashHandle, 0, buffer, size);
        }
        while (status & FLASH_PARSE_NEED_DATA);
       
        FlashGetInfo(item->flashHandle, item->fi);

        if(item->fi->frameWidth/20 < 400 && item->fi->frameHeight/20 < 300)
        {
           item->fi->frameWidth *= 2;
           item->fi->frameHeight *= 2;
        }

     if((FlashGraphicInitMg(item->flashHandle, hWnd,item->xc,item->sound)==2)||(item->fi->frameRate==0))
     {
         FlashClose(item->flashHandle); 
         free(item->xc);
         free(item->fi);
         g_free(item);
         return FALSE;
     }

     if(item->sound==1)
         FlashSoundInit(item->flashHandle, "/dev/dsp");
     FlashSetGetUrlMethod(item->flashHandle, showUrl, 0);
     speed = 100/item->fi->frameRate;

#if 0     
        printf("*************************************\n");
        printf("The flash version is %d. \n",buffer[3]);
        printf("The flash control number is %d. \n",buffer[8]);
        printf("the frame rate is %d\n",buffer[9+9]);
        framenumber=(buffer[19]&0xff)|((buffer[20]<<8)&0xff00);
        printf("the frame number is %x\n",framenumber);
        tmp=g_list_length (flashlist);
        printf("the flashlist list is %d\n",tmp);
        printf("the flash will be played on %d\n",hWnd);
        printf("the flash handle is %x\n",item->flashHandle);
        printf("#####################################\n");
#endif

       //  if( !(SetClientTimer(speed)) ) 
         //  printf("setitimer fail\n");		
#ifndef _NOUNIX_
         if(speed==0)
#endif
             speed=1;
             
         SetTimer(item->hWnd,ID_TIMER,speed);
         flashlist = g_list_append(flashlist,item);
         return TRUE;
    }
}

static int FlashProc(HWND hWnd, int message, WPARAM wParam, LPARAM lParam)
{
    long cmd;
    long wakeUp;
    int skip;
    int flag=0;
    int sound=0;
    int ret;
    FlashEvent fe;
    struct timeval wd;
    GList *tmplist;
	flashflag *tmp = NULL;
    MgContext *xc;
    FlashHandle flashHandle;  //hz

    tmplist = flashlist;
    while(tmplist)
    {
        if(tmplist->data)
        {
			tmp = (flashflag *)tmplist->data;
			if ((tmp->hWnd == hWnd)&&(tmp->flag==1)) {
                flag=1;
                sound=tmp->sound;
                flashHandle = tmp->flashHandle;
                xc=tmp->xc;
                break;
			}
        }
        tmplist=tmplist->next;
    }

    switch (message) {
        case MSG_CREATE:
            break;

        case MSG_PAINT:
            if(flag==0)
                break;
            fe.type = FeRefresh;
	       	FlashExec(flashHandle, FLASH_EVENT, &fe, &wd);
		    break;

        case MSG_TIMER:
            if(flag==0)
                break;
            if(tmp->sound)
            {
                ret=task2(&skip);
            }
            else
            {
                ret=1;
                skip=0;
            }
            if(ret)
            {
                while(skip > 0)
                {
                    cmd = FLASH_STEP;
                    wakeUp = FlashExec(flashHandle, cmd, 0, &wd);				
                    skip--;
                }	
                cmd = FLASH_WAKEUP;
                wakeUp = FlashExec(flashHandle, cmd, 0, &wd);
                if (xc->fd.flash_refresh)
                {
                    FlashCopyMg(xc);
                    xc->fd.flash_refresh = 0;
                }
            }
            break;

        case MSG_KEYDOWN:
            if(flag==0)
                break;
            if (wParam == SCANCODE_ENTER) {
                cmd = FLASH_EVENT;
                fe.key = FeKeyEnter;
            }
            wakeUp = FlashExec(flashHandle, cmd, &fe, &wd);
            break;

		case MSG_LBUTTONDOWN:
            if(flag==0)
                break;
			fe.type = FeButtonPress;
            if(soundonly==0)
                FlashExec(flashHandle, FLASH_EVENT, &fe, &wd);
		    break;	
			
		case MSG_LBUTTONUP:
            if(flag==0)
                break;
			fe.type = FeButtonRelease;
            if(soundonly==0)
			FlashExec(flashHandle, FLASH_EVENT, &fe, &wd);
		    break;	
			
		case MSG_MOUSEMOVE:
            if(flag==0)
                break;
			fe.type = FeMouseMove;
			fe.x = LOWORD (lParam);
			fe.y = HIWORD (lParam);
			FlashExec(flashHandle, FLASH_EVENT, &fe, &wd);
   		    break;	

        case MSG_DESTROY:
            soundplay=1;
            if(soundframe==1)
                soundframe=0;
            if(flag==1)
             {
                 if(tmp->sound)
                     KillTimer(tmp->hWnd,ID_TIMER);
                 if(xc->bmp)
                 {
                     tmp->flag=0;
                     if(xc->bmp->bmBits) free(xc->bmp->bmBits);
                     free(xc->bmp);
                 }
                 free(tmp->xc);
                 free(tmp->fi);
                 FlashClose(flashHandle);
                 g_free(tmp); 
                 flashlist = g_list_remove(flashlist,tmp);
             }
            break;
	}
      return DefaultControlProc (hWnd, message, wParam, lParam);
}

BOOL RegisterMyFlashPlayer(void)
{
    WNDCLASS MyClass;
    MyClass.spClassName = CTRL_FLASH;
    MyClass.dwStyle     = WS_NONE;
    MyClass.dwExStyle   = WS_EX_NONE;
    MyClass.hCursor     = GetSystemCursor (IDC_ARROW);
    MyClass.iBkColor    = COLOR_lightwhite;
    MyClass.WinProc     = FlashProc;
    return RegisterWindowClass (&MyClass);
}
void UnregisterFlashControl (void)
{
    UnregisterWindowClass (CTRL_FLASH);
}

#endif
