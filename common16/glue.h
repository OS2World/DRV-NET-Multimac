/*
 * Copyright (C) 2013 David Azarewicz
 * Copyright (C) 2012 Mensys BV
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
#ifndef GLUE_H
#define GLUE_H

/* The following are all involved in forcing a particular link
 * mode for the device for setting things.  When getting the
 * devices settings, these indicate the current mode and whether
 * it was forced up into this mode or autonegotiated.
 */

/* The forced speed, 10Mb, 100Mb, gigabit, 2.5Gb, 10GbE. */
#define SPEED_10		10
#define SPEED_100		100
#define SPEED_1000		1000
#define SPEED_2500		2500
#define SPEED_10000		10000
#define SPEED_UNKNOWN		-1

/* Duplex, half or full. */
#define DUPLEX_HALF		0x00
#define DUPLEX_FULL		0x01
#define DUPLEX_UNKNOWN		0xff

/* Which connector port. */
#define PORT_TP			0x00
#define PORT_AUI		0x01
#define PORT_MII		0x02
#define PORT_FIBRE		0x03
#define PORT_BNC		0x04
#define PORT_DA			0x05
#define PORT_NONE		0xef
#define PORT_OTHER		0xff

#define FULL_DUPLEX 2
#define HALF_DUPLEX 1

#define ETH_ALEN	6
#define ETH_DATA_LEN 1500

#define PCI_ANY_ID (~0)

#define ALIGN(x,a) __ALIGN_MASK(x,(u32)(a)-1)
#define __ALIGN_MASK(x,mask) (((x)+(mask))&~(u32)(mask))

#define le16_to_cpu(x) (x)

#define usleep_range(a,b) udelay(a)

#define swab16(x) ((__u16)(				\
	(((__u16)(x) & (__u16)0x00ffU) << 8) |			\
	(((__u16)(x) & (__u16)0xff00U) >> 8)))

#define swab32(x) ((u32)(				\
	(((u32)(x) & (u32)0x000000ffUL) << 24) |		\
	(((u32)(x) & (u32)0x0000ff00UL) <<  8) |		\
	(((u32)(x) & (u32)0x00ff0000UL) >>  8) |		\
	(((u32)(x) & (u32)0xff000000UL) >> 24)))

#define swab64(x) ((__u64)(				\
	(((__u64)(x) & (__u64)0x00000000000000ffULL) << 56) |	\
	(((__u64)(x) & (__u64)0x000000000000ff00ULL) << 40) |	\
	(((__u64)(x) & (__u64)0x0000000000ff0000ULL) << 24) |	\
	(((__u64)(x) & (__u64)0x00000000ff000000ULL) <<  8) |	\
	(((__u64)(x) & (__u64)0x000000ff00000000ULL) >>  8) |	\
	(((__u64)(x) & (__u64)0x0000ff0000000000ULL) >> 24) |	\
	(((__u64)(x) & (__u64)0x00ff000000000000ULL) >> 40) |	\
	(((__u64)(x) & (__u64)0xff00000000000000ULL) >> 56)))

#define readl(p) ReadDword(p)
#define writel(v,p) WriteDword(p,v)
#define readw(p) ReadWord(p)
#define writew(v,p) WriteWord(p,v)
#define readb(p) ReadByte(p)
#define writeb(v,p) WriteByte(p,v)
#define test_and_set_bit(b,p) TestAndSetBitL(b,p)
#define clear_bit(b,p) ClearBitL(b,p)

#define DEFINE_MUTEX(var) Mutex var = 0
#define mutex_request(a) MutexRequest(a)
#define mutex_lock(a) MutexLock(a)
#define mutex_unlock(a) MutexUnlock(a)

#endif  // GLUE_H

