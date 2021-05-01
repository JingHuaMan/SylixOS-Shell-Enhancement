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
** ��   ��   ��: ThreadVarDelete.c
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2007 �� 07 �� 19 ��
**
** ��        ��: ɾ���߳�˽�б�����(ULONG)

** BUG
2007.07.19  ���� _DebugHandle() ����
2007.11.13  ʹ���������������������ȫ��װ.
2008.05.18  ʹ�� __KERNEL_ENTER() ���� ThreadLock();
2013.07.18  ʹ���µĻ�ȡ TCB �ķ���, ȷ�� SMP ϵͳ��ȫ.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
/*********************************************************************************************************
** ��������: API_ThreadVarDelete
** ��������: ɾ���߳�˽�б�����
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
ULONG  API_ThreadVarDelete (LW_OBJECT_HANDLE  ulId, ULONG  *pulAddr)
{
    REGISTER UINT16                usIndex;
    REGISTER PLW_CLASS_TCB         ptcbCur;
    REGISTER PLW_CLASS_TCB         ptcb;
    REGISTER PLW_LIST_LINE         plineVar;
    REGISTER PLW_CLASS_THREADVAR   pthreadvar;
    
    usIndex = _ObjectGetIndex(ulId);
    
    if (LW_CPU_GET_CUR_NESTING()) {                                     /*  �������ж��е���            */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "called from ISR.\r\n");
        _ErrorHandle(ERROR_KERNEL_IN_ISR);
        return  (ERROR_KERNEL_IN_ISR);
    }
    
    LW_TCB_GET_CUR_SAFE(ptcbCur);
    
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
    
    ptcb = _K_ptcbTCBIdTable[usIndex];
    
    for (plineVar  = ptcb->TCB_plinePrivateVars;                        /*  ����                        */
         plineVar != LW_NULL;
         plineVar  = _list_line_get_next(plineVar)) {
         
        pthreadvar = _LIST_ENTRY(plineVar, LW_CLASS_THREADVAR, PRIVATEVAR_lineVarList);
        if (pthreadvar->PRIVATEVAR_pulAddress == pulAddr) {
            if (ptcb == ptcbCur) {
                *pulAddr = pthreadvar->PRIVATEVAR_ulValueSave;
            }
            _List_Line_Del(plineVar, &ptcb->TCB_plinePrivateVars);      /*  �� TCB �н���               */
            _Free_ThreadVar_Object(pthreadvar);                         /*  �ͷſ��ƿ�                  */
        
            __KERNEL_EXIT();                                            /*  �˳��ں�                    */
            return  (ERROR_NONE);
        }
    }
    
    __KERNEL_EXIT();                                                    /*  �˳��ں�                    */
    
    _DebugHandle(__ERRORMESSAGE_LEVEL, "var is not in thread context.\r\n");
    _ErrorHandle(ERROR_THREAD_VAR_NOT_EXIST);
    return  (ERROR_THREAD_VAR_NOT_EXIST);
}

#endif                                                                  /*  LW_CFG_SMP_EN == 0          */
                                                                        /*  (LW_CFG_THREAD_PRIVATE_VA...*/
                                                                        /*  (LW_CFG_MAX_THREAD_GLB_VA...*/
/*********************************************************************************************************
  END
*********************************************************************************************************/
