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
** ��   ��   ��: ppcCacheE200.c
**
** ��   ��   ��: Jiao.JinXing (������)
**
** �ļ���������: 2016 �� 05 �� 03 ��
**
** ��        ��: PowerPC E200 ��ϵ���� CACHE ����.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "SylixOS.h"
/*********************************************************************************************************
  �ü�֧��
*********************************************************************************************************/
#if LW_CFG_CACHE_EN > 0
#include "../common/ppcCache.h"
#define  __SYLIXOS_PPC_E200__
#include "arch/ppc/arch_e500.h"
/*********************************************************************************************************
  Probe ������Ҫ�޸ĵı���(�����Ҫ�õ�)
*********************************************************************************************************/
UINT32  PPC_E200_CACHE_CNWAY      = 8;
UINT32  PPC_E200_CACHE_SETS       = 128;
UINT32  PPC_E200_CACHE_ALIGN_SIZE = 32;
UINT32  PPC_E200_CACHE_LINE_NUM   = 128 * 8;
/*********************************************************************************************************
  �ⲿ�ӿ�����
*********************************************************************************************************/
extern VOID     ppcE200CacheDisable(VOID);
extern VOID     ppcE200CacheEnable(VOID);

extern VOID     ppcE200DCacheClear(PVOID  pvStart, PVOID  pvEnd, UINT32  uiStep);
extern VOID     ppcE200DCacheFlush(PVOID  pvStart, PVOID  pvEnd, UINT32  uiStep);
extern VOID     ppcE200DCacheInvalidate(PVOID  pvStart, PVOID  pvEnd, UINT32  uiStep);
extern VOID     ppcE200ICacheInvalidate(PVOID  pvStart, PVOID  pvEnd, UINT32  uiStep);

extern VOID     ppcE200BranchPredictionDisable(VOID);
extern VOID     ppcE200BranchPredictionEnable(VOID);
extern VOID     ppcE200BranchPredictorInvalidate(VOID);

extern UINT32   ppcE200CacheGetL1CFG0(VOID);

extern VOID     ppcE200TextUpdate(PVOID  pvStart, PVOID  pvEnd,
                                  UINT32  uiICacheLineSize, UINT32  uiDCacheLineSize);
/*********************************************************************************************************
** ��������: __ppcE200CacheDisable
** ��������: �ر� CACHE
** �䡡��  : NONE
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static VOID  __ppcE200CacheDisable (VOID)
{
    if (ppcCacheStatus() == L1_CACHE_DIS) {
        ppcE200CacheDisable();
    }
}
/*********************************************************************************************************
** ��������: __ppcE200CacheEnable
** ��������: ʹ�� CACHE
** �䡡��  : NONE
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static VOID  __ppcE200CacheEnable (VOID)
{
    if (ppcCacheStatus() == L1_CACHE_EN) {
        ppcE200CacheEnable();
    }
}
/*********************************************************************************************************
** ��������: ppcE200CacheProbe
** ��������: CACHE ̽��
** �䡡��  : pcMachineName         ������
**           pICache               ICACHE ��Ϣ
**           pDCache               DCACHE ��Ϣ
** �䡡��  : ERROR or OK
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  ppcE200CacheProbe (CPCHAR  pcMachineName, PPC_CACHE  *pICache, PPC_CACHE  *pDCache)
{
    if (lib_strcmp(pcMachineName, PPC_MACHINE_E200) == 0) {
        E200_L1CFG0     l1cfg0;
        UINT32          uiCacheSize;

        l1cfg0.L1CFG0_uiValue = ppcE200CacheGetL1CFG0();

        uiCacheSize = l1cfg0.L1CFG0_usCSIZE * 1024;

        switch (l1cfg0.L1CFG0_ucCBSIZE) {

        case 0:
            PPC_E200_CACHE_ALIGN_SIZE = 32;
            break;

        case 1:
            PPC_E200_CACHE_ALIGN_SIZE = 64;
            break;

        case 2:
            PPC_E200_CACHE_ALIGN_SIZE = 128;
            break;

        default:
            return  (PX_ERROR);
        }

        PPC_E200_CACHE_CNWAY      = l1cfg0.L1CFG0_ucCNWAY + 1;
        PPC_E200_CACHE_LINE_NUM   = (uiCacheSize / PPC_E200_CACHE_ALIGN_SIZE);
        PPC_E200_CACHE_SETS       = PPC_E200_CACHE_LINE_NUM / PPC_E200_CACHE_CNWAY;

        pICache->CACHE_uiSize     = uiCacheSize;
        pICache->CACHE_uiLineSize = PPC_E200_CACHE_ALIGN_SIZE;
        pICache->CACHE_uiSetNr    = PPC_E200_CACHE_SETS;
        pICache->CACHE_uiWayNr    = PPC_E200_CACHE_CNWAY;
        pICache->CACHE_uiWaySize  = pDCache->CACHE_uiSetNr * pDCache->CACHE_uiLineSize;

        *pDCache = *pICache;
        return  (ERROR_NONE);

    } else {
        return  (PX_ERROR);
    }
}
/*********************************************************************************************************
  E200 CACHE ����
*********************************************************************************************************/
LW_WEAK PPC_L1C_DRIVER  _G_ppcE200CacheDriver = {
    "E200",
    ppcE200CacheProbe,

    __ppcE200CacheDisable,
    __ppcE200CacheEnable,
    __ppcE200CacheDisable,
    __ppcE200CacheEnable,

    /*
     * E200 Ϊͳһ CACHE, ��֧�� ALL ����
     */
    LW_NULL,
    LW_NULL,
    LW_NULL,

    ppcE200DCacheClear,
    ppcE200DCacheFlush,
    ppcE200DCacheInvalidate,
    ppcE200ICacheInvalidate,

    ppcE200BranchPredictionDisable,
    ppcE200BranchPredictionEnable,
    ppcE200BranchPredictorInvalidate,

    ppcE200TextUpdate,
};

#endif                                                                  /*  LW_CFG_CACHE_EN > 0         */
/*********************************************************************************************************
  END
*********************************************************************************************************/
