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
** ��   ��   ��: ppcMmu.c
**
** ��   ��   ��: Jiao.JinXing (������)
**
** �ļ���������: 2015 �� 12 �� 15 ��
**
** ��        ��: PowerPC ��ϵ���� MMU ����.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "SylixOS.h"
/*********************************************************************************************************
  �ü�֧��
*********************************************************************************************************/
#if LW_CFG_VMM_EN > 0
#include "mmu/hash/ppcMmuHash.h"
#include "mmu/e500/ppcMmuE500.h"
#include "mmu/ppc460/ppcMmu460.h"
/*********************************************************************************************************
  MMU ���� TLB Ԥ������������
*********************************************************************************************************/
static  INT  (*_G_pfuncMmuDataTlbPreLoad)(addr_t  ulAddr) = LW_NULL;
/*********************************************************************************************************
** ��������: archMmuInit
** ��������: ��ʼ�� MMU
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

    if ((lib_strcmp(pcMachineName, PPC_MACHINE_603)     == 0) ||
        (lib_strcmp(pcMachineName, PPC_MACHINE_EC603)   == 0) ||
        (lib_strcmp(pcMachineName, PPC_MACHINE_604)     == 0) ||
        (lib_strcmp(pcMachineName, PPC_MACHINE_750)     == 0) ||
        (lib_strcmp(pcMachineName, PPC_MACHINE_MPC83XX) == 0) ||
        (lib_strcmp(pcMachineName, PPC_MACHINE_745X)    == 0) ||
        (lib_strcmp(pcMachineName, PPC_MACHINE_E300)    == 0) ||
        (lib_strcmp(pcMachineName, PPC_MACHINE_E600)    == 0)) {
        _BugHandle(LW_CFG_PPC_PAGE_SHIFT != 12, LW_TRUE, "LW_CFG_PPC_PAGE_SHIFT MUST be 12!\r\n");
        _BugHandle(LW_CFG_CPU_PHYS_ADDR_64BIT, LW_TRUE, "LW_CFG_CPU_PHYS_ADDR_64BIT MUST be 0!\r\n");
        ppcHashMmuInit(pmmuop, pcMachineName);
        _G_pfuncMmuDataTlbPreLoad = ppcHashMmuPtePreLoad;

    } else if ((lib_strcmp(pcMachineName, PPC_MACHINE_E500)   == 0) ||
               (lib_strcmp(pcMachineName, PPC_MACHINE_E500V1) == 0) ||
               (lib_strcmp(pcMachineName, PPC_MACHINE_E500V2) == 0) ||
               (lib_strcmp(pcMachineName, PPC_MACHINE_E500MC) == 0) ||
               (lib_strcmp(pcMachineName, PPC_MACHINE_E5500)  == 0) ||
               (lib_strcmp(pcMachineName, PPC_MACHINE_E6500)  == 0)) {
        ppcE500MmuInit(pmmuop, pcMachineName);

    } else if (lib_strcmp(pcMachineName, PPC_MACHINE_460) == 0) {
        _BugHandle(LW_CFG_CPU_PHYS_ADDR_64BIT, LW_TRUE, "LW_CFG_CPU_PHYS_ADDR_64BIT MUST be 0!\r\n");
        ppc460MmuInit(pmmuop, pcMachineName);

    } else {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "unknown machine name.\r\n");
    }
}
/*********************************************************************************************************
** ��������: archMmuDataTlbPreLoad
** ��������: MMU ���� TLB Ԥ����
** �䡡��  : ulAddr        ���ʵ�ַ
** �䡡��  : ERROR CODE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
INT  archMmuDataTlbPreLoad (addr_t  ulAddr)
{
    if (_G_pfuncMmuDataTlbPreLoad) {
        return  (_G_pfuncMmuDataTlbPreLoad(ulAddr));
    } else {
        return  (PX_ERROR);
    }
}

#endif                                                                  /*  LW_CFG_VMM_EN > 0           */
/*********************************************************************************************************
  END
*********************************************************************************************************/
