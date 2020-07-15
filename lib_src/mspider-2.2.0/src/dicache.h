#ifndef __DICACHE_H__
#define __DICACHE_H__

#include "bitvec.h"
#include "image.h"
#include "cache.h"

/* These will reflect the entry's "state" */
typedef enum {
   DIC_Empty,      /* Just created the entry */
   DIC_SetParms,   /* Parameters set */
   DIC_SetCmap,    /* Color map set */
   DIC_Write,      /* Feeding the entry */
   DIC_Close,      /* Whole image got! */
   DIC_Abort       /* Image transfer aborted */
} DicEntryState;

typedef struct _DICacheEntry DICacheEntry;

struct _DICacheEntry {
   mSpiderUrl *url;          /* Image URL for this entry */
   gint width, height;     /* As taken from image data */
   mSpiderImgType type;      /* Image type */
   guchar *cmap;           /* Color map */
   guchar *ImageBuffer;    /* Decompressed buffer */
   size_t TotalSize;       /* Amount of memory the image takes up */
   gint Y;                 /* Current decoding row */
   bitvec_t *BitVec;       /* Bit vector for decoded rows */
   DicEntryState State;    /* Current status for this entry */
   gint RefCount;          /* Reference Counter */
   gint version;           /* Version number, used for different
                              versions of the same URL image */

   DICacheEntry *next;     /* Link to the next "newer" version */
};


void a_Dicache_init (void);

DICacheEntry *a_Dicache_get_entry(const mSpiderUrl *Url);
DICacheEntry *a_Dicache_add_entry(const mSpiderUrl *Url);

void a_Dicache_callback(gint Op, CacheClient_t *Client);

void a_Dicache_set_parms(mSpiderUrl *url, gint version, mSpiderImage *Image,
                         gint width, gint height, mSpiderImgType type);
void a_Dicache_set_cmap(mSpiderUrl *url, gint version, mSpiderImage *Image,
                        const guchar *cmap, gint num_colors,
                        gint num_colors_max, gint bg_index);
void a_Dicache_write(mSpiderImage *Image, mSpiderUrl *url, gint version,
                     const guchar *buf, gint x, gint Y);
void a_Dicache_close(mSpiderUrl *url, gint version, CacheClient_t *Client);

void a_Dicache_write_ex(DwImage* dw, const guchar *buf, guint Y);

void a_Dicache_invalidate_entry(const mSpiderUrl *Url);
DICacheEntry* a_Dicache_ref(const mSpiderUrl *Url, gint version);
void a_Dicache_unref(const mSpiderUrl *Url, gint version);
void a_Dicache_freeall(void);

#ifdef ENABLE_ANIMATION
void a_Dicache_set_frame_cmap(mSpiderUrl *url, gint version, mSpiderImage *Image,
                        const guchar *cmap, gint num_colors,
                        gint num_colors_max, gint bg_index, guchar ** rcmap);
#endif

#endif /* __DICACHE_H__ */
