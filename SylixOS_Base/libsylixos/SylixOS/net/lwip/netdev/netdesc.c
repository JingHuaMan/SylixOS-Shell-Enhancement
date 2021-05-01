/**
 * @file
 * Lwip platform independent driver interface.
 * This set of driver Tx/Rx descriptor helper,
 * as much as possible compatible with different versions of LwIP
 * Verification using sylixos(tm) real-time operating system
 */

/*
 * Copyright (c) 2006-2019 SylixOS Group.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 * 4. This code has been or is applying for intellectual property protection
 *    and can only be used with acoinfo software products.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT
 * SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT
 * OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY
 * OF SUCH DAMAGE.
 *
 * Author: Han.hui <hanhui@acoinfo.com>
 *
 */

#define  __SYLIXOS_KERNEL
#include "lwip/mem.h"
#include "lwip/sys.h"
#include "lwip/pbuf.h"

#if LW_CFG_NET_DEV_DESC_HELPER_EN > 0

#include "unistd.h"
#include "string.h"
#include "netdev.h"

#ifndef NETDEV_DESC_EACH_BUF_MIN_SIZE
#define NETDEV_DESC_EACH_BUF_MIN_SIZE   1280
#endif /* !NETDEV_DESC_EACH_BUF_MIN_SIZE */

/* delete descriptor Tx buffer array */
static void netdev_desc_tx_buf_delete (struct netdev_desc_helper *helper)
{
  void *fbuf;
  size_t page_size;
  struct netdev_desc_buf *tx_buf;
  int i, nbufs_per_page;

#if LW_CFG_VMM_EN > 0
  page_size = getpagesize();
#else /* LW_CFG_VMM_EN */
  page_size = 4 * LW_CFG_KB_SIZE;
#endif /* !LW_CFG_VMM_EN */

  tx_buf = helper->tx_buf;
  helper->tx_buf = NULL;

  if (page_size >= helper->each_buf_size) {
    nbufs_per_page = page_size / helper->each_buf_size;
    for (i = 0; i < helper->tx_buf_cnt; i++) {
      if (!(i % nbufs_per_page) && tx_buf[i].buffer) {
        fbuf = (void *)((addr_t)tx_buf[i].buffer - helper->pad_size);
#if LW_CFG_VMM_EN > 0
        vmmDmaFree(fbuf);
#else /* LW_CFG_VMM_EN */
        sys_free(fbuf);
#endif /* !LW_CFG_VMM_EN */
      }
    }

  } else {
    for (i = 0; i < helper->tx_buf_cnt; i++) {
      if (tx_buf[i].buffer) {
        fbuf = (void *)((addr_t)tx_buf[i].buffer - helper->pad_size);
#if LW_CFG_VMM_EN > 0
        vmmDmaFree(fbuf);
#else /* LW_CFG_VMM_EN */
        sys_free(fbuf);
#endif /* !LW_CFG_VMM_EN */
      }
    }
  }

  sys_free(tx_buf);
}

/* create descriptor Tx buffer array */
static int netdev_desc_tx_buf_create (struct netdev_desc_helper *helper)
{
  void *stbuf = NULL; /* avoid warning */
  size_t page_size;
  struct netdev_desc_buf *tx_buf;
  int i, nbufs_per_page, npages_per_buf;

  tx_buf = (struct netdev_desc_buf *)sys_zalloc(sizeof(struct netdev_desc_buf) * helper->tx_buf_cnt);
  if (!tx_buf) {
    return (-1);
  }

#if LW_CFG_VMM_EN > 0
  page_size = getpagesize();
#else /* LW_CFG_VMM_EN */
  page_size = 4 * LW_CFG_KB_SIZE;
#endif /* !LW_CFG_VMM_EN */

  if (page_size >= helper->each_buf_size) {
    nbufs_per_page = page_size / helper->each_buf_size;
    for (i = 0; i < helper->tx_buf_cnt; i++) {
      if (i % nbufs_per_page) {
          stbuf = (void *)((addr_t)stbuf + helper->each_buf_size);
          tx_buf[i].buffer = (void *)((addr_t)stbuf + helper->pad_size);

      } else {
#if LW_CFG_VMM_EN > 0
        stbuf = vmmDmaAllocAlignWithFlags(page_size, page_size, helper->cache_ts_flags);
#else /* LW_CFG_VMM_EN */
        stbuf = sys_malloc(page_size);
#endif /* !LW_CFG_VMM_EN */
        if (!stbuf) {
          goto error;
        }
        tx_buf[i].buffer = (void *)((addr_t)stbuf + helper->pad_size);
      }
    }

  } else {
    npages_per_buf = ROUND_UP(helper->each_buf_size, page_size) / page_size;
    for (i = 0; i < helper->tx_buf_cnt; i++) {
#if LW_CFG_VMM_EN > 0
      stbuf = vmmDmaAllocAlignWithFlags(npages_per_buf * page_size, page_size, helper->cache_ts_flags);
#else /* LW_CFG_VMM_EN */
      stbuf = sys_malloc(npages_per_buf * page_size);
#endif /* !LW_CFG_VMM_EN */
      if (!stbuf) {
        goto error;
      }
      tx_buf[i].buffer = (void *)((addr_t)stbuf + helper->pad_size);
    }
  }

  helper->tx_buf = tx_buf;
  return (0);

error:
  helper->tx_buf = tx_buf;
  netdev_desc_tx_buf_delete(helper);
  return (-1);
}

/* delete descriptor Tx buffer array */
static void netdev_desc_rx_buf_delete (struct netdev_desc_helper *helper)
{
  void *fbuf;
  size_t page_size;
  struct netdev_desc_buf *rx_buf;
  int i, nbufs_per_page;

#if LW_CFG_VMM_EN > 0
  page_size = getpagesize();
#else /* LW_CFG_VMM_EN */
  page_size = 4 * LW_CFG_KB_SIZE;
#endif /* !LW_CFG_VMM_EN */

  rx_buf = helper->rx_buf;
  helper->rx_buf = NULL;

  if (page_size >= helper->each_buf_size) {
    nbufs_per_page = page_size / helper->each_buf_size;
    for (i = 0; i < helper->rx_buf_cnt; i++) {
#if LW_CFG_NET_DEV_ZCBUF_EN > 0
      if (rx_buf[i].p) {
        netdev_zc_pbuf_free(rx_buf[i].p);
      }
#endif /* LW_CFG_NET_DEV_ZCBUF_EN */
      if (!(i % nbufs_per_page) && rx_buf[i].buffer) {
        fbuf = (void *)((addr_t)rx_buf[i].buffer - helper->pad_size);
#if LW_CFG_VMM_EN > 0
        vmmDmaFree(fbuf);
#else /* LW_CFG_VMM_EN */
        sys_free(fbuf);
#endif /* !LW_CFG_VMM_EN */
      }
    }

  } else {
    for (i = 0; i < helper->rx_buf_cnt; i++) {
#if LW_CFG_NET_DEV_ZCBUF_EN > 0
      if (rx_buf[i].p) {
        netdev_zc_pbuf_free(rx_buf[i].p);
      }
#endif /* LW_CFG_NET_DEV_ZCBUF_EN */
      if (rx_buf[i].buffer) {
        fbuf = (void *)((addr_t)rx_buf[i].buffer - helper->pad_size);
#if LW_CFG_VMM_EN > 0
        vmmDmaFree(fbuf);
#else /* LW_CFG_VMM_EN */
        sys_free(fbuf);
#endif /* !LW_CFG_VMM_EN */
      }
    }
  }

  sys_free(rx_buf);
}

/* create descriptor Rx buffer array */
static int netdev_desc_rx_buf_create (struct netdev_desc_helper *helper)
{
  void *stbuf = NULL; /* avoid warning */
  size_t page_size;
  struct netdev_desc_buf *rx_buf;
  int i, nbufs_per_page, npages_per_buf;

  rx_buf = (struct netdev_desc_buf *)sys_zalloc(sizeof(struct netdev_desc_buf) * helper->rx_buf_cnt);
  if (!rx_buf) {
    return (-1);
  }

#if LW_CFG_VMM_EN > 0
  page_size = getpagesize();
#else /* LW_CFG_VMM_EN */
  page_size = 4 * LW_CFG_KB_SIZE;
#endif /* !LW_CFG_VMM_EN */

  if (page_size >= helper->each_buf_size) {
    nbufs_per_page = page_size / helper->each_buf_size;
    for (i = 0; i < helper->rx_buf_cnt; i++) {
      if (i % nbufs_per_page) {
        stbuf = (void *)((addr_t)stbuf + helper->each_buf_size);
        rx_buf[i].buffer = (void *)((addr_t)stbuf + helper->pad_size);

      } else {
#if LW_CFG_VMM_EN > 0
        stbuf = vmmDmaAllocAlignWithFlags(page_size, page_size, helper->cache_rs_flags);
#else /* LW_CFG_VMM_EN */
        stbuf = sys_malloc(page_size);
#endif /* !LW_CFG_VMM_EN */
        if (!stbuf) {
          goto error;
        }
        rx_buf[i].buffer = (void *)((addr_t)stbuf + helper->pad_size);
      }
    }

  } else {
    npages_per_buf = ROUND_UP(helper->each_buf_size, page_size) / page_size;
    for (i = 0; i < helper->rx_buf_cnt; i++) {
#if LW_CFG_VMM_EN > 0
      stbuf = vmmDmaAllocAlignWithFlags(npages_per_buf * page_size, page_size, helper->cache_rs_flags);
#else /* LW_CFG_VMM_EN */
      stbuf = sys_malloc(npages_per_buf * page_size);
#endif /* !LW_CFG_VMM_EN */
      if (!stbuf) {
        goto error;
      }
      rx_buf[i].buffer = (void *)((addr_t)stbuf + helper->pad_size);
    }
  }

  helper->rx_buf = rx_buf;
  return (0);

error:
  helper->rx_buf = rx_buf;
  netdev_desc_rx_buf_delete(helper);
  return (-1);
}

/* create descriptor helper */
struct netdev_desc_helper *
netdev_desc_helper_create (size_t each_buf_size, size_t pad_size,
                           int cache_ts_en, int cache_rs_en, int cache_zc_en,
                           int tx_buf_cnt, int rx_buf_cnt, int tx_zc_en, int rx_zc_cnt)
{
  struct netdev_desc_helper *helper;
  size_t cache_line;

  if ((tx_buf_cnt < 0) || (rx_buf_cnt < 0)) {
    return (NULL);
  }

  each_buf_size += pad_size;
  if (each_buf_size < NETDEV_DESC_EACH_BUF_MIN_SIZE) {
    return (NULL);
  }

#if LW_CFG_CACHE_EN > 0
  cache_line = cacheLine(DATA_CACHE);
#else /* LW_CFG_CACHE_EN */
#ifdef LW_CFG_CPU_ARCH_CACHE_LINE
  cache_line = LW_CFG_CPU_ARCH_CACHE_LINE;
#else
  cache_line = 32;
#endif /* LW_CFG_CPU_ARCH_CACHE_LINE */
#endif /* !LW_CFG_CACHE_EN */

  each_buf_size = ROUND_UP(each_buf_size, cache_line);

  helper = (struct netdev_desc_helper *)sys_zalloc(sizeof(struct netdev_desc_helper));
  if (!helper) {
    return (NULL);
  }

  helper->tx_buf_cnt = tx_buf_cnt;
  helper->rx_buf_cnt = rx_buf_cnt;

  helper->each_buf_size = each_buf_size;
  helper->pad_size = pad_size;
  helper->tx_zc_en = tx_zc_en;
  helper->rx_zc_cnt = rx_zc_cnt;

#if LW_CFG_CACHE_EN > 0
  if (cacheGetMode(DATA_CACHE) & CACHE_SNOOP_ENABLE) {  /* cache has snoop unit */
    helper->cache_zc_flags   = LW_VMM_FLAG_RDWR;
    helper->cache_ts_flags   = LW_VMM_FLAG_RDWR;
    helper->cache_rs_flags   = LW_VMM_FLAG_RDWR;
    helper->cache_pb_flush   = 0;
    helper->cache_ts_flush   = 0;
    helper->cache_zc_invalid = 0;
    helper->cache_rs_invalid = 0;

  } else {
    if (cacheGetMode(DATA_CACHE) & CACHE_WRITETHROUGH) { /* cache is writethrough */
      helper->cache_ts_flags = LW_VMM_FLAG_RDWR;
      helper->cache_pb_flush = 0;
      helper->cache_ts_flush = 0;

    } else {
      helper->cache_pb_flush = 1; /* must flush send pbuf */
      if (cache_ts_en) {
        helper->cache_ts_flags = LW_VMM_FLAG_RDWR; /* send static buffer has cache */
        helper->cache_ts_flush = 1;

      } else {
        helper->cache_ts_flags = LW_VMM_FLAG_DMA; /* send static buffer no cache */
        helper->cache_ts_flush = 0;
      }
    }

    if (cache_rs_en) { /* recv static buffer has cache */
      helper->cache_rs_flags   = LW_VMM_FLAG_RDWR;
      helper->cache_rs_invalid = 1;

    } else {
      helper->cache_rs_flags   = LW_VMM_FLAG_DMA;
      helper->cache_rs_invalid = 0;
    }

    if (cache_zc_en) { /* recv zc buffer has cache */
      helper->cache_zc_flags   = LW_VMM_FLAG_RDWR;
      helper->cache_zc_invalid = 1;

    } else {
      helper->cache_zc_flags   = LW_VMM_FLAG_DMA;
      helper->cache_zc_invalid = 0;
    }
  }
#endif /* !LW_CFG_CACHE_EN */

#if LW_CFG_NET_DEV_ZCBUF_EN > 0
  if (rx_zc_cnt > 0) {
    size_t page_size;
    size_t blk_size;

    /* each_buf_size add zc_buf size */
    blk_size = each_buf_size + sizeof(struct pbuf_custom) + sizeof(void *);
    blk_size = ROUND_UP(blk_size, cache_line);

#if LW_CFG_VMM_EN > 0
    page_size = getpagesize();
    helper->rx_zpmem = vmmDmaAllocAlignWithFlags(rx_zc_cnt * blk_size, page_size, helper->cache_zc_flags);
#else /* LW_CFG_VMM_EN */
    page_size = 4 * LW_CFG_KB_SIZE;
    helper->rx_zpmem = sys_malloc_align(rx_zc_cnt * blk_size, page_size);
#endif /* !LW_CFG_VMM_EN */

    if (!helper->rx_zpmem) {
      goto error;
    }

    helper->rx_hzcpool = netdev_zc_pbuf_pool_create((addr_t)helper->rx_zpmem, rx_zc_cnt, blk_size);
    if (!helper->rx_hzcpool) {
      goto error;
    }
  }
#endif /* LW_CFG_NET_DEV_ZCBUF_EN */

  if (netdev_desc_tx_buf_create(helper)) {
    goto error;
  }

  if (netdev_desc_rx_buf_create(helper)) {
    goto error;
  }

  return (helper);

error:
#if LW_CFG_NET_DEV_ZCBUF_EN > 0
  if (rx_zc_cnt > 0) {
    if (helper->rx_hzcpool) {
      netdev_zc_pbuf_pool_delete(helper->rx_hzcpool, 1);
    }

    if (helper->rx_zpmem) {
#if LW_CFG_VMM_EN > 0
      vmmDmaFree(helper->rx_zpmem);
#else /* LW_CFG_VMM_EN */
      sys_free(helper->rx_zpmem);
#endif /* !LW_CFG_VMM_EN */
    }
  }
#endif /* LW_CFG_NET_DEV_ZCBUF_EN */

  sys_free(helper);

  return (NULL);
}

/* delete descriptor helper (you must STOP netdev hardware first!) */
int netdev_desc_helper_delete (struct netdev_desc_helper *helper)
{
  if (!helper) {
    return (-1);
  }

  netdev_desc_tx_buf_delete(helper);
  netdev_desc_rx_buf_delete(helper);

#if LW_CFG_NET_DEV_ZCBUF_EN > 0
  if (helper->rx_hzcpool) {
    int i;
    struct pbuf *p;

    /* recycle all pbuf */
    for (i = 0; i < helper->rx_zc_cnt; i++) {
      do {
        p = netdev_zc_pbuf_alloc(helper->rx_hzcpool, -1);
      } while (!p);
    }

    netdev_zc_pbuf_pool_delete(helper->rx_hzcpool, 1);

#if LW_CFG_VMM_EN > 0
    vmmDmaFree(helper->rx_zpmem);
#else /* LW_CFG_VMM_EN */
    sys_free(helper->rx_zpmem);
#endif /* !LW_CFG_VMM_EN */
  }
#endif /* LW_CFG_NET_DEV_ZCBUF_EN */

  sys_free(helper);

  return (0);
}

/* netdev_desc_tx_prepare (you must ensure 'idx' is valid) */
netdev_desc_btype netdev_desc_tx_prepare (struct netdev_desc_helper *helper, int idx, struct pbuf *p)
{
  struct netdev_desc_buf *tx_buf;

  tx_buf = NETDEV_TX_DESC_BUF(helper, idx);
  if (NETDEV_TX_CAN_REF_PBUF(p) && helper->tx_zc_en) {
    pbuf_ref(p);
    tx_buf->p = p;
#if LW_CFG_CACHE_EN > 0
    if (helper->cache_pb_flush) {
      cacheFlush(DATA_CACHE, p->payload, p->tot_len);
    }
#endif /* LW_CFG_CACHE_EN */
    return (NETDEV_DESC_PBUF);

  } else {
    LWIP_ASSERT("buffer length to long!", p->tot_len <= helper->each_buf_size);
    pbuf_copy_partial(p, tx_buf->buffer, p->tot_len, 0);
#if LW_CFG_CACHE_EN > 0
    if (helper->cache_ts_flush) {
      cacheFlush(DATA_CACHE, tx_buf->buffer, p->tot_len);
    }
#endif /* LW_CFG_CACHE_EN */
    return (NETDEV_DESC_SBUF);
  }
}

/* netdev_desc_tx_clean (you must ensure 'idx' is valid) */
void netdev_desc_tx_clean (struct netdev_desc_helper *helper, int idx)
{
  struct netdev_desc_buf *tx_buf;

  tx_buf = NETDEV_TX_DESC_BUF(helper, idx);
  if (tx_buf->p) {
    pbuf_free(tx_buf->p);
    tx_buf->p = NULL;
  }
}

/* netdev_desc_rx_input (you must ensure 'idx' is valid) */
struct pbuf *netdev_desc_rx_input (struct netdev_desc_helper *helper, int idx, int len)
{
  struct pbuf *p;
  struct netdev_desc_buf *rx_buf;

  rx_buf = NETDEV_RX_DESC_BUF(helper, idx);
  if (rx_buf->p) {
    p = rx_buf->p;
    p->tot_len = p->len = (u16_t)len;
    rx_buf->p = NULL;
#if LW_CFG_CACHE_EN > 0
    if (helper->cache_zc_invalid) {
      cacheInvalidate(DATA_CACHE, p->payload, p->tot_len);
    }
#endif /* LW_CFG_CACHE_EN */

  } else {
    p = netdev_pbuf_alloc((u16_t)len);
    if (!p) {
      return (NULL);
    }
#if LW_CFG_CACHE_EN > 0
    if (helper->cache_rs_invalid) {
      cacheInvalidate(DATA_CACHE, rx_buf->buffer, len);
    }
#endif /* LW_CFG_CACHE_EN */
    pbuf_take(p, rx_buf->buffer, (u16_t)len);
  }

  return (p);
}

/* netdev_desc_rx_input_offset (you must ensure 'idx' is valid) */
struct pbuf *netdev_desc_rx_input_offset (struct netdev_desc_helper *helper, int idx, int len, int offset)
{
  struct pbuf *p;
  struct netdev_desc_buf *rx_buf;

  rx_buf = NETDEV_RX_DESC_BUF(helper, idx);
  if (rx_buf->p) {
    p = rx_buf->p;
    p->tot_len = p->len = (u16_t)len;
    rx_buf->p = NULL;
#if LW_CFG_CACHE_EN > 0
    if (helper->cache_zc_invalid) {
      cacheInvalidate(DATA_CACHE, p->payload, p->tot_len);
    }
#endif /* LW_CFG_CACHE_EN */

  } else {
    p = netdev_pbuf_alloc((u16_t)len);
    if (!p) {
      return (NULL);
    }
#if LW_CFG_CACHE_EN > 0
    if (helper->cache_rs_invalid) {
      cacheInvalidate(DATA_CACHE, (char *)rx_buf->buffer + offset, len);
    }
#endif /* LW_CFG_CACHE_EN */
    pbuf_take(p, (char *)rx_buf->buffer + offset, (u16_t)len);
  }

  return (p);
}

/* netdev_desc_rx_refill (you must ensure 'idx' is valid) */
netdev_desc_btype netdev_desc_rx_refill (struct netdev_desc_helper *helper, int idx)
{
#if LW_CFG_NET_DEV_ZCBUF_EN > 0
  struct netdev_desc_buf *rx_buf;

  rx_buf = NETDEV_RX_DESC_BUF(helper, idx);
  if (rx_buf->p) {
    return (NETDEV_DESC_PBUF);
  }

  if (helper->rx_hzcpool) {
    rx_buf->p = netdev_zc_pbuf_alloc(helper->rx_hzcpool, LW_OPTION_NOT_WAIT);
    if (rx_buf->p) {
#if LW_CFG_CACHE_EN > 0
      if (helper->cache_zc_invalid) {
        cacheInvalidate(DATA_CACHE, rx_buf->p->payload, rx_buf->p->tot_len);
      }
#endif /* LW_CFG_CACHE_EN */
      return (NETDEV_DESC_PBUF);
    }
  }
#endif /* LW_CFG_NET_DEV_ZCBUF_EN */

  return (NETDEV_DESC_SBUF);
}

/* netdev_desc_rx_refill_res (you must ensure 'idx' is valid) */
netdev_desc_btype netdev_desc_rx_refill_res (struct netdev_desc_helper *helper, int idx, UINT16 res)
{
#if LW_CFG_NET_DEV_ZCBUF_EN > 0
  struct netdev_desc_buf *rx_buf;

  rx_buf = NETDEV_RX_DESC_BUF(helper, idx);
  if (rx_buf->p) {
    return (NETDEV_DESC_PBUF);
  }

  if (helper->rx_hzcpool) {
    rx_buf->p = netdev_zc_pbuf_alloc_res(helper->rx_hzcpool, LW_OPTION_NOT_WAIT, res);
    if (rx_buf->p) {
#if LW_CFG_CACHE_EN > 0
      if (helper->cache_zc_invalid) {
        cacheInvalidate(DATA_CACHE, rx_buf->p->payload, rx_buf->p->tot_len);
      }
#endif /* LW_CFG_CACHE_EN */
      return (NETDEV_DESC_PBUF);
    }
  }
#endif /* LW_CFG_NET_DEV_ZCBUF_EN */

  return (NETDEV_DESC_SBUF);
}

#endif /* LW_CFG_NET_DEV_DESC_HELPER_EN > 0 */
/*
 * end
 */
