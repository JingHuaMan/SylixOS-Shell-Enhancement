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
** ��   ��   ��: arm64Cache.c
**
** ��   ��   ��: Wang.Xuan (���Q)
**
** �ļ���������: 2018 �� 06 �� 23 ��
**
** ��        ��: ARM64 ��ϵ���� CACHE ����.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "SylixOS.h"
/*********************************************************************************************************
  �ü�֧��
*********************************************************************************************************/
#if LW_CFG_CACHE_EN > 0
#include "cache/arm64Cache.h"
/*********************************************************************************************************
** ��������: archCacheInit
** ��������: ��ʼ�� CACHE 
** �䡡��  : uiInstruction  ָ�� CACHE ����
**           uiData         ���� CACHE ����
**           pcMachineName  ��������
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
VOID  archCacheInit (CACHE_MODE  uiInstruction, CACHE_MODE  uiData, CPCHAR  pcMachineName)
{
    LW_CACHE_OP *pcacheop = API_CacheGetLibBlock();
    
    _DebugFormat(__LOGMESSAGE_LEVEL, "%s %s L1 cache controller initialization.\r\n", 
                 LW_CFG_CPU_ARCH_FAMILY, pcMachineName);

    if ((lib_strcmp(pcMachineName, ARM_MACHINE_A53) == 0) ||
        (lib_strcmp(pcMachineName, ARM_MACHINE_A57) == 0) ||
        (lib_strcmp(pcMachineName, ARM_MACHINE_A72) == 0) ||
        (lib_strcmp(pcMachineName, ARM_MACHINE_A73) == 0)) {
        arm64CacheInit(pcacheop, uiInstruction, uiData, pcMachineName);
    
    } else {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "unknown machine name.\r\n");
    }
}
/*********************************************************************************************************
** ��������: archCacheReset
** ��������: ��λ CACHE, MMU ��ʼ��ʱ��Ҫ���ô˺���
** �䡡��  : pcMachineName  ��������
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
VOID  archCacheReset (CPCHAR  pcMachineName)
{
    if ((lib_strcmp(pcMachineName, ARM_MACHINE_A53) == 0) ||
        (lib_strcmp(pcMachineName, ARM_MACHINE_A57) == 0) ||
        (lib_strcmp(pcMachineName, ARM_MACHINE_A72) == 0) ||
        (lib_strcmp(pcMachineName, ARM_MACHINE_A73) == 0)) {
        arm64CacheReset(pcMachineName);
    
    } else {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "unknown machine name.\r\n");
    }
}

#endif                                                                  /*  LW_CFG_CACHE_EN > 0         */
/*********************************************************************************************************
  END
*********************************************************************************************************/
