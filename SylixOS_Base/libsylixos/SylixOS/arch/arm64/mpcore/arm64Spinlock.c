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
** 文   件   名: arm64Spinlock.c
**
** 创   建   人: Wang.Xuan (王翾)
**
** 文件创建日期: 2018 年 06 月 29 日
**
** 描        述: ARM64 体系构架自旋锁驱动.
*********************************************************************************************************/
#define  __SYLIXOS_SMPFMB
#define  __SYLIXOS_KERNEL
#include "SylixOS.h"
/*********************************************************************************************************
  spinlock 状态
*********************************************************************************************************/
#if LW_CFG_SMP_EN > 0
/*********************************************************************************************************
  L1 cache 同步请参考: http://www.cnblogs.com/jiayy/p/3246133.html
*********************************************************************************************************/
#ifdef __LW_SPINLOCK_BUG_TRACE_EN
#define __LW_SPINLOCK_RECURSIVE_TRACE() \
        _BugFormat((psl->SL_ulCounter > 10), LW_TRUE, \
                   "spinlock RECURSIVE %lu!\r\n", psl->SL_ulCounter)
#else
#define __LW_SPINLOCK_RECURSIVE_TRACE()
#endif
/*********************************************************************************************************
** 函数名称: arm64SpinLock
** 功能描述: ARM64 spin lock
** 输　入  : psld       spinlock data 指针
**           pfuncPoll  循环等待时调用函数
**           pvArg      回调参数
** 输　出  : NONE
** 全局变量:
** 调用模块:
** 注  意  : 自旋结束时, 操作系统会调用内存屏障, 所以这里不需要调用.
*********************************************************************************************************/
static VOID  arm64SpinLock (SPINLOCKTYPE  *psld, VOIDFUNCPTR  pfuncPoll, PVOID  pvArg)
{
    SPINLOCKTYPE    sldVal;
    UINT32          uiNewVal;
    
    ARM_PREFETCH_W(&psld->SLD_uiLock);

    __asm__ __volatile__(
        "1: ldaxr    %w0,      %2   \n"
        "   add      %w1, %w0, %3   \n"
        "   stxr     %w1, %w1, %2   \n"
        "   cbnz     %w1,      1b"
        : "=&r" (sldVal), "=&r" (uiNewVal),  "+Q" (psld->SLD_uiLock)
        : "I" (1 << LW_SPINLOCK_TICKET_SHIFT)
        : "memory");

    while (sldVal.SLD_usTicket != sldVal.SLD_usSvcNow) {
        if (pfuncPoll) {
            pfuncPoll(pvArg);
        } else {
            ARM_WFE();
        }
        sldVal.SLD_usSvcNow = LW_ACCESS_ONCE(UINT16, psld->SLD_usSvcNow);

#if LW_CFG_ARM64_ACCESS_ONCE_RMB > 0
        KN_SMP_RMB();
#endif
    }
}
/*********************************************************************************************************
** 函数名称: arm64SpinTryLock
** 功能描述: ARM64 spin trylock
** 输　入  : psld       spinlock data 指针
** 输　出  : 1: busy 0: ok
** 全局变量:
** 调用模块:
** 注  意  : 自旋结束时, 操作系统会调用内存屏障, 所以这里不需要调用.
*********************************************************************************************************/
static UINT32  arm64SpinTryLock (SPINLOCKTYPE  *psld)
{
    UINT32    uiLock;
    UINT32    uiRes;

    ARM_PREFETCH_W(&psld->SLD_uiLock);

    __asm__ __volatile__(
        "1: ldaxr    %w0, %2                    \n"
        "   eor      %w1, %w0, %w0, ror #16     \n"
        "   cbnz     %w1, 2f                    \n"
        "   add      %w0, %w0, %3               \n"
        "   stxr     %w1, %w0, %2               \n"
        "   cbnz     %w1, 1b                    \n"
        "2:"
        : "=&r" (uiLock), "=&r" (uiRes),  "+Q" (psld->SLD_uiLock)
        : "I" (1 << LW_SPINLOCK_TICKET_SHIFT)
        : "memory");

    if (uiRes) {
        return  (1);
    }

    return  (0);
}
/*********************************************************************************************************
** 函数名称: armSpinUnlock
** 功能描述: ARM spin unlock
** 输　入  : psld       spinlock data 指针
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static VOID  arm64SpinUnlock (SPINLOCKTYPE  *psld)
{
    psld->SLD_usSvcNow++;
    armDsb(ishst);
    ARM_SEV();
}
/*********************************************************************************************************
** 函数名称: armSpinLockDummy
** 功能描述: 空操作
** 输　入  : psl        spinlock 指针
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static VOID  arm64SpinLockDummy (SPINLOCKTYPE  *psl, VOIDFUNCPTR  pfuncPoll, PVOID  pvArg)
{
}
/*********************************************************************************************************
** 函数名称: arm64SpinTryLockDummy
** 功能描述: 空操作
** 输　入  : psl        spinlock 指针
** 输　出  : 0
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static UINT32  arm64SpinTryLockDummy (SPINLOCKTYPE  *psl)
{
    return  (0);
}
/*********************************************************************************************************
** 函数名称: arm64SpinUnlockDummy
** 功能描述: 空操作
** 输　入  : psl        spinlock 指针
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static VOID  arm64SpinUnlockDummy (SPINLOCKTYPE  *psl)
{
}
/*********************************************************************************************************
  spin lock cache 依赖处理
*********************************************************************************************************/
static VOID             (*pfuncArm64SpinLock)(SPINLOCKTYPE *, VOIDFUNCPTR, PVOID) = arm64SpinLockDummy;
static volatile UINT32  (*pfuncArm64SpinTryLock)(SPINLOCKTYPE *)                  = arm64SpinTryLockDummy;
static VOID             (*pfuncArm64SpinUnlock)(SPINLOCKTYPE *)                   = arm64SpinUnlockDummy;
/*********************************************************************************************************
** 函数名称: archSpinBypass
** 功能描述: spinlock 函数不起效
** 输　入  : NONE
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
VOID  archSpinBypass (VOID)
{
    pfuncArm64SpinLock    = arm64SpinLockDummy;
    pfuncArm64SpinTryLock = arm64SpinTryLockDummy;
    pfuncArm64SpinUnlock  = arm64SpinUnlockDummy;
}
/*********************************************************************************************************
** 函数名称: archSpinWork
** 功能描述: spinlock 函数起效
** 输　入  : NONE
** 输　出  : NONE
** 全局变量:
** 调用模块:
** 注  意  : 主核开启 CACHE 后, BSP 应立即调用此函数, 使 spinlock 生效,
             从核启动到开启 CACHE 过程中, 不得操作 spinlock.
*********************************************************************************************************/
VOID  archSpinWork (VOID)
{
    pfuncArm64SpinUnlock  = arm64SpinUnlock;
    pfuncArm64SpinTryLock = arm64SpinTryLock;
    pfuncArm64SpinLock    = arm64SpinLock;
}
/*********************************************************************************************************
** 函数名称: archSpinInit
** 功能描述: 初始化一个 spinlock
** 输　入  : psl        spinlock 指针
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
VOID  archSpinInit (spinlock_t  *psl)
{
    psl->SL_sltData.SLD_uiLock = 0;                                     /*  0: 未锁定状态  1: 锁定状态  */
    psl->SL_pcpuOwner          = LW_NULL;
    psl->SL_ulCounter          = 0;
    psl->SL_pvReserved         = LW_NULL;
    KN_SMP_WMB();
}
/*********************************************************************************************************
** 函数名称: archSpinDelay
** 功能描述: 等待事件
** 输　入  : NONE
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
VOID  archSpinDelay (VOID)
{
    volatile INT  i;

    for (i = 0; i < 3; i++) {
    }
}
/*********************************************************************************************************
** 函数名称: archSpinNotify
** 功能描述: 发送 spin 事件
** 输　入  : NONE
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
VOID  archSpinNotify (VOID)
{
}
/*********************************************************************************************************
** 函数名称: archSpinLock
** 功能描述: spinlock 上锁
** 输　入  : psl        spinlock 指针
**           pcpuCur    当前 CPU
**           pfuncPoll  循环等待时调用函数
**           pvArg      回调参数
** 输　出  : 0: 没有获取
**           1: 正常加锁
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
INT  archSpinLock (spinlock_t  *psl, PLW_CLASS_CPU  pcpuCur, VOIDFUNCPTR  pfuncPoll, PVOID  pvArg)
{
    if (psl->SL_pcpuOwner == pcpuCur) {
        psl->SL_ulCounter++;
        __LW_SPINLOCK_RECURSIVE_TRACE();
        return  (1);                                                    /*  重复调用                    */
    }
    
    pfuncArm64SpinLock(&psl->SL_sltData, pfuncPoll, pvArg);
    
    psl->SL_pcpuOwner = pcpuCur;                                        /*  保存当前 CPU                */

    return  (1);                                                        /*  加锁成功                    */
}
/*********************************************************************************************************
** 函数名称: archSpinTryLock
** 功能描述: spinlock 试图上锁
** 输　入  : psl        spinlock 指针
**           pcpuCur    当前 CPU
** 输　出  : 0: 没有获取
**           1: 正常加锁
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
INT  archSpinTryLock (spinlock_t  *psl, PLW_CLASS_CPU  pcpuCur)
{
    if (psl->SL_pcpuOwner == pcpuCur) {
        psl->SL_ulCounter++;
        __LW_SPINLOCK_RECURSIVE_TRACE();
        return  (1);                                                    /*  重复调用                    */
    }
    
    if (pfuncArm64SpinTryLock(&psl->SL_sltData)) {
        return  (0);                                                    /*  尝试加锁                    */
    }

    psl->SL_pcpuOwner = pcpuCur;                                        /*  保存当前 CPU                */

    return  (1);
}
/*********************************************************************************************************
** 函数名称: archSpinUnlock
** 功能描述: spinlock 解锁
** 输　入  : psl        spinlock 指针
**           pcpuCur    当前 CPU
** 输　出  : 0: 没有获取
**           1: 正常解锁
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
INT  archSpinUnlock (spinlock_t  *psl, PLW_CLASS_CPU  pcpuCur)
{
    if (psl->SL_pcpuOwner != pcpuCur) {
        return  (0);                                                    /*  没有权利释放                */
    }
    
    if (psl->SL_ulCounter) {
        psl->SL_ulCounter--;                                            /*  减少重复调用次数            */
        return  (1);
    }

    psl->SL_pcpuOwner = LW_NULL;                                        /*  没有 CPU 获取               */
    KN_SMP_WMB();

    pfuncArm64SpinUnlock(&psl->SL_sltData);                             /*  解锁                        */

    return  (1);
}
/*********************************************************************************************************
** 函数名称: archSpinLockRaw
** 功能描述: spinlock 上锁 (不进行重入判断)
** 输　入  : psl        spinlock 指针
** 输　出  : 0: 没有获取
**           1: 正常加锁
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
INT  archSpinLockRaw (spinlock_t  *psl)
{
    pfuncArm64SpinLock(&psl->SL_sltData, LW_NULL, LW_NULL);

    return  (1);                                                        /*  加锁成功                    */
}
/*********************************************************************************************************
** 函数名称: archSpinTryLockRaw
** 功能描述: spinlock 试图上锁 (不进行重入判断)
** 输　入  : psl        spinlock 指针
** 输　出  : 0: 没有获取
**           1: 正常加锁
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
INT  archSpinTryLockRaw (spinlock_t  *psl)
{
    if (pfuncArm64SpinTryLock(&psl->SL_sltData)) {
        return  (0);                                                    /*  尝试加锁                    */
    }
    
    return  (1);                                                        /*  加锁成功                    */
}
/*********************************************************************************************************
** 函数名称: archSpinUnlockRaw
** 功能描述: spinlock 解锁
** 输　入  : psl        spinlock 指针
** 输　出  : 0: 没有获取
**           1: 正常解锁
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
INT  archSpinUnlockRaw (spinlock_t  *psl)
{
    pfuncArm64SpinUnlock(&psl->SL_sltData);
    
    return  (1);
}

#endif                                                                  /*  LW_CFG_SMP_EN               */
/*********************************************************************************************************
  END
*********************************************************************************************************/
