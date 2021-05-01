/*
 * SylixOS(TM)  LW : long wing
 * Copyright All Rights Reserved
 *
 * MODULE PRIVATE DYNAMIC MEMORY IN VPROCESS PATCH
 * this file is support current module malloc/free use his own heap in a vprocess.
 * 
 * Author: Han.hui <sylixos@gmail.com>
 */

#define  __SYLIXOS_STDIO
#define  __SYLIXOS_KERNEL /* need some kernel function */
#include <sys/cdefs.h>
#include <unistd.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <dlfcn.h>
#include <errno.h>
#include <assert.h>
#include <wchar.h>
#include <pthread.h>

/*
 * select heap memory algorithm.
 */
#if LW_CFG_VP_HEAP_ALGORITHM == 0
#include "orig/orig_malloc.h"
#define MALLOC_ALGORITHM "orig-malloc"
#define NEED_CALL_SBRK 1

#elif LW_CFG_VP_HEAP_ALGORITHM == 1
#include "tlsf/tlsf_malloc.h"
#define MALLOC_ALGORITHM "tlsf-malloc"
#define NEED_CALL_SBRK 1

#elif LW_CFG_VP_HEAP_ALGORITHM == 2
#include "dlmalloc/dl_malloc.h"
#define MALLOC_ALGORITHM "dl-malloc"
#define NEED_CALL_SBRK 0

#elif LW_CFG_VP_HEAP_ALGORITHM == 3
#include "ptmalloc3/pt_malloc.h"
#define MALLOC_ALGORITHM "pt-malloc"
#define NEED_CALL_SBRK 0

#else
#error LW_CFG_VP_HEAP_ALGORITHM set error.
#endif /* LW_CFG_VP_HEAP_ALGORITHM */

/*
 * fixed gcc for arm.
 */
#ifdef LW_CFG_CPU_ARCH_ARM
#include "./loader/include/loader_lib.h" /* need _Unwind_Ptr */
#endif /* LW_CFG_CPU_ARCH_ARM */

/*
 * fixed gcc for PowerPC.
 */
#ifdef LW_CFG_CPU_ARCH_PPC
#include "./loader/include/loader_lib.h" /* need __eabi */
#endif /* LW_CFG_CPU_ARCH_PPC */

#define __VP_PATCH_VERSION      "2.1.5" /* vp patch version */

/*
 * fixed gcc old version.
 */
#ifdef __GNUC__
#if __GNUC__ < 2  || (__GNUC__ == 2 && __GNUC_MINOR__ < 5)
#error must use GCC include "__attribute__((...))" function.
#endif /*  __GNUC__ < 2  || ...        */
#endif /*  __GNUC__                    */

/*
 * fixed error handle.
 */
#if (LW_CFG_DEVICE_EN > 0) && (LW_CFG_FIO_LIB_EN > 0)
#define __LIB_PERROR(s) perror(s)
#else
#define __LIB_PERROR(s)
#endif /*  (LW_CFG_DEVICE_EN > 0)...   */

/*
 * config orig heap memory algorithm safety check.
 */
#if LW_CFG_USER_HEAP_SAFETY
#define __HEAP_CHECK    TRUE
#else
#define __HEAP_CHECK    FALSE
#endif /* LW_CFG_USER_HEAP_SAFETY */

/*
 *  atexit node
 */
typedef struct exit_node {
    struct exit_node *next; /* next atexit node */
    void (*func)(void); /* atexit function */
} exit_node;

/*
 *  vprocess context
 */
#define MAX_MEM_BLKS 16

typedef struct vp_ctx {
    LW_OBJECT_HANDLE  locker; /* vpmpdm lock */
    LW_CLASS_HEAP  heap; /* this private heap */
    size_t  pagesize; /* vmm page size */
    size_t  blksize; /* vmm allocate size per time */
    int memdirect; /* vmm direct allocate memory */
    int memcontin; /* vmm continuous physical memory */
    void  *vmem[MAX_MEM_BLKS]; /* vmm memory block pointer */
    void  *proc; /* this process */
    int alloc_en; /* allocate memory is enable */
    exit_node *atexit_header; /* atexit node list */
} vp_ctx;

static vp_ctx ctx = {
    .pagesize = 4096, /* default vmm page size */
    .blksize = 8192, /* default vmm block memory size */
    .memdirect = 0, /* default vmm do not direct allocate */
    .memcontin = 0, /* default no continuous physical memory */
};

/*
 * lock & unlock vpmpdm
 */
void __vp_patch_lock (void)
{
    API_SemaphoreMPend(ctx.locker, LW_OPTION_WAIT_INFINITE);
}

void __vp_patch_unlock (void)
{
    API_SemaphoreMPost(ctx.locker);
}

 
/*
 *  libgcc support
 */
#ifdef LW_CFG_CPU_ARCH_ARM
/* For a given PC, find the .so that it belongs to.
 * Returns the base address of the .ARM.exidx section
 * for that .so, and the number of 8-byte entries
 * in that section (via *pcount).
 *
 * libgcc declares __gnu_Unwind_Find_exidx() as a weak symbol, with
 * the expectation that libc will define it and call through to
 * a differently-named function in the dynamic linker.
 */
_Unwind_Ptr __gnu_Unwind_Find_exidx (_Unwind_Ptr pc, int *pcount)
{
    return dl_unwind_find_exidx(pc, pcount, ctx.proc);
}

/*
 * handle div zero
 */
void __aeabi_idiv0(void)
{
    fprintf(stderr, "Div by ZERO, abort!\n");
    lib_abort();
}

/*
 * handle div zero
 */
void __aeabi_ldiv0(void)
{
    fprintf(stderr, "Div by ZERO, abort!\n");
    lib_abort();
}
#endif /* LW_CFG_CPU_ARCH_ARM */

#ifdef LW_CFG_CPU_ARCH_PPC
/* a simple __eabi() that initializes registers GPR2 and GPR13.
 *
 * The symbols _SDA_BASE and _SDA2_BASE are defined during linking.
 * They specify the locations of the small data areas.
 * A program must load these values into GPR13 and GPR2, respectively, before accessing program data.
 * The small data areas contain part of the data of the executable.
 * They hold a number of variables that can be accessed within a 16-bit signed offset
 * of _SDA_BASE or _SDA2_BASE.
 * References to these variables are performed through references to GPR13 and GPR2 by the user program.
 * Typically, the small data areas contain program variables that are less than or
 * equal to 8 bytes in size, although this differs by compiler. The variables in SDA2 are read-only.
 */
void __eabi (void)
{
    if (ctx.proc) {
        LW_LD_EXEC_MODULE *pmodule = _LIST_ENTRY(((LW_LD_VPROC *)ctx.proc)->VP_ringModules,
                                                 LW_LD_EXEC_MODULE, EMOD_ringModules);
        if (pmodule) {
            addr_t  _SDA_BASE_  = (addr_t)API_ModuleSym(pmodule, "_SDA_BASE_");
            addr_t  _SDA2_BASE_ = (addr_t)API_ModuleSym(pmodule, "_SDA2_BASE_");

            asm volatile ("mr 2,  %0" :: "r"(_SDA2_BASE_));
            asm volatile ("mr 13, %0" :: "r"(_SDA_BASE_));
        }
    }
}
#endif /* LW_CFG_CPU_ARCH_PPC */

#if !defined(LW_CFG_CPU_ARCH_ARM) && !defined(LW_CFG_CPU_ARCH_C6X)
/*
 * Unwinding stack frames for c++ exception handling support. (libstdc++ Need!)
 * Each process needs to include the following variables.
 * These variables are defined in stdc++ library in before,
 * Because the shared libraries in the process may have introduced libstdc++ static libraries many times.
 * So move the definition of variables from libstdc++ to here.
 */
void           *__gnu_unwind_unseen_objects;
void           *__gnu_unwind_seen_objects;
pthread_mutex_t __gnu_unwind_object_mutex = PTHREAD_MUTEX_INITIALIZER;
#endif

/*
 *  get vp patch version
 */
char *__vp_patch_version (void *pvproc)
{
    (void)pvproc;

    return  (__VP_PATCH_VERSION " " MALLOC_ALGORITHM);
}

/*
 *  get vp patch heap
 */
void *__vp_patch_heap (void *pvproc)
{
    VP_MEM_INFO(&ctx.heap);
    return  ((void *)&ctx.heap);
}

/*
 *  get vp vmem heap
 */
int __vp_patch_vmem (void *pvproc, void **pvmem, int size)
{
    int  i, cnt = 0;
    
    for (i = 0; i < MAX_MEM_BLKS; i++) {
        if (cnt >= size) {
            break;
        }
        if (ctx.vmem[i]) {
            pvmem[cnt] = ctx.vmem[i];
            cnt++;
        }
    }
    
    return  (cnt);
}

/*
 *  init vp patch process 
 *  (sylixos load this module will call this function first)
 */
void __vp_patch_ctor (void *pvproc, PVOIDFUNCPTR *ppfuncMalloc, VOIDFUNCPTR *ppfuncFree)
{
    char buf[12] = "8192"; /* default 8192 pages */

    if (ctx.proc) {
        return;
    }

    ctx.proc = pvproc; /* save this process handle, dlopen will use this */

    if (API_TShellVarGetRt("SO_MEM_DIRECT", buf, sizeof(buf)) > 0) { /* must not use getenv() */
        ctx.memdirect = lib_atoi(buf);
    }

    if (API_TShellVarGetRt("SO_MEM_CONTIN", buf, sizeof(buf)) > 0) { /* must not use getenv() */
        ctx.memcontin = lib_atoi(buf);
        if (ctx.memcontin) {
            ctx.memdirect = 1;
        }
    }

#if LW_CFG_VMM_EN > 0
    ctx.pagesize = (size_t)getpagesize();
#endif /* LW_CFG_VMM_EN > 0 */

    if (API_TShellVarGetRt("SO_MEM_MBYTES", buf, sizeof(buf)) > 0) { /* must not use getenv() */
        ctx.blksize = (size_t)lib_atol(buf);
        if (ctx.blksize < 0) {
            return;
        }
        ctx.blksize *= LW_CFG_MB_SIZE;

    } else {
        if (API_TShellVarGetRt("SO_MEM_PAGES", buf, sizeof(buf)) > 0) { /* must not use getenv() */
            ctx.blksize = (size_t)lib_atol(buf);
            if (ctx.blksize < 0) {
                return;
            }
        }
        ctx.blksize *= ctx.pagesize; /* blocks * pages */
    }

    ctx.locker = API_SemaphoreMCreate("vp_lock", LW_PRIO_DEF_CEILING,
                                      LW_OPTION_WAIT_PRIORITY |
                                      LW_OPTION_INHERIT_PRIORITY | 
                                      LW_OPTION_DELETE_SAFE |
                                      LW_OPTION_OBJECT_GLOBAL, LW_NULL);
    if (!ctx.locker) {
        fprintf(stderr, "WARNING: ctx.locker create error!\n");
    }

#if LW_CFG_VMM_EN > 0
    if (ctx.memdirect) {
        if (ctx.memcontin) {
            ctx.vmem[0] = vmmMallocEx(ctx.blksize, LW_VMM_FLAG_RDWR | LW_VMM_FLAG_PHY_CONTINUOUS);
        } else {
            ctx.vmem[0] = vmmMalloc(ctx.blksize);
        }
    } else {
        ctx.vmem[0] = vmmMallocArea(ctx.blksize, NULL, NULL);
    }
#else
    ctx.vmem[0] = __SHEAP_ALLOC(ctx.blksize);
#endif /* LW_CFG_VMM_EN > 0 */

    if (ctx.vmem[0]) {
        lib_itoa(getpid(), ctx.heap.HEAP_cHeapName, 10);
        VP_MEM_CTOR(&ctx.heap, ctx.vmem[0], ctx.blksize);
        ctx.alloc_en = 1;

    } else {
        fprintf(stderr, "WARNING: ctx.vmem create error!\n");
    }
    
    if (ppfuncMalloc) {
       *ppfuncMalloc = lib_malloc;
    }

    if (ppfuncFree) {
       *ppfuncFree = lib_free;
    }
}

/*
 *  deinit vp patch process 
 *  (sylixos unload this module will call this function last)
 */
void __vp_patch_dtor (void *pvproc)
{
    int i;

    if (ctx.alloc_en) {
        VP_MEM_DTOR(&ctx.heap);
        for (i = 0; i < MAX_MEM_BLKS; i++) {
            if (ctx.vmem[i]) {
#if LW_CFG_VMM_EN > 0
                if (ctx.memdirect) {
                    vmmFree(ctx.vmem[i]);
                } else {
                    vmmFreeArea(ctx.vmem[i]); /* free all module private memory area */
                }
#else
                __SHEAP_FREE(ctx.vmem[i]);
#endif /* LW_CFG_VMM_EN > 0 */
                ctx.vmem[i] = NULL;
            }
        }
    }

    if (ctx.locker) {
        API_SemaphoreMDelete(&ctx.locker);
    }

    ctx.proc = NULL;
}

/*
 *  vp patch heap memory extern
 */
void __vp_patch_sbrk (BOOL lock)
{
    int i;
    volatile char  temp;

    if (ctx.alloc_en) {
        if (lock) {
            __vp_patch_lock();
        }

        for (i = 0; i < MAX_MEM_BLKS; i++) {
            if (ctx.vmem[i] == NULL) {
#if LW_CFG_VMM_EN > 0
                if (ctx.memdirect) {
                    if (ctx.memcontin) {
                        ctx.vmem[i] = vmmMallocEx(ctx.blksize, LW_VMM_FLAG_RDWR | LW_VMM_FLAG_PHY_CONTINUOUS);
                    } else {
                        ctx.vmem[i] = vmmMalloc(ctx.blksize);
                    }
                } else {
                    ctx.vmem[i] = vmmMallocArea(ctx.blksize, NULL, NULL);
                }
#else
                ctx.vmem[i] = __SHEAP_ALLOC(ctx.blksize);
#endif /* LW_CFG_VMM_EN > 0 */
                if (ctx.vmem[i]) {
                    temp = *(char *)ctx.vmem[i];    /* vmm alloc physical page */
                    VP_MEM_ADD(&ctx.heap, ctx.vmem[i], ctx.blksize);
                }
                break;
            }
        }

        if (lock) {
            __vp_patch_unlock();
        }
    }

    (void)temp; /* no warning for this variable */
}

/*
 *  __vp_patch_aerun
 *  (sylixos unload this module will call this function before __vp_patch_dtor)
 */
void __vp_patch_aerun (void)
{
    exit_node *temp;
    exit_node *next;
    
    temp = ctx.atexit_header;
    while (temp) {
        temp->func();
        next = temp->next;
        lib_free(temp);
        temp = next;
    }
    ctx.atexit_header = NULL;
}

/*
 *  atexit
 */
int atexit (void (*func)(void))
{
    exit_node *node;

    if (!func) {
        errno = EINVAL;
        return  (-1);
    }

    node = (exit_node *)lib_malloc(sizeof(exit_node));
    if (!node) {
        errno = ENOMEM;
        return  (-1);
    }

    node->func = func;

    __KERNEL_ENTER();
    node->next = ctx.atexit_header;
    ctx.atexit_header = node;
    __KERNEL_EXIT();

    return  (0);
}

/*
 *  pre-alloc physical pages (use page fault mechanism)
 */
void  __vp_pre_alloc_phy (const void *pmem, size_t nbytes, int mmap)
{
#if LW_CFG_VMM_EN > 0
    if (!ctx.memdirect || mmap) {
        unsigned long  algin = (unsigned long)pmem;
        unsigned long  end   = algin + nbytes - 1;
        volatile char  temp;

        if (!pmem || !nbytes) {
            return;
        }
    
        temp = *(char *)algin;

        algin |= (ctx.pagesize - 1);
        algin += 1;

        while (algin <= end) {
            temp = *(char *)algin;
            algin += ctx.pagesize;
        }

        (void)temp; /* no warning for this variable */
    }
#endif /* LW_CFG_VMM_EN > 0 */
}

/*
 * aligned_malloc
 */
void *aligned_malloc (size_t  bytes, size_t alignment)
{
    return  (lib_mallocalign(bytes, alignment));
}

/*
 * aligned_free
 */
void aligned_free (void *p)
{
    lib_free(p);
}

/*
 *  lib_malloc
 */
__weak_reference(lib_malloc, malloc);

void *lib_malloc (size_t  nbytes)
{
    int mextern = 0;
    void *pmem;
    
    nbytes = (nbytes) ? nbytes : 1;

    if (ctx.alloc_en) {
__re_try:
        pmem = VP_MEM_ALLOC(&ctx.heap, nbytes);
        if (pmem) {
            __vp_pre_alloc_phy(pmem, nbytes, 0);

        } else {
            if (mextern || !NEED_CALL_SBRK) {
                errno = ENOMEM;

            } else {
                mextern = 1;
                __vp_patch_sbrk(TRUE);
                goto    __re_try;
            }
        }
        return  (pmem);
    }

    return  (NULL);
}

/*
 *  lib_xmalloc
 */
__weak_reference(lib_xmalloc, xmalloc);

void *lib_xmalloc (size_t  nbytes)
{
    void  *ptr = lib_malloc(nbytes);
    
    if (ptr == NULL) {
        __LIB_PERROR("lib_xmalloc() process not enough memory");
        exit(0);
    }
    
    return  (ptr);
}

/*
 *  lib_mallocalign
 */
__weak_reference(lib_mallocalign, mallocalign);

void *lib_mallocalign (size_t  nbytes, size_t align)
{
    int mextern = 0;
    void *pmem;
    
    if (align & (align - 1)) {
        errno = EINVAL;
        return  (NULL);
    }
    
    nbytes = (nbytes) ? nbytes : 1;

    if (ctx.alloc_en) {
__re_try:
        pmem = VP_MEM_ALLOC_ALIGN(&ctx.heap, nbytes, align);
        if (pmem) {
            __vp_pre_alloc_phy(pmem, nbytes, 0);

        } else {
            if (mextern || !NEED_CALL_SBRK) {
                errno = ENOMEM;

            } else {
                mextern = 1;
                __vp_patch_sbrk(TRUE);
                goto    __re_try;
            }
        }
        return  (pmem);
    }

    return  (NULL);
}

/*
 * lib_xmallocalign
 */
__weak_reference(lib_xmallocalign, xmallocalign);

void *lib_xmallocalign (size_t  nbytes, size_t align)
{
    void  *ptr = lib_mallocalign(nbytes, align);
    
    if (ptr == NULL) {
        __LIB_PERROR("lib_xmallocalign() process not enough memory");
        exit(0);
    }
    
    return  (ptr);
}

/*
 *  lib_memalign
 */
__weak_reference(lib_memalign, memalign);

void *lib_memalign (size_t align, size_t  nbytes)
{
    return  (lib_mallocalign(nbytes, align));
}

/*
 * lib_xmemalign
 */
__weak_reference(lib_xmemalign, xmemalign);

void *lib_xmemalign (size_t align, size_t  nbytes)
{
    void  *ptr = lib_memalign(align, nbytes);
    
    if (ptr == NULL) {
        __LIB_PERROR("lib_xmemalign() process not enough memory");
        exit(0);
    }
    
    return  (ptr);
}

/*
 *  lib_free
 */
__weak_reference(lib_free, free);

void lib_free (void *ptr)
{
    if (ptr && ctx.alloc_en) {
        VP_MEM_FREE(&ctx.heap, ptr, __HEAP_CHECK);
    }
}

/*
 *  lib_calloc
 */
__weak_reference(lib_calloc, calloc);

void *lib_calloc (size_t  num, size_t  nbytes)
{
    size_t total = num * nbytes;
    void *pmem;
    
    pmem = lib_malloc(total);
    if (pmem) {
        lib_bzero(pmem, total);
    }
    
    return  (pmem);
}

/*
 *  lib_xcalloc
 */
__weak_reference(lib_xcalloc, xcalloc);

void *lib_xcalloc (size_t  num, size_t  nbytes)
{
    void  *ptr = lib_calloc(num, nbytes);
    
    if (ptr == NULL) {
        __LIB_PERROR("lib_xcalloc() process not enough memory");
        exit(0);
    }
    
    return  (ptr);
}

/*
 *  lib_realloc
 */
__weak_reference(lib_realloc, realloc);

void *lib_realloc (void *ptr, size_t  new_size)
{
    int mextern = 0;
    void *pmem;
    
    if (ctx.alloc_en) {
__re_try:
        pmem = VP_MEM_REALLOC(&ctx.heap, ptr, new_size, __HEAP_CHECK);
        if (pmem) {
            __vp_pre_alloc_phy(pmem, new_size, 0);

        } else {
            if (mextern || !NEED_CALL_SBRK) {
                errno = ENOMEM;

            } else {
                mextern = 1;
                __vp_patch_sbrk(TRUE);
                goto    __re_try;
            }
        }
        return  (pmem);
    }
    
    return  (NULL);
}

/*
 *  lib_xrealloc
 */
__weak_reference(lib_xrealloc, xrealloc);

void *lib_xrealloc (void *ptr, size_t  new_size)
{
    void  *new_ptr = lib_realloc(ptr, new_size);
    
    if ((new_ptr == NULL) && new_size) {
        __LIB_PERROR("lib_xrealloc() process not enough memory");
        exit(0);
    }
    
    return  (new_ptr);
}

/*
 *  lib_posix_memalign
 */
__weak_reference(lib_posix_memalign, posix_memalign);

int  lib_posix_memalign (void **memptr, size_t align, size_t size)
{
    if (memptr == NULL) {
        errno = EINVAL;
        return  (EINVAL);
    }

    if (align & (align - 1)) {
        errno = EINVAL;
        return  (EINVAL);
    }

    size = (size) ? size : 1;

    *memptr = lib_mallocalign(size, align);
    if (*memptr) {
        return  (ERROR_NONE);

    } else {
        errno = ENOMEM;
        return  (ENOMEM);
    }
}

/*
 *  lib_malloc_new
 */
__weak_reference(lib_malloc_new, malloc_new);

void *lib_malloc_new (size_t  nbytes)
{
    int mextern = 0;
    void *p;

    nbytes = (nbytes) ? nbytes : 1;

    if (ctx.alloc_en) {
__re_try:
        p = VP_MEM_ALLOC(&ctx.heap, nbytes);
        if (p) {
            __vp_pre_alloc_phy(p, nbytes, 0);

        } else {
            if (mextern || !NEED_CALL_SBRK) {
                __LIB_PERROR("process C++ new() not enough memory");
                errno = ENOMEM;

            } else {
                mextern = 1;
                __vp_patch_sbrk(TRUE);
                goto    __re_try;
            }
        }
        return  (p);
    }
    
    return  (NULL);
}

/*
 *  lib_strdup
 */
__weak_reference(lib_strdup, strdup);

char *lib_strdup (const char *str)
{
    char *mem = NULL;
    size_t size;

    if (str == NULL) {
        return  (NULL);
    }
    
    size = lib_strlen(str);
    mem = (char *)lib_malloc(size + 1);
    if (mem) {
        lib_strcpy(mem, str);
    }
    
    return  (mem);
}

/*
 *  lib_xstrdup
 */
__weak_reference(lib_xstrdup, xstrdup);

char *lib_xstrdup (const char *str)
{
    char *str_ret = lib_strdup(str);
    
    if (str_ret == NULL) {
        __LIB_PERROR("lib_xstrdup() process not enough memory");
        exit(0);
    }
    
    return  (str_ret);
}

/*
 *  lib_strndup
 */
__weak_reference(lib_strndup, strndup);

char *lib_strndup (const char *str, size_t size)
{
    size_t len;
    char *news;
    
    if (str == NULL) {
        return  (NULL);
    }
    
    len = lib_strnlen(str, size);
    news = (PCHAR)lib_malloc(len + 1);
    if (news == NULL) {
        return  (NULL);
    }
    
    news[len] = PX_EOS;
    
    return  (lib_memcpy(news, str, len));
}

/*
 *  lib_xstrndup
 */
__weak_reference(lib_xstrndup, xstrndup);

char *lib_xstrndup (const char *str, size_t size)
{
    char *str_ret = lib_strndup(str, size);
    
    if (str_ret == NULL) {
        __LIB_PERROR("lib_xstrndup() process not enough memory");
        exit(0);
    }
    
    return  (str_ret);
}

/*
 * getpass
 */
char *getpass (const char *prompt)
{
    static char cPass[PASS_MAX + 1];
    
    return getpass_r(prompt, cPass, sizeof(cPass));
}

/*
 * end
 */
