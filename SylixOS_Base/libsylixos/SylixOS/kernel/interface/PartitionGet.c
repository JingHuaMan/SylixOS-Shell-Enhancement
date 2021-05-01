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
** ��   ��   ��: PartitionGet.c
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2007 �� 01 �� 17 ��
**
** ��        ��: ���һ���ڴ�������ڴ��

** BUG
2008.01.13  ���� _DebugHandle() ���ܡ�
2009.04.08  ����� SMP ��˵�֧��.
2009.11.09  ���� errno �ж�����.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
/*********************************************************************************************************
** ��������: API_PartitionGet
** ��������: ���һ���ڴ�������ڴ��
** �䡡��  : 
**           ulId                         PARTITION ���
** �䡡��  : 
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
#if (LW_CFG_PARTITION_EN > 0) && (LW_CFG_MAX_PARTITIONS > 0)

LW_API  
PVOID  API_PartitionGet (LW_OBJECT_HANDLE  ulId)
{
             INTREG                    iregInterLevel;
    REGISTER PLW_CLASS_PARTITION       p_part;
    REGISTER UINT16                    usIndex;
    REGISTER PVOID                     pvRet;
    
    usIndex = _ObjectGetIndex(ulId);
    
#if LW_CFG_ARG_CHK_EN > 0
    if (!_ObjectClassOK(ulId, _OBJECT_PARTITION)) {                     /*  �������ͼ��                */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "partition handle invalidate.\r\n");
        _ErrorHandle(ERROR_KERNEL_HANDLE_NULL);
        return  (LW_NULL);
    }
    if (_Partition_Index_Invalid(usIndex)) {                            /*  �������������              */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "partition handle invalidate.\r\n");
        _ErrorHandle(ERROR_KERNEL_HANDLE_NULL);
        return  (LW_NULL);
    }
#endif
    p_part = &_K__partBuffer[usIndex];
    
    LW_SPIN_LOCK_QUICK(&p_part->PARTITION_slLock, &iregInterLevel);     /*  �ر��ж�ͬʱ��ס spinlock   */
    
    if (_Partition_Type_Invalid(usIndex)) {
        LW_SPIN_UNLOCK_QUICK(&p_part->PARTITION_slLock, 
                             iregInterLevel);                           /*  ���ж�, ͬʱ�� spinlock */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "partition handle invalidate.\r\n");
        _ErrorHandle(ERROR_PARTITION_NULL);
        return  (LW_NULL);
    }
    
    pvRet = _PartitionAllocate(p_part);
    
    LW_SPIN_UNLOCK_QUICK(&p_part->PARTITION_slLock, iregInterLevel);    /*  ���ж�, ͬʱ�� spinlock */
    
    MONITOR_EVT_LONG2(MONITOR_EVENT_ID_PART, MONITOR_EVENT_PART_GET, ulId, pvRet, LW_NULL);
    
    if (!pvRet) {
        _ErrorHandle(ERROR_PARTITION_NULL);
    }
    
    return  (pvRet);
}

#endif                                                                  /*  (LW_CFG_PARTITION_EN > 0)   */
                                                                        /*  (LW_CFG_MAX_PARTITIONS > 0) */
/*********************************************************************************************************
  END
*********************************************************************************************************/
