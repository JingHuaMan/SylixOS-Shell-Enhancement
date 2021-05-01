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
** ��   ��   ��: InterPriority.c
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2016 �� 05 �� 26 ��
**
** ��        ��: �ж����ȼ�����.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
/*********************************************************************************************************
  �ü�֧��
*********************************************************************************************************/
#if LW_CFG_INTER_PRIO > 0
/*********************************************************************************************************
** ��������: API_InterVectorSetPriority
** ��������: ����ָ���ж��������ȼ�
** �䡡��  : ulVector          �ж�������
**           uiPrio            �ж����ȼ� LW_INTER_PRIO_HIGHEST ~ LW_INTER_PRIO_LOWEST
** �䡡��  : ERROR CODE
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API
ULONG  API_InterVectorSetPriority (ULONG  ulVector, UINT  uiPrio)
{
    INTREG  iregInterLevel;
    ULONG   ulError;
    
    if (_Inter_Vector_Invalid(ulVector)) {
        _ErrorHandle(ERROR_KERNEL_VECTOR_NULL);
        return  (ERROR_KERNEL_VECTOR_NULL);
    }

    LW_SPIN_LOCK_QUICK(&_K_slcaVectorTable.SLCA_sl, &iregInterLevel);
    ulError = __ARCH_INT_VECTOR_SETPRIO(ulVector, uiPrio);
    LW_SPIN_UNLOCK_QUICK(&_K_slcaVectorTable.SLCA_sl, iregInterLevel);
    
    return  (ulError);
}
/*********************************************************************************************************
** ��������: API_InterVectorGetPriority
** ��������: ��ȡָ���ж��������ȼ�
** �䡡��  : ulVector          �ж�������
**           puiPrio           �ж����ȼ� LW_INTER_PRIO_HIGHEST ~ LW_INTER_PRIO_LOWEST
** �䡡��  : ERROR CODE
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API
ULONG  API_InterVectorGetPriority (ULONG  ulVector, UINT  *puiPrio)
{
    INTREG  iregInterLevel;
    ULONG   ulError;
    
    if (!puiPrio) {
        _ErrorHandle(EINVAL);
        return  (EINVAL);
    }

    if (_Inter_Vector_Invalid(ulVector)) {
        _ErrorHandle(ERROR_KERNEL_VECTOR_NULL);
        return  (ERROR_KERNEL_VECTOR_NULL);
    }

    LW_SPIN_LOCK_QUICK(&_K_slcaVectorTable.SLCA_sl, &iregInterLevel);
    ulError = __ARCH_INT_VECTOR_GETPRIO(ulVector, puiPrio);
    LW_SPIN_UNLOCK_QUICK(&_K_slcaVectorTable.SLCA_sl, iregInterLevel);
    
    return  (ulError);
}

#endif                                                                  /*  LW_CFG_INTER_PRIO > 0       */
/*********************************************************************************************************
  END
*********************************************************************************************************/
