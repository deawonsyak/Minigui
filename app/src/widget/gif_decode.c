/*
 *   This file is part of MiniGUI, a mature cross-platform windowing 
 *   and Graphics User Interface (GUI) support system for embedded systems
 *   and smart IoT devices.
 * 
 *   Copyright (C) 2002~2018, Beijing FMSoft Technologies Co., Ltd.
 *   Copyright (C) 1998~2002, WEI Yongming
 * 
 *   This program is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *   (at your option) any later version.
 * 
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 * 
 *   You should have received a copy of the GNU General Public License
 *   along with this program.  If not, see <http://www.gnu.org/licenses/>.
 * 
 *   Or,
 * 
 *   As this program is a library, any link to this program must follow
 *   GNU General Public License version 3 (GPLv3). If you cannot accept
 *   GPLv3, you need to be licensed from FMSoft.
 * 
 *   If you have got a commercial license of this program, please use it
 *   under the terms and conditions of the commercial license.
 * 
 *   For more information about the commercial license, please refer to
 *   <http://www.minigui.com/en/about/licensing-policy/>.
 */
/*
 ** gif89a.c: the animation GIF89a module.
 **
 ** Create date: 2001/2/02
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "gif_decode.h"
#undef _DEBUG

#include "minigui/common.h"

#include "minigui/minigui.h"
#include "minigui/gdi.h"
#include "minigui/ctrl/animation.h"

/*
 * GIF decoding routine
 */
#define MAX_LWZ_BITS            12
#define INTERLACE               0x40
#define LOCALCOLORMAP           0x80

#define CM_RED                  0
#define CM_GREEN                1
#define CM_BLUE                 2

#define BitSet(byte, bit)               (((byte) & (bit)) == (bit))
#define ReadOK(file,buffer,len)         MGUI_RWread(file, buffer, len, 1)
#define LM_to_uint(a,b)                 (((b)<<8)|(a))
#define PIX2BYTES(n)                    (((n)+7)/8)

typedef struct tagIMAGEDESC {
	int Top;
	int Left;
	int Width;
	int Height;
	BOOL haveColorMap;
	int bitPixel;
	int grayScale;
	RGB ColorMap [MAXCOLORMAPSIZE];
	BOOL interlace;
} IMAGEDESC;

static int ZeroDataBlock = 0;

static int bmpComputePitch (int bpp, Uint32 width, Uint32* pitch, BOOL does_round);
static int LWZReadByte (MG_RWops *area, int flag, int input_code_size);
static int GetCode (MG_RWops *area, int code_size, int flag);
static int GetDataBlock (MG_RWops *area, unsigned char *buf);
static int DoExtension (MG_RWops *area, int label, GIFSCREEN* GifScreen);
static int ReadColorMap (MG_RWops *area, int number, RGB* ColorMap);
static int ReadImageDesc (MG_RWops *area, IMAGEDESC* ImageDesc, GIFSCREEN* GifScreen);

static int bmpComputePitch (int bpp, Uint32 width, Uint32* pitch, BOOL does_round)
{
	Uint32 linesize;
	int bytespp = 1;

	if(bpp == 1)
		linesize = PIX2BYTES (width);
	else if(bpp <= 4)
		linesize = PIX2BYTES (width << 2);
	else if (bpp <= 8)
		linesize = width;
	else if(bpp <= 16) {
		linesize = width * 2;
		bytespp = 2;
	} else if(bpp <= 24) {
		linesize = width * 3;
		bytespp = 3;
	} else {
		linesize = width * 4;
		bytespp = 4;
	}

	/* rows are DWORD right aligned*/
	if (does_round)
		*pitch = (linesize + 3) & -4;
	else
		*pitch = linesize;
	return bytespp;
}

static int LWZReadByte (MG_RWops *area, int flag, int input_code_size)
{
	int code, incode;
	register int i;
	static int fresh = FALSE;
	static int code_size, set_code_size;
	static int max_code, max_code_size;
	static int firstcode, oldcode;
	static int clear_code, end_code;
	static int table[2][(1 << MAX_LWZ_BITS)];
	static int stack[(1 << (MAX_LWZ_BITS)) * 2], *sp;

	if (flag) {
		set_code_size = input_code_size;
		code_size = set_code_size + 1;
		clear_code = 1 << set_code_size;
		end_code = clear_code + 1;
		max_code_size = 2 * clear_code;
		max_code = clear_code + 2;

		GetCode(area, 0, TRUE);

		fresh = TRUE;

		for (i = 0; i < clear_code; ++i) {
			table[0][i] = 0;
			table[1][i] = i;
		}
		for (; i < (1 << MAX_LWZ_BITS); ++i)
			table[0][i] = table[1][0] = 0;

		sp = stack;

		return 0;
	} else if (fresh) {
		fresh = FALSE;
		do {
			firstcode = oldcode = GetCode(area, code_size, FALSE);
		} while (firstcode == clear_code);
		return firstcode;
	}
	if (sp > stack)
		return *--sp;

	while ((code = GetCode(area, code_size, FALSE)) >= 0) {
		if (code == clear_code) {
			for (i = 0; i < clear_code; ++i) {
				table[0][i] = 0;
				table[1][i] = i;
			}
			for (; i < (1 << MAX_LWZ_BITS); ++i)
				table[0][i] = table[1][i] = 0;
			code_size = set_code_size + 1;
			max_code_size = 2 * clear_code;
			max_code = clear_code + 2;
			sp = stack;
			firstcode = oldcode = GetCode(area, code_size, FALSE);
			return firstcode;
		} else if (code == end_code) {
			int count;
			unsigned char buf[260];

			if (ZeroDataBlock){
				printf("error ZeroDataBlock is %d", ZeroDataBlock);
				return -2;
			}

			while ((count = GetDataBlock(area, buf)) > 0);

			if (count != 0) {
				/*
				 * fprintf (stderr,"missing EOD in data stream (common occurence)");
				 */
			}
			return -2;
		}
		incode = code;

		if (code >= max_code) {
			*sp++ = firstcode;
			code = oldcode;
		}
		while (code >= clear_code) {
			*sp++ = table[1][code];
			if (code == table[0][code]) {
				printf("EX_CTRL>GIF89a: circular table entry\n");
				return -1;
			}
			code = table[0][code];
		}

		*sp++ = firstcode = table[1][code];

		if ((code = max_code) < (1 << MAX_LWZ_BITS)) {
			table[0][code] = oldcode;
			table[1][code] = firstcode;
			++max_code;
			if ((max_code >= max_code_size) &&
					(max_code_size < (1 << MAX_LWZ_BITS))) {
				max_code_size *= 2;
				++code_size;
			}
		}
		oldcode = incode;

		if (sp > stack)
			return *--sp;
	}
	return code;
}


static int GetCode(MG_RWops *area, int code_size, int flag)
{
	static unsigned char buf[280];
	static int curbit, lastbit, done, last_byte;
	int i, j, ret;
	unsigned char count;

	if (flag) {
		curbit = 0;
		lastbit = 0;
		done = FALSE;
		return 0;
	}
	if ((curbit + code_size) >= lastbit) {
		if (done) {
			if (curbit >= lastbit)
				printf("EX_CTRL>GIF89a: bad decode\n");
			return -1;
		}
		buf[0] = buf[last_byte - 2];
		buf[1] = buf[last_byte - 1];

		if ((count = GetDataBlock(area, &buf[2])) == 0)
			done = TRUE;

		last_byte = 2 + count;
		curbit = (curbit - lastbit) + 16;
		lastbit = (2 + count) * 8;
	}
	ret = 0;
	for (i = curbit, j = 0; j < code_size; ++i, ++j)
		ret |= ((buf[i / 8] & (1 << (i % 8))) != 0) << j;

	curbit += code_size;

	return ret;
}

static int GetDataBlock (MG_RWops *area, unsigned char *buf)
{
	unsigned char count;

	if (!ReadOK(area, &count, 1))
		return 0;
	ZeroDataBlock = (count == 0);

	if ((count != 0) && (!ReadOK(area, buf, count)))
		return 0;
	return count;
}

static int DoExtension (MG_RWops *area, int label, GIFSCREEN* GifScreen)
{
	static unsigned char buf[256];

	switch (label) {
		case 0x01:                        /* Plain Text Extension */
			while (GetDataBlock (area, (unsigned char *) buf) != 0);
			break;
		case 0xff:                        /* Application Extension */
			while (GetDataBlock (area, (unsigned char *) buf) != 0);
			break;
		case 0xfe:                        /* Comment Extension */
			while (GetDataBlock (area, (unsigned char *) buf) != 0);
			return 0;
		case 0xf9:                        /* Graphic Control Extension */
			GetDataBlock (area, (unsigned char *) buf);
			GifScreen->disposal = (buf[0] >> 2) & 0x7;//000 000 0 0 the middle 2 bit is disposal
			GifScreen->inputFlag = (buf[0] >> 1) & 0x1;//000 000 0 0 the secand last bit 
			//is user input flag
			GifScreen->delayTime = LM_to_uint(buf[1], buf[2]);
			if ((buf[0] & 0x1) != 0)// 000 000 0 0 the last bit is transparent flag
				GifScreen->transparent = buf[3];
			else
				GifScreen->transparent = -1;

			while (GetDataBlock (area, (unsigned char *) buf) != 0);
			return 0;
		default:
			while (GetDataBlock (area, (unsigned char *) buf) != 0);
			break;
	}

	return 0;
}

static int ReadColorMap (MG_RWops *area, int number, RGB* ColorMap)
{
	int i;
	unsigned char rgb[3];

	for (i = 0; i < number; ++i) {
		if (!ReadOK (area, rgb, sizeof(rgb))) {
			return -1;
		}

		ColorMap [i].r = rgb[0];
		ColorMap [i].g = rgb[1];
		ColorMap [i].b = rgb[2];
	}

	return 0;
}

static int ReadGIFGlobal (MG_RWops *area, GIFSCREEN* GifScreen)
{
	unsigned char buf[9];
	unsigned char version[4];

	if (!ReadOK (area, buf, 6))
		return -1;                /* not gif image*/

	if (strncmp((char *) buf, "GIF", 3) != 0)
		return -1;

	strncpy ((char*)version, (char *) buf + 3, 3);
	version [3] = '\0';

	if (strcmp ((const char*)version, "87a") != 0 && strcmp ((const char*)version, "89a") != 0) {
		printf("EX_CTRL>GIF89a: GIF version number not 87a or 89a.\n");
		return -1;                /* image loading error*/
	}

	GifScreen->Background = -1;
	GifScreen->transparent = -1;
	GifScreen->delayTime = -1;
	GifScreen->inputFlag = -1;
	GifScreen->disposal = 0;

	if (!ReadOK (area, buf, 7)) {
		printf("EX_CTRL>GIF89a: bad screen descriptor\n");
		return -1;                /* image loading error*/
	}
	GifScreen->Width = LM_to_uint (buf[0], buf[1]);
	GifScreen->Height = LM_to_uint (buf[2], buf[3]);
	GifScreen->BitPixel = 2 << (buf[4] & 0x07);
	GifScreen->ColorResolution = (((buf[4] & 0x70) >> 3) + 1);
	GifScreen->Background = buf[5];
	GifScreen->AspectRatio = buf[6];

	if (BitSet(buf[4], LOCALCOLORMAP)) {        /* Global Colormap */
		if (ReadColorMap (area, GifScreen->BitPixel, GifScreen->ColorMap)) {
			printf("EX_CTRL>GIF89a: bad global colormap\n");
			return -1;                /* image loading error*/
		}
	}

	return 0;
}

static int ReadImageDesc (MG_RWops *area, IMAGEDESC* ImageDesc, GIFSCREEN* GifScreen)
{
	unsigned char buf[16];
	if (!ReadOK (area, buf, 9)) {
		printf("EX_CTRL>GIF89a: bad image size\n");
		return -1;
	}

	ImageDesc->Top = LM_to_uint (buf[0], buf[1]);
	ImageDesc->Left = LM_to_uint (buf[2], buf[3]);
	ImageDesc->Width = LM_to_uint (buf[4], buf[5]);
	ImageDesc->Height = LM_to_uint (buf[6], buf[7]);
	ImageDesc->haveColorMap = BitSet (buf[8], LOCALCOLORMAP);

	ImageDesc->bitPixel = 1 << ((buf[8] & 0x07) + 1);

	ImageDesc->interlace = BitSet(buf[8], INTERLACE);

	if (ImageDesc->haveColorMap) {
		if (ReadColorMap (area, ImageDesc->bitPixel, ImageDesc->ColorMap) < 0) {
			printf("EX_CTRL>GIF89a: bad local colormap\n");
			return -1;
		}
	} else {
		memcpy (ImageDesc->ColorMap, GifScreen->ColorMap, MAXCOLORMAPSIZE*sizeof (RGB));
	}

	return 0;
}

static int ReadImage (MG_RWops* area, MYBITMAP* bmp, IMAGEDESC* ImageDesc, GIFSCREEN* GifScreen, int ignore)
{

	unsigned char c;
	int v;
	int xpos = 0, ypos = 0, pass = 0;

	/*
	 * initialize the compression routines
	 */
	if (!ReadOK (area, &c, 1)) {
		printf("EX_CTRL>GIF89a: eof on image data\n");
		return -1;
	}

	if (LWZReadByte (area, TRUE, c) < 0) {
		printf("EX_CTRL>GIF89a: error reading image\n");
		return -1;
	}

	/*
	 * if this is an "uninteresting picture" ignore it.
	 */
	if (ignore) {
		while (LWZReadByte (area, FALSE, c) >= 0);
		return 0;
	}

	bmp->w = ImageDesc->Width;
	bmp->h = ImageDesc->Height;

	bmp->flags = MYBMP_FLOW_DOWN;
	if (GifScreen->transparent >= 0) {
		bmp->flags |= MYBMP_TRANSPARENT;
		bmp->transparent = GifScreen->transparent;
	}
	bmp->frames = 1;
	bmp->depth = 8;
	bmpComputePitch (bmp->depth, bmp->w, &bmp->pitch, TRUE);
	bmp->bits = malloc (bmp->h * bmp->pitch);

	if(!bmp->bits){
		printf("bmp bits is NULL");
		return -1;
	}

	while ((v = LWZReadByte (area, FALSE, c)) >= 0) {
		bmp->bits[ypos * bmp->pitch + xpos] = v;
		++xpos;
		if (xpos == ImageDesc->Width) {
			xpos = 0;
			if (ImageDesc->interlace) {
				switch (pass) {
					case 0:
					case 1:
						ypos += 8;
						break;
					case 2:
						ypos += 4;
						break;
					case 3:
						ypos += 2;
						break;
				}

				if (ypos >= ImageDesc->Height) {
					++pass;
					switch (pass) {
						case 1:
							ypos = 4;
							break;
						case 2:
							ypos = 2;
							break;
						case 3:
							ypos = 1;
							break;
						default:
							goto fini;
					}
				}
			} else {
				++ypos;
			}
		}
		if (ypos >= ImageDesc->Height)
			break;
	}

fini:
	if (v >= 0){
		return 0;
	}
	printf("V: %d", v);
	return -1;
}

void uni_AnimationFrameNodeDestory(ANIMATIONFRAME *frame)
{
	if(frame == NULL){
		return;
	}
	if (frame->mem_dc) DeleteMemDC (frame->mem_dc);
	if (frame->bits) free (frame->bits);
	free (frame);
}

GIFSCREEN *uni_AnimationFileHeadAnalysis(MG_RWops* area)
{
	GIFSCREEN *GifScreen = (GIFSCREEN *)calloc(1, sizeof(GIFSCREEN));
	if(GifScreen == NULL){
		return NULL;
	}
	area->seek(area, 0, SEEK_SET);
	if (ReadGIFGlobal (area, GifScreen) < 0){		//处理GIF头，GIF头固定长度13byte
		return NULL;
	}
	return GifScreen;
}

int gif_decode(gifData *gif)
{
	if(gif == NULL){
		return -1;
	}
	MG_RWops *area = gif->area;
	ANIMATIONFRAME* frame = NULL, *current = NULL;

	GIFSCREEN *GifScreen = gif->GifScreen;
	IMAGEDESC ImageDesc;
	MYBITMAP mybmp;
	if(area == NULL || GifScreen == NULL){
		return -1;
	}

	//printf("%s %d\r\n",__func__,__LINE__);
	unsigned char c;
	int ok = 0;
	if(area->eof(area) != 0){	//文件结束
		if(gif->autoloop == 1){
			area->seek(area, 0, SEEK_SET);	
			if (ReadGIFGlobal (area, GifScreen) < 0){
				return -1;
			}
		}else{
			return 1;
		}
	}

	while(1){
		if (ReadOK (area, &c, 1) == 0) {
			return -1;
		}

		switch (c) {
			case '!':		//扩展数据块
				if ( (ok = ReadOK (area, &c, 1)) == 0) {
					printf("EX_CTRL>GIF89a: EOF on image data\n");
					return -1;
				}
				DoExtension (area, c, GifScreen);
				break;

			case ',':		//图像数据块
				if (ReadImageDesc (area, &ImageDesc, GifScreen) < 0) {
					printf("%s %d\r\n",__func__,__LINE__);
					return -1;
				} else {
					if (ReadImage (area, &mybmp, &ImageDesc, GifScreen, 0) < 0){
						printf("%s %d\r\n",__func__,__LINE__);
						return -1;
					}
				}

				frame = (ANIMATIONFRAME*) calloc (1, sizeof (ANIMATIONFRAME));
				if (!frame){
					return -1;
				}else {
					frame->next = NULL;
					frame->disposal = GifScreen->disposal;
					frame->off_y = ImageDesc.Left;
					frame->off_x = ImageDesc.Top;
					frame->width = mybmp.w;
					frame->height = mybmp.h;
					frame->delay_time = (GifScreen->delayTime>10)?GifScreen->delayTime:10;

					if ((frame->mem_dc = CreateMemDCFromMyBitmap (&mybmp, ImageDesc.ColorMap)) == 0) {
						free (mybmp.bits);
						free (frame);
						printf("EX_CTRL>GIF89a: Error when expand frame bitmap.\n");
						return -1;
					}
					frame->bits = mybmp.bits;

					if(gif->frames == NULL){
						gif->frames = frame;
						gif->cur_frame = frame;
					}else{
						current = gif->frames;
						while(current->next){
							current = current->next;
						}
						current->next = frame;
						frame->prev = current;
						if(gif->cur_frame == NULL){
							gif->cur_frame = current;
						}
					}
					gif->frame_num++;
					orb_sem_post(gif->get_sem);
					return 0;

				}
				break;
		}
	}

	return 2;
}

static void gif_decode_thread(void *param)
{
	gifData *gif = param;

	//printf("%s %d\r\n",__func__,__LINE__);
	while(1){
		orb_sem_wait(gif->sem);
		if(gif->gif_stop_flag){
			break;
		}
		int ret = gif_decode(gif);	
		if(ret == 1){
			//TODO  gif play done
			printf("gif play done\n");
		}
		if(ret < 0){
			//printf("gif decode error!\n");
			orb_sem_post(gif->sem);
		}
		//printf("gif frame num:%d\n",gif->frame_num);

		if(gif->frame_num > gif->frame_limit){
			if(gif->cur_frame == NULL || gif->cur_frame->prev != gif->frames){
				ANIMATIONFRAME *frame = gif->frames;
				gif->frames = gif->frames->next;
				uni_AnimationFrameNodeDestory(frame);
				gif->frame_num--;
			}
		}
	}
	gif->gif_stop_done = 1;
	printf("%s %d ************* stop\r\n",__func__,__LINE__);
	orb_thread_exit(NULL);
}

static ANIMATIONFRAME *get_current_frame(gifData *gif)
{
	if(gif == NULL){
		return NULL;
	}


	if(orb_sem_wait_timeout(gif->get_sem,100)){
		printf("get_current_frame timeout\n");
		return NULL;
	}
	ANIMATIONFRAME *cur = gif->cur_frame;
	if(cur == NULL){
		return NULL;
	}

	gif->cur_frame = gif->cur_frame->next;
	orb_sem_post(gif->sem);

	return cur;
}

gifData *CreateGifData(char *file, int frame_limit)
{
	gifData *data = calloc(1,sizeof(gifData));
	if(data == NULL){
		return NULL;
	}

	if (!(data->area = MGUI_RWFromFile (file, "rb"))) {
		printf("file %s open failed", file);
		goto err;
	}

	data->GifScreen = uni_AnimationFileHeadAnalysis(data->area);
	if(data->GifScreen == NULL){
		goto err;
	}

	data->frame_limit = frame_limit;
	data->autoloop = 1;
	data->get_current_frame= get_current_frame;

	orb_sem_init(&data->sem,frame_limit);
	orb_sem_init(&data->get_sem,0);
	struct thread_param param = {"gif",1024,OS_PRIORITY_HIGH};
	orb_thread_create(&data->thread,&param,gif_decode_thread,data);

	orb_msleep(500);

	return data;
err:
	if(data->area){
		MGUI_RWclose(data->area);
	}
	free(data);
	return NULL;
}

void gif_data_destroy(gifData *gif)
{
	if(gif == NULL){
		return;
	}

	gif->gif_stop_flag = 1;
	orb_sem_post(gif->sem);
	while(gif->gif_stop_done == 0){
		orb_msleep(20);
	}

	orb_sem_destroy(gif->sem);
	orb_sem_destroy(gif->get_sem);

	if(gif->area){
		MGUI_RWclose(gif->area);
	}

	if(gif->GifScreen){
		free(gif->GifScreen);
	}

	ANIMATIONFRAME *frame = gif->frames;
	ANIMATIONFRAME *next;
	while(frame){
		next = frame->next;
		uni_AnimationFrameNodeDestory(frame);
		frame = next;
	}

	free(gif);
}

