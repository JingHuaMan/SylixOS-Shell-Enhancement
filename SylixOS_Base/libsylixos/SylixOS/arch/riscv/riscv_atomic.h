/*********************************************************************************************************
**
**                                    �й������Դ��֯
**
**                                   Ƕ��ʽʵʱ����ϵͳ
**
**                                SylixOS(TM)  LW : long wing
**
**                               Copyright All Rights Reserved
**
**--------------�ļ���Ϣ--------------------------------------------------------------------------------
**
** ��   ��   ��: riscv_atomic.h
**
** ��   ��   ��: Jiao.Jinxing (������)
**
** �ļ���������: 2018 �� 07 �� 27 ��
**
** ��        ��: RISC-V ��ϵ���� ATOMIC �ӿ�.
*********************************************************************************************************/

#ifndef __ARCH_RISCV_ATOMIC_H
#define __ARCH_RISCV_ATOMIC_H

/*********************************************************************************************************
  ATOMIC
*********************************************************************************************************/
#if LW_CFG_CPU_ATOMIC_EN > 0

#define ATOMIC_OP_RETURN(op, asm_op, c_op, I)                   \
static LW_INLINE INT  archAtomic##op (INT  i, atomic_t  *v)     \
{                                                               \
    REGISTER INT  iRet;                                         \
                                                                \
    __asm__ __volatile__ (                                      \
        "   amo" #asm_op ".w.aqrl  %1, %2, %0"                  \
        : "+A" (v->counter), "=r" (iRet)                        \
        : "r" (I)                                               \
        : "memory");                                            \
    return  (iRet c_op I);                                      \
}

ATOMIC_OP_RETURN(Add, add, +,  i)
ATOMIC_OP_RETURN(Sub, add, +, -i)
ATOMIC_OP_RETURN(And, and, &,  i)
ATOMIC_OP_RETURN(Or,  or,  |,  i)
ATOMIC_OP_RETURN(Xor, xor, ^,  i)

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
             INT    iResult;
    REGISTER UINT   uiTemp;

    __asm__ __volatile__ (
        "0: lr.w    %0, %2              \n"
        "   bne     %0, %z3, 1f         \n"
        "   sc.w.rl %1, %z4, %2         \n"
        "   bnez    %1, 0b              \n"
        "   fence   rw, rw              \n"
        "1:                             \n"
        : "=&r" (iResult), "=&r" (uiTemp), "+A" (v->counter)
        : "rJ" (iOld), "rJ" (iNew)
        : "memory");

    return  (iResult);
}

static LW_INLINE addr_t  archAtomicAddrCas (volatile addr_t *p, addr_t  ulOld, addr_t  ulNew)
{
             addr_t ulResult;
    REGISTER UINT   uiTemp;

#if LW_CFG_CPU_WORD_LENGHT == 64
    __asm__ __volatile__ (
        "0: lr.d    %0, %2              \n"
        "   bne     %0, %z3, 1f         \n"
        "   sc.d.rl %1, %z4, %2         \n"
        "   bnez    %1, 0b              \n"
        "   fence   rw, rw              \n"
        "1:                             \n"
        : "=&r" (ulResult), "=&r" (uiTemp), "+A" (*p)
        : "rJ" (ulOld), "rJ" (ulNew)
        : "memory");

#else                                                                   /*  LW_CFG_CPU_WORD_LENGHT 64   */
    __asm__ __volatile__ (
        "0: lr.w    %0, %2              \n"
        "   bne     %0, %z3, 1f         \n"
        "   sc.w.rl %1, %z4, %2         \n"
        "   bnez    %1, 0b              \n"
        "   fence   rw, rw              \n"
        "1:                             \n"
        : "=&r" (ulResult), "=&r" (uiTemp), "+A" (*p)
        : "rJ" (ulOld), "rJ" (ulNew)
        : "memory");
#endif                                                                  /*  LW_CFG_CPU_WORD_LENGHT 32   */

    return  (ulResult);
}

/*********************************************************************************************************
  ATOMIC64
*********************************************************************************************************/
#if LW_CFG_CPU_ATOMIC64_EN > 0

#define ATOMIC64_OP_RETURN(op, asm_op, c_op, I)                         \
static LW_INLINE INT64  archAtomic64##op (INT64  i, atomic64_t  *v)     \
{                                                                       \
    REGISTER INT64  i64Ret;                                             \
                                                                        \
    __asm__ __volatile__ (                                              \
        "   amo" #asm_op ".d.aqrl  %1, %2, %0"                          \
        : "+A" (v->counter), "=r" (i64Ret)                              \
        : "r" (I)                                                       \
        : "memory");                                                    \
    return  (i64Ret c_op I);                                            \
}

ATOMIC64_OP_RETURN(Add, add, +,  i)
ATOMIC64_OP_RETURN(Sub, add, +, -i)
ATOMIC64_OP_RETURN(And, and, &,  i)
ATOMIC64_OP_RETURN(Or,  or,  |,  i)
ATOMIC64_OP_RETURN(Xor, xor, ^,  i)

static LW_INLINE VOID  archAtomic64Set (INT64  i, atomic64_t  *v)
{
    LW_ACCESS_ONCE(INT64, v->counter) = i;
}

static LW_INLINE INT  archAtomic64Get (atomic64_t  *v)
{
    return  (LW_ACCESS_ONCE(INT64, v->counter));
}

/*********************************************************************************************************
  atomic64 cas op
*********************************************************************************************************/

static LW_INLINE INT64  archAtomic64Cas (atomic64_t  *v, INT64  i64Old, INT64  i64New)
{
             INT64    i64Result;
    REGISTER UINT     uiTemp;

    __asm__ __volatile__ (
        "0: lr.d    %0, %2              \n"
        "   bne     %0, %z3, 1f         \n"
        "   sc.d.rl %1, %z4, %2         \n"
        "   bnez    %1, 0b              \n"
        "   fence   rw, rw              \n"
        "1:                             \n"
        : "=&r" (i64Result), "=&r" (uiTemp), "+A" (v->counter)
        : "rJ" (i64Old), "rJ" (i64New)
        : "memory");

    return  (i64Result);
}

#endif                                                                  /*  LW_CFG_CPU_ATOMIC64_EN      */
#endif                                                                  /*  LW_CFG_CPU_ATOMIC_EN        */
#endif                                                                  /*  __ARCH_RISCV_ATOMIC_H       */
/*********************************************************************************************************
  END
*********************************************************************************************************/
