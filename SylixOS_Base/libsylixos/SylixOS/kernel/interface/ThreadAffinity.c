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
** 文   件   名: ThreadAffinity.c
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2014 年 11 月 11 日
**
** 描        述: 线程亲和度模型.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
/*********************************************************************************************************
  裁剪支持
*********************************************************************************************************/
#if LW_CFG_SMP_EN > 0
/*********************************************************************************************************
** 函数名称: API_ThreadSetAffinity
** 功能描述: 将线程锁定到指定的 CPU 运行.
** 输　入  : ulId          线程
**           stSize        CPU 掩码集内存大小
**           pcpuset       CPU 掩码
** 输　出  : ERROR
** 全局变量: 
** 调用模块: 
** 注  意  : 当前只能将任务锁定到一个 CPU 上, 如果指定的 CPU 没有激活则其他所有核均可调度.

                                           API 函数
*********************************************************************************************************/
LW_API  
ULONG  API_ThreadSetAffinity (LW_OBJECT_HANDLE  ulId, size_t  stSize, const PLW_CLASS_CPUSET  pcpuset)
{
    REGISTER UINT16         usIndex;
    REGISTER PLW_CLASS_TCB  ptcb;
             PLW_CLASS_TCB  ptcbCur;
             ULONG          ulError;

    usIndex = _ObjectGetIndex(ulId);

#if LW_CFG_ARG_CHK_EN > 0
    if (!_ObjectClassOK(ulId, _OBJECT_THREAD)) {                        /*  检查 ID 类型有效性          */
        _ErrorHandle(ERROR_KERNEL_HANDLE_NULL);
        return  (ERROR_KERNEL_HANDLE_NULL);
    }
    if (_Thread_Index_Invalid(usIndex)) {                               /*  检查线程有效性              */
        _ErrorHandle(ERROR_KERNEL_HANDLE_NULL);
        return  (ERROR_KERNEL_HANDLE_NULL);
    }
    if (!stSize || !pcpuset) {
        _ErrorHandle(EINVAL);
        return  (EINVAL);
    }
#endif

    if (ulId == API_KernelGetExc()) {                                   /*  不允许设置 exce 线程        */
        _ErrorHandle(EPERM);
        return  (EPERM);
    }
    
    __KERNEL_ENTER();                                                   /*  进入内核                    */
    if (_Thread_Invalid(usIndex)) {
        __KERNEL_EXIT();                                                /*  退出内核                    */
        _ErrorHandle(ERROR_KERNEL_HANDLE_NULL);
        return  (ERROR_KERNEL_HANDLE_NULL);
    }

    ptcb = _K_ptcbTCBIdTable[usIndex];
    if (ptcb->TCB_iDeleteProcStatus) {                                  /*  在删除和重启的过程中        */
        __KERNEL_EXIT();                                                /*  退出内核                    */
        _ErrorHandle(ERROR_THREAD_NULL);
        return  (ERROR_THREAD_NULL);
    }

    LW_TCB_GET_CUR(ptcbCur);
    if (ptcb == ptcbCur) {
        if (__THREAD_LOCK_GET(ptcb) > 1) {                              /*  任务被锁定                  */
            __KERNEL_EXIT();                                            /*  退出内核                    */
            _ErrorHandle(EBUSY);
            return  (EBUSY);
        }
        _ThreadSetAffinity(ptcb, stSize, pcpuset);                      /*  设置                        */

    } else {
        if (__THREAD_LOCK_GET(ptcb)) {                                  /*  任务被锁定                  */
            __KERNEL_EXIT();                                            /*  退出内核                    */
            _ErrorHandle(EBUSY);
            return  (EBUSY);
        }

        ulError = _ThreadStop(ptcb);
        __KERNEL_EXIT();                                                /*  退出内核                    */
        if (ulError) {
            return  (ulError);
        }

#if LW_CFG_SMP_EN > 0
        if (ptcbCur->TCB_uiStatusChangeReq) {
            ptcbCur->TCB_uiStatusChangeReq = 0;
            _ErrorHandle(ERROR_THREAD_NULL);
            return  (ERROR_THREAD_NULL);
        }
#endif                                                                  /*  LW_CFG_SMP_EN               */

        __KERNEL_ENTER();                                               /*  进入内核                    */
        _ThreadSetAffinity(ptcb, stSize, pcpuset);                      /*  设置                        */
        _ThreadContinue(ptcb, LW_FALSE);                                /*  唤醒目标                    */
    }

    __KERNEL_EXIT();                                                    /*  退出内核                    */

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: API_ThreadGetAffinity
** 功能描述: 获取线程 CPU 亲和度情况
** 输　入  : ulId          线程
**           stSize        CPU 掩码集内存大小
**           pcpuset       CPU 掩码
** 输　出  : ERROR
** 全局变量: 
** 调用模块: 
** 注  意  : 如果获取的掩码全为 0 则所有 CPU 均可调度此任务.

                                           API 函数
*********************************************************************************************************/
LW_API  
ULONG  API_ThreadGetAffinity (LW_OBJECT_HANDLE  ulId, size_t  stSize, PLW_CLASS_CPUSET  pcpuset)
{
    REGISTER UINT16         usIndex;
    REGISTER PLW_CLASS_TCB  ptcb;

    usIndex = _ObjectGetIndex(ulId);

#if LW_CFG_ARG_CHK_EN > 0
    if (!_ObjectClassOK(ulId, _OBJECT_THREAD)) {                        /*  检查 ID 类型有效性          */
        _ErrorHandle(ERROR_KERNEL_HANDLE_NULL);
        return  (ERROR_KERNEL_HANDLE_NULL);
    }
    if (_Thread_Index_Invalid(usIndex)) {                               /*  检查线程有效性              */
        _ErrorHandle(ERROR_KERNEL_HANDLE_NULL);
        return  (ERROR_KERNEL_HANDLE_NULL);
    }
    if ((stSize < sizeof(LW_CLASS_CPUSET)) || !pcpuset) {
        _ErrorHandle(EINVAL);
        return  (EINVAL);
    }
#endif

    __KERNEL_ENTER();                                                   /*  进入内核                    */
    if (_Thread_Invalid(usIndex)) {
        __KERNEL_EXIT();                                                /*  退出内核                    */
        _ErrorHandle(ERROR_KERNEL_HANDLE_NULL);
        return  (ERROR_KERNEL_HANDLE_NULL);
    }
    
    ptcb = _K_ptcbTCBIdTable[usIndex];
    _ThreadGetAffinity(ptcb, stSize, pcpuset);
    __KERNEL_EXIT();                                                    /*  退出内核                    */
    
    return  (ERROR_NONE);
}

#endif                                                                  /*  LW_CFG_SMP_EN > 0           */
/*********************************************************************************************************
  END
*********************************************************************************************************/
