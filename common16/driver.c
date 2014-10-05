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
#include "Dev16lib.h"
#include "base.h"
#include "ndis.h"
#include "driver.h"
#include "version.h"

#define PAGESIZE 4096

char cDevName[MAX_DEVNAME+1] = DEV_NAME;

struct net_device netdev;

static u32 const ethernet_polynomial = 0x04c11db7;
u32 ether_crc(short length, unsigned char far *data)
{
  long crc = -1;

  while (--length >= 0) {
    unsigned char current_octet = *data++;
    u32 bit;
    for (bit = 0; bit < 8; bit++, current_octet >>= 1)
        crc = (crc << 1) ^ ((crc < 0) ^ (current_octet & 1) ? ethernet_polynomial : 0);
  }

  return crc;
}

#ifdef STATIC_RING_BUFFER_GDT
#define MAX_GDT 32
#define MAP_SIZE 0x10000L
static short MapRingBuffers(struct Os2RingInfo *pRi)
{
  short rc;
  u16 i;
  u32 AllocSize;
  u32 DmaAddress;
  u32 MapSize;
  u16 GdtCount, GdtIx;
  fpu8 VirtAddress;
  u16 VirtOffset;
  u16 GdtList[MAX_GDT];

  /* calculate how many GDTs we need for the ring buffers */
  i = MAP_SIZE / pRi->AllocSize; /* number of buffers per GDT */
  GdtCount = pRi->RingCount / i;
  if (pRi->RingCount % i) GdtCount++;
  if (GdtCount >= MAX_GDT) return -10;
  MapSize = (u32)pRi->AllocSize * i;

  /* allocate the GDTs */
  rc = DevHelp_AllocGDTSelector((u16 far*)GdtList, GdtCount);
  if (rc) return(rc);

  /* Map the ring buffers */
  rc = 0;
  GdtIx = 0;
  VirtOffset = 0;
  DmaAddress = pRi->BufferDma;
  AllocSize = (u32)pRi->RingCount * pRi->AllocSize;
  DPRINTF(9, "BufSize=%x nBuf=%x AllocSize=%lx MapSize=%lx nGdt=%x\n", pRi->AllocSize, pRi->RingCount, AllocSize, MapSize, GdtCount);
  for (i = 0; i < pRi->RingCount; i++)
  {
    if (VirtOffset == 0)
    {
      if (GdtIx >= GdtCount)
      {
        rc = -11;
        break;
      }

      if (AllocSize < MapSize) MapSize = AllocSize;
      VirtAddress = (fpu8)MapPhysAddressToGdt(GdtList[GdtIx], DmaAddress, MapSize);
      if (!VirtAddress)
      {
        rc = -12;
        break;
      }
      GdtIx++;
      AllocSize -= MapSize;
      DmaAddress += MapSize;
    }
    pRi->BufferVirt[i] = VirtAddress + VirtOffset;
    VirtOffset += pRi->AllocSize;
    if (VirtOffset >= MapSize) VirtOffset = 0;
  }

  if (rc)
  {
    for (i = 0; i < GdtCount; i++)
    {
      DevHelp_FreeGDTSelector(GdtList[i]);
    }
  }

  return(rc);
}
#endif

/* returns non-zero on fail */
short AllocateRingsAndBuffers(struct net_device *pDev)
{
  u32 AllocSize;
  u32 RxSize;
  u16 rc;

  /* Verify all parameters are in range */
  if (pDev->rx.RingCount > MAX_RING_SIZE) return -1;
  if (pDev->tx.RingCount > MAX_RING_SIZE) return -2;
  if (pDev->rx.AllocSize > MAX_RB_ALLOC_SIZE) return -3;
  if (pDev->tx.AllocSize > MAX_RB_ALLOC_SIZE) return -4;
  if (pDev->rx.AllocSize & 3) return -5;
  if (pDev->tx.AllocSize & 3) return -6;

  /* allocate the ring descriptors */
  RxSize = pDev->rx.RingCount * pDev->rx.DescSize;
  AllocSize = RxSize + (pDev->tx.RingCount * pDev->tx.DescSize);
  pDev->rx.RingDma = AllocPhysMemory(AllocSize);
  if(!pDev->rx.RingDma)
  {
    DPRINTF(1, "Cannot allocate rings Size=%ld bytes\n", AllocSize);
    return -7;
  }

  rc = 0;
  do {
    /* map the ring descriptors */
    pDev->rx.RingVirt = (fptr)MapPhysToVirt(pDev->rx.RingDma, AllocSize);
    if(!pDev->rx.RingVirt)
    {
      DPRINTF(1, "Cannot map physical address %lx\n", pDev->rx.RingDma);
      rc = -8;
      break;
    }

    /* Calculate the start of the transmit descriptors */
    pDev->tx.RingVirt = (fpu8)pDev->rx.RingVirt + RxSize;
    pDev->tx.RingDma = pDev->rx.RingDma + RxSize;

    /* Setup the Ring Buffers */
    AllocSize = ((u32)pDev->tx.AllocSize * pDev->tx.RingCount) + ((u32)pDev->rx.AllocSize * pDev->rx.RingCount);

    /* Allocate the Ring Buffer memory */
    pDev->rx.BufferDma = AllocPhysMemory(AllocSize);
    if(!pDev->rx.BufferDma)
    {
      DPRINTF(1, "Cannot allocate buffers Size=%ld bytes\n", AllocSize);
      rc = -9;
      break;
    }

    pDev->tx.BufferDma = pDev->rx.BufferDma + ((u32)pDev->rx.RingCount * pDev->rx.AllocSize);

    #ifdef STATIC_RING_BUFFER_GDT
    /* Map the ring buffers */
    rc = MapRingBuffers(&pDev->rx);
    if (rc) break;
    rc = MapRingBuffers(&pDev->tx);
    #else
    /* allocate the GDTs */
    rc = DevHelp_AllocGDTSelector((u16 far*)&pDev->rx.Gdt, 1);
    if (rc) return(rc);
    rc = DevHelp_AllocGDTSelector((u16 far*)&pDev->tx.Gdt, 1);
    #endif
  } while (0);

  if (rc)
  {
    if (pDev->rx.RingDma)
    {
      DevHelp_FreePhys(pDev->rx.RingDma);
      pDev->rx.RingDma = 0;
    }
    if (pDev->rx.BufferDma)
    {
      DevHelp_FreePhys(pDev->rx.BufferDma);
      pDev->rx.BufferDma = 0;
    }
    return rc;
  }

  /* dump the results */
  #ifdef DEBUG
  DPRINTF(1, "RX RingCount=%x RingDma=%lx RingVirt=%lx BufferDma=%lx\n",
      pDev->rx.RingCount, pDev->rx.RingDma, pDev->rx.RingVirt, pDev->rx.BufferDma);
  #ifdef STATIC_RING_BUFFER_GDT
  if (D16g_DbgLevel >= 9)
  {
    u16 i;
    for (i=0; i<pDev->rx.RingCount; i++)
    {
      DPRINTF(6, "  %d: Phys=%lx Virt=%lx\n", i, pDev->rx.BufferDma+((u32)i*pDev->rx.AllocSize), pDev->rx.BufferVirt[i]);
    }
  }
  #endif
  DPRINTF(1, "TX RingCount=%x RingDma=%lx RingVirt=%lx BufferDma=%lx\n",
        pDev->tx.RingCount, pDev->tx.RingDma, pDev->tx.RingVirt, pDev->tx.BufferDma);
  #ifdef STATIC_RING_BUFFER_GDT
  if (D16g_DbgLevel >= 9)
  {
    u16 i;
    for (i=0; i<pDev->tx.RingCount; i++)
    {
      DPRINTF(6, "  %d: Phys=%lx Virt=%lx\n", i, pDev->tx.BufferDma+((u32)i*pDev->tx.AllocSize), pDev->tx.BufferVirt[i]);
    }
  }
  #endif
  #endif

  return 0;
}

u32 GetRxBufferPhysicalAddress(struct net_device *pDev, u16 Ix)
{
  return pDev->rx.BufferDma + ((u32)Ix * pDev->rx.AllocSize);
}

u32 GetTxBufferPhysicalAddress(struct net_device *pDev, u16 Ix)
{
  return pDev->tx.BufferDma + ((u32)Ix * pDev->tx.AllocSize);
}

fptr GetRxBufferVirtualAddress(struct net_device *pDev, u16 Ix)
{
  #ifdef STATIC_RING_BUFFER_GDT
  return pDev->rx.BufferVirt[Ix];
  #else
  return (fptr)MapPhysAddressToGdt(pDev->rx.Gdt, pDev->rx.BufferDma + ((u32)Ix * pDev->rx.AllocSize), pDev->rx.AllocSize);
  #endif
}

fptr GetTxBufferVirtualAddress(struct net_device *pDev, u16 Ix)
{
#ifdef STATIC_RING_BUFFER_GDT
    return pDev->tx.BufferVirt[Ix];
#else
    return (fptr)MapPhysAddressToGdt(pDev->tx.Gdt, pDev->tx.BufferDma + ((u32)Ix * pDev->tx.AllocSize), pDev->tx.AllocSize);
#endif
}

#pragma code_seg ("_inittext");
#pragma data_seg ("_initdata","endds");

int RegisterRM(u16 adapternum)
{
  short rc;
  short n;
  char adapterkey[128];

  static DRIVERSTRUCT drs =
  {
    (PSZ)DFILE,
    (PSZ)DNAME,
    (PSZ)DVENDOR,
    CMVERSION_MAJOR,
    CMVERSION_MINOR,
    {
      (DDATE / 10000),
      (DDATE % 10000) / 100,
      (DDATE % 100)
    },
    DRF_STATIC,
    DRT_NETWORK,
    0,
    NULL
  };

  static ADJUNCT adj =
  {
    NULL, sizeof(ADJUNCT), ADJ_ADAPTER_NUMBER
  };

  static ADAPTERSTRUCT  ads =
  {
    NULL,
    AS_NO16MB_ADDRESS_LIMIT,
    AS_BASE_NETWORK,
    AS_SUB_ETHERNET,
    0,
    AS_HOSTBUS_PCI,
    AS_BUSWIDTH_32BIT,
    &adj,
    0
  };

  if (Rm1CreateDriver(&drs)) return 1;

  rc = 0;
  do
  {
    for (n = 0; !rc && n < 6; n++)
    {
      if (netdev.bars[n].bar)
      {
        if (netdev.bars[n].io)
        {
          rc = Rm1AddIo((u16)netdev.bars[n].start, (u16)netdev.bars[n].size);
          if (rc) break;
        }
        else
        {
          rc = Rm1AddMem(netdev.bars[n].start, netdev.bars[n].size);
          if (rc) break;
        }
      }
    }
    if (rc) break;

    rc = Rm1AddIrq(netdev.irq, netdev.ipin);
    if (rc) break;

    sprintf(adapterkey, "%s%s Network Adapter", ADAPTER_KEY, netdev.name);
    ads.AdaptDescriptName = (PSZ)adapterkey;
    adj.Adapter_Number = adapternum;
    rc = Rm1CreateAdapter(&ads);
  } while (0);
  if (rc)
  {
    Rm1Destroy(1);
  }
  return rc;
}

int FindAndSetupAdapter(void)
{
  int rc;
  int Success;

  UtSetDriverName(cDevName);

  Success = 0;
  do {
    int iAdapterNumber;
    int iFoundAdapters;
    int iRequestedAdapter;
    ULONG pciclass;
    USHORT PciBusDevFunc;

    iRequestedAdapter = 0; /* find the first adapter */
    if (mg_wAdapter != 0xffff) iRequestedAdapter = mg_wAdapter;
    pciclass = 0x00068000;
    iFoundAdapters = -1;

    for (iAdapterNumber=0; iAdapterNumber<10; iAdapterNumber++)
    {
      PciBusDevFunc = PciFindClass(pciclass, iAdapterNumber);
      if (PciBusDevFunc == 0xffff)
      {
        if (pciclass == 0x00020000) break; /* can't find a device. Stop looking. */
        else
        {
          pciclass = 0x00020000;
          iAdapterNumber = -1;
          continue;
        }
      }

      if (PciGetDeviceInfo(PciBusDevFunc, (PCI_DEVICEINFO *)&netdev)) continue;

      if (!DriverCheckDevice(&netdev)) continue;

      iFoundAdapters++;
      if (iFoundAdapters < iRequestedAdapter) continue;

      if (RegisterRM(iFoundAdapters)) continue;

      if (iFoundAdapters)
      {
        /* we add in the suffix for multiple instances starting with 2, since
         * that is how MPTS numbers them */
        UtModifyName(cDevName, iFoundAdapters+1, iFoundAdapters>1);
        UtSetDriverName(cDevName);
      }

      rc = NdisInit(MSGFILE); /* also opens the log file even if rc is non-zero */
      NdisLogMsg(1, 1, 1, (fpchar)mg_BuildLevel);
      if (rc < 0) {
        NdisLogMsg(2, 1, 0, NULL); /* Protocol Manager error */
        break;
      }
      if (rc)
      {
        NdisLogMsg(5, 1, 0, NULL); /* Invalid parameter in PROTOCOL.INI */
        break;
      }

      rc = DriverInitAdapter(&netdev);
      if (rc) break;

      Success = 1;
      break;
    } /* for iAdapterNumber loop */
  } while (0);

  if (Success)
  {
    if (NdisRegisterDriver()) Success = 0;
  }
  else
  {
    if (!rc) NdisLogMsg(4, 1, 0, NULL); /* No supported hardware was found. */
  }
  NdisLogMsg(0, 0, 0, NULL); /* Close the log file for ring 3 access */

  return (!Success);
}


