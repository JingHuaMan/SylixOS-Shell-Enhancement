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
** ��   ��   ��: cskySpinlock.c
**
** ��   ��   ��: Wang.Xuan (���Q)
**
** �ļ���������: 2018 �� 05 �� 12 ��
**
** ��        ��: C-SKY ��ϵ�ܹ�����������.
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
** ��������: cskySpinLock
** ��������: C-SKY spin lock
** �䡡��  : psld       spinlock data ָ��
**           pfuncPoll  ѭ���ȴ�ʱ���ú���
**           pvArg      �ص�����
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
** ע  ��  : ��������ʱ, ����ϵͳ������ڴ�����, �������ﲻ��Ҫ����.
*********************************************************************************************************/
static LW_INLINE VOID  cskySpinLock (SPINLOCKTYPE *psld, VOIDFUNCPTR  pfuncPoll, PVOID  pvArg)
{
#if LW_CFG_CSKY_HAS_LDSTEX_INSTR > 0
    SPINLOCKTYPE    sldVal;
    UINT32          uiInc = 1 << LW_SPINLOCK_TICKET_SHIFT;
    UINT32          uiTemp;

    __asm__ __volatile__ (
        "1: ldex.w      %0 , (%2)   \n"
        "   mov         %1 , %0     \n"
        "   add         %0 , %3     \n"
        "   stex.w      %0 , (%2)   \n"
        "   bez         %0 , 1b     \n"
        : "=&r" (uiTemp), "=&r" (sldVal)
        : "r"(&psld->SLD_uiLock), "r"(uiInc)
        : "cc");

    while (sldVal.SLD_usTicket != sldVal.SLD_usSvcNow) {
        if (pfuncPoll) {
            pfuncPoll(pvArg);
        }
        sldVal.SLD_usSvcNow = LW_ACCESS_ONCE(UINT16, psld->SLD_usSvcNow);
    }
#endif                                                                  /*  LW_CFG_CSKY_HAS_LDSTEX_INSTR*/
}
/*********************************************************************************************************
** ��������: cskySpinTryLock
** ��������: C-SKY spin trylock
** �䡡��  : psld       spinlock data ָ��
** �䡡��  : 1: busy 0: ok
** ȫ�ֱ���:
** ����ģ��:
** ע  ��  : ��������ʱ, ����ϵͳ������ڴ�����, �������ﲻ��Ҫ����.
*********************************************************************************************************/
static LW_INLINE UINT32  cskySpinTryLock (SPINLOCKTYPE *psld)
{
#if LW_CFG_CSKY_HAS_LDSTEX_INSTR > 0
    UINT32  uiCont, uiRes, uiTemp;
    UINT32  uiInc = 1 << LW_SPINLOCK_TICKET_SHIFT;

    do {
        asm volatile (
        "   ldex.w      %0 , (%3)       \n"
        "   movi        %2 , 1          \n"
        "   rotli       %1 , %0 , 16    \n"
        "   cmpne       %1 , %0         \n"
        "   bt          1f              \n"
        "   movi        %2 , 0          \n"
        "   add         %0 , %0 , %4    \n"
        "   stex.w      %0 , (%3)       \n"
        "1:                             \n"
        : "=&r" (uiRes), "=&r" (uiTemp), "=&r" (uiCont)
        : "r"(&psld->SLD_uiLock), "r"(uiInc)
        : "cc");
    } while (!uiRes);

    if (!uiCont) {
        return  (1);
    }
#endif                                                                  /*  LW_CFG_CSKY_HAS_LDSTEX_INSTR*/

    return  (0);
}
/*********************************************************************************************************
** ��������: cskySpinUnlock
** ��������: C-SKY spin unlock
** �䡡��  : psld       spinlock data ָ��
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static LW_INLINE VOID  cskySpinUnlock (SPINLOCKTYPE *psld)
{
#if LW_CFG_CSKY_HAS_LDSTEX_INSTR > 0
    psld->SLD_usSvcNow++;
    KN_SMP_WMB();
#endif                                                                  /*  LW_CFG_CSKY_HAS_LDSTEX_INSTR*/
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

    cskySpinLock(&psl->SL_sltData, pfuncPoll, pvArg);

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

    if (cskySpinTryLock(&psl->SL_sltData)) {                            /*  ���Լ���                    */
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

    cskySpinUnlock(&psl->SL_sltData);                                   /*  ����                        */

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
    cskySpinLock(&psl->SL_sltData, LW_NULL, LW_NULL);

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
    if (cskySpinTryLock(&psl->SL_sltData)) {                            /*  ���Լ���                    */
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
    cskySpinUnlock(&psl->SL_sltData);                                   /*  ����                        */

    return  (1);
}

#endif                                                                  /*  LW_CFG_SMP_EN               */
/*********************************************************************************************************
  END
*********************************************************************************************************/
