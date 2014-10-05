#ifndef _SPIGOT_INCLUDED_
#define _SPIGOT_INCLUDED_

#include "dialect.h"
#include "ioctl.h"

struct device {
	struct device _far *parent;
	int i;
};

#ifdef CONFIG_NET_NS

static inline void write_pnet(struct net **pnet, struct net *net)
{
        *pnet = net;
}

static inline struct net *read_pnet(struct net * const *pnet)
{
        return *pnet;
}

#else

#define write_pnet(pnet, net)   do { (void)(net);} while (0)
#define read_pnet(pnet)         (&init_net)

#endif


#define rmb()
#define wmb()

#define ioread8(addr)           readb(addr)
#define ioread16(addr)          readw(addr)
#define ioread32(addr)          readl(addr)
#define iowrite8(v, addr)       writeb((v), (addr))
#define iowrite16(v, addr)      writew((v), (addr))
#define iowrite32(v, addr)      writel((v), (addr))
u8 readb(u8 far* reg);
void writeb(u8 val, u8 far* reg);
u16 readw(u8 far* reg);
void writew(u16 val, u8 far* reg);
u32 readl(u8 far* reg);
void udelay(u32 usecs);
void mdelay(u32 msecs);
void msleep(u32 msecs);
void writel(u32 val, u8 far* reg);


///////////////////////asm-generic/errno.h
#define ETIMEDOUT       110     /* Connection timed out */
#define EOPNOTSUPP      95      /* Operation not supported on transport endpoint */
#define EINVAL          22      /* Invalid argument */
#define ENOMEM          12      /* Out of memory */
#define EIO              5      /* I/O error */

///////////////////////asm-generic/errno-base.h
#define EBUSY           16      /* Device or resource busy */
#define EAGAIN          11      /* Try again */


static inline void *
dma_alloc_coherent(struct device *dev, size_t size, dma_addr_t *dma_handle,
                 gfp_t gfp);


static inline int test_and_clear_bit(int nr, volatile unsigned long *addr)
{
        unsigned long mask = BIT_MASK(nr);
        unsigned long *p = ((unsigned long *)addr) + BIT_WORD(nr);
        unsigned long old;
        unsigned long flags;

///!!!///        _atomic_spin_lock_irqsave(p, flags);
        old = *p;
        *p = old & ~mask;
///!!!///        _atomic_spin_unlock_irqrestore(p, flags);

        return (old & mask) != 0;
}

/**
 * test_and_change_bit - Change a bit and return its old value
 * @nr: Bit to change
 * @addr: Address to count from
 *
 * This operation is atomic and cannot be reordered.
 * It also implies a memory barrier.
 */
static inline int test_and_change_bit(int nr, volatile unsigned long *addr)
{
        unsigned long mask = BIT_MASK(nr);
        unsigned long *p = ((unsigned long *)addr) + BIT_WORD(nr);
        unsigned long old;
        unsigned long flags;
 
///!!!///        _atomic_spin_lock_irqsave(p, flags);
        old = *p;
        *p = old ^ mask;
///!!!///        _atomic_spin_unlock_irqrestore(p, flags);

        return (old & mask) != 0;
}

/**
 * test_and_set_bit - Set a bit and return its old value
 * @nr: Bit to set
 * @addr: Address to count from
 *
 * This operation is atomic and cannot be reordered.
 * It may be reordered on other architectures than x86.
 * It also implies a memory barrier.
 */
static inline int test_and_set_bit(int nr, volatile unsigned long *addr)
{
        unsigned long mask = BIT_MASK(nr);
        unsigned long *p = ((unsigned long *)addr) + BIT_WORD(nr);
        unsigned long old;
        unsigned long flags;

///!!!///        _atomic_spin_lock_irqsave(p, flags);
        old = *p;
        *p = old | mask;
///!!!///        _atomic_spin_unlock_irqrestore(p, flags);

        return (old & mask) != 0;
}

static inline int atomic_add_return(int i, atomic_t *v)
{
        unsigned long flags;
        int temp;

///!!!///        raw_local_irq_save(flags); /* Don't trace it in an irqsoff handler */
        temp = v->counter;
        temp += i;
        v->counter = temp;
///!!!///        raw_local_irq_restore(flags);

        return temp;
}

/**
 * atomic_sub_return - subtract integer from atomic variable
 * @i: integer value to subtract
 * @v: pointer of type atomic_t
 *
 * Atomically subtracts @i from @v and returns the result
 */
static inline int atomic_sub_return(int i, atomic_t *v)
{
        unsigned long flags;
        int temp;

///!!!///        raw_local_irq_save(flags); /* Don't trace it in an irqsoff handler */
        temp = v->counter;
        temp -= i;
        v->counter = temp;
///!!!///        raw_local_irq_restore(flags);

        return temp;
}

#define atomic_dec_return(v)            atomic_sub_return(1, (v))
#define atomic_inc_return(v)            atomic_add_return(1, (v))

#ifdef CONFIG_NEED_DMA_MAP_STATE
#define DEFINE_DMA_UNMAP_ADDR(ADDR_NAME)        dma_addr_t ADDR_NAME
#define DEFINE_DMA_UNMAP_LEN(LEN_NAME)          __u32 LEN_NAME
#define dma_unmap_addr(PTR, ADDR_NAME)           ((PTR)->ADDR_NAME)
#define dma_unmap_addr_set(PTR, ADDR_NAME, VAL)  (((PTR)->ADDR_NAME) = (VAL))
#define dma_unmap_len(PTR, LEN_NAME)             ((PTR)->LEN_NAME)
#define dma_unmap_len_set(PTR, LEN_NAME, VAL)    (((PTR)->LEN_NAME) = (VAL))
#else
#define DEFINE_DMA_UNMAP_ADDR(ADDR_NAME)
#define DEFINE_DMA_UNMAP_LEN(LEN_NAME)
#define dma_unmap_addr(PTR, ADDR_NAME)           (0)
#define dma_unmap_addr_set(PTR, ADDR_NAME, VAL)  do { } while (0)
#define dma_unmap_len(PTR, LEN_NAME)             (0)
#define dma_unmap_len_set(PTR, LEN_NAME, VAL)    do { } while (0)
#endif

/* This defines the direction arg to the DMA mapping routines. */
#define PCI_DMA_BIDIRECTIONAL   0
#define PCI_DMA_TODEVICE        1
#define PCI_DMA_FROMDEVICE      2
#define PCI_DMA_NONE            3

//////////////////////////kernel/time.h
#define MSEC_PER_SEC    1000L
#ifndef HZ
#define HZ 100
#endif
/* some arch's have a small-data section that can be accessed register-relative
 * but that can only take up to, say, 4-byte variables. jiffies being part of
 * an 8-byte variable may not be correctly accessed unless we force the issue
 */
#define __jiffy_data  __attribute__((section(".data")))
extern unsigned long volatile __jiffy_data jiffies;

unsigned long msecs_to_jiffies(const unsigned int m)
{
        /*
         * Negative value, means infinite timeout:
         */
        if ((int)m < 0)
                return MAX_JIFFY_OFFSET;

#if HZ <= MSEC_PER_SEC && !(MSEC_PER_SEC % HZ)
        /*
         * HZ is equal to or smaller than 1000, and 1000 is a nice
         * round multiple of HZ, divide with the factor between them,
         * but round upwards:
         */
        return (m + (MSEC_PER_SEC / HZ) - 1) / (MSEC_PER_SEC / HZ);
#elif HZ > MSEC_PER_SEC && !(HZ % MSEC_PER_SEC)
        /*
         * HZ is larger than 1000, and HZ is a nice round multiple of
         * 1000 - simply multiply with the factor between them.
         *
         * But first make sure the multiplication result cannot
         * overflow:
         */
        if (m > jiffies_to_msecs(MAX_JIFFY_OFFSET))
                return MAX_JIFFY_OFFSET;

        return m * (HZ / MSEC_PER_SEC);
#else
        /*
         * Generic case - multiply, round and divide. But first
         * check that if we are doing a net multiplication, that
         * we wouldn't overflow:
         */
        if (HZ > MSEC_PER_SEC && m > jiffies_to_msecs(MAX_JIFFY_OFFSET))
                return MAX_JIFFY_OFFSET;

        return (MSEC_TO_HZ_MUL32 * m + MSEC_TO_HZ_ADJ32)
                >> MSEC_TO_HZ_SHR32;
#endif
}

#endif
