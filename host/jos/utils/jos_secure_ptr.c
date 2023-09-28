/* (c) 2012 Jungo Ltd. All Rights Reserved. Jungo Confidential */

#include <jos.h>

#define SPTR_MAX 0xfffe
/* TODO add resize step as init parameter */
#define SPTR_RESIZE_STEP 10
#define SPTR_MAKE(i, cnt) (((uint32_t)(cnt)<<16) | ((uint32_t)(i)+1))
#define SPTR_GET_IDX(sptr) ((uint16_t)(sptr) - 1)
#define SPTR_GET_CNT(sptr) ((uint16_t)((sptr)>>16))

typedef struct {
    void *ptr;
    uint16_t count;
} sptr_entry_t;

struct j_sptr_pool_t {
    sptr_entry_t *entries;
    int32_t num_entries;
    j_sptr_pool_type_t type;
    /* TODO manage free indexes lists (odd and even lists) */
};

static BOOL resize_pool(j_sptr_pool_h pool, int32_t new_num);

/**
 * Function name:  j_secure_ptr_pool_init
 * Description:    Initializes secure ptr pool
 * Parameters:
 *     @type: (IN) Defines the j_secure_ptr_create behaviour:
 *                 JSPTR_NORMAL - sptr may be any value in range 1..SPTR_MAX
 *                 JSPTR_ODD    - sptr may be only odd value
 *                 JSPTR_EVEN   - sptr may be only even value
 *
 * Return value:   Pool handle or NULL on failure
 * Scope:          Global
 **/
j_sptr_pool_h j_secure_ptr_pool_init(j_sptr_pool_type_t type)
{
    j_sptr_pool_h pool;

    pool = (j_sptr_pool_h)jmalloc(sizeof(struct j_sptr_pool_t), M_ZERO);
    pool->type = type;

    return pool;
}

/**
 * Function name:  j_secure_ptr_pool_uninit
 * Description:    Uninitializes secure ptr pool
 * Parameters:
 *     @pool: (IN) Secure ptr pool handle
 *
 * Return value:   None
 * Scope:          Global
 **/
void j_secure_ptr_pool_uninit(j_sptr_pool_h pool)
{
    if (pool->entries)
    {
        jfree(pool->entries);
        pool->entries = NULL;
    }
    
    jfree(pool);
    pool = NULL;
}

/**
 * Function name:  j_secure_ptr_domesticize
 * Description:    Stores user ptr in corresponding secure ptr if it is not in
 *                 use
 * Parameters:
 *     @pool: (IN) Secure ptr pool handle
 *     @sptr: (IN) Secure ptr
 *     @ptr:  (IN) User ptr
 *
 * Return value:   UWE_OK on success or error code on failure
 * Scope:          Global
 **/
result_t j_secure_ptr_domesticize(j_sptr_pool_h pool, j_sptr_t sptr, void *ptr)
{
    uint16_t i = SPTR_GET_IDX(sptr);

    KASSERT0(sptr);
    if (!sptr)
        return UWE_INVAL;

    if (i >= pool->num_entries)
    {
        if (!resize_pool(pool, i + SPTR_RESIZE_STEP))
            return UWE_NOMEM;
    }
    else if (pool->entries[i].ptr)
    {
        return UWE_BUSY;
    }

    pool->entries[i].count = SPTR_GET_CNT(sptr);
    pool->entries[i].ptr = ptr;

    return UWE_OK;
}

/**
 * Function name:  j_secure_ptr_create
 * Description:    Creates secure ptr
 * Parameters:
 *     @pool: (IN) Secure ptr pool handle
 *     @ptr:  (IN) User ptr
 *
 * Return value:   Secure ptr or 0 on failure
 * Scope:          Global
 **/
j_sptr_t j_secure_ptr_create(j_sptr_pool_h pool, void *ptr)
{
    int32_t i;

    /* in SPTR_MAKE we increment i, therefore we are looking for odd index for
     * even pool type and vice versa */
    for (i = (pool->type == JSPTR_EVEN); i < pool->num_entries;
        i += 1 + (pool->type != JSPTR_NORMAL))
    {
        if (!pool->entries[i].ptr)
            goto Exit;
    }

    if (!resize_pool(pool, pool->num_entries + SPTR_RESIZE_STEP))
        return 0;

Exit:
    pool->entries[i].ptr = ptr;
    return SPTR_MAKE(i, pool->entries[i].count);
}

static BOOL resize_pool(j_sptr_pool_h pool, int32_t new_num)
{
    sptr_entry_t *new_entries;

    if (new_num > SPTR_MAX)
        return 0;

    new_entries = (sptr_entry_t *)jmalloc(new_num * sizeof(sptr_entry_t), M_ZERO);
    if (pool->entries)
    {
        j_memcpy(new_entries, pool->entries, pool->num_entries *
            sizeof(sptr_entry_t));
        jfree(pool->entries);
        pool->entries = NULL;
    }

    j_memset(new_entries + pool->num_entries, 0,
        SPTR_RESIZE_STEP * sizeof(sptr_entry_t));
    pool->entries = new_entries;
    pool->num_entries = new_num;

    return 1;
}

#ifdef J_DEBUG
static void check_validity(j_sptr_pool_h pool, j_sptr_t sptr, uint16_t i,
    const char *file, int32_t line, const char *func)
{
    KASSERT(sptr, ("%s: sptr is NULL [%s:%d]\n", func, file, line));

    KASSERT(i < pool->num_entries, ("%s: Invalid sptr=0x%08lx, index out of "
        "range (pool_size=%d) [%s:%d]\n", func, sptr, pool->num_entries, file,
        line));

    if (!pool->entries[i].ptr || pool->entries[i].count != SPTR_GET_CNT(sptr))
    {
        DBG_E(DJOS_UTILS, ("%s: sptr=0x%08lx, was destroyed [%s:%d]\n", func,
            sptr, file, line));
    }
}
#endif

/**
 * Function name:  j_secure_ptr_destroy
 * Description:    Destroes secure ptr
 * Parameters:
 *     @pool: (IN) Secure ptr pool handle
 *     @sptr: (IN) Secure ptr
 *
 * Return value:   None
 * Scope:          Global
 **/
#ifdef J_DEBUG
void j_secure_ptr_destroy_d(j_sptr_pool_h pool, j_sptr_t sptr, char *file,
    int32_t line)
#else
void j_secure_ptr_destroy(j_sptr_pool_h pool, j_sptr_t sptr)
#endif
{
    uint16_t i = SPTR_GET_IDX(sptr);
    DECLARE_FNAME("j_secure_ptr_destroy");

    DBG_RUN(DJOS_UTILS, DL_ERROR, check_validity(pool, sptr, i, file, line,
        FNAME));

    if (!sptr || i >= pool->num_entries)
        return;

    if (pool->entries[i].count != SPTR_GET_CNT(sptr))
        return;

    if (!pool->entries[i].ptr)
        return;

    pool->entries[i].count++;
    pool->entries[i].ptr = NULL;
}

/**
 * Function name:  j_secure_ptr_get
 * Description:    Returns user ptr corresponding to secure ptr
 * Parameters:
 *     @pool: (IN) Secure ptr pool handle
 *     @sptr: (IN) Secure ptr
 *
 * Return value:   User ptr or NULL on failure
 * Scope:          Global
 **/
#ifdef J_DEBUG
void *j_secure_ptr_get_d(j_sptr_pool_h pool, j_sptr_t sptr, char *file,
    int32_t line)
#else
void *j_secure_ptr_get(j_sptr_pool_h pool, j_sptr_t sptr)
#endif
{
    int32_t i = SPTR_GET_IDX(sptr);
    DECLARE_FNAME("j_secure_ptr_get");

    DBG_RUN(DJOS_UTILS, DL_ERROR, check_validity(pool, sptr, i, file, line,
        FNAME));

    if (!sptr || i >= pool->num_entries)
        return NULL;

    if (pool->entries[i].count != SPTR_GET_CNT(sptr))
        return NULL;

    return pool->entries[i].ptr;
}

