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
** ��   ��   ��: sparc_atomic.h
**
** ��   ��   ��: Jiao.Jinxing (������)
**
** �ļ���������: 2018 �� 07 �� 27 ��
**
** ��        ��: SPARC ��ϵ���� ATOMIC �ӿ�.
*********************************************************************************************************/

#ifndef __ARCH_SPARC_ATOMIC_H
#define __ARCH_SPARC_ATOMIC_H

/*********************************************************************************************************
  ATOMIC
*********************************************************************************************************/
#if LW_CFG_CPU_ATOMIC_EN > 0

#define ATOMIC_OP_RETURN(op)    \
INT  archAtomic##op (INT  i, atomic_t  *v);

ATOMIC_OP_RETURN(Add)
ATOMIC_OP_RETURN(Sub)
ATOMIC_OP_RETURN(And)
ATOMIC_OP_RETURN(Or)
ATOMIC_OP_RETURN(Xor)

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
    __asm__ __volatile__ ("casa     [%2] 0xb, %3, %0"
                         : "=&r" (iNew)
                         : "0" (iNew), "r" (&v->counter), "r" (iOld)
                         : "memory");
    return  (iNew);
}

static LW_INLINE addr_t  archAtomicAddrCas (volatile addr_t *p, addr_t  ulOld, addr_t  ulNew)
{
    __asm__ __volatile__ ("casa     [%2] 0xb, %3, %0"
                         : "=&r" (ulNew)
                         : "0" (ulNew), "r" (p), "r" (ulOld)
                         : "memory");
    return  (ulNew);
}

#endif                                                                  /*  LW_CFG_CPU_ATOMIC_EN        */
#endif                                                                  /*  __ARCH_SPARC_ATOMIC_H       */
/*********************************************************************************************************
  END
*********************************************************************************************************/
