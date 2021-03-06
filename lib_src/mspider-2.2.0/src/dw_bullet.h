#ifndef __DW_BULLET_H__
#define __DW_BULLET_H__

#include "dw_widget.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#define DW_TYPE_BULLET          (a_Dw_bullet_get_type())
#define DW_BULLET(obj)          (G_TYPE_CHECK_INSTANCE_CAST ((obj), DW_TYPE_BULLET, DwBullet))
#define DW_BULLET_CLASS(klass)  (G_TYPE_CHECK_CLASS_CAST ((klass), DW_TYPE_BULLET, DwBulletClass))
#define DW_BULLET_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS ((obj), DW_TYPE_BULLET, DwBulletClass))

#define DW_IS_BULLET(obj)       (G_TYPE_CHECK_INSTANCE_TYPE ((obj), DW_TYPE_BULLET))

typedef struct _DwBullet      DwBullet;
typedef struct _DwBulletClass DwBulletClass;

struct _DwBullet
{
   DwWidget widget;

   gboolean selected[DW_HIGHLIGHT_NUM_LAYERS];
};

struct _DwBulletClass
{
   DwWidgetClass parent_class;
};

GType           a_Dw_bullet_get_type    (void);
DwWidget*       a_Dw_bullet_new         (void);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __DW_BULLET_H__ */
