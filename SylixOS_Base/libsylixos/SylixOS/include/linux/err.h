#ifndef __LINUX_ERR_H
#define __LINUX_ERR_H

#include <errno.h>
#include "compat.h"

#ifndef __must_check
#ifdef __GNUC__
#define __must_check   __attribute__ ((warn_unused_result))
#else
#define __must_check
#endif
#endif

/*
 * Kernel pointers have redundant information, so we can use a
 * scheme where we can return either an error code or a dentry
 * pointer with the same return value.
 *
 * This should be a per-architecture thing, to allow different
 * error and pointer decisions.
 */
#define MAX_ERRNO	ERRMAX

#ifndef __ASSEMBLY__

#define IS_ERR_VALUE(x) unlikely((x) >= (unsigned long)-MAX_ERRNO)

static LW_INLINE void *ERR_PTR(long error)
{
	return (void *) error;
}

static LW_INLINE long __must_check PTR_ERR(const void *ptr)
{
	return (long) ptr;
}

static LW_INLINE long __must_check IS_ERR(const void *ptr)
{
	return IS_ERR_VALUE((unsigned long)ptr);
}

static inline BOOL __must_check IS_ERR_OR_NULL(const void *ptr)
{
    return unlikely(!ptr) || IS_ERR_VALUE((unsigned long)ptr);
}

static LW_INLINE int __must_check PTR_ERR_OR_ZERO(const void *ptr)
{
    if (IS_ERR(ptr))
        return PTR_ERR(ptr);
    else
        return 0;
}

#endif

#endif /* __LINUX_ERR_H */
