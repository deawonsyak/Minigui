/** $Id: frameset.c,v 1.26 2007-09-06 05:26:35 jpzhang Exp $
 **
 ** frameset.c: the emspider frameset support
 **
 ** Copyright (C) 2005-2006 Feynman Softwaver.
 **
 ** License: GPL
 */

#include <glib.h> 
#include <string.h>              /* for strpbrk() */
#include <locale.h>              /* for setlocale */

#include <minigui/common.h>
#include <minigui/minigui.h>
#include <minigui/gdi.h>
#include <minigui/window.h>
#include <minigui/control.h>

#include "mgdconfig.h"

#include "emspider.h"
#include "frameset.h"
#include "dw_viewport.h"
#include "url.h"
#include "doc.h"

#define DEBUG_ALLOC 10
#define DEBUG_EVENT 10
/* #define DEBUG_LEVEL 10 */
#include "debug.h"

/* child args */
enum
{
  CHILD_ARG_0,
  CHILD_ARG_ROW_ATTACH,
  CHILD_ARG_COL_ATTACH,
  CHILD_ARG_X_PADDING,
  CHILD_ARG_Y_PADDING,
  CHILD_ARG_BORDER,
  CHILD_ARG_NORESIZE
};

  
/* declarations */
static void Dw_frameset_class_init          (DwFramesetClass  *klass);
static void Dw_frameset_init	             (DwFrameset       *frameset);
static void Dw_frameset_finalize	     (GObject	        *object);
static void Dw_frameset_draw	             (DwWidget	        *widget,
                                                HDC              hdc,
					                         DwRectangle      *area);
static void Dw_frameset_add	             (DwContainer      *container,
					      DwWidget	        *widget);
static void Dw_frameset_remove	             (DwContainer      *container,
					      DwWidget	        *widget);
static void Dw_frameset_forall	             (DwContainer      *container,
					      DwCallback        callback,
					      void *	         callback_data);
static void Dw_frameset_realize             (DwWidget         *widget);
static void Dw_frameset_unrealize           (DwWidget         *widget);

/* private functions */
static GSList *Dw_frameset_get_multi_length(const gchar        *attr);

static void Dw_split_frameset(DwFrameset * frameset);

static DwContainerClass *parent_class = NULL;



GType
a_Dw_frameset_get_type (void)
{
  static GType frameset_type = 0;

  if (!frameset_type)
    {
      static const GTypeInfo frameset_info =
      {
    	sizeof (DwFramesetClass),
        (GBaseInitFunc) NULL,
         (GBaseFinalizeFunc) NULL,
         (GClassInitFunc) Dw_frameset_class_init,
         (GClassFinalizeFunc) NULL,
         (gconstpointer) NULL,
         sizeof (DwFrameset),
         0,
         (GInstanceInitFunc) Dw_frameset_init
      };
     frameset_type = g_type_register_static (DW_TYPE_CONTAINER, "DwFrameset", &frameset_info, 0);
 
    }
  
  return frameset_type;
}

/* standard Glib function */
static void
Dw_frameset_class_init (DwFramesetClass *class)
{
  GObjectClass *object_class;
  DwWidgetClass *widget_class;
  DwContainerClass *container_class;
  
  object_class = (GObjectClass*) class;
  widget_class = (DwWidgetClass*) class;
  container_class = (DwContainerClass*) class;
  
  parent_class = g_type_class_peek_parent (class);
/*------------------------------------------------------------------------*/
  object_class->finalize = Dw_frameset_finalize;
  
  widget_class->draw = Dw_frameset_draw;
  widget_class->realize = Dw_frameset_realize;
  widget_class->unrealize = Dw_frameset_unrealize;
  
  container_class->add = Dw_frameset_add;
  container_class->remove = Dw_frameset_remove;
  container_class->forall = Dw_frameset_forall;
}


/*
 * Standard GTK function
 */
static void
Dw_frameset_realize (DwWidget *widget)
{
 
  DWORD dwstyle;
  RECT rc;
  DwViewport * viewport;
  HWND parent;
    
  DwFrameset *frameset = DW_FRAMESET(widget);

  if (frameset->top_level) 
  {/*it is a top level frameset*/
      parent = ((BrowserWindow *)(widget->add_data))->main_window; 
  }
  else
      parent = widget->parent->hwnd;
    
  DW_WIDGET_SET_FLAGS (widget, DW_REALIZED);

  if (frameset->top_level) 
  {
        rc.left = ((BrowserWindow *)(widget->add_data))->client_rc.left;
        rc.top = ((BrowserWindow *)(widget->add_data))->client_rc.top;
        rc.right = ((BrowserWindow *)(widget->add_data))->client_rc.right;
        rc.bottom = ((BrowserWindow *)(widget->add_data))->client_rc.bottom;
  }
  else
  {
        rc.left = ((RECT*)(widget->add_data))->left;
        rc.top = ((RECT*)(widget->add_data))->top;
        rc.right = ((RECT*)(widget->add_data))->right;
        rc.bottom = ((RECT*)(widget->add_data))->bottom;
  }

  dwstyle = WS_CHILD | WS_BORDER | WS_VISIBLE;
  widget->hwnd =CreateWindowEx(CTRL_FRAME,
                                 "",
                                 dwstyle,
                                 WS_EX_CLIPCHILDREN,
                                 10,
                                 rc.left , rc.top, rc.right - rc.left , rc.bottom - rc.top,
                                 parent, 0);
  
   viewport = a_Dw_viewport_new(widget->hwnd);
   widget->viewport =viewport; 

   Dw_split_frameset(frameset);

}
/*
 *  split the frameset window    
 *
 */

static void Dw_split_frameset(DwFrameset * frameset)
{

   gint startx = 0 , starty = 0;
   gint rows , cols , height , width;
   RECT frc;
   int i , j;

   RECT trc;
   RECT * area;
    
   GetClientRect(DW_WIDGET(frameset)->hwnd , &frc);

   width = frc.right;
   height = frc.bottom;
   rows = frameset->nrows;
   cols = frameset->ncols;
   area = NULL;

    
    for (i = 0; i < rows; i++)
    {
        trc.top = starty;
        
        if (LENGTH_IS_PERCENTAGE(frameset->rows[i].length))
        {
            trc.bottom = (gint) (height * LENGTH_GET_PERCENTAGE(frameset->rows[i].length));
            starty += trc.bottom; 
        }
        else if (LENGTH_IS_ABSOLUTE(frameset->rows[i].length))
        {
            height -= (gint) LENGTH_GET_ABSOLUTE(frameset->rows[i].length);
            trc.bottom = (gint) LENGTH_GET_ABSOLUTE(frameset->rows[i].length);
            starty += trc.bottom; 
        }
        else
        {
           trc.bottom = (gint) LENGTH_GET_RELATIVE(frameset->rows[i].length);
           starty += trc.bottom;
        }        

        
        for (j = 0; j < cols; j++)
        {
            trc.left = startx;

            if (LENGTH_IS_PERCENTAGE(frameset->cols[j].length))
             {
                trc.right = (gint) (width * LENGTH_GET_PERCENTAGE(frameset->cols[j].length));
                startx += trc.right; 
             }
            else if (LENGTH_IS_ABSOLUTE(frameset->cols[j].length))
            {
                width -= (gint) LENGTH_GET_ABSOLUTE(frameset->cols[j].length);
                trc.right = (gint) LENGTH_GET_ABSOLUTE(frameset->cols[j].length);
                startx += trc.right; 
            }
            else
            {
                trc.right = (gint) LENGTH_GET_RELATIVE(frameset->cols[j].length);
                startx += trc.right;
            }        

            area = g_new0(RECT , 1);
            area->top = trc.top;
            area->left = trc.left;
            area->right = trc.right;
            area->bottom = trc.bottom;
            frameset->children_area = g_slist_append(frameset->children_area , area);
        }
       startx = 0; 
    }
    



}



/*
 * standard GTK function
 */
static void
Dw_frameset_unrealize (DwWidget *widget)
{
    DwFrameset * frameset = DW_FRAMESET(widget);

    GSList * area ;
    RECT * tmp;
    tmp = NULL;
    area = frameset->children_area;

    while (NULL != area)
    {
       tmp = area->data;
       area = g_slist_next(area);

       frameset->children_area = g_slist_remove(frameset->children_area , tmp); 
       frameset->area_used--;
       g_free(tmp);
    }
    
    frameset->children_area = NULL;
}

/* standard GTK function */
static void
Dw_frameset_init (DwFrameset *frameset)
{
  frameset->children = NULL;
  frameset->children_area = NULL;
  frameset->area_used = 0;
  frameset->rows = NULL;
  frameset->cols = NULL;
  frameset->nrows = 0;
  frameset->ncols = 0;
  frameset->resize_row = RESIZE_NONE;
  frameset->resize_col = RESIZE_NONE;
  frameset->in_drag = FALSE;
  frameset->cursor_rowcol = 0;
  frameset->cursor_row = 0;
  frameset->cursor_col = 0;
  frameset->top_level= FALSE;

  a_Dw_frameset_resize (frameset, "*", "*");
}

/*
 * standard Glib function
 *
 * resize frameset. It is only possible to enlarge the
 * frameset (or change dimensions while keeping the same
 * size) as reducing the size could orphan children.
 */
void
a_Dw_frameset_resize (DwFrameset *frameset,
		     gchar *row_multilengths,
		     gchar *col_multilengths)
{
  gint n_rows, n_cols, n;
  GSList *rows, *cols;
  gfloat total_percentage, total_relative;
  gfloat row_per_relative, row_per_percent;
  gfloat col_per_relative, col_per_percent;

  g_return_if_fail (frameset != NULL);
  g_return_if_fail (DW_IS_FRAMESET(frameset));
  g_return_if_fail (row_multilengths || col_multilengths);

  
  rows = Dw_frameset_get_multi_length(row_multilengths);
  cols = Dw_frameset_get_multi_length(col_multilengths);

  /* (older versions of) compiler happiness */
  row_per_relative = 0;
  row_per_percent = 0;
  col_per_relative = 0;
  col_per_percent = 0;

  n_rows = g_slist_length(rows);
  n_cols = g_slist_length(cols);

   /* calculate row and column dimensions. These calculations are slightly hairy...
    * The theory goes as follows:
    * 
    * Starting point is the frameset width and height. 
    * First come frames with absolute dimensions. No frame can be bigger than the frameset,
    * so checks are performed to make sure that absolute frame dimensions do not
    * exceed frameset dimensions. When the total number of frames with absolute dimensions
    * exceeds the frameset dimensions, the available space is divided by ratio to those frames.
    * In case there are only absolute dimensioned frames, available space is divided by ratio to
    * these frames and the absolute dimensions are transformed into percentual dimensions.
    *
    * Next come frames with percentual dimensions. They get to divide the remaining space after
    * all absolute dimensions have been allocated. If the total of all percentual dimensions adds
    * up to more than 100, the calculation is normalised to 100%.
    *
    * Last come frames with relative dimensions. They get to divide the remaining space after
    * all absolute and percentual dimensions have been allocated. All available space is divided
    * by ratio to relative weights and allocated to frames. In the second stage, relative dimensions
    * are transformed into percentual dimensions so the frameset ends up having only absolute (pixel)
    * and percentual dimensions.
    *
    * The calculations are performed in two stages. In the first (aggregation) stage, the total weight of
    * absolute, percentual and relative dimensions is calculated. In the second (normalisation) stage, relative
    * dimensions are transformed into percentual dimensions. The actual calculation of pixel dimensions
    * is performed in the callback function as it depends on information about the current size of the
    * frameset widget.
    *
    * Can this be optimised? Sure, but... later...
    */

  if(n_rows >= frameset->nrows)
    {
      frameset->nrows = n_rows;
      frameset->rows = g_realloc (frameset->rows, frameset->nrows * sizeof (DwFramesetRowCol));

      frameset->row_total_absolute = 0;
      total_percentage = 0;
      total_relative = 0;
      
      for(n = 0; n < frameset->nrows; n++) {
	frameset->rows[n].length = GPOINTER_TO_INT(g_slist_nth_data(rows, n));
	frameset->rows[n].noresize = FALSE;
	frameset->rows[n].border = FALSE;
	if(LENGTH_IS_RELATIVE(frameset->rows[n].length))
	  (LENGTH_GET_RELATIVE(frameset->rows[n].length) ?
	   total_relative += LENGTH_GET_RELATIVE(frameset->rows[n].length) :
	   total_relative++);
	else if(LENGTH_IS_PERCENTAGE(frameset->rows[n].length))
	  total_percentage += LENGTH_GET_PERCENTAGE(frameset->rows[n].length);
	else if(LENGTH_IS_ABSOLUTE(frameset->rows[n].length)) {
	  frameset->row_total_absolute += LENGTH_GET_ABSOLUTE(frameset->rows[n].length);
	}
      }
      
      if((total_percentage == 0) && (total_relative == 0)) {
	for(n = 0; n < frameset->nrows; n++)
	  if(LENGTH_IS_ABSOLUTE(frameset->rows[n].length))
	    frameset->rows[n].length =
	      LENGTH_CREATE_PERCENTAGE((gfloat) LENGTH_GET_ABSOLUTE(frameset->rows[n].length) /
				       frameset->row_total_absolute);
	frameset->row_total_absolute = 0;
	row_per_percent = 1;
      } else if(total_percentage < 1) {
	row_per_relative = (gfloat) (1 - total_percentage) / total_relative;
	row_per_percent = (gfloat) (total_relative > 0 ? 1 : 1.0 / total_percentage);
      } else {
	row_per_percent = (gfloat) (1 / total_percentage);
	row_per_relative = 0;
      }
    }
  
  if(n_cols >= frameset->ncols)
    {
      frameset->ncols = n_cols;
      frameset->cols = g_realloc (frameset->cols, frameset->ncols * sizeof (DwFramesetRowCol));
      
      frameset->col_total_absolute = 0;
      total_percentage = 0;
      total_relative = 0;
      
      for(n = 0; n < frameset->ncols; n++) {
	frameset->cols[n].length = GPOINTER_TO_INT(g_slist_nth_data(cols, n));
	frameset->cols[n].noresize = FALSE;
	frameset->cols[n].border = FALSE;
	if(LENGTH_IS_RELATIVE(frameset->cols[n].length))
	  (LENGTH_GET_RELATIVE(frameset->cols[n].length) ?
	   total_relative += LENGTH_GET_RELATIVE(frameset->cols[n].length) :
	   total_relative++);
	else if(LENGTH_IS_PERCENTAGE(frameset->cols[n].length))
	  total_percentage += LENGTH_GET_PERCENTAGE(frameset->cols[n].length);
	else if(LENGTH_IS_ABSOLUTE(frameset->cols[n].length)) {
	  frameset->col_total_absolute += LENGTH_GET_ABSOLUTE(frameset->cols[n].length);
	}
      }

      /* normalize values */
      if((total_percentage == 0) && (total_relative == 0)) {
	for(n = 0; n < frameset->ncols; n++)
	  if(LENGTH_IS_ABSOLUTE(frameset->cols[n].length))
	    frameset->cols[n].length =
	      LENGTH_CREATE_PERCENTAGE((gfloat) LENGTH_GET_ABSOLUTE(frameset->cols[n].length) /
				       frameset->col_total_absolute);
	frameset->col_total_absolute = 0;
	col_per_percent = 1;
	col_per_relative = 0;
      } else if(total_percentage < 1) {
	col_per_relative = (gfloat) (1 - total_percentage) / total_relative;
	col_per_percent = (gfloat) (total_relative > 0 ? 1 : 1.0 / total_percentage);
      } else {
	col_per_percent = (gfloat) (1 / total_percentage);
	col_per_relative = 0;
      }
    }

  /* now, calculate actual width/height distribution */
  if(frameset->nrows > 1) {
     for(n_rows = 0; n_rows < frameset->nrows; n_rows++) {
       if(LENGTH_IS_RELATIVE(frameset->rows[n_rows].length))
	 frameset->rows[n_rows].length = 
	   LENGTH_CREATE_PERCENTAGE((LENGTH_GET_RELATIVE(frameset->rows[n_rows].length) ?
				     (LENGTH_GET_RELATIVE(frameset->rows[n_rows].length) * row_per_relative) :
				     row_per_relative));
       else if(LENGTH_IS_PERCENTAGE(frameset->rows[n_rows].length))
	 frameset->rows[n_rows].length =
	   LENGTH_CREATE_PERCENTAGE((LENGTH_GET_PERCENTAGE(frameset->rows[n_rows].length) * row_per_percent));
     }
  } else
    frameset->rows[0].length = LENGTH_CREATE_PERCENTAGE(1);

  if(frameset->ncols > 1) {
     for(n_cols = 0; n_cols < frameset->ncols; n_cols++) {
       if(LENGTH_IS_RELATIVE(frameset->cols[n_cols].length))
	 frameset->cols[n_cols].length = 
	   LENGTH_CREATE_PERCENTAGE((LENGTH_GET_RELATIVE(frameset->cols[n_cols].length) ?
				     (LENGTH_GET_RELATIVE(frameset->cols[n_cols].length) * col_per_relative) :
				     col_per_relative));
       else if(LENGTH_IS_PERCENTAGE(frameset->cols[n_cols].length))
	 frameset->cols[n_cols].length =
	   LENGTH_CREATE_PERCENTAGE(LENGTH_GET_PERCENTAGE(frameset->cols[n_cols].length) * col_per_percent);
     }
  } else
    frameset->cols[0].length = LENGTH_CREATE_PERCENTAGE(1);

  g_slist_free(rows);
  g_slist_free(cols);
}

/* standard GTK function */
DwWidget*
a_Dw_frameset_new (gchar	*row_multilengths,
		  gchar	*col_multilengths)
{
  DwFrameset *frameset;

  
  if (!row_multilengths)
    row_multilengths = "*";
  if (!col_multilengths)
    col_multilengths = "*";
  
  frameset =  DW_FRAMESET (g_object_new (DW_TYPE_FRAMESET, NULL));

  DEBUG_MSG(DEBUG_EVENT, "/*\n");

  a_Dw_frameset_resize(frameset, row_multilengths, col_multilengths);

  return DW_WIDGET (frameset);
}

/* standard GTK function */
void
Dw_frameset_attach (DwFrameset	  *frameset,
		     DwWidget	          *child,
		     guint		   row_attach,
		     guint		   col_attach,
		     guint                 xpadding,
		     guint                 ypadding,
		     gboolean              border,
		     gboolean              noresize)
{
  DwFramesetChild *frameset_child;


  g_return_if_fail (frameset != NULL);
  g_return_if_fail (DW_IS_FRAMESET (frameset));
  g_return_if_fail (child != NULL);
  g_return_if_fail (DW_IS_WIDGET (child));
  g_return_if_fail (child->parent == NULL);
  g_return_if_fail (col_attach < frameset->ncols);
  g_return_if_fail (row_attach < frameset->nrows);

  DEBUG_MSG(DEBUG_EVENT, "gtk_frameset_attach(%d, %d, %d, %d, %d, %d, %d, %d)\n",
	    (gint) frameset, (gint) child, row_attach, col_attach,
	    xpadding, ypadding, border, noresize);
  
  frameset_child = g_new (DwFramesetChild, 1);
  frameset_child->widget = child;
  frameset_child->row_attach = row_attach;
  frameset_child->col_attach = col_attach;
  frameset_child->xpadding = xpadding;
  frameset_child->ypadding = ypadding;
  frameset_child->border = border;
  frameset_child->noresize = noresize;
  
  frameset->children = g_list_prepend (frameset->children, frameset_child);
  

   child->parent = DW_WIDGET (frameset);

  if (DW_WIDGET_REALIZED (child->parent))
  {
    a_Dw_widget_realize (child);
  }

  if (DW_WIDGET_VISIBLE (child->parent) && DW_WIDGET_VISIBLE (child))
    {
      p_Dw_widget_queue_resize (child,0,TRUE);
    }
  
}

/*
 * Standard GTK function
 *
 * add a widget (frame) to the frameset. The widget
 * will be put in the current_frame (which will
 * be increased in the process)
 */
static void
Dw_frameset_add (DwContainer *frameset,
		  DwWidget    *widget)
{
  guint row, col;

  /* is there space left in the frameset for this frame? */
  if (DW_FRAMESET(frameset)->current_frame >= 
      (DW_FRAMESET(frameset)->nrows * DW_FRAMESET(frameset)->ncols)) {
    DEBUG_MSG(DEBUG_EVENT, "No space in frameset for frame\n");
    return;
  }
  
  /* calculate row and column for frame */
  col = DW_FRAMESET(frameset)->current_frame % DW_FRAMESET(frameset)->ncols;
  row = DW_FRAMESET(frameset)->current_frame / DW_FRAMESET(frameset)->ncols;
  
  DEBUG_MSG(DEBUG_EVENT, "NEW FRAME in frameset %d\n", (gint) frameset);
  DEBUG_MSG(DEBUG_EVENT, "      ROW %d COL %d\n", row, col);
  
  Dw_frameset_attach(DW_FRAMESET(frameset), /* the frameset widget */
		      widget,                 /* the child widget */
		      row,                    /* row to attach to */
		      col,                    /* column to attach to */
		      0,                      /* marginwidth */
		      0,                      /* marginheight */
		      FALSE,                  /* border */
		      FALSE);                 /* noresize */

  (DW_FRAMESET(frameset)->current_frame)++;
}

/* standard GTK function */
static void
Dw_frameset_remove (DwContainer *container,
		     DwWidget    *widget)
{
  DwFrameset *frameset;
  DwFramesetChild *child;
  GList *children;
  
  g_return_if_fail (container != NULL);
  g_return_if_fail (DW_IS_FRAMESET (container));
  g_return_if_fail (widget != NULL);
  
  frameset = DW_FRAMESET (container);
  children = frameset->children;
  
  while (children)
    {
      child = children->data;
      children = children->next;
      
      if (child->widget == widget)
	{
	  gboolean was_visible = DW_WIDGET_VISIBLE (widget);
	  
      widget->parent = NULL;
	  
	  frameset->children = g_list_remove (frameset->children, child);

	  g_free (child);
	  if (was_visible && DW_WIDGET_VISIBLE (container))
	   p_Dw_widget_queue_resize (DW_WIDGET (container) , 0 , TRUE);
	  break;
	}
    }
}

/* standard GTK function */
static void
Dw_frameset_forall (DwContainer *container,
		  DwCallback	callback,
		  void*	callback_data)
{
  DwFrameset *frameset;
  DwFramesetChild *child;
  GList *children;
  
  g_return_if_fail (container != NULL);
  g_return_if_fail (DW_IS_FRAMESET (container));
  g_return_if_fail (callback != NULL);
  
  frameset = DW_FRAMESET (container);
  children = frameset->children;
  
  while (children)
    {
      child = children->data;
      children = children->next;
      
      (* callback) (child->widget, callback_data);
    }
}

/* standard GTK function */
static void
Dw_frameset_finalize (GObject *object)
{
  DwFrameset *frameset;
  g_return_if_fail (object != NULL);
  g_return_if_fail (DW_IS_FRAMESET (object));
  
  frameset = DW_FRAMESET (object);
  
  g_free (frameset->rows);
  g_free (frameset->cols);

 (* G_OBJECT_CLASS (parent_class)->finalize) (object);

    if (DW_WIDGET(frameset)->viewport->child != NULL)
         g_signal_emit_by_name (G_OBJECT(DW_WIDGET(frameset)->viewport->child), "destroy", 0);
    
    a_Dw_viewport_destroy(DW_WIDGET(frameset)->viewport);
    DestroyWindow(DW_WIDGET(frameset)->hwnd);
}


/* standard GTK function */
static void
Dw_frameset_draw (DwWidget    *widget,
                HDC    hdc,
		   DwRectangle *area)
{
  DwFrameset *frameset;
  DwFramesetChild *child;
  GList *children;
  DwRectangle child_area;
  
  g_return_if_fail (widget != NULL);
  g_return_if_fail (DW_IS_FRAMESET (widget));
  if (DW_WIDGET_VISIBLE (widget))/* && GTK_WIDGET_MAPPED (widget))*/
    {
      frameset = DW_FRAMESET (widget);
      children = frameset->children;
      while (children)
	{
	  child = children->data;
	  children = children->next;
	  
	  if (p_Dw_widget_intersect (child->widget, area, &child_area))
	  a_Dw_widget_draw (child->widget,hdc , &child_area);
	}
    }
}

/*
 * Parse a comma-separated list of %MultiLengths, and returns a GSList
 * of lenghts. The caller has to free the GSList.
 */
static GSList*
Dw_frameset_get_multi_length (const gchar *attr)
{
  GSList *list;
  gdouble value;
  gchar *end, *locale;
  Length length;

  g_return_val_if_fail(attr != NULL, NULL);

  /* Set 'C' locale to avoid parsing problems with float numbers */
  locale = g_strdup (setlocale (LC_NUMERIC, NULL));
  setlocale (LC_NUMERIC, "C");

  list = NULL;

  while(TRUE) {
    value = g_strtod (attr, &end);
    switch (*end) {
    case '%':
      length = LENGTH_CREATE_PERCENTAGE (value / 100);
      break;
      
    case '*':
      length = LENGTH_CREATE_RELATIVE (value);
      break;
      
    default:
      length = LENGTH_CREATE_ABSOLUTE ((gint) value);
      break;
    }

    list = g_slist_append(list, GINT_TO_POINTER(length));

    /* there MUST be a comma between values */
    if(!(end = strchr(end, ',')))
      break;
    /* valid %MultiLength characters: 0123456789%* */
    if(!(attr = strpbrk(end, "0123456789%*")))
      break;
  }

  setlocale (LC_NUMERIC, locale);
  g_free (locale);

  return list;
}

