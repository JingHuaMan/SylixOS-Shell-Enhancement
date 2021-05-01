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
** 文   件   名: ppcMmu460Reg.h
**
** 创   建   人: Jiao.JinXing (焦进星)
**
** 文件创建日期: 2019 年 08 月 14 日
**
** 描        述: PowerPC 460 体系构架 MMU 寄存器函数库.
*********************************************************************************************************/

#ifndef __ARCH_PPCMMU460REG_H
#define __ARCH_PPCMMU460REG_H

/*********************************************************************************************************
  460 的 MMU 关不掉, 并且没有经典的 BAT 或 E500 的 TLB1 这种能固定映射的方法,
  这里使用前面几个 TLB 来做固定映射, TLB MISS 时不会替换掉前面几个固定的 TLB
*********************************************************************************************************/

#define PPC460_FTLB_SIZE    12
#define PPC460_TLB_BASE     PPC460_FTLB_SIZE
#define PPC460_TLB_SIZE     64

/*********************************************************************************************************
  MMUCR
*********************************************************************************************************/

#define PPC460_MMUCR        0x3b2

/*********************************************************************************************************
  MMUCR
*********************************************************************************************************/

#define MMU_RPN_SHIFT       12
#define MMU_RPN_BITS        20

#if   LW_CFG_VMM_PAGE_SIZE == (4 * LW_CFG_KB_SIZE)
#define MMU_RPN_MASK        0xfffff
#define MMU_TSIZED          MMU_TRANS_SZ_4K
#elif LW_CFG_VMM_PAGE_SIZE == (16 * LW_CFG_KB_SIZE)
#define MMU_RPN_MASK        0xffffc
#define MMU_TSIZED          MMU_TRANS_SZ_16K
#elif LW_CFG_VMM_PAGE_SIZE == (64 * LW_CFG_KB_SIZE)
#define MMU_RPN_MASK        0xffff0
#define MMU_TSIZED          MMU_TRANS_SZ_64K
#endif                                                                  /*  LW_CFG_VMM_PAGE_SIZE = 4KB  */

/*********************************************************************************************************
  TLB 配置字
*********************************************************************************************************/
#if !defined(__ASSEMBLY__) && !defined(ASSEMBLY)

typedef union {
    struct {
        UINT    WORD0_uiEPN         :  20;                              /*  effective page number       */
        UINT    WORD0_ucReserved    :   2;                              /*  reserved                    */
        UINT    WORD0_bValid        :   1;                              /*  valid bit                   */
        UINT    WORD0_bTS           :   1;                              /*  translation space bit       */
#define MMU_TRANS_SZ_4K     0x1                                         /*  4KB page size               */
#define MMU_TRANS_SZ_16K    0x2                                         /*  16KB page size              */
#define MMU_TRANS_SZ_64K    0x3                                         /*  64KB page size              */
#define MMU_TRANS_SZ_256K   0x4                                         /*  256KB page size             */
#define MMU_TRANS_SZ_1M     0x5                                         /*  1MB page size               */
#define MMU_TRANS_SZ_16M    0x7                                         /*  16MB page size              */
#define MMU_TRANS_SZ_256M   0x9                                         /*  256MB page size             */
#define MMU_TRANS_SZ_1G     0xa                                         /*  1GB page size               */
        UINT    WORD0_ucSIZE        :   4;                              /*  page size                   */
        UINT    WORD0_ucTPAR        :   4;                              /*  tag parity                  */
    };
    UINT32      WORD0_uiValue;
} TLB_WORD0;

typedef union {
    struct {
        UINT    WORD1_uiRPN         :  20;                              /*  real page number            */
        UINT    WORD1_ucReserved0   :   2;                              /*  reserved                    */
        UINT    WORD1_ucPAR1        :   2;                              /*  data parity                 */
        UINT    WORD1_ucReserved1   :   4;                              /*  reserved                    */
        UINT    WORD1_ucERPN        :   4;                              /*  extended real page number   */
    };
    UINT32      WORD1_uiValue;
} TLB_WORD1;

typedef union {
    struct {
        UINT    WORD2_ucPAR2        :   2;                              /*  data parity                 */
        UINT    WORD2_usReserved0   :  14;                              /*  reserved                    */
        UINT    WORD2_bUser0        :   1;                              /*  user attribute 0            */
        UINT    WORD2_bUser1        :   1;                              /*  user attribute 1            */
        UINT    WORD2_bUser2        :   1;                              /*  user attribute 2            */
        UINT    WORD2_bUser3        :   1;                              /*  user attribute 3            */
        UINT    WORD2_bWT           :   1;                              /*  write through/copy back     */
        UINT    WORD2_bUnCache      :   1;                              /*  cache inhibited             */
        UINT    WORD2_bMemCoh       :   1;                              /*  memory coherent             */
        UINT    WORD2_bGuarded      :   1;                              /*  memory guarded              */
        UINT    WORD2_bLittleEndian :   1;                              /*  little endian bit           */
        UINT    WORD2_bReserved1    :   1;                              /*  reserved                    */
        UINT    WORD2_bUserExec     :   1;                              /*  user execute protection     */
        UINT    WORD2_bUserWrite    :   1;                              /*  user write protection       */
        UINT    WORD2_bUserRead     :   1;                              /*  user read protection        */
        UINT    WORD2_bSuperExec    :   1;                              /*  supervisor execute          */
        UINT    WORD2_bSuperWrite   :   1;                              /*  supervisor write protection */
        UINT    WORD2_bSuperRead    :   1;                              /*  supervisor read protection  */
    };
    UINT32      WORD2_uiValue;
} TLB_WORD2;

UINT32  ppc460MmuGetMMUCR(VOID);
VOID    ppc460MmuSetMMUCR(UINT32  uiValue);

UINT32  ppc460MmuGetPID(VOID);
VOID    ppc460MmuSetPID(UINT32  uiValue);

#endif                                                                  /*  !defined(__ASSEMBLY__)      */
#endif                                                                  /*  __ARCH_PPCMMU460REG_H       */
/*********************************************************************************************************
  END
*********************************************************************************************************/
