#ifndef __RESOURCE_H__
#define __RESOURCE_H__

#include <minigui/common.h>
#include <minigui/minigui.h>
#include <minigui/gdi.h>
#include <minigui/window.h>
#include <minigui/control.h>

typedef struct winProcPara{
	const char *spClassName;
	int (*WinProc)(HWND hwnd, int message, WPARAM wParam, LPARAM lParam);
}winProcPara;

typedef struct viewContent{
	BITMAP fgbitmap;
	PLOGFONT mCurLogFont;
	const char *text;
}viewContent;


typedef struct {
	PLOGFONT mFont_SimSun20;
	PLOGFONT mFont_SimSun25;
	PLOGFONT mFont_SimSun30;
	PLOGFONT mFont_SimSun40;
	PLOGFONT mFont_SimSun50;

//	PLOGFONT mCurLogFont;
}fontLib;

enum FontID {
	ID_FONT_SIMSUN_20 = 20,
	ID_FONT_SIMSUN_25 = 25,
	ID_FONT_SIMSUN_30 = 30,
	ID_FONT_SIMSUN_40 = 40,
	ID_FONT_SIMSUN_50 = 50,
};

extern fontLib *fontlib;

typedef struct pngSize{
	int w;
	int h;
}pngSize;

typedef struct {
	int x;
	int y;
	pngSize ps;
	char *pngName;
	BITMAP bmp;
	RECT pngRect;
} pngPara;

typedef struct textPara{
	int textPX;
	char *text;
	RECT textRect;
	PLOGFONT logFont;
}textPara;

typedef struct widgetPara{
	pngPara *pngLocation;
	textPara *textContent;
	HWND handler;
	int id;
}widgetPara;

extern PLOGFONT GetLogFont(enum FontID id);
extern void InitLangAndFont();
extern void unloadBitMap(BITMAP *bmp);
gal_pixel rgb2pixel(unsigned int rgb);
int color_shode(unsigned int start, unsigned int end, unsigned int *pixels, int num);

#endif

