/*
 */

#ifndef __DRIVER_H
#define __DRIVER_H

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
  struct net_device_stats stats;
  u32 hw_features;
  u32 vlan_features;
  u32 priv_flags;
};

#define MAX_DEVNAME     8
#define MAX_DRIVERS     9

extern char cDevName[MAX_DEVNAME + 1];

extern int mg_Verbose;
extern int mg_Adapter;
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

#ifndef TRACING
#define TraceBuf(a,b,c)
#define TraceArgs(a,b, ...)
#endif

#ifdef DEBUG
#define DPRINTF(a,b,...) dprintf(a, b, ##__VA_ARGS__)
#define DHEXDUMP(a,b,c,...) dHexDump(a, b, c, ##__VA_ARGS__)
#else
#define DPRINTF(a,b,...)
#define DHEXDUMP(a,b,c,...)
#endif

/* interface exported from individual driver */
extern void DriverIOCtlGenMac(REQPACKET *pPacket);
extern int DriverSuspend(void);
extern int DriverResume(void);
extern int FindAndSetupAdapter(void);
extern int DriverCheckDevice(struct net_device *pDev);
extern int DriverInitAdapter(struct net_device *pDev);

#endif

