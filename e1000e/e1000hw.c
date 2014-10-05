/*
 * This source is the part of e1000e - Intel PRO/1000 NDIS driver for OS/2
 *
 * Copyright (C) 2010
 * Copyright (c) 2012 Mensys BV
 * 2012-02-11 DAZ resynced with e1000e driver from Linux kernel v 3.2.4
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
 *
 * Debug levels:
 *  1) critical errors
 *  2) non-critical errors
 *  3) warnings
 *  4) information messages
 *  5) function call trace
 *  6) logging from interrupts - debug output may be broken
 */

#include "Dev16lib.h"
#include "base.h"
#include "ndis.h"
#include "driver.h"
#include "ioctl.h"
#include "e1000hw.h"
#include "version.h"

#define MSG_HWDET 11
u16 fixed_mode = 0;
u16 dorxchain = 1;
u16 use_hw_netaddress = 1;

struct e1000_private {
    struct net_device *pDev;
    u32 flags;

    fpu8 ba;
    u8 mac[6];
    u32 linkspeed;
    u16 rxstarted;
    u16 Suspended;

    u16 vlanctl_bits;
    u32 ring_addr;
    u16 gdt[2];

    pring_desc rx_ring;
    u16 rx_ring_size;
    u16 rxpos;
    fpu8 currentrxbuffer;
    u16 currentlen;
    pring_desc tx_ring;
    u16 txpos;
    u16 tx_ring_size;

    Mutex mutex;
    u16 txbusy;
    struct e1000_adapter adapter;
} Priv;

static struct pci_device_id
{
    fpchar name;
    u16   vendor;
    u16   device;
    u32  flags;
} pci_tbl[] = {
    {
        "82571EB",
        E1000_VENDOR_ID_INTEL, E1000_DEV_ID_82571EB_COPPER,
        board_82571
    },
    {
        "82571EB",
        E1000_VENDOR_ID_INTEL, E1000_DEV_ID_82571EB_FIBER,
        board_82571
    },
    {
        "82571EB",
        E1000_VENDOR_ID_INTEL, E1000_DEV_ID_82571EB_QUAD_COPPER,
        board_82571
    },
    {
        "82571EB",
        E1000_VENDOR_ID_INTEL, E1000_DEV_ID_82571EB_QUAD_COPPER_LP,
        board_82571
    },
    {
        "82571EB",
        E1000_VENDOR_ID_INTEL, E1000_DEV_ID_82571EB_QUAD_FIBER,
        board_82571
    },
    {
        "82571EB",
        E1000_VENDOR_ID_INTEL, E1000_DEV_ID_82571EB_SERDES,
        board_82571
    },
    {
        "82571EB",
        E1000_VENDOR_ID_INTEL, E1000_DEV_ID_82571EB_SERDES_DUAL,
        board_82571
    },
    {
        "82571EB",
        E1000_VENDOR_ID_INTEL, E1000_DEV_ID_82571EB_SERDES_QUAD,
        board_82571
    },
    {
        "82571PT",
        E1000_VENDOR_ID_INTEL, E1000_DEV_ID_82571PT_QUAD_COPPER,
        board_82571
    },
    {
        "82572EI",
        E1000_VENDOR_ID_INTEL, E1000_DEV_ID_82572EI,
        board_82572
    },
    {
        "82572EI",
        E1000_VENDOR_ID_INTEL, E1000_DEV_ID_82572EI_COPPER,
        board_82572
    },
    {
        "82572EI",
        E1000_VENDOR_ID_INTEL, E1000_DEV_ID_82572EI_FIBER,
        board_82572
    },
    {
        "82572EI",
        E1000_VENDOR_ID_INTEL, E1000_DEV_ID_82572EI_SERDES,
        board_82572
    },
    {
        "82573V",
        E1000_VENDOR_ID_INTEL, E1000_DEV_ID_82573E,
        board_82573
    },
    {
        "82573E",
        E1000_VENDOR_ID_INTEL, E1000_DEV_ID_82573E_IAMT,
        board_82573
    },
    {
        "82573L",
        E1000_VENDOR_ID_INTEL, E1000_DEV_ID_82573L,
        board_82573
    },
    {
        "82574L",
        E1000_VENDOR_ID_INTEL, E1000_DEV_ID_82574L,
        board_82574
    },
    {
        "82574L",
        E1000_VENDOR_ID_INTEL, E1000_DEV_ID_82574LA,
        board_82574
    },
    {
        "82583V",
        E1000_VENDOR_ID_INTEL, E1000_DEV_ID_82583V,
        board_82583
    },
    {
        "80003ES2LAN",
        E1000_VENDOR_ID_INTEL, E1000_DEV_ID_80003ES2LAN_COPPER_DPT,
        board_80003es2lan
    },
    {
        "80003ES2LAN",
        E1000_VENDOR_ID_INTEL, E1000_DEV_ID_80003ES2LAN_COPPER_SPT,
        board_80003es2lan
    },
    {
        "80003ES2LAN",
        E1000_VENDOR_ID_INTEL, E1000_DEV_ID_80003ES2LAN_SERDES_DPT,
        board_80003es2lan
    },
    {
        "80003ES2LAN",
        E1000_VENDOR_ID_INTEL, E1000_DEV_ID_80003ES2LAN_SERDES_SPT,
        board_80003es2lan
    },
    {
        "82562V",
        E1000_VENDOR_ID_INTEL, E1000_DEV_ID_ICH8_IFE,
        board_ich8lan
    },
    {
        "82562G",
        E1000_VENDOR_ID_INTEL, E1000_DEV_ID_ICH8_IFE_G,
        board_ich8lan
    },
    {
        "82562GT",
        E1000_VENDOR_ID_INTEL, E1000_DEV_ID_ICH8_IFE_GT,
        board_ich8lan
    },
    {
        "82566DM",
        E1000_VENDOR_ID_INTEL, E1000_DEV_ID_ICH8_IGP_AMT,
        board_ich8lan
    },
    {
        "82566DC",
        E1000_VENDOR_ID_INTEL, E1000_DEV_ID_ICH8_IGP_C,
        board_ich8lan
    },
    {
        "82566MC",
        E1000_VENDOR_ID_INTEL, E1000_DEV_ID_ICH8_IGP_M,
        board_ich8lan
    },
    {
        "82566MM",
        E1000_VENDOR_ID_INTEL, E1000_DEV_ID_ICH8_IGP_M_AMT,
        board_ich8lan
    },
    {
        "82567V-3",
        E1000_VENDOR_ID_INTEL, E1000_DEV_ID_ICH8_82567V_3,
        board_ich8lan
    },
    {
        "82562V-2",
        E1000_VENDOR_ID_INTEL, E1000_DEV_ID_ICH9_IFE,
        board_ich9lan
    },
    {
        "82562G-2",
        E1000_VENDOR_ID_INTEL, E1000_DEV_ID_ICH9_IFE_G,
        board_ich9lan
    },
    {
        "82562GT-2",
        E1000_VENDOR_ID_INTEL, E1000_DEV_ID_ICH9_IFE_GT,
        board_ich9lan
    },
    {
        "82566DM-2",
        E1000_VENDOR_ID_INTEL, E1000_DEV_ID_ICH9_IGP_AMT,
        board_ich9lan
    },
    {
        "82566DC-2",
        E1000_VENDOR_ID_INTEL, E1000_DEV_ID_ICH9_IGP_C,
        board_ich9lan
    },
    {
        "82567LM-4",
        E1000_VENDOR_ID_INTEL, E1000_DEV_ID_ICH9_BM,
        board_ich9lan
    },
    {
        "82567LF",
        E1000_VENDOR_ID_INTEL, E1000_DEV_ID_ICH9_IGP_M,
        board_ich9lan
    },
    {
        "82567LM",
        E1000_VENDOR_ID_INTEL, E1000_DEV_ID_ICH9_IGP_M_AMT,
        board_ich9lan
    },
    {
        "82567V",
        E1000_VENDOR_ID_INTEL, E1000_DEV_ID_ICH9_IGP_M_V,
        board_ich9lan
    },
    {
        "82567LM-2",
        E1000_VENDOR_ID_INTEL, E1000_DEV_ID_ICH10_R_BM_LM,
        board_ich9lan
    },
    {
        "82567LF-2",
        E1000_VENDOR_ID_INTEL, E1000_DEV_ID_ICH10_R_BM_LF,
        board_ich9lan
    },
    {
        "82567V-2",
        E1000_VENDOR_ID_INTEL, E1000_DEV_ID_ICH10_R_BM_V,
        board_ich9lan
    },
    {
        "82567LM-3",
        E1000_VENDOR_ID_INTEL, E1000_DEV_ID_ICH10_D_BM_LM,
        board_ich10lan
    },
    {
        "82567LF-3",
        E1000_VENDOR_ID_INTEL, E1000_DEV_ID_ICH10_D_BM_LF,
        board_ich10lan
    },
    {
        "82577LM",
        E1000_VENDOR_ID_INTEL, E1000_DEV_ID_PCH_M_HV_LM,
        board_pchlan
    },
    {
        "82577LC",
        E1000_VENDOR_ID_INTEL, E1000_DEV_ID_PCH_M_HV_LC,
        board_pchlan
    },
    {
        "82578DM",
        E1000_VENDOR_ID_INTEL, E1000_DEV_ID_PCH_D_HV_DM,
        board_pchlan
    },
    {
        "82578DC",
        E1000_VENDOR_ID_INTEL, E1000_DEV_ID_PCH_D_HV_DC,
        board_pchlan
    },
    {
        "82579LM",
        E1000_VENDOR_ID_INTEL, E1000_DEV_ID_PCH2_LV_LM,
        board_pch2lan
    },
    {
        "82579V",
        E1000_VENDOR_ID_INTEL, E1000_DEV_ID_PCH2_LV_V,
        board_pch2lan
    },
    { NULL, 0, 0, 0 }
};

const struct e1000_info *e1000_info_tbl[] = {
    &e1000_82571_info,  // board_82571
    &e1000_82572_info,  // board_82572
    &e1000_82573_info,  // board_82573
    &e1000_82574_info,  // board_82574
    &e1000_82583_info,  // board_82583
    &e1000_es2_info,    // board_80003es2lan
    &e1000_ich8_info,   // board_ich8lan
    &e1000_ich9_info,   // board_ich9lan
    &e1000_ich10_info,  // board_ich10lan
    &e1000_pch_info,    // board_pchlan
    &e1000_pch2_info   // board_pch2lan
};

u32 __er32(struct e1000_hw *hw, unsigned long reg)
{
    return readl(hw->hw_addr + reg);
}

void __ew32(struct e1000_hw *hw, unsigned long reg, u32 val)
{
    writel(val, hw->hw_addr + reg);
}

/**
 * e1000_get_hw_control - get control of the h/w from f/w
 * @adapter: address of board private structure
 *
 * e1000_get_hw_control sets {CTRL_EXT|SWSM}:DRV_LOAD bit.
 * For ASF and Pass Through versions of f/w this means that
 * the driver is loaded. For AMT version (only with 82573)
 * of the f/w this means that the network i/f is open.
 **/
void e1000_get_hw_control(struct e1000_adapter *adapter)
{
    struct e1000_hw *hw = &adapter->hw;
    u32 ctrl_ext;
    u32 swsm;

    /* Let firmware know the driver has taken over */
    if (adapter->flags & FLAG_HAS_SWSM_ON_LOAD) {
        swsm = er32(SWSM);
        ew32(SWSM, swsm | E1000_SWSM_DRV_LOAD);
    } else if (adapter->flags & FLAG_HAS_CTRLEXT_ON_LOAD) {
        ctrl_ext = er32(CTRL_EXT);
        ew32(CTRL_EXT, ctrl_ext | E1000_CTRL_EXT_DRV_LOAD);
    }
}

static void set_default_params(struct e1000_adapter *adapter)
{
    adapter->tx_int_delay = 8;
    adapter->tx_abs_int_delay = 32;
    adapter->rx_int_delay = 0;
    adapter->rx_abs_int_delay = 8;
    adapter->itr_setting = 3;
    adapter->itr = 20000;
    adapter->int_mode = 2;
    adapter->flags2 |= FLAG2_CRC_STRIPPING;
    adapter->flags |= FLAG_READ_ONLY_NVM;
    if(adapter->hw.mac.type == e1000_ich8lan)
    {
        e1000e_set_kmrn_lock_loss_workaround_ich8lan(&adapter->hw, true);
    }
}

static void e1000_eeprom_checks(struct e1000_adapter *adapter)
{
    struct e1000_hw *hw = &adapter->hw;
    long int ret_val;
    u16 buf = 0;

    if (hw->mac.type != e1000_82573)
        return;

    ret_val = e1000_read_nvm(hw, NVM_INIT_CONTROL2_REG, 1, &buf);
    if (!ret_val && (!(le16_to_cpu(buf) & (1 << 0)))) {
        /* Deep Smart Power Down (DSPD) */
        DPRINTF(3, "Warning: detected DSPD enabled in EEPROM\n");
    }

    ret_val = e1000_read_nvm(hw, NVM_INIT_3GIO_3, 1, &buf);
    if (!ret_val && (le16_to_cpu(buf) & (3 << 2))) {
        /* ASPM enable */
        DPRINTF(3, "Warning: detected ASPM enabled in EEPROM\n");
    }
}

/**
 * e1000_irq_disable - Mask off interrupt generation on the NIC
 **/
static void e1000_irq_disable(struct e1000_adapter *adapter)
{
    struct e1000_hw *hw = &adapter->hw;

    ew32(IMC, ~0L);
    if (adapter->msix_entries)
        ew32(EIAC_82574, 0);
    e1e_flush();
}

/**
 * e1000_sw_init - Initialize general software structures (struct e1000_adapter)
 * @adapter: board private structure to initialize
 *
 * e1000_sw_init initializes the Adapter private data structure.
 * Fields are initialized based on PCI device information and
 * OS network device settings (MTU size).
 **/
static int e1000_sw_init(struct e1000_adapter *adapter)
{
    adapter->rx_buffer_len = ETH_FRAME_LEN + ETH_FCS_LEN;
    adapter->rx_ps_bsize0 = 128;
    adapter->max_frame_size = ETH_FRAME_LEN + ETH_HLEN + ETH_FCS_LEN;
    adapter->min_frame_size = ETH_ZLEN + ETH_FCS_LEN;

    /* Explicitly disable IRQ since the NIC can be in any state. */
    e1000_irq_disable(adapter);

    return 0;
}

int initRing()
{
    u16 i;

    int rc = DevHelp_AllocGDTSelector(Priv.gdt, 2);
    if(rc) return 1;

    for(i = 0; i < Priv.tx_ring_size; i++)
    {
        Priv.tx_ring[i].flaglen_lo = 0;
        Priv.tx_ring[i].flaglen_hi = 0;
        Priv.tx_ring[i].buf_lo = 0;
        Priv.tx_ring[i].buf_hi = 0;
    }

    for(i = 0; i < Priv.rx_ring_size; i++)
    {
        Priv.rx_ring[i].flaglen_lo = 0;
        Priv.rx_ring[i].flaglen_hi = 0;
        Priv.rx_ring[i].buf_lo = cpu_to_le32(GetRxBufferPhysicalAddress(Priv.pDev, i));
        Priv.rx_ring[i].buf_hi = 0;
    }

    Priv.rxpos = 0;
    Priv.txpos = 0;

    return 0;
}

static void e1000_init_manageability(struct e1000_adapter *adapter)
{
    struct e1000_hw *hw = &adapter->hw;
    u32 manc, manc2h;

    if (!(adapter->flags & FLAG_MNG_PT_ENABLED))
        return;

    manc = er32(MANC);

    /*
     * enable receiving management packets to the host. this will probably
     * generate destination unreachable messages from the host OS, but
     * the packets will be handled on SMBUS
     */
    manc |= E1000_MANC_EN_MNG2HOST;
    manc2h = er32(MANC2H);
#define E1000_MNG2HOST_PORT_623 (1 << 5)
#define E1000_MNG2HOST_PORT_664 (1 << 6)
    manc2h |= E1000_MNG2HOST_PORT_623;
    manc2h |= E1000_MNG2HOST_PORT_664;
    ew32(MANC2H, manc2h);
    ew32(MANC, manc);
}

/**
 * e1000_configure_tx - Configure 8254x Transmit Unit after Reset
 * @adapter: board private structure
 *
 * Configure the Tx unit of the MAC after a reset.
 **/
static void e1000_configure_tx(struct e1000_adapter *adapter)
{
    struct e1000_hw *hw = &adapter->hw;
    //struct e1000_ring *tx_ring = adapter->tx_ring;
    u32 tctl, tipg, tarc;
    u32 ipgr1, ipgr2;

    /* Setup the HW Tx Head and Tail descriptor pointers */
    ew32(TDBAL, Priv.pDev->tx.RingDma);
    ew32(TDBAH, 0);
    ew32(TDLEN, (u32)Priv.pDev->tx.RingCount * sizeof(struct e1000_tx_desc));
    ew32(TDH, 0);
    ew32(TDT, 0);
    //tx_ring->head = E1000_TDH;
    //tx_ring->tail = E1000_TDT;

    /* Set the default values for the Tx Inter Packet Gap timer */
    tipg = DEFAULT_82543_TIPG_IPGT_COPPER;          /*  8  */
    ipgr1 = DEFAULT_82543_TIPG_IPGR1;               /*  8  */
    ipgr2 = DEFAULT_82543_TIPG_IPGR2;               /*  6  */

    if (adapter->flags & FLAG_TIPG_MEDIUM_FOR_80003ESLAN)
        ipgr2 = DEFAULT_80003ES2LAN_TIPG_IPGR2; /*  7  */

    tipg |= ipgr1 << E1000_TIPG_IPGR1_SHIFT;
    tipg |= ipgr2 << E1000_TIPG_IPGR2_SHIFT;
    ew32(TIPG, tipg);

    /* Set the Tx Interrupt Delay register */
    ew32(TIDV, adapter->tx_int_delay);
    /* Tx irq moderation */
    ew32(TADV, adapter->tx_abs_int_delay);

    /* Program the Transmit Control Register */
    tctl = er32(TCTL);
    tctl &= ~(u32)E1000_TCTL_CT;
    tctl |= E1000_TCTL_PSP | E1000_TCTL_RTLC |
        (E1000_COLLISION_THRESHOLD << E1000_CT_SHIFT);

    if (adapter->flags & FLAG_TARC_SPEED_MODE_BIT) {
        tarc = er32(TARC(0));
        /*
         * set the speed mode bit, we'll clear it if we're not at
         * gigabit link later
         */
#define SPEED_MODE_BIT (1L << 21)
        tarc |= SPEED_MODE_BIT;
        ew32(TARC(0), tarc);
    }

    /* errata: program both queues to unweighted RR */
    if (adapter->flags & FLAG_TARC_SET_BIT_ZERO) {
        tarc = er32(TARC(0));
        tarc |= 1;
        ew32(TARC(0), tarc);
        tarc = er32(TARC(1));
        tarc |= 1;
        ew32(TARC(1), tarc);
    }

    /* Setup Transmit Descriptor Settings for eop descriptor */
    adapter->txd_cmd = E1000_TXD_CMD_EOP | E1000_TXD_CMD_IFCS;

    /* only set IDE if we are delaying interrupts using the timers */
    if (adapter->tx_int_delay)
        adapter->txd_cmd |= E1000_TXD_CMD_IDE;

    /* enable Report Status bit */
    adapter->txd_cmd |= E1000_TXD_CMD_RS;

    ew32(TCTL, tctl);

    e1000e_config_collision_dist(hw);
}

/**
 * e1000_setup_rctl - configure the receive control registers
 * @adapter: Board private structure
 **/
static void e1000_setup_rctl(struct e1000_adapter *adapter)
{
    struct e1000_hw *hw = &adapter->hw;
    u32 rctl;
    //u32 psrctl = 0;
    //u32 pages = 0;

        /* Workaround Si errata on 82579 - configure jumbo frame flow */
        if (hw->mac.type == e1000_pch2lan) {
                s32 ret_val;

                //if (adapter->netdev->mtu > ETH_DATA_LEN)
                //      ret_val = e1000_lv_jumbo_workaround_ich8lan(hw, true);
                //else
                        ret_val = e1000_lv_jumbo_workaround_ich8lan(hw, false);

                if (ret_val)
                        e_dbg("failed to enable jumbo frame workaround mode\n");
        }

    /* Program MC offset vector base */
    rctl = er32(RCTL);
    rctl &= ~(3L << E1000_RCTL_MO_SHIFT);
    rctl |= E1000_RCTL_EN | E1000_RCTL_BAM |
        E1000_RCTL_LBM_NO | E1000_RCTL_RDMTS_HALF |
        (adapter->hw.mac.mc_filter_type << E1000_RCTL_MO_SHIFT);

    /* Do not Store bad packets */
    rctl &= ~(u32)E1000_RCTL_SBP;

    /* Disable Long Packet receive */
    rctl &= ~(u32)E1000_RCTL_LPE;

    /* Some systems expect that the CRC is included in SMBUS traffic. The
     * hardware strips the CRC before sending to both SMBUS (BMC) and to
     * host memory when this is enabled
     */
    if (adapter->flags2 & FLAG2_CRC_STRIPPING)
        rctl |= E1000_RCTL_SECRC;

    /* Workaround Si errata on 82577 PHY - configure IPG for jumbos */
    if ((hw->phy.type == e1000_phy_82577) && (rctl & E1000_RCTL_LPE)) {
        u16 phy_data;

        e1e_rphy(hw, PHY_REG(770, 26), &phy_data);
        phy_data &= 0xfff8;
        phy_data |= (1 << 2);
        e1e_wphy(hw, PHY_REG(770, 26), phy_data);

        e1e_rphy(hw, 22, &phy_data);
        phy_data &= 0x0fff;
        phy_data |= (1 << 14);
        e1e_wphy(hw, 0x10, 0x2823);
        e1e_wphy(hw, 0x11, 0x0003);
        e1e_wphy(hw, 22, phy_data);
    }

    /* Setup buffer sizes */
    rctl &= ~E1000_RCTL_SZ_4096;
    rctl |= E1000_RCTL_BSEX;
    switch (adapter->rx_buffer_len) {
    case 2048:
    default:
        rctl |= E1000_RCTL_SZ_2048;
        rctl &= ~E1000_RCTL_BSEX;
        break;
    case 4096:
        rctl |= E1000_RCTL_SZ_4096;
        break;
    case 8192:
        rctl |= E1000_RCTL_SZ_8192;
        break;
    case 16384:
        rctl |= E1000_RCTL_SZ_16384;
        break;
    }

    ew32(RCTL, rctl);
    /* just started the receive unit, no need to restart */
    adapter->flags &= ~FLAG_RX_RESTART_NOW;
}

/**
 * e1000_configure_rx - Configure Receive Unit after Reset
 * @adapter: board private structure
 *
 * Configure the Rx unit of the MAC after a reset.
 **/
static void e1000_configure_rx(struct e1000_adapter *adapter)
{
    struct e1000_hw *hw = &adapter->hw;
    //struct e1000_ring *rx_ring = adapter->rx_ring;
    u32 rctl, rxcsum, ctrl_ext;

    /* disable receives while setting up the descriptors */
    rctl = er32(RCTL);

        if (!(adapter->flags2 & FLAG2_NO_DISABLE_RX))
        ew32(RCTL, rctl & ~(u32)E1000_RCTL_EN);
    e1e_flush();
    msleep(10);

    /* set the Receive Delay Timer Register */
    ew32(RDTR, adapter->rx_int_delay);

    /* irq moderation */
    ew32(RADV, adapter->rx_abs_int_delay);
    if (adapter->itr_setting != 0)
        //ew32(ITR, div_u32(1000000000, adapter->itr * 256));
        ew32(ITR, 1000000000 / (adapter->itr * 256));

    ctrl_ext = er32(CTRL_EXT);
    /* Auto-Mask interrupts upon ICR access */
//  ctrl_ext |= E1000_CTRL_EXT_IAME;
    ew32(IAM, 0xffffffff);
    ew32(CTRL_EXT, ctrl_ext);
    e1e_flush();

    /*
     * Setup the HW Rx Head and Tail Descriptor Pointers and
     * the Base and Length of the Rx Descriptor Ring
     */
    ew32(RDBAL, Priv.pDev->rx.RingDma);
    ew32(RDBAH, 0);
    ew32(RDLEN, (u32)Priv.pDev->rx.RingCount * sizeof(struct e1000_rx_desc));
    ew32(RDH, 0);
    ew32(RDT, 0);
    //rx_ring->head = E1000_RDH;
    //rx_ring->tail = E1000_RDT;

    /* Enable Receive Checksum Offload for TCP and UDP */
    rxcsum = er32(RXCSUM);
    if (adapter->flags & FLAG_RX_CSUM_ENABLED) {
        rxcsum |= E1000_RXCSUM_TUOFL;
    } else {
        rxcsum &= ~(u32)E1000_RXCSUM_TUOFL;
        /* no need to clear IPPCSE as it defaults to 0 */
    }
    ew32(RXCSUM, rxcsum);

    /* Enable Receives */
    ew32(RCTL, rctl);
}

/**
 * e1000_configure - configure the hardware for Rx and Tx
 * @adapter: private board structure
 **/
static void e1000_configure(struct e1000_adapter *adapter)
{
    e1000_init_manageability(adapter);

    e1000_configure_tx(adapter);
    e1000_setup_rctl(adapter);
    e1000_configure_rx(adapter);
}

void startRX()
{
    //DPRINTF(5, "NIC_DEVICE::startRX enter\n");

    struct e1000_hw* hw = &Priv.adapter.hw;

    ew32(IMS, E1000_IMS_RXT0 | E1000_IMS_RXDMT0);
    ew32(RDT, Priv.rx_ring_size - 1);
    ew32(RCTL, er32(TCTL) | E1000_RCTL_EN);

    Priv.rxstarted = 1;

    //DPRINTF(5, "NIC_DEVICE::startRX exit\n");
}

void startTX()
{
    //DPRINTF(5, "NIC_DEVICE::startTX enter\n");

    struct e1000_hw* hw = &Priv.adapter.hw;

    ew32(TCTL, er32(TCTL) | E1000_TCTL_EN);

    //DPRINTF(5, "NIC_DEVICE::startTX exit\n");
}

void drainTX()
{
    int i;

    //DPRINTF(5, "NIC_DEVICE::drainTX enter.\n");

    for(i = 0; i < Priv.tx_ring_size; i++)
    {
        Priv.tx_ring[i].flaglen_lo = 0;
        Priv.tx_ring[i].flaglen_hi = 0;
        Priv.tx_ring[i].buf_lo = 0;
        Priv.tx_ring[i].buf_hi = 0;
    }

    //DPRINTF(5, "NIC_DEVICE::drainTX exit.\n");
}

void drainRX()
{
    int i;

    //DPRINTF(5, "NIC_DEVICE::drainRX enter.\n");

    u32 dma = Priv.rx_ring[0].buf_lo;
    for(i = 0; i < Priv.rx_ring_size; i++)
    {
        Priv.rx_ring[i].flaglen_lo = 0;
        Priv.rx_ring[i].flaglen_hi = 0;
        Priv.rx_ring[i].buf_lo = 0;
        Priv.rx_ring[i].buf_hi = 0;
    }
    if(dma)
    {
        DevHelp_FreePhys(dma);
    }

    //DPRINTF(5, "NIC_DEVICE::drainRX exit.\n");
}

void drainRing()
{
    //DPRINTF(5, "NIC_DEVICE::drainRing enter.\n");

    drainTX();
    drainRX();

    //DPRINTF(5, "NIC_DEVICE::drainRing exit.\n");
}

int rxProcess()
{
    //DPRINTF(5, "NIC_DEVICE::rxProcess enter.\n");

    u32 flags;
    u16 len;
    u16 release;
    struct e1000_hw* hw = &Priv.adapter.hw;
    int i;
    fpu8 data, pcInd;

    TraceArgs(0x0010, 10, (u16)Priv.rxpos, (u32)er32(RDH), (u32)er32(RDT));
    DPRINTF(6, "rxpos=%x RDH=%lx RDT=%lx\n", Priv.rxpos, er32(RDH), er32(RDT));

    for (i = 0; ((flags = le32_to_cpu(Priv.rx_ring[Priv.rxpos].flaglen_hi)) & E1000_RXD_STAT_DD) && i < Priv.rx_ring_size; Priv.rxpos = ((Priv.rxpos + 1 >= Priv.rx_ring_size) ? 0 : Priv.rxpos + 1), i++)
    {
        release = 1;
        len = (u16) Priv.rx_ring[Priv.rxpos].flaglen_lo;

        if (len > 0)
        {
            if (NdisGetInfo(NDISINFO_Indications)) break;

            data = GetRxBufferVirtualAddress(Priv.pDev, Priv.rxpos);
            if (!data) return -1;
            pcInd = &data[E1000_RX_ALLOCATE - 1];

            NdisIndicationsOff();
            *pcInd = 0xFF;

            TraceArgs(0x0014, 8, (u16)Priv.rxpos, (u16)len, (u32)flags);
            TraceBuf(0x0011, len<64?len:64, data);

            if (dorxchain)
            {
                u16 rc;

                RxBufOne far * rxbuf = (RxBufOne far *)(data + E1000_RX_ALLOCATE - 1 - sizeof(RxBufOne));
                rxbuf->RxDataCount = 1;
                rxbuf->RxDataBlk.RxDataLen = (u16)len;
                rxbuf->RxDataBlk.RxDataPtr = (fpu8)data;
                rc = ProtDT.PldRcvChain(AdapterCC.CcModuleID, (u16)len, Priv.rxpos, (fpu8)rxbuf, pcInd, ProtCC.CcDataSeg);
                if (rc == 1) release = 0;
            }
            else
            {
                Priv.currentrxbuffer = data;
                Priv.currentlen = (u16)len;
                ProtDT.PldRcvLkAhead(AdapterCC.CcModuleID, Priv.currentlen, Priv.currentlen, Priv.currentrxbuffer, pcInd, ProtCC.CcDataSeg);
                Priv.currentrxbuffer = NULL;
                Priv.currentlen = 0;
            }

            if (*pcInd) NdisIndicationsOn();
            ProtDT.PldIndComplete(AdapterCC.CcModuleID, ProtCC.CcDataSeg);
        }
        if(release)
        {
            u16 rx_tail;

            Priv.rx_ring[Priv.rxpos].flaglen_hi = 0;
            rx_tail = (u16) er32(RDT);
            if(++rx_tail == Priv.tx_ring_size)
            {
                rx_tail = 0;
            }
            ew32(RDT, rx_tail);
        }
    }

    TraceBuf(0x8010, 0, NULL);
    //DPRINTF(5, "NIC_DEVICE::rxProcess exit.\n");

    return i;
}

static void far IrqHandler()
{
    u32 events;
    u16 i;
    struct e1000_hw* hw = &Priv.adapter.hw;

    DevCli();


    i = 0;
    events = er32(ICR);

    TraceArgs(0x0020, 4, events);

    if(events & (E1000_ICR_RXT0 | E1000_ICR_RXDMT0))
    {
        if (mutex_request(&Priv.mutex))
        {
            rxProcess();
            mutex_unlock(&Priv.mutex);
        }
        ++i;
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

short NdisDriverStartIrq(void)
{
    u16 rc;

    TraceArgs(0x0040, 2, (u16)Priv.pDev->irq);
    //DPRINTF(5, "NIC_DEVICE::startIRQ enter\n");

    DevCli();
    rc = DevHelp_SetIRQ((NPFN)IrqHandler, Priv.pDev->irq, 1);
    DevSti();
    if(rc)
    {
        drainRing();
        return 1;
    }

    //DPRINTF(5, "NIC_DEVICE::startIRQ exit\n");

    return 0;
}

void stopRX()
{
    //DPRINTF(5, "NIC_DEVICE::stopRX enter\n");

    struct e1000_hw* hw = &Priv.adapter.hw;

    ew32(RCTL, er32(RCTL) & ~(u32)E1000_RCTL_EN);

    //DPRINTF(5, "NIC_DEVICE::stopRX exit\n");

    Priv.rxstarted = 0;
}

void stopTX()
{
    //DPRINTF(5, "NIC_DEVICE::stopTX enter.\n");

    struct e1000_hw* hw = &Priv.adapter.hw;

    ew32(TCTL, er32(TCTL) & ~(u32)E1000_TCTL_EN);

    //DPRINTF(5, "NIC_DEVICE::stopTX exit.\n");
}

/**
 *  e1000_update_mc_addr_list - Update Multicast addresses
 *  @hw: pointer to the HW structure
 *  @mc_addr_list: array of multicast addresses to program
 *  @mc_addr_count: number of multicast addresses to program
 *  @rar_used_count: the first RAR register free to program
 *  @rar_count: total number of supported Receive Address Registers
 *
 *  Updates the Receive Address Registers and Multicast Table Array.
 *  The caller must have a packed mc_addr_list of multicast addresses.
 *  The parameter rar_count will usually be hw->mac.rar_entry_count
 *  unless there are workarounds that change this.  Currently no func pointer
 *  exists and all implementations are handled in the generic version of this
 *  function.
 **/
static void e1000_update_mc_addr_list(struct e1000_hw *hw, u8 far *mc_addr_list, u32 mc_addr_count)
{
    hw->mac.ops.update_mc_addr_list(hw, mc_addr_list, mc_addr_count);
}

void NdisDriverSetMcast(u16 wFlags)
{
    struct e1000_hw* hw = &Priv.adapter.hw;
    u8 addr[E1000_RAR_ENTRIES][6] = { 0 };
        u32 rctl;
    u16 i, j;

    TraceArgs(0x0041, 4, (u16)AdapterMCB.McbCnt, (u16)Priv.pDev->flags);
    DPRINTF(5, "DriverSetMcast BEG cnt=%d Flags=%x\n", AdapterMCB.McbCnt, Priv.pDev->flags);

    mutex_lock(&Priv.mutex);
    DevCli();
    stopRX();
    /* Check for Promiscuous and All Multicast modes */
    rctl = er32(RCTL);

    /* clear the affected bits */
    rctl &= ~(E1000_RCTL_UPE | E1000_RCTL_MPE);

    if ( (wFlags & (FLTR_SET_FILTER|FLTR_PRMSCS))==(FLTR_SET_FILTER|FLTR_PRMSCS) )
    {
      DPRINTF(1, "Promiscuous mode enabled\n");
      rctl |= (E1000_RCTL_UPE | E1000_RCTL_MPE);
      /* Do not hardware filter VLANs in promisc mode */
      //e1000e_vlan_filter_disable(adapter);
    }
    else
    {
      if (AdapterMCB.McbCnt >= E1000_RAR_ENTRIES)
      {
        rctl |= E1000_RCTL_MPE;
      }
      else
      {
        if (AdapterMCB.McbCnt > 0)
        {
          for(i = 0; i < AdapterMCB.McbCnt; i++)
          {
            for(j = 0; j < 6; j++)
            {
              addr[i][j] = AdapterMCB.McbAddrs[i].mAddr[j];
            }
          }
          e1000_update_mc_addr_list(hw, &addr[0][0], min(AdapterMCB.McbCnt, E1000_RAR_ENTRIES));
        }
      }
    }

    ew32(RCTL, rctl);
    startRX();
    mutex_unlock(&Priv.mutex);
    DevSti();
}

/**
 * e1000_set_mac - Change the Ethernet Address of the NIC
 * @netdev: network interface device structure
 * @p: pointer to an address structure
 *
 * Returns 0 on success, negative on failure
 **/
static int e1000_set_mac(struct e1000_adapter *adapter, u8* mac)
{
    memcpy(adapter->hw.mac.addr, mac, sizeof adapter->hw.mac.addr);

    e1000e_rar_set(&adapter->hw, adapter->hw.mac.addr, 0);

    if (adapter->flags & FLAG_RESET_OVERWRITES_LAA) {
        /* activate the work around */
        e1000e_set_laa_state_82571(&adapter->hw, 1);

        /*
         * Hold a copy of the LAA in RAR[14] This is done so that
         * between the time RAR[0] gets clobbered  and the time it
         * gets fixed (in e1000_watchdog), the actual LAA is in one
         * of the RARs and no incoming packets directed to this port
         * are dropped. Eventually the LAA will be in RAR[0] and
         * RAR[14]
         */
        e1000e_rar_set(&adapter->hw,
                  adapter->hw.mac.addr,
                  adapter->hw.mac.rar_entry_count - 1);
    }

    return 0;
}

void NdisDriverSetMac(char *pMscCurrStnAdr, unsigned short len)
{
    u16 i;

    //DPRINTF(5, "NIC_DEVICE::setMac enter.\n");

    memcpy(Priv.mac, pMscCurrStnAdr, sizeof Priv.mac);

    TraceBuf(0x0046, 6, Priv.mac);

    if(Priv.rxstarted)
    {
        mutex_lock(&Priv.mutex);
        DevCli();
        stopRX();
        for(i = 0; i < 6; i++)
        {
            e1000_set_mac(&Priv.adapter, Priv.mac);
        }
        startRX();
        mutex_unlock(&Priv.mutex);
        DevSti();
    }
    else
    {
        for(i = 0; i < 6; i++)
        {
            e1000_set_mac(&Priv.adapter, Priv.mac);
        }
    }

    //DPRINTF(5, "NIC_DEVICE::setMac exit.\n");
}

void NdisDriverGetHwStats(void)
{
    struct e1000_hw* hw = &Priv.adapter.hw;

    AdapterSS.MssFS += er32(TPT);
    AdapterSS.MssFR += er32(TPR);
    AdapterSS.MssFSByt += er32(TOTL);
    AdapterSS.MssSFHW += er32(CEXTERR);
    AdapterSS.MssRFMin += er32(SYMERRS);
    AdapterSS.MssRFMax += er32(RLEC);
    AdapterSS.MssRFLack += er32(MPC);
    AdapterSS.MssRFCRC += er32(CRCERRS);
    AdapterSS.MssRFErr += er32(RXERRC);
    AdapterSS.MssFRMC += er32(MPRC);
    AdapterSS.MssFRBC += er32(BPRC);
    AdapterSS.MssFRByt += er32(TORL);
}

#ifdef DEBUG
void PrintState()
{
    pring_desc pr;
    struct e1000_hw* hw = &Priv.adapter.hw;

    pring_desc p = &(Priv.tx_ring[Priv.txpos]);

    DPRINTF(6, "txpos = %x\n", Priv.txpos);
    DPRINTF(6, "descr = %0lx%0lx : %0lx%0lx\n", p->buf_hi, p->buf_lo, p->flaglen_hi, p->flaglen_lo);
    DPRINTF(6, "TCTL = %0lx, TXDCTL = %0lx\n", er32(TCTL), er32(TXDCTL_BASE));
    DPRINTF(6, "TDBAL = %0lx, TDBAH = %0lx, TDLEN = %0lx\n", er32(TDBAL), er32(TDBAH), er32(TDLEN));
    DPRINTF(6, "TDH = %0lx, TDT = %0lx\n", er32(TDH), er32(TDT));
    pr = &(Priv.rx_ring[0]);
    DPRINTF(6, "rxpos = %x", Priv.rxpos);
    DPRINTF(6, "descr = %0lx%0lx : %0lx%0lx\n", pr->buf_hi, pr->buf_lo, pr->flaglen_hi, pr->flaglen_lo);
    DPRINTF(6, "RCTL = %0lx, RXDCTL = %0lx\n", er32(RCTL), er32(RXDCTL_BASE));
    DPRINTF(6, "RDBAL = %0lx, RDBAH = %0lx, RDLEN = %0lx\n", er32(RDBAL), er32(RDBAH), er32(RDLEN));
    DPRINTF(6, "RDH = %0lx, RDT = %0lx\n", er32(RDH), er32(RDT));
    DPRINTF(6, "IMS = %0lx\n", er32(IMS));
}
#endif

short NdisDriverStartXmit(struct TxBufDesc far *pDsc)
{
    //DPRINTF(5, "NIC_DEVICE::startXmit enter.\n");

    struct e1000_hw* hw = &Priv.adapter.hw;

    u16 fragments = pDsc->TxDataCount;
    u16 len = pDsc->TxImmedLen;
    u16 i;
    pring_desc p;
    u32 phys;
    fpu8 pPtr;

    DPRINTF(6, "TxDataCount=%d TxImmedLen=%d\n", fragments, len);

    DevCli();
    if (Priv.txbusy) {
        DevSti();
        DPRINTF(1, "DriverStartXmit: Reentered!\n");
        return OUT_OF_RESOURCE;
    }
    Priv.txbusy++;
    DevSti();

    for(i = 0 ; i < fragments; i++)
    {
        len += pDsc->TxDataBlk[i].TxDataLen;
    }

    if(len < ETH_HEADER_LEN || len > AdapterCC.CcSCp->MscMaxFrame)
    {
        DPRINTF(3, "error INVALID_PARAMETER\n");
        Priv.txbusy--;
        return INVALID_PARAMETER;
    }

    //PrintState();

    p = &(Priv.tx_ring[Priv.txpos]);

    phys = cpu_to_le32(GetTxBufferPhysicalAddress(Priv.pDev, Priv.txpos));

    DPRINTF(6, "txpos=%x TDH=%lx TDT=%lx\n", Priv.txpos, er32(TDH), er32(TDT));
    DPRINTF(6, "phys=%0lx\n", phys);

    p->buf_lo = phys;
    pPtr = GetTxBufferVirtualAddress(Priv.pDev, Priv.txpos);

    if (!pPtr)
    {
        Priv.txbusy--;
        return OUT_OF_RESOURCE;
    }

    if (pDsc->TxImmedLen)
    {
        memcpy(pPtr, pDsc->TxImmedPtr, pDsc->TxImmedLen);
        pPtr += pDsc->TxImmedLen;
    }

    for (i = 0 ; i < fragments; i++)
    {
        u16 ptrtype = pDsc->TxDataBlk[i].TxPtrType;
        u16 wLen = pDsc->TxDataBlk[i].TxDataLen;
        if (ptrtype == 2)
        {
            memcpy(pPtr, pDsc->TxDataBlk[i].TxDataPtr, wLen);
        }
        else
        {
            fptr pData;
            DevHelp_PhysToGDTSelector((u32)pDsc->TxDataBlk[i].TxDataPtr, wLen, GDT_READ);
            SELECTOROF(pData) = GDT_READ;
            OFFSETOF  (pData) = 0;
            memcpy(pPtr, pData, wLen);
        }
        pPtr += pDsc->TxDataBlk[i].TxDataLen;
    }

    p->flaglen_lo = E1000_TXD_CMD_RS | E1000_TXD_CMD_EOP | E1000_TXD_CMD_IFCS | len;
    p->flaglen_hi = 0;

    TraceArgs(0x0030, 8, (u16)Priv.txpos, (u16)len, (u32)p->flaglen_lo);
    TraceBuf(0x0031, len<64?len:64, GetTxBufferVirtualAddress(Priv.pDev, Priv.txpos));

    if(++Priv.txpos == Priv.tx_ring_size)
    {
        Priv.txpos = 0;
    }
    ew32(TDT, Priv.txpos);

    //DPRINTF(5, "NIC_DEVICE::startXmit exit.\n");

    Priv.txbusy--;
    return SUCCESS;
}

short NdisDriverReleaseRx(u16 pos)
{
    //DPRINTF(5, "NIC_DEVICE::releaseRX enter.\n");

    int res;

    TraceArgs(0x0013, 2, pos);

    if(pos < Priv.rx_ring_size)
    {
        Priv.rx_ring[pos].flaglen_hi = 0;
        res = SUCCESS;
    }
    else
    {
        res = INVALID_PARAMETER;
    }

    //DPRINTF(5, "NIC_DEVICE::releaseRX exit.\n");

    return res;
}

short NdisDriverXferRx(fpu16 pcopied, u16 frameOffset, struct TDBufDesc far * pd)
{
    fpu8 start;
    u16 copied;
    u16 i;

    //DPRINTF(5, "NIC_DEVICE::xferRX enter.\n");
    TraceArgs(0x0012, 0, NULL);

    if(!Priv.currentlen || !Priv.currentrxbuffer)
    {
        return GENERAL_FAILURE;
    }
    if(Priv.currentlen < frameOffset)
    {
        return INVALID_PARAMETER;
    } else if(Priv.currentlen == frameOffset)
    {
        return SUCCESS;
    }

    start = Priv.currentrxbuffer + frameOffset;
    Priv.currentlen -= frameOffset;

    copied = 0;
    for(i = 0; i < pd->TDDataCount && Priv.currentlen > 0; i++)
    {
        u16 ptrtype = pd->TDDataBlk[i].TDPtrType;
        u16 bufsize = pd->TDDataBlk[i].TDDataLen;
        u16 tocopy = min((u16)Priv.currentlen, (u16)bufsize);
        if(ptrtype == 2)
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

    //DPRINTF(5, "NIC_DEVICE::xferRX exit.\n");

    return SUCCESS;
}

/* stop the traffic and power down the device
 *
 * This function will get called from the strategy function on ACPI suspend.
 * It could also get called by the APM driver if it is installed.
 */
short DriverSuspend()
{
    if (Priv.Suspended) return 0; /* protection if we get called more than once */

    TraceBuf(0x0047, 0, NULL);
    //DPRINTF(5, "NIC_DEVICE::Suspend enter.\n");

    stopRX();
    stopTX();

    if(Priv.adapter.hw.phy.type == e1000_phy_igp_3)
        e1000e_igp3_phy_powerdown_workaround_ich8lan(&Priv.adapter.hw);

    //SavePciState(); // Not needed
    PciSetPowerStateD3hot((PPCI_DEVICEINFO)Priv.pDev);
    Priv.Suspended = 1;

    //DPRINTF(5, "NIC_DEVICE::Suspend exit.\n");

    return 0;
}

/* power on the device and restore as if powered off
 *
 * This function will get called from the strategy function on ACPI resume.
 * It could also get called by the APM driver if it is installed.
 */
short DriverResume()
{
    if (!Priv.Suspended) return 0; /* protection if we get called more than once */

    TraceBuf(0x0048, 0, NULL);
    //DPRINTF(5, "NIC_DEVICE::Resume enter.\n");

    //RestorePciState(); // Not needed
    PciSetPowerStateD0((PPCI_DEVICEINFO)Priv.pDev);
    e1000_irq_disable(&Priv.adapter);
    PciSetBusMaster((PPCI_DEVICEINFO)Priv.pDev);

        if (Priv.adapter.hw.mac.type == e1000_pch2lan)
                e1000_resume_workarounds_pchlan(&Priv.adapter.hw);

    e1000e_power_up_phy(&Priv.adapter);
    e1000e_reset(&Priv.adapter);

    e1000_configure(&Priv.adapter);
    Priv.txpos = Priv.rxpos = 0;
    startRX();
    startTX();

    Priv.Suspended = 0;

    //DPRINTF(5, "NIC_DEVICE::Resume exit.\n");

    return 0;
}

void DriverIOCtlGenMac(PREQPACKET pPacket) {
    ULONG far *pData;
    ULONG far *pParm;

    if (pPacket->ioctl.bFunction != GENMAC_WRAPPER_OID_GET) {
        pPacket->usStatus |= RPERR_PARAMETER;
        return;
    }
    if (pPacket->ioctl.usParmLen < (sizeof(ULONG)*3)) {
        pPacket->usStatus |= RPERR_PARAMETER;
        return;
    }
    if (pPacket->ioctl.usDataLen < sizeof(ULONG)) {
        pPacket->usStatus |= RPERR_PARAMETER;
        return;
    }

    pParm = (ULONG far *)pPacket->ioctl.pvParm;
    pData = (ULONG far *)pPacket->ioctl.pvData;

    switch (pParm[0]) {
    case OID_GEN_LINK_SPEED:
        *pData = AdapterSC.MscLinkSpd / 100;
        pParm[2] = 1;
        break;
    case OID_GEN_MEDIA_CONNECT_STATUS:
        {
            struct e1000_hw* hw = &Priv.adapter.hw;

            *pData = ((er32(STATUS) & E1000_STATUS_LU) == 0); /* return TRUE if link is disconnected */
            pParm[2] = 1;
        }
        break;
    default:
        pParm[2] = 0;
        *pData = 0;
        break;
    }
}

/**
 * e1000e_reset - bring the hardware into a known good state
 *
 * This function boots the hardware and enables some settings that
 * require a configuration cycle of the hardware - those cannot be
 * set/changed during runtime. After reset the device needs to be
 * properly configured for Rx, Tx etc.
 */
void e1000e_reset(struct e1000_adapter *adapter)
{
    struct e1000_mac_info *mac = &adapter->hw.mac;
    struct e1000_fc_info *fc = &adapter->hw.fc;
    struct e1000_hw *hw = &adapter->hw;
    u32 tx_space, min_tx_space, min_rx_space;
    u32 pba = adapter->pba;
    u16 hwm;

    /* reset Packet Buffer Allocation to default */
    ew32(PBA, pba);

    if (adapter->max_frame_size > ETH_FRAME_LEN + ETH_FCS_LEN) {
        /*
         * To maintain wire speed transmits, the Tx FIFO should be
         * large enough to accommodate two full transmit packets,
         * rounded up to the next 1KB and expressed in KB.  Likewise,
         * the Rx FIFO should be large enough to accommodate at least
         * one full receive packet and is similarly rounded up and
         * expressed in KB.
         */
        pba = er32(PBA);
        /* upper 16 bits has Tx packet buffer allocation size in KB */
        tx_space = pba >> 16;
        /* lower 16 bits has Rx packet buffer allocation size in KB */
        pba &= 0xffff;
        /*
         * the Tx fifo also stores 16 bytes of information about the tx
         * but don't include ethernet FCS because hardware appends it
         */
        min_tx_space = (adapter->max_frame_size +
                sizeof(struct e1000_tx_desc) -
                ETH_FCS_LEN) * 2;
        min_tx_space = ALIGN(min_tx_space, 1024);
        min_tx_space >>= 10;
        /* software strips receive CRC, so leave room for it */
        min_rx_space = adapter->max_frame_size;
        min_rx_space = ALIGN(min_rx_space, 1024);
        min_rx_space >>= 10;

        /*
         * If current Tx allocation is less than the min Tx FIFO size,
         * and the min Tx FIFO size is less than the current Rx FIFO
         * allocation, take space away from current Rx allocation
         */
        if ((tx_space < min_tx_space) &&
            ((min_tx_space - tx_space) < pba)) {
            pba -= min_tx_space - tx_space;

            /*
             * if short on Rx space, Rx wins and must trump tx
             * adjustment or use Early Receive if available
             */
            if ((pba < min_rx_space) &&
                (!(adapter->flags & FLAG_HAS_ERT)))
                /* ERT enabled in e1000_configure_rx */
                pba = min_rx_space;
        }

        ew32(PBA, pba);
    }


    /*
     * flow control settings
     *
     * The high water mark must be low enough to fit one full frame
     * (or the size used for early receive) above it in the Rx FIFO.
     * Set it to the lower of:
     * - 90% of the Rx FIFO size, and
     * - the full Rx FIFO size minus the early receive size (for parts
     *   with ERT support assuming ERT set to E1000_ERT_2048), or
     * - the full Rx FIFO size minus one full frame
     */
    if (adapter->flags & FLAG_DISABLE_FC_PAUSE_TIME)
        fc->pause_time = 0xFFFF;
    else
        fc->pause_time = E1000_FC_PAUSE_TIME;
    fc->send_xon = 1;
    fc->current_mode = fc->requested_mode;

    switch (hw->mac.type) {
    default:
        //hwm = (u16) min(div_u32(mul_u32(pba << 10, 9), 10),
        hwm = (u16) min((pba << 10) * 9 / 10,
              ((pba << 10) - adapter->max_frame_size));

        fc->high_water = hwm & E1000_FCRTH_RTH; /* 8-byte granularity */
        fc->low_water = fc->high_water - 8;
        break;
    case e1000_pchlan:
        /*
         * Workaround PCH LOM adapter hangs with certain network
         * loads.  If hangs persist, try disabling Tx flow control.
         */
        fc->high_water = 0x5000;
        fc->low_water  = 0x3000;
                fc->refresh_time = 0x1000;
        break;
        case e1000_pch2lan:
                fc->high_water = 0x05C20;
                fc->low_water = 0x05048;
                fc->pause_time = 0x0650;
                fc->refresh_time = 0x0400;
                break;
    }

    /* Allow time for pending master requests to run */
    mac->ops.reset_hw(hw);

    /*
     * For parts with AMT enabled, let the firmware know
     * that the network interface is in control
     */
    if (adapter->flags & FLAG_HAS_AMT)
        e1000_get_hw_control(adapter);

    ew32(WUC, 0);
    if (adapter->flags2 & FLAG2_HAS_PHY_WAKEUP)
        e1e_wphy(&adapter->hw, BM_WUC, 0);

    if (mac->ops.init_hw(hw))
        DPRINTF(1, "Hardware Error\n");

    /* additional part of the flow-control workaround above */
    if (hw->mac.type == e1000_pchlan)
        ew32(FCRTV_PCH, 0x1000);

    e1000e_reset_adaptive(hw);
    e1000_get_phy_info(hw);

    if ((adapter->flags & FLAG_HAS_SMART_POWER_DOWN) &&
        !(adapter->flags & FLAG_SMART_POWER_DOWN)) {
        u16 phy_data = 0;
        /*
         * speed up time to link by disabling smart power down, ignore
         * the return value of this function because there is nothing
         * different we would do if it failed
         */
        e1e_rphy(hw, IGP02E1000_PHY_POWER_MGMT, &phy_data);
        phy_data &= ~IGP02E1000_PM_SPD;
        e1e_wphy(hw, IGP02E1000_PHY_POWER_MGMT, phy_data);
    }
}

/**
 * e1000e_power_up_phy - restore link in case the phy was powered down
 * @adapter: address of board private structure
 *
 * The phy may be powered down to save power and turn off link when the
 * driver is unloaded and wake on lan is not enabled (among others)
 * *** this routine MUST be followed by a call to e1000e_reset ***
 **/
void e1000e_power_up_phy(struct e1000_adapter *adapter)
{
    if (adapter->hw.phy.ops.power_up)
        adapter->hw.phy.ops.power_up(&adapter->hw);

    adapter->hw.mac.ops.setup_link(&adapter->hw);
}

/**
 * Enable the driver and hardware for normal operation.
 *
 * Called by the NDIS driver (ring 0).
 */
short NdisDriverOpen(void)
{
    TraceBuf(0x0045, 0, NULL);

    /* ring size defaults */
    Priv.rx_ring_size = RX_RING_DEFAULT;
    Priv.tx_ring_size = TX_RING_DEFAULT;

    /* Setup input to AllocateRingsAndBuffers() */
    Priv.pDev->rx.RingCount = RX_RING_DEFAULT;
    Priv.pDev->rx.DescSize = sizeof(ring_desc);
    Priv.pDev->rx.AllocSize = E1000_RX_ALLOCATE;
    Priv.pDev->tx.RingCount = TX_RING_DEFAULT;
    Priv.pDev->tx.DescSize = sizeof(ring_desc);
    Priv.pDev->tx.AllocSize = E1000_TX_ALLOCATE;
    if (AllocateRingsAndBuffers(Priv.pDev)) return 1;

    Priv.rx_ring = Priv.pDev->rx.RingVirt;
    Priv.tx_ring = Priv.pDev->tx.RingVirt;

    if (initRing()) return 1;

    e1000e_power_up_phy(&Priv.adapter);

    /*
     * If AMT is enabled, let the firmware know that the network
     * interface is now open
     */
    if (Priv.adapter.flags & FLAG_HAS_AMT)
        e1000_get_hw_control(&Priv.adapter);

    /*
     * before we allocate an interrupt, we must be ready to handle it.
     * Setting DEBUG_SHIRQ in the kernel makes it fire an interrupt
     * as soon as we call pci_request_irq, so we have to setup our
     * clean_rx handler before we do so.
     */
    e1000_configure(&Priv.adapter);

    Priv.linkspeed = 0;
    startRX();
    startTX();

    TraceBuf(0x8045, 0, NULL);
    //DPRINTF(5, "NIC_DEVICE::open exit\n");

    return 0;
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

    p = GetConfigString(FindKey(pConfig, "MODE"));
    if (*p) {
        if(!stricmp(p, "100HALF")) {
            fixed_mode = LPA_100HALF;
            AdapterSC.MscLinkSpd = 100000000;
        } else if(!stricmp(p, "10HALF")) {
            fixed_mode = LPA_10HALF;
            AdapterSC.MscLinkSpd = 10000000;
        } else if(!stricmp(p, "1000FULL")) {
            fixed_mode = LPA_1000FULL;
            AdapterSC.MscLinkSpd = 1000000000;
        }   else if(!stricmp(p, "10FULL")) {
            fixed_mode = LPA_10FULL;
            AdapterSC.MscLinkSpd = 10000000;
        } else if(!stricmp(p, "100FULL")) {
            fixed_mode = LPA_100FULL;
            AdapterSC.MscLinkSpd = 100000000;
        } else {
            rc = 1;
        }
    }

    p = GetConfigString(FindKey(pConfig, "RXCHAIN"));
    if(*p) {
        if(!stricmp(p, "YES")) {
            dorxchain = 1;
        } else if(!stricmp(p, "NO")) {
            dorxchain = 0;
        } else {
            rc = 1;
        }
    }

    p = GetConfigString(FindKey(pConfig, "NETADDRESS"));
    if (*p) {
        if (GetHex(p, Priv.mac, 6) != 6) rc = 1;
        use_hw_netaddress = 0;
    }

    if (dorxchain) AdapterSC.MscService |= RECEIVECHAIN_MOSTLY;

    return(rc);
}

/**
 * Initialize the hardware
 *
 * Called at DevInit time.
 */
short DriverInitAdapter(struct net_device *pDev)
{
    int i;
    int err;

    TraceArgs(0x0050, 6, pDev->vendor, pDev->device, pDev->driver_data);
    DPRINTF(1, "PCI ID=%x:%x driver_data=%d\n", pDev->vendor, pDev->device, pDev->driver_data);

    Priv.pDev = pDev;
    i = pDev->driver_data;

    Priv.flags = pci_tbl[i].flags;
    sprintf(Priv.pDev->name, "Intel %s", pci_tbl[i].name);
    NdisLogMsg(MSG_HWDET, 1, 1, (fpchar)Priv.pDev->name);

    Priv.currentrxbuffer = NULL;
    Priv.currentlen = 0;

    AdapterSC.MscInterrupt = Priv.pDev->irq;
    AdapterSC.MscTBufCap = 1514 * TX_RING_DEFAULT;
    AdapterSC.MscRBufCap = 1514 * RX_RING_DEFAULT;
    AdapterSC.MscVenAdaptDesc = DNAME; /* Description for user info */

    Priv.vlanctl_bits = 0;
    Priv.rxstarted = 0;
    Priv.Suspended = 0;

    Priv.ba = 0;
    for(i = 0; i < 6; i++)
    {
        if (pDev->bars[i].bar && !pDev->bars[i].io && !pDev->bars[i].type && pDev->bars[i].size >= E1000_PCI_REGISTER_SIZE)
        {
            u16 size;
            fptr addr;

            size = min(pDev->bars[i].size, 0xF000);
            addr = MapPhysToVirt(pDev->bars[i].start, size);
            if (!addr) continue;
            Priv.ba = (fpu8)addr;
            break;
        }
    }
    if(!Priv.ba)
    {
        DPRINTF(1, "Cannot find base address.\n");
        return -1;
    }

    Priv.adapter.ei = e1000_info_tbl[Priv.flags];
    Priv.adapter.pba = Priv.adapter.ei->pba;
    Priv.adapter.flags = Priv.adapter.ei->flags;
    Priv.adapter.flags2 = Priv.adapter.ei->flags2;
    Priv.adapter.hw.adapter = &Priv.adapter;
    Priv.adapter.hw.mac.type = Priv.adapter.ei->mac;
    Priv.adapter.max_hw_frame_size = Priv.adapter.ei->max_hw_frame_size;
    Priv.adapter.hw.hw_addr = Priv.ba;
    Priv.adapter.pci_dev_id = pDev->device;
    TraceArgs(0x0051, 8, (u32)Priv.ba, (u16)Priv.flags, (u16)Priv.adapter.ei->mac);
    DPRINTF(1, "ba=%lx flags=%x mac=%x\n", (u32)Priv.ba, (u16)Priv.flags, (u16)Priv.adapter.ei->mac);

    if(Priv.adapter.flags & FLAG_HAS_FLASH)
    {
        u16 size = min((u16)pDev->bars[i+1].size, 0xF000);
        fptr addr = MapPhysToVirt(pDev->bars[i+1].start, size);
        if (!addr)
        {
            DPRINTF(1, "ERROR: Cannot map FLASH base address.\n");
            return -1;
        }
        Priv.adapter.hw.flash_address = (fpu8)addr;

        TraceArgs(0x0052, 4, Priv.adapter.hw.flash_address);
        DPRINTF(1, "flash=%lx\n", Priv.adapter.hw.flash_address);
    }

    set_default_params(&Priv.adapter);

    TraceBuf(0x0053, 0, NULL);
    err = e1000_sw_init(&Priv.adapter);
    if(err)
    {
        TraceArgs(0x0054, 2, (u16)err);
        DPRINTF(1, "e1000_sw_init error=%x\n", err);
        return err;
    }

    memcpy(&Priv.adapter.hw.mac.ops, Priv.adapter.ei->mac_ops, sizeof(Priv.adapter.hw.mac.ops));
    memcpy(&Priv.adapter.hw.nvm.ops, Priv.adapter.ei->nvm_ops, sizeof(Priv.adapter.hw.nvm.ops));
    memcpy(&Priv.adapter.hw.phy.ops, Priv.adapter.ei->phy_ops, sizeof(Priv.adapter.hw.phy.ops));

    TraceBuf(0x0055, 0, NULL);
    err = (int) Priv.adapter.ei->get_variants(&Priv.adapter);
    if(err)
    {
        DPRINTF(1, "get_variants error=%x\n", err);
        return err;
    }

    if ((Priv.adapter.flags & FLAG_IS_ICH) &&
        (Priv.adapter.flags & FLAG_READ_ONLY_NVM))
        e1000e_write_protect_nvm_ich8lan(&Priv.adapter.hw);

    TraceBuf(0x0056, 0, NULL);
    Priv.adapter.hw.mac.ops.get_bus_info(&Priv.adapter.hw);

    Priv.adapter.hw.phy.autoneg_wait_to_complete = 0;

    /* Copper options */
    if (Priv.adapter.hw.phy.media_type == e1000_media_type_copper) {
        Priv.adapter.hw.phy.mdix = AUTO_ALL_MODES;
        Priv.adapter.hw.phy.disable_polarity_correction = 0;
        Priv.adapter.hw.phy.ms_type = e1000_ms_hw_default;
    }

    TraceBuf(0x0057, 0, NULL);
    if (e1000_check_reset_block(&Priv.adapter.hw))
        DPRINTF(2, "PHY reset is blocked due to SOL/IDER session.\n");

    if (e1000e_enable_mng_pass_thru(&Priv.adapter.hw))
        Priv.adapter.flags |= FLAG_MNG_PT_ENABLED;

    /*
     * before reading the NVM, reset the controller to
     * put the device in a known good starting state
     */
    TraceBuf(0x0058, 0, NULL);
    Priv.adapter.hw.mac.ops.reset_hw(&Priv.adapter.hw);

    /*
     * systems with ASPM and others may see the checksum fail on the first
     * attempt. Let's give it a few tries
     */
    for(i = 0; ; i++) {
        if (e1000_validate_nvm_checksum(&Priv.adapter.hw) >= 0)
            break;
        if (i == 2) {
            DPRINTF(1, "The NVM Checksum Is Not Valid\n");
            return -1;
        }
    }

    TraceBuf(0x0059, 0, NULL);
    e1000_eeprom_checks(&Priv.adapter);

    if (use_hw_netaddress) {
        /* copy the MAC address out of the NVM */
        if (e1000e_read_mac_addr(&Priv.adapter.hw))
        {
            DPRINTF(1, "NVM Read Error while reading MAC address\n");
            return -1;
        }

        memcpy(Priv.mac, Priv.adapter.hw.mac.addr, sizeof Priv.mac);
    }

    TraceBuf(0x0044, 6, Priv.mac);
    DPRINTF(3, "MAC address = %x:%x:%x:%x:%x:%x\n", Priv.mac[0], Priv.mac[1], Priv.mac[2], Priv.mac[3], Priv.mac[4], Priv.mac[5]);

    for(i = 0; i < 6; i++)
    {
        AdapterSC.MscPermStnAdr[i] = Priv.mac[i];
        AdapterSC.MscCurrStnAdr[i] = Priv.mac[i];
    }

    /* Initialize link parameters. User can change them with ethtool */
    if(!fixed_mode)
    {
        Priv.adapter.hw.mac.autoneg = 1;
        Priv.adapter.fc_autoneg = 1;
        Priv.adapter.hw.fc.requested_mode = e1000_fc_default;
        Priv.adapter.hw.fc.current_mode = e1000_fc_default;
        Priv.adapter.hw.phy.autoneg_advertised = 0x2f;
    }
    else
    {
        Priv.adapter.hw.mac.autoneg = 0;

        switch(fixed_mode)
        {
            case LPA_10HALF:
                Priv.adapter.hw.mac.forced_speed_duplex = ADVERTISE_10_HALF;
                break;

            case LPA_10FULL:
                Priv.adapter.hw.mac.forced_speed_duplex = ADVERTISE_10_FULL;
                break;

            case LPA_100HALF:
                Priv.adapter.hw.mac.forced_speed_duplex = ADVERTISE_100_HALF;
                break;

            case LPA_100FULL:
                Priv.adapter.hw.mac.forced_speed_duplex = ADVERTISE_100_FULL;
                break;

            case LPA_1000FULL:
                Priv.adapter.hw.mac.forced_speed_duplex = ADVERTISE_1000_FULL;
        }
    }


    Priv.adapter.eeprom_wol = 0;
    Priv.adapter.wol = Priv.adapter.eeprom_wol;

    /* save off EEPROM version number */
    e1000_read_nvm(&Priv.adapter.hw, 5, 1, &Priv.adapter.eeprom_vers);

    DPRINTF(1,"e1000e_reset\n");
    /* reset the hardware with the new settings */
    e1000e_reset(&Priv.adapter);

    /*
     * If the controller has AMT, do not set DRV_LOAD until the interface
     * is up.  For all other cases, let the f/w know that the h/w is now
     * under the control of the driver.
     */
    if (!(Priv.adapter.flags & FLAG_HAS_AMT))
        e1000_get_hw_control(&Priv.adapter);

    TraceBuf(0x8050, 0, NULL);
    //DPRINTF(5, "NIC_DEVICE::setup exit\n");

    return 0;
}

