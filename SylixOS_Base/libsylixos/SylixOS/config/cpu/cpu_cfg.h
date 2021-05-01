/*********************************************************************************************************
**
**                                    �й������Դ��֯
**
**                                   Ƕ��ʽʵʱ����ϵͳ
**
**                                SylixOS(TM)  LW : long wing
**
**                               Copyright All Rights Reserved
**
**--------------�ļ���Ϣ--------------------------------------------------------------------------------
**
** ��   ��   ��: cpu_cfg.h
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2015 �� 11 �� 20 ��
**
** ��        ��: CPU �����빦������.
*********************************************************************************************************/

#ifndef __CPU_CFG_H
#define __CPU_CFG_H

#ifdef __GNUC__
#if defined(__arm__)
#include "cpu_cfg_arm.h"

#elif defined(__aarch64__)
#include "cpu_cfg_arm64.h"

#elif defined(__mips__) || defined(__mips64)
#include "cpu_cfg_mips.h"

#elif defined(__PPC__)
#include "cpu_cfg_ppc.h"

#elif defined(__i386__) || defined(__x86_64__)
#include "cpu_cfg_x86.h"

#elif defined(__TMS320C6X__)
#include "cpu_cfg_c6x.h"

#elif defined(__sparc__)
#include "cpu_cfg_sparc.h"

#elif defined(__riscv)
#include "cpu_cfg_riscv.h"

#elif defined(__csky__)
#include "cpu_cfg_csky.h"
#endif

#else
#include "cpu_cfg_arm.h"
#endif

#endif                                                                  /*  __CPU_CFG_H                 */
/*********************************************************************************************************
  END
*********************************************************************************************************/
