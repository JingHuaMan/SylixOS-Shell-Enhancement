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
** ��   ��   ��: InterVectorFlag.c
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2013 �� 12 �� 12 ��
**
** ��        ��: ����/��ȡָ���ж�����������.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
/*********************************************************************************************************
** ��������: API_InterVectorSetFlag
** ��������: ����ָ���ж�����������. 
** �䡡��  : ulVector                      �ж�������
**           ulFlag                        ����
** �䡡��  : ERROR CODE
** ȫ�ֱ���: 
** ����ģ��: 
** ע  ��  : LW_IRQ_FLAG_QUEUE �����ڰ�װ�κ�һ������ǰ����, �����ú�����ȡ��.
             ��÷��� bspIntInit() �������������.
                                           API ����
*********************************************************************************************************/
LW_API
ULONG  API_InterVectorSetFlag (ULONG  ulVector, ULONG  ulFlag)
{
    INTREG              iregInterLevel;
    PLW_CLASS_INTDESC   pidesc;

    if (_Inter_Vector_Invalid(ulVector)) {
        _ErrorHandle(ERROR_KERNEL_VECTOR_NULL);
        return  (ERROR_KERNEL_VECTOR_NULL);
    }
    
    pidesc = LW_IVEC_GET_IDESC(ulVector);
    
    LW_SPIN_LOCK_QUICK(&pidesc->IDESC_slLock, &iregInterLevel);         /*  �ر��ж�ͬʱ��ס spinlock   */
    
    if (LW_IVEC_GET_FLAG(ulVector) & LW_IRQ_FLAG_QUEUE) {               /*  �Ѿ��� QUEUE �����ж�����   */
        LW_IVEC_SET_FLAG(ulVector, ulFlag | LW_IRQ_FLAG_QUEUE);
    
    } else {
        LW_IVEC_SET_FLAG(ulVector, ulFlag);
    }
    
    LW_SPIN_UNLOCK_QUICK(&pidesc->IDESC_slLock, iregInterLevel);        /*  ���ж�, ͬʱ�� spinlock */
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: API_InterVectorGetFlag
** ��������: ���ָ���ж�����������. 
** �䡡��  : ulVector                      �ж�������
**           *pulFlag                      ����
** �䡡��  : ERROR CODE
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API
ULONG  API_InterVectorGetFlag (ULONG  ulVector, ULONG  *pulFlag)
{
    INTREG              iregInterLevel;
    PLW_CLASS_INTDESC   pidesc;

    if (_Inter_Vector_Invalid(ulVector)) {
        _ErrorHandle(ERROR_KERNEL_VECTOR_NULL);
        return  (ERROR_KERNEL_VECTOR_NULL);
    }
    
    if (!pulFlag) {
        _ErrorHandle(ERROR_KERNEL_MEMORY);
        return  (ERROR_KERNEL_MEMORY);
    }
    
    pidesc = LW_IVEC_GET_IDESC(ulVector);
    
    LW_SPIN_LOCK_QUICK(&pidesc->IDESC_slLock, &iregInterLevel);         /*  �ر��ж�ͬʱ��ס spinlock   */
    
    *pulFlag = LW_IVEC_GET_FLAG(ulVector);
    
    LW_SPIN_UNLOCK_QUICK(&pidesc->IDESC_slLock, iregInterLevel);        /*  ���ж�, ͬʱ�� spinlock */
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
  END
*********************************************************************************************************/
