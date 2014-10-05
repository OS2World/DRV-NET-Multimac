/*
 * This source is the part of the generic ndis driver for OS/2
 *
 * Copyright (C) 2010-2014 David Azarewicz
 * Copyright (C) 2010-2011 Mensys
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

#ifndef __DRIVER_H
#define __DRIVER_H

#ifdef DEBUG

#define DPRINTF dprintf

#define e_dbg(f, ...) dprintf(1, f, ##__VA_ARGS__)
#define e_err(f, ...) dprintf(1, f, ##__VA_ARGS__)
#define e_info(f, ...) dprintf(1, f, ##__VA_ARGS__)
#define e_warn(f, ...) dprintf(1, f, ##__VA_ARGS__)
#define e_noticef, ...) dprintf(1, f, ##__VA_ARGS__)
#define dprintk(f, ...) dprintf(1, f, ##__VA_ARGS__)
#define printk(f, ...) dprintf(1, f, ##__VA_ARGS__)

#define DBG_MEMBUFF_SIZE 16384

#else  // DEBUG == 0

#define DBGCALLCONV	NEAR

#define DPRINTF(x, ...)
#define e_dbg(f, ...)
#define e_err(f, ...)
#define e_info(f, ...)
#define e_warn(f, ...)
#define e_notice(f, ...)
#define dprintk(x, ...)
#define printk(x, ...)

#endif  // DEBUG

#define MAX_RING_SIZE 256
#define MAX_RB_ALLOC_SIZE 16384
#define STATIC_RING_BUFFER_GDT

struct Os2RingInfo {
    u16 AllocSize;
    u16 RingCount;
    u16 DescSize;
#ifndef STATIC_RING_BUFFER_GDT
    u16 Gdt;
#endif
    u32 RingDma;
    fptr RingVirt;
    u32 BufferDma;
#ifdef STATIC_RING_BUFFER_GDT
    fptr BufferVirt[MAX_RING_SIZE];
#endif
};

/*
 *      Old network device statistics. Fields are native words
 *      (unsigned long) so they can be read and written atomically.
 */

struct net_device_stats {
  unsigned long   rx_packets;
  unsigned long   tx_packets;
  unsigned long   rx_bytes;
  unsigned long   tx_bytes;
  unsigned long   rx_errors;
  unsigned long   tx_errors;
  unsigned long   rx_dropped;
  unsigned long   tx_dropped;
  unsigned long   multicast;
  unsigned long   collisions;
  unsigned long   rx_length_errors;
  unsigned long   rx_over_errors;
  unsigned long   rx_crc_errors;
  unsigned long   rx_frame_errors;
  unsigned long   rx_fifo_errors;
  unsigned long   rx_missed_errors;
  unsigned long   tx_aborted_errors;
  unsigned long   tx_carrier_errors;
  unsigned long   tx_fifo_errors;
  unsigned long   tx_heartbeat_errors;
  unsigned long   tx_window_errors;
  unsigned long   rx_compressed;
  unsigned long   tx_compressed;
};

#define MAX_ADDR_LEN 32

struct net_device {
  struct pci_dev; /* must be the first item in net_device */
  u16 driver_data;
  unsigned short mtu;     /* interface MTU value          */
  u16 flags;
  void *private_data;
  u8 dev_addr[ADDR_SIZE];
  u8 perm_addr[ADDR_SIZE];
  u16 addr_len;
  char name[48];
  u32 pci_config[16];
  struct Os2RingInfo tx;
  struct Os2RingInfo rx;
  struct net_device_stats stats;
};

#define MAX_DEVNAME     8
#define MAX_DRIVERS     9

extern char cDevName[MAX_DEVNAME + 1];

extern u16 mg_wVerbose;
extern u16 mg_wAdapter;
extern char mg_BuildLevel[];
extern char mg_FwDir[];

extern struct CommChar ProtCC;
extern struct ProtLwrDisp ProtDT;

extern struct CommChar   AdapterCC;
extern struct MACUprDisp AdapterDT;

extern struct MACSpecChar AdapterSC;
extern struct MACSpecStat AdapterSS;
extern struct MCastBuf AdapterMCB;
extern struct MAC8023Stat MACSS;

#ifdef TRACING
extern void TraceBuf(USHORT minCode, USHORT DataLen, void far *data);
extern void TraceArgs(USHORT minCode, USHORT DataLen, ...);
#else
#define TraceBuf(a,b,c)
#define TraceArgs(a,b, ...)
#endif

#define cpu_to_le32(a) (a)
#define le32_to_cpu(a) (a)
#define BITS_PER_BYTE 8
#define BITS_PER_LONG 32
#define BIT(nr) (1UL << (nr))
#define BIT_MASK(nr) (1UL << (nr))

u32 ether_crc(short length, unsigned char far *data);
short AllocateRingsAndBuffers(struct net_device *pDev);

u32 GetRxBufferPhysicalAddress(struct net_device *pDev, u16 Ix);
u32 GetTxBufferPhysicalAddress(struct net_device *pDev, u16 Ix);
fptr GetRxBufferVirtualAddress(struct net_device *pDev, u16 Ix);
fptr GetTxBufferVirtualAddress(struct net_device *pDev, u16 Ix);

/* interface exported from individual driver */
extern void DriverIOCtlGenMac(PREQPACKET pPacket);
extern short DriverSuspend(void);
extern short DriverResume(void);
extern int FindAndSetupAdapter(void);
extern short DriverCheckDevice(struct net_device *pDev);
extern short DriverInitAdapter(struct net_device *pDev);

#endif

