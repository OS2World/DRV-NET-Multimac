/* Linux compatibility things */

#define __LITTLE_ENDIAN
#define PCI_SUBSYSTEM_VENDOR_ID 0x2c
#define PCI_SUBSYSTEM_ID        0x2e
#define EOPNOTSUPP 1
#define EINVAL 1
#define ENOMEM 1
#define EBUSY 1
#define EIO 1
#ifndef HZ
#define HZ 100
#endif
#define PCI_CACHE_LINE_SIZE	0x0c	/* 8 bits */
#define PCI_LATENCY_TIMER	0x0d	/* 8 bits */
#define VLAN_ETH_HLEN	18		/* Total octets in header.	 */
#ifndef NET_IP_ALIGN
#define NET_IP_ALIGN	2
#endif
/* This defines the direction arg to the DMA mapping routines. */
#define PCI_DMA_BIDIRECTIONAL	0
#define PCI_DMA_TODEVICE	1
#define PCI_DMA_FROMDEVICE	2
#define PCI_DMA_NONE		3

#define int long
#define KERN_WARNING
#define KERN_ERR
#define PFX
#define GFP_KERNEL 0

#define msleep_interruptible msleep

#define IFNAMSIZ 16
#define PAGE_SHIFT 12

/* Standard interface flags (netdevice->flags). */
#define	IFF_UP		0x1		/* interface is up		*/
#define	IFF_BROADCAST	0x2		/* broadcast address valid	*/
/* DAZ changed from 0x100 to 0x04 to be compatible with NDIS bit
 * This is the only bit we use in this flags field */
#define	IFF_PROMISC	0x04    /* receive all packets		*/
//#define	IFF_DEBUG	0x4		/* turn on debugging		*/
#define	IFF_LOOPBACK	0x8		/* is a loopback net		*/
#define	IFF_POINTOPOINT	0x10		/* interface is has p-p link	*/
#define	IFF_NOTRAILERS	0x20		/* avoid use of trailers	*/
#define	IFF_RUNNING	0x40		/* interface RFC2863 OPER_UP	*/
#define	IFF_NOARP	0x80		/* no ARP protocol		*/
#define	IFF_ALLMULTI	0x200		/* receive all multicast packets*/

#define IFF_MASTER	0x400		/* master of a load balancer 	*/
#define IFF_SLAVE	0x800		/* slave of a load balancer	*/

#define IFF_MULTICAST	0x1000		/* Supports multicast		*/

#define IFF_PORTSEL	0x2000          /* can set media type		*/
#define IFF_AUTOMEDIA	0x4000		/* auto media select active	*/
#define IFF_DYNAMIC	0x8000		/* dialup device with changing addresses*/

#define IFF_LOWER_UP	0x10000		/* driver signals L1 up		*/
#define IFF_DORMANT	0x20000		/* driver signals dormant	*/

#define IFF_ECHO	0x40000		/* echo sent packets		*/

#define IFF_VOLATILE	(IFF_LOOPBACK|IFF_POINTOPOINT|IFF_BROADCAST|IFF_ECHO|\
		IFF_MASTER|IFF_SLAVE|IFF_RUNNING|IFF_LOWER_UP|IFF_DORMANT)

/* Indicates what features are advertised by the interface. */
#define ADVERTISED_10baseT_Half	(1L << 0)
#define ADVERTISED_10baseT_Full	(1L << 1)
#define ADVERTISED_100baseT_Half	(1L << 2)
#define ADVERTISED_100baseT_Full	(1L << 3)
#define ADVERTISED_1000baseT_Half	(1L << 4)
#define ADVERTISED_1000baseT_Full	(1L << 5)
#define ADVERTISED_Autoneg		(1L << 6)
#define ADVERTISED_TP			(1L << 7)
#define ADVERTISED_AUI			(1L << 8)
#define ADVERTISED_MII			(1L << 9)
#define ADVERTISED_FIBRE		(1L << 10)
#define ADVERTISED_BNC			(1L << 11)
#define ADVERTISED_10000baseT_Full	(1L << 12)
#define ADVERTISED_Pause		(1L << 13)
#define ADVERTISED_Asym_Pause		(1L << 14)
#define ADVERTISED_2500baseX_Full	(1L << 15)
#define ADVERTISED_Backplane		(1L << 16)
#define ADVERTISED_1000baseKX_Full	(1L << 17)
#define ADVERTISED_10000baseKX4_Full	(1L << 18)
#define ADVERTISED_10000baseKR_Full	(1L << 19)
#define ADVERTISED_10000baseR_FEC	(1L << 20)

/**
 * ethtool_adv_to_mii_adv_t
 * @ethadv: the ethtool advertisement settings
 *
 * A small helper function that translates ethtool advertisement
 * settings to phy autonegotiation advertisements for the
 * MII_ADVERTISE register.
 */
static inline u32 ethtool_adv_to_mii_adv_t(u32 ethadv)
{
	u32 result = 0;

	if (ethadv & ADVERTISED_10baseT_Half)
		result |= ADVERTISE_10HALF;
	if (ethadv & ADVERTISED_10baseT_Full)
		result |= ADVERTISE_10FULL;
	if (ethadv & ADVERTISED_100baseT_Half)
		result |= ADVERTISE_100HALF;
	if (ethadv & ADVERTISED_100baseT_Full)
		result |= ADVERTISE_100FULL;
	if (ethadv & ADVERTISED_Pause)
		result |= ADVERTISE_PAUSE_CAP;
	if (ethadv & ADVERTISED_Asym_Pause)
		result |= ADVERTISE_PAUSE_ASYM;

	return result;
}

/**
 * ethtool_adv_to_mii_ctrl1000_t
 * @ethadv: the ethtool advertisement settings
 *
 * A small helper function that translates ethtool advertisement
 * settings to phy autonegotiation advertisements for the
 * MII_CTRL1000 register when in 1000T mode.
 */
static inline u32 ethtool_adv_to_mii_ctrl1000_t(u32 ethadv)
{
	u32 result = 0;

	if (ethadv & ADVERTISED_1000baseT_Half)
		result |= ADVERTISE_1000HALF;
	if (ethadv & ADVERTISED_1000baseT_Full)
		result |= ADVERTISE_1000FULL;

	return result;
}


struct firmware {
	size_t size;
	const u8 *data;
//	struct page **pages;
};

struct mii_if_info {
	int phy_id;
	int advertising;
	int phy_id_mask;
	int reg_num_mask;

	unsigned int full_duplex : 1;	/* is full duplex? */
	unsigned int force_media : 1;	/* is autoneg. disabled? */
	unsigned int supports_gmii : 1; /* are GMII registers supported? */

	struct net_device *dev;
	int (*mdio_read) (struct net_device *dev, int phy_id, int location);
	void (*mdio_write) (struct net_device *dev, int phy_id, int location, int val);
};

struct sk_buff {
    fptr data;
    u32 len;
    u16 ip_summed;
    u16 protocol;
};

struct pci_device_id {
    u16 vendor, device;     /* Vendor and device ID or PCI_ANY_ID*/
    u16 subvendor, subdevice;   /* Subsystem ID's or PCI_ANY_ID */
    u16 class_id, class_mask;   /* (class,subclass,prog-if) triplet */
    u16 driver_data;    /* Data private to the driver */
};

struct device {
    u32 x;
};

#define uninitialized_var(a) a
#define __releases(a)
#define __acquires(a)

/**
 * DEFINE_PCI_DEVICE_TABLE - macro used to describe a pci device table
 * @_table: device table name
 *
 * This macro is used to create a struct pci_device_id array (a device table)
 * in a generic manner.
 */
#define DEFINE_PCI_DEVICE_TABLE(_table) const struct pci_device_id _table[]

/**
 * PCI_DEVICE - macro used to describe a specific pci device
 * @vend: the 16 bit PCI Vendor ID
 * @dev: the 16 bit PCI Device ID
 *
 * This macro is used to create a struct pci_device_id that matches a
 * specific device.  The subvendor and subdevice fields will be set to
 * PCI_ANY_ID.
 */
#define PCI_DEVICE(vend,dev) (vend), (dev), PCI_ANY_ID, PCI_ANY_ID
#define DEFINE_DMA_UNMAP_ADDR(x) u32 x
#define PTR_ALIGN(a,b) a
#define NET_SKB_PAD 0

#define ____cacheline_aligned
#define __devinit
#define __devinitdata
#define __iomem	_far
#define __always_unused
typedef unsigned short spinlock_t;
typedef short netdev_tx_t;
typedef short irqreturn_t;
typedef u32 irq_handler_t;

struct timer_list {
    u32 expires;
};

#define BIT(nr) (1UL << (nr))
#define cpu_to_le16(x) (x)
#define le64_to_cpu(x) (x)
#define cpu_to_le64(x) (x)
//#define DMA_BIT_MASK(x) 0xFFFFFFFFL
#define DMA_BIT_MASK(n)	(((n) == 64) ? ~0ULL : ((1ULL<<(n))-1))

#define __attribute__(x)

/* Functions that do not need to be implemented */
#define IS_ERR(x) (!(x))
#define IS_ERR_OR_NULL(x) (!(x))
#define ASSERT_RTNL()
#define BUG_ON(x)
#define WARN_ON(x)
#define BUILD_BUG_ON(x)
#define napi_enable(a)
#define wmb()
#define rmb()
#define smp_rmb()
#define smp_wmb()
#define mmiowb()
#define pci_unmap_single(pdev,addr,rx_buf_sz,flags)
//#define skb_reserve(skb,align)

/* Functions that might need to be implemented */
extern u32 jiffies;
//int dev_kfree_skb(struct sk_buff *);
//struct sk_buff *netdev_alloc_skb(struct net_device *pDev, u32 size);
//u32 pci_map_single(struct pci_dev *pdev, fptr data, u16 rx_buf_sz, u16 flags);

#ifdef DEBUG
#define BUG() DevInt3()
#else
#define BUG()
#endif

#define strlcpy(a,b,c) memcpy(a,b,c)
#define netif_running(x) x
#define pm_request_resume(a)
#define pm_schedule_suspend(a,b)
#define netif_carrier_on(a)
#define netif_carrier_off(a)
#define netif_info(a,b,c,d,...) DPRINTF(1, d, ##__VA_ARGS__)
#define net_ratelimit() 1
#define netif_warn(a,b,c,d,...) DPRINTF(1, d, ##__VA_ARGS__)
#define netif_err(tp,link,dev,str,...) DPRINTF(1, str, ##__VA_ARGS__)
#define netif_start_queue(a)
#define netif_notice(a,b,c,d) DPRINTF(1, d)
#define dev_err(a,b) DPRINTF(1, b)
#define pr_cont(a,...) DPRINTF(1, a, ##__VA_ARGS__)
#define pr_err(a,...) DPRINTF(1, a, ##__VA_ARGS__)

#define pci_alloc_consistent(a,b,c) (0)
#define pci_free_consistent(a,b,c,d)

#define netdev_info(a,b,...) DPRINTF(1, b, ##__VA_ARGS__)
#define netdev_err(a,b,...) DPRINTF(1, b, ##__VA_ARGS__)
#define netdev_mc_count(a) (0)
#define netdev_for_each_mc_addr(ha,dev) for (ha=&dev->mc[0]; ha < &dev->mc[2]; ha++)

/* functions in l_compat.c */
#define netdev_priv(pdev) (pdev->private_data)
#define pci_get_drvdata(pdev) (struct net_device *)(pdev)
#define pci_set_master(pdev) PciSetBusMaster((PPCI_DEVICEINFO)pdev)
#define pci_find_capability(pdev,val) PciFindCaps()

#define unlikely(x) (x)

/**
 * pci_pcie_cap - get the saved PCIe capability offset
 * @dev: PCI device
 *
 * PCIe capability offset is calculated at PCI device initialization
 * time and saved in the data structure. This function returns saved
 * PCIe capability offset. Using this instead of pci_find_capability()
 * reduces unnecessary search in the PCI configuration space. If you
 * need to calculate PCIe capability offset from raw device for some
 * reasons, please use pci_find_capability() instead.
 */
static inline int pci_pcie_cap(struct pci_dev *dev)
{
	return dev->pcie_cap;
}

/**
 * pci_is_pcie - check if the PCI device is PCI Express capable
 * @dev: PCI device
 *
 * Retrun true if the PCI device is PCI Express capable, false otherwise.
 */
static inline bool pci_is_pcie(struct pci_dev *dev)
{
	return !!pci_pcie_cap(dev);
}

/**
 * pci_pcie_type - get the PCIe device/port type
 * @dev: PCI device
 */
static inline int pci_pcie_type(const struct pci_dev *dev)
{
	return (dev->pcie_flags_reg & PCI_EXP_FLAGS_TYPE) >> 4;
}

static inline int pcie_cap_version(const struct pci_dev *dev)
{
	return dev->pcie_flags_reg & PCI_EXP_FLAGS_VERS;
}

static inline bool pcie_cap_has_devctl(const struct pci_dev *dev)
{
	return true;
}

static inline bool pcie_cap_has_lnkctl(const struct pci_dev *dev)
{
	int type = pci_pcie_type(dev);

	return pcie_cap_version(dev) > 1 ||
	       type == PCI_EXP_TYPE_ROOT_PORT ||
	       type == PCI_EXP_TYPE_ENDPOINT ||
	       type == PCI_EXP_TYPE_LEG_END;
}

static inline bool pcie_cap_has_sltctl(const struct pci_dev *dev)
{
	int type = pci_pcie_type(dev);

	return pcie_cap_version(dev) > 1 ||
	       type == PCI_EXP_TYPE_ROOT_PORT ||
	       (type == PCI_EXP_TYPE_DOWNSTREAM &&
		dev->pcie_flags_reg & PCI_EXP_FLAGS_SLOT);
}

static inline bool pcie_cap_has_rtctl(const struct pci_dev *dev)
{
	int type = pci_pcie_type(dev);

	return pcie_cap_version(dev) > 1 ||
	       type == PCI_EXP_TYPE_ROOT_PORT ||
	       type == PCI_EXP_TYPE_RC_EC;
}

int pcie_capability_clear_and_set_word(struct pci_dev *dev, int pos, u16 clear, u16 set);

static inline int pcie_capability_clear_word(struct pci_dev *dev, int pos, u16 clear)
{
	return pcie_capability_clear_and_set_word(dev, pos, clear, 0);
}

static inline int pcie_capability_set_word(struct pci_dev *dev, int pos, u16 set)
{
	return pcie_capability_clear_and_set_word(dev, pos, 0, set);
}

void pci_write_config_byte(struct pci_dev *pdev, u16 reg, u8 val);
void pci_read_config_byte(struct pci_dev *pdev, u16 reg, u8 far *pval);
int pci_write_config_word(struct pci_dev *pdev, u16 reg, u16 val);
int pci_read_config_word(struct pci_dev *pdev, u16 reg, u16 far *pval);
short request_firmware(const struct firmware **fw, const char *Name, struct net_device *dev);
void release_firmware(const struct firmware *fw);
fptr kmalloc(u32 AllocSize, u16 Unused);
void kfree(fptr Vadr);
fptr kzalloc(u32 AllocSize, u16 Unused);

//int mod_timer(struct timer_list *timer, unsigned long expires);
#define mod_timer(a,b)
int del_timer_sync(struct timer_list *timer);

#define spin_lock_irqsave(a,b) mutex_lock(a)
#define spin_unlock_irqrestore(a,b) mutex_unlock(a)
#define spin_lock_init(a)
#define mutex_init(a)
#define spin_lock_irq(a) mutex_lock(a)
#define spin_lock(a) mutex_lock(a)
#define spin_unlock_irq(a) mutex_unlock(a)
#define spin_unlock(a) mutex_unlock(a)

