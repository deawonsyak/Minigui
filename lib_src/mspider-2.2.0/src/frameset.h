/*
 **    File:frameset.h   
 **    $Id: frameset.h,v 1.4 2005-09-13 10:48:21 xgdu Exp $
 **    
 **    The emspider frameset widget
 **    Lisence:  GPL
 */

#ifndef __FRAMESET_H_
#define __FRAMESET_H_

#include <glib.h>
#include "dw_container.h"

#ifdef __cplusplus
extern "C" {
#endif


#define DW_FRAMESET(obj)	  (G_TYPE_CHECK_INSTANCE_CAST ((obj), DW_TYPE_FRAMESET, DwFrameset))
#define DW_TYPE_FRAMESET	  (a_Dw_frameset_get_type ())
#define DW_FRAMESET_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), DW_TYPE_FRAMESET, DwFramesetClass))
#define DW_IS_FRAMESET(obj)	     (G_TYPE_CHECK_INSTANCE_TYPE ((obj), DW_TYPE_FRAMESET))
#define DW_IS_FRAMESET_CLASS(klass)      (G_TYPE_CHECK_CLASS_TYPE ((klass), DW_TYPE_FRAMESET))


typedef struct _DwFrameset	     DwFrameset;
typedef struct _DwFramesetClass     DwFramesetClass;
typedef struct _DwFramesetChild     DwFramesetChild;
typedef struct _DwFramesetRowCol    DwFramesetRowCol;
typedef gint32 Length;


/* (adapted from dw_style.h) Lengths */
#define LENGTH_CREATE_ABSOLUTE(n)     (((n) << 2) | 1)
#define LENGTH_CREATE_PERCENTAGE(n)   ((LENGTH_FLOAT_TO_REAL (n) << 3) | 2)
#define LENGTH_CREATE_RELATIVE(n)     ((LENGTH_FLOAT_TO_REAL (n) << 3) | 6)
#define LENGTH_UNDEF_LENGTH           0

#define LENGTH_IS_ABSOLUTE(l)         ((l) & 1)
#define LENGTH_IS_PERCENTAGE(l)       (((l) & 7) == 2)
#define LENGTH_IS_RELATIVE(l)         (((l) & 7) == 6)

#define LENGTH_GET_ABSOLUTE(l)        ((l) >> 2)
#define LENGTH_GET_PERCENTAGE(l)      LENGTH_REAL_TO_FLOAT ((l) >> 3)
#define LENGTH_GET_RELATIVE(l)        LENGTH_REAL_TO_FLOAT ((l) >> 3)

#define LENGTH_REAL_TO_FLOAT(v)       ((gfloat)(v) / 0x10000)
#define LENGTH_FLOAT_TO_REAL(v)       ((gint)((v) * 0x10000))

/* used in frame resize */
#define RESIZE_NONE                   -1

/* default border size */
#define DWFRAMESET_DEFAULT_BORDER_SIZE 2

struct _DwFrameset
{
  DwContainer container;
  
  GList *children;
  GSList *children_area;
  gint area_used;
  DwFramesetRowCol *rows;
  DwFramesetRowCol *cols;
  guint row_total_absolute, col_total_absolute;
  guint16 nrows;
  guint16 ncols;
  guint16 current_frame;
  gboolean in_drag : 1;
  gint16 resize_row, resize_col;
  HCURSOR cursor_rowcol, cursor_row, cursor_col;
  gboolean top_level;
};

struct _DwFramesetClass
{
  DwContainerClass parent_class;
};

struct _DwFramesetChild
{
  DwWidget *widget;
  guint16 row_attach;
  guint16 col_attach;
  guint16 xpadding;
  guint16 ypadding;
  gboolean border : 1;
  gboolean noresize : 1;
};

struct _DwFramesetRowCol
{
  guint16 location;
  guint16 allocation;
  gboolean noresize : 1;
  gboolean border: 1;
  Length length;
  guint16 spacing;
};


GType  a_Dw_frameset_get_type    (void);
DwWidget* a_Dw_frameset_new	        (gchar	       *rows,
					 gchar	       *columns);
void       a_Dw_frameset_resize          (DwFrameset   *frameset,
					 gchar	       *rows,
					 gchar	       *columns);
void	   a_Dw_frameset_attach	        (DwFrameset   *frameset,
					 DwWidget     *child,
					 guint		row_attach,
					 guint		col_attach,
					 guint          xpadding,
					 guint          ypadding,
					 gboolean       border,
					 gboolean       noresize);
    

#ifdef __cplusplus
}
#endif

#endif  /*__FRAMESET_H_*/
