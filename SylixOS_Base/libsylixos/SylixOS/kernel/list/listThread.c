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
** ��   ��   ��: listThread.c
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2006 �� 12 �� 13 ��
**
** ��        ��: ����ϵͳ Theard ���������

** BUG:
2013.05.07  �ӽ�����, SylixOS TCB ���ٴ���������ջ��.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
/*********************************************************************************************************
** ��������: _Allocate_Tcb_Object
** ��������: �ӿ��� Tcb �ؼ�����ȡ��һ������ TCB
** �䡡��  : 
** �䡡��  : ��õ� Object ��ַ��ʧ�ܷ��� NULL
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
PLW_CLASS_TCB  _Allocate_Tcb_Object (VOID)
{
    REGISTER PLW_LIST_MONO       pmonoFree;
    REGISTER PLW_CLASS_TCB       ptcbFree;
    
    if (_LIST_MONO_IS_EMPTY(_K_resrcTcb.RESRC_pmonoFreeHeader)) {       /*  ��黺����Ƿ�Ϊ��          */
        return  (LW_NULL);
    }
    
    pmonoFree = _list_mono_allocate_seq(&_K_resrcTcb.RESRC_pmonoFreeHeader, 
                                        &_K_resrcTcb.RESRC_pmonoFreeTail);
                                                                        /*  �����Դ                    */
                                                                        
    ptcbFree = _LIST_ENTRY(pmonoFree, LW_CLASS_TCB, TCB_monoResrcList); /*  �����Դ��������ַ          */
    
    _K_resrcTcb.RESRC_uiUsed++;
    if (_K_resrcTcb.RESRC_uiUsed > _K_resrcTcb.RESRC_uiMaxUsed) {
        _K_resrcTcb.RESRC_uiMaxUsed = _K_resrcTcb.RESRC_uiUsed;
    }
    
    return  (ptcbFree);
}
/*********************************************************************************************************
** ��������: _Free_Tcb_Object
** ��������: �� Tcb ���ƿ齻�������
** �䡡��  : ptcbFree  ��Ҫ������ TCB
** �䡡��  : 
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
VOID  _Free_Tcb_Object (PLW_CLASS_TCB    ptcbFree)
{
    REGISTER PLW_LIST_MONO    pmonoFree;
    
    pmonoFree = &ptcbFree->TCB_monoResrcList;
    
    _list_mono_free_seq(&_K_resrcTcb.RESRC_pmonoFreeHeader, 
                        &_K_resrcTcb.RESRC_pmonoFreeTail, 
                        pmonoFree);
                        
    _K_resrcTcb.RESRC_uiUsed--;
}
/*********************************************************************************************************
  END
*********************************************************************************************************/
