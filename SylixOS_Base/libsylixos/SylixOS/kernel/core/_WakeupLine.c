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
** 文   件   名: _WakeupLine.c
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2008 年 03 月 29 日
**
** 描        述: 这是系统等待唤醒链表操作函数 (调用以下函数时, 一定要在内核状态)

** BUG:
2010.01.22  昨晚观看了 3D 版 AVATAR. 和 TITANIC 一样经典!
            将队列改为 FIFO 形式 (由于历史原因, 暂不改为环表)
2013.09.03  使用新的差分时间链表唤醒队列机制.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
/*********************************************************************************************************
** 函数名称: _WakeupAdd
** 功能描述: 将一个 wakeup 节点加入管理器
** 输　入  : pwu           wakeup 管理器
**           pwun          节点
**           bProcTime     是否尝试处理非周期任务时间. (LW_TRUE: 时必须在进入内核模式调用)
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
VOID  _WakeupAdd (PLW_CLASS_WAKEUP  pwu, PLW_CLASS_WAKEUP_NODE  pwun, BOOL  bProcTime)
{
    PLW_LIST_LINE           plineTemp = pwu->WU_plineHeader;
    PLW_CLASS_WAKEUP_NODE   pwunTemp  = LW_NULL;
    INT64                   i64CurTime;
    ULONG                   ulCounter;
    BOOL                    bSaveTime = LW_FALSE;

    if (bProcTime && plineTemp && pwu->WU_pfuncWakeup) {                /*  非周期任务时间预处理        */
        __KERNEL_TIME_GET(i64CurTime, INT64);
        ulCounter = (ULONG)(i64CurTime - pwu->WU_i64LastTime);
        pwu->WU_i64LastTime = i64CurTime;

        pwunTemp = _LIST_ENTRY(plineTemp, LW_CLASS_WAKEUP_NODE, WUN_lineManage);
        if (pwunTemp->WUN_ulCounter > ulCounter) {
            pwunTemp->WUN_ulCounter -= ulCounter;
        } else {
            pwunTemp->WUN_ulCounter = 0;
        }
    }

    while (plineTemp) {
        pwunTemp = _LIST_ENTRY(plineTemp, LW_CLASS_WAKEUP_NODE, WUN_lineManage);
        if (pwun->WUN_ulCounter >= pwunTemp->WUN_ulCounter) {           /*  需要继续向后找              */
            pwun->WUN_ulCounter -= pwunTemp->WUN_ulCounter;
            plineTemp = _list_line_get_next(plineTemp);
        
        } else {
            if (plineTemp == pwu->WU_plineHeader) {                     /*  如果是链表头                */
                _List_Line_Add_Ahead(&pwun->WUN_lineManage, &pwu->WU_plineHeader);
            } else {
                _List_Line_Add_Left(&pwun->WUN_lineManage, plineTemp);  /*  不是表头则插在左边          */
            }
            pwunTemp->WUN_ulCounter -= pwun->WUN_ulCounter;             /*  右侧的点从新就算计数器      */
            break;
        }
    }
    
    if (plineTemp == LW_NULL) {
        if (pwu->WU_plineHeader == LW_NULL) {
            _List_Line_Add_Ahead(&pwun->WUN_lineManage, &pwu->WU_plineHeader);
            bSaveTime = LW_TRUE;
        } else {
            _List_Line_Add_Right(&pwun->WUN_lineManage, &pwunTemp->WUN_lineManage);
        }
    }
    
    pwun->WUN_bInQ = LW_TRUE;

    if (bProcTime && pwu->WU_pfuncWakeup) {
        if (bSaveTime) {
            __KERNEL_TIME_GET(pwu->WU_i64LastTime, INT64);
        }
        pwu->WU_pfuncWakeup(pwu->WU_pvWakeupArg);                       /*  唤醒                        */
    }
}
/*********************************************************************************************************
** 函数名称: _WakeupDel
** 功能描述: 从 wakeup 管理器中删除指定节点
** 输　入  : pwu           wakeup 管理器
**           pwun          节点
**           bProcTime     是否尝试处理非周期任务时间. (LW_TRUE: 时必须在进入内核模式调用)
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
VOID  _WakeupDel (PLW_CLASS_WAKEUP  pwu, PLW_CLASS_WAKEUP_NODE  pwun, BOOL  bProcTime)
{
    PLW_LIST_LINE           plineRight;
    PLW_CLASS_WAKEUP_NODE   pwunRight;
    INT64                   i64CurTime;
    ULONG                   ulCounter;

    if (&pwun->WUN_lineManage == pwu->WU_plineOp) {
        pwu->WU_plineOp = _list_line_get_next(pwu->WU_plineOp);
    }
    
    plineRight = _list_line_get_next(&pwun->WUN_lineManage);
    if (plineRight) {
        pwunRight = _LIST_ENTRY(plineRight, LW_CLASS_WAKEUP_NODE, WUN_lineManage);
        pwunRight->WUN_ulCounter += pwun->WUN_ulCounter;
    }
    
    if (bProcTime && !_list_line_get_prev(&pwun->WUN_lineManage) && pwu->WU_pfuncWakeup) {
        __KERNEL_TIME_GET(i64CurTime, INT64);                           /*  非周期任务时间预处理        */
        ulCounter = (ULONG)(i64CurTime - pwu->WU_i64LastTime);
        pwu->WU_i64LastTime = i64CurTime;

        if (plineRight) {
            if (pwunRight->WUN_ulCounter > ulCounter) {
                pwunRight->WUN_ulCounter -= ulCounter;
            } else {
                pwunRight->WUN_ulCounter = 0;
            }
        }
    }

    _List_Line_Del(&pwun->WUN_lineManage, &pwu->WU_plineHeader);
    pwun->WUN_bInQ = LW_FALSE;
}
/*********************************************************************************************************
** 函数名称: _WakeupStatus
** 功能描述: 获得指定节点等待信息
** 输　入  : pwu           wakeup 管理器
**           pwun          节点
**           pulLeft       剩余时间
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
VOID  _WakeupStatus (PLW_CLASS_WAKEUP  pwu, PLW_CLASS_WAKEUP_NODE  pwun, ULONG  *pulLeft)
{
    PLW_LIST_LINE           plineTemp;
    PLW_CLASS_WAKEUP_NODE   pwunTemp;
    INT64                   i64CurTime;
    ULONG                   ulDelta, ulCounter = 0;
    
    for (plineTemp  = pwu->WU_plineHeader;
         plineTemp != LW_NULL;
         plineTemp  = _list_line_get_next(plineTemp)) {
        
        pwunTemp   = _LIST_ENTRY(plineTemp, LW_CLASS_WAKEUP_NODE, WUN_lineManage);
        ulCounter += pwunTemp->WUN_ulCounter;
        if (pwunTemp == pwun) {
            break;
        }
    }
    
    if (plineTemp) {
        if (pwu->WU_i64LastTime) {                                      /*  包含时间预处理              */
            __KERNEL_TIME_GET(i64CurTime, INT64);
            ulDelta   = (ULONG)(i64CurTime - pwu->WU_i64LastTime);
            ulCounter = (ulCounter > ulDelta) ? (ulCounter - ulDelta) : 0ul;
        }
        *pulLeft  = ulCounter;
    
    } else {
        *pulLeft = 0ul;                                                 /*  没有找到节点                */
    }
}
/*********************************************************************************************************
  END
*********************************************************************************************************/
