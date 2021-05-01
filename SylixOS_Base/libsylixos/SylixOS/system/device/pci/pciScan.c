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
** ��   ��   ��: pciScan.c
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2013 �� 10 �� 01 ��
**
** ��        ��: PCI �����Զ�ɨ��ƥ����豸��������.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
#include "../SylixOS/system/include/s_system.h"
/*********************************************************************************************************
  �ü���
*********************************************************************************************************/
#if (LW_CFG_DEVICE_EN > 0) && (LW_CFG_PCI_EN > 0)
#include "pciDev.h"
#include "pciLib.h"
#include "pciScan.h"
/*********************************************************************************************************
  PCI ������
*********************************************************************************************************/
extern  PCI_CTRL_HANDLE      _G_hPciCtrlHandle;
#define PCI_CTRL             _G_hPciCtrlHandle
/*********************************************************************************************************
  �ڲ��ص�����
*********************************************************************************************************/
typedef struct {
    PCI_DEV_DRV_DESC  *PSA_p_pdddTable;
    UINT               PSA_uiNum;
} PCI_SCAN_ARG;
/*********************************************************************************************************
** ��������: __pciScanCb
** ��������: PCI ɨ��ص�����.
** �䡡��  : iBus      ���ߺ�
**           iSlot     ���
**           iFunc     ����
**           p_psa     ɨ�����
** �䡡��  : ERROR_NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  __pciScanCb (INT iBus, INT iSlot, INT iFunc, PCI_SCAN_ARG *p_psa)
{
    INT     i;
    UINT32  usVenDevId;
    UINT16  usVendorId;
    UINT16  usDeviceId;
    
    API_PciConfigInDword(iBus, iSlot, iFunc, PCI_VENDOR_ID, &usVenDevId);
    
    usVendorId = (UINT16)(usVenDevId & 0xffff);
    usDeviceId = (UINT16)(usVenDevId >> 16);
    
    for (i = 0; i < p_psa->PSA_uiNum; i++) {
        if ((p_psa->PSA_p_pdddTable[i].PDDD_usVendorId == usVendorId) ||
            (p_psa->PSA_p_pdddTable[i].PDDD_usVendorId == 0xffff)) {
            if ((p_psa->PSA_p_pdddTable[i].PDDD_usDeviceId == usDeviceId) ||
                (p_psa->PSA_p_pdddTable[i].PDDD_usDeviceId == 0xffff)) {
                if (p_psa->PSA_p_pdddTable[i].PDDD_pfuncDrv) {
                    p_psa->PSA_p_pdddTable[i].PDDD_pfuncDrv(iBus, iSlot, iFunc,
                                                            p_psa->PSA_p_pdddTable[i].PDDD_pvArg);
                }
            }
        }
    }
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: API_PciScan
** ��������: ɨ������ PCI �豸�����������������������ƥ��, �������, ���Զ���װ��������.
** �䡡��  : p_pdddTable   ���������
**           uiNum         ����������С
** �䡡��  : ERROR or OK
** ȫ�ֱ���:
** ����ģ��:
**                                            API ����
*********************************************************************************************************/
LW_API 
INT  API_PciScan (PCI_DEV_DRV_DESC  *p_pdddTable, UINT  uiNum)
{
    PCI_SCAN_ARG    psa;
    
    if (PCI_CTRL == LW_NULL) {
        _ErrorHandle(ENOSYS);
        return  (PX_ERROR);
    }
    
    if (!p_pdddTable || !uiNum) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    
    psa.PSA_p_pdddTable = p_pdddTable;
    psa.PSA_uiNum       = uiNum;
    
    return  (API_PciTraversalDev(0, LW_TRUE, __pciScanCb, &psa));
}

#endif                                                                  /*  (LW_CFG_DEVICE_EN > 0) &&   */
                                                                        /*  (LW_CFG_PCI_EN > 0)         */
/*********************************************************************************************************
  END
*********************************************************************************************************/
