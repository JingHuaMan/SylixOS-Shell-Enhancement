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
** ��   ��   ��: PartitionDelete.c
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2007 �� 01 �� 17 ��
**
** ��        ��: ɾ��һ���ڴ����

** BUG
2008.01.13  ���� _DebugHandle() ���ܡ�
2009.04.08  ����� SMP ��˵�֧��.
2011.07.29  ������󴴽�/���ٻص�.
2012.12.06  ����ǿ��ɾ���� API ������Դ����.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
/*********************************************************************************************************
** ��������: API_PartitionDeleteEx
** ��������: ɾ��һ���ڴ����
** �䡡��  : 
**           pulId                         PARTITION ���ָ��
**           bForce                        �Ƿ�ǿ��ɾ��
** �䡡��  : 
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
                                           
                                       (�������ж��е���)
*********************************************************************************************************/
#if (LW_CFG_PARTITION_EN > 0) && (LW_CFG_MAX_PARTITIONS > 0)

LW_API  
ULONG  API_PartitionDeleteEx (LW_OBJECT_HANDLE   *pulId, BOOL  bForce)
{
             INTREG                    iregInterLevel;
    REGISTER PLW_CLASS_PARTITION       p_part;
    REGISTER UINT16                    usIndex;
    REGISTER LW_OBJECT_HANDLE          ulId;
    
    ulId = *pulId;
    
    usIndex = _ObjectGetIndex(ulId);
    
    if (LW_CPU_GET_CUR_NESTING()) {                                     /*  �������ж��е���            */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "called from ISR.\r\n");
        _ErrorHandle(ERROR_KERNEL_IN_ISR);
        return  (ERROR_KERNEL_IN_ISR);
    }
    
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
    
    if (!bForce) {
        if (p_part->PARTITION_ulBlockCounter != 
            p_part->PARTITION_ulFreeBlockCounter) {                     /*  �Ƿ��зֶα�ʹ��            */
            LW_SPIN_UNLOCK_QUICK(&p_part->PARTITION_slLock, 
                                 iregInterLevel);                       /*  ���ж�, ͬʱ�� spinlock */
            _ErrorHandle(ERROR_PARTITION_BLOCK_USED);
            return  (ERROR_PARTITION_BLOCK_USED);
        }
    }
    
    _ObjectCloseId(pulId);                                              /*  �رվ��                    */
    
    p_part->PARTITION_ucType = LW_PARTITION_UNUSED;
    
    LW_SPIN_UNLOCK_QUICK(&p_part->PARTITION_slLock, iregInterLevel);    /*  ���ж�, ͬʱ�� spinlock */
    
    __LW_OBJECT_DELETE_HOOK(ulId);
    
    _DebugFormat(__LOGMESSAGE_LEVEL, "partition \"%s\" has been delete.\r\n", 
                 p_part->PARTITION_cPatitionName);
    
    __KERNEL_MODE_PROC(
        _Free_Partition_Object(p_part);                                 /*  �������ƿ�                  */
    );
    
    MONITOR_EVT_LONG1(MONITOR_EVENT_ID_PART, MONITOR_EVENT_REGION_DELETE, ulId, LW_NULL);
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: API_PartitionDelete
** ��������: ɾ��һ���ڴ����
** �䡡��  : 
**           pulId                         PARTITION ���ָ��
** �䡡��  : 
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
                                           
                                       (�������ж��е���)
*********************************************************************************************************/
LW_API  
ULONG  API_PartitionDelete (LW_OBJECT_HANDLE   *pulId)
{
    return  (API_PartitionDeleteEx(pulId, LW_FALSE));
}

#endif                                                                  /*  (LW_CFG_PARTITION_EN > 0)   */
                                                                        /*  (LW_CFG_MAX_PARTITIONS > 0) */
/*********************************************************************************************************
  END
*********************************************************************************************************/
