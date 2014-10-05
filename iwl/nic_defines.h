#define NIC_TX_RING_DEFAULT 9
#define NIC_RX_RING_DEFAULT 9

#include "ndis.h"
#include "devrp.h"
#include "devhelp.h"
#include "pci_dev.h"
#include "driver.h"

/*************** DRIVER STATUS FUNCTIONS   *****/

#define STATUS_HCMD_ACTIVE	0	/* host command in progress */
/* 1 is unused (used to be STATUS_HCMD_SYNC_ACTIVE) */
#define STATUS_INT_ENABLED	2
#define STATUS_RF_KILL_HW	3
#define STATUS_CT_KILL		4
#define STATUS_INIT		5
#define STATUS_ALIVE		6
#define STATUS_READY		7
#define STATUS_TEMPERATURE	8
#define STATUS_GEO_CONFIGURED	9
#define STATUS_EXIT_PENDING	10
#define STATUS_STATISTICS	12
#define STATUS_SCANNING		13
#define STATUS_SCAN_ABORTING	14
#define STATUS_SCAN_HW		15
#define STATUS_POWER_PMI	16
#define STATUS_FW_ERROR		17

