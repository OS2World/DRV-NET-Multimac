
#define spinlock_t unsigned
#define ktime_t unsigned
typedef struct {
        int counter;
} atomic_t;

#define dma_addr_t u64
////////////////linux/gfp.h
extern void __free_pages(struct page *page, unsigned int order);
extern void free_pages(unsigned long addr, unsigned int order);
extern void free_hot_cold_page(struct page *page, int cold);

#define __free_page(page) __free_pages((page), 0)
#define free_page(addr) free_pages((addr), 0)

#define IWL_INFO(a) DPRINTF(9,a)
#define IWL_ERROR(a) DPRINTF(1,a)
