/*********************************************************************************************************
**
**                                    中国软件开源组织
**
**                                   嵌入式实时操作系统
**
**                                SylixOS(TM)  LW : long wing
**
**                               Copyright All Rights Reserved
**
**--------------文件信息--------------------------------------------------------------------------------
**
** 文   件   名: arm64Elf.h
**
** 创   建   人: Wang.Xuan (王翾)
**
** 文件创建日期: 2018 年 06 月 29 日
**
** 描        述: 实现 ARM64 体系结构的 ELF 文件重定位.
*********************************************************************************************************/

#ifndef __ARCH_ARM64ELF_H
#define __ARCH_ARM64ELF_H

#ifdef LW_CFG_CPU_ARCH_ARM64                                            /*  ARM64 体系结构              */

#define ELF_CLASS       ELFCLASS64
#define ELF_ARCH        EM_AARCH64

#endif                                                                  /*  LW_CFG_CPU_ARCH_ARM64       */
#endif                                                                  /*  __ARCH_ARM64ELF_H           */
/*********************************************************************************************************
  END
*********************************************************************************************************/
