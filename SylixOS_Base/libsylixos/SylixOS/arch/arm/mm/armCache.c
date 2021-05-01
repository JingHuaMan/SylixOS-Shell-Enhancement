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
** ��   ��   ��: armCache.c
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2013 �� 12 �� 09 ��
**
** ��        ��: ARM ��ϵ���� CACHE ����.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "SylixOS.h"
/*********************************************************************************************************
  �ü�֧��
*********************************************************************************************************/
#if LW_CFG_CACHE_EN > 0
#if defined(__SYLIXOS_ARM_ARCH_M__)
#include "cache/v7m/armCacheV7M.h"
#else
#include "cache/v4/armCacheV4.h"
#include "cache/v5/armCacheV5.h"
#include "cache/v6/armCacheV6.h"
#include "cache/v7/armCacheV7.h"
#include "cache/v8/armCacheV8.h"
#endif                                                                  /*  !__SYLIXOS_ARM_ARCH_M__     */
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

#if defined(__SYLIXOS_ARM_ARCH_M__)
    armCacheV7MInit(pcacheop, uiInstruction, uiData, pcMachineName);

#else
    if (lib_strcmp(pcMachineName, ARM_MACHINE_920) == 0) {
        armCacheV4Init(pcacheop, uiInstruction, uiData, pcMachineName);
        
    } else if (lib_strcmp(pcMachineName, ARM_MACHINE_926) == 0) {
        armCacheV5Init(pcacheop, uiInstruction, uiData, pcMachineName);
        
    } else if ((lib_strcmp(pcMachineName, ARM_MACHINE_1136) == 0) ||
               (lib_strcmp(pcMachineName, ARM_MACHINE_1176) == 0)) {
        armCacheV6Init(pcacheop, uiInstruction, uiData, pcMachineName);
        
    } else if ((lib_strcmp(pcMachineName, ARM_MACHINE_A5)  == 0) ||
               (lib_strcmp(pcMachineName, ARM_MACHINE_A7)  == 0) ||
               (lib_strcmp(pcMachineName, ARM_MACHINE_A8)  == 0) ||
               (lib_strcmp(pcMachineName, ARM_MACHINE_A9)  == 0) ||
               (lib_strcmp(pcMachineName, ARM_MACHINE_A15) == 0) ||
               (lib_strcmp(pcMachineName, ARM_MACHINE_A17) == 0)) {
        if (__SYLIXOS_ARM_ARCH__ < 7) {
            _DebugHandle(__ERRORMESSAGE_LEVEL, "machine name is NOT fix with "
                                               "compiler -mcpu or -march parameter.\r\n");
        }
        armCacheV7Init(pcacheop, uiInstruction, uiData, pcMachineName);
    
    } else if ((lib_strcmp(pcMachineName, ARM_MACHINE_A53) == 0) ||
               (lib_strcmp(pcMachineName, ARM_MACHINE_A57) == 0) ||
               (lib_strcmp(pcMachineName, ARM_MACHINE_A72) == 0) ||
               (lib_strcmp(pcMachineName, ARM_MACHINE_A73) == 0)) {
        if (__SYLIXOS_ARM_ARCH__ < 8) {
            _DebugHandle(__ERRORMESSAGE_LEVEL, "machine name is NOT fix with "
                                               "compiler -mcpu or -march parameter.\r\n");
        }
        armCacheV8Init(pcacheop, uiInstruction, uiData, pcMachineName);
    
    } else if ((lib_strcmp(pcMachineName, ARM_MACHINE_R4) == 0) ||
               (lib_strcmp(pcMachineName, ARM_MACHINE_R5) == 0) ||
               (lib_strcmp(pcMachineName, ARM_MACHINE_R7) == 0)) {
        armCacheV7Init(pcacheop, uiInstruction, uiData, pcMachineName);
    
    } else {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "unknown machine name.\r\n");
    }
#endif                                                                  /*  !__SYLIXOS_ARM_ARCH_M__     */
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
#if defined(__SYLIXOS_ARM_ARCH_M__)
    armCacheV7MReset(pcMachineName);

#else
    if (lib_strcmp(pcMachineName, ARM_MACHINE_920) == 0) {
        armCacheV4Reset(pcMachineName);
        
    } else if (lib_strcmp(pcMachineName, ARM_MACHINE_926) == 0) {
        armCacheV5Reset(pcMachineName);
        
    } else if ((lib_strcmp(pcMachineName, ARM_MACHINE_1136) == 0) ||
               (lib_strcmp(pcMachineName, ARM_MACHINE_1176) == 0)) {
        armCacheV6Reset(pcMachineName);
        
    } else if ((lib_strcmp(pcMachineName, ARM_MACHINE_A5)  == 0) ||
               (lib_strcmp(pcMachineName, ARM_MACHINE_A7)  == 0) ||
               (lib_strcmp(pcMachineName, ARM_MACHINE_A8)  == 0) ||
               (lib_strcmp(pcMachineName, ARM_MACHINE_A9)  == 0) ||
               (lib_strcmp(pcMachineName, ARM_MACHINE_A15) == 0) ||
               (lib_strcmp(pcMachineName, ARM_MACHINE_A17) == 0)) {
        armCacheV7Reset(pcMachineName);
    
    } else if ((lib_strcmp(pcMachineName, ARM_MACHINE_A53) == 0) ||
               (lib_strcmp(pcMachineName, ARM_MACHINE_A57) == 0) ||
               (lib_strcmp(pcMachineName, ARM_MACHINE_A72) == 0) ||
               (lib_strcmp(pcMachineName, ARM_MACHINE_A73) == 0)) {
        armCacheV8Reset(pcMachineName);
    
    } else if ((lib_strcmp(pcMachineName, ARM_MACHINE_R4) == 0) ||
               (lib_strcmp(pcMachineName, ARM_MACHINE_R5) == 0) ||
               (lib_strcmp(pcMachineName, ARM_MACHINE_R7) == 0)) {
        armCacheV7Reset(pcMachineName);

    } else {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "unknown machine name.\r\n");
    }
#endif                                                                  /*  !__SYLIXOS_ARM_ARCH_M__     */
}

#endif                                                                  /*  LW_CFG_CACHE_EN > 0         */
/*********************************************************************************************************
  END
*********************************************************************************************************/
