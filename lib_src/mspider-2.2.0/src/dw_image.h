#ifndef __DW_IMAGE_H__
#define __DW_IMAGE_H__

#include <stdio.h>
#include <stdlib.h>

#include "mgdconfig.h"
#include <minigui/common.h>
#include <minigui/gdi.h>
#include "dw_widget.h"
#include "url.h"           /* for mSpiderUrl */

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#define DW_TYPE_IMAGE           (a_Dw_image_get_type ())
#define DW_IMAGE(obj)          (G_TYPE_CHECK_INSTANCE_CAST ((obj), DW_TYPE_IMAGE, DwImage))
#define DW_IMAGE_CLASS(klass)  (G_TYPE_CHECK_CLASS_CAST ((klass), DW_TYPE_IMAGE, DwImageClass))
#define DW_IMAGE_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS ((obj), DW_TYPE_IMAGE, DwImageClass))

#define DW_IS_IMAGE(obj)       (G_TYPE_CHECK_INSTANCE_TYPE ((obj), DW_TYPE_IMAGE))



#define        MAX_LINEBUF             1024

#ifdef ENABLE_ANIMATION
typedef struct _AnimationFrame AnimationFrame;

struct _AnimationFrame {
    /** The disposal method (from GIF89a specification):
     *  Indicates the way in which the graphic is to be treated after being displayed.
     *  - 0\n No disposal specified. The decoder is not required to take any action.
     *  - 1\n Do not dispose. The graphic is to be left in place. 
     *  - 2\n Restore to background color. The area used by the frame must be restored to 
     *        the background color.
     *  - 3\n Restore to previous. The decoder is required to restore the area overwritten by 
     *        the frame with what was there prior to rendering the frame.
     */
    int disposal;
    /** the x-coordinate of top-left corner of the frame in whole animation screen. */
    int off_x;
    /** the y-coordinate of top-left corner of the frame in whole animation screen. */
    int off_y;
    /** the width of the frame. */
    unsigned int width;
    /** the height of the frame. */
    unsigned int height;

    /** the time of the frame will be display, in the unit of animation time_unit. */
    unsigned int delay_time;
    /** the local color map */
    guchar* cmap;
    /** the bits of the frame. */
    guchar* bits;

    int filled;

    /** The next frame */
    struct _AnimationFrame* next;
    /** The previous frame */
    struct _AnimationFrame* prev;
};
#endif
typedef enum {
   DW_IMAGE_RGB
} DwImageType;

typedef struct _DwImage       DwImage;
typedef struct _DwImageClass  DwImageClass;

typedef struct _DwImageMap      DwImageMap;
typedef struct _DwImageMapList  DwImageMapList;
typedef struct _DwImageMapShape DwImageMapShape;

typedef enum {
    DW_IMAGE_EMPTY,
    DW_IMAGE_FILLING,
    DW_IMAGE_SCALEING,
    DW_IMAGE_DONE
}DwImageState;

struct _DwImage
{
   DwWidget widget;

   mSpiderUrl *url;
   gint version;
   DwImageType type;
   guchar *buffer;
   DwImageState state;
   gint width;
   gint height;

   gboolean is_widget;
   gint client_id;
   
   gint alt_text_width;

   /* non NULL if image is scaled */
   guchar *scaled_buffer;

   /* ALT text (for selection) */
   gchar *alt_text;

   DwImageMapList *map_list;
   mSpiderUrl *usemap_url;
   gboolean ismap;

   gint hover_link;
   gint pressed_link;
   gboolean selected[DW_HIGHLIGHT_NUM_LAYERS];

   int Y;
#ifdef ENABLE_ANIMATION
   /* Just for animated Gif */
   gint bkcolor; /* the global background index */
   gint transparent; /* the transparent index */
   guchar* cmap; /* the global color map */
   gint timer;
   int nr_frames;
   int elapsed_10ms;
   AnimationFrame* current_frame;
   AnimationFrame* done_frame;
   AnimationFrame* frames;
#endif
};

struct _DwImageClass
{
   DwWidgetClass parent_class;

   gboolean (*link_entered)  (DwImage *page,
                              gint link, gint x, gint y);
   gboolean (*link_pressed)  (DwImage *page,
                              gint link, gint x, gint y,
                              DWORD flags);
   gboolean (*link_released) (DwImage *page,
                              gint link, gint x, gint y,
                              DWORD flags);
   gboolean (*link_clicked)  (DwImage *page,
                              gint link, gint x, gint y,
                              DWORD flags);
};


/*
 * Image Maps
 */

#define DW_IMAGE_MAP_SHAPE_RECT    0
#define DW_IMAGE_MAP_SHAPE_CIRCLE  1
#define DW_IMAGE_MAP_SHAPE_POLY    2

struct _DwImageMapList
{
   DwImageMap *maps;
   gint num_maps;
   gint num_maps_max;

   DwImageMapShape *shapes;
   gint num_shapes;
   gint num_shapes_max;
};

struct _DwImageMap
{
   mSpiderUrl *url;
   gint start_shape;
};

struct _DwImageMapShape
{
   gint type;
   gint link;

   union {
      CLIPRGN poly;
      struct  {
         gint32 x;
         gint32 y;
         gint32 r2;
      } circle;
      struct  {
         gint32 top;
         gint32 bottom;
         gint32 left;
         gint32 right;
      } rect;
   } data;
};


/*
 * Function prototypes
 */
GType a_Dw_image_get_type (void);
DwWidget* a_Dw_image_new  (DwImageType type, const gchar *alt_text);
void a_Dw_image_size (DwImage *image, gint width, gint height);
void a_Dw_image_draw_row(DwImage *image,
                         gint Width, gint Height, gint x, gint y);
void a_Dw_image_set_buffer(DwImage *image, guchar *ImageBuffer,
                           mSpiderUrl *url, gint version);

void a_Dw_image_set_ismap (DwImage *image);
void a_Dw_image_set_usemap (DwImage *image,  DwImageMapList *map_list,
                            mSpiderUrl *usemap_url);

/* Image maps */
void a_Dw_image_map_list_init      (DwImageMapList *list);
void a_Dw_image_map_list_free      (DwImageMapList *list);

void a_Dw_image_map_list_add_map    (DwImageMapList *list,
                                     mSpiderUrl *url);
void a_Dw_image_map_list_add_shape  (DwImageMapList *list,
                                     gint type,
                                     gint link,
                                     POINT *points,
                                     gint num_points);
void Dw_image_scale_row          (DwImage *image, gint y_dest);
#ifdef ENABLE_ANIMATION
void destroy_gif_frames (DwImage* image);
#endif
#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __DW_IMAGE_H__ */
