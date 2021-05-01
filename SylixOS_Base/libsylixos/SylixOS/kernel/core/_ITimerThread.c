/*********************************************************************************************************
**
**                                    中国软件开源组织
**
**                                   嵌入式实时操作系统
**
**                                SylixOS(TM)  LW : long wing
**
**                               Copyright All Rights Reserved
**
**--------------文件信息--------------------------------------------------------------------------------
**
** 文   件   名: _ITimerThread.c
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2006 年 12 月 18 日
**
** 描        述: 这是系统普通定时器周期服务线程。

** BUG
2007.05.11  删除了错误的注释信息
2007.11.13  使用链表库对链表操作进行完全封装.
2008.10.17  改使用精度单调调度进行延迟.
2011.11.29  调用回调时, 不能锁定内核.
2019.02.23  最优时间等待.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
/*********************************************************************************************************

*********************************************************************************************************/
#define LW_ITIMER_IDLE_TICK     (LW_TICK_HZ * 100)
/*********************************************************************************************************
** 函数名称: _ITimerThread
** 功能描述: 这是系统普通定时器周期服务线程。
** 输　入  : 
** 输　出  : 
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
#if	(LW_CFG_ITIMER_EN > 0) && (LW_CFG_MAX_TIMERS > 0)

PVOID  _ITimerThread (PVOID  pvArg)
{
             INTREG                     iregInterLevel;
    REGISTER PLW_CLASS_TIMER            ptmr;
             PTIMER_CALLBACK_ROUTINE    pfuncRoutine;
             PVOID                      pvRoutineArg;

             PLW_CLASS_TCB              ptcbCur;
             PLW_CLASS_PCB              ppcb;

             PLW_CLASS_WAKEUP_NODE      pwun;
             INT64                      i64CurTime;
             ULONG                      ulCounter;
             BOOL                       bNoTimer;
    
    (VOID)pvArg;
    
    LW_TCB_GET_CUR_SAFE(ptcbCur);                                       /*  当前任务控制块              */

    for (;;) {
        iregInterLevel = __KERNEL_ENTER_IRQ();                          /*  进入内核同时关闭中断        */

        __KERNEL_TIME_GET_IGNIRQ(_K_wuITmr.WU_i64LastTime, INT64);      /*  原始时间                    */

        __WAKEUP_GET_FIRST(&_K_wuITmr, pwun);                           /*  获得第一个节点              */

        if (pwun) {
            ulCounter = pwun->WUN_ulCounter;                            /*  已第一个节点等待时间 Sleep  */
            bNoTimer  = LW_FALSE;

        } else {
            ulCounter = LW_ITIMER_IDLE_TICK;                            /*  没有任何节点 (会被自动唤醒) */
            bNoTimer  = LW_TRUE;
        }

        ppcb = _GetPcb(ptcbCur);
        __DEL_FROM_READY_RING(ptcbCur, ppcb);                           /*  从就绪表中删除              */

        ptcbCur->TCB_ulDelay = ulCounter;
        __ADD_TO_WAKEUP_LINE(ptcbCur);                                  /*  加入等待扫描链              */

        __KERNEL_EXIT_IRQ(iregInterLevel);                              /*  退出内核同时打开中断        */

        iregInterLevel = __KERNEL_ENTER_IRQ();                          /*  进入内核同时关闭中断        */
        
        if (bNoTimer) {
            __KERNEL_EXIT_IRQ(iregInterLevel);                          /*  退出内核同时打开中断        */
            continue;
        }

        __KERNEL_TIME_GET_IGNIRQ(i64CurTime, INT64);                    /*  获得 Sleep 后时间           */
        ulCounter = (ULONG)(i64CurTime - _K_wuITmr.WU_i64LastTime);     /*  真正睡眠时间                */
        _K_wuITmr.WU_i64LastTime = i64CurTime;

        __WAKEUP_PASS_FIRST(&_K_wuITmr, pwun, ulCounter);
        
        ptmr = _LIST_ENTRY(pwun, LW_CLASS_TIMER, TIMER_wunTimer);
        
        _WakeupDel(&_K_wuITmr, pwun, LW_FALSE);
        
        if (ptmr->TIMER_ulOption & LW_OPTION_AUTO_RESTART) {
            ptmr->TIMER_ulCounter = ptmr->TIMER_ulCounterSave;
            _WakeupAdd(&_K_wuITmr, pwun, LW_FALSE);
            
        } else {
            ptmr->TIMER_ucStatus = LW_TIMER_STATUS_STOP;                /*  填写停止标志位              */
        }
        
        pfuncRoutine = ptmr->TIMER_cbRoutine;
        pvRoutineArg = ptmr->TIMER_pvArg;
        
        __KERNEL_EXIT_IRQ(iregInterLevel);                              /*  退出内核同时打开中断        */
        
        LW_SOFUNC_PREPARE(pfuncRoutine);
        pfuncRoutine(pvRoutineArg);
        
        iregInterLevel = __KERNEL_ENTER_IRQ();                          /*  进入内核同时关闭中断        */
        
        __WAKEUP_PASS_SECOND();
        
        KN_INT_ENABLE(iregInterLevel);                                  /*  这里允许响应中断            */
    
        iregInterLevel = KN_INT_DISABLE();
        
        __WAKEUP_PASS_END();
        
        __KERNEL_EXIT_IRQ(iregInterLevel);                              /*  退出内核同时打开中断        */
    }
    
    return  (LW_NULL);
}
/*********************************************************************************************************
** 函数名称: _ITimerWakeup
** 功能描述: 唤醒 ITimer 线程。
** 输　入  :
** 输　出  :
** 全局变量:
** 调用模块:
*********************************************************************************************************/
VOID  _ITimerWakeup (VOID)
{
    API_ThreadWakeup(_K_ulThreadItimerId);
}

#endif                                                                  /*  (LW_CFG_ITIMER_EN > 0)      */
                                                                        /*  (LW_CFG_MAX_TIMERS > 0)     */
/*********************************************************************************************************
  END
*********************************************************************************************************/
