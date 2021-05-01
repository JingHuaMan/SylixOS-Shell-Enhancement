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
** 文   件   名: ppcMmu460FTlb.c
**
** 创   建   人: Jiao.JinXing (焦进星)
**
** 文件创建日期: 2019 年 08 月 14 日
**
** 描        述: PowerPC 460 体系构架 MMU 固定 TLB 函数库.
**
** 注        意: 不依赖于任何操作系统服务, 可以在操作系统初始化前调用.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "SylixOS.h"
/*********************************************************************************************************
  裁剪支持
*********************************************************************************************************/
#if LW_CFG_VMM_EN > 0
#include "arch/ppc/arch_e500.h"
#include "arch/ppc/common/e500/ppcSprE500.h"
#include "./ppcMmu460Reg.h"
#include "alloca.h"
/*********************************************************************************************************
  TLB 配置字
*********************************************************************************************************/
typedef struct {
    ULONG       FTLB_ulFlag;                                            /*  映射标志                    */
    TLB_WORD0   FTLB_uiWord0;                                           /*  tlb word0                   */
    TLB_WORD1   FTLB_uiWord1;                                           /*  tlb word1                   */
    TLB_WORD2   FTLB_uiWord2;                                           /*  tlb word2                   */
} TLB_WORDS;
/*********************************************************************************************************
** 函数名称: arch460MmuFTLBGlobalMap
** 功能描述: 460 MMU 固定 TLB 条目全局映射
** 输　入  : pcMachineName          使用的机器名称
**           pdesc                  映射关系数组
**           pfuncPreRemoveTempMap  移除临时映射前的回调函数
** 输　出  : ERROR or OK
** 全局变量:
** 调用模块:
** 注  意  : 必须在关中断情况下调用
*********************************************************************************************************/
INT  arch460MmuFTLBGlobalMap (CPCHAR                pcMachineName,
                              PPPC_FTLB_MAP_DESC    pdesc,
                              VOID                (*pfuncPreRemoveTempMap)(VOID))
{
    PPC_FTLB_MAP_DESC       desc;
    TLB_WORD0               uiWord0;
    TLB_WORD1               uiWord1;
    TLB_WORD2               uiWord2;
    UINT                    i;
    size_t                  stRemainSize;
    TLB_WORDS              *tlbWords;

    if (!pdesc) {
        return  (PX_ERROR);
    }

    ppc460MmuSetPID(0);                                                 /*  设置 PID:0                  */
    ppc460MmuSetMMUCR(0);                                               /*  设置 MMUCR(STID:0, STS:0)   */

    /*
     * 第一步: 分析物理内存信息描述
     */
    tlbWords = (TLB_WORDS *)alloca(sizeof(TLB_WORDS)*PPC460_FTLB_SIZE); /*  从栈里分配                  */
    lib_bzero(tlbWords, sizeof(TLB_WORDS) * PPC460_FTLB_SIZE);

    desc         = *pdesc;                                              /*  从第一个开始分析            */
    stRemainSize = desc.FTLBD_stSize;

    for (i = 0; (i < PPC460_FTLB_SIZE) && stRemainSize;) {
        if (!(desc.FTLBD_ulFlag & PPC_FTLB_FLAG_VALID)) {               /*  无效的映射关系              */
            pdesc++;
            desc         = *pdesc;
            stRemainSize = desc.FTLBD_stSize;
            continue;
        }

        /*
         * WORD0
         */
        uiWord0.WORD0_uiValue = 0;
        uiWord0.WORD0_bValid  = LW_TRUE;                                /*   TLB 有效                   */
        uiWord0.WORD0_bTS     = 0;                                      /*   地址空间 0                 */
        uiWord0.WORD0_uiEPN   = desc.FTLBD_ulVirMap >> MMU_RPN_SHIFT;

        if ((desc.FTLBD_stSize >= 1 * LW_CFG_GB_SIZE) &&
           !(desc.FTLBD_ui64PhyAddr & (1 * LW_CFG_GB_SIZE - 1))) {
            desc.FTLBD_stSize    = 1 * LW_CFG_GB_SIZE;
            uiWord0.WORD0_ucSIZE = MMU_TRANS_SZ_1G;

        } else if ((desc.FTLBD_stSize >= 256 * LW_CFG_MB_SIZE) &&
                  !(desc.FTLBD_ui64PhyAddr & (256 * LW_CFG_MB_SIZE - 1))) {
            desc.FTLBD_stSize    = 256 * LW_CFG_MB_SIZE;
            uiWord0.WORD0_ucSIZE = MMU_TRANS_SZ_256M;

        } else if ((desc.FTLBD_stSize >= 16 * LW_CFG_MB_SIZE) &&
                  !(desc.FTLBD_ui64PhyAddr & (16 * LW_CFG_MB_SIZE - 1))) {
            desc.FTLBD_stSize    = 16 * LW_CFG_MB_SIZE;
            uiWord0.WORD0_ucSIZE = MMU_TRANS_SZ_16M;

        } else if ((desc.FTLBD_stSize >= 1 * LW_CFG_MB_SIZE) &&
                  !(desc.FTLBD_ui64PhyAddr & (1 * LW_CFG_MB_SIZE - 1))) {
            desc.FTLBD_stSize    = 1 * LW_CFG_MB_SIZE;
            uiWord0.WORD0_ucSIZE = MMU_TRANS_SZ_1M;

        } else if ((desc.FTLBD_stSize >= 256 * LW_CFG_KB_SIZE) &&
                  !(desc.FTLBD_ui64PhyAddr & (256 * LW_CFG_KB_SIZE - 1))) {
            desc.FTLBD_stSize    = 256 * LW_CFG_KB_SIZE;
            uiWord0.WORD0_ucSIZE = MMU_TRANS_SZ_256K;

        } else if ((desc.FTLBD_stSize >= 64 * LW_CFG_KB_SIZE) &&
                  !(desc.FTLBD_ui64PhyAddr & (64 * LW_CFG_KB_SIZE - 1))) {
            desc.FTLBD_stSize    = 64 * LW_CFG_KB_SIZE;
            uiWord0.WORD0_ucSIZE = MMU_TRANS_SZ_64K;

        } else if ((desc.FTLBD_stSize >= 16 * LW_CFG_KB_SIZE) &&
                  !(desc.FTLBD_ui64PhyAddr & (16 * LW_CFG_KB_SIZE - 1))) {
            desc.FTLBD_stSize    = 16 * LW_CFG_KB_SIZE;
            uiWord0.WORD0_ucSIZE = MMU_TRANS_SZ_16K;

        } else if ((desc.FTLBD_stSize >= 4 * LW_CFG_KB_SIZE) &&
                  !(desc.FTLBD_ui64PhyAddr & (4 * LW_CFG_KB_SIZE - 1))) {
            desc.FTLBD_stSize    = 4 * LW_CFG_KB_SIZE;
            uiWord0.WORD0_ucSIZE = MMU_TRANS_SZ_4K;

        } else {
            _BugFormat(LW_TRUE, LW_TRUE, "map size 0x%x is NOT 4K align!\r\n", pdesc->FTLBD_stSize);
        }

        /*
         * WORD1
         */
        uiWord1.WORD1_uiValue = 0;
        uiWord1.WORD1_uiRPN   = (desc.TLB1D_ui64PhyAddr >> MMU_RPN_SHIFT) & 0xfffff;
        uiWord1.WORD1_ucERPN  = desc.FTLBD_ui64PhyAddr >> 32;

        /*
         * WORD2
         */
        uiWord2.WORD2_uiValue       = 0;
        uiWord2.WORD2_bLittleEndian = LW_FALSE;                         /*  大端                        */
        uiWord2.WORD2_bMemCoh       = LW_FALSE;                         /*  没有多核, 不需要一致性      */

        if (desc.FTLBD_ulFlag & PPC_FTLB_FLAG_CACHEABLE) {              /*  回写 CACHE                  */
            uiWord2.WORD2_bUnCache = LW_FALSE;
            uiWord2.WORD2_bWT      = LW_FALSE;
            uiWord2.WORD2_bGuarded = LW_FALSE;

        } else if (desc.FTLBD_ulFlag & PPC_FTLB_FLAG_WRITETHROUGH) {    /*  写穿透 CACHE                */
            uiWord2.WORD2_bUnCache = LW_FALSE;
            uiWord2.WORD2_bWT      = LW_TRUE;
            uiWord2.WORD2_bGuarded = LW_FALSE;

        } else {                                                        /*  不可以 CACHE                */
            uiWord2.WORD2_bUnCache = LW_TRUE;
            uiWord2.WORD2_bWT      = LW_TRUE;
            uiWord2.WORD2_bGuarded = LW_TRUE;
        }

        if (desc.FTLBD_ulFlag & PPC_FTLB_FLAG_ACCESS) {
            uiWord2.WORD2_bSuperRead = LW_TRUE;                         /*  特权态可读                  */
            uiWord2.WORD2_bUserRead  = LW_FALSE;                        /*  用户态不可读                */
        }

        if (desc.FTLBD_ulFlag & PPC_FTLB_FLAG_WRITABLE) {
            uiWord2.WORD2_bSuperWrite = LW_TRUE;                        /*  特权态可写                  */
            uiWord2.WORD2_bUserWrite  = LW_FALSE;                       /*  用户态不可写                */
        }

        if (desc.FTLBD_ulFlag & PPC_FTLB_FLAG_EXECABLE) {
            uiWord2.WORD2_bSuperExec = LW_TRUE;                         /*  特权态可执行                */
            uiWord2.WORD2_bUserExec  = LW_FALSE;                        /*  用户态不可执行              */
        }

        /*
         * 将分析结果记录到 tlbWords 数组
         */
        tlbWords[i].FTLB_ulFlag  = desc.FTLBD_ulFlag;
        tlbWords[i].FTLB_uiWord0 = uiWord0;
        tlbWords[i].FTLB_uiWord1 = uiWord1;
        tlbWords[i].FTLB_uiWord2 = uiWord2;

        i++;

        stRemainSize = stRemainSize - desc.FTLBD_stSize;
        if (stRemainSize > 0) {                                         /*  有未"映射"的部分            */
            desc.FTLBD_ui64PhyAddr += desc.FTLBD_stSize;                /*  继续"映射"剩余的部分        */
            desc.FTLBD_ulVirMap    += desc.FTLBD_stSize;
            desc.FTLBD_stSize       = stRemainSize;

        } else {
            pdesc++;                                                    /*  "映射"下一个                */
            desc         = *pdesc;
            stRemainSize = desc.FTLBD_stSize;
        }
    }

    _BugHandle(i > PPC460_FTLB_SIZE, LW_TRUE, "to many map desc!\r\n");

    /*
     * 第二步: 用 tlbWords 数组记录的值来进行真正的映射
     */
    for (i = 0; i < PPC460_FTLB_SIZE; i++) {
        __asm__ __volatile__(
            "isync\n"
            "tlbwe  %2, %3, 2\n"
            "tlbwe  %1, %3, 1\n"
            "tlbwe  %0, %3, 0\n"
            "isync\n"
            :
            : "r" (tlbWords[i].FTLB_uiWord0.WORD0_uiValue),
              "r" (tlbWords[i].FTLB_uiWord1.WORD1_uiValue),
              "r" (tlbWords[i].FTLB_uiWord2.WORD2_uiValue),
              "r" (i));
    }

    if (pfuncPreRemoveTempMap) {
        pfuncPreRemoveTempMap();
    }

    /*
     * 第三步: 删除临时映射
     */
    for (i = 0; i < PPC460_FTLB_SIZE; i++) {
        if (tlbWords[i].FTLB_ulFlag & PPC_FTLB_FLAG_TEMP) {
            __asm__ __volatile__(
                "sync\n"
                "tlbwe  %0, %0, 0\n"
                "isync\n"
                :
                : "r" (i));
        }
    }

    return  (ERROR_NONE);
}

#endif                                                                  /*  LW_CFG_VMM_EN > 0           */
/*********************************************************************************************************
  END
*********************************************************************************************************/
