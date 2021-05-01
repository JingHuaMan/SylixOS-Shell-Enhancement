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
** ��   ��   ��: ppcCache.h
**
** ��   ��   ��: Yang.HaiFeng (���)
**
** �ļ���������: 2016 �� 01 �� 18 ��
**
** ��        ��: PowerPC ��ϵ���� CACHE ����.
*********************************************************************************************************/

#ifndef __ARCH_PPCCACHE_H
#define __ARCH_PPCCACHE_H

/*********************************************************************************************************
  L1-CACHE ״̬
*********************************************************************************************************/

#define L1_CACHE_I_EN   0x01
#define L1_CACHE_D_EN   0x02
#define L1_CACHE_EN     (L1_CACHE_I_EN | L1_CACHE_D_EN)
#define L1_CACHE_DIS    0x00

/*********************************************************************************************************
  CACHE ��Ϣ
*********************************************************************************************************/
typedef struct {
    UINT32              CACHE_uiSize;                                   /*  Cache ��С                  */
    UINT32              CACHE_uiLineSize;                               /*  Cache �д�С                */
    UINT32              CACHE_uiSetNr;                                  /*  ����                        */
    UINT32              CACHE_uiWayNr;                                  /*  ·��                        */
    UINT32              CACHE_uiWaySize;                                /*  ·��С                      */
} PPC_CACHE;

/*********************************************************************************************************
  CACHE ����
*********************************************************************************************************/

typedef struct {
    CPCHAR     L1CD_pcName;
    INT      (*L1CD_pfuncProbe)(CPCHAR  pcMachineName, PPC_CACHE  *pICache, PPC_CACHE  *pDCache);

    VOID     (*L1CD_pfuncDCacheDisable)(VOID);
    VOID     (*L1CD_pfuncDCacheEnable)(VOID);
    VOID     (*L1CD_pfuncICacheDisable)(VOID);
    VOID     (*L1CD_pfuncICacheEnable)(VOID);

    /*
     * ���������� All ������, ���к�������ʵ��
     */
    VOID     (*L1CD_pfuncDCacheClearAll)(VOID);
    VOID     (*L1CD_pfuncDCacheFlushAll)(VOID);
    VOID     (*L1CD_pfuncICacheInvalidateAll)(VOID);

    VOID     (*L1CD_pfuncDCacheClear)(PVOID  pvStart, PVOID  pvEnd, UINT32  uiStep);
    VOID     (*L1CD_pfuncDCacheFlush)(PVOID  pvStart, PVOID  pvEnd, UINT32  uiStep);
    VOID     (*L1CD_pfuncDCacheInvalidate)(PVOID  pvStart, PVOID  pvEnd, UINT32  uiStep);
    VOID     (*L1CD_pfuncICacheInvalidate)(PVOID  pvStart, PVOID  pvEnd, UINT32  uiStep);

    VOID     (*L1CD_pfuncBranchPredictionDisable)(VOID);
    VOID     (*L1CD_pfuncBranchPredictionEnable)(VOID);
    VOID     (*L1CD_pfuncBranchPredictorInvalidate)(VOID);

    VOID     (*L1CD_pfuncTextUpdate)(PVOID  pvStart, PVOID  pvEnd,
                                     UINT32  uiICacheLineSize, UINT32  uiDCacheLineSize);
} PPC_L1C_DRIVER;

/*********************************************************************************************************
  CACHE ��������
*********************************************************************************************************/

INT  ppcCacheInit(LW_CACHE_OP *pcacheop,
                  CACHE_MODE   uiInstruction,
                  CACHE_MODE   uiData,
                  CPCHAR       pcMachineName);

INT  ppcCacheReset(CPCHAR  pcMachineName);

INT  ppcCacheStatus(VOID);

#endif                                                                  /*  __ARCH_PPCCACHE_H           */
/*********************************************************************************************************
  END
*********************************************************************************************************/
