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
** 文   件   名: ThreadDetach.c
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2007 年 07 月 18 日
**
** 描        述: 禁止其他线程合并指定线程

** BUG
2007.07.18  加入了 _DebugHandle() 功能
2008.05.18  使用 __KERNEL_ENTER() 代替 ThreadLock();
2013.04.01  修正 GCC 4.7.3 引发的新 warning.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
/*********************************************************************************************************
  loader
*********************************************************************************************************/
#if LW_CFG_MODULELOADER_EN > 0
extern pid_t  vprocGetPidByTcbNoLock(PLW_CLASS_TCB  ptcb);
#endif                                                                  /*  LW_CFG_MODULELOADER_EN > 0  */
/*********************************************************************************************************
** 函数名称: API_ThreadDetachEx
** 功能描述: 禁止其他线程合并指定线程
** 输　入  :
**           ulId             线程句柄
**           pvRetVal         返回值
** 输　出  : ID
** 全局变量:
** 调用模块:
                                           API 函数

                                       (不得在中断中调用)
*********************************************************************************************************/
LW_API
ULONG  API_ThreadDetachEx (LW_OBJECT_HANDLE  ulId, PVOID  pvRetVal)
{
    REGISTER UINT16                usIndex;
#if LW_CFG_MODULELOADER_EN > 0
    REGISTER PLW_CLASS_TCB         ptcbCur;
#endif                                                                  /*  LW_CFG_MODULELOADER_EN > 0  */
    REGISTER PLW_CLASS_TCB         ptcb;
    REGISTER PLW_CLASS_WAITJOIN    ptwj;

    usIndex = _ObjectGetIndex(ulId);

    if (LW_CPU_GET_CUR_NESTING()) {                                     /*  不能在中断中调用            */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "called from ISR.\r\n");
        _ErrorHandle(ERROR_KERNEL_IN_ISR);
        return  (ERROR_KERNEL_IN_ISR);
    }

#if LW_CFG_MODULELOADER_EN > 0
    LW_TCB_GET_CUR_SAFE(ptcbCur);
#endif                                                                  /*  LW_CFG_MODULELOADER_EN > 0  */

#if LW_CFG_ARG_CHK_EN > 0
    if (!_ObjectClassOK(ulId, _OBJECT_THREAD)) {                        /*  检查 ID 类型有效性          */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "thread handle invalidate.\r\n");
        _ErrorHandle(ERROR_KERNEL_HANDLE_NULL);
        return  (ERROR_KERNEL_HANDLE_NULL);
    }

    if (_Thread_Index_Invalid(usIndex)) {                               /*  检查线程有效性              */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "thread handle invalidate.\r\n");
        _ErrorHandle(ERROR_THREAD_NULL);
        return  (ERROR_THREAD_NULL);
    }
#endif

    __KERNEL_ENTER();                                                   /*  进入内核                    */

    ptcb = _K_ptcbTCBIdTable[usIndex];
    if (ptcb) {
        if (ptcb->TCB_bDetachFlag) {
            __KERNEL_EXIT();                                            /*  退出内核                    */
            _ErrorHandle(ERROR_THREAD_DETACHED);
            return  (ERROR_THREAD_DETACHED);
        }

#if LW_CFG_MODULELOADER_EN > 0
        if (vprocGetPidByTcbNoLock(ptcb) !=
            vprocGetPidByTcbNoLock(ptcbCur)) {                          /*  只能 join 同进程线程        */
            __KERNEL_EXIT();                                            /*  退出内核                    */
            _ErrorHandle(ERROR_THREAD_NULL);
            return  (ERROR_THREAD_NULL);
        }
#endif                                                                  /*  LW_CFG_MODULELOADER_EN > 0  */

        _ThreadDetach(ptcb, LW_NULL, pvRetVal);                         /*  DETACH 处理                 */

    } else if (!LW_KERN_AUTO_REC_TCB_GET()) {                           /*  需要手动回收                */
        ptwj = &_K_twjTable[usIndex];
        if (ptwj->TWJ_ptcb) {
#if LW_CFG_MODULELOADER_EN > 0
            if (vprocGetPidByTcbNoLock(ptwj->TWJ_ptcb) !=
                vprocGetPidByTcbNoLock(ptcbCur)) {                      /*  只能 join 同进程线程        */
                __KERNEL_EXIT();                                        /*  退出内核                    */
                _ErrorHandle(ERROR_THREAD_NULL);
                return  (ERROR_THREAD_NULL);
            }
#endif                                                                  /*  LW_CFG_MODULELOADER_EN > 0  */
            _ThreadDetach(LW_NULL, ptwj, pvRetVal);                     /*  在等待回收队列中            */
        }

    } else {
        __KERNEL_EXIT();                                                /*  退出内核                    */
        _ErrorHandle(ERROR_THREAD_NULL);
        return  (ERROR_THREAD_NULL);
    }

    __KERNEL_EXIT();                                                    /*  退出内核                    */

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: API_ThreadDetach
** 功能描述: 禁止其他线程合并指定线程
** 输　入  :
**           ulId             线程句柄
** 输　出  : ID
** 全局变量:
** 调用模块:
                                           API 函数

                                       (不得在中断中调用)
*********************************************************************************************************/
LW_API
ULONG  API_ThreadDetach (LW_OBJECT_HANDLE  ulId)
{
    return  (API_ThreadDetachEx(ulId, LW_NULL));
}
/*********************************************************************************************************
  END
*********************************************************************************************************/
