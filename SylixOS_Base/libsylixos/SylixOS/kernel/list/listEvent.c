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
** ��   ��   ��: listEvent.c
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2006 �� 12 �� 13 ��
**
** ��        ��: ����ϵͳ Event ���������

** BUG
2007.04.08  �����˶Բü��ĺ�֧��
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
/*********************************************************************************************************
** ��������: _Allocate_Event_Object
** ��������: �ӿ���Event�ؼ�����ȡ��һ������Event
** �䡡��  : 
** �䡡��  : ��õ�Object��ַ��ʧ�ܷ��� NULL
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
#if (LW_CFG_EVENT_EN > 0) && (LW_CFG_MAX_EVENTS > 0)

PLW_CLASS_EVENT  _Allocate_Event_Object (VOID)
{
    REGISTER PLW_LIST_MONO    pmonoFree;
    REGISTER PLW_CLASS_EVENT  peventFree;
    
    if (_LIST_MONO_IS_EMPTY(_K_resrcEvent.RESRC_pmonoFreeHeader)) {     /*  ��黺�����Ƿ�Ϊ��          */
        return  (LW_NULL);
    }
    
    pmonoFree  = _list_mono_allocate_seq(&_K_resrcEvent.RESRC_pmonoFreeHeader, 
                                         &_K_resrcEvent.RESRC_pmonoFreeTail);
                                                                        /*  �����Դ                    */
    peventFree = _LIST_ENTRY(pmonoFree, LW_CLASS_EVENT, 
                             EVENT_monoResrcList);                      /*  �����Դ��������ַ          */
    
    _K_resrcEvent.RESRC_uiUsed++;
    if (_K_resrcEvent.RESRC_uiUsed > _K_resrcEvent.RESRC_uiMaxUsed) {
        _K_resrcEvent.RESRC_uiMaxUsed = _K_resrcEvent.RESRC_uiUsed;
    }
    
    return  (peventFree);
}
/*********************************************************************************************************
** ��������: _Free_Event_Object
** ��������: ��Event���ƿ齻�������
** �䡡��  : 
** �䡡��  : 
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
VOID  _Free_Event_Object (PLW_CLASS_EVENT  pevent)
{
    REGISTER PLW_LIST_MONO    pmonoFree;
    
    pmonoFree = &pevent->EVENT_monoResrcList;
    
    _list_mono_free_seq(&_K_resrcEvent.RESRC_pmonoFreeHeader, 
                        &_K_resrcEvent.RESRC_pmonoFreeTail, 
                        pmonoFree);
                        
    _K_resrcEvent.RESRC_uiUsed--;
}

#endif                                                                  /*  (LW_CFG_EVENT_EN > 0)       */
                                                                        /*  (LW_CFG_MAX_EVENTS > 0)     */
/*********************************************************************************************************
  END
*********************************************************************************************************/
