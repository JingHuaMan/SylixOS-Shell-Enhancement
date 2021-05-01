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
** ��   ��   ��: ThreadVarAdd.c
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2007 �� 07 �� 19 ��
**
** ��        ��: �����߳�˽�б�����(ULONG)

** BUG
2007.07.19  ���� _DebugHandle() ����
2008.05.18  ʹ�� __KERNEL_ENTER() ���� ThreadLock();
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
/*********************************************************************************************************
** ��������: API_ThreadVarAdd
** ��������: �����߳�˽�б���, (�ദ������Ч)
** �䡡��  : 
**           ulId            �߳�ID
**           pulAddr         ˽�б�����ַ
** �䡡��  : 
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
                                           
                                       (�������ж��е���)
*********************************************************************************************************/
#if (LW_CFG_SMP_EN == 0) && (LW_CFG_THREAD_PRIVATE_VARS_EN > 0) && (LW_CFG_MAX_THREAD_GLB_VARS > 0)

LW_API
ULONG  API_ThreadVarAdd (LW_OBJECT_HANDLE  ulId, ULONG  *pulAddr)
{
    REGISTER UINT16                usIndex;
    REGISTER PLW_CLASS_TCB         ptcb;
    REGISTER PLW_CLASS_THREADVAR   pthreadvar;
    
    usIndex = _ObjectGetIndex(ulId);
    
    if (LW_CPU_GET_CUR_NESTING()) {                                     /*  �������ж��е���            */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "called from ISR.\r\n");
        _ErrorHandle(ERROR_KERNEL_IN_ISR);
        return  (ERROR_KERNEL_IN_ISR);
    }
    
#if LW_CFG_ARG_CHK_EN > 0
    if (!_ObjectClassOK(ulId, _OBJECT_THREAD)) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "thread handle invalidate.\r\n");
        _ErrorHandle(ERROR_KERNEL_HANDLE_NULL);
        return  (ERROR_KERNEL_HANDLE_NULL);
    }
    
    if (_Thread_Index_Invalid(usIndex)) {                               /*  ����߳���Ч��              */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "thread handle invalidate.\r\n");
        _ErrorHandle(ERROR_THREAD_NULL);
        return  (ERROR_THREAD_NULL);
    }
#endif

    __KERNEL_ENTER();                                                   /*  �����ں�                    */
    if (_Thread_Invalid(usIndex)) {
        __KERNEL_EXIT();                                                /*  �˳��ں�                    */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "thread handle invalidate.\r\n");
        _ErrorHandle(ERROR_THREAD_NULL);
        return  (ERROR_THREAD_NULL);
    }
    
    pthreadvar = _Allocate_ThreadVar_Object();                          /*  ����һ�����ƿ�              */
    
    if (!pthreadvar) {
        __KERNEL_EXIT();                                                /*  �˳��ں�                    */
        _ErrorHandle(ERROR_THREAD_VAR_FULL);
        return  (ERROR_THREAD_VAR_FULL);
    }
    
    pthreadvar->PRIVATEVAR_pulAddress  =  pulAddr;
    pthreadvar->PRIVATEVAR_ulValueSave = *pulAddr;
    
    ptcb = _K_ptcbTCBIdTable[usIndex];
                                                                        /*  ����TCB������               */
    _List_Line_Add_Ahead(&pthreadvar->PRIVATEVAR_lineVarList, &ptcb->TCB_plinePrivateVars);
    
    __KERNEL_EXIT();                                                    /*  �˳��ں�                    */

    return  (ERROR_NONE);
}

#endif                                                                  /*  LW_CFG_SMP_EN == 0          */
                                                                        /*  (LW_CFG_THREAD_PRIVATE_VA...*/
                                                                        /*  (LW_CFG_MAX_THREAD_GLB_VARS */
/*********************************************************************************************************
  END
*********************************************************************************************************/
