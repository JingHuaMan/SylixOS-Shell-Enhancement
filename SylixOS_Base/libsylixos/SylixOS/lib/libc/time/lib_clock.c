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

** BUG:
2013.11.28  ���� lib_clock_nanosleep().
2015.07.30  ���� lib_clock_getcpuclockid().
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
#include "../SylixOS/system/include/s_system.h"
#if LW_CFG_MODULELOADER_EN > 0
#include "../SylixOS/loader/include/loader_vppatch.h"
#endif                                                                  /*  LW_CFG_MODULELOADER_EN > 0  */
/*********************************************************************************************************
  �߷ֱ���ʱ�ӽӿ�
*********************************************************************************************************/
#if LW_CFG_TIME_HIGH_RESOLUTION_EN > 0
#define LW_TIME_HIGH_RESOLUTION(tv)     bspTickHighResolution(tv)
#else
#define LW_TIME_HIGH_RESOLUTION(tv)
#endif                                                                  /*  LW_CFG_TIME_HIGH_RESOLUT... */
/*********************************************************************************************************
** ��������: lib_clock
** ��������: 
** �䡡��  : 
** �䡡��  : 
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
#if LW_CFG_RTC_EN > 0

clock_t  lib_clock (VOID)
{
    clock_t  clockRet;

#if LW_CFG_MODULELOADER_EN > 0
    INTREG        iregInterLevel;
    LW_LD_VPROC  *pvproc = __LW_VP_GET_CUR_PROC();
    
    if (pvproc == LW_NULL) {
        clockRet = (clock_t)API_TimeGet();

    } else {
        LW_SPIN_KERN_LOCK_QUICK(&iregInterLevel);
        clockRet = pvproc->VP_clockUser;
        LW_SPIN_KERN_UNLOCK_QUICK(iregInterLevel);
    }

#else
    clockRet = (clock_t)API_TimeGet();
#endif                                                                  /*  LW_CFG_MODULELOADER_EN > 0  */
    
    return  (clockRet);
}
/*********************************************************************************************************
** ��������: clock_getcpuclockid
** ��������: 
** �䡡��  : 
** �䡡��  : 
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
INT  lib_clock_getcpuclockid (pid_t pid, clockid_t *clock_id)
{
#if LW_CFG_MODULELOADER_EN > 0
    if (!vprocGet(pid)) {
        return  (PX_ERROR);
    }
    
#else
    if (pid) {
        _ErrorHandle(ESRCH);
        return  (PX_ERROR);
    }
#endif                                                                  /*  LW_CFG_MODULELOADER_EN > 0  */

    if (!clock_id) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    
    *clock_id = CLOCK_PROCESS_CPUTIME_ID;
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: lib_clock_getres
** ��������: 
** �䡡��  : 
** �䡡��  : 
** ȫ�ֱ���: 
** ����ģ��: 
** ע  ��  : ���ﲢû���ж�������ʱ�Ӿ���.
*********************************************************************************************************/
INT  lib_clock_getres (clockid_t  clockid, struct timespec *res)
{
    if ((clockid < 0) || (clockid > CLOCK_THREAD_CPUTIME_ID) || !res) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    
    res->tv_sec  = 0;
    res->tv_nsec = 1;
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: lib_clock_gettime
** ��������: 
** �䡡��  : 
** �䡡��  : 
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
INT  lib_clock_gettime (clockid_t  clockid, struct timespec  *tv)
{
    INTREG          iregInterLevel;
    PLW_CLASS_TCB   ptcbCur;

    if (tv == LW_NULL) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    
    switch (clockid) {
    
    case CLOCK_REALTIME:
        LW_SPIN_KERN_TIME_LOCK_QUICK(&iregInterLevel);
        *tv = _K_tvTODCurrent;
        LW_TIME_HIGH_RESOLUTION(tv);
        LW_SPIN_KERN_TIME_UNLOCK_QUICK(iregInterLevel);
        break;
    
    case CLOCK_MONOTONIC:
        LW_SPIN_KERN_TIME_LOCK_QUICK(&iregInterLevel);
        *tv = _K_tvTODMono;
        LW_TIME_HIGH_RESOLUTION(tv);
        LW_SPIN_KERN_TIME_UNLOCK_QUICK(iregInterLevel);
        break;
        
    case CLOCK_PROCESS_CPUTIME_ID:
#if LW_CFG_MODULELOADER_EN > 0
        {
            LW_LD_VPROC *pvproc = __LW_VP_GET_CUR_PROC();
            if (pvproc == LW_NULL) {
                _ErrorHandle(ESRCH);
                return  (PX_ERROR);
            }
            LW_SPIN_KERN_LOCK_QUICK(&iregInterLevel);
            __tickToTimespec(pvproc->VP_clockUser + pvproc->VP_clockSystem, tv);
            LW_SPIN_KERN_TIME_LOCK_IGNIRQ();
            LW_TIME_HIGH_RESOLUTION(tv);
            LW_SPIN_KERN_TIME_UNLOCK_IGNIRQ();
            LW_SPIN_KERN_UNLOCK_QUICK(iregInterLevel);
        }
#else
        _ErrorHandle(ENOSYS);
        return  (PX_ERROR);
#endif                                                                  /*  LW_CFG_MODULELOADER_EN > 0  */
        break;
        
    case CLOCK_THREAD_CPUTIME_ID:
        LW_SPIN_KERN_LOCK_QUICK(&iregInterLevel);
        LW_TCB_GET_CUR(ptcbCur);
        __tickToTimespec(ptcbCur->TCB_ulCPUTicks, tv);
        LW_SPIN_KERN_TIME_LOCK_IGNIRQ();
        LW_TIME_HIGH_RESOLUTION(tv);
        LW_SPIN_KERN_TIME_UNLOCK_IGNIRQ();
        LW_SPIN_KERN_UNLOCK_QUICK(iregInterLevel);
        break;
        
    default:
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: lib_clock_settime
** ��������: 
** �䡡��  : 
** �䡡��  : 
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
INT  lib_clock_settime (clockid_t  clockid, const struct timespec  *tv)
{
    INTREG      iregInterLevel;

    if (tv == LW_NULL) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    
    if (clockid != CLOCK_REALTIME) {                                    /*  CLOCK_REALTIME              */
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    
    LW_SPIN_KERN_TIME_LOCK_QUICK(&iregInterLevel);
    _K_tvTODCurrent = *tv;
    _K_iTODDelta    = 0;                                                /*  ���֮ǰ��΢��ʱ��          */
    _K_iTODDeltaNs  = 0;
    LW_SPIN_KERN_TIME_UNLOCK_QUICK(iregInterLevel);
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: lib_clock_nanosleep
** ��������: 
** �䡡��  : 
** �䡡��  : 
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
INT  lib_clock_nanosleep (clockid_t  clockid, int  iFlags, 
                          const struct timespec  *rqtp, struct timespec  *rmtp)
{
    INTREG           iregInterLevel;
    struct timespec  tvValue;

    if ((clockid != CLOCK_REALTIME) && (clockid != CLOCK_MONOTONIC)) {
        _ErrorHandle(ENOTSUP);
        return  (ENOTSUP);
    }
    
    if ((!rqtp) ||
        LW_NSEC_INVALD(rqtp->tv_nsec)) {                                /*  ʱ���ʽ����                */
        _ErrorHandle(EINVAL);
        return  (EINVAL);
    }
    
    tvValue = *rqtp;
    
    if (iFlags == TIMER_ABSTIME) {                                      /*  ����ʱ��                    */
        struct timespec  tvNow;
        
        LW_SPIN_KERN_TIME_LOCK_QUICK(&iregInterLevel);
        if (clockid == CLOCK_REALTIME) {
            tvNow = _K_tvTODCurrent;
        } else {
            tvNow = _K_tvTODMono;
        }
        LW_TIME_HIGH_RESOLUTION(&tvNow);
        LW_SPIN_KERN_TIME_UNLOCK_QUICK(iregInterLevel);

        if (__timespecLeftTime(rqtp, &tvNow)) {
            return  (ERROR_NONE);                                       /*  ����Ҫ�ӳ�                  */
        }
        
        __timespecSub(&tvValue, &tvNow);
        
        if (nanosleep(&tvValue, LW_NULL) < 0) {
            return  (errno);
        }
    
    } else {
        if (nanosleep(&tvValue, rmtp) < 0) {
            return  (errno);
        }
    }

    return  (ERROR_NONE);
}

#endif                                                                  /*  LW_CFG_RTC_EN               */
/*********************************************************************************************************
  END
*********************************************************************************************************/
