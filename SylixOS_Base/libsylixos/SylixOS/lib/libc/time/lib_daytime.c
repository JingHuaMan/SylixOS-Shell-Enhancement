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
** ��   ��   ��: lib_clock.c
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2008 �� 08 �� 30 ��
**
** ��        ��: ϵͳ��.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
#include "../SylixOS/system/include/s_system.h"
#include "lib_local.h"
/*********************************************************************************************************
** ��������: lib_gettimeofday
** ��������: 
** �䡡��  : 
** �䡡��  : 
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
#if LW_CFG_RTC_EN > 0

INT  lib_gettimeofday (struct timeval *tv, struct timezone *tz)
{
    struct timespec  timespecNew;

    if (tv) {
        lib_clock_gettime(CLOCK_REALTIME, &timespecNew);
        tv->tv_sec  = timespecNew.tv_sec;
        tv->tv_usec = timespecNew.tv_nsec / 1000;
    }
    
    if (tz) {
        tz->tz_minuteswest = (int)(timezone / SECSPERMIN);
        tz->tz_dsttime     = 0;
    }
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: lib_settimeofday
** ��������: 
** �䡡��  : 
** �䡡��  : 
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
INT  lib_settimeofday (const struct timeval *tv, const struct timezone *tz)
{
    struct timespec  timespecNew;

    if (tv) {
        timespecNew.tv_sec  = tv->tv_sec;
        timespecNew.tv_nsec = tv->tv_usec * 1000;
        lib_clock_settime(CLOCK_REALTIME, &timespecNew);
    }
    
    if (tz) {
        /*
         *  �����ʱ����������ʱ���ö���, ʹ�� tzset() ���Զ��� TZ ���������л�ȡʱ����Ϣ.
         */
        timezone = SECSPERMIN * (time_t)tz->tz_minuteswest;
    }
    
    return  (ERROR_NONE);
}

#endif                                                                  /*  LW_CFG_RTC_EN               */
/*********************************************************************************************************
  END
*********************************************************************************************************/
