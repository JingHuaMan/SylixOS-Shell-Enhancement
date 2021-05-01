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
** ��   ��   ��: armL2.c
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2013 �� 12 �� 09 ��
**
** ��        ��: ARM ��ϵ���� L2 CACHE ����
** ע        ��: ���۴������Ǵ�˻���С��, L2 ��������ΪС��.
**
** BUG:
2015.08.20  ����� ARMv8 ֧��.
*********************************************************************************************************/
#define  __SYLIXOS_IO
#define  __SYLIXOS_KERNEL
#include "SylixOS.h"
/*********************************************************************************************************
  �ü�����
*********************************************************************************************************/
#if LW_CFG_CACHE_EN > 0 && LW_CFG_ARM_CACHE_L2 > 0
#include "armL2.h"
#include "../../../common/cp15/armCp15.h"
/*********************************************************************************************************
  L2 �� (��˹���һ�� L2 CACHE, ���Բ���ʱ��Ҫ��������, ��������Ѿ����ж�, ����ֻ��������������)
*********************************************************************************************************/
static LW_SPINLOCK_CA_DEFINE_CACHE_ALIGN(l2slca);
#define L2_OP_ENTER()   LW_SPIN_LOCK_IGNIRQ(&l2slca.SLCA_sl)
#define L2_OP_EXIT()    LW_SPIN_UNLOCK_IGNIRQ(&l2slca.SLCA_sl)
/*********************************************************************************************************
  L2 ����
*********************************************************************************************************/
static L2C_DRVIER       l2cdrv;
/*********************************************************************************************************
  L2 ��������ʼ������
*********************************************************************************************************/
extern VOID     armL2A8Init(L2C_DRVIER  *pl2cdrv,
                            CACHE_MODE   uiInstruction,
                            CACHE_MODE   uiData,
                            CPCHAR       pcMachineName);
extern VOID     armL2A17Init(L2C_DRVIER  *pl2cdrv,
                             CACHE_MODE   uiInstruction,
                             CACHE_MODE   uiData,
                             CPCHAR       pcMachineName);
extern VOID     armL2x0Init(L2C_DRVIER  *pl2cdrv,
                            CACHE_MODE   uiInstruction,
                            CACHE_MODE   uiData,
                            CPCHAR       pcMachineName,
                            UINT32       uiAux);
/*********************************************************************************************************
** ��������: armL2Enable
** ��������: ʹ�� L2 CACHE 
** �䡡��  : NONE
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
VOID armL2Enable (VOID)
{
    if (l2cdrv.L2CD_pfuncEnable) {
        L2_OP_ENTER();
        l2cdrv.L2CD_pfuncEnable(&l2cdrv);
        L2_OP_EXIT();
    }
}
/*********************************************************************************************************
** ��������: armL2Disable
** ��������: ���� L2 CACHE 
** �䡡��  : NONE
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
** ע  ��  : ��Щ ARM �������ر� L1 Cache �󲻿���ʹ�� spinlock.
*********************************************************************************************************/
VOID armL2Disable (VOID)
{
#if LW_CFG_SMP_EN > 0
    if (l2cdrv.L2CD_pfuncDisable) {
        l2cdrv.L2CD_pfuncDisable(&l2cdrv);
    }

#else
    if (l2cdrv.L2CD_pfuncDisable) {
        L2_OP_ENTER();
        l2cdrv.L2CD_pfuncDisable(&l2cdrv);
        L2_OP_EXIT();
    }
#endif
}
/*********************************************************************************************************
** ��������: armL2IsEnable
** ��������: L2 CACHE �Ƿ��
** �䡡��  : NONE
** �䡡��  : L2 CACHE �Ƿ��
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
BOOL armL2IsEnable (VOID)
{
    BOOL    bIsEnable;

    if (l2cdrv.L2CD_pfuncIsEnable) {
        L2_OP_ENTER();
        bIsEnable = l2cdrv.L2CD_pfuncIsEnable(&l2cdrv);
        L2_OP_EXIT();

    } else {
        bIsEnable = LW_FALSE;
    }
    
    return  (bIsEnable);
}
/*********************************************************************************************************
** ��������: armL2Sync
** ��������: L2 CACHE ͬ��
** �䡡��  : NONE
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
VOID armL2Sync (VOID)
{
    if (l2cdrv.L2CD_pfuncSync) {
        L2_OP_ENTER();
        l2cdrv.L2CD_pfuncSync(&l2cdrv);
        L2_OP_EXIT();
    }
}
/*********************************************************************************************************
** ��������: armL2FlushAll
** ��������: L2 CACHE ��д����������
** �䡡��  : NONE
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
VOID armL2FlushAll (VOID)
{
    if (l2cdrv.L2CD_pfuncFlushAll) {
        L2_OP_ENTER();
        l2cdrv.L2CD_pfuncFlushAll(&l2cdrv);
        L2_OP_EXIT();
    }
}
/*********************************************************************************************************
** ��������: armL2Flush
** ��������: L2 CACHE ��д����������
** �䡡��  : pvPdrs        ��ʼ�����ַ
**           stBytes       ���ݿ��С
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
VOID armL2Flush (PVOID  pvPdrs, size_t  stBytes)
{
    if (l2cdrv.L2CD_pfuncFlush) {
        L2_OP_ENTER();
        l2cdrv.L2CD_pfuncFlush(&l2cdrv, pvPdrs, stBytes);
        L2_OP_EXIT();
    }
}
/*********************************************************************************************************
** ��������: armL2InvalidateAll
** ��������: L2 CACHE ��Ч
** �䡡��  : NONE
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
VOID armL2InvalidateAll (VOID)
{
    if (l2cdrv.L2CD_pfuncInvalidateAll) {
        L2_OP_ENTER();
        l2cdrv.L2CD_pfuncInvalidateAll(&l2cdrv);
        L2_OP_EXIT();
    }
}
/*********************************************************************************************************
** ��������: armL2InvalidateAll
** ��������: L2 CACHE ��Ч
** �䡡��  : pvPdrs        ��ʼ�����ַ
**           stBytes       ���ݿ��С
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
VOID armL2Invalidate (PVOID  pvPdrs, size_t  stBytes)
{
    if (l2cdrv.L2CD_pfuncInvalidate) {
        L2_OP_ENTER();
        l2cdrv.L2CD_pfuncInvalidate(&l2cdrv, pvPdrs, stBytes);
        L2_OP_EXIT();
    }
}
/*********************************************************************************************************
** ��������: armL2ClearAll
** ��������: L2 CACHE ��д����Ч
** �䡡��  : NONE
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
VOID armL2ClearAll (VOID)
{
    if (l2cdrv.L2CD_pfuncClearAll) {
        L2_OP_ENTER();
        l2cdrv.L2CD_pfuncClearAll(&l2cdrv);
        L2_OP_EXIT();
    }
}
/*********************************************************************************************************
** ��������: armL2Clear
** ��������: L2 CACHE ��д����Ч
** �䡡��  : pvPdrs        ��ʼ�����ַ
**           stBytes       ���ݿ��С
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
VOID armL2Clear (PVOID  pvPdrs, size_t  stBytes)
{
    if (l2cdrv.L2CD_pfuncClear) {
        L2_OP_ENTER();
        l2cdrv.L2CD_pfuncClear(&l2cdrv, pvPdrs, stBytes);
        L2_OP_EXIT();
    }
}
/*********************************************************************************************************
** ��������: armL2Name
** ��������: ��� L2 CACHE ����������
** �䡡��  : NONE
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
CPCHAR  armL2Name (VOID)
{
    return  (l2cdrv.L2CD_pcName);
}
/*********************************************************************************************************
** ��������: armL2Init
** ��������: ��ʼ�� L2 CACHE ������
** �䡡��  : uiInstruction      ָ�� CACHE ����
**           uiData             ���� CACHE ����
**           pcMachineName      ��������
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
VOID armL2Init (CACHE_MODE   uiInstruction,
                CACHE_MODE   uiData,
                CPCHAR       pcMachineName)
{
    UINT32  uiCacheId;
    UINT32  uiAux;
    UINT32  uiAuxVal;
    UINT32  uiAuxMask;
    UINT32  uiWays;
    UINT32  uiWaySize;
    UINT32  uiWaySizeShift = L2C_WAY_SIZE_SHIFT;

    LW_SPIN_INIT(&l2slca.SLCA_sl);
    
    if (lib_strcmp(pcMachineName, ARM_MACHINE_A8) == 0) {               /*  A8 ������ L2 CACHE          */
        l2cdrv.L2CD_pcName    = ARM_MACHINE_A8;
        l2cdrv.L2CD_ulBase    = 0ul;
        l2cdrv.L2CD_uiWayMask = 0;
        l2cdrv.L2CD_uiAux     = 0;
        l2cdrv.L2CD_uiType    = 0;
        l2cdrv.L2CD_uiRelease = 0;
        
        _DebugFormat(__LOGMESSAGE_LEVEL, "%s %s L2 cache controller initialization.\r\n", 
                     LW_CFG_CPU_ARCH_FAMILY, l2cdrv.L2CD_pcName);
        
        armL2A8Init(&l2cdrv, uiInstruction, uiData, pcMachineName);

    } else if (lib_strcmp(pcMachineName, ARM_MACHINE_A7) == 0) {        /*  A7 ������ L2 CACHE          */
        l2cdrv.L2CD_pcName    = ARM_MACHINE_A7;
        l2cdrv.L2CD_ulBase    = 0ul;
        l2cdrv.L2CD_uiWayMask = 0;
        l2cdrv.L2CD_uiAux     = 0;
        l2cdrv.L2CD_uiType    = 0;
        l2cdrv.L2CD_uiRelease = 0;
        
        _DebugFormat(__LOGMESSAGE_LEVEL, "%s %s L2 cache controller initialization.\r\n", 
                     LW_CFG_CPU_ARCH_FAMILY, l2cdrv.L2CD_pcName);
                     
    } else if (lib_strcmp(pcMachineName, ARM_MACHINE_A15) == 0) {       /*  A15 ������ L2 CACHE         */
#if LW_CFG_ARM_CACHE_L2_ECC > 0
        UINT32  uiL2Ctl = armA1xL2CtlGet();
#endif                                                                  /*  LW_CFG_ARM_CACHE_L2_ECC > 0 */

        l2cdrv.L2CD_pcName    = ARM_MACHINE_A15;
        l2cdrv.L2CD_ulBase    = 0ul;
        l2cdrv.L2CD_uiWayMask = 0;
        l2cdrv.L2CD_uiAux     = 0;
        l2cdrv.L2CD_uiType    = 0;
        l2cdrv.L2CD_uiRelease = 0;
        
        _DebugFormat(__LOGMESSAGE_LEVEL, "%s %s L2 cache controller initialization.\r\n", 
                     LW_CFG_CPU_ARCH_FAMILY, l2cdrv.L2CD_pcName);

#if LW_CFG_ARM_CACHE_L2_ECC > 0
        uiL2Ctl |= A15_L2_CTL_L2_ECC_EN;
        armA1xL2CtlSet(uiL2Ctl);
#endif                                                                  /*  LW_CFG_ARM_CACHE_L2_ECC > 0 */

    } else if (lib_strcmp(pcMachineName, ARM_MACHINE_A17) == 0) {       /*  A17 ������ L2 CACHE         */
        l2cdrv.L2CD_pcName    = ARM_MACHINE_A17;
        l2cdrv.L2CD_ulBase    = 0ul;
        l2cdrv.L2CD_uiWayMask = 0;
        l2cdrv.L2CD_uiAux     = 0;
        l2cdrv.L2CD_uiType    = 0;
        l2cdrv.L2CD_uiRelease = 0;
        
        _DebugFormat(__LOGMESSAGE_LEVEL, "%s %s L2 cache controller initialization.\r\n", 
                     LW_CFG_CPU_ARCH_FAMILY, l2cdrv.L2CD_pcName);
        
        armL2A17Init(&l2cdrv, uiInstruction, uiData, pcMachineName);
        
    } else if ((lib_strcmp(pcMachineName, ARM_MACHINE_A5)  == 0) ||
               (lib_strcmp(pcMachineName, ARM_MACHINE_A9)  == 0)) {
        if (bspL2CBase(&(l2cdrv.L2CD_ulBase))) {                        /*  ��ÿ���������ַ            */
            return;
        }
        if (bspL2CAux(&uiAuxVal, &uiAuxMask)) {
            return;
        }
    
        uiCacheId = read32_le(L2C_BASE(&l2cdrv) + L2C_CACHE_ID);        /*  ��ȡ ID �Ĵ���              */
        l2cdrv.L2CD_uiRelease = (uiCacheId & 0x1f);
        l2cdrv.L2CD_uiType    = (uiCacheId >> 6) & 0xf;
        
        uiAux  = read32_le(L2C_BASE(&l2cdrv) + L2C_AUX_CTRL);
        uiAux &= uiAuxMask;
        uiAux |= uiAuxVal;
        
        switch (l2cdrv.L2CD_uiType) {
    
        case 0x01:
            l2cdrv.L2CD_pcName = "PL210";
            uiWays = (uiAux >> 13) & 0xf;
            break;
            
        case 0x02:
            l2cdrv.L2CD_pcName = "PL220";
            uiWays = (uiAux >> 13) & 0xf;
            break;
            
        case 0x03:
            l2cdrv.L2CD_pcName = "PL310";
            if (uiAux & (1 << 16)) {
                uiWays = 16;
            } else {
                uiWays = 8;
            }
            break;
            
        default:
            _DebugHandle(__ERRORMESSAGE_LEVEL, "unknown l2-cache type.\r\n");
            return;
        }
    
        L2C_AUX(&l2cdrv)     = uiAux;
        L2C_WAYMASK(&l2cdrv) = (1 << uiWays) - 1;
        
        uiWaySize = (uiAux & L2C_AUX_CTRL_WAY_SIZE_MASK) >> 17;
        uiWaySize = 1 << (uiWaySize + uiWaySizeShift);
        
        l2cdrv.L2CD_stSize = uiWays * uiWaySize * LW_CFG_KB_SIZE;
        
        _DebugFormat(__LOGMESSAGE_LEVEL, "%s %s L2 cache controller initialization.\r\n",
                     LW_CFG_CPU_ARCH_FAMILY, l2cdrv.L2CD_pcName);
        
        armL2x0Init(&l2cdrv, uiInstruction, uiData, pcMachineName, uiAux);
        
        if ((lib_strcmp(pcMachineName, ARM_MACHINE_A9) == 0) &&
            (l2cdrv.L2CD_uiType == 0x03)) {                             /*  PL310 L2 Controler support  */
            armAuxControlFeatureEnable(AUX_CTRL_A9_L2_PREFETCH);        /*  L2: Prefetch Enable         */
        }
    
    } else if ((lib_strcmp(pcMachineName, ARM_MACHINE_A53) == 0) ||
               (lib_strcmp(pcMachineName, ARM_MACHINE_A57) == 0) ||
               (lib_strcmp(pcMachineName, ARM_MACHINE_A72) == 0) ||
               (lib_strcmp(pcMachineName, ARM_MACHINE_A73) == 0)) {     /*  ARMv8                       */
        l2cdrv.L2CD_pcName    = "ARMv8";
        l2cdrv.L2CD_ulBase    = 0ul;
        l2cdrv.L2CD_uiWayMask = 0;
        l2cdrv.L2CD_uiAux     = 0;
        l2cdrv.L2CD_uiType    = 0;
        l2cdrv.L2CD_uiRelease = 0;

        _DebugFormat(__LOGMESSAGE_LEVEL, "%s L2 cache controller initialization.\r\n",
                     l2cdrv.L2CD_pcName);
    
    } else {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "unknown machine name.\r\n");
    }
}

#endif                                                                  /*  LW_CFG_CACHE_EN > 0         */
                                                                        /*  LW_CFG_ARM_CACHE_L2 > 0     */
/*********************************************************************************************************
  END
*********************************************************************************************************/
