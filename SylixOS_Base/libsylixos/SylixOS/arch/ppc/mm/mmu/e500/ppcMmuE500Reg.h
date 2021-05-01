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
** 文   件   名: ppcMmuE500Reg.h
**
** 创   建   人: Jiao.JinXing (焦进星)
**
** 文件创建日期: 2016 年 05 月 06 日
**
** 描        述: PowerPC E500 体系构架 MMU 寄存器函数库.
*********************************************************************************************************/

#ifndef __ARCH_PPCMMUE500REG_H
#define __ARCH_PPCMMUE500REG_H

/*********************************************************************************************************
  全局变量
*********************************************************************************************************/

extern BOOL                 _G_bMas2MBit;                               /*  多核一致性                  */
extern BOOL                 _G_bHasMAS7;                                /*  是否有 MAS7 寄存器          */
extern BOOL                 _G_bHasHID1;                                /*  是否有 HID1 寄存器          */

/*********************************************************************************************************
  定义
*********************************************************************************************************/

#define MMU_MAS2_M          _G_bMas2MBit                                /*  是否多核一致性              */
#define MMU_MAS4_X0D        0                                           /*  Implement depend page attr  */
#define MMU_MAS4_X1D        0                                           /*  Implement depend page attr  */

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
  PowerPC 执行汇编指令
*********************************************************************************************************/

#define PPC_EXEC_INS(ins)   __asm__ __volatile__ (ins)

/*********************************************************************************************************
  MAS1:
   0   1               8              16    19 20     24             31
  32  33  34          40              48    51 52     56             63
  +-+-----+-+-+-+-+-+-'-+-+-+-+-+-+-+-'-+-+-+-+-+-+-+-'-+-+-+-+-+-+-+-+
  |V|IPROT|           |   TID         |     |T| TSIZE |               |
  | |     |           |               |     |S|       |               |
  +-+-----+-----------'---------------'---------------'-+-+-+-+-+-+-+-+

  V     - TLB Valid bit
  IPROT - Invalidate protect
  TID   - Translation ID
  TS    - Translation space
  TSIZE - Translation size


  MAS2:
   0               8              16      20  22      25          31
  32              40              48      52  54      57          63
  +-+-+-+-+-+-+-+-'-+-+-+-+-+-+-+-'-+-+-+-+-+-+---+-'-+-+-+-+-+-+-+-+
  |                    EPN                |   |SHA|   |X|X|W|I|M|G|E|
  |                                       |   |REN|   |1|0| | | | | |
  +---------------'---------------'-------+-+-+---+-'-+-+-+-+-+-+-+-+

  EPN    - Effective (Virtual) address Page Number
  SHAREN - Enable cache fills to use shared cache state
  X0-X1  - Implementation dependant page attribute
  W      - Write-Through
  I      - Caching Inhibited
  M      - Memory Coherency required
  G      - Guarded
  E      - Endianess (Unused and set to zero.)


  MAS3:
   0               8              16      20  22      26          31
  32              40              48      52  54      58          63
  +-+-+-+-+-+-+-+-'-+-+-+-+-+-+-+-'-+-+-+-+-+-+-+-'-+-+-+-+-+-+-+-+
  |                    RPN                |   | U0-U3 |U|S|U|S|U|S|
  |                                       |   |       |X|X|W|W|R|R|
  +---------------'---------------'-------+-+-+-+-'-+-+-+-+-+-+-+-+

  RPN    - Real (Physical) address Page Number
  U0-U3  - User bits
  UX     - User execute permission
  SX     - Supervisor execute permission
  UW     - User write permission
  SW     - Supervisor write permission
  UR     - User read permission
  SR     - Supervisor read permission
*********************************************************************************************************/

typedef union {
    struct {
        UINT        MAS0_ucReserved0    :  2;
        UINT        MAS0_ucTLBSEL       :  2;
        UINT        MAS0_ucESEL         : 12;
        UINT        MAS0_ucReserved1    :  4;
        UINT        MAS0_usNV           : 12;
    };
    UINT32          MAS0_uiValue;
} MAS0_REG;                                                             /*  MAS0 寄存器                 */

typedef union {
    struct {
        UINT        MAS1_bVaild         :  1;                           /*  valid bit                   */
        UINT        MAS1_bIPROT         :  1;                           /*  invalidate protect bit      */
        UINT        MAS1_usTID          : 14;                           /*  translation ID              */
        UINT        MAS1_ucReserved0    :  3;                           /*  reserved                    */
        UINT        MAS1_bTS            :  1;                           /*  translation space           */
#define MMU_TRANS_SZ_4K     0x1                                         /*  4KB page size               */
#define MMU_TRANS_SZ_16K    0x2                                         /*  16KB page size              */
#define MMU_TRANS_SZ_64K    0x3                                         /*  64KB page size              */
#define MMU_TRANS_SZ_256K   0x4                                         /*  256KB page size             */
#define MMU_TRANS_SZ_1M     0x5                                         /*  1MB page size               */
#define MMU_TRANS_SZ_4M     0x6                                         /*  4MB page size               */
#define MMU_TRANS_SZ_16M    0x7                                         /*  16MB page size              */
#define MMU_TRANS_SZ_64M    0x8                                         /*  64MB page size              */
#define MMU_TRANS_SZ_256M   0x9                                         /*  256MB page size             */
#define MMU_TRANS_SZ_1G     0xa                                         /*  1GB page size               */
#define MMU_TRANS_SZ_4G     0xb                                         /*  4GB page size               */
        UINT        MAS1_ucTSIZE        :  4;                           /*  translation size            */
        UINT        MAS1_ucReserved1    :  8;                           /*  reserved                    */
    };
    UINT32          MAS1_uiValue;
} MAS1_REG;                                                             /*  MAS1 寄存器                 */

typedef union {
    struct {
        UINT        MAS2_uiEPN          : 20;                           /*  effective page number       */
        UINT        MAS2_ucReserved0    :  5;                           /*  reserved                    */
        UINT        MAS2_bX0            :  1;
        UINT        MAS2_bX1            :  1;
        UINT        MAS2_bWT            :  1;                           /*  write thru/back             */
        UINT        MAS2_bUnCache       :  1;                           /*  cache inhibited             */
        UINT        MAS2_bMemCoh        :  1;                           /*  memory coherent             */
        UINT        MAS2_bGuarded       :  1;                           /*  memory guarded              */
        UINT        MAS2_bLittleEndian  :  1;                           /*  little endian bit           */
    };
    UINT32          MAS2_uiValue;
} MAS2_REG;                                                             /*  MAS2 寄存器                 */

typedef LW_PTE_TRANSENTRY  MAS3_REG;                                    /*  MAS3 寄存器                 */

typedef union {
    struct {
        UINT        MAS4_ucReserved0    :  2;
        UINT        MAS4_ucTLBSELD      :  2;
        UINT        MAS4_usReserved1    : 16;
        UINT        MAS4_ucTSIZED       :  4;
        UINT        MAS4_bReserved2     :  1;
        UINT        MAS4_bX0D           :  1;
        UINT        MAS4_bX1D           :  1;
        UINT        MAS4_bWD            :  1;
        UINT        MAS4_bID            :  1;
        UINT        MAS4_bMD            :  1;
        UINT        MAS4_bGD            :  1;
        UINT        MAS4_bED            :  1;
    };
    UINT32          MAS4_uiValue;
} MAS4_REG;                                                             /*  MAS4 寄存器                 */

typedef union {
    struct {
        UINT        MAS6_ucReserved0    :  2;
        UINT        MAS6_usSPID         : 14;
        UINT        MAS6_usReserved1    : 15;
        UINT        MAS6_bSAS           :  1;
    };
    UINT32          MAS6_uiValue;
} MAS6_REG;                                                             /*  MAS6 寄存器                 */

typedef union {
    struct {
        UINT        MAS7_uiReserved0    : 28;
        UINT        MAS7_uiHigh4RPN     :  4;
    };
    UINT32          MAS7_uiValue;
} MAS7_REG;                                                             /*  MAS7 寄存器                 */

typedef union {
    struct {
        UINT        TLBCFG_ucASSOC      :  8;
        UINT        TLBCFG_ucMINSIZE    :  4;
        UINT        TLBCFG_ucMAXSIZE    :  4;
        UINT        TLBCFG_bIPROT       :  1;
        UINT        TLBCFG_bAVAIL       :  1;
        UINT        TLBCFG_ucReserved0  :  2;
        UINT        TLBCFG_usNENTRY     : 12;
    };
    UINT32          TLBCFG_uiValue;
} TLBCFG_REG;                                                           /*  TLBCFG 寄存器               */

typedef union {
    struct {
        UINT        MMUCFG_uiReserved0  : 17;
        UINT        MMUCFG_ucNPIDS      :  4;
        UINT        MMUCFG_ucPIDSIZE    :  5;
        UINT        MMUCFG_ucReserved1  :  2;
        UINT        MMUCFG_ucNTLBS      :  2;
        UINT        MMUCFG_ucMAVN       :  2;
    };
    UINT32          MMUCFG_uiValue;
} MMUCFG_REG;                                                           /*  MMUCFG 寄存器               */

UINT32  ppcE500MmuGetMMUCSR0(VOID);
VOID    ppcE500MmuSetMMUCSR0(UINT32  uiValue);

UINT32  ppcE500MmuGetMMUCFG(VOID);

UINT32  ppcE500MmuGetTLB0CFG(VOID);
UINT32  ppcE500MmuGetTLB1CFG(VOID);

UINT32  ppcE500MmuGetMAS0(VOID);
VOID    ppcE500MmuSetMAS0(UINT32  uiValue);

UINT32  ppcE500MmuGetMAS1(VOID);
VOID    ppcE500MmuSetMAS1(UINT32  uiValue);

UINT32  ppcE500MmuGetMAS2(VOID);
VOID    ppcE500MmuSetMAS2(UINT32  uiValue);

UINT32  ppcE500MmuGetMAS3(VOID);
VOID    ppcE500MmuSetMAS3(UINT32  uiValue);

UINT32  ppcE500MmuGetMAS4(VOID);
VOID    ppcE500MmuSetMAS4(UINT32  uiValue);

UINT32  ppcE500MmuGetMAS6(VOID);
VOID    ppcE500MmuSetMAS6(UINT32  uiValue);

UINT32  ppcE500MmuGetMAS7(VOID);
VOID    ppcE500MmuSetMAS7(UINT32  uiValue);

UINT32  ppcE500MmuGetPID0(VOID);
VOID    ppcE500MmuSetPID0(UINT32  uiValue);

UINT32  ppcE500MmuGetPID1(VOID);
VOID    ppcE500MmuSetPID1(UINT32  uiValue);

UINT32  ppcE500MmuGetPID2(VOID);
VOID    ppcE500MmuSetPID2(UINT32  uiValue);

VOID    ppcE500MmuInvalidateTLB(VOID);
VOID    ppcE500MmuInvalidateTLBEA(addr_t  ulAddr);

VOID    ppcE500MmuTLB1Invalidate(VOID);
VOID    ppcE500MmuTLB1InvalidateEA(addr_t  ulAddr);

#endif                                                                  /*  __ARCH_PPCMMUE500REG_H      */
/*********************************************************************************************************
  END
*********************************************************************************************************/
