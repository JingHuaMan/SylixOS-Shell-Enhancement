/*********************************************************************************************************
**
**                                    中国软件开源组织
**
**                                   嵌入式实时操作系统
**
**                                       SylixOS(TM)
**
**                               Copyright  All Rights Reserved
**
**--------------文件信息--------------------------------------------------------------------------------
**
** 文   件   名: cskyVfp.h
**
** 创   建   人: Wang.Xuan (王翾)
**
** 文件创建日期: 2018 年 05 月 14 日
**
** 描        述: C-SKY 体系架构 VFP 支持.
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
