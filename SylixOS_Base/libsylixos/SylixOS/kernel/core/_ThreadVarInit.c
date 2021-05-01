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
** ��   ��   ��: _ThreadVarInit.c
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2006 �� 12 �� 13 ��
**
** ��        ��: ����ϵͳ�¼���ʼ�������⡣
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
/*********************************************************************************************************
** ��������: _ThreadVarInit
** ��������: ��ʼ���߳�˽�б������ƿ黺���
** �䡡��  : 
** �䡡��  : 
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
VOID  _ThreadVarInit (VOID)
{
#if (LW_CFG_SMP_EN == 0) && (LW_CFG_THREAD_PRIVATE_VARS_EN > 0) && (LW_CFG_MAX_THREAD_GLB_VARS > 0)
    
#if  LW_CFG_MAX_THREAD_GLB_VARS == 1
    
    _K_resrcThreadVar.RESRC_pmonoFreeHeader = &_K_threavarBuffer[0].PRIVATEVAR_monoResrcList;
    
    _INIT_LIST_MONO_HEAD(_K_resrcThreadVar.RESRC_pmonoFreeHeader);
    
    _K_resrcThreadVar.RESRC_pmonoFreeTail = _K_resrcThreadVar.RESRC_pmonoFreeHeader;
    
#else

    REGISTER ULONG                  ulI;
    REGISTER PLW_LIST_MONO          pmonoTemp1;
    REGISTER PLW_LIST_MONO          pmonoTemp2;
    REGISTER PLW_CLASS_THREADVAR    threadvarTemp1;
    REGISTER PLW_CLASS_THREADVAR    threadvarTemp2;
    
    _K_resrcThreadVar.RESRC_pmonoFreeHeader = &_K_threavarBuffer[0].PRIVATEVAR_monoResrcList;
    
    threadvarTemp1 = &_K_threavarBuffer[0];                             /*  ָ�򻺳���׵�ַ            */
    threadvarTemp2 = &_K_threavarBuffer[1];                             /*  ָ�򻺳���׵�ַ            */

    for (ulI = 0; ulI < ((LW_CFG_MAX_THREAD_GLB_VARS) - 1); ulI++) {
        pmonoTemp1 = &threadvarTemp1->PRIVATEVAR_monoResrcList;
        pmonoTemp2 = &threadvarTemp2->PRIVATEVAR_monoResrcList;
        
        _LIST_MONO_LINK(pmonoTemp1, pmonoTemp2);
        
        threadvarTemp1++;
        threadvarTemp2++;
    }
    
    pmonoTemp1 = &threadvarTemp1->PRIVATEVAR_monoResrcList;
    
    _INIT_LIST_MONO_HEAD(pmonoTemp1);
    
    _K_resrcThreadVar.RESRC_pmonoFreeTail = pmonoTemp1;
#endif

    _K_resrcThreadVar.RESRC_uiUsed    = 0;
    _K_resrcThreadVar.RESRC_uiMaxUsed = 0;

#endif                                                                  /* LW_CFG_THREAD_PRIVATE_VARS...*/
                                                                        /* LW_CFG_MAX_THREAD_GLB_VARS...*/
}
/*********************************************************************************************************
  END
*********************************************************************************************************/
