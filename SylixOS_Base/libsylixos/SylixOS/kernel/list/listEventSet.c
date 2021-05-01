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
** ��   ��   ��: listEventSet.c
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2006 �� 12 �� 13 ��
**
** ��        ��: ����ϵͳ Event Set ���������

** BUG
2007.04.08  �����˶Բü��ĺ�֧��
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
/*********************************************************************************************************
** ��������: _Allocate_EventSet_Object
** ��������: �ӿ���EventSet�ؼ�����ȡ��һ������EventSet
** �䡡��  : 
** �䡡��  : ��õ�Object��ַ��ʧ�ܷ��� NULL
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
#if (LW_CFG_EVENTSET_EN > 0) && (LW_CFG_MAX_EVENTSETS > 0)

PLW_CLASS_EVENTSET  _Allocate_EventSet_Object (VOID)
{
    REGISTER PLW_LIST_MONO       pmonoFree;
    REGISTER PLW_CLASS_EVENTSET  pesFree;
    
    if (_LIST_MONO_IS_EMPTY(_K_resrcEventSet.RESRC_pmonoFreeHeader)) {  /*  ��黺�����Ƿ�Ϊ��          */
        return  (LW_NULL);
    }
    
    pmonoFree  = _list_mono_allocate_seq(&_K_resrcEventSet.RESRC_pmonoFreeHeader, 
                                         &_K_resrcEventSet.RESRC_pmonoFreeTail);
                                                                        /*  �����Դ                    */
    pesFree    = _LIST_ENTRY(pmonoFree, LW_CLASS_EVENTSET, 
                             EVENTSET_monoResrcList);                   /*  �����Դ��������ַ          */
                             
    _K_resrcEventSet.RESRC_uiUsed++;
    if (_K_resrcEventSet.RESRC_uiUsed > _K_resrcEventSet.RESRC_uiMaxUsed) {
        _K_resrcEventSet.RESRC_uiMaxUsed = _K_resrcEventSet.RESRC_uiUsed;
    }
    
    return  (pesFree);
}
/*********************************************************************************************************
** ��������: _Free_EventSet_Object
** ��������: ��EventSet���ƿ齻�������
** �䡡��  : 
** �䡡��  : 
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
VOID  _Free_EventSet_Object (PLW_CLASS_EVENTSET  pesFree)
{
    REGISTER PLW_LIST_MONO    pmonoFree;
    
    pmonoFree = &pesFree->EVENTSET_monoResrcList;
    
    _list_mono_free_seq(&_K_resrcEventSet.RESRC_pmonoFreeHeader, 
                        &_K_resrcEventSet.RESRC_pmonoFreeTail, 
                        pmonoFree);
    
    _K_resrcEventSet.RESRC_uiUsed--;
}

#endif                                                                  /*  (LW_CFG_EVENTSET_EN > 0)    */
                                                                        /*  (LW_CFG_MAX_EVENTSETS > 0)  */
/*********************************************************************************************************
  END
*********************************************************************************************************/
