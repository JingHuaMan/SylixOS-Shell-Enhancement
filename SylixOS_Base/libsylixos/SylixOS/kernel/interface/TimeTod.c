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
** 文   件   名: TimeTod.c
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2014 年 07 月 04 日
**
** 描        述: 系统 TOD 时间库.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
/*********************************************************************************************************
** 函数名称: API_TimeTodAdj
** 功能描述: 微调 TOD 时间.
** 输　入  : piDelta           TOD 调整时间 正数表示时间加速多少个对应的 ticks
**                                          负数表示时间减速多少个对应的 ticks
**                                          例如想让 TOD 时间加速一秒, 而且时钟 hz 为 100, 则此参数为  100
**                                              想让 TOD 时间减速一秒, 而且时钟 hz 为 100, 则此参数为 -100
**           piOldDelta        上次没有调整完的剩余调整值
** 输　出  : 
** 全局变量: 
** 调用模块: 
** 注  意  : 为了避免时光倒流, TOD 调回以前的时间系统将自动减速 TOD 的运算.

                                           API 函数
*********************************************************************************************************/
LW_API
VOID  API_TimeTodAdj (INT32  *piDelta, INT32 *piOldDelta)
{
    INTREG      iregInterLevel;

    LW_SPIN_KERN_TIME_LOCK_QUICK(&iregInterLevel);
    if (piOldDelta) {
        *piOldDelta = _K_iTODDelta;
    }
    if (piDelta) {
        _K_iTODDelta = *piDelta;
    }
    LW_SPIN_KERN_TIME_UNLOCK_QUICK(iregInterLevel);
}
/*********************************************************************************************************
** 函数名称: API_TimeTodAdjEx
** 功能描述: 微调 TOD 时间.
** 输　入  : piDelta           TOD 调整时间 正数表示时间加速多少个对应的 ticks
**                                          负数表示时间减速多少个对应的 ticks
**                                          例如想让 TOD 时间加速一秒, 而且时钟 hz 为 100, 则此参数为  100
**                                              想让 TOD 时间减速一秒, 而且时钟 hz 为 100, 则此参数为 -100
**           piDeltaNs         一个 tick 以内 ns 数的调整
**           piOldDelta        上次没有调整完的剩余调整值
**           piOldDeltaNs      一个 tick 以内 ns 数的调整
** 输　出  : 
** 全局变量: 
** 调用模块: 
** 注  意  : 为了避免时光倒流, TOD 调回以前的时间系统将自动减速 TOD 的运算.

                                           API 函数
*********************************************************************************************************/
LW_API
INT  API_TimeTodAdjEx (INT32  *piDelta, INT32  *piDeltaNs, INT32 *piOldDelta, INT32 *piOldDeltaNs)
{
    INTREG      iregInterLevel;
    
    if (piDeltaNs && (lib_abs(*piDeltaNs) > LW_NSEC_PER_TICK)) {
        _ErrorHandle(E2BIG);
        return  (PX_ERROR);
    }

    LW_SPIN_KERN_TIME_LOCK_QUICK(&iregInterLevel);
    if (piOldDelta) {
        *piOldDelta = _K_iTODDelta;
    }
    if (piOldDeltaNs) {
        *piOldDeltaNs = _K_iTODDeltaNs;
    }
    if (piDelta) {
        _K_iTODDelta = *piDelta;
    }
    if (piDeltaNs) {
        _K_iTODDeltaNs = *piDeltaNs;
    }
    LW_SPIN_KERN_TIME_UNLOCK_QUICK(iregInterLevel);
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
  END
*********************************************************************************************************/
