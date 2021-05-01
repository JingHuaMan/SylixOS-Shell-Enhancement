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
** ��   ��   ��: virPage.c
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2008 �� 12 �� 09 ��
**
** ��        ��: �����ڴ����.

** BUG:
2013.05.24  ��������ռ���뿪��.
2014.08.08  ���� __vmmVirtualDesc() ����.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
/*********************************************************************************************************
  ����ü�֧��
*********************************************************************************************************/
#if LW_CFG_VMM_EN > 0
#include "phyPage.h"
#include "virPage.h"
/*********************************************************************************************************
  ��ַ�ռ��ͻ���
*********************************************************************************************************/
extern BOOL     __vmmLibVirtualOverlap(addr_t  ulAddr, size_t  stSize);
/*********************************************************************************************************
  ����ռ�����
*********************************************************************************************************/
static LW_MMU_VIRTUAL_DESC  _G_vmvirDescApp[LW_CFG_VMM_VIR_NUM];        /*  Ӧ�ó���                    */
static LW_MMU_VIRTUAL_DESC  _G_vmvirDescDev;                            /*  �����豸                    */
/*********************************************************************************************************
  �豸����ռ� zone ���ƿ�
*********************************************************************************************************/
static LW_VMM_ZONE          _G_vmzoneVirApp[LW_CFG_VMM_VIR_NUM];
static LW_VMM_ZONE          _G_vmzoneVirDev;
/*********************************************************************************************************
  �л�ͨ��
*********************************************************************************************************/
static addr_t               _G_ulVmmSwitchAddr = PAGE_MAP_ADDR_INV;
/*********************************************************************************************************
** ��������: __vmmVirtualDesc
** ��������: �������ռ�����.
** �䡡��  : uiType        ����
**           ulZoneIndex   ���������±�
**           pulFreePage   ʣ��ռ�ҳ����
** �䡡��  : ����ռ�����
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
PLW_MMU_VIRTUAL_DESC  __vmmVirtualDesc (UINT32  uiType, ULONG  ulZoneIndex, ULONG  *pulFreePage)
{
    if (ulZoneIndex >= LW_CFG_VMM_VIR_NUM) {
        _ErrorHandle(EINVAL);
        return  (LW_NULL);
    }

    if (uiType == LW_VIRTUAL_MEM_APP) {
        if (pulFreePage) {
            *pulFreePage = _G_vmzoneVirApp[ulZoneIndex].ZONE_ulFreePage;
        }
        return  (&_G_vmvirDescApp[ulZoneIndex]);
    
    } else {
        if (pulFreePage) {
            *pulFreePage = _G_vmzoneVirDev.ZONE_ulFreePage;
        }
        return  (&_G_vmvirDescDev);
    }
}
/*********************************************************************************************************
** ��������: __vmmVirtualSwitch
** ��������: �������ռ����򽻻���.
** �䡡��  : NONE
** �䡡��  : ����ռ�����
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
addr_t  __vmmVirtualSwitch (VOID)
{
    return  (_G_ulVmmSwitchAddr);
}
/*********************************************************************************************************
** ��������: __vmmVirtualGetZone
** ��������: ȷ������ zone �±�.
** �䡡��  : ulAddr        ��ַ
** �䡡��  : zone �±�.
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static BOOL  __vmmVirtualGetZone (addr_t  ulAddr)
{
             INT            i;
    REGISTER PLW_VMM_ZONE   pvmzone;

    for (i = 0; i < LW_CFG_VMM_VIR_NUM; i++) {
        pvmzone = &_G_vmzoneVirApp[i];
        if (!pvmzone->ZONE_stSize) {                                    /*  ��Ч zone                   */
            break;
        }
        if ((ulAddr >= pvmzone->ZONE_ulAddr) &&
            (ulAddr <= pvmzone->ZONE_ulAddr + pvmzone->ZONE_stSize - 1)) {
            return  ((ULONG)i);
        }
    }
    
    return  (LW_CFG_VMM_VIR_NUM);
}
/*********************************************************************************************************
** ��������: __vmmVirtualIsInside
** ��������: �жϵ�ַ�Ƿ�������ռ���.
** �䡡��  : ulAddr        ��ַ
** �䡡��  : �Ƿ��� VMM ����ռ���
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
BOOL  __vmmVirtualIsInside (addr_t  ulAddr)
{
    ULONG   ulZoneIndex = __vmmVirtualGetZone(ulAddr);
    
    if (ulZoneIndex >= LW_CFG_VMM_VIR_NUM) {
        return  (LW_FALSE);
    }
    
    return  (LW_TRUE);
}
/*********************************************************************************************************
** ��������: __vmmVirtualCreate
** ��������: ��������ռ�����.
** �䡡��  : pvirdes       ����ռ�����
** �䡡��  : ERROR CODE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
ULONG  __vmmVirtualCreate (LW_MMU_VIRTUAL_DESC   pvirdes[])
{
    REGISTER ULONG  ulError = ERROR_NONE;
             ULONG  ulZone  = 0;
             addr_t ulAddr;
             size_t stSize;
             INT    i;
    
    for (i = 0; ; i++) {
        if (pvirdes[i].VIRD_stSize == 0) {
            break;
        }
        
        _BugFormat(!ALIGNED(pvirdes[i].VIRD_ulVirAddr, LW_CFG_VMM_PAGE_SIZE), LW_TRUE,
                   "virtual zone vaddr 0x%08lx not page aligned.\r\n", 
                   pvirdes[i].VIRD_ulVirAddr);
        
        _BugFormat(__vmmLibVirtualOverlap(pvirdes[i].VIRD_ulVirAddr, 
                                          pvirdes[i].VIRD_stSize), LW_TRUE,
                   "virtual zone vaddr 0x%08lx size: 0x%08zx overlap with virtual space.\r\n",
                   pvirdes[i].VIRD_ulVirAddr, pvirdes[i].VIRD_stSize);
                   
        switch (pvirdes[i].VIRD_uiType) {
        
        case LW_VIRTUAL_MEM_APP:
            _BugHandle((pvirdes[i].VIRD_ulVirAddr == (addr_t)LW_NULL), LW_TRUE,
                       "virtual APP area can not use NULL address, you can move offset page.\r\n");
                                                                        /*  Ŀǰ��֧�� NULL ��ʼ��ַ    */
            if (ulZone < LW_CFG_VMM_VIR_NUM) {
                _G_vmvirDescApp[ulZone] = pvirdes[i];
                if (_G_ulVmmSwitchAddr == PAGE_MAP_ADDR_INV) {
                    _G_ulVmmSwitchAddr =  pvirdes[i].VIRD_ulVirAddr;
                    ulAddr = _G_ulVmmSwitchAddr + LW_CFG_VMM_PAGE_SIZE;
                    stSize = pvirdes[i].VIRD_stSize - LW_CFG_VMM_PAGE_SIZE;

                } else {
                    ulAddr = pvirdes[i].VIRD_ulVirAddr;
                    stSize = pvirdes[i].VIRD_stSize;
                }
                ulError = __pageZoneCreate(&_G_vmzoneVirApp[ulZone], 
                                           ulAddr, stSize,
                                           LW_ZONE_ATTR_NONE,
                                           __VMM_PAGE_TYPE_VIRTUAL);
                if (ulError) {
                    _DebugHandle(__ERRORMESSAGE_LEVEL, "kernel low memory.\r\n");
                    _ErrorHandle(ulError);
                    return  (ulError);
                }
                ulZone++;
            }
            break;
            
        case LW_VIRTUAL_MEM_DEV:
            _BugHandle((pvirdes[i].VIRD_ulVirAddr == (addr_t)LW_NULL), LW_TRUE,
                       "virtual device area can not use NULL address, you can move offset page.\r\n");
                                                                        /*  Ŀǰ��֧�� NULL ��ʼ��ַ    */
            _BugHandle((_G_vmvirDescDev.VIRD_stSize), LW_TRUE,
                       "there are ONLY one virtual section allowed used to device.\r\n");
                       
            _G_vmvirDescDev = pvirdes[i];
            ulError = __pageZoneCreate(&_G_vmzoneVirDev, 
                                       pvirdes[i].VIRD_ulVirAddr, 
                                       pvirdes[i].VIRD_stSize, 
                                       LW_ZONE_ATTR_NONE,
                                       __VMM_PAGE_TYPE_VIRTUAL);
            if (ulError) {
                _DebugHandle(__ERRORMESSAGE_LEVEL, "kernel low memory.\r\n");
                _ErrorHandle(ulError);
                return  (ulError);
            }
            break;
            
        default:
            break;
        }
    }
    
    _BugHandle((_G_ulVmmSwitchAddr == PAGE_MAP_ADDR_INV), LW_TRUE,
               "virtual switich page invalidate.\r\n");
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: __vmmVirtualPageAlloc
** ��������: ����ָ������������ҳ��
** �䡡��  : ulPageNum     ��Ҫ���������ҳ�����
** �䡡��  : ҳ����ƿ�
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
PLW_VMM_PAGE  __vmmVirtualPageAlloc (ULONG  ulPageNum)
{
             INT            i;
    REGISTER PLW_VMM_ZONE   pvmzone;
    REGISTER PLW_VMM_PAGE   pvmpage = LW_NULL;
    
    for (i = 0; i < LW_CFG_VMM_VIR_NUM; i++) {
        pvmzone = &_G_vmzoneVirApp[i];
        if (!pvmzone->ZONE_stSize) {                                    /*  ��Ч zone                   */
            break;
        }
        if (pvmzone->ZONE_ulFreePage >= ulPageNum) {
            pvmpage = __pageAllocate(pvmzone, ulPageNum, __VMM_PAGE_TYPE_VIRTUAL);
            if (pvmpage) {
                return  (pvmpage);
            }
        }
    }
    
    return  (LW_NULL);
}
/*********************************************************************************************************
** ��������: __vmmVirDevPageAlloc
** ��������: ����ָ�������������豸ҳ��
** �䡡��  : ulPageNum     ��Ҫ���������ҳ�����
** �䡡��  : ҳ����ƿ�
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
PLW_VMM_PAGE  __vmmVirDevPageAlloc (ULONG  ulPageNum)
{
    return  (__pageAllocate(&_G_vmzoneVirDev, ulPageNum, __VMM_PAGE_TYPE_VIRTUAL));
}
/*********************************************************************************************************
** ��������: __vmmVirtualPageAllocAlign
** ��������: ����ָ������������ҳ�� (ָ�������ϵ)
** �䡡��  : ulPageNum     ��Ҫ���������ҳ�����
**           stAlign       �ڴ�����ϵ
** �䡡��  : ҳ����ƿ�
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
PLW_VMM_PAGE  __vmmVirtualPageAllocAlign (ULONG  ulPageNum, size_t  stAlign)
{
             INT            i;
    REGISTER PLW_VMM_ZONE   pvmzone;
    REGISTER PLW_VMM_PAGE   pvmpage = LW_NULL;
    
    for (i = 0; i < LW_CFG_VMM_VIR_NUM; i++) {
        pvmzone = &_G_vmzoneVirApp[i];
        if (!pvmzone->ZONE_stSize) {                                    /*  ��Ч zone                   */
            break;
        }
        if (pvmzone->ZONE_ulFreePage >= ulPageNum) {
            pvmpage = __pageAllocateAlign(pvmzone, ulPageNum, stAlign, __VMM_PAGE_TYPE_VIRTUAL);
            if (pvmpage) {
                return  (pvmpage);
            }
        }
    }
    
    return  (LW_NULL);
}
/*********************************************************************************************************
** ��������: __vmmVirDevPageAllocAlign
** ��������: ����ָ�������������豸ҳ�� (ָ�������ϵ)
** �䡡��  : ulPageNum     ��Ҫ���������ҳ�����
**           stAlign       �ڴ�����ϵ
** �䡡��  : ҳ����ƿ�
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
PLW_VMM_PAGE  __vmmVirDevPageAllocAlign (ULONG  ulPageNum, size_t  stAlign)
{
    return  (__pageAllocateAlign(&_G_vmzoneVirDev, ulPageNum, 
                                 stAlign, __VMM_PAGE_TYPE_VIRTUAL));
}
/*********************************************************************************************************
** ��������: __vmmVirtualPageFree
** ��������: ����ָ����������ҳ��
** �䡡��  : pvmpage       ҳ����ƿ�
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
VOID  __vmmVirtualPageFree (PLW_VMM_PAGE  pvmpage)
{
    ULONG   ulZoneIndex = __vmmVirtualGetZone(pvmpage->PAGE_ulPageAddr);

    __pageFree(&_G_vmzoneVirApp[ulZoneIndex], pvmpage);
}
/*********************************************************************************************************
** ��������: __vmmVirDevPageFree
** ��������: ����ָ�����������豸ҳ��
** �䡡��  : pvmpage       ҳ����ƿ�
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
VOID  __vmmVirDevPageFree (PLW_VMM_PAGE  pvmpage)
{
    __pageFree(&_G_vmzoneVirDev, pvmpage);
}

#endif                                                                  /*  LW_CFG_VMM_EN > 0           */
/*********************************************************************************************************
  END
*********************************************************************************************************/
