#ifndef __IMAGE_H__
#define __IMAGE_H__

/* The mSpiderImage data-structure and methods */

#include "mgdconfig.h"
#include "dw_image.h"
#include "bitvec.h"

typedef struct _mSpiderImage mSpiderImage;

typedef enum {
   MSPIDER_IMG_TYPE_INDEXED,
   MSPIDER_IMG_TYPE_RGB,
   MSPIDER_IMG_TYPE_GRAY,
   MSPIDER_IMG_TYPE_NOTSET    /* Initial value */
} mSpiderImgType;

/* These will reflect the Image's "state" */
typedef enum {
   IMG_Empty,      /* Just created the entry */
   IMG_SetParms,   /* Parameters set */
   IMG_SetCmap,    /* Color map set */
   IMG_Write,      /* Feeding the entry */
   IMG_Close,      /* Whole image got! */
   IMG_Abort       /* Image transfer aborted */
} ImageState;

struct _mSpiderImage {
   DwImage *dw;

   /* Parameters as told by image data */
   guint width;
   guint height;

   const guchar *cmap;     /* Color map (only for indexed) */
   mSpiderImgType in_type;   /* Image Type */
   gint32 bg_color;        /* Background color */

   gint ProcessedBytes;    /* Amount of bytes already decoded */
   bitvec_t *BitVec;       /* Bit vector for decoded rows */
   ImageState State;
   guchar RGBor;        /*1 means RGB,0 means BGR */  
   gint RefCount;          /* Reference counter */
};


/*
 * Function prototypes
 */
mSpiderImage *a_Image_new (gint width, gint height, const char *alt_text, gint32 bg_color);
mSpiderImage *a_Image_jsnew (DwImage *dimage);
mSpiderImage *a_Image_new_for_background (gint width, gint height);

void a_Image_ref (mSpiderImage *Image);
void a_Image_unref (mSpiderImage *Image);

void a_Image_set_parms(mSpiderImage *Image, guchar *EntryBuf, mSpiderUrl *url,
                       gint version, guint width, guint height,
                       mSpiderImgType type);
void a_Image_set_cmap(mSpiderImage *Image, const guchar *cmap,int size);
void a_Image_write(mSpiderImage *Image, const guchar *buf, guint y, gint decode);
void a_Image_close(mSpiderImage *Image);

#endif /* __IMAGE_H__ */

