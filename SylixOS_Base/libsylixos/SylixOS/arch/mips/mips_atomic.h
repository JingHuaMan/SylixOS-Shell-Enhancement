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
** 文   件   名: mips_atomic.h
**
** 创   建   人: Jiao.Jinxing (焦进星)
**
** 文件创建日期: 2018 年 07 月 27 日
**
** 描        述: MIPS 体系构架 ATOMIC 接口.
*********************************************************************************************************/

#ifndef __ARCH_MIPS_ATOMIC_H
#define __ARCH_MIPS_ATOMIC_H

/*********************************************************************************************************
  ATOMIC
*********************************************************************************************************/
#if LW_CFG_CPU_ATOMIC_EN > 0

#include "arch/assembler.h"

#define ATOMIC_OP_RETURN(op, c_op, asm_op)                                  \
static LW_INLINE INT  archAtomic##op (INT  i, atomic_t  *v)                 \
{                                                                           \
    INT  iTemp;                                                             \
    INT  iResult;                                                           \
                                                                            \
    KN_SMP_MB_BEFORE_ATOMIC();                                              \
                                                                            \
    do {                                                                    \
        __asm__ __volatile__(                                               \
            "   .set    push                        \n"                     \
            "   .set    noreorder                   \n"                     \
            KN_WEAK_LLSC_MB                                                 \
            "   ll      %1, %2                      \n"                     \
            "   " #asm_op " %0, %1, %3              \n"                     \
            "   sc      %0, %2                      \n"                     \
            "   .set    pop                         \n"                     \
            : "=&r" (iResult), "=&r" (iTemp),                               \
              "+R" (v->counter)                                             \
            : "Ir" (i));                                                    \
    } while (!iResult);                                                     \
                                                                            \
    iResult = iTemp;                                                        \
    iResult c_op i;                                                         \
                                                                            \
    KN_SMP_MB_AFTER_ATOMIC();                                               \
                                                                            \
    return  (iResult);                                                      \
}

ATOMIC_OP_RETURN(Add,  +=,  addu)
ATOMIC_OP_RETURN(Sub,  -=,  subu)
ATOMIC_OP_RETURN(And,  &=,  and)
ATOMIC_OP_RETURN(Or,   |=,  or)
ATOMIC_OP_RETURN(Xor,  ^=,  xor)

static LW_INLINE VOID  archAtomicSet (INT  i, atomic_t  *v)
{
#if (LW_CFG_MIPS_CPU_LOONGSON3 > 0) || (LW_CFG_MIPS_CPU_LOONGSON2K > 0) || defined(_MIPS_ARCH_HR2)
    __asm__ __volatile__(
        "   .set    push                \n"
        "   .set    noreorder           \n"
        "   sync                        \n"
        "   sw      %1, %0              \n"
        "   sync                        \n"
        "   .set    pop                 \n"
        : "+m" (v->counter)
        : "r" (i));
#else
    LW_ACCESS_ONCE(INT, v->counter) = i;
#endif
}

static LW_INLINE INT  archAtomicGet (atomic_t  *v)
{
    return  (LW_ACCESS_ONCE(INT, v->counter));
}

/*********************************************************************************************************
  atomic cas op
*********************************************************************************************************/

static LW_INLINE INT  archAtomicCas (atomic_t  *v, INT  iOld, INT  iNew)
{
    INT  iResult;

    KN_SMP_MB_BEFORE_ATOMIC();

    __asm__ __volatile__(
        "   .set    push                \n"
        "   .set    noreorder           \n"
        "   .set    noat                \n"
        "1:                             \n"
        KN_WEAK_LLSC_MB
        "   ll      %0, %2              \n"
        "   bne     %0, %z3, 2f         \n"
        "   move    $1, %z4             \n"
        "   sc      $1, %1              \n"
        "   beqz    $1, 1b              \n"
        "   nop                         \n"
        "   .set    pop                 \n"
        "2:                             \n"
        : "=&r" (iResult), "=R" (v->counter)
        : "R" (v->counter), "Jr" (iOld), "Jr" (iNew)
        : "memory");

    KN_SMP_MB_AFTER_ATOMIC();

    return  (iResult);
}

static LW_INLINE addr_t  archAtomicAddrCas (volatile addr_t *p, addr_t  ulOld, addr_t  ulNew)
{
    addr_t  ulResult;

    KN_SMP_MB_BEFORE_ATOMIC();

#if LW_CFG_CPU_WORD_LENGHT == 64
    __asm__ __volatile__(
        "   .set    push                \n"
        "   .set    noreorder           \n"
        "1:                             \n"
        KN_WEAK_LLSC_MB
        "   lld     %0, %2              \n"
        "   bne     %0, %z3, 2f         \n"
        "   move    $1, %z4             \n"
        "   scd     $1, %1              \n"
        "   beqz    $1, 1b              \n"
        "   nop                         \n"
        "   .set    pop                 \n"
        "2:                             \n"
        : "=&r" (ulResult), "=R" (*p)
        : "R" (*p), "Jr" (ulOld), "Jr" (ulNew)
        : "memory");

#else                                                                   /*  LW_CFG_CPU_WORD_LENGHT 64   */
    __asm__ __volatile__(
        "   .set    push                \n"
        "   .set    noreorder           \n"
        "1:                             \n"
        KN_WEAK_LLSC_MB
        "   ll      %0, %2              \n"
        "   bne     %0, %z3, 2f         \n"
        "   move    $1, %z4             \n"
        "   sc      $1, %1              \n"
        "   beqz    $1, 1b              \n"
        "   nop                         \n"
        "   .set    pop                 \n"
        "2:                             \n"
        : "=&r" (ulResult), "=R" (*p)
        : "R" (*p), "Jr" (ulOld), "Jr" (ulNew)
        : "memory");
#endif                                                                  /*  LW_CFG_CPU_WORD_LENGHT 32   */

    KN_SMP_MB_AFTER_ATOMIC();

    return  (ulResult);
}

/*********************************************************************************************************
  ATOMIC64
*********************************************************************************************************/
#if LW_CFG_CPU_ATOMIC64_EN > 0

#define ATOMIC64_OP_RETURN(op, c_op, asm_op)                                \
static LW_INLINE INT64  archAtomic64##op (INT64  i, atomic64_t  *v)         \
{                                                                           \
    INT64  i64Temp;                                                         \
    INT64  i64Result;                                                       \
                                                                            \
    KN_SMP_MB_BEFORE_ATOMIC();                                              \
                                                                            \
    do {                                                                    \
        __asm__ __volatile__(                                               \
            "   .set    push                        \n"                     \
            "   .set    noreorder                   \n"                     \
            KN_WEAK_LLSC_MB                                                 \
            "   lld      %1, %2                     \n"                     \
            "   " #asm_op " %0, %1, %3              \n"                     \
            "   scd      %0, %2                     \n"                     \
            "   .set    pop                         \n"                     \
            : "=&r" (i64Result), "=&r" (i64Temp),                           \
              "+R" (v->counter)                                             \
            : "Ir" (i));                                                    \
    } while (!i64Result);                                                   \
                                                                            \
    i64Result = i64Temp;                                                    \
    i64Result c_op i;                                                       \
                                                                            \
    KN_SMP_MB_AFTER_ATOMIC();                                               \
                                                                            \
    return  (i64Result);                                                    \
}

ATOMIC64_OP_RETURN(Add,  +=,  daddu)
ATOMIC64_OP_RETURN(Sub,  -=,  dsubu)
ATOMIC64_OP_RETURN(And,  &=,  and)
ATOMIC64_OP_RETURN(Or,   |=,  or)
ATOMIC64_OP_RETURN(Xor,  ^=,  xor)

static LW_INLINE VOID  archAtomic64Set (INT64  i, atomic64_t  *v)
{
#if (LW_CFG_MIPS_CPU_LOONGSON3 > 0) || (LW_CFG_MIPS_CPU_LOONGSON2K > 0) || defined(_MIPS_ARCH_HR2)
    __asm__ __volatile__(
        "   .set    push                \n"
        "   .set    noreorder           \n"
        "   sync                        \n"
        "   sd      %1, %0              \n"
        "   sync                        \n"
        "   .set    pop                 \n"
        : "+m" (v->counter)
        : "r" (i));
#else
    LW_ACCESS_ONCE(INT64, v->counter) = i;
#endif
}

static LW_INLINE INT64  archAtomic64Get (atomic64_t  *v)
{
    return  (LW_ACCESS_ONCE(INT64, v->counter));
}

/*********************************************************************************************************
  atomic64 cas op
*********************************************************************************************************/

static LW_INLINE INT64  archAtomic64Cas (atomic64_t  *v, INT64  i64Old, INT64  i64New)
{
    INT64  i64Result;

    KN_SMP_MB_BEFORE_ATOMIC();

    __asm__ __volatile__(
        "   .set    push                \n"
        "   .set    noreorder           \n"
        "   .set    noat                \n"
        "1:                             \n"
        KN_WEAK_LLSC_MB
        "   lld     %0, %2              \n"
        "   bne     %0, %z3, 2f         \n"
        "   move    $1, %z4             \n"
        "   scd     $1, %1              \n"
        "   beqz    $1, 1b              \n"
        "   nop                         \n"
        "   .set    pop                 \n"
        "2:                             \n"
        : "=&r" (i64Result), "=R" (v->counter)
        : "R" (v->counter), "Jr" (i64Old), "Jr" (i64New)
        : "memory");

    KN_SMP_MB_AFTER_ATOMIC();

    return  (i64Result);
}

#endif                                                                  /*  LW_CFG_CPU_ATOMIC64_EN      */
#endif                                                                  /*  LW_CFG_CPU_ATOMIC_EN        */
#endif                                                                  /*  __ARCH_MIPS_ATOMIC_H        */
/*********************************************************************************************************
  END
*********************************************************************************************************/
