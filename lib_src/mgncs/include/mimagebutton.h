/**
 * \file 
 * \author 
 * \date 
 *
 * \brief 
 *
 \verbatim

    This file is part of mGNCS, a component for MiniGUI.

    Copyright (C) 2008~2018, Beijing FMSoft Technologies Co., Ltd.

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.

    Or,

    As this program is a library, any link to this program must follow
    GNU General Public License version 3 (GPLv3). If you cannot accept
    GPLv3, you need to be licensed from FMSoft.

    If you have got a commercial license of this program, please use it
    under the terms and conditions of the commercial license.

    For more information about the commercial license, please refer to
    <http://www.minigui.com/blog/minigui-licensing-policy/>.

 \endverbatim
 */

#ifdef _MGNCSCTRL_IMAGEBUTTON

#ifndef _MGUI_NCSCTRL_IMAGEBUTTON_H
#define _MGUI_NCSCTRL_IMAGEBUTTON_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * \defgroup ControlButton mImageButton
 * @{
 */

/**
 *  \def NCSCTRL_IMAGEBUTTON
 *  \bref the name of image button control
 */
#define NCSCTRL_IMAGEBUTTON NCSCLASSNAME("imagebutton")

DECLARE_OBJECT(mImageButton)


#define mImageButtonHeader(clss) \
	mButtonHeader(clss)

#define mImageButtonClassHeader(clss, superCls) \
	mButtonClassHeader(clss, superCls)

typedef mButtonRenderer mImageButtonRenderer;

DEFINE_OBJECT(mImageButton, mButton)

enum mImageButtonProp {
	NCSP_IMGBTN_IMAGE = NCSP_BUTTON_IMAGE,
	NCSP_IMGBTN_TOOLIMAGEFILE = NCSP_BUTTON_MAX + 1,
	NCSP_IMGBTN_TOOLIMAGE,
	NCSP_IMGBTN_MAX
};

enum mImageButtonNotify{
	NCSN_IMGBTN_MAX = NCSN_BUTTON_MAX + 1
};

#ifdef __cplusplus
}
#endif

#endif
#endif		//_MGNCSCTRL_IMAGEBUTTON
