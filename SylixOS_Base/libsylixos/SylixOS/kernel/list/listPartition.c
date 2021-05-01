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
** ��   ��   ��: listPartition.c
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2007 �� 01 �� 16 ��
**
** ��        ��: ����ϵͳPARTITION��Դ���������

** BUG:
2009-04-09  ʹ����Դ˳�����.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
/*********************************************************************************************************
** ��������: _Allocate_Partition_Object
** ��������: �ӿ��� Partition �ؼ�����ȡ��һ������ Partition
** �䡡��  : 
** �䡡��  : ��õ� Partition ��ַ��ʧ�ܷ��� NULL
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
#if	(LW_CFG_PARTITION_EN > 0) && (LW_CFG_MAX_PARTITIONS > 0)

PLW_CLASS_PARTITION  _Allocate_Partition_Object (VOID)
{
    REGISTER PLW_LIST_MONO             pmonoFree;
    REGISTER PLW_CLASS_PARTITION       p_partFree;
    
    if (_LIST_MONO_IS_EMPTY(_K_resrcPart.RESRC_pmonoFreeHeader)) {
        return  (LW_NULL);
    }
    
    pmonoFree   = _list_mono_allocate_seq(&_K_resrcPart.RESRC_pmonoFreeHeader, 
                                          &_K_resrcPart.RESRC_pmonoFreeTail);
                                                                        /*  �����Դ                    */
    p_partFree  = _LIST_ENTRY(pmonoFree, 
                              LW_CLASS_PARTITION, 
                              PARTITION_monoResrcList);                 /*  �����Դ��������ַ          */
                              
    _K_resrcPart.RESRC_uiUsed++;
    if (_K_resrcPart.RESRC_uiUsed > _K_resrcPart.RESRC_uiMaxUsed) {
        _K_resrcPart.RESRC_uiMaxUsed = _K_resrcPart.RESRC_uiUsed;
    }
    
    return  (p_partFree);
}
/*********************************************************************************************************
** ��������: _Free_Partition_Object
** ��������: �� Partition ���ƿ齻�������
** �䡡��  : 
** �䡡��  : 
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
VOID  _Free_Partition_Object (PLW_CLASS_PARTITION    p_partFree)
{
    REGISTER PLW_LIST_MONO    pmonoFree;
    
    pmonoFree = &p_partFree->PARTITION_monoResrcList;
    
    _list_mono_free_seq(&_K_resrcPart.RESRC_pmonoFreeHeader, 
                        &_K_resrcPart.RESRC_pmonoFreeTail, 
                        pmonoFree);
                        
    _K_resrcPart.RESRC_uiUsed--;
}

#endif                                                                  /*  (LW_CFG_PARTITION_EN > 0)   */
                                                                        /*  (LW_CFG_MAX_PARTITIONS > 0) */
/*********************************************************************************************************
  END
*********************************************************************************************************/
