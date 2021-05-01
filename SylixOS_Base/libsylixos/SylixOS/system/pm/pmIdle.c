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
** ��   ��   ��: pmIdle.c
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2014 �� 07 �� 19 ��
**
** ��        ��: ��Դ�����豸����ʱ�����ӿ�.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
#include "../SylixOS/system/include/s_system.h"
/*********************************************************************************************************
  �ü�����
*********************************************************************************************************/
#if LW_CFG_POWERM_EN > 0
/*********************************************************************************************************
  ȫ�ֱ���
*********************************************************************************************************/
static LW_CLASS_WAKEUP  _G_wuPowerM;
/*********************************************************************************************************
** ��������: _PowerMThread
** ��������: ��Դ�����߳�
** �䡡��  : NONE
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static  PVOID  _PowerMThread (PVOID  pvArg)
{
    PLW_PM_DEV              pmdev;
    PLW_CLASS_WAKEUP_NODE   pwun;

    for (;;) {
        ULONG   ulCounter = LW_TICK_HZ;
        
        __POWERM_LOCK();                                                /*  ����                        */
                           
        __WAKEUP_PASS_FIRST(&_G_wuPowerM, pwun, ulCounter);
        
        pmdev = _LIST_ENTRY(pwun, LW_PM_DEV, PMD_wunTimer);
        
        _WakeupDel(&_G_wuPowerM, pwun, LW_FALSE);
        
        if (pmdev->PMD_pmdfunc &&
            pmdev->PMD_pmdfunc->PMDF_pfuncIdleEnter) {
            pmdev->PMD_uiStatus = LW_PMD_STAT_IDLE;
            __POWERM_UNLOCK();                                          /*  ��ʱ����                    */
            pmdev->PMD_pmdfunc->PMDF_pfuncIdleEnter(pmdev);
            __POWERM_LOCK();                                            /*  ����                        */
        }
        
        __WAKEUP_PASS_SECOND();
        
        __WAKEUP_PASS_END();
        
        __POWERM_UNLOCK();                                              /*  ����                        */
        
        API_TimeSleep(LW_TICK_HZ);                                      /*  �ȴ�һ��                    */
    }
    
    return  (LW_NULL);
}
/*********************************************************************************************************
** ��������: _PowerMInit
** ��������: ��ʼ����Դ����
** �䡡��  : NONE
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
VOID  _PowerMInit (VOID)
{
    LW_CLASS_THREADATTR       threadattr;
    
    _G_ulPowerMLock = API_SemaphoreMCreate("power_lock", LW_PRIO_DEF_CEILING,
                               LW_OPTION_WAIT_PRIORITY | LW_OPTION_DELETE_SAFE |
                               LW_OPTION_INHERIT_PRIORITY | LW_OPTION_OBJECT_GLOBAL,
                               LW_NULL);                                /*  �������ź���                */

    if (!_G_ulPowerMLock) {
        return;
    }
    
    API_ThreadAttrBuild(&threadattr, 
                        LW_CFG_THREAD_POWERM_STK_SIZE, 
                        LW_PRIO_T_POWER, 
                        LW_OPTION_THREAD_STK_CHK | LW_OPTION_THREAD_SAFE | LW_OPTION_OBJECT_GLOBAL, 
                        LW_NULL);
    
	_S_ulPowerMId = API_ThreadCreate("t_power", 
	                                 _PowerMThread,
	                                 &threadattr,
	                                 LW_NULL);                          /*  ���������߳�                */
}
/*********************************************************************************************************
** ��������: API_PowerMDevSetWatchDog
** ��������: �豸��Դ����ʱ�������
** �䡡��  : pmdev         ��Դ����, �豸�ڵ�
**           ulSecs        ����ָ��������, �豸������ idle ģʽ.
** �䡡��  : ERROR ok OK
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
INT  API_PowerMDevSetWatchDog (PLW_PM_DEV  pmdev, ULONG  ulSecs)
{
    BOOL    bBecomeNor = LW_FALSE;

    if (LW_CPU_GET_CUR_NESTING()) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "called from ISR.\r\n");
        _ErrorHandle(ERROR_KERNEL_IN_ISR);
        return  (PX_ERROR);
    }
    
    if (!pmdev || !ulSecs) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    
    __POWERM_LOCK();                                                    /*  ����                        */
    if (pmdev->PMD_bInQ) {
        _WakeupDel(&_G_wuPowerM, &pmdev->PMD_wunTimer, LW_FALSE);
    }
    
    pmdev->PMD_ulCounter = ulSecs;                                      /*  ��λ��ʱ��                  */
    
    _WakeupAdd(&_G_wuPowerM, &pmdev->PMD_wunTimer, LW_FALSE);
    
    if (pmdev->PMD_uiStatus == LW_PMD_STAT_IDLE) {
        pmdev->PMD_uiStatus =  LW_PMD_STAT_NOR;
        bBecomeNor          =  LW_TRUE;
    }
    __POWERM_UNLOCK();                                                  /*  ����                        */
    
    if (bBecomeNor && 
        pmdev->PMD_pmdfunc &&
        pmdev->PMD_pmdfunc->PMDF_pfuncIdleExit) {
        pmdev->PMD_pmdfunc->PMDF_pfuncIdleExit(pmdev);                  /*  �˳� idle                   */
    }
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: API_PowerMDevGetWatchDog
** ��������: �豸��Դ����ʣ��ʱ��
** �䡡��  : pmdev         ��Դ����, �豸�ڵ�
**           pulSecs       �豸���� idle ģʽʣ���ʱ��.
** �䡡��  : ERROR ok OK
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
INT  API_PowerMDevGetWatchDog (PLW_PM_DEV  pmdev, ULONG  *pulSecs)
{
    if (LW_CPU_GET_CUR_NESTING()) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "called from ISR.\r\n");
        _ErrorHandle(ERROR_KERNEL_IN_ISR);
        return  (PX_ERROR);
    }
    
    if (!pmdev || !pulSecs) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    
    __POWERM_LOCK();                                                    /*  ����                        */
    if (pmdev->PMD_bInQ) {
        _WakeupStatus(&_G_wuPowerM, &pmdev->PMD_wunTimer, pulSecs);
    
    } else {
        *pulSecs = 0ul;
    }
    __POWERM_UNLOCK();                                                  /*  ����                        */
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: API_PowerMDevWatchDogOff
** ��������: �豸��Դ����ʱ�����ֹͣ
** �䡡��  : pmdev         ��Դ����, �豸�ڵ�
** �䡡��  : ERROR ok OK
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
INT  API_PowerMDevWatchDogOff (PLW_PM_DEV  pmdev)
{
    BOOL    bBecomeNor = LW_FALSE;

    if (LW_CPU_GET_CUR_NESTING()) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "called from ISR.\r\n");
        _ErrorHandle(ERROR_KERNEL_IN_ISR);
        return  (PX_ERROR);
    }
    
    if (!pmdev) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }

    __POWERM_LOCK();                                                    /*  ����                        */
    if (pmdev->PMD_bInQ) {
        _WakeupDel(&_G_wuPowerM, &pmdev->PMD_wunTimer, LW_FALSE);
    }
    if (pmdev->PMD_uiStatus == LW_PMD_STAT_IDLE) {
        pmdev->PMD_uiStatus =  LW_PMD_STAT_NOR;
        bBecomeNor          =  LW_TRUE;
    }
    __POWERM_UNLOCK();                                                  /*  ����                        */
    
    if (bBecomeNor && 
        pmdev->PMD_pmdfunc &&
        pmdev->PMD_pmdfunc->PMDF_pfuncIdleExit) {
        pmdev->PMD_pmdfunc->PMDF_pfuncIdleExit(pmdev);                  /*  �˳� idle                   */
    }
    
    return  (ERROR_NONE);
}

#endif                                                                  /*  LW_CFG_POWERM_EN            */
/*********************************************************************************************************
  END
*********************************************************************************************************/
