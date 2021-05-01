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
** ��   ��   ��: pciSioNetmos.c
**
** ��   ��   ��: Lu.Zhenping (¬��ƽ)
**
** �ļ���������: 2016 �� 06 �� 12 ��
**
** ��        ��: PCI NETMOS 16c550 �豸����.
*********************************************************************************************************/
#define  __SYLIXOS_STDIO
#define  __SYLIXOS_KERNEL
#define  __SYLIXOS_PCI_DRV
#include "SylixOS.h"
#include "../SylixOS/config/driver/drv_cfg.h"
/*********************************************************************************************************
  �ü���
*********************************************************************************************************/
#if (LW_CFG_DEVICE_EN > 0) && (LW_CFG_PCI_EN > 0) && (LW_CFG_DRV_SIO_16C550 > 0)
#include "pci_ids.h"
#include "linux/compat.h"
#include "driver/sio/16c550.h"
#include "pciSioNetmos.h"
/*********************************************************************************************************
  �˿ڵ�ַ�궨��
*********************************************************************************************************/
#define FL_BASE_MASK        0x0007
#define FL_BASE0            0x0000
#define FL_BASE1            0x0001
#define FL_BASE2            0x0002
#define FL_BASE3            0x0003
#define FL_BASE4            0x0004
#define FL_GET_BASE(x)      (x & FL_BASE_MASK)
/*********************************************************************************************************
  Use successive BARs (PCI base address registers), else use offset into some specified BAR
*********************************************************************************************************/
#define FL_BASE_BARS        0x0008
/*********************************************************************************************************
  Do not assign an irq
*********************************************************************************************************/
#define FL_NOIRQ            0x0080
/*********************************************************************************************************
  Use the Base address register size to cap number of ports
*********************************************************************************************************/
#define FL_REGION_SZ_CAP    0x0100
/*********************************************************************************************************
  ��������.
*********************************************************************************************************/
enum {
    netmos_9912 = 0
};
/*********************************************************************************************************
  ����֧�ֵ��豸 ID ��, �����������豸�����Զ�ƥ��, �� Linux ��������һ��.
*********************************************************************************************************/
static const PCI_DEV_ID_CB  pciSioNetmosIdTbl[] = {
    {
        PCI_VENDOR_ID_NETMOS, PCI_DEVICE_ID_NETMOS_9901,
        0xa000, 0x1000, 0, 0,
        netmos_9912
    },
    {
        PCI_VENDOR_ID_NETMOS, PCI_DEVICE_ID_NETMOS_9912,
        0xa000, 0x1000, 0, 0,
        netmos_9912
    },
    {
        PCI_VENDOR_ID_NETMOS, PCI_DEVICE_ID_NETMOS_9922,
        0xa000, 0x1000, 0, 0,
        netmos_9912
    },
    {
        PCI_VENDOR_ID_NETMOS, PCI_DEVICE_ID_NETMOS_9904,
        0xa000, 0x1000, 0, 0,
        netmos_9912
    },
    {
        PCI_VENDOR_ID_NETMOS, PCI_DEVICE_ID_NETMOS_9900,
        0xa000, 0x1000, 0, 0,
        netmos_9912
    },
    {
    }                                                                   /* terminate list               */
};
/*********************************************************************************************************
  �豸֧�����
*********************************************************************************************************/
typedef struct {
    UINT32      NETMOS_uiFlags;
    UINT32      NETMOS_uiPorts;
    UINT32      NETMOS_uiBaud;
    UINT32      NETMOS_uiUartOff;
    UINT32      NETMOS_uiRegShift;
    UINT32      NETMOS_uiFirstOff;
} PCI_SIO_NETMOS;
/*********************************************************************************************************
  NETMOS 16C550 SIO ���ö���
*********************************************************************************************************/
typedef struct {
    ULONG            CFG_ulBase;
    ULONG            CFG_ulXtal;
    ULONG            CFG_ulBaud;
    ULONG            CFG_ulVector;
    INT              CFG_idx;                                           /* ͨ��������                   */
    PCI_DEV_HANDLE   CFG_pciHandle;
} PCI_SIO_NETMOS_CFG;
/*********************************************************************************************************
  �豸�忨��Ϣ
*********************************************************************************************************/
static PCI_SIO_NETMOS   pciSioNetmosCard[] = {
    {
        FL_BASE1, 1, 115200, 0, 0, 0
    },
};
/*********************************************************************************************************
** ��������: pciSioNetmosSetReg
** ��������: ���� NETMOS 16C550 �Ĵ���
** �䡡��  : psiochan        16C550 SIO ͨ��
**           iReg            �Ĵ���
**           ucValue         ֵ
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static VOID  pciSioNetmosSetReg (SIO16C550_CHAN  *psiochan, INT  iReg, UINT8  ucValue)
{
    REGISTER PCI_SIO_NETMOS_CFG  *pcisiocfg = (PCI_SIO_NETMOS_CFG *)psiochan->priv;

    write8(ucValue, pcisiocfg->CFG_ulBase + 0x280 + ((addr_t)iReg * 4));
}
/*********************************************************************************************************
** ��������: pciSioNetmosGetReg
** ��������: ��� NETMOS 16C550 �Ĵ�����ֵ
** �䡡��  : psiochan        16C550 SIO ͨ��
**           iReg            �Ĵ���
** �䡡��  : �Ĵ�����ֵ
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static UINT8  pciSioNetmosGetReg (SIO16C550_CHAN  *psiochan, INT  iReg)
{
    REGISTER PCI_SIO_NETMOS_CFG  *pcisiocfg = (PCI_SIO_NETMOS_CFG *)psiochan->priv;
    
    return  (read8(pcisiocfg->CFG_ulBase + 0x280 + ((addr_t)iReg * 4)));
}
/*********************************************************************************************************
** ��������: pciSioNetmosIsr
** ��������: PCI �豸�жϷ���
** �䡡��  : pvArg      �жϲ���
**           ulVector   �ж�����
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static irqreturn_t  pciSioNetmosIsr (PVOID  pvArg, ULONG  ulVector)
{
    REGISTER SIO16C550_CHAN  *psiochan = (SIO16C550_CHAN *)pvArg;
             UINT8            ucIIR;
    
    ucIIR = pciSioNetmosGetReg(psiochan, IIR);
    if (ucIIR & 0x01) {
        return  (LW_IRQ_NONE);
    }

    sio16c550Isr(psiochan);
    
    return  (LW_IRQ_HANDLED);
}
/*********************************************************************************************************
** ��������: pciSioNetmosChan
** ��������: ����һ�� SIO ͨ��
** �䡡��  : uiChannel     Ӳ��ͨ����
**           psiochan      16c550 chan
**           pcisiocfg     netmos cfg
** �䡡��  : SIO ͨ��
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static SIO_CHAN  *pciSioNetmosChan (UINT                uiChannel, 
                                    SIO16C550_CHAN     *psiochan, 
                                    PCI_SIO_NETMOS_CFG *pcisiocfg)
{
    CHAR    cIrqName[64];

    psiochan->pdeferq = API_InterDeferGet(0);

    /*
     *  Receiver FIFO Trigger Level and Tirgger bytes table
     *  level  16 Bytes FIFO Trigger   32 Bytes FIFO Trigger  64 Bytes FIFO Trigger
     *    0              1                       8                    1
     *    1              4                      16                   16
     *    2              8                      24                   32
     *    3             14                      28                   56
     */
    psiochan->fifo_len         = 8;
    psiochan->rx_trigger_level = 1;
    psiochan->iobase           = 0;                                     /*  NO IO Base                  */

    psiochan->baud   = pcisiocfg->CFG_ulBaud;
    psiochan->xtal   = pcisiocfg->CFG_ulXtal;
    psiochan->setreg = pciSioNetmosSetReg;
    psiochan->getreg = pciSioNetmosGetReg;

    psiochan->priv = pcisiocfg;

    API_PciDevInterDisable(pcisiocfg->CFG_pciHandle, pcisiocfg->CFG_ulVector,
                           (PINT_SVR_ROUTINE)pciSioNetmosIsr,
                           (PVOID)psiochan);

    sio16c550Init(psiochan);

    snprintf(cIrqName, sizeof(cIrqName), "pci_netmos_%d", uiChannel);
    
    API_PciDevInterConnect(pcisiocfg->CFG_pciHandle, pcisiocfg->CFG_ulVector,
                           (PINT_SVR_ROUTINE)pciSioNetmosIsr, 
                           (PVOID)psiochan, cIrqName);

    API_PciDevInterEnable(pcisiocfg->CFG_pciHandle, pcisiocfg->CFG_ulVector,
                          (PINT_SVR_ROUTINE)pciSioNetmosIsr,
                          (PVOID)psiochan);

    return  ((SIO_CHAN *)psiochan);
}
/*********************************************************************************************************
** ��������: pciSioNetmosIdTblGet
** ��������: ��ȡ�豸 ID ��ı�ͷ���Ĵ�С
** �䡡��  : hPciDevId      �豸 ID �б���������
**           puiSzie        �豸 ID �б��С������
** �䡡��  : ERROR or OK
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  pciSioNetmosIdTblGet (PCI_DEV_ID_HANDLE *hPciDevId, UINT32 *puiSzie)
{
    if ((!hPciDevId) || (!puiSzie)) {                                   /*  ������Ч                    */
        return  (PX_ERROR);                                             /*  ���󷵻�                    */
    }

    *hPciDevId = (PCI_DEV_ID_HANDLE)pciSioNetmosIdTbl;                  /*  ��ȡ��ͷ                    */
    *puiSzie   = sizeof(pciSioNetmosIdTbl) / sizeof(PCI_DEV_ID_CB);     /*  ��ȡ��Ĵ�С                */

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: pciSioNetmosRemove
** ��������: �������Ƴ� PCI �豸ʱ�Ĵ���
** �䡡��  : hPciDevHandle     PCI �豸���
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static VOID  pciSioNetmosRemove (PCI_DEV_HANDLE hPciDevHandle)
{
}
/*********************************************************************************************************
** ��������: pciSioNetmosProbe
** ��������: NETMOS 16c550 �忨����̽���豸
** �䡡��  : hDevHandle         PCI �豸���ƿ���
**           hIdEntry           ƥ��ɹ����豸 ID ��Ŀ(�������豸 ID ��)
** �䡡��  : ERROR or OK
** ȫ�ֱ���:
** ����ģ��:
                                           API ����
*********************************************************************************************************/
static INT  pciSioNetmosProbe (PCI_DEV_HANDLE hPciDevHandle, const PCI_DEV_ID_HANDLE hIdEntry)
{
    INT                     i, iChanNum, iTtyNum;
    PCI_SIO_NETMOS         *pcisio;
    PCI_SIO_NETMOS_CFG     *pcisiocfg;
    SIO16C550_CHAN         *psiochan;
    SIO_CHAN               *psio;
    
    ULONG                   ulVector;
    CHAR                    cDevName[64];
    PCI_RESOURCE_HANDLE     hResource;
    phys_addr_t             paBaseAddr;                                 /*  ��ʼ��ַ                    */
    addr_t                  ulBaseAddr;                                 /*  ��ʼ��ַ                    */
    size_t                  stBaseSize;                                 /*  ��Դ��С                    */
    
    if ((!hPciDevHandle) || (!hIdEntry)) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }

    if (hIdEntry->PCIDEVID_ulData > ARRAY_SIZE(pciSioNetmosCard)) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }

    hResource  = API_PciDevResourceGet(hPciDevHandle, PCI_IORESOURCE_MEM, 0);
    paBaseAddr = (phys_addr_t)(PCI_RESOURCE_START(hResource));          /*  ��ȡ MEM ����ʼ��ַ         */
    stBaseSize = (size_t)(PCI_RESOURCE_SIZE(hResource));                /*  ��ȡ MEM �Ĵ�С             */
    ulBaseAddr = (addr_t)API_PciDevIoRemap2(paBaseAddr, stBaseSize);
    if (!ulBaseAddr) {
        return  (PX_ERROR);
    }

    pcisio    = &pciSioNetmosCard[hIdEntry->PCIDEVID_ulData];
    iChanNum  = pcisio->NETMOS_uiPorts;                                 /*  ����豸ͨ����              */

    hResource = API_PciDevResourceGet(hPciDevHandle, PCI_IORESOURCE_IRQ, 0);
    ulVector  = (ULONG)PCI_RESOURCE_START(hResource);

    API_PciDevMasterEnable(hPciDevHandle, LW_TRUE);

    write32(0, ulBaseAddr + 0x3fc);

    /*
     *  ��������ͨ��
     */
    for (i = 0; i < iChanNum; ++i) {
        psiochan = (SIO16C550_CHAN *)__SHEAP_ZALLOC(sizeof(SIO16C550_CHAN));
        if (!psiochan) {
            _ErrorHandle(ENOMEM);
            return  (PX_ERROR);
        }

        pcisiocfg = (PCI_SIO_NETMOS_CFG *)__SHEAP_ZALLOC(sizeof(PCI_SIO_NETMOS_CFG));
        if (!pcisiocfg) {
            __SHEAP_FREE(psiochan);
            _ErrorHandle(ENOMEM);
            return  (PX_ERROR);
        }

        pcisiocfg->CFG_idx       = hPciDevHandle->PCIDEV_iDevFunction;
        pcisiocfg->CFG_ulVector  = ulVector;
        pcisiocfg->CFG_ulBase    = ulBaseAddr;
        pcisiocfg->CFG_ulBaud    = pcisio->NETMOS_uiBaud;
        pcisiocfg->CFG_ulXtal    = pcisio->NETMOS_uiBaud * 16;
        pcisiocfg->CFG_pciHandle = hPciDevHandle;

        psio = pciSioNetmosChan(hPciDevHandle->PCIDEV_iDevFunction, psiochan, pcisiocfg);

        for (iTtyNum = 0; iTtyNum < 512; iTtyNum++) {
            snprintf(cDevName, sizeof(cDevName), 
                     PCI_SIO_NETMOS_TTY_PERFIX "%d", iTtyNum);
            if (!API_IosDevMatchFull(cDevName)) {
                break;
            }
        }
        
        ttyDevCreate(cDevName, psio, 
                     PCI_SIO_NETMOS_TTY_RBUF_SZ, 
                     PCI_SIO_NETMOS_TTY_SBUF_SZ);                       /*  add tty device              */
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: pciSioNetmosInit
** ��������: PCI NETMOS 16c550 ������������س�ʼ��
** �䡡��  : NONE
** �䡡��  : ERROR or OK
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
INT  pciSioNetmosInit (VOID)
{
    INT               iRet;
    PCI_DRV_CB        tPciDrv;
    PCI_DRV_HANDLE    hPciDrv = &tPciDrv;

    lib_bzero(hPciDrv, sizeof(PCI_DRV_CB));
    iRet = pciSioNetmosIdTblGet(&hPciDrv->PCIDRV_hDrvIdTable, &hPciDrv->PCIDRV_uiDrvIdTableSize);
    if (iRet != ERROR_NONE) {
        return  (PX_ERROR);
    }

    lib_strlcpy(&hPciDrv->PCIDRV_cDrvName[0], "pci_netmos", PCI_DRV_NAME_MAX);
    hPciDrv->PCIDRV_pvPriv         = LW_NULL;                           /*  �豸������˽������          */
    hPciDrv->PCIDRV_hDrvErrHandler = LW_NULL;                           /*  ����������                */
    hPciDrv->PCIDRV_pfuncDevProbe  = pciSioNetmosProbe;
    hPciDrv->PCIDRV_pfuncDevRemove = pciSioNetmosRemove;

    iRet = API_PciDrvRegister(hPciDrv);
    if (iRet != ERROR_NONE) {
        return  (PX_ERROR);
    }

    return  (ERROR_NONE);
}

#endif                                                                  /*  (LW_CFG_DEVICE_EN > 0) &&   */
                                                                        /*  (LW_CFG_PCI_EN > 0) &&      */
                                                                        /*  (LW_CFG_DRV_SIO_16C550 > 0) */
/*********************************************************************************************************
  END
*********************************************************************************************************/
