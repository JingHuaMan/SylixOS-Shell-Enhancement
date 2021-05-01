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
** ��   ��   ��: mipsCacheHr2.h
**
** ��   ��   ��: Jiao.JinXing (������)
**
** �ļ���������: 2018 �� 03 �� 14 ��
**
** ��        ��: ��� 2 �Ŵ����� CACHE ����.
*********************************************************************************************************/

#ifndef __ARCH_MIPSCACHEHR2_H
#define __ARCH_MIPSCACHEHR2_H

VOID  hr2CacheFlushAll(VOID);
VOID  hr2CacheEnableHw(VOID);

VOID  mipsCacheHr2Init(LW_CACHE_OP *pcacheop,
                       CACHE_MODE   uiInstruction,
                       CACHE_MODE   uiData,
                       CPCHAR       pcMachineName);
VOID  mipsCacheHr2Reset(CPCHAR  pcMachineName);

#endif                                                                  /*  __ARCH_MIPSCACHEHR2_H       */
/*********************************************************************************************************
  END
*********************************************************************************************************/
