/*********************************************************************************************************
**
**                                    中国软件开源组织
**
**                                   嵌入式实时操作系统
**
**                                       SylixOS(TM)
**
**                               Copyright  All Rights Reserved
**
**--------------文件信息--------------------------------------------------------------------------------
**
** 文   件   名: k_scanlink.h
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2010 年 01 月 22 日
**
** 描        述: 这是系统基本扫描连(超时唤醒 & 看门狗链)。
*********************************************************************************************************/

#ifndef __K_SCANLINK_H
#define __K_SCANLINK_H

/*********************************************************************************************************
  获取唤醒队列第一个节点
*********************************************************************************************************/

#define __WAKEUP_GET_FIRST(pwu, pwun) \
        if ((pwu)->WU_plineHeader) { \
            (pwun) = _LIST_ENTRY((pwu)->WU_plineHeader, LW_CLASS_WAKEUP_NODE, WUN_lineManage); \
        } else { \
            (pwun) = LW_NULL; \
        }

/*********************************************************************************************************
  唤醒队列扫描
*********************************************************************************************************/

#define __WAKEUP_PASS_FIRST(pwu, pwun, ulCounter) \
        (pwu)->WU_plineOp = (pwu)->WU_plineHeader;  \
        while ((pwu)->WU_plineOp) {    \
            (pwun) = _LIST_ENTRY((pwu)->WU_plineOp, LW_CLASS_WAKEUP_NODE, WUN_lineManage);  \
            (pwu)->WU_plineOp = _list_line_get_next((pwu)->WU_plineOp); \
            if ((pwun)->WUN_ulCounter > ulCounter) {    \
                (pwun)->WUN_ulCounter -= ulCounter; \
                break;  \
            } else {    \
                ulCounter -= (pwun)->WUN_ulCounter; \
                (pwun)->WUN_ulCounter = 0;
                    
#define __WAKEUP_PASS_SECOND() \
            }
            
#define __WAKEUP_PASS_END() \
        }
        
/*********************************************************************************************************
  初始化
*********************************************************************************************************/

#define __WAKEUP_INIT(pwu, pfuncWakeup, pvArg)   \
        do {    \
            (pwu)->WU_plineHeader = LW_NULL;     \
            (pwu)->WU_plineOp     = LW_NULL;     \
            (pwu)->WU_pfuncWakeup = pfuncWakeup; \
            (pwu)->WU_pvWakeupArg = pvArg;       \
        } while (0)
        
#define __WAKEUP_NODE_INIT(pwun)    \
        do {    \
            _LIST_LINE_INIT_IN_CODE((pwun)->WUN_lineManage);    \
            (pwun)->WUN_bInQ = LW_FALSE; \
            (pwun)->WUN_ulCounter = 0ul;    \
        } while (0)
        
/*********************************************************************************************************
  将线程加入超时唤醒队列
*********************************************************************************************************/

#define __ADD_TO_WAKEUP_LINE(ptcb)                                  \
        do {                                                        \
            ptcb->TCB_usStatus |= LW_THREAD_STATUS_DELAY;           \
            _WakeupAdd(&_K_wuDelay, &ptcb->TCB_wunDelay, LW_FALSE); \
        } while (0)
        
/*********************************************************************************************************
  将线程从超时唤醒队列退出
*********************************************************************************************************/

#define __DEL_FROM_WAKEUP_LINE(ptcb)                                \
        do {                                                        \
            ptcb->TCB_usStatus &= ~LW_THREAD_STATUS_DELAY;          \
            _WakeupDel(&_K_wuDelay, &ptcb->TCB_wunDelay, LW_FALSE); \
        } while (0)
        
/*********************************************************************************************************
  将线程加入看门狗队列
*********************************************************************************************************/

#define __ADD_TO_WATCHDOG_LINE(ptcb)        _WakeupAdd(&_K_wuWatchDog, &ptcb->TCB_wunWatchDog, LW_FALSE)

/*********************************************************************************************************
  将线程从看门狗队列退出
*********************************************************************************************************/

#define __DEL_FROM_WATCHDOG_LINE(ptcb)      _WakeupDel(&_K_wuWatchDog, &ptcb->TCB_wunWatchDog, LW_FALSE)

#endif                                                                  /*  __K_SCANLINK_H              */
/*********************************************************************************************************
  END
*********************************************************************************************************/
