/* Modifications by Jungo Ltd. Copyright (c) 2012 Jungo Ltd. All Rights reserved */

/*
 * Copyright (c) 1999, 2001 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Jason R. Thorpe of the Numerical Aerospace Simulation Facility,
 * NASA Ames Research Center.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * Copyright (c) 1982, 1986, 1988, 1991, 1993
 *      The Regents of the University of California.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <jos/jos_mbuf.h>

#define mbstat_type_add(type, diff) do {} while (0)

/* malloc size */
#define MALLOC_LEN(data_len, ext) \
    ((ext) ? MSIZE_EXT : MAX((data_len), MLEN_MIN) + MSIZE_NODATA)

/*
 * Copy mbuf pkthdr from `from' to `to'.
 * `from' must have M_PKTHDR set, and `to' must be empty.
 */
static __INLINE__ void m_copy_pkthdr(struct mbuf *to, const struct mbuf *from)
{
    KASSERT0((to->m_flags & M_PKTHDR) == 0);
    KASSERT0((from->m_flags & M_PKTHDR) != 0);

    to->m_pkt_len = from->m_pkt_len;
    to->m_pkt_priv = from->m_pkt_priv;
    to->m_flags = from->m_flags & M_COPYFLAGS;
}

/*
 * Move mbuf pkthdr from `from' to `to'.
 * `from' must have M_PKTHDR set, and `to' must be empty.
 */
static __INLINE__ void m_move_pkthdr(struct mbuf *to, struct mbuf *from)
{
    m_copy_pkthdr(to, from);

    from->m_flags &= ~M_PKTHDR;
}

/*
 * Initialize the mbuf allocator
 */
result_t mbuf_init(void)
{
    return UWE_OK;
}

void mbuf_uninit(void)
{
}

struct mbuf *m_get_flags_ext(mtype_t type, uint16_t flags, int32_t len,
   void *ext_buf, void(*ext_free)(void *), void *args)
{
    struct mbuf *m;
    BOOL is_ext_buf = ((flags & M_EXT) == M_EXT);

#ifdef CONFIG_JOS_MBUF_DMA_POOL
    jdma_handle dma_h;

    if(jdma_alloc(MALLOC_LEN(len, is_ext_buf), 0, (void **)&m, NULL,
        flags | M_CACHABLE, &dma_h) != UWE_OK)
    {
        return NULL;
    }
#else
    m = (struct mbuf *)jmalloc(MALLOC_LEN(len, is_ext_buf), flags);
#endif

    KASSERT0((flags & ~(M_ZERO | M_PKTHDR | M_EXT)) == 0);
    KASSERT0((flags & (M_ZERO | M_EXT)) != (M_ZERO | M_EXT));
    KASSERT0((is_ext_buf && ext_buf) || !is_ext_buf);

    /* TODO: use file and line */

    mbstat_type_add(type, 1);

    /* If the entire mbuf was not zeroed out already, zero the non-data bytes */
    if(!(flags & M_ZERO))
        j_memset(m, 0, J_OFFSET_OF(struct mbuf, mh_dat));

#ifdef CONFIG_JOS_MBUF_DMA_POOL
    m->m_dma = dma_h;
#endif
    m->m_buf_len = len;         /* size of buffer */

    if(is_ext_buf)
    {
        /* set initial position of the data pointer and external buf */
        m->m_data = m->m_buf_ext.ext_buf = ext_buf;
        m->m_buf_ext.ext_free = ext_free;
        m->m_buf_ext.args = args;
        m->m_flags |= M_EXT;
    }
    else
    {
        /* set initial position of the data pointer */
        m->m_data = m->m_buf;
    }

    /* type of data in this mbuf */
    m->m_type = type;

    if(flags & M_PKTHDR)
        m->m_flags |= M_PKTHDR;
    return m;
}

/* Zero out mbuf chain, keep M_PKTHDR flag of the first mbuf (if present).
 * The data buffer is not zeroed. */
void m_reset(struct mbuf *m0)
{
    struct mbuf *m;
    uint16_t pkthdr = m0->m_flags & M_PKTHDR;

    for (m = m0; m; m = m->m_next)
    {
        m->m_len = 0;
        m->m_pkt_len = 0;
        m->m_pkt_priv = NULL;
        m->m_flags &= M_EXT; /* clear all flags except M_EXT */
        m->m_data = M_BUF(m);
    }

    m0->m_flags = pkthdr;
}

/* Free mbuf m, return a pointer to the next mbuf (if any) */
struct mbuf *m_free(struct mbuf *m)
{
    struct mbuf *n;

    mbstat_type_add(m->m_type, -1);
    n = m->m_next;
    if((m->m_flags & M_EXT) && m->m_buf_ext.ext_free)
        m->m_buf_ext.ext_free(m->m_buf_ext.args);

#ifdef CONFIG_JOS_MBUF_DMA_POOL
    jdma_free(m->m_dma);
#else
    jfree(m);
    m = NULL;
#endif

    return n;
}

/* Free the mbuf chain beginning with m */
void m_freem(struct mbuf *m)
{
    while (m)
        m = m_free(m);
}

/*
 * Shaves off req_len bytes from head or tail of the (valid) data area.  If
 * req_len is greater than zero, front bytes are being shaved off, if it's
 * smaller, from the back (and if it is zero, the mbuf will stay bearded).  This
 * function does not move data in any way, but is used to manipulate the data
 * area pointer and data length variable of the mbuf in a non-clobbering way.
 */
void m_adj(struct mbuf *mp, int32_t req_len)
{
    int32_t len = req_len;
    struct mbuf *m = mp;
    int32_t count;

    if(m == NULL || !len)
        return;

    if(len > 0)
    {
        /*
         * Trim from head.
         */
        while (m != NULL && len > 0)
        {
            if(m->m_len <= len)
            {
                len -= m->m_len;
                m->m_len = 0;
                m = m->m_next;
            }
            else
            {
                m->m_len -= len;
                m->m_data += len;
                len = 0;
            }
        }
        m = mp;
        if(mp->m_flags & M_PKTHDR)
            m->m_pkt_len -= (req_len - len);
    }
    else
    {
        /*
         * Trim from tail.  Scan the mbuf chain,
         * calculating its length and finding the last mbuf.
         * If the adjustment only affects this mbuf, then just
         * adjust and return.  Otherwise, rescan and truncate
         * after the remaining size.
         */
        len = -len;
        for (count = 0; ; m = m->m_next)
        {
            count += m->m_len;
            if(m->m_next == NULL)
                break;
        }

        if(m->m_len >= len)
        {
            m->m_len -= len;
            if(mp->m_flags & M_PKTHDR)
                mp->m_pkt_len -= len;
            return;
        }
        count -= len;
        if(count < 0)
            count = 0;
        /*
         * Correct length for chain is "count".
         * Find the mbuf with last data, adjust its length,
         * and toss data from remaining mbufs on chain.
         */
        m = mp;
        if(m->m_flags & M_PKTHDR)
            m->m_pkt_len = count;
        for (; m; m = m->m_next)
        {
            if(m->m_len >= count)
            {
                m->m_len = count;
                break;
            }
            count -= m->m_len;
        }
        if(m)
        {
            for (m = m->m_next; m; m = m->m_next)
                m->m_len = 0;
        }
    }
}

/*
 * Concatenate mbuf chain n to m.
 * n might be copied into m (when n->m_len is small), therefore data portion of
 * n could be copied into an mbuf of different mbuf type.
 * m_pkt_len is not updated.
 */
void m_cat(struct mbuf *m, struct mbuf *n)
{
    while (m->m_next)
        m = m->m_next;

    m->m_next = n;
}

/*
 * Concatenate mbuf chain n to m.
 * n might be copied into m (when n->m_len is small), therefore data portion of
 * n could be copied into an mbuf of different mbuf type.
 * m_pkt_len is updated.
 */
struct mbuf *m_cat_pkt(struct mbuf *m, struct mbuf *n)
{
    m_cat(m, n);

    /* m_cat() simply appends 'n' to the chain, it does not update m_pkt_len */
    m->m_pkt_len += n->m_pkt_len;
    n->m_pkt_len = 0;
    n->m_flags &= ~M_PKTHDR;

    return m;
}

/*
 * Prepend 'len' bytes to the chain, allocate a new mbuf if needed.
 * m_len and m_pkt_len are updated.
 */
struct mbuf *m_prepend(struct mbuf *m, int32_t len)
{
    if(M_LEADINGSPACE(m) >= len)
    {
        m->m_data -= len;
        m->m_len += len;
    }
    else
    {
        struct mbuf *n = m_get_flags(m->m_type, 0, len);

        if(n == NULL)
            return NULL;

        if(m->m_flags & M_PKTHDR)
            m_move_pkthdr(n, m);

        n->m_next = m;
        m = n;
        m->m_len = len;
    }

    if(m->m_flags & M_PKTHDR)
        m->m_pkt_len += len;

    return m;
}

/*
 * Partition an mbuf chain in two pieces, returning the tail --
 * all but the first len0 bytes.  In case of failure, it returns NULL and
 * attempts to restore the chain to its original state.
 */
struct mbuf *m_split(struct mbuf *m0, int32_t len0)
{
    struct mbuf *m, *n;
    uint32_t len = len0, remain;

    /* Walk through the chain, looking for an mbuf that will get split */
    for (m = m0; m && len > m->m_len; m = m->m_next)
        len -= m->m_len;
    if(m == NULL)
        goto Error;

    KASSERT(!(m->m_flags & M_EXT), ("no support for split of ext buffers\n"));

    if(m->m_flags & M_EXT)
        goto Error;

    /* we split the current mbuf here.
     * 'len' bytes belong to the last mbuf of m0 chain.
     * 'remain' bytes belong to the first mbuf in n chain
     */
    remain = m->m_len - len;

    if(m0->m_flags & M_PKTHDR)
    {
        n = m_get_flags(m->m_type, M_PKTHDR, remain);
        if(n == NULL)
            goto Error;

        n->m_pkt_len = m0->m_pkt_len - len0;
        m0->m_pkt_len = len0;
    }
    else if(remain == 0)
    {
        /* The "clear cut" case */
        n = m->m_next;
        m->m_next = NULL;
        goto Exit;
    }
    else
    {
        n = m_get_flags(m->m_type, 0, remain);
        if(n == NULL)
            goto Error;
    }

    if(remain)
        j_memcpy(mtod(n, void *), mtod(m, uint8_t *) + len, remain);

    n->m_len = remain;
    m->m_len = len;
    n->m_next = m->m_next;
    m->m_next = NULL;

Exit:
    return n;

Error:
    return NULL;
}

struct mbuf *m_copym(const struct mbuf *m, int32_t off0, int32_t len)
{
    struct mbuf *n, **np;
    int32_t off = off0;
    struct mbuf *top;
    BOOL copyhdr = 0;

    KASSERT(off >= 0 && len >= 0, ("m_copym: off %ld, len %ld", off, len));

    if(off == 0 && (m->m_flags & M_PKTHDR))
        copyhdr = 1;

    /* Skip 'off' bytes */
    while (off > 0)
    {
        KASSERT(m, ("m_copym: m == NULL, off %ld", off));
        if(off < m->m_len)
            break;
        off -= m->m_len;
        m = m->m_next;
    }

    /* Copy mbuf chain */
    for (np = &top, top = NULL; len > 0; np = &n->m_next)
    {
        if(m == NULL)
        {
            KASSERT(len == M_COPYALL, ("m_copym: m == NULL, len %ld [!COPYALL]",
                len));
            break;
        }
        n = m_get(m->m_type);
        *np = n;
        if(n == NULL)
            goto Nomem;
        if(copyhdr)
        {
            m_copy_pkthdr(n, m);
            if(len == M_COPYALL)
                n->m_pkt_len -= off0;
            else
                n->m_pkt_len = len;
            copyhdr = 0;
        }
        n->m_len = MIN(len, m->m_len - off);
        j_memcpy(mtod(n, void *), mtod(m, uint8_t *) + off, n->m_len);
        if(len != M_COPYALL)
            len -= n->m_len;
        off += n->m_len;

        KASSERT(off <= m->m_len, ("m_copym overrun"));

        if(off == m->m_len)
        {
            m = m->m_next;
            off = 0;
        }
    }

    return top;

Nomem:
    m_freem(top);
    return NULL;
}

/*
 * Copy an entire packet, including header (which must be present).
 * An optimization of the common case `m_copym(m, 0, M_COPYALL)'.
 */
struct mbuf *m_copypacket(const struct mbuf *m)
{
    struct mbuf *top, *n, *o;

    top = m_get_flags(m->m_type, 0, m->m_len);
    if(!top)
        return NULL;

    m_copy_pkthdr(top, m);
    top->m_len = m->m_len;
    j_memcpy(mtod(top, uint8_t *), mtod(m, uint8_t *), top->m_len);

    for (m = m->m_next, n = top; m; m = m->m_next)
    {
        o = m_get_flags(m->m_type, 0, m->m_len);
        if(!o)
            goto Nomem;

        n->m_next = o;
        n = n->m_next;
        n->m_len = m->m_len;
        j_memcpy(mtod(n, uint8_t *), mtod(m, uint8_t *), n->m_len);
    }

    return top;
Nomem:
    m_freem(top);
    return NULL;
}

/*
 * Copy data from an mbuf chain starting "off" bytes from the beginning,
 * continuing for "len" bytes, into the indicated buffer.
 */
void m_copydata(const struct mbuf *m, int32_t off, int32_t len, void *vp)
{
    uint32_t count;
    uint8_t *cp = (uint8_t *)vp;

    KASSERT0((off >= 0 && len >= 0));

    /* Skip 'off' bytes from the beginning of mbuf chain */
    for (; off > 0; m = m->m_next)
    {
        KASSERT0(m);
        if(off < m->m_len)
            break;
        off -= m->m_len;
    }

    /* Copy 'len' bytes */
    for (; len > 0; m = m->m_next)
    {
        KASSERT(m, ("off %lu, len %lu\n", off, len));

        count = MIN(m->m_len - off, len);
        j_memcpy(cp, mtod(m, uint8_t *) + off, count);
        off = 0;

        len -= count;
        cp += count;
    }
}

/*
 * Copy data from a buffer back into the indicated mbuf chain,
 * starting "off" bytes from the beginning, extending the mbuf
 * chain if necessary.
 */
result_t m_copyback(struct mbuf *m0, int32_t off, int32_t len, const void *vp)
{
    int32_t mlen, tspace, totlen = 0;
    struct mbuf *m = m0, *n;
    const uint8_t *cp = (const uint8_t *)vp;

    KASSERT0(m0 != NULL);

    /* Skip 'off' bytes from the beginning of mbuf chain. Extend the chain if
     * it's too short. */
    for (mlen = m->m_len; off > mlen; m = m->m_next, mlen = m->m_len)
    {
        off -= mlen;
        totlen += mlen;
        if(m->m_next == NULL)
        {
            /*
             * This is the last 'm'. If it has a trailing space - use it.
             */
            tspace = M_TRAILINGSPACE(m);
            if(!tspace)
                goto Extend;

            /* Skip bytes within the trailing space.
             * The skipped bytes are zeroed out */
            j_memset(mtod(m, uint8_t *) + mlen, 0, MIN(off, tspace));

            if(off > tspace)
            {
                /* Skip 'tspace' bytes, i.e. skip the entire trailing space */
                m->m_len += tspace;
                off -= tspace;
                totlen += tspace;
                goto Extend;
            }

            /* Skip 'off' bytes, the forthcoming "copy" loop will copy data into
             * the remainder of the trailing space.
             *
             * Adjust 'm_len' and 'off' for the "copy" loop */
            m->m_len += MIN(tspace, off + len);
            off += mlen;
            totlen -= mlen;
            break;
        }
    }

    /* Copy as much as we can to 'm' */
    while (len > 0)
    {
        mlen = MIN(m->m_len - off, len);
        j_memcpy(mtod(m, uint8_t *) + off, cp, mlen);
        cp += mlen;
        len -= mlen;
        totlen += mlen + off;
        off = 0;
        if(len == 0)
            goto Exit;

        if(m->m_next == NULL)
        {
            tspace = M_TRAILINGSPACE(m);
            if(!tspace)
                goto Extend;

            /* Increase 'm_len' to accomodate the trailing space */
            off = m->m_len;
            totlen -= off;
            m->m_len += MIN(tspace, len);
            continue;
        }

        m = m->m_next;
    }

    goto Exit;

Extend:
    /* Append a new mbuf to the end of chain and copy the remaining data. */
    n = m_get_flags(m->m_type, 0, MAX(off + len, MLEN));
    if(n == NULL)
        goto Nomem;

    j_memcpy(mtod(n, uint8_t *) + off, cp, len);
    n->m_len = off + len;
    totlen += len;
    m->m_next = n;

Exit:
    if((m0->m_flags & M_PKTHDR) && (m0->m_pkt_len < totlen))
        m0->m_pkt_len = totlen;

    DBG_IF((m0->m_flags & M_PKTHDR) && !m0->m_next)
    {
        KASSERT(m0->m_pkt_len == m0->m_len,
            ("m_next %p m_pkt_len 0x%lx m_len 0x%lx\n", m0->m_next,
            m0->m_pkt_len, m0->m_len));
    }

    return UWE_OK;

Nomem:
    return UWE_NOMEM;
}

/*
 * Copy the first 'len' bytes from 'm' to 'p'. Move the data pointer by 'len'.
 * Return a failure if 'm' is not long enough.
 */
result_t m_trim(struct mbuf *m, int32_t len, void *p)
{
    if(m_length(m) < len)
        return UWE_MSGSIZE;

    m_copydata(m, 0, len, p);
    m_adj(m, len);
    return UWE_OK;
}

/*
 * Free all the buffers pointing to the first 'len' bytes as long as a whole
 * buffer can be released. If not, return the number of bytes not released.
 * Update the new packet header
 */
struct mbuf *m_release_pkt(struct mbuf *m, uint32_t *len)
{
    uint32_t pkt_len = m_length(m);

    while (m && (*len >= m->m_len))
    {
        *len -= m->m_len;
        pkt_len -= m->m_len;
        m = m_free(m);
    }

    if(m)
    {
        m->m_pkt_len = pkt_len;
        m->m_flags |= M_PKTHDR;
    }

    return m;
}
