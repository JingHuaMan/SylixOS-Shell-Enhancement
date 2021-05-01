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
** 文   件   名: MsgQueueReceive.c
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2006 年 12 月 20 日
**
** 描        述: 等待消息队列消息

** BUG
2007.09.19  加入 _DebugHandle() 功能。
2007.10.28  不能在调度器锁定情况下调用.
2008.01.20  调度器已经有了重大的改进, 可以在调度器被锁定的情况下调用此 API.
2008.03.30  由于使用了分离的关闭调度器方式, 所以对线程属性块的操作必须放在关闭中断的情况下.
2008.05.18  加入信号令调度器返回 restart 进行重新阻塞的处理.
2008.05.18  去掉对 LW_EVENT_EXIST, 放在了其他地方.
2008.05.18  使用 __KERNEL_ENTER() 代替 ThreadLock();
2009.04.08  加入对 SMP 多核的支持.
2009.04.15  更新退出时操作, 全面保护多核互斥.
2009.05.28  自从加入多核支持, 关闭中断时间延长了, 今天进行一些优化.
2009.06.05  上个月做的优化有一处问题调试了很久, 就是超时退出时应该关中断.
2009.06.25  pulMsgLen 可以为 NULL,
2009.10.11  将TCB_ulWakeTimer赋值和ulTimeSave赋值提前. 放在等待类型判断分支前面.
2010.08.03  修改获取系统时钟的方式.
2011.02.23  加入 LW_OPTION_SIGNAL_INTER 选项, 事件可以选择自己是否可被中断打断.
2012.03.20  减少对 _K_ptcbTCBCur 的引用, 尽量采用局部变量, 减少对当前 CPU ID 获取的次数.
2013.03.17  加入 API_MsgQueueReceiveEx 可以设置接收选项.
2013.05.05  判断调度器返回值, 决定是重启调用还是退出.
2013.07.18  使用新的获取 TCB 的方法, 确保 SMP 系统安全.
2014.05.29  修复超时后瞬间被激活时对消息的判断错误.
2016.07.21  需要激活写等待的任务.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
/*********************************************************************************************************
ulTimeout 取值：
    
    LW_OPTION_NOT_WAIT                       不进行等待
    LW_OPTION_WAIT_A_TICK                    等待一个系统时钟
    LW_OPTION_WAIT_A_SECOND                  等待一秒
    LW_OPTION_WAIT_INFINITE                  永远等待，直到发生为止
*********************************************************************************************************/
/*********************************************************************************************************
** 函数名称: API_MsgQueueReceive
** 功能描述: 等待消息队列消息
** 输　入  : 
**           ulId            消息队列句柄
**           pvMsgBuffer     消息缓冲区
**           stMaxByteSize   消息缓冲区大小
**           pstMsgLen       消息长度
**           ulTimeout       等待时间
** 输　出  : ERROR_CODE
** 全局变量: 
** 调用模块: 
                                           API 函数
                                           
                                       (不得在中断中调用)
*********************************************************************************************************/
#if (LW_CFG_MSGQUEUE_EN > 0) && (LW_CFG_MAX_MSGQUEUES > 0)

LW_API  
ULONG  API_MsgQueueReceive (LW_OBJECT_HANDLE    ulId,
                            PVOID               pvMsgBuffer,
                            size_t              stMaxByteSize,
                            size_t             *pstMsgLen,
                            ULONG               ulTimeout)

{
             INTREG                iregInterLevel;
             
             PLW_CLASS_TCB         ptcbCur;
    REGISTER UINT16                usIndex;
    REGISTER PLW_CLASS_EVENT       pevent;
    REGISTER PLW_CLASS_MSGQUEUE    pmsgqueue;
    REGISTER PLW_CLASS_TCB         ptcb;
    REGISTER UINT8                 ucPriorityIndex;
    REGISTER PLW_LIST_RING        *ppringList;
             ULONG                 ulTimeSave;                          /*  系统事件记录                */
             INT                   iSchedRet;
             
             ULONG                 ulEventOption;                       /*  事件创建选项                */
             size_t                stMsgLenTemp;                        /*  临时记录变量                */
    
    usIndex = _ObjectGetIndex(ulId);
    
    if (LW_CPU_GET_CUR_NESTING()) {                                     /*  不能在中断中调用            */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "called from ISR.\r\n");
        _ErrorHandle(ERROR_KERNEL_IN_ISR);
        return  (ERROR_KERNEL_IN_ISR);
    }
    
    LW_TCB_GET_CUR_SAFE(ptcbCur);                                       /*  当前任务控制块              */
    
    if (pstMsgLen == LW_NULL) {
        pstMsgLen =  &stMsgLenTemp;                                     /*  临时变量记录消息长短        */
    }
    
__wait_again:
#if LW_CFG_ARG_CHK_EN > 0
    if (!pvMsgBuffer || !stMaxByteSize) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "pvMsgBuffer invalidate.\r\n");
        _ErrorHandle(ERROR_MSGQUEUE_MSG_NULL);
        return  (ERROR_MSGQUEUE_MSG_NULL);
    }
    if (!_ObjectClassOK(ulId, _OBJECT_MSGQUEUE)) {
        _ErrorHandle(ERROR_KERNEL_HANDLE_NULL);
        return  (ERROR_KERNEL_HANDLE_NULL);
    }
    if (_Event_Index_Invalid(usIndex)) {
        _ErrorHandle(ERROR_KERNEL_HANDLE_NULL);
        return  (ERROR_KERNEL_HANDLE_NULL);
    }
#endif
    pevent = &_K_eventBuffer[usIndex];
    
    iregInterLevel = __KERNEL_ENTER_IRQ();                              /*  进入内核                    */
    if (_Event_Type_Invalid(usIndex, LW_TYPE_EVENT_MSGQUEUE)) {
        __KERNEL_EXIT_IRQ(iregInterLevel);                              /*  退出内核                    */
        _ErrorHandle(ERROR_MSGQUEUE_TYPE);
        return  (ERROR_MSGQUEUE_TYPE);
    }
    
    pmsgqueue = (PLW_CLASS_MSGQUEUE)pevent->EVENT_pvPtr;
    
    ptcbCur->TCB_ulRecvOption = LW_OPTION_NOERROR;                      /*  接收大消息自动截断          */
    
    if (pevent->EVENT_ulCounter) {                                      /*  事件有效                    */
        pevent->EVENT_ulCounter--;
        _MsgQueueGet(pmsgqueue, pvMsgBuffer, 
                     stMaxByteSize, pstMsgLen);                         /*  获得消息                    */
        
        if (_EventWaitNum(EVENT_MSG_Q_S, pevent)) {                     /*  有任务在等待写消息          */
            if (pevent->EVENT_ulOption & LW_OPTION_WAIT_PRIORITY) {     /*  优先级等待队列              */
                _EVENT_DEL_Q_PRIORITY(EVENT_MSG_Q_S, ppringList);       /*  激活优先级等待线程          */
                ptcb = _EventReadyPriorityLowLevel(pevent, LW_NULL, ppringList);
            
            } else {
                _EVENT_DEL_Q_FIFO(EVENT_MSG_Q_S, ppringList);           /*  激活FIFO等待线程            */
                ptcb = _EventReadyFifoLowLevel(pevent, LW_NULL, ppringList);
            }
            
            KN_INT_ENABLE(iregInterLevel);                              /*  使能中断                    */
            _EventReadyHighLevel(ptcb, LW_THREAD_STATUS_MSGQUEUE);      /*  处理 TCB                    */
            __KERNEL_EXIT();                                            /*  退出内核                    */
        
        } else {
            __KERNEL_EXIT_IRQ(iregInterLevel);                          /*  退出内核                    */
        }
        return  (ERROR_NONE);
    }
    
    if (ulTimeout == LW_OPTION_NOT_WAIT) {                              /*  不等待                      */
        __KERNEL_EXIT_IRQ(iregInterLevel);                              /*  退出内核                    */
        _ErrorHandle(ERROR_THREAD_WAIT_TIMEOUT);                        /*  超时                        */
        return  (ERROR_THREAD_WAIT_TIMEOUT);
    }
    
    ptcbCur->TCB_pstMsgByteSize    = pstMsgLen;
    ptcbCur->TCB_stMaxByteSize     = stMaxByteSize;
    ptcbCur->TCB_pvMsgQueueMessage = pvMsgBuffer;                       /*  记录信息                    */
    
    ptcbCur->TCB_iPendQ         = EVENT_MSG_Q_R;
    ptcbCur->TCB_usStatus      |= LW_THREAD_STATUS_MSGQUEUE;            /*  写状态位，开始等待          */
    ptcbCur->TCB_ucWaitTimeout  = LW_WAIT_TIME_CLEAR;                   /*  清空等待时间                */
    
    if (ulTimeout == LW_OPTION_WAIT_INFINITE) {                         /*  是否是无穷等待              */
        ptcbCur->TCB_ulDelay = 0ul;
    } else {
        ptcbCur->TCB_ulDelay = ulTimeout;                               /*  设置超时时间                */
    }
    __KERNEL_TIME_GET_IGNIRQ(ulTimeSave, ULONG);                        /*  记录系统时间                */
    
    if (pevent->EVENT_ulOption & LW_OPTION_WAIT_PRIORITY) {
        _EVENT_INDEX_Q_PRIORITY(ptcbCur->TCB_ucPriority, ucPriorityIndex);
        _EVENT_PRIORITY_Q_PTR(EVENT_MSG_Q_R, ppringList, ucPriorityIndex);
        ptcbCur->TCB_ppringPriorityQueue = ppringList;                  /*  记录等待队列位置            */
        _EventWaitPriority(pevent, ppringList);                         /*  加入优先级等待表            */
        
    } else {                                                            /*  按 FIFO 等待                */
        _EVENT_FIFO_Q_PTR(EVENT_MSG_Q_R, ppringList);                   /*  确定 FIFO 队列的位置        */
        _EventWaitFifo(pevent, ppringList);                             /*  加入 FIFO 等待表            */
    }
    
    KN_INT_ENABLE(iregInterLevel);                                      /*  使能中断                    */
    
    ulEventOption = pevent->EVENT_ulOption;
    
    MONITOR_EVT_LONG2(MONITOR_EVENT_ID_MSGQ, MONITOR_EVENT_MSGQ_PEND, 
                      ulId, ulTimeout, LW_NULL);
    
    iSchedRet = __KERNEL_EXIT();                                        /*  调度器解锁                  */
    if (iSchedRet) {
        if ((iSchedRet == LW_SIGNAL_EINTR) && 
            (ulEventOption & LW_OPTION_SIGNAL_INTER)) {
            _ErrorHandle(EINTR);
            return  (EINTR);
        }
        ulTimeout = _sigTimeoutRecalc(ulTimeSave, ulTimeout);           /*  重新计算超时时间            */
        if (ulTimeout == LW_OPTION_NOT_WAIT) {
            _ErrorHandle(ERROR_THREAD_WAIT_TIMEOUT);
            return  (ERROR_THREAD_WAIT_TIMEOUT);
        }
        goto    __wait_again;
    }
    
    if (ptcbCur->TCB_ucWaitTimeout == LW_WAIT_TIME_OUT) {
        _ErrorHandle(ERROR_THREAD_WAIT_TIMEOUT);                        /*  超时                        */
        return  (ERROR_THREAD_WAIT_TIMEOUT);
        
    } else {
        if (ptcbCur->TCB_ucIsEventDelete == LW_EVENT_EXIST) {           /*  事件是否存在                */
            return  (ERROR_NONE);
        
        } else {
            _ErrorHandle(ERROR_MSGQUEUE_WAS_DELETED);                   /*  已经被删除                  */
            return  (ERROR_MSGQUEUE_WAS_DELETED);
        }
    }
}
/*********************************************************************************************************
** 函数名称: API_MsgQueueReceiveEx
** 功能描述: 等待消息队列消息
** 输　入  : 
**           ulId            消息队列句柄
**           pvMsgBuffer     消息缓冲区
**           stMaxByteSize   消息缓冲区大小
**           pstMsgLen       消息长度
**           ulTimeout       等待时间
**           ulOption        接收选项
** 输　出  : ERROR_CODE
** 全局变量: 
** 调用模块: 
                                           API 函数
                                           
                                       (不得在中断中调用)
*********************************************************************************************************/
LW_API  
ULONG  API_MsgQueueReceiveEx (LW_OBJECT_HANDLE    ulId,
                              PVOID               pvMsgBuffer,
                              size_t              stMaxByteSize,
                              size_t             *pstMsgLen,
                              ULONG               ulTimeout,
                              ULONG               ulOption)
{
    
             INTREG                iregInterLevel;
             
             PLW_CLASS_TCB         ptcbCur;
    REGISTER UINT16                usIndex;
    REGISTER PLW_CLASS_EVENT       pevent;
    REGISTER PLW_CLASS_TCB         ptcb;
    REGISTER PLW_CLASS_MSGQUEUE    pmsgqueue;
    REGISTER UINT8                 ucPriorityIndex;
    REGISTER PLW_LIST_RING        *ppringList;
             ULONG                 ulTimeSave;                          /*  系统事件记录                */
             INT                   iSchedRet;
             
             ULONG                 ulEventOption;                       /*  事件创建选项                */
             size_t                stMsgLenTemp;                        /*  临时记录变量                */
    
    usIndex = _ObjectGetIndex(ulId);
    
    if (LW_CPU_GET_CUR_NESTING()) {                                     /*  不能在中断中调用            */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "called from ISR.\r\n");
        _ErrorHandle(ERROR_KERNEL_IN_ISR);
        return  (ERROR_KERNEL_IN_ISR);
    }
    
    LW_TCB_GET_CUR_SAFE(ptcbCur);                                       /*  当前任务控制块              */
    
    if (pstMsgLen == LW_NULL) {
        pstMsgLen =  &stMsgLenTemp;                                     /*  临时变量记录消息长短        */
    }
    
__wait_again:
#if LW_CFG_ARG_CHK_EN > 0
    if (!pvMsgBuffer || !stMaxByteSize) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "pvMsgBuffer invalidate.\r\n");
        _ErrorHandle(ERROR_MSGQUEUE_MSG_NULL);
        return  (ERROR_MSGQUEUE_MSG_NULL);
    }
    if (!_ObjectClassOK(ulId, _OBJECT_MSGQUEUE)) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "msgqueue handle invalidate.\r\n");
        _ErrorHandle(ERROR_KERNEL_HANDLE_NULL);
        return  (ERROR_KERNEL_HANDLE_NULL);
    }
    if (_Event_Index_Invalid(usIndex)) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "msgqueue handle invalidate.\r\n");
        _ErrorHandle(ERROR_KERNEL_HANDLE_NULL);
        return  (ERROR_KERNEL_HANDLE_NULL);
    }
#endif
    pevent = &_K_eventBuffer[usIndex];
    
    iregInterLevel = __KERNEL_ENTER_IRQ();                              /*  进入内核                    */
    if (_Event_Type_Invalid(usIndex, LW_TYPE_EVENT_MSGQUEUE)) {
        __KERNEL_EXIT_IRQ(iregInterLevel);                              /*  退出内核                    */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "msgqueue handle invalidate.\r\n");
        _ErrorHandle(ERROR_MSGQUEUE_TYPE);
        return  (ERROR_MSGQUEUE_TYPE);
    }
    
    pmsgqueue = (PLW_CLASS_MSGQUEUE)pevent->EVENT_pvPtr;
    
    ptcbCur->TCB_ulRecvOption = ulOption;
    
    if (pevent->EVENT_ulCounter) {                                      /*  事件有效                    */
        size_t  stMsgLenInBuffer;
        
        _MsgQueueMsgLen(pmsgqueue, &stMsgLenInBuffer);
        if ((stMsgLenInBuffer > stMaxByteSize) && 
            !(ulOption & LW_OPTION_NOERROR)) {
            __KERNEL_EXIT_IRQ(iregInterLevel);                          /*  退出内核                    */
            _ErrorHandle(E2BIG);                                        /*  退出                        */
            return  (E2BIG);
        }
        
        pevent->EVENT_ulCounter--;
        _MsgQueueGet(pmsgqueue, pvMsgBuffer, 
                     stMaxByteSize, pstMsgLen);                         /*  获得消息                    */
        
        if (_EventWaitNum(EVENT_MSG_Q_S, pevent)) {                     /*  有任务在等待写消息          */
            if (pevent->EVENT_ulOption & LW_OPTION_WAIT_PRIORITY) {     /*  优先级等待队列              */
                _EVENT_DEL_Q_PRIORITY(EVENT_MSG_Q_S, ppringList);       /*  激活优先级等待线程          */
                ptcb = _EventReadyPriorityLowLevel(pevent, LW_NULL, ppringList);
            
            } else {
                _EVENT_DEL_Q_FIFO(EVENT_MSG_Q_S, ppringList);           /*  激活FIFO等待线程            */
                ptcb = _EventReadyFifoLowLevel(pevent, LW_NULL, ppringList);
            }
            
            KN_INT_ENABLE(iregInterLevel);                              /*  使能中断                    */
            _EventReadyHighLevel(ptcb, LW_THREAD_STATUS_MSGQUEUE);      /*  处理 TCB                    */
            __KERNEL_EXIT();                                            /*  退出内核                    */
        
        } else {
            __KERNEL_EXIT_IRQ(iregInterLevel);                          /*  退出内核                    */
        }
        return  (ERROR_NONE);
    }
    
    if (ulTimeout == LW_OPTION_NOT_WAIT) {                              /*  不等待                      */
        __KERNEL_EXIT_IRQ(iregInterLevel);                              /*  退出内核                    */
        _ErrorHandle(ERROR_THREAD_WAIT_TIMEOUT);                        /*  超时                        */
        return  (ERROR_THREAD_WAIT_TIMEOUT);
    }
    
    ptcbCur->TCB_pstMsgByteSize    = pstMsgLen;
    ptcbCur->TCB_stMaxByteSize     = stMaxByteSize;
    ptcbCur->TCB_pvMsgQueueMessage = pvMsgBuffer;                       /*  记录信息                    */
    
    ptcbCur->TCB_iPendQ         = EVENT_MSG_Q_R;
    ptcbCur->TCB_usStatus      |= LW_THREAD_STATUS_MSGQUEUE;            /*  写状态位，开始等待          */
    ptcbCur->TCB_ucWaitTimeout  = LW_WAIT_TIME_CLEAR;                   /*  清空等待时间                */
    
    if (ulTimeout == LW_OPTION_WAIT_INFINITE) {                         /*  是否是无穷等待              */
        ptcbCur->TCB_ulDelay = 0ul;
    } else {
        ptcbCur->TCB_ulDelay = ulTimeout;                               /*  设置超时时间                */
    }
    __KERNEL_TIME_GET_IGNIRQ(ulTimeSave, ULONG);                        /*  记录系统时间                */
    
    if (pevent->EVENT_ulOption & LW_OPTION_WAIT_PRIORITY) {
        _EVENT_INDEX_Q_PRIORITY(ptcbCur->TCB_ucPriority, ucPriorityIndex);
        _EVENT_PRIORITY_Q_PTR(EVENT_MSG_Q_R, ppringList, ucPriorityIndex);
        ptcbCur->TCB_ppringPriorityQueue = ppringList;                  /*  记录等待队列位置            */
        _EventWaitPriority(pevent, ppringList);                         /*  加入优先级等待表            */
        
    } else {                                                            /*  按 FIFO 等待                */
        _EVENT_FIFO_Q_PTR(EVENT_MSG_Q_R, ppringList);                   /*  确定 FIFO 队列的位置        */
        _EventWaitFifo(pevent, ppringList);                             /*  加入 FIFO 等待表            */
    }
    
    KN_INT_ENABLE(iregInterLevel);                                      /*  使能中断                    */
    
    ulEventOption = pevent->EVENT_ulOption;
    
    MONITOR_EVT_LONG2(MONITOR_EVENT_ID_MSGQ, MONITOR_EVENT_MSGQ_PEND, 
                      ulId, ulTimeout, LW_NULL);
    
    iSchedRet = __KERNEL_EXIT();                                        /*  调度器解锁                  */
    if (iSchedRet) {
        if ((iSchedRet == LW_SIGNAL_EINTR) && 
            (ulEventOption & LW_OPTION_SIGNAL_INTER)) {
            _ErrorHandle(EINTR);
            return  (EINTR);
        }
        ulTimeout = _sigTimeoutRecalc(ulTimeSave, ulTimeout);           /*  重新计算超时时间            */
        if (ulTimeout == LW_OPTION_NOT_WAIT) {
            _ErrorHandle(ERROR_THREAD_WAIT_TIMEOUT);
            return  (ERROR_THREAD_WAIT_TIMEOUT);
        }
        goto    __wait_again;
    }
    
    if (ptcbCur->TCB_ucWaitTimeout == LW_WAIT_TIME_OUT) {
        _ErrorHandle(ERROR_THREAD_WAIT_TIMEOUT);                        /*  超时                        */
        return  (ERROR_THREAD_WAIT_TIMEOUT);
        
    } else {
        if (ptcbCur->TCB_ucIsEventDelete == LW_EVENT_EXIST) {           /*  事件是否存在                */
            if ((*pstMsgLen == 0) && (ptcbCur->TCB_stMaxByteSize == 0)) {
                _ErrorHandle(E2BIG);                                    /*  退出                        */
                return  (E2BIG);
            
            } else {
                return  (ERROR_NONE);
            }
        
        } else {
            _ErrorHandle(ERROR_MSGQUEUE_WAS_DELETED);                   /*  已经被删除                  */
            return  (ERROR_MSGQUEUE_WAS_DELETED);
        }
    }
}

#endif                                                                  /*  (LW_CFG_MSGQUEUE_EN > 0)    */
                                                                        /*  (LW_CFG_MAX_MSGQUEUES > 0)  */
/*********************************************************************************************************
  END
*********************************************************************************************************/
