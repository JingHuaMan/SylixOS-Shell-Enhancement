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
** ��   ��   ��: cskyCache.c
**
** ��   ��   ��: Wang.Xuan (���Q)
**
** �ļ���������: 2018 �� 05 �� 14 ��
**
** ��        ��: C-SKY ��ϵ�ܹ� CACHE ����.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "SylixOS.h"
/*********************************************************************************************************
  �ü�֧��
*********************************************************************************************************/
#if LW_CFG_CACHE_EN > 0
#include "cache/cskyCache.h"
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
    LW_CACHE_OP  *pcacheop = API_CacheGetLibBlock();

    _DebugFormat(__LOGMESSAGE_LEVEL, "%s %s L1 cache controller initialization.\r\n", 
                 LW_CFG_CPU_ARCH_FAMILY, pcMachineName);

    if ((lib_strcmp(pcMachineName, CSKY_MACHINE_510) == 0) ||
        (lib_strcmp(pcMachineName, CSKY_MACHINE_610) == 0) ||
        (lib_strcmp(pcMachineName, CSKY_MACHINE_801) == 0) ||
        (lib_strcmp(pcMachineName, CSKY_MACHINE_802) == 0) ||
        (lib_strcmp(pcMachineName, CSKY_MACHINE_803) == 0) ||
        (lib_strcmp(pcMachineName, CSKY_MACHINE_807) == 0) ||
        (lib_strcmp(pcMachineName, CSKY_MACHINE_810) == 0) ||
        (lib_strcmp(pcMachineName, CSKY_MACHINE_860) == 0)) {
        cskyCacheInit(pcacheop, uiInstruction, uiData, pcMachineName);

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
    if ((lib_strcmp(pcMachineName, CSKY_MACHINE_510) == 0) ||
        (lib_strcmp(pcMachineName, CSKY_MACHINE_610) == 0) ||
        (lib_strcmp(pcMachineName, CSKY_MACHINE_801) == 0) ||
        (lib_strcmp(pcMachineName, CSKY_MACHINE_802) == 0) ||
        (lib_strcmp(pcMachineName, CSKY_MACHINE_803) == 0) ||
        (lib_strcmp(pcMachineName, CSKY_MACHINE_807) == 0) ||
        (lib_strcmp(pcMachineName, CSKY_MACHINE_810) == 0) ||
        (lib_strcmp(pcMachineName, CSKY_MACHINE_860) == 0)) {
        cskyCacheReset(pcMachineName);

    } else {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "unknown machine name.\r\n");
    }
}

#endif                                                                  /*  LW_CFG_CACHE_EN > 0         */
/*********************************************************************************************************
  END
*********************************************************************************************************/
