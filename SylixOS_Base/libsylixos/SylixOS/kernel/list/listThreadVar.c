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
** ��   ��   ��: listThreadVar.c
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2006 �� 12 �� 13 ��
**
** ��        ��: ����ϵͳ Theard Var ��Դ���������
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
/*********************************************************************************************************
** ��������: _Allocate_ThreadVar_Object
** ��������: �ӿ��� ThreadVar �ؼ�����ȡ��һ������ ThreadVar
** �䡡��  : 
** �䡡��  : ��õ� ThreadVar ��ַ��ʧ�ܷ��� NULL
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
#if (LW_CFG_SMP_EN == 0) && (LW_CFG_THREAD_PRIVATE_VARS_EN > 0) && (LW_CFG_MAX_THREAD_GLB_VARS > 0)

PLW_CLASS_THREADVAR  _Allocate_ThreadVar_Object (VOID)
{
    REGISTER PLW_LIST_MONO         pmonoFree;
    REGISTER PLW_CLASS_THREADVAR   pthreadvarFree;
    
    if (_LIST_MONO_IS_EMPTY(_K_resrcThreadVar.RESRC_pmonoFreeHeader)) {
        return  (LW_NULL);
    }
    
    pmonoFree      = _list_mono_allocate_seq(&_K_resrcThreadVar.RESRC_pmonoFreeHeader, 
                                             &_K_resrcThreadVar.RESRC_pmonoFreeTail);
                                                                        /*  �����Դ                    */
    pthreadvarFree = _LIST_ENTRY(pmonoFree, LW_CLASS_THREADVAR, 
                                 PRIVATEVAR_monoResrcList);             /*  �����Դ��������ַ          */
                                 
    _K_resrcThreadVar.RESRC_uiUsed++;
    if (_K_resrcThreadVar.RESRC_uiUsed > _K_resrcThreadVar.RESRC_uiMaxUsed) {
        _K_resrcThreadVar.RESRC_uiMaxUsed = _K_resrcThreadVar.RESRC_uiUsed;
    }
    
    return  (pthreadvarFree);
}
/*********************************************************************************************************
** ��������: _Free_ThreadVar_Object
** ��������: �� ThreadVar ���ƿ齻�������
** �䡡��  : 
** �䡡��  : 
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
VOID  _Free_ThreadVar_Object (PLW_CLASS_THREADVAR    pthreadvarFree)
{
    REGISTER PLW_LIST_MONO    pmonoFree;
    
    pmonoFree = &pthreadvarFree->PRIVATEVAR_monoResrcList;
    
    _list_mono_free_seq(&_K_resrcThreadVar.RESRC_pmonoFreeHeader, 
                        &_K_resrcThreadVar.RESRC_pmonoFreeTail, 
                        pmonoFree);
    
    _K_resrcThreadVar.RESRC_uiUsed--;
}

#endif                                                                  /*  LW_CFG_SMP_EN == 0          */
                                                                        /*  (LW_CFG_THREAD_PRIVATE_VAR..*/
                                                                        /*  (LW_CFG_MAX_THREAD_GLB_VAR..*/
/*********************************************************************************************************
  END
*********************************************************************************************************/
