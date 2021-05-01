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
** ��   ��   ��: ThreadIsReady.c
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2007 �� 07 �� 18 ��
**
** ��        ��: ����ϵͳ�̹߳�������

** BUG
2007.07.18  ���� _DebugHandle() ����
2008.01.16  ��������ʽ.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
/*********************************************************************************************************
** ��������: API_ThreadIsReady
** ��������: ����߳��Ƿ����
** �䡡��  : ulId               �߳�ID
** �䡡��  : �������� LW_TRUE   ���򷵻� LW_FALSE
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API
BOOL  API_ThreadIsReady (LW_OBJECT_HANDLE    ulId)
{
    REGISTER UINT16                usIndex;
    REGISTER PLW_CLASS_TCB         ptcb;
	REGISTER BOOL                  bIsReady;
	
    usIndex = _ObjectGetIndex(ulId);
    
#if LW_CFG_ARG_CHK_EN > 0
    if (!_ObjectClassOK(ulId, _OBJECT_THREAD)) {                        /*  ��� ID ������Ч��          */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "thread handle invalidate.\r\n");
        _ErrorHandle(ERROR_KERNEL_HANDLE_NULL);
        return  (LW_FALSE);
    }
    
    if (_Thread_Index_Invalid(usIndex)) {                               /*  ����߳���Ч��              */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "thread handle invalidate.\r\n");
        _ErrorHandle(ERROR_THREAD_NULL);
        return  (LW_FALSE);
    }
#endif
    
    __KERNEL_ENTER();                                                   /*  �����ں�                    */
    if (_Thread_Invalid(usIndex)) {
        __KERNEL_EXIT();                                                /*  �˳��ں�                    */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "thread handle invalidate.\r\n");
        _ErrorHandle(ERROR_THREAD_NULL);
        return  (LW_FALSE);
    }
    
    ptcb = _K_ptcbTCBIdTable[usIndex];
    
    if (__LW_THREAD_IS_READY(ptcb)) {
        bIsReady = LW_TRUE;
    
    } else {
        bIsReady = LW_FALSE;
    }
    
    __KERNEL_EXIT();                                                    /*  �˳��ں�                    */
    
    return  (bIsReady);
}
/*********************************************************************************************************
** ��������: API_ThreadIsRunning
** ��������: ����߳��Ƿ�������
** �䡡��  : ulId               �߳�ID
**           pbIsRunning        �Ƿ�����ִ��
** �䡡��  : ERROR CODE
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API
ULONG  API_ThreadIsRunning (LW_OBJECT_HANDLE   ulId, BOOL  *pbIsRunning)
{
    REGISTER UINT16         usIndex;
    REGISTER PLW_CLASS_TCB  ptcb;
	
    usIndex = _ObjectGetIndex(ulId);
    
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
    if (_Thread_Invalid(usIndex)) {
        __KERNEL_EXIT();                                                /*  �˳��ں�                    */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "thread handle invalidate.\r\n");
        _ErrorHandle(ERROR_THREAD_NULL);
        return  (ERROR_THREAD_NULL);
    }
    
    ptcb = _K_ptcbTCBIdTable[usIndex];
    
    if (pbIsRunning) {
        if (__LW_THREAD_IS_RUNNING(ptcb)) {
            *pbIsRunning = LW_TRUE;
        
        } else {
            *pbIsRunning = LW_FALSE;
        }
    }
    
    __KERNEL_EXIT();                                                    /*  �˳��ں�                    */
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
  END
*********************************************************************************************************/
