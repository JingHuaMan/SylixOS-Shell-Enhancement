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
** 文   件   名: arch_mmu.h
**
** 创   建   人: Jiao.JinXing (焦进星)
**
** 文件创建日期: 2015 年 11 月 26 日
**
** 描        述: PowerPC 内存管理相关.
*********************************************************************************************************/

#ifndef __PPC_ARCH_MMU_H
#define __PPC_ARCH_MMU_H

/*********************************************************************************************************
  L4 微内核虚拟机 MMU
*********************************************************************************************************/

#define LW_CFG_VMM_L4_HYPERVISOR_EN           0                         /*  是否使用 L4 虚拟机 MMU      */

/*********************************************************************************************************
  是否需要内核超过 3 级页表支持
*********************************************************************************************************/

#define LW_CFG_VMM_PAGE_4L_EN                 0                         /*  是否需要 4 级页表支持       */

/*********************************************************************************************************
  虚拟内存页表相关配置
*********************************************************************************************************/

#define LW_CFG_VMM_PAGE_SHIFT                 LW_CFG_PPC_PAGE_SHIFT     /*  2^n                         */
#define LW_CFG_VMM_PAGE_SIZE                  (1 << LW_CFG_VMM_PAGE_SHIFT)
#define LW_CFG_VMM_PAGE_MASK                  (~(LW_CFG_VMM_PAGE_SIZE - 1))

#define LW_CFG_VMM_PMD_SHIFT                  22                        /*  NO PMD same as PGD          */
#define LW_CFG_VMM_PMD_SIZE                   (1 << LW_CFG_VMM_PMD_SHIFT)
#define LW_CFG_VMM_PMD_MASK                   (~(LW_CFG_VMM_PMD_SIZE - 1))

#define LW_CFG_VMM_PGD_SHIFT                  22                        /*  2^22 = 4MB                  */
#define LW_CFG_VMM_PGD_SIZE                   (1 << LW_CFG_VMM_PGD_SHIFT)
#define LW_CFG_VMM_PGD_MASK                   (~(LW_CFG_VMM_PGD_SIZE - 1))

#if   LW_CFG_VMM_PAGE_SIZE == (4 * LW_CFG_KB_SIZE)
#define LW_CFG_VMM_PTE_MASK                   (0x3ff << LW_CFG_VMM_PAGE_SHIFT)
#elif LW_CFG_VMM_PAGE_SIZE == (16 * LW_CFG_KB_SIZE)
#define LW_CFG_VMM_PTE_MASK                   (0xff  << LW_CFG_VMM_PAGE_SHIFT)
#elif LW_CFG_VMM_PAGE_SIZE == (64 * LW_CFG_KB_SIZE)
#define LW_CFG_VMM_PTE_MASK                   (0x3f  << LW_CFG_VMM_PAGE_SHIFT)
#else
#error  LW_CFG_VMM_PAGE_SIZE must be (4K, 16K, 64K)!
#endif                                                                  /*  LW_CFG_VMM_PAGE_SIZE = 4KB  */

/*********************************************************************************************************
  物理内存分组数量
*********************************************************************************************************/
#ifdef __SYLIXOS_KERNEL

#define LW_CFG_VMM_ZONE_NUM                   8                         /*  物理分区数                  */
#define LW_CFG_VMM_VIR_NUM                    8                         /*  虚拟分区数                  */

/*********************************************************************************************************
  MMU 转换条目类型
*********************************************************************************************************/
#if !defined(__ASSEMBLY__) && !defined(ASSEMBLY)

typedef UINT32  LW_PGD_TRANSENTRY;                                      /*  页目录类型                  */
typedef UINT32  LW_PMD_TRANSENTRY;                                      /*  中间页目录类型              */

typedef union {
    struct {
        UINT        PTE_uiRPN       : 20;                               /*  物理页号                    */
#define PTE_bValid      PTE_bReserved0                                  /*  是否有效                    */
        UINT        PTE_bReserved0  :  1;                               /*  保留                        */
        UINT        PTE_bReserved1  :  1;                               /*  保留                        */
        UINT        PTE_bReserved2  :  1;                               /*  保留                        */
        UINT        PTE_bRef        :  1;                               /*  引用位                      */
        UINT        PTE_bChange     :  1;                               /*  修改位                      */
        UINT        PTE_ucWIMG      :  4;                               /*  内存和 CACHE 属性位         */
        UINT        PTE_bReserved3  :  1;                               /*  保留                        */
        UINT        PTE_ucPP        :  2;                               /*  页保护权限位                */
    };                                                                  /*  通用的 PPC32 PTE            */
    UINT32          PTE_uiValue;                                        /*  值                          */

    struct {
        /*
         * 以下值用于 TLB MISS 时重装到 MAS2 MAS3 MAS7 寄存器
         */
        UINT        MAS3_uiRPN      : 20;                               /*  物理页号                    */

        UINT        MAS3_bReserved0 :  1;                               /*  保留                        */
        UINT        MAS3_bReserved1 :  1;                               /*  保留                        */

        /*
         * 以下用户属性用于 TLB MISS 时重装到 MAS2 寄存器的 WIMG 域
         *
         * MAS2 寄存器还有 X0 X1 E 位, X0 X1 E 位由 MAS4 寄存器的 X0D X1D ED 自动重装
         */
#define MAS3_bGlobal     MAS3_bReserved0                                /*  是否全局映射                */
#define MAS3_bValid      MAS3_bReserved1                                /*  是否有效                    */
#define MAS3_bWT         MAS3_bUserAttr0                                /*  是否写穿透                  */
#define MAS3_bUnCache    MAS3_bUserAttr1                                /*  是否不可 CACHE              */
#define MAS3_bMemCoh     MAS3_bUserAttr2                                /*  是否多核内存一致性          */
#define MAS3_bGuarded    MAS3_bUserAttr3                                /*  是否阻止猜测访问            */

        UINT        MAS3_bUserAttr0 :  1;                               /*  用户属性 0                  */
        UINT        MAS3_bUserAttr1 :  1;                               /*  用户属性 1                  */
        UINT        MAS3_bUserAttr2 :  1;                               /*  用户属性 2                  */
        UINT        MAS3_bUserAttr3 :  1;                               /*  用户属性 3                  */

        UINT        MAS3_bUserExec  :  1;                               /*  用户态可执行权限            */
        UINT        MAS3_bSuperExec :  1;                               /*  内核态可执行权限            */

        UINT        MAS3_bUserWrite :  1;                               /*  用户态可写权限              */
        UINT        MAS3_bSuperWrite:  1;                               /*  内核态可写权限              */

        UINT        MAS3_bUserRead  :  1;                               /*  用户态可读权限              */
        UINT        MAS3_bSuperRead :  1;                               /*  内核态可读权限              */

#if LW_CFG_CPU_PHYS_ADDR_64BIT > 0
        UINT        MAS7_uiReserved0: 28;                               /*  保留                        */
        UINT        MAS7_uiHigh4RPN :  4;                               /*  高 4 位物理页号             */
#endif                                                                  /*  LW_CFG_CPU_PHYS_ADDR_64BIT>0*/
    };                                                                  /*  E500 PTE                    */

    struct {
        UINT32      MAS3_uiValue;                                       /*  MAS3 值                     */
#if LW_CFG_CPU_PHYS_ADDR_64BIT > 0
        UINT32      MAS7_uiValue;                                       /*  MAS7 值                     */
#endif                                                                  /*  LW_CFG_CPU_PHYS_ADDR_64BIT>0*/
    };

    struct {
        /*
         * 以下值用于 TLB MISS 时重装到 TLB WORD0 WORD1 WORD2 寄存器
         */
        UINT        WORD1_uiRPN      : 20;                              /*  物理页号                    */
        UINT        WORD0_bValid     :  1;                              /*  是否有效                    */
        UINT        WORD2_bGlobal    :  1;                              /*  是否全局映射                */

        UINT        WORD2_bWT        :  1;                              /*  是否写穿透                  */
        UINT        WORD2_bUnCache   :  1;                              /*  是否不可 CACHE              */
        UINT        WORD2_bMemConh   :  1;                              /*  是否多核内存一致性          */
        UINT        WORD2_bGuarded   :  1;                              /*  是否阻止猜测访问            */

        UINT        WORD2_bUserExec  :  1;                              /*  用户态可执行权限            */
        UINT        WORD2_bUserWrite :  1;                              /*  用户态可写权限              */
        UINT        WORD2_bUserRead  :  1;                              /*  用户态可读权限              */
        UINT        WORD2_bSuperExec :  1;                              /*  内核态可执行权限            */
        UINT        WORD2_bSuperWrite:  1;                              /*  内核态可写权限              */
        UINT        WORD2_bSuperRead :  1;                              /*  内核态可读权限              */
    };                                                                  /*  PPC460 PTE                  */
    UINT32          WORD_uiValue;                                       /*  WORD0 WORD1 WORD2 值        */

} LW_PTE_TRANSENTRY;                                                    /*  页表条目类型                */

/*********************************************************************************************************
  PowerPC 固定 TLB 条目映射描述
*********************************************************************************************************/

typedef struct {
    UINT64      FTLBD_ui64PhyAddr;                                      /*  物理地址 (页对齐地址)       */
    ULONG       FTLBD_ulVirMap;                                         /*  需要初始化的映射关系        */
    ULONG       FTLBD_stSize;                                           /*  物理内存区长度 (页对齐长度) */
    ULONG       FTLBD_ulFlag;                                           /*  物理内存区间类型            */

#define PPC_FTLB_FLAG_VALID             0x01                            /*  映射有效                    */
#define PPC_FTLB_FLAG_UNVALID           0x00                            /*  映射无效                    */

#define PPC_FTLB_FLAG_ACCESS            0x02                            /*  可以访问                    */
#define PPC_FTLB_FLAG_UNACCESS          0x00                            /*  不能访问                    */

#define PPC_FTLB_FLAG_WRITABLE          0x04                            /*  可以写操作                  */
#define PPC_FTLB_FLAG_UNWRITABLE        0x00                            /*  不可以写操作                */

#define PPC_FTLB_FLAG_EXECABLE          0x08                            /*  可以执行代码                */
#define PPC_FTLB_FLAG_UNEXECABLE        0x00                            /*  不可以执行代码              */

#define PPC_FTLB_FLAG_CACHEABLE         0x10                            /*  可以 CACHE Writeback        */
#define PPC_FTLB_FLAG_UNCACHEABLE       0x00                            /*  不可以 CACHE Writeback      */

#define PPC_FTLB_FLAG_WRITETHROUGH      0x20                            /*  可以 CACHE Writethrough     */
#define PPC_FTLB_FLAG_UNWRITETHROUGH    0x00                            /*  不可以 CACHE Writethrough   */

#define PPC_FTLB_FLAG_GUARDED           0x40                            /*  阻止猜测访问                */
#define PPC_FTLB_FLAG_UNGUARDED         0x00                            /*  不阻止猜测访问              */

#define PPC_FTLB_FLAG_TEMP              0x80                            /*  临时映射                    */

#define PPC_FTLB_FLAG_MEM               (PPC_FTLB_FLAG_VALID     | \
                                         PPC_FTLB_FLAG_ACCESS    | \
                                         PPC_FTLB_FLAG_WRITABLE  | \
                                         PPC_FTLB_FLAG_EXECABLE  | \
                                         PPC_FTLB_FLAG_CACHEABLE)       /*  普通内存                    */

#define PPC_FTLB_FLAG_BOOTSFR           (PPC_FTLB_FLAG_VALID      | \
                                         PPC_FTLB_FLAG_GUARDED    | \
                                         PPC_FTLB_FLAG_ACCESS     | \
                                         PPC_FTLB_FLAG_WRITABLE)        /*  特殊功能寄存器, FLASH       */

/*********************************************************************************************************
  向前兼容老的 E500 BSP
*********************************************************************************************************/

#define TLB1D_ui64PhyAddr               FTLBD_ui64PhyAddr
#define TLB1D_ulVirMap                  FTLBD_ulVirMap
#define TLB1D_stSize                    FTLBD_stSize
#define TLB1D_ulFlag                    FTLBD_ulFlag

#define E500_TLB1_FLAG_VALID            PPC_FTLB_FLAG_VALID
#define E500_TLB1_FLAG_UNVALID          PPC_FTLB_FLAG_UNVALID

#define E500_TLB1_FLAG_ACCESS           PPC_FTLB_FLAG_ACCESS
#define E500_TLB1_FLAG_UNACCESS         PPC_FTLB_FLAG_UNACCESS

#define E500_TLB1_FLAG_WRITABLE         PPC_FTLB_FLAG_WRITABLE
#define E500_TLB1_FLAG_UNWRITABLE       PPC_FTLB_FLAG_UNWRITABLE

#define E500_TLB1_FLAG_EXECABLE         PPC_FTLB_FLAG_EXECABLE
#define E500_TLB1_FLAG_UNEXECABLE       PPC_FTLB_FLAG_UNEXECABLE

#define E500_TLB1_FLAG_CACHEABLE        PPC_FTLB_FLAG_CACHEABLE
#define E500_TLB1_FLAG_UNCACHEABLE      PPC_FTLB_FLAG_UNCACHEABLE

#define E500_TLB1_FLAG_WRITETHROUGH     PPC_FTLB_FLAG_WRITETHROUGH
#define E500_TLB1_FLAG_UNWRITETHROUGH   PPC_FTLB_FLAG_UNWRITETHROUGH

#define E500_TLB1_FLAG_GUARDED          PPC_FTLB_FLAG_GUARDED
#define E500_TLB1_FLAG_UNGUARDED        PPC_FTLB_FLAG_UNGUARDED

#define E500_TLB1_FLAG_TEMP             PPC_FTLB_FLAG_TEMP

#define E500_TLB1_FLAG_MEM              PPC_FTLB_FLAG_MEM
#define E500_TLB1_FLAG_BOOTSFR          PPC_FTLB_FLAG_BOOTSFR
} PPC_FTLB_MAP_DESC, E500_TLB1_MAP_DESC;

typedef PPC_FTLB_MAP_DESC        *PPPC_FTLB_MAP_DESC;
typedef E500_TLB1_MAP_DESC       *PE500_TLB1_MAP_DESC;

/*********************************************************************************************************
  E500 PPC460 BSP 需要调用以下函数初始化固定 TLB 条目
*********************************************************************************************************/

INT  archE500MmuTLB1GlobalMap(CPCHAR               pcMachineName,
                              PE500_TLB1_MAP_DESC  pdesc,
                              VOID               (*pfuncPreRemoveTempMap)(VOID));

INT  arch460MmuFTLBGlobalMap(CPCHAR                pcMachineName,
                             PPPC_FTLB_MAP_DESC    pdesc,
                             VOID                (*pfuncPreRemoveTempMap)(VOID));

#endif
#endif                                                                  /*  __SYLIXOS_KERNEL            */
#endif                                                                  /*  __PPC_ARCH_MMU_H            */
/*********************************************************************************************************
  END
*********************************************************************************************************/
