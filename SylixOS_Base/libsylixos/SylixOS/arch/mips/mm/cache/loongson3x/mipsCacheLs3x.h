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
** ��   ��   ��: mipsCacheLs3x.h
**
** ��   ��   ��: Jiao.JinXing (������)
**
** �ļ���������: 2016 �� 11 �� 02 ��
**
** ��        ��: Loongson3x ��ϵ���� CACHE ����.
*********************************************************************************************************/

#ifndef __ARCH_MIPSCACHELS3X_H
#define __ARCH_MIPSCACHELS3X_H

VOID  ls3xCacheFlushAll(VOID);
VOID  ls3xCacheEnableHw(VOID);

VOID  mipsCacheLs3xInit(LW_CACHE_OP *pcacheop,
                        CACHE_MODE   uiInstruction,
                        CACHE_MODE   uiData,
                        CPCHAR       pcMachineName);
VOID  mipsCacheLs3xReset(CPCHAR  pcMachineName);

#endif                                                                  /*  __ARCH_MIPSCACHELS3X_H      */
/*********************************************************************************************************
  END
*********************************************************************************************************/
