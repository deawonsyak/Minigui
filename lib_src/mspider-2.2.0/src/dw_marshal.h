#ifndef __DW_MARSHAL_H__
#define __DW_MARSHAL_H__

#include <glib.h>
#include <glib-object.h>

void p_Dw_marshal_BOOL__INT_INT_UINT (GClosure     *closure,
                                      GValue       *return_value,
                                      guint         n_param_values,
                                      const GValue *param_values,
                                      gpointer      invocation_hint,
                                      gpointer      marshal_data);

void p_Dw_marshal_VOID__INT_INT (GClosure     *closure,
                                      GValue       *return_value,
                                      guint         n_param_values,
                                      const GValue *param_values,
                                      gpointer      invocation_hint,
                                      gpointer      marshal_data);

void p_Dw_marshal_BOOL__INT_INT_INT (GClosure     *closure,
                                      GValue       *return_value,
                                      guint         n_param_values,
                                      const GValue *param_values,
                                      gpointer      invocation_hint,
                                      gpointer      marshal_data);

void p_Dw_marshal_BOOL__INT_INT_INT_UINT (GClosure     *closure,
                                      GValue       *return_value,
                                      guint         n_param_values,
                                      const GValue *param_values,
                                      gpointer      invocation_hint,
                                      gpointer      marshal_data);

void p_Dw_marshal_BOOL__POINTER_POINTER (GClosure     *closure,
                                         GValue       *return_value,
                                         guint         n_param_values,
                                         const GValue *param_values,
                                         gpointer      invocation_hint,
                                         gpointer      marshal_data);
                                         
/*
 * Marshal fuctions for standard link signals.
 */
#define p_Dw_marshal_link_enter  p_Dw_marshal_BOOL__INT_INT_INT
#define p_Dw_marshal_link_button p_Dw_marshal_BOOL__INT_INT_INT_UINT


#endif /* __DW_MARSHAL_H__ */
