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
** ��   ��   ��: endian.h
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2010 �� 05 �� 22 ��
**
** ��        ��: ϵͳ��С��.
*********************************************************************************************************/

#ifndef __SYS_ENDIAN_H
#define __SYS_ENDIAN_H

#include <config/cpu/cpu_cfg.h>
#include <config/kernel/endian_cfg.h>

#undef  BYTE_ORDER
#undef  __BYTE_ORDER

#ifndef LITTLE_ENDIAN
#define LITTLE_ENDIAN               1234
#endif                                                                  /*  LITTLE_ENDIAN               */
#ifndef __LITTLE_ENDIAN
#define __LITTLE_ENDIAN             LITTLE_ENDIAN
#endif                                                                  /*  __LITTLE_ENDIAN             */

#ifndef BIG_ENDIAN
#define BIG_ENDIAN                  4321
#endif                                                                  /*  BIG_ENDIAN                  */
#ifndef __BIG_ENDIAN
#define __BIG_ENDIAN                BIG_ENDIAN
#endif                                                                  /*  __BIG_ENDIAN                */

#if LW_CFG_CPU_ENDIAN > 0
#define BYTE_ORDER                  BIG_ENDIAN
#define __BYTE_ORDER                BIG_ENDIAN
#else
#define BYTE_ORDER                  LITTLE_ENDIAN
#define __BYTE_ORDER                LITTLE_ENDIAN
#endif                                                                  /*  LW_CFG_CPU_ENDIAN > 0       */

#endif                                                                  /*  __SYS_ENDIAN_H              */
/*********************************************************************************************************
  END
*********************************************************************************************************/
