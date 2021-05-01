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
** ��   ��   ��: arch_mmu.h
**
** ��   ��   ��: Jiao.JinXing (������)
**
** �ļ���������: 2015 �� 11 �� 26 ��
**
** ��        ��: PowerPC �ڴ�������.
*********************************************************************************************************/

#ifndef __PPC_ARCH_MMU_H
#define __PPC_ARCH_MMU_H

/*********************************************************************************************************
  L4 ΢�ں������ MMU
*********************************************************************************************************/

#define LW_CFG_VMM_L4_HYPERVISOR_EN           0                         /*  �Ƿ�ʹ�� L4 ����� MMU      */

/*********************************************************************************************************
  �Ƿ���Ҫ�ں˳��� 3 ��ҳ��֧��
*********************************************************************************************************/

#define LW_CFG_VMM_PAGE_4L_EN                 0                         /*  �Ƿ���Ҫ 4 ��ҳ��֧��       */

/*********************************************************************************************************
  �����ڴ�ҳ���������
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
  �����ڴ��������
*********************************************************************************************************/
#ifdef __SYLIXOS_KERNEL

#define LW_CFG_VMM_ZONE_NUM                   8                         /*  ���������                  */
#define LW_CFG_VMM_VIR_NUM                    8                         /*  ���������                  */

/*********************************************************************************************************
  MMU ת����Ŀ����
*********************************************************************************************************/
#if !defined(__ASSEMBLY__) && !defined(ASSEMBLY)

typedef UINT32  LW_PGD_TRANSENTRY;                                      /*  ҳĿ¼����                  */
typedef UINT32  LW_PMD_TRANSENTRY;                                      /*  �м�ҳĿ¼����              */

typedef union {
    struct {
        UINT        PTE_uiRPN       : 20;                               /*  ����ҳ��                    */
#define PTE_bValid      PTE_bReserved0                                  /*  �Ƿ���Ч                    */
        UINT        PTE_bReserved0  :  1;                               /*  ����                        */
        UINT        PTE_bReserved1  :  1;                               /*  ����                        */
        UINT        PTE_bReserved2  :  1;                               /*  ����                        */
        UINT        PTE_bRef        :  1;                               /*  ����λ                      */
        UINT        PTE_bChange     :  1;                               /*  �޸�λ                      */
        UINT        PTE_ucWIMG      :  4;                               /*  �ڴ�� CACHE ����λ         */
        UINT        PTE_bReserved3  :  1;                               /*  ����                        */
        UINT        PTE_ucPP        :  2;                               /*  ҳ����Ȩ��λ                */
    };                                                                  /*  ͨ�õ� PPC32 PTE            */
    UINT32          PTE_uiValue;                                        /*  ֵ                          */

    struct {
        /*
         * ����ֵ���� TLB MISS ʱ��װ�� MAS2 MAS3 MAS7 �Ĵ���
         */
        UINT        MAS3_uiRPN      : 20;                               /*  ����ҳ��                    */

        UINT        MAS3_bReserved0 :  1;                               /*  ����                        */
        UINT        MAS3_bReserved1 :  1;                               /*  ����                        */

        /*
         * �����û��������� TLB MISS ʱ��װ�� MAS2 �Ĵ����� WIMG ��
         *
         * MAS2 �Ĵ������� X0 X1 E λ, X0 X1 E λ�� MAS4 �Ĵ����� X0D X1D ED �Զ���װ
         */
#define MAS3_bGlobal     MAS3_bReserved0                                /*  �Ƿ�ȫ��ӳ��                */
#define MAS3_bValid      MAS3_bReserved1                                /*  �Ƿ���Ч                    */
#define MAS3_bWT         MAS3_bUserAttr0                                /*  �Ƿ�д��͸                  */
#define MAS3_bUnCache    MAS3_bUserAttr1                                /*  �Ƿ񲻿� CACHE              */
#define MAS3_bMemCoh     MAS3_bUserAttr2                                /*  �Ƿ����ڴ�һ����          */
#define MAS3_bGuarded    MAS3_bUserAttr3                                /*  �Ƿ���ֹ�²����            */

        UINT        MAS3_bUserAttr0 :  1;                               /*  �û����� 0                  */
        UINT        MAS3_bUserAttr1 :  1;                               /*  �û����� 1                  */
        UINT        MAS3_bUserAttr2 :  1;                               /*  �û����� 2                  */
        UINT        MAS3_bUserAttr3 :  1;                               /*  �û����� 3                  */

        UINT        MAS3_bUserExec  :  1;                               /*  �û�̬��ִ��Ȩ��            */
        UINT        MAS3_bSuperExec :  1;                               /*  �ں�̬��ִ��Ȩ��            */

        UINT        MAS3_bUserWrite :  1;                               /*  �û�̬��дȨ��              */
        UINT        MAS3_bSuperWrite:  1;                               /*  �ں�̬��дȨ��              */

        UINT        MAS3_bUserRead  :  1;                               /*  �û�̬�ɶ�Ȩ��              */
        UINT        MAS3_bSuperRead :  1;                               /*  �ں�̬�ɶ�Ȩ��              */

#if LW_CFG_CPU_PHYS_ADDR_64BIT > 0
        UINT        MAS7_uiReserved0: 28;                               /*  ����                        */
        UINT        MAS7_uiHigh4RPN :  4;                               /*  �� 4 λ����ҳ��             */
#endif                                                                  /*  LW_CFG_CPU_PHYS_ADDR_64BIT>0*/
    };                                                                  /*  E500 PTE                    */

    struct {
        UINT32      MAS3_uiValue;                                       /*  MAS3 ֵ                     */
#if LW_CFG_CPU_PHYS_ADDR_64BIT > 0
        UINT32      MAS7_uiValue;                                       /*  MAS7 ֵ                     */
#endif                                                                  /*  LW_CFG_CPU_PHYS_ADDR_64BIT>0*/
    };

    struct {
        /*
         * ����ֵ���� TLB MISS ʱ��װ�� TLB WORD0 WORD1 WORD2 �Ĵ���
         */
        UINT        WORD1_uiRPN      : 20;                              /*  ����ҳ��                    */
        UINT        WORD0_bValid     :  1;                              /*  �Ƿ���Ч                    */
        UINT        WORD2_bGlobal    :  1;                              /*  �Ƿ�ȫ��ӳ��                */

        UINT        WORD2_bWT        :  1;                              /*  �Ƿ�д��͸                  */
        UINT        WORD2_bUnCache   :  1;                              /*  �Ƿ񲻿� CACHE              */
        UINT        WORD2_bMemConh   :  1;                              /*  �Ƿ����ڴ�һ����          */
        UINT        WORD2_bGuarded   :  1;                              /*  �Ƿ���ֹ�²����            */

        UINT        WORD2_bUserExec  :  1;                              /*  �û�̬��ִ��Ȩ��            */
        UINT        WORD2_bUserWrite :  1;                              /*  �û�̬��дȨ��              */
        UINT        WORD2_bUserRead  :  1;                              /*  �û�̬�ɶ�Ȩ��              */
        UINT        WORD2_bSuperExec :  1;                              /*  �ں�̬��ִ��Ȩ��            */
        UINT        WORD2_bSuperWrite:  1;                              /*  �ں�̬��дȨ��              */
        UINT        WORD2_bSuperRead :  1;                              /*  �ں�̬�ɶ�Ȩ��              */
    };                                                                  /*  PPC460 PTE                  */
    UINT32          WORD_uiValue;                                       /*  WORD0 WORD1 WORD2 ֵ        */

} LW_PTE_TRANSENTRY;                                                    /*  ҳ����Ŀ����                */

/*********************************************************************************************************
  PowerPC �̶� TLB ��Ŀӳ������
*********************************************************************************************************/

typedef struct {
    UINT64      FTLBD_ui64PhyAddr;                                      /*  �����ַ (ҳ�����ַ)       */
    ULONG       FTLBD_ulVirMap;                                         /*  ��Ҫ��ʼ����ӳ���ϵ        */
    ULONG       FTLBD_stSize;                                           /*  �����ڴ������� (ҳ���볤��) */
    ULONG       FTLBD_ulFlag;                                           /*  �����ڴ���������            */

#define PPC_FTLB_FLAG_VALID             0x01                            /*  ӳ����Ч                    */
#define PPC_FTLB_FLAG_UNVALID           0x00                            /*  ӳ����Ч                    */

#define PPC_FTLB_FLAG_ACCESS            0x02                            /*  ���Է���                    */
#define PPC_FTLB_FLAG_UNACCESS          0x00                            /*  ���ܷ���                    */

#define PPC_FTLB_FLAG_WRITABLE          0x04                            /*  ����д����                  */
#define PPC_FTLB_FLAG_UNWRITABLE        0x00                            /*  ������д����                */

#define PPC_FTLB_FLAG_EXECABLE          0x08                            /*  ����ִ�д���                */
#define PPC_FTLB_FLAG_UNEXECABLE        0x00                            /*  ������ִ�д���              */

#define PPC_FTLB_FLAG_CACHEABLE         0x10                            /*  ���� CACHE Writeback        */
#define PPC_FTLB_FLAG_UNCACHEABLE       0x00                            /*  ������ CACHE Writeback      */

#define PPC_FTLB_FLAG_WRITETHROUGH      0x20                            /*  ���� CACHE Writethrough     */
#define PPC_FTLB_FLAG_UNWRITETHROUGH    0x00                            /*  ������ CACHE Writethrough   */

#define PPC_FTLB_FLAG_GUARDED           0x40                            /*  ��ֹ�²����                */
#define PPC_FTLB_FLAG_UNGUARDED         0x00                            /*  ����ֹ�²����              */

#define PPC_FTLB_FLAG_TEMP              0x80                            /*  ��ʱӳ��                    */

#define PPC_FTLB_FLAG_MEM               (PPC_FTLB_FLAG_VALID     | \
                                         PPC_FTLB_FLAG_ACCESS    | \
                                         PPC_FTLB_FLAG_WRITABLE  | \
                                         PPC_FTLB_FLAG_EXECABLE  | \
                                         PPC_FTLB_FLAG_CACHEABLE)       /*  ��ͨ�ڴ�                    */

#define PPC_FTLB_FLAG_BOOTSFR           (PPC_FTLB_FLAG_VALID      | \
                                         PPC_FTLB_FLAG_GUARDED    | \
                                         PPC_FTLB_FLAG_ACCESS     | \
                                         PPC_FTLB_FLAG_WRITABLE)        /*  ���⹦�ܼĴ���, FLASH       */

/*********************************************************************************************************
  ��ǰ�����ϵ� E500 BSP
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
  E500 PPC460 BSP ��Ҫ�������º�����ʼ���̶� TLB ��Ŀ
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
