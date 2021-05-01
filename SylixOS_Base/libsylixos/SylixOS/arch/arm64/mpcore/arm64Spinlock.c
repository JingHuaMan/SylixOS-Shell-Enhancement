/*********************************************************************************************************
**
**                                    �й������Դ��֯
**
**                                   Ƕ��ʽʵʱ����ϵͳ
**
**                                       SylixOS(TM)
**
**                               Copyright  All Rights Reserved
**
**--------------�ļ���Ϣ--------------------------------------------------------------------------------
**
** ��   ��   ��: arm64Spinlock.c
**
** ��   ��   ��: Wang.Xuan (���Q)
**
** �ļ���������: 2018 �� 06 �� 29 ��
**
** ��        ��: ARM64 ��ϵ��������������.
*********************************************************************************************************/
#define  __SYLIXOS_SMPFMB
#define  __SYLIXOS_KERNEL
#include "SylixOS.h"
/*********************************************************************************************************
  spinlock ״̬
*********************************************************************************************************/
#if LW_CFG_SMP_EN > 0
/*********************************************************************************************************
  L1 cache ͬ����ο�: http://www.cnblogs.com/jiayy/p/3246133.html
*********************************************************************************************************/
#ifdef __LW_SPINLOCK_BUG_TRACE_EN
#define __LW_SPINLOCK_RECURSIVE_TRACE() \
        _BugFormat((psl->SL_ulCounter > 10), LW_TRUE, \
                   "spinlock RECURSIVE %lu!\r\n", psl->SL_ulCounter)
#else
#define __LW_SPINLOCK_RECURSIVE_TRACE()
#endif
/*********************************************************************************************************
** ��������: arm64SpinLock
** ��������: ARM64 spin lock
** �䡡��  : psld       spinlock data ָ��
**           pfuncPoll  ѭ���ȴ�ʱ���ú���
**           pvArg      �ص�����
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
** ע  ��  : ��������ʱ, ����ϵͳ������ڴ�����, �������ﲻ��Ҫ����.
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
** ��������: arm64SpinTryLock
** ��������: ARM64 spin trylock
** �䡡��  : psld       spinlock data ָ��
** �䡡��  : 1: busy 0: ok
** ȫ�ֱ���:
** ����ģ��:
** ע  ��  : ��������ʱ, ����ϵͳ������ڴ�����, �������ﲻ��Ҫ����.
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
** ��������: armSpinUnlock
** ��������: ARM spin unlock
** �䡡��  : psld       spinlock data ָ��
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static VOID  arm64SpinUnlock (SPINLOCKTYPE  *psld)
{
    psld->SLD_usSvcNow++;
    armDsb(ishst);
    ARM_SEV();
}
/*********************************************************************************************************
** ��������: armSpinLockDummy
** ��������: �ղ���
** �䡡��  : psl        spinlock ָ��
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static VOID  arm64SpinLockDummy (SPINLOCKTYPE  *psl, VOIDFUNCPTR  pfuncPoll, PVOID  pvArg)
{
}
/*********************************************************************************************************
** ��������: arm64SpinTryLockDummy
** ��������: �ղ���
** �䡡��  : psl        spinlock ָ��
** �䡡��  : 0
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static UINT32  arm64SpinTryLockDummy (SPINLOCKTYPE  *psl)
{
    return  (0);
}
/*********************************************************************************************************
** ��������: arm64SpinUnlockDummy
** ��������: �ղ���
** �䡡��  : psl        spinlock ָ��
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static VOID  arm64SpinUnlockDummy (SPINLOCKTYPE  *psl)
{
}
/*********************************************************************************************************
  spin lock cache ��������
*********************************************************************************************************/
static VOID             (*pfuncArm64SpinLock)(SPINLOCKTYPE *, VOIDFUNCPTR, PVOID) = arm64SpinLockDummy;
static volatile UINT32  (*pfuncArm64SpinTryLock)(SPINLOCKTYPE *)                  = arm64SpinTryLockDummy;
static VOID             (*pfuncArm64SpinUnlock)(SPINLOCKTYPE *)                   = arm64SpinUnlockDummy;
/*********************************************************************************************************
** ��������: archSpinBypass
** ��������: spinlock ��������Ч
** �䡡��  : NONE
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
VOID  archSpinBypass (VOID)
{
    pfuncArm64SpinLock    = arm64SpinLockDummy;
    pfuncArm64SpinTryLock = arm64SpinTryLockDummy;
    pfuncArm64SpinUnlock  = arm64SpinUnlockDummy;
}
/*********************************************************************************************************
** ��������: archSpinWork
** ��������: spinlock ������Ч
** �䡡��  : NONE
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
** ע  ��  : ���˿��� CACHE ��, BSP Ӧ�������ô˺���, ʹ spinlock ��Ч,
             �Ӻ����������� CACHE ������, ���ò��� spinlock.
*********************************************************************************************************/
VOID  archSpinWork (VOID)
{
    pfuncArm64SpinUnlock  = arm64SpinUnlock;
    pfuncArm64SpinTryLock = arm64SpinTryLock;
    pfuncArm64SpinLock    = arm64SpinLock;
}
/*********************************************************************************************************
** ��������: archSpinInit
** ��������: ��ʼ��һ�� spinlock
** �䡡��  : psl        spinlock ָ��
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
VOID  archSpinInit (spinlock_t  *psl)
{
    psl->SL_sltData.SLD_uiLock = 0;                                     /*  0: δ����״̬  1: ����״̬  */
    psl->SL_pcpuOwner          = LW_NULL;
    psl->SL_ulCounter          = 0;
    psl->SL_pvReserved         = LW_NULL;
    KN_SMP_WMB();
}
/*********************************************************************************************************
** ��������: archSpinDelay
** ��������: �ȴ��¼�
** �䡡��  : NONE
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
VOID  archSpinDelay (VOID)
{
    volatile INT  i;

    for (i = 0; i < 3; i++) {
    }
}
/*********************************************************************************************************
** ��������: archSpinNotify
** ��������: ���� spin �¼�
** �䡡��  : NONE
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
VOID  archSpinNotify (VOID)
{
}
/*********************************************************************************************************
** ��������: archSpinLock
** ��������: spinlock ����
** �䡡��  : psl        spinlock ָ��
**           pcpuCur    ��ǰ CPU
**           pfuncPoll  ѭ���ȴ�ʱ���ú���
**           pvArg      �ص�����
** �䡡��  : 0: û�л�ȡ
**           1: ��������
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
INT  archSpinLock (spinlock_t  *psl, PLW_CLASS_CPU  pcpuCur, VOIDFUNCPTR  pfuncPoll, PVOID  pvArg)
{
    if (psl->SL_pcpuOwner == pcpuCur) {
        psl->SL_ulCounter++;
        __LW_SPINLOCK_RECURSIVE_TRACE();
        return  (1);                                                    /*  �ظ�����                    */
    }
    
    pfuncArm64SpinLock(&psl->SL_sltData, pfuncPoll, pvArg);
    
    psl->SL_pcpuOwner = pcpuCur;                                        /*  ���浱ǰ CPU                */

    return  (1);                                                        /*  �����ɹ�                    */
}
/*********************************************************************************************************
** ��������: archSpinTryLock
** ��������: spinlock ��ͼ����
** �䡡��  : psl        spinlock ָ��
**           pcpuCur    ��ǰ CPU
** �䡡��  : 0: û�л�ȡ
**           1: ��������
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
INT  archSpinTryLock (spinlock_t  *psl, PLW_CLASS_CPU  pcpuCur)
{
    if (psl->SL_pcpuOwner == pcpuCur) {
        psl->SL_ulCounter++;
        __LW_SPINLOCK_RECURSIVE_TRACE();
        return  (1);                                                    /*  �ظ�����                    */
    }
    
    if (pfuncArm64SpinTryLock(&psl->SL_sltData)) {
        return  (0);                                                    /*  ���Լ���                    */
    }

    psl->SL_pcpuOwner = pcpuCur;                                        /*  ���浱ǰ CPU                */

    return  (1);
}
/*********************************************************************************************************
** ��������: archSpinUnlock
** ��������: spinlock ����
** �䡡��  : psl        spinlock ָ��
**           pcpuCur    ��ǰ CPU
** �䡡��  : 0: û�л�ȡ
**           1: ��������
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
INT  archSpinUnlock (spinlock_t  *psl, PLW_CLASS_CPU  pcpuCur)
{
    if (psl->SL_pcpuOwner != pcpuCur) {
        return  (0);                                                    /*  û��Ȩ���ͷ�                */
    }
    
    if (psl->SL_ulCounter) {
        psl->SL_ulCounter--;                                            /*  �����ظ����ô���            */
        return  (1);
    }

    psl->SL_pcpuOwner = LW_NULL;                                        /*  û�� CPU ��ȡ               */
    KN_SMP_WMB();

    pfuncArm64SpinUnlock(&psl->SL_sltData);                             /*  ����                        */

    return  (1);
}
/*********************************************************************************************************
** ��������: archSpinLockRaw
** ��������: spinlock ���� (�����������ж�)
** �䡡��  : psl        spinlock ָ��
** �䡡��  : 0: û�л�ȡ
**           1: ��������
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
INT  archSpinLockRaw (spinlock_t  *psl)
{
    pfuncArm64SpinLock(&psl->SL_sltData, LW_NULL, LW_NULL);

    return  (1);                                                        /*  �����ɹ�                    */
}
/*********************************************************************************************************
** ��������: archSpinTryLockRaw
** ��������: spinlock ��ͼ���� (�����������ж�)
** �䡡��  : psl        spinlock ָ��
** �䡡��  : 0: û�л�ȡ
**           1: ��������
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
INT  archSpinTryLockRaw (spinlock_t  *psl)
{
    if (pfuncArm64SpinTryLock(&psl->SL_sltData)) {
        return  (0);                                                    /*  ���Լ���                    */
    }
    
    return  (1);                                                        /*  �����ɹ�                    */
}
/*********************************************************************************************************
** ��������: archSpinUnlockRaw
** ��������: spinlock ����
** �䡡��  : psl        spinlock ָ��
** �䡡��  : 0: û�л�ȡ
**           1: ��������
** ȫ�ֱ���: 
** ����ģ��: 
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
