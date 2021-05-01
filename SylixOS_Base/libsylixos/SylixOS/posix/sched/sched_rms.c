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
** ��   ��   ��: sched_rms.c
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2015 �� 04 �� 17 ��
**
** ��        ��: �߾��� RMS ������ (ʹ�� LW_CFG_TIME_HIGH_RESOLUTION_EN ��Ϊʱ�侫�ȱ�֤).
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "unistd.h"
#include "../include/px_pthread.h"
#include "../include/px_sched_rms.h"                                    /*  �Ѱ�������ϵͳͷ�ļ�        */
/*********************************************************************************************************
  �ü�֧��
*********************************************************************************************************/
#if LW_CFG_POSIX_EN > 0 && LW_CFG_POSIXEX_EN > 0
/*********************************************************************************************************
** ��������: sched_rms_init
** ��������: ��ʼ�� RMS ������
** �䡡��  : prms      RMS ������
**           thread    ��Ҫ���� RMS ���߳�.
** �䡡��  : �Ƿ��ʼ���ɹ�
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API 
int  sched_rms_init (sched_rms_t  *prms, pthread_t  thread)
{
    if (!prms) {
        errno = EINVAL;
        return  (PX_ERROR);
    }
    
    if (API_ThreadSetSchedParam(thread, LW_OPTION_SCHED_FIFO, 
                                LW_OPTION_RESPOND_IMMIEDIA)) {
        errno = ESRCH;
        return  (PX_ERROR);
    }
    
    lib_bzero(prms, sizeof(sched_rms_t));
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: sched_rms_destroy
** ��������: ɾ�� RMS ������
** �䡡��  : prms      RMS ������
** �䡡��  : �Ƿ�ɾ���ɹ�
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API 
int  sched_rms_destroy (sched_rms_t  *prms)
{
    if (!prms) {
        errno = EINVAL;
        return  (PX_ERROR);
    }
    
    prms->PRMS_iStatus = PRMS_STATUS_INACTIVE;
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: sched_rms_period
** ��������: ɾ�� RMS ������
** �䡡��  : prms      RMS ������
**           period    RMS ����
** �䡡��  : 0 ��ʾ��ȷ
**           error == EINTR    ��ʾ���źż���.
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API 
int  sched_rms_period (sched_rms_t  *prms, const struct timespec *period)
{
    struct timespec temp;
    struct timespec etime;
    
    if (!prms || !period) {
        errno = EINVAL;
        return  (PX_ERROR);
    }
    
    switch (prms->PRMS_iStatus) {
    
    case PRMS_STATUS_INACTIVE:
        lib_clock_gettime(CLOCK_MONOTONIC, &prms->PRMS_tsSave);         /*  ��õ�ǰʱ��                */
        prms->PRMS_iStatus = PRMS_STATUS_ACTIVE;
        return  (ERROR_NONE);
        
    case PRMS_STATUS_ACTIVE:
        lib_clock_gettime(CLOCK_MONOTONIC, &temp);
        __timespecSub2(&etime, &temp, &prms->PRMS_tsSave);
        if (__timespecLeftTime(period, &etime)) {                       /*  ִ��ʱ�䳬������            */
            lib_clock_gettime(CLOCK_MONOTONIC, &prms->PRMS_tsSave);     /*  ��õ�ǰʱ��                */
            errno = EOVERFLOW;
            return  (PX_ERROR);
        }
        
        __timespecSub2(&temp, period, &etime);
        
        /*
         *  ע��: ����ֱ�Ӽ���������Ϊ����ÿ�β��㶼����һ���̶������ɽ���
         *        ������ھ���. (��ʹ�� lib_clock_gettime())
         */
        __timespecAdd(&prms->PRMS_tsSave, period);                      /*  ��ȷ����������              */
        return  (nanosleep(&temp, LW_NULL));
        
    default:
        errno = ENOTSUP;
        return  (PX_ERROR);
    }
}

#endif                                                                  /*  LW_CFG_POSIX_EN > 0         */
                                                                        /*  LW_CFG_POSIXEX_EN > 0       */
/*********************************************************************************************************
  END
*********************************************************************************************************/
