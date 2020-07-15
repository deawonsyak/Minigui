/*
 *   This file is part of MiniGUI, a mature cross-platform windowing 
 *   and Graphics User Interface (GUI) support system for embedded systems
 *   and smart IoT devices.
 * 
 *   Copyright (C) 2002~2018, Beijing FMSoft Technologies Co., Ltd.
 *   Copyright (C) 1998~2002, WEI Yongming
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
 *   <http://www.minigui.com/en/about/licensing-policy/>.
 */

#include "common.h"

#ifdef _MGINCORE_RES

//data of "skin_cpn_btn.gif"

unsigned char _mgir_gif_skin_cpn_btn_data[]={
	0x47,0x49,0x46,0x38,0x39,0x61,0x40,0x00,	0x80,0x00,0xF7,0x00,0x00,0x00,0x00,0x00,
	0xFF,0xD8,0x50,0xFE,0xFF,0x9D,0xE7,0xC4,	0x48,0xD4,0xB4,0x42,0xBE,0x93,0x00,0xAD,
	0x93,0x36,0xA0,0xA0,0xA0,0x95,0x73,0x00,	0x8A,0x8A,0x8A,0x4F,0x4F,0x4F,0x5F,0x60,
	0x5B,0xB1,0xB2,0x6D,0xFB,0xFC,0x9B,0xEF,	0xF0,0x94,0xE9,0xEA,0x90,0xC2,0xC3,0x78,
	0xBF,0xC0,0x76,0xBC,0xBD,0x74,0x98,0x99,	0x5E,0x93,0x94,0x5B,0xF4,0xF5,0x98,0xF2,
	0xF3,0x96,0xE0,0xE1,0x8B,0xD7,0xD8,0x86,	0xCB,0xCC,0x7E,0xBE,0xBF,0x76,0xB9,0xBA,
	0x73,0xA4,0xA5,0x66,0x7F,0x80,0x4F,0xB6,	0xB7,0x76,0xAC,0xAD,0x71,0x0F,0x0F,0x09,
	0x38,0x38,0x22,0x33,0x33,0x1F,0x6D,0x6D,	0x43,0x79,0x79,0x4B,0x69,0x69,0x41,0x66,
	0x66,0x3F,0x3F,0x3F,0x27,0x15,0x15,0x0D,	0x2D,0x2D,0x1C,0x29,0x29,0x1A,0x61,0x61,
	0x3E,0x55,0x55,0x42,0x0A,0x0A,0x08,0x37,	0x37,0x32,0x0B,0x0B,0x0A,0x61,0x61,0x5B,
	0x5D,0x5D,0x59,0x5A,0x58,0x36,0x5D,0x5C,	0x56,0x1E,0x1D,0x18,0x59,0x58,0x53,0x5C,
	0x5B,0x56,0x0B,0x09,0x00,0x0E,0x0C,0x04,	0x51,0x4B,0x31,0x20,0x1F,0x1B,0x5D,0x5B,
	0x53,0x61,0x5F,0x57,0x58,0x57,0x53,0xAE,	0x87,0x00,0xA8,0x82,0x00,0xA5,0x80,0x00,
	0xA0,0x7C,0x00,0x98,0x76,0x00,0x8E,0x6E,	0x00,0x89,0x6A,0x00,0x86,0x68,0x00,0x79,
	0x5E,0x00,0x75,0x5B,0x00,0x68,0x51,0x00,	0x5F,0x4A,0x00,0x5E,0x49,0x00,0x5A,0x46,
	0x00,0x51,0x3F,0x00,0x4E,0x3D,0x00,0x4C,	0x3B,0x00,0x48,0x38,0x00,0x47,0x37,0x00,
	0x43,0x34,0x00,0x3C,0x2F,0x00,0x36,0x2A,	0x00,0x2E,0x24,0x00,0x09,0x07,0x00,0xB4,
	0x8C,0x01,0xA4,0x81,0x08,0x5D,0x4A,0x06,	0x1F,0x19,0x02,0x4A,0x3B,0x06,0x8C,0x70,
	0x0F,0x85,0x6B,0x11,0x14,0x11,0x06,0x2E,	0x27,0x0E,0x62,0x53,0x1E,0xEA,0xC6,0x49,
	0xE4,0xC2,0x47,0xD7,0xB6,0x43,0xC4,0xA6,	0x3D,0xBD,0xA0,0x3B,0xBA,0x9E,0x3A,0xB4,
	0x98,0x38,0xB1,0x96,0x37,0xAE,0x94,0x36,	0xAB,0x91,0x35,0xA4,0x8B,0x33,0x8D,0x78,
	0x2C,0x8B,0x76,0x2B,0x88,0x73,0x2A,0x87,	0x73,0x2A,0x7A,0x68,0x26,0x74,0x62,0x24,
	0x6E,0x5D,0x22,0x67,0x58,0x20,0x5A,0x4C,	0x1C,0x57,0x4A,0x1B,0x2D,0x26,0x0E,0xFC,
	0xD5,0x4F,0xE4,0xC1,0x48,0xE1,0xBF,0x47,	0xDB,0xBA,0x45,0xCC,0xAD,0x40,0xBF,0xA2,
	0x3C,0xB9,0x9D,0x3A,0xB1,0x96,0x38,0xA8,	0x8F,0x35,0xA0,0x88,0x32,0x99,0x82,0x30,
	0x95,0x7F,0x2F,0x90,0x7A,0x2D,0x83,0x6F,	0x29,0x69,0x59,0x21,0x5F,0x51,0x1E,0x5C,
	0x4E,0x1D,0x46,0x3C,0x16,0xF2,0xCD,0x4D,	0xDE,0xBD,0x47,0x78,0x66,0x26,0x2F,0x28,
	0x0F,0x29,0x23,0x0D,0x4E,0x42,0x19,0x7D,	0x6A,0x29,0x71,0x61,0x25,0xC6,0xA9,0x43,
	0xC1,0xA5,0x42,0x25,0x20,0x0D,0xA7,0x90,	0x3E,0xB2,0x99,0x43,0x9E,0x89,0x3D,0x48,
	0x40,0x22,0x36,0x34,0x2D,0xBC,0x91,0x00,	0xA3,0x7E,0x00,0xA2,0x7D,0x00,0x8C,0x6C,
	0x00,0x8B,0x6B,0x00,0x89,0x69,0x00,0x84,	0x66,0x00,0x7E,0x61,0x00,0x7B,0x5F,0x00,
	0x77,0x5C,0x00,0x72,0x58,0x00,0x70,0x56,	0x00,0x63,0x4C,0x00,0x56,0x42,0x00,0x4B,
	0x3A,0x00,0x4A,0x39,0x00,0x40,0x31,0x00,	0x33,0x27,0x00,0x2A,0x20,0x00,0x26,0x1D,
	0x00,0x25,0x1C,0x00,0x22,0x1A,0x00,0x1E,	0x17,0x00,0x91,0x70,0x01,0x8F,0x6F,0x03,
	0x1A,0x14,0x01,0xA1,0x7E,0x09,0x81,0x65,	0x09,0x71,0x5A,0x0F,0x6C,0x57,0x11,0x52,
	0x45,0x19,0x80,0x6C,0x28,0x6C,0x5B,0x22,	0x66,0x56,0x20,0x40,0x36,0x14,0x39,0x30,
	0x12,0x33,0x2B,0x10,0x34,0x2C,0x11,0xDA,	0xB9,0x49,0xD5,0xB5,0x49,0x40,0x39,0x22,
	0x24,0x1B,0x00,0x10,0x0C,0x00,0x33,0x30,	0x28,0x5B,0x59,0x53,0x5F,0x5D,0x57,0x5A,
	0x59,0x56,0x57,0x56,0x53,0x0B,0x0A,0x09,	0x9F,0x9F,0x9F,0x9C,0x9C,0x9C,0x9A,0x9A,
	0x9A,0x99,0x99,0x99,0x97,0x97,0x97,0x94,	0x94,0x94,0x93,0x93,0x93,0x8F,0x8F,0x8F,
	0x8D,0x8D,0x8D,0x8B,0x8B,0x8B,0x89,0x89,	0x89,0x87,0x87,0x87,0x83,0x83,0x83,0x80,
	0x80,0x80,0x7F,0x7F,0x7F,0x7A,0x7A,0x7A,	0x78,0x78,0x78,0x77,0x77,0x77,0x75,0x75,
	0x75,0x73,0x73,0x73,0x71,0x71,0x71,0x6E,	0x6E,0x6E,0x69,0x69,0x69,0x67,0x67,0x67,
	0x64,0x64,0x64,0x60,0x60,0x60,0x5D,0x5D,	0x5D,0x5A,0x5A,0x5A,0x58,0x58,0x58,0x57,
	0x57,0x57,0x53,0x53,0x53,0x50,0x50,0x50,	0x4E,0x4E,0x4E,0x4C,0x4C,0x4C,0x48,0x48,
	0x48,0x46,0x46,0x46,0x45,0x45,0x45,0x42,	0x42,0x42,0x40,0x40,0x40,0x3E,0x3E,0x3E,
	0x39,0x39,0x39,0x36,0x36,0x36,0x31,0x31,	0x31,0x2F,0x2F,0x2F,0x2B,0x2B,0x2B,0x28,
	0x28,0x28,0x22,0x22,0x22,0x20,0x20,0x20,	0x1E,0x1E,0x1E,0x1C,0x1C,0x1C,0x19,0x19,
	0x19,0x17,0x17,0x17,0x0D,0x0D,0x0D,0x08,	0x08,0x08,0xFF,0xFF,0xFF,0x21,0xF9,0x04,
	0x01,0x00,0x00,0xFF,0x00,0x2C,0x00,0x00,	0x00,0x00,0x40,0x00,0x80,0x00,0x00,0x08,
	0xFF,0x00,0x15,0x90,0x03,0xB7,0xAD,0x1A,	0x34,0x66,0xC9,0x0E,0x28,0x73,0x96,0xE0,
	0x5A,0xB7,0x70,0xE5,0x04,0x12,0x34,0x88,	0x50,0x21,0x43,0x87,0x10,0x25,0x16,0x3C,
	0x98,0x70,0x61,0xC3,0x87,0x11,0x07,0x6E,	0xAC,0xE8,0x11,0x63,0x44,0x5C,0x8A,0x6E,
	0xA9,0x5C,0xC9,0x52,0xA5,0x31,0x05,0x28,	0x5B,0xCA,0xBC,0xF5,0x32,0xE6,0x4C,0x96,
	0x35,0x53,0xDE,0x5C,0xF9,0x32,0x50,0x9F,	0x3D,0x40,0x83,0x0A,0x05,0x3A,0x43,0x81,
	0xCF,0xA1,0x48,0xF7,0x14,0x3D,0x9A,0x54,	0xE8,0xD2,0x9F,0x4D,0x83,0x16,0xED,0x23,
	0x06,0x8C,0xD5,0xAB,0x58,0xAD,0xF2,0x50,	0x40,0x35,0xAB,0x57,0x30,0x5B,0xBB,0x7E,
	0xC5,0x1A,0xB6,0xEA,0xD8,0xAB,0x5B,0xF7,	0x80,0x09,0xC0,0x36,0x80,0x8C,0x1C,0x70,
	0xE3,0x22,0x4A,0xBB,0xB6,0xAD,0x5D,0xBB,	0x74,0x03,0x90,0x01,0xC4,0xB7,0x2F,0xA0,
	0x31,0x6C,0xF3,0x06,0xD8,0x63,0xC7,0x6E,	0x99,0xB6,0x82,0xD9,0x5E,0xA2,0xF4,0xAB,
	0x31,0xA5,0x7A,0x73,0x15,0xA8,0xBD,0x4B,	0x39,0x40,0xDE,0x39,0x00,0x32,0x6B,0x06,
	0x40,0x28,0xB0,0xE4,0xB5,0x79,0x00,0x14,	0x6A,0x1B,0x07,0x80,0x1E,0xCF,0x93,0x75,
	0xF1,0xE2,0xF5,0x82,0xD1,0x6A,0x3A,0xBE,	0x20,0x27,0xAE,0x8C,0x7A,0xED,0x9C,0x3D,
	0x7A,0x72,0xE7,0x36,0xD3,0xD9,0xF2,0x67,	0xB6,0x81,0x00,0x7C,0x09,0xA0,0x08,0xC0,
	0x20,0xC4,0xBF,0x71,0x7C,0x01,0xA4,0xC3,	0x11,0x20,0x5B,0x75,0x62,0x47,0x9E,0x4C,
	0x1B,0xF9,0xE4,0x39,0x88,0xEE,0xE2,0xE9,	0x9D,0x58,0x10,0x80,0x5D,0x00,0x1C,0xE1,
	0xFF,0x4D,0xAE,0x26,0xC0,0xA5,0x7A,0xF5,	0xF8,0xF9,0x43,0x3F,0xBD,0x6E,0x75,0xDF,
	0xD7,0xB3,0xDB,0xDD,0x5E,0xDB,0xEE,0x21,	0x00,0xBD,0xEE,0xE6,0xC5,0x51,0xBE,0xAD,
	0x9B,0x3A,0xD6,0xB9,0x57,0xDD,0x65,0xF2,	0xB5,0x45,0x1F,0x7C,0xEE,0x79,0x07,0xDE,
	0x1B,0xE3,0x4D,0x86,0xC3,0x20,0xBA,0xE9,	0x81,0x0B,0x80,0xF5,0x05,0xD0,0xC8,0x6A,
	0x18,0xEA,0x12,0x60,0x00,0xD8,0x69,0xC7,	0xDD,0x6F,0x01,0x00,0x22,0x5C,0x00,0x8E,
	0x00,0x70,0xCB,0x86,0xAA,0x61,0xF8,0xDA,	0x86,0x16,0xAA,0xC8,0x8B,0x86,0xF5,0xDD,
	0x16,0xA1,0x1E,0xBC,0xD5,0x87,0x48,0x17,	0xA3,0xB1,0xF5,0x08,0x0E,0x77,0x54,0xF8,
	0xDE,0x6C,0xB4,0xE5,0x05,0x48,0x2E,0x44,	0x16,0x99,0x4B,0x7F,0x82,0x25,0x52,0x58,
	0x5B,0x79,0xB0,0xF8,0x23,0x88,0x4F,0x52,	0x17,0xA5,0x80,0x41,0x42,0x39,0xA0,0x95,
	0x55,0x4A,0x79,0xA5,0x96,0x55,0x8A,0x44,	0x51,0x47,0x17,0x81,0xA4,0xD1,0x97,0x16,
	0x7D,0x94,0x91,0x97,0x1C,0x95,0x69,0xD2,	0x98,0x69,0x96,0x24,0x26,0x9A,0x24,0x85,
	0x79,0xE6,0x44,0x6D,0xCA,0x19,0x12,0x9D,	0x71,0x9A,0x79,0xE7,0x48,0x60,0xEA,0xA9,
	0xC0,0x08,0x24,0x74,0x20,0xE8,0xA0,0x84,	0x0A,0xFA,0x12,0xA0,0x85,0x26,0xDA,0xC1,
	0xA1,0x81,0x2A,0x4A,0x28,0xA3,0x8E,0x3E,	0xAA,0x00,0x05,0x0C,0x68,0x60,0xE9,0xA5,
	0x98,0x5A,0x1A,0xC3,0xA4,0x95,0x66,0xEA,	0xE9,0xA6,0x94,0x7A,0xFA,0x29,0xA7,0xA2,
	0x66,0xBA,0x29,0x03,0x18,0x3C,0xA0,0xEA,	0xAA,0xAC,0xAA,0xBA,0x80,0x02,0xA8,0xB6,
	0xFF,0x2A,0xEB,0x03,0xAF,0xC6,0x3A,0x2B,	0xAB,0xB5,0xA6,0x7A,0xEB,0xAA,0xAF,0x6A,
	0xF0,0x80,0x00,0xC0,0x0A,0xB0,0x02,0x0B,	0xC4,0x16,0x5B,0x01,0x0C,0x0A,0xF8,0x1A,
	0xEC,0xB2,0xCC,0x22,0xAB,0xAC,0x04,0x13,	0x44,0x2B,0xED,0x04,0x10,0x00,0xEB,0xEC,
	0xAF,0x02,0x44,0xD0,0xC0,0xB2,0x1B,0x04,	0x7B,0xED,0xB2,0x2E,0x7C,0x80,0xC1,0xB8,
	0x1E,0xD4,0x63,0xC1,0xB7,0xCC,0xA6,0xFB,	0xAD,0x0C,0x9B,0x69,0x56,0x82,0xB5,0xC9,
	0xFE,0x8A,0x01,0x00,0x2B,0x04,0x3B,0x02,	0x00,0x19,0xC0,0xAB,0xEC,0x09,0x22,0x88,
	0xF0,0x82,0x0A,0xFD,0xCA,0x80,0x81,0xB9,	0xE8,0xA6,0xBB,0xEC,0xBA,0x11,0x64,0xA0,
	0xB0,0xC2,0x0C,0xBC,0x2B,0x00,0xBA,0x14,	0xD0,0x2B,0x00,0x09,0x00,0x74,0xE0,0x6D,
	0xBC,0x02,0x80,0xB0,0xC2,0x04,0xF8,0x90,	0x30,0x41,0x0E,0x29,0x0C,0x7C,0x2E,0xC6,
	0x06,0x1F,0x8C,0xB1,0x0C,0x0E,0x30,0x7B,	0x81,0xC3,0x05,0x47,0x1C,0x42,0xC5,0x26,
	0x2B,0x0B,0x02,0x07,0x02,0xB8,0x80,0x9E,	0x7A,0xE8,0x8D,0xAC,0x6C,0xC9,0x17,0x2B,
	0x8B,0xB2,0xCA,0x2C,0x93,0x0C,0x6C,0x0E,	0x00,0x88,0xD0,0x2C,0xC6,0x33,0x2F,0xDB,
	0x41,0x0A,0x3D,0x63,0xCB,0xF3,0xC3,0x27,	0xA7,0xBC,0xEC,0xCA,0xFA,0x3A,0xED,0x32,
	0x00,0x24,0xC4,0xFC,0x2B,0x08,0x1D,0x2C,	0x9C,0xC1,0x08,0x4C,0x57,0x3D,0x74,0xBF,
	0x64,0x9F,0xD0,0xB4,0x00,0x3F,0x4F,0x1D,	0xF4,0xCE,0x13,0x48,0xDC,0x01,0xCC,0x62,
	0xF3,0x4B,0x76,0xC0,0x67,0x0B,0x90,0xC3,	0xDC,0x22,0x98,0x2D,0xB6,0x0C,0x09,0x7B,
	0xFF,0xDD,0xB0,0xD8,0x16,0xA0,0x50,0x2F,	0xB0,0x23,0x80,0x70,0x81,0xD8,0x4F,0x43,
	0xBD,0xF3,0xD3,0xDF,0x4E,0x60,0xC2,0xE3,	0x90,0x9B,0x40,0xB3,0xE2,0xD8,0x5E,0xB0,
	0x6D,0xB0,0x18,0xD4,0xCD,0xB8,0xD0,0x3C,	0x17,0xDC,0x39,0xE7,0x25,0x7B,0x1E,0x3A,
	0xE8,0x06,0x8B,0x5E,0x3A,0xE9,0xEA,0xB2,	0x99,0xE7,0x9A,0x70,0xF6,0xC9,0x3A,0x9E,
	0xAE,0xBF,0x09,0xBB,0x9A,0xB2,0xF3,0x49,	0xFB,0x9C,0xB6,0xBB,0x89,0x3B,0x99,0xBA,
	0xEF,0xC9,0xBB,0x9D,0x0A,0x30,0xB1,0x44,	0x12,0xC4,0x17,0x6F,0x3C,0xF1,0x3D,0x04,
	0x3F,0xFC,0xF1,0xCC,0x27,0x2F,0x3C,0xF3,	0xCD,0x2B,0x0F,0xFD,0xF1,0xC9,0x8F,0xE2,
	0xC9,0x10,0xD8,0x67,0xAF,0x3D,0xF6,0xC4,	0x28,0x60,0xFD,0xF6,0xE0,0x0F,0xD1,0xFD,
	0xF7,0xE1,0x6B,0x3F,0xFE,0xF5,0xE5,0x67,	0xDF,0xBD,0x27,0x41,0xF8,0xE0,0xFE,0xFB,
	0xF0,0xBB,0xBF,0x83,0x02,0xEC,0xC7,0x6F,	0xBF,0x0F,0xF3,0xD7,0x7F,0x3F,0xFC,0xF9,
	0xB7,0xBF,0xFF,0xFB,0xF3,0x1B,0x82,0x0F,	0x0A,0x40,0xC0,0x02,0xD8,0xC2,0x12,0x08,
	0x4C,0xA0,0x15,0x02,0x38,0xC0,0x02,0x3A,	0xD0,0x81,0x0C,0x2C,0xC0,0x10,0x44,0x41,
	0xC1,0x0A,0x8A,0xE2,0x15,0x04,0x8C,0xA0,	0x04,0x31,0xE1,0x40,0x4E,0x14,0x50,0x83,
	0x04,0x1C,0x06,0x17,0x64,0x41,0xC2,0x2D,	0xD4,0x63,0x81,0x0A,0x10,0xE0,0x03,0x57,
	0x58,0x80,0x08,0x46,0xA1,0x5D,0x99,0x69,	0x42,0x06,0x53,0x38,0xC0,0x4C,0x00,0xE0,
	0x09,0x05,0x64,0x02,0x00,0x84,0x30,0x43,	0x15,0x52,0x61,0x15,0xAB,0x68,0x41,0x16,
	0xFF,0x80,0x88,0x8A,0x2B,0x9C,0x10,0x84,	0x2C,0xEC,0xE1,0x00,0xA3,0x30,0x04,0x21,
	0x38,0xD1,0x89,0x45,0x90,0x61,0x0B,0x69,	0x48,0xC0,0x51,0x00,0xE0,0x14,0x05,0x58,
	0x02,0x00,0x48,0xF1,0x41,0x2A,0xDE,0xE0,	0x14,0xA2,0xA0,0x01,0x16,0x44,0x21,0x85,
	0x56,0x18,0x11,0x85,0x2A,0x4C,0x62,0x17,	0x55,0x18,0x05,0x2B,0x3C,0xF0,0x07,0x52,
	0x04,0x21,0x12,0x00,0xA0,0x0A,0x00,0x28,	0x01,0x82,0x5E,0x04,0x45,0x01,0x86,0x71,
	0xB3,0xF5,0x1C,0x91,0x8A,0x6A,0x54,0x62,	0x01,0xDA,0xF8,0xC6,0x38,0x02,0x92,0x80,
	0x53,0x00,0xC0,0x2A,0x1E,0x18,0xC1,0x1B,	0xE8,0xB1,0x80,0xA4,0x68,0xC5,0x1A,0x1B,
	0x18,0x48,0x17,0xBA,0xD1,0x81,0x70,0x14,	0x24,0x01,0xE7,0x58,0xC7,0x25,0xE0,0x51,
	0x85,0x37,0x20,0xC5,0x13,0x85,0xC0,0x04,	0x49,0x6A,0x72,0x0A,0x40,0x4C,0x25,0x15,
	0x26,0x39,0xC8,0x4B,0x16,0x30,0x93,0x53,	0x4C,0xA3,0x28,0xAE,0x58,0x00,0x25,0x00,
	0x20,0x09,0xAC,0xFC,0x61,0x2A,0x89,0xC8,	0xCA,0x02,0xA0,0x72,0x97,0xAB,0x14,0x24,
	0x13,0x47,0x29,0x84,0x28,0x0A,0xD2,0x0A,	0xC2,0xC0,0x21,0x01,0x4B,0x71,0x83,0x1F,
	0x68,0xB2,0x92,0x87,0x54,0x63,0x04,0x8F,	0xE0,0x84,0x6A,0x5A,0xD3,0x09,0x8F,0xD4,
	0x20,0x10,0x38,0x58,0xC0,0x4C,0xF4,0x12,	0x9A,0x69,0x04,0x27,0x25,0xA5,0x19,0xCD,
	0x24,0x22,0xD1,0x9C,0xE5,0x64,0xE1,0x39,	0xD5,0x99,0xCE,0x15,0xCE,0xAF,0x75,0xB7,
	0xF3,0x5D,0x9D,0xFC,0x04,0xCF,0xDE,0xA9,	0x2E,0x76,0xBB,0x9B,0xE7,0xEB,0x72,0x07,
	0xFF,0xBC,0x7A,0xF6,0x73,0x76,0xF6,0x74,	0x87,0x3A,0xD0,0x41,0xD0,0x82,0x1A,0x94,
	0xA0,0xE6,0x50,0x80,0x40,0x0F,0xCA,0x50,	0x74,0x24,0x74,0xA1,0x0D,0x35,0xE8,0x43,
	0x07,0x1A,0xD1,0x82,0x26,0x74,0x1C,0xDE,	0xC8,0x86,0x46,0x37,0xCA,0x51,0x8D,0x92,
	0x43,0x01,0x18,0xED,0xA8,0x48,0xB3,0xF1,	0xD1,0x90,0x8E,0x94,0xA3,0x25,0xCD,0xE8,
	0x49,0x37,0xFA,0x51,0x6F,0x50,0xE3,0x19,	0x30,0x8D,0xA9,0x4C,0x61,0xDA,0xD2,0x97,
	0xCE,0xF4,0xA6,0x35,0xBD,0x29,0x4E,0x15,	0xE0,0x52,0x9D,0xCE,0xF4,0xA3,0xD9,0x78,
	0xC6,0x01,0x86,0x7A,0x80,0x75,0xB8,0xE3,	0xA8,0x48,0x5D,0x06,0x50,0x85,0x4A,0xD4,
	0xA6,0x36,0x75,0xA9,0x07,0xD0,0x86,0x38,	0xA6,0x4A,0x55,0x71,0x60,0x63,0xA8,0x50,
	0x3D,0x40,0x36,0x12,0x42,0xD4,0x6D,0x10,	0x35,0xAB,0x43,0x9D,0x07,0x37,0xA6,0x41,
	0xD6,0x6C,0xD4,0x83,0x19,0x60,0x75,0xEA,	0x53,0x15,0x10,0xD4,0x03,0xC4,0x03,0x86,
	0x00,0x70,0x07,0x56,0xD9,0x2A,0xD4,0x69,	0x00,0x00,0x1E,0x44,0x65,0x07,0x00,0xAC,
	0x31,0xD7,0xB6,0xDA,0x03,0x1F,0xF8,0x78,	0xC1,0x3E,0x00,0x2B,0x0F,0x69,0x9C,0x35,
	0xAD,0x6A,0xED,0xAB,0x50,0xE3,0x91,0x0D,	0x6B,0x38,0xD6,0xB1,0xDD,0x90,0xEB,0x01,
	0xB2,0x3A,0x8E,0xBB,0x1E,0x40,0x1D,0x00,	0x38,0xC7,0x57,0xE9,0x7A,0x80,0x16,0xC0,
	0x43,0x1C,0xF9,0x50,0x80,0x38,0xE6,0xA1,	0x0F,0xC3,0xA2,0x95,0xB3,0x89,0x5D,0x6B,
	0x5B,0xE3,0xD1,0x0C,0xA7,0x46,0x43,0xB2,	0x60,0x25,0x07,0x00,0xEE,0x01,0x00,0x05,
	0xFF,0xA8,0x56,0xA8,0x2D,0xF8,0xC6,0x01,	0xE6,0xD1,0x47,0xF4,0x9C,0xB6,0xAD,0xA9,
	0xDD,0xEC,0x6A,0x5B,0xDB,0xD4,0xD7,0x2A,	0xB6,0xA9,0xF4,0x98,0xAD,0x53,0xA1,0x9A,
	0xDB,0xA6,0x1E,0x43,0x1F,0xC2,0x65,0x6A,	0x70,0x27,0xCB,0x59,0xD6,0xBA,0x16,0xB6,
	0xA8,0x9D,0xEC,0x6C,0x01,0x90,0x8E,0xDB,	0x76,0xF6,0x1C,0x8F,0xB5,0x46,0x3B,0xA0,
	0x7B,0xDC,0x03,0xD0,0x03,0xB0,0xE8,0xB5,	0x47,0x74,0xDD,0x4A,0x5C,0xA2,0x1A,0x97,
	0xBA,0xC0,0x15,0x87,0x65,0x15,0x00,0x00,	0x74,0xAC,0xF7,0xAF,0xE8,0x25,0xEC,0x7A,
	0xCD,0x9B,0x5F,0x7C,0xA8,0xF7,0xB8,0x8C,	0x0D,0xAF,0x35,0x22,0x7B,0x5C,0x66,0xF4,
	0x03,0xAF,0x43,0x5D,0x47,0x0B,0xA0,0x51,	0xDE,0xE9,0x22,0x36,0xB1,0x50,0x0D,0xC7,
	0x3B,0x26,0x4C,0xE1,0x77,0xE8,0x16,0xBE,	0x4C,0x95,0x06,0x57,0x87,0x3A,0x8D,0xFD,
	0x3A,0x38,0xBB,0xC1,0x7D,0xB0,0x5A,0x45,	0xBC,0x5C,0x10,0xA7,0x96,0xC4,0xDE,0x9D,
	0x2E,0x86,0x55,0x4C,0x5D,0x7F,0xD2,0x13,	0xA0,0xFF,0xE4,0xE7,0x8B,0x65,0xBC,0xCF,
	0xDF,0xCD,0xD8,0xC6,0x35,0xD6,0x67,0xED,	0x70,0x2C,0xA6,0x2F,0xC4,0x01,0x0E,0x40,
	0x0E,0xB2,0x90,0x81,0xFC,0x12,0x1F,0x0F,	0xF9,0xC8,0x70,0x28,0xF2,0x8F,0x91,0x2C,
	0x64,0x25,0x33,0xB9,0xC9,0x0A,0x70,0xC3,	0x1F,0x0C,0x40,0xE5,0x2A,0x5B,0x99,0xCA,
	0x36,0x88,0xF2,0x94,0xAF,0xCC,0xE5,0x2C,	0x4B,0x99,0xCB,0x5D,0xD6,0x32,0x98,0xAF,
	0x9C,0xE5,0x3F,0x8C,0x81,0x00,0x68,0x4E,	0xB3,0x9A,0xD1,0x5C,0x14,0x33,0xAF,0xF9,
	0xFF,0xCD,0x04,0x68,0xF3,0x99,0xE1,0xAC,	0x66,0x39,0xD3,0xB9,0xCE,0x0A,0x30,0x00,
	0x01,0x06,0xC0,0xE7,0x01,0xBC,0x25,0x2E,	0x70,0x49,0x44,0x31,0xF2,0xBC,0xE7,0x3E,
	0x1B,0xDA,0xD0,0x83,0xD6,0xF3,0x00,0xD2,	0xC0,0x86,0x46,0x3B,0x9A,0x0D,0x67,0xE0,
	0x73,0xA2,0x0B,0x8D,0x86,0x30,0x18,0xDA,	0x0F,0x7D,0x9E,0xB4,0xA1,0x2F,0x51,0x89,
	0x48,0x78,0x7A,0x12,0xF5,0xC8,0x83,0xA6,	0x0F,0x4D,0x6A,0x4D,0xDB,0x02,0xAE,0x85,
	0x90,0x34,0xA1,0x07,0x30,0x06,0x00,0xD0,	0xA1,0xCF,0xB9,0x00,0x00,0x1F,0x54,0xAD,
	0xE8,0x5D,0x78,0xC1,0x0B,0xC8,0x90,0xC4,	0xAD,0x1B,0x01,0x89,0x50,0x8F,0x9A,0xD4,
	0x88,0x5E,0xB5,0x2D,0xD0,0xC0,0x87,0x62,	0x17,0x5B,0x0D,0xA9,0x1E,0xC0,0xA8,0xDB,
	0x00,0x80,0x39,0x0C,0xA0,0x34,0x8A,0xC8,	0xF4,0xAA,0x71,0x30,0x07,0x36,0xE8,0xE0,
	0x11,0x6C,0x38,0x04,0x23,0x7A,0x2D,0xEA,	0x55,0x03,0x3B,0xD8,0x8A,0xB6,0x85,0x18,
	0x0E,0xAD,0x87,0x64,0xFF,0x7A,0x10,0x00,	0xE0,0x05,0x00,0x1E,0x01,0xEE,0x3D,0xE3,
	0x20,0x10,0x03,0x38,0x4F,0x7A,0xFC,0xD8,	0x6D,0x45,0x7F,0x5B,0xDA,0xE1,0x1E,0xB7,
	0xA1,0xCB,0x4D,0xEB,0x42,0xF3,0x59,0x17,	0x00,0x58,0xC4,0xA1,0x35,0xFD,0x6E,0x43,
	0xBF,0x81,0x11,0xF8,0xF6,0xF7,0xBD,0x4D,	0xAD,0xEF,0x3E,0xF3,0x5B,0xD9,0xDE,0x1E,
	0x00,0xBA,0xD5,0x1D,0x87,0x76,0x0F,0x00,	0x07,0x8A,0x30,0x36,0x1F,0xBE,0x80,0xF0,
	0x7E,0xF3,0xF9,0x10,0xB7,0x0E,0xF9,0x2E,	0x12,0x3E,0x00,0x71,0x93,0xDB,0xDC,0xDE,
	0xFF,0x66,0x43,0xB3,0x07,0xF0,0x08,0x00,	0xC0,0x81,0xE4,0xB6,0x0E,0xF9,0xAE,0x49,
	0x3E,0x00,0x90,0xCB,0x7C,0xE4,0x1E,0x1F,	0xB6,0xC6,0xF9,0x80,0x6C,0x8F,0xE7,0xA1,
	0x0B,0xAF,0xE6,0xB3,0x1C,0x70,0xA0,0x07,	0x8F,0xDF,0xDB,0xE8,0x47,0xD7,0xF4,0x1A,
	0x0C,0xC1,0xF4,0xA6,0x1B,0x02,0xDE,0x10,	0xB7,0xB7,0x1E,0x2C,0xDD,0xE7,0x31,0xD0,
	0x3C,0xE9,0x11,0x5F,0x78,0xD6,0xBF,0xFD,	0x6B,0xAD,0xDB,0x1B,0xEB,0x5F,0xF7,0xBA,
	0xC2,0xB9,0xBE,0x75,0x60,0x0F,0xDA,0xC5,	0x39,0x5E,0xDD,0x8E,0x75,0x9C,0x4F,0xB5,
	0xB7,0x1D,0x9F,0xF2,0x74,0x7B,0xDC,0xE1,	0x7E,0xCF,0x78,0x2A,0xC0,0xC8,0x4F,0x26,
	0xF2,0xDD,0x97,0x9C,0x77,0x27,0xE7,0x3D,	0xC9,0x7B,0xFF,0x3B,0xE0,0xBF,0x3C,0xE6,
	0x2A,0x7B,0x79,0xCB,0x85,0x37,0xC0,0xE1,	0x13,0x8F,0x65,0x31,0x33,0xBE,0xCC,0x73,
	0xBE,0x73,0x9C,0x15,0xE0,0x66,0xC9,0x4F,	0xBE,0xF2,0x92,0xB7,0xB3,0xE5,0x8B,0x12,
	0xF6,0x3F,0x03,0x5A,0xD0,0x65,0x2F,0xF5,	0xAA,0x19,0xFD,0xE8,0x46,0x47,0x3A,0xEA,
	0x94,0xA6,0x3A,0x9F,0x31,0x8D,0xF4,0x78,	0x77,0xFA,0xD3,0xBE,0x0E,0xFD,0xC0,0x85,
	0x8D,0x6A,0x8F,0xB7,0x3A,0xE8,0x03,0x88,	0xF5,0xAC,0x51,0x3F,0x80,0x98,0xE7,0x7A,
	0xD7,0xDC,0xEE,0x3A,0xD9,0xC3,0x4D,0x6C,	0x8D,0xF7,0x9C,0xF7,0x03,0x60,0xB6,0xB3,
	0xA1,0x4D,0x72,0x6A,0x5B,0x1B,0xDB,0xDA,	0x0E,0xBE,0xEC,0x2D,0x6E,0xF2,0x7D,0xA3,
	0xFC,0xEB,0x13,0x5F,0xB7,0xC5,0x0B,0x2E,	0x6F,0x9C,0xC5,0x3E,0xEC,0xC3,0xDF,0x73,
	0xFF,0xF5,0x1D,0x7E,0x7D,0x85,0x03,0x5C,	0xE0,0xDB,0x87,0x3A,0x9F,0x0F,0x7E,0x75,
	0xB1,0x97,0xBC,0xE1,0x7C,0x7E,0xF8,0xB9,	0xD3,0x0D,0x80,0x8A,0x37,0x3F,0xE3,0xC6,
	0xE6,0x38,0xCD,0x6D,0x2E,0x72,0x92,0x8F,	0x3F,0xFE,0xE5,0xC7,0x67,0x2A,0xE7,0x6C,
	0x2D,0xF7,0x72,0x1E,0x17,0x73,0x32,0xD7,	0x08,0xFB,0x27,0x73,0x5E,0x80,0x73,0xBC,
	0xA7,0x73,0xC6,0x17,0x80,0x3F,0x87,0x7B,	0x43,0x57,0x74,0xC8,0x07,0x76,0x63,0x67,
	0x76,0xAB,0xB6,0x74,0x4E,0xC7,0x74,0x50,	0x37,0x6A,0x53,0x67,0x68,0x56,0xD7,0x7A,
	0xE1,0x77,0x74,0x24,0xA8,0x81,0xE0,0x87,	0x82,0x19,0x28,0x7A,0x29,0xC8,0x82,0x2B,
	0x38,0x7B,0x2D,0x38,0x7B,0x68,0xB7,0x76,	0x72,0x57,0x77,0xF6,0x34,0x83,0x6F,0x67,
	0x77,0x38,0x38,0x77,0x3A,0x08,0x63,0x7E,	0x82,0x0A,0x50,0x60,0x0A,0x42,0x38,0x84,
	0x44,0x28,0x84,0xC7,0xA0,0x00,0x40,0x58,	0x84,0x4A,0x68,0x0A,0x47,0x98,0x84,0x4B,
	0x48,0x84,0x47,0x18,0x05,0x4E,0xC0,0x04,	0x54,0x58,0x85,0x56,0x48,0x85,0x47,0x58,
	0x0A,0x48,0x30,0x0A,0x5C,0xD8,0x85,0x5E,	0xC8,0x85,0x35,0xA0,0x00,0x5A,0xF8,0x85,
	0x64,0x38,0x0A,0x61,0x38,0x86,0x65,0xE8,	0x85,0x61,0x98,0x04,0xA3,0x60,0x04,0x6E,
	0xF8,0x86,0x70,0xE8,0x86,0x61,0x88,0x04,	0x9F,0x40,0x04,0x76,0x78,0x87,0x78,0x68,
	0x87,0xDD,0x43,0x87,0x79,0xD8,0x87,0x44,	0xB0,0x87,0x75,0xE8,0x87,0x78,0x38,0x3E,
	0x9D,0x80,0x00,0x86,0x78,0x88,0x88,0x68,	0x88,0xE3,0x43,0x04,0x88,0x68,0x09,0xC0,
	0xFF,0xF0,0x88,0x90,0xF8,0x0A,0x8B,0x98,	0x88,0x94,0x88,0x00,0x93,0x38,0x0A,0x4B,
	0x90,0x89,0x9A,0xB8,0x04,0xA2,0xA0,0x88,	0x0A,0x60,0x04,0x08,0xA0,0x09,0x9A,0x60,
	0x04,0x41,0x20,0x8A,0xA2,0x78,0x04,0xA6,	0x38,0x3F,0xA3,0xC0,0x88,0x87,0x38,0x0C,
	0xB5,0x30,0x0B,0xB0,0x48,0x0B,0xF5,0x00,	0x0B,0x93,0x58,0x89,0x88,0x38,0x89,0x89,
	0x04,0x43,0x52,0xE0,0x89,0xA0,0xA8,0x09,	0x9C,0x00,0x00,0x52,0x60,0x8A,0x50,0x00,
	0x00,0x9E,0x20,0x8A,0xAA,0xC8,0x88,0xAC,	0xE0,0x0A,0xAE,0xD0,0x02,0xB1,0xA0,0x8C,
	0xA9,0x30,0x0B,0xB3,0x58,0x8B,0xB6,0xE8,	0x89,0xAB,0x88,0x00,0x53,0x30,0x0A,0xA1,
	0x90,0x8D,0xD9,0x88,0x04,0xBB,0x68,0x89,	0x9F,0x18,0x8A,0x9A,0x90,0x04,0x00,0x80,
	0x0A,0x9A,0xE0,0x04,0x00,0x50,0x0A,0xA9,	0xE8,0x3D,0x8C,0x58,0x05,0x52,0xB0,0x04,
	0x34,0xA0,0x05,0x4B,0x40,0x05,0xB1,0x00,	0x8D,0xB4,0xA8,0x8E,0xD3,0x78,0x88,0xB8,
	0xB8,0x09,0x89,0xE8,0x09,0xDD,0xD8,0x3D,	0xBD,0x28,0x8A,0x5A,0x14,0x0C,0x00,0xC0,
	0x04,0xA6,0xA8,0x09,0xC7,0x88,0x00,0x55,	0x90,0x04,0x08,0xC0,0x47,0xF3,0x86,0x1E,
	0xF5,0x58,0x8D,0xF7,0xE8,0x8D,0xD5,0x38,	0x05,0xFA,0x88,0x88,0xFC,0xC8,0x8B,0xE0,
	0x28,0x8A,0x54,0x00,0x00,0xAD,0x50,0x90,	0x06,0x69,0x8F,0x09,0x89,0x88,0x4D,0x10,
	0x0B,0xF8,0x68,0x8F,0x11,0x99,0x8F,0xFB,	0xD8,0x8F,0xDF,0x68,0x8A,0x01,0x09,0x00,
	0x4D,0x50,0x90,0x07,0x59,0x05,0x4D,0xA0,	0x8D,0xA1,0x80,0x0A,0x24,0x49,0x8D,0xAC,
	0xFF,0x48,0x05,0xCA,0xB8,0x93,0xAC,0x50,	0x92,0x13,0x59,0x91,0x87,0x78,0x91,0xDE,
	0xF8,0x8F,0xE2,0x48,0x8E,0x3A,0x44,0x90,	0xC6,0x68,0x8F,0xC9,0xB8,0x93,0xCE,0xE8,
	0x93,0x39,0xC9,0x94,0xAE,0xD0,0x93,0x38,	0x69,0x8D,0xD8,0x48,0x93,0xDC,0x88,0x91,
	0x9A,0x20,0x04,0x37,0x10,0x8C,0xA2,0xF8,	0x04,0x55,0xF0,0x0A,0x49,0x09,0x91,0xF7,
	0x28,0x8D,0xD3,0x38,0x89,0x4B,0x20,0x05,	0x68,0x99,0x96,0x52,0xA0,0x90,0x43,0x09,
	0x8E,0x43,0x50,0x8A,0xA6,0xC8,0x09,0xE9,	0x28,0x96,0x65,0x69,0x92,0x63,0x69,0x97,
	0x75,0xF9,0x8F,0x1E,0xE9,0x91,0x07,0x79,	0x92,0x78,0x69,0x8B,0x64,0x09,0x98,0x2B,
	0xB9,0x97,0x7C,0x69,0x83,0x31,0xC6,0x63,	0x39,0x78,0x83,0x3E,0x98,0x76,0x74,0xB7,
	0x83,0x86,0x79,0x63,0x6C,0x17,0x11,0xF1,	0xE0,0x0E,0xED,0x50,0x99,0x96,0x79,0x99,
	0x95,0x99,0x50,0x93,0x89,0x99,0x9C,0xD9,	0x0E,0x9A,0x49,0x99,0x9D,0x79,0x99,0x9F,
	0x19,0x9A,0xA2,0xA9,0x00,0xE8,0x20,0x0E,	0xE0,0x90,0x9A,0xAA,0xB9,0x9A,0xA9,0x19,
	0x11,0xA7,0xC9,0x9A,0xB0,0x09,0x0E,0xAE,	0x89,0x9A,0xB1,0xB9,0x9A,0xB3,0x59,0x9B,
	0xB6,0x29,0x5A,0xDB,0x70,0x0D,0xBC,0xD9,	0x9B,0xBE,0xC9,0x9B,0x1F,0x25,0x0E,0xBB,
	0xF9,0x9B,0xC4,0x19,0x9C,0xC3,0x49,0x9C,	0xBE,0x69,0x9C,0xC8,0xF9,0x9B,0x1F,0x05,
	0x0E,0xD7,0x90,0x00,0xD0,0x99,0x00,0xEE,	0x00,0x0F,0xD4,0x59,0x9D,0xD4,0xD0,0x9C,
	0xCF,0x19,0x9D,0xDA,0xA9,0x9D,0xD8,0x99,	0x00,0xE0,0x70,0x0E,0xE0,0x19,0x9E,0xE7,
	0xFF,0xF0,0x0D,0xD0,0xD9,0x9D,0x09,0xF0,	0x0D,0xD3,0xA0,0x9D,0xE1,0x10,0x9D,0xE6,
	0x09,0x9D,0xF3,0x10,0x0E,0xDA,0x10,0x9F,	0xDF,0x50,0x0F,0xD5,0xD0,0x9E,0xDB,0xC9,
	0x9D,0x0A,0xE0,0x9C,0x09,0x30,0x0F,0x70,	0x15,0x0F,0xE5,0x99,0x9F,0xCF,0xA9,0x0D,
	0x00,0x20,0x0F,0xD1,0x09,0x0F,0x00,0xE0,	0x0D,0xFF,0xA9,0x9F,0xF7,0xA0,0x0F,0xFA,
	0xF0,0x02,0xFC,0xC0,0xA0,0xF4,0x80,0x0D,	0xF4,0x69,0x9F,0xF7,0x99,0xA0,0xCF,0x39,
	0x0F,0xDF,0xE0,0x0D,0x1A,0xAA,0xA1,0xE2,	0xE0,0x9F,0x09,0x60,0x9E,0xE8,0x30,0xA0,
	0xD2,0x09,0x00,0xEB,0xC0,0x9E,0x00,0x9A,	0x00,0xFE,0x20,0x0F,0xE7,0x90,0x0F,0xEC,
	0x70,0x0E,0xF5,0xB0,0x0F,0x12,0x5A,0x9F,	0x27,0x5A,0xA1,0xF8,0xA9,0x9F,0xF3,0x50,
	0x0D,0xDB,0x89,0x0D,0x1E,0xDA,0x9E,0xE9,	0x00,0x00,0xF9,0x00,0x00,0xED,0x50,0xA3,
	0xCF,0xE9,0x0F,0xE4,0xB0,0x9F,0xBD,0x35,	0xA1,0x33,0x4A,0xA3,0x16,0xBA,0x9F,0x38,
	0xAA,0x9D,0x3A,0xBA,0xA4,0xD1,0x69,0x0F,	0x00,0xA0,0x0F,0xDB,0xD9,0x9D,0x44,0xAA,
	0x9D,0xEB,0xB0,0x0F,0x26,0xAA,0x9F,0x4A,	0xBA,0xA4,0x37,0x9A,0xA3,0x3B,0x9A,0xA4,
	0x3D,0xFA,0xA3,0xEE,0x20,0xA4,0x28,0xBA,	0x0E,0x1B,0xEA,0x0D,0xF1,0xA0,0xA5,0x50,
	0x6A,0x0F,0x0C,0xFA,0xA6,0xF7,0xB0,0xA5,	0x17,0xDA,0xA4,0xD1,0xF9,0xA4,0x1F,0x3A,
	0xA3,0xE7,0x20,0xA2,0xED,0x00,0x00,0xEC,	0x20,0xA7,0x09,0xB0,0xA0,0x6F,0x0A,0xA1,
	0x7E,0x9A,0x00,0x6E,0x1A,0xA8,0x71,0xEA,	0xA5,0x19,0x9A,0xA6,0x1D,0xBA,0xA4,0xD5,
	0x44,0xF0,0x02,0x04,0x0A,0x9D,0xF0,0xE0,	0x0F,0xD8,0x00,0xA5,0x5D,0x4A,0xA1,0x15,
	0xDA,0x9D,0xE7,0x20,0x0F,0x9A,0xBA,0xA9,	0xF2,0x50,0xA4,0x77,0xCA,0xA5,0xD9,0x90,
	0x9E,0xD1,0xA9,0x0D,0x83,0x5A,0xA9,0x49,	0xAA,0xA4,0x96,0x7A,0x9F,0xA9,0x5A,0xA5,
	0xA7,0x4A,0xA3,0xAB,0x6A,0xA6,0x5D,0xFA,	0xA9,0xD9,0x69,0xAA,0x8E,0x59,0xAB,0x8B,
	0x49,0x83,0x70,0x17,0x10,0x00,0x3B
};
#endif //_MGINCORE_RES

