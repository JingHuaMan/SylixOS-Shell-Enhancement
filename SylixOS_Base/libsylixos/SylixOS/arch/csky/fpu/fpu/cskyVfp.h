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
** ��   ��   ��: cskyVfp.h
**
** ��   ��   ��: Wang.Xuan (���Q)
**
** �ļ���������: 2018 �� 05 �� 14 ��
**
** ��        ��: C-SKY ��ϵ�ܹ� VFP ֧��.
*********************************************************************************************************/

#ifndef __ARCH_CSKYVFP_H
#define __ARCH_CSKYVFP_H

#include "../cskyFpu.h"

PCSKY_FPU_OP  cskyVfpPrimaryInit(CPCHAR    pcMachineName, CPCHAR  pcFpuName);
VOID          cskyVfpSecondaryInit(CPCHAR  pcMachineName, CPCHAR  pcFpuName);

UINT32        cskyVfpGetFESR(VOID);
VOID          cskyVfpSetFESR(UINT32  uiFESR);

#endif                                                                  /*  __ARCH_CSKYVFP_H            */
/*********************************************************************************************************
  END
*********************************************************************************************************/
