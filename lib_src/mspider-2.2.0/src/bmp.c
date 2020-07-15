/* * File: bmp.c *
 *
 * Copyright (C) 2005-2006 Feynman Software
 *
 * This program is g_free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include "mgdconfig.h"

#ifdef ENABLE_BMP

#include <stdio.h>
#include <glib.h>

#include <minigui/common.h>
#include <minigui/gdi.h>

#include "image.h"
#include "web.h"
#include "cache.h"
#include "dicache.h"

#define DEBUG_LEVEL 6
#include "debug.h"

typedef enum {
   MSPIDER_BMP_INIT,
   MSPIDER_BMP_STARTING,
   MSPIDER_BMP_READING,
   MSPIDER_BMP_DONE,
   MSPIDER_BMP_ERROR
} mSpiderBmpState;

typedef struct BITMAPFILEHEADER
{
   unsigned short bfType;
   unsigned long  bfSize;
   unsigned short bfReserved1;
   unsigned short bfReserved2;
   unsigned long  bfOffBits;
} BITMAPFILEHEADER;

typedef struct BITMAPINFOHEADER
{
   unsigned long  biSize;
   unsigned long  biWidth;
   unsigned long  biHeight;
   unsigned short biBitCount;
   unsigned long  biCompression;
} BITMAPINFOHEADER;

typedef struct WINBMPINFOHEADER  /* size: 40 */
{
   unsigned long  biSize;
   unsigned long  biWidth;
   unsigned long  biHeight;
   unsigned short biPlanes;
   unsigned short biBitCount;
   unsigned long  biCompression;
   unsigned long  biSizeImage;
   unsigned long  biXPelsPerMeter;
   unsigned long  biYPelsPerMeter;
   unsigned long  biClrUsed;
   unsigned long  biClrImportant;
} WINBMPINFOHEADER;

#define BI_RGB          0
#define BI_RLE8         1
#define BI_RLE4         2
#define BI_BITFIELDS    3

#define OS2INFOHEADERSIZE  12
#define WININFOHEADERSIZE  40

#define ReadGuchar(Buf)  *((guchar *)Buf)
#define ReadShortInt(Buf) *((guchar *)Buf)|*((guchar *)Buf+1)<<8
#define ReadInt(Buf) *((guchar *)Buf)|*((guchar *)Buf+1)<<8|*((guchar *)Buf+2)<<16|*((guchar *)Buf+3)<<24
#define PIX2BYTES(n)    (((n)+7)/8)
#define red 0
#define green 2
typedef struct _mSpiderBmp {
   mSpiderImage *Image;
   mSpiderUrl *url;
   gint version;
   BITMAPFILEHEADER fileheader;
   BITMAPINFOHEADER infoheader;
   guchar flag;
   gint pitch;
   gint effect_depth;
   gint size;
   gint line;//used by BI_RLE8
   guint linerec;//how many line have received!
   guint lineout;//how many line have written to dicache!
   char *Data;
   gint  bytessaved;//how many image data have be saved!
   gint  bytesdelt;//how many image data have be delt!
   size_t Start_Ofs, Skip, NewStart;
   mSpiderBmpState state;
   guint rmask, gmask, bmask;
} mSpiderBmp;

/*
 * Forward declarations
 */
static mSpiderBmp *Bmp_new(mSpiderImage *Image, mSpiderUrl *url, gint version);
static void Bmp_callback(int Op, CacheClient_t *Client);
static void Bmp_write(mSpiderBmp *bmp, void *Buf, guint BufSize);
static void Bmp_close(mSpiderBmp *bmp, CacheClient_t *Client);
static int read_bmfileheader(guchar *f, BITMAPFILEHEADER *fileheader);
static int read_win_bminfoheader(guchar *f, BITMAPINFOHEADER *infoheader);
static int bmpComputePitch (int bpp, Uint32 width, Uint32* pitch, BOOL does_round);
static void image_4bit_save (mSpiderBmp *bmp,guchar *f, guint Bufsize);
static void image_8bit_save (mSpiderBmp *bmp,guchar *f, guint Bufsize);
static void image_16bit_save(mSpiderBmp *bmp,guchar *f, guint Bufsize);
static void image_24bit_save(mSpiderBmp *bmp,guchar *f, guint Bufsize);

/* exported function */
DwWidget *a_Bmp_image(const char *Type, void *P, CA_Callback_t *Call,
                       void **Data)
{
   mSpiderWeb *web = P;
   DICacheEntry *DicEntry;

   if ( !web->Image )
      web->Image = a_Image_new(0, 0, NULL, 0);

   /* Add an extra reference to the Image (for dicache usage) */
   a_Image_ref(web->Image);

   DicEntry = a_Dicache_get_entry(web->url);
   if ( !DicEntry ) {
      /* Let's create an entry for this image... */
      DicEntry = a_Dicache_add_entry(web->url);

      /* ... and let the decoder feed it! */
      *Data = Bmp_new(web->Image, DicEntry->url, DicEntry->version);
      *Call = (CA_Callback_t) Bmp_callback;
   } else {
      /* Let's feed our client from the dicache */
      a_Dicache_ref(DicEntry->url, DicEntry->version);
      *Data = web->Image;
      *Call = (CA_Callback_t) a_Dicache_callback;
   }
   return DW_WIDGET (web->Image->dw);
}

/*
 * Finish the decoding process
 */
static void Bmp_close(mSpiderBmp *bmp, CacheClient_t *Client)
{
   a_Dicache_close(bmp->url, bmp->version, Client);
   if(!bmp->Data)  
      g_free(bmp->Data);
   g_free(bmp);

}

static mSpiderBmp *Bmp_new(mSpiderImage *Image, mSpiderUrl *url, gint version)
{
   mSpiderBmp *bmp = g_malloc(sizeof(*bmp));

   bmp->Image = Image;
   bmp->url = url;
   bmp->version = version;

   bmp->state = MSPIDER_BMP_INIT;
   bmp->Start_Ofs = 0;
   bmp->bytessaved=0;
   bmp->bytesdelt=0;
   bmp->linerec=0;
   bmp->lineout=0;

   return bmp;
}

static void Bmp_callback (int Op, CacheClient_t *Client)
{
   DEBUG_MSG (5, "Bmp_callback is called\n");

   if (Op)
   {
      Bmp_write(Client->CbData, Client->Buf, Client->BufSize);
      Bmp_close(Client->CbData, Client);
   }
   else
   {
      Bmp_write(Client->CbData, Client->Buf, Client->BufSize);
   }
}

static void Bmp_write(mSpiderBmp *bmp, void *Buf, guint Bufsize)
{
   mSpiderImgType type;
   gint i;
   unsigned long biSize;
  gint ncol;

   /*Save the received data to bmp->data*/
    bmp-> rmask = 0x001f, bmp->gmask = 0x03e0, bmp->bmask = 0x7c00;
    if (bmp->state == MSPIDER_BMP_INIT)
    {  
     if(Bufsize>54)/*Make sure the header of the bmp is got!*/
        {
            if (read_bmfileheader (Buf, &bmp->fileheader) != 0)
            {
                bmp->state=MSPIDER_BMP_ERROR;
                    return;
            }
            biSize =*((guchar *)Buf+14)|*((guchar *)Buf+15)<<8 ;
            ncol = (bmp->fileheader.bfOffBits - biSize - 14) / 4;
            if (biSize >= WININFOHEADERSIZE)
            {
       	        if (read_win_bminfoheader (Buf+14, &bmp->infoheader) != 0)
                {
                    bmp->state=MSPIDER_BMP_ERROR;
                    return;
                }

            }
            else if (biSize == OS2INFOHEADERSIZE) 
            {
                bmp->state=MSPIDER_BMP_ERROR;
                return;
            }
            else
            {
                 bmp->state=MSPIDER_BMP_ERROR;
                 return;
            }

            bmp->effect_depth = bmp->infoheader.biBitCount;
            bmpComputePitch (bmp->effect_depth, bmp->infoheader.biWidth,
                                 (guint *)(&bmp->pitch), TRUE);
           // biSize =bmp->pitch*bmp->infoheader.biHeight;
            biSize =bmp->infoheader.biWidth*3*bmp->infoheader.biHeight;
            if( !(bmp->Data =g_malloc(biSize)) )
             {
                 bmp->state=MSPIDER_BMP_ERROR;
                 return;
             }
	   // bmp->Image->widget.RGBorBGR=0xaa;
           if (bmp->infoheader.biBitCount == 1)
            type = MSPIDER_IMG_TYPE_GRAY;
           else 
            type = MSPIDER_IMG_TYPE_RGB;

            a_Dicache_set_parms(bmp->url, bmp->version, bmp->Image,
                             (guint)bmp->infoheader.biWidth,
                             (guint)bmp->infoheader.biHeight,
                             type);

            bmp->state = MSPIDER_BMP_READING;
        }
    }
  if (bmp->state == MSPIDER_BMP_READING)
   {
           switch (bmp->infoheader.biCompression)
          {
            case BI_BITFIELDS: /* ignore the bit fileds */
                 bmp->rmask = ReadShortInt(Buf+14+bmp->infoheader.biSize);
                 bmp->gmask = ReadShortInt(Buf+16+bmp->infoheader.biSize);
                 bmp->bmask =ReadShortInt(Buf+18+bmp->infoheader.biSize);

            case BI_RGB:
                 if (bmp->infoheader.biBitCount == 4)
                 {
                    //printf("start to process 4 bits image\n");
                    image_4bit_save(bmp,Buf,Bufsize);
                 }
                 else if (bmp->infoheader.biBitCount == 8)
                 {
                    //printf("start to process 8 bits image\n");
                    image_8bit_save(bmp,Buf,Bufsize);
                 }
                 else if (bmp->infoheader.biBitCount == 16)
                 {
                    //printf("start to process 16 bits image\n");
                    image_16bit_save(bmp,Buf,Bufsize);
                 }
                 else if (bmp->infoheader.biBitCount == 24)
                 {
                   //printf("start to process 24 bits image\n");
                   image_24bit_save(bmp,Buf,Bufsize);
                 }
                else
                {
                    bmp->state=MSPIDER_BMP_ERROR;
                    return;
                }
                 break;
            default:
                 bmp->state=MSPIDER_BMP_ERROR;
                 return;

        }

      
       for(i=bmp->lineout;i<bmp->linerec;i++)
       {
          a_Dicache_write (bmp->Image, bmp->url, bmp->version,
                         bmp->Data+i*bmp->infoheader.biWidth*3, 0,
                         bmp->infoheader.biHeight-1-i);
          bmp->lineout++;
       }

       if(bmp->lineout==bmp->infoheader.biHeight)
       {
          bmp->state = MSPIDER_BMP_DONE;
       }
   }

}

static int 
bmpComputePitch (int bpp, Uint32 width, Uint32* pitch, BOOL does_round)
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

static int read_win_bminfoheader(guchar *f, BITMAPINFOHEADER *infoheader)
{
   WINBMPINFOHEADER win_infoheader;

   win_infoheader.biSize = ReadInt(f);
   win_infoheader.biWidth = ReadInt(f+4);
   win_infoheader.biHeight = ReadInt(f+8);
   win_infoheader.biPlanes = ReadShortInt(f+12);
   win_infoheader.biBitCount =ReadShortInt(f+14) ;
   win_infoheader.biCompression =ReadInt(f+16) ;
   win_infoheader.biSizeImage = ReadInt(f+20);
   win_infoheader.biXPelsPerMeter = ReadInt(f+24);
   win_infoheader.biYPelsPerMeter = ReadInt(f+28);
   win_infoheader.biClrUsed = ReadInt(f+32);
   win_infoheader.biClrImportant =ReadInt(f+36) ;

   infoheader->biSize = win_infoheader.biSize;
   infoheader->biWidth = win_infoheader.biWidth;
   infoheader->biHeight = win_infoheader.biHeight;
   infoheader->biBitCount = win_infoheader.biBitCount;
   infoheader->biCompression = win_infoheader.biCompression;
   return 0;
}

static int read_bmfileheader(guchar *f, BITMAPFILEHEADER *fileheader)
{
   fileheader->bfType =ReadShortInt(f) ;
   fileheader->bfSize= ReadInt(f+2);
   fileheader->bfReserved1=ReadShortInt(f+6) ;
   fileheader->bfReserved2= ReadShortInt(f+8);
   fileheader->bfOffBits= ReadInt(f+10);

   if (fileheader->bfType != 19778)
      return -1;

   return 0;
}

static void image_4bit_save (mSpiderBmp *bmp,guchar *f, guint Bufsize)
{
    int  j;
    guchar  pixel,temp0,temp1;
    guchar *line;
    guchar *index;
    index=f+14+bmp->infoheader.biSize;

    do
    {
        line = bmp->Data+bmp->infoheader.biWidth*bmp->linerec*3;
        for (j = 0; j < bmp->infoheader.biWidth/2; j++)
        {
            pixel = ReadGuchar(f+bmp->fileheader.bfOffBits+bmp->bytesdelt+j);
            temp0=pixel&0x0f;
            temp1=(pixel>>4)&0x0f;
            line [2] = index[temp0*4+red];
            line [1] = index[temp0*4+1];
            line [0] = index[temp0*4+green];
            line += 3;
            line [2] = index[temp0*4+red];
            line [1] = index[temp0*4+1];
            line [0] = index[temp0*4+green];
            line += 3;
        }
        if(bmp->infoheader.biWidth&0x01)
        {
            pixel = ReadGuchar(f+bmp->fileheader.bfOffBits+bmp->bytesdelt+(++j));
            temp0=pixel&0x0f;
            line [2] = index[temp0*4+red];
            line [1] = index[temp0*4+1];
            line [0] = index[temp0*4+green];
            line += 3;
        }
        if((bmp->bytesdelt+j)>Bufsize-bmp->fileheader.bfOffBits)
          return;
        bmp->bytesdelt+=bmp->pitch;
        bmp->linerec++;
    }
    while(bmp->linerec < bmp->infoheader.biHeight-1);
}

/* read_8bit_image:
 *  For reading the 8-bit BMP image format.
 * This only support bit masks specific to Windows 95.
 */
static void image_8bit_save (mSpiderBmp *bmp,guchar *f, guint Bufsize)
{
    int  j;
    guchar  pixel;
    guchar *line;
    guchar *index;
    index=f+14+bmp->infoheader.biSize;

    do
    {
        line = bmp->Data+bmp->infoheader.biWidth*bmp->linerec*3;
        for (j = 0; j < bmp->infoheader.biWidth; j++)
        {
            pixel = ReadGuchar(f+bmp->fileheader.bfOffBits+bmp->bytesdelt+j);
            line [2] = index[pixel*4+red];
            line [1] = index[pixel*4+1];
            line [0] = index[pixel*4+green];
            line += 3;
            if((bmp->bytesdelt+j)>Bufsize-bmp->fileheader.bfOffBits)
              return;
        }
        bmp->bytesdelt+=bmp->pitch;
        bmp->linerec++;
    }
    while(bmp->linerec < bmp->infoheader.biHeight-1);
}

/* read_16bit_image:
 *  For reading the 16-bit BMP image format.
 * This only support bit masks specific to Windows 95.
 */
static void image_16bit_save (mSpiderBmp *bmp,guchar *f,guint Bufsize)
{
    int  j;
    WORD pixel;
    guchar *line;
    do
    {
        line = bmp->Data+bmp->infoheader.biWidth*bmp->linerec*3;
        for (j = 0; j < bmp->infoheader.biWidth; j++)
        {
            pixel = ReadShortInt(f+bmp->fileheader.bfOffBits+bmp->bytesdelt+2*j);
            if (bmp->gmask == 0x03e0)    /* 5-5-5 */
            {
                line [red] = ((pixel >> 10) & 0x1f) << 3;
                line [1] = ((pixel >> 5) & 0x1f) << 3;
                line [green] = (pixel & 0x1f) << 3;
            }
            else                    /* 5-6-5 */
            {
                line [red] = ((pixel >> 11) & 0x1f) << 3;
                line [1] = ((pixel >> 5) & 0x3f) << 2;
                line [green] = (pixel & 0x1f) << 3;
            }

            line += 3;
            if((bmp->bytesdelt+2*j)>Bufsize-bmp->fileheader.bfOffBits)
              return;
        }
#if 0
        if (bmp->infoheader.biWidth & 0x01)
           bmp->bytesdelt=bmp->infoheader.biWidth*2+2;
        else
            bmp->bytesdelt=bmp->infoheader.biWidth*2;
#endif
        bmp->bytesdelt+=bmp->pitch;
        bmp->linerec++;
    }
    while(bmp->linerec < bmp->infoheader.biHeight-1);
}

/* read_24bit_image:
 *  For reading the 24-bit BMP image format.
 * This only support bit masks specific to Windows 95.
 */
static void image_24bit_save (mSpiderBmp *bmp,guchar *f,guint Bufsize)
{
    guchar *line,*orig;
    guint i;
    do
    {
        line = bmp->Data+bmp->infoheader.biWidth*bmp->linerec*3;
        orig=f+bmp->fileheader.bfOffBits+bmp->bytesdelt;
        for(i=0;i<bmp->infoheader.biWidth;i++)
        {
                line [2] = orig[red];
                line [1] = orig[1];
                line [0] = orig[green];
                line+=3;
                orig+=3;
 
        }
       // memcpy(line,orig,bmp->infoheader.biWidth*3);

        if((bmp->bytesdelt+bmp->infoheader.biWidth*3)>Bufsize-bmp->fileheader.bfOffBits)
              return;
        bmp->bytesdelt+=bmp->pitch;
        bmp->linerec++;
    }
    while(bmp->linerec < bmp->infoheader.biHeight-1);
}

/* read_RLE8_compressed_image:
 *  For reading the 8 bit RLE compressed BMP image format.
 */
#if 0
static void RLE8_compressed_image_save(mSpiderBmp *bmp,guchar *f )
{
    unsigned char count, val, val0;
    int j,i, pos;
    int eolflag, eopicflag;
    guchar *index;
    guchar *lines;
    eopicflag = 0;
    bmp->line = bmp->infoheader.biHeight - 1;
    i=0;
    index=f+14+bmp->infoheader.biSize;
  
   while (eopicflag == 0)
   {
      
      pos = 0;                               /* x position in bitmap */
      eolflag = 0;                           /* end of line flag */
      lines = bmp->Data+bmp->pitch*bmp->linerec;
      while ((eolflag == 0) && (eopicflag == 0))
      {
          count = ReadGuchar(f+bmp->fileheader.bfOffBits+bmp->bytesdelt+i);
         i++;
         val = ReadGuchar(f+bmp->fileheader.bfOffBits+bmp->bytesdelt+i);
         i++;
         if (count > 0)
         { 
         /* repeat pixel count times */
            for (j=0;j<count;j++)
            {
               lines[0] = index[val*4];
               lines[1] = index[val*4+1];
               lines[2] = index[val*4+2];
               lines+=3;
               pos++;
            }
         }
         else {
            switch (val) {

               case 0:                       /* end of line flag */
                 bmp->linerec+=1;
                 eolflag=1;
                 bmp->bytesdelt+=i;
                 i=0;
                  break;

               case 1:                       /* end of picture flag */
                  eopicflag=1;
                  bmp->linerec+=1;
                  break;

               case 2:                       /* displace picture */
                  count = ReadGuchar(f+bmp->fileheader.bfOffBits+bmp->bytesdelt+i);
                  i++;
                  val = ReadGuchar(f+bmp->fileheader.bfOffBits+bmp->bytesdelt+i);
                  i++; 
                  pos += count;//?
                  bmp->line -= val;//?
                  break;

               default:                      /* read in absolute mode */
                  for (j=0; j<val; j++)
                  {
                     val0 = ReadGuchar(f+bmp->fileheader.bfOffBits+bmp->bytesdelt+i);
                     i++;
                     lines[0] = index[val*4];
                     lines[1] = index[val*4+1];
                     lines[2] = index[val*4+2];
                     lines+=3;
                     pos++;
                  }

                  if (j%2 == 1)
                    i++; 
                  break;
            }
         }

         if (pos-1 > (int)bmp->infoheader.biWidth)
            eolflag=1;
      }
      if((bmp->bytesdelt+i)>bmp->bytessaved)
        return;
      //      bits += pitch;
     bmp->line--;
      if (bmp->line < 0)
            eopicflag = 1;
   }
}
/* read_RLE4_compressed_image:
 *  For reading the 4 bit RLE compressed BMP image format.
 */
static void read_RLE4_compressed_image (mSpiderBmp *bmp,guchar *f )
{
   unsigned char b[8];
   unsigned char count;
   unsigned short val0, val;
   int j, k, pos, line;
   int eolflag, eopicflag;
   guchar *index;
   guchar *lines;

    eopicflag = 0;                            /* end of picture flag */
    bmp->line =bmp->infoheader->biHeight - 1;
    i=0;
    index=f+14+bmp->infoheader.biSize;
    lines = bmp->Data+bmp->pitch*bmp->linerec;

   while (eopicflag == 0) {
      pos = 0;
      eolflag = 0;                           /* end of line flag */

      while ((eolflag == 0) && (eopicflag == 0)) {
         count = ReadGuchar(f+bmp->fileheader.bfOffBits+bmp->bytesdelt+i);
         i++;
         val =ReadGuchar(f+bmp->fileheader.bfOffBits+bmp->bytesdelt+i);
         i++;

         if (count > 0) {                    /* repeat pixels count times */
            b[1] = val & 15;
            b[0] = (val >> 4) & 15;
            for (j=0; j<count; j++) {
            if (pos % 2 == 0)
               {
               lines[0] = index[b[j%2]*4];
               lines[1] = index[b[j%2]+1];
               lines[2] = index[b[j%2]+2];
               lines+=3;
               }
            else
              {
               lines[0] = index[val*4];
               lines[1] = index[val*4+1];
               lines[2] = index[val*4+2];
               lines+=3;
              }
            pos++;

            }
         }
         else {
            switch (val) {

               case 0:                       /* end of line */
                 bmp->linerec+=1;
                 eolflag=1;
                 bmp->bytesdelt+=i;
                 i=0;
                 break;

               case 1:                       /* end of picture */
                  eopicflag=1;
                  bmp->linerec+=1;
                  break;

               case 2:                       /* displace image */
                  count = ReadGuchar(f+bmp->fileheader.bfOffBits+bmp->bytesdelt+i);
                  i++;
                  val = ReadGuchar(f+bmp->fileheader.bfOffBits+bmp->bytesdelt+i);
                  i++; 
                  pos += count;
                  bmp->line -= val;
                  break;

               default:                      /* read in absolute mode */
                  for (j=0; j<val; j++) {
                     if ((j%4) == 0) {
                        val0 = fp_igetw(f);
                        for (k=0; k<2; k++) {
                           b[2*k+1] = val0 & 15;
                           val0 = val0 >> 4;
                           b[2*k] = val0 & 15;
                           val0 = val0 >> 4;
                        }
                     }
            if (pos % 2 == 0)
                bits [pos/2] = b[j%4] << 4;
            else
                bits [pos/2] = bits [pos/2] | b[j%4];
                     pos++;
                  }
                  break;
            }
         }

         if (pos-1 > (int)infoheader->biWidth)
            eolflag=1;
      }

      bits += pitch;
      line--;
      if (line < 0)
            eopicflag = 1;
   }
}
#endif


#endif /* ENABLE_BMP */

