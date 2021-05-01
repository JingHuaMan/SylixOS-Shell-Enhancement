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
** ��   ��   ��: _EventInit.c
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2006 �� 12 �� 13 ��
**
** ��        ��: ����ϵͳ�¼���ʼ�������⡣

** BUG:
2009.07.28  ������������ĳ�ʼ��.
2013.11.14  ʹ�ö�����Դ�������ṹ���������Դ.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
/*********************************************************************************************************
** ��������: _EventInit
** ��������: ��ʼ���¼������
** �䡡��  : 
** �䡡��  : 
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
VOID  _EventInit (VOID)
{
#if  (LW_CFG_EVENT_EN > 0) && (LW_CFG_MAX_EVENTS > 0)
    REGISTER ULONG                 ulI;
    REGISTER PLW_CLASS_EVENT       peventTemp1;
    REGISTER PLW_LIST_MONO         pmonoTemp1;

#if  LW_CFG_MAX_EVENTS == 1

    _K_resrcEvent.RESRC_pmonoFreeHeader = &_K_eventBuffer[0].EVENT_monoResrcList;
                                                                            /*  ������Դ��ͷ            */
    peventTemp1 = &_K_eventBuffer[0];                                       /*  ָ�򻺳���׵�ַ        */
    pmonoTemp1  = &peventTemp1->EVENT_monoResrcList;                        /*  �����Դ��              */

    peventTemp1->EVENT_ucType  = LW_TYPE_EVENT_UNUSED;                      /*  �¼�����                */
    peventTemp1->EVENT_usIndex = 0;                                         /*  �¼��������±�          */
    
    _INIT_LIST_MONO_HEAD(pmonoTemp1);                                       /*  ��ʼ�����ڵ�          */
    
    _K_resrcEvent.RESRC_pmonoFreeTail = pmonoTemp1;
    
#else
    REGISTER PLW_CLASS_EVENT    peventTemp2;
    REGISTER PLW_LIST_MONO      pmonoTemp2;
    
    _K_resrcEvent.RESRC_pmonoFreeHeader = &_K_eventBuffer[0].EVENT_monoResrcList;
                                                                            /*  ������Դ��ͷ            */
    peventTemp1 = &_K_eventBuffer[0];                                       /*  ָ�򻺳���׵�ַ        */
    peventTemp2 = &_K_eventBuffer[1];                                       /*  ָ�򻺳���׵�ַ        */
    
    for (ulI = 0; ulI < ((LW_CFG_MAX_EVENTS) - 1); ulI++) {
        pmonoTemp1 = &peventTemp1->EVENT_monoResrcList;                     /*  �����Դ��              */
        pmonoTemp2 = &peventTemp2->EVENT_monoResrcList;                     /*  �����Դ��              */
        
        peventTemp1->EVENT_ucType  = LW_TYPE_EVENT_UNUSED;                  /*  �¼�����                */
        peventTemp1->EVENT_usIndex = (UINT16)ulI;                           /*  �¼��������±�          */
   
        _LIST_MONO_LINK(pmonoTemp1, pmonoTemp2);                            /*  ������Դ����            */
        
        peventTemp1++;
        peventTemp2++;
    }
                                                                            /*  ��ʼ�����һ���ڵ�      */
    pmonoTemp1 = &peventTemp1->EVENT_monoResrcList;                         /*  �����Դ��              */
    
    peventTemp1->EVENT_ucType  = LW_TYPE_EVENT_UNUSED;                      /*  �¼�����                */
    peventTemp1->EVENT_usIndex = (UINT16)ulI;                               /*  �¼��������±�          */

    _INIT_LIST_MONO_HEAD(pmonoTemp1);                                       /*  ��ʼ�����ڵ�          */
    
    _K_resrcEvent.RESRC_pmonoFreeTail = pmonoTemp1;
#endif                                                                      /*  LW_CFG_MAX_EVENTS == 1  */

    _K_resrcEvent.RESRC_uiUsed    = 0;
    _K_resrcEvent.RESRC_uiMaxUsed = 0;

#endif                                                                      /*  (LW_CFG_EVENT_EN > 0)   */
                                                                            /*  (LW_CFG_MAX_EVENTS > 0) */
}
/*********************************************************************************************************
  END
*********************************************************************************************************/
