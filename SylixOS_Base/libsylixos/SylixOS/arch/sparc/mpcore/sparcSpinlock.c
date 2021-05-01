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
** ��   ��   ��: sparcSpinlock.c
**
** ��   ��   ��: Xu.Guizhou (�����)
**
** �ļ���������: 2017 �� 05 �� 15 ��
**
** ��        ��: SPARC ��ϵ��������������.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "SylixOS.h"
/*********************************************************************************************************
  spinlock ״̬
*********************************************************************************************************/
#if LW_CFG_SMP_EN > 0
/*********************************************************************************************************
  spinlock bug trace
*********************************************************************************************************/
#ifdef __LW_SPINLOCK_BUG_TRACE_EN
#define __LW_SPINLOCK_RECURSIVE_TRACE() \
        _BugFormat((psl->SL_ulCounter > 10), LW_TRUE, \
                   "spinlock RECURSIVE %lu!\r\n", psl->SL_ulCounter)
#else
#define __LW_SPINLOCK_RECURSIVE_TRACE()
#endif
/*********************************************************************************************************
** ��������: sparcSpinLock
** ��������: sparc spin lock
** �䡡��  : psld       spinlock data ָ��
**           pfuncPoll  ѭ���ȴ�ʱ���ú���
**           pvArg      �ص�����
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
** ע  ��  : ��������ʱ, ����ϵͳ������ڴ�����, �������ﲻ��Ҫ����.
*********************************************************************************************************/
static LW_INLINE VOID  sparcSpinLock (SPINLOCKTYPE *psld, VOIDFUNCPTR  pfuncPoll, PVOID  pvArg)
{
#if LW_CFG_CPU_ATOMIC_EN > 0
    UINT32          uiNewVal;
    SPINLOCKTYPE    sldVal;

    for (;;) {
        sldVal.SLD_uiLock = LW_ACCESS_ONCE(UINT32, psld->SLD_uiLock);
        uiNewVal          = sldVal.SLD_uiLock + (1 << LW_SPINLOCK_TICKET_SHIFT);
        __asm__ __volatile__ ("casa     [%2] 0xb, %3, %0"
                             : "=&r" (uiNewVal)
                             : "0" (uiNewVal), "r" (&psld->SLD_uiLock), "r" (sldVal)
                             : "memory");
        if (uiNewVal == sldVal.SLD_uiLock) {
            break;
        }
    }

    while (sldVal.SLD_usTicket != sldVal.SLD_usSvcNow) {
        if (pfuncPoll) {
            pfuncPoll(pvArg);
        }
        sldVal.SLD_usSvcNow = LW_ACCESS_ONCE(UINT16, psld->SLD_usSvcNow);
    }

#else
    UINT32  uiRes;

    for (;;) {
        __asm__ __volatile__("ldstub    [%1], %0"
                             : "=r" (uiRes)
                             : "r" (psld)
                             : "memory");
        if (uiRes == 0) {
            break;
        }
        if (pfuncPoll) {
            pfuncPoll(pvArg);
        }
    }
#endif                                                                  /*  LW_CFG_CPU_ATOMIC_EN > 0    */
}
/*********************************************************************************************************
** ��������: sparcSpinTryLock
** ��������: sparc spin trylock
** �䡡��  : psld       spinlock data ָ��
** �䡡��  : 1: busy 0: ok
** ȫ�ֱ���:
** ����ģ��:
** ע  ��  : ��������ʱ, ����ϵͳ������ڴ�����, �������ﲻ��Ҫ����.
*********************************************************************************************************/
static LW_INLINE UINT32  sparcSpinTryLock (SPINLOCKTYPE *psld)
{
#if LW_CFG_CPU_ATOMIC_EN > 0
    UINT32          uiNewVal;
    SPINLOCKTYPE    sldVal;

    if (psld->SLD_usTicket == psld->SLD_usSvcNow) {
        sldVal.SLD_uiLock = LW_ACCESS_ONCE(UINT32, psld->SLD_uiLock);
        uiNewVal          = sldVal.SLD_uiLock + (1 << LW_SPINLOCK_TICKET_SHIFT);
        __asm__ __volatile__ ("casa     [%2] 0xb, %3, %0"
                             : "=&r" (uiNewVal)
                             : "0" (uiNewVal), "r" (&psld->SLD_uiLock), "r" (sldVal)
                             : "memory");
        return  ((uiNewVal == sldVal.SLD_uiLock) ? 0 : 1);

    } else {
        return  (1);
    }

#else
    UINT32  uiRes;

    __asm__ __volatile__("ldstub    [%1], %0"
                         : "=r" (uiRes)
                         : "r" (psld)
                         : "memory");

    return  (uiRes);
#endif                                                                  /*  LW_CFG_CPU_ATOMIC_EN > 0    */
}
/*********************************************************************************************************
** ��������: sparcSpinUnlock
** ��������: sparc spin unlock
** �䡡��  : psld       spinlock data ָ��
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static LW_INLINE VOID  sparcSpinUnlock (SPINLOCKTYPE *psld)
{
#if LW_CFG_CPU_ATOMIC_EN > 0
    psld->SLD_usSvcNow++;
#else
    __asm__ __volatile__("stb   %%g0, [%0]" : : "r" (psld) : "memory");
#endif                                                                  /*  LW_CFG_CPU_ATOMIC_EN > 0    */
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

    sparcSpinLock(&psl->SL_sltData, pfuncPoll, pvArg);

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

    if (sparcSpinTryLock(&psl->SL_sltData)) {
        return  (0);
    }

    psl->SL_pcpuOwner = pcpuCur;                                        /*  ���浱ǰ CPU                */

    return  (1);                                                        /*  �����ɹ�                    */
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

    sparcSpinUnlock(&psl->SL_sltData);                                  /*  ���ʵ���д��ڴ�����        */

    return  (1);                                                        /*  �����ɹ�                    */
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
    sparcSpinLock(&psl->SL_sltData, LW_NULL, LW_NULL);

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
    if (sparcSpinTryLock(&psl->SL_sltData)) {
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
    sparcSpinUnlock(&psl->SL_sltData);

    return  (1);                                                        /*  �����ɹ�                    */
}

#endif                                                                  /*  LW_CFG_SMP_EN               */
/*********************************************************************************************************
  END
*********************************************************************************************************/
