/*
 * This source is the part of the generic ndis driver for OS/2
 *
 * Copyright (C) 2010-2014 David Azarewicz
 * Copyright (C) 2007 2010 2011 Mensys
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
#include "ioctl.h"
#include "sas.h"
#ifdef LEGACY_APM
#include "apmcalls.h"
#endif

#define TRACE_MAJOR 0x00F8
u16 mg_wVerbose = 0;
u16 mg_wAdapter = 0xffff;
char mg_BuildLevel[] = BLDLEVEL;
char mg_FwDir[32] = "/IBMCOM";

#define ST_CLOSED 1
#define ST_OPENED 2

static u16 wState = ST_CLOSED;

#ifdef LEGACY_APM
USHORT FAR _cdecl APMEventHandler(PAPMEVENT Event)
{
  USHORT Message = (USHORT)Event->ulParm1; /* SubID */
  USHORT PowerState;

  if(Message == APM_SETPWRSTATE)
  {
    PowerState = (USHORT)(Event->ulParm2 >> 16);

    if(PowerState == APM_PWRSTATESUSPEND)
    {
      return DriverSuspend();
    }
  }
  else if(Message == APM_NORMRESUMEEVENT || Message == APM_CRITRESUMEEVENT)
  {
    return DriverResume();
  }

  return 0;
}
#endif

u16 StratInitComplete(PREQPACKET pPacket)
{
  (void)pPacket;

  Drv16InitComplete();
  #ifdef LEGACY_APM
  if(!APMAttach())
  {
    APMRegister((APMHANDLER)APMEventHandler, APM_NOTIFYSETPWR | APM_NOTIFYNORMRESUME | APM_NOTIFYCRITRESUME, 0);
  }
  #endif

  return RPDONE;
}

u16 StratOpen (PREQPACKET pPacket)
{
  (void)pPacket;

  if (wState != ST_CLOSED)
  {
    return RPDONE | RPERR_DEVINUSE;
  }

  if (!(AdapterCC.CcSSp->MssStatus & MS_BOUND))
  {
    return RPDONE | RPERR_NOTREADY;
  }

  wState = ST_OPENED;

  return RPDONE;
}

u16 StratClose(PREQPACKET pPacket)
{
  (void) pPacket;

  if (wState != ST_OPENED)
  {
    return RPDONE | RPERR_GENERAL;
  }

  wState = ST_CLOSED;

  return RPDONE;
}

#ifdef DEBUG
u16 StratRead(PREQPACKET pPacket)
{
  fptr VirtAddr;

  if (DevHelp_PhysToVirt(pPacket->io.ulAddress, pPacket->io.usCount, &VirtAddr, NULL)) {
    pPacket->io.usCount = 0;
    return RPDONE | RPERR_GENERAL;
  }

  pPacket->io.usCount = dCopyToUser((unsigned char far *)VirtAddr, pPacket->io.usCount);

  return RPDONE;
}
#endif


u16 StratIoctl(PREQPACKET pPacket)
{
  TraceArgs(0x0003, 4, (u16)pPacket->ioctl.bCategory, (u16)pPacket->ioctl.bFunction);
  DPRINTF(1, "StratIoctl:cat:0x%x, code:0x%x\n", pPacket->ioctl.bCategory, pPacket->ioctl.bFunction);
  switch (pPacket->ioctl.bCategory) {

  case GENMAC_CATEGORY: /* 0x99 */
    DriverIOCtlGenMac(pPacket);
    break;

  default:
    return RPDONE | RPERR_BADCOMMAND;
  }

  return RPDONE;
}

/* Handle ACPI suspend/resume
 */
u16 StratSaveRestore(PREQPACKET pPacket)
{
  /* FuncCode 0=save/suspend/sleep, 1=restore/resume/wake */
  if (pPacket->save_restore.Function) DriverResume();
  else DriverSuspend();

  return RPDONE;
}

u16 StratInit(PREQPACKET pPacket);

#pragma aux StrategyHandler parm [es bx];
void far StrategyHandler(PREQPACKET prp)
{
  TraceArgs(0x0002, 2, (u16)prp->bCommand);

  switch (prp->bCommand) {
  case STRATEGY_INIT:
    prp->usStatus = StratInit(prp);
    break;
  case STRATEGY_OPEN:
    prp->usStatus = StratOpen(prp);
    break;
  case STRATEGY_CLOSE:
    prp->usStatus = StratClose(prp);
    break;
  case STRATEGY_READ:
    #ifdef DEBUG
    prp->usStatus = StratRead(prp);
    #else
    prp->usStatus = RPDONE | RPERR_BADCOMMAND;
    #endif
    break;
  case STRATEGY_GENIOCTL:
    prp->usStatus = StratIoctl(prp);
    break;
  case STRATEGY_DEINSTALL:
    prp->usStatus = RPDONE;
    break;
  case STRATEGY_INITCOMPLETE:
    prp->usStatus = StratInitComplete(prp);
    break;
  case STRATEGY_SAVERESTORE:
    prp->usStatus = StratSaveRestore(prp);
    break;
  default:
    prp->usStatus = RPDONE | RPERR_BADCOMMAND;
    break;
  }
}

#pragma code_seg ("_inittext");
#pragma data_seg ("_initdata","endds");

int ParseCmdParms(const char far *pszCmdLine) {
  int iStatus;

  if (!pszCmdLine) return 0;

  iStatus = 0;
  while (*pszCmdLine)
  {
    if (*pszCmdLine++ != '/') continue; /* Ignore anything that doesn't start with '/' */
    /* pszCmdLine now points to first char of argument */

    if (ArgCmp(pszCmdLine, "V"))
    {
      pszCmdLine++; /* skip the 'V' */
      mg_wVerbose = 1;
      continue;
    }
    if (ArgCmp(pszCmdLine, "Q"))
    {
      pszCmdLine++; /* skip the 'Q' */
      mg_wVerbose = 0;
      continue;
    }
    if (ArgCmp(pszCmdLine, "A"))
    {
      pszCmdLine++; /* skip the 'A' */
      mg_wAdapter = *pszCmdLine++ & 0x0f;
      continue;
    }
    if (ArgCmp(pszCmdLine, "F"))
    {
      pszCmdLine++; /* skip the 'F' */
      pszCmdLine += GetString(pszCmdLine, &mg_FwDir, sizeof(mg_FwDir));
      continue;
    }
    #ifdef TRACING
    if (ArgCmp(pszCmdLine, "T"))
    {
      pszCmdLine++; /* skip the 'T' */
      TraceInit(TRACE_MAJOR, strtol(pszCmdLine, &pszCmdLine, 0));
      continue;
    }
    #endif
    #ifdef DEBUG
    if (ArgCmp(pszCmdLine, "D"))
    {
      pszCmdLine++; /* skip the 'D' */
      D16g_DbgLevel = strtol(pszCmdLine, &pszCmdLine, 0);
      continue;
    }
    if (ArgCmp(pszCmdLine, "WRAP"))
    {
      pszCmdLine += 4; /* skip the 'WRAP' */
      D16g_DbgBufWrap = 1;
      continue;
    }
    if (ArgCmp(pszCmdLine, "COM="))
    {
      pszCmdLine += 4; /* skip the "COM=" */
      D16g_ComBase = strtol(pszCmdLine, &pszCmdLine, 0);
      if (D16g_ComBase == 1) D16g_ComBase = 0x3f8;
      if (D16g_ComBase == 2) D16g_ComBase = 0x2f8;
      continue;
    }
    #endif
    cprintf("Unrecognized switch: %Fs\n", pszCmdLine-1);
    iStatus = 1; /* unrecognized argument */
  }

  return(iStatus);
}

u16 StratInit(PREQPACKET pPacket)
{
  u16 rc;

  Drv16Init(pPacket);
  #ifdef TRACING
  TraceInit(TRACE_MAJOR, 0x8000);
  #endif

  ParseCmdParms(pPacket->init_in.szArgs); // Process command line parameters.

  TraceBuf(0x0001, sizeof(mg_BuildLevel), mg_BuildLevel);
  DPRINTF(0, "%s\n", mg_BuildLevel);

  TraceBuf(0x0005, strlen(pPacket->init_in.szArgs)+1, pPacket->init_in.szArgs);
  DPRINTF(0, "%Fs\n", (fptr)pPacket->init_in.szArgs);

  if (mg_wVerbose >= 1) {
      cprintf("\n%s %s Copyright (C) 2013 %s\n", DNAME, VERSION, DVENDOR);
  }

  rc = RPDONE;
  if (FindAndSetupAdapter())
  {
    cprintf("%s driver not loaded.\nSee the lantran.log for more information.\n", cDevName);
    pPacket->init_out.usCodeEnd = 0;
    pPacket->init_out.usDataEnd = 0;
    rc |= RPERR;
  }
  else
  {
    TimeInit();
    if (mg_wVerbose >= 1) cprintf("%s loaded.\r\n", cDevName);
    pPacket->init_out.usCodeEnd = ((u16)_TextEnd);
    pPacket->init_out.usDataEnd = ((u16)&_DataEnd);
  }

  return rc;
}


