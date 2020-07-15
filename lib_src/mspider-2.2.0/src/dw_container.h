#ifndef __DW_CONTAINER_H__
#define __DW_CONTAINER_H__

#include "dw_widget.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#define DW_TYPE_CONTAINER           (a_Dw_container_get_type())
#define DW_CONTAINER(obj)           (G_TYPE_CHECK_INSTANCE_CAST ((obj), DW_TYPE_CONTAINER, DwContainer))
#define DW_CONTAINER_CLASS(klass)   (G_TYPE_CHECK_CLASS_CAST ((klass), DW_TYPE_CONTAINER, DwContainerClass))
#define DW_CONTAINER_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj), DW_TYPE_CONTAINER, DwContainerClass))

#define DW_IS_CONTAINER(obj)        (G_TYPE_CHECK_INSTANCE_TYPE ((obj), DW_TYPE_CONTAINER))

typedef void (*DwCallback) (DwWidget *widget, void* data);

typedef struct _DwContainer       DwContainer;
typedef struct _DwContainerClass  DwContainerClass;

struct _DwContainer
{
   DwWidget widget;
};


struct _DwContainerClass
{
   DwWidgetClass parent_class;

   void (* add)                 (DwContainer *container,
                                 DwWidget *child);
   void (* remove)              (DwContainer *container,
                                 DwWidget *widget);
   void (* forall)              (DwContainer *container,
                                 DwCallback callback,
                                 void* callbabck_data);
};

GType   a_Dw_container_get_type         (void);

void    a_Dw_container_add              (DwContainer *container,
                                         DwWidget *child);
void    a_Dw_container_forall           (DwContainer *container,
                                         DwCallback callback,
                                         void* callback_data);

void    Dw_container_remove             (DwContainer *container,
                                         DwWidget *child);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __DW_CONTAINER_H__ */

