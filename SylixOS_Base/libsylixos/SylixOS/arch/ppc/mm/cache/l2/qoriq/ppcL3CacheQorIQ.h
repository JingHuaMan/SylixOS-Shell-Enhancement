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
** ��   ��   ��: ppcL3CacheQorIQ.h
**
** ��   ��   ��: Jiao.JinXing (������)
**
** �ļ���������: 2018 �� 08 �� 07 ��
**
** ��        ��: QorIQ ��ϵ���� L3 CACHE ����.
*********************************************************************************************************/

#ifndef __ARCH_PPCL3CACHEQORIQ_H
#define __ARCH_PPCL3CACHEQORIQ_H

#define PPC_QORIQ_L3_CACHE_MAX_NR  4

typedef struct {
    BOOL        CFG_bPresent;                                           /*  �Ƿ���� L3 CACHE           */
    UINT        CFG_uiNum;                                              /*  L3 CACHE ��Ŀ               */
    addr_t      CFG_ulBase[PPC_QORIQ_L3_CACHE_MAX_NR];                  /*  L3 CACHE ����������ַ       */
} PPC_QORIQ_L3CACHE_CONFIG;

VOID  ppcQorIQL3CacheConfig(PPC_QORIQ_L3CACHE_CONFIG  *pL3Config);

#endif                                                                  /*  __ARCH_PPCL3CACHEQORIQ_H    */
/*********************************************************************************************************
  END
*********************************************************************************************************/
