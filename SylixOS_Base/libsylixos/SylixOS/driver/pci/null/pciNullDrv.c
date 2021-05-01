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
** ��   ��   ��: pciNullDrv.c
**
** ��   ��   ��: Gong.YuJian (�����)
**
** �ļ���������: 2016 �� 06 �� 11 ��
**
** ��        ��: PCI NULL (ʾ��) �豸����.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#define  __SYLIXOS_PCI_DRV
#include "SylixOS.h"
/*********************************************************************************************************
  �ü���
*********************************************************************************************************/
#if (LW_CFG_DEVICE_EN > 0) && (LW_CFG_PCI_EN > 0)
#include "pciNullDrv.h"
#include "pci_ids.h"
/*********************************************************************************************************
  ��ȡ�豸�Ĵ�����Ϣ read32((addr_t)((ULONG)(ctrl)->addr + reg)).
*********************************************************************************************************/
#define PCI_NONE_READ(ctrl, reg)    0
/*********************************************************************************************************
  ��������, �豸������չ����, �� Linux ��չ���ݱ���һ��, �˴�ֻ�оٳ���������.
*********************************************************************************************************/
enum {
    board_none,
    board_none_yes_fbs
};
/*********************************************************************************************************
  ����֧�ֵ��豸 ID ��, �����������豸�����Զ�ƥ��, �� Linux ��������һ��, �˴�ֻ�оٳ���������.
*********************************************************************************************************/
static const PCI_DEV_ID_CB  pciNullDrvIdTbl[] = {
    {
        PCI_VDEVICE(INTEL, 0x2652), 
        board_none  
    },
    {
        PCI_VDEVICE(INTEL, 0x2653), 
        board_none  
    },
    {   
    }                                                                   /* terminate list               */
};
/*********************************************************************************************************
** ��������: pciNullDevIsr
** ��������: PCI �豸�жϷ���
** �䡡��  : pvArg      �жϲ���
**           ulVector   �ж�����
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static irqreturn_t  pciNullDevIsr (PVOID  pvArg, ULONG  ulVector)
{
    UINT32  uiReg;                                                      /*  �Ĵ�����Ϣ                  */

    uiReg = PCI_NONE_READ(pvArg, 0);                                    /*  ��ȡ��״̬                  */
    if (!uiReg) {                                                       /*  ���Ǳ��豸���ж�            */
        return  (LW_IRQ_NONE);                                          /*  ��ʶ���Ǳ��豸���ж�        */
    }

    /*
     *  TODO �жϴ���
     */
    return  (LW_IRQ_HANDLED);                                            /*  ��ʶ�Ǳ��豸���ж�         */
}
/*********************************************************************************************************
** ��������: pciNullDevIdTblGet
** ��������: ��ȡ�豸 ID ��ı�ͷ���Ĵ�С
** �䡡��  : hPciDevId      �豸 ID �б���������
**           puiSzie        �豸 ID �б��С������
** �䡡��  : ERROR or OK
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  pciNullDevIdTblGet (PCI_DEV_ID_HANDLE *hPciDevId, UINT32 *puiSzie)
{
    if ((!hPciDevId) || (!puiSzie)) {                                   /*  ������Ч                    */
        return  (PX_ERROR);                                             /*  ���󷵻�                    */
    }

    *hPciDevId = (PCI_DEV_ID_HANDLE)pciNullDrvIdTbl;                    /*  ��ȡ��ͷ                    */
    *puiSzie   = sizeof(pciNullDrvIdTbl) / sizeof(PCI_DEV_ID_CB);       /*  ��ȡ��Ĵ�С                */

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: pciNullDevRemove
** ��������: �������Ƴ� PCI �豸ʱ�Ĵ���
** �䡡��  : hPciDevHandle     PCI �豸���
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static VOID  pciNullDevRemove (PCI_DEV_HANDLE hPciDevHandle)
{
    PCI_RESOURCE_HANDLE     hResource;                                  /*  PCI �豸��Դ��Ϣ            */
    ULONG                   ulIrqVector;                                /*  �ж�����                    */

    hResource = API_PciDevResourceGet(hPciDevHandle, PCI_IORESOURCE_IRQ, 0);
    if (!hResource) {
        return;
    }

    ulIrqVector = (ULONG)(PCI_RESOURCE_START(hResource));               /*  ��ȡ�ж�����                */

    /*
     *  TODO �ر��豸, ����������������Դ (�ڴ�, �ź���, �̵߳�), ͬʱ�����ڴ����豸�ĵ�Դ�����.
     */

    /*
     *  �жϽ�����Ӳ������ж�
     */
    API_PciDevInterDisableEx(hPciDevHandle, ulIrqVector, pciNullDevIsr, LW_NULL, 1);
    API_PciDevInterDisconnect(hPciDevHandle, ulIrqVector, pciNullDevIsr, LW_NULL);

    /*
     *  ���� I/O MEM
     */
    API_PciDevIoUnmap(hPciDevHandle->PCIDEV_pvPrivate);
}
/*********************************************************************************************************
** ��������: pciNullDevProbe
** ��������: ����̽�⵽ ID �б����豸ʱ�Ĵ���
** �䡡��  : hPciDevHandle      PCI �豸���
**           hIdEntry           ƥ��ɹ����豸 ID ��Ŀ(�������豸 ID ��)
** �䡡��  : ERROR or OK
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  pciNullDevProbe (PCI_DEV_HANDLE hPciDevHandle, const PCI_DEV_ID_HANDLE hIdEntry)
{
    PCI_RESOURCE_HANDLE     hResource;                                  /*  PCI �豸��Դ��Ϣ            */
    phys_addr_t             paBaseAddr;                                 /*  ��ʼ��ַ                    */
    PVOID                   pvBaseAddr;                                 /*  ��ʼ��ַ                    */
    size_t                  stBaseSize;                                 /*  ��Դ��С                    */
    ULONG                   ulIrqVector;                                /*  �ж�����                    */

    if ((!hPciDevHandle) || (!hIdEntry)) {                              /*  �豸������Ч                */
        _ErrorHandle(EINVAL);                                           /*  ��Ǵ���                    */
        return  (PX_ERROR);                                             /*  ���󷵻�                    */
    }

    /*
     *  �����豸����, �豸�����汾��Ϣ����ǰ�����豸��������
     */
    hPciDevHandle->PCIDEV_uiDevVersion = PCI_NULL_DRV_VER_NUM;          /*  ��ǰ�豸�����汾��          */
    hPciDevHandle->PCIDEV_uiUnitNumber = 0;                             /*  �����豸����                */

    /*
     *  ��ȡ�豸��Դ��Ϣ (MEM IO IRQ ��)
     */
    hResource = API_PciDevResourceGet(hPciDevHandle, PCI_IORESOURCE_MEM, 0);
    if (!hResource) {                                                   /*  ��ȡ MEM ��Դ��Ϣ           */
        return  (PX_ERROR);
    }
    
    paBaseAddr = (phys_addr_t)(PCI_RESOURCE_START(hResource));          /*  ��ȡ MEM ����ʼ��ַ         */
    stBaseSize = (size_t)(PCI_RESOURCE_SIZE(hResource));                /*  ��ȡ MEM �Ĵ�С             */
    pvBaseAddr = API_PciDevIoRemap2(paBaseAddr, stBaseSize);            /*  ������ӳ��󷽿�ʹ��        */
    if (!pvBaseAddr) {
        return  (PX_ERROR);
    }
    hPciDevHandle->PCIDEV_pvPrivate = pvBaseAddr;                       /*  I/O MEM                     */
                                                                        /*  ��ȡ IRQ ��Դ               */
    hResource = API_PciDevResourceGet(hPciDevHandle, PCI_IORESOURCE_IRQ, 0);
    if (!hResource) {
        return  (PX_ERROR);
    }
    
    ulIrqVector = (ULONG)(PCI_RESOURCE_START(hResource));               /*  ��ȡ�ж�����                */

    /*
     *  TODO ��������
     */

    /*
     *  �����жϲ�ʹ���ж�
     */
    API_PciDevInterConnect(hPciDevHandle, ulIrqVector, pciNullDevIsr, LW_NULL, "pci_nulldev");
    API_PciDevInterEnable(hPciDevHandle, ulIrqVector, pciNullDevIsr, LW_NULL);

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: pciNullDrvRemove
** ��������: �豸����ɾ��
** �䡡��  : NONE
** �䡡��  : ERROR or OK
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
INT  pciNullDrvRemove (VOID)
{
    INT                 iRet;                                           /*  �������                    */
    PCI_DRV_HANDLE      hPciDrv;                                        /*  �������ƿ���              */

    hPciDrv = API_PciDrvHandleGet(PCI_NULL_DRV_NAME);                   /*  ��ȡ�Ѿ�ע����������      */
    if (!hPciDrv) {                                                     /*  ��ȡ�������ʧ��            */
        return  (PX_ERROR);
    }

    iRet = API_PciDrvUnregister(hPciDrv);                               /*  ɾ�������Լ�ȫ���豸����    */
    if (iRet != ERROR_NONE) {
        return  (PX_ERROR);
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: pciNullDrvInit
** ��������: �豸������ʼ��
** �䡡��  : NONE
** �䡡��  : ERROR or OK
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
INT  pciNullDrvInit (VOID)
{
    INT                 iRet;                                           /*  �������                    */
    PCI_DRV_CB          tPciDrv;                                        /*  ��������������ע������      */
    PCI_DRV_HANDLE      hPciDrv = &tPciDrv;                             /*  �������ƿ���              */

    lib_bzero(hPciDrv, sizeof(PCI_DRV_CB));                             /*  ��λ�������ƿ����          */
    iRet = pciNullDevIdTblGet(&hPciDrv->PCIDRV_hDrvIdTable, &hPciDrv->PCIDRV_uiDrvIdTableSize);
    if (iRet != ERROR_NONE) {                                           /*  ��ȡ�豸 ID ��ʧ��          */
        return  (PX_ERROR);
    }
                                                                        /*  ������������                */
    lib_strlcpy(& hPciDrv->PCIDRV_cDrvName[0], PCI_NULL_DRV_NAME, PCI_DRV_NAME_MAX);
    hPciDrv->PCIDRV_pvPriv         = LW_NULL;                           /*  �豸������˽������          */
    hPciDrv->PCIDRV_hDrvErrHandler = LW_NULL;                           /*  ����������                */
    hPciDrv->PCIDRV_pfuncDevProbe  = pciNullDevProbe;                   /*  �豸̽�⺯��, ����Ϊ��      */
    hPciDrv->PCIDRV_pfuncDevRemove = pciNullDevRemove;                  /*  �����Ƴ�����, ����Ϊ��      */

    iRet = API_PciDrvRegister(hPciDrv);                                 /*  ע�� PCI �豸����           */
    if (iRet != ERROR_NONE) {
        return  (PX_ERROR);
    }

    return  (ERROR_NONE);
}

#endif                                                                  /*  (LW_CFG_DEVICE_EN > 0) &&   */
                                                                        /*  (LW_CFG_PCI_EN > 0) &&      */
/*********************************************************************************************************
  END
*********************************************************************************************************/
