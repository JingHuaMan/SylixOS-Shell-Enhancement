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
** ��   ��   ��: dualportmemLib.c
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2007 �� 04 �� 10 ��
**
** ��        ��: ϵͳ֧�ֹ����ڴ�ʽ�ദ����˫���ڴ��ڲ�����
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
#define  __DPMA_MAIN_FILE
#include "../SylixOS/mpi/include/mpi_mpi.h"
/*********************************************************************************************************
** ��������: _DpmaInit
** ��������: ˫ͨ���ڴ��������ʼ��
** �䡡��  : 
** �䡡��  : 
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
#if LW_CFG_MPI_EN > 0

VOID  _DpmaInit (VOID)
{
#if LW_CFG_MAX_MPDPMAS == 1

    REGISTER PLW_CLASS_DPMA         pdpmaTemp1;
    
    _G_resrcDpma.RESRC_pmonoFreeHeader = &_G_dpmaBuffer[0].DPMA_monoResrcList;
    
    pdpmaTemp1 = &_G_dpmaBuffer[0];
    pdpmaTemp1->DPMA_usIndex = 0;
    
    _INIT_LIST_MONO_HEAD(_G_resrcDpma.RESRC_pmonoFreeHeader);           /*  ��һ���ڵ�                  */
    
    _G_resrcDpma.RESRC_pmonoFreeTail = _G_resrcDpma.RESRC_pmonoFreeHeader;

#else
    
    REGISTER ULONG                  ulI;
    REGISTER PLW_LIST_MONO          pmonoTemp1;
    REGISTER PLW_LIST_MONO          pmonoTemp2;
    REGISTER PLW_CLASS_DPMA         pdpmaTemp1;
    REGISTER PLW_CLASS_DPMA         pdpmaTemp2;
    
    _G_resrcDpma.RESRC_pmonoFreeHeader = &_G_dpmaBuffer[0].DPMA_monoResrcList;
    
    pdpmaTemp1 = &_G_dpmaBuffer[0];                                     /*  ָ�򻺳���׵�ַ            */
    pdpmaTemp2 = &_G_dpmaBuffer[1];
    
    for (ulI = 0; ulI < (LW_CFG_MAX_MPDPMAS - 1); ulI++) {
        pdpmaTemp1->DPMA_usIndex = (UINT16)ulI;
        
        pmonoTemp1 = &pdpmaTemp1->DPMA_monoResrcList;
        pmonoTemp2 = &pdpmaTemp2->DPMA_monoResrcList;
        
        _LIST_MONO_LINK(pmonoTemp1, pmonoTemp2);
        
        pdpmaTemp1++;
        pdpmaTemp2++;
    }
    
    pdpmaTemp1->DPMA_usIndex = (UINT16)ulI;
    
    pmonoTemp1 = &pdpmaTemp1->DPMA_monoResrcList;
    
    _INIT_LIST_MONO_HEAD(pmonoTemp1);
    
    _G_resrcDpma.RESRC_pmonoFreeTail = pmonoTemp1;                      /*  last node                   */
#endif                                                                  /*  LW_CFG_MAX_MPDPMAS == 1     */

    _G_resrcDpma.RESRC_uiUsed    = 0;
    _G_resrcDpma.RESRC_uiMaxUsed = 0;
}
/*********************************************************************************************************
** ��������: _Allocate_Dpma_Object
** ��������: �ӿ��� DPMA �ؼ�����ȡ��һ������ DPMA
** �䡡��  : 
** �䡡��  : ��õ� DPMA ��ַ��ʧ�ܷ��� NULL
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
PLW_CLASS_DPMA  _Allocate_Dpma_Object (VOID)
{
    REGISTER PLW_LIST_MONO         pmonoFree;
    REGISTER PLW_CLASS_DPMA        pdpma;
    
    if (_LIST_MONO_IS_EMPTY(_G_resrcDpma.RESRC_pmonoFreeHeader)) {
        return  (LW_NULL);
    }
    
    pmonoFree = _list_mono_allocate_seq(&_G_resrcDpma.RESRC_pmonoFreeHeader, 
                                        &_G_resrcDpma.RESRC_pmonoFreeTail);
                                                                        /*  �����Դ                    */
    pdpma     = _LIST_ENTRY(pmonoFree, LW_CLASS_DPMA, DPMA_monoResrcList);
    
    _G_resrcDpma.RESRC_uiUsed++;
    if (_G_resrcDpma.RESRC_uiUsed > _G_resrcDpma.RESRC_uiMaxUsed) {
        _G_resrcDpma.RESRC_uiMaxUsed = _G_resrcDpma.RESRC_uiUsed;
    }

    return  (pdpma);
}
/*********************************************************************************************************
** ��������: _Free_Dpma_Object
** ��������: �� Dpma ���ƿ齻�������
** �䡡��  : 
** �䡡��  : 
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
VOID  _Free_Dpma_Object (PLW_CLASS_DPMA  pdpma)
{
    REGISTER PLW_LIST_MONO    pmonoFree;
    
    pmonoFree = &pdpma->DPMA_monoResrcList;
    
    _list_mono_free_seq(&_G_resrcDpma.RESRC_pmonoFreeHeader, 
                        &_G_resrcDpma.RESRC_pmonoFreeTail, 
                        pmonoFree);
                        
    _G_resrcDpma.RESRC_uiUsed--;
}

#endif                                                                  /*  LW_CFG_MPI_EN > 0           */
/*********************************************************************************************************
  END
*********************************************************************************************************/
