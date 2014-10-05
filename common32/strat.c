/*
 * This source is the part of the generic ndis driver for OS/2
 *
 * Copyright (C) 2012 David Azarewicz david@88watts.net
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
#include "Dev32lib.h"
#include "Dev32ndis.h"
#include "types.h"
#include "driver.h"
#include "version.h"
#include "ioctl.h"
#include "sas.h"

#define TRACE_MAJOR 0x00F8
int mg_Verbose = 0;
int mg_Adapter = -1;
char mg_BuildLevel[] = BLDLEVEL;
char mg_FwDir[32] = "/IBMCOM";
#ifdef TRACING
ULONG mg_TraceLevel = 0x8000;
#endif

#define ST_CLOSED 1
#define ST_OPENED 2
static int OpenState = ST_CLOSED;

void StratOpen (REQPACKET *pPacket)
{
  (void)pPacket;

  if (OpenState != ST_CLOSED)
  {
    pPacket->usStatus |= RPERR_DEVINUSE;
    return;
  }

  if (!(AdapterCC.CcSSp->MssStatus & MS_BOUND))
  {
    pPacket->usStatus |= RPERR_NOTREADY;
    return;
  }

  OpenState = ST_OPENED;
}

void StratClose(REQPACKET *pPacket)
{
  (void) pPacket;

  if (OpenState != ST_OPENED)
  {
    pPacket->usStatus |= RPERR_GENERAL;
    return;
  }

  OpenState = ST_CLOSED;
}

#ifdef DEBUG
void StratRead(REQPACKET *pPacket)
{
  void *LinAdr;

  if (Dev32Help_PhysToLin(pPacket->io.ulAddress, pPacket->io.usCount, &LinAdr))
  {
    pPacket->io.usCount = 0;
    pPacket->usStatus |= RPERR_GENERAL;
    return;
  }

  pPacket->io.usCount = dCopyToUser(LinAdr, pPacket->io.usCount);
}
#endif


void StratIoctl(REQPACKET *pPacket)
{
  TraceArgs(0x0003, 8, pPacket->ioctl.bCategory, pPacket->ioctl.bFunction);
  DPRINTF(1, "StratIoctl:cat:0x%x, code:0x%x\n", pPacket->ioctl.bCategory, pPacket->ioctl.bFunction);
  switch (pPacket->ioctl.bCategory) {

  case GENMAC_CATEGORY: /* 0x99 */
    DriverIOCtlGenMac(pPacket);
    break;

  default:
    pPacket->usStatus |= RPERR_BADCOMMAND;
    break;
  }
}


/* Handle ACPI suspend/resume
 */
u16 StratSaveRestore(REQPACKET *pPacket)
{
  /* FuncCode 0=save/suspend/sleep, 1=restore/resume/wake */
  if (pPacket->save_restore.Function) DriverResume();
  else DriverSuspend();

  return RPDONE;
}

int ParseCmdParms(const char *pszCmdLine) {
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
      mg_Verbose = 1;
      continue;
    }
    if (ArgCmp(pszCmdLine, "Q"))
    {
      pszCmdLine++; /* skip the 'Q' */
      mg_Verbose = 0;
      continue;
    }
    if (ArgCmp(pszCmdLine, "A"))
    {
      pszCmdLine++; /* skip the 'A' */
      mg_Adapter = *pszCmdLine++ & 0x0f;
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
      mg_TraceLevel = strtol(pszCmdLine, &pszCmdLine, 0);
      continue;
    }
    #endif
    #ifdef DEBUG
    if (ArgCmp(pszCmdLine, "D"))
    {
      pszCmdLine++; /* skip the 'D' */
      D32g_DbgLevel = strtol(pszCmdLine, &pszCmdLine, 0);
      DPRINTF(0,"DbgLevel=%d\n", D32g_DbgLevel);
      continue;
    }
    if (ArgCmp(pszCmdLine, "COM="))
    {
      pszCmdLine += 4; /* skip the "COM=" */
      D32g_ComBase = strtol(pszCmdLine, &pszCmdLine, 0);
      if (D32g_ComBase == '1') D32g_ComBase = 0x3f8;
      if (D32g_ComBase == '2') D32g_ComBase = 0x2f8;
      continue;
    }
    #endif
    cprintf("Unrecognized switch: %s\n", pszCmdLine-1);
    iStatus = 1; /* unrecognized argument */
  }

  return(iStatus);
}

void StratInit(REQPACKET *pPacket)
{
  #ifdef TRACING
  TraceInit(TRACE_MAJOR, 0x8000);
  #endif

  ParseCmdParms(pPacket->init_in.szArgs); // Process command line parameters.

  TraceBuf(0x0001, sizeof(mg_BuildLevel), mg_BuildLevel);
  DPRINTF(0, "%s\n", mg_BuildLevel);

  TraceBuf(0x0005, strlen(pPacket->init_in.szArgs)+1, Far16ToFlat(pPacket->init_in.szArgs));
  DPRINTF(0, "%s\n", Far16ToFlat(pPacket->init_in.szArgs));

  if (mg_Verbose >= 1) {
    cprintf("\n%s %s Copyright (C) 2013 %s\n", DNAME, VERSION, DVENDOR);
  }

  if (FindAndSetupAdapter())
  {
    cprintf("%s driver not loaded.\nSee the lantran.log for more information.\n", cDevName);
    pPacket->usStatus |= RPERR_GENERAL;
  }
  else
  {
    TimeInit();
    if (mg_Verbose >= 1) cprintf("%s loaded.\n", cDevName);
  }
}

void StrategyHandler(REQPACKET *prp)
{
  prp->usStatus = RPDONE;

  TraceArgs(0x0002, 4, prp->bCommand);

  switch (prp->bCommand) {
  case STRATEGY_INIT:
    StratInit(prp);
    break;
  case STRATEGY_OPEN:
    StratOpen(prp);
    break;
  case STRATEGY_CLOSE:
    StratClose(prp);
    break;
  case STRATEGY_READ:
    #ifdef DEBUG
    StratRead(prp);
    #else
    prp->usStatus = RPDONE | RPERR_BADCOMMAND;
    #endif
    break;
  case STRATEGY_GENIOCTL:
    StratIoctl(prp);
    break;
  case STRATEGY_DEINSTALL:
    break;
  case STRATEGY_INITCOMPLETE:
    break;
  case STRATEGY_SAVERESTORE:
    StratSaveRestore(prp);
    break;
  default:
    prp->usStatus = RPDONE | RPERR_BADCOMMAND;
    break;
  }
}

