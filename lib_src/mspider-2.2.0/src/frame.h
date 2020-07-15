/*
 ** frame.h  definitions for frame
 **
 ** Copyright (C) 2005 Feynman Softwaver.
 **
 ** GPL License
 */
#ifndef __FRAME_H__
#define __FRAME_H__

#include <glib.h>

#include "dw_viewport.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

BOOL register_frame_control (void);
void unregister_frame_control (void);
#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __FRAME_H__ */

