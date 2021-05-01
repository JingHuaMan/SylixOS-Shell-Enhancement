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
** ��   ��   ��: armSpinlock.c
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2013 �� 12 �� 09 ��
**
** ��        ��: ARM ��ϵ��������������.
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
** ��������: armSpinLock
** ��������: ARM spin lock
** �䡡��  : psld       spinlock data ָ��
**           pfuncPoll  ѭ���ȴ�ʱ���ú���
**           pvArg      �ص�����
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
** ע  ��  : ��������ʱ, ����ϵͳ������ڴ�����, �������ﲻ��Ҫ����.
*********************************************************************************************************/
static VOID  armSpinLock (SPINLOCKTYPE *psld, VOIDFUNCPTR  pfuncPoll, PVOID  pvArg)
{
#if __SYLIXOS_ARM_ARCH__ >= 6
    UINT32          uiTemp;
    UINT32          uiNewVal;
    SPINLOCKTYPE    sldVal;

    ARM_PREFETCH_W(&psld->SLD_uiLock);

    __asm__ __volatile__(
        "1: ldrex   %[oldvalue], [%[slock]]                 \n"
        "   add     %[newvalue], %[oldvalue], %[tshift]     \n"
        "   strex   %[temp],     %[newvalue], [%[slock]]    \n"
        "   teq     %[temp],     #0                         \n"
        "   bne     1b"
        : [oldvalue] "=&r" (sldVal), [newvalue] "=&r" (uiNewVal), [temp] "=&r" (uiTemp)
        : [slock] "r" (&psld->SLD_uiLock), [tshift] "I" (1 << LW_SPINLOCK_TICKET_SHIFT)
        : "cc");

    while (sldVal.SLD_usTicket != sldVal.SLD_usSvcNow) {
        if (pfuncPoll) {
            pfuncPoll(pvArg);
        } else {
            __asm__ __volatile__(ARM_WFE(""));
        }
        sldVal.SLD_usSvcNow = LW_ACCESS_ONCE(UINT16, psld->SLD_usSvcNow);

#if LW_CFG_ARM_ACCESS_ONCE_RMB > 0
        KN_SMP_RMB();
#endif
    }
#endif                                                                  /*  __SYLIXOS_ARM_ARCH__ >= 6   */
}
/*********************************************************************************************************
** ��������: armSpinTryLock
** ��������: ARM spin trylock
** �䡡��  : psld       spinlock data ָ��
** �䡡��  : 1: busy 0: ok
** ȫ�ֱ���:
** ����ģ��:
** ע  ��  : ��������ʱ, ����ϵͳ������ڴ�����, �������ﲻ��Ҫ����.
*********************************************************************************************************/
static UINT32  armSpinTryLock (SPINLOCKTYPE *psld)
{
#if __SYLIXOS_ARM_ARCH__ >= 6
    UINT32  uiCont, uiRes, uiLock;

    ARM_PREFETCH_W(&psld->SLD_uiLock);

    do {
        __asm__ __volatile__(
            "   ldrex   %[lock], [%[slock]]                 \n"
            "   mov     %[res],  #0                         \n"
            "   subs    %[cont], %[lock], %[lock], ror #16  \n"
            "   addeq   %[lock], %[lock], %[tshift]         \n"
            "   strexeq %[res],  %[lock], [%[slock]]"
            : [lock] "=&r" (uiLock), [cont] "=&r" (uiCont), [res] "=&r" (uiRes)
            : [slock] "r" (&psld->SLD_uiLock), [tshift] "I" (1 << LW_SPINLOCK_TICKET_SHIFT)
            : "cc");
    } while (uiRes);

    if (uiCont) {
        return  (1);
    }
#endif                                                                  /*  __SYLIXOS_ARM_ARCH__ >= 6   */

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
static VOID  armSpinUnlock (SPINLOCKTYPE *psld)
{
#if __SYLIXOS_ARM_ARCH__ >= 6
    psld->SLD_usSvcNow++;
    armDsb(ishst);
    __asm__ __volatile__(ARM_SEV);
#endif                                                                  /*  __SYLIXOS_ARM_ARCH__ >= 6   */
}
/*********************************************************************************************************
** ��������: armSpinLockDummy
** ��������: �ղ���
** �䡡��  : psl        spinlock ָ��
**           pfuncPoll  ѭ���ȴ�ʱ���ú���
**           pvArg      �ص�����
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static VOID  armSpinLockDummy (SPINLOCKTYPE  *psl, VOIDFUNCPTR  pfuncPoll, PVOID  pvArg)
{
}
/*********************************************************************************************************
** ��������: armSpinTryLockDummy
** ��������: �ղ���
** �䡡��  : psl        spinlock ָ��
** �䡡��  : 0
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static UINT32  armSpinTryLockDummy (SPINLOCKTYPE  *psl)
{
    return  (0);
}
/*********************************************************************************************************
** ��������: armSpinUnlockDummy
** ��������: �ղ���
** �䡡��  : psl        spinlock ָ��
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static VOID  armSpinUnlockDummy (SPINLOCKTYPE  *psl)
{
}
/*********************************************************************************************************
  spin lock cache ��������
*********************************************************************************************************/
static VOID    (*pfuncArmSpinLock)(SPINLOCKTYPE *, VOIDFUNCPTR, PVOID) = armSpinLock;
static UINT32  (*pfuncArmSpinTryLock)(SPINLOCKTYPE *)                  = armSpinTryLock;
static VOID    (*pfuncArmSpinUnlock)(SPINLOCKTYPE *)                   = armSpinUnlock;
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
    pfuncArmSpinLock    = armSpinLockDummy;
    pfuncArmSpinTryLock = armSpinTryLockDummy;
    pfuncArmSpinUnlock  = armSpinUnlockDummy;
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
    pfuncArmSpinUnlock  = armSpinUnlock;
    pfuncArmSpinTryLock = armSpinTryLock;
    pfuncArmSpinLock    = armSpinLock;
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
    
    pfuncArmSpinLock(&psl->SL_sltData, pfuncPoll, pvArg);
    
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
    
    if (pfuncArmSpinTryLock(&psl->SL_sltData)) {                        /*  ���Լ���                    */
        return  (0);
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
    
    pfuncArmSpinUnlock(&psl->SL_sltData);                               /*  ����                        */

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
    pfuncArmSpinLock(&psl->SL_sltData, LW_NULL, LW_NULL);

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
    if (pfuncArmSpinTryLock(&psl->SL_sltData)) {                        /*  ���Լ���                    */
        return  (0);
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
    pfuncArmSpinUnlock(&psl->SL_sltData);
    
    return  (1);
}

#endif                                                                  /*  LW_CFG_SMP_EN               */
/*********************************************************************************************************
  END
*********************************************************************************************************/
