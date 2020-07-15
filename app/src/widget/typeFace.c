#include <minigui/common.h>
#include <minigui/minigui.h>
#include <minigui/gdi.h>
#include <minigui/window.h>
#include <minigui/control.h>

#include "typeFace.h"

void unloadBitMap(BITMAP *bmp)
{
	if (bmp->bmBits != NULL) {
		UnloadBitmap(bmp);
		bmp->bmBits = NULL;
	}
}

gal_pixel rgb2pixel(unsigned int rgb)
{
	gal_pixel pixel;

	pixel = rgb & 0x00ff00;
	pixel |= (rgb & 0xff0000) >> 16;
	pixel |= (rgb & 0x0000ff) << 16;

	return pixel;
}

int color_shode(unsigned int start, unsigned int end, unsigned int *pixels, int num)
{
	int sr = (start & 0xff0000) >> 16;
	int sg = (start & 0x00ff00) >> 8;
	int sb = (start & 0x0000ff);

	int er = (end & 0xff0000) >> 16;
	int eg = (end & 0x00ff00) >> 8;
	int eb = (end & 0x0000ff);

	float step_r = (er-sr);
	float step_g = (eg-sg);
	float step_b = (eb-sb);

	int i;
	for(i = 0; i < num; i++){
		int dr = sr + i*step_r/num;
		int dg = sg + i*step_g/num;
		int db = sb + i*step_b/num;

		pixels[i] = (dr << 16) | (dg << 8) | db;
		//printf("i:%d 0x%06x\n",i,pixels[i]);
	}

	return 0;
}
