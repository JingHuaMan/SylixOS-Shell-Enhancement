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
** ��   ��   ��: armMmu.c
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2013 �� 12 �� 09 ��
**
** ��        ��: ARM ��ϵ���� MMU ����.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "SylixOS.h"
/*********************************************************************************************************
  �ü�֧��
*********************************************************************************************************/
#if LW_CFG_VMM_EN > 0
#include "mmu/v4/armMmuV4.h"
#include "mmu/v7/armMmuV7.h"
/*********************************************************************************************************
** ��������: archMmuInit
** ��������: ��ʼ�� CACHE 
** �䡡��  : pcMachineName  ��������
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
VOID  archMmuInit (CPCHAR  pcMachineName)
{
    LW_MMU_OP *pmmuop = API_VmmGetLibBlock();
    
    _DebugFormat(__LOGMESSAGE_LEVEL, "%s %s MMU initialization.\r\n", 
                 LW_CFG_CPU_ARCH_FAMILY, pcMachineName);

    if ((lib_strcmp(pcMachineName, ARM_MACHINE_920)  == 0) ||
        (lib_strcmp(pcMachineName, ARM_MACHINE_926)  == 0) ||
        (lib_strcmp(pcMachineName, ARM_MACHINE_1136) == 0) ||
        (lib_strcmp(pcMachineName, ARM_MACHINE_1176) == 0)) {           /* ARMv4/v5/v6 ����             */
        _BugHandle(LW_CFG_CPU_PHYS_ADDR_64BIT, LW_TRUE, "LW_CFG_CPU_PHYS_ADDR_64BIT MUST be 0!\r\n");
        armMmuV4Init(pmmuop, pcMachineName);
        
    } else if ((lib_strcmp(pcMachineName, ARM_MACHINE_A5)  == 0) ||
               (lib_strcmp(pcMachineName, ARM_MACHINE_A7)  == 0) ||
               (lib_strcmp(pcMachineName, ARM_MACHINE_A8)  == 0) ||
               (lib_strcmp(pcMachineName, ARM_MACHINE_A9)  == 0) ||
               (lib_strcmp(pcMachineName, ARM_MACHINE_A15) == 0) ||
               (lib_strcmp(pcMachineName, ARM_MACHINE_A17) == 0) ||
               (lib_strcmp(pcMachineName, ARM_MACHINE_A53) == 0) ||
               (lib_strcmp(pcMachineName, ARM_MACHINE_A57) == 0) ||
               (lib_strcmp(pcMachineName, ARM_MACHINE_A72) == 0) ||
               (lib_strcmp(pcMachineName, ARM_MACHINE_A73) == 0)) {     /* ARMv7/v8 ����                */
        if (__SYLIXOS_ARM_ARCH__ < 7) {
            _DebugHandle(__ERRORMESSAGE_LEVEL, "machine name is NOT fix with "
                                               "compiler -mcpu or -march parameter.\r\n");
        }
        armMmuV7Init(pmmuop, pcMachineName);
    
    } else {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "unknown machine name.\r\n");
    }
}

#endif                                                                  /*  LW_CFG_VMM_EN > 0           */
/*********************************************************************************************************
  END
*********************************************************************************************************/
