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
** 文   件   名: arm_atomic.h
**
** 创   建   人: Jiao.Jinxing (焦进星)
**
** 文件创建日期: 2018 年 07 月 27 日
**
** 描        述: ARM 体系构架 ATOMIC 接口.
*********************************************************************************************************/

#ifndef __ARCH_ARM_ATOMIC_H
#define __ARCH_ARM_ATOMIC_H

/*********************************************************************************************************
  ATOMIC
*********************************************************************************************************/
#if LW_CFG_CPU_ATOMIC_EN > 0

#define ATOMIC_OP_RETURN(op, c_op, asm_op)                                  \
static LW_INLINE INT  archAtomic##op (INT  i, atomic_t  *v)                 \
{                                                                           \
    ULONG   ulTemp;                                                         \
    INT     iResult;                                                        \
                                                                            \
    ARM_PREFETCH_W(&v->counter);                                            \
                                                                            \
    __asm__ __volatile__(                                                   \
        "1: ldrex       %0, [%2]            \n"                             \
        "   " #asm_op " %0, %0, %3          \n"                             \
        "   strex       %1, %0, [%2]        \n"                             \
        "   teq         %1, #0              \n"                             \
        "   bne         1b"                                                 \
            : "=&r" (iResult), "=&r" (ulTemp)                               \
            : "r" (&v->counter), "Ir" (i)                                   \
            : "cc");                                                        \
                                                                            \
    return  (iResult);                                                      \
}

ATOMIC_OP_RETURN(Add,  +=,  add)
ATOMIC_OP_RETURN(Sub,  -=,  sub)
ATOMIC_OP_RETURN(And,  &=,  and)
ATOMIC_OP_RETURN(Or,   |=,  orr)
ATOMIC_OP_RETURN(Xor,  ^=,  eor)

/*********************************************************************************************************
  On ARM, ordinary assignment (str instruction) doesn't clear the local
  strex/ldrex monitor on some implementations. The reason we can use it for
  archAtomicSet() is the clrex or dummy strex done on every exception return.
*********************************************************************************************************/

static LW_INLINE VOID  archAtomicSet (INT  i, atomic_t  *v)
{
    LW_ACCESS_ONCE(INT, v->counter) = i;
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
    INT    iOldVal;
    UINT   uiRes;

    ARM_PREFETCH_W(&v->counter);

    do {
        __asm__ __volatile__(
            "ldrex      %1, [%2]        \n"
            "mov        %0, #0          \n"
            "teq        %1, %3          \n"
#if defined(__SYLIXOS_ARM_ARCH_M__)
            "itt        eq              \n"
#endif
            "strexeq    %0, %4, [%2]    \n"
#if defined(__SYLIXOS_ARM_ARCH_M__)
            "nopeq                      \n"
#endif
                : "=&r" (uiRes), "=&r" (iOldVal)
                : "r" (&v->counter), "Ir" (iOld), "r" (iNew)
                : "cc");
    } while (uiRes);

    return  (iOldVal);
}

static LW_INLINE addr_t  archAtomicAddrCas (volatile addr_t *p, addr_t  ulOld, addr_t  ulNew)
{
    addr_t  ulOldVal;
    UINT    uiRes;

    ARM_PREFETCH_W(p);

    do {
        __asm__ __volatile__(
            "ldrex      %1, [%2]        \n"
            "mov        %0, #0          \n"
            "teq        %1, %3          \n"
#if defined(__SYLIXOS_ARM_ARCH_M__)
            "itt        eq              \n"
#endif
            "strexeq    %0, %4, [%2]    \n"
#if defined(__SYLIXOS_ARM_ARCH_M__)
            "nopeq                      \n"
#endif
                : "=&r" (uiRes), "=&r" (ulOldVal)
                : "r" (p), "Ir" (ulOld), "r" (ulNew)
                : "cc");
    } while (uiRes);

    return  (ulOldVal);
}

/*********************************************************************************************************
  ATOMIC64
*********************************************************************************************************/
#if LW_CFG_CPU_ATOMIC64_EN > 0

#define ATOMIC64_OP_RETURN(op, c_op, asm_op1, asm_op2)                      \
static LW_INLINE INT64  archAtomic64##op (INT64  i, atomic64_t  *v)         \
{                                                                           \
    ULONG   ulTemp;                                                         \
    INT64   i64Result;                                                      \
                                                                            \
    ARM_PREFETCH_W(&v->counter);                                            \
                                                                            \
    __asm__ __volatile__(                                                   \
        "1: ldrexd          %0, %H0, [%3]       \n"                         \
        "   " #asm_op1 "    %Q0, %Q0, %Q4       \n"                         \
        "   " #asm_op2 "    %R0, %R0, %R4       \n"                         \
        "   strexd          %1, %0, %H0, [%3]   \n"                         \
        "   teq             %1, #0              \n"                         \
        "   bne             1b"                                             \
           : "=&r" (i64Result), "=&r" (ulTemp), "+Qo" (v->counter)          \
           : "r" (&v->counter), "r" (i)                                     \
           : "cc");                                                         \
                                                                            \
    return  (i64Result);                                                    \
}

ATOMIC64_OP_RETURN(Add,  +=,  adds, adc)
ATOMIC64_OP_RETURN(Sub,  -=,  subs, sbc)
ATOMIC64_OP_RETURN(And,  &=,  and,  and)
ATOMIC64_OP_RETURN(Or,   |=,  orr,  orr)
ATOMIC64_OP_RETURN(Xor,  ^=,  eor,  eor)

/*********************************************************************************************************
  On ARM, ordinary assignment (str instruction) doesn't clear the local
  strexd/ldrexd monitor on some implementations. The reason we can use it for
  archAtomic64Set() is the clrexd or dummy strexd done on every exception return.
*********************************************************************************************************/

static LW_INLINE VOID  archAtomic64Set (INT64  i, atomic64_t  *v)
{
    INT64  i64Temp;

    ARM_PREFETCH_W(&v->counter);

    __asm__ __volatile__(
        "1: ldrexd      %0, %H0, [%2]       \n"
        "   strexd      %0, %3, %H3, [%2]   \n"
        "   teq         %0, #0              \n"
        "   bne         1b"
            : "=&r" (i64Temp), "=Qo" (v->counter)
            : "r" (&v->counter), "r" (i)
            : "cc");
}

static LW_INLINE INT64  archAtomic64Get (atomic64_t  *v)
{
    INT64  i64Result;

    __asm__ __volatile__(
        "ldrexd         %0, %H0, [%1]"
            : "=&r" (i64Result)
            : "r" (&v->counter), "Qo" (v->counter)
            );

    return  (i64Result);
}

/*********************************************************************************************************
  atomic64 cas op
*********************************************************************************************************/

static LW_INLINE INT64  archAtomic64Cas (atomic64_t  *v, INT64  i64Old, INT64  i64New)
{
    INT64    i64OldVal;
    ULONG    ulRes;

    ARM_PREFETCH_W(&v->counter);

    do {
        __asm__ __volatile__(
            "ldrexd     %1, %H1, [%3]   \n"
            "mov        %0, #0          \n"
            "teq        %1, %4          \n"
            "teqeq      %H1, %H4        \n"
            "strexdeq   %0, %5, %H5, [%3]"
                : "=&r" (ulRes), "=&r" (i64OldVal), "+Qo" (v->counter)
                : "r" (&v->counter), "r" (i64Old), "r" (i64New)
                : "cc");
    } while (ulRes);

    return  (i64OldVal);
}

#endif                                                                  /*  LW_CFG_CPU_ATOMIC64_EN      */
#endif                                                                  /*  LW_CFG_CPU_ATOMIC_EN        */
#endif                                                                  /*  __ARCH_ARM_ATOMIC_H         */
/*********************************************************************************************************
  END
*********************************************************************************************************/
