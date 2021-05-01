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
** ��   ��   ��: adjtime.c
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2014 �� 07 �� 04 ��
**
** ��        ��: posix adjtime ���ݿ�.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "unistd.h"
/*********************************************************************************************************
  �ü�֧��
*********************************************************************************************************/
#if LW_CFG_POSIX_EN > 0
#include "time.h"
/*********************************************************************************************************
  ������Χ
*********************************************************************************************************/
#define ADJTIME_DELTA_MAX       ((__ARCH_INT_MAX / 1000000) - 2)
#define ADJTIME_DELTA_MIN       ((__ARCH_INT_MIN / 1000000) + 2)
/*********************************************************************************************************
** ��������: adjtime
** ��������: ΢��ϵͳʱ��
** �䡡��  : delta             ϵͳʱ���������� (��ϸ˵���ο� POSIX �ֲ�)
**           olddelta          ����ϴ�����û������򷵻��ϴ�����ʣ���ʱ��
** �䡡��  : ERROR or OK
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API 
int  adjtime (const struct timeval *delta, struct timeval *olddelta)
{
    INT32   iDelta, iDeltaNs;
    INT32   iOldDelta, iOldDeltaNs;
    
    if (geteuid()) {                                                    /*  ֻ�� root Ȩ�޿��Բ���      */
        errno = EPERM;
        return  (PX_ERROR);
    }
    
    if (delta) {
        if ((delta->tv_sec < ADJTIME_DELTA_MIN) ||
            (delta->tv_sec > ADJTIME_DELTA_MAX)) {                      /*  ����ʱ���Ƿ񳬹���Χ        */
            errno = ENOTSUP;
            return  (PX_ERROR);
        }
        iDelta    = (INT32)delta->tv_sec * (INT32)LW_TICK_HZ;
        iDelta   += (INT32)((((delta->tv_usec * (INT32)LW_TICK_HZ) / 100) / 100) / 100);
        iDeltaNs  = (INT32)(delta->tv_usec % ((100 * 100 * 100) / (INT32)LW_TICK_HZ));
        iDeltaNs *= 1000;
        API_TimeTodAdjEx(&iDelta, &iDeltaNs, &iOldDelta, &iOldDeltaNs);
    
    } else {
        API_TimeTodAdjEx(LW_NULL, LW_NULL, &iOldDelta, &iOldDeltaNs);
    }
    
    if (olddelta) {
        olddelta->tv_sec   = (time_t)(iOldDelta / (INT32)LW_TICK_HZ);
        olddelta->tv_usec  = (LONG)(iOldDelta % (INT32)LW_TICK_HZ) * ((100 * 100 * 100)
                           / (INT32)LW_TICK_HZ);
        olddelta->tv_usec += iOldDeltaNs / 1000;
    }
    
    return  (ERROR_NONE);
}

#endif                                                                  /*  LW_CFG_POSIX_EN > 0         */
/*********************************************************************************************************
  END
*********************************************************************************************************/
