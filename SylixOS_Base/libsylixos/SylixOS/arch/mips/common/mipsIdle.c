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
** ��   ��   ��: mipsIdle.c
**
** ��   ��   ��: Han.hui (����)
**
** �ļ���������: 2017 �� 04 �� 14 ��
**
** ��        ��: MIPS ��ϵ�ܹ� IDLE ���ܳ���.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "SylixOS.h"
#include "arch/mips/common/cp0/mipsCp0.h"
/*********************************************************************************************************
  ��ຯ��
*********************************************************************************************************/
extern VOID  mipsWaitInstruction(VOID);
/*********************************************************************************************************
** ��������: mipsIdleHookGet
** ��������: ��� IDLE ���ܳ���
** �䡡��  : pcMachineName     ��������
** �䡡��  : IDLE ���ܳ���
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
VOIDFUNCPTR  mipsIdleHookGet (CPCHAR  pcMachineName)
{
    if ((lib_strcmp(pcMachineName, MIPS_MACHINE_LS1X)   == 0) ||
        (lib_strcmp(pcMachineName, MIPS_MACHINE_24KF)   == 0) ||
        (lib_strcmp(pcMachineName, MIPS_MACHINE_JZ47XX) == 0)) {
        return  (mipsWaitInstruction);

    } else if ((lib_strcmp(pcMachineName, MIPS_MACHINE_LS3X) == 0)) {
        ARCH_REG_T  ulPrid = mipsCp0PRIdRead();

        if (((ulPrid & 0xf) == PRID_REV_LOONGSON2K_R1) ||
            ((ulPrid & 0xf) == PRID_REV_LOONGSON2K_R2)) {               /*  ������о2K �ֲ�����֧�� wait*/
            return  (mipsWaitInstruction);                              /*  ����о Linux ȴû��ʹ��!    */

        } else if ((ulPrid & 0xf) >= PRID_REV_LOONGSON3A_R2) {          /*  ����о Linux ��ͬ           */
            return  (mipsWaitInstruction);
        }

    } else if ((lib_strcmp(pcMachineName, MIPS_MACHINE_HR2) == 0)) {
        return  (mipsWaitInstruction);
    }

    return  (LW_NULL);
}
/*********************************************************************************************************
  END
*********************************************************************************************************/
