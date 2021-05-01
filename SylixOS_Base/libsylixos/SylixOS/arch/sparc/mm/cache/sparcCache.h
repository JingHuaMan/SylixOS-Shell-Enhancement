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
** ��   ��   ��: sparcCache.h
**
** ��   ��   ��: Jiao.JinXing (������)
**
** �ļ���������: 2017 �� 10 �� 10 ��
**
** ��        ��: SPARC ��ϵ���� CACHE ����.
*********************************************************************************************************/

#ifndef __ARCH_SPARC_CACHE_H
#define __ARCH_SPARC_CACHE_H

VOID  sparcCacheInit(LW_CACHE_OP *pcacheop,
                     CACHE_MODE   uiInstruction,
                     CACHE_MODE   uiData,
                     CPCHAR       pcMachineName);

VOID  sparcCacheReset(CPCHAR  pcMachineName);

INT   sparcCacheFlush(LW_CACHE_TYPE  cachetype, PVOID  pvAdrs, size_t  stBytes);

#endif                                                                  /*  __ARCH_SPARC_CACHE_H        */
/*********************************************************************************************************
  END
*********************************************************************************************************/
