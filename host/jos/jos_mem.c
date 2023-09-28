/* (c) 2012 Jungo Ltd. All Rights Reserved. Jungo Confidential */
#include "jos_internal.h"
#include "driver_beken_includes.h"

//#define CONFIG_MEMPOOL_OV_DEBUG

#ifndef CONFIG_MEMPOOL_ALIGN
#define CONFIG_MEMPOOL_ALIGN (sizeof(void *))
#endif

#ifdef JOS_MALLOC
typedef struct memory_info_s {
    uint32_t                    act_size;

#ifdef DEBUG_WILD_POINTER
    struct memory_info_s        *next, *prev;
#endif
} memory_info_t;

#endif /* JOS_MALLOC */

#define ALLOC_OVERHEAD (sizeof(memory_info_t))

#define J_MEM_ASSERT_MAGIC(m)          do {} while (0)
#ifdef CONFIG_MEMORY_USAGE_SHOW
#define J_MEM_DBG_INFO_FILL(m, s)     (m)->user_size = (s)
#else
#define J_MEM_DBG_INFO_FILL(m, s)      do {} while (0)
#endif


#ifdef CONFIG_MEMPOOL_OV_DEBUG

#define MEMPOOL_OV_DEBUG_LIST_LEN 256
typedef struct{
    uint32_t idx;
    uint32_t addr;
    uint16_t size;
}memov_debug_t;

extern uint32_t _player_decoder_end;

memov_debug_t *memov_debug_list = (memov_debug_t*)_player_decoder_end;

static uint16_t memov_debug_current_idx = 0;
static uint32_t memov_debug_num = 0;

void memov_debug_print(void)
{
    int i;
    for(i=0; i<MEMPOOL_OV_DEBUG_LIST_LEN; i++)
    {
        os_printf("%d:%d,0x%x,%d\r\n",i,memov_debug_list[i].idx,memov_debug_list[i].addr,memov_debug_list[i].size);
    }
}

static void memov_debug_add(uint32_t addr,uint16_t size)
{
    int i;
    
    if(memov_debug_num%10 == 0) 
        os_printf("ov:%d\r\n",memov_debug_num);
    
    for(i = 0; i<MEMPOOL_OV_DEBUG_LIST_LEN; i++)
    {
        if(memov_debug_list[memov_debug_current_idx].addr == 0)
        {
            memov_debug_list[memov_debug_current_idx].idx = memov_debug_num++;
            memov_debug_list[memov_debug_current_idx].addr = addr;
            memov_debug_list[memov_debug_current_idx].size = size;
            memov_debug_current_idx++;
            if(memov_debug_current_idx>=MEMPOOL_OV_DEBUG_LIST_LEN) 
                memov_debug_current_idx = 0;
            break;
        }
        memov_debug_current_idx++;
        if(memov_debug_current_idx>=MEMPOOL_OV_DEBUG_LIST_LEN) 
            memov_debug_current_idx = 0;
    }
    if(i>=MEMPOOL_OV_DEBUG_LIST_LEN)
    {
        os_printf("memov_debug_list full!");
        memov_debug_print();
        while(1);
    }
}

static void memov_debug_remove(uint32_t addr)
{
    int i;
    for(i=0; i<MEMPOOL_OV_DEBUG_LIST_LEN; i++)
    {
        if(memov_debug_list[i].addr == addr)
        {
            memov_debug_list[i].addr = 0;
            break;
        }
    }
}

void memov_debug_reset(void)
{
    j_memset(memov_debug_list,0x00,sizeof(memov_debug_list));
    memov_debug_current_idx = 0;
    memov_debug_num = 0;
}
#endif

/*
 * Statically allocated memory management
 *
 * Automaticaly allocate fragments from a large buffer passed during
 * initialization of the stack. Used instead of having to call the OS's
 * os_alloc/os_free/os_dma_alloc/etc. */

#ifdef CONFIG_MEMPOOL
/* Mempool segments operations */
#define INUSE_BIT      (1UL << 31) /* Current segment inuse */
#define PREV_INUSE_BIT (1UL << 30) /* Previous segment inuse */
#define MP_SIZE_BM     (~(INUSE_BIT | PREV_INUSE_BIT))
/* Segment size setter & getter */
#define J_MP_SEG_GET_SIZE(m)           ((m)->act_size & MP_SIZE_BM)
#define J_MP_SEG_SET_SIZE(m, s)                                        \
    do {                                                                \
        uint32_t _size = (s);                                           \
        (m)->act_size &= ~MP_SIZE_BM;                                   \
        (m)->act_size |= _size;                                         \
    } while (0)
/* Is current segment in use? */
#define J_MP_SEG_IS_INUSE(m)           ((m)->act_size & INUSE_BIT)
/* Is adjoined previous segment in use? */
#define J_MP_SEG_IS_PREV_INUSE(m)      ((m)->act_size & PREV_INUSE_BIT)
/* Is adjoined next segment in use? */
#define J_MP_SEG_IS_NEXT_INUSE(m)                                      \
    J_MP_SEG_IS_INUSE((memory_info_t *)((uint8_t *)(m) +                \
        J_MP_SEG_GET_SIZE((m))))
/* Last four bytes in segment */
#define J_MP_SEG_END(m) \
    (*((memory_info_t **)((uint8_t *)(m) + J_MP_SEG_GET_SIZE(m) -       \
        sizeof(memory_info_t *))))
        
/* Last four bytes in previous segment */
#define J_MP_SEG_PREV(m) \
    (*(memory_info_t **)((uint8_t *)(m) - sizeof(memory_info_t *)))

/* Mark current segment as used */
#define J_MP_SEG_SET_USED(m)                                           \
    do {                                                                \
        memory_info_t *n = ((memory_info_t *)((uint8_t *)(m) +          \
            J_MP_SEG_GET_SIZE(m)));                                     \
        (m)->act_size |= INUSE_BIT;                                     \
        n->act_size |= PREV_INUSE_BIT;                                  \
    } while (0)
/* Mark current segment as free */
#define J_MP_SEG_SET_FREE(m)                                           \
    do {                                                                \
        memory_info_t *n = ((memory_info_t *)((uint8_t *)(m) +          \
            J_MP_SEG_GET_SIZE(m)));                                     \
        (m)->act_size &= ~INUSE_BIT;                                    \
        J_MP_SEG_END(m) = (m);                                          \
        (n)->act_size &= ~PREV_INUSE_BIT;                               \
    } while (0)

typedef enum {
    POOL_VIRTUAL,
#ifdef CONFIG_POOL_DMA
    POOL_DMA,
#  ifdef CONFIG_POOL_DMA_CACHABLE
    POOL_DMA_CACHABLE,
#  endif
#endif /* CONFIG_POOL_DMA */
    POOL_LAST
} pool_types;

typedef struct {
    uint32_t offset;
    uint8_t *vbuf;
    uint8_t *pbuf;
    uint32_t size;
} j_mpool_t;

static j_mpool_t pools[POOL_LAST];

/* Address of next un-allocated element in pool */
#define J_MPOOL_NEXT_FREE(p) \
    (&((pools[(p)].vbuf)[pools[(p)].offset]))

#define J_MPOOL_ASSERT_IN_RANGE(m) do {} while (0)

#ifdef CONFIG_MEMORY_USAGE_SHOW
/* Monitor peak pool usage */
static mu_stat_t mu_max_pool_usage = {0, 0};
#endif /* CONFIG_MEMORY_USAGE_SHOW */

static void pool_init(pool_types type, void *vstart, void *pstart,
    uint32_t size)
{
    uint32_t offset;
    j_mpool_t *p = &pools[type];
    memory_info_t *head;
    DECLARE_FNAME("pool_init");

    DBG_I(DJOS_MEM, ("%s: type %d size %lu [%p:%p]\n", FNAME, type, size,
        vstart, pstart));

    /* Align starting offset */
    offset = (uint32_t)((uint8_t *)vstart) % CONFIG_MEMPOOL_ALIGN;
    p->offset = offset ? CONFIG_MEMPOOL_ALIGN - offset : 0;

    p->size = size;
    p->vbuf = (uint8_t *)vstart;
    p->pbuf = (uint8_t *)pstart;

    /* Set first item's prev bit as used to avoid attempt to merge it */
    head = (memory_info_t *)J_MPOOL_NEXT_FREE(type);
    head->act_size = PREV_INUSE_BIT;

    DBG_V(DJOS_MEM, ("%s: Pool %d initialized\n", FNAME, type));
}

static result_t pool_alloc(uint32_t *size, void **vaddr, void **paddr,
    pool_types type)
{
    j_mpool_t *p = &pools[type];
    DECLARE_FNAME("pool_alloc");

    DBG_V(DJOS_MEM, ("%s: %lu from pool %d\n", FNAME, *size, type));

    if (*size % CONFIG_MEMPOOL_ALIGN)
        *size = (*size + CONFIG_MEMPOOL_ALIGN) - (*size % CONFIG_MEMPOOL_ALIGN);

    if (p->offset + *size >= p->size)
    {
    	#ifdef BEKEN_DEBUG
		os_printf("Not enough memory on pool %d, need %lu have %lu\n", type, *size, p->size - p->offset);
		#endif
        DBG_I(DJOS_MEM, ("%s: Not enough memory on pool %d, need %lu have "
            "%lu\n", FNAME, type, *size, p->size - p->offset));
        return UWE_NOMEM;
    }

#ifdef CONFIG_MEMORY_USAGE_SHOW
    MU_INC_STAT(mu_max_pool_usage, *size);
#endif

    if (vaddr)
        *vaddr = (void *)&((p->vbuf)[p->offset]);

    if (paddr)
        *paddr = (void *)&((p->pbuf)[p->offset]);

    p->offset += *size;

    return UWE_OK;
}

static void pool_free(pool_types type, memory_info_t *item)
{
    DECLARE_FNAME("pool_free");

#ifdef CONFIG_MEMORY_USAGE_SHOW
    MU_DEC_STAT(mu_max_pool_usage, J_MP_SEG_GET_SIZE(item));
#endif

    pools[type].offset -= J_MP_SEG_GET_SIZE(item);

    DBG_V(DJOS_MEM, ("%s: Item %p (size %lu) returned to pool %d, new offset "
        "%lu\n", FNAME, item, J_MP_SEG_GET_SIZE(item), type,
        pools[type].offset));
}

/*
 * Implementation of Memory Pool allocation
 * ----------------------------------------
 * The code below allocates a single large block of memory from the
 * porting layer during the stack initialization and uses block lists to
 * support basic malloc/free
 *
 * */

/* Holds the head of all free segments */
static memory_info_t *mp_free_list = NULL;

/* Minimal user buffer when using mempool - minimum 12 bytes. Required for
 * maintaining the segment after call to jfree() */
#define MPOOL_MIN_USER_SIZE    12

/* Chain free items */
#ifdef DEBUG_WILD_POINTER
#define ITEM_NEXT(x)      (((memory_info_t *)x)->next)
#define ITEM_PREV(x)      (((memory_info_t *)x)->prev)
#else
#define ITEM_NEXT(x)        (*((void **)((memory_info_t *)(x) + 1)))
#define ITEM_PREV(x)        (*(void **)((uint32_t *)((memory_info_t *)(x) + 1) + 1))
#endif

/* When reusing big segments - split if remainder is at least
 * MP_MIN_SPLIT_SIZE */
#define MP_MIN_SPLIT_SIZE      (MPOOL_MIN_USER_SIZE + ALLOC_OVERHEAD)

static memory_info_t *memp_item_get(uint32_t size);
uint32_t dbg_show_dynamic_mem_info(uint8_t en)
{
    int size = 0;
    int count = 0;
    int max_size = 0;
    uint32_t pool_left_size;    
    memory_info_t *mp = mp_free_list;
    
    max_size = 0;
    pool_left_size = pools[0].size - pools[0].offset;
    for(mp = mp_free_list; mp != NULL; mp = ITEM_NEXT(mp))
    {
        int act_size = (mp->act_size&0xffffff);
        
        if(act_size > max_size)
        {
            max_size = act_size;
        }
        
        size += act_size;
        count++;
    }
    
    if(en) os_printf("[malloc] left:%d, act:%d, act_max:%d\n", pool_left_size, size, max_size);

    return (size + pool_left_size);
}

uint32_t memory_usage_show(void)
{
    return dbg_show_dynamic_mem_info(0);
}

#ifdef BEKEN_DEBUG
uint32_t check_in_free_list(memory_info_t *item)
{
    memory_info_t *mp;
    memory_info_t *hdr = mp_free_list;

    if(0 == hdr)
    {
        return 1;
    }
    
    for(mp = hdr; mp != NULL; mp = ITEM_NEXT(mp))
    {
        if(mp == item)
        {
            return 1;
        }
    }

    return 0;
}

#endif

#ifdef J_DEBUG
/* Dump the contents of the Mempool free list */
static void mp_free_list_dump(void)
{
    memory_info_t *item;
    memory_info_t *next;
    int32_t inuse;
    int32_t n;

    jprintf(" Dumping free list (head %p) items:\n", mp_free_list);
    for (n = 0, item = mp_free_list; item; item = ITEM_NEXT(item), n++)
    {
        J_MPOOL_ASSERT_IN_RANGE(item);

        next = (memory_info_t *)((uint8_t *)item + J_MP_SEG_GET_SIZE(item));
        /* Check if next segment is the pool... */
        if ((uint8_t *)next >= J_MPOOL_NEXT_FREE(POOL_VIRTUAL))
            inuse = 2;
        else
            inuse = J_MP_SEG_IS_INUSE(next) ? 1 : 0;

        jprintf("  %p, act_size %lu (prev item %p, next segment %p, size %lu - "
            "%s)\n", item, J_MP_SEG_GET_SIZE(item), ITEM_PREV(item), next,
            J_MP_SEG_GET_SIZE(next), (inuse == 1) ? "INUSE" : (inuse == 2) ?
            "IN POOL" : "FREE");
    }
    jprintf(" Free list - %d elements\n", n);
}

/* Define the following in order to activate the assert_memory_free() */
#endif /* J_DEBUG */

static void mempool_init(void)
{
    /* Zero the list */
    mp_free_list = NULL;
}

void mempool_uninit(void)
{
}

static void mp_free_list_remove(memory_info_t *item)
{
    memory_info_t *prev = ITEM_PREV(item);
    memory_info_t *next = ITEM_NEXT(item);
    DECLARE_FNAME("mp_free_list_remove");

    J_MPOOL_ASSERT_IN_RANGE(item);

    if (prev)
    {
        J_MPOOL_ASSERT_IN_RANGE(prev);
        ITEM_NEXT(prev) = next;
    }
    else
    {
        mp_free_list = next;
    }
    if (next)
    {
        J_MPOOL_ASSERT_IN_RANGE(next);
        ITEM_PREV(next) = prev;
    }
}

static void mp_free_list_add(memory_info_t *item)
{
    memory_info_t **temp = &mp_free_list;
    DECLARE_FNAME("mp_free_list_add");

    DBG_V(DJOS_MEM, ("%s: Entered, item %p\n", FNAME, item));
    J_MPOOL_ASSERT_IN_RANGE(item);

    if (*temp)
        ITEM_PREV(*temp) = item;
    ITEM_NEXT(item) = *temp;
    ITEM_PREV(item) = NULL;
    *temp = item;
    /* Mark memory segment as freed */
    J_MP_SEG_SET_FREE(item);
    DBG_X(DJOS_MEM, ("%s: Item %p added to list\n", FNAME, item));
}

/* Merge adjoined memory segments, return the united segment */
static memory_info_t *mp_seg_merge(memory_info_t *prev, memory_info_t *next)
{
    /* Merge segments */
    J_MP_SEG_SET_SIZE(prev, (J_MP_SEG_GET_SIZE(prev) +
        J_MP_SEG_GET_SIZE(next)));
    J_MP_SEG_SET_FREE(prev);

    return prev;
}

static void *jmemp_alloc(uint32_t size, uint16_t flags)
{
    memory_info_t *item;
    DECLARE_FNAME("jmemp_alloc");

    LOCK_CHECK;
    KASSERT(size == (size & MP_SIZE_BM), ("%s: Requested size too big - %lu\n",
        FNAME, size));
    DBG_I(DJOS_MEM, ("%s: Entered, size %lu\n", FNAME, size));

    item = memp_item_get(size);
    if (!item)
        return NULL;

    /* Zero the user memory chunk */
    if (flags & M_ZERO)
        j_memset((item + 1), 0, size - sizeof(memory_info_t));

#ifdef J_ASSERT_MEMORY_FREE
    DBG_RUN(DJOS_MEM, DL_ERROR, assert_memory_free(item));
#endif

    J_MP_SEG_SET_USED(item);

    return item;
}

static void jmemp_free(memory_info_t *item)
{
    memory_info_t *next;
    BOOL item_in_list = 0;
    DECLARE_FNAME("jmemp_free");

    DBG_I(DJOS_MEM, ("%s: Entered, item %p, size %lu\n", FNAME, item,
        J_MP_SEG_GET_SIZE(item)));
    DBG_RUN(DJOS_MEM, DL_EX_VERBOSE, mp_free_list_dump());

    /* If possible - merge with adjoined previous segment */
    if (!J_MP_SEG_IS_PREV_INUSE(item))
    {
        item = mp_seg_merge(J_MP_SEG_PREV(item), item);
        item_in_list = 1;
    }

    next = (memory_info_t *)((uint8_t *)item + J_MP_SEG_GET_SIZE(item));
    /* If possible - merge with adjoined next segment */
    if (((uint8_t *)next != J_MPOOL_NEXT_FREE(POOL_VIRTUAL)) &&
        !J_MP_SEG_IS_NEXT_INUSE(item))
    {
        mp_free_list_remove(next);
        item = mp_seg_merge(item, next);
        /* Recalculate adjoined next */
        next = (memory_info_t *)((uint8_t *)item + J_MP_SEG_GET_SIZE(item));
    }

    /* If possible - return to pool */
    if ((uint8_t *)next == J_MPOOL_NEXT_FREE(POOL_VIRTUAL))
    {
        if (item_in_list)
            mp_free_list_remove(item);
        pool_free(POOL_VIRTUAL, item);
    }
    else if (!item_in_list)
    {
        /* Add to list */
        mp_free_list_add(item);
    }
    DBG_RUN(DJOS_MEM, DL_EX_VERBOSE, mp_free_list_dump());
}

static memory_info_t *memp_item_alloc(uint32_t size)
{
    memory_info_t *item;
    void *ptr = NULL;
    DECLARE_FNAME("memp_item_alloc");

    DBG_X(DJOS_MEM, ("%s: Entered, size %lu\n" , FNAME, size));

    if (pool_alloc(&size, &ptr, NULL, POOL_VIRTUAL))
    {
        DBG_I(DJOS_MEM, ("%s: Not enough memory in pool\n", FNAME));
        return NULL;
    }

    item = (memory_info_t *)ptr;
    J_MP_SEG_SET_SIZE(item, size);

    DBG_X(DJOS_MEM, ("%s: Allocated %p-%p - %lu\n", FNAME, item,
        (uint8_t *)item + J_MP_SEG_GET_SIZE(item), size));

    return item;
}

static memory_info_t *memp_reuse_item(memory_info_t *curr, uint32_t size)
{
    memory_info_t *item;
    uint32_t remainder;
    DECLARE_FNAME("memp_reuse_item");

    DBG_V(DJOS_MEM, ("%s: Entered\n", FNAME));

    /* Make sure requested size is aligned. curr's size is already aligned, so
     * rounding up size cannot exceed curr's size */
    if (size % CONFIG_MEMPOOL_ALIGN)
        size = (size + CONFIG_MEMPOOL_ALIGN) - (size % CONFIG_MEMPOOL_ALIGN);

    remainder = J_MP_SEG_GET_SIZE(curr) - size;
    if (remainder >= MP_MIN_SPLIT_SIZE)
    {
        /* Current segment is big enough to split, keep the head in the list,
         * return the tail to the caller */
        item = (memory_info_t *)((uint8_t *)curr + remainder);
        J_MP_SEG_SET_SIZE(item, size);
        J_MP_SEG_SET_USED(item);
        /* Update the new size of 'curr' */
        J_MP_SEG_SET_SIZE(curr, remainder);
        /* Let next segment (item) know curr is free and update the new size at
         * the end of curr's buffer */
        J_MP_SEG_SET_FREE(curr);

        DBG_X(DJOS_MEM, ("%s: Split segment: first item %p (%lu bytes), "
            "second item %p (%lu bytes)\n", FNAME, curr, remainder, item,
            size));
    }
    else
    {
        /* Current segment cannot be split - reuse as is */
        item = curr;
        mp_free_list_remove(curr);
        DBG_X(DJOS_MEM, ("%s: Reusing big block %p sized %lu\n", FNAME, item,
            J_MP_SEG_GET_SIZE(item)));
    }
    DBG_RUN(DJOS_MEM, DL_EX_VERBOSE, mp_free_list_dump());

    return item;
}

static memory_info_t *memp_item_get(uint32_t size)
{
    memory_info_t *item;
    memory_info_t **curr;
    memory_info_t *found = NULL;
    DECLARE_FNAME("memp_item_get");

    DBG_I(DJOS_MEM, ("%s: Entered, size %lu\n", FNAME, size));

    for (curr = &mp_free_list; *curr;
        curr = (memory_info_t **)&(ITEM_NEXT(*curr)))
    {
        J_MPOOL_ASSERT_IN_RANGE(*curr);
        DBG_X(DJOS_MEM, ("%s: Checking %p size %lu\n", FNAME, *curr,
            J_MP_SEG_GET_SIZE(*curr)));

        if (J_MP_SEG_GET_SIZE(*curr) >= size)
        {
            if ((!found) || 
                (J_MP_SEG_GET_SIZE(found) > J_MP_SEG_GET_SIZE(*curr)))
            {
                found = *curr;
            }
        }
    }

    if (found)
        item = memp_reuse_item(found, size);
    else
        item = memp_item_alloc(size);

    DBG_IF(!item)
    {
        DBG_E(DJOS_MEM, ("%s: Failed allocating item, size %lu\n", FNAME,
            size));
        DBG_RUN(DJOS_MEM, DL_ERROR, mem_log_print());
    }

    return item;
}

#ifdef CONFIG_POOL_DMA
static result_t pool_dma_alloc(uint32_t size, void **vaddr, void **paddr,
    uint16_t flags, BOOL *cachable)
{
#  ifdef CONFIG_POOL_DMA_CACHABLE
    result_t rc;
#  endif

    DBG_V(DJOS_MEM, ("JOS_MEM: Allocating a new dma buffer size %lu\n" , size));

#  ifdef CONFIG_POOL_DMA_CACHABLE
    /* For cachable buffers check if there is room in the cachable pool */
    if (flags & M_CACHABLE)
    {
        rc = pool_alloc(&size, vaddr, paddr, POOL_DMA_CACHABLE);
        if (!rc)
        {
            (*cachable) = TRUE;
            return UWE_OK;
        }
    }
#  endif

    *cachable = FALSE;
    return pool_alloc(&size, vaddr, paddr, POOL_DMA);
}
#endif /* CONFIG_POOL_DMA */

#endif /* ifdef CONFIG_MEMPOOL */

#ifdef JOS_MALLOC
#ifdef JMALLOC_STATISTICAL
void *jmalloc_cm(const char *call_func_name, uint32_t size, uint16_t flags)
#else
void *jmalloc_s(uint32_t size, uint16_t flags)
#endif
{
    uint32_t si;
    memory_info_t *m;
	uint32_t interrupts_info, mask;
    uint32_t lr_val = get_lr();
    void *m_ptr=NULL;
    //os_printf("         jmalloc_size=%d\r\n", size);
    LOCK_CHECK_D(file, line);
	SYSirq_Disable_Interrupts_Save_Flags(&interrupts_info, &mask);
#ifdef CONFIG_MEMPOOL
    si = MAX(size, MPOOL_MIN_USER_SIZE) + ALLOC_OVERHEAD;
    m = (memory_info_t *)jmemp_alloc(si, flags);
    if (!m)
    {
    	SYSirq_Interrupts_Restore_Flags(interrupts_info, mask);
        os_printf("\r\njmalloc(%d) = null, lr=0x%08x\r\n", size, lr_val);
        memory_usage_show();
        while(1);
        /* return NULL; */
    }
    si = J_MP_SEG_GET_SIZE(m);
#else
    si = size + ALLOC_OVERHEAD;
    m = (memory_info_t *)os_malloc(si, flags);
    if (!m)
    {
    	SYSirq_Interrupts_Restore_Flags(interrupts_info, mask);
        os_printf("\r\njmalloc(%d) = null, lr=0x%08x\r\n", size, lr_val);
        memory_usage_show();
#ifdef CONFIG_MEMPOOL_OV_DEBUG
        memov_debug_print();
#endif
        while(1);
        /* return NULL; */
    }
    m->act_size = si;
#endif

    J_MEM_DBG_INFO_FILL(m, size);
#ifdef CONFIG_MEMORY_USAGE_SHOW
    mu_stat_update_alloc(m, size, si);
#endif

	SYSirq_Interrupts_Restore_Flags(interrupts_info, mask);

    m_ptr = (void *)(m + 1);
#ifdef JMALLOC_STATISTICAL
    os_printf("jmalloc(%s:%x-%x)\r\n", call_func_name, (uint32_t)m_ptr, size);
#endif
    //os_printf("     malloc_addr=%x\r\n", (int)m_ptr);
#ifdef CONFIG_MEMPOOL_OV_DEBUG
    memov_debug_add((uint32_t)m_ptr, J_MP_SEG_GET_SIZE(m));
#endif
    return m_ptr;
}

#ifdef JMALLOC_STATISTICAL
void jfree_cm(const char *call_func_name, void *addr)
#else
void jfree_s(void *addr)
#endif
{
# if defined(J_DEBUG) && !defined(CONFIG_NO_FREE_DEBUG)
    memory_info_t *m = (memory_info_t *)*addr;
#else
    memory_info_t *m = (memory_info_t *)addr;
	uint32_t interrupts_info, mask;
#endif
    DECLARE_FNAME("jfree");
    //os_printf(" free_addr=%x\r\n", (int)addr);
    LOCK_CHECK_D(file, line);
#ifdef CONFIG_MEMPOOL_OV_DEBUG
    memov_debug_remove((uint32_t)m);
#endif
	SYSirq_Disable_Interrupts_Save_Flags(&interrupts_info, &mask);

    m--;

    DBG_I(DJOS_MEM, ("%s: Entered, address %p (item %p)\n", FNAME, addr, m));

#ifdef J_DEBUG
    KASSERT(addr, ("%s: NULL address %s:%d\n", FNAME, file, line));

    J_MEM_ASSERT_MAGIC(m);

    KASSERT(!m->freed, ("%s: Memory was already freed %s:%d\n", FNAME, file,
        line));
    m->freed++;

    m->prev->next = m->next;
    if (m->next)
        m->next->prev = m->prev;
#endif

#ifdef CONFIG_MEMORY_USAGE_SHOW
    mu_stat_update_free(m);
#endif

#ifdef CONFIG_MEMPOOL
    jmemp_free(m);
#else
    os_free(m);
#endif /* CONFIG_MEMPOOL */

	SYSirq_Interrupts_Restore_Flags(interrupts_info, mask);

# if defined(J_DEBUG) && !defined(CONFIG_NO_FREE_DEBUG) 
    /* Change the caller's pointer - that will (hopefully) cause a crash in case
     * of future attemts to access the pointer of the freed memory */
    *addr = (uint32_t *)0x33333333;
#endif
#ifdef JMALLOC_STATISTICAL
    os_printf("jfree(%s:%x)\r\n", call_func_name, (uint32_t)addr);
#endif
}
#endif /* JOS_MALLOC */

/*
 * DMA Memory pool management
 *
 * We need to allocate a lot of small (many 8 byte, some larger)
 * memory blocks that can be used for DMA.  Using the bus_dma
 * routines directly would incur large overheads in space and time. */

#define DMA_MEM_BLOCK   32

/* In order to ideally utilize DMA allocations we split each segment into three
 * regions. The middle one is returned to the caller of jdma_alloc, and the head
 * and tail are added to the free list, and can be allocated to other callers.
 * This prevent really freeing the DMA buffer during jdma_free, since the other
 * regions of this buffer may be in use.
 * The following defines a minimal size, that if allocation accedes this limit
 * we may assume that they are used temporarily (for data transfers), and
 * therefore we avoid splitting the segment.
 * When jdma_free is called for such segment it is returned to the OS. */
#ifdef CONFIG_POOL_DMA
/* This feature is disable for POOL_DMA */
#define DMA_NO_RECOVER_MIN_SIZE UW_MAX_UINT32
#else
#ifdef CONFIG_DMA_NO_RECOVER_MIN_SIZE
#define DMA_NO_RECOVER_MIN_SIZE CONFIG_DMA_NO_RECOVER_MIN_SIZE
#else
#define DMA_NO_RECOVER_MIN_SIZE (UW_PAGE_SIZE << 2)
#endif
#endif

static LIST_HEAD(dma_frag_s_s, dma_block_s) dma_freelist =
    LIST_HEAD_INITIALIZER(dma_freelist);

void *jdma_phys_addr(jdma_handle p, uint32_t offset)
{
    return (void *)(((char *)((jdma_t *)p)->buffer.paddr) + offset);
}

void *jdma_virt_addr(jdma_handle p, uint32_t offset)
{
    return (void *)(((char *)((jdma_t *)p)->buffer.vaddr) + offset);
}

mem_desc_h jdma_buffer_get(jdma_handle p)
{
    return &(((jdma_t *)p)->buffer);
}

static void jdma_block_free(jdma_t *dma)
{
#if defined(J_DEBUG) || defined(CONFIG_MEMORY_USAGE_SHOW)
    total_dma_free_items_curr++;
    total_dma_free_items_max =
        MAX(total_dma_free_items_max, total_dma_free_items_curr);
#endif

#ifdef J_DEBUG
    ((uint32_t *)jdma_virt_addr(dma, 0))[0] = CORRUPT_CHECK_MAGIC;
#endif

    LIST_INSERT_HEAD(&dma_freelist, dma, next_free);
}

static result_t dma_block_add(uint32_t size, BOOL cachable, uint8_t *vaddr,
    uint8_t *paddr, void *handle, BOOL allow_split)
{
    uint32_t addr = (uint32_t)paddr;
    jdma_t *dma;
    uint16_t i;

    KASSERT(!((uint32_t)vaddr % CONFIG_MEMPOOL_ALIGN),
        ("MEM: New buffer is incorrectly aligned\n"));

    dma = (jdma_t *)jmalloc(sizeof(struct dma_block_s), M_ZERO);

    DBG_V(DJOS_MEM, ("MEM: dma_block_add(size %lu [%p:%p] as %p)\n",
        size, vaddr, paddr, dma));

    MEM_DESC_SET(&(dma->buffer), (void *)vaddr, (phys_addr)paddr, NULL);
    dma->buffer.dma_h = (jdma_handle)dma;

    dma->size = size;
#ifdef J_DEBUG
    dma->split = allow_split;
#endif

    /* Calculate buffer alignment */
    for (i = 1; !(addr & i) && (i <= 4096); i *= 2)
            ;
    dma->align = i;

    if (cachable)
        dma->flags |= BLOCK_CACHABLE;

    /* Check if does not cross page boundary */
    if (((addr / UW_PAGE_SIZE) == (addr + size) / UW_PAGE_SIZE))
        dma->flags |= BLOCK_PAGE_ALIGN;

    /* Save handle if this struct owns the real buffer */
    if (handle)
    {
        dma->flags |= BLOCK_OWNER;
        dma->os_handle = handle;
    }

    /* Put at head of list */
    jdma_block_free(dma);

#if defined(J_DEBUG) || defined(CONFIG_MEMORY_USAGE_SHOW)
    total_dma_items++;
#endif

    return UWE_OK;
}

static result_t dma_block_alloc_os(uint32_t size, void **vaddr, void **paddr,
    uint16_t flags, BOOL *cachable, void **handle)
{
    result_t rc = UWE_OK;

    *vaddr = jmalloc(size, M_ZERO);
    *handle = *paddr = *vaddr;
    *cachable = FALSE;

    return rc;
}

static result_t find_existing(uint32_t size, uint16_t align, uint16_t flags,
    jdma_t **found);

/**
 * Function name:  dma_block_recover
 * Description:    Try to recover memory region around a new block
 * Parameters:
 *     @size:      Size of available space for recovery
 *     @cachable:  Cachability of memory region
 *     @vaddr:     Virtual address of start of free region
 *     @paddr:     Physical address of start of free region
 *
 * Return value:   None
 * Scope:          Local
 **/
static void dma_block_recover(uint32_t size, BOOL cachable, uint8_t *vaddr,
    uint8_t *paddr)
{
    uint32_t phys_addr = (uint32_t)paddr;
    uint32_t offset = 0;
    DECLARE_FNAME("dma_block_recover");

    DBG_V(DJOS_MEM, ("%s: Entered, size %lu, cachable %d, vaddr %p, paddr %p\n",
        FNAME, size, cachable, vaddr, paddr));

    /* Is the size enough to fit in the minimal block ? */
    if (size < DMA_MEM_BLOCK)
        return;

    /* Do we need to re-align for platform requirements ? */
    if (phys_addr % CONFIG_MEMPOOL_ALIGN)
        offset = CONFIG_MEMPOOL_ALIGN - (phys_addr % CONFIG_MEMPOOL_ALIGN);

    /* Did the re-alignment larger then the size we have ? */
    if (size <= offset)
        return;

    /* Do we still fit the minimal block (after re-align) */
    if ((size - offset) < DMA_MEM_BLOCK)
        return;

    DBG_X(DJOS_MEM, ("\n%s: Adding new block - size %lu, cachable %d, "
        "vaddr %p, paddr %p\n\n", FNAME, size - offset, cachable,
        (uint8_t *)((uint32_t)vaddr + offset),
        (uint8_t *)((uint32_t)paddr + offset)));
    /* Add new block */
    dma_block_add(size - offset, cachable,
        (uint8_t *)((uint32_t)vaddr + offset),
        (uint8_t *)((uint32_t)paddr + offset), NULL, 1);
}

static result_t dma_block_alloc_full(uint32_t size, uint16_t align,
    uint16_t flags)
{
    result_t rc;
    BOOL cachable = 0;
    uint32_t needed_offset = 0;
    uint32_t alloc_size = size;
    uint8_t *vaddr, *paddr;
    void *vtemp, *ptemp = NULL;
    void *handle = NULL;
    BOOL allow_split = 1;
    DECLARE_FNAME("dma_block_alloc_full");

    /* Make sure the user doesn't want something crazy */
    if ((flags & M_PAGE_ALIGN) && (size > UW_PAGE_SIZE))
    {
        DBG_E(DJOS_MEM, ("%s: Requested PAGE_ALIGN while buffer size is larger "
            "than a page (%lu:%lu)\n", FNAME, size, UW_PAGE_SIZE));
        return UWE_INVAL;
    }

    /* Make sure we allocate aleast DMA_MEM_BLOCK size */
    alloc_size = MAX(alloc_size, DMA_MEM_BLOCK);

    /* Extra space for alignment */
    alloc_size += align - 1;

    /* Extra space for page boundary restriction */
    if (flags & M_PAGE_ALIGN)
        alloc_size += alloc_size;

    DBG_V(DJOS_MEM, ("%s: Size %lu, real %lu\n", FNAME, alloc_size, size));

    /* Alloc memory block */
    rc = dma_block_alloc_os(alloc_size, &vtemp, &ptemp, flags, &cachable,
        &handle);
    if (rc)
    {
        DBG_E(DJOS_MEM, ("%s: Failed allocating DMA buffer of %lu bytes - %s\n",
            FNAME, alloc_size, uwe_str(rc)));
        return rc;
    }

    vaddr = (uint8_t *)vtemp;
    paddr = (uint8_t *)ptemp;

    /* See if page bounds are crossed */
    if (flags & M_PAGE_ALIGN)
    {
        if ((((uint32_t)paddr % UW_PAGE_SIZE) + (size + align - 1)) >
            UW_PAGE_SIZE)
        {
            needed_offset = UW_PAGE_SIZE - ((uint32_t)paddr % UW_PAGE_SIZE);
        }
    }

    /* See if we need to realign the resulting buffer */
    if (((uint32_t)paddr + needed_offset) % align)
    {
        needed_offset = needed_offset + align -
            (((uint32_t)paddr + needed_offset) % align);
    }

    if (alloc_size <= DMA_NO_RECOVER_MIN_SIZE)
    {
        /* Recover the wasted space around the allocated buffer */
        dma_block_recover(needed_offset, cachable, vaddr, paddr);
        dma_block_recover(alloc_size - size - needed_offset, cachable,
            vaddr + needed_offset + size, paddr + needed_offset + size);
    }
    else
    {
        DBG_X(DJOS_MEM, ("Allocated large DMA chunk - %lu, skipped recover\n",
            alloc_size));
        allow_split = 0;
        size = alloc_size;
    }

    /* Add the required block */
    return dma_block_add(size, cachable, vaddr + needed_offset,
        paddr + needed_offset, handle, allow_split);
}

static result_t dma_block_alloc(uint32_t size, uint16_t align, uint16_t flags)
{
    KASSERT(size, ("MEM: Cannot use dma_block_alloc for zero sized blocks\n"));

#ifdef CONFIG_MEM_DMA_MIN_ALLOC
    /* If this is a small alloc, create a big buffer and fragment it */
    if ((size * 4) < CONFIG_MEM_DMA_MIN_ALLOC &&
        align < CONFIG_MEM_DMA_MIN_ALLOC)
    {
        result_t rc;

        rc = dma_block_alloc_frags(size, align, flags);
        /* If succeed, then stop */
        if (rc == UWE_OK)
            return UWE_OK;

        /* Corruption may have occurred */
        if (rc == UWE_STATE)
        {
            DBG_E(DJOS_MEM, ("MEM: Corruption occurred!\n"));

            return UWE_STATE;
        }
    }
#endif

    return dma_block_alloc_full(size, align, flags);
}

static result_t find_existing(uint32_t size, uint16_t align, uint16_t flags,
    jdma_t **found)
{
    jdma_t *dma;

    *found = NULL;

    if (!size)
        return UWE_OK;

    /* First check the free list. */
    for (dma = LIST_FIRST(&dma_freelist); dma; dma = LIST_NEXT(dma, next_free))
    {
        /* Make sure it has the same cachabilitity */
        if (((flags & M_CACHABLE) && !(dma->flags & BLOCK_CACHABLE)) ||
            (!(flags & M_CACHABLE) && (dma->flags & BLOCK_CACHABLE)))
        {
            continue;
        }

        /* Make sure buffer is page aligned, if required */
        if ((flags & M_PAGE_ALIGN) && !(dma->flags & BLOCK_PAGE_ALIGN))
            continue;

        /* Make sure the alignment requirement is met */
        if (dma->align < align)
            continue;

        /* We need a block about the same size */
        if (dma->size < size || dma->size >= size * 2)
            continue;

        KASSERT(size <= DMA_NO_RECOVER_MIN_SIZE, ("Found chunk bigger than "
            "%lu\n", (uint32_t)DMA_NO_RECOVER_MIN_SIZE));
        /* Found! */
        LIST_REMOVE(dma, next_free);

#if defined(J_DEBUG) || defined(CONFIG_MEMORY_USAGE_SHOW)
        total_dma_free_items_curr--;
#endif

        *found = dma;

        DBG_X(DJOS_MEM,("MEM: Found block %p size %lu\n", dma, dma->size));
        break;
    }

    return UWE_OK;
}

result_t jdma_alloc(uint32_t size, uint16_t align, void **vaddr, void **paddr,
    uint16_t flags, jdma_handle *handle)
{
    result_t rc;
    jdma_t *dma = NULL;
#ifdef J_DEBUG
    uint32_t calc;
#endif
    DECLARE_FNAME("jdma_alloc");

    LOCK_CHECK;

    DBG_I(DJOS_MEM, ("%s: Entered, size %lu, align %u, flags 0x%x [%s:%d]\n",
        FNAME, size, align, flags, file, line));

#ifdef CONFIG_MEM_CACHE_LINE_SIZE
    if (size % CONFIG_MEM_CACHE_LINE_SIZE)
    {
        size += CONFIG_MEM_CACHE_LINE_SIZE -
            (size % CONFIG_MEM_CACHE_LINE_SIZE);
    }
#endif

#ifdef CONFIG_USER_SPACE
# ifndef CONFIG_SHARED_MEM
    if (flags & M_SHARED)
        flags &= ~(M_SHARED | M_KERNEL);
#endif
    if (flags & (M_KERNEL | M_SHARED))
        return u_dma_kernel_alloc(size, align, vaddr, paddr, flags, handle);
#endif /* CONFIG_USER_SPACE */

    /* Handle empty dummy blocks */
    if (size == 0)
    {
        dma = (jdma_t *)jmalloc(sizeof(struct dma_block_s), M_ZERO);

        DBG_X(DJOS_MEM, ("%s: Alloc dummy %p\n", FNAME, dma));

        dma->flags |= BLOCK_DUMMY;

        KASSERT0(handle);

        *handle = (jdma_handle)dma;

#if defined(J_DEBUG) || defined(CONFIG_MEMORY_USAGE_SHOW)
        blocks_used_curr++;
#endif
        return UWE_OK;
    }

    /* Make sure we allocate at least DMA_MEM_BLOCK size */
    size = MAX(size, DMA_MEM_BLOCK);

    /* Make alignment reasonable */
    align = MAX(align, CONFIG_MEMPOOL_ALIGN);

    /* Try to find an existing buffer on the free list */
    rc = find_existing(size, align, flags, &dma);
    if (rc)
        return rc;

    /* If we didn't find a cachable buffer, get a normal one */
    if ((!dma) && (flags & M_CACHABLE))
    {
        rc = find_existing(size, align, (flags & ~M_CACHABLE), &dma);
        if (rc)
            return rc;
    }

    /* If we didn't find a suitable buffer, allocate a new one */
    if (!dma)
    {
        /* Alloc a new block if nothing matching was found */
        rc = dma_block_alloc(size, align, flags);
        if (rc)
        {
            DBG_E(DJOS_MEM, ("%s: Failed allocating dma item - %s (size %lu, "
                "align %d, flags %x)\n", FNAME, uwe_str(rc), size, align,
                flags));
            DBG_RUN(DJOS_MEM, DL_ERROR, dump_dma_freelist());
            return rc;
        }

        /* Get the first block
         * (dma_alloc_block will put the needed block at head) */
        dma = LIST_FIRST(&dma_freelist);
        LIST_REMOVE(dma, next_free);

#ifdef J_DEBUG
        if (dma->size > DMA_NO_RECOVER_MIN_SIZE)
        {
            KASSERT(!dma->split && dma->os_handle, ("jdma_alloc: large DMA "
                "chunk (%lu) with split set or missing os_handle!\n",
                dma->size));
        }

        if (((uint32_t *)jdma_virt_addr(dma, 0))[0] != CORRUPT_CHECK_MAGIC)
        {
            DBG_E(DJOS_MEM, ("%s: New item %p on freelist is corrupted\n",
                FNAME, dma));
            /* XXX - Should this be an assert?*/
            return UWE_UNKNOWN;
        }
#endif

#if defined(J_DEBUG) || defined(CONFIG_MEMORY_USAGE_SHOW)
        total_dma_free_items_curr--;
#endif
    }

    if (handle)
        *handle = (jdma_handle)dma;

    if (vaddr)
        *vaddr = jdma_virt_addr(dma, 0);

    if (paddr)
        *paddr = jdma_phys_addr(dma, 0);

    if (flags & M_ZERO)
        j_memset(jdma_virt_addr(dma, 0), 0, size);

    DBG_I(DJOS_MEM, ("%s: Allocated %p - [%p:%p] size %lu\n", FNAME, dma,
        jdma_virt_addr(dma, 0), jdma_phys_addr(dma, 0), dma->size));

#if defined(J_DEBUG) || defined(CONFIG_MEMORY_USAGE_SHOW)
    total_dma_peak_curr += dma->size;
    blocks_used_curr++;
#endif

#ifdef CONFIG_MEMORY_USAGE_SHOW
    total_dma_peak_max = MAX(total_dma_peak_max, total_dma_peak_curr);
    blocks_used_max = MAX(blocks_used_max, blocks_used_curr);
#endif

#ifdef J_DEBUG
    dma->file = file;
    dma->line = line;
    /* dma->freed should be either 0 (when DMA segment first allocated) or 1
     * (when reusing a DMA segment) */
    KASSERT(dma->freed <= 1, ("%s: freed field in dma invalid %d\n", FNAME,
        dma->freed));
    dma->freed = 0;
    LIST_INSERT_HEAD(&dma_alloclist, dma, next_alloc);
    DBG_RUN(DJOS_MEM, DL_EX_VERBOSE, dump_dma_alloclist());

    /* Check the alignment is as requested */
    calc = (uint32_t)jdma_phys_addr(dma, 0);
    if (calc % align)
    {
        DBG_E(DJOS_MEM, ("%s: Allocated buffer is not properly aligned\n",
            FNAME));
        goto Release;
    }

    if (flags & M_PAGE_ALIGN)
    {
        uint32_t page_end = calc - (calc % UW_PAGE_SIZE) + UW_PAGE_SIZE;

        if (calc + size > page_end)
        {
            DBG_E(DJOS_MEM, ("%s: Allocated buffer 0x%lx size %lu is not PAGE "
                "aligned (page end 0x%08lx)\n", FNAME, calc, size, page_end));
            goto Release;
        }
    }

    DBG_V(DJOS_MEM, ("%s: Completed successfully\n", FNAME));
    return UWE_OK;

Release:
    jdma_free((jdma_handle *)&dma);
    return UWE_INVAL;
#else
    return UWE_OK;
#endif
}

void jdma_free(jdma_handle p)
{
    jdma_t *dma = (jdma_t *)p;
    DECLARE_FNAME("jdma_free");

    LOCK_CHECK;

    KASSERT(!dma->freed, ("%s: [%s:%d] DMA buffer was already freed! [%s:%d]\n",
        FNAME, file, line, dma->file, dma->line));
    DBG_I(DJOS_MEM, ("%s: %p - [%p:%p] size %lu, [%s:%d], allocated at %s:%d\n",
        FNAME, dma, jdma_virt_addr(dma, 0), jdma_phys_addr(dma, 0), dma->size,
        file, line, dma->file, dma->line));

#ifdef CONFIG_USER_SPACE
    if (dma->flags & BLOCK_KERNEL)
    {
        u_dma_kernel_free(dma);
        return;
    }
#endif

    if (dma->flags & BLOCK_DUMMY)
    {
        DBG_IF(dma->size)
        {
            DBG_E(DJOS_MEM, ("%s: Dummy was not reset before being freed\n",
                FNAME));
        }

        DBG_X(DJOS_MEM, ("%s: Freeing dummy !\n", FNAME));
        jfree(dma);

#if defined(J_DEBUG) || defined(CONFIG_MEMORY_USAGE_SHOW)
        blocks_used_curr--;
#endif
        return;
    }

#ifdef J_DEBUG
    LIST_REMOVE(dma, next_alloc);
    dma->file = file;
    dma->line = line;
    dma->freed++;
#endif

#if defined(J_DEBUG) || defined(CONFIG_MEMORY_USAGE_SHOW)
    total_dma_peak_curr -= dma->size;
    blocks_used_curr--;
#endif
    DBG_V(DJOS_MEM, ("%s: Completed:\n", FNAME));
    DBG_RUN(DJOS_MEM, DL_EX_VERBOSE, dump_dma_freelist());

    if (dma->size <= DMA_NO_RECOVER_MIN_SIZE)
    {
        KASSERT0(dma->split);
        jdma_block_free(dma);
    }
    else
    {
        KASSERT0(!dma->split);
        DBG_X(DJOS_MEM, ("%s: Large chunk (%lu) freeing\n", FNAME, dma->size));
#ifdef CONFIG_MEMPOOL_DMABLE
        jfree(dma->os_handle);
#elif defined(CONFIG_POOL_DMA)
        KASSERT(0, ("DMA_NO_RECOVER_MIN_SIZE with POOL_DMA Not supported!\n"));
#else
        KASSERT0(dma->os_handle);
        os_dma_free(dma->os_handle);
#endif

        jfree(dma);
    }
}

void jdma_free_all(void)
{
    jdma_t *dma;
    DECLARE_FNAME("jdma_free_all");

#ifdef J_DEBUG
    LIST_FOREACH(dma, &dma_freelist, next_free)
    {
        if (((uint32_t *)jdma_virt_addr(dma, 0))[0] != CORRUPT_CHECK_MAGIC)
            DBG_E(DJOS_MEM, ("%s: DMA block %p corrupted\n", FNAME, dma));
    }
#endif

    for (dma = LIST_FIRST(&dma_freelist); dma; dma = LIST_FIRST(&dma_freelist))
    {
        LIST_REMOVE(dma, next_free);

        if (!(dma->flags & BLOCK_OWNER))
        {
            jfree(dma);

            continue;
        }

        DBG_I(DJOS_MEM, ("%s: Freeing block %p handle %p alignment %d\n", FNAME,
            dma, dma->os_handle, dma->align));

#ifdef CONFIG_MEMPOOL_DMABLE
        jfree(dma->os_handle);
#elif defined(CONFIG_POOL_DMA)
        /* We give the whole pool back on uninit */
#else
        os_dma_free(dma->os_handle);
#endif /* CONFIG_MEMPOOL_DMABLE */

        jfree(dma);
    }
}

result_t jos_mem_init(uw_args_t *args)
{
    result_t rc = UWE_OK;
    
    LOG_D(APP,"jos_mem_init\r\n");

    DBG_I(DJOS_MEM, ("%s: Entered, args %p\n", FNAME, args));
    KASSERT(sizeof(uint32_t) <= DMA_MEM_BLOCK, ("%s: Minimal dma alloc buffer "
        "is too small (%d) should be at least %d\n", FNAME, DMA_MEM_BLOCK,
        sizeof(uint32_t)));

#ifdef CONFIG_MEMORY_USAGE_SHOW
#  if (RANGE_STATISTICS != 0)
    j_memset(mu_alloc_ranges, 0, sizeof(mu_alloc_ranges));
#  endif
#endif

#ifdef CONFIG_MEMPOOL
    mempool_init();

    pool_init(POOL_VIRTUAL, args->mem_buf, args->mem_buf, args->mem_buf_size);
#endif

#ifdef CONFIG_POOL_DMA
    pool_init(POOL_DMA, args->dma_vbuf, args->dma_pbuf, args->dma_buf_size);

#ifdef CONFIG_POOL_DMA_CACHABLE
    pool_init(POOL_DMA_CACHABLE, args->dma_cachable_vbuf,
        args->dma_cachable_pbuf, args->dma_cachable_size);
#endif
#endif /* CONFIG_POOL_DMA */

    return rc;
}

void jos_mem_uninit(void)
{
    DECLARE_FNAME("jos_mem_uninit");

    DBG_I(DJOS_MEM, ("%s: Entered\n", FNAME));
#ifdef CONFIG_MEMPOOL
    mempool_uninit();
#endif
#ifdef CONFIG_MEMORY_USAGE_SHOW
    dump_mem_status();
#endif
}

// EOF
