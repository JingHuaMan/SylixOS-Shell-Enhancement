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
** ��   ��   ��: ptimer.c
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2008 �� 04 �� 13 ��
**
** ��        ��: posix ����ʱ���.

** BUG
2008.06.06  API_Nanosleep() errno ���ô���, �����źż���ʱ, errno = EINTR, ����ʱ errno = 0;
2008.09.04  POSIX ��ʱ��ʹ��Ӧ�ö�ʱ������, posix �ں˶�ʱ��.
2009.07.01  ���� usleep ����.
2009.10.12  ����� SMP ��˵�֧��.
2009.12.31  �����˾���ʱ��ȴ��� bug.
2010.01.21  alarm() ��ʹ�� abs time.
2010.08.03  ʹ���µĻ�ȡϵͳʱ�ӷ�ʽ.
2010.10.06  ����� cancel type �Ĳ���, ���� POSIX ��׼.
2011.02.26  ʹ�� sigevent ��� sigpend ����.
2011.06.14  nanosleep() usleep() �����ӳ� 1 �� tick.
2011.08.15  �ش����: ������ posix ����ĺ����Ժ�����ʽ(�Ǻ�)����.
2012.03.20  ���ٶ� _K_ptcbTCBCur ������, �������þֲ�����, ���ٶԵ�ǰ CPU ID ��ȡ�Ĵ���.
2013.05.05  sleep �� nanosleep �жϵ���������ֵ, �������������û����˳�.
2013.10.08  ���� setitimer �� getitimer ����.
2013.11.20  ֧�� timerfd ����.
2014.09.29  ����� ITIMER_VIRTUAL �� ITIMER_PROF ֧��.
2015.04.07  ���� clock id ���͵�֧��.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
#include "../SylixOS/system/include/s_system.h"
/*********************************************************************************************************
  �ü�����
*********************************************************************************************************/
#if LW_CFG_PTIMER_EN > 0
#include "sys/time.h"
#if LW_CFG_MODULELOADER_EN > 0
#include "../SylixOS/loader/include/loader_vppatch.h"
#endif                                                                  /*  LW_CFG_MODULELOADER_EN > 0  */
/*********************************************************************************************************
  �ڲ���������
*********************************************************************************************************/
static VOID  __ptimerCallback(LW_OBJECT_ID  ulTimer);
#if LW_CFG_PTIMER_AUTO_DEL_EN > 0
static VOID  __ptimerThreadDeleteHook(LW_OBJECT_HANDLE  ulId);
#endif                                                                  /*  LW_CFG_PTIMER_AUTO_DEL_EN   */
/*********************************************************************************************************
** ��������: __ptimerHookInstall
** ��������: ��ʱ������ɾ���ص���װ
** �䡡��  : NONE
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
#if LW_CFG_PTIMER_AUTO_DEL_EN > 0

static VOID __ptimerHookInstall (VOID)
{
    API_SystemHookAdd(__ptimerThreadDeleteHook, 
                      LW_OPTION_THREAD_DELETE_HOOK);                    /*  ʹ��ɾ���ص�                */
}

#endif                                                                  /*  LW_CFG_PTIMER_AUTO_DEL_EN   */
/*********************************************************************************************************
** ��������: timer_create
** ��������: ����һ����ʱ�� (POSIX)
** �䡡��  : clockid      ʱ���׼
**           sigeventT    �ź�ʱ��
**           ptimer       ��ʱ�����
** �䡡��  : ERROR_NONE  or  PX_ERROR
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
                                           
                                       (�������ж��е���)
*********************************************************************************************************/
LW_API  
INT  timer_create (clockid_t  clockid, struct sigevent *sigeventT, timer_t *ptimer)
{
    return  (timer_create_internal(clockid, sigeventT, ptimer, LW_OPTION_NONE));
}
/*********************************************************************************************************
** ��������: timer_create_internal
** ��������: ����һ����ʱ�� (�ڲ��ӿ�)
** �䡡��  : clockid      ʱ���׼
**           sigeventT    �ź�ʱ��
**           ptimer       ��ʱ�����
**           ulOption     SylixOS ��ʱ��ѡ��
** �䡡��  : ERROR_NONE  or  PX_ERROR
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
INT  timer_create_internal (clockid_t  clockid, struct sigevent *sigeventT, 
                            timer_t *ptimer, ULONG  ulOption)
{
#if LW_CFG_PTIMER_AUTO_DEL_EN > 0
    static BOOL         bIsInstallHook = LW_FALSE;
#endif                                                                  /*  LW_CFG_PTIMER_AUTO_DEL_EN   */

    LW_OBJECT_HANDLE    ulTimer;
    PLW_CLASS_TIMER     ptmr;
    
    if (!ptimer) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    
    if ((clockid != CLOCK_REALTIME) && (clockid != CLOCK_MONOTONIC)) {  /*  ֧�� REAL �� MONO ʱ��      */
        _ErrorHandle(ENOTSUP);
        return  (PX_ERROR);
    }
    
    if (sigeventT) {
        if (!__issig(sigeventT->sigev_signo)) {                         /*  ����ź���Ч��              */
            _ErrorHandle(EINVAL);
            return  (PX_ERROR);
        }
    }
                                                                        /*  �����ź�����ڴ�            */
    ulTimer = API_TimerCreate("posix_tmr", 
                              LW_OPTION_ITIMER | ulOption, LW_NULL);    /*  ������ͨ��ʱ��              */
    if (ulTimer == LW_OBJECT_HANDLE_INVALID) {
        return  (PX_ERROR);
    }
    
#if LW_CFG_PTIMER_AUTO_DEL_EN > 0
    API_ThreadOnce(&bIsInstallHook, __ptimerHookInstall);               /*  ��װ�ص�                    */
#endif                                                                  /*  LW_CFG_PTIMER_AUTO_DEL_EN   */
    
    __KERNEL_ENTER();                                                   /*  �����ں�                    */
    ptmr = &_K_tmrBuffer[_ObjectGetIndex(ulTimer)];
    ptmr->TIMER_ulThreadId = API_ThreadIdSelf();
    ptmr->TIMER_clockid    = clockid;
    __KERNEL_EXIT();                                                    /*  �˳��ں�                    */
    
    if (sigeventT == LW_NULL) {
        ptmr->TIMER_sigevent.sigev_signo  = SIGALRM;                    /*  Ĭ���ź��¼�                */
        ptmr->TIMER_sigevent.sigev_notify = SIGEV_SIGNAL;
        ptmr->TIMER_sigevent.sigev_value.sival_ptr = (PVOID)ulTimer;
    } else {
        ptmr->TIMER_sigevent = *sigeventT;                              /*  ��¼��Ϣ                    */
    }
    
    *ptimer = ulTimer;                                                  /*  ��¼���                    */
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: timer_delete
** ��������: ɾ��һ����ʱ�� (POSIX)
** �䡡��  : timer        ��ʱ�����
** �䡡��  : ERROR_NONE  or  PX_ERROR
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
                                           
                                       (�������ж��е���)
*********************************************************************************************************/
LW_API  
INT  timer_delete (timer_t  timer)
{
             LW_OBJECT_HANDLE   ulTimer = timer;
    REGISTER ULONG              ulError;
    
    if (!timer) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    
    ulError = API_TimerDelete(&ulTimer);
    if (ulError) {
        return  (PX_ERROR);
    }
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: timer_gettime
** ��������: ���һ����ʱ����ʱ����� (POSIX)
** �䡡��  : timer        ��ʱ�����
**           ptvTime      ʱ��
** �䡡��  : ERROR_NONE  or  PX_ERROR
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
                                           
                                       (�������ж��е���)
*********************************************************************************************************/
LW_API  
INT  timer_gettime (timer_t  timer, struct itimerspec  *ptvTime)
{
             BOOL        bIsRunning;
             ULONG       ulCounter;
             ULONG       ulInterval;
    REGISTER ULONG       ulError;

    if (!timer || (ptvTime == LW_NULL)) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    
    ulError = API_TimerStatus((LW_OBJECT_HANDLE)timer, &bIsRunning,
                              LW_NULL, &ulCounter, &ulInterval);        /*  ��ö�ʱ��״̬              */
    if (ulError) {
        return  (PX_ERROR);
    }
                    
    if (bIsRunning == LW_FALSE) {                                       /*  ��ʱ��û������              */
        ptvTime->it_interval.tv_sec  = 0;
        ptvTime->it_interval.tv_nsec = 0;
        ptvTime->it_value.tv_sec     = 0;
        ptvTime->it_value.tv_nsec    = 0;
        return  (ERROR_NONE);
    }
    
    __tickToTimespec(ulInterval, &ptvTime->it_interval);                /*  ��ʱ����װʱ��              */
    __tickToTimespec(ulCounter, &ptvTime->it_value);                    /*  ����һ�ζ�ʱ��ʣ��ʱ��      */
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: timer_getoverrun
** ��������: ���һ����ʱ����������õĴ��� (POSIX)
** �䡡��  : timer        ��ʱ�����
** �䡡��  : �������õĴ���
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
                                           
                                       (�������ж��е���)
*********************************************************************************************************/
LW_API  
INT  timer_getoverrun (timer_t  timer)
{
             INTREG     iregInterLevel;
    REGISTER UINT16     usIndex;
    REGISTER INT        iOverrun;
    LW_OBJECT_HANDLE    ulTimer = timer;
    PLW_CLASS_TIMER     ptmr;
    
    usIndex = _ObjectGetIndex(ulTimer);
    
    iregInterLevel = __KERNEL_ENTER_IRQ();                              /*  �����ں�                    */
    
#if LW_CFG_ARG_CHK_EN > 0
    if (!_ObjectClassOK(ulTimer, _OBJECT_TIMER)) {                      /*  �������ͼ��                */
        __KERNEL_EXIT_IRQ(iregInterLevel);                              /*  �˳��ں�                    */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "timer handle invalidate.\r\n");
        _ErrorHandle(ERROR_KERNEL_HANDLE_NULL);
        return  (PX_ERROR);
    }
    if (_Timer_Index_Invalid(usIndex)) {                                /*  �������������              */
        __KERNEL_EXIT_IRQ(iregInterLevel);                              /*  �˳��ں�                    */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "timer handle invalidate.\r\n");
        _ErrorHandle(ERROR_KERNEL_HANDLE_NULL);
        return  (PX_ERROR);
    }
#endif                                                                  /*  LW_CFG_ARG_CHK_EN > 0       */
    
    ptmr = &_K_tmrBuffer[usIndex];
    if (ptmr->TIMER_u64Overrun > DELAYTIMER_MAX) {
        iOverrun = DELAYTIMER_MAX;
    } else {
        iOverrun = (INT)ptmr->TIMER_u64Overrun;
    }
    
    __KERNEL_EXIT_IRQ(iregInterLevel);                                  /*  �˳��ں�                    */

    return  (iOverrun);
}
/*********************************************************************************************************
** ��������: timer_getoverrun_64
** ��������: ���һ����ʱ����������õĴ���
** �䡡��  : timer        ��ʱ�����
**           pu64Overruns overrun ����
**           bClear       �Ƿ����
** �䡡��  : ERROR_NONE  or  PX_ERROR
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
#if LW_CFG_TIMERFD_EN > 0

INT  timer_getoverrun_64 (timer_t  timer, UINT64  *pu64Overruns, BOOL  bClear)
{
             INTREG     iregInterLevel;
    REGISTER UINT16     usIndex;
    LW_OBJECT_HANDLE    ulTimer = timer;
    PLW_CLASS_TIMER     ptmr;
    
    usIndex = _ObjectGetIndex(ulTimer);
    
    iregInterLevel = __KERNEL_ENTER_IRQ();                              /*  �����ں�                    */
    
#if LW_CFG_ARG_CHK_EN > 0
    if (!_ObjectClassOK(ulTimer, _OBJECT_TIMER)) {                      /*  �������ͼ��                */
        __KERNEL_EXIT_IRQ(iregInterLevel);                              /*  �˳��ں�                    */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "timer handle invalidate.\r\n");
        _ErrorHandle(ERROR_KERNEL_HANDLE_NULL);
        return  (PX_ERROR);
    }
    if (_Timer_Index_Invalid(usIndex)) {                                /*  �������������              */
        __KERNEL_EXIT_IRQ(iregInterLevel);                              /*  �˳��ں�                    */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "timer handle invalidate.\r\n");
        _ErrorHandle(ERROR_KERNEL_HANDLE_NULL);
        return  (PX_ERROR);
    }
#endif                                                                  /*  LW_CFG_ARG_CHK_EN > 0       */
    
    ptmr = &_K_tmrBuffer[usIndex];
    if (pu64Overruns) {
        *pu64Overruns = ptmr->TIMER_u64Overrun;
    }
    if (bClear) {
        ptmr->TIMER_u64Overrun = 0ull;
    }
    
    __KERNEL_EXIT_IRQ(iregInterLevel);                                  /*  �˳��ں�                    */

    return  (ERROR_NONE);
}

#endif                                                                  /*  #if LW_CFG_TIMERFD_EN > 0   */
/*********************************************************************************************************
** ��������: timer_settime
** ��������: �趨һ����ʱ����ʼ��ʱ (POSIX)
** �䡡��  : timer        ��ʱ�����
**           iFlag        ��־
**           ptvNew       �µ�ʱ����Ϣ
**           ptvOld       �����ʱ����Ϣ
** �䡡��  : ERROR_NONE  or  PX_ERROR
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
                                           
                                       (�������ж��е���)
*********************************************************************************************************/
LW_API  
INT  timer_settime (timer_t                  timer, 
                    INT                      iFlag, 
                    const struct itimerspec *ptvNew,
                    struct itimerspec       *ptvOld)
{
             BOOL        bIsRunning;
             ULONG       ulCounter;
             ULONG       ulInterval;
             
    REGISTER ULONG       ulError;
    REGISTER ULONG       ulOption;
    
    LW_OBJECT_HANDLE     ulTimer = timer;
    clockid_t            clockidTimer;
    
    if (!timer) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    
    ulError = API_TimerStatusEx((LW_OBJECT_HANDLE)timer, &bIsRunning,
                                LW_NULL, &ulCounter, &ulInterval, 
                                &clockidTimer);                         /*  ��ö�ʱ��״̬              */
    if (ulError) {
        return  (PX_ERROR);
    }
    
    if (ptvOld) {
        if (bIsRunning == LW_FALSE) {
            ptvOld->it_interval.tv_sec  = 0;
            ptvOld->it_interval.tv_nsec = 0;
            ptvOld->it_value.tv_sec     = 0;
            ptvOld->it_value.tv_nsec    = 0;
        
        } else {
            __tickToTimespec(ulInterval, &ptvOld->it_interval);         /*  ��ʱ����װʱ��              */
            __tickToTimespec(ulCounter, &ptvOld->it_value);             /*  ����һ�ζ�ʱ��ʣ��ʱ��      */
        }
    }
    
    if ((ptvNew == LW_NULL) || 
        LW_NSEC_INVALD(ptvNew->it_value.tv_nsec) ||
        LW_NSEC_INVALD(ptvNew->it_interval.tv_nsec)) {                  /*  ʱ����Ч                    */
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    
    if (ptvNew->it_value.tv_sec  == 0 &&
        ptvNew->it_value.tv_nsec == 0) {                                /*  ֹͣ��ʱ��                  */
        API_TimerCancel(ulTimer);
        return  (ERROR_NONE);
    }
    
    if (ptvNew->it_interval.tv_sec  == 0 &&
        ptvNew->it_interval.tv_nsec == 0) {                             /*  ONE SHOT                    */
        ulOption = LW_OPTION_MANUAL_RESTART;
    } else {
        ulOption = LW_OPTION_AUTO_RESTART;                              /*  AUTO RELOAD                 */
    }
    
    if (iFlag == TIMER_ABSTIME) {                                       /*  ����ʱ��                    */
        struct timespec  tvNow;
        struct timespec  tvValue = ptvNew->it_value;
        
        lib_clock_gettime(clockidTimer, &tvNow);                        /*  ʹ�� clockidTimer           */
        
        if ((ptvNew->it_value.tv_sec < tvNow.tv_sec) ||
            ((ptvNew->it_value.tv_sec == tvNow.tv_sec) &&
             (ptvNew->it_value.tv_nsec < tvNow.tv_nsec))) {
            _ErrorHandle(EINVAL);
            return  (PX_ERROR);
        
        } else {
            __timespecSub(&tvValue, &tvNow);                            /*  ����ʱ���                  */
        }
        
        ulCounter  = __timespecToTick(&tvValue);
        ulInterval = __timespecToTick(&ptvNew->it_interval);
                  
    } else {                                                            /*  ���ʱ��                    */
        ulCounter  = __timespecToTick(&ptvNew->it_value);
        ulInterval = __timespecToTick(&ptvNew->it_interval);
    }
    
    ulCounter  = (ulCounter  == 0) ? 1 : ulCounter;
    ulInterval = (ulInterval == 0) ? 1 : ulInterval;
    
    ulError = API_TimerStartEx(ulTimer, 
                               ulCounter,
                               ulInterval,
                               ulOption,
                               (PTIMER_CALLBACK_ROUTINE)__ptimerCallback,
                               (PVOID)ulTimer);
    if (ulError) {
        return  (PX_ERROR);
    }
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: timer_setfile
** ��������: �趨һ����ʱ����Ӧ�� timerfd �ļ��ṹ
** �䡡��  : timer        ��ʱ�����
**           pvFile       �ļ��ṹ
** �䡡��  : ERROR_NONE  or  PX_ERROR
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
#if LW_CFG_TIMERFD_EN > 0

INT  timer_setfile (timer_t  timer, PVOID  pvFile)
{
             INTREG     iregInterLevel;
    REGISTER UINT16     usIndex;
    LW_OBJECT_HANDLE    ulTimer = timer;
    PLW_CLASS_TIMER     ptmr;
    
    usIndex = _ObjectGetIndex(ulTimer);
    
    iregInterLevel = __KERNEL_ENTER_IRQ();                              /*  �����ں�                    */
    
#if LW_CFG_ARG_CHK_EN > 0
    if (!_ObjectClassOK(ulTimer, _OBJECT_TIMER)) {                      /*  �������ͼ��                */
        __KERNEL_EXIT_IRQ(iregInterLevel);                              /*  �˳��ں�                    */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "timer handle invalidate.\r\n");
        _ErrorHandle(ERROR_KERNEL_HANDLE_NULL);
        return  (PX_ERROR);
    }
    if (_Timer_Index_Invalid(usIndex)) {                                /*  �������������              */
        __KERNEL_EXIT_IRQ(iregInterLevel);                              /*  �˳��ں�                    */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "timer handle invalidate.\r\n");
        _ErrorHandle(ERROR_KERNEL_HANDLE_NULL);
        return  (PX_ERROR);
    }
#endif                                                                  /*  LW_CFG_ARG_CHK_EN > 0       */
    
    ptmr = &_K_tmrBuffer[usIndex];
    ptmr->TIMER_pvTimerfd = pvFile;
    
    __KERNEL_EXIT_IRQ(iregInterLevel);                                  /*  �˳��ں�                    */

    return  (ERROR_NONE);
}

#endif                                                                  /*  #if LW_CFG_TIMERFD_EN > 0   */
/*********************************************************************************************************
** ��������: __ptimerCallback
** ��������: ��ʱ���Ļص�����
** �䡡��  : timer        ��ʱ�����
**           iFlag        ��־
**           ptvNew       �µ�ʱ����Ϣ
**           ptvOld       �����ʱ����Ϣ
** �䡡��  : ERROR_NONE  or  PX_ERROR
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static VOID  __ptimerCallback (LW_OBJECT_ID  ulTimer)
{
#if LW_CFG_TIMERFD_EN > 0
    VOID  _tmrfdCallback(PLW_CLASS_TIMER ptmr);
#endif                                                                  /*  LW_CFG_TIMERFD_EN > 0       */

             INTREG     iregInterLevel;
    REGISTER UINT16     usIndex;
    
    LW_OBJECT_HANDLE    ulThreadId;
    struct siginfo      siginfoTimer;
    struct sigevent     sigeventTimer;
    PLW_CLASS_TIMER     ptmr;
    
    usIndex = _ObjectGetIndex(ulTimer);
    
    LW_SPIN_KERN_LOCK_QUICK(&iregInterLevel);
    ptmr          = &_K_tmrBuffer[usIndex];
    sigeventTimer = ptmr->TIMER_sigevent;
    ulThreadId    = ptmr->TIMER_ulThreadId;
    ptmr->TIMER_u64Overrun++;
    LW_SPIN_KERN_UNLOCK_QUICK(iregInterLevel);
    
#if LW_CFG_TIMERFD_EN > 0
    if (ptmr->TIMER_pvTimerfd) {
        _tmrfdCallback(ptmr);
        return;
    }
#endif                                                                  /*  LW_CFG_TIMERFD_EN > 0       */

    siginfoTimer.si_signo   = ptmr->TIMER_sigevent.sigev_signo;
    siginfoTimer.si_errno   = ERROR_NONE;
    siginfoTimer.si_code    = SI_TIMER;
    siginfoTimer.si_timerid = (INT)ulTimer;
    
    if (ptmr->TIMER_u64Overrun > DELAYTIMER_MAX) {
        siginfoTimer.si_overrun = DELAYTIMER_MAX;
    } else {
        siginfoTimer.si_overrun = (INT)ptmr->TIMER_u64Overrun;
    }
     
    _doSigEventEx(ulThreadId, &sigeventTimer, &siginfoTimer);           /*  ���Ͷ�ʱ�ź�                */
}
/*********************************************************************************************************
** ��������: __ptimerThreadDeleteHook
** ��������: �߳�ɾ��ʱ, ��Ҫ���Լ������� posix ��ʱ��һ��ɾ��, ��Ϊ�������ٴν��յ���ص��ź���
** �䡡��  : timer        ��ʱ�����
**           iFlag        ��־
**           ptvNew       �µ�ʱ����Ϣ
**           ptvOld       �����ʱ����Ϣ
** �䡡��  : ERROR_NONE  or  PX_ERROR
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
#if LW_CFG_PTIMER_AUTO_DEL_EN > 0

static VOID  __ptimerThreadDeleteHook (LW_OBJECT_HANDLE  ulId)
{
    REGISTER INT                 i;
    REGISTER PLW_CLASS_TIMER     ptmr;
    
    for (i = 0; i < LW_CFG_MAX_TIMERS; i++) {                           /*  ɨ�����ж�ʱ��              */
        ptmr = &_K_tmrBuffer[i];
        if (ptmr->TIMER_ucType != LW_TYPE_TIMER_UNUSED) {
            if (ptmr->TIMER_ulThreadId == ulId) {                       /*  ������̴߳�����            */
                API_TimerDelete(&ptmr->TIMER_ulTimer);                  /*  ɾ�������ʱ��              */
            }
        }
    }
}

#endif                                                                  /*  LW_CFG_PTIMER_AUTO_DEL_EN   */
/*********************************************************************************************************
** ��������: setitimer
** ��������: �����ڲ���ʱ��
** �䡡��  : iWhich        ����, ��֧�� ITIMER_REAL 
**           pitValue      ��ʱ����
**           pitOld        ��ǰ����
** �䡡��  : PX_ERROR or ERROR_NONE
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
                                           
                                       (�������ж��е���)
*********************************************************************************************************/
#if LW_CFG_MODULELOADER_EN > 0

LW_API  
INT  setitimer (INT                     iWhich, 
                const struct itimerval *pitValue,
                struct itimerval       *pitOld)
{
    ULONG      ulCounter;
    ULONG      ulInterval;
    ULONG      ulGetCounter;
    ULONG      ulGetInterval;
    INT        iError;
    
    if ((iWhich != ITIMER_REAL)    &&
        (iWhich != ITIMER_VIRTUAL) &&
        (iWhich != ITIMER_PROF)) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    
    if (pitValue) {
        ulCounter  = __timevalToTick(&pitValue->it_value);
        ulInterval = __timevalToTick(&pitValue->it_interval);
        iError = vprocSetitimer(iWhich, ulCounter, ulInterval,
                                &ulGetCounter, &ulGetInterval);
    } else {
        iError = vprocGetitimer(iWhich, &ulGetCounter, &ulGetInterval);
    }
    
    if ((iError == ERROR_NONE) && pitOld) {
        __tickToTimeval(ulGetCounter,  &pitOld->it_value);
        __tickToTimeval(ulGetInterval, &pitOld->it_interval);
    }
    
    return  (iError);
}
/*********************************************************************************************************
** ��������: setitimer
** ��������: ��ȡ�ڲ���ʱ��
** �䡡��  : iWhich        ����, ��֧�� ITIMER_REAL 
**           pitValue      ��ȡ��ǰ��ʱ��Ϣ
** �䡡��  : PX_ERROR or ERROR_NONE
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
                                           
                                       (�������ж��е���)
*********************************************************************************************************/
LW_API  
INT  getitimer (INT iWhich, struct itimerval *pitValue)
{
    ULONG      ulGetCounter;
    ULONG      ulGetInterval;
    INT        iError;
    
    if ((iWhich != ITIMER_REAL)    &&
        (iWhich != ITIMER_VIRTUAL) &&
        (iWhich != ITIMER_PROF)) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    
    iError = vprocGetitimer(iWhich, &ulGetCounter, &ulGetInterval);
    if ((iError == ERROR_NONE) && pitValue) {
        __tickToTimeval(ulGetCounter,  &pitValue->it_value);
        __tickToTimeval(ulGetInterval, &pitValue->it_interval);
    }
    
    return  (iError);
}
/*********************************************************************************************************
** ��������: alarm
** ��������: ����һ������, ��ָ��ʱ�����������ź� (POSIX)
** �䡡��  : uiSeconds    ����
** �䡡��  : PX_ERROR ����ǰ������ʣ������
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
                                           
                                       (�������ж��е���)
*********************************************************************************************************/
LW_API  
UINT  alarm (UINT  uiSeconds)
{
    struct itimerval tvValue, tvOld;
    
    tvValue.it_value.tv_sec  = uiSeconds;
    tvValue.it_value.tv_usec = 0;
    tvValue.it_interval.tv_sec  = 0;
    tvValue.it_interval.tv_usec = 0;
    
    if (setitimer(ITIMER_REAL, &tvValue, &tvOld) < ERROR_NONE) {
        return  (0);
    }
    
    return  ((UINT)tvOld.it_value.tv_sec);
}
/*********************************************************************************************************
** ��������: ualarm
** ��������: ����һ������, ��ָ��ʱ�����������ź� (POSIX)
** �䡡��  : usec          ΢����
**           usecInterval  ���΢����
** �䡡��  : PX_ERROR ����ǰ������ʣ��΢����
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
                                           
                                       (�������ж��е���)
*********************************************************************************************************/
LW_API  
useconds_t ualarm (useconds_t usec, useconds_t usecInterval)
{
    struct itimerval tvValue, tvOld;
    
    tvValue.it_value.tv_sec  = (time_t)(usec / __TIMEVAL_USEC_MAX);
    tvValue.it_value.tv_usec = (LONG)(usec % __TIMEVAL_USEC_MAX);
    tvValue.it_interval.tv_sec  = (time_t)(usecInterval / __TIMEVAL_USEC_MAX);
    tvValue.it_interval.tv_usec = (LONG)(usecInterval % __TIMEVAL_USEC_MAX);
    
    if (setitimer(ITIMER_REAL, &tvValue, &tvOld) < ERROR_NONE) {
        return  (0);
    }
    
    return  ((useconds_t)(tvOld.it_value.tv_sec * __TIMEVAL_USEC_MAX) + 
             (useconds_t)tvOld.it_value.tv_usec);
}

#endif                                                                  /*  LW_CFG_MODULELOADER_EN > 0  */
#endif                                                                  /*  LW_CFG_PTIMER_EN > 0        */
/*********************************************************************************************************
  END
*********************************************************************************************************/
