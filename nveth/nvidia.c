/*
 * This source is the part of nveth - NVIDIA ndis driver for OS/2
 *
 * Copyright (C) 2010 Mensys
 * Copyright (C) 2007 nickk
 *
 * This driver is based on forcedeth.c
 *
 * Copyright (C) 2003,4,5 Manfred Spraul
 * Copyright (C) 2004 Andrew de Quincey (wol support)
 * Copyright (C) 2004 Carl-Daniel Hailfinger (invalid MAC handling, insane
 *      IRQ rate fixes, bigendian fixes, cleanups, verification)
 * Copyright (c) 2004,2005,2006,2007,2008,2009 NVIDIA Corporation
 *
 * NVIDIA, nForce and other NVIDIA marks are trademarks or registered
 * trademarks of NVIDIA Corporation in the United States and other
 * countries.
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
#include "Dev16lib.h"
#include "base.h"
#include "ndis.h"
#include "driver.h"
#include "pci_regs.h"
#include "nvidia.h"
#include "ioctl.h"
#include "glue.h"
#include "version.h"

#define MSG_HWDET 11

struct nveth_private
{
    struct net_device *pDev;
    u16 desc_ver;
    u32 txrxctl_bits;
    u32 flags;
    u16 rx_csum;
    u16 msi_flags;
    u16 pause_flags;
    u16 register_size;
    fpu8 ba;
    u8 mac[6];
    u16 irqmask;
    u16 need_linktimer;
    u32 mac_in_use;
    u16 phy_model;
    u16 phyaddr;
    u16 phy_oui;
    u16 phy_rev;
    u16 gigabit;
    u32 linkspeed;
    u16 duplex;
    u16 autoneg;
    u16 in_shutdown;
    u16 rxstarted;
    u16 txbusy;
    u16 Suspended;

    u16 vlanctl_bits;

    u16 gdt[2];

    union ring_type rx_ring;
    u32 rx_buf_sz;
    u32 pkt_limit;
    u16 rx_ring_size;
    u16 rxpos;

    fpu8 currentrxbuffer;
    u16 currentlen;

    union ring_type tx_ring;
    u16 txpos;
    u32 tx_flags;
    u16 tx_ring_size;

    Mutex mutex;
} Priv;

//    void setFlags(u32 newflags) { flags = newflags; }

u16 optimization_mode = NV_OPTIMIZATION_MODE_THROUGHPUT;
u16 poll_interval = -1;
u16 fixed_mode = 0;
u16 max_interrupt_work = 30;
u16 notimer = 1;
u16 nolinktimer = 1;
u16 dorxchain = 1;
u16 netaddress_override = 0;
u8 macaddr[6];

static struct pci_device_id {
  char  *name;
  u16    vendor;
  u16    device;
  u32   flags;
} pci_tbl[] = {
  {   "nForce",
    PCI_VENDOR_ID_NVIDIA, PCI_DEVICE_ID_NVIDIA_NVENET_1,
    DEV_NEED_TIMERIRQ|DEV_NEED_LINKTIMER,
  },
  {   "nForce2",
    PCI_VENDOR_ID_NVIDIA, PCI_DEVICE_ID_NVIDIA_NVENET_2,
    DEV_NEED_TIMERIRQ|DEV_NEED_LINKTIMER,
  },
  {   "nForce3",
    PCI_VENDOR_ID_NVIDIA, PCI_DEVICE_ID_NVIDIA_NVENET_3,
    DEV_NEED_TIMERIRQ|DEV_NEED_LINKTIMER,
  },
  {   "nForce3",
    PCI_VENDOR_ID_NVIDIA, PCI_DEVICE_ID_NVIDIA_NVENET_4,
    DEV_NEED_TIMERIRQ|DEV_NEED_LINKTIMER|DEV_HAS_LARGEDESC|DEV_HAS_CHECKSUM,
  },
  {   "nForce3",
    PCI_VENDOR_ID_NVIDIA, PCI_DEVICE_ID_NVIDIA_NVENET_5,
    DEV_NEED_TIMERIRQ|DEV_NEED_LINKTIMER|DEV_HAS_LARGEDESC|DEV_HAS_CHECKSUM,
  },
  {   "nForce3",
    PCI_VENDOR_ID_NVIDIA, PCI_DEVICE_ID_NVIDIA_NVENET_6,
    DEV_NEED_TIMERIRQ|DEV_NEED_LINKTIMER|DEV_HAS_LARGEDESC|DEV_HAS_CHECKSUM,
  },
  {   "nForce3",
    PCI_VENDOR_ID_NVIDIA, PCI_DEVICE_ID_NVIDIA_NVENET_7,
    DEV_NEED_TIMERIRQ|DEV_NEED_LINKTIMER|DEV_HAS_LARGEDESC|DEV_HAS_CHECKSUM,
  },
  {   "CK804",
    PCI_VENDOR_ID_NVIDIA, PCI_DEVICE_ID_NVIDIA_NVENET_8,
    DEV_NEED_TIMERIRQ|DEV_NEED_LINKTIMER|DEV_HAS_LARGEDESC|DEV_HAS_CHECKSUM|DEV_HAS_HIGH_DMA|DEV_HAS_STATISTICS_V1,
  },
  {   "CK804",
    PCI_VENDOR_ID_NVIDIA, PCI_DEVICE_ID_NVIDIA_NVENET_9,
    DEV_NEED_TIMERIRQ|DEV_NEED_LINKTIMER|DEV_HAS_LARGEDESC|DEV_HAS_CHECKSUM|DEV_HAS_HIGH_DMA|DEV_HAS_STATISTICS_V1,
  },
  {   "MCP04",
    PCI_VENDOR_ID_NVIDIA, PCI_DEVICE_ID_NVIDIA_NVENET_10,
    DEV_NEED_TIMERIRQ|DEV_NEED_LINKTIMER|DEV_HAS_LARGEDESC|DEV_HAS_CHECKSUM|DEV_HAS_HIGH_DMA|DEV_HAS_STATISTICS_V1,
  },
  {   "MCP04",
    PCI_VENDOR_ID_NVIDIA, PCI_DEVICE_ID_NVIDIA_NVENET_11,
    DEV_NEED_TIMERIRQ|DEV_NEED_LINKTIMER|DEV_HAS_LARGEDESC|DEV_HAS_CHECKSUM|DEV_HAS_HIGH_DMA|DEV_HAS_STATISTICS_V1,
  },
  {   "MCP51",
    PCI_VENDOR_ID_NVIDIA, PCI_DEVICE_ID_NVIDIA_NVENET_12,
    DEV_NEED_TIMERIRQ|DEV_NEED_LINKTIMER|DEV_HAS_HIGH_DMA|DEV_HAS_POWER_CNTRL|DEV_HAS_STATISTICS_V1,
  },
  {   "MCP51",
    PCI_VENDOR_ID_NVIDIA, PCI_DEVICE_ID_NVIDIA_NVENET_13,
    DEV_NEED_TIMERIRQ|DEV_NEED_LINKTIMER|DEV_HAS_HIGH_DMA|DEV_HAS_POWER_CNTRL|DEV_HAS_STATISTICS_V1,
  },
  {   "MCP55",
    PCI_VENDOR_ID_NVIDIA, PCI_DEVICE_ID_NVIDIA_NVENET_14,
    DEV_NEED_TIMERIRQ|DEV_NEED_LINKTIMER|DEV_HAS_LARGEDESC|DEV_HAS_CHECKSUM|DEV_HAS_HIGH_DMA|DEV_HAS_VLAN|DEV_HAS_MSI|DEV_HAS_MSI_X|DEV_HAS_POWER_CNTRL|DEV_HAS_PAUSEFRAME_TX|DEV_HAS_STATISTICS_V2|DEV_HAS_TEST_EXTENDED|DEV_HAS_MGMT_UNIT,
  },
  {   "MCP55",
    PCI_VENDOR_ID_NVIDIA, PCI_DEVICE_ID_NVIDIA_NVENET_15,
    DEV_NEED_TIMERIRQ|DEV_NEED_LINKTIMER|DEV_HAS_LARGEDESC|DEV_HAS_CHECKSUM|DEV_HAS_HIGH_DMA|DEV_HAS_VLAN|DEV_HAS_MSI|DEV_HAS_MSI_X|DEV_HAS_POWER_CNTRL|DEV_HAS_PAUSEFRAME_TX|DEV_HAS_STATISTICS_V2|DEV_HAS_TEST_EXTENDED|DEV_HAS_MGMT_UNIT,
  },
  {   "MCP61",
    PCI_VENDOR_ID_NVIDIA, PCI_DEVICE_ID_NVIDIA_NVENET_16,
    DEV_NEED_TIMERIRQ|DEV_NEED_LINKTIMER|DEV_HAS_HIGH_DMA|DEV_HAS_POWER_CNTRL|DEV_HAS_MSI|DEV_HAS_PAUSEFRAME_TX|DEV_HAS_STATISTICS_V2|DEV_HAS_TEST_EXTENDED|DEV_HAS_MGMT_UNIT|DEV_HAS_CORRECT_MACADDR,
  },
  {   "MCP61",
    PCI_VENDOR_ID_NVIDIA, PCI_DEVICE_ID_NVIDIA_NVENET_17,
    DEV_NEED_TIMERIRQ|DEV_NEED_LINKTIMER|DEV_HAS_HIGH_DMA|DEV_HAS_POWER_CNTRL|DEV_HAS_MSI|DEV_HAS_PAUSEFRAME_TX|DEV_HAS_STATISTICS_V2|DEV_HAS_TEST_EXTENDED|DEV_HAS_MGMT_UNIT|DEV_HAS_CORRECT_MACADDR,
  },
  {   "MCP61",
    PCI_VENDOR_ID_NVIDIA, PCI_DEVICE_ID_NVIDIA_NVENET_18,
    DEV_NEED_TIMERIRQ|DEV_NEED_LINKTIMER|DEV_HAS_HIGH_DMA|DEV_HAS_POWER_CNTRL|DEV_HAS_MSI|DEV_HAS_PAUSEFRAME_TX|DEV_HAS_STATISTICS_V2|DEV_HAS_TEST_EXTENDED|DEV_HAS_MGMT_UNIT|DEV_HAS_CORRECT_MACADDR,
  },
  {   "MCP61",
    PCI_VENDOR_ID_NVIDIA, PCI_DEVICE_ID_NVIDIA_NVENET_19,
    DEV_NEED_TIMERIRQ|DEV_NEED_LINKTIMER|DEV_HAS_HIGH_DMA|DEV_HAS_POWER_CNTRL|DEV_HAS_MSI|DEV_HAS_PAUSEFRAME_TX|DEV_HAS_STATISTICS_V2|DEV_HAS_TEST_EXTENDED|DEV_HAS_MGMT_UNIT|DEV_HAS_CORRECT_MACADDR,
  },
  {   "MCP65",
    PCI_VENDOR_ID_NVIDIA, PCI_DEVICE_ID_NVIDIA_NVENET_20,
    DEV_NEED_TIMERIRQ|DEV_NEED_LINKTIMER|DEV_HAS_LARGEDESC|DEV_HAS_HIGH_DMA|DEV_HAS_POWER_CNTRL|DEV_HAS_MSI|DEV_HAS_PAUSEFRAME_TX|DEV_HAS_STATISTICS_V2|DEV_HAS_TEST_EXTENDED|DEV_HAS_MGMT_UNIT|DEV_HAS_CORRECT_MACADDR,
  },
  {   "MCP65",
    PCI_VENDOR_ID_NVIDIA, PCI_DEVICE_ID_NVIDIA_NVENET_21,
    DEV_NEED_TIMERIRQ|DEV_NEED_LINKTIMER|DEV_HAS_LARGEDESC|DEV_HAS_HIGH_DMA|DEV_HAS_POWER_CNTRL|DEV_HAS_MSI|DEV_HAS_PAUSEFRAME_TX|DEV_HAS_STATISTICS_V2|DEV_HAS_TEST_EXTENDED|DEV_HAS_MGMT_UNIT|DEV_HAS_CORRECT_MACADDR,
  },
  {   "MCP65",
    PCI_VENDOR_ID_NVIDIA, PCI_DEVICE_ID_NVIDIA_NVENET_22,
    DEV_NEED_TIMERIRQ|DEV_NEED_LINKTIMER|DEV_HAS_LARGEDESC|DEV_HAS_HIGH_DMA|DEV_HAS_POWER_CNTRL|DEV_HAS_MSI|DEV_HAS_PAUSEFRAME_TX|DEV_HAS_STATISTICS_V2|DEV_HAS_TEST_EXTENDED|DEV_HAS_MGMT_UNIT|DEV_HAS_CORRECT_MACADDR,
  },
  {   "MCP65",
    PCI_VENDOR_ID_NVIDIA, PCI_DEVICE_ID_NVIDIA_NVENET_23,
    DEV_NEED_TIMERIRQ|DEV_NEED_LINKTIMER|DEV_HAS_LARGEDESC|DEV_HAS_HIGH_DMA|DEV_HAS_POWER_CNTRL|DEV_HAS_MSI|DEV_HAS_PAUSEFRAME_TX|DEV_HAS_STATISTICS_V2|DEV_HAS_TEST_EXTENDED|DEV_HAS_MGMT_UNIT|DEV_HAS_CORRECT_MACADDR,
  },
  {   "MCP67",
    PCI_VENDOR_ID_NVIDIA, PCI_DEVICE_ID_NVIDIA_NVENET_24,
    DEV_NEED_TIMERIRQ|DEV_NEED_LINKTIMER|DEV_HAS_HIGH_DMA|DEV_HAS_POWER_CNTRL|DEV_HAS_MSI|DEV_HAS_PAUSEFRAME_TX|DEV_HAS_STATISTICS_V2|DEV_HAS_TEST_EXTENDED|DEV_HAS_MGMT_UNIT|DEV_HAS_CORRECT_MACADDR,
  },
  {   "MCP67",
    PCI_VENDOR_ID_NVIDIA, PCI_DEVICE_ID_NVIDIA_NVENET_25,
    DEV_NEED_TIMERIRQ|DEV_NEED_LINKTIMER|DEV_HAS_HIGH_DMA|DEV_HAS_POWER_CNTRL|DEV_HAS_MSI|DEV_HAS_PAUSEFRAME_TX|DEV_HAS_STATISTICS_V2|DEV_HAS_TEST_EXTENDED|DEV_HAS_MGMT_UNIT|DEV_HAS_CORRECT_MACADDR,
  },
  {   "MCP67",
    PCI_VENDOR_ID_NVIDIA, PCI_DEVICE_ID_NVIDIA_NVENET_26,
    DEV_NEED_TIMERIRQ|DEV_NEED_LINKTIMER|DEV_HAS_HIGH_DMA|DEV_HAS_POWER_CNTRL|DEV_HAS_MSI|DEV_HAS_PAUSEFRAME_TX|DEV_HAS_STATISTICS_V2|DEV_HAS_TEST_EXTENDED|DEV_HAS_MGMT_UNIT|DEV_HAS_CORRECT_MACADDR,
  },
  {   "MCP67",
    PCI_VENDOR_ID_NVIDIA, PCI_DEVICE_ID_NVIDIA_NVENET_27,
    DEV_NEED_TIMERIRQ|DEV_NEED_LINKTIMER|DEV_HAS_HIGH_DMA|DEV_HAS_POWER_CNTRL|DEV_HAS_MSI|DEV_HAS_PAUSEFRAME_TX|DEV_HAS_STATISTICS_V2|DEV_HAS_TEST_EXTENDED|DEV_HAS_MGMT_UNIT|DEV_HAS_CORRECT_MACADDR,
  },
  {   "MCP73",
    PCI_VENDOR_ID_NVIDIA, PCI_DEVICE_ID_NVIDIA_NVENET_28,
    DEV_NEED_TIMERIRQ|DEV_NEED_LINKTIMER|DEV_HAS_HIGH_DMA|DEV_HAS_POWER_CNTRL|DEV_HAS_MSI|DEV_HAS_PAUSEFRAME_TX|DEV_HAS_STATISTICS_V2|DEV_HAS_TEST_EXTENDED|DEV_HAS_MGMT_UNIT|DEV_HAS_CORRECT_MACADDR|DEV_HAS_COLLISION_FIX,
  },
  {   "MCP73",
    PCI_VENDOR_ID_NVIDIA, PCI_DEVICE_ID_NVIDIA_NVENET_29,
    DEV_NEED_TIMERIRQ|DEV_NEED_LINKTIMER|DEV_HAS_HIGH_DMA|DEV_HAS_POWER_CNTRL|DEV_HAS_MSI|DEV_HAS_PAUSEFRAME_TX|DEV_HAS_STATISTICS_V2|DEV_HAS_TEST_EXTENDED|DEV_HAS_MGMT_UNIT|DEV_HAS_CORRECT_MACADDR|DEV_HAS_COLLISION_FIX,
  },
  {   "MCP73",
    PCI_VENDOR_ID_NVIDIA, PCI_DEVICE_ID_NVIDIA_NVENET_30,
    DEV_NEED_TIMERIRQ|DEV_NEED_LINKTIMER|DEV_HAS_HIGH_DMA|DEV_HAS_POWER_CNTRL|DEV_HAS_MSI|DEV_HAS_PAUSEFRAME_TX|DEV_HAS_STATISTICS_V2|DEV_HAS_TEST_EXTENDED|DEV_HAS_MGMT_UNIT|DEV_HAS_CORRECT_MACADDR|DEV_HAS_COLLISION_FIX,
  },
  {   "MCP73",
    PCI_VENDOR_ID_NVIDIA, PCI_DEVICE_ID_NVIDIA_NVENET_31,
    DEV_NEED_TIMERIRQ|DEV_NEED_LINKTIMER|DEV_HAS_HIGH_DMA|DEV_HAS_POWER_CNTRL|DEV_HAS_MSI|DEV_HAS_PAUSEFRAME_TX|DEV_HAS_STATISTICS_V2|DEV_HAS_TEST_EXTENDED|DEV_HAS_MGMT_UNIT|DEV_HAS_CORRECT_MACADDR|DEV_HAS_COLLISION_FIX,
  },
  {   "MCP77",
    PCI_VENDOR_ID_NVIDIA, PCI_DEVICE_ID_NVIDIA_NVENET_32,
    DEV_NEED_TIMERIRQ|DEV_NEED_LINKTIMER|DEV_HAS_HIGH_DMA|DEV_HAS_POWER_CNTRL|DEV_HAS_MSI|DEV_HAS_PAUSEFRAME_TX|DEV_HAS_STATISTICS_V2|DEV_HAS_TEST_EXTENDED|DEV_HAS_MGMT_UNIT|DEV_NEED_PHY_INIT_FIX|DEV_HAS_CORRECT_MACADDR|DEV_HAS_COLLISION_FIX,
  },
  {   "MCP77",
    PCI_VENDOR_ID_NVIDIA, PCI_DEVICE_ID_NVIDIA_NVENET_33,
    DEV_NEED_TIMERIRQ|DEV_NEED_LINKTIMER|DEV_HAS_HIGH_DMA|DEV_HAS_POWER_CNTRL|DEV_HAS_MSI|DEV_HAS_PAUSEFRAME_TX|DEV_HAS_STATISTICS_V2|DEV_HAS_TEST_EXTENDED|DEV_HAS_MGMT_UNIT|DEV_NEED_PHY_INIT_FIX|DEV_HAS_CORRECT_MACADDR|DEV_HAS_COLLISION_FIX,
  },
  {   "MCP77",
    PCI_VENDOR_ID_NVIDIA, PCI_DEVICE_ID_NVIDIA_NVENET_34,
    DEV_NEED_TIMERIRQ|DEV_NEED_LINKTIMER|DEV_HAS_HIGH_DMA|DEV_HAS_POWER_CNTRL|DEV_HAS_MSI|DEV_HAS_PAUSEFRAME_TX|DEV_HAS_STATISTICS_V2|DEV_HAS_TEST_EXTENDED|DEV_HAS_MGMT_UNIT|DEV_NEED_PHY_INIT_FIX|DEV_HAS_CORRECT_MACADDR|DEV_HAS_COLLISION_FIX,
  },
  {   "MCP77",
    PCI_VENDOR_ID_NVIDIA, PCI_DEVICE_ID_NVIDIA_NVENET_35,
    DEV_NEED_TIMERIRQ|DEV_NEED_LINKTIMER|DEV_HAS_HIGH_DMA|DEV_HAS_POWER_CNTRL|DEV_HAS_MSI|DEV_HAS_PAUSEFRAME_TX|DEV_HAS_STATISTICS_V2|DEV_HAS_TEST_EXTENDED|DEV_HAS_MGMT_UNIT|DEV_NEED_PHY_INIT_FIX|DEV_HAS_CORRECT_MACADDR|DEV_HAS_COLLISION_FIX,
  },
  {   "MCP79",
    PCI_VENDOR_ID_NVIDIA, PCI_DEVICE_ID_NVIDIA_NVENET_36,
    DEV_NEED_TIMERIRQ|DEV_NEED_LINKTIMER|DEV_HAS_HIGH_DMA|DEV_HAS_POWER_CNTRL|DEV_HAS_MSI|DEV_HAS_PAUSEFRAME_TX|DEV_HAS_STATISTICS_V2|DEV_HAS_TEST_EXTENDED|DEV_HAS_MGMT_UNIT|DEV_NEED_PHY_INIT_FIX|DEV_HAS_CORRECT_MACADDR|DEV_HAS_COLLISION_FIX,
  },
  {   "MCP79",
    PCI_VENDOR_ID_NVIDIA, PCI_DEVICE_ID_NVIDIA_NVENET_37,
    DEV_NEED_TIMERIRQ|DEV_NEED_LINKTIMER|DEV_HAS_HIGH_DMA|DEV_HAS_POWER_CNTRL|DEV_HAS_MSI|DEV_HAS_PAUSEFRAME_TX|DEV_HAS_STATISTICS_V2|DEV_HAS_TEST_EXTENDED|DEV_HAS_MGMT_UNIT|DEV_NEED_PHY_INIT_FIX|DEV_HAS_CORRECT_MACADDR|DEV_HAS_COLLISION_FIX,
  },
  {   "MCP79",
    PCI_VENDOR_ID_NVIDIA, PCI_DEVICE_ID_NVIDIA_NVENET_38,
    DEV_NEED_TIMERIRQ|DEV_NEED_LINKTIMER|DEV_HAS_HIGH_DMA|DEV_HAS_POWER_CNTRL|DEV_HAS_MSI|DEV_HAS_PAUSEFRAME_TX|DEV_HAS_STATISTICS_V2|DEV_HAS_TEST_EXTENDED|DEV_HAS_MGMT_UNIT|DEV_NEED_PHY_INIT_FIX|DEV_HAS_CORRECT_MACADDR|DEV_HAS_COLLISION_FIX,
  },
  {   "MCP79",
    PCI_VENDOR_ID_NVIDIA, PCI_DEVICE_ID_NVIDIA_NVENET_39,
    DEV_NEED_TIMERIRQ|DEV_NEED_LINKTIMER|DEV_HAS_HIGH_DMA|DEV_HAS_POWER_CNTRL|DEV_HAS_MSI|DEV_HAS_PAUSEFRAME_TX|DEV_HAS_STATISTICS_V2|DEV_HAS_TEST_EXTENDED|DEV_HAS_MGMT_UNIT|DEV_NEED_PHY_INIT_FIX|DEV_HAS_CORRECT_MACADDR|DEV_HAS_COLLISION_FIX,
  },
  {   "MCP89",
    PCI_VENDOR_ID_NVIDIA, PCI_DEVICE_ID_NVIDIA_NVENET_40,
    DEV_NEED_TIMERIRQ|DEV_NEED_LINKTIMER|DEV_HAS_HIGH_DMA|DEV_HAS_POWER_CNTRL|DEV_HAS_MSI|DEV_HAS_PAUSEFRAME_TX|DEV_HAS_STATISTICS_V2|DEV_HAS_TEST_EXTENDED|DEV_HAS_MGMT_UNIT|DEV_NEED_PHY_INIT_FIX|DEV_HAS_CORRECT_MACADDR|DEV_HAS_COLLISION_FIX,
  },
  {NULL, 0,0,0},
};

int mgmtAcquireSema(void)
{
  int i;
  u32 mgmt_sema;
  int rc;

  for (i = 0; i < 10; i++)
  {
    mgmt_sema = NV_R32(NvRegTransmitterControl);
    mgmt_sema &= NVREG_XMITCTL_MGMT_SEMA_MASK;
    if (mgmt_sema == NVREG_XMITCTL_MGMT_SEMA_FREE) break;
    msleep(500);
  }

  rc = 0;
  if (mgmt_sema == NVREG_XMITCTL_MGMT_SEMA_FREE)
  {
    for (i = 0; i < 2; i++)
    {
      u32 tx_ctrl = NV_R32(NvRegTransmitterControl);
      tx_ctrl |= NVREG_XMITCTL_HOST_SEMA_ACQ;
      NV_W32(tx_ctrl, NvRegTransmitterControl);

      tx_ctrl = NV_R32(NvRegTransmitterControl);
      if (((tx_ctrl & NVREG_XMITCTL_HOST_SEMA_MASK) == NVREG_XMITCTL_HOST_SEMA_ACQ) &&
          ((tx_ctrl & NVREG_XMITCTL_MGMT_SEMA_MASK) == NVREG_XMITCTL_MGMT_SEMA_FREE))
      {
        rc = 1;
        break;
      }
      else
      {
        udelay(50);
      }
    }
  }
  return rc;
}

int regDelay(u16 offset, u32 mask, u32 target, u16 delay, long delaymax, char *msg)
{
    NV_R32(0);
    do
    {
        udelay(delay);
        delaymax -= delay;
        if (delaymax < 0)
        {
          #ifdef DEBUG
          if (msg) DPRINTF(1,msg);
          #endif
          return 1;
        }
    } while ((NV_R32(offset) & mask) != target);
    return 0;
}

/* phyRW read/write a register on the PHY.
 *
 * Caller must guarantee serialization
 */
u32 phyRW(u16 addr, u16 miireg, u32 value)
{
  u32 reg;
  u32 retval;

  NV_W32(NVREG_MIISTAT_MASK, NvRegMIIStatus);

  reg = NV_R32(NvRegMIIControl);
  if (reg & NVREG_MIICTL_INUSE)
  {
    NV_W32(NVREG_MIICTL_INUSE, NvRegMIIControl);
    udelay(NV_MIIBUSY_DELAY);
  }

  reg = ((u32)addr << NVREG_MIICTL_ADDRSHIFT) + miireg;
  if (value != MII_READ)
  {
    NV_W32(value, NvRegMIIData);
    reg |= NVREG_MIICTL_WRITE;
  }
  NV_W32(reg, NvRegMIIControl);

  if (regDelay(NvRegMIIControl, NVREG_MIICTL_INUSE, 0, NV_MIIPHY_DELAY, NV_MIIPHY_DELAYMAX, NULL))
  {
    retval = -1L;
  }
  else if (value != MII_READ)
  {
    /* it was a write operation - fewer failures are detectable */
    retval = 0;
  }
  else
  {
    u32 status = NV_R32(NvRegMIIStatus);
    u32 data = NV_R32(NvRegMIIData);
    if (status & NVREG_MIISTAT_ERROR)
    {
      retval = -1L;
    }
    else
    {
      retval = data;
    }
  }
  return retval;
}

int phyReset(u32 bmcr_setup)
{
    u32 miicontrol;
    u16 tries = 0;

    miicontrol = BMCR_RESET | bmcr_setup;
    if (phyRW(Priv.phyaddr, MII_BMCR, miicontrol)) {
        return 1;
    }

    msleep(500); /* wait for 500ms */

    /* must wait till reset is deasserted */
    while (miicontrol & BMCR_RESET) {
        msleep(32);
        miicontrol = phyRW(Priv.phyaddr, MII_BMCR, MII_READ);
        /* FIXME: 100 tries seem excessive */
        if (tries++ > 100) {
            return 1;
        }
    }
    return 0;
}

int phyInit(void)
{
    u32 phyinterface, phy_reserved, mii_status, mii_control, mii_control_1000, reg, powerstate;

    /* phy errata for E3016 phy */
    if (Priv.phy_model == PHY_MODEL_MARVELL_E3016) {
        reg = phyRW(Priv.phyaddr, MII_NCONFIG, MII_READ);
        reg &= ~PHY_MARVELL_E3016_INITMASK;
        if (phyRW(Priv.phyaddr, MII_NCONFIG, reg)) {
            return 1;
        }
    }

    if (Priv.phy_oui == PHY_OUI_REALTEK) {
        if (Priv.phy_model == PHY_MODEL_REALTEK_8211 && Priv.phy_rev == PHY_REV_REALTEK_8211B) {
            if (phyRW(Priv.phyaddr, PHY_REALTEK_INIT_REG1, PHY_REALTEK_INIT1)) {
                return 1;
            }
            if (phyRW(Priv.phyaddr, PHY_REALTEK_INIT_REG2, PHY_REALTEK_INIT2)) {
                return 1;
            }
            if (phyRW(Priv.phyaddr, PHY_REALTEK_INIT_REG1, PHY_REALTEK_INIT3)) {
                return 1;
            }
            if (phyRW(Priv.phyaddr, PHY_REALTEK_INIT_REG3, PHY_REALTEK_INIT4)) {
                return 1;
            }
            if (phyRW(Priv.phyaddr, PHY_REALTEK_INIT_REG4, PHY_REALTEK_INIT5)) {
                return 1;
            }
            if (phyRW(Priv.phyaddr, PHY_REALTEK_INIT_REG5, PHY_REALTEK_INIT6)) {
                return 1;
            }
            if (phyRW(Priv.phyaddr, PHY_REALTEK_INIT_REG1, PHY_REALTEK_INIT1)) {
                return 1;
            }
        }
        if (Priv.phy_model == PHY_MODEL_REALTEK_8211 && Priv.phy_rev == PHY_REV_REALTEK_8211C) {

            powerstate = NV_R32(NvRegPowerState2);

            /* need to perform hw phy reset */
            powerstate |= NVREG_POWERSTATE2_PHY_RESET;
            NV_W32(powerstate, NvRegPowerState2);
            msleep(25);

            powerstate &= ~NVREG_POWERSTATE2_PHY_RESET;
            NV_W32(powerstate, NvRegPowerState2);
            msleep(25);

            reg = phyRW(Priv.phyaddr, PHY_REALTEK_INIT_REG6, MII_READ);
            reg |= PHY_REALTEK_INIT9;
            if (phyRW(Priv.phyaddr, PHY_REALTEK_INIT_REG6, reg)) {
                return 1;
            }
            if (phyRW(Priv.phyaddr, PHY_REALTEK_INIT_REG1, PHY_REALTEK_INIT10)) {
                return 1;
            }
            reg = phyRW(Priv.phyaddr, PHY_REALTEK_INIT_REG7, MII_READ);
            if (!(reg & PHY_REALTEK_INIT11)) {
                reg |= PHY_REALTEK_INIT11;
                if (phyRW(Priv.phyaddr, PHY_REALTEK_INIT_REG7, reg)) {
                    return 1;
                }
            }
            if (phyRW(Priv.phyaddr, PHY_REALTEK_INIT_REG1, PHY_REALTEK_INIT1)) {
                return 1;
            }
        }
        if (Priv.phy_model == PHY_MODEL_REALTEK_8201) {
            if (Priv.flags & DEV_NEED_PHY_INIT_FIX) {
                phy_reserved = phyRW(Priv.phyaddr, PHY_REALTEK_INIT_REG6, MII_READ);
                phy_reserved |= PHY_REALTEK_INIT7;
                if (phyRW(Priv.phyaddr, PHY_REALTEK_INIT_REG6, phy_reserved)) {
                    return 1;
                }
            }
        }
    }

    /* set advertise register */
    reg = phyRW(Priv.phyaddr, MII_ADVERTISE, MII_READ);
    reg |= (ADVERTISE_10HALF|ADVERTISE_10FULL|ADVERTISE_100HALF|ADVERTISE_100FULL|ADVERTISE_PAUSE_ASYM|ADVERTISE_PAUSE_CAP);
    if (phyRW(Priv.phyaddr, MII_ADVERTISE, reg)) {
        return 1;
    }

    /* get phy interface type */
    phyinterface = NV_R32(NvRegPhyInterface);

    /* see if gigabit phy */
    mii_status = phyRW(Priv.phyaddr, MII_BMSR, MII_READ);
    if (mii_status & PHY_GIGABIT) {
        Priv.gigabit = PHY_GIGABIT;
        mii_control_1000 = phyRW(Priv.phyaddr, MII_CTRL1000, MII_READ);
        mii_control_1000 &= ~ADVERTISE_1000HALF;
        if (phyinterface & PHY_RGMII) {
            mii_control_1000 |= ADVERTISE_1000FULL;
        } else {
            mii_control_1000 &= ~ADVERTISE_1000FULL;
        }

        if (phyRW(Priv.phyaddr, MII_CTRL1000, mii_control_1000)) {
            return 1;
        }
    } else {
        Priv.gigabit = 0;
    }

    mii_control = phyRW(Priv.phyaddr, MII_BMCR, MII_READ);
    mii_control |= BMCR_ANENABLE;

    if (Priv.phy_oui == PHY_OUI_REALTEK
        && Priv.phy_model == PHY_MODEL_REALTEK_8211
        && Priv.phy_rev == PHY_REV_REALTEK_8211C)
    {
        /* start autoneg since we already performed hw reset above */
        mii_control |= BMCR_ANRESTART;
        if (phyRW(Priv.phyaddr, MII_BMCR, mii_control)) {
            return 1;
        }
    } else {
        /* reset the phy (certain phys need bmcr to be setup with reset) */
        if (phyReset(mii_control)) {
            return 1;
        }
    }

    /* phy vendor specific configuration */
    if (Priv.phy_oui == PHY_OUI_CICADA && phyinterface & PHY_RGMII) {
        phy_reserved = phyRW(Priv.phyaddr, MII_RESV1, MII_READ);
        phy_reserved &= ~(PHY_CICADA_INIT1 | PHY_CICADA_INIT2);
        phy_reserved |= (PHY_CICADA_INIT3 | PHY_CICADA_INIT4);
        if (phyRW(Priv.phyaddr, MII_RESV1, phy_reserved)) {
            return 1;
        }
        phy_reserved = phyRW(Priv.phyaddr, MII_NCONFIG, MII_READ);
        phy_reserved |= PHY_CICADA_INIT5;
        if (phyRW(Priv.phyaddr, MII_NCONFIG, phy_reserved)) {
            return 1;
        }
    }
    if (Priv.phy_oui == PHY_OUI_CICADA) {
        phy_reserved = phyRW(Priv.phyaddr, MII_SREVISION, MII_READ);
        phy_reserved |= PHY_CICADA_INIT6;
        if (phyRW(Priv.phyaddr, MII_SREVISION, phy_reserved)) {
            return 1;
        }
    }
    if (Priv.phy_oui == PHY_OUI_VITESSE) {
        if (phyRW(Priv.phyaddr, PHY_VITESSE_INIT_REG1, PHY_VITESSE_INIT1)) {
            return 1;
        }
        if (phyRW(Priv.phyaddr, PHY_VITESSE_INIT_REG2, PHY_VITESSE_INIT2)) {
            return 1;
        }
        phy_reserved = phyRW(Priv.phyaddr, PHY_VITESSE_INIT_REG4, MII_READ);
        if (phyRW(Priv.phyaddr, PHY_VITESSE_INIT_REG4, phy_reserved)) {
            return 1;
        }
        phy_reserved = phyRW(Priv.phyaddr, PHY_VITESSE_INIT_REG3, MII_READ);
        phy_reserved &= ~PHY_VITESSE_INIT_MSK1;
        phy_reserved |= PHY_VITESSE_INIT3;
        if (phyRW(Priv.phyaddr, PHY_VITESSE_INIT_REG3, phy_reserved)) {
            return 1;
        }
        if (phyRW(Priv.phyaddr, PHY_VITESSE_INIT_REG2, PHY_VITESSE_INIT4)) {
            return 1;
        }
        if (phyRW(Priv.phyaddr, PHY_VITESSE_INIT_REG2, PHY_VITESSE_INIT5)) {
            return 1;
        }
        phy_reserved = phyRW(Priv.phyaddr, PHY_VITESSE_INIT_REG4, MII_READ);
        phy_reserved &= ~PHY_VITESSE_INIT_MSK1;
        phy_reserved |= PHY_VITESSE_INIT3;
        if (phyRW(Priv.phyaddr, PHY_VITESSE_INIT_REG4, phy_reserved)) {
            return 1;
        }
        phy_reserved = phyRW(Priv.phyaddr, PHY_VITESSE_INIT_REG3, MII_READ);
        if (phyRW(Priv.phyaddr, PHY_VITESSE_INIT_REG3, phy_reserved)) {
            return 1;
        }
        if (phyRW(Priv.phyaddr, PHY_VITESSE_INIT_REG2, PHY_VITESSE_INIT6)) {
            return 1;
        }
        if (phyRW(Priv.phyaddr, PHY_VITESSE_INIT_REG2, PHY_VITESSE_INIT7)) {
            return 1;
        }
        phy_reserved = phyRW(Priv.phyaddr, PHY_VITESSE_INIT_REG4, MII_READ);
        if (phyRW(Priv.phyaddr, PHY_VITESSE_INIT_REG4, phy_reserved)) {
            return 1;
        }
        phy_reserved = phyRW(Priv.phyaddr, PHY_VITESSE_INIT_REG3, MII_READ);
        phy_reserved &= ~PHY_VITESSE_INIT_MSK2;
        phy_reserved |= PHY_VITESSE_INIT8;
        if (phyRW(Priv.phyaddr, PHY_VITESSE_INIT_REG3, phy_reserved)) {
            return 1;
        }
        if (phyRW(Priv.phyaddr, PHY_VITESSE_INIT_REG2, PHY_VITESSE_INIT9)) {
            return 1;
        }
        if (phyRW(Priv.phyaddr, PHY_VITESSE_INIT_REG1, PHY_VITESSE_INIT10)) {
            return 1;
        }
    }
    if (Priv.phy_oui == PHY_OUI_REALTEK) {
        if (Priv.phy_model == PHY_MODEL_REALTEK_8211 && Priv.phy_rev == PHY_REV_REALTEK_8211B) {
            /* reset could have cleared these out, set them back */
            if (phyRW(Priv.phyaddr, PHY_REALTEK_INIT_REG1, PHY_REALTEK_INIT1)) {
                return 1;
            }
            if (phyRW(Priv.phyaddr, PHY_REALTEK_INIT_REG2, PHY_REALTEK_INIT2)) {
                return 1;
            }
            if (phyRW(Priv.phyaddr, PHY_REALTEK_INIT_REG1, PHY_REALTEK_INIT3)) {
                return 1;
            }
            if (phyRW(Priv.phyaddr, PHY_REALTEK_INIT_REG3, PHY_REALTEK_INIT4)) {
                return 1;
            }
            if (phyRW(Priv.phyaddr, PHY_REALTEK_INIT_REG4, PHY_REALTEK_INIT5)) {
                return 1;
            }
            if (phyRW(Priv.phyaddr, PHY_REALTEK_INIT_REG5, PHY_REALTEK_INIT6)) {
                return 1;
            }
            if (phyRW(Priv.phyaddr, PHY_REALTEK_INIT_REG1, PHY_REALTEK_INIT1)) {
                return 1;
            }
        }
        if (Priv.phy_model == PHY_MODEL_REALTEK_8201) {
            if (Priv.flags & DEV_NEED_PHY_INIT_FIX) {
                phy_reserved = phyRW(Priv.phyaddr, PHY_REALTEK_INIT_REG6, MII_READ);
                phy_reserved |= PHY_REALTEK_INIT7;
                if (phyRW(Priv.phyaddr, PHY_REALTEK_INIT_REG6, phy_reserved)) {
                    return 1;
                }
            }
        }
    }

    /* some phys clear out pause advertisment on reset, set it back */
    phyRW(Priv.phyaddr, MII_ADVERTISE, reg);

    /* restart auto negotiation, power down phy */
    mii_control = phyRW(Priv.phyaddr, MII_BMCR, MII_READ);
    mii_control |= (BMCR_ANRESTART | BMCR_ANENABLE);
    if (phyRW(Priv.phyaddr, MII_BMCR, mii_control)) {
        return 1;
    }

    return 0;
}

void macReset(void)
{
    NV_W32(NVREG_TXRXCTL_BIT2 | NVREG_TXRXCTL_RESET | Priv.txrxctl_bits, NvRegTxRxControl);
    NV_R32(0);
    NV_W32(NVREG_MAC_RESET_ASSERT, NvRegMacReset);
    NV_R32(0);
    udelay(NV_MAC_RESET_DELAY);
    NV_W32(0, NvRegMacReset);
    NV_R32(0);
    udelay(NV_MAC_RESET_DELAY);
    NV_W32(NVREG_TXRXCTL_BIT2 | Priv.txrxctl_bits, NvRegTxRxControl);
    NV_R32(0);
}

int initRing(void)
{
    u16 i;
    u32 flaglen;

    int rc = DevHelp_AllocGDTSelector(Priv.gdt, 2);
    if (rc) return 1;

    for (i = 0; i < Priv.rx_ring_size; i++)
    {
        flaglen = cpu_to_le32((Priv.rx_buf_sz + NV_RX_AVAIL));
        if (Priv.desc_ver == DESC_VER_1 || Priv.desc_ver == DESC_VER_2)
        {
            Priv.rx_ring.orig[i].flaglen = flaglen;
            Priv.rx_ring.orig[i].buf = cpu_to_le32(GetRxBufferPhysicalAddress(Priv.pDev, i));
        }
        else
        {
            Priv.rx_ring.ex[i].flaglen = flaglen;
            Priv.rx_ring.ex[i].txvlan = 0;
            Priv.rx_ring.ex[i].bufhigh = 0;
            Priv.rx_ring.ex[i].buflow = cpu_to_le32(GetRxBufferPhysicalAddress(Priv.pDev, i));
        }
    }

    for (i = 0; i < Priv.tx_ring_size; i++)
    {
        if (Priv.desc_ver == DESC_VER_1 || Priv.desc_ver == DESC_VER_2)
        {
            Priv.tx_ring.orig[i].flaglen = 0;
            Priv.tx_ring.orig[i].buf = 0;
        }
        else
        {
            Priv.tx_ring.ex[i].flaglen = 0;
            Priv.tx_ring.ex[i].txvlan = 0;
            Priv.tx_ring.ex[i].bufhigh = 0;
            Priv.tx_ring.ex[i].buflow = 0;
        }
    }

    Priv.rxpos = 0;
    Priv.txpos = 0;

    return 0;
}

void txrxReset(void)
{
    NV_W32(NVREG_TXRXCTL_BIT2 | NVREG_TXRXCTL_RESET | Priv.txrxctl_bits, NvRegTxRxControl);
    NV_R32(0);
    udelay(NV_TXRX_RESET_DELAY);
    NV_W32(NVREG_TXRXCTL_BIT2 | Priv.txrxctl_bits, NvRegTxRxControl);
    NV_R32(0);
}

void setupHWrings(u16 rxtx_flags)
{
    if (Priv.desc_ver == DESC_VER_1 || Priv.desc_ver == DESC_VER_2)
    {
        if (rxtx_flags & NV_SETUP_RX_RING)
        {
            NV_W32(cpu_to_le32(Priv.pDev->rx.RingDma), NvRegRxRingPhysAddr);
        }
        if (rxtx_flags & NV_SETUP_TX_RING)
        {
            NV_W32(cpu_to_le32(Priv.pDev->tx.RingDma), NvRegTxRingPhysAddr);
        }
    }
    else
    {
        if (rxtx_flags & NV_SETUP_RX_RING)
        {
            NV_W32(cpu_to_le32(Priv.pDev->rx.RingDma), NvRegRxRingPhysAddr);
            NV_W32(0, NvRegRxRingPhysAddrHigh);
        }
        if (rxtx_flags & NV_SETUP_TX_RING)
        {
            NV_W32(cpu_to_le32(Priv.pDev->tx.RingDma), NvRegTxRingPhysAddr);
            NV_W32(0, NvRegTxRingPhysAddrHigh);
        }
    }
}

void disableHWInterrupts(u32 mask)
{
    DPRINTF(1, "disableHWInterrupts: %x\n", mask);
    if (Priv.msi_flags & NV_MSI_X_ENABLED)
    {
        NV_W32(mask, NvRegIrqMask);
    }
    else
    {
        if (Priv.msi_flags & NV_MSI_ENABLED)
        {
            NV_W32(0, NvRegMSIIrqMask);
        }
        NV_W32(0, NvRegIrqMask);
    }
}

void updatePause(u32 pause_flags)
{
    pause_flags &= ~(NV_PAUSEFRAME_TX_ENABLE | NV_PAUSEFRAME_RX_ENABLE);

    if (pause_flags & NV_PAUSEFRAME_RX_CAPABLE)
    {
        u32 pff = NV_R32(NvRegPacketFilterFlags) & ~NVREG_PFF_PAUSE_RX;
        if (pause_flags & NV_PAUSEFRAME_RX_ENABLE)
        {
            NV_W32(pff|NVREG_PFF_PAUSE_RX, NvRegPacketFilterFlags);
            pause_flags |= NV_PAUSEFRAME_RX_ENABLE;
        }
        else
        {
            NV_W32(pff, NvRegPacketFilterFlags);
        }
    }
    if (pause_flags & NV_PAUSEFRAME_TX_CAPABLE)
    {
        u32 regmisc = NV_R32(NvRegMisc1) & ~NVREG_MISC1_PAUSE_TX;
        if (pause_flags & NV_PAUSEFRAME_TX_ENABLE)
        {
            NV_W32(NVREG_TX_PAUSEFRAME_ENABLE,  NvRegTxPauseFrame);
            NV_W32(regmisc|NVREG_MISC1_PAUSE_TX, NvRegMisc1);
            pause_flags |= NV_PAUSEFRAME_TX_ENABLE;
        }
        else
        {
            NV_W32(NVREG_TX_PAUSEFRAME_DISABLE,  NvRegTxPauseFrame);
            NV_W32(regmisc, NvRegMisc1);
        }
    }
}

int updateLinkspeed(void)
{
    u32 adv = 0;
    u32 lpa = 0;
    u32 adv_lpa, adv_pause, lpa_pause;
    u32 newls = Priv.linkspeed;
    u16 newdup = Priv.duplex;
    long mii_status;
    int retval = 0;
    u32 control_1000, status_1000, phyreg, pause_flags, txreg, phy_exp;
    long status2;

    /* BMSR_LSTATUS is latched, read it twice: we want the current value. */

    mii_status = phyRW(Priv.phyaddr, MII_BMSR, MII_READ);
    status2 = phyRW(Priv.phyaddr, MII_BMSR, MII_READ);
    if (status2 != -1L) mii_status = status2;

    do {
        if (Priv.autoneg == 0) {
            DPRINTF(1, "updateLinkspeed: autoneg off, PHY set to %x\n", fixed_mode);
            if (fixed_mode & LPA_100FULL) {
                newls = NVREG_LINKSPEED_FORCE|NVREG_LINKSPEED_100;
                newdup = 1;
            } else
            if (fixed_mode & LPA_100HALF) {
                newls = NVREG_LINKSPEED_FORCE|NVREG_LINKSPEED_100;
                newdup = 0;
            } else
            if (fixed_mode & LPA_10FULL) {
                newls = NVREG_LINKSPEED_FORCE|NVREG_LINKSPEED_10;
                newdup = 1;
            }
            else {
                newls = NVREG_LINKSPEED_FORCE|NVREG_LINKSPEED_10;
                newdup = 0;
            }
            retval = 1;
            break;
        }

        if (mii_status == -1L || !(mii_status & BMSR_LSTATUS)) {
            DPRINTF(1, "updateLinkspeed: no link detected by phy - falling back to 10HD\n");
            newls = NVREG_LINKSPEED_FORCE|NVREG_LINKSPEED_10;
            newdup = 0;
            retval = 0;
            break;
        }

        /* check auto negotiation is complete */
        if (!(mii_status & BMSR_ANEGCOMPLETE)) {
            /* still in autonegotiation - configure nic for 10 MBit HD and wait. */
            newls = NVREG_LINKSPEED_FORCE|NVREG_LINKSPEED_10;
            newdup = 0;
            retval = 0;
            DPRINTF(1, "updateLinkspeed: autoneg not completed - falling back to 10HD\n");
            break;
        }

        adv = phyRW(Priv.phyaddr, MII_ADVERTISE, MII_READ);
        lpa = phyRW(Priv.phyaddr, MII_LPA, MII_READ);
        DPRINTF(1, "updateLinkspeed: PHY advertises %lx lpa=%lx\n", adv, lpa);

        retval = 1;
        if (Priv.gigabit == PHY_GIGABIT) {
            control_1000 = phyRW(Priv.phyaddr, MII_CTRL1000, MII_READ);
            status_1000 = phyRW(Priv.phyaddr, MII_STAT1000, MII_READ);

            if ((control_1000 & ADVERTISE_1000FULL) && (status_1000 & LPA_1000FULL)) {
                DPRINTF(1, "updateLinkspeed: GBit ethernet detected\n");
                newls = NVREG_LINKSPEED_FORCE|NVREG_LINKSPEED_1000;
                newdup = 1;
                break;
            }
        }

        adv_lpa = lpa & adv;
        if (adv_lpa & LPA_100FULL) {
            newls = NVREG_LINKSPEED_FORCE|NVREG_LINKSPEED_100;
            newdup = 1;
        } else
        if (adv_lpa & LPA_100HALF) {
            newls = NVREG_LINKSPEED_FORCE|NVREG_LINKSPEED_100;
            newdup = 0;
        } else
        if (adv_lpa & LPA_10FULL) {
            newls = NVREG_LINKSPEED_FORCE|NVREG_LINKSPEED_10;
            newdup = 1;
        } else
        if (adv_lpa & LPA_10HALF) {
            newls = NVREG_LINKSPEED_FORCE|NVREG_LINKSPEED_10;
            newdup = 0;
        }
        else {
            DPRINTF(1, "updateLinkspeed: bad ability %x - falling back to 10HD\n", adv_lpa);
            newls = NVREG_LINKSPEED_FORCE|NVREG_LINKSPEED_10;
            newdup = 0;
        }
    } while (0);

    DPRINTF(1, "updateLinkspeed: old: %lx/%x New: %lx/%x\n", Priv.linkspeed, Priv.duplex, newls, newdup);

    if ((newls & NVREG_LINKSPEED_MASK) == NVREG_LINKSPEED_10) {
        AdapterSC.MscLinkSpd = 10000000;
    } else
    if ((newls & NVREG_LINKSPEED_MASK) == NVREG_LINKSPEED_100) {
        AdapterSC.MscLinkSpd = 100000000;
    } else
    if ((newls & NVREG_LINKSPEED_MASK) == NVREG_LINKSPEED_1000) {
        AdapterSC.MscLinkSpd = 1000000000;
    }

    if (Priv.duplex == newdup && Priv.linkspeed == newls) {
        return retval;
    }

    Priv.duplex = newdup;
    Priv.linkspeed = newls;

    if (Priv.gigabit == PHY_GIGABIT) {
        phyreg = NV_R32(NvRegRandomSeed);
        phyreg &= ~(0x3FF00);
        if ((Priv.linkspeed & NVREG_LINKSPEED_MASK) == NVREG_LINKSPEED_10) {
            phyreg |= NVREG_RNDSEED_FORCE3;
        } else
        if ((Priv.linkspeed & NVREG_LINKSPEED_MASK) == NVREG_LINKSPEED_100) {
            phyreg |= NVREG_RNDSEED_FORCE2;
        } else
        if ((Priv.linkspeed & NVREG_LINKSPEED_MASK) == NVREG_LINKSPEED_1000) {
            phyreg |= NVREG_RNDSEED_FORCE;
        }
        NV_W32(phyreg, NvRegRandomSeed);
    }

    phyreg = NV_R32(NvRegPhyInterface);
    phyreg &= ~(PHY_HALF|PHY_100|PHY_1000);
    if (Priv.duplex == 0) {
        phyreg |= PHY_HALF;
    }
    if ((Priv.linkspeed & NVREG_LINKSPEED_MASK) == NVREG_LINKSPEED_100) {
        phyreg |= PHY_100;
    } else
    if ((Priv.linkspeed & NVREG_LINKSPEED_MASK) == NVREG_LINKSPEED_1000) {
        phyreg |= PHY_1000;
    }
    NV_W32(phyreg, NvRegPhyInterface);

    phy_exp = phyRW(Priv.phyaddr, MII_EXPANSION, MII_READ) & EXPANSION_NWAY;
    if (phyreg & PHY_RGMII) {
        if ((Priv.linkspeed & NVREG_LINKSPEED_MASK) == NVREG_LINKSPEED_1000) {
            txreg = NVREG_TX_DEFERRAL_RGMII_1000;
        }
        else {
            if (!phy_exp && !Priv.duplex && (Priv.flags & DEV_HAS_COLLISION_FIX)) {
                if ((Priv.linkspeed & NVREG_LINKSPEED_MASK) == NVREG_LINKSPEED_10) {
                    txreg = NVREG_TX_DEFERRAL_RGMII_STRETCH_10;
                } else {
                    txreg = NVREG_TX_DEFERRAL_RGMII_STRETCH_100;
                }
            } else {
                txreg = NVREG_TX_DEFERRAL_RGMII_10_100;
            }
        }
    } else {
        if (!phy_exp && !Priv.duplex && (Priv.flags & DEV_HAS_COLLISION_FIX)) {
            txreg = NVREG_TX_DEFERRAL_MII_STRETCH;
        } else {
            txreg = NVREG_TX_DEFERRAL_DEFAULT;
        }
    }
    NV_W32(txreg, NvRegTxDeferral);

    if (Priv.desc_ver == DESC_VER_1) {
        txreg = NVREG_TX_WM_DESC1_DEFAULT;
    } else {
        if ((Priv.linkspeed & NVREG_LINKSPEED_MASK) == NVREG_LINKSPEED_1000) {
            txreg = NVREG_TX_WM_DESC2_3_1000;
        } else {
            txreg = NVREG_TX_WM_DESC2_3_DEFAULT;
        }
    }
    NV_W32(txreg, NvRegTxWatermark);

    NV_W32(NVREG_MISC1_FORCE | ( Priv.duplex ? 0 : NVREG_MISC1_HD), NvRegMisc1);
    NV_R32(0);
    NV_W32(Priv.linkspeed, NvRegLinkSpeed);
    NV_R32(0);

    pause_flags = 0;
    /* setup pause frame */
    if (Priv.duplex != 0) {
        if (Priv.autoneg && pause_flags & NV_PAUSEFRAME_AUTONEG) {
            adv_pause = adv & (ADVERTISE_PAUSE_CAP| ADVERTISE_PAUSE_ASYM);
            lpa_pause = lpa & (LPA_PAUSE_CAP| LPA_PAUSE_ASYM);

            switch (adv_pause) {
                case ADVERTISE_PAUSE_CAP:
                {
                    if (lpa_pause & LPA_PAUSE_CAP) {
                        pause_flags |= NV_PAUSEFRAME_RX_ENABLE;
                        if (pause_flags & NV_PAUSEFRAME_TX_REQ) {
                            pause_flags |= NV_PAUSEFRAME_TX_ENABLE;
                        }
                    }
                } break;
                case ADVERTISE_PAUSE_ASYM:
                {
                    if (lpa_pause == (LPA_PAUSE_CAP| LPA_PAUSE_ASYM)) {
                        pause_flags |= NV_PAUSEFRAME_TX_ENABLE;
                    }
                } break;
                case ADVERTISE_PAUSE_CAP| ADVERTISE_PAUSE_ASYM:
                {
                    if (lpa_pause & LPA_PAUSE_CAP) {
                        pause_flags |=  NV_PAUSEFRAME_RX_ENABLE;
                        if (pause_flags & NV_PAUSEFRAME_TX_REQ) {
                            pause_flags |= NV_PAUSEFRAME_TX_ENABLE;
                        }
                    }
                    if (lpa_pause == LPA_PAUSE_ASYM) {
                        pause_flags |= NV_PAUSEFRAME_RX_ENABLE;
                    }
                } break;
            }
        } else  {
            pause_flags = pause_flags;
        }
    }
    updatePause(pause_flags);

    return retval;
}

void startRX(void)
{
    u32 rx_ctrl = NV_R32(NvRegReceiverControl);

    /* Already running? Stop it. */
    if ((NV_R32(NvRegReceiverControl) & NVREG_RCVCTL_START) && !Priv.mac_in_use)
    {
        rx_ctrl &= ~NVREG_RCVCTL_START;
        NV_W32(rx_ctrl, NvRegReceiverControl);
        NV_R32(0);
    }
    NV_W32(Priv.linkspeed, NvRegLinkSpeed);
    NV_R32(0);
    rx_ctrl |= NVREG_RCVCTL_START;
    if (Priv.mac_in_use)
    {
        rx_ctrl &= ~NVREG_RCVCTL_RX_PATH_EN;
    }
    NV_W32(rx_ctrl, NvRegReceiverControl);
    Priv.rxstarted = 1;
    NV_R32(0);
}

void startTX(void)
{
    u32 tx_ctrl = NV_R32(NvRegTransmitterControl);

    tx_ctrl |= NVREG_XMITCTL_START;
    if (Priv.mac_in_use)
    {
        tx_ctrl &= ~NVREG_XMITCTL_TX_PATH_EN;
    }
    NV_W32(tx_ctrl, NvRegTransmitterControl);
    NV_R32(0);
}

/* In MSIX mode, a write to irqmask behaves as XOR */
void enableHWInterrupts(u32 mask)
{
    DPRINTF(1, "enableHWInterrupts: %x\n", mask);
    NV_W32(mask, NvRegIrqMask);
}

/* Called when the nic notices a mismatch between the actual data len on the
 * wire and the len indicated in the 802 header
 */
long getlen(fpu8 packet, long datalen)
{
    u16 hdrlen; /* length of the 802 header */
    u16 protolen; /* length as stored in the proto field */

    /* 1) calculate len according to header */
    {
        protolen = ntohs(((struct ethhdr far *)packet)->h_proto);
        hdrlen = ETH_HLEN;
    }
    if (protolen > ETH_DATA_LEN) {
        return datalen; /* Value in proto field not a len, no checks possible */
    }

    protolen += hdrlen;
    /* consistency checks: */
    if (datalen > ETH_ZLEN) {
        if (datalen >= protolen) {
            /* more data on wire than in 802 header, trim of additional data. */
            return protolen;
        } else {
            /* less data on wire than mentioned in header. Discard the packet. */
            return -1;
        }
    } else {
        /* short packet. Accept only if 802 values are also short */
        if (protolen > ETH_ZLEN) {
            return -1;
        }
        return datalen;
    }
}

int rxProcess(void)
{
  u32 flags;
  long len;
  u16 release;
  fpu8 data, pcInd;

  int i;

  TraceArgs(0x0010, 6, (u16)Priv.rxpos, (u32)Priv.rx_ring.orig[Priv.rxpos].flaglen);
  for (i = 0; !((flags = le32_to_cpu(Priv.rx_ring.orig[Priv.rxpos].flaglen)) & NV_RX_AVAIL) && i < Priv.rx_ring_size; Priv.rxpos = ((Priv.rxpos + 1 >= Priv.rx_ring_size) ? 0 : Priv.rxpos + 1), i++)
  {
    if (NdisGetInfo(NDISINFO_Indications)) return i;
    data = GetRxBufferVirtualAddress(Priv.pDev, Priv.rxpos);
    if (!data) return -1;
    pcInd = &data[NV_RX_ALLOCATE - 1];

    do {
      release = 1;

      /* look at what we actually got: */
      if (Priv.desc_ver == DESC_VER_1) {
        if (flags & NV_RX_DESCRIPTORVALID) {
          len = flags & LEN_MASK_V1;
          if (flags & NV_RX_ERROR) {
            if (flags & NV_RX_ERROR4) {
              len = getlen(data, len);
              if (len < 0) break;
            }
            /* framing errors are soft errors */
            else if (flags & NV_RX_FRAMINGERR) {
              if (flags & NV_RX_SUBSTRACT1) len--;
            }
            /* the rest are hard errors */
            else break;
          }
        } else break;
      } else { /* if !(Priv.desc_ver == DESC_VER_1) */
        if (flags & NV_RX2_DESCRIPTORVALID) {
          len = flags & LEN_MASK_V2;
          if (flags & NV_RX2_ERROR) {
            if (flags & NV_RX2_ERROR4) {
              len = getlen(data, len);
              if (len < 0) break;
            }
            /* framing errors are soft errors */
            else if (flags & NV_RX2_FRAMINGERR) {
              if (flags & NV_RX2_SUBSTRACT1) len--;
            }
            /* the rest are hard errors */
            else break;
          }
        } else break;
      }

      if (len > 0) {
        /* got a valid packet - forward it to the network core */
        NdisIndicationsOff();
        *pcInd = 0xFF;

        TraceArgs(0x0014, 8, (u16)Priv.rxpos, (u16)len, (u32)flags);
        TraceBuf(0x0011, len<64?len:64, data);

        if (dorxchain) {
          u16 rc;

          RxBufOne far * rxbuf = (RxBufOne far *)(data + NV_RX_ALLOCATE - 1 - sizeof(RxBufOne));
          rxbuf->RxDataCount = 1;
          rxbuf->RxDataBlk.RxDataLen = (u16)len;
          rxbuf->RxDataBlk.RxDataPtr = (fpu8)data;
          rc = ProtDT.PldRcvChain(AdapterCC.CcModuleID, (u16)len, Priv.rxpos, (fpu8)rxbuf, pcInd, ProtCC.CcDataSeg);
          if (rc == 1) release = 0;
        } else {
          Priv.currentrxbuffer = data;
          Priv.currentlen = (u16)len;
          ProtDT.PldRcvLkAhead(AdapterCC.CcModuleID, Priv.currentlen, Priv.currentlen, Priv.currentrxbuffer, pcInd, ProtCC.CcDataSeg);
          Priv.currentrxbuffer = NULL;
          Priv.currentlen = 0;
        }

        if (*pcInd) NdisIndicationsOn();
        ProtDT.PldIndComplete(AdapterCC.CcModuleID, ProtCC.CcDataSeg);
      }
    } while (0);
    if (release) {
      Priv.rx_ring.orig[Priv.rxpos].flaglen = cpu_to_le32(Priv.rx_buf_sz | NV_RX_AVAIL);
    }
  }

  TraceBuf(0x8010, 0, NULL);
  return i;
}

void linkChange(void)
{
    if (updateLinkspeed()) {
        if (!Priv.rxstarted) {
            startRX();
        }
    } else {
        if (Priv.rxstarted) {
        }
    }
}

void linkIRQ(void)
{
    u32 miistat = NV_R32(NvRegMIIStatus);
    NV_W32(NVREG_MIISTAT_MASK, NvRegMIIStatus);

    if (miistat & (NVREG_MIISTAT_LINKCHANGE)) {
        linkChange();
    }
}

#define TX_WORK_PER_LOOP  64
#define RX_WORK_PER_LOOP  64
static void far IrqHandler(void)
{
    u32 events;
    u16 i;
    u16 timer = 0;

    DevCli();
    for (i=0; ; i++) {
        if (!(Priv.msi_flags & NV_MSI_X_ENABLED)) {
            events = NV_R32(NvRegIrqStatus) & NVREG_IRQSTAT_MASK;
            NV_W32(NVREG_IRQSTAT_MASK, NvRegIrqStatus);
        } else {
            events = NV_R32(NvRegMSIXIrqStatus) & NVREG_IRQSTAT_MASK;
            NV_W32(NVREG_IRQSTAT_MASK, NvRegMSIXIrqStatus);
        }
        NV_R32(0); /* force out pending posted writes */

        if (!(events & Priv.irqmask)) {
            break;
        }

        TraceArgs(0x0020, 4, events);

        if (events & NVREG_IRQ_TIMER) {
            timer++;
            if (timer > 1 && !(events & ~NVREG_IRQ_TIMER)) {
                continue;
            }
        }

        if (Priv.desc_ver == DESC_VER_3) {
            u32 flaglen = le32_to_cpu(Priv.tx_ring.ex[Priv.txpos].flaglen);
            if (!(flaglen & NV_TX2_VALID) && (flaglen & LEN_MASK_V2)) {
                DevHelp_ProcRun(Priv.pDev->tx.RingDma);
            }
        } else {
            u32 flaglen = le32_to_cpu(Priv.tx_ring.orig[Priv.txpos].flaglen);
            if (!(flaglen & NV_TX_VALID) && (flaglen & LEN_MASK_V1)) {
                DevHelp_ProcRun(Priv.pDev->tx.RingDma);
            }
        }

        /* Check for receive packets every interrupt (even if no receives are ready).
         * Think of it as a kind of semi-polling since actual polling is not implemented.
         * It seems to make a difference on older hardware.
         */
        if (mutex_request(&Priv.mutex)) {
            rxProcess();
            mutex_unlock(&Priv.mutex);
        }

        if (events & NVREG_IRQ_LINK) {
            if (mutex_request(&Priv.mutex)) {
                linkIRQ();
                mutex_unlock(&Priv.mutex);
            }
        }

        if (Priv.need_linktimer && TimeDiff() >= LINK_TIMEOUT) {
            if (mutex_request(&Priv.mutex)) {
                linkChange();
                TimeReset();
                mutex_unlock(&Priv.mutex);
            }
        }

        if (events & (NVREG_IRQ_TX_ERR)) {
            DPRINTF(1, "TX error!\n");
        }

        if (events & (NVREG_IRQ_UNKNOWN)) {
            DPRINTF(1, "Unknown IRQ!\n");
        }

        if (events & NVREG_IRQ_RECOVER_ERROR) {
            DPRINTF(1, "IRQ Recover Error!\n");
            break;
        }

        if (events & NVREG_IRQ_RX_ERROR) {
            DPRINTF(1, "RX error!\n");
        }

        if (events & NVREG_IRQ_RX_NOBUF) {
            DPRINTF(1, "No RX buffer!\n");
        }

        if (i > max_interrupt_work) {
            DPRINTF(1, "Max IRQ work!\n");
            break;
        }

    }

    if (i)
    {
        DevCli();
        DevHelp_EOI(Priv.pDev->irq);
        DevClc();
    }
    else
    {
        DevSti();
        DevStc();
    }
}

void drainTX(void)
{
    int i;

    for (i = 0; i < Priv.tx_ring_size; i++)
    {
        if (Priv.desc_ver == DESC_VER_1 || Priv.desc_ver == DESC_VER_2)
        {
            Priv.tx_ring.orig[i].flaglen = 0;
            Priv.tx_ring.orig[i].buf = 0;
        }
        else
        {
            Priv.tx_ring.ex[i].flaglen = 0;
            Priv.tx_ring.ex[i].txvlan = 0;
            Priv.tx_ring.ex[i].bufhigh = 0;
            Priv.tx_ring.ex[i].buflow = 0;
        }
    }
}

void drainRX(void)
{
    int i;
    u32 dma;

    dma = Priv.rx_ring.orig[0].buf;
    for (i = 0; i < Priv.rx_ring_size; i++)
    {
        if (Priv.desc_ver == DESC_VER_1 || Priv.desc_ver == DESC_VER_2)
        {
            Priv.rx_ring.orig[i].flaglen = 0;
            Priv.rx_ring.orig[i].buf = 0;
        }
        else
        {
            Priv.rx_ring.ex[i].flaglen = 0;
            Priv.rx_ring.ex[i].txvlan = 0;
            Priv.rx_ring.ex[i].bufhigh = 0;
            Priv.rx_ring.ex[i].buflow = 0;
        }
    }
    if (dma)
    {
        DevHelp_FreePhys(dma);
    }
}

void drainRing(void)
{
    drainTX();
    drainRX();
}

short NdisDriverStartIrq(void)
{
    u16 rc;

    TraceArgs(0x0040, 2, (u16)Priv.pDev->irq);

    enableHWInterrupts(Priv.irqmask);

    DevCli();
    rc = DevHelp_SetIRQ((NPFN)IrqHandler, Priv.pDev->irq, 1);
    DevSti();
    if(rc)
    {
        drainRing();
        return 1;
    }

    return 0;
}

void stopRX(void)
{
    u32 rx_ctrl = NV_R32(NvRegReceiverControl);

    if (!Priv.mac_in_use)
    {
        rx_ctrl &= ~NVREG_RCVCTL_START;
    }
    else
    {
        rx_ctrl |= NVREG_RCVCTL_RX_PATH_EN;
    }
    NV_W32(rx_ctrl, NvRegReceiverControl);
    regDelay(NvRegReceiverStatus, NVREG_RCVSTAT_BUSY, 0,
            NV_RXSTOP_DELAY1, NV_RXSTOP_DELAY1MAX,
            "stop rx: ReceiverStatus remained busy\n");

    udelay(NV_RXSTOP_DELAY2);
    if (!Priv.mac_in_use)
    {
        NV_W32(0, NvRegLinkSpeed);
    }
    Priv.rxstarted = 0;
}

void stopTX(void)
{
    u32 tx_ctrl = NV_R32(NvRegTransmitterControl);

    if (!Priv.mac_in_use)
    {
        tx_ctrl &= ~NVREG_XMITCTL_START;
    }
    else
    {
        tx_ctrl |= NVREG_XMITCTL_TX_PATH_EN;
    }
    NV_W32(tx_ctrl, NvRegTransmitterControl);
    regDelay(NvRegTransmitterStatus, NVREG_XMITSTAT_BUSY, 0,
            NV_TXSTOP_DELAY1, NV_TXSTOP_DELAY1MAX,
            "stop tx: TransmitterStatus remained busy\n");

    udelay(NV_TXSTOP_DELAY2);
    if (!Priv.mac_in_use)
    {
        NV_W32(NV_R32(NvRegTransmitPoll) & NVREG_TRANSMITPOLL_MAC_ADDR_REV,
               NvRegTransmitPoll);
    }
}

void NdisDriverSetMcast(u16 wFlags)
{
    u8 addr[6] = {0};
    u8 mask[6] = {0};
    u8 alwaysOn[6] = {0xFF};
    u8 alwaysOff[6] = {0xFF};
    u16 i, j;
    u32 pff;

    TraceArgs(0x0041, 4, (u16)AdapterMCB.McbCnt, (u16)Priv.pDev->flags);
    DPRINTF(5, "DriverSetMcast BEG cnt=%d Flags=%x\n", AdapterMCB.McbCnt, Priv.pDev->flags);

    pff = ((NV_R32(NvRegPacketFilterFlags) & NVREG_PFF_PAUSE_RX) & ~((u32)NVREG_PFF_PROMISC | NVREG_PFF_MYADDR)) | NVREG_PFF_ALWAYS;
    if ( (wFlags & (FLTR_SET_FILTER|FLTR_PRMSCS))==(FLTR_SET_FILTER|FLTR_PRMSCS) )
    {
      TraceArgs(0x0004, 2, wFlags);
      DPRINTF(1, "Promiscuous mode enabled\n");
      pff |= NVREG_PFF_PROMISC;
    }
    else if (AdapterMCB.McbCnt > 0)
    {
        pff |= NVREG_PFF_MYADDR;
        for(i = 0; i < AdapterMCB.McbCnt; i++)
        {
            for (j = 0; j < 6; j++)
            {
                alwaysOn[j] &= AdapterMCB.McbAddrs[i].mAddr[j];
                alwaysOff[j] &= ~AdapterMCB.McbAddrs[i].mAddr[j];
            }
        }
        for (j = 0; j < 6; j++)
        {
            addr[j] = alwaysOn[j];
            mask[j] = alwaysOn[j] | alwaysOff[j];
        }
    }
    addr[0] |= NVREG_MCASTADDRA_FORCE;
    mutex_lock(&Priv.mutex);
    DevCli();
    stopRX();
    for (j = 0; j < 6; j++)
    {
        NV_W8(addr[j], NvRegMulticastAddrA + j);
    }
    for (j = 0; j < 6; j++)
    {
        NV_W8(mask[j], NvRegMulticastMaskA + j);
    }
    NV_W32(pff, NvRegPacketFilterFlags);
    startRX();
    mutex_unlock(&Priv.mutex);
    DevSti();
}

void NdisDriverSetMac(char *pMscCurrStnAdr, unsigned short len)
{
  u16 i;

  TraceBuf(0x0046, 6, pMscCurrStnAdr);

  if (Priv.rxstarted)
  {
    mutex_lock(&Priv.mutex);
    DevCli();
    stopRX();
    for (i = 0; i < 6; i++)
    {
      NV_W8(AdapterSC.MscCurrStnAdr[i], NvRegMacAddrA + i);
    }
    startRX();
    mutex_unlock(&Priv.mutex);
    DevSti();
  }
  else
  {
    for (i = 0; i < 6; i++)
    {
      NV_W8(AdapterSC.MscCurrStnAdr[i], NvRegMacAddrA + i);
    }
  }
}

void NdisDriverGetHwStats(void)
{
    AdapterSS.MssFSByt += NV_R32(NvRegTxCnt);
    AdapterSS.MssSFHW += NV_R32(NvRegTxLossCarrier);
    AdapterSS.MssSFTime += NV_R32(NvRegTxExcessDef);
    AdapterSS.MssRFMin += NV_R32(NvRegRxFrameErr);
    AdapterSS.MssRFMax += NV_R32(NvRegRxFrameTooLong);
    AdapterSS.MssRFLack += NV_R32(NvRegRxOverflow);
    AdapterSS.MssRFCRC += NV_R32(NvRegRxFCSErr);
    AdapterSS.MssRFErr += NV_R32(NvRegRxFrameAlignErr) + NV_R32(NvRegRxLenErr) + NV_R32(NvRegRxLateCol);
    AdapterSS.MssFRMCByt += NV_R32(NvRegRxMulticast);
    AdapterSS.MssFRBCByt += NV_R32(NvRegRxBroadcast);
    AdapterSS.MssFRByt += NV_R32(NvRegRxMulticast) + NV_R32(NvRegRxBroadcast) + NV_R32(NvRegRxUnicast);
}

short NdisDriverStartXmit(struct TxBufDesc far *pDsc)
{
    u16 fragments;
    u16 size;
    u16 i;
    pring_desc p;
    u32 flags;
    u32 phys;
    fpu8 pPtr;
    u32 tx_flags_extra;

    //DPRINTF(1, "startXmit\n");
    DevCli();
    if (Priv.txbusy) {
        DevSti();
        DPRINTF(1, "startXmit: Reentered!\n");
        return OUT_OF_RESOURCE;
    }
    Priv.txbusy++;
    DevSti();

    fragments = pDsc->TxDataCount;
    size = pDsc->TxImmedLen;
    for(i = 0 ; i < fragments; i++) {
        size += pDsc->TxDataBlk[i].TxDataLen;
    }

    if (size < ETH_HEADER_LEN || size > AdapterCC.CcSCp->MscMaxFrame) {
        Priv.txbusy--;
        return INVALID_PARAMETER;
    }

    p = &(Priv.tx_ring.orig[Priv.txpos]);
    flags = le32_to_cpu(p->flaglen);
    if (flags & NV_TX_VALID) {
        DPRINTF(1, "startXmit: no tx ring!\n");
        Priv.txbusy--;
        return OUT_OF_RESOURCE;
    }

    phys = cpu_to_le32(GetTxBufferPhysicalAddress(Priv.pDev, Priv.txpos));
    p->buf = phys;
    pPtr = GetTxBufferVirtualAddress(Priv.pDev, Priv.txpos);

    //DPRINTF(1, "startXmit: txpos=%x flags=%lx phys=%lx virt=%lx\n", Priv.txpos, flags, phys, pPtr);

    if (!pPtr) {
        Priv.txbusy--;
        return OUT_OF_RESOURCE;
    }

    if (pDsc->TxImmedLen) {
        memcpy(pPtr, pDsc->TxImmedPtr, pDsc->TxImmedLen);
        pPtr += pDsc->TxImmedLen;
    }

    for (i = 0 ; i < fragments; i++) {
        u16 ptrtype = pDsc->TxDataBlk[i].TxPtrType;
        u16 wLen = pDsc->TxDataBlk[i].TxDataLen;
        if (ptrtype == 2) {
            memcpy(pPtr, pDsc->TxDataBlk[i].TxDataPtr, wLen);
        } else {
            fptr pData;
            DevHelp_PhysToGDTSelector((u32)pDsc->TxDataBlk[i].TxDataPtr, wLen, GDT_READ);
            SELECTOROF(pData) = GDT_READ;
            OFFSETOF  (pData) = 0;
            memcpy(pPtr, pData, wLen);
        }
        pPtr += pDsc->TxDataBlk[i].TxDataLen;
    }

    /* set tx flags */
    tx_flags_extra = Priv.tx_flags | (Priv.desc_ver == DESC_VER_1 ? NV_TX_LASTPACKET : NV_TX2_LASTPACKET);
    p->flaglen = cpu_to_le32((size - 1) | tx_flags_extra);

    TraceArgs(0x0030, 8, (u16)Priv.txpos, (u16)size, (u32)p->flaglen);
    TraceBuf(0x0031, size<64?size:64, GetTxBufferVirtualAddress(Priv.pDev, Priv.txpos));

    NV_W32(NVREG_TXRXCTL_KICK|Priv.txrxctl_bits, NvRegTxRxControl);

    Priv.txpos++;
    if (Priv.txpos == Priv.tx_ring_size) {
        Priv.txpos = 0;
    }

    //DPRINTF(1, "startXmit: Success\n");
    Priv.txbusy--;
    return SUCCESS;
}

short NdisDriverReleaseRx(u16 pos)
{
    TraceArgs(0x0013, 2, pos);

    if (pos < Priv.rx_ring_size)
    {
        Priv.rx_ring.orig[pos].flaglen = cpu_to_le32(Priv.rx_buf_sz | NV_RX_AVAIL);
        return SUCCESS;
    }
    return INVALID_PARAMETER;
}

short NdisDriverXferRx(fpu16 pcopied, u16 frameOffset, struct TDBufDesc far * pd)
{
    fpu8 start;
    u16 copied;
    u16 i;

    TraceArgs(0x0012, 0, NULL);

    if (!Priv.currentlen || !Priv.currentrxbuffer)
    {
        return GENERAL_FAILURE;
    }
    if (Priv.currentlen < frameOffset)
    {
        return INVALID_PARAMETER;
    } else
    if (Priv.currentlen == frameOffset)
    {
        return SUCCESS;
    }

    start = Priv.currentrxbuffer + frameOffset;
    Priv.currentlen -= frameOffset;

    copied = 0;
    for (i = 0; i < pd->TDDataCount && Priv.currentlen > 0; i++)
    {
        u16 ptrtype = pd->TDDataBlk[i].TDPtrType;
        u16 bufsize = pd->TDDataBlk[i].TDDataLen;
        u16 tocopy = min((u16)Priv.currentlen, (u16)bufsize);
        if (ptrtype == 2)
        {
            memcpy(pd->TDDataBlk[i].TDDataPtr, start, tocopy);
        }
        else
        {
            fptr pData;
            DevHelp_PhysToGDTSelector((u32)pd->TDDataBlk[i].TDDataPtr, bufsize, GDT_WRITE);
            SELECTOROF(pData) = GDT_WRITE;
            OFFSETOF  (pData) = 0;
            memcpy(pData, start, tocopy);
        }
        start += tocopy;
        copied += tocopy;
        Priv.currentlen -= tocopy;
    }
    *pcopied = copied;
    return SUCCESS;
}


/* These Suspend and Resume routines have not been tested
 * and are probably not complete - DAZ
 */

/* stop the traffic and power down the device
 *
 * This function will get called from the strategy function on ACPI suspend.
 * It could also get called by the APM driver if it is installed.
 */
short DriverSuspend(void)
{
    TraceBuf(0x0047, 0, NULL);

    if (!Priv.Suspended) /* protection if we get called more than once */
    {
        // not needed SavePciState();
        PciSetPowerStateD3hot((PPCI_DEVICEINFO)Priv.pDev);
        Priv.Suspended = 1;
    }

    return 0;
}

/* power on the device and restore as if powered off
 *
 * This function will get called from the strategy function on ACPI resume.
 * It could also get called by the APM driver if it is installed.
 */
short DriverResume(void)
{
    TraceBuf(0x0048, 0, NULL);

    if (Priv.Suspended) /* protection if we get called more than once */
    {
        PciSetPowerStateD0((PPCI_DEVICEINFO)Priv.pDev);
        // not needed RestorePciState();
        PciSetBusMaster((PPCI_DEVICEINFO)Priv.pDev);
        phyInit();
        Priv.Suspended = 0;
    }

    return 0;
}

void DriverIOCtlGenMac(PREQPACKET pPacket) {
    ULONG far *pData;
    ULONG far *pParm;
    u16 wError;

    if (pPacket->ioctl.usParmLen < (sizeof(ULONG)*4)) {
        pPacket->usStatus |= RPERR_PARAMETER;
        return;
    }
    pParm = (ULONG far *)pPacket->ioctl.pvParm;
    pData = (ULONG far *)pPacket->ioctl.pvData;

    wError = RPERR_PARAMETER;
    switch (pPacket->ioctl.bFunction) {
    case GENMAC_WRAPPER_OID_GET:
        switch (pParm[0]) {
        case OID_GEN_LINK_SPEED:
            if (pPacket->ioctl.usDataLen < sizeof(ULONG)) break; /* returns a ULONG */
            *pData = AdapterSC.MscLinkSpd / 100;
            wError = 0;
            break;
        case OID_GEN_MEDIA_CONNECT_STATUS:
            if (pPacket->ioctl.usDataLen < sizeof(ULONG)) break; /* returns a ULONG */
            *pData = ((NV_R32(NvRegAdapterControl) & NVREG_ADAPTCTL_LINKUP) == 0); /* return TRUE if link is disconnected */
            wError = 0;
            break;
#ifdef IS_WIRELESS
        case OID_802_11_BSSID:
            /* returns a string */
            /* pData is a pointer to the strimg */
            /* pPacket->usDataLen is the max size of the returned data */
            break;
        case OID_802_11_BSSID_LIST:
            /* returns a PNDIS_802_11_BSSID_LIST_EX structure */
            /* pData is a pointer to the structure */
            /* pPacket->usDataLen is the max size of the returned data */
            break;
        case OID_802_11_ENCRYPTION_STATUS:
            if (pPacket->usDataLen < sizeof(ULONG)) break; /* returns a ULONG of values enumerated in NDIS_802_11_ENCRYPTION_STATUS */
            break;
        case OID_802_11_FRAGMENTATION_THRESHOLD:
            if (pPacket->usDataLen < sizeof(ULONG)) break; /* returns a ULONG */
            break;
        case OID_802_11_INFRASTRUCTURE_MODE:
            if (pPacket->usDataLen < sizeof(ULONG)) break; /* returns a ULONG */
            break;
        case OID_802_11_NETWORK_TYPES_SUPPORTED:
            /* returns an array of ULONGs */
            /* pData is a pointer to the array */
            /* pPacket->usDataLen is the max size of the returned data */
            break;
        case OID_802_11_NETWORK_TYPE_IN_USE:
            if (pPacket->usDataLen < sizeof(ULONG)) break; /* returns a ULONG */
            break;
        case OID_802_11_POWER_MODE:
            if (pPacket->usDataLen < sizeof(ULONG)) break; /* returns a ULONG */
            break;
        case OID_802_11_RSSI:
            if (pPacket->usDataLen < sizeof(ULONG)) break; /* returns a LONG */
            break;
        case OID_802_11_RTS_THRESHOLD:
            if (pPacket->usDataLen < sizeof(ULONG)) break; /* returns a ULONG */
            break;
        case OID_802_11_SSID:
            if (pPacket->usDataLen < sizeof(NDIS_802_11_SSID)) break;
            /* returns a NDIS_802_11_SSID which is a structure */
            /* pData is a pointer to the NDIS_802_11_SSID structure */
            /* pPacket->usDataLen is the size of the structure */
            break;
        case OID_PRIVATE_WRAPPER_GENMAC_VERSION:
            if (pPacket->usDataLen < sizeof(ULONG)) break; /* returns a ULONG */
            break;
        case OID_PRIVATE_WRAPPER_LANNUMBER:
            if (pPacket->usDataLen < sizeof(ULONG)) break; /* returns a ULONG */
            break;
        case OID_PRIVATE_WRAPPER_LINKSTATUS:
            if (pPacket->usDataLen < sizeof(ULONG)) break; /* returns a ULONG */
            break;
        case OID_PRIVATE_WRAPPER_WINDRIVER_NIFNAME:
            /* returns a string */
            /* pData is a pointer to the strimg */
            /* pPacket->usDataLen is the max size of the returned data */
            break;
        case OID_PRIVATE_WRAPPER_ISWIRELESS:
            if (pPacket->usDataLen < sizeof(ULONG)) break; /* returns a ULONG */
            *pData = 1; /* is wireless */
            wError = 0;
            break;
#else
        case OID_PRIVATE_WRAPPER_ISWIRELESS:
            if (pPacket->ioctl.usDataLen < sizeof(ULONG)) break; /* returns a ULONG */
            *pData = 0; /* not wireless */
            wError = 0;
            break;
#endif
        default:
            break;
        }
        break;

#ifdef IS_WIRELESS
    case GENMAC_WRAPPER_OID_SET:
        switch (pParm[0]) {
        case OID_802_11_ADD_WEP:
            /* pData is a pointer to a PNDIS_802_11_WEP structure */
            break;
        case OID_802_11_AUTHENTICATION_MODE:
            /* *pData is a ULONG */
            break;
        case OID_802_11_BSSID_LIST_SCAN:
            /* no data */
            break;
        case OID_802_11_DISASSOCIATE:
            /* no data */
            break;
        case OID_802_11_ENCRYPTION_STATUS:
            /* *pData is a ULONG */
            break;
        case OID_802_11_FRAGMENTATION_THRESHOLD:
            /* *pData is a ULONG */
            break;
        case OID_802_11_INFRASTRUCTURE_MODE:
            /* *pData is a ULONG */
            break;
        case OID_802_11_NETWORK_TYPE_IN_USE:
            /* *pData is a ULONG */
            break;
        case OID_802_11_POWER_MODE:
            /* *pData is a ULONG */
            break;
        case OID_802_11_PRIVACY_FILTER:
            /* *pData is a ULONG */
            break;
        case OID_802_11_RELOAD_DEFAULTS:
            /* no data */
            break;
        case OID_802_11_REMOVE_WEP:
            /* *pData is a ULONG */
            break;
        case OID_802_11_RTS_THRESHOLD:
            /* *pData is a ULONG */
            break;
        case OID_802_11_SSID:
            /* pData is a pointer to a NDIS_802_11_SSID structure */
            break;
        default:
            break;
        }
        break;
#endif

    default:
        break;
    }

    if (wError) {
        pPacket->usStatus |= wError;
        pParm[2] = 0;
        pParm[3] = 1;
    } else {
        pParm[2] = 1;
        pParm[3] = 0;
    }
}

/**
 * Enable the driver and hardware for normal operation.
 *
 * Called by the NDIS driver (ring 0).
 */
short NdisDriverOpen(void)
{
    int ringinitrc;
    u32 i;
    u16 retries;

    TraceBuf(0x0045, 0, NULL);

    /* erase previous misconfiguration */
    if (Priv.flags & DEV_HAS_POWER_CNTRL) {
        macReset();
    }
    NV_W32(NVREG_MCASTADDRA_FORCE, NvRegMulticastAddrA);
    NV_W32(0, NvRegMulticastAddrB);
    NV_W32(0, NvRegMulticastMaskA);
    NV_W32(0, NvRegMulticastMaskB);
    NV_W32(0, NvRegPacketFilterFlags);

    NV_W32(0, NvRegTransmitterControl);
    NV_W32(0, NvRegReceiverControl);

    NV_W32(0, NvRegAdapterControl);

    if (Priv.pause_flags & NV_PAUSEFRAME_TX_CAPABLE) {
        NV_W32(NVREG_TX_PAUSEFRAME_DISABLE,  NvRegTxPauseFrame);
    }

    Priv.rx_buf_sz = ETH_DATA_LEN + NV_RX_HEADERS;

    /* ring size defaults */
    Priv.rx_ring_size = RX_RING_DEFAULT;
    Priv.tx_ring_size = TX_RING_DEFAULT;

    /* Setup input to AllocateRingsAndBuffers() */
    Priv.pDev->rx.RingCount = RX_RING_DEFAULT;
    Priv.pDev->rx.DescSize = (Priv.desc_ver == DESC_VER_1 || Priv.desc_ver == DESC_VER_2) ? sizeof(ring_desc) : sizeof(ring_desc_ex);
    Priv.pDev->rx.AllocSize = NV_RX_ALLOCATE;
    Priv.pDev->tx.RingCount = TX_RING_DEFAULT;
    Priv.pDev->tx.DescSize = (Priv.desc_ver == DESC_VER_1 || Priv.desc_ver == DESC_VER_2) ? sizeof(ring_desc) : sizeof(ring_desc_ex);
    Priv.pDev->tx.AllocSize = NV_TX_ALLOCATE;
    if (AllocateRingsAndBuffers(Priv.pDev)) return -1;

    Priv.rx_ring.orig = Priv.pDev->rx.RingVirt;
    Priv.tx_ring.orig = Priv.pDev->tx.RingVirt;

    ringinitrc = initRing();
    if (ringinitrc) {
        return 1;
    }

    NV_W32(0, NvRegLinkSpeed);
    NV_W32(NV_R32(NvRegTransmitPoll) & NVREG_TRANSMITPOLL_MAC_ADDR_REV, NvRegTransmitPoll);
    txrxReset();
    NV_W32(0, NvRegUnknownSetupReg6);

    Priv.in_shutdown = 0;

    /* give hw rings */
    NV_W32(Priv.rx_buf_sz, NvRegOffloadConfig); // DAZ moved from lower
    setupHWrings(NV_SETUP_RX_RING | NV_SETUP_TX_RING);

    NV_W32( ((u32)(Priv.rx_ring_size-1) << NVREG_RINGSZ_RXSHIFT) | ((u32)(Priv.tx_ring_size-1) << NVREG_RINGSZ_TXSHIFT), NvRegRingSizes);

    NV_W32(Priv.linkspeed, NvRegLinkSpeed);
    if (Priv.desc_ver == DESC_VER_1) {
        NV_W32(NVREG_TX_WM_DESC1_DEFAULT, NvRegTxWatermark);
    } else {
        NV_W32(NVREG_TX_WM_DESC2_3_DEFAULT, NvRegTxWatermark);
    }
    NV_W32(Priv.txrxctl_bits, NvRegTxRxControl);
    NV_W32(Priv.vlanctl_bits, NvRegVlanControl);
    NV_R32(0); /* force out pending posted writes */
    NV_W32(NVREG_TXRXCTL_BIT1|Priv.txrxctl_bits, NvRegTxRxControl);
    regDelay(NvRegUnknownSetupReg5, NVREG_UNKSETUP5_BIT31, NVREG_UNKSETUP5_BIT31,
            NV_SETUP5_DELAY, NV_SETUP5_DELAYMAX,
            "open: SetupReg5, Bit 31 remained off\r\n");

    NV_W32(0, NvRegMIIMask);
    NV_W32(NVREG_IRQSTAT_MASK, NvRegIrqStatus);
    NV_W32(NVREG_MIISTAT_MASK2, NvRegMIIStatus);

    NV_W32(NVREG_MISC1_FORCE | NVREG_MISC1_HD, NvRegMisc1);
    NV_W32(NV_R32(NvRegTransmitterStatus), NvRegTransmitterStatus);
    NV_W32(NVREG_PFF_ALWAYS, NvRegPacketFilterFlags);

    NV_W32(NV_R32(NvRegReceiverStatus), NvRegReceiverStatus);
    i = (u32)(fptr)NdisDriverOpen;
    NV_W32(NVREG_RNDSEED_FORCE | (i & NVREG_RNDSEED_MASK), NvRegRandomSeed);
    NV_W32(NVREG_TX_DEFERRAL_DEFAULT, NvRegTxDeferral);
    NV_W32(NVREG_RX_DEFERRAL_DEFAULT, NvRegRxDeferral);
    if (poll_interval == -1) {
        if (optimization_mode == NV_OPTIMIZATION_MODE_THROUGHPUT) {
            NV_W32(NVREG_POLL_DEFAULT_THROUGHPUT, NvRegPollingInterval);
        } else {
            NV_W32(NVREG_POLL_DEFAULT_CPU, NvRegPollingInterval);
        }
    } else {
        NV_W32(poll_interval & 0xFFFF, NvRegPollingInterval);
    }
    NV_W32(NVREG_UNKSETUP6_VAL, NvRegUnknownSetupReg6);
    NV_W32(((u32)Priv.phyaddr << NVREG_ADAPTCTL_PHYSHIFT)|NVREG_ADAPTCTL_PHYVALID|NVREG_ADAPTCTL_RUNNING,
            NvRegAdapterControl);
    NV_W32(NVREG_MIISPEED_BIT8|NVREG_MIIDELAY, NvRegMIISpeed);
    NV_W32(NVREG_MII_LINKCHANGE, NvRegMIIMask);

    i = NV_R32(NvRegPowerState);
    if ( (i & NVREG_POWERSTATE_POWEREDUP) == 0) {
        NV_W32(NVREG_POWERSTATE_POWEREDUP | i, NvRegPowerState);
    }

    NV_R32(0);  /* force out pending posted writes */
    udelay(10);
    NV_W32(NV_R32(NvRegPowerState) | NVREG_POWERSTATE_VALID, NvRegPowerState);

    disableHWInterrupts(Priv.irqmask);
    NV_R32(0); /* force out pending posted writes */
    NV_W32(NVREG_MIISTAT_MASK2, NvRegMIIStatus);
    NV_W32(NVREG_IRQSTAT_MASK, NvRegIrqStatus);
    NV_R32(0); /* force out pending posted writes */

    DevCli();
    NV_W32(NVREG_MCASTADDRA_FORCE, NvRegMulticastAddrA);
    NV_W32(0, NvRegMulticastAddrB);
    NV_W32(0, NvRegMulticastMaskA);
    NV_W32(0, NvRegMulticastMaskB);
    NV_W32(NVREG_PFF_ALWAYS|NVREG_PFF_MYADDR, NvRegPacketFilterFlags);
    {
        NV_R32(NvRegMIIStatus);
        NV_W32(NVREG_MIISTAT_MASK, NvRegMIIStatus);
    }

    if (fixed_mode) Priv.autoneg = 0;
    Priv.linkspeed = 0;

    for (retries = 5; !updateLinkspeed() && retries; retries--) msleep(500);

    startRX();
    startTX();

#if 0
    enableHWInterrupts(Priv.irqmask);

    DevCli();
    u16 rc = DevHelp_SetIRQ((NPFN)IrqHandler, Priv.pDev->irq, 1);
    DevSti();
    if (rc)
    {
        drainRing();
        return 1;
    }
#endif
    TraceBuf(0x8045, 0, NULL);
    return 0;
}

/**
 * Process the parameters from PROTOCOL.INI
 *
 * Called at DevInit time.
 */
short NdisDriverProcessParms(struct ModCfg far *pConfig)
{
  fpchar p;
  int rc = 0;

  if (stricmp(GetConfigString(FindKey(pConfig, "DRIVERNAME")), cDevName)) return -4;

  memcpy(AdapterCC.CcName, pConfig->ModName, NAME_LEN);

  p = GetConfigString(FindKey(pConfig, "MAX_IRQ"));
  if (*p) {
    int val = atol(p);
    if (val > 0) {
      max_interrupt_work = val;
    } else {
      rc = 1;
    }
  }

  p = GetConfigString(FindKey(pConfig, "MODE"));
  if (*p) {
    if (!stricmp(p, "1000HALF")) {
      fixed_mode = LPA_1000XHALF;
      AdapterSC.MscLinkSpd = 1000000000;
    } else if (!stricmp(p, "100HALF")) {
      fixed_mode = LPA_100HALF;
      AdapterSC.MscLinkSpd = 100000000;
    } else if (!stricmp(p, "10HALF")) {
      fixed_mode = LPA_10HALF;
      AdapterSC.MscLinkSpd = 10000000;
    } else if (!stricmp(p, "1000FULL")) {
      fixed_mode = LPA_1000XFULL;
      AdapterSC.MscLinkSpd = 1000000000;
    } else if (!stricmp(p, "10FULL")) {
      fixed_mode = LPA_10FULL;
      AdapterSC.MscLinkSpd = 10000000;
    } else if (!stricmp(p, "100FULL")) {
      fixed_mode = LPA_100FULL;
      AdapterSC.MscLinkSpd = 100000000;
    } else {
       rc = 1;
    }
  }

  p = GetConfigString(FindKey(pConfig, "OPTIMIZE"));
  if (*p) {
    if (!stricmp(p, "YES")) {
      optimization_mode = NV_OPTIMIZATION_MODE_THROUGHPUT;
    } else if (!stricmp(p, "NO")) {
      optimization_mode = NV_OPTIMIZATION_MODE_CPU;
    } else {
      rc = 1;
    }
  }

  p = GetConfigString(FindKey(pConfig, "TIMER"));
  if (*p) {
    if (!stricmp(p, "YES")) {
      notimer = 0;
    } else if (!stricmp(p, "NO")) {
      notimer = 1;
    } else {
      rc = 1;
    }
  }

  p = GetConfigString(FindKey(pConfig, "LINK_TIMER"));
  if (*p) {
    if (!stricmp(p, "YES")) {
      nolinktimer = 0;
    } else if (!stricmp(p, "NO")) {
      nolinktimer = 1;
    } else {
      rc = 1;
    }
  }

  p = GetConfigString(FindKey(pConfig, "RXCHAIN"));
  if (*p) {
    if (!stricmp(p, "YES")) {
      dorxchain = 1;
    } else if (!stricmp(p, "NO")) {
      dorxchain = 0;
    } else {
      rc = 1;
    }
  }

  p = GetConfigString(FindKey(pConfig, "NETADDRESS"));
  if (*p) {
    if (GetHex(p, macaddr, 6) != 6) rc = 1;
    netaddress_override = 1;
  }

  if (dorxchain) AdapterSC.MscService |= RECEIVECHAIN_MOSTLY;

  return(rc);
}

/**
 * Determine if the device is supported by this driver.
 *
 * Called at DevInit time.
 */
short DriverCheckDevice(struct net_device *pDev)
{
  int i;

  for (i = 0; pci_tbl[i].vendor; i++)
  {
    if (pci_tbl[i].vendor == pDev->vendor && pci_tbl[i].device == pDev->device)
    {
      pDev->driver_data = i;
      return 1;
    }
  }
  return 0;
}

/**
 * Initialize the hardware
 *
 * Called at DevInit time.
 */
short DriverInitAdapter(struct net_device *pDev)
{
  int i;
  u32 txreg;
  u32 powerstate;
  u32 phystate;
  u16 phyinitialized;

  Priv.pDev = pDev;

  DPRINTF(1,"DriverInitAdapter %x:%x %x\n", Priv.pDev->vendor, Priv.pDev->device, Priv.pDev->driver_data);
  TraceArgs(0x0050, 6, Priv.pDev->vendor, Priv.pDev->device, Priv.pDev->driver_data);

  Priv.flags = pci_tbl[Priv.pDev->driver_data].flags;
  sprintf(Priv.pDev->name, "NVIDIA %s", pci_tbl[Priv.pDev->driver_data].name);
  NdisLogMsg(MSG_HWDET, 1, 1, (fpchar)Priv.pDev->name);

  Priv.currentrxbuffer = NULL;
  Priv.currentlen = 0;

  AdapterSC.MscInterrupt = Priv.pDev->irq;
  AdapterSC.MscTBufCap = 1514 * TX_RING_DEFAULT; //DAZ was wrong field.
  AdapterSC.MscRBufCap = 1514 * RX_RING_DEFAULT; //DAZ was wrong field.
  AdapterSC.MscVenAdaptDesc = DNAME; /* Description for user info */

  Priv.vlanctl_bits = 0;
  Priv.rxstarted = 0;
  Priv.txbusy = 0;
  Priv.Suspended = 0;

  Priv.desc_ver = DESC_VER_1;
  Priv.txrxctl_bits = NVREG_TXRXCTL_DESC_1;

  Priv.pkt_limit = NV_PKTLIMIT_1;
  if (Priv.flags & DEV_HAS_LARGEDESC) Priv.pkt_limit = NV_PKTLIMIT_2;

  if (Priv.flags & DEV_HAS_CHECKSUM)
  {
    Priv.rx_csum = 1;
    Priv.txrxctl_bits |= NVREG_TXRXCTL_RXCHECK;
  }

  Priv.msi_flags = 0;
  if (Priv.flags & DEV_HAS_MSI) Priv.msi_flags |= NV_MSI_CAPABLE;
  if (Priv.flags & DEV_HAS_MSI_X) Priv.msi_flags |= NV_MSI_X_CAPABLE;

  Priv.pause_flags = NV_PAUSEFRAME_RX_CAPABLE | NV_PAUSEFRAME_RX_REQ | NV_PAUSEFRAME_AUTONEG;
  if (Priv.flags & DEV_HAS_PAUSEFRAME_TX)
  {
    Priv.pause_flags |= NV_PAUSEFRAME_TX_CAPABLE | NV_PAUSEFRAME_TX_REQ;
  }

  if (Priv.flags & (DEV_HAS_VLAN|DEV_HAS_MSI_X|DEV_HAS_POWER_CNTRL|DEV_HAS_STATISTICS_V2))
  {
    Priv.register_size = NV_PCI_REGSZ_VER3;
  }
  else if (Priv.flags & DEV_HAS_STATISTICS_V1)
  {
    Priv.register_size = NV_PCI_REGSZ_VER2;
  }
  else
  {
    Priv.register_size = NV_PCI_REGSZ_VER1;
  }

  PciSetPowerStateD0((PPCI_DEVICEINFO)Priv.pDev);
  PciSetBusMaster((PPCI_DEVICEINFO)Priv.pDev);

  Priv.ba = 0;
  for (i = 0; i < 6; i++)
  {
    if (Priv.pDev->bars[i].bar && !Priv.pDev->bars[i].io && !Priv.pDev->bars[i].type && Priv.pDev->bars[i].size >= Priv.register_size)
    {
      Priv.ba = MapPhysToVirt(Priv.pDev->bars[i].start, min(Priv.pDev->bars[i].size, 0xF000));
      if (!Priv.ba) continue;
      break;
    }
  }
  if (!Priv.ba) return -1;
  TraceArgs(0x0051, 6, (u32)Priv.ba, (u16)Priv.flags);

  if (!netaddress_override) {
    for (i = 0; i < 6; i++) {
      macaddr[i] = Priv.ba[NvRegMacAddrA + i];
    }
  }
  txreg = NV_R32(NvRegTransmitPoll);
  if (Priv.flags & DEV_HAS_CORRECT_MACADDR || txreg & NVREG_TRANSMITPOLL_MAC_ADDR_REV || netaddress_override)
  {
    for (i = 0; i < 6; i++)
    {
      Priv.mac[i] = macaddr[i];
    }
  }
  else
  {
    for (i = 0; i < 6; i++)
    {
      Priv.mac[i] = macaddr[5 - i];
    }
    NV_W32(txreg | NVREG_TRANSMITPOLL_MAC_ADDR_REV, NvRegTransmitPoll);
  }
  if (Priv.mac[0] & 0x01)
  {
    Priv.mac[0] = 0x00;
    Priv.mac[1] = 0x00;
    Priv.mac[2] = 0x6c;
  }
  for (i = 0; i < 6; i++)
  {
    NV_W8(Priv.mac[i], NvRegMacAddrA + i);
    AdapterSC.MscPermStnAdr[i] = Priv.mac[i];
    AdapterSC.MscCurrStnAdr[i] = Priv.mac[i];
  }

  TraceBuf(0x0044, 6, Priv.mac);

  NV_W32(NvRegWakeUpFlags, 0);
  if (Priv.flags & DEV_HAS_POWER_CNTRL)
  {
    u8 rev = 0;
    PciReadConfig(Priv.pDev->BusDevFunc, PCI_REVISION_ID, sizeof(u8), &rev);

    powerstate = NV_R32(NvRegPowerState2);
    powerstate &= ~NVREG_POWERSTATE2_POWERUP_MASK;
    if ((Priv.pDev->device == PCI_DEVICE_ID_NVIDIA_NVENET_12 ||
         Priv.pDev->device == PCI_DEVICE_ID_NVIDIA_NVENET_13) && rev >= 0xA3)
    {
      powerstate |= NVREG_POWERSTATE2_POWERUP_REV_A3;
    }
    NV_W32(powerstate, NvRegPowerState2);
  }
  if (Priv.desc_ver == DESC_VER_1)
  {
    Priv.tx_flags = NV_TX_VALID;
  }
  else
  {
    Priv.tx_flags = NV_TX2_VALID;
  }
  if (optimization_mode == NV_OPTIMIZATION_MODE_THROUGHPUT)
  {
    Priv.irqmask = NVREG_IRQMASK_THROUGHPUT;
    if (Priv.msi_flags & NV_MSI_X_CAPABLE) Priv.msi_flags |= 0x0003;
  }
  else
  {
    Priv.irqmask = NVREG_IRQMASK_CPU;
    if (Priv.msi_flags & NV_MSI_X_CAPABLE) Priv.msi_flags |= 0x0001;
  }

  if (!notimer && (Priv.flags & DEV_NEED_TIMERIRQ)) Priv.irqmask |= NVREG_IRQ_TIMER;
  if (!notimer && !nolinktimer && (Priv.flags & DEV_NEED_LINKTIMER))
  {
    Priv.need_linktimer = 1;
  }
  else
  {
    Priv.need_linktimer = 0;
  }

  NV_W32(0, NvRegMIIMask);
  phystate = NV_R32(NvRegAdapterControl);
  if (phystate & NVREG_ADAPTCTL_RUNNING)
  {
    phystate &= ~NVREG_ADAPTCTL_RUNNING;
    NV_W32(phystate, NvRegAdapterControl);
  }
  NV_W32(NVREG_MIISTAT_MASK, NvRegMIIStatus);

  phyinitialized = 0;
  if (Priv.flags & DEV_HAS_MGMT_UNIT)
  {
    u32 tc = NV_R32(NvRegTransmitterControl);
    if (tc & NVREG_XMITCTL_SYNC_PHY_INIT)
    {
      Priv.mac_in_use = NV_R32(NvRegTransmitterControl) & NVREG_XMITCTL_MGMT_ST;
      for (i = 0; i < 2 ; i++)
      {
        msleep(32);
        if (mgmtAcquireSema())
        {
          if ((NV_R32(NvRegTransmitterControl) & NVREG_XMITCTL_SYNC_MASK) == NVREG_XMITCTL_SYNC_PHY_INIT)
          {
            phyinitialized = 1;
          }
          break;
        }
      }
    }
  }

  Priv.phyaddr = 0xFFFF;
  for (i = 1; i <= 32; i++)
  {
    long res;
    u16 id1, id2;
    u16 addr = i & 0x1F;

    res = phyRW(addr, MII_PHYSID1, MII_READ);
    if (res == -1L || res == 0xffff) continue;
    id1 = (u16)res;
    res = phyRW(addr, MII_PHYSID2, MII_READ);
    if (res == -1L || res == 0xffff) continue;
    id2 = (u16)res;

    Priv.phy_model = (u16)(id2 & PHYID2_MODEL_MASK);
    id1 = (id1 & PHYID1_OUI_MASK) << PHYID1_OUI_SHFT;
    id2 = (id2 & PHYID2_OUI_MASK) >> PHYID2_OUI_SHFT;
    Priv.phyaddr = addr;
    Priv.phy_oui = id1 | id2;

    if (Priv.phy_oui == PHY_OUI_REALTEK2) Priv.phy_oui = PHY_OUI_REALTEK;

    if (Priv.phy_oui == PHY_OUI_REALTEK && Priv.phy_model == PHY_MODEL_REALTEK_8211)
    {
      Priv.phy_rev = (u16)phyRW(addr, MII_RESV1, MII_READ) & PHY_REV_MASK;
    }

    break;
  }
  if (Priv.phyaddr == 0xFFFF) return -1;

  if (!phyinitialized)
  {
    phyInit();
  }
  else
  {
    u32 mii_status = phyRW(Priv.phyaddr, MII_BMSR, MII_READ);
    if (mii_status & PHY_GIGABIT) Priv.gigabit = PHY_GIGABIT;
  }

  Priv.linkspeed = NVREG_LINKSPEED_FORCE|NVREG_LINKSPEED_10;
  Priv.duplex = 0;
  Priv.autoneg = 1;

  return 0;
}

