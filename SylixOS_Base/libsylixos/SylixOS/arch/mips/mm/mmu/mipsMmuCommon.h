/*********************************************************************************************************
**
**                                    �й������Դ��֯
**
**                                   Ƕ��ʽʵʱ����ϵͳ
**
**                                       SylixOS(TM)
**
**                               Copyright  All Rights Reserved
**
**--------------�ļ���Ϣ--------------------------------------------------------------------------------
**
** ��   ��   ��: mipsMmuCommon.h
**
** ��   ��   ��: Jiao.JinXing (������)
**
** �ļ���������: 2015 �� 12 �� 25 ��
**
** ��        ��: MIPS ��ϵ���� MMU ͨ�ýӿ�.
*********************************************************************************************************/

#ifndef __ARCH_MIPSMMUCOMMON_H
#define __ARCH_MIPSMMUCOMMON_H

/*********************************************************************************************************
  ȫ�ֱ�������
*********************************************************************************************************/
extern BOOL                 _G_bMmuHasXI;                               /*  �Ƿ��� XI λ                */
extern UINT32               _G_uiMmuTlbSize;                            /*  TLB �����С                */
extern UINT32               _G_uiMmuEntryLoUnCache;                     /*  �Ǹ��ٻ���                  */
extern UINT32               _G_uiMmuEntryLoUnCacheWb;                   /*  �Ǹ��ٻ������(д����)      */
extern UINT32               _G_uiMmuEntryLoCache;                       /*  һ���Ը��ٻ���              */
/*********************************************************************************************************
  MMU ����
*********************************************************************************************************/
#define MIPS_MMU_HAS_XI                 _G_bMmuHasXI                    /*  �Ƿ��� XI λ                */
#define MIPS_MMU_TLB_SIZE               _G_uiMmuTlbSize                 /*  TLB �����С                */
#define MIPS_MMU_ENTRYLO_UNCACHE        _G_uiMmuEntryLoUnCache          /*  �Ǹ��ٻ���                  */
#define MIPS_MMU_ENTRYLO_UNCACHE_WB     _G_uiMmuEntryLoUnCacheWb        /*  �Ǹ��ٻ������(д����)      */
#define MIPS_MMU_ENTRYLO_CACHE          _G_uiMmuEntryLoCache            /*  һ���Ը��ٻ���              */
/*********************************************************************************************************
  PAGE ����
*********************************************************************************************************/
#if   LW_CFG_VMM_PAGE_SIZE == (4  * LW_CFG_KB_SIZE)
#define MIPS_MMU_PAGE_MASK              PM_4K
#elif LW_CFG_VMM_PAGE_SIZE == (16 * LW_CFG_KB_SIZE)
#define MIPS_MMU_PAGE_MASK              PM_16K
#elif LW_CFG_VMM_PAGE_SIZE == (64 * LW_CFG_KB_SIZE)
#define MIPS_MMU_PAGE_MASK              PM_64K
#elif LW_CFG_VMM_PAGE_SIZE == (256 * LW_CFG_KB_SIZE)
#define MIPS_MMU_PAGE_MASK              PM_256K
#else
#error  LW_CFG_VMM_PAGE_SIZE must be (4K, 16K, 64K, 256K)!
#endif                                                                  /*  LW_CFG_VMM_PAGE_SIZE        */
/*********************************************************************************************************
  TLB ����
*********************************************************************************************************/
#define MIPS_MMU_TLB_WRITE_INDEX()      tlb_write_indexed()             /*  дָ������ TLB              */
#define MIPS_MMU_TLB_WRITE_RANDOM()     tlb_write_random()              /*  д��� TLB                  */
#define MIPS_MMU_TLB_READ()             tlb_read()                      /*  �� TLB                      */
#define MIPS_MMU_TLB_PROBE()            tlb_probe()                     /*  ̽�� TLB                    */

VOID   mipsMmuInit(LW_MMU_OP  *pmmuop, CPCHAR  pcMachineName);
ULONG  mipsMmuTlbLoadStoreExcHandle(addr_t  ulAbortAddr, BOOL  bStore);

#endif                                                                  /*  __ARCH_MIPSMMUCOMMON_H      */
/*********************************************************************************************************
  END
*********************************************************************************************************/
