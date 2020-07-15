#ifndef __GIF_DECODE_H__
#define __GIF_DECODE_H__
#include "minigui/common.h"
#include "minigui/minigui.h"
#include "minigui/gdi.h"
#include "minigui/ctrl/animation.h"

#include "platform.h"

#define MAXCOLORMAPSIZE         256
typedef struct tagGIFSCREEN {
    unsigned int Width;
    unsigned int Height;
    RGB ColorMap [MAXCOLORMAPSIZE];
    unsigned int BitPixel;
    unsigned int ColorResolution;
    unsigned int Background;
    unsigned int AspectRatio;
    int transparent;
    int delayTime;
    int inputFlag;
    int disposal;
} GIFSCREEN;

typedef struct _gif_data{
	ANIMATIONFRAME *frames;
	ANIMATIONFRAME *cur_frame;
	MG_RWops* area;	
	GIFSCREEN *GifScreen;

	HDC hdc;

	int frame_limit;
	int frame_num;

	unsigned char autoloop:1;
	unsigned char gif_stop_flag:1;
	unsigned char gif_stop_done:1;

	orb_thread_t	thread;
	orb_sem_t		sem;
	orb_sem_t		get_sem;

	ANIMATIONFRAME *(*get_current_frame)(struct _gif_data *gif);

}gifData;

gifData *CreateGifData(char *file, int frame_limit);
void gif_data_destroy(gifData *gif);

#endif
