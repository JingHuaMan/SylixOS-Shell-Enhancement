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
** ��   ��   ��: mipsCache.c
**
** ��   ��   ��: Jiao.JinXing (������)
**
** �ļ���������: 2015 �� 09 �� 01 ��
**
** ��        ��: MIPS ��ϵ�ܹ� CACHE ����.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "SylixOS.h"
/*********************************************************************************************************
  �ü�֧��
*********************************************************************************************************/
#if LW_CFG_CACHE_EN > 0
#include "cache/r4k/mipsCacheR4k.h"
#include "cache/loongson3x/mipsCacheLs3x.h"
#include "cache/hr2/mipsCacheHr2.h"
#include "arch/mips/common/mipsCpuProbe.h"
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

    mipsCpuProbe(pcMachineName);                                        /*  MIPS CPU ̽��               */

    if ((_G_uiMipsCpuType == CPU_LOONGSON3) ||                          /*  Loongson-3x/2G/2H           */
        (_G_uiMipsCpuType == CPU_LOONGSON2K)) {                         /*  Loongson-2K                 */
        mipsCacheLs3xInit(pcacheop, uiInstruction, uiData, pcMachineName);

    } else if (_G_uiMipsCpuType == CPU_CETC_HR2) {                      /*  CETC-HR2                    */
        mipsCacheHr2Init(pcacheop, uiInstruction, uiData, pcMachineName);

    } else if ((lib_strcmp(pcMachineName, MIPS_MACHINE_LS1X)   == 0) ||
               (lib_strcmp(pcMachineName, MIPS_MACHINE_LS2X)   == 0) || /*  Loongson-2E/2F              */
               (lib_strcmp(pcMachineName, MIPS_MACHINE_24KF)   == 0) ||
               (lib_strcmp(pcMachineName, MIPS_MACHINE_JZ47XX) == 0)) {
        mipsCacheR4kInit(pcacheop, uiInstruction, uiData, pcMachineName);

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
    mipsCpuProbe(pcMachineName);                                        /*  MIPS CPU ̽��               */

    if ((_G_uiMipsCpuType == CPU_LOONGSON3) ||                          /*  Loongson-3x/2G/2H           */
        (_G_uiMipsCpuType == CPU_LOONGSON2K)) {                         /*  Loongson-2K                 */
        mipsCacheLs3xReset(pcMachineName);

    } else if (_G_uiMipsCpuType == CPU_CETC_HR2) {                      /*  CETC-HR2                    */
        mipsCacheHr2Reset(pcMachineName);

    } else if ((lib_strcmp(pcMachineName, MIPS_MACHINE_LS1X)   == 0) ||
               (lib_strcmp(pcMachineName, MIPS_MACHINE_LS2X)   == 0) || /*  Loongson-2E/2F              */
               (lib_strcmp(pcMachineName, MIPS_MACHINE_24KF)   == 0) ||
               (lib_strcmp(pcMachineName, MIPS_MACHINE_JZ47XX) == 0)) {
        mipsCacheR4kReset(pcMachineName);

    } else {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "unknown machine name.\r\n");
    }
}

#endif                                                                  /*  LW_CFG_CACHE_EN > 0         */
/*********************************************************************************************************
  END
*********************************************************************************************************/
