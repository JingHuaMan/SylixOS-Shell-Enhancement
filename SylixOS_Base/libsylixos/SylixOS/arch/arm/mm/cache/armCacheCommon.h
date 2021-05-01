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
** ��   ��   ��: armCacheCommon.h
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2013 �� 12 �� 09 ��
**
** ��        ��: ARM ��ϵ�ܹ� CACHE ͨ�ú���֧��.
*********************************************************************************************************/

#ifndef __ARMCACHECOMMON_H
#define __ARMCACHECOMMON_H

/*********************************************************************************************************
  �ü�֧��
*********************************************************************************************************/
#if LW_CFG_CACHE_EN > 0

UINT32  armCacheTypeReg(VOID);
VOID    armCacheRetireRR(VOID);
VOID    armCacheRetireDefault(VOID);
VOID    armICacheEnable(VOID);
VOID    armDCacheEnable(VOID);
VOID    armICacheDisable(VOID);
VOID    armICacheInvalidate(PVOID  pvStart, PVOID  pvEnd, UINT32  uiStep);
VOID    armICacheInvalidateAll(VOID);
VOID    armDCacheInvalidate(PVOID  pvStart, PVOID  pvEnd, UINT32  uiStep);
VOID    armDCacheFlush(PVOID  pvStart, PVOID  pvEnd, UINT32  uiStep);
VOID    armDCacheClear(PVOID  pvStart, PVOID  pvEnd, UINT32  uiStep);

/*********************************************************************************************************
  CACHE ��� pvAdrs �� pvEnd λ��
*********************************************************************************************************/

#define ARM_CACHE_GET_END(pvAdrs, stBytes, ulEnd, uiLineSize)               \
        do {                                                                \
            ulEnd  = (addr_t)((size_t)pvAdrs + stBytes);                    \
            pvAdrs = (PVOID)((addr_t)pvAdrs & ~((addr_t)uiLineSize - 1));   \
        } while (0)

#endif                                                                  /*  LW_CFG_CACHE_EN > 0         */
#endif                                                                  /*  __ARMCACHECOMMON_H          */
/*********************************************************************************************************
  END
*********************************************************************************************************/
