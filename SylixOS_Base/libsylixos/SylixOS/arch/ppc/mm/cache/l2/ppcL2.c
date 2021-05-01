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
** ��   ��   ��: ppcL2.c
**
** ��   ��   ��: Jiao.JinXing (������)
**
** �ļ���������: 2016 �� 01 �� 28 ��
**
** ��        ��: PowerPC ��ϵ���� L2 CACHE ����.
*********************************************************************************************************/
#define  __SYLIXOS_IO
#define  __SYLIXOS_KERNEL
#include "SylixOS.h"
/*********************************************************************************************************
  �ü�����
*********************************************************************************************************/
#if LW_CFG_CACHE_EN > 0 && LW_CFG_PPC_CACHE_L2 > 0
#include "ppcL2.h"
/*********************************************************************************************************
  L2 �� (��˹���һ�� L2 CACHE, ���Բ���ʱ��Ҫ��������, ��������Ѿ����ж�, ����ֻ��������������)
*********************************************************************************************************/
static LW_SPINLOCK_CA_DEFINE_CACHE_ALIGN(_G_l2slca);
#define L2_OP_ENTER()   LW_SPIN_LOCK_IGNIRQ(&_G_l2slca.SLCA_sl)
#define L2_OP_EXIT()    LW_SPIN_UNLOCK_IGNIRQ(&_G_l2slca.SLCA_sl)
/*********************************************************************************************************
  L2/L3 CACHE ����
*********************************************************************************************************/
static L2C_DRVIER       _G_l2cdrv;
static L2C_DRVIER       _G_l3cdrv;
/*********************************************************************************************************
  ���� L2/L3 ��������ʼ������
*********************************************************************************************************/
VOID  ppc750L2CacheInit(L2C_DRVIER  *pl2cdrv,
                        CACHE_MODE   uiInstruction,
                        CACHE_MODE   uiData,
                        CPCHAR       pcMachineName);
VOID  ppcCoreNetL2CacheInit(L2C_DRVIER  *pl2cdrv,
                            CACHE_MODE   uiInstruction,
                            CACHE_MODE   uiData,
                            CPCHAR       pcMachineName);
VOID  ppcE500mcL2CacheInit(L2C_DRVIER  *pl2cdrv,
                           CACHE_MODE   uiInstruction,
                           CACHE_MODE   uiData,
                           CPCHAR       pcMachineName);
VOID  ppcQorIQL2CacheInit(L2C_DRVIER  *pl2cdrv,
                          CACHE_MODE   uiInstruction,
                          CACHE_MODE   uiData,
                          CPCHAR       pcMachineName);
VOID  ppcQorIQL3CacheInit(L2C_DRVIER  *pl3cdrv,
                          CACHE_MODE   uiInstruction,
                          CACHE_MODE   uiData,
                          CPCHAR       pcMachineName);
/*********************************************************************************************************
** ��������: bspL2CacheInit
** ��������: BSP ��ص� L2 CACHE ��ʼ��
** �䡡��  : pl2cdrv            L2 ����
**           uiInstruction      ָ�� CACHE ����
**           uiData             ���� CACHE ����
**           pcMachineName      ��������
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
LW_WEAK VOID  bspL2CacheInit (PVOID        pl2cdrv,
                              CACHE_MODE   uiInstruction,
                              CACHE_MODE   uiData,
                              CPCHAR       pcMachineName)
{
    _DebugHandle(__ERRORMESSAGE_LEVEL, "L2 Cache: unknown machine name.\r\n");
}
/*********************************************************************************************************
** ��������: bspL3CacheInit
** ��������: BSP ��ص� L3 CACHE ��ʼ��
** �䡡��  : pl3cdrv            L3 ����
**           uiInstruction      ָ�� CACHE ����
**           uiData             ���� CACHE ����
**           pcMachineName      ��������
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
LW_WEAK VOID  bspL3CacheInit (PVOID        pl3cdrv,
                              CACHE_MODE   uiInstruction,
                              CACHE_MODE   uiData,
                              CPCHAR       pcMachineName)
{
    _DebugHandle(__ERRORMESSAGE_LEVEL, "L3 Cache: unknown machine name.\r\n");
}
/*********************************************************************************************************
** ��������: ppcL2Enable
** ��������: ʹ�� L2 CACHE
** �䡡��  : NONE
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
VOID  ppcL2Enable (VOID)
{
    L2_OP_ENTER();
    if (_G_l2cdrv.L2CD_pfuncEnable) {
        _G_l2cdrv.L2CD_pfuncEnable(&_G_l2cdrv);
    }
    if (_G_l3cdrv.L2CD_pfuncEnable) {
        _G_l3cdrv.L2CD_pfuncEnable(&_G_l3cdrv);
    }
    L2_OP_EXIT();
}
/*********************************************************************************************************
** ��������: ppcL2Disable
** ��������: ���� L2 CACHE 
** �䡡��  : NONE
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
VOID  ppcL2Disable (VOID)
{
    L2_OP_ENTER();
    if (_G_l2cdrv.L2CD_pfuncDisable) {
        _G_l2cdrv.L2CD_pfuncDisable(&_G_l2cdrv);
    }
    if (_G_l3cdrv.L2CD_pfuncDisable) {
        _G_l3cdrv.L2CD_pfuncDisable(&_G_l3cdrv);
    }
    L2_OP_EXIT();
}
/*********************************************************************************************************
** ��������: ppcL2IsEnable
** ��������: L2 CACHE �Ƿ��
** �䡡��  : NONE
** �䡡��  : L2 CACHE �Ƿ��
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
BOOL  ppcL2IsEnable (VOID)
{
    BOOL    bIsEnable;

    L2_OP_ENTER();
    if (_G_l2cdrv.L2CD_pfuncIsEnable) {
        bIsEnable = _G_l2cdrv.L2CD_pfuncIsEnable(&_G_l2cdrv);
    } else {
        bIsEnable = LW_FALSE;
    }
    L2_OP_EXIT();
    
    return  (bIsEnable);
}
/*********************************************************************************************************
** ��������: ppcL2Sync
** ��������: L2 CACHE ͬ��
** �䡡��  : NONE
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
VOID  ppcL2Sync (VOID)
{
    L2_OP_ENTER();
    if (_G_l2cdrv.L2CD_pfuncSync) {
        _G_l2cdrv.L2CD_pfuncSync(&_G_l2cdrv);
    }
    L2_OP_EXIT();
}
/*********************************************************************************************************
** ��������: ppcL2FlushAll
** ��������: L2 CACHE ��д����������
** �䡡��  : NONE
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
VOID  ppcL2FlushAll (VOID)
{
    L2_OP_ENTER();
    if (_G_l2cdrv.L2CD_pfuncFlushAll) {
        _G_l2cdrv.L2CD_pfuncFlushAll(&_G_l2cdrv);
    }
    L2_OP_EXIT();
}
/*********************************************************************************************************
** ��������: ppcL2Flush
** ��������: L2 CACHE ��д����������
** �䡡��  : pvPdrs        ��ʼ�����ַ
**           stBytes       ���ݿ��С
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
VOID  ppcL2Flush (PVOID  pvPdrs, size_t  stBytes)
{
    L2_OP_ENTER();
    if (_G_l2cdrv.L2CD_pfuncFlush) {
        _G_l2cdrv.L2CD_pfuncFlush(&_G_l2cdrv, pvPdrs, stBytes);
    }
    L2_OP_EXIT();
}
/*********************************************************************************************************
** ��������: ppcL2InvalidateAll
** ��������: L2 CACHE ��Ч
** �䡡��  : NONE
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
VOID  ppcL2InvalidateAll (VOID)
{
    L2_OP_ENTER();
    if (_G_l2cdrv.L2CD_pfuncInvalidateAll) {
        _G_l2cdrv.L2CD_pfuncInvalidateAll(&_G_l2cdrv);
    }
    L2_OP_EXIT();
}
/*********************************************************************************************************
** ��������: ppcL2InvalidateAll
** ��������: L2 CACHE ��Ч
** �䡡��  : pvPdrs        ��ʼ�����ַ
**           stBytes       ���ݿ��С
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
VOID  ppcL2Invalidate (PVOID  pvPdrs, size_t  stBytes)
{
    L2_OP_ENTER();
    if (_G_l2cdrv.L2CD_pfuncInvalidate) {
        _G_l2cdrv.L2CD_pfuncInvalidate(&_G_l2cdrv, pvPdrs, stBytes);
    }
    L2_OP_EXIT();
}
/*********************************************************************************************************
** ��������: ppcL2ClearAll
** ��������: L2 CACHE ��д����Ч
** �䡡��  : NONE
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
VOID  ppcL2ClearAll (VOID)
{
    L2_OP_ENTER();
    if (_G_l2cdrv.L2CD_pfuncClearAll) {
        _G_l2cdrv.L2CD_pfuncClearAll(&_G_l2cdrv);
    }
    L2_OP_EXIT();
}
/*********************************************************************************************************
** ��������: ppcL2Clear
** ��������: L2 CACHE ��д����Ч
** �䡡��  : pvPdrs        ��ʼ�����ַ
**           stBytes       ���ݿ��С
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
VOID  ppcL2Clear (PVOID  pvPdrs, size_t  stBytes)
{
    L2_OP_ENTER();
    if (_G_l2cdrv.L2CD_pfuncClear) {
        _G_l2cdrv.L2CD_pfuncClear(&_G_l2cdrv, pvPdrs, stBytes);
    }
    L2_OP_EXIT();
}
/*********************************************************************************************************
** ��������: ppcL2Name
** ��������: ��� L2 CACHE ����������
** �䡡��  : NONE
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
CPCHAR  ppcL2Name (VOID)
{
    return  (_G_l2cdrv.L2CD_pcName);
}
/*********************************************************************************************************
** ��������: ppcL2Init
** ��������: ��ʼ�� L2 CACHE ������
** �䡡��  : uiInstruction      ָ�� CACHE ����
**           uiData             ���� CACHE ����
**           pcMachineName      ��������
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
VOID  ppcL2Init (CACHE_MODE   uiInstruction,
                 CACHE_MODE   uiData,
                 CPCHAR       pcMachineName)
{
    LW_SPIN_INIT(&_G_l2slca.u.SLUCA_sl);
    
    _G_l2cdrv.L2CD_uiType    = 0;
    _G_l2cdrv.L2CD_uiRelease = 0;

    _G_l3cdrv.L2CD_uiType    = 0;
    _G_l3cdrv.L2CD_uiRelease = 0;

    if ((lib_strcmp(pcMachineName, PPC_MACHINE_750)  == 0) ||
        (lib_strcmp(pcMachineName, PPC_MACHINE_745X) == 0)) {
        ppc750L2CacheInit(&_G_l2cdrv, uiInstruction, uiData, pcMachineName);

    } else if ((lib_strcmp(pcMachineName, PPC_MACHINE_E500)   == 0) ||
               (lib_strcmp(pcMachineName, PPC_MACHINE_E500V1) == 0) ||
               (lib_strcmp(pcMachineName, PPC_MACHINE_E500V2) == 0)) {
        ppcQorIQL2CacheInit(&_G_l2cdrv, uiInstruction, uiData, pcMachineName);

    } else if ((lib_strcmp(pcMachineName, PPC_MACHINE_E500MC) == 0) ||
               (lib_strcmp(pcMachineName, PPC_MACHINE_E5500)  == 0)) {
        ppcE500mcL2CacheInit(&_G_l2cdrv, uiInstruction, uiData, pcMachineName);
        ppcQorIQL3CacheInit (&_G_l3cdrv, uiInstruction, uiData, pcMachineName);


    } else if (lib_strcmp(pcMachineName, PPC_MACHINE_E6500) == 0) {
        ppcCoreNetL2CacheInit(&_G_l2cdrv, uiInstruction, uiData, pcMachineName);
        ppcQorIQL3CacheInit  (&_G_l3cdrv, uiInstruction, uiData, pcMachineName);

    } else {
        bspL2CacheInit((PVOID)(&_G_l2cdrv), uiInstruction, uiData, pcMachineName);
        bspL3CacheInit((PVOID)(&_G_l3cdrv), uiInstruction, uiData, pcMachineName);
    }
}

#endif                                                                  /*  LW_CFG_CACHE_EN > 0         */
                                                                        /*  LW_CFG_PPC_CACHE_L2 > 0     */
/*********************************************************************************************************
  END
*********************************************************************************************************/
