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
** ��   ��   ��: loader_vptimer.c
**
** ��   ��   ��: Han.hui (����)
**
** �ļ���������: 2011 �� 08 �� 03 ��
**
** ��        ��: ���̶�ʱ��֧��.
**
** BUG:
2017.05.27  ���� vprocItimerHook() ��һ�� tick ������, �ڶ���϶�μ���ͬһ�����̵����.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "SylixOS.h"
#include "sys/time.h"
/*********************************************************************************************************
  �ü�֧��
*********************************************************************************************************/
#if (LW_CFG_MODULELOADER_EN > 0) && (LW_CFG_PTIMER_EN > 0)
#include "../include/loader_lib.h"
/*********************************************************************************************************
  �����б�
*********************************************************************************************************/
extern LW_LIST_LINE_HEADER      _G_plineVProcHeader;
/*********************************************************************************************************
** ��������: vprocItimerMainHook
** ��������: ���̶�ʱ�� tick hook (�ж����������������ں�״̬������, ÿ�� tick �жϽ�����һ��)
** �䡡��  : NONE
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
VOID  vprocItimerMainHook (VOID)
{
    LW_LD_VPROC     *pvproc;
    LW_LD_VPROC_T   *pvptimer;
    LW_LIST_LINE    *plineTemp;
    struct sigevent  sigeventTimer;
    struct siginfo   siginfoTimer;
    
    sigeventTimer.sigev_signo           = SIGALRM;
    sigeventTimer.sigev_notify          = SIGEV_SIGNAL;
    sigeventTimer.sigev_value.sival_ptr = LW_NULL;
    
    siginfoTimer.si_errno   = ERROR_NONE;
    siginfoTimer.si_code    = SI_TIMER;
    siginfoTimer.si_signo   = SIGALRM;
    siginfoTimer.si_timerid = ITIMER_REAL;
    
    for (plineTemp  = _G_plineVProcHeader;
         plineTemp != LW_NULL;
         plineTemp  = _list_line_get_next(plineTemp)) {                 /*  ITIMER_REAL ��ʱ��          */
         
        pvproc = _LIST_ENTRY(plineTemp, LW_LD_VPROC, VP_lineManage);
        if (pvproc == &_G_vprocKernel) {
            continue;
        }
        
        pvptimer = &pvproc->VP_vptimer[ITIMER_REAL];
        if (pvptimer->VPT_ulCounter) {
            pvptimer->VPT_ulCounter--;
            if (pvptimer->VPT_ulCounter == 0ul) {
                _doSigEventEx(pvproc->VP_ulMainThread, &sigeventTimer, &siginfoTimer);
                pvptimer->VPT_ulCounter = pvptimer->VPT_ulInterval;
            }
        }
    }
}
/*********************************************************************************************************
** ��������: vprocItimerEachHook
** ��������: ���̶�ʱ�� tick hook (�ж����������������ں�״̬������, ��Ҫ���� CPU)
** �䡡��  : pcpu    ��Ӧ CPU ���ƿ�
**           pvproc  ���̿��ƿ�
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
VOID  vprocItimerEachHook (PLW_CLASS_CPU  pcpu, LW_LD_VPROC  *pvproc)
{
    LW_LD_VPROC_T   *pvptimer;
    struct sigevent  sigeventTimer;
    struct siginfo   siginfoTimer;
    LW_OBJECT_HANDLE ulThreadId;
    
    sigeventTimer.sigev_notify          = SIGEV_SIGNAL;
    sigeventTimer.sigev_value.sival_ptr = LW_NULL;
    
    siginfoTimer.si_errno = ERROR_NONE;
    siginfoTimer.si_code  = SI_TIMER;
    
    ulThreadId = pvproc->VP_ulMainThread;                               /*  ���߳�                      */
    
    if (pcpu->CPU_iKernelCounter == 0) {                                /*  ITIMER_VIRTUAL ��ʱ��       */
        pvptimer = &pvproc->VP_vptimer[ITIMER_VIRTUAL];
        if (pvptimer->VPT_ulCounter) {
            pvptimer->VPT_ulCounter--;
            if (pvptimer->VPT_ulCounter == 0ul) {
                sigeventTimer.sigev_signo = SIGVTALRM;
                siginfoTimer.si_signo     = SIGVTALRM;
                siginfoTimer.si_timerid   = ITIMER_VIRTUAL;
                _doSigEventEx(ulThreadId, &sigeventTimer, &siginfoTimer);
                pvptimer->VPT_ulCounter = pvptimer->VPT_ulInterval;
            }
        }
    }
    
    pvptimer = &pvproc->VP_vptimer[ITIMER_PROF];
    if (pvptimer->VPT_ulCounter) {                                      /*  ITIMER_PROF ��ʱ��          */
        pvptimer->VPT_ulCounter--;
        if (pvptimer->VPT_ulCounter == 0ul) {
            sigeventTimer.sigev_signo = SIGPROF;
            siginfoTimer.si_signo     = SIGPROF;
            siginfoTimer.si_timerid   = ITIMER_PROF;
            _doSigEventEx(ulThreadId, &sigeventTimer, &siginfoTimer);
            pvptimer->VPT_ulCounter = pvptimer->VPT_ulInterval;
        }
    }
}
/*********************************************************************************************************
** ��������: vprocSetitimer
** ��������: ���ý����ڲ���ʱ��
** �䡡��  : iWhich        ����, ITIMER_REAL / ITIMER_VIRTUAL / ITIMER_PROF
**           pitValue      ��ʱ����
**           pitOld        ��ǰ����
** �䡡��  : PX_ERROR or ERROR_NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
INT  vprocSetitimer (INT        iWhich, 
                     ULONG      ulCounter,
                     ULONG      ulInterval,
                     ULONG     *pulCounter,
                     ULONG     *pulInterval)
{
    INTREG           iregInterLevel;
    LW_LD_VPROC     *pvproc;
    LW_LD_VPROC_T   *pvptimer;
    PLW_CLASS_TCB    ptcbCur;

    if (LW_KERN_NO_ITIMER_EN_GET()) {
        _PrintHandle("Warning: no itimer support (kernel start parameter: noitmr=yes)!\r\n");
    }

    LW_TCB_GET_CUR_SAFE(ptcbCur);
    
    pvproc = __LW_VP_GET_TCB_PROC(ptcbCur);
    if (pvproc == LW_NULL) {                                            /*  ���Խ�����Ч                */
        _ErrorHandle(ESRCH);
        return  (PX_ERROR);
    }
    
    pvptimer = &pvproc->VP_vptimer[iWhich];
    
    LW_SPIN_KERN_LOCK_QUICK(&iregInterLevel);
    if (pulCounter) {
        *pulCounter = pvptimer->VPT_ulCounter;
    }
    if (pulInterval) {
        *pulInterval = pvptimer->VPT_ulInterval;
    }
    pvptimer->VPT_ulCounter  = ulCounter;
    pvptimer->VPT_ulInterval = ulInterval;
    LW_SPIN_KERN_UNLOCK_QUICK(iregInterLevel);
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: vprocGetitimer
** ��������: ��ȡ�����ڲ���ʱ��
** �䡡��  : iWhich        ����, ITIMER_REAL / ITIMER_VIRTUAL / ITIMER_PROF
**           pitValue      ��ȡ��ǰ��ʱ��Ϣ
** �䡡��  : PX_ERROR or ERROR_NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
INT  vprocGetitimer (INT        iWhich, 
                     ULONG     *pulCounter,
                     ULONG     *pulInterval)
{
    INTREG           iregInterLevel;
    LW_LD_VPROC     *pvproc;
    LW_LD_VPROC_T   *pvptimer;
    PLW_CLASS_TCB    ptcbCur;

    LW_TCB_GET_CUR_SAFE(ptcbCur);
    
    pvproc = __LW_VP_GET_TCB_PROC(ptcbCur);
    if (pvproc == LW_NULL) {                                            /*  ���Խ�����Ч                */
        _ErrorHandle(ESRCH);
        return  (PX_ERROR);
    }
    
    pvptimer = &pvproc->VP_vptimer[iWhich];
    
    LW_SPIN_KERN_LOCK_QUICK(&iregInterLevel);
    if (pulCounter) {
        *pulCounter = pvptimer->VPT_ulCounter;
    }
    if (pulInterval) {
        *pulInterval = pvptimer->VPT_ulInterval;
    }
    LW_SPIN_KERN_UNLOCK_QUICK(iregInterLevel);
    
    return  (ERROR_NONE);
}

#endif                                                                  /*  LW_CFG_MODULELOADER_EN > 0  */
                                                                        /*  LW_CFG_PTIMER_EN > 0        */
/*********************************************************************************************************
  END
*********************************************************************************************************/
