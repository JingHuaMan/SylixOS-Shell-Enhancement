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
** ��   ��   ��: Xu.Guizhou (�����)
**
** �ļ���������: 2017 �� 05 �� 15 ��
**
** ��        ��: SPARC ��ض���.
*********************************************************************************************************/

#ifndef __SPARC_ARCH_DEF_H
#define __SPARC_ARCH_DEF_H

#include "arch_asi.h"
#include "arch_leon.h"

/*********************************************************************************************************
  PSR CWP MASK
*********************************************************************************************************/
#if defined(__SYLIXOS_KERNEL) || defined(__ASSEMBLY__) || defined(ASSEMBLY)

#if (LW_CFG_SPARC_REG_WIN_NR == 8)
#define SPARC_PSR_CWP_MASK                0x07                         /*  bits  0 -  4                 */
#elif (LW_CFG_SPARC_REG_WIN_NR == 16)
#define SPARC_PSR_CWP_MASK                0x0f                         /*  bits  0 -  4                 */
#elif (LW_CFG_SPARC_REG_WIN_NR == 32)
#define SPARC_PSR_CWP_MASK                0x1f                         /*  bits  0 -  4                 */
#else
#error "Unsupported number of register windows for this cpu"
#endif

/*********************************************************************************************************
  PSR �Ĵ�������
  ------------------------------------------------------------------------
  | impl  | vers  | icc   | resv  | EC | EF | PIL  | S | PS | ET |  CWP  |
  | 31-28 | 27-24 | 23-20 | 19-14 | 13 | 12 | 11-8 | 7 | 6  | 5  |  4-0  |
  ------------------------------------------------------------------------
*********************************************************************************************************/

#define PSR_CWP                 0x0000001f                              /*  current window pointer      */
#define PSR_ET                  0x00000020                              /*  enable traps field          */
#define PSR_PS                  0x00000040                              /*  previous privilege level    */
#define PSR_S                   0x00000080                              /*  current privilege level     */
#define PSR_PIL                 0x00000f00                              /*  processor interrupt level   */
#define PSR_EF                  0x00001000                              /*  enable floating point       */
#define PSR_EC                  0x00002000                              /*  enable co-processor         */
#define PSR_SYSCALL             0x00004000                              /*  inside of a syscall         */
#define PSR_LE                  0x00008000                              /*  SuperSparcII little-endian  */
#define PSR_ICC                 0x00f00000                              /*  integer condition codes     */
#define PSR_C                   0x00100000                              /*  carry bit                   */
#define PSR_V                   0x00200000                              /*  overflow bit                */
#define PSR_Z                   0x00400000                              /*  zero bit                    */
#define PSR_N                   0x00800000                              /*  negative bit                */
#define PSR_VERS                0x0f000000                              /*  cpu-version field           */
#define PSR_IMPL                0xf0000000                              /*  cpu-implementation field    */

#define PSR_VERS_SHIFT          24
#define PSR_IMPL_SHIFT          28
#define PSR_VERS_SHIFTED_MASK   0xf
#define PSR_IMPL_SHIFTED_MASK   0xf

#define PSR_IMPL_TI             0x4
#define PSR_IMPL_LEON           0xf

/*********************************************************************************************************
  TBR �Ĵ�������
  -------------------------------------------------------------------
  |                  TBA                    |       TT      |  zero |
  |                 31-12                   |      11-4     |  3-0  |
  -------------------------------------------------------------------
*********************************************************************************************************/

#define TBR_TT_MASK             0x00000ff0
#define TBR_TT_OFFSET           4

#define TBR_TBA_MASK            0xfffff000
#define TBR_TBA_OFFSET          12

#define TT_INTERRUPT_LEVEL_0    (0x10)                                  /*  Not used                    */
#define TRAP_TO_IRQ(trap)       ((trap) - TT_INTERRUPT_LEVEL_0)         /*  Trap number to IRQ number   */

#endif                                                                  /*  __SYLIXOS_KERNEL            */
#endif                                                                  /*  __SPARC_ARCH_DEF_H          */
/*********************************************************************************************************
  END
*********************************************************************************************************/
