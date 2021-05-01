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
** ��   ��   ��: ppcL2CacheCoreNet.c
**
** ��   ��   ��: Jiao.JinXing (������)
**
** �ļ���������: 2018 �� 08 �� 07 ��
**
** ��        ��: CoreNet ��ϵ���� L2 CACHE ����.
*********************************************************************************************************/

#ifndef __ARCH_PPCL2CACHECORENET_H
#define __ARCH_PPCL2CACHECORENET_H

#define PPC_CORENET_L2_CACHE_MAX_NR 3

typedef struct {
    BOOL        CFG_bPresent;                                           /*  �Ƿ���� L2 CACHE           */
    UINT32      CFG_uiL2CacheCsr1;                                      /*  L2 CACHE CSR1 ����ֵ        */
    UINT        CFG_uiNum;                                              /*  L2 CACHE ��Ŀ               */
    addr_t      CFG_ulBase[PPC_CORENET_L2_CACHE_MAX_NR];                /*  L2 CACHE ����������ַ       */
} PPC_CORENET_L2CACHE_CONFIG;

VOID  ppcCoreNetL2CacheConfig(PPC_CORENET_L2CACHE_CONFIG  *pL2Config);

#endif                                                                  /*  __ARCH_PPCL2CACHECORENET_H  */
/*********************************************************************************************************
  END
*********************************************************************************************************/
