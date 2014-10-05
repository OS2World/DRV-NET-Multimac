/*
 * This source is the part of the generic ndis driver for OS/2
 *
 * Copyright (C) 2010 Mensys
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

#ifndef     GENMIOCTL_HPP
#define     GENMIOCTL_HPP

/* From ioctl.hpp */
#define GENMAC_CATEGORY 		0x99
#define GENMAC_READ_PHY_ADDR	0x01
#define GENMAC_READ_MII_REG 	0x02
#define GENMAC_WRITE_MII_REG	0x03
#define GENMAC_READ_PHY_TYPE	0x04

#define GENMAC_GET_DEBUGLEVEL	0x10
#define GENMAC_SET_DEBUGLEVEL	0x11

#define GENMAC_QUERY_DEV_BUS	0x20

#define GENMAC_HELPER_REQUEST	0x30
#define GENMAC_HELPER_RETURN	0x31

#define GENMAC_WRAPPER_OID_GET	0x40
#define GENMAC_WRAPPER_OID_SET	0x41

/* From poid.hpp */
#define OID_PRIVATE_WRAPPER_LANNUMBER           0xFFFFFF00
#define OID_PRIVATE_WRAPPER_ISWIRELESS          0xFFFFFF01
#define OID_PRIVATE_WRAPPER_LINKSTATUS          0xFFFFFF02
#define OID_PRIVATE_WRAPPER_STATUSCHANGE        0xFFFFFF03
#define OID_PRIVATE_WRAPPER_HALT                0xFFFFFF04
#define OID_PRIVATE_WRAPPER_REINIT              0xFFFFFF05
#define OID_PRIVATE_WRAPPER_ARPLIST             0xFFFFFF06
#define OID_PRIVATE_WRAPPER_DEFAULT_STATE       0xFFFFFF07
#define OID_PRIVATE_WRAPPER_WINDRIVER_NIFNAME   0xFFFFFF08
#define OID_PRIVATE_WRAPPER_GENMAC_VERSION      0xFFFFFF09


/* From xlan\...\ntndis.h */
#define OID_GEN_LINK_SPEED                      0x00010107
#define OID_GEN_MEDIA_CONNECT_STATUS            0x00010114

#endif

