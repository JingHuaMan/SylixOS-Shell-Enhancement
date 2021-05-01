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
** ��   ��   ��: ThreadCond.c
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2009 �� 02 �� 05 ��
**
** ��        ��: ����ϵͳ�� pthread_cond_??? ��������֧��.

** BUG:
2009.07.11  ������̬��ʼ����һ������.
2009.12.30  API_ThreadCondWait() �ڳ�ʱʱ, ����Ҫ��ȡ�����ź���.
2010.01.05  API_ThreadCondWait() ���ͷŻ������͵ȴ��ź���ʱ, ʹ��ԭ�ӵĲ���.
2012.10.17  API_ThreadCondSignal() ��û������ȴ�ʱ, ֱ���˳�.
2013.04.02  API_ThreadCondDestroy() �����ж�æ��־.
2013.04.08  API_ThreadCondDestroy() ����û�г�ʼ������������ֱ���˳�.
            API_ThreadCondInit() ������û�д�����, �����г�ʼ��.
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
** ��������: __threadCondInit
** ��������: ��ʼ����������(��̬).
** �䡡��  : ptcd               ��������
**           pulAttr            ����
** �䡡��  : ERROR CODE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
#if LW_CFG_THREAD_EXT_EN > 0

static ULONG  __threadCondInit (PLW_THREAD_COND  ptcd, ULONG  ulAttr)
{
    ptcd->TCD_ulSignal = API_SemaphoreBCreate("cond_signal", LW_FALSE, 
                                              LW_OPTION_WAIT_PRIORITY, 
                                              LW_NULL);
    if (!ptcd->TCD_ulSignal) {
        return  (EAGAIN);
    }
    
    ptcd->TCD_ulMutex   = 0ul;
    ptcd->TCD_ulCounter = 0ul;
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: API_ThreadCondAttrInit
** ��������: ��ʼ��һ�������������Կ��ƿ�.
** �䡡��  : pulAttr            ����
** �䡡��  : ERROR CODE
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
ULONG  API_ThreadCondAttrInit (ULONG  *pulAttr)
{
    if (pulAttr) {
        *pulAttr = 0ul;
        return  (ERROR_NONE);
    
    } else {
        _ErrorHandle(EINVAL);
        return  (EINVAL);
    }
}
/*********************************************************************************************************
** ��������: API_ThreadCondAttrDestroy
** ��������: ����һ�������������Կ��ƿ�.
** �䡡��  : pulAttr            ����
** �䡡��  : ERROR CODE
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
ULONG  API_ThreadCondAttrDestroy (ULONG  *pulAttr)
{
    if (pulAttr) {
        *pulAttr = 0ul;
        return  (ERROR_NONE);
    
    } else {
        _ErrorHandle(EINVAL);
        return  (EINVAL);
    }
}
/*********************************************************************************************************
** ��������: API_ThreadCondAttrSetPshared
** ��������: ����һ�������������Կ��ƿ�Ĺ�������.
** �䡡��  : pulAttr            ����
**           iShared            ��������
** �䡡��  : ERROR CODE
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
ULONG  API_ThreadCondAttrSetPshared (ULONG  *pulAttr, INT  iShared)
{
    if (pulAttr) {
        if (iShared) {
            (*pulAttr) |= LW_THREAD_PROCESS_SHARED;
        } else {
            (*pulAttr) &= ~LW_THREAD_PROCESS_SHARED;
        }
        return  (ERROR_NONE);
    
    } else {
        _ErrorHandle(EINVAL);
        return  (EINVAL);
    }
}
/*********************************************************************************************************
** ��������: API_ThreadCondAttrGetPshared
** ��������: ���һ�������������Կ��ƿ�Ĺ�������.
** �䡡��  : pulAttr            ����
**           piShared           ��������
** �䡡��  : ERROR CODE
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
ULONG  API_ThreadCondAttrGetPshared (const ULONG  *pulAttr, INT  *piShared)
{
    if (pulAttr && piShared) {
        *piShared = ((*pulAttr) & LW_THREAD_PROCESS_SHARED) ? 1 : 0;
        return  (ERROR_NONE);
    
    } else {
        _ErrorHandle(EINVAL);
        return  (EINVAL);
    }
}
/*********************************************************************************************************
** ��������: API_ThreadCondInit
** ��������: ��ʼ��һ�������������ƿ�.
** �䡡��  : ptcd              �����������ƿ�
**           ulAttr            �������� (��ǰ����, ��Ҫ�������̹�����)
** �䡡��  : ERROR CODE
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
ULONG  API_ThreadCondInit (PLW_THREAD_COND  ptcd, ULONG  ulAttr)
{
    ULONG      ulError = ERROR_NONE;
    
    if (!ptcd) {
        _ErrorHandle(EINVAL);
        return  (EINVAL);
    }
    
    ulError = __threadCondInit(ptcd, ulAttr);                           /*  ��ʼ��                      */
    
    _ErrorHandle(ulError);
    return  (ulError);
}
/*********************************************************************************************************
** ��������: API_ThreadCondDestroy
** ��������: ����һ�������������ƿ�.
** �䡡��  : ptcd              �����������ƿ�
** �䡡��  : ERROR CODE
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
ULONG  API_ThreadCondDestroy (PLW_THREAD_COND  ptcd)
{
    REGISTER ULONG      ulError = ERROR_NONE;

    if (!ptcd) {
        _ErrorHandle(EINVAL);
        return  (EINVAL);
    }
    
    if (!_ObjectClassOK(ptcd->TCD_ulSignal, _OBJECT_SEM_B) ||
        _Event_Index_Invalid(_ObjectGetIndex(ptcd->TCD_ulSignal))) {
        return  (ERROR_NONE);                                           /*  ֱ���˳�                    */
    }
    
    if (ptcd->TCD_ulSignal) {
        ulError = API_SemaphoreBDelete(&ptcd->TCD_ulSignal);
    }
    
    return  (ulError);
}
/*********************************************************************************************************
** ��������: API_ThreadCondSignal
** ��������: ��ȴ������������̷߳����ź�.
** �䡡��  : ptcd              �����������ƿ�
** �䡡��  : ERROR CODE
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
ULONG  API_ThreadCondSignal (PLW_THREAD_COND  ptcd)
{
    REGISTER ULONG      ulError = ERROR_NONE;

    if (!ptcd) {
        _ErrorHandle(EINVAL);
        return  (EINVAL);
    }
    
    if (!_ObjectClassOK(ptcd->TCD_ulSignal, _OBJECT_SEM_B) ||
        _Event_Index_Invalid(_ObjectGetIndex(ptcd->TCD_ulSignal))) {
        ulError = __threadCondInit(ptcd, (ULONG)(~0));                  /*  ��ʼ��                      */
    }
    
    if (ulError) {
        _ErrorHandle(ulError);
        return  (ulError);
    }
    
    if (ptcd->TCD_ulSignal) {
        ULONG   ulThreadBlockNum = 0;
        
        ulError = API_SemaphoreBStatus(ptcd->TCD_ulSignal, LW_NULL, LW_NULL, &ulThreadBlockNum);
        if (ulError) {
            return  (ulError);
        }
            
        if (ulThreadBlockNum == 0) {                                    /*  û������ȴ�, ��ֱ���˳�    */
            return  (ulError);
        }
    
        ulError = API_SemaphoreBPost(ptcd->TCD_ulSignal);
    }
    
    return  (ulError);
}
/*********************************************************************************************************
** ��������: API_ThreadCondBroadcast
** ��������: �����еȴ������������̷߳����ź�.
** �䡡��  : ptcd              �����������ƿ�
** �䡡��  : ERROR CODE
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
ULONG  API_ThreadCondBroadcast (PLW_THREAD_COND  ptcd)
{
    REGISTER ULONG      ulError = ERROR_NONE;

    if (!ptcd) {
        _ErrorHandle(EINVAL);
        return  (EINVAL);
    }
    
    if (!_ObjectClassOK(ptcd->TCD_ulSignal, _OBJECT_SEM_B) ||
        _Event_Index_Invalid(_ObjectGetIndex(ptcd->TCD_ulSignal))) {
        ulError = __threadCondInit(ptcd, (ULONG)(~0));                  /*  ��ʼ��                      */
    }
    
    if (ulError) {
        _ErrorHandle(ulError);
        return  (ulError);
    }
    
    if (ptcd->TCD_ulSignal) {
        ulError = API_SemaphoreBFlush(ptcd->TCD_ulSignal, LW_NULL);
    }
    
    return  (ulError);
}
/*********************************************************************************************************
** ��������: API_ThreadCondWait
** ��������: �ȴ���������.
** �䡡��  : ptcd              �����������ƿ�
**           ulMutex           �����ź���
**           ulTimeout         ��ʱʱ��
** �䡡��  : ERROR CODE
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
ULONG  API_ThreadCondWait (PLW_THREAD_COND  ptcd, LW_OBJECT_HANDLE  ulMutex, ULONG  ulTimeout)
{
             LW_OBJECT_HANDLE   ulOwerThread;
    REGISTER ULONG              ulError = ERROR_NONE;
    
    if (!ptcd) {
        _ErrorHandle(EINVAL);
        return  (EINVAL);
    }
    
    if (!_ObjectClassOK(ptcd->TCD_ulSignal, _OBJECT_SEM_B) ||
        _Event_Index_Invalid(_ObjectGetIndex(ptcd->TCD_ulSignal))) {
        ulError = __threadCondInit(ptcd, (ULONG)(~0));                  /*  ��ʼ��                      */
    }
    
    if (ulError) {
        _ErrorHandle(ulError);
        return  (ulError);
    }
    
    if (!ulMutex) {
        _ErrorHandle(EINVAL);
        return  (EINVAL);
    }
    
    ulError = API_SemaphoreMStatusEx(ulMutex, LW_NULL, LW_NULL, LW_NULL, &ulOwerThread);
    if (ulError) {
        return  (ulError);
    }
    
    if (((ptcd->TCD_ulMutex) && (ptcd->TCD_ulMutex != ulMutex)) ||
        (ulOwerThread != API_ThreadIdSelf())) {
        _ErrorHandle(EINVAL);
        return (EINVAL);
    
    } else {
        ptcd->TCD_ulMutex = ulMutex;
    }
    
    ptcd->TCD_ulCounter++;
    
    __THREAD_CANCEL_POINT();                                            /*  ����ȡ����                  */
    
    ulError = API_SemaphorePostBPend(ulMutex, 
                                     ptcd->TCD_ulSignal, 
                                     ulTimeout);                        /*  ԭ���ͷŲ��ȴ��ź���        */
    if (ulError) {                                                      /*  ����ȴ���ʱ                */
        API_SemaphoreMPend(ulMutex, LW_OPTION_WAIT_INFINITE);           /*  ���»�ȡ������              */
        if (ulError != ERROR_EVENT_WAS_DELETED) {
            ptcd->TCD_ulCounter--;
            if (ptcd->TCD_ulCounter == 0) {
                ptcd->TCD_ulMutex = 0ul;
            }
        }
        return  (ulError);
    }
    
    ulError = API_SemaphoreMPend(ulMutex, LW_OPTION_WAIT_INFINITE);
    if (ulError) {
        return  (ulError);
    }
    
    ptcd->TCD_ulCounter--;
    if (ptcd->TCD_ulCounter == 0) {
        ptcd->TCD_ulMutex = 0ul;
    }
    
    return  (ERROR_NONE);
}

#endif                                                                  /*  LW_CFG_THREAD_EXT_EN > 0    */
/*********************************************************************************************************
  END
*********************************************************************************************************/
