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
** ��   ��   ��: pciStorageAta.c
**
** ��   ��   ��: Gong.YuJian (�����)
**
** �ļ���������: 2018 �� 09 �� 04 ��
**
** ��        ��: ATA/IDE ����.

** BUG:
** 2019.07.02  ���� INTEL ICH4 IDE ��������ʱ�����⴦��, �������ֿ�������λʧ��.
*********************************************************************************************************/
#define  __SYLIXOS_PCI_DRV
#define  __SYLIXOS_ATA_DRV
#define  __SYLIXOS_STDIO
#define  __SYLIXOS_KERNEL
#include "SylixOS.h"
#include "../SylixOS/config/driver/drv_cfg.h"
/*********************************************************************************************************
  �ü���
*********************************************************************************************************/
#if (LW_CFG_DEVICE_EN > 0) && (LW_CFG_PCI_EN > 0) && (LW_CFG_ATA_EN > 0) && (LW_CFG_DRV_ATA_IDE > 0)
#include "pci_ids.h"
#include "pciStorageAta.h"
#include "../SylixOS/system/device/ata/ataLib.h"
/*********************************************************************************************************
  ��������.
*********************************************************************************************************/
enum {
    board_std = 0,
    board_res = 1
};
/*********************************************************************************************************
  ����֧�ֵ��豸 ID ��.
*********************************************************************************************************/
static const PCI_DEV_ID_CB  pciStorageAtaIdTbl[] = {
    {   PCI_VDEVICE(INTEL, 0x1c01),     board_res   },
    {   PCI_VDEVICE(INTEL, 0x1c09),     board_res   },
    {   PCI_VDEVICE(INTEL, 0x1f20),     board_res   },
    {   PCI_VDEVICE(INTEL, 0x1f21),     board_res   },
    {   PCI_VDEVICE(INTEL, 0x2411),     board_res   },
    {   PCI_VDEVICE(INTEL, 0x2421),     board_res   },
    {   PCI_VDEVICE(INTEL, 0x244b),     board_res   },
    {   PCI_VDEVICE(INTEL, 0x248a),     board_res   },
    {   PCI_VDEVICE(INTEL, 0x24aa),     board_res   },
    {   PCI_VDEVICE(INTEL, 0x24d1),     board_res   },
    {   PCI_VDEVICE(INTEL, 0x24db),     board_res   },
    {   PCI_VDEVICE(INTEL, 0x24df),     board_res   },
    {   PCI_VDEVICE(INTEL, 0x25a2),     board_res   },
    {   PCI_VDEVICE(INTEL, 0x25a3),     board_res   },
    {   PCI_VDEVICE(INTEL, 0x2680),     board_res   },
    {   PCI_VDEVICE(INTEL, 0x24aa),     board_res   },
    {   PCI_VDEVICE(INTEL, 0x2651),     board_res   },
    {   PCI_VDEVICE(INTEL, 0x2652),     board_res   },
    {   PCI_VDEVICE(INTEL, 0x2653),     board_res   },
    {   PCI_VDEVICE(INTEL, 0x266f),     board_res   },
    {   PCI_VDEVICE(INTEL, 0x2680),     board_res   },
    {   PCI_VDEVICE(INTEL, 0x269e),     board_res   },
    {   PCI_VDEVICE(INTEL, 0x27c0),     board_std   },
    {   PCI_VDEVICE(INTEL, 0x2828),     board_res   },
    {   PCI_VDEVICE(INTEL, 0x2850),     board_res   },
    {   PCI_VDEVICE(INTEL, 0x2920),     board_res   },
    {   PCI_VDEVICE(INTEL, 0x2c8a),     board_res   },
    {   PCI_VDEVICE(INTEL, 0x3a20),     board_res   },
    {   PCI_VDEVICE(INTEL, 0x3a26),     board_res   },
    {   PCI_VDEVICE(INTEL, 0x3b20),     board_res   },
    {   PCI_VDEVICE(INTEL, 0x3b2D),     board_res   },
    {   PCI_VDEVICE(INTEL, 0x3b2e),     board_res   },
    {   PCI_VDEVICE(INTEL, 0x3b66),     board_res   },
    {   PCI_VDEVICE(INTEL, 0x5028),     board_res   },
    {   PCI_VDEVICE(INTEL, 0x7010),     board_std   },
    {   PCI_VDEVICE(INTEL, 0x811a),     board_res   },
    {   PCI_VDEVICE(INTEL, 0x8c00),     board_res   },
    {   PCI_VDEVICE(INTEL, 0x8c01),     board_res   },
    {   PCI_VDEVICE(INTEL, 0x8c80),     board_res   },
    {   PCI_VDEVICE(INTEL, 0x8c81),     board_res   },
    {   PCI_VDEVICE(INTEL, 0x8d00),     board_res   },
    {   PCI_VDEVICE(INTEL, 0x8d60),     board_res   },
    {   PCI_VDEVICE(INTEL, 0x9c00),     board_res   },
    {   PCI_VDEVICE(INTEL, 0x9c01),     board_res   },

    {   PCI_ANY_ID, PCI_ANY_ID, PCI_ANY_ID, PCI_ANY_ID,
        PCI_CLASS_STORAGE_IDE << 8, 0xffffff00ul, board_res
    },

    { }                                                                 /* terminate list               */
};
/*********************************************************************************************************
  I82371AB ���������ü�����ѡ��
*********************************************************************************************************/
#define ATA_I82371AB_CTRL_MAX               0x2                         /* ����������                   */
#define ATA_I82371AB_DRIVE_MAX              0x2                         /* ����������                   */

#define ATA_I82371AB_MAC_512                0x00200                     /* 2 ����  9 λ                 */
#define ATA_I82371AB_MAC_4K                 0x01000                     /* 2 ���� 12 λ                 */
#define ATA_I82371AB_MAC_64K                0x10000                     /* 2 ���� 16 λ                 */

#define ATA_I82371AB_IN8(dma, add)          in8((addr_t)add)
#define ATA_I82371AB_IN32(dma, add)         in32((addr_t)add)
#define ATA_I82371AB_OUT8(dma, add, byte)   out8((UINT8)byte, (addr_t)add)
#define ATA_I82371AB_OUT32(dma, add, word)  out32((UINT32)word, (addr_t)add)
/*********************************************************************************************************
  5.1.3     PCICMD��PCI COMMAND REGISTER (04�C05h)
*********************************************************************************************************/
#define ATA_I82371AB_PCICMD_BME             0x0004                      /* Bus Master Function Enable   */
#define ATA_I82371AB_PCICMD_IOSE            0x0001                      /* I/O Space Enable             */
/*********************************************************************************************************
  5.1.9     BMIBA��BUS MASTER INTERFACE BASE ADDRESS REGISTER (20�C23h)
*********************************************************************************************************/
#define ATA_PCI_CFG_BMIBA                   PCI_CFG_BASE_ADDRESS_4
#define ATA_I82371AB_BMBIA_ADD_MASK         0xfff0                      /* Bits 15 to 4                 */
/*********************************************************************************************************
  5.1.10    IDETIM��IDE TIMING REGISTER (40�C41h=Primary Channel; 42�C43h=Secondary Channel)
  5.1.11    SIDETIM��SLAVE IDE TIMING REGISTER (44h)
  5.1.12    UDMACTL��ULTRA DMA/33 CONTROL REGISTER (48h)
  5.1.13    UDMATIM��ULTRA DMA/33 TIMING REGISTER (4A�C4Bh)
*********************************************************************************************************/
#define ATA_PCI_CFG_IDETIM(ctrl)            (0x40 + ctrl * 2)
#define ATA_PCI_CFG_IDETIM_PRI              0x40                        /* 0x40-0x41 def = 0x0000 R/W   */
#define ATA_PCI_CFG_IDETIM_SEC              0x42                        /* 0x42-0x43 def = 0x0000 R/W   */
#define ATA_PCI_CFG_SIDETIME                0x44                        /* 0x44      def = 0x00   R/W   */
#define ATA_PCI_CFG_UDMACTL                 0x48                        /* 0x48      def = 0x00   R/W   */
#define ATA_PCI_CFG_UDMATIM                 0x4a                        /* 0x4A-0x4B def = 0x0000 R/W   */

#define ATA_I82371AB_IDETIM_IDE             0x8000                      /* IDE Decode Enable            */
#define ATA_I82371AB_IDETIM_PPE0            0x0004                      /* Prefetch and Posting Enable  */
#define ATA_I82371AB_IDETIM_IE0             0x0002                      /* IORDY Sample Point Enable 0  */
/*********************************************************************************************************
  5.2.1     BMICX��BUS MASTER IDE COMMAND REGISTER (IO)
      (Primary Channel��Base + 00h; Secondary Channel��Base + 08h)
  5.2.2     BMISX��BUS MASTER IDE STATUS REGISTER (IO)
      (Primary Channel��Base + 02h; Secondary Channel��Base + 0Ah)
  5.2.3     BMIDTPX��BUS MASTER IDE DESCRIPTOR TABLE POINTER REGISTER (IO)
      (Primary Channel��Base + 04h; Secondary Channel��Base + 0Ch)
*********************************************************************************************************/
#define ATA_I82371AB_BMICX(dma, ctrl)       (UINT32)(dma->ATAPCIDMA_uiBmiba + 0x0 + ctrl * 8)
#define ATA_I82371AB_BMISX(dma, ctrl)       (UINT32)(dma->ATAPCIDMA_uiBmiba + 0x2 + ctrl * 8)
#define ATA_I82371AB_BMIDX(dma, ctrl)       (UINT32)(dma->ATAPCIDMA_uiBmiba + 0x4 + ctrl * 8)

#define ATA_I82371AB_BMICX_RWCON            0x08                        /* Bus Master Read/Write Control*/
#define ATA_I82371AB_BMICX_SSBM             0x01                        /* Start/Stop Bus Master        */

#define ATA_I82371AB_BMISX_DMA1CAP          0x40                        /* Drive 1 DMA Capable          */
#define ATA_I82371AB_BMISX_DMA0CAP          0x20                        /* Drive 0 DMA Capable          */
#define ATA_I82371AB_BMISX_IDEINTS          0x04                        /* IDE Interrupt Status         */
#define ATA_I82371AB_BMISX_IDEDMA_ERROR     0x02                        /* IDE DMA Error                */
#define ATA_I82371AB_BMISX_BMIDE            0x01                        /* Bus Master IDE Active        */
/*********************************************************************************************************
  Read unused ISA register, about 720ns per read (A small delay)
*********************************************************************************************************/
#define ATA_I82371AB_DELAY()                do {            \
                                                in8(0x84);  \
                                                in8(0x84);  \
                                                in8(0x84);  \
                                                in8(0x84);  \
                                            } while (0)
/*********************************************************************************************************
  ATA PCI ���� DMA ������
  PCI Configuaration Registers
  5.1.3     PCICMD��PCI COMMAND REGISTER (04�C05h)
  5.1.9     BMIBA��BUS MASTER INTERFACE BASE ADDRESS REGISTER (20�C23h)
  5.1.10    IDETIM��IDE TIMING REGISTER (40�C41h=Primary Channel; 42�C43h=Secondary Channel)
  5.1.11    SIDETIM��SLAVE IDE TIMING REGISTER (44h)
  5.1.12    UDMACTL��ULTRA DMA/33 CONTROL REGISTER (48h)
  5.1.13    UDMATIM��ULTRA DMA/33 TIMING REGISTER (4A�C4Bh)

  PCI IO Space Registers
  5.2.1     BMICX��BUS MASTER IDE COMMAND REGISTER (IO)
      (Primary Channel��Base + 00h; Secondary Channel��Base + 08h)
  5.2.2     BMISX��BUS MASTER IDE STATUS REGISTER (IO)
      (Primary Channel��Base + 02h; Secondary Channel��Base + 0Ah)
  5.2.3     BMIDTPX��BUS MASTER IDE DESCRIPTOR TABLE POINTER REGISTER (IO)
      (Primary Channel��Base + 04h; Secondary Channel��Base + 0Ch)
*********************************************************************************************************/
typedef struct ata_pci_dma_ctrl {
    PVOID           ATAPCIDMA_pvHandle;                                 /* �����������                 */
    /*
     * ���ÿռ�
     */
    UINT16      ATAPCIDMA_usPciCmd;                                     /* PCI COMMAND REGISTER         */
    UINT16      ATAPCIDMA_usIdeTim[2];                                  /* IDE TIMING REGISTER          */
    UINT8       ATAPCIDMA_ucSlaveIdeTim;                                /* SLAVE IDE TIMING REGISTER    */
    UINT8       ATAPCIDMA_ucUdmaCtl;                                    /* ULTRA DMA/33 CONTROL REGISTER*/
    UINT16      ATAPCIDMA_usUdmaTim;                                    /* ULTRA DMA/33 CONTROL REGISTER*/

    /*
     * IO �ռ�
     */
    UINT32      ATAPCIDMA_uiBmiba;                                      /* BUS MASTER INTERFACE BASE    */
    UINT8       ATAPCIDMA_ucBmicx[2];                                   /* BUS MASTER IDE COMMAND REG   */
    UINT8       ATAPCIDMA_ucBmisx[2];                                   /* BUS MASTER IDE STATUS REG    */
    UINT32      ATAPCIDMA_uiBmidtpx[2];                                 /* BUS MASTER IDE DESCRIPTOR TAB*/

    UINT32     *ATAPCIDMA_puiPrdTable[ATA_I82371AB_CTRL_MAX];           /* PRDT                         */
} ATA_PCI_DMA_CTRL_CB;

typedef ATA_PCI_DMA_CTRL_CB    *ATA_PCI_DMA_CTRL_HANDLE;
/*********************************************************************************************************
** ��������: pciStorageAtaCtrlIch4Delay
** ��������: ICH4 ��������ʱ����
** �䡡��  : hCtrl      ���������
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static VOID  pciStorageAtaCtrlIch4Delay (ATA_CTRL_HANDLE  hCtrl)
{
    UINT8       ucReg;

    ATA_IO_BYTES_READ(hCtrl, ATA_A_STATUS_ADDR(hCtrl), &ucReg, 1);
    ATA_IO_BYTES_READ(hCtrl, ATA_A_STATUS_ADDR(hCtrl), &ucReg, 1);
    ATA_IO_BYTES_READ(hCtrl, ATA_A_STATUS_ADDR(hCtrl), &ucReg, 1);
    ATA_IO_BYTES_READ(hCtrl, ATA_A_STATUS_ADDR(hCtrl), &ucReg, 1);
    ATA_IO_BYTES_READ(hCtrl, ATA_A_STATUS_ADDR(hCtrl), &ucReg, 1);
    ATA_IO_BYTES_READ(hCtrl, ATA_A_STATUS_ADDR(hCtrl), &ucReg, 1);

    ATA_IO_BYTES_READ(hCtrl, ATA_A_STATUS_ADDR(hCtrl), &ucReg, 1);
    ATA_IO_BYTES_READ(hCtrl, ATA_A_STATUS_ADDR(hCtrl), &ucReg, 1);
    ATA_IO_BYTES_READ(hCtrl, ATA_A_STATUS_ADDR(hCtrl), &ucReg, 1);
}
/*********************************************************************************************************
** ��������: pciStorageAtaCtrlQuirk
** ��������: �����������⴦��
** �䡡��  : hPciDev        PCI �豸���
**           hDrv           �������
**           hCtrlCfg       ���������þ��
** �䡡��  : ERROR or OK
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  pciStorageAtaCtrlQuirk (PCI_DEV_HANDLE       hPciDev,
                                    ATA_DRV_HANDLE       hDrv,
                                    ATA_CTRL_CFG_HANDLE  hCtrlCfg)
{
    UINT16      usVendorId;
    UINT16      usDeviceId;

    if ((!hPciDev) || (!hDrv) || (!hCtrlCfg)) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }

    usVendorId = PCI_DEV_VENDOR_ID(hPciDev);
    usDeviceId = PCI_DEV_DEVICE_ID(hPciDev);

    switch (usVendorId) {

    case PCI_VENDOR_ID_INTEL:
        if (usDeviceId == PCI_DEVICE_ID_INTEL_82801DB_11) {
            hCtrlCfg->ATACTRLCFG_pfuncDelay = (FUNCPTR)pciStorageAtaCtrlIch4Delay;
        }
        break;

    default:
        break;
    }

    return (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: pciStorageAtaDevIdTblGet
** ��������: ��ȡ�豸�б�
** �䡡��  : phPciDevId     �豸 ID �б���������
**           puiSzie        �豸�б��С
** �䡡��  : ERROR or OK
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  pciStorageAtaDevIdTblGet (PCI_DEV_ID_HANDLE  *phPciDevId, UINT32  *puiSzie)
{
    if ((!phPciDevId) || (!puiSzie)) {
        return  (PX_ERROR);
    }

    *phPciDevId = (PCI_DEV_ID_HANDLE)pciStorageAtaIdTbl;
    *puiSzie    = sizeof(pciStorageAtaIdTbl) / sizeof(PCI_DEV_ID_CB);

    return (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: pciStorageAtaDevRemove
** ��������: �Ƴ� ATA �豸
** �䡡��  : hPciDev        PCI �豸���ƿ���
** �䡡��  : ���������
** ȫ�ֱ���:
** ����ģ��:
                                           API ����
*********************************************************************************************************/
static VOID  pciStorageAtaDevRemove (PCI_DEV_HANDLE  hPciDev)
{
}
/*********************************************************************************************************
** ��������: pciStorageAtaDmaCtrlInit
** ��������: ��ʼ�� PCI IDE DMA ������
** �䡡��  : hDrv       �������
**           uiCtrl     ��������
** �䡡��  : ERROR or OK
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  pciStorageAtaDmaCtrlInit (ATA_DRV_HANDLE  hDrv, UINT  uiCtrl)
{
    ATA_PCI_DMA_CTRL_HANDLE     hDmaCtrl;
    PCI_DEV_HANDLE              hPciDev;
    PVOID                       pvBuff;
    UINT32                      uiData;

    if ((!hDrv) || (uiCtrl >= ATA_I82371AB_CTRL_MAX)) {
        return  (PX_ERROR);
    }

    hPciDev  = (PCI_DEV_HANDLE)hDrv->ATADRV_pvArg;
    hDmaCtrl = (ATA_PCI_DMA_CTRL_HANDLE)hDrv->ATADRV_tDma.ATADRVDMA_pvDmaCtrl;
    API_PciDevConfigReadDword(hPciDev, ATA_PCI_CFG_BMIBA, (UINT32 *)&hDmaCtrl->ATAPCIDMA_uiBmiba);
    hDmaCtrl->ATAPCIDMA_uiBmiba &= ATA_I82371AB_BMBIA_ADD_MASK;
    if (hDmaCtrl->ATAPCIDMA_puiPrdTable[uiCtrl] == LW_NULL) {
        pvBuff = API_CacheDmaMallocAlign(ATA_I82371AB_MAC_512, API_CacheLine(DATA_CACHE));
        hDmaCtrl->ATAPCIDMA_puiPrdTable[uiCtrl] = (UINT32 *)pvBuff;
        if (hDmaCtrl->ATAPCIDMA_puiPrdTable[uiCtrl]) {
            uiData = (UINT32)(ULONG)hDmaCtrl->ATAPCIDMA_puiPrdTable[uiCtrl];
            ATA_I82371AB_OUT32(hDmaCtrl, ATA_I82371AB_BMIDX(hDmaCtrl, uiCtrl), uiData);

        } else {
            return  (PX_ERROR);
        }

    } else {
        return  (PX_ERROR);
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: pciStorageAtaDmaPrdtSet
** ��������: ͨ�������Ļ��������� PRDT ����
** �䡡��  : hDrv       �������
**           uiCtrl     ��������
**           pcBuff     ��������ַ
**           stSize     ��������С
** �䡡��  : ERROR or OK
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  pciStorageAtaDmaPrdtSet (ATA_DRV_HANDLE  hDrv, UINT  uiCtrl, PCHAR  pcBuff, size_t  stSize)
{
    ATA_PCI_DMA_CTRL_HANDLE     hDmaCtrl;
    UINT32                     *puiPrdt;
    UINT32                      i;

    if ((!hDrv) || (uiCtrl >= ATA_I82371AB_CTRL_MAX) || (!pcBuff) || (!stSize)) {
        return  (PX_ERROR);
    }

    hDmaCtrl = (ATA_PCI_DMA_CTRL_HANDLE)hDrv->ATADRV_tDma.ATADRVDMA_pvDmaCtrl;
    puiPrdt  = hDmaCtrl->ATAPCIDMA_puiPrdTable[uiCtrl];
    ATA_I82371AB_OUT32(hDmaCtrl, ATA_I82371AB_BMIDX(hDmaCtrl, uiCtrl), (UINT32)(ULONG)puiPrdt);

    *puiPrdt = (UINT32)(ULONG)pcBuff;
    i = ((~((UINT32)(ULONG)pcBuff & 0xffff)) + 1) & 0xffff;
    if ((stSize < i) || ((i == 0) && (stSize <= ATA_I82371AB_MAC_64K))) {
        *(++puiPrdt) = (stSize | 0x80000000);
        stSize = 0;

    } else if (i) {
        *(++puiPrdt) = i;
        stSize  = stSize - i;
        pcBuff += i;
    }

    while (stSize > ATA_I82371AB_MAC_64K) {
        *(++puiPrdt) = (UINT32)(ULONG)pcBuff;
        *(++puiPrdt) = 0x00;
        pcBuff += ATA_I82371AB_MAC_64K;
        stSize -= ATA_I82371AB_MAC_64K;
    }

    if (stSize) {
        *(++puiPrdt) = (UINT32)(ULONG)pcBuff;
        *(++puiPrdt) = (stSize | 0x80000000);
    }

    API_CacheDmaFlush(hDmaCtrl->ATAPCIDMA_puiPrdTable[uiCtrl], (size_t)ATA_I82371AB_MAC_512);

    return(ERROR_NONE);
}
/*********************************************************************************************************
** ��������: pciStorageAtaDmaCtrlCfg
** ��������: ��������֮ǰ�� DMA ������
** �䡡��  : hDrv       �������
**           uiCtrl     ��������
**           uiDrive    ��������
**           pcBuff     ��������ַ
**           stSize     ��������С
** �䡡��  : ERROR or OK
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  pciStorageAtaDmaCtrlCfg (ATA_DRV_HANDLE  hDrv,
                                     UINT            uiCtrl,
                                     UINT            uiDrive,
                                     PCHAR           pcBuff,
                                     size_t          stSize,
                                     INT             iDirection)
{
    ATA_PCI_DMA_CTRL_HANDLE     hDmaCtrl;
    PCI_DEV_HANDLE              hPciDev;
    UINT8                       ucBmicx;
    UINT8                       ucBmisx;
    UINT32                      uiBmidtpx;
    UINT16                      usIdeTim;

    if ((!hDrv)                             ||
        (uiCtrl  >= ATA_I82371AB_CTRL_MAX)  ||
        (uiDrive >= ATA_I82371AB_DRIVE_MAX) ||
        (!pcBuff)                           ||
        (!stSize)) {
        return  (PX_ERROR);
    }

    hPciDev  = (PCI_DEV_HANDLE)hDrv->ATADRV_pvArg;
    hDmaCtrl = (ATA_PCI_DMA_CTRL_HANDLE)hDrv->ATADRV_tDma.ATADRVDMA_pvDmaCtrl;

    hDmaCtrl->ATAPCIDMA_usPciCmd = (ATA_I82371AB_PCICMD_BME | ATA_I82371AB_PCICMD_IOSE);
    API_PciDevConfigWriteWord(hPciDev, PCI_CFG_COMMAND, hDmaCtrl->ATAPCIDMA_usPciCmd);

    usIdeTim = (ATA_I82371AB_IDETIM_IDE | ATA_I82371AB_IDETIM_PPE0 | ATA_I82371AB_IDETIM_IE0);
    hDmaCtrl->ATAPCIDMA_usIdeTim[uiCtrl] = usIdeTim;
    API_PciDevConfigWriteWord(hPciDev, ATA_PCI_CFG_IDETIM(uiCtrl), hDmaCtrl->ATAPCIDMA_usIdeTim[uiCtrl]);

    ucBmicx   = ATA_I82371AB_IN8(hDmaCtrl, ATA_I82371AB_BMICX(hDmaCtrl, uiCtrl));
    ucBmisx   = ATA_I82371AB_IN8(hDmaCtrl, ATA_I82371AB_BMISX(hDmaCtrl, uiCtrl));
    uiBmidtpx = ATA_I82371AB_IN32(hDmaCtrl, ATA_I82371AB_BMIDX(hDmaCtrl,uiCtrl));
    hDmaCtrl->ATAPCIDMA_ucBmicx[uiCtrl]   = ucBmicx;
    hDmaCtrl->ATAPCIDMA_ucBmisx[uiCtrl]   = ucBmisx;
    hDmaCtrl->ATAPCIDMA_uiBmidtpx[uiCtrl] = uiBmidtpx;

    ATA_I82371AB_OUT8(hDmaCtrl, ATA_I82371AB_BMICX(hDmaCtrl, uiCtrl), (ucBmicx & 0xFE));
    ATA_I82371AB_DELAY();
    ATA_I82371AB_OUT8(hDmaCtrl, ATA_I82371AB_BMISX(hDmaCtrl, uiCtrl), (ucBmisx | 0x66));

    pciStorageAtaDmaPrdtSet(hDrv, uiCtrl, pcBuff, stSize);

    ucBmicx  = ATA_I82371AB_IN8(hDmaCtrl, ATA_I82371AB_BMICX(hDmaCtrl, uiCtrl));
    ucBmicx |= ((iDirection == OUT_DATA) ? 0x00 : ATA_I82371AB_BMICX_RWCON);
    hDmaCtrl->ATAPCIDMA_ucBmicx[uiCtrl] = ucBmicx;
    ATA_I82371AB_OUT8(hDmaCtrl, ATA_I82371AB_BMICX(hDmaCtrl, uiCtrl), ucBmicx);

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: pciStorageAtaDmaCtrlStart
** ��������: ���� DMA ����
** �䡡��  : hDrv       �������
**           uiCtrl     ��������
** �䡡��  : ERROR or OK
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  pciStorageAtaDmaCtrlStart (ATA_DRV_HANDLE  hDrv, UINT  uiCtrl)
{
    REGISTER INT                i;
    ATA_PCI_DMA_CTRL_HANDLE     hDmaCtrl;
    ATA_CTRL_HANDLE             hCtrl;
    UINT8                       ucReg;
    UINT8                       ucStatus;
    struct timespec             tvOld;
    struct timespec             tvNow;

    if ((!hDrv) || (uiCtrl >= ATA_I82371AB_CTRL_MAX)) {
        return  (PX_ERROR);
    }

    hDmaCtrl = (ATA_PCI_DMA_CTRL_HANDLE)hDrv->ATADRV_tDma.ATADRVDMA_pvDmaCtrl;
    ucReg = ATA_I82371AB_IN8(hDmaCtrl, ATA_I82371AB_BMISX(hDmaCtrl, uiCtrl));
    if (!(ucReg & ATA_I82371AB_BMISX_BMIDE)) {
        ucReg  = ATA_I82371AB_IN8(hDmaCtrl, ATA_I82371AB_BMICX(hDmaCtrl, uiCtrl));
        ucReg |= ATA_I82371AB_BMICX_SSBM;
        hDmaCtrl->ATAPCIDMA_ucBmicx[uiCtrl] = ucReg;
        ATA_I82371AB_OUT8(hDmaCtrl, ATA_I82371AB_BMICX(hDmaCtrl, uiCtrl), ucReg);

    } else {
        return  (PX_ERROR);
    }

    if (hDrv->ATADRV_iIntDisable) {
        hCtrl = &hDrv->ATADRV_tCtrl[uiCtrl];

        lib_clock_gettime(CLOCK_MONOTONIC, &tvOld);
        for (i = 0; i < hCtrl->ATACTRL_ulSyncTimeoutLoop; i++) {
            ucStatus = ATA_I82371AB_IN8(hDmaCtrl, ATA_I82371AB_BMISX(hDmaCtrl, uiCtrl));
            if (ucStatus & ATA_I82371AB_BMISX_IDEINTS) {
                ucStatus = (UINT8)(ucStatus & ~(ATA_I82371AB_BMISX_IDEDMA_ERROR));
                ATA_I82371AB_OUT8(hDmaCtrl, ATA_I82371AB_BMISX(hDmaCtrl, uiCtrl), ucStatus);
                break;
            }

            lib_clock_gettime(CLOCK_MONOTONIC, &tvNow);
            if ((tvNow.tv_sec - tvOld.tv_sec) >= hCtrl->ATACTRL_ulSemSyncTimeout) {
                return  (PX_ERROR);
            }

            ATA_I82371AB_DELAY();
        }

        if (i >= hCtrl->ATACTRL_ulSyncTimeoutLoop) {
            return  (PX_ERROR);
        }
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: pciStorageAtaDmaCtrlStop
** ��������: ֹͣ DMA ����
** �䡡��  : hDrv       �������
**           uiCtrl     ��������
** �䡡��  : ERROR or OK
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  pciStorageAtaDmaCtrlStop (ATA_DRV_HANDLE  hDrv, UINT  uiCtrl)
{
    ATA_PCI_DMA_CTRL_HANDLE     hDmaCtrl;
    UINT8                       ucBmicx;
    UINT8                       ucBmisx;
    UINT32                      uiBmidtpx;

    if ((!hDrv) || (uiCtrl >= ATA_I82371AB_CTRL_MAX)) {
        return  (PX_ERROR);
    }

    hDmaCtrl  = (ATA_PCI_DMA_CTRL_HANDLE)hDrv->ATADRV_tDma.ATADRVDMA_pvDmaCtrl;
    ucBmicx   = ATA_I82371AB_IN8(hDmaCtrl, ATA_I82371AB_BMICX(hDmaCtrl, uiCtrl));
    ucBmisx   = ATA_I82371AB_IN8(hDmaCtrl, ATA_I82371AB_BMISX(hDmaCtrl, uiCtrl));
    uiBmidtpx = ATA_I82371AB_IN32(hDmaCtrl, ATA_I82371AB_BMIDX(hDmaCtrl, uiCtrl));
    hDmaCtrl->ATAPCIDMA_ucBmicx[uiCtrl]   = ucBmicx;
    hDmaCtrl->ATAPCIDMA_ucBmisx[uiCtrl]   = ucBmisx;
    hDmaCtrl->ATAPCIDMA_uiBmidtpx[uiCtrl] = uiBmidtpx;

    ucBmisx |= 0x05;
    ATA_I82371AB_OUT8(hDmaCtrl, ATA_I82371AB_BMISX(hDmaCtrl, uiCtrl), ucBmisx);
    ATA_I82371AB_DELAY();
    ATA_I82371AB_OUT8(hDmaCtrl, ATA_I82371AB_BMICX(hDmaCtrl, uiCtrl), 0x00);

    ucBmicx   = ATA_I82371AB_IN8(hDmaCtrl, ATA_I82371AB_BMICX(hDmaCtrl, uiCtrl));
    ucBmisx   = ATA_I82371AB_IN8(hDmaCtrl, ATA_I82371AB_BMISX(hDmaCtrl, uiCtrl));
    uiBmidtpx = ATA_I82371AB_IN32(hDmaCtrl, ATA_I82371AB_BMIDX(hDmaCtrl, uiCtrl));

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: pciStorageAtaDmaModeGet
** ��������: ��ȡ DMA ����ģʽ
** �䡡��  : iMode      ���� DMA ����ģʽ
** �䡡��  : ERROR or OK
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  pciStorageAtaDmaModeGet (INT  iMode)
{
    switch (iMode) {

    case ATA_DMA_ULTRA_5:
    case ATA_DMA_ULTRA_4:
    case ATA_DMA_ULTRA_3:
    case ATA_DMA_ULTRA_2:
        iMode = ATA_DMA_ULTRA_2;
        break;

    case ATA_DMA_ULTRA_1:
    case ATA_DMA_ULTRA_0:
    case ATA_DMA_MULTI_2:
    case ATA_DMA_MULTI_1:
    case ATA_DMA_MULTI_0:
    case ATA_PIO_4:
    case ATA_PIO_3:
    case ATA_PIO_2:
    case ATA_PIO_1:
    case ATA_PIO_0:
    default:
        break;
    }

    return  (iMode);
}
/*********************************************************************************************************
** ��������: pciStorageAtaDmaCtrlModeSet
** ��������: ���� DMA ����ģʽ
** �䡡��  : hDrv       �������
**           uiCtrl     ��������
**           uiDrive    ��������
**           iMode      ����ģʽ
** �䡡��  : ERROR or OK
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  pciStorageAtaDmaCtrlModeSet (ATA_DRV_HANDLE  hDrv, UINT  uiCtrl, UINT  uiDrive, INT  iMode)
{
    ATA_PCI_DMA_CTRL_HANDLE     hDmaCtrl;
    PCI_DEV_HANDLE              hPciDev;
    UINT8                       ucReg;
    UINT16                      usReg;

    if ((!hDrv) || (uiCtrl >= ATA_I82371AB_CTRL_MAX) || (uiDrive >= ATA_I82371AB_DRIVE_MAX)) {
        return  (PX_ERROR);
    }

    hPciDev  = (PCI_DEV_HANDLE)hDrv->ATADRV_pvArg;
    hDmaCtrl = (ATA_PCI_DMA_CTRL_HANDLE)hDrv->ATADRV_tDma.ATADRVDMA_pvDmaCtrl;

    switch (iMode) {

    case ATA_DMA_ULTRA_5:
    case ATA_DMA_ULTRA_4:
    case ATA_DMA_ULTRA_3:
    case ATA_DMA_ULTRA_2:
    case ATA_DMA_ULTRA_1:
    case ATA_DMA_ULTRA_0:
        API_PciDevConfigReadByte(hPciDev, ATA_PCI_CFG_UDMACTL, &ucReg);
        ucReg |= (UINT8)(0x01 << (uiCtrl * 2 + uiDrive));
        hDmaCtrl->ATAPCIDMA_ucUdmaCtl = ucReg;
        API_PciDevConfigWriteByte(hPciDev, ATA_PCI_CFG_UDMACTL, ucReg);

        API_PciDevConfigReadWord(hPciDev, ATA_PCI_CFG_UDMATIM, &usReg);
        usReg &= (UINT16)(~(0x03 << ((uiCtrl * 2 + uiDrive) * 4)));
        hDmaCtrl->ATAPCIDMA_usUdmaTim = usReg;

        switch (iMode) {

        case ATA_DMA_ULTRA_5:
        case ATA_DMA_ULTRA_3:
        case ATA_DMA_ULTRA_1:
            usReg = 0x01;
            break;

        case ATA_DMA_ULTRA_4:
        case ATA_DMA_ULTRA_2:
            usReg = 0x02;
            break;

        case ATA_DMA_ULTRA_0:
        default:
            break;
        }

        hDmaCtrl->ATAPCIDMA_usUdmaTim |= (UINT16)(usReg << ((uiCtrl * 2 + uiDrive) * 4));
        API_PciDevConfigWriteWord(hPciDev, ATA_PCI_CFG_UDMATIM, hDmaCtrl->ATAPCIDMA_usUdmaTim);
        break;
        
    case ATA_DMA_MULTI_2:
    case ATA_DMA_MULTI_1:
    case ATA_DMA_MULTI_0:
    case ATA_PIO_4:
    case ATA_PIO_3:
    case ATA_PIO_2:
    case ATA_PIO_1:
    case ATA_PIO_0:
    default:
        break;
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: pciStorageAtaIoBytesRead
** ��������: ���ֽڶ�
** �䡡��  : hDrv       �������
**           uiCtrl     ��������
**           addrIo     ��ַ
**           pvBuff     ����
**           stLen      ����
** �䡡��  : ERROR or OK
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  pciStorageAtaIoBytesRead (ATA_DRV_HANDLE  hDrv,
                                      UINT            uiCtrl,
                                      addr_t          addrIo,
                                      PVOID           pvBuff,
                                      size_t          stLen)
{
    ins8(addrIo, pvBuff, stLen);
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: pciStorageAtaIoBytesWrite
** ��������: ����д
** �䡡��  : hDrv       �������
**           uiCtrl     ��������
**           addrIo     ��ַ
**           pvBuff     ����
**           stLen      ����
** �䡡��  : ERROR or OK
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  pciStorageAtaIoBytesWrite (ATA_DRV_HANDLE  hDrv,
                                       UINT            uiCtrl,
                                       addr_t          addrIo,
                                       PVOID           pvBuff,
                                       size_t          stLen)
{
    outs8(addrIo, pvBuff, stLen);
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: pciStorageAtaIoWordsRead
** ��������: ���ֶ�
** �䡡��  : hDrv       �������
**           uiCtrl     ��������
**           addrIo     ��ַ
**           pvBuff     ����
**           stLen      ����
** �䡡��  : ERROR or OK
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  pciStorageAtaIoWordsRead (ATA_DRV_HANDLE  hDrv,
                                      UINT            uiCtrl,
                                      addr_t          addrIo,
                                      PVOID           pvBuff,
                                      size_t          stLen)
{
    ins16(addrIo, pvBuff, stLen);
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: pciStorageAtaIoWordsWrite
** ��������: ����д
** �䡡��  : hDrv       �������
**           uiCtrl     ��������
**           addrIo     ��ַ
**           pvBuff     ����
**           stLen      ����
** �䡡��  : ERROR or OK
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  pciStorageAtaIoWordsWrite (ATA_DRV_HANDLE  hDrv,
                                       UINT            uiCtrl,
                                       addr_t          addrIo,
                                       PVOID           pvBuff,
                                       size_t          stLen)
{
    outs16(addrIo, pvBuff, stLen);
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: pciStorageAtaIoDwordsRead
** ��������: ���ֶ�
** �䡡��  : hDrv       �������
**           uiCtrl     ��������
**           addrIo     ��ַ
**           pvBuff     ����
**           stLen      ����
** �䡡��  : ERROR or OK
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  pciStorageAtaIoDwordsRead (ATA_DRV_HANDLE  hDrv,
                                       UINT            uiCtrl,
                                       addr_t          addrIo,
                                       PVOID           pvBuff,
                                       size_t          stLen)
{
    ins32(addrIo, pvBuff, stLen);
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: pciStorageAtaIoDwordsWrite
** ��������: ����д
** �䡡��  : hDrv       �������
**           uiCtrl     ��������
**           addrIo     ��ַ
**           pvBuff     ����
**           stLen      ����
** �䡡��  : ERROR or OK
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  pciStorageAtaIoDwordsWrite (ATA_DRV_HANDLE  hDrv,
                                        UINT            uiCtrl,
                                        addr_t          addrIo,
                                        PVOID           pvBuff,
                                        size_t          stLen)
{
    outs32(addrIo, pvBuff, stLen);
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: pciStorageAtaCtrlEdgePreIsr
** ��������: ���������жϷ���ǰ�ڴ���
** �䡡��  : hDrv       �������
**           uiCtrl     ��������
** �䡡��  : ERROR or OK
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  pciStorageAtaCtrlEdgePreIsr (ATA_DRV_HANDLE  hDrv, UINT  uiCtrl)
{
    if ((!hDrv) || (uiCtrl >= ATA_I82371AB_CTRL_MAX)) {
        return  (PX_ERROR);
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: pciStorageAtaCtrlLevelPreIsr
** ��������: ��ƽ�����жϷ���ǰ�ڴ���
** �䡡��  : hDrv       �������
**           uiCtrl     ��������
** �䡡��  : ERROR or OK
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  pciStorageAtaCtrlLevelPreIsr (ATA_DRV_HANDLE  hDrv, UINT  uiCtrl)
{
    ATA_PCI_DMA_CTRL_HANDLE     hDmaCtrl;
    UINT8                       ucStatus;

    if ((!hDrv) || (uiCtrl >= ATA_I82371AB_CTRL_MAX)) {
        return  (PX_ERROR);
    }

    hDmaCtrl = (ATA_PCI_DMA_CTRL_HANDLE)hDrv->ATADRV_tDma.ATADRVDMA_pvDmaCtrl;
    ucStatus = ATA_I82371AB_IN8(hDmaCtrl, ATA_I82371AB_BMISX(hDmaCtrl, uiCtrl));
    if (ucStatus & ATA_I82371AB_BMISX_IDEINTS) {
        return  (ERROR_NONE);
    }

    return  (PX_ERROR);
}
/*********************************************************************************************************
** ��������: pciStorageAtaCtrlLevelPostIsr
** ��������: ��ƽ�����жϷ�����ڴ���
** �䡡��  : hDrv       �������
**           uiCtrl     ��������
** �䡡��  : ERROR or OK
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  pciStorageAtaCtrlLevelPostIsr (ATA_DRV_HANDLE  hDrv, UINT  uiCtrl)
{
    ATA_PCI_DMA_CTRL_HANDLE     hDmaCtrl;
    UINT8                       ucStatus;

    if ((!hDrv) || (uiCtrl >= ATA_I82371AB_CTRL_MAX)) {
        return  (PX_ERROR);
    }

    hDmaCtrl = (ATA_PCI_DMA_CTRL_HANDLE)hDrv->ATADRV_tDma.ATADRVDMA_pvDmaCtrl;
    ucStatus = ATA_I82371AB_IN8(hDmaCtrl, ATA_I82371AB_BMISX(hDmaCtrl, uiCtrl));
    ucStatus = (UINT8)(ucStatus & ~(ATA_I82371AB_BMISX_IDEDMA_ERROR));
    ATA_I82371AB_OUT8(hDmaCtrl, ATA_I82371AB_BMISX(hDmaCtrl, uiCtrl), ucStatus);

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: pciStorageAtaCtrlIntConnect
** ��������: �ҽ��жϷ���
** �䡡��  : hDrv       �������
**           uiCtrl     ��������
**           cpcName    �жϷ�������
** �䡡��  : ERROR or OK
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  pciStorageAtaCtrlIntConnect (ATA_DRV_HANDLE  hDrv, UINT  uiCtrl, CPCHAR  cpcName)
{
    INT                 iRet = PX_ERROR;
    ATA_CTRL_HANDLE     hCtrl;
    PCI_DEV_HANDLE      hPciDev;

    if ((!hDrv) || (uiCtrl >= ATA_I82371AB_CTRL_MAX) || (!cpcName)) {
        return  (PX_ERROR);
    }

    hCtrl   = &hDrv->ATADRV_tCtrl[uiCtrl];
    hPciDev = (PCI_DEV_HANDLE)hDrv->ATADRV_pvArg;
    iRet    = API_PciDevInterConnect(hPciDev,
                                     hCtrl->ATACTRL_ulCtlrIntVector, hCtrl->ATACTRL_pfuncCtlrIntServ,
                                     hCtrl, cpcName);
    return  (iRet);
}
/*********************************************************************************************************
** ��������: pciStorageAtaCtrlIntEnable
** ��������: ʹ���жϷ���
** �䡡��  : hDrv       �������
**           uiCtrl     ��������
** �䡡��  : ERROR or OK
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  pciStorageAtaCtrlIntEnable (ATA_DRV_HANDLE  hDrv, UINT  uiCtrl)
{
    INT                 iRet = PX_ERROR;
    ATA_CTRL_HANDLE     hCtrl;
    PCI_DEV_HANDLE      hPciDev;

    if ((!hDrv) || (uiCtrl >= ATA_I82371AB_CTRL_MAX)) {
        return  (PX_ERROR);
    }

    hCtrl   = &hDrv->ATADRV_tCtrl[uiCtrl];
    hPciDev = (PCI_DEV_HANDLE)hDrv->ATADRV_pvArg;
    iRet    = API_PciDevInterEnable(hPciDev,
                                    hCtrl->ATACTRL_ulCtlrIntVector, hCtrl->ATACTRL_pfuncCtlrIntServ,
                                    hCtrl);
    return  (iRet);
}
/*********************************************************************************************************
** ��������: pciStorageAtaDevProbe
** ��������: ATA ����̽�⵽�豸
** �䡡��  : hDevHandle         PCI �豸���ƿ���
**           hIdEntry           ƥ��ɹ����豸 ID ��Ŀ(�������豸 ID ��)
** �䡡��  : ���������
** ȫ�ֱ���:
** ����ģ��:
                                           API ����
*********************************************************************************************************/
static INT  pciStorageAtaDevProbe (PCI_DEV_HANDLE  hPciDev, const PCI_DEV_ID_HANDLE  hIdEntry)
{
    REGISTER INT                i;
    REGISTER INT                j;
    ATA_DRIVE_INFO_HANDLE       hDriveInfo;
    ATA_CTRL_CFG_HANDLE         hCtrlCfg;
    ATA_DRV_CB                  tDrv;
    ATA_DRV_HANDLE              hDrv = &tDrv;
    static INT                  iCtrlIndex = 0;
    UINT8                       ucReg;
    PCI_RESOURCE_HANDLE         hResource;
    ATA_DRV_IO_HANDLE           hDrvIo;
    ATA_DRV_DMA_HANDLE          hDrvDma;
    ATA_DRV_INT_HANDLE          hDrvInt;

    if ((!hPciDev) || (!hIdEntry)) {                                    /*  �豸������Ч                */
        _ErrorHandle(EINVAL);                                           /*  ��Ǵ���                    */
        return  (PX_ERROR);                                             /*  ���󷵻�                    */
    }

    /*
     *  �����豸����, �豸�����汾��Ϣ����ǰ�����豸��������
     */
    hPciDev->PCIDEV_uiDevVersion = ATA_PCI_DRV_VER_NUM;                 /*  ��ǰ�豸�����汾��          */
    hPciDev->PCIDEV_uiUnitNumber = iCtrlIndex++;                        /*  �����豸����                */
    
    /*
     *  ����������ʼ��
     */
    lib_bzero((PVOID)hDrv, sizeof(ATA_DRV_CB));
    lib_strlcpy(&hDrv->ATADRV_cDrvName[0], ATA_PCI_DRV_NAME, ATA_DRV_NAME_MAX);
    hDrv->ATADRV_uiDrvVer     = ATA_CTRL_DRV_VER_NUM;
    hDrv->ATADRV_uiUnitNumber = hPciDev->PCIDEV_uiUnitNumber;
    hDrv->ATADRV_pvArg        = (PVOID)hPciDev;

    hDrv->ATADRV_iIntDisable = LW_FALSE;
    hDrv->ATADRV_iBeSwap     = LW_FALSE;

    /*
     *  ���Ʋ�����ʼ��
     */
    hDrv->ATADRV_uiCtrlNum = ATA_I82371AB_CTRL_MAX;
    if (hDrv->ATADRV_uiCtrlNum > ATA_CTRL_MAX) {                        /*  �豸������Ч                */
        _ErrorHandle(EINVAL);                                           /*  ��Ǵ���                    */
        return  (PX_ERROR);                                             /*  ���󷵻�                    */
    }

    for (i = 0; i < hDrv->ATADRV_uiCtrlNum; i++) {
        hCtrlCfg = &hDrv->ATADRV_tCtrlCfg[i];
        hCtrlCfg->ATACTRLCFG_ucDrives          = ATA_I82371AB_DRIVE_MAX;
        hCtrlCfg->ATACTRLCFG_uiConfigType      = (UINT32)(ATA_GEO_CURRENT | ATA_DMA_AUTO | ATA_BITS_32);
        hCtrlCfg->ATACTRLCFG_ulWdgTimeout      = ATA_WDG_TIMEOUT_DEF;
        if (hDrv->ATADRV_iIntDisable) {
            hCtrlCfg->ATACTRLCFG_ulSyncTimeout = ATA_WDG_TIMEOUT_DEF;
        } else {
            hCtrlCfg->ATACTRLCFG_ulSyncTimeout = ATA_SEM_TIMEOUT_DEF;
        }
        hCtrlCfg->ATACTRLCFG_ulSyncTimeoutLoop = ATA_TIMEOUT_LOOP_NUM;

        if (hCtrlCfg->ATACTRLCFG_ucDrives > ATA_DRIVE_MAX) {            /*  �豸������Ч                */
            _ErrorHandle(EINVAL);                                       /*  ��Ǵ���                    */
            return  (PX_ERROR);                                         /*  ���󷵻�                    */
        }

        /*
         *  ��������������
         */
        for (j = 0; j < hCtrlCfg->ATACTRLCFG_ucDrives; j++) {
            hDriveInfo = &hDrv->ATADRV_tDriveInfo[(i * hCtrlCfg->ATACTRLCFG_ucDrives) + j];

            hDriveInfo->ATADINFO_uiCylinders    = 16383;
            hDriveInfo->ATADINFO_uiHeads        = 16;
            hDriveInfo->ATADINFO_uiSectors      = 63;
            hDriveInfo->ATADINFO_uiBytes        = ATA_SECTOR_SIZE;

            hDriveInfo->ATADINFO_stCacheMemSize = ATA_CACHE_SIZE;
            hDriveInfo->ATADINFO_iBurstRead     = ATA_CACHE_BURST_RD;
            hDriveInfo->ATADINFO_iBurstWrite    = ATA_CACHE_BURST_WR;
            hDriveInfo->ATADINFO_iPipeline      = ATA_CACHE_PL;
            hDriveInfo->ATADINFO_iMsgCount      = ATA_CACHE_MSG_CNT;
        }
    }

    /*
     *  ��������ʼ��
     */
    hCtrlCfg = &hDrv->ATADRV_tCtrlCfg[0];
    API_PciDevConfigReadByte(hPciDev, PCI_CFG_PROGRAMMING_IF, &ucReg);
    if (ucReg & ATA_PRI_NATIVE_EN) {
        hResource = API_PciDevResourceGet(hPciDev, PCI_IORESOURCE_IRQ, 0);
        hCtrlCfg->ATACTRLCFG_ulCtlrIntVector  = (ULONG)(PCI_RESOURCE_START(hResource));
        hCtrlCfg->ATACTRLCFG_pfuncCtlrIntServ = (PINT_SVR_ROUTINE)API_AtaCtrlIrqIsr;
        hCtrlCfg->ATACTRLCFG_pfuncCtrlIntPre  = (FUNCPTR)pciStorageAtaCtrlLevelPreIsr;
        hCtrlCfg->ATACTRLCFG_pfuncCtrlIntPost = (FUNCPTR)pciStorageAtaCtrlLevelPostIsr;

        hResource = API_PciDevResourceGet(hPciDev, PCI_IORESOURCE_IO, 0);
        hCtrlCfg->ATA_CTRLCFG_CMD_BASE = (addr_t)(PCI_RESOURCE_START(hResource));

        hResource = API_PciDevResourceGet(hPciDev, PCI_IORESOURCE_IO, 1);
        hCtrlCfg->ATA_CTRLCFG_CTRL_BASE= (addr_t)(PCI_RESOURCE_START(hResource) + 0x02);

        hResource = API_PciDevResourceGet(hPciDev, PCI_IORESOURCE_IO, 4);
        hCtrlCfg->ATA_CTRLCFG_BUSM_BASE = (addr_t)(PCI_RESOURCE_START(hResource));

        hCtrlCfg->ATA_CTRLCFG_CMD_HANDLE  = hPciDev;
        hCtrlCfg->ATA_CTRLCFG_CTRL_HANDLE = hPciDev;
        hCtrlCfg->ATA_CTRLCFG_BUSM_HANDLE = hPciDev;

    } else {
        hCtrlCfg->ATACTRLCFG_ulCtlrIntVector  = (ULONG)(0x0e);
        hCtrlCfg->ATACTRLCFG_pfuncCtlrIntServ = (PINT_SVR_ROUTINE)API_AtaCtrlIrqIsr;
        hCtrlCfg->ATACTRLCFG_pfuncCtrlIntPre  = (FUNCPTR)pciStorageAtaCtrlEdgePreIsr;
        hCtrlCfg->ATACTRLCFG_pfuncCtrlIntPost = (FUNCPTR)LW_NULL;

        hCtrlCfg->ATA_CTRLCFG_CMD_BASE  = (addr_t)(0x1f0);
        hCtrlCfg->ATA_CTRLCFG_CTRL_BASE = (addr_t)(0x3f4 + 0x02);

        hResource = API_PciDevResourceGet(hPciDev, PCI_IORESOURCE_IO, 0);
        hCtrlCfg->ATA_CTRLCFG_BUSM_BASE = (addr_t)(PCI_RESOURCE_START(hResource));

        hCtrlCfg->ATA_CTRLCFG_CMD_HANDLE  = hPciDev;
        hCtrlCfg->ATA_CTRLCFG_CTRL_HANDLE = hPciDev;
        hCtrlCfg->ATA_CTRLCFG_BUSM_HANDLE = hPciDev;
    }
    hCtrlCfg->ATACTRLCFG_pfuncDelay = (FUNCPTR)LW_NULL;

    pciStorageAtaCtrlQuirk(hPciDev, hDrv, hCtrlCfg);

    hCtrlCfg = &hDrv->ATADRV_tCtrlCfg[1];
    if (ucReg & ATA_SEC_NATIVE_EN) {
        hResource = API_PciDevResourceGet(hPciDev, PCI_IORESOURCE_IRQ, 0);
        hCtrlCfg->ATACTRLCFG_ulCtlrIntVector  = (ULONG)(PCI_RESOURCE_START(hResource));
        hCtrlCfg->ATACTRLCFG_pfuncCtlrIntServ = (PINT_SVR_ROUTINE)API_AtaCtrlIrqIsr;
        hCtrlCfg->ATACTRLCFG_pfuncCtrlIntPre  = (FUNCPTR)pciStorageAtaCtrlLevelPreIsr;
        hCtrlCfg->ATACTRLCFG_pfuncCtrlIntPost = (FUNCPTR)pciStorageAtaCtrlLevelPostIsr;

        hResource = API_PciDevResourceGet(hPciDev, PCI_IORESOURCE_IO, 2);
        hCtrlCfg->ATA_CTRLCFG_CMD_BASE = (addr_t)(PCI_RESOURCE_START(hResource));

        hResource = API_PciDevResourceGet(hPciDev, PCI_IORESOURCE_IO, 3);
        hCtrlCfg->ATA_CTRLCFG_CTRL_BASE= (addr_t)(PCI_RESOURCE_START(hResource) + 0x02);

        hResource = API_PciDevResourceGet(hPciDev, PCI_IORESOURCE_IO, 4);
        hCtrlCfg->ATA_CTRLCFG_BUSM_BASE = (addr_t)(PCI_RESOURCE_START(hResource) + 0x08);

        hCtrlCfg->ATA_CTRLCFG_CMD_HANDLE  = hPciDev;
        hCtrlCfg->ATA_CTRLCFG_CTRL_HANDLE = hPciDev;
        hCtrlCfg->ATA_CTRLCFG_BUSM_HANDLE = hPciDev;

    } else {
        hCtrlCfg->ATACTRLCFG_ulCtlrIntVector  = (ULONG)(0x0f);
        hCtrlCfg->ATACTRLCFG_pfuncCtlrIntServ = (PINT_SVR_ROUTINE)API_AtaCtrlIrqIsr;
        hCtrlCfg->ATACTRLCFG_pfuncCtrlIntPre  = (FUNCPTR)pciStorageAtaCtrlEdgePreIsr;
        hCtrlCfg->ATACTRLCFG_pfuncCtrlIntPost = (FUNCPTR)LW_NULL;

        hCtrlCfg->ATA_CTRLCFG_CMD_BASE  = (addr_t)(0x170);
        hCtrlCfg->ATA_CTRLCFG_CTRL_BASE = (addr_t)(0x374 + 0x02);

        hResource = API_PciDevResourceGet(hPciDev, PCI_IORESOURCE_IO, 0);
        hCtrlCfg->ATA_CTRLCFG_BUSM_BASE = (addr_t)(PCI_RESOURCE_START(hResource) + 0x08);

        hCtrlCfg->ATA_CTRLCFG_CMD_HANDLE  = hPciDev;
        hCtrlCfg->ATA_CTRLCFG_CTRL_HANDLE = hPciDev;
        hCtrlCfg->ATA_CTRLCFG_BUSM_HANDLE = hPciDev;
    }
    hCtrlCfg->ATACTRLCFG_pfuncDelay = (FUNCPTR)LW_NULL;

    pciStorageAtaCtrlQuirk(hPciDev, hDrv, hCtrlCfg);

    /*
     *  ���� IO ������ʼ��
     */
    hDrvIo = &hDrv->ATADRV_tIo;
    hDrvIo->ATADRVIO_pfuncIoBytesRead   = (FUNCPTR)pciStorageAtaIoBytesRead;
    hDrvIo->ATADRVIO_pfuncIoBytesWrite  = (FUNCPTR)pciStorageAtaIoBytesWrite;
    hDrvIo->ATADRVIO_pfuncIoWordsRead   = (FUNCPTR)pciStorageAtaIoWordsRead;
    hDrvIo->ATADRVIO_pfuncIoWordsWrite  = (FUNCPTR)pciStorageAtaIoWordsWrite;
    hDrvIo->ATADRVIO_pfuncIoDwordsRead  = (FUNCPTR)pciStorageAtaIoDwordsRead;
    hDrvIo->ATADRVIO_pfuncIoDwordsWrite = (FUNCPTR)pciStorageAtaIoDwordsWrite;

    /*
     *  ���� DMA ������ʼ��
     */
    hDrvDma = &hDrv->ATADRV_tDma;
    hDrvDma->ATADRVDMA_pvDmaCtrl = (ATA_PCI_DMA_CTRL_HANDLE)__SHEAP_ZALLOC(sizeof(ATA_PCI_DMA_CTRL_CB));
    if (!hDrvDma->ATADRVDMA_pvDmaCtrl) {
        _ErrorHandle(ENOMEM);
        return  (PX_ERROR);
    }
    hDrvDma->ATADRVDMA_pfuncDmaCtrlReset   = (FUNCPTR)LW_NULL;
    hDrvDma->ATADRVDMA_pfuncDmaCtrlInit    = (FUNCPTR)pciStorageAtaDmaCtrlInit;
    hDrvDma->ATADRVDMA_pfuncDmaCtrlModeGet = (FUNCPTR)pciStorageAtaDmaModeGet;
    hDrvDma->ATADRVDMA_pfuncDmaCtrlModeSet = (FUNCPTR)pciStorageAtaDmaCtrlModeSet;
    hDrvDma->ATADRVDMA_pfuncDmaCtrlCfg     = (FUNCPTR)pciStorageAtaDmaCtrlCfg;
    hDrvDma->ATADRVDMA_pfuncDmaCtrlStart   = (FUNCPTR)pciStorageAtaDmaCtrlStart;
    hDrvDma->ATADRVDMA_pfuncDmaCtrlStop    = (FUNCPTR)pciStorageAtaDmaCtrlStop;
    hDrvDma->ATADRVDMA_iDmaCtrlSupport     = LW_TRUE;

    /*
     *  �����жϲ�����ʼ��
     */
    hDrvInt = &hDrv->ATADRV_tInt;
    hDrvInt->ATADRVINT_pfuncCtrlIntConnect = (FUNCPTR)pciStorageAtaCtrlIntConnect;
    hDrvInt->ATADRVINT_pfuncCtrlIntEnable  = (FUNCPTR)pciStorageAtaCtrlIntEnable;

    API_AtaDrvInit(hDrv);

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: pciStorageAtaInit
** ��������: �豸������ʼ��
** �䡡��  : NONE
** �䡡��  : ERROR or OK
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
INT  pciStorageAtaInit (VOID)
{
    INT                 iRet;                                           /*  �������                    */
    PCI_DRV_CB          tPciDrv;                                        /*  ��������������ע������      */
    PCI_DRV_HANDLE      hPciDrv = &tPciDrv;                             /*  �������ƿ���              */

    lib_bzero(hPciDrv, sizeof(PCI_DRV_CB));                             /*  ��λ�������ƿ����          */
    iRet = pciStorageAtaDevIdTblGet(&hPciDrv->PCIDRV_hDrvIdTable, &hPciDrv->PCIDRV_uiDrvIdTableSize);
    if (iRet != ERROR_NONE) {                                           /*  ��ȡ�豸 ID ��ʧ��          */
        return  (PX_ERROR);
    }
                                                                        /*  ������������                */
    lib_strlcpy(&hPciDrv->PCIDRV_cDrvName[0], ATA_PCI_DRV_NAME, PCI_DRV_NAME_MAX);
    hPciDrv->PCIDRV_pvPriv         = LW_NULL;                           /*  �豸������˽������          */
    hPciDrv->PCIDRV_hDrvErrHandler = LW_NULL;                           /*  ����������                */
    hPciDrv->PCIDRV_pfuncDevProbe  = pciStorageAtaDevProbe;             /*  �豸̽�⺯��, ����Ϊ��      */
    hPciDrv->PCIDRV_pfuncDevRemove = pciStorageAtaDevRemove;            /*  �����Ƴ�����, ����Ϊ��      */

    iRet = API_PciDrvRegister(hPciDrv);                                 /*  ע�� PCI �豸����           */
    if (iRet != ERROR_NONE) {
        return  (PX_ERROR);
    }

    return  (ERROR_NONE);
}

#endif                                                                  /*  (LW_CFG_DEVICE_EN > 0) &&   */
                                                                        /*  (LW_CFG_PCI_EN > 0) &&      */
                                                                        /*  (LW_CFG_ATA_EN > 0)         */
                                                                        /*  (LW_CFG_DRV_ATA_IDE > 0)    */
/*********************************************************************************************************
  END
*********************************************************************************************************/
