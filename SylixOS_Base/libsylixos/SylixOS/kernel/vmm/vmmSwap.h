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
** ��   ��   ��: vmmSwap.h
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2011 �� 05 �� 16 ��
**
** ��        ��: ƽ̨�޹��ڴ潻��������.
*********************************************************************************************************/

#ifndef __VMMSWAP_H
#define __VMMSWAP_H

/*********************************************************************************************************
  �쳣��Ϣ
*********************************************************************************************************/

#define __ABTCTX_SIZE_ALIGN         ROUND_UP(sizeof(LW_VMM_ABORT_CTX), sizeof(LW_STACK))
#define __ABTCTX_ABORT_TYPE(pctx)   (pctx->ABTCTX_abtInfo.VMABT_uiType)
#define __ABTCTX_ABORT_METHOD(pctx) (pctx->ABTCTX_abtInfo.VMABT_uiMethod)

/*********************************************************************************************************
  ����ü�֧��
*********************************************************************************************************/
#if LW_CFG_VMM_EN > 0

/*********************************************************************************************************
  ȱҳ�ж�ϵͳ֧�ֽṹ
*********************************************************************************************************/
typedef struct {
    LW_LIST_LINE        PAGEP_lineManage;                               /*  area ����                   */
    PLW_VMM_PAGE        PAGEP_pvmpageVirtual;                           /*  ��ָ����ҳ����ƿ�          */
    
#define LW_VMM_SHARED_CHANGE    0x1                                     /*  �������                    */
#define LW_VMM_PRIVATE_CHANGE   0x2                                     /*  дʱ����                    */
#define LW_VMM_PHY_PREALLOC     0x4                                     /*  �����ڴ�Ԥ����              */
    INT                 PAGEP_iFlags;                                   /*  like mmap flags             */
    
#if LW_CFG_MODULELOADER_TEXT_RO_EN > 0
    addr_t              PAGEP_ulPtStart;                                /*  ��ȱҳ�жϱ�����            */
    size_t              PAGEP_stPtSize;
#endif                                                                  /*  LW_CFG_MODULELOADER_TEXT... */

    FUNCPTR             PAGEP_pfuncFiller;                              /*  ҳ�������                  */
    PVOID               PAGEP_pvArg;                                    /*  ҳ�����������              */
    
    PVOIDFUNCPTR        PAGEP_pfuncFindShare;                           /*  Ѱ�ҿ��Թ����ҳ��          */
    PVOID               PAGEP_pvFindArg;                                /*  ����                        */
} LW_VMM_PAGE_PRIVATE;
typedef LW_VMM_PAGE_PRIVATE *PLW_VMM_PAGE_PRIVATE;

/*********************************************************************************************************
  �����ҳ�ռ����� (ȱҳ�жϲ��ұ�)
*********************************************************************************************************/
extern LW_LIST_LINE_HEADER  _K_plineVmmVAddrSpaceHeader;

/*********************************************************************************************************
  �ڴ潻������
*********************************************************************************************************/
BOOL            __vmmPageSwapIsNeedLoad(pid_t  pid, addr_t  ulVirtualPageAddr);
ULONG           __vmmPageSwapLoad(pid_t  pid, addr_t  ulDestVirtualPageAddr, addr_t  ulVirtualPageAddr);
PLW_VMM_PAGE    __vmmPageSwapSwitch(pid_t  pid, ULONG  ulPageNum, UINT  uiAttr);

#endif                                                                  /*  LW_CFG_VMM_EN > 0           */
#endif                                                                  /*  __VMMSWAP_H                 */
/*********************************************************************************************************
  END
*********************************************************************************************************/
