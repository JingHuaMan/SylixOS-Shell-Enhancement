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
** ��   ��   ��: InterTarget.c
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2016 �� 05 �� 26 ��
**
** ��        ��: �ж�Ŀ�� CPU ����.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
/*********************************************************************************************************
  �ü�֧��
*********************************************************************************************************/
#if LW_CFG_INTER_TARGET > 0
/*********************************************************************************************************
** ��������: API_InterSetTarget
** ��������: ����ָ���ж�����Ŀ�� CPU
** �䡡��  : ulVector      �ж�������
**           stSize        CPU ���뼯�ڴ��С
**           pcpuset       CPU ����
** �䡡��  : ERROR CODE
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API
ULONG  API_InterSetTarget (ULONG  ulVector, size_t  stSize, const PLW_CLASS_CPUSET  pcpuset)
{
    INTREG  iregInterLevel;
    ULONG   ulError;
    
    if (!stSize || !pcpuset) {
        _ErrorHandle(EINVAL);
        return  (EINVAL);
    }

    if (_Inter_Vector_Invalid(ulVector)) {
        _ErrorHandle(ERROR_KERNEL_VECTOR_NULL);
        return  (ERROR_KERNEL_VECTOR_NULL);
    }

    LW_SPIN_LOCK_QUICK(&_K_slcaVectorTable.SLCA_sl, &iregInterLevel);
    ulError = __ARCH_INT_VECTOR_SETTARGET(ulVector, stSize, pcpuset);
    LW_SPIN_UNLOCK_QUICK(&_K_slcaVectorTable.SLCA_sl, iregInterLevel);
    
    return  (ulError);
}
/*********************************************************************************************************
** ��������: API_InterGetTarget
** ��������: ��ȡָ���ж�����Ŀ�� CPU
** �䡡��  : ulVector      �ж�������
**           stSize        CPU ���뼯�ڴ��С
**           pcpuset       CPU ����
** �䡡��  : ERROR CODE
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API
ULONG  API_InterGetTarget (ULONG  ulVector, size_t  stSize, PLW_CLASS_CPUSET  pcpuset)
{
    INTREG  iregInterLevel;
    ULONG   ulError;
    
    if (!stSize || !pcpuset) {
        _ErrorHandle(EINVAL);
        return  (EINVAL);
    }

    if (_Inter_Vector_Invalid(ulVector)) {
        _ErrorHandle(ERROR_KERNEL_VECTOR_NULL);
        return  (ERROR_KERNEL_VECTOR_NULL);
    }

    LW_SPIN_LOCK_QUICK(&_K_slcaVectorTable.SLCA_sl, &iregInterLevel);
    ulError = __ARCH_INT_VECTOR_GETTARGET(ulVector, stSize, pcpuset);
    LW_SPIN_UNLOCK_QUICK(&_K_slcaVectorTable.SLCA_sl, iregInterLevel);
    
    return  (ulError);
}

#endif                                                                  /*  LW_CFG_INTER_PRIO > 0       */
/*********************************************************************************************************
  END
*********************************************************************************************************/
