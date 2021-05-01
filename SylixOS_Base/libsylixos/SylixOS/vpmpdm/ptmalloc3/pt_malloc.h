/*
 * SylixOS(TM)  LW : long wing
 * Copyright All Rights Reserved
 *
 * MODULE PRIVATE DYNAMIC MEMORY IN VPROCESS PATCH
 * this file is support current module malloc/free use his own heap in a vprocess.
 *
 * Author: Jiao.jinxing <jiaojixing1987@gmail.com>
 */

#ifndef __PT_MALLOC_H
#define __PT_MALLOC_H

#include "ptmalloc3.h"

void  ptmalloc_sylixos_init(PLW_CLASS_HEAP  pheap, void  *mem, size_t  size);

/*
 * update memory infomation
 */
#define VP_MEM_INFO(pheap)

/*
 * build a memory management
 */
#define VP_MEM_CTOR(pheap, mem, size) ptmalloc_sylixos_init(pheap, mem, size)

/*
 * destory memory management
 */
#define VP_MEM_DTOR(pheap) _HeapDtor(pheap, FALSE)

/*
 * add memory to management
 */
#define VP_MEM_ADD(pheap, mem, size) _HeapAddMemory(pheap, mem, size)

/*
 * allocate memory
 */
static LW_INLINE void *VP_MEM_ALLOC (PLW_CLASS_HEAP  pheap, size_t nbytes)
{
    REGISTER void *ret = ptmalloc(nbytes);

    if (ret) {
        _HeapTraceAlloc(pheap, ret, nbytes, "mem alloc");
    }

    return  (ret);
}

/*
 * allocate memory align
 */
static LW_INLINE void *VP_MEM_ALLOC_ALIGN (PLW_CLASS_HEAP pheap, size_t nbytes, size_t align)
{
    REGISTER void *ret = ptmemalign(align, nbytes);

    if (ret) {
        _HeapTraceAlloc(pheap, ret, nbytes, "mem align");
    }

    return  (ret);
}

/*
 * re-allocate memory
 */
static LW_INLINE void *VP_MEM_REALLOC (PLW_CLASS_HEAP pheap, void *ptr, size_t new_size, int do_check)
{
    REGISTER void *ret = ptrealloc(ptr, new_size);

    if (ptr) {
        if (ptr != ret) {
            _HeapTraceFree(pheap, ptr);
            _HeapTraceAlloc(pheap, ret, new_size, "mem realloc");
        }
    } else {
        _HeapTraceAlloc(pheap, ret, new_size, "mem realloc");
    }

    return  (ret);
}

/*
 * free memory
 */
#define VP_MEM_FREE(pheap, ptr, do_check)   \
        do {    \
            _HeapTraceFree(pheap, ptr); \
            ptfree(ptr);    \
        } while (0)

#endif /* __PT_MALLOC_H */

/*
 * end
 */
