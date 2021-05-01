/* vi: set sw=4 ts=4: */
/*
 * ascii-to-numbers implementations for busybox
 *
 * Copyright (C) 2003  Manuel Novoa III  <mjn3@codepoet.org>
 *
 * Licensed under GPLv2, see file LICENSE in this source tree.
 */

#ifndef __DD_XATONUM_H
#define __DD_XATONUM_H

/* Provides extern declarations of functions */
#define DECLARE_STR_CONV(type, T, UT) \
\
unsigned type xstrto##UT##_range_sfx(const char *str, int b, unsigned type l, unsigned type u, const struct suffix_mult *sfx) FAST_FUNC; \
unsigned type xstrto##UT##_range(const char *str, int b, unsigned type l, unsigned type u) FAST_FUNC; \
unsigned type xstrto##UT##_sfx(const char *str, int b, const struct suffix_mult *sfx) FAST_FUNC; \
unsigned type xstrto##UT(const char *str, int b) FAST_FUNC; \
unsigned type xato##UT##_range_sfx(const char *str, unsigned type l, unsigned type u, const struct suffix_mult *sfx) FAST_FUNC; \
unsigned type xato##UT##_range(const char *str, unsigned type l, unsigned type u) FAST_FUNC; \
unsigned type xato##UT##_sfx(const char *str, const struct suffix_mult *sfx) FAST_FUNC; \
unsigned type xato##UT(const char *str) FAST_FUNC; \
type xstrto##T##_range_sfx(const char *str, int b, type l, type u, const struct suffix_mult *sfx) FAST_FUNC; \
type xstrto##T##_range(const char *str, int b, type l, type u) FAST_FUNC; \
type xstrto##T(const char *str, int b) FAST_FUNC; \
type xato##T##_range_sfx(const char *str, type l, type u, const struct suffix_mult *sfx) FAST_FUNC; \
type xato##T##_range(const char *str, type l, type u) FAST_FUNC; \
type xato##T##_sfx(const char *str, const struct suffix_mult *sfx) FAST_FUNC; \
type xato##T(const char *str) FAST_FUNC; \

/* Unsigned long long functions always exist */
DECLARE_STR_CONV(long long, ll, ull)


/* Provides inline definitions of functions */
/* (useful for mapping them to the type of the same width) */
#define DEFINE_EQUIV_STR_CONV(narrow, N, W, UN, UW) \
\
static ALWAYS_INLINE \
unsigned narrow xstrto##UN##_range_sfx(const char *str, int b, unsigned narrow l, unsigned narrow u, const struct suffix_mult *sfx) \
{ return xstrto##UW##_range_sfx(str, b, l, u, sfx); } \
static ALWAYS_INLINE \
unsigned narrow xstrto##UN##_range(const char *str, int b, unsigned narrow l, unsigned narrow u) \
{ return xstrto##UW##_range(str, b, l, u); } \
static ALWAYS_INLINE \
unsigned narrow xstrto##UN##_sfx(const char *str, int b, const struct suffix_mult *sfx) \
{ return xstrto##UW##_sfx(str, b, sfx); } \
static ALWAYS_INLINE \
unsigned narrow xstrto##UN(const char *str, int b) \
{ return xstrto##UW(str, b); } \
static ALWAYS_INLINE \
unsigned narrow xato##UN##_range_sfx(const char *str, unsigned narrow l, unsigned narrow u, const struct suffix_mult *sfx) \
{ return xato##UW##_range_sfx(str, l, u, sfx); } \
static ALWAYS_INLINE \
unsigned narrow xato##UN##_range(const char *str, unsigned narrow l, unsigned narrow u) \
{ return xato##UW##_range(str, l, u); } \
static ALWAYS_INLINE \
unsigned narrow xato##UN##_sfx(const char *str, const struct suffix_mult *sfx) \
{ return xato##UW##_sfx(str, sfx); } \
static ALWAYS_INLINE \
unsigned narrow xato##UN(const char *str) \
{ return xato##UW(str); } \
static ALWAYS_INLINE \
narrow xstrto##N##_range_sfx(const char *str, int b, narrow l, narrow u, const struct suffix_mult *sfx) \
{ return xstrto##W##_range_sfx(str, b, l, u, sfx); } \
static ALWAYS_INLINE \
narrow xstrto##N##_range(const char *str, int b, narrow l, narrow u) \
{ return xstrto##W##_range(str, b, l, u); } \
static ALWAYS_INLINE \
narrow xstrto##N(const char *str, int b) \
{ return xstrto##W(str, b); } \
static ALWAYS_INLINE \
narrow xato##N##_range_sfx(const char *str, narrow l, narrow u, const struct suffix_mult *sfx) \
{ return xato##W##_range_sfx(str, l, u, sfx); } \
static ALWAYS_INLINE \
narrow xato##N##_range(const char *str, narrow l, narrow u) \
{ return xato##W##_range(str, l, u); } \
static ALWAYS_INLINE \
narrow xato##N##_sfx(const char *str, const struct suffix_mult *sfx) \
{ return xato##W##_sfx(str, sfx); } \
static ALWAYS_INLINE \
narrow xato##N(const char *str) \
{ return xato##W(str); } \

/* If long == long long, then just map them one-to-one */
#if ULONG_MAX == ULLONG_MAX
DEFINE_EQUIV_STR_CONV(long, l, ll, ul, ull)
#else
/* Else provide extern defs */
DECLARE_STR_CONV(long, l, ul)
#endif

/* Same for int -> [long] long */
#if UINT_MAX == ULLONG_MAX
DEFINE_EQUIV_STR_CONV(int, i, ll, u, ull)
#elif UINT_MAX == ULONG_MAX
DEFINE_EQUIV_STR_CONV(int, i, l, u, ul)
#else
DECLARE_STR_CONV(int, i, u)
#endif

#if ENABLE_LFS
/* CONFIG_LFS is on */
# if ULONG_MAX > 0xffffffff
/* "long" is long enough on this system */
typedef unsigned long uoff_t;
#  define XATOOFF(a) xatoul_range((a), 0, LONG_MAX)
/* usage: sz = BB_STRTOOFF(s, NULL, 10); if (errno || sz < 0) die(); */
#  define BB_STRTOOFF bb_strtoul
#  define STRTOOFF strtoul
/* usage: printf("size: %"OFF_FMT"d (%"OFF_FMT"x)\n", sz, sz); */
#  define OFF_FMT "l"
# else
/* "long" is too short, need "long long" */
typedef unsigned long long uoff_t;
#  define XATOOFF(a) xatoull_range((a), 0, LLONG_MAX)
#  define BB_STRTOOFF bb_strtoull
#  define STRTOOFF strtoull
#  define OFF_FMT "ll"
# endif
#else
/* CONFIG_LFS is off */
# if UINT_MAX == 0xffffffff
/* While sizeof(off_t) == sizeof(int), off_t is typedef'ed to long anyway.
 * gcc will throw warnings on printf("%d", off_t). Crap... */
typedef unsigned long uoff_t;
#  define XATOOFF(a) xatoi_positive(a)
#  define BB_STRTOOFF bb_strtou
#  define STRTOOFF strtol
#  define OFF_FMT "l"
# else
typedef unsigned long uoff_t;
#  define XATOOFF(a) xatoul_range((a), 0, LONG_MAX)
#  define BB_STRTOOFF bb_strtoul
#  define STRTOOFF strtol
#  define OFF_FMT "l"
# endif
#endif

#endif /* __DD_XATONUM_H */
