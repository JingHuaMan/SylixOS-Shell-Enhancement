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
** ��   ��   ��: ppcExcE500.h
**
** ��   ��   ��: Jiao.JinXing (������)
**
** �ļ���������: 2016 �� 05 �� 04 ��
**
** ��        ��: PowerPC E500 ��ϵ�����쳣����.
*********************************************************************************************************/

#ifndef __ARCH_PPCEXCE500_H
#define __ARCH_PPCEXCE500_H

/*********************************************************************************************************
  �ж���������
*********************************************************************************************************/

#define PPC_E500_ISA_VECTOR_BASE            0
#define PPC_E500_ARCH_VECTOR_BASE           16
#define PPC_E500_MPIC_VECTOR_BASE           32

#define PPC_E500_ARCH_DEC_VECTOR            (PPC_E500_ARCH_VECTOR_BASE + 0)
#define PPC_E500_ARCH_WATCHDOG_VECTOR       (PPC_E500_ARCH_VECTOR_BASE + 1)
#define PPC_E500_ARCH_TIMER_VECTOR          (PPC_E500_ARCH_VECTOR_BASE + 2)

/*********************************************************************************************************
  ���¼����쳣������Ϊ������, BSP ���Ը�����Ҫʵ������
*********************************************************************************************************/

VOID  archE500PerfMonitorExceptionHandle(addr_t  ulRetAddr);
VOID  archE500DoorbellExceptionHandle(addr_t  ulRetAddr);
VOID  archE500DoorbellCriticalExceptionHandle(addr_t  ulRetAddr);
VOID  archE500DecrementerInterruptHandle(addr_t  ulRetAddr);
VOID  archE500TimerInterruptHandle(addr_t  ulRetAddr);
VOID  archE500WatchdogInterruptHandle(addr_t  ulRetAddr);

/*********************************************************************************************************
  ��������
*********************************************************************************************************/

VOID  archE500DecrementerInterruptAck(VOID);
VOID  archE500DecrementerInterruptEnable(VOID);
VOID  archE500DecrementerInterruptDisable(VOID);
BOOL  archE500DecrementerInterruptIsEnable(VOID);
VOID  archE500DecrementerAutoReloadEnable(VOID);
VOID  archE500DecrementerAutoReloadDisable(VOID);

VOID  archE500TimerInterruptAck(VOID);
VOID  archE500TimerInterruptEnable(VOID);
VOID  archE500TimerInterruptDisable(VOID);
BOOL  archE500TimerInterruptIsEnable(VOID);

VOID  archE500WatchdogInterruptAck(VOID);
VOID  archE500WatchdogInterruptEnable(VOID);
VOID  archE500WatchdogInterruptDisable(VOID);
BOOL  archE500WatchdogInterruptIsEnable(VOID);

VOID  archE500VectorInit(CPCHAR  pcMachineName, addr_t  ulVectorBase);
VOID  arch460VectorInit(CPCHAR  pcMachineName, addr_t  ulVectorBase);

#endif                                                                  /*  __ARCH_PPCEXCE500_H         */
/*********************************************************************************************************
  END
*********************************************************************************************************/
