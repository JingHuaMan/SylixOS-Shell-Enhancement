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
** ��   ��   ��: _ThreadMiscLib.c
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2006 �� 12 �� 18 ��
**
** ��        ��: �̸߳ı����ȼ���

** BUG
2008.03.30  ʹ���µľ���������.
2012.07.04  �ϲ� _ThreadDeleteWaitDeath() ������.
2012.08.24  ���� _ThreadUserGet() ����.
2012.10.23  ���� _ThreadContinue() ����.
2013.07.18  ���� _ThreadStop() ����.
2013.08.27  �����ں��¼������.
2013.12.02  ��״̬�ı亯���ƶ��� _ThreadStatus.c �ļ���.
2014.05.13  _ThreadContinue() ֧��ǿ�ƻָ�.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
/*********************************************************************************************************
  �������
*********************************************************************************************************/
#if LW_CFG_MODULELOADER_EN > 0
#include "../SylixOS/loader/include/loader_vppatch.h"
#endif                                                                  /*  LW_CFG_MODULELOADER_EN > 0  */
/*********************************************************************************************************
** ��������: _ThreadUserGet
** ��������: ��ȡ�߳��û���Ϣ
** �䡡��  : ulId          �߳� ID
**           puid          uid
**           pgid          gid
** �䡡��  : ERROR
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
ULONG  _ThreadUserGet (LW_HANDLE  ulId, uid_t  *puid, gid_t  *pgid)
{
    REGISTER UINT16         usIndex;
    REGISTER PLW_CLASS_TCB  ptcb;
    
    usIndex = _ObjectGetIndex(ulId);
    
    if (!_ObjectClassOK(ulId, _OBJECT_THREAD)) {                        /*  ��� ID ������Ч��          */
        return  (ERROR_KERNEL_HANDLE_NULL);
    }
    
    if (_Thread_Index_Invalid(usIndex)) {                               /*  ����߳���Ч��              */
        return  (ERROR_THREAD_NULL);
    }
    
    __KERNEL_ENTER();                                                   /*  �����ں�                    */
    if (_Thread_Invalid(usIndex)) {
        __KERNEL_EXIT();                                                /*  �˳��ں�                    */
        return  (ERROR_THREAD_NULL);
    }
    
    ptcb = _K_ptcbTCBIdTable[usIndex];
    
    if (puid) {
        *puid = ptcb->TCB_uid;
    }
    if (pgid) {
        *pgid = ptcb->TCB_gid;
    }
    __KERNEL_EXIT();                                                    /*  �˳��ں�                    */
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: _ThreadDeleteWaitDeath
** ��������: ����Ҫɾ�����߳̽��뽩��״̬.
** �䡡��  : ptcbDel         ��Ҫɾ�����߳̿��ƿ�
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
VOID  _ThreadDeleteWaitDeath (PLW_CLASS_TCB  ptcbDel)
{
    __KERNEL_ENTER();
    _ThreadStatusChange(ptcbDel, LW_TCB_REQ_WDEATH);
    __KERNEL_EXIT();                                                    /*  �ȴ��Է�״̬ת�����        */
}
/*********************************************************************************************************
** ��������: _ThreadSched
** ��������: �߳̽������Խ��е���
** �䡡��  : ptcbCur       ��ǰ������ƿ�
** �䡡��  : ����������ֵ
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
INT  _ThreadSched (PLW_CLASS_TCB  ptcbCur)
{
    LW_KERNEL_JOB_EXEC();                                               /*  ����ִ���첽��������        */
    
    return  (__KERNEL_SCHED());                                         /*  ����һ�ε���                */
}
/*********************************************************************************************************
** ��������: _ThreadLock
** ��������: �߳�������ǰ CPU ���� (�������ж��е���)
** �䡡��  : NONE
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
VOID  _ThreadLock (VOID)
{
    PLW_CLASS_TCB   ptcbCur;
    
    if (!LW_SYS_STATUS_IS_RUNNING()) {                                  /*  ϵͳ�����Ѿ�����            */
        return;
    }
    
    LW_TCB_GET_CUR_SAFE(ptcbCur);
    
    if (__THREAD_LOCK_GET(ptcbCur) != __ARCH_ULONG_MAX) {
        __THREAD_LOCK_INC(ptcbCur);
    }
    KN_SMP_MB();
}
/*********************************************************************************************************
** ��������: _ThreadUnlock
** ��������: �߳̽�����ǰ CPU ���� (�������ж��е���)
** �䡡��  : NONE
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
VOID  _ThreadUnlock (VOID)
{
    INTREG          iregInterLevel;
    PLW_CLASS_CPU   pcpuCur;
    PLW_CLASS_TCB   ptcbCur;
    BOOL            bTrySched = LW_FALSE;
   
    if (!LW_SYS_STATUS_IS_RUNNING()) {                                  /*  ϵͳ�����Ѿ�����            */
        return;
    }
    
    LW_TCB_GET_CUR_SAFE(ptcbCur);
    
    KN_SMP_MB();
    if (__THREAD_LOCK_GET(ptcbCur)) {
        __THREAD_LOCK_DEC(ptcbCur);                                     /*  ��������                    */
    }
    
    iregInterLevel = KN_INT_DISABLE();                                  /*  ���ж�, ��ֹ CPU ����       */
    
    pcpuCur = LW_CPU_GET_CUR();
    if (__COULD_SCHED(pcpuCur, 0)) {
        bTrySched = LW_TRUE;                                            /*  ��Ҫ���Ե���                */
    }
    
    KN_INT_ENABLE(iregInterLevel);

    if (bTrySched) {
        _ThreadSched(ptcbCur);                                          /*  ���Ե���                    */
    }
}
/*********************************************************************************************************
** ��������: _ThreadStop
** ��������: ֹͣ�߳� (�����ں˺󱻵���)
** �䡡��  : ptcb          �߳̿��ƿ�
** �䡡��  : ERROR
** ȫ�ֱ���: 
** ����ģ��: 
** ע  ��  : ptcb != current tcb
*********************************************************************************************************/
ULONG  _ThreadStop (PLW_CLASS_TCB  ptcb)
{
    MONITOR_EVT_LONG1(MONITOR_EVENT_ID_THREAD, MONITOR_EVENT_THREAD_STOP, 
                      ptcb->TCB_ulId, LW_NULL);

    return  (_ThreadStatusChange(ptcb, LW_TCB_REQ_STOP));
}
/*********************************************************************************************************
** ��������: _ThreadContinue
** ��������: �� _ThreadStop() ���̼߳���ִ�� (�����ں˺󱻵���)
** �䡡��  : ptcb          �߳̿��ƿ�
**           bForce        ��������ݹ��Ƿ�ǿ�ƻָ�.
** �䡡��  : ERROR
** ȫ�ֱ���: 
** ����ģ��: 
** ע  ��  : ptcb != current tcb
*********************************************************************************************************/
ULONG  _ThreadContinue (PLW_CLASS_TCB  ptcb, BOOL  bForce)
{
             INTREG         iregInterLevel;
    REGISTER PLW_CLASS_PCB  ppcb;
    
    MONITOR_EVT_LONG1(MONITOR_EVENT_ID_THREAD, MONITOR_EVENT_THREAD_CONT, 
                      ptcb->TCB_ulId, LW_NULL);
    
    iregInterLevel = KN_INT_DISABLE();                                  /*  �ر��ж�                    */
    
    if (!(ptcb->TCB_usStatus & LW_THREAD_STATUS_STOP)) {                /*  �Ѿ��˳��� STOP ״̬        */
        KN_INT_ENABLE(iregInterLevel);                                  /*  ���ж�                    */
        return  (ERROR_NONE);
    }
    
    if (ptcb->TCB_ulStopNesting) {
        ptcb->TCB_ulStopNesting--;
        if ((ptcb->TCB_ulStopNesting == 0ul) || bForce) {
            ptcb->TCB_ulStopNesting = 0ul;
            ptcb->TCB_usStatus &= (~LW_THREAD_STATUS_STOP);
            if (__LW_THREAD_IS_READY(ptcb)) {                           /*  ���� ?                      */
                ptcb->TCB_ucSchedActivate = LW_SCHED_ACT_INTERRUPT;     /*  �жϼ��ʽ                */
                ppcb = _GetPcb(ptcb);
                __ADD_TO_READY_RING(ptcb, ppcb);                        /*  ���뵽������ȼ�������      */
            }
        }
    }
    
    KN_INT_ENABLE(iregInterLevel);                                      /*  ���ж�                    */

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: _ThreadDebugUnpendSem
** ��������: �̲߳��ȴ��κε��Թؼ��ź��� (�����ں˺󱻵���)
** �䡡��  : ptcb      �߳̿��ƿ�ָ��
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
** ע  ��  : ������ֹͣ���߳�ǰ����ô˺���, ��ֹ�������ͷŵ���Դ���Ѿ� stop ���̻߳�ȡ.
*********************************************************************************************************/
#if LW_CFG_GDB_EN > 0

VOID  _ThreadDebugUnpendSem (PLW_CLASS_TCB  ptcb)
{
    INTREG          iregInterLevel;
    PLW_CLASS_PCB   ppcb;

    iregInterLevel = KN_INT_DISABLE();                                  /*  �ر��ж�                    */
    
    if (__LW_THREAD_IS_READY(ptcb)) {                                   /*  ���ھ���״̬, ֱ���˳�      */
        KN_INT_ENABLE(iregInterLevel);                                  /*  ���ж�                    */
        return;
    }
    
    ppcb = _GetPcb(ptcb);                                               /*  ������ȼ����ƿ�            */
    if (ptcb->TCB_usStatus & LW_THREAD_STATUS_SEM) {
        if (ptcb->TCB_peventPtr &&
            (ptcb->TCB_peventPtr->EVENT_ulOption & 
             LW_OPTION_OBJECT_DEBUG_UNPEND)) {                          /*  ���Թؼ���Դ                */
            if (ptcb->TCB_usStatus & LW_THREAD_STATUS_DELAY) {
                __DEL_FROM_WAKEUP_LINE(ptcb);                           /*  �ӵȴ�����ɾ��              */
                ptcb->TCB_ulDelay = 0ul;
            }
            
            ptcb->TCB_usStatus &= (~LW_THREAD_STATUS_SEM);              /*  �ȴ���ʱ����¼��ȴ�λ      */
            ptcb->TCB_ucWaitTimeout = LW_WAIT_TIME_OUT;                 /*  �ȴ���ʱ                    */
            
            _EventUnQueue(ptcb);
            
            if (__LW_THREAD_IS_READY(ptcb)) {
                ptcb->TCB_ucSchedActivate = LW_SCHED_ACT_OTHER;
                __ADD_TO_READY_RING(ptcb, ppcb);                        /*  ���������                  */
            }
            
            if (ptcb->TCB_iSchedRet == ERROR_NONE) {
                ptcb->TCB_iSchedRet =  LW_SIGNAL_RESTART;               /*  �´ε�����Ҫ���µȴ�        */
            }
        }
    }
    
    KN_INT_ENABLE(iregInterLevel);                                      /*  ���ж�                    */
}

#endif                                                                  /*  LW_CFG_GDB_EN > 0           */
/*********************************************************************************************************
** ��������: _ThreadMigrateToProc
** ��������: ��ָ���߳�ת��Ϊ�������߳�
** �䡡��  : ulId          �߳� ID
**           pvVProc       ���̿��ƿ�
**           bMain         �Ƿ�Ϊ���߳�
** �䡡��  : ERROR
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
#if LW_CFG_MODULELOADER_EN > 0

ULONG  _ThreadMigrateToProc (LW_HANDLE  ulId, PVOID   pvVProc, BOOL  bMain)
{
             INTREG         iregInterLevel;
    REGISTER UINT16         usIndex;
    REGISTER PLW_CLASS_TCB  ptcb;
             BOOL           bVpAdd;
             LW_LD_VPROC   *pvproc = (LW_LD_VPROC *)pvVProc;
    
    usIndex = _ObjectGetIndex(ulId);
    
    if (!_ObjectClassOK(ulId, _OBJECT_THREAD)) {                        /*  ��� ID ������Ч��          */
        return  (ERROR_KERNEL_HANDLE_NULL);
    }
    
    if (_Thread_Index_Invalid(usIndex)) {                               /*  ����߳���Ч��              */
        return  (ERROR_THREAD_NULL);
    }
    
    if (_Thread_Invalid(usIndex)) {
        return  (ERROR_THREAD_NULL);
    }
    
    iregInterLevel = __KERNEL_ENTER_IRQ();                              /*  �����ں˹ر��ж�            */
    ptcb = _K_ptcbTCBIdTable[usIndex];
    if (ptcb->TCB_ulOption & LW_OPTION_OBJECT_GLOBAL) {
        ptcb->TCB_ulOption &= (~LW_OPTION_OBJECT_GLOBAL);
        bVpAdd = LW_TRUE;
    
    } else {
        bVpAdd = LW_FALSE;
    }
    
    ptcb->TCB_ulOption          |= LW_OPTION_THREAD_POSIX;              /*  �������߳�Ĭ��Ϊ POSIX �߳� */
    ptcb->TCB_pvVProcessContext  = pvVProc;

    if (bMain) {
        pvproc->VP_ulMainThread = ulId;                                 /*  ����Ϊ���߳�                */
    }
    __KERNEL_EXIT_IRQ(iregInterLevel);                                  /*  �����ں˴��ж�            */
    
    if (bVpAdd) {
        vprocThreadAdd(ptcb->TCB_pvVProcessContext, ptcb);
    }
    
    __resHandleMakeLocal(ulId);
    
    return  (ERROR_NONE);
}

#endif                                                                  /*  LW_CFG_MODULELOADER_EN > 0  */
/*********************************************************************************************************
** ��������: _ThreadTraversal
** ��������: �������е��߳� (�����������ں˵�ǰ���µ���)
** �䡡��  : pfunc             �ص�����
**           pvArg[0 ~ 5]      ������������
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
VOID  _ThreadTraversal (VOIDFUNCPTR    pfunc, 
                        PVOID          pvArg0,
                        PVOID          pvArg1,
                        PVOID          pvArg2,
                        PVOID          pvArg3,
                        PVOID          pvArg4,
                        PVOID          pvArg5)
{
    REGISTER PLW_CLASS_TCB         ptcb;
             PLW_LIST_LINE         plineList;
             
    for (plineList  = _K_plineTCBHeader;
         plineList != LW_NULL;
         plineList  = _list_line_get_next(plineList)) {
         
        ptcb = _LIST_ENTRY(plineList, LW_CLASS_TCB, TCB_lineManage);
        pfunc(ptcb, pvArg0, pvArg1, pvArg2, pvArg3, pvArg4, pvArg5);
    }
}
/*********************************************************************************************************
** ��������: _ThreadRestartProcHook
** ��������: �߳� Restart ʱ�� hook �Ĵ���
** �䡡��  : ptcb      ������ƿ�
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
VOID  _ThreadRestartProcHook (PLW_CLASS_TCB  ptcb)
{
    LW_OBJECT_HANDLE  ulId = ptcb->TCB_ulId;

    _TCBCleanupPopExt(ptcb);                                            /*  cleanup ����                */
    bspTaskDeleteHook(ulId, LW_NULL, ptcb);                             /*  �û����Ӻ���                */
    __LW_THREAD_DELETE_HOOK(ulId, LW_NULL, ptcb);
    bspTCBInitHook(ulId, ptcb);                                         /*  ���ù��Ӻ���                */
    __LW_THREAD_INIT_HOOK(ulId, ptcb);
    bspTaskCreateHook(ulId);                                            /*  ���ù��Ӻ���                */
    __LW_THREAD_CREATE_HOOK(ulId, ptcb->TCB_ulOption);
}
/*********************************************************************************************************
** ��������: _ThreadDeleteProcHook
** ��������: �߳� Delete ʱ�� hook �Ĵ���
** �䡡��  : ptcb          ������ƿ�
**           pvRetVal      ���񷵻�ֵ
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
VOID  _ThreadDeleteProcHook (PLW_CLASS_TCB  ptcb, PVOID  pvRetVal)
{
    LW_OBJECT_HANDLE  ulId = ptcb->TCB_ulId;

    _TCBCleanupPopExt(ptcb);
    bspTaskDeleteHook(ulId, pvRetVal, ptcb);                            /*  �û����Ӻ���                */
    __LW_THREAD_DELETE_HOOK(ulId, pvRetVal, ptcb);
    _TCBDestroyExt(ptcb);                                               /*  ���� TCB ��չ��             */
}
/*********************************************************************************************************
  END
*********************************************************************************************************/
