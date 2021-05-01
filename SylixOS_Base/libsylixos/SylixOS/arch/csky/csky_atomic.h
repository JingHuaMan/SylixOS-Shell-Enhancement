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
** 文   件   名: csky_atomic.h
**
** 创   建   人: Hui.Kai (惠凯)
**
** 文件创建日期: 2018 年 07 月 27 日
**
** 描        述: C-SKY 体系构架 ATOMIC 接口.
*********************************************************************************************************/

#ifndef __ARCH_CSKY_ATOMIC_H
#define __ARCH_CSKY_ATOMIC_H

/*********************************************************************************************************
  ATOMIC
*********************************************************************************************************/
#if LW_CFG_CPU_ATOMIC_EN > 0

#define ATOMIC_OP_RETURN(op, c_op, asm_op)                  \
static LW_INLINE INT  archAtomic##op (INT  i, atomic_t  *v) \
{                                                           \
    INT  iTemp, iResult;                                    \
                                                            \
    __asm__ __volatile__(                                   \
        "1: ldex.w      %0, (%3) \n"                        \
        "   " #asm_op " %0, %2   \n"                        \
        "   mov         %1, %0   \n"                        \
        "   stex.w      %0, (%3) \n"                        \
        "   bez         %0, 1b   \n"                        \
            : "=&r" (iTemp), "=&r" (iResult)                \
            : "r" (i), "r"(&v->counter)                     \
            : "memory");                                    \
                                                            \
    return  (iResult);                                      \
}

ATOMIC_OP_RETURN(Add,  +=,  add)
ATOMIC_OP_RETURN(Sub,  -=,  sub)
ATOMIC_OP_RETURN(And,  &=,  and)
ATOMIC_OP_RETURN(Or,   |=,  or)
ATOMIC_OP_RETURN(Xor,  ^=,  xor)

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
    INT    iTemp;

    __asm__ __volatile__ (
        "1: ldex.w    %0, (%3) \n"
        "   cmpne     %0, %4   \n"
        "   bt        2f       \n"
        "   mov       %1, %2   \n"
        "   stex.w    %1, (%3) \n"
        "   bez       %1, 1b   \n"
        "2:                    \n"
        : "=&r" (iOldVal), "=&r" (iTemp)
        : "r" (iNew), "r"(&v->counter), "r"(iOld)
        : "memory");

    return  (iOldVal);
}

static LW_INLINE addr_t  archAtomicAddrCas (volatile addr_t *p, addr_t  ulOld, addr_t  ulNew)
{
    addr_t  ulOldVal;
    addr_t  ulTemp;

    __asm__ __volatile__ (
        "1: ldex.w    %0, (%3) \n"
        "   cmpne     %0, %4   \n"
        "   bt        2f       \n"
        "   mov       %1, %2   \n"
        "   stex.w    %1, (%3) \n"
        "   bez       %1, 1b   \n"
        "2:                    \n"
        : "=&r" (ulOldVal), "=&r" (ulTemp)
        : "r" (ulNew), "r"(p), "r"(ulOld)
        : "memory");

    return  (ulOldVal);
}

#endif                                                                  /*  LW_CFG_CPU_ATOMIC_EN        */
#endif                                                                  /*  __ARCH_CSKY_ATOMIC_H        */
/*********************************************************************************************************
  END
*********************************************************************************************************/
