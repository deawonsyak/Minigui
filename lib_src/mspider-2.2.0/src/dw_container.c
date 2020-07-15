/* * File: dw_container.c *
 *
 * Copyright (C) 2005-2006 Feynman Software
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include "dw_container.h"

static void Dw_container_init          (DwContainer *container);
static void Dw_container_class_init    (DwContainerClass *klass);

static void Dw_container_finalize (GObject *object);

enum
{
   ADD,
   REMOVE,
   LAST_SIGNAL
};

// static guint container_signals[LAST_SIGNAL] = { 0 };

static DwWidgetClass *parent_class;


/*
 * Standard GObject function
 */
GType a_Dw_container_get_type (void)
{
   static GType type = 0;

   if (!type) {
      static const GTypeInfo info = {
         sizeof (DwContainerClass),
         (GBaseInitFunc) NULL,
         (GBaseFinalizeFunc) NULL,
         (GClassInitFunc) Dw_container_class_init,
         (GClassFinalizeFunc) NULL,
         (gconstpointer) NULL,
         sizeof (DwContainer),
         0,
         (GInstanceInitFunc) Dw_container_init
      };

      type = g_type_register_static (DW_TYPE_WIDGET, "DwContainer", &info, 0);
   }

   return type;
}


/*
 * Standard GObject function
 */
static void Dw_container_init (DwContainer *container)
{
}


/*
 * Standard GObject function
 */
static void Dw_container_class_init (DwContainerClass *klass)
{
   GObjectClass *object_class;

   parent_class = g_type_class_peek_parent (klass);
   object_class = G_OBJECT_CLASS (klass);

#if 0
   container_signals[ADD] =
      gtk_signal_new ("add",
                      GTK_RUN_FIRST,
                      object_class->type,
                      GTK_SIGNAL_OFFSET (DwContainerClass, add),
                      gtk_marshal_NONE__POINTER,
                      GTK_TYPE_NONE, 1,
                      GTK_TYPE_WIDGET);
  container_signals[REMOVE] =
     gtk_signal_new ("remove",
                     GTK_RUN_FIRST,
                     object_class->type,
                     GTK_SIGNAL_OFFSET (DwContainerClass, remove),
                     gtk_marshal_NONE__POINTER,
                     GTK_TYPE_NONE, 1,
                     GTK_TYPE_WIDGET);
#endif

   object_class->finalize = Dw_container_finalize;
}


/*
 * Standard GObject function
 */
static void Dw_container_finalize (GObject *object)
{
   a_Dw_container_forall (DW_CONTAINER (object),
                          (DwCallback) g_object_unref,
                          NULL);

   G_OBJECT_CLASS (parent_class)->finalize (object);
}



/*
 * Add a widget in a "standard" way. Currently not used in mspider,
 * perhaps never needed.
 */
void a_Dw_container_add (DwContainer *container,
                         DwWidget *child)
{
   DwContainerClass *klass;

   klass = DW_CONTAINER_GET_CLASS (container);
   if (klass->add)
      (* (klass->add)) (container, child);
}


/*
 *
 */
void a_Dw_container_forall (DwContainer *container,
                            DwCallback   callback,
                            gpointer     callback_data)
{
   DwContainerClass *klass;

   klass = DW_CONTAINER_GET_CLASS (container);
   if (klass->forall) {
      (* (klass->forall)) (container, callback, callback_data);
   }

   /* The following code is handy for testing DwIterator: */

   /*
   DwIterator *it;

   if ((it = a_Dw_widget_iterator (DW_WIDGET (container),
                                   DW_CONTENT_WIDGET))) {
      while (a_Dw_iterator_next(it))
         callback (it->content.data.widget, callback_data);
      a_Dw_iterator_free (it);
   }
   */
}

void Dw_container_remove (DwContainer *container,
                          DwWidget *child)
{
   DwContainerClass *klass;

   klass = DW_CONTAINER_GET_CLASS (container);
   if (klass->remove)
      (* (klass->remove)) (container, child);
}


