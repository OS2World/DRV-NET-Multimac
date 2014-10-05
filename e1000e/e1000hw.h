/*
 * This source is the part of e1000e - Intel PRO/1000 NDIS driver for OS/2
 *
 * Copyright (C) 2010
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

#ifndef E1000HW_H
#define E1000HW_H

#include "glue.h"
#include "mii.h"
#include "driver.h"
#include "hw.h"
#include "e1000.h"

#define E1000_VENDOR_ID_INTEL   0x8086

#define E1000_RX_ALLOCATE   2048
#define E1000_TX_ALLOCATE   2048

#define E1000_PCI_REGISTER_SIZE 0x05B54

#define RX_RING_DEFAULT         E1000_DEFAULT_RXD
#define TX_RING_DEFAULT         E1000_DEFAULT_TXD

typedef struct _ring_desc
{
    u32 buf_lo;
    u32 buf_hi;
    u32 flaglen_lo;
    u32 flaglen_hi;
} ring_desc, far *pring_desc;

#define GDT_READ    (Priv.gdt[0])
#define GDT_WRITE   (Priv.gdt[1])

#endif  // E1000HW_H
