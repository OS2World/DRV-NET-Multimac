#include "nic_defines.h"
#include "cfg80211.h"
#include "iwl-dev.h"

#define TX_RING_DEFAULT NIC_TX_RING_DEFAULT
#define RX_RING_DEFAULT NIC_TX_RING_DEFAULT
#define  RX_GDT_COUNT 9
#define  TX_GDT_COUNT 9

typedef struct _ring_desc
{
    u32 buf_lo;
    u32 buf_hi;
    u32 flaglen_lo;
    u32 flaglen_hi;
} ring_desc, far *pring_desc;


union ring_type
{
    pring_desc orig;
};

class NIC_DEVICE : public PCI_NIC_DEVICE
{
private:

    u32 flags;
    u16 register_size;

    fpu8 ba;
    u8 mac[6];
    u16 irqmask;
    u32 linkspeed;
    u16 rxstarted;

    u16 vlanctl_bits;
    u32 ring_addr;
    u16 gdt[2];

    union ring_type get_rx, put_rx, first_rx, last_rx;
    union ring_type tx_ring;
    u32 rxdma;
    u32 rxbufsize;
    union ring_type rx_ring;
    u32 rx_buf_sz;
    u16 rx_ring_size;
    u16 rxgdt[RX_GDT_COUNT];
    fpu8 rx_virt_buf[RX_RING_DEFAULT];
    u16 rxpos;
    fpu8 currentrxbuffer;
    u16 currentlen;
    u16 txpos;
    u32 tx_flags;
    u16 tx_ring_size;
    u32 txdma;
    u32 txbufsize;
    u16 txgdt[TX_GDT_COUNT];
    fpu8 tx_virt_buf[TX_RING_DEFAULT];
    u16 pos;

    Mutex mutex;

public:
    NIC_DEVICE() : PCI_NIC_DEVICE(), mutex() {}
    ~NIC_DEVICE() {}
    int setup();
    int open();
    void setFlags(u32 newflags) { flags = newflags; }
    u16 IRQHandler();
    void setMcast();
    void setMac();
    void getHWStats();
    int startXmit(TxBufDesc far *pDsc);
    int releaseRX(u16 pos);
    int xferRX(fpu16 pcopied, u16 frameOffset, TDBufDesc far * pd);
    int Suspend();
    int Resume();
    int startIRQ();
    int ProcessParms(ModCfg far *pConfig);
    void IoctlGenMac(RPIOCtl far *pPacket);

struct iwl_priv {
      /* ieee device used by generic ieee processing code */
      struct ieee80211_hw *hw;
      struct ieee80211_channel *ieee_channels;
      struct ieee80211_rate *ieee_rates;
      struct iwl_cfg *cfg;
      /* temporary frame storage list */
      struct list_head free_frames;
      int frames_count;
      enum ieee80211_band band;
      int alloc_rxb_page;
} priv;

private:
    void initRX();
    void initTX();
    int initRing();
    int allocRX();
    int allocTX();
    void drainRX();
    void drainTX();
    void drainRing();
    void startRX();
    void stopRX();
    void startTX();
    void stopTX();
    int rxProcess();
};






extern NIC_DEVICE dev;
int checkDevice(NIC_DEVICE &dev);
