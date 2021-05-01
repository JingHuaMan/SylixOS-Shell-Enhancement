/**
 * @file
 * net device receive 'zero copy'(custom) buffer.
 * This set of driver interface shields the netif details, 
 * as much as possible compatible with different versions of LwIP
 * Verification using sylixos(tm) real-time operating system
 */

/*
 * Copyright (c) 2006-2017 SylixOS Group.
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
#include "lwip/sys.h"
#include "lwip/pbuf.h"
#include "lwip/mem.h"
#include "net/if_ether.h"

#include "netdev.h"

#if LW_CFG_NET_DEV_ZCBUF_EN > 0

/*
 * zcpbuf header size cache alignment.
 */
#define ZCPBUF_HEADER_SIZE  ROUND_UP(sizeof(struct zc_pbuf), zc_buf_cache_line_size)

/*
 * cache size alignment.
 */
static u32_t zc_buf_cache_line_size;

/*
 * Statistical variable
 */
static u32_t zc_buf_used, zc_buf_max, zc_buf_error;

/*
 * zc_buf_sl
 */
static LW_SPINLOCK_CA_DEFINE_CACHE_ALIGN(zc_buf_sl) = LW_SPIN_CA_INITIALIZER;

/*    zc_pool
 * +------------+
 * |  magic_no  |
 * |  total_cnt |
 * |  free_cnt  |                 zc_pbuf                        zc_pbuf
 * |  pbuf_len  |             +--------------+               +--------------+
 * |    free    | ----------> |    hzcpool   |       |-----> |    hzcpool   |       |-----> ... NULL
 * |    sem     |             | next / cpbuf | ------/       | next / cpbuf | ------/
 * +------------+             |     ....     |               |     ....     |
 *                            +--------------+               +--------------+
 */

struct zc_pool;

/* netdev zero copy buffer */
struct zc_pbuf {
  struct zc_pool *zcpool;
  union {
    struct zc_pbuf *next;
    struct pbuf_custom cpbuf;
  } b;
};

/* netdev zero copy pool */
struct zc_pool {
#define ZCPOOL_MAGIC  0xf7e34a82
  UINT32 magic_no;
  UINT32 total_cnt;
  UINT32 free_cnt;
  UINT32 pbuf_len;
  struct zc_pbuf *free;
  LW_HANDLE sem;
};

/* netdev zero copy buffer pool create */
void *netdev_zc_pbuf_pool_create (addr_t addr, UINT32 blkcnt, size_t blksize)
{
  int i;
  struct zc_pool *zcpool;
  struct zc_pbuf *zcpbuf1, *zcpbuf2;
  
  if (!zc_buf_cache_line_size) {
#if LW_CFG_CACHE_EN > 0
    zc_buf_cache_line_size = cacheLine(DATA_CACHE);
#else /* LW_CFG_CACHE_EN */
#ifdef LW_CFG_CPU_ARCH_CACHE_LINE
    zc_buf_cache_line_size = LW_CFG_CPU_ARCH_CACHE_LINE;
#else
    zc_buf_cache_line_size = 32;
#endif /* LW_CFG_CPU_ARCH_CACHE_LINE */
#endif /* !LW_CFG_CACHE_EN */
    _BugFormat(!zc_buf_cache_line_size, LW_TRUE,
               "cache line size: %s error!\r\n", zc_buf_cache_line_size);
  }

  if ((blkcnt < 2) || (blksize < (ETH_PAD_SIZE + 
      SIZEOF_VLAN_HDR + ETH_FRAME_LEN + ZCPBUF_HEADER_SIZE))) {
    return (NULL);
  }
  
  zcpool = (struct zc_pool *)mem_malloc(sizeof(struct zc_pool));
  if (!zcpool) {
    return (NULL);
  }
  
  zcpool->sem = API_SemaphoreBCreate("zc_pool", FALSE, LW_OPTION_DEFAULT, NULL); /* test-pend */
  if (!zcpool->sem) {
    mem_free(zcpool);
    return (NULL);
  }
  
  zcpool->free = (struct zc_pbuf *)addr;
  
  zcpbuf1 = (struct zc_pbuf *)addr;
  zcpbuf2 = (struct zc_pbuf *)((addr_t)zcpbuf1 + blksize);
  
  for (i = 0; i < (blkcnt - 1); i++) {
    zcpbuf1->zcpool = zcpool;
    zcpbuf1->b.next = zcpbuf2;
    zcpbuf1 = (struct zc_pbuf *)((addr_t)zcpbuf1 + blksize);
    zcpbuf2 = (struct zc_pbuf *)((addr_t)zcpbuf2 + blksize);
  }
  
  zcpbuf1->zcpool = zcpool;
  zcpbuf1->b.next = NULL;
  
  zcpool->magic_no  = ZCPOOL_MAGIC;
  zcpool->total_cnt = blkcnt;
  zcpool->free_cnt  = blkcnt;
  zcpool->pbuf_len  = blksize - ZCPBUF_HEADER_SIZE;
  
  return ((void *)zcpool);
}

/* netdev zero copy buffer pool delete */
int netdev_zc_pbuf_pool_delete (void *hzcpool, int force)
{
  struct zc_pool *zcpool = (struct zc_pool *)hzcpool;

  if (!zcpool || (zcpool->magic_no != ZCPOOL_MAGIC)) {
    return (-1);
  }
  
  if (!force && (zcpool->total_cnt != zcpool->free_cnt)) {
    return (-1);
  }
  
  API_SemaphoreBDelete(&zcpool->sem);
  mem_free(zcpool);
  
  return (0);
}

/* zc buffer free internal */
static void netdev_zc_pbuf_free_cb (struct pbuf *p)
{
  struct zc_pbuf *zcpbuf = _LIST_ENTRY(p, struct zc_pbuf, b.cpbuf);
  struct zc_pool *zcpool = zcpbuf->zcpool;
  int wakeup;
  INTREG level;
  
  LW_SPIN_LOCK_QUICK(&zc_buf_sl.SLCA_sl, &level);
  wakeup = (zcpool->free) ? 0 : 1;
  zcpbuf->b.next = zcpool->free;
  zcpool->free = zcpbuf;
  zcpool->free_cnt++;
  zc_buf_used--;
  LW_SPIN_UNLOCK_QUICK(&zc_buf_sl.SLCA_sl, level);
  
  if (wakeup) {
    API_SemaphoreBPost(zcpool->sem);
  }
}

/* netdev input 'zero copy' buffer get a blk
 * reserve: ETH_PAD_SIZE + SIZEOF_VLAN_HDR size. 
 * ticks = 0  no wait
 *       = -1 wait forever */
struct pbuf *netdev_zc_pbuf_alloc_res (void *hzcpool, int ticks, UINT16 hdr_res)
{
  struct zc_pool *zcpool = (struct zc_pool *)hzcpool;
  struct zc_pbuf *zcpbuf;
  struct pbuf *ret;
  ULONG to;
  INTREG level;
  u16_t reserve = ETH_PAD_SIZE + SIZEOF_VLAN_HDR + hdr_res;

  if (!zcpool || (zcpool->magic_no != ZCPOOL_MAGIC)) {
    return (NULL);
  }
  
  do {
    LW_SPIN_LOCK_QUICK(&zc_buf_sl.SLCA_sl, &level);
    if (zcpool->free) {
      break;
    }
    LW_SPIN_UNLOCK_QUICK(&zc_buf_sl.SLCA_sl, level);
    
    if (ticks == 0) {
      zc_buf_error++;
      return (NULL);
    }
    
    to = (ticks == -1) ? LW_OPTION_WAIT_INFINITE : (ULONG)ticks;
    if (API_SemaphoreBPend(zcpool->sem, to)) {
      zc_buf_error++;
      return (NULL);
    }
  } while (1);
  
  zcpbuf = zcpool->free;
  zcpool->free = zcpbuf->b.next;
  zcpool->free_cnt--;
  zc_buf_used++;
  if (zc_buf_used > zc_buf_max) {
    zc_buf_max = zc_buf_used;
  }
  LW_SPIN_UNLOCK_QUICK(&zc_buf_sl.SLCA_sl, level);
  
  zcpbuf->b.cpbuf.custom_free_function = netdev_zc_pbuf_free_cb;
  
  ret = pbuf_alloced_custom(PBUF_RAW, (u16_t)zcpool->pbuf_len, PBUF_POOL, &zcpbuf->b.cpbuf, 
                            (char *)zcpbuf + ZCPBUF_HEADER_SIZE,
                            (u16_t)zcpool->pbuf_len);
                            
  LWIP_ASSERT("netdev_zc_pbuf_alloc: bad pbuf", ret);
  
  pbuf_header(ret, (u16_t)-reserve);
  
  return (ret);
}

/* netdev input 'zero copy' buffer get a blk
 * reserve: ETH_PAD_SIZE + SIZEOF_VLAN_HDR size.
 * ticks = 0  no wait
 *       = -1 wait forever */
struct pbuf *netdev_zc_pbuf_alloc (void *hzcpool, int ticks)
{
  return (netdev_zc_pbuf_alloc_res(hzcpool, ticks, 0));
}

/* free zero copy pbuf */
void netdev_zc_pbuf_free (struct pbuf *p)
{
  pbuf_free(p);
}

/* get zero copy pbuf stat */
void netdev_zc_pbuf_stat (u32_t *zcused, u32_t *zcmax, u32_t *zcerror)
{
  if (zcused) {
    *zcused = zc_buf_used;
  }

  if (zcmax) {
    *zcmax = zc_buf_max;
  }

  if (zcerror) {
    *zcerror = zc_buf_error;
  }
}

#endif /* LW_CFG_NET_DEV_ZCBUF_EN > 0 */
/*
 * end
 */
