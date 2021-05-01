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
** ��   ��   ��: ThreadCleanup.c
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2009 �� 02 �� 04 ��
**
** ��        ��: ����ϵͳ�� pthread_cleanup_??? ֧��.

** BUG:
2011.04.23  �ں˷����ڴ�ʧ��, ��ʹ�� _DebugHandle ��ӡ������Ϣ.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
/*********************************************************************************************************
** ��������: API_ThreadCleanupPush
** ��������: ��һ���������ѹ�뺯����ջ.
** �䡡��  : pfuncRoutine      ��Ҫѹջ�ĺ���.
**           pvArg             ѹջ��������
** �䡡��  : ERROR CODE
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
#if LW_CFG_THREAD_EXT_EN > 0

LW_API  
ULONG  API_ThreadCleanupPush (VOIDFUNCPTR  pfuncRoutine, PVOID  pvArg)
{
             INTREG                 iregInterLevel;
             PLW_CLASS_TCB          ptcbCur;
    REGISTER __PLW_CLEANUP_ROUTINE  pcurNode;
    REGISTER __PLW_THREAD_EXT       ptex;
    
    if (pfuncRoutine == LW_NULL) {
        _ErrorHandle(ERROR_KERNEL_HOOK_NULL);                           /*  cleanup hook null           */
        return  (ERROR_KERNEL_HOOK_NULL);
    }
    
    pcurNode = (__PLW_CLEANUP_ROUTINE)__KHEAP_ALLOC(sizeof(__LW_CLEANUP_ROUTINE));
    if (pcurNode == LW_NULL) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "kernel low memory.\r\n");
        _ErrorHandle(ERROR_KERNEL_LOW_MEMORY);                          /*  �ں��ڴ治��                */
        return  (ERROR_KERNEL_LOW_MEMORY);
    }
    
    iregInterLevel = KN_INT_DISABLE();                                  /*  �ر��ж�                    */
    
    LW_TCB_GET_CUR(ptcbCur);
    ptex = &ptcbCur->TCB_texExt;
    
    pcurNode->CUR_pfuncClean = pfuncRoutine;
    pcurNode->CUR_pvArg      = pvArg;
    
    _LIST_MONO_LINK(&pcurNode->CUR_monoNext, ptex->TEX_pmonoCurHeader);
    ptex->TEX_pmonoCurHeader = &pcurNode->CUR_monoNext;
    
    KN_INT_ENABLE(iregInterLevel);                                      /*  ���ж�                    */
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: API_ThreadCleanupPushEx
** ��������: ��һ���������ѹ�뺯����ջ.
** �䡡��  : ulId              �߳̾��
**           pfuncRoutine      ��Ҫѹջ�ĺ���.
**           pvArg             ѹջ��������
** �䡡��  : ERROR CODE
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
ULONG  API_ThreadCleanupPushEx (LW_OBJECT_HANDLE  ulId, VOIDFUNCPTR  pfuncRoutine, PVOID  pvArg)
{
             INTREG                 iregInterLevel;
    REGISTER UINT16                 usIndex;
    REGISTER PLW_CLASS_TCB          ptcb;
    
    REGISTER __PLW_CLEANUP_ROUTINE  pcurNode;
    REGISTER __PLW_THREAD_EXT       ptex;
    
    pcurNode = (__PLW_CLEANUP_ROUTINE)__KHEAP_ALLOC(sizeof(__LW_CLEANUP_ROUTINE));
    if (pcurNode == LW_NULL) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "kernel low memory.\r\n");
        _ErrorHandle(ERROR_KERNEL_LOW_MEMORY);                          /*  �ں��ڴ治��                */
        return  (ERROR_KERNEL_LOW_MEMORY);
    }
    
    usIndex = _ObjectGetIndex(ulId);
    
#if LW_CFG_ARG_CHK_EN > 0
    if (pfuncRoutine == LW_NULL) {
        __KHEAP_FREE(pcurNode);
        _ErrorHandle(ERROR_KERNEL_HOOK_NULL);                           /*  cleanup hook null           */
        return  (ERROR_KERNEL_HOOK_NULL);
    }
    if (!_ObjectClassOK(ulId, _OBJECT_THREAD)) {                        /*  ��� ID ������Ч��          */
        __KHEAP_FREE(pcurNode);
        _DebugHandle(__ERRORMESSAGE_LEVEL, "thread handle invalidate.\r\n");
        _ErrorHandle(ERROR_KERNEL_HANDLE_NULL);
        return  (ERROR_KERNEL_HANDLE_NULL);
    }
    if (_Thread_Index_Invalid(usIndex)) {                               /*  ����߳���Ч��              */
        __KHEAP_FREE(pcurNode);
        _DebugHandle(__ERRORMESSAGE_LEVEL, "thread handle invalidate.\r\n");
        _ErrorHandle(ERROR_THREAD_NULL);
        return  (ERROR_THREAD_NULL);
    }
#endif
    
    __KERNEL_ENTER();                                                   /*  �����ں�                    */
    if (_Thread_Invalid(usIndex)) {
        __KERNEL_EXIT();                                                /*  �˳��ں�                    */
        __KHEAP_FREE(pcurNode);
        _DebugHandle(__ERRORMESSAGE_LEVEL, "thread handle invalidate.\r\n");
        _ErrorHandle(ERROR_THREAD_NULL);
        return  (ERROR_THREAD_NULL);
    }
    
    iregInterLevel = KN_INT_DISABLE();                                  /*  �ر��ж�                    */
    
    ptcb = _K_ptcbTCBIdTable[usIndex];
    ptex = &ptcb->TCB_texExt;
    
    pcurNode->CUR_pfuncClean = pfuncRoutine;
    pcurNode->CUR_pvArg      = pvArg;
    
    _LIST_MONO_LINK(&pcurNode->CUR_monoNext, ptex->TEX_pmonoCurHeader);
    ptex->TEX_pmonoCurHeader = &pcurNode->CUR_monoNext;
    
    __KERNEL_EXIT_IRQ(iregInterLevel);                                  /*  �˳��ں˲����ж�          */
        
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: API_ThreadCleanupPop
** ��������: ��һ��ѹջ�������в��ͷ�
** �䡡��  : bRun          �Ƿ���Ҫ����
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
VOID  API_ThreadCleanupPop (BOOL  bRun)
{
             INTREG                 iregInterLevel;
             PLW_CLASS_TCB          ptcbCur;
    REGISTER __PLW_THREAD_EXT       ptex;
    REGISTER __PLW_CLEANUP_ROUTINE  pcurNode;
    
    iregInterLevel = KN_INT_DISABLE();                                  /*  �ر��ж�                    */
    
    LW_TCB_GET_CUR(ptcbCur);
    ptex = &ptcbCur->TCB_texExt;
    
    pcurNode = (__PLW_CLEANUP_ROUTINE)ptex->TEX_pmonoCurHeader;
    if (pcurNode) {
        _list_mono_next(&ptex->TEX_pmonoCurHeader);
        KN_INT_ENABLE(iregInterLevel);                                  /*  ���ж�                    */
        if (bRun) {
            LW_SOFUNC_PREPARE(pcurNode->CUR_pfuncClean);
            pcurNode->CUR_pfuncClean(pcurNode->CUR_pvArg);
        }
        __KHEAP_FREE(pcurNode);
    
    } else {
        KN_INT_ENABLE(iregInterLevel);                                  /*  ���ж�                    */
    }
}

#endif                                                                  /*  LW_CFG_THREAD_EXT_EN > 0    */
/*********************************************************************************************************
  END
*********************************************************************************************************/
