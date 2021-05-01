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
** ��   ��   ��: loader_vpdebug.c
**
** ��   ��   ��: Han.hui (����)
**
** �ļ���������: 2018 �� 05 �� 24 ��
**
** ��        ��: ģ��� VPROCESS ���̵���֧��.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "SylixOS.h"
#if LW_CFG_GDB_EN > 0 && LW_CFG_NET_EN > 0
#include "lwip/tcpip.h"
#endif                                                                  /*  GDB_EN && NET_EN            */
/*********************************************************************************************************
  �ü�֧��
*********************************************************************************************************/
#if LW_CFG_MODULELOADER_EN > 0
#include "../include/loader_lib.h"
#if LW_CFG_VMM_EN > 0
#include "../SylixOS/kernel/vmm/pageTable.h"
#endif                                                                  /*  LW_CFG_VMM_EN > 0           */
/*********************************************************************************************************
** ��������: vprocDebugCriResLock
** ��������: �ؼ�����Դ����
** �䡡��  : NONE
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
#if LW_CFG_GDB_EN > 0

static VOID  vprocDebugCriResLock (VOID)
{
#if LW_CFG_VMM_EN > 0
    __VMM_LOCK();
#endif                                                                  /*  LW_CFG_VMM_EN > 0           */
#if LW_CFG_NET_EN > 0
    LOCK_TCPIP_CORE();
#endif
    API_SemaphoreMPend(_K_pheapSystem->HEAP_ulLock, LW_OPTION_WAIT_INFINITE);
}
/*********************************************************************************************************
** ��������: vprocDebugCriResUnlock
** ��������: �ؼ�����Դ����
** �䡡��  : NONE
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static VOID  vprocDebugCriResUnlock (VOID)
{
#if LW_CFG_VMM_EN > 0
    __VMM_UNLOCK();
#endif                                                                  /*  LW_CFG_VMM_EN > 0           */
#if LW_CFG_NET_EN > 0
    UNLOCK_TCPIP_CORE();
#endif
    API_SemaphoreMPost(_K_pheapSystem->HEAP_ulLock);
}
/*********************************************************************************************************
** ��������: vprocDebugStop
** ��������: ֹͣ�����ڵ������߳�
** �䡡��  : pvVProc    ���̿��ƿ�ָ��
**           ptcbExcp   ��������֮��
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
** ע  ��  : LW_VP_LOCK() ��֤�˽��������񲻿ɱ�ɾ����.
*********************************************************************************************************/
VOID  vprocDebugStop (PVOID  pvVProc, PLW_CLASS_TCB  ptcbExcp)
{
    LW_LD_VPROC    *pvproc = (LW_LD_VPROC *)pvVProc;
    PLW_LIST_LINE   plineTemp;
    PLW_CLASS_TCB   ptcb;
    
    LW_VP_LOCK(pvproc);                                                 /*  ������ǰ����                */
    vprocDebugCriResLock();                                             /*  �ȴ����Թؼ�����Դ          */
    for (plineTemp  = pvproc->VP_plineThread;
         plineTemp != LW_NULL;
         plineTemp  = _list_line_get_next(plineTemp)) {
    
        ptcb = _LIST_ENTRY(plineTemp, LW_CLASS_TCB, TCB_lineProcess);
        if (ptcb == ptcbExcp) {
            continue;                                                   /*  ��ֹͣ������                */
        }
        if (ptcb->TCB_iDeleteProcStatus == LW_TCB_DELETE_PROC_NONE) {
            __KERNEL_ENTER();                                           /*  �����ں�                    */
            _ThreadStop(ptcb);
            __KERNEL_EXIT();                                            /*  �˳��ں�                    */
                                                                        /*  ���뱣֤�̱߳���ȫֹͣ      */
            __KERNEL_ENTER();                                           /*  �����ں�                    */
            _ThreadDebugUnpendSem(ptcb);
            __KERNEL_EXIT();                                            /*  �˳��ں�                    */
        }
    }
    vprocDebugCriResUnlock();                                           /*  �ͷŵ��Թؼ�����Դ          */
    LW_VP_UNLOCK(pvproc);                                               /*  ������ǰ����                */
}
/*********************************************************************************************************
** ��������: vprocDebugContinue
** ��������: �ָ������ڵ�����ֹͣ���߳�
** �䡡��  : pvVProc    ���̿��ƿ�ָ��
**           ptcbExcp   ��������֮��
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
VOID  vprocDebugContinue (PVOID  pvVProc, PLW_CLASS_TCB  ptcbExcp)
{
    LW_LD_VPROC    *pvproc = (LW_LD_VPROC *)pvVProc;
    PLW_LIST_LINE   plineTemp;
    PLW_CLASS_TCB   ptcb;
    
    LW_VP_LOCK(pvproc);                                                 /*  ������ǰ����                */
    for (plineTemp  = pvproc->VP_plineThread;
         plineTemp != LW_NULL;
         plineTemp  = _list_line_get_next(plineTemp)) {
    
        ptcb = _LIST_ENTRY(plineTemp, LW_CLASS_TCB, TCB_lineProcess);
        if (ptcb == ptcbExcp) {
            continue;                                                   /*  ������������                */
        }
        if (ptcb->TCB_iDeleteProcStatus == LW_TCB_DELETE_PROC_NONE) {
            __KERNEL_ENTER();                                           /*  �����ں�                    */
            _ThreadContinue(ptcb, LW_FALSE);
            __KERNEL_EXIT();                                            /*  �˳��ں�                    */
        }
    }
    LW_VP_UNLOCK(pvproc);                                               /*  ������ǰ����                */
}
/*********************************************************************************************************
** ��������: vprocDebugThreadStop
** ��������: ָֹͣ�����߳�
** �䡡��  : pvVProc    ���̿��ƿ�ָ��
**           ulId       �߳̾��
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
** ע  ��  : LW_VP_LOCK() ��֤�˽��������񲻿ɱ�ɾ����.
*********************************************************************************************************/
VOID  vprocDebugThreadStop (PVOID  pvVProc, LW_OBJECT_HANDLE  ulId)
{
             LW_LD_VPROC   *pvproc = (LW_LD_VPROC *)pvVProc;
    REGISTER UINT16         usIndex;
    REGISTER PLW_CLASS_TCB  ptcb;
    
    usIndex = _ObjectGetIndex(ulId);
    
#if LW_CFG_ARG_CHK_EN > 0
    if (!_ObjectClassOK(ulId, _OBJECT_THREAD)) {                        /*  ��� ID ������Ч��          */
        _ErrorHandle(ERROR_KERNEL_HANDLE_NULL);
        return;
    }
    if (usIndex >= LW_CFG_MAX_THREADS) {                                /*  �����ÿ����̶߳�ջ��Ϣ    */
        _ErrorHandle(ERROR_THREAD_NULL);
        return;
    }
#endif

    LW_VP_LOCK(pvproc);                                                 /*  ������ǰ����                */
    vprocDebugCriResLock();                                             /*  �ȴ����Թؼ�����Դ          */
    
    __KERNEL_ENTER();                                                   /*  �����ں�                    */
    if (_Thread_Invalid(usIndex)) {
        goto    __error;
    }
    
    ptcb = _K_ptcbTCBIdTable[usIndex];
    
    if (__LW_VP_GET_TCB_PROC(ptcb) != (LW_LD_VPROC *)pvVProc) {         /*  ���̿��ƿ����              */
        goto    __error;
    }
    
    if (ptcb->TCB_iDeleteProcStatus) {
        goto    __error;
    }
    
    _ThreadStop(ptcb);
    __KERNEL_EXIT();                                                    /*  �˳��ں�                    */
                                                                        /*  ���뱣֤�̱߳���ȫֹͣ      */
    __KERNEL_ENTER();                                                   /*  �����ں�                    */
    _ThreadDebugUnpendSem(ptcb);
    __KERNEL_EXIT();                                                    /*  �˳��ں�                    */
    
    vprocDebugCriResUnlock();                                           /*  �ͷŵ��Թؼ�����Դ          */
    LW_VP_UNLOCK(pvproc);                                               /*  ������ǰ����                */
    return;
    
__error:
    __KERNEL_EXIT();                                                    /*  �˳��ں�                    */
    vprocDebugCriResUnlock();                                           /*  �ͷŵ��Թؼ�����Դ          */
    LW_VP_UNLOCK(pvproc);                                               /*  ������ǰ����                */
    _ErrorHandle(ERROR_THREAD_NULL);
}
/*********************************************************************************************************
** ��������: vprocDebugThreadContinue
** ��������: �ָ�ָ�����߳�
** �䡡��  : pvVProc    ���̿��ƿ�ָ��
**           ulId       �߳̾��
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
VOID  vprocDebugThreadContinue (PVOID  pvVProc, LW_OBJECT_HANDLE  ulId)
{
    REGISTER UINT16         usIndex;
    REGISTER PLW_CLASS_TCB  ptcb;
    
    usIndex = _ObjectGetIndex(ulId);
    
#if LW_CFG_ARG_CHK_EN > 0
    if (!_ObjectClassOK(ulId, _OBJECT_THREAD)) {                        /*  ��� ID ������Ч��          */
        _ErrorHandle(ERROR_KERNEL_HANDLE_NULL);
        return;
    }
    if (usIndex >= LW_CFG_MAX_THREADS) {                                /*  �����ÿ����̶߳�ջ��Ϣ    */
        _ErrorHandle(ERROR_THREAD_NULL);
        return;
    }
#endif

    __KERNEL_ENTER();                                                   /*  �����ں�                    */
    if (_Thread_Invalid(usIndex)) {
        goto    __error;
    }
    
    ptcb = _K_ptcbTCBIdTable[usIndex];
    
    if (__LW_VP_GET_TCB_PROC(ptcb) != (LW_LD_VPROC *)pvVProc) {         /*  ���̿��ƿ����              */
        goto    __error;
    }
    
    _ThreadContinue(ptcb, LW_FALSE);
    __KERNEL_EXIT();                                                    /*  �˳��ں�                    */
    return;

__error:
    __KERNEL_EXIT();                                                    /*  �˳��ں�                    */
    _ErrorHandle(ERROR_THREAD_NULL);
}
/*********************************************************************************************************
** ��������: vprocDebugThreadGet
** ��������: ��ý����ڵ������߳̾��
** �䡡��  : pvVProc        ���̿��ƿ�ָ��
**           ulId           �̱߳�
**           uiTableNum     ������
** �䡡��  : ���߳���
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
UINT  vprocDebugThreadGet (PVOID  pvVProc, LW_OBJECT_HANDLE  ulId[], UINT   uiTableNum)
{
    LW_LD_VPROC    *pvproc = (LW_LD_VPROC *)pvVProc;
    PLW_LIST_LINE   plineTemp;
    PLW_CLASS_TCB   ptcb;
    UINT            uiNum = 0;
    
    __KERNEL_ENTER();                                                   /*  �����ں�                    */
    for (plineTemp  = pvproc->VP_plineThread;
         plineTemp != LW_NULL;
         plineTemp  = _list_line_get_next(plineTemp)) {
    
        ptcb = _LIST_ENTRY(plineTemp, LW_CLASS_TCB, TCB_lineProcess);
        if (ptcb->TCB_iDeleteProcStatus) {
            continue;                                                   /*  �Ѿ���ɾ��������            */
        }
        if (uiNum < uiTableNum) {
            ulId[uiNum] = ptcb->TCB_ulId;
            uiNum++;
        }
    }
    __KERNEL_EXIT();                                                    /*  �˳��ں�                    */

    return  (uiNum);
}

#endif                                                                  /*  LW_CFG_GDB_EN > 0           */
#endif                                                                  /*  LW_CFG_MODULELOADER_EN > 0  */
/*********************************************************************************************************
  END
*********************************************************************************************************/
