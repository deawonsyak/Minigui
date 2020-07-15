/*
 *   This file is part of mGEff, a component for MiniGUI.
 * 
 *   Copyright (C) 2008~2018, Beijing FMSoft Technologies Co., Ltd.
 * 
 *   This program is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *   (at your option) any later version.
 * 
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 * 
 *   You should have received a copy of the GNU General Public License
 *   along with this program.  If not, see <http://www.gnu.org/licenses/>.
 * 
 *   Or,
 * 
 *   As this program is a library, any link to this program must follow
 *   GNU General Public License version 3 (GPLv3). If you cannot accept
 *   GPLv3, you need to be licensed from FMSoft.
 * 
 *   If you have got a commercial license of this program, please use it
 *   under the terms and conditions of the commercial license.
 * 
 *   For more information about the commercial license, please refer to
 *   <http://www.minigui.com/blog/minigui-licensing-policy/>.
 */
#ifndef _EFF_EFFECTORFLIP_H
#define _EFF_EFFECTORFLIP_H
#include "effector.h"

typedef struct _EffFlipCtxt{
    int mode;
    HGRAPHICS graphics;
    HPATH path;
}EffFlipCtxt;

MGEFF_EFFECTOR effflipeffector_init(MGEFF_EFFECTOR _effector);
void effflipeffector_finalize(MGEFF_EFFECTOR _effector);
int  effflipeffector_setproperty(MGEFF_EFFECTOR _effector, int property_id, int value);
void effflipeffector_begindraw(MGEFF_ANIMATION animation, MGEFF_EFFECTOR _effector);
void effflipeffector_enddraw(MGEFF_ANIMATION animation, MGEFF_EFFECTOR _effector);
void effflipeffector_ondraw(MGEFF_ANIMATION animation, MGEFF_EFFECTOR _effector, 
        HDC sink_dc, intptr_t id, void* value);
#endif