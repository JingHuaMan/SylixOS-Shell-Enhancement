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
** ��   ��   ��: KernelAtomic.c
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2009 �� 09 �� 30 ��
**
** ��        ��: ϵͳ�ں�ԭ�Ӳ�����.

** BUG:
2013.03.30  ���� API_AtomicSwp ����.
2014.05.05  ��ԭ�Ӳ���������, Ϊ Qt4.8.6 ���ض�ϵͳ�ṩ����.
2018.11.30  ���� LLSC Nesting CPU bug ����.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
/*********************************************************************************************************
  CPU ATOMIC BUG ̽��
*********************************************************************************************************/
#if (LW_CFG_CPU_ATOMIC_EN > 0) && defined(LW_CFG_CPU_ARCH_MIPS) && (LW_CFG_MIPS_NEST_LLSC_BUG > 0)
#define __LW_ATOMIC_INTREG(ireg)    INTREG  ireg
#define __LW_ATOMIC_INTDIS(ireg)    ireg = KN_INT_DISABLE()
#define __LW_ATOMIC_INTEN(ireg)     KN_INT_ENABLE(ireg)

#else                                                                   /*  LW_CFG_MIPS_NEST_LLSC_BUG   */
#define __LW_ATOMIC_INTREG(ireg)
#define __LW_ATOMIC_INTDIS(ireg)
#define __LW_ATOMIC_INTEN(ireg)
#endif                                                                  /*  !LW_CFG_MIPS_NEST_LLSC_BUG  */
/*********************************************************************************************************
** ��������: API_AtomicAdd
** ��������: ԭ�� + ����
** �䡡��  : iVal      ��������
**           patomic   ԭ�Ӳ�����
** �䡡��  : �������
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
INT  API_AtomicAdd (INT  iVal, atomic_t  *patomic)
{
    REGISTER INT  iRet;

    if (patomic) {
        __LW_ATOMIC_INTREG(ireg);
        __LW_ATOMIC_INTDIS(ireg);
        iRet = __LW_ATOMIC_ADD(iVal, patomic);
        __LW_ATOMIC_INTEN(ireg);
        return  (iRet);
    
    } else {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
}
/*********************************************************************************************************
** ��������: API_AtomicSub
** ��������: ԭ�� - ����
** �䡡��  : iVal      ��������
**           patomic   ԭ�Ӳ�����
** �䡡��  : �������
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
INT  API_AtomicSub (INT  iVal, atomic_t  *patomic)
{
    REGISTER INT  iRet;

    if (patomic) {
        __LW_ATOMIC_INTREG(ireg);
        __LW_ATOMIC_INTDIS(ireg);
        iRet = __LW_ATOMIC_SUB(iVal, patomic);
        __LW_ATOMIC_INTEN(ireg);
        return  (iRet);
    
    } else {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
}
/*********************************************************************************************************
** ��������: API_AtomicInc
** ��������: ԭ�� + 1����
** �䡡��  : patomic   ԭ�Ӳ�����
** �䡡��  : �������
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
INT  API_AtomicInc (atomic_t  *patomic)
{
    REGISTER INT  iRet;

    if (patomic) {
        __LW_ATOMIC_INTREG(ireg);
        __LW_ATOMIC_INTDIS(ireg);
        iRet = __LW_ATOMIC_INC(patomic);
        __LW_ATOMIC_INTEN(ireg);
        return  (iRet);
    
    } else {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
}
/*********************************************************************************************************
** ��������: API_AtomicDec
** ��������: ԭ�� - 1����
** �䡡��  : patomic   ԭ�Ӳ�����
** �䡡��  : �������
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
INT  API_AtomicDec (atomic_t  *patomic)
{
    REGISTER INT  iRet;

    if (patomic) {
        __LW_ATOMIC_INTREG(ireg);
        __LW_ATOMIC_INTDIS(ireg);
        iRet = __LW_ATOMIC_DEC(patomic);
        __LW_ATOMIC_INTEN(ireg);
        return  (iRet);
    
    } else {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
}
/*********************************************************************************************************
** ��������: API_AtomicAnd
** ��������: ԭ�� & ����
** �䡡��  : iVal      ��������
**           patomic   ԭ�Ӳ�����
** �䡡��  : �������
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
INT  API_AtomicAnd (INT  iVal, atomic_t  *patomic)
{
    REGISTER INT  iRet;

    if (patomic) {
        __LW_ATOMIC_INTREG(ireg);
        __LW_ATOMIC_INTDIS(ireg);
        iRet = __LW_ATOMIC_AND(iVal, patomic);
        __LW_ATOMIC_INTEN(ireg);
        return  (iRet);
    
    } else {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
}
/*********************************************************************************************************
** ��������: API_AtomicNand
** ��������: ԭ�� &~ ����
** �䡡��  : iVal      ��������
**           patomic   ԭ�Ӳ�����
** �䡡��  : �������
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
INT  API_AtomicNand (INT  iVal, atomic_t  *patomic)
{
    REGISTER INT  iRet;

    if (patomic) {
        __LW_ATOMIC_INTREG(ireg);
        __LW_ATOMIC_INTDIS(ireg);
        iRet = __LW_ATOMIC_NAND(iVal, patomic);
        __LW_ATOMIC_INTEN(ireg);
        return  (iRet);
    
    } else {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
}
/*********************************************************************************************************
** ��������: API_AtomicOr
** ��������: ԭ�� | ����
** �䡡��  : iVal      ��������
**           patomic   ԭ�Ӳ�����
** �䡡��  : �������
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
INT  API_AtomicOr (INT  iVal, atomic_t  *patomic)
{
    REGISTER INT  iRet;

    if (patomic) {
        __LW_ATOMIC_INTREG(ireg);
        __LW_ATOMIC_INTDIS(ireg);
        iRet = __LW_ATOMIC_OR(iVal, patomic);
        __LW_ATOMIC_INTEN(ireg);
        return  (iRet);
    
    } else {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
}
/*********************************************************************************************************
** ��������: API_AtomicXor
** ��������: ԭ�� ^ ����
** �䡡��  : iVal      ��������
**           patomic   ԭ�Ӳ�����
** �䡡��  : �������
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
INT  API_AtomicXor (INT  iVal, atomic_t  *patomic)
{
    REGISTER INT  iRet;

    if (patomic) {
        __LW_ATOMIC_INTREG(ireg);
        __LW_ATOMIC_INTDIS(ireg);
        iRet = __LW_ATOMIC_XOR(iVal, patomic);
        __LW_ATOMIC_INTEN(ireg);
        return  (iRet);
    
    } else {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
}
/*********************************************************************************************************
** ��������: API_AtomicSet
** ��������: ԭ�Ӹ�ֵ����
** �䡡��  : iVal      ��������
**           patomic   ԭ�Ӳ�����
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
VOID  API_AtomicSet (INT  iVal, atomic_t  *patomic)
{
    if (patomic) {
        __LW_ATOMIC_SET(iVal, patomic);
    }
}
/*********************************************************************************************************
** ��������: API_AtomicGet
** ��������: ԭ�ӻ�ȡ����
** �䡡��  : patomic   ԭ�Ӳ�����
** �䡡��  : ԭ�Ӳ�����
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
INT  API_AtomicGet (atomic_t  *patomic)
{
    if (patomic) {
        return  (__LW_ATOMIC_GET(patomic));
    
    } else {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
}
/*********************************************************************************************************
** ��������: API_AtomicSwp
** ��������: ԭ�ӽ�������
** �䡡��  : iVal      ��������
**           patomic   ԭ�Ӳ�����
** �䡡��  : ֮ǰ����ֵ
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
INT  API_AtomicSwp (INT  iVal, atomic_t  *patomic)
{
    REGISTER INT  iRet;

    if (patomic) {
        __LW_ATOMIC_INTREG(ireg);
        __LW_ATOMIC_INTDIS(ireg);
        iRet = __LW_ATOMIC_SWP(iVal, patomic);
        __LW_ATOMIC_INTEN(ireg);
        return  (iRet);
    
    } else {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
}
/*********************************************************************************************************
** ��������: API_AtomicCas
** ��������: ԭ�ӽ������� ()
** �䡡��  : patomic   ԭ�Ӳ�����
**           iOldVal   ��ֵ
**           iNewVal   ��ֵ
** �䡡��  : ��ֵ, �������ֵ�� iOldVal ��ͬ, ���ʾ���óɹ�.
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
INT  API_AtomicCas (atomic_t  *patomic, INT  iOldVal, INT  iNewVal)
{
    REGISTER INT  iRet;

    if (patomic) {
        __LW_ATOMIC_INTREG(ireg);
        __LW_ATOMIC_INTDIS(ireg);
        iRet = __LW_ATOMIC_CAS(patomic, iOldVal, iNewVal);
        __LW_ATOMIC_INTEN(ireg);
        return  (iRet);
    
    } else {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
}
/*********************************************************************************************************
** ��������: API_Atomic64Add
** ��������: ԭ�� + ����
** �䡡��  : i64Val      ��������
**           patomic64   ԭ�Ӳ�����
** �䡡��  : �������
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
INT64  API_Atomic64Add (INT64  i64Val, atomic64_t  *patomic64)
{
    REGISTER INT64  i64Ret;

    if (patomic64) {
        __LW_ATOMIC_INTREG(ireg);
        __LW_ATOMIC_INTDIS(ireg);
        i64Ret = __LW_ATOMIC64_ADD(i64Val, patomic64);
        __LW_ATOMIC_INTEN(ireg);
        return  (i64Ret);
    
    } else {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
}
/*********************************************************************************************************
** ��������: API_Atomic64Sub
** ��������: ԭ�� - ����
** �䡡��  : i64Val      ��������
**           patomic64   ԭ�Ӳ�����
** �䡡��  : �������
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
INT64  API_Atomic64Sub (INT64  i64Val, atomic64_t  *patomic64)
{
    REGISTER INT64  i64Ret;

    if (patomic64) {
        __LW_ATOMIC_INTREG(ireg);
        __LW_ATOMIC_INTDIS(ireg);
        i64Ret = __LW_ATOMIC64_SUB(i64Val, patomic64);
        __LW_ATOMIC_INTEN(ireg);
        return  (i64Ret);
    
    } else {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
}
/*********************************************************************************************************
** ��������: API_Atomic64Inc
** ��������: ԭ�� + 1����
** �䡡��  : patomic64   ԭ�Ӳ�����
** �䡡��  : �������
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
INT64  API_Atomic64Inc (atomic64_t  *patomic64)
{
    REGISTER INT64  i64Ret;

    if (patomic64) {
        __LW_ATOMIC_INTREG(ireg);
        __LW_ATOMIC_INTDIS(ireg);
        i64Ret = __LW_ATOMIC64_INC(patomic64);
        __LW_ATOMIC_INTEN(ireg);
        return  (i64Ret);
    
    } else {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
}
/*********************************************************************************************************
** ��������: API_Atomic64Dec
** ��������: ԭ�� - 1����
** �䡡��  : patomic64   ԭ�Ӳ�����
** �䡡��  : �������
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
INT64  API_Atomic64Dec (atomic64_t  *patomic64)
{
    REGISTER INT64  i64Ret;

    if (patomic64) {
        __LW_ATOMIC_INTREG(ireg);
        __LW_ATOMIC_INTDIS(ireg);
        i64Ret = __LW_ATOMIC64_DEC(patomic64);
        __LW_ATOMIC_INTEN(ireg);
        return  (i64Ret);
    
    } else {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
}
/*********************************************************************************************************
** ��������: API_Atomic64And
** ��������: ԭ�� & ����
** �䡡��  : i64Val      ��������
**           patomic64   ԭ�Ӳ�����
** �䡡��  : �������
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
INT64  API_Atomic64And (INT64  i64Val, atomic64_t  *patomic64)
{
    REGISTER INT64  i64Ret;

    if (patomic64) {
        __LW_ATOMIC_INTREG(ireg);
        __LW_ATOMIC_INTDIS(ireg);
        i64Ret = __LW_ATOMIC64_AND(i64Val, patomic64);
        __LW_ATOMIC_INTEN(ireg);
        return  (i64Ret);
    
    } else {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
}
/*********************************************************************************************************
** ��������: API_Atomic64Nand
** ��������: ԭ�� &~ ����
** �䡡��  : i64Val      ��������
**           patomic64   ԭ�Ӳ�����
** �䡡��  : �������
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
INT64  API_Atomic64Nand (INT64  i64Val, atomic64_t  *patomic64)
{
    REGISTER INT64  i64Ret;

    if (patomic64) {
        __LW_ATOMIC_INTREG(ireg);
        __LW_ATOMIC_INTDIS(ireg);
        i64Ret = __LW_ATOMIC64_NAND(i64Val, patomic64);
        __LW_ATOMIC_INTEN(ireg);
        return  (i64Ret);
    
    } else {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
}
/*********************************************************************************************************
** ��������: API_Atomic64Or
** ��������: ԭ�� | ����
** �䡡��  : i64Val      ��������
**           patomic64   ԭ�Ӳ�����
** �䡡��  : �������
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
INT64  API_Atomic64Or (INT64  i64Val, atomic64_t  *patomic64)
{
    REGISTER INT64  i64Ret;

    if (patomic64) {
        __LW_ATOMIC_INTREG(ireg);
        __LW_ATOMIC_INTDIS(ireg);
        i64Ret = __LW_ATOMIC64_OR(i64Val, patomic64);
        __LW_ATOMIC_INTEN(ireg);
        return  (i64Ret);
    
    } else {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
}
/*********************************************************************************************************
** ��������: API_Atomic64Xor
** ��������: ԭ�� ^ ����
** �䡡��  : i64Val      ��������
**           patomic64   ԭ�Ӳ�����
** �䡡��  : �������
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
INT64  API_Atomic64Xor (INT64  i64Val, atomic64_t  *patomic64)
{
    REGISTER INT64  i64Ret;

    if (patomic64) {
        __LW_ATOMIC_INTREG(ireg);
        __LW_ATOMIC_INTDIS(ireg);
        i64Ret = __LW_ATOMIC64_XOR(i64Val, patomic64);
        __LW_ATOMIC_INTEN(ireg);
        return  (i64Ret);
    
    } else {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
}
/*********************************************************************************************************
** ��������: API_Atomic64Set
** ��������: ԭ�Ӹ�ֵ����
** �䡡��  : i64Val      ��������
**           patomic64   ԭ�Ӳ�����
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
VOID  API_Atomic64Set (INT64  i64Val, atomic64_t  *patomic64)
{
    if (patomic64) {
#if LW_CFG_CPU_WORD_LENGHT == 32
        __LW_ATOMIC_INTREG(ireg);
        __LW_ATOMIC_INTDIS(ireg);
        __LW_ATOMIC64_SET(i64Val, patomic64);
        __LW_ATOMIC_INTEN(ireg);
        
#else
        __LW_ATOMIC64_SET(i64Val, patomic64);
#endif
    }
}
/*********************************************************************************************************
** ��������: API_Atomic64Get
** ��������: ԭ�ӻ�ȡ����
** �䡡��  : patomic64   ԭ�Ӳ�����
** �䡡��  : ԭ�Ӳ�����
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
INT64  API_Atomic64Get (atomic64_t  *patomic64)
{
    if (patomic64) {
        return  (__LW_ATOMIC64_GET(patomic64));
    
    } else {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
}
/*********************************************************************************************************
** ��������: API_Atomic64Swp
** ��������: ԭ�ӽ�������
** �䡡��  : i64Val      ��������
**           patomic64   ԭ�Ӳ�����
** �䡡��  : ֮ǰ����ֵ
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
INT64  API_Atomic64Swp (INT64  i64Val, atomic64_t  *patomic64)
{
    REGISTER INT64  i64Ret;

    if (patomic64) {
        __LW_ATOMIC_INTREG(ireg);
        __LW_ATOMIC_INTDIS(ireg);
        i64Ret = __LW_ATOMIC64_SWP(i64Val, patomic64);
        __LW_ATOMIC_INTEN(ireg);
        return  (i64Ret);
    
    } else {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
}
/*********************************************************************************************************
** ��������: API_Atomic64Cas
** ��������: ԭ�ӽ������� ()
** �䡡��  : patomic64   ԭ�Ӳ�����
**           i64OldVal   ��ֵ
**           i64NewVal   ��ֵ
** �䡡��  : ��ֵ, �������ֵ�� i64OldVal ��ͬ, ���ʾ���óɹ�.
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
INT64  API_Atomic64Cas (atomic64_t  *patomic64, INT64  i64OldVal, INT64  i64NewVal)
{
    REGISTER INT64  i64Ret;

    if (patomic64) {
        __LW_ATOMIC_INTREG(ireg);
        __LW_ATOMIC_INTDIS(ireg);
        i64Ret = __LW_ATOMIC64_CAS(patomic64, i64OldVal, i64NewVal);
        __LW_ATOMIC_INTEN(ireg);
        return  (i64Ret);
    
    } else {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
}
/*********************************************************************************************************
  END
*********************************************************************************************************/
