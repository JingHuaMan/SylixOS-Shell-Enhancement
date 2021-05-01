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
** ��   ��   ��: ThreadStop.c
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2013 �� 12 �� 02 ��
**
** ��        ��: ֹͣ/�ָ�һ���̡߳�
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
/*********************************************************************************************************
** ��������: API_ThreadStop
** ��������: ֹͣһ���߳�
** �䡡��  : ulId            �߳� ID
** �䡡��  : ERROR
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API
ULONG  API_ThreadStop (LW_OBJECT_HANDLE  ulId)
{
    REGISTER UINT16         usIndex;
    REGISTER PLW_CLASS_TCB  ptcb;
             ULONG          ulError;

    usIndex = _ObjectGetIndex(ulId);
    
#if LW_CFG_ARG_CHK_EN > 0
    if (!_ObjectClassOK(ulId, _OBJECT_THREAD)) {                        /*  ��� ID ������Ч��          */
        _ErrorHandle(ERROR_KERNEL_HANDLE_NULL);
        return  (ERROR_KERNEL_HANDLE_NULL);
    }
    if (usIndex >= LW_CFG_MAX_THREADS) {                                /*  �����ÿ����̶߳�ջ��Ϣ    */
        _ErrorHandle(ERROR_THREAD_NULL);
        return  (ERROR_THREAD_NULL);
    }
#endif
    
    __KERNEL_ENTER();                                                   /*  �����ں�                    */
    if (_Thread_Invalid(usIndex)) {
        __KERNEL_EXIT();                                                /*  �˳��ں�                    */
        _ErrorHandle(ERROR_THREAD_NULL);
        return  (ERROR_THREAD_NULL);
    }
    
    ptcb = _K_ptcbTCBIdTable[usIndex];
    
    if (ptcb->TCB_iDeleteProcStatus) {                                  /*  ��ɾ���������Ĺ�����        */
        __KERNEL_EXIT();                                                /*  �˳��ں�                    */
        _ErrorHandle(ERROR_THREAD_NULL);
        return  (ERROR_THREAD_NULL);
    }
    
    ulError = _ThreadStop(ptcb);
    
    __KERNEL_EXIT();                                                    /*  �˳��ں�                    */
    
#if LW_CFG_SMP_EN > 0
    {
        PLW_CLASS_TCB  ptcbCur;
        
        LW_TCB_GET_CUR_SAFE(ptcbCur);
        if (ptcbCur->TCB_uiStatusChangeReq) {                           /*  ״̬�ı��Ƿ�ɹ�            */
            ptcbCur->TCB_uiStatusChangeReq = 0;
            ulError = ERROR_THREAD_NULL;                                /*  Ŀ���߳��Ѿ���ɾ����������  */
        }
    }
#endif                                                                  /*  LW_CFG_SMP_EN               */
    
    _ErrorHandle(ulError);
    return  (ulError);
}
/*********************************************************************************************************
** ��������: API_ThreadContinue
** ��������: �ָ�һ����ֹͣ���߳�
** �䡡��  : ulId            �߳� ID
** �䡡��  : ERROR
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API
ULONG  API_ThreadContinue (LW_OBJECT_HANDLE  ulId)
{
    REGISTER UINT16         usIndex;
    REGISTER PLW_CLASS_TCB  ptcb;
             ULONG          ulError;

    usIndex = _ObjectGetIndex(ulId);
    
#if LW_CFG_ARG_CHK_EN > 0
    if (!_ObjectClassOK(ulId, _OBJECT_THREAD)) {                        /*  ��� ID ������Ч��          */
        _ErrorHandle(ERROR_KERNEL_HANDLE_NULL);
        return  (ERROR_KERNEL_HANDLE_NULL);
    }
    if (usIndex >= LW_CFG_MAX_THREADS) {                                /*  �����ÿ����̶߳�ջ��Ϣ    */
        _ErrorHandle(ERROR_THREAD_NULL);
        return  (ERROR_THREAD_NULL);
    }
#endif
    
    __KERNEL_ENTER();                                                   /*  �����ں�                    */
    if (_Thread_Invalid(usIndex)) {
        __KERNEL_EXIT();                                                /*  �˳��ں�                    */
        _ErrorHandle(ERROR_THREAD_NULL);
        return  (ERROR_THREAD_NULL);
    }
    
    ptcb = _K_ptcbTCBIdTable[usIndex];
    
    ulError = _ThreadContinue(ptcb, LW_FALSE);
    
    __KERNEL_EXIT();                                                    /*  �˳��ں�                    */
    
    _ErrorHandle(ulError);
    return  (ulError);
}
/*********************************************************************************************************
  END
*********************************************************************************************************/
