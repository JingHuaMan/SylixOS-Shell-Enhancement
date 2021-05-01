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
** ��   ��   ��: _PartitionInit.c
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2007 �� 01 �� 16 ��
**
** ��        ��: PARTITION ��ʼ�������⡣

** BUG:
2009.07.28  ������������ĳ�ʼ��.
2013.11.14  ʹ�ö�����Դ�������ṹ���������Դ.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
/*********************************************************************************************************
** ��������: _PartitionInit
** ��������: PARTITION ��ʼ��
** �䡡��  : 
** �䡡��  : 
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
VOID  _PartitionInit (VOID)
{
#if (LW_CFG_PARTITION_EN > 0) && (LW_CFG_MAX_PARTITIONS > 0)

#if  LW_CFG_MAX_PARTITIONS == 1

    REGISTER PLW_CLASS_PARTITION    p_partTemp1;

    _K_resrcPart.RESRC_pmonoFreeHeader = &_K__partBuffer[0].PARTITION_monoResrcList;
    
    p_partTemp1 = &_K__partBuffer[0];
    
    p_partTemp1->PARTITION_ucType  = LW_PARTITION_UNUSED;
    p_partTemp1->PARTITION_usIndex = 0;
    LW_SPIN_INIT(&p_partTemp1->PARTITION_slLock);
    
    _INIT_LIST_MONO_HEAD(_K_resrcPart.RESRC_pmonoFreeHeader);
    
    _K_resrcPart.RESRC_pmonoFreeTail = _K_resrcPart.RESRC_pmonoFreeHeader;
    
#else
    
    REGISTER ULONG                  ulI;
    REGISTER PLW_LIST_MONO          pmonoTemp1;
    REGISTER PLW_LIST_MONO          pmonoTemp2;
    REGISTER PLW_CLASS_PARTITION    p_partTemp1;
    REGISTER PLW_CLASS_PARTITION    p_partTemp2;
    
    _K_resrcPart.RESRC_pmonoFreeHeader = &_K__partBuffer[0].PARTITION_monoResrcList;
    
    p_partTemp1 = &_K__partBuffer[0];
    p_partTemp2 = &_K__partBuffer[1];
    
    for (ulI = 0; ulI < (LW_CFG_MAX_PARTITIONS - 1); ulI++) {
        p_partTemp1->PARTITION_ucType  = LW_PARTITION_UNUSED;
        p_partTemp1->PARTITION_usIndex = (UINT16)ulI;
        
        pmonoTemp1 = &p_partTemp1->PARTITION_monoResrcList;
        pmonoTemp2 = &p_partTemp2->PARTITION_monoResrcList;
        LW_SPIN_INIT(&p_partTemp1->PARTITION_slLock);
        
        _LIST_MONO_LINK(pmonoTemp1, pmonoTemp2);
        
        p_partTemp1++;
        p_partTemp2++;
    }
    
    p_partTemp1->PARTITION_ucType  = LW_PARTITION_UNUSED;
    p_partTemp1->PARTITION_usIndex = (UINT16)ulI;
    LW_SPIN_INIT(&p_partTemp1->PARTITION_slLock);
    
    pmonoTemp1 = &p_partTemp1->PARTITION_monoResrcList;
    
    _INIT_LIST_MONO_HEAD(pmonoTemp1);
    
    _K_resrcPart.RESRC_pmonoFreeTail = pmonoTemp1;
    
#endif                                                                  /*  LW_CFG_MAX_PARTITIONS == 1  */

    _K_resrcPart.RESRC_uiUsed    = 0;
    _K_resrcPart.RESRC_uiMaxUsed = 0;
    
#endif                                                                  /*  (LW_CFG_PARTITION_EN > 0)   */
                                                                        /*  (LW_CFG_MAX_PARTITIONS > 0) */
}
/*********************************************************************************************************
  END
*********************************************************************************************************/
