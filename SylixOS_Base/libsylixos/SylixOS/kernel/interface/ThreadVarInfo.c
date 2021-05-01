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
** ��   ��   ��: ThreadVarInfo.c
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2007 �� 07 �� 19 ��
**
** ��        ��: ����߳�˽�б���״̬��(ULONG)

** BUG
2007.07.19  ���� _DebugHandle() ����
2007.11.13  ʹ���������������������ȫ��װ.
2008.05.18  ʹ�� __KERNEL_ENTER() ���� ThreadLock();
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
/*********************************************************************************************************
** ��������: API_ThreadVarInfo
** ��������: ����߳�˽�б���״̬��
** �䡡��  : 
**           ulId            �߳�ID
**           pulAddr[]       ˽�б�����ַ�б�
**           iMaxCounter     ��ַ��Ĵ�С
** �䡡��  : ˽�б�������
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
                                           
                                       (�������ж��е���)
*********************************************************************************************************/
#if (LW_CFG_SMP_EN == 0) && (LW_CFG_THREAD_PRIVATE_VARS_EN > 0) && (LW_CFG_MAX_THREAD_GLB_VARS > 0)

LW_API
ULONG  API_ThreadVarInfo (LW_OBJECT_HANDLE  ulId, ULONG  *pulAddr[], INT  iMaxCounter)
{
    REGISTER UINT16                usIndex;
    REGISTER PLW_CLASS_TCB         ptcb;
    REGISTER PLW_LIST_LINE         plineVar;
    REGISTER PLW_CLASS_THREADVAR   pthreadvar;
    REGISTER ULONG                 ulNum = 0;
    
    usIndex = _ObjectGetIndex(ulId);
    
    if (LW_CPU_GET_CUR_NESTING()) {                                     /*  �������ж��е���            */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "called from ISR.\r\n");
        _ErrorHandle(ERROR_KERNEL_IN_ISR);
        return  (0);
    }
    
#if LW_CFG_ARG_CHK_EN > 0
    if (!_ObjectClassOK(ulId, _OBJECT_THREAD)) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "thread handle invalidate.\r\n");
        _ErrorHandle(ERROR_KERNEL_HANDLE_NULL);
        return  (0);
    }
    
    if (_Thread_Index_Invalid(usIndex)) {                               /*  ����߳���Ч��              */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "thread handle invalidate.\r\n");
        _ErrorHandle(ERROR_THREAD_NULL);
        return  (0);
    }
#endif

    __KERNEL_ENTER();                                                   /*  �����ں�                    */
    if (_Thread_Invalid(usIndex)) {
        __KERNEL_EXIT();                                                /*  �˳��ں�                    */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "thread handle invalidate.\r\n");
        _ErrorHandle(ERROR_THREAD_NULL);
        return  (0);
    }
    
    ptcb = _K_ptcbTCBIdTable[usIndex];
    
    for (plineVar   = ptcb->TCB_plinePrivateVars;                       /*  ����                        */
         (plineVar != LW_NULL) && (iMaxCounter > 0);
         plineVar   = _list_line_get_next(plineVar),
         ulNum++) {                                                     /*  ���������ֵ                */
        
        pthreadvar = _LIST_ENTRY(plineVar, LW_CLASS_THREADVAR, PRIVATEVAR_lineVarList);
        pulAddr[ulNum] = pthreadvar->PRIVATEVAR_pulAddress;
        iMaxCounter--;
    }
    
    __KERNEL_EXIT();                                                    /*  �˳��ں�                    */

    return  (ulNum);
}

#endif                                                                  /*  LW_CFG_SMP_EN == 0          */
                                                                        /*  (LW_CFG_THREAD_PRIVATE_VA...*/
                                                                        /*  (LW_CFG_MAX_THREAD_GLB_VA...*/
/*********************************************************************************************************
  END
*********************************************************************************************************/
