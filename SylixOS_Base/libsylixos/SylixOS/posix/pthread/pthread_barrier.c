/*********************************************************************************************************
**
**                                    �й������Դ��֯
**
**                                   Ƕ��ʽʵʱ����ϵͳ
**
**                                       SylixOS(TM)
**
**                               Copyright  All Rights Reserved
**
**--------------�ļ���Ϣ--------------------------------------------------------------------------------
**
** ��   ��   ��: pthread_barrier.c
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2010 �� 10 �� 04 ��
**
** ��        ��: pthread �߳����ϼ��ݿ�. (barrier �������ᷢ�� EINTR)

** BUG:
2011.03.06  ���� gcc 4.5.1 ��� warning.
2013.05.01  Upon successful completion, these functions shall return zero; 
            otherwise, an error number shall be returned to indicate the error.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../include/px_pthread.h"                                      /*  �Ѱ�������ϵͳͷ�ļ�        */
/*********************************************************************************************************
  �ü�֧��
*********************************************************************************************************/
#if LW_CFG_POSIX_EN > 0
/*********************************************************************************************************
** ��������: pthread_barrierattr_init
** ��������: ��ʼ���߳��������Կ�.
** �䡡��  : pbarrierattr     ����
** �䡡��  : ERROR CODE
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API 
int  pthread_barrierattr_init (pthread_barrierattr_t  *pbarrierattr)
{
    if (pbarrierattr) {
        *pbarrierattr = 0;
    }
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: pthread_barrierattr_destroy
** ��������: ����һ���߳��������Կ�.
** �䡡��  : pbarrierattr     ����
** �䡡��  : ERROR CODE
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API 
int  pthread_barrierattr_destroy (pthread_barrierattr_t  *pbarrierattr)
{
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: pthread_barrierattr_setpshared
** ��������: �����߳��������Կ鹲��״̬.
** �䡡��  : pbarrierattr     ����
**           shared           PTHREAD_PROCESS_SHARED    PTHREAD_PROCESS_PRIVATE
** �䡡��  : ERROR CODE
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API 
int  pthread_barrierattr_setpshared (pthread_barrierattr_t  *pbarrierattr, int  shared)
{
    if (pbarrierattr) {
        *pbarrierattr = shared;
    }
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: pthread_barrierattr_getpshared
** ��������: ��ȡ�߳��������Կ鹲��״̬.
** �䡡��  : pbarrierattr     ����
**           pshared          ���� shared ״̬
** �䡡��  : ERROR CODE
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API 
int  pthread_barrierattr_getpshared (const pthread_barrierattr_t  *pbarrierattr, int  *pshared)
{
    if (pbarrierattr && pshared) {
        *pshared = *pbarrierattr;
    }
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: pthread_barrier_init
** ��������: ����(��ʼ��)�߳�����.
** �䡡��  : pbarrier         �����󱣴����Ͼ��
**           pbarrierattr     ����
**           count            �߳����ϵļ�������
** �䡡��  : ERROR CODE
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API 
int  pthread_barrier_init (pthread_barrier_t            *pbarrier, 
                           const pthread_barrierattr_t  *pbarrierattr,
                           unsigned int                  count)
{
    if (pbarrier == LW_NULL) {
        errno = EINVAL;
        return  (EINVAL);
    }
    if (count == 0) {
        errno = EINVAL;
        return  (EINVAL);
    }
    
    /*
     *  ע��, ����ֻ�ܰ� LW_OPTION_WAIT_FIFO �ȴ�, �������ܱ�֤������߳�������ȴ����Ǽ���!
     */
    pbarrier->PBARRIER_ulSync = API_SemaphoreBCreate("barrier_sync", 
                                                     LW_FALSE, 
                                                     LW_OPTION_WAIT_FIFO,
                                                     LW_NULL);          /*  ����ͬ���ź���              */
    if (pbarrier->PBARRIER_ulSync == LW_OBJECT_HANDLE_INVALID) {
        errno = ENOSPC;
        return  (ENOSPC);
    }
    
    pbarrier->PBARRIER_iCounter   = count;
    pbarrier->PBARRIER_iInitValue = count;
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: pthread_barrier_destroy
** ��������: ɾ��(����)�߳�����.
** �䡡��  : pbarrier         �߳����Ͼ��
** �䡡��  : ERROR CODE
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API 
int  pthread_barrier_destroy (pthread_barrier_t  *pbarrier)
{
    if (pbarrier == LW_NULL) {
        errno = EINVAL;
        return  (EINVAL);
    }
    
    if (pbarrier->PBARRIER_ulSync) {
        API_SemaphoreBDelete(&pbarrier->PBARRIER_ulSync);               /*  ɾ��ͬ���ź���              */
    }
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: pthread_barrier_wait
** ��������: �ȴ��߳�����.
** �䡡��  : pbarrier         �߳����Ͼ��
** �䡡��  : ERROR CODE
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API 
int  pthread_barrier_wait (pthread_barrier_t  *pbarrier)
{
             INTREG  iregInterLevel;
             BOOL    bRelease = LW_FALSE;
    REGISTER ULONG   ulReleaseNum = 0;

    if (pbarrier == LW_NULL) {
        errno = EINVAL;
        return  (EINVAL);
    }
    
    __LW_ATOMIC_LOCK(iregInterLevel);                                   /*  ���� atomic �ٽ���          */
    pbarrier->PBARRIER_iCounter -= 1;
    if (pbarrier->PBARRIER_iCounter == 0) {                             /*  ��Ҫ����ȴ��߳�            */
        pbarrier->PBARRIER_iCounter =  pbarrier->PBARRIER_iInitValue;
        ulReleaseNum = (ULONG)(pbarrier->PBARRIER_iInitValue - 1);      /*  ��Ҫ�����̵߳�����          */
        bRelease     = LW_TRUE;
    }
    __LW_ATOMIC_UNLOCK(iregInterLevel);                                 /*  �˳� atomic �ٽ���          */
    
    if (bRelease) {
        if (ulReleaseNum) {
            API_SemaphoreBRelease(pbarrier->PBARRIER_ulSync, 
                                  ulReleaseNum,
                                  LW_NULL);                             /*  ������ǰ�ȴ����߳�          */
        }
        return  (PTHREAD_BARRIER_SERIAL_THREAD);
    
    } else {
        __THREAD_CANCEL_POINT();                                        /*  ����ȡ����                  */
    
        API_SemaphoreBPend(pbarrier->PBARRIER_ulSync, 
                           LW_OPTION_WAIT_INFINITE);                    /*  �ȴ����һ�� wait �ͷ�      */
        
        return  (ERROR_NONE);
    }
}

#endif                                                                  /*  LW_CFG_POSIX_EN > 0         */
/*********************************************************************************************************
  END
*********************************************************************************************************/
