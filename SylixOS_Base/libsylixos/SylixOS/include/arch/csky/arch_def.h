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
** ��   ��   ��: arch_def.h
**
** ��   ��   ��: Wang.Xuan (���Q)
**
** �ļ���������: 2018 �� 05 �� 07 ��
**
** ��        ��: C-SKY ��ض���.
*********************************************************************************************************/

#ifndef __CSKY_ARCH_DEF_H
#define __CSKY_ARCH_DEF_H

#include "asm/archprob.h"

/*********************************************************************************************************
  SSEG0 SSEG1 ��ַת��
*********************************************************************************************************/
#if defined(__SYLIXOS_KERNEL) || defined(__ASSEMBLY__) || defined(ASSEMBLY)

#define CSKY_SSEG0_PA(va)           ((va) & 0x7fffffff)
#define CSKY_SSEG1_PA(va)           ((va) & 0x1fffffff)

#define CSKY_SSEG0_VA(pa)           ((pa) | 0x80000000)
#define CSKY_SSEG1_VA(pa)           ((pa) | 0xa0000000)

/*********************************************************************************************************
  C-SKY ָ��
*********************************************************************************************************/

#if !defined(__ASSEMBLY__) && !defined(ASSEMBLY)
typedef UINT16                      CSKY_INSTRUCTION;
#define IS_T32(hi16)                (((hi16) & 0xc000) == 0xc000)
#endif                                                                  /*  !defined(__ASSEMBLY__)      */

/*********************************************************************************************************
  PSR Process Status Register (CR0)
    31  30                              24  23                         16
  +---+--------------------------------+---------------------------------+
  | S |      Reserved                  |            VEC[7:0]             |
  +------+----------------------------------+----------------------------+
   15     14  13  12   11  10   9    8    7    6   5    4   3  2   1   0
  +---------+---+----+-------+----+----+----+----+---+----+------+---+---+
  | TM[1:0] | 0 | TE |   0   | MM | EE | IC | IE | 0 | FE |   0  |AF*| C |
  +---------+---+----+-------+----+----+----+----+---+----+------+---+---+
*********************************************************************************************************/

#define M_PSR_S         (1 << S_PSR_S)
#define S_PSR_S         31
#define M_PSR_TE        (1 << S_PSR_TE)
#define S_PSR_TE        12
#define M_PSR_MM        (1 << S_PSR_MM)
#define S_PSR_MM        9
#define M_PSR_EE        (1 << S_PSR_EE)
#define S_PSR_EE        8
#define M_PSR_IC        (1 << S_PSR_IC)
#define S_PSR_IC        7
#define M_PSR_IE        (1 << S_PSR_IE)
#define S_PSR_IE        6
#define M_PSR_FE        (1 << S_PSR_FE)
#define S_PSR_FE        4
#define M_PSR_AF        (1 << S_PSR_AF)
#define S_PSR_AF        1
#define M_PSR_C         (1 << S_PSR_C)
#define S_PSR_C         0

/*********************************************************************************************************
  CFR Cache Function Register (CR17)
     31    30                            18    17        16
  +------+---------------------------------+---------+----------+
  | LICF |      Reserved                   | BTB_INV | BHT_INV  |
  +------+---------------------------------+---------+----------+
   15       9     8      7     6     5     4    3  2   1    0    
  +----------+--------+-----+-----+-----+-----+-----+-----------+
  | Reserved | UNLOCK | ITS | OMS | CLR | INV |  0  | CACHE_SEL |
  +----------+--------+-----+-----+-----+-----+-----+-----------+
*********************************************************************************************************/

#define M_CFR_LICF      (1 << S_CFR_LICF)
#define S_CFR_LICF      31
#define M_CFR_BTB_INV   (1 << S_CFR_BTB_INV)
#define S_CFR_BTB_INV   17
#define M_CFR_BHT_INV   (1 << S_CFR_BHT_INV)
#define S_CFR_BHT_INV   16
#define M_CFR_UNLOCK    (1 << S_CFR_UNLOCK)
#define S_CFR_UNLOCK    8
#define M_CFR_ITS       (1 << S_CFR_ITS)
#define S_CFR_ITS       7
#define M_CFR_OMS       (1 << S_CFR_OMS)
#define S_CFR_OMS       6
#define M_CFR_CLR       (1 << S_CFR_CLR)
#define S_CFR_CLR       5
#define M_CFR_INV       (1 << S_CFR_INV)
#define S_CFR_INV       4
#define M_CFR_CACHE_SEL (3 << S_CFR_CACHE_SEL)
#define B_CFR_CACHE_I   (1 << S_CFR_CACHE_SEL)
#define B_CFR_CACHE_D   (2 << S_CFR_CACHE_SEL)
#define B_CFR_CACHE_A   (3 << S_CFR_CACHE_SEL)
#define S_CFR_CACHE_SEL 0

/*********************************************************************************************************
  MSA0 MMU SSEG0 Config Register (cr30)

  31   29 28                   7  6   5   4   3   2   1   0
  +------+----------------------+---+---+---+---+---+---+---+
  |  BA  |      Reserved        | B | SO|SEC| C | D | V | 0 |
  +------+----------------------+---+---+---+---+---+---+---+
*********************************************************************************************************/

#define M_MSA0_BA       (0x7 << S_MSA0_BA)                              /*  BA-SSEG0 ӳ��������ַ     */
#define S_MSA0_BA       29
#define M_MSA0_B        (0x1 << S_MSA0_B)                               /*  ָʾ SSEG0 ���Ƿ�� Buffer  */
#define S_MSA0_B        6
#define M_MSA0_SO       (0x1 << S_MSA0_SO)                              /*  �Ƿ�ͳ������ķ���˳����ͬ  */
#define S_MSA0_SO       5 
#define M_MSA0_SEC      (0x1 << S_MSA0_SEC)                             /*  �Ƿ�֧�ְ�ȫ����            */
#define S_MSA0_SEC      4
#define M_MSA0_C        (0x1 << S_MSA0_C)                               /*  SSEG0 ���Ƿ�ɸ��ٻ���      */
#define S_MSA0_C        3        
#define M_MSA0_D        (0x1 << S_MSA0_D)                               /*  ָʾ SSEG0 ���Ƿ��д       */
#define S_MSA0_D        2            
#define M_MSA0_V        (0x1 << S_MSA0_V)                               /*  ָʾ SSEG0 ��ӳ���Ƿ���Ч   */
#define S_MSA0_V        1                                                                          

/*********************************************************************************************************
  MSA1 MMU SSEG1 Config Register (cr31)

  31   29 28                   7  6   5   4   3   2   1   0
  +------+----------------------+---+---+---+---+---+---+---+
  |  BA  |      Reserved        | B | SO|SEC| C | D | V | 0 |
  +------+----------------------+---+---+---+---+---+---+---+
*********************************************************************************************************/

#define M_MSA1_BA       (0x7 << S_MSA1_BA)                              /*  BA-SSEG1 ӳ��������ַ     */
#define S_MSA1_BA       29
#define M_MSA1_B        (0x1 << S_MSA1_B)                               /*  ָʾ SSEG1 ���Ƿ�� Buffer  */
#define S_MSA1_B        6
#define M_MSA1_SO       (0x1 << S_MSA1_SO)                              /*  �Ƿ�ͳ������ķ���˳����ͬ  */
#define S_MSA1_SO       5 
#define M_MSA1_SEC      (0x1 << S_MSA1_SEC)                             /*  �Ƿ�֧�ְ�ȫ����            */
#define S_MSA1_SEC      4
#define M_MSA1_C        (0x1 << S_MSA1_C)                               /*  SSEG1 ���Ƿ�ɸ��ٻ���      */
#define S_MSA1_C        3        
#define M_MSA1_D        (0x1 << S_MSA1_D)                               /*  ָʾ SSEG1 ���Ƿ��д       */
#define S_MSA1_D        2            
#define M_MSA1_V        (0x1 << S_MSA1_V)                               /*  ָʾ SSEG1 ��ӳ���Ƿ���Ч   */
#define S_MSA1_V        1                                                                          

/*********************************************************************************************************
  C-SKY CACHE ����
*********************************************************************************************************/

#define M_CACHE_CFG_WB  (0x1 << S_CACHE_CFG_WB)
#define S_CACHE_CFG_WB  4
#define M_CACHE_CFG_WA  (0x1 << S_CACHE_CFG_WA)
#define S_CACHE_CFG_WA  12

#endif                                                                  /*  __SYLIXOS_KERNEL            */
                                                                        /*  __ASSEMBLY__                */
#endif                                                                  /*  __CSKY_ARCH_DEF_H           */
/*********************************************************************************************************
  END
*********************************************************************************************************/
