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
** ��   ��   ��: ThreadJoin.c
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2007 �� 07 �� 19 ��
**
** ��        ��: �̺߳ϲ�

** BUG
2007.07.19  ���� _DebugHandle() ����
2008.05.18  ʹ�� __KERNEL_ENTER() ���� ThreadLock();
2010.10.06  ����� cancel type �Ĳ���, ���� POSIX ��׼.
2012.08.25  API_ThreadJoin() �������ʱ����ӡ����. �������Ա���һЩ���ӡ.
2013.04.01  ���� GCC 4.7.3 �������� warning.
2013.07.18  ʹ���µĻ�ȡ TCB �ķ���, ȷ�� SMP ϵͳ��ȫ.
2013.09.17  ʹ�� POSIX �涨��ȡ���㶯��.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
/*********************************************************************************************************
  s_internal.h ��Ҳ����ض���
*********************************************************************************************************/
#if LW_CFG_THREAD_CANCEL_EN > 0
#define __THREAD_CANCEL_POINT()         API_ThreadTestCancel()
#else
#define __THREAD_CANCEL_POINT()
#endif                                                                  /*  LW_CFG_THREAD_CANCEL_EN     */
/*********************************************************************************************************
  loader
*********************************************************************************************************/
#if LW_CFG_MODULELOADER_EN > 0
extern pid_t  vprocGetPidByTcbNoLock(PLW_CLASS_TCB  ptcb);
#endif                                                                  /*  LW_CFG_MODULELOADER_EN > 0  */
/*********************************************************************************************************
** ��������: API_ThreadJoin
** ��������: �̺߳ϲ�
** �䡡��  : 
**           ulId             Ҫ�ϲ���Ŀ���߳̾��
**           ppvRetValAddr    ����̷߳���ֵ�ĵ�ַ
** �䡡��  : ID
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
                                           
                                       (�������ж��е���)
*********************************************************************************************************/
LW_API
ULONG  API_ThreadJoin (LW_OBJECT_HANDLE  ulId, PVOID  *ppvRetValAddr)
{
    REGISTER UINT16                usIndex;
    REGISTER PLW_CLASS_TCB         ptcbCur;
    REGISTER PLW_CLASS_TCB         ptcb;
    REGISTER PLW_CLASS_WAITJOIN    ptwj;
	
    usIndex = _ObjectGetIndex(ulId);
    
    if (LW_CPU_GET_CUR_NESTING()) {                                     /*  �������ж��е���            */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "called from ISR.\r\n");
        _ErrorHandle(ERROR_KERNEL_IN_ISR);
        return  (ERROR_KERNEL_IN_ISR);
    }
    
    LW_TCB_GET_CUR_SAFE(ptcbCur);
    
#if LW_CFG_ARG_CHK_EN > 0
    if (!_ObjectClassOK(ulId, _OBJECT_THREAD)) {                        /*  ��� ID ������Ч��          */
        _ErrorHandle(ERROR_KERNEL_HANDLE_NULL);
        return  (ERROR_KERNEL_HANDLE_NULL);
    }
    
    if (_Thread_Index_Invalid(usIndex)) {                               /*  ����߳���Ч��              */
        _ErrorHandle(ERROR_THREAD_NULL);
        return  (ERROR_THREAD_NULL);
    }
#endif

    __THREAD_CANCEL_POINT();                                            /*  ����ȡ����                  */
    
    __KERNEL_ENTER();                                                   /*  �����ں�                    */

    ptcb = _K_ptcbTCBIdTable[usIndex];
    if (ptcb) {
        if (ptcb == ptcbCur) {                                          /*  ���������Լ�                */
            __KERNEL_EXIT();                                            /*  �˳��ں�                    */
            _DebugHandle(__ERRORMESSAGE_LEVEL, "thread join self.\r\n");
            _ErrorHandle(ERROR_THREAD_JOIN_SELF);
            return  (ERROR_THREAD_JOIN_SELF);
        }

#if LW_CFG_MODULELOADER_EN > 0
        if (vprocGetPidByTcbNoLock(ptcb) !=
            vprocGetPidByTcbNoLock(ptcbCur)) {                          /*  ֻ�� join ͬ�����߳�        */
            __KERNEL_EXIT();                                            /*  �˳��ں�                    */
            _ErrorHandle(ERROR_THREAD_NULL);
            return  (ERROR_THREAD_NULL);
        }
#endif                                                                  /*  LW_CFG_MODULELOADER_EN > 0  */

        if (ptcb->TCB_bDetachFlag) {
            __KERNEL_EXIT();                                            /*  �˳��ں�                    */
            _ErrorHandle(ERROR_THREAD_DETACHED);
            return  (ERROR_THREAD_DETACHED);
        }

        _ThreadJoin(ptcb, LW_NULL, ppvRetValAddr);                      /*  �ϲ�                        */

    } else if (!LW_KERN_AUTO_REC_TCB_GET()) {                           /*  ��Ҫ�ֶ�����                */
        ptwj = &_K_twjTable[usIndex];
        if (ptwj->TWJ_ptcb) {
#if LW_CFG_MODULELOADER_EN > 0
            if (vprocGetPidByTcbNoLock(ptwj->TWJ_ptcb) !=
                vprocGetPidByTcbNoLock(ptcbCur)) {                      /*  ֻ�� join ͬ�����߳�        */
                __KERNEL_EXIT();                                        /*  �˳��ں�                    */
                _ErrorHandle(ERROR_THREAD_NULL);
                return  (ERROR_THREAD_NULL);
            }
#endif                                                                  /*  LW_CFG_MODULELOADER_EN > 0  */
            _ThreadJoin(LW_NULL, ptwj, ppvRetValAddr);                  /*  �ڵȴ����ն�����            */

        } else {
            __KERNEL_EXIT();                                            /*  �˳��ں�                    */
            _ErrorHandle(ERROR_THREAD_NULL);
            return  (ERROR_THREAD_NULL);
        }

    } else {
        __KERNEL_EXIT();                                                /*  �˳��ں�                    */
        _ErrorHandle(ERROR_THREAD_NULL);
        return  (ERROR_THREAD_NULL);
    }
    
    __KERNEL_EXIT();                                                    /*  �˳��ں�                    */
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
  END
*********************************************************************************************************/
