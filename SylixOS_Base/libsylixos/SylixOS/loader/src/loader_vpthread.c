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
** ��   ��   ��: loader_vpthread.c
**
** ��   ��   ��: Han.hui (����)
**
** �ļ���������: 2016 �� 11 �� 23 ��
**
** ��        ��: ģ��� VPROCESS �������̹߳���.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "SylixOS.h"
/*********************************************************************************************************
  �ü�֧��
*********************************************************************************************************/
#if LW_CFG_MODULELOADER_EN > 0
#include "../include/loader_lib.h"
/*********************************************************************************************************
** ��������: vprocThreadAdd
** ��������: ��һ���̼߳������
** �䡡��  : pvVProc    ���̿��ƿ�ָ��
**           ptcb       �߳̿��ƿ�
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
VOID  vprocThreadAdd (PVOID   pvVProc, PLW_CLASS_TCB  ptcb)
{
    LW_LD_VPROC  *pvproc = (LW_LD_VPROC *)pvVProc;
    
    if (pvproc) {
        __KERNEL_ENTER();                                               /*  �����ں�                    */
        if (_LIST_LINE_IS_NOTLNK(&ptcb->TCB_lineProcess)) {
            _List_Line_Add_Tail(&ptcb->TCB_lineProcess, &pvproc->VP_plineThread);
        }
        __KERNEL_EXIT();                                                /*  �˳��ں�                    */
    }
}
/*********************************************************************************************************
** ��������: vprocThreadDelete
** ��������: ��һ���̴߳ӽ��̱���ɾ��
** �䡡��  : pvVProc    ���̿��ƿ�ָ��
**           ptcb       �߳̿��ƿ�
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
** ע  ��  : ���ڴ˺�����ɾ������ṹ, ���Ա���ʹ�� VP_LOCK ��֤��ȫ.
*********************************************************************************************************/
VOID  vprocThreadDelete (PVOID   pvVProc, PLW_CLASS_TCB  ptcb)
{
    LW_LD_VPROC  *pvproc = (LW_LD_VPROC *)pvVProc;

    if (pvproc) {
        LW_VP_LOCK(pvproc);                                             /*  ������ǰ����                */
        __KERNEL_ENTER();                                               /*  �����ں�                    */
        if (!_LIST_LINE_IS_NOTLNK(&ptcb->TCB_lineProcess)) {
            _List_Line_Del(&ptcb->TCB_lineProcess, &pvproc->VP_plineThread);
        }
        __KERNEL_EXIT();                                                /*  �˳��ں�                    */
        LW_VP_UNLOCK(pvproc);                                           /*  ������ǰ����                */
    }
}
/*********************************************************************************************************
** ��������: vprocThreadNum
** ��������: ͳ�ƽ������̸߳���
** �䡡��  : pid        ���� ID
**           pulNum     ͳ�Ƹ���
** �䡡��  : ERROR or OK
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
INT  vprocThreadNum (pid_t  pid, ULONG  *pulNum)
{
    LW_LD_VPROC     *pvproc;
    PLW_LIST_LINE    plineTemp;
    PLW_CLASS_TCB    ptcb;
    
    if (!pulNum) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    
    pvproc = vprocGet(pid);
    if (!pvproc) {
        return  (PX_ERROR);
    }
    
    __KERNEL_ENTER();                                                   /*  �����ں�                    */
    for (plineTemp  = pvproc->VP_plineThread;
         plineTemp != LW_NULL;
         plineTemp  = _list_line_get_next(plineTemp)) {

        ptcb = _LIST_ENTRY(plineTemp, LW_CLASS_TCB, TCB_lineProcess);
        if (ptcb->TCB_iDeleteProcStatus) {
            continue;                                                   /*  �Ѿ���ɾ��������            */
        }
        (*pulNum)++;
    }
    __KERNEL_EXIT();                                                    /*  �˳��ں�                    */
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: vprocThreadKill
** ��������: ɱ�������ڵĳ����߳���������߳�
** �䡡��  : pvVProc        ���̿��ƿ�ָ��
**           ptcbExcp       ����֮��
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
** ע  ��  : ���ڴ˺����м佫�ͷ�ϵͳ����Ȩ, ���Ա���ʹ�� VP_LOCK ��֤��ȫ.
*********************************************************************************************************/
VOID  vprocThreadKill (PVOID  pvVProc, PLW_CLASS_TCB  ptcbExcp)
{
    LW_LD_VPROC      *pvproc = (LW_LD_VPROC *)pvVProc;
    PLW_LIST_LINE     plineTemp;
    PLW_CLASS_TCB     ptcb;
    
    LW_VP_LOCK(pvproc);                                                 /*  ������ǰ����                */
    for (plineTemp  = pvproc->VP_plineThread;
         plineTemp != LW_NULL;
         plineTemp  = _list_line_get_next(plineTemp)) {
        
        ptcb = _LIST_ENTRY(plineTemp, LW_CLASS_TCB, TCB_lineProcess);
        if (ptcb->TCB_iDeleteProcStatus) {
            continue;                                                   /*  �Ѿ���ɾ��������            */
        }
        if (ptcb != ptcbExcp) {                                         /*  ptcbCur Ϊ���߳�            */
            _excJobAdd((VOIDFUNCPTR)kill, (PVOID)ptcb->TCB_ulId, (PVOID)SIGKILL, 0, 0, 0, 0);
        }
    }
    LW_VP_UNLOCK(pvproc);                                               /*  ������ǰ����                */
}
/*********************************************************************************************************
** ��������: vprocThreadTraversal
** ��������: ���������ڵ������߳� (Ϊ�����Ч�ʴ˺��������� KERNEL)
** �䡡��  : pid            ���� id
**           pfunc          �ص�����
**           pvArg[0 ~ 5]   ������������
** �䡡��  : ERROR or OK
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
INT  vprocThreadTraversal (pid_t          pid,
                           VOIDFUNCPTR    pfunc,
                           PVOID          pvArg0,
                           PVOID          pvArg1,
                           PVOID          pvArg2,
                           PVOID          pvArg3,
                           PVOID          pvArg4,
                           PVOID          pvArg5)
{
    LW_LD_VPROC      *pvproc;
    PLW_LIST_LINE     plineTemp;
    PLW_CLASS_TCB     ptcb;

    __KERNEL_ENTER();                                                   /*  �����ں�                    */
    pvproc = vprocGet(pid);
    if (!pvproc) {
        __KERNEL_EXIT();                                                /*  �˳��ں�                    */
        return  (PX_ERROR);
    }

    for (plineTemp  = pvproc->VP_plineThread;
         plineTemp != LW_NULL;
         plineTemp  = _list_line_get_next(plineTemp)) {

        ptcb = _LIST_ENTRY(plineTemp, LW_CLASS_TCB, TCB_lineProcess);
        if (ptcb->TCB_iDeleteProcStatus) {
            continue;                                                   /*  �Ѿ���ɾ��������            */
        }
        pfunc(ptcb, pvArg0, pvArg1, pvArg2, pvArg3, pvArg4, pvArg5);
    }
    __KERNEL_EXIT();                                                    /*  �˳��ں�                    */

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: vprocThreadSigaction
** ��������: ���ý�������������� sigaction
** �䡡��  : pvVProc        ���̿��ƿ�ָ��
**           pfunc          ���ú���
**           iSigIndex      �ź��±�
**           psigactionNew  �źž��
** �䡡��  : ERROR or OK
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
#if LW_CFG_SIGNAL_EN > 0

INT  vprocThreadSigaction (PVOID  pvVProc, VOIDFUNCPTR  pfunc, INT  iSigIndex, 
                           const struct sigaction  *psigactionNew)
{
    LW_LD_VPROC    *pvproc = (LW_LD_VPROC *)pvVProc;
    PLW_LIST_LINE   plineTemp;
    PLW_CLASS_TCB   ptcb;
    PLW_CLASS_TCB   ptcbCur;
    
    if (!pvproc) {
        _ErrorHandle(ESRCH);
        return  (PX_ERROR);
    }
    
    LW_TCB_GET_CUR_SAFE(ptcbCur);
    
    __KERNEL_ENTER();                                                   /*  �����ں�                    */
    for (plineTemp  = pvproc->VP_plineThread;
         plineTemp != LW_NULL;
         plineTemp  = _list_line_get_next(plineTemp)) {
         
        ptcb = _LIST_ENTRY(plineTemp, LW_CLASS_TCB, TCB_lineProcess);
        if (ptcb->TCB_iDeleteProcStatus) {
            continue;                                                   /*  �Ѿ���ɾ��������            */
        }
        if (ptcb != ptcbCur) {
            pfunc(ptcb, iSigIndex, psigactionNew);
        }
    }
    __KERNEL_EXIT();                                                    /*  �˳��ں�                    */

    return  (ERROR_NONE);
}

#endif                                                                  /*  LW_CFG_SIGNAL_EN > 0        */
/*********************************************************************************************************
** ��������: vprocThreadAffinity
** ��������: ���ý����������̵߳��ȵ� CPU ����
** �䡡��  : pvVProc       ���̿��ƿ�ָ��
**           stSize        CPU ���뼯�ڴ��С
**           pcpuset       CPU ����
** �䡡��  : ERROR or OK
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
#if LW_CFG_SMP_EN > 0

INT  vprocThreadAffinity (PVOID  pvVProc, size_t  stSize, const PLW_CLASS_CPUSET  pcpuset)
{
    LW_LD_VPROC    *pvproc = (LW_LD_VPROC *)pvVProc;
    PLW_LIST_LINE   plineTemp;
    PLW_CLASS_TCB   ptcb, ptcbCur;

    if (!pvproc) {
        _ErrorHandle(ESRCH);
        return  (PX_ERROR);
    }

    LW_TCB_GET_CUR_SAFE(ptcbCur);

    LW_VP_LOCK(pvproc);                                                 /*  ������ǰ����                */
    for (plineTemp  = pvproc->VP_plineThread;
         plineTemp != LW_NULL;
         plineTemp  = _list_line_get_next(plineTemp)) {
    
        ptcb = _LIST_ENTRY(plineTemp, LW_CLASS_TCB, TCB_lineProcess);
        if (__THREAD_LOCK_GET(ptcb)) {                                  /*  �ⲿû������������          */
            continue;                                                   /*  �̱߳�����                  */
        }

        if (ptcb == ptcbCur) {
            __KERNEL_ENTER();                                           /*  �����ں�                    */
            _ThreadSetAffinity(ptcb, stSize, pcpuset);
            __KERNEL_EXIT();                                            /*  �˳��ں�                    */

        } else if (ptcb->TCB_iDeleteProcStatus == LW_TCB_DELETE_PROC_NONE) {
            __KERNEL_ENTER();                                           /*  �����ں�                    */
            _ThreadStop(ptcb);
            __KERNEL_EXIT();                                            /*  �˳��ں� (���ܲ�������)     */

            __KERNEL_ENTER();                                           /*  �����ں�                    */
            _ThreadSetAffinity(ptcb, stSize, pcpuset);
            _ThreadContinue(ptcb, LW_FALSE);
            __KERNEL_EXIT();                                            /*  �˳��ں�                    */
        }
    }
    LW_VP_UNLOCK(pvproc);                                               /*  ������ǰ����                */
    
    return  (ERROR_NONE);
}

#endif                                                                  /*  LW_CFG_SMP_EN > 0           */
#endif                                                                  /*  LW_CFG_MODULELOADER_EN > 0  */
/*********************************************************************************************************
  END
*********************************************************************************************************/
