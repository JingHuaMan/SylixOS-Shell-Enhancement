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
** ��   ��   ��: armExcV7M.c
**
** ��   ��   ��: Jiao.JinXing (������)
**
** �ļ���������: 2017 �� 11 �� 14 ��
**
** ��        ��: ARMv7M ��ϵ�����쳣����.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "SylixOS.h"
/*********************************************************************************************************
  ARMv7M ��ϵ����
*********************************************************************************************************/
#if defined(__SYLIXOS_ARM_ARCH_M__)
#include "armExcV7M.h"
/*********************************************************************************************************
  ARMv7M �쳣�ṹ����
*********************************************************************************************************/
TRAPS  _G_armV7MTraps[] = {
    {"Vector Read error",        VECTTBL,     HARDFAULT},
    {"uCode stack push error",   STKERR,      BUSFAULT},
    {"uCode stack pop error",    UNSTKERR,    BUSFAULT},
    {"Escalated to Hard Fault",  FORCED,      HARDFAULT},
    {"Pre-fetch error",          IBUSERR,     BUSFAULT},
    {"Precise data bus error",   PRECISERR,   BUSFAULT},
    {"Imprecise data bus error", IMPRECISERR, BUSFAULT},
    {"No Coprocessor",           NOCP,        USAGEFAULT},
    {"Undefined Instruction",    UNDEFINSTR,  USAGEFAULT},
    {"Invalid ISA state",        INVSTATE,    USAGEFAULT},
    {"Return to invalid PC",     INVPC,       USAGEFAULT},
    {"Illegal unaligned access", UNALIGNED,   USAGEFAULT},
    {"Divide By 0",              DIVBYZERO,   USAGEFAULT},
    {NULL}
};
/*********************************************************************************************************
** ��������: armv7mTrapsInit
** ��������: The function initializes Bus fault and Usage fault exceptions,
**           forbids unaligned data access and division by 0.
** �䡡��  : NONE
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
VOID  armv7mTrapsInit (VOID)
{
    write32(read32((addr_t)&SCB->SHCSR) | USGFAULTENA | BUSFAULTENA,
            (addr_t)&SCB->SHCSR);

    write32(DIV_0_TRP | read32((addr_t)&SCB->CCR),
            (addr_t)&SCB->CCR);
}
/*********************************************************************************************************
** ��������: armv7mFaultPrintInfo
** ��������: The function prints information about the reason of the exception
** �䡡��  : in            IPSR, the number of the exception
**           ulAbortAddr   address caused the interrupt, or current pc
**           uiHStatus     status register for hard fault
**           uiLStatus     status register for local fault
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
VOID  armv7mFaultPrintInfo (INTERRUPTS  in,
                            addr_t      ulAbortAddr,
                            UINT32      uiHStatus,
                            UINT32      uiLStatus)
{
    INT  i;

    _PrintFormat("\r\n\r\nFault %d at 0x%08lx [hstatus=0x%08lx, lstatus=0x%08lx]\r\n",
                (INT)in, ulAbortAddr, uiHStatus, uiLStatus);

    for (i = 0; _G_armV7MTraps[i].pcName != NULL; i++) {
        if ((_G_armV7MTraps[i].iHandler == HARDFAULT ? uiHStatus : uiLStatus) & _G_armV7MTraps[i].iTestBit) {
            _PrintFormat("%s\r\n", _G_armV7MTraps[i].pcName);
        }
    }
    _PrintFormat("\r\n");
}

#endif                                                                  /*  __SYLIXOS_ARM_ARCH_M__      */
/*********************************************************************************************************
  END
*********************************************************************************************************/
