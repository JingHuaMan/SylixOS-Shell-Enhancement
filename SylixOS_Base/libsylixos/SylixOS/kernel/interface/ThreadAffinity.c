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
** ��   ��   ��: ThreadAffinity.c
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2014 �� 11 �� 11 ��
**
** ��        ��: �߳��׺Ͷ�ģ��.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
/*********************************************************************************************************
  �ü�֧��
*********************************************************************************************************/
#if LW_CFG_SMP_EN > 0
/*********************************************************************************************************
** ��������: API_ThreadSetAffinity
** ��������: ���߳�������ָ���� CPU ����.
** �䡡��  : ulId          �߳�
**           stSize        CPU ���뼯�ڴ��С
**           pcpuset       CPU ����
** �䡡��  : ERROR
** ȫ�ֱ���: 
** ����ģ��: 
** ע  ��  : ��ǰֻ�ܽ�����������һ�� CPU ��, ���ָ���� CPU û�м������������к˾��ɵ���.

                                           API ����
*********************************************************************************************************/
LW_API  
ULONG  API_ThreadSetAffinity (LW_OBJECT_HANDLE  ulId, size_t  stSize, const PLW_CLASS_CPUSET  pcpuset)
{
    REGISTER UINT16         usIndex;
    REGISTER PLW_CLASS_TCB  ptcb;
             PLW_CLASS_TCB  ptcbCur;
             ULONG          ulError;

    usIndex = _ObjectGetIndex(ulId);

#if LW_CFG_ARG_CHK_EN > 0
    if (!_ObjectClassOK(ulId, _OBJECT_THREAD)) {                        /*  ��� ID ������Ч��          */
        _ErrorHandle(ERROR_KERNEL_HANDLE_NULL);
        return  (ERROR_KERNEL_HANDLE_NULL);
    }
    if (_Thread_Index_Invalid(usIndex)) {                               /*  ����߳���Ч��              */
        _ErrorHandle(ERROR_KERNEL_HANDLE_NULL);
        return  (ERROR_KERNEL_HANDLE_NULL);
    }
    if (!stSize || !pcpuset) {
        _ErrorHandle(EINVAL);
        return  (EINVAL);
    }
#endif

    if (ulId == API_KernelGetExc()) {                                   /*  ���������� exce �߳�        */
        _ErrorHandle(EPERM);
        return  (EPERM);
    }
    
    __KERNEL_ENTER();                                                   /*  �����ں�                    */
    if (_Thread_Invalid(usIndex)) {
        __KERNEL_EXIT();                                                /*  �˳��ں�                    */
        _ErrorHandle(ERROR_KERNEL_HANDLE_NULL);
        return  (ERROR_KERNEL_HANDLE_NULL);
    }

    ptcb = _K_ptcbTCBIdTable[usIndex];
    if (ptcb->TCB_iDeleteProcStatus) {                                  /*  ��ɾ���������Ĺ�����        */
        __KERNEL_EXIT();                                                /*  �˳��ں�                    */
        _ErrorHandle(ERROR_THREAD_NULL);
        return  (ERROR_THREAD_NULL);
    }

    LW_TCB_GET_CUR(ptcbCur);
    if (ptcb == ptcbCur) {
        if (__THREAD_LOCK_GET(ptcb) > 1) {                              /*  ��������                  */
            __KERNEL_EXIT();                                            /*  �˳��ں�                    */
            _ErrorHandle(EBUSY);
            return  (EBUSY);
        }
        _ThreadSetAffinity(ptcb, stSize, pcpuset);                      /*  ����                        */

    } else {
        if (__THREAD_LOCK_GET(ptcb)) {                                  /*  ��������                  */
            __KERNEL_EXIT();                                            /*  �˳��ں�                    */
            _ErrorHandle(EBUSY);
            return  (EBUSY);
        }

        ulError = _ThreadStop(ptcb);
        __KERNEL_EXIT();                                                /*  �˳��ں�                    */
        if (ulError) {
            return  (ulError);
        }

#if LW_CFG_SMP_EN > 0
        if (ptcbCur->TCB_uiStatusChangeReq) {
            ptcbCur->TCB_uiStatusChangeReq = 0;
            _ErrorHandle(ERROR_THREAD_NULL);
            return  (ERROR_THREAD_NULL);
        }
#endif                                                                  /*  LW_CFG_SMP_EN               */

        __KERNEL_ENTER();                                               /*  �����ں�                    */
        _ThreadSetAffinity(ptcb, stSize, pcpuset);                      /*  ����                        */
        _ThreadContinue(ptcb, LW_FALSE);                                /*  ����Ŀ��                    */
    }

    __KERNEL_EXIT();                                                    /*  �˳��ں�                    */

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: API_ThreadGetAffinity
** ��������: ��ȡ�߳� CPU �׺Ͷ����
** �䡡��  : ulId          �߳�
**           stSize        CPU ���뼯�ڴ��С
**           pcpuset       CPU ����
** �䡡��  : ERROR
** ȫ�ֱ���: 
** ����ģ��: 
** ע  ��  : �����ȡ������ȫΪ 0 ������ CPU ���ɵ��ȴ�����.

                                           API ����
*********************************************************************************************************/
LW_API  
ULONG  API_ThreadGetAffinity (LW_OBJECT_HANDLE  ulId, size_t  stSize, PLW_CLASS_CPUSET  pcpuset)
{
    REGISTER UINT16         usIndex;
    REGISTER PLW_CLASS_TCB  ptcb;

    usIndex = _ObjectGetIndex(ulId);

#if LW_CFG_ARG_CHK_EN > 0
    if (!_ObjectClassOK(ulId, _OBJECT_THREAD)) {                        /*  ��� ID ������Ч��          */
        _ErrorHandle(ERROR_KERNEL_HANDLE_NULL);
        return  (ERROR_KERNEL_HANDLE_NULL);
    }
    if (_Thread_Index_Invalid(usIndex)) {                               /*  ����߳���Ч��              */
        _ErrorHandle(ERROR_KERNEL_HANDLE_NULL);
        return  (ERROR_KERNEL_HANDLE_NULL);
    }
    if ((stSize < sizeof(LW_CLASS_CPUSET)) || !pcpuset) {
        _ErrorHandle(EINVAL);
        return  (EINVAL);
    }
#endif

    __KERNEL_ENTER();                                                   /*  �����ں�                    */
    if (_Thread_Invalid(usIndex)) {
        __KERNEL_EXIT();                                                /*  �˳��ں�                    */
        _ErrorHandle(ERROR_KERNEL_HANDLE_NULL);
        return  (ERROR_KERNEL_HANDLE_NULL);
    }
    
    ptcb = _K_ptcbTCBIdTable[usIndex];
    _ThreadGetAffinity(ptcb, stSize, pcpuset);
    __KERNEL_EXIT();                                                    /*  �˳��ں�                    */
    
    return  (ERROR_NONE);
}

#endif                                                                  /*  LW_CFG_SMP_EN > 0           */
/*********************************************************************************************************
  END
*********************************************************************************************************/
