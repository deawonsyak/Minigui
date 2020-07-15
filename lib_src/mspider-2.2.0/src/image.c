/*
 * File: image.c
 *
 * Copyright (C) 2005-2006 Feynman Software
 * Copyright (C) 1997 Raph Levien <raph@acm.org>
 * Copyright (C) 1999 James McCollough <jamesm@gtwn.net>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

/*
 * This file implements image data transfer methods. It handles the transfer
 * of data from an Image to a DwImage widget.
 */

#include <stdio.h>
#include <string.h>
#include "mgdconfig.h"
#include "msg.h"
#include "image.h"

/*
 * Local data
 */
static size_t linebuf_size = 0;
static guchar *linebuf = NULL;


/*
 * Create and initialize a new image structure.
 */
mSpiderImage *a_Image_new (gint width,
                        gint height,
                        const char *alt_text,
                        gint32 bg_color)
{
    mSpiderImage *Image;

    Image = g_new(mSpiderImage, 1);
    Image->dw = (DwImage *) a_Dw_image_new(DW_IMAGE_RGB, alt_text);
    Image->width = 0;
    Image->height = 0;
    Image->cmap = NULL;
    Image->in_type = MSPIDER_IMG_TYPE_NOTSET;
    Image->bg_color = bg_color;
    Image->ProcessedBytes = 0;
    Image->BitVec = NULL;
    Image->State = IMG_Empty;

    Image->RefCount = 1;
    return Image;
}

/*
 * Create and initialize a new image structure for background.
 */
mSpiderImage *a_Image_new_for_background (gint width, gint height)
{
    mSpiderImage *Image;
    DwImage* dw;

    /* create a fake DwImage object for background image */
    if ((dw = g_malloc0 (sizeof (DwImage))) == NULL)
        return NULL;

    dw->state = DW_IMAGE_EMPTY;

    if ((Image = g_new (mSpiderImage, 1)) == NULL) {
        g_free (dw);
        return NULL;
    }

    Image->dw = dw;

    Image->width = 0;
    Image->height = 0;
    Image->cmap = NULL;
    Image->in_type = MSPIDER_IMG_TYPE_NOTSET;
    Image->bg_color = 0;
    Image->ProcessedBytes = 0;
    Image->BitVec = NULL;
    Image->State = IMG_Empty;
    Image->RefCount = 1;

    return Image;
}

/*
 * Create and initialize a new image structure.
 */
mSpiderImage *a_Image_jsnew (DwImage *dimage)
{
    mSpiderImage *Image;

    Image = g_new(mSpiderImage, 1);
    Image->dw = dimage;
    Image->width = 0;
    Image->height = 0;
    Image->cmap = NULL;
    Image->in_type = MSPIDER_IMG_TYPE_NOTSET;
    Image->bg_color = 0x0fff;
    Image->ProcessedBytes = 0;
    Image->BitVec = NULL;
    Image->State = IMG_Empty;

    Image->RefCount = 1;
    return Image;
}

/*
 * Deallocate an Image structure
 */
static void Image_free (mSpiderImage *Image)
{
    a_Bitvec_free (Image->BitVec);
    g_free (Image);
}

/*
 * Unref and free if necessary
 */
void a_Image_unref (mSpiderImage *Image)
{
    _MSG(" %d ", Image->RefCount);
    if (Image && --Image->RefCount == 0)
        Image_free(Image);
}

/*
 * Add a reference to an Image struct
 */
void a_Image_ref (mSpiderImage *Image)
{
   if (Image)
      ++Image->RefCount;
}

/*
 * Decode 'buf' (an image line) into RGB format.
 */
static guchar * Image_line (mSpiderImage *Image, const guchar *buf, const guchar *cmap, gint y)
{
    guint x;

    switch (Image->in_type) {
    case MSPIDER_IMG_TYPE_INDEXED:
        if (cmap) {
            for (x = 0; x < Image->width; x++)
                memcpy(linebuf + x * 3, cmap + buf[x] * 3, 3);
        }
        else {
            MSG("Gif:: WARNING, image lacks a color map\n");
        }
        break;

    case MSPIDER_IMG_TYPE_GRAY:
        for (x = 0; x < Image->width; x++)
            memset(linebuf + x * 3, buf[x], 3);
        break;

    case MSPIDER_IMG_TYPE_RGB:
        /* avoid a memcpy here!  --Jcid */
        return (guchar *)buf;

    case MSPIDER_IMG_TYPE_NOTSET:
        g_warning("ERROR: Image type not set...\n");
        break;
    }
    return linebuf;
}

/*
 * Set initial parameters of the image
 */
void a_Image_set_parms (mSpiderImage *Image, guchar *EntryBuf, mSpiderUrl *url,
                       gint version, guint width, guint height,
                       mSpiderImgType type)
{
    if (!Image->dw->buffer) {
        if (Image->dw->is_widget)
            a_Dw_image_set_buffer (Image->dw, EntryBuf, url, version);
        else {
            Image->dw->buffer = EntryBuf;
            Image->dw->url = url;
            Image->dw->version = version;
        }
    }

    if (!Image->BitVec)
        Image->BitVec = a_Bitvec_new(height);

    Image->in_type = type;
    Image->width = width;
    Image->height = height;

    if (3 * width > linebuf_size) {
        linebuf_size = (((3 * width + 31) >> 5) << 5);
        linebuf = g_realloc (linebuf, linebuf_size);
    }
    Image->State = IMG_SetParms;

    if (Image->dw->is_widget)
        a_Dw_image_size (Image->dw, width, height);
    else {
        Image->dw->width = width;
        Image->dw->height = height;
    }
}

/*
 * Reference the dicache entry color map
 */
void a_Image_set_cmap (mSpiderImage *Image, const guchar *cmap , int size)
{
    Image->cmap = cmap;
#ifdef ENABLE_ANIMATION
    if (Image->dw->is_widget) {
        if (!Image->dw->cmap)
            Image->dw->cmap = g_new0 (guchar, 256*3);
        memcpy (Image->dw->cmap, cmap, size);
    }
#endif
    Image->State = IMG_SetCmap;
}

/*
 * Implement the write method
 */
void a_Image_write (mSpiderImage *Image, const guchar *buf, guint y, gint decode)
{
    guchar *newbuf;
    int i;

    g_return_if_fail ( y < Image->height || Image->dw->buffer);

    if (decode) {
        /* Decode 'buf' and copy it into the DicEntry buffer */
        newbuf = Image_line (Image, buf, Image->cmap, y);
#ifdef _BIG_ENDIAN
        guchar *buf = Image->dw->buffer + y*Image->width*3; 
        for (i = 0; i < Image->width*3; i+=3 ) {
            buf[i] = newbuf[i+2];
            buf[i+1] = newbuf[i+1];
            buf[i+2] = newbuf[i];
        }
#else
        memcpy (Image->dw->buffer + y*Image->width*3, newbuf, Image->width*3);
#endif
    }
    a_Bitvec_set_bit (Image->BitVec, y);
    Image->State = IMG_Write;

    /* Update the row in DwImage */
    if (Image->dw->is_widget)
        a_Dw_image_draw_row (Image->dw, Image->width, Image->height, 0, y);
}

/*
 * Implement the close method
 */
void a_Image_close (mSpiderImage *Image)
{
    a_Image_unref (Image);
}

