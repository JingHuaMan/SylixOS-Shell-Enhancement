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
** 文   件   名: ppcMmuHash.h
**
** 创   建   人: Jiao.JinXing (焦进星)
**
** 文件创建日期: 2016 年 01 月 14 日
**
** 描        述: PowerPC 体系构架 HASH 页表 MMU 驱动.
*********************************************************************************************************/

#ifndef __ARCH_PPCMMUHASH_H
#define __ARCH_PPCMMUHASH_H

VOID    ppcHashMmuInit(LW_MMU_OP *pmmuop, CPCHAR  pcMachineName);

ULONG   ppcHashMmuPteMissHandle(addr_t  ulAddr);
INT     ppcHashMmuPtePreLoad(addr_t  ulAddr);

UINT32  ppcHashMmuGetSRR1(VOID);
UINT32  ppcHashMmuGetDSISR(VOID);

#endif                                                                  /*  __ARCH_PPCMMUHASH_H         */
/*********************************************************************************************************
  END
*********************************************************************************************************/
