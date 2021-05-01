/*********************************************************************************************************
**
**                                    �й������Դ��֯
**
**                                   Ƕ��ʽʵʱ����ϵͳ
**
**                                SylixOS(TM)  LW : long wing
**
**                               Copyright All Rights Reserved
**
**--------------�ļ���Ϣ--------------------------------------------------------------------------------
**
** ��   ��   ��: armExcV7MSvc.c
**
** ��   ��   ��: Jiao.JinXing (������)
**
** �ļ���������: 2017 �� 11 �� 14 ��
**
** ��        ��: ARMv7M ��ϵ�����쳣����(SVC ��ʽ�����л�).
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "SylixOS.h"
/*********************************************************************************************************
  ARMv7M ��ϵ����
*********************************************************************************************************/
#if defined(__SYLIXOS_ARM_ARCH_M__) && (LW_CFG_CORTEX_M_SVC_SWITCH > 0)
#include "armExcV7M.h"
#include "armSvcV7M.h"
/*********************************************************************************************************
  ����ʹ���������
*********************************************************************************************************/
#if LW_CFG_SMP_EN > 0
#define VECTOR_OP_LOCK()    LW_SPIN_LOCK_IGNIRQ(&_K_slcaVectorTable.SLCA_sl)
#define VECTOR_OP_UNLOCK()  LW_SPIN_UNLOCK_IGNIRQ(&_K_slcaVectorTable.SLCA_sl)
#else
#define VECTOR_OP_LOCK()
#define VECTOR_OP_UNLOCK()
#endif                                                                  /*  LW_CFG_SMP_EN > 0           */
/*********************************************************************************************************
** ��������: armv7mIntHandle
** ��������: �жϴ���
** �䡡��  : uiVector  �ж�����
**           pregctx   ������
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
VOID  armv7mIntHandle (UINT32  uiVector, ARCH_REG_CTX  *pregctx)
{
    REGISTER irqreturn_t irqret;
    REGISTER irqvect_t   ulVector = (irqvect_t)uiVector;
    REGISTER BOOL        bPreemptive;

    if (_Inter_Vector_Invalid(ulVector)) {
        return;                                                         /*  �����Ų���ȷ                */
    }

    if (LW_IVEC_GET_FLAG(ulVector) & LW_IRQ_FLAG_PREEMPTIVE) {
        bPreemptive = LW_TRUE;
    } else {
        bPreemptive = LW_FALSE;
    }

    if (bPreemptive) {
        VECTOR_OP_LOCK();
        __ARCH_INT_VECTOR_DISABLE(ulVector);                            /*  ���� vector �ж�            */
        VECTOR_OP_UNLOCK();
        KN_INT_ENABLE_FORCE();                                          /*  �����ж�                    */
    }

    irqret = API_InterVectorIsr(ulVector);                              /*  �����жϷ������            */

    KN_INT_DISABLE();                                                   /*  �����ж�                    */

    if (bPreemptive) {
        if (irqret != LW_IRQ_HANDLED_DISV) {
            VECTOR_OP_LOCK();
            __ARCH_INT_VECTOR_ENABLE(ulVector);                         /*  ���� vector �ж�            */
            VECTOR_OP_UNLOCK();
        }

    } else if (irqret == LW_IRQ_HANDLED_DISV) {
        VECTOR_OP_LOCK();
        __ARCH_INT_VECTOR_DISABLE(ulVector);                            /*  ���� vector �ж�            */
        VECTOR_OP_UNLOCK();
    }
}
/*********************************************************************************************************
** ��������: armv7mNMIIntHandle
** ��������: NMI ����
** �䡡��  : uiVector  �ж�����
**           pregctx   ������
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
LW_WEAK VOID  armv7mNMIIntHandle (UINT32  uiVector, ARCH_REG_CTX  *pregctx)
{
    armv7mIntHandle(uiVector, pregctx);
}
/*********************************************************************************************************
** ��������: armv7mSysTickIntHandle
** ��������: SysTick �жϴ���
** �䡡��  : uiVector  �ж�����
**           pregctx   ������
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
VOID  armv7mSysTickIntHandle (UINT32  uiVector, ARCH_REG_CTX  *pregctx)
{
    armv7mIntHandle(uiVector, pregctx);
}
/*********************************************************************************************************
** ��������: armv7mSvcHandle
** ��������: SVC ����
** �䡡��  : uiVector  �ж�����
**           pregctx   ������
** �䡡��  : ������
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
ARCH_REG_CTX  *armv7mSvcHandle (ARCH_HW_SAVE_REG_CTX  *pHwSaveCtx, ARCH_SW_SAVE_REG_CTX  *pSwSaveCtx)
{
    UINT32          uiCmd = pHwSaveCtx->REG_uiR1;
    PLW_CLASS_CPU   pcpuCur;

    switch (uiCmd) {

    case SVC_archTaskCtxStart:
        pcpuCur = (PLW_CLASS_CPU)pHwSaveCtx->REG_uiR0;
        return  (&pcpuCur->CPU_ptcbTCBCur->TCB_archRegCtx);

    case SVC_archTaskCtxSwitch:
        pcpuCur = (PLW_CLASS_CPU)pHwSaveCtx->REG_uiR0;
        archTaskCtxCopy(&pcpuCur->CPU_ptcbTCBCur->TCB_archRegCtx, pSwSaveCtx, pHwSaveCtx);
        _SchedSwp(pcpuCur);
        return  (&pcpuCur->CPU_ptcbTCBCur->TCB_archRegCtx);

#if LW_CFG_COROUTINE_EN > 0
    case SVC_archCrtCtxSwitch:
        pcpuCur = (PLW_CLASS_CPU)pregctx->REG_uiR0;
        archTaskCtxCopy(&pcpuCur->CPU_pcrcbCur->COROUTINE_archRegCtx, pSwSaveCtx, pHwSaveCtx);
        _SchedCrSwp(pcpuCur);
        return  (&pcpuCur->CPU_pcrcbCur->COROUTINE_archRegCtx));
#endif                                                                  /*  LW_CFG_COROUTINE_EN > 0     */

    case SVC_archSigCtxLoad:
        return  ((ARCH_REG_CTX *)pHwSaveCtx->REG_uiR0);

    default:
        _BugHandle(LW_TRUE, LW_TRUE, "unknown SVC command!\r\n");
        break;
    }

    return  (LW_NULL);
}
/*********************************************************************************************************
** ��������: armv7mDebugMonitorHandle
** ��������: Debug Monitor ����
** �䡡��  : uiVector  �ж�����
**           pregctx   ������
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
VOID  armv7mDebugMonitorHandle (UINT32  uiVector, ARCH_REG_CTX  *pregctx)
{
    LW_VMM_ABORT    abtInfo;
    PLW_CLASS_TCB   ptcbCur;

    LW_TCB_GET_CUR(ptcbCur);

#if LW_CFG_CORTEX_M_FAULT_REBOOT > 0
    abtInfo.VMABT_uiType = LW_VMM_ABORT_TYPE_FATAL_ERROR;
#else
    abtInfo.VMABT_uiType = LW_VMM_ABORT_TYPE_BREAK;
#endif

    abtInfo.VMABT_uiMethod = 0;

    API_VmmAbortIsr(pregctx->REG_uiPc, pregctx->REG_uiPc, &abtInfo, ptcbCur);
}
/*********************************************************************************************************
** ��������: armv7mReservedIntHandle
** ��������: Reserved �жϴ���
** �䡡��  : uiVector  �ж�����
**           pregctx   ������
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
VOID  armv7mReservedIntHandle (UINT32  uiVector, ARCH_REG_CTX  *pregctx)
{
    LW_VMM_ABORT    abtInfo;
    PLW_CLASS_TCB   ptcbCur;

    LW_TCB_GET_CUR(ptcbCur);

#if LW_CFG_CORTEX_M_FAULT_REBOOT > 0
    abtInfo.VMABT_uiType = LW_VMM_ABORT_TYPE_FATAL_ERROR;
#else
    abtInfo.VMABT_uiType = LW_VMM_ABORT_TYPE_TERMINAL;
#endif

    abtInfo.VMABT_uiMethod = 0;

    API_VmmAbortIsr(pregctx->REG_uiPc, pregctx->REG_uiPc, &abtInfo, ptcbCur);
}
/*********************************************************************************************************
** ��������: armv7mFaultCommonHandle
** ��������: Common routine for high-level exception handlers.
** �䡡��  : pregctx   ������
**           ptcbCur   ��ǰ������ƿ�
**           in        �쳣��
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static VOID  armv7mFaultCommonHandle (ARCH_REG_CTX   *pregctx,
                                      PLW_CLASS_TCB   ptcbCur,
                                      INTERRUPTS      in)
{
    LW_VMM_ABORT    abtInfo;
    UINT32          uiHStatus;
    UINT32          uiLStatus;
    addr_t          ulAbortAddr;

    uiHStatus = read32((addr_t)&SCB->HFSR);
    uiLStatus = read32((addr_t)&SCB->CFSR);

    if (uiLStatus & BFARVALID && (in == BUSFAULT ||
        (in == HARDFAULT && uiHStatus & FORCED))) {
        ulAbortAddr = read32((addr_t)&SCB->BFAR);

    } else {
        ulAbortAddr = pregctx->REG_uiPc;
    }

    write32(uiHStatus, (addr_t)&SCB->HFSR);
    write32(uiLStatus, (addr_t)&SCB->CFSR);

    armv7mFaultPrintInfo(in, ulAbortAddr, uiHStatus, uiLStatus);

#if LW_CFG_CORTEX_M_FAULT_REBOOT > 0
    abtInfo.VMABT_uiType = LW_VMM_ABORT_TYPE_FATAL_ERROR;
#else
    abtInfo.VMABT_uiType = LW_VMM_ABORT_TYPE_TERMINAL;
#endif

    abtInfo.VMABT_uiMethod = 0;

    API_VmmAbortIsr(pregctx->REG_uiPc, ulAbortAddr, &abtInfo, ptcbCur);
}
/*********************************************************************************************************
** ��������: armv7mHardFaultHandle
** ��������: Hard Fault ����
** �䡡��  : uiVector  �ж�����
**           pregctx   ������
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
VOID  armv7mHardFaultHandle (UINT32  uiVector, ARCH_REG_CTX  *pregctx)
{
    PLW_CLASS_TCB   ptcbCur;

    LW_TCB_GET_CUR(ptcbCur);

    armv7mFaultCommonHandle(pregctx, ptcbCur, HARDFAULT);
}
/*********************************************************************************************************
** ��������: armv7mBusFaultHandle
** ��������: Bus Fault ����
** �䡡��  : uiVector  �ж�����
**           pregctx   ������
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
VOID  armv7mBusFaultHandle (UINT32  uiVector, ARCH_REG_CTX  *pregctx)
{
    PLW_CLASS_TCB   ptcbCur;

    LW_TCB_GET_CUR(ptcbCur);

    armv7mFaultCommonHandle(pregctx, ptcbCur, BUSFAULT);
}
/*********************************************************************************************************
** ��������: armv7mUsageFaultHandle
** ��������: Usage Fault ����
** �䡡��  : uiVector  �ж�����
**           pregctx   ������
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
VOID  armv7mUsageFaultHandle (UINT32  uiVector, ARCH_REG_CTX  *pregctx)
{
    PLW_CLASS_TCB   ptcbCur;

    LW_TCB_GET_CUR(ptcbCur);

#if LW_CFG_CPU_FPU_EN > 0
    if (archFpuUndHandle(ptcbCur) == ERROR_NONE) {                      /*  ���� FPU ָ��̽��           */
        return;
    }
#endif                                                                  /*  LW_CFG_CPU_FPU_EN > 0       */

    armv7mFaultCommonHandle(pregctx, ptcbCur, USAGEFAULT);
}
/*********************************************************************************************************
** ��������: armv7mMemFaultHandle
** ��������: Mem Fault ����
** �䡡��  : uiVector  �ж�����
**           pregctx   ������
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
VOID  armv7mMemFaultHandle (UINT32  uiVector, ARCH_REG_CTX  *pregctx)
{
    UINT32          uiLStatus;
    addr_t          ulAbortAddr;
    LW_VMM_ABORT    abtInfo;
    PLW_CLASS_TCB   ptcbCur;

    LW_TCB_GET_CUR(ptcbCur);

    /*
     * Read the fault status register from the MPU hardware
     */
    uiLStatus = read32((addr_t)&SCB->CFSR);

    /*
     * Clean up the memory fault status register for a next exception
     */
    write32(uiLStatus, (addr_t)&SCB->CFSR);

    if ((uiLStatus & 0xf0) == 0x80) {
        /*
         * Did we get a valid address in the memory fault address register?
         * If so, this is a data access failure (can't tell read or write).
         */
        ulAbortAddr = read32((addr_t)&SCB->MMFAR);

        abtInfo.VMABT_uiType   = LW_VMM_ABORT_TYPE_PERM;
        abtInfo.VMABT_uiMethod = LW_VMM_ABORT_METHOD_WRITE;

    } else if (uiLStatus & 0x1) {
        /*
         * If there is no valid address, did we get an instuction access
         * failure? If so, this is a code access failure.
         */
        ulAbortAddr = pregctx->REG_uiPc;

        abtInfo.VMABT_uiType   = LW_VMM_ABORT_TYPE_PERM;
        abtInfo.VMABT_uiMethod = LW_VMM_ABORT_METHOD_EXEC;

    } else {
        /*
         * Some error bit set in the memory fault address register.
         * This must be MUNSTKERR due to a stacking error, which
         * implies that we have gone beyond the low stack boundary
         * (or somehow SP got incorrect).
         * There is no recovery from that since no registers
         * were saved on the stack on this exception, that is,
         * we have no PC saved to return to user mode.
         */
        ulAbortAddr = (addr_t)-1;
        abtInfo.VMABT_uiType   = LW_VMM_ABORT_TYPE_PERM;
        abtInfo.VMABT_uiMethod = LW_VMM_ABORT_METHOD_EXEC;
    }

#if LW_CFG_CORTEX_M_FAULT_REBOOT > 0
    abtInfo.VMABT_uiType = LW_VMM_ABORT_TYPE_FATAL_ERROR;
#endif

    API_VmmAbortIsr(pregctx->REG_uiPc, ulAbortAddr, &abtInfo, ptcbCur);
}

#endif                                                                  /*  __SYLIXOS_ARM_ARCH_M__      */
/*********************************************************************************************************
  END
*********************************************************************************************************/
