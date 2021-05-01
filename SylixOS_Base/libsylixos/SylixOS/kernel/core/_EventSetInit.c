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
** ��   ��   ��: _EventSetInit.c
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2006 �� 12 �� 13 ��
**
** ��        ��: �¼�����ʼ����

** BUG
2007.01.10  û�г�ʼ�� index
2007.01.10  �� LW_CFG_MAX_EVENTSETS == 1 ʱ _K_pmonoEventSetFreeHeader->EVENTSET_usIndex = 0
2007.04.12  �� LW_CFG_MAX_EVENTSETS == 1 ʱ _INIT_LIST_MONO_HEAD() ��������
2009.07.28  ������������ĳ�ʼ��.  
2013.11.14  ʹ�ö�����Դ�������ṹ���������Դ.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
/*********************************************************************************************************
** ��������: _EventSetInit
** ��������: �¼�����ʼ����
** �䡡��  : 
** �䡡��  : 
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
VOID  _EventSetInit (VOID)
{
#if (LW_CFG_EVENTSET_EN > 0) && (LW_CFG_MAX_EVENTSETS > 0)

#if LW_CFG_MAX_EVENTSETS == 1
    REGISTER PLW_CLASS_EVENTSET     pesTemp1;
    
    _K_resrcEventSet.RESRC_pmonoFreeHeader = &_K_esBuffer[0].EVENTSET_monoResrcList;
    
    pesTemp1 = &_K_esBuffer[0];
    
    pesTemp1->EVENTSET_ucType        = LW_TYPE_EVENT_UNUSED;
    pesTemp1->EVENTSET_plineWaitList = LW_NULL;
    pesTemp1->EVENTSET_usIndex       = 0;
    
    _INIT_LIST_MONO_HEAD(_K_resrcEventSet.RESRC_pmonoFreeHeader);
    
    _K_resrcEventSet.RESRC_pmonoFreeTail = _K_resrcEventSet.RESRC_pmonoFreeHeader;
    
#else
    REGISTER ULONG                  ulI;
    REGISTER PLW_LIST_MONO          pmonoTemp1;
    REGISTER PLW_LIST_MONO          pmonoTemp2;
    REGISTER PLW_CLASS_EVENTSET     pesTemp1;
    REGISTER PLW_CLASS_EVENTSET     pesTemp2;
    
    _K_resrcEventSet.RESRC_pmonoFreeHeader = &_K_esBuffer[0].EVENTSET_monoResrcList;
    
    pesTemp1 = &_K_esBuffer[0];
    pesTemp2 = &_K_esBuffer[1];
    
    for (ulI = 0; ulI < ((LW_CFG_MAX_EVENTSETS) - 1); ulI++) {
        pesTemp1->EVENTSET_ucType        = LW_TYPE_EVENT_UNUSED;
        pesTemp1->EVENTSET_plineWaitList = LW_NULL;
        pesTemp1->EVENTSET_usIndex       = (UINT16)ulI;
        
        pmonoTemp1 = &pesTemp1->EVENTSET_monoResrcList;
        pmonoTemp2 = &pesTemp2->EVENTSET_monoResrcList;
        
        _LIST_MONO_LINK(pmonoTemp1, pmonoTemp2);
        
        pesTemp1++;
        pesTemp2++;
    }
    
    pesTemp1->EVENTSET_ucType        = LW_TYPE_EVENT_UNUSED;
    pesTemp1->EVENTSET_plineWaitList = LW_NULL;
    pesTemp1->EVENTSET_usIndex       = (UINT16)ulI;
        
    pmonoTemp1 = &pesTemp1->EVENTSET_monoResrcList;
    
    _INIT_LIST_MONO_HEAD(pmonoTemp1);
    
    _K_resrcEventSet.RESRC_pmonoFreeTail = pmonoTemp1;
    
#endif                                                                  /*  LW_CFG_MAX_EVENTSETS == 1   */

    _K_resrcEventSet.RESRC_uiUsed    = 0;
    _K_resrcEventSet.RESRC_uiMaxUsed = 0;
#endif                                                                  /*  (LW_CFG_EVENTSET_EN > 0)    */
                                                                        /*  (LW_CFG_MAX_EVENTSETS > 0)  */
}
/*********************************************************************************************************
  END
*********************************************************************************************************/
