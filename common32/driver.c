/*
 * This source is the part of the generic ndis driver for OS/2
 *
 * Copyright (C) 2012 David Azarewicz david@88watts.net
 *
 */
#include "Dev32lib.h"
#include "Dev32ndis.h"
#include "types.h"
#include "driver.h"
#include "version.h"

#define PAGESIZE 4096

char cDevName[MAX_DEVNAME+1] = DEV_NAME;

struct net_device netdev;

int RegisterRM(u16 adapternum)
{
  short rc;
  short n;
  char adapterkey[128];

  static DRIVERSTRUCT drs =
  {
    NULL,
    NULL,
    NULL,
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
    NULL,
    0
  };

  drs.DrvrName = DFILE;
  drs.DrvrDescript = DNAME;
  drs.VendorName = DVENDOR;
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
    ads.AdaptDescriptName = adapterkey;
    adj.Adapter_Number = adapternum;
    ads.pAdjunctList = &adj;
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
    if (mg_Adapter != -1) iRequestedAdapter = mg_Adapter;
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
      NdisLogMsg(1, 1, 1, mg_BuildLevel);
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

  return (!Success);
}


