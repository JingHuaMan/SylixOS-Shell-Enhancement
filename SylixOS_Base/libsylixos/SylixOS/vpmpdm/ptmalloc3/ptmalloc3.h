/*
 * SylixOS(TM)  LW : long wing
 * Copyright All Rights Reserved
 *
 * MODULE PRIVATE DYNAMIC MEMORY IN VPROCESS PATCH
 * this file is support current module malloc/free use his own heap in a vprocess.
 *
 * Author: Jiao.jinxing <jiaojixing1987@gmail.com>
 */


#ifndef __PTMALLOC3_H
#define __PTMALLOC3_H

void *ptmalloc(size_t bytes);
void *ptmemalign(size_t alignment, size_t bytes);
void *ptrealloc(void *oldmem, size_t bytes);
void  ptfree(void *mem);

#endif /* __PTMALLOC3_H */
