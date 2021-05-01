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
** 文   件   名: vmmSwap.h
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2011 年 05 月 16 日
**
** 描        述: 平台无关内存交换区管理.
*********************************************************************************************************/

#ifndef __VMMSWAP_H
#define __VMMSWAP_H

/*********************************************************************************************************
  异常消息
*********************************************************************************************************/

#define __ABTCTX_SIZE_ALIGN         ROUND_UP(sizeof(LW_VMM_ABORT_CTX), sizeof(LW_STACK))
#define __ABTCTX_ABORT_TYPE(pctx)   (pctx->ABTCTX_abtInfo.VMABT_uiType)
#define __ABTCTX_ABORT_METHOD(pctx) (pctx->ABTCTX_abtInfo.VMABT_uiMethod)

/*********************************************************************************************************
  加入裁剪支持
*********************************************************************************************************/
#if LW_CFG_VMM_EN > 0

/*********************************************************************************************************
  缺页中断系统支持结构
*********************************************************************************************************/
typedef struct {
    LW_LIST_LINE        PAGEP_lineManage;                               /*  area 链表                   */
    PLW_VMM_PAGE        PAGEP_pvmpageVirtual;                           /*  回指虚拟页面控制块          */
    
#define LW_VMM_SHARED_CHANGE    0x1                                     /*  共享更改                    */
#define LW_VMM_PRIVATE_CHANGE   0x2                                     /*  写时拷贝                    */
#define LW_VMM_PHY_PREALLOC     0x4                                     /*  物理内存预分配              */
    INT                 PAGEP_iFlags;                                   /*  like mmap flags             */
    
#if LW_CFG_MODULELOADER_TEXT_RO_EN > 0
    addr_t              PAGEP_ulPtStart;                                /*  非缺页中断保护段            */
    size_t              PAGEP_stPtSize;
#endif                                                                  /*  LW_CFG_MODULELOADER_TEXT... */

    FUNCPTR             PAGEP_pfuncFiller;                              /*  页面填充器                  */
    PVOID               PAGEP_pvArg;                                    /*  页面填充器参数              */
    
    PVOIDFUNCPTR        PAGEP_pfuncFindShare;                           /*  寻找可以共享的页面          */
    PVOID               PAGEP_pvFindArg;                                /*  参数                        */
} LW_VMM_PAGE_PRIVATE;
typedef LW_VMM_PAGE_PRIVATE *PLW_VMM_PAGE_PRIVATE;

/*********************************************************************************************************
  虚拟分页空间链表 (缺页中断查找表)
*********************************************************************************************************/
extern LW_LIST_LINE_HEADER  _K_plineVmmVAddrSpaceHeader;

/*********************************************************************************************************
  内存交换函数
*********************************************************************************************************/
BOOL            __vmmPageSwapIsNeedLoad(pid_t  pid, addr_t  ulVirtualPageAddr);
ULONG           __vmmPageSwapLoad(pid_t  pid, addr_t  ulDestVirtualPageAddr, addr_t  ulVirtualPageAddr);
PLW_VMM_PAGE    __vmmPageSwapSwitch(pid_t  pid, ULONG  ulPageNum, UINT  uiAttr);

#endif                                                                  /*  LW_CFG_VMM_EN > 0           */
#endif                                                                  /*  __VMMSWAP_H                 */
/*********************************************************************************************************
  END
*********************************************************************************************************/
