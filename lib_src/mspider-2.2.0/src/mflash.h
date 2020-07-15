/*
 ** mflash.h  definitions for mflash
 **
 ** Copyright (C) 2005 Feynman Softwaver.
 **
 ** GPL License
 */
#ifndef __MFLASH_H__
#define __MFLASH_H__

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */
BOOL RegisterMyFlashPlayer(void);
void UnregisterFlashControl (void);
BOOL PrepareFlashPlay (char *buffer,long size,HWND hWnd);
#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __MFLASH_H__ */

