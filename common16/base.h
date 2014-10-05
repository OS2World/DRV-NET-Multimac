/*
 * This source is the part of the generic ndis driver for OS/2
 *
 * Copyright (C) 2010-2014 David Azarewicz
 * Copyright (C) 2010 Mensys
 * Copyright (C) 2007 nickk
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */
#ifndef __BASE_H
#define __BASE_H

#ifndef min
#define max(a,b) (((a) > (b)) ? (a) : (b))
#define min(a,b) (((a) < (b)) ? (a) : (b))
#endif

typedef unsigned short u16;
typedef unsigned long u32;
typedef unsigned char u8;
typedef signed char s8;
typedef signed short s16;
typedef signed long s32;
typedef signed __int64 s64;
typedef unsigned __int64 u64;

typedef u8 far * fpu8;
typedef char far * fpchar;
typedef void far * fptr;
typedef u32 far * fpu32;
typedef u16 far * fpu16;

typedef u16 __le16;
typedef u32 __le32;
typedef u64 __le64;
typedef u16 __be16;
typedef u32 __be32;
typedef u64 __be64;
typedef u64 __u64;
typedef u32 __u32;
typedef u16 __u16;
typedef u8 __u8;
typedef s8 __s8;
typedef s16 __s16;
typedef s32 __s32;
typedef s64 __s64;

typedef u64 dma_addr_t;

typedef _Bool bool;
#define true 1
#define false 0

#ifndef NULL
#define NULL (0)
#endif

#define ntohs(x) (((u16)x<<8)|((u16)x>>8))

#define ARRAY_SIZE(x) (sizeof(x) / sizeof((x)[0]))

#endif

