/**
 * @file
 * Lwip use tlsf memory management.
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

#define __SYLIXOS_KERNEL
#include "SylixOS.h"
#include "lwip/opt.h"

#if LW_CFG_LWIP_MEM_TLSF > 0

#include "lwip/mem.h"
#include "lwip/stats.h"
#include "tlsf.h"

static tlsf_t tlsf_mem; /* tlsf memory control */
static spinlock_t tlsf_lock; /* tlsf memory spinlock */
static uint_t brk_times;

/* create a memory pool */
void tlsf_mem_create (void)
{
  void *mem;

  LW_SPIN_INIT(&tlsf_lock);

  mem = __SHEAP_ALLOC(MEM_SIZE);
  _BugHandle(!mem, TRUE, "tlsf_mem_create() fail!\r\n");

  tlsf_mem = tlsf_create_with_pool(mem, MEM_SIZE);
  _BugHandle(!tlsf_mem, TRUE, "tlsf_mem_create() fail!\r\n");
  
#if LW_CFG_LWIP_MEM_TLSF_BRK > 0
  MEM_STATS_AVAIL(avail, LWIP_MEM_ALIGN_SIZE(MEM_SIZE) * (LW_CFG_LWIP_MEM_TLSF_BRK + 1));
#else
  MEM_STATS_AVAIL(avail, LWIP_MEM_ALIGN_SIZE(MEM_SIZE));
#endif
}

#if LW_CFG_CPU_WORD_LENGHT == 64

/* tlsf malloc */
void *tlsf_mem_malloc (size_t size)
{
  void *ret;

#if LW_CFG_LWIP_MEM_TLSF_BRK > 0
  void *mem;
  
__retry:
  LW_SPIN_LOCK_TASK(&tlsf_lock);
  ret = tlsf_malloc(tlsf_mem, size);
  if (ret) {
    MEM_STATS_INC_USED(used, size);
    LW_SPIN_UNLOCK_TASK(&tlsf_lock);
    return (ret);
  
  } else if (brk_times >= LW_CFG_LWIP_MEM_TLSF_BRK_TIMES) {
    MEM_STATS_INC(err);
    LW_SPIN_UNLOCK_TASK(&tlsf_lock);
    return (NULL);
  }
  brk_times++;
  LW_SPIN_UNLOCK_TASK(&tlsf_lock); /* TODO: here unlock maybe preemptive! */
  
  mem = __SHEAP_ALLOC(MEM_SIZE);
  if (mem) {
    LW_SPIN_LOCK_TASK(&tlsf_lock);
    tlsf_add_pool(tlsf_mem, mem, MEM_SIZE);
    LW_SPIN_UNLOCK_TASK(&tlsf_lock);
    goto __retry;
  }

#else
  LW_SPIN_LOCK_TASK(&tlsf_lock);
  ret = tlsf_malloc(tlsf_mem, size);
  if (ret) {
    MEM_STATS_INC_USED(used, size);
  } else {
    MEM_STATS_INC(err);
  }
  LW_SPIN_UNLOCK_TASK(&tlsf_lock);
#endif

  return (ret);
}

#else /* LW_CFG_CPU_WORD_LENGHT == 64 */

#define TLSF_ALIGN_PADDING_MARK 0xffaa5500
#define TLSF_ALIGN_PADDING_SIZE sizeof(size_t)

/* tlsf malloc */
void *tlsf_mem_malloc (size_t size)
{
  void *ret;

#if LW_CFG_LWIP_MEM_TLSF_BRK > 0
  void *mem;

__retry:
  LW_SPIN_LOCK_TASK(&tlsf_lock);
  ret = tlsf_malloc(tlsf_mem, size + TLSF_ALIGN_PADDING_SIZE);
  if (ret) {
    MEM_STATS_INC_USED(used, size);
    LW_SPIN_UNLOCK_TASK(&tlsf_lock);
    if ((addr_t)ret & 0x7) { /* Not 64bits align */
      *(size_t *)ret = TLSF_ALIGN_PADDING_MARK;
      ret = (void *)((addr_t)ret + TLSF_ALIGN_PADDING_SIZE);
    }
    return (ret);
    
  } else if (brk_times >= LW_CFG_LWIP_MEM_TLSF_BRK_TIMES) {
    MEM_STATS_INC(err);
    LW_SPIN_UNLOCK_TASK(&tlsf_lock);
    return (NULL);
  }
  brk_times++;
  LW_SPIN_UNLOCK_TASK(&tlsf_lock); /* TODO: here unlock maybe preemptive! */

  mem = __SHEAP_ALLOC(MEM_SIZE);
  if (mem) {
    LW_SPIN_LOCK_TASK(&tlsf_lock);
    tlsf_add_pool(tlsf_mem, mem, MEM_SIZE);
    LW_SPIN_UNLOCK_TASK(&tlsf_lock);
    goto __retry;
  }

#else
  LW_SPIN_LOCK_TASK(&tlsf_lock);
  ret = tlsf_malloc(tlsf_mem, size + TLSF_ALIGN_PADDING_SIZE);
  if (ret) {
    MEM_STATS_INC_USED(used, size);
  } else {
    MEM_STATS_INC(err);
  }
  LW_SPIN_UNLOCK_TASK(&tlsf_lock);

  if (ret && ((addr_t)ret & 0x7)) { /* Not 64bits align */
    *(size_t *)ret = TLSF_ALIGN_PADDING_MARK;
    ret = (void *)((addr_t)ret + TLSF_ALIGN_PADDING_SIZE);
  }
#endif

  return (ret);
}

#endif /* LW_CFG_CPU_WORD_LENGHT == 32 */

/* tlsf calloc */
void *tlsf_mem_calloc (size_t count, size_t size)
{
  void *ret;
  
  ret = tlsf_mem_malloc(count * size);
  if (ret) {
    lib_bzero(ret, count * size);
  }
  
  return (ret);
}

/* tlsf free */
void tlsf_mem_free (void *f, mem_size_t size)
{
#if LW_CFG_CPU_WORD_LENGHT != 64
  if (f) {
    if ((*(size_t *)((addr_t)f - TLSF_ALIGN_PADDING_SIZE) == TLSF_ALIGN_PADDING_MARK)) {
      f = (void *)((addr_t)f - TLSF_ALIGN_PADDING_SIZE);
    }
  }
#endif /* LW_CFG_CPU_WORD_LENGHT == 32 */

  LW_SPIN_LOCK_TASK(&tlsf_lock);
  MEM_STATS_DEC_USED(used, size);
  tlsf_free(tlsf_mem, f);
  LW_SPIN_UNLOCK_TASK(&tlsf_lock);
}

#endif /* LW_CFG_LWIP_MEM_TLSF */
/*
 * end
 */
