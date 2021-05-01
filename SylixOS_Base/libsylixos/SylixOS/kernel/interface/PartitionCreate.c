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
** ��   ��   ��: PartitionCreate.c
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2007 �� 01 �� 11 ��
**
** ��        ��: ����һ���ڴ����

** BUG
2007.08.29  ����ԷǶ����ַ�ļ�鹦�ܡ�
2008.01.13  ���� _DebugHandle() ����.
2008.05.18  ʹ�� __KERNEL_ENTER() ���� ThreadLock();
2008.05.31  ʹ�� __KERNEL_MODE_...().
2009.04.08  ����� SMP ��˵�֧��.
2009.07.28  �������ĳ�ʼ�����ڳ�ʼ�����еĿ��ƿ���, ����ȥ����ز���.
2011.07.29  ������󴴽�/���ٻص�.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
/*********************************************************************************************************
** ��������: API_PartitionCreate
** ��������: ����һ���ڴ����
** �䡡��  : 
**           pcName                        ����
**           pvLowAddr                     �ڴ����ʼ��ַ
**           ulBlockCounter                �ڴ����� 
**           stBlockByteSize               �ڴ���С
**           ulOption                      ѡ��
**           pulId                         Id ��
** �䡡��  : 
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
                                           
                                       (�������ж��е���)
*********************************************************************************************************/
#if (LW_CFG_PARTITION_EN > 0) && (LW_CFG_MAX_PARTITIONS > 0)

LW_API  
LW_OBJECT_HANDLE  API_PartitionCreate (CPCHAR             pcName,
                                       PVOID              pvLowAddr,
                                       ULONG              ulBlockCounter,
                                       size_t             stBlockByteSize,
                                       ULONG              ulOption,
                                       LW_OBJECT_ID      *pulId)
{
    REGISTER PLW_CLASS_PARTITION   p_part;
    REGISTER ULONG                 ulIdTemp;
    REGISTER ULONG                 ulI;

    REGISTER PLW_LIST_MONO         pmonoTemp1;
    REGISTER PLW_LIST_MONO         pmonoTemp2;
    
    if (LW_CPU_GET_CUR_NESTING()) {                                     /*  �������ж��е���            */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "called from ISR.\r\n");
        _ErrorHandle(ERROR_KERNEL_IN_ISR);
        return  (LW_OBJECT_HANDLE_INVALID);
    }
    
#if LW_CFG_ARG_CHK_EN > 0
    if (!pvLowAddr) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "pvLowAddr invalidate.\r\n");
        _ErrorHandle(ERROR_PARTITION_NULL);
        return  (LW_OBJECT_HANDLE_INVALID);
    }
    if (!_Addresses_Is_Aligned(pvLowAddr)) {                            /*  ����ַ�Ƿ����            */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "pvLowAddr is not aligned.\r\n");
        _ErrorHandle(ERROR_KERNEL_MEMORY);
        return  (LW_OBJECT_HANDLE_INVALID);
    }
    if (ulBlockCounter == 0ul) {                                        /*  �����������ֿ�              */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "ulBlockCounter invalidate.\r\n");
        _ErrorHandle(ERROR_PARTITION_BLOCK_COUNTER);
        return  (LW_OBJECT_HANDLE_INVALID);
    }
    if (_Partition_BlockSize_Invalid(stBlockByteSize)) {                /*  �ֶ�̫С                    */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "ulBockByteSize is too low.\r\n");
        _ErrorHandle(ERROR_PARTITION_BLOCK_SIZE);
        return  (LW_OBJECT_HANDLE_INVALID);
    }
#endif
    
    if (_Object_Name_Invalid(pcName)) {                                 /*  ���������Ч��              */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "name too long.\r\n");
        _ErrorHandle(ERROR_KERNEL_PNAME_TOO_LONG);
        return  (LW_OBJECT_HANDLE_INVALID);
    }
    
    __KERNEL_MODE_PROC(
        p_part = _Allocate_Partition_Object();                          /*  ���һ���������ƿ�          */
    );
    
    if (!p_part) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "there is no ID to build a partition.\r\n");
        _ErrorHandle(ERROR_PARTITION_FULL);
        return  (LW_OBJECT_HANDLE_INVALID);
    }
    
    if (pcName) {                                                       /*  ��������                    */
        lib_strcpy(p_part->PARTITION_cPatitionName, pcName);
    } else {
        p_part->PARTITION_cPatitionName[0] = PX_EOS;                    /*  �������                    */
    }
    
    p_part->PARTITION_ucType              = LW_PARTITION_USED;          /*  ���ͱ�־                    */
    p_part->PARTITION_pmonoFreeBlockList  = (PLW_LIST_MONO)pvLowAddr;   /*  �����ڴ���ͷ              */
    p_part->PARTITION_stBlockByteSize     = stBlockByteSize;            /*  ÿһ��Ĵ�С                */
    p_part->PARTITION_ulBlockCounter      = ulBlockCounter;             /*  ������                      */
    p_part->PARTITION_ulFreeBlockCounter  = ulBlockCounter;             /*  ���п�����                  */
    
    pmonoTemp1 = (PLW_LIST_MONO)pvLowAddr;
    pmonoTemp2 = (PLW_LIST_MONO)((UINT8 *)pvLowAddr + stBlockByteSize);
    
    for (ulI = 0; ulI < (ulBlockCounter - 1); ulI++) {                  /*  ������                      */
        _LIST_MONO_LINK(pmonoTemp1, pmonoTemp2);
        
        pmonoTemp1 = (PLW_LIST_MONO)((UINT8 *)pmonoTemp1 + stBlockByteSize);
        pmonoTemp2 = (PLW_LIST_MONO)((UINT8 *)pmonoTemp2 + stBlockByteSize);
    }
    
    _INIT_LIST_MONO_HEAD(pmonoTemp1);                                   /*  ���һ����                  */
    
    ulIdTemp = _MakeObjectId(_OBJECT_PARTITION, 
                             LW_CFG_PROCESSOR_NUMBER, 
                             p_part->PARTITION_usIndex);                /*  �������� id                 */
    
    if (pulId) {
        *pulId = ulIdTemp;
    }
    
    __LW_OBJECT_CREATE_HOOK(ulIdTemp, ulOption);
    
    MONITOR_EVT_LONG4(MONITOR_EVENT_ID_PART, MONITOR_EVENT_PART_CREATE,
                      ulIdTemp, pvLowAddr, ulBlockCounter, stBlockByteSize, LW_NULL);
    
    _DebugFormat(__LOGMESSAGE_LEVEL, "partition \"%s\" has been create.\r\n", (pcName ? pcName : ""));
    
    return  (ulIdTemp);
}

#endif                                                                  /*  (LW_CFG_PARTITION_EN > 0)   */
                                                                        /*  (LW_CFG_MAX_PARTITIONS > 0) */
/*********************************************************************************************************
  END
*********************************************************************************************************/
