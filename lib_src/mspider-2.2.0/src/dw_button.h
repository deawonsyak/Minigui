/* This module contains the dw_button widget, which is the "back end" to
   Web text widgets including html. */

#ifndef __DW_BUTTON_H__
#define __DW_BUTTON_H__

#include "dw_container.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#define DW_TYPE_BUTTON          (a_Dw_button_get_type())
#define DW_BUTTON(obj)          (G_TYPE_CHECK_INSTANCE_CAST ((obj), DW_TYPE_BUTTON, DwButton))
#define DW_BUTTON_CLASS(klass)  (G_TYPE_CHECK_CLASS_CAST ((klass), DW_TYPE_BUTTON, DwButtonClass))
#define DW_BUTTON_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS ((obj), DW_TYPE_BUTTON, DwButtonClass))

#define DW_IS_BUTTON(obj)       (G_TYPE_CHECK_INSTANCE_TYPE ((obj), DW_TYPE_BUTTON))

typedef struct _DwButton        DwButton;
typedef struct _DwButtonClass   DwButtonClass;

struct _DwButton
{
   DwContainer container;

   DwWidget *child;
   gboolean relief, in_button, pressed, sensitive;
};


struct _DwButtonClass
{
   DwContainerClass parent_class;

   void (*clicked)    (DwButton *button);
   void (*clicked_at) (DwButton *button,
                       gint32 x,
                       gint32 y);
};

GType      a_Dw_button_get_type      (void);
DwWidget*  a_Dw_button_new           (gint flags,
                                      gboolean relief);
void       a_Dw_button_set_sensitive (DwButton *button,
                                      gboolean sensitive);


#ifdef __cplusplus
}
#endif /* __cplusplus */


#endif /* __DW_BUTTON_H__ */
