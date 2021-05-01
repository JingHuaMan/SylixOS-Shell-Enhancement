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
** 文   件   名: adjtime.c
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2014 年 07 月 04 日
**
** 描        述: posix adjtime 兼容库.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "unistd.h"
/*********************************************************************************************************
  裁剪支持
*********************************************************************************************************/
#if LW_CFG_POSIX_EN > 0
#include "time.h"
/*********************************************************************************************************
  调整范围
*********************************************************************************************************/
#define ADJTIME_DELTA_MAX       ((__ARCH_INT_MAX / 1000000) - 2)
#define ADJTIME_DELTA_MIN       ((__ARCH_INT_MIN / 1000000) + 2)
/*********************************************************************************************************
** 函数名称: adjtime
** 功能描述: 微调系统时间
** 输　入  : delta             系统时间修正参数 (详细说明参考 POSIX 手册)
**           olddelta          如果上次修正没有完成则返回上次修正剩余的时间
** 输　出  : ERROR or OK
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API 
int  adjtime (const struct timeval *delta, struct timeval *olddelta)
{
    INT32   iDelta, iDeltaNs;
    INT32   iOldDelta, iOldDeltaNs;
    
    if (geteuid()) {                                                    /*  只有 root 权限可以操作      */
        errno = EPERM;
        return  (PX_ERROR);
    }
    
    if (delta) {
        if ((delta->tv_sec < ADJTIME_DELTA_MIN) ||
            (delta->tv_sec > ADJTIME_DELTA_MAX)) {                      /*  调整时间是否超过范围        */
            errno = ENOTSUP;
            return  (PX_ERROR);
        }
        iDelta    = (INT32)delta->tv_sec * (INT32)LW_TICK_HZ;
        iDelta   += (INT32)((((delta->tv_usec * (INT32)LW_TICK_HZ) / 100) / 100) / 100);
        iDeltaNs  = (INT32)(delta->tv_usec % ((100 * 100 * 100) / (INT32)LW_TICK_HZ));
        iDeltaNs *= 1000;
        API_TimeTodAdjEx(&iDelta, &iDeltaNs, &iOldDelta, &iOldDeltaNs);
    
    } else {
        API_TimeTodAdjEx(LW_NULL, LW_NULL, &iOldDelta, &iOldDeltaNs);
    }
    
    if (olddelta) {
        olddelta->tv_sec   = (time_t)(iOldDelta / (INT32)LW_TICK_HZ);
        olddelta->tv_usec  = (LONG)(iOldDelta % (INT32)LW_TICK_HZ) * ((100 * 100 * 100)
                           / (INT32)LW_TICK_HZ);
        olddelta->tv_usec += iOldDeltaNs / 1000;
    }
    
    return  (ERROR_NONE);
}

#endif                                                                  /*  LW_CFG_POSIX_EN > 0         */
/*********************************************************************************************************
  END
*********************************************************************************************************/
