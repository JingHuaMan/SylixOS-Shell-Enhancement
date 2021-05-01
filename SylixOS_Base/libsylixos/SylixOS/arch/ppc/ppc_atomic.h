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
** 文   件   名: ppc_atomic.h
**
** 创   建   人: Jiao.Jinxing (焦进星)
**
** 文件创建日期: 2018 年 07 月 27 日
**
** 描        述: PowerPC 体系构架 ATOMIC 接口.
*********************************************************************************************************/

#ifndef __ARCH_PPC_ATOMIC_H
#define __ARCH_PPC_ATOMIC_H

/*********************************************************************************************************
  ATOMIC
*********************************************************************************************************/
#if LW_CFG_CPU_ATOMIC_EN > 0

#define ATOMIC_OP_RETURN(op, c_op, asm_op)                                  \
static LW_INLINE INT  archAtomic##op (INT  i, atomic_t  *v)                 \
{                                                                           \
    INT  iResult;                                                           \
                                                                            \
    __asm__ __volatile__(                                                   \
        "1: lwarx       %0, 0,  %3      \n"                                 \
            #asm_op "   %0, %2, %0      \n"                                 \
        "   stwcx.      %0, 0,  %3      \n"                                 \
        "   bne-        1b              \n"                                 \
            : "=&r" (iResult), "+m" (v->counter)                            \
            : "r" (i), "r" (&v->counter)                                    \
            : "cc");                                                        \
                                                                            \
    return  (iResult);                                                      \
}

ATOMIC_OP_RETURN(Add,  +=,  add)
ATOMIC_OP_RETURN(Sub,  -=,  subf)
ATOMIC_OP_RETURN(And,  &=,  and)
ATOMIC_OP_RETURN(Or,   |=,  or)
ATOMIC_OP_RETURN(Xor,  ^=,  xor)

static LW_INLINE INT  archAtomicGet (atomic_t  *v)
{
    INT  iValue;

    __asm__ __volatile__("lwz%U1%X1   %0, %1" : "=r"(iValue) : "m"(v->counter));

    return  (iValue);
}

static LW_INLINE VOID  archAtomicSet (INT  i, atomic_t  *v)
{
    __asm__ __volatile__("stw%U0%X0   %1, %0" : "=m"(v->counter) : "r"(i));
}

#if LW_CFG_SMP_EN > 0
#define PPC_ATOMIC_ENTRY_BARRIER    "      sync    \n"
#define PPC_ATOMIC_EXIT_BARRIER     "      sync    \n"
#else
#define PPC_ATOMIC_ENTRY_BARRIER
#define PPC_ATOMIC_EXIT_BARRIER
#endif                                                                  /*  LW_CFG_SMP_EN > 0           */

/*********************************************************************************************************
  atomic cas op
*********************************************************************************************************/

static LW_INLINE INT  archAtomicCas (atomic_t  *v, INT  iOld, INT  iNew)
{
    INT  iOldValue;

    __asm__ __volatile__ (
            PPC_ATOMIC_ENTRY_BARRIER
        "1: lwarx   %0, 0,  %2   \n"
        "   cmpw    0,  %0, %3   \n"
        "   bne-    2f           \n"
        "   stwcx.  %4, 0,  %2   \n"
        "   bne-    1b           \n"
            PPC_ATOMIC_EXIT_BARRIER
        "2:"
            : "=&r" (iOldValue), "+m" (v->counter)
            : "r" (&v->counter), "r" (iOld), "r" (iNew)
            : "cc", "memory");

    return  (iOldValue);
}

static LW_INLINE addr_t  archAtomicAddrCas (volatile addr_t *p, addr_t  ulOld, addr_t  ulNew)
{
    addr_t  ulOldValue;

#if LW_CFG_CPU_WORD_LENGHT == 64
    __asm__ __volatile__ (
            PPC_ATOMIC_ENTRY_BARRIER
        "1: ldarx   %0, 0,  %2   \n"
        "   cmpd    0,  %0, %3   \n"
        "   bne-    2f           \n"
        "   stdcx.  %4, 0,  %2   \n"
        "   bne-    1b           \n"
            PPC_ATOMIC_EXIT_BARRIER
        "2:"
            : "=&r" (ulOldValue), "+m" (*p)
            : "r" (p), "r" (ulOld), "r" (ulNew)
            : "cc", "memory");

#else                                                                   /*  LW_CFG_CPU_WORD_LENGHT 64   */
    __asm__ __volatile__ (
            PPC_ATOMIC_ENTRY_BARRIER
        "1: lwarx   %0, 0,  %2   \n"
        "   cmpw    0,  %0, %3   \n"
        "   bne-    2f           \n"
        "   stwcx.  %4, 0,  %2   \n"
        "   bne-    1b           \n"
            PPC_ATOMIC_EXIT_BARRIER
        "2:"
            : "=&r" (ulOldValue), "+m" (*p)
            : "r" (p), "r" (ulOld), "r" (ulNew)
            : "cc", "memory");
#endif                                                                  /*  LW_CFG_CPU_WORD_LENGHT 32   */

    return  (ulOldValue);
}

/*********************************************************************************************************
  ATOMIC64
*********************************************************************************************************/
#if LW_CFG_CPU_ATOMIC64_EN > 0

#define ATOMIC64_OP_RETURN(op, c_op, asm_op)                                \
static LW_INLINE INT64  archAtomic64##op (INT64  i, atomic64_t  *v)         \
{                                                                           \
    INT64  i64Result;                                                       \
                                                                            \
    __asm__ __volatile__(                                                   \
        "1: ldarx       %0, 0,  %3      \n"                                 \
            #asm_op "   %0, %2, %0      \n"                                 \
        "   stdcx.      %0, 0,  %3      \n"                                 \
        "   bne-        1b              \n"                                 \
            : "=&r" (i64Result), "+m" (v->counter)                          \
            : "r" (i), "r" (&v->counter)                                    \
            : "cc");                                                        \
                                                                            \
    return  (i64Result);                                                    \
}

ATOMIC64_OP_RETURN(Add,  +=,  add)
ATOMIC64_OP_RETURN(Sub,  -=,  subf)
ATOMIC64_OP_RETURN(And,  &=,  and)
ATOMIC64_OP_RETURN(Or,   |=,  or)
ATOMIC64_OP_RETURN(Xor,  ^=,  xor)

static LW_INLINE INT64  archAtomic64Get (atomic64_t  *v)
{
    INT64  i64Value;

    __asm__ __volatile__("ld%U1%X1 %0,%1" : "=r"(i64Value) : "m"(v->counter));

    return  (i64Value);
}

static LW_INLINE VOID  archAtomic64Set (INT64  i, atomic64_t  *v)
{
    __asm__ __volatile__("std%U0%X0 %1,%0" : "=m"(v->counter) : "r"(i));
}

/*********************************************************************************************************
  atomic64 cas op
*********************************************************************************************************/

static LW_INLINE INT64  archAtomicCas (atomic64_t  *v, INT64  i64Old, INT64  i64New)
{
    INT64  i64OldValue;

    __asm__ __volatile__ (
            PPC_ATOMIC_ENTRY_BARRIER
        "1: ldarx   %0, 0,  %2   \n"
        "   cmpd    0,  %0, %3   \n"
        "   bne-    2f           \n"
        "   stdcx.  %4, 0,  %2   \n"
        "   bne-    1b           \n"
            PPC_ATOMIC_EXIT_BARRIER
        "2:"
            : "=&r" (i64OldValue), "+m" (v->counter)
            : "r" (&v->counter), "r" (i64Old), "r" (i64New)
            : "cc", "memory");

    return  (i64OldValue);
}

#endif                                                                  /*  LW_CFG_CPU_ATOMIC64_EN      */
#endif                                                                  /*  LW_CFG_CPU_ATOMIC_EN        */
#endif                                                                  /*  __ARCH_PPC_ATOMIC_H         */
/*********************************************************************************************************
  END
*********************************************************************************************************/
