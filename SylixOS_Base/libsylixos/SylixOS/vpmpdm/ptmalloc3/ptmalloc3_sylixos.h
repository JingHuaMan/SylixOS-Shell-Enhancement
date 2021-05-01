/*
 * SylixOS(TM)  LW : long wing
 * Copyright All Rights Reserved
 *
 * MODULE PRIVATE DYNAMIC MEMORY IN VPROCESS PATCH
 * this file is support current module malloc/free use his own heap in a vprocess.
 *
 * Author: Jiao.jinxing <jiaojixing1987@gmail.com>
 */

#ifndef __PTMALLOC3_SYLIXOS_H
#define __PTMALLOC3_SYLIXOS_H

/*
 * sylixos config
 */
#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <sys/mman.h>

void *ptmalloc_sbrk(int  size);
void  ptmalloc_abort(void);

/* use pt prefix */
#define USE_DL_PREFIX           1

/* big page size */
#define malloc_getpagesize      ((size_t)16U * (size_t)1024U)

/* Use the supplied emulation of sbrk */
#define MORECORE                ptmalloc_sbrk
#define MORECORE_CONTIGUOUS     0
#define MORECORE_FAILURE        ((void*)(-1))
#define MORECORE_CANNOT_TRIM    1

/* Have mmap to allocate large memory */
#if LW_CFG_VMM_EN > 0
#define HAVE_MMAP               1
#define HAVE_MREMAP             1

#define DEFAULT_MMAP_THRESHOLD  (128 * 1024)    /* 128K */
#endif /* LW_CFG_VMM_EN > 0 */

/* Abort */
#define ABORT                   ptmalloc_abort()

/* Use realloc 0 as free */
#define REALLOC_ZERO_BYTES_FREES   1

#endif /* __PTMALLOC3_SYLIXOS_H */
