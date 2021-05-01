/*********************************************************************************************************
**
**                                    中国软件开源组织
**
**                                   嵌入式实时操作系统
**
**                                SylixOS(TM)  LW : long wing
**
**                               Copyright All Rights Reserved
**
**--------------文件信息--------------------------------------------------------------------------------
**
** 文   件   名: x86_atomic.h
**
** 创   建   人: Jiao.Jinxing (焦进星)
**
** 文件创建日期: 2018 年 07 月 27 日
**
** 描        述: x86 体系构架 ATOMIC 接口.
*********************************************************************************************************/

#ifndef __ARCH_X86_ATOMIC_H
#define __ARCH_X86_ATOMIC_H

/*********************************************************************************************************
  ATOMIC
*********************************************************************************************************/
#if LW_CFG_CPU_ATOMIC_EN > 0

static LW_INLINE VOID  archAtomicSet (INT  i, atomic_t  *v)
{
    LW_ACCESS_ONCE(INT, v->counter) = i;
}

static LW_INLINE INT  archAtomicGet (atomic_t  *v)
{
    return  (LW_ACCESS_ONCE(INT, v->counter));
}

INT  archAtomicAdd(INT  i, atomic_t  *v);
INT  archAtomicSub(INT  i, atomic_t  *v);
INT  archAtomicAnd(INT  i, atomic_t  *v);
INT  archAtomicOr (INT  i, atomic_t  *v);
INT  archAtomicXor(INT  i, atomic_t  *v);

/*********************************************************************************************************
  atomic cas op
*********************************************************************************************************/

static LW_INLINE INT  archAtomicCas (atomic_t  *v, INT  iOld, INT  iNew)
{
    INT  iOldValue;

    __asm__ __volatile__("lock; cmpxchgl  %2, %1"
                        : "=a" (iOldValue), "+m" (v->counter)
                        : "r" (iNew), "0" (iOld)
                        : "memory");
    return  (iOldValue);
}

static LW_INLINE addr_t  archAtomicAddrCas (volatile addr_t *p, addr_t  ulOld, addr_t  ulNew)
{
    addr_t  ulOldValue;

#if LW_CFG_CPU_WORD_LENGHT == 64
    __asm__ __volatile__("lock; cmpxchgq  %2, %1"
                        : "=a" (ulOldValue), "+m" (*p)
                        : "r" (ulNew), "0" (ulOld)
                        : "memory");

#else                                                                   /*  LW_CFG_CPU_WORD_LENGHT 64   */
    __asm__ __volatile__("lock; cmpxchgl  %2, %1"
                        : "=a" (ulOldValue), "+m" (*p)
                        : "r" (ulNew), "0" (ulOld)
                        : "memory");
#endif                                                                  /*  LW_CFG_CPU_WORD_LENGHT 32   */

    return  (ulOldValue);
}

/*********************************************************************************************************
  ATOMIC64
*********************************************************************************************************/
#if LW_CFG_CPU_ATOMIC64_EN > 0
#if LW_CFG_CPU_WORD_LENGHT == 64

static LW_INLINE VOID   archAtomic64Set (INT64  i, atomic64_t  *v)
{
    LW_ACCESS_ONCE(INT64, v->counter) = i;
}

static LW_INLINE INT64  archAtomic64Get (atomic64_t  *v)
{
    return  (LW_ACCESS_ONCE(INT64, v->counter));
}

INT64  archAtomic64Add(INT64  i, atomic64_t  *v);
INT64  archAtomic64Sub(INT64  i, atomic64_t  *v);
INT64  archAtomic64And(INT64  i, atomic64_t  *v);
INT64  archAtomic64Or (INT64  i, atomic64_t  *v);
INT64  archAtomic64Xor(INT64  i, atomic64_t  *v);

/*********************************************************************************************************
  atomic64 cas op
*********************************************************************************************************/

static LW_INLINE INT64  archAtomic64Cas (atomic64_t  *v, INT64  i64Old, INT64  i64New)
{
    INT64  i64OldValue;

    __asm__ __volatile__("lock; cmpxchgq  %2, %1"
                        : "=a" (i64OldValue), "+m" (v->counter)
                        : "r" (i64New), "0" (i64Old)
                        : "memory");

    return  (i64OldValue);
}

#else                                                                   /*  LW_CFG_CPU_WORD_LENGHT 64   */

VOID  archAtomic64SetCx8(atomic64_t *, ...);
VOID  archAtomic64GetCx8(atomic64_t *, ...);
VOID  archAtomic64AddReturnCx8(atomic64_t *, ...);
VOID  archAtomic64SubReturnCx8(atomic64_t *, ...);

/*********************************************************************************************************
  use this macro(s) if you need more than one output parameter in alternative_io
*********************************************************************************************************/

#define __ASM_OUTPUT2(a...)                 a

/*********************************************************************************************************
  use this macro if you need clobbers but no inputs in alternative_{input,io,call}()
*********************************************************************************************************/

#define __ASM_NO_INPUT_CLOBBER(clbr...)     "i" (0) : clbr

#define __alternative_atomic64(f, g, out, in...)    \
    __asm__ __volatile__("call  %P[func]"           \
                         : out : [func] "i" (archAtomic64##g##Cx8), ## in)

#define alternative_atomic64(f, out, in...)         \
    __alternative_atomic64(f, f, __ASM_OUTPUT2(out), ## in)

static LW_INLINE VOID   archAtomic64Set (INT64  i, atomic64_t  *v)
{
    UINT32  uiHigh = (UINT32)(i >> 32);
    UINT32  uiLow  = (UINT32)i;

    alternative_atomic64(Set, /* no output */,
                         "S" (v), "b" (uiLow), "c" (uiHigh)
                         : "eax", "edx", "memory");
}

static LW_INLINE INT64  archAtomic64Get (atomic64_t  *v)
{
    INT64  i64Ret;

    alternative_atomic64(Get, "=&A" (i64Ret), "c" (v) : "memory");

    return  (i64Ret);
}

static LW_INLINE INT64  archAtomic64Add (INT64  i, atomic64_t  *v)
{
    alternative_atomic64(AddReturn,
                         __ASM_OUTPUT2("+A" (i), "+c" (v)),
                         __ASM_NO_INPUT_CLOBBER("memory"));

    return  (i);
}

static LW_INLINE INT64  archAtomic64Sub (INT64  i, atomic64_t  *v)
{
    alternative_atomic64(SubReturn,
                         __ASM_OUTPUT2("+A" (i), "+c" (v)),
                         __ASM_NO_INPUT_CLOBBER("memory"));

    return  (i);
}

#undef alternative_atomic64
#undef __alternative_atomic64
#undef __ASM_OUTPUT2
#undef __ASM_NO_INPUT_CLOBBER

/*********************************************************************************************************
  atomic64 cas op
*********************************************************************************************************/

static LW_INLINE INT64  archAtomic64Cas (atomic64_t  *v, INT64  i64Old, INT64  i64New)
{
    INT64  i64OldValue;

    __asm__ __volatile__("lock; cmpxchg8b  %1"
                         : "=A" (i64OldValue), "+m" (v->counter)
                         : "b" ((UINT32)i64New), "c" ((UINT32)(i64New >> 32)), "0" (i64Old)
                         : "memory");

    return  (i64OldValue);
}

static LW_INLINE INT64  archAtomic64And (INT64  i, atomic64_t  *v)
{
    INT64  i64Old, i64Temp = 0;

    while ((i64Old = archAtomic64Cas(v, i64Temp, i64Temp & i)) != i64Temp) {
        i64Temp = i64Old;
    }

    return  (i64Temp & i);
}

static LW_INLINE INT64  archAtomic64Or (INT64  i, atomic64_t  *v)
{
    INT64  i64Old, i64Temp = 0;

    while ((i64Old = archAtomic64Cas(v, i64Temp, i64Temp | i)) != i64Temp) {
        i64Temp = i64Old;
    }

    return  (i64Temp | i);
}

static LW_INLINE INT64  archAtomic64Xor(INT64  i, atomic64_t  *v)
{
    INT64  i64Old, i64Temp = 0;

    while ((i64Old = archAtomic64Cas(v, i64Temp, i64Temp ^ i)) != i64Temp) {
        i64Temp = i64Old;
    }

    return  (i64Temp ^ i);
}

#endif                                                                  /*  LW_CFG_CPU_WORD_LENGHT 32   */
#endif                                                                  /*  LW_CFG_CPU_ATOMIC64_EN      */
#endif                                                                  /*  LW_CFG_CPU_ATOMIC_EN        */
#endif                                                                  /*  __ARCH_X86_ATOMIC_H         */
/*********************************************************************************************************
  END
*********************************************************************************************************/
