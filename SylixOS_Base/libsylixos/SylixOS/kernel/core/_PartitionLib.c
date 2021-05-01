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
** ��   ��   ��: _PartitionLib.c
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2007 �� 01 �� 17 ��
**
** ��        ��: PARTITION �ں˲��������⡣

** BUG
2007.04.16  ֱ��ʹ�÷�������ĵ�һ����ַ��������ʹ����һ���ֶ����ַ��
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
/*********************************************************************************************************
** ��������: _PartitionAllocate
** ��������: PARTITION ����һ����
** �䡡��  : 
** �䡡��  : 
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
#if (LW_CFG_PARTITION_EN > 0) && (LW_CFG_MAX_PARTITIONS > 0)

PVOID  _PartitionAllocate (PLW_CLASS_PARTITION  p_part)
{
    REGISTER PVOID            pvRet;
    REGISTER PLW_LIST_MONO   *ppmonoHeader;
    REGISTER PLW_LIST_MONO    pmonoAllocate;
    
    ppmonoHeader = &p_part->PARTITION_pmonoFreeBlockList;
    
    if (p_part->PARTITION_pmonoFreeBlockList) {                         /*  ����Ƿ���ڿ��п�          */
        pmonoAllocate = _list_mono_allocate(ppmonoHeader);
        pvRet = (PVOID)pmonoAllocate;
    
        p_part->PARTITION_ulFreeBlockCounter--;                         /*  ���п��һ                  */
        
        return  (pvRet);
    
    } else {
        return  (LW_NULL);
    }
}

/*********************************************************************************************************
** ��������: _PartitionFree
** ��������: PARTITION ����һ����
** �䡡��  : 
** �䡡��  : 
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
VOID  _PartitionFree (PLW_CLASS_PARTITION  p_part, PVOID  pvFree)
{
    REGISTER PLW_LIST_MONO   *ppmonoHeader;
    REGISTER PLW_LIST_MONO    pmonoFree;
    
    ppmonoHeader = &p_part->PARTITION_pmonoFreeBlockList;
    pmonoFree    = (PLW_LIST_MONO)pvFree;
    
    _list_mono_free(ppmonoHeader, pmonoFree);
    
    p_part->PARTITION_ulFreeBlockCounter++;                             /*  ���п��һ                  */
}

#endif                                                                  /*  (LW_CFG_PARTITION_EN > 0)   */
                                                                        /*  (LW_CFG_MAX_PARTITIONS > 0) */
/*********************************************************************************************************
  END
*********************************************************************************************************/
