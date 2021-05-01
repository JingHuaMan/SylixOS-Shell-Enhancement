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
** ��   ��   ��: armExcV7MPendSv.c
**
** ��   ��   ��: Jiao.JinXing (������)
**
** �ļ���������: 2017 �� 11 �� 14 ��
**
** ��        ��: ARMv7M ��ϵ�����쳣����(PendSV ��ʽ�����л�).
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "SylixOS.h"
/*********************************************************************************************************
  ARMv7M ��ϵ����
*********************************************************************************************************/
#if defined(__SYLIXOS_ARM_ARCH_M__) && (LW_CFG_CORTEX_M_SVC_SWITCH == 0)
#include "armExcV7M.h"
/*********************************************************************************************************
** ��������: armv7mIntHandle
** ��������: �жϴ���
** �䡡��  : uiVector  �ж�����
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
VOID  armv7mIntHandle (UINT32  uiVector)
{
    KN_INT_ENABLE_FORCE();                                              /*  ���ж�, ����Ƕ��          */

    API_InterVectorIsr((ULONG)uiVector);

    KN_INT_DISABLE();                                                   /*  �����ж�                    */
}
/*********************************************************************************************************
** ��������: armv7mNMIIntHandle
** ��������: NMI ����
** �䡡��  : uiVector  �ж�����
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
LW_WEAK VOID  armv7mNMIIntHandle (UINT32  uiVector)
{
    armv7mIntHandle(uiVector);
}
/*********************************************************************************************************
** ��������: armv7mSysTickIntHandle
** ��������: SysTick �жϴ���
** �䡡��  : uiVector  �ж�����
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
VOID  armv7mSysTickIntHandle (UINT32  uiVector)
{
    armv7mIntHandle(uiVector);
}
/*********************************************************************************************************
** ��������: armv7mSvcHandle
** ��������: SVC ����
** �䡡��  : uiVector  �ж�����
**           ulRetAddr ���ص�ַ
** �䡡��  : ������
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
VOID  armv7mSvcHandle (UINT32  uiVector, addr_t  ulRetAddr)
{
    PLW_CLASS_TCB   ptcbCur;

    LW_TCB_GET_CUR(ptcbCur);

    _DebugFormat(__ERRORMESSAGE_LEVEL,                                  /*  �ؼ��Դ���                  */
                 "FATAL ERROR: exception in thread %lx[%s]. "
                 "ret_addr: 0x%08lx abt_addr: 0x%08lx abt_type: %s\r\n"
                 "rebooting...\r\n",
                 ptcbCur->TCB_ulId, ptcbCur->TCB_cThreadName,
                 ulRetAddr, ulRetAddr, "SVC");

    API_KernelReboot(LW_REBOOT_FORCE);                                  /*  ֱ��������������ϵͳ        */
}
/*********************************************************************************************************
** ��������: armv7mDebugMonitorHandle
** ��������: Debug Monitor ����
** �䡡��  : uiVector  �ж�����
**           ulRetAddr ���ص�ַ
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
VOID  armv7mDebugMonitorHandle (UINT32  uiVector, addr_t  ulRetAddr)
{
    PLW_CLASS_TCB   ptcbCur;

    LW_TCB_GET_CUR(ptcbCur);

    _DebugFormat(__ERRORMESSAGE_LEVEL,                                  /*  �ؼ��Դ���                  */
                 "FATAL ERROR: exception in thread %lx[%s]. "
                 "ret_addr: 0x%08lx abt_addr: 0x%08lx abt_type: %s\r\n"
                 "rebooting...\r\n",
                 ptcbCur->TCB_ulId, ptcbCur->TCB_cThreadName,
                 ulRetAddr, ulRetAddr, "DebugMonitor");

    API_KernelReboot(LW_REBOOT_FORCE);                                  /*  ֱ��������������ϵͳ        */
}
/*********************************************************************************************************
** ��������: armv7mReservedIntHandle
** ��������: Reserved �жϴ���
** �䡡��  : uiVector  �ж�����
**           ulRetAddr ���ص�ַ
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
VOID  armv7mReservedIntHandle (UINT32  uiVector, addr_t  ulRetAddr)
{
    PLW_CLASS_TCB   ptcbCur;

    LW_TCB_GET_CUR(ptcbCur);

    _DebugFormat(__ERRORMESSAGE_LEVEL,                                  /*  �ؼ��Դ���                  */
                 "FATAL ERROR: exception in thread %lx[%s]. "
                 "ret_addr: 0x%08lx abt_addr: 0x%08lx abt_type: %s\r\n"
                 "rebooting...\r\n",
                 ptcbCur->TCB_ulId, ptcbCur->TCB_cThreadName,
                 ulRetAddr, ulRetAddr, "ReservedInt");

    API_KernelReboot(LW_REBOOT_FORCE);                                  /*  ֱ��������������ϵͳ        */
}
/*********************************************************************************************************
** ��������: armv7mFaultCommonHandle
** ��������: Common routine for high-level exception handlers.
** �䡡��  : ulRetAddr ���ص�ַ
**           ptcbCur   ��ǰ������ƿ�
**           in        �쳣��
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static VOID  armv7mFaultCommonHandle (addr_t          ulRetAddr,
                                      PLW_CLASS_TCB   ptcbCur,
                                      INTERRUPTS      in)
{
    UINT32  uiHStatus;
    UINT32  uiLStatus;
    addr_t  ulAbortAddr;

    uiHStatus = read32((addr_t)&SCB->HFSR);
    uiLStatus = read32((addr_t)&SCB->CFSR);

    if (uiLStatus & BFARVALID && (in == BUSFAULT ||
        (in == HARDFAULT && uiHStatus & FORCED))) {
        ulAbortAddr = read32((addr_t)&SCB->BFAR);

    } else {
        ulAbortAddr = ulRetAddr;
    }

    write32(uiHStatus, (addr_t)&SCB->HFSR);
    write32(uiLStatus, (addr_t)&SCB->CFSR);

    _DebugFormat(__ERRORMESSAGE_LEVEL,                                  /*  �ؼ��Դ���                  */
                 "FATAL ERROR: exception in thread %lx[%s].\r\n",
                 ptcbCur->TCB_ulId, ptcbCur->TCB_cThreadName);

    armv7mFaultPrintInfo(in, ulAbortAddr, uiHStatus, uiLStatus);

    _PrintFormat("rebooting...\r\n");

    API_KernelReboot(LW_REBOOT_FORCE);                                  /*  ֱ��������������ϵͳ        */
}
/*********************************************************************************************************
** ��������: armv7mHardFaultHandle
** ��������: Hard Fault ����
** �䡡��  : uiVector  �ж�����
**           ulRetAddr ���ص�ַ
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
VOID  armv7mHardFaultHandle (UINT32  uiVector, addr_t  ulRetAddr)
{
    PLW_CLASS_TCB   ptcbCur;

    LW_TCB_GET_CUR(ptcbCur);

    armv7mFaultCommonHandle(ulRetAddr, ptcbCur, HARDFAULT);
}
/*********************************************************************************************************
** ��������: armv7mBusFaultHandle
** ��������: Bus Fault ����
** �䡡��  : uiVector  �ж�����
**           ulRetAddr ���ص�ַ
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
VOID  armv7mBusFaultHandle (UINT32  uiVector, addr_t  ulRetAddr)
{
    PLW_CLASS_TCB   ptcbCur;

    LW_TCB_GET_CUR(ptcbCur);

    armv7mFaultCommonHandle(ulRetAddr, ptcbCur, BUSFAULT);
}
/*********************************************************************************************************
** ��������: armv7mUsageFaultHandle
** ��������: Usage Fault ����
** �䡡��  : uiVector  �ж�����
**           ulRetAddr ���ص�ַ
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
VOID  armv7mUsageFaultHandle (UINT32  uiVector, addr_t  ulRetAddr)
{
    PLW_CLASS_TCB   ptcbCur;

    LW_TCB_GET_CUR(ptcbCur);

#if LW_CFG_CPU_FPU_EN > 0
    if (archFpuUndHandle(ptcbCur) == ERROR_NONE) {                      /*  ���� FPU ָ��̽��           */
        return;
    }
#endif                                                                  /*  LW_CFG_CPU_FPU_EN > 0       */

    armv7mFaultCommonHandle(ulRetAddr, ptcbCur, USAGEFAULT);
}
/*********************************************************************************************************
** ��������: armv7mMemFaultHandle
** ��������: Mem Fault ����
** �䡡��  : uiVector  �ж�����
**           ulRetAddr ���ص�ַ
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
VOID  armv7mMemFaultHandle (UINT32  uiVector, addr_t  ulRetAddr)
{
    UINT32          uiLStatus;
    addr_t          ulAbortAddr;
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

    } else if (uiLStatus & 0x1) {
        /*
         * If there is no valid address, did we get an instuction access
         * failure? If so, this is a code access failure.
         */
        ulAbortAddr = ulRetAddr;

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
    }

    _DebugFormat(__ERRORMESSAGE_LEVEL,                                  /*  �ؼ��Դ���                  */
                 "FATAL ERROR: exception in thread %lx[%s]. "
                 "ret_addr: 0x%08lx abt_addr: 0x%08lx abt_type: %s\r\n"
                 "rebooting...\r\n",
                 ptcbCur->TCB_ulId, ptcbCur->TCB_cThreadName,
                 ulRetAddr, ulAbortAddr, "MemFault");

    API_KernelReboot(LW_REBOOT_FORCE);                                  /*  ֱ��������������ϵͳ        */
}

#endif                                                                  /*  __SYLIXOS_ARM_ARCH_M__      */
/*********************************************************************************************************
  END
*********************************************************************************************************/
