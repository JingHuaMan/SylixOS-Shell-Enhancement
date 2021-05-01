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
** ��   ��   ��: PartitionStatus.c
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2007 �� 01 �� 17 ��
**
** ��        ��: ��ѯһ���ڴ������״̬

** BUG
2008.01.13  ���� _DebugHandle() ���ܡ�
2009.04.08  ����� SMP ��˵�֧��.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
/*********************************************************************************************************
** ��������: API_PartitionStatus
** ��������: ��ѯһ���ڴ������״̬
** �䡡��  : 
**           ulId                         PARTITION ���
**           pulBlockCounter              ������ָ��       ����Ϊ NULL
**           pulFreeBlockCounter          ���п�����ָ��   ����Ϊ NULL
**           pstBlockByteSize             ���С           ����Ϊ NULL
** �䡡��  : 
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
#if (LW_CFG_PARTITION_EN > 0) && (LW_CFG_MAX_PARTITIONS > 0)

LW_API  
ULONG  API_PartitionStatus (LW_OBJECT_HANDLE    ulId,
                            ULONG              *pulBlockCounter,
                            ULONG              *pulFreeBlockCounter,
                            size_t             *pstBlockByteSize)
{
             INTREG                    iregInterLevel;
    REGISTER PLW_CLASS_PARTITION       p_part;
    REGISTER UINT16                    usIndex;
    
    usIndex = _ObjectGetIndex(ulId);
    
#if LW_CFG_ARG_CHK_EN > 0
    if (!_ObjectClassOK(ulId, _OBJECT_PARTITION)) {                     /*  �������ͼ��                */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "partition handle invalidate.\r\n");
        _ErrorHandle(ERROR_KERNEL_HANDLE_NULL);
        return  (ERROR_KERNEL_HANDLE_NULL);
    }
    if (_Partition_Index_Invalid(usIndex)) {                            /*  �������������              */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "partition handle invalidate.\r\n");
        _ErrorHandle(ERROR_KERNEL_HANDLE_NULL);
        return  (ERROR_KERNEL_HANDLE_NULL);
    }
#endif
    p_part = &_K__partBuffer[usIndex];
    
    LW_SPIN_LOCK_QUICK(&p_part->PARTITION_slLock, &iregInterLevel);     /*  �ر��ж�ͬʱ��ס spinlock   */
    
    if (_Partition_Type_Invalid(usIndex)) {
        LW_SPIN_UNLOCK_QUICK(&p_part->PARTITION_slLock, 
                             iregInterLevel);                           /*  ���ж�, ͬʱ�� spinlock */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "partition handle invalidate.\r\n");
        _ErrorHandle(ERROR_PARTITION_NULL);
        return  (ERROR_PARTITION_NULL);
    }
    
    if (pulBlockCounter) {
        *pulBlockCounter = p_part->PARTITION_ulBlockCounter;
    }
    
    if (pulFreeBlockCounter) {
        *pulFreeBlockCounter = p_part->PARTITION_ulFreeBlockCounter;
    }
    
    if (pstBlockByteSize) {
        *pstBlockByteSize = p_part->PARTITION_stBlockByteSize;
    }
    
    LW_SPIN_UNLOCK_QUICK(&p_part->PARTITION_slLock, iregInterLevel);    /*  ���ж�, ͬʱ�� spinlock */
    
    return  (ERROR_NONE);
}

#endif                                                                  /*  (LW_CFG_PARTITION_EN > 0)   */
                                                                        /*  (LW_CFG_MAX_PARTITIONS > 0) */
/*********************************************************************************************************
  END
*********************************************************************************************************/
