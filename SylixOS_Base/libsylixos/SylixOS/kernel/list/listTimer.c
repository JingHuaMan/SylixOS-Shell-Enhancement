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
** ��   ��   ��: listTimer.c
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2006 �� 12 �� 13 ��
**
** ��        ��: ����ϵͳ��ʱ����Դ���������
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
/*********************************************************************************************************
** ��������: _Allocate_Timer_Object
** ��������: �ӿ��� Timer �ؼ�����ȡ��һ������ Timer
** �䡡��  : 
** �䡡��  : ��õ� Timer ��ַ��ʧ�ܷ��� NULL
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
#if	((LW_CFG_HTIMER_EN > 0) || (LW_CFG_ITIMER_EN > 0)) && (LW_CFG_MAX_TIMERS)

PLW_CLASS_TIMER  _Allocate_Timer_Object (VOID)
{
    REGISTER PLW_LIST_MONO         pmonoFree;
    REGISTER PLW_CLASS_TIMER       ptmrFree;
    
    if (_LIST_MONO_IS_EMPTY(_K_resrcTmr.RESRC_pmonoFreeHeader)) {
        return  (LW_NULL);
    }
    
    pmonoFree = _list_mono_allocate_seq(&_K_resrcTmr.RESRC_pmonoFreeHeader, 
                                        &_K_resrcTmr.RESRC_pmonoFreeTail);
                                                                        /*  �����Դ                    */
    ptmrFree  = _LIST_ENTRY(pmonoFree, LW_CLASS_TIMER, 
                            TIMER_monoResrcList);                       /*  �����Դ��������ַ          */
    
    _K_resrcTmr.RESRC_uiUsed++;
    if (_K_resrcTmr.RESRC_uiUsed > _K_resrcTmr.RESRC_uiMaxUsed) {
        _K_resrcTmr.RESRC_uiMaxUsed = _K_resrcTmr.RESRC_uiUsed;
    }
    
    return  (ptmrFree);
}
/*********************************************************************************************************
** ��������: _Free_Timer_Object
** ��������: �� Timer ���ƿ齻�������
** �䡡��  : 
** �䡡��  : 
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
VOID  _Free_Timer_Object (PLW_CLASS_TIMER    ptmrFree)
{
    REGISTER PLW_LIST_MONO    pmonoFree;
    
    pmonoFree = &ptmrFree->TIMER_monoResrcList;
    
    _list_mono_free_seq(&_K_resrcTmr.RESRC_pmonoFreeHeader, 
                        &_K_resrcTmr.RESRC_pmonoFreeTail, 
                        pmonoFree);
    
    _K_resrcTmr.RESRC_uiUsed--;
}

#endif                                                                  /*  ((LW_CFG_HTIMER_EN > 0)     */
                                                                        /*  (LW_CFG_ITIMER_EN > 0))     */
                                                                        /*  (LW_CFG_MAX_TIMERS)         */
/*********************************************************************************************************
  END
*********************************************************************************************************/
