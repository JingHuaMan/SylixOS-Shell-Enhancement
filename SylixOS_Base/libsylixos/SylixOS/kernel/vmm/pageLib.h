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
** ��   ��   ��: pageLib.h
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2008 �� 12 �� 09 ��
**
** ��        ��: ƽ̨�޹������ڴ����, ����ҳ������.

** BUG:
2009.11.10  LW_VMM_ZONE �м��� DMA �����жϷ�, �����������Ƿ�ɹ� DMA ʹ��.
2013.05.30  ҳ����ƿ��м������ô�����ָ����ʵ����ҳ����ƿ��ָ��, ���ڹ�������ҳ��.
*********************************************************************************************************/

#ifndef __PAGELIB_H
#define __PAGELIB_H

/*********************************************************************************************************
  ����ü�֧��
*********************************************************************************************************/
#if LW_CFG_VMM_EN > 0
/*********************************************************************************************************
  buddy free area struct (diff than linux)
*********************************************************************************************************/

typedef struct __lw_vmm_freearea {
    PLW_LIST_LINE           FA_lineFreeHeader;                          /*  ��������ҳ������            */
    ULONG                   FA_ulCount;                                 /*  ��ǰ hash ��ڽڵ�����      */
} LW_VMM_FREEAREA;
typedef LW_VMM_FREEAREA    *PLW_VMM_FREEAREA;

/*********************************************************************************************************
  physical zone struct 
*********************************************************************************************************/

typedef struct __lw_vmm_zone {
    ULONG                   ZONE_ulFreePage;                            /*  ����ҳ��ĸ���              */
    addr_t                  ZONE_ulAddr;                                /*  ������ʼ��ַ                */
    size_t                  ZONE_stSize;                                /*  ������С                    */
    UINT                    ZONE_uiAttr;                                /*  ��������                    */
    LW_VMM_FREEAREA         ZONE_vmfa[LW_CFG_VMM_MAX_ORDER];            /*  ����ҳ�� hash ��            */
} LW_VMM_ZONE;
typedef LW_VMM_ZONE        *PLW_VMM_ZONE;

/*********************************************************************************************************
  page struct (��ҳ��Ϊ����ҳ��ʱ PAGE_lineFreeHash ��ʹ��ʱ����������ҳ������, hash key Ϊ�����ַ)
*********************************************************************************************************/

typedef struct __lw_vmm_page {
    LW_LIST_LINE            PAGE_lineFreeHash;                          /*  free area hash �е�����     */
    LW_LIST_LINE            PAGE_lineManage;                            /*  �����ھ�ָ��                */
    LW_TREE_RB_NODE         PAGE_trbnNode;                              /*  ������ڵ�                  */
    
    ULONG                   PAGE_ulCount;                               /*  ҳ�����                    */
    addr_t                  PAGE_ulPageAddr;                            /*  ҳ���ַ                    */
    
#define __VMM_PAGE_TYPE_PHYSICAL      0                                 /*  ����ռ�ҳ��                */
#define __VMM_PAGE_TYPE_VIRTUAL       1                                 /*  ����ռ�ҳ��                */
    INT                     PAGE_iPageType;                             /*  ҳ������                    */
    ULONG                   PAGE_ulFlags;                               /*  ҳ������                    */
    BOOL                    PAGE_bUsed;                                 /*  ҳ���Ƿ���ʹ��              */
    PLW_VMM_ZONE            PAGE_pvmzoneOwner;                          /*  ҳ����������                */
    
    union {
        struct {
            /*
             *  �����ֶ�ֻ�ڴ�ҳ��Ϊ����ҳ��ʱ��Ч.
             *  ��� PAGE_pvmpageReal ��Ч, �������ҳ����ƿ�Ϊα��, ��ʵ������ҳ����ƿ�Ϊ 
             *  PAGE_pvmpageReal ָ���Ŀ��. PAGE_iChange ��ʾ�����������ı�, 
             */
            addr_t                  PPAGE_ulMapPageAddr;                /*  ҳ�汻ӳ��ĵ�ַ ����->���� */
            INT                     PPAGE_iChange;                      /*  ����ҳ���Ѿ��仯            */
            ULONG                   PPAGE_ulRef;                        /*  ����ҳ�����ô���            */
            struct __lw_vmm_page   *PPAGE_pvmpageReal;                  /*  ָ����ʵ����ҳ��            */
        } __phy_page_status;
        
        struct {
            /*
             *  �����ֶ�ֻ�ڴ�ҳ��Ϊ����ҳ��ʱ��Ч. 
             *  PAGE_pvPrivate ���ݽṹ����ȱҳ�жϹ�������Ч.
             *  ��ҳ��������ҳ��, �� VPAGE_pvAreaCb ָ�� LW_VMM_PAGE_PRIVATE ���ݽṹ
             */
            PVOID                   VPAGE_pvAreaCb;                     /*  ȱҳ�ж�������ƿ�          */
            LW_LIST_LINE_HEADER    *VPAGE_plinePhyLink;                 /*  ����ҳ�� hash ����          */
        } __vir_page_status;
    
    } __lw_vmm_page_status;
} LW_VMM_PAGE;
typedef LW_VMM_PAGE        *PLW_VMM_PAGE;

#define PAGE_ulMapPageAddr  __lw_vmm_page_status.__phy_page_status.PPAGE_ulMapPageAddr
#define PAGE_iChange        __lw_vmm_page_status.__phy_page_status.PPAGE_iChange
#define PAGE_ulRef          __lw_vmm_page_status.__phy_page_status.PPAGE_ulRef
#define PAGE_pvmpageReal    __lw_vmm_page_status.__phy_page_status.PPAGE_pvmpageReal

#define PAGE_pvAreaCb       __lw_vmm_page_status.__vir_page_status.VPAGE_pvAreaCb
#define PAGE_plinePhyLink   __lw_vmm_page_status.__vir_page_status.VPAGE_plinePhyLink

#define LW_VMM_PAGE_IS_FAKE(pvmpagePhysical)    (pvmpagePhysical->PAGE_pvmpageReal ? LW_TRUE : LW_FALSE)
#define LW_VMM_PAGE_GET_REAL(pvmpagePhysical)   (pvmpagePhysical->PAGE_pvmpageReal)

/*********************************************************************************************************
  ӳ���ַ��Ч
*********************************************************************************************************/

#define PAGE_MAP_ADDR_INV   ((addr_t)PX_ERROR)

/*********************************************************************************************************
  init
*********************************************************************************************************/

ULONG         __pageCbInit(ULONG  ulPageNum);

/*********************************************************************************************************
  service
*********************************************************************************************************/

ULONG         __pageZoneCreate(PLW_VMM_ZONE   pvmzone,
                               addr_t         ulAddr, 
                               size_t         stSize, 
                               UINT           uiAttr,
                               INT            iPageType);               /*  ��������                    */
PLW_VMM_PAGE  __pageAllocate(PLW_VMM_ZONE  pvmzone, 
                             ULONG         ulPageNum, 
                             INT           iPageType);                  /*  ����������ҳ                */
PLW_VMM_PAGE  __pageAllocateAlign(PLW_VMM_ZONE  pvmzone, 
                                  ULONG         ulPageNum, 
                                  size_t        stAlign,
                                  INT           iPageType);             /*  ����ָ�������ϵ������ҳ    */
VOID          __pageFree(PLW_VMM_ZONE   pvmzone, 
                         PLW_VMM_PAGE   pvmpage);                       /*  ����������ҳ                */
ULONG         __pageSplit(PLW_VMM_PAGE   pvmpage, 
                          PLW_VMM_PAGE  *ppvmpageSplit, 
                          ULONG          ulPageNum,
                          PVOID          pvAreaCb);                     /*  ҳ����                    */
ULONG         __pageExpand(PLW_VMM_PAGE   pvmpage, 
                           ULONG          ulExpPageNum);                /*  ҳ����չ                    */
ULONG         __pageMerge(PLW_VMM_PAGE   pvmpageL, 
                          PLW_VMM_PAGE   pvmpageR);                     /*  ҳ��ϲ�                    */
VOID          __pageLink(PLW_VMM_PAGE   pvmpageVirtual, 
                         PLW_VMM_PAGE   pvmpagePhysical);               /*  ҳ������                    */
VOID          __pageUnlink(PLW_VMM_PAGE  pvmpageVirtual, 
                           PLW_VMM_PAGE  pvmpagePhysical);              /*  ���ҳ������                */
PLW_VMM_PAGE  __pageFindLink(PLW_VMM_PAGE  pvmpageVirtual, 
                             addr_t        ulVirAddr);                  /*  ����ҳ������                */
VOID          __pageTraversalLink(PLW_VMM_PAGE   pvmpageVirtual,
                                  VOIDFUNCPTR    pfunc, 
                                  PVOID          pvArg0,
                                  PVOID          pvArg1,
                                  PVOID          pvArg2,
                                  PVOID          pvArg3,
                                  PVOID          pvArg4,
                                  PVOID          pvArg5);               /*  ����ҳ������                */
ULONG         __pageGetMinContinue(PLW_VMM_ZONE  pvmzone);              /*  ��õ�ǰ��С������ҳ����    */

#endif                                                                  /*  __VMM_H                     */
#endif                                                                  /*  __PAGELIB_H                 */
/*********************************************************************************************************
  END
*********************************************************************************************************/
