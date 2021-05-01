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
** ��   ��   ��: ThreadDetach.c
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2007 �� 07 �� 18 ��
**
** ��        ��: ��ֹ�����̺߳ϲ�ָ���߳�

** BUG
2007.07.18  ������ _DebugHandle() ����
2008.05.18  ʹ�� __KERNEL_ENTER() ���� ThreadLock();
2013.04.01  ���� GCC 4.7.3 �������� warning.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
/*********************************************************************************************************
  loader
*********************************************************************************************************/
#if LW_CFG_MODULELOADER_EN > 0
extern pid_t  vprocGetPidByTcbNoLock(PLW_CLASS_TCB  ptcb);
#endif                                                                  /*  LW_CFG_MODULELOADER_EN > 0  */
/*********************************************************************************************************
** ��������: API_ThreadDetachEx
** ��������: ��ֹ�����̺߳ϲ�ָ���߳�
** �䡡��  :
**           ulId             �߳̾��
**           pvRetVal         ����ֵ
** �䡡��  : ID
** ȫ�ֱ���:
** ����ģ��:
                                           API ����

                                       (�������ж��е���)
*********************************************************************************************************/
LW_API
ULONG  API_ThreadDetachEx (LW_OBJECT_HANDLE  ulId, PVOID  pvRetVal)
{
    REGISTER UINT16                usIndex;
#if LW_CFG_MODULELOADER_EN > 0
    REGISTER PLW_CLASS_TCB         ptcbCur;
#endif                                                                  /*  LW_CFG_MODULELOADER_EN > 0  */
    REGISTER PLW_CLASS_TCB         ptcb;
    REGISTER PLW_CLASS_WAITJOIN    ptwj;

    usIndex = _ObjectGetIndex(ulId);

    if (LW_CPU_GET_CUR_NESTING()) {                                     /*  �������ж��е���            */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "called from ISR.\r\n");
        _ErrorHandle(ERROR_KERNEL_IN_ISR);
        return  (ERROR_KERNEL_IN_ISR);
    }

#if LW_CFG_MODULELOADER_EN > 0
    LW_TCB_GET_CUR_SAFE(ptcbCur);
#endif                                                                  /*  LW_CFG_MODULELOADER_EN > 0  */

#if LW_CFG_ARG_CHK_EN > 0
    if (!_ObjectClassOK(ulId, _OBJECT_THREAD)) {                        /*  ��� ID ������Ч��          */
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

    ptcb = _K_ptcbTCBIdTable[usIndex];
    if (ptcb) {
        if (ptcb->TCB_bDetachFlag) {
            __KERNEL_EXIT();                                            /*  �˳��ں�                    */
            _ErrorHandle(ERROR_THREAD_DETACHED);
            return  (ERROR_THREAD_DETACHED);
        }

#if LW_CFG_MODULELOADER_EN > 0
        if (vprocGetPidByTcbNoLock(ptcb) !=
            vprocGetPidByTcbNoLock(ptcbCur)) {                          /*  ֻ�� join ͬ�����߳�        */
            __KERNEL_EXIT();                                            /*  �˳��ں�                    */
            _ErrorHandle(ERROR_THREAD_NULL);
            return  (ERROR_THREAD_NULL);
        }
#endif                                                                  /*  LW_CFG_MODULELOADER_EN > 0  */

        _ThreadDetach(ptcb, LW_NULL, pvRetVal);                         /*  DETACH ����                 */

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
            _ThreadDetach(LW_NULL, ptwj, pvRetVal);                     /*  �ڵȴ����ն�����            */
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
** ��������: API_ThreadDetach
** ��������: ��ֹ�����̺߳ϲ�ָ���߳�
** �䡡��  :
**           ulId             �߳̾��
** �䡡��  : ID
** ȫ�ֱ���:
** ����ģ��:
                                           API ����

                                       (�������ж��е���)
*********************************************************************************************************/
LW_API
ULONG  API_ThreadDetach (LW_OBJECT_HANDLE  ulId)
{
    return  (API_ThreadDetachEx(ulId, LW_NULL));
}
/*********************************************************************************************************
  END
*********************************************************************************************************/
