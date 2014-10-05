#include "spigot.h"
#include "hw_common.h"
#include "iw_handler.h"
#include "apmcalls.h"

NIC_DEVICE near dev;

int near checkDevice( NIC_DEVICE near & dev)
{
  return 0;
};

short unsigned near NIC_DEVICE::IRQHandler()
{
  return 0;
};

void NIC_DEVICE::IoctlGenMac(RPIOCtl far *pPacket) {
	ULONG far *pData;
	ULONG far *pParm;

	if (pPacket->Function != GENMAC_WRAPPER_OID_GET) {
		pPacket->Status |= RPERR_PARAMETER;
		return;
	}
	if (pPacket->ParmLength < (sizeof(ULONG)*3)) {
		pPacket->Status |= RPERR_PARAMETER;
		return;
	}
	if (pPacket->DataLength < sizeof(ULONG)) {
		pPacket->Status |= RPERR_PARAMETER;
		return;
	}

	pParm = (ULONG far *)pPacket->ParmPacket;
	pData = (ULONG far *)pPacket->DataPacket;

	switch (pParm[0]) {
	case OID_GEN_LINK_SPEED:
		*pData = AdapterSC.MscLinkSpd / 100;
		pParm[2] = 1;
		break;
	default:
		pParm[2] = 0;
		*pData = 0;
		break;
	}

};

int near NIC_DEVICE::startIRQ()
{
  return 0;
};

int near NIC_DEVICE::ProcessParms( ModCfg far * )
{
  return 0;
};

int near NIC_DEVICE::setup()
{
  return 0;
};

int near NIC_DEVICE::open()
{
  return 0;
};

void near NIC_DEVICE::setMac()
{
  ;
};

void near NIC_DEVICE::setMcast()
{
  ;
};

int near NIC_DEVICE::startXmit( TxBufDesc far * bufdesc)
{
  return 0;
};

int near NIC_DEVICE::xferRX( short unsigned far *buf, short unsigned i, TDBufDesc far *bufdesc)
{
  return 0;
};

int near NIC_DEVICE::releaseRX( short unsigned r)
{
	DPRINTF(5, "NIC_DEVICE::releaseRX enter.\n");

	int res;

	if(pos < rx_ring_size)
	{
		rx_ring.orig[pos].flaglen_hi = 0;
		res = SUCCESS;
	}
	else
	{
		res = INVALID_PARAMETER;
	}

	DPRINTF(5, "NIC_DEVICE::releaseRX exit.\n");

	return res;
};

void near NIC_DEVICE::getHWStats()
{
  ;
};
