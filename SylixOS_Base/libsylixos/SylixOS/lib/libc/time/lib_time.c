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
** ��   ��   ��: lib_time.c
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
** ��������: lib_time
** ��������: 
** �䡡��  : 
** �䡡��  : 
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
#if LW_CFG_RTC_EN > 0

time_t  lib_time (time_t *time)
{
    INTREG        iregInterLevel;
    time_t        timetmp;
    
    LW_SPIN_KERN_TIME_LOCK_QUICK(&iregInterLevel);
    timetmp = _K_tvTODCurrent.tv_sec;
    LW_SPIN_KERN_TIME_UNLOCK_QUICK(iregInterLevel);

    if (time) {
        *time = timetmp;
    }
    
    return  (timetmp);
}
/*********************************************************************************************************
** ��������: lib_timelocal
** ��������: 
** �䡡��  : 
** �䡡��  : 
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
time_t  lib_timelocal (time_t *time)
{
    INTREG        iregInterLevel;
    time_t        timetmp;
    
    LW_SPIN_KERN_TIME_LOCK_QUICK(&iregInterLevel);
    timetmp = UTC2LOCAL(_K_tvTODCurrent.tv_sec);
    LW_SPIN_KERN_TIME_UNLOCK_QUICK(iregInterLevel);

    if (time) {
        *time = timetmp;
    }
    
    return  (timetmp);
}
/*********************************************************************************************************
** ��������: time
** ��������: 
** �䡡��  : 
** �䡡��  : 
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
time_t  time (time_t *time)
{
    INTREG        iregInterLevel;
    time_t        timetmp;
    
    LW_SPIN_KERN_TIME_LOCK_QUICK(&iregInterLevel);
    timetmp = _K_tvTODCurrent.tv_sec;
    LW_SPIN_KERN_TIME_UNLOCK_QUICK(iregInterLevel);

    if (time) {
        *time = timetmp;
    }
    
    return  (timetmp);
}

#endif                                                                  /*  LW_CFG_RTC_EN               */
/*********************************************************************************************************
  END
*********************************************************************************************************/
