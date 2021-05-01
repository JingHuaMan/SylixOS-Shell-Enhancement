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
** ��   ��   ��: pciDev.h
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2013 �� 09 �� 28 ��
**
** ��        ��: PCI ��������ģ��, �豸ͷ����.
*********************************************************************************************************/

#ifndef __PCIDEV_H
#define __PCIDEV_H

#include "pciBus.h"
#include "pciPm.h"
#include "pciMsi.h"
#include "pciMsix.h"
#include "pciCap.h"
#include "pciCapExt.h"
#include "pciExpress.h"
#include "pciAuto.h"

/*********************************************************************************************************
  �ü���
*********************************************************************************************************/
#if (LW_CFG_DEVICE_EN > 0) && (LW_CFG_PCI_EN > 0)

/*********************************************************************************************************
  The PCI interface treats multi-function devices as independent
  devices. The slot/function address of each device is encoded
  in a single byte as follows:
  7:3 = slot
  2:0 = function
*********************************************************************************************************/

#define PCI_SLOTFN(slot, func)              ((((slot) & 0x1f) << 3) | ((func) & 0x07))
#define PCI_DEVFN(slot, func)               PCI_SLOTFN(slot, func)
#define PCI_SLOT(slotfn)                    (((slotfn) >> 3) & 0x1f)
#define PCI_FUNC(slotfn)                    ((slotfn) & 0x07)

/*********************************************************************************************************
  pack parameters for the PCI Configuration Address Register
*********************************************************************************************************/
#define PCI_PACKET(bus, slot, func)         ((((bus) << 16) & 0x00ff0000) | \
                                            (((slot) << 11) & 0x0000f800) | \
                                            (((func) << 8)  & 0x00000700))

/*********************************************************************************************************
  T. Straumann, 7/31/2001: increased to 32 - PMC slots are not
  scanned on mvme2306 otherwise
*********************************************************************************************************/

#define PCI_MAX_BUS                         256
#define PCI_MAX_SLOTS                       32
#define PCI_MAX_FUNCTIONS                   8

/*********************************************************************************************************
  Configuration I/O addresses for mechanism 1
*********************************************************************************************************/

#define PCI_CONFIG_ADDR                     0x0cf8                      /* write 32 bits to set address */
#define PCI_CONFIG_DATA                     0x0cfc                      /* 8, 16, or 32 bit accesses    */

/*********************************************************************************************************
  Configuration I/O addresses for mechanism 2
*********************************************************************************************************/

#define PCI_CONFIG_CSE                      0x0cf8                      /* CSE register                 */
#define PCI_CONFIG_FORWARD                  0x0cfa                      /* forward register             */
#define PCI_CONFIG_BASE                     0xc000                      /* base register                */

/*********************************************************************************************************
  ���Բ���
*********************************************************************************************************/
#define PCI_DEBUG_EN                        0                           /* �Ƿ�ʹ�ܵ�����Ϣ             */

/*********************************************************************************************************
  BAR ����
*********************************************************************************************************/
#define PCI_DEV_BAR_MAX                     6                           /* �豸 BAR �������            */
#define PCI_BRG_BAR_MAX                     2                           /* ��   BAR �������            */
#define PCI_CBUS_BAR_MAX                    1                           /* ���� BAR �������            */
#define PCI_BAR_INDEX_0                     0                           /* BAR 0                        */
#define PCI_BAR_INDEX_1                     1                           /* BAR 1                        */
#define PCI_BAR_INDEX_2                     2                           /* BAR 2                        */
#define PCI_BAR_INDEX_3                     3                           /* BAR 3                        */
#define PCI_BAR_INDEX_4                     4                           /* BAR 4                        */
#define PCI_BAR_INDEX_5                     5                           /* BAR 5                        */

#define PCI_CONFIG_LEN_MAX                  256                         /* ���ÿռ��С                 */

/*********************************************************************************************************
  PCI ��Դ����
*********************************************************************************************************/
#define PCI_BAR_TYPE_UNKNOWN                0                           /* Standard PCI BAR probe       */
#define PCI_BAR_TYPE_IO                     1                           /* An io port BAR               */
#define PCI_BAR_TYPE_MEM32                  2                           /* A 32-bit memory BAR          */
#define PCI_BAR_TYPE_MEM64                  3                           /* A 64-bit memory BAR          */

/*********************************************************************************************************
  ��Դ���� (resource) For PCI devices, the region numbers are assigned this way
*********************************************************************************************************/
enum {
    /*
     *  0-5: standard PCI resources
     */
    PCI_STD_RESOURCES,
    PCI_STD_RESOURCE_END = 5,

    /*
     *  6: expansion ROM resource
     */
    PCI_ROM_RESOURCE,

    PCI_IRQ_RESOURCE,

    /*
     *  device specific resources
     */
#ifdef CONFIG_PCI_IOV
    PCI_IOV_RESOURCES,
    PCI_IOV_RESOURCE_END = PCI_IOV_RESOURCES + PCI_SRIOV_NUM_BARS - 1,
#endif

    /*
     *  resources assigned to buses behind the bridge
     */
#define PCI_BRIDGE_RESOURCE_NUM 4

    PCI_BRIDGE_RESOURCES,
    PCI_BRIDGE_RESOURCE_END = PCI_BRIDGE_RESOURCES + PCI_BRIDGE_RESOURCE_NUM - 1,

    /*
     *  total resources associated with a PCI device
     */
    PCI_NUM_RESOURCES,

    /*
     *  preserve this for compatibility
     */
    DEVICE_COUNT_RESOURCE = PCI_NUM_RESOURCES
};

/*********************************************************************************************************
  IO resources have these defined flags
*********************************************************************************************************/
#define PCI_IORESOURCE_BITS                 0x000000ff                  /* Bus-specific bits            */

#define PCI_IORESOURCE_TYPE_BITS            0x00001f00                  /* Resource type                */
#define PCI_IORESOURCE_IO                   0x00000100                  /* PCI/ISA I/O ports            */
#define PCI_IORESOURCE_MEM                  0x00000200
#define PCI_IORESOURCE_REG                  0x00000300                  /* Register offsets             */
#define PCI_IORESOURCE_IRQ                  0x00000400
#define PCI_IORESOURCE_DMA                  0x00000800
#define PCI_IORESOURCE_BUS                  0x00001000

#define PCI_IORESOURCE_PREFETCH             0x00002000                  /* No side effects              */
#define PCI_IORESOURCE_READONLY             0x00004000
#define PCI_IORESOURCE_CACHEABLE            0x00008000
#define PCI_IORESOURCE_RANGELENGTH          0x00010000
#define PCI_IORESOURCE_SHADOWABLE           0x00020000

#define PCI_IORESOURCE_SIZEALIGN            0x00040000                  /* size indicates alignment     */
#define PCI_IORESOURCE_STARTALIGN           0x00080000                  /* start field is alignment     */

#define PCI_IORESOURCE_MEM_64               0x00100000
#define PCI_IORESOURCE_WINDOW               0x00200000                  /* forwarded by bridge          */
#define PCI_IORESOURCE_MUXED                0x00400000                  /* Resource is software muxed   */

#define PCI_IORESOURCE_EXT_TYPE_BITS        0x01000000                  /* Resource extended types      */
#define PCI_IORESOURCE_SYSRAM               0x01000000                  /* System RAM (modifier)        */

#define PCI_IORESOURCE_EXCLUSIVE            0x08000000                  /* may not map this resource    */

#define PCI_IORESOURCE_DISABLED             0x10000000
#define PCI_IORESOURCE_UNSET                0x20000000                  /* No address assigned yet      */
#define PCI_IORESOURCE_AUTO                 0x40000000
#define PCI_IORESOURCE_BUSY                 0x80000000                  /* has marked  resource busy    */

/*********************************************************************************************************
  I/O resource extended types
*********************************************************************************************************/
#define PCI_IORESOURCE_SYSTEM_RAM           (PCI_IORESOURCE_MEM  | PCI_IORESOURCE_SYSRAM)

/*********************************************************************************************************
  PnP IRQ specific bits (PCI_IORESOURCE_BITS)
*********************************************************************************************************/
#define PCI_IORESOURCE_IRQ_HIGHEDGE         (1 << 0)
#define PCI_IORESOURCE_IRQ_LOWEDGE          (1 << 1)
#define PCI_IORESOURCE_IRQ_HIGHLEVEL        (1 << 2)
#define PCI_IORESOURCE_IRQ_LOWLEVEL         (1 << 3)
#define PCI_IORESOURCE_IRQ_SHAREABLE        (1 << 4)
#define PCI_IORESOURCE_IRQ_OPTIONAL         PCI_PCI_IORESOURCE

/*********************************************************************************************************
  PnP DMA specific bits (PCI_IORESOURCE_BITS)
*********************************************************************************************************/
#define PCI_IORESOURCE_DMA_TYPE_MASK        (3 << 0)
#define PCI_IORESOURCE_DMA_8BIT             (0 << 0)
#define PCI_IORESOURCE_DMA_8AND16BIT        (1 << 0)
#define PCI_IORESOURCE_DMA_16BIT            (2 << 0)

#define PCI_IORESOURCE_DMA_MASTER           (1 << 2)
#define PCI_IORESOURCE_DMA_BYTE             (1 << 3)
#define PCI_IORESOURCE_DMA_WORD             (1 << 4)

#define PCI_IORESOURCE_DMA_SPEED_MASK       (3 << 6)
#define PCI_IORESOURCE_DMA_COMPATIBLE       (0 << 6)
#define PCI_IORESOURCE_DMA_TYPEA            (1 << 6)
#define PCI_IORESOURCE_DMA_TYPEB            (2 << 6)
#define PCI_IORESOURCE_DMA_TYPEF            (3 << 6)

/*********************************************************************************************************
  PnP memory I/O specific bits (PCI_IORESOURCE_BITS)
*********************************************************************************************************/
#define PCI_IORESOURCE_MEM_WRITEABLE        (1 << 0)                    /* dup: _IORESOURCE_READONLY    */
#define PCI_IORESOURCE_MEM_CACHEABLE        (1 << 1)                    /* dup: _IORESOURCE_CACHEABLE   */
#define PCI_IORESOURCE_MEM_RANGELENGTH      (1 << 2)                    /* dup: _IORESOURCE_RANGELENGTH */
#define PCI_IORESOURCE_MEM_TYPE_MASK        (3 << 3)
#define PCI_IORESOURCE_MEM_8BIT             (0 << 3)
#define PCI_IORESOURCE_MEM_16BIT            (1 << 3)
#define PCI_IORESOURCE_MEM_8AND16BIT        (2 << 3)
#define PCI_IORESOURCE_MEM_32BIT            (3 << 3)
#define PCI_IORESOURCE_MEM_SHADOWABLE       (1 << 5)                    /* dup: _IORESOURCE_SHADOWABLE  */
#define PCI_IORESOURCE_MEM_EXPANSIONROM     (1 << 6)

/*********************************************************************************************************
  Maximum PCI I/O space address supported
*********************************************************************************************************/
#define PCI_IO_SPACE_LIMIT                  0xffffffff

/*********************************************************************************************************
  PnP I/O specific bits (PCI_IORESOURCE_BITS)
*********************************************************************************************************/
#define PCI_IORESOURCE_IO_16BIT_ADDR        (1 << 0)
#define PCI_IORESOURCE_IO_FIXED             (1 << 1)
#define PCI_IORESOURCE_IO_SPARSE            (1 << 2)

/*********************************************************************************************************
  PCI ROM control bits (PCI_IORESOURCE_BITS)
*********************************************************************************************************/
#define PCI_IORESOURCE_ROM_ENABLE           (1 << 0)                    /* ROM is enabled               */
#define PCI_IORESOURCE_ROM_SHADOW           (1 << 1)                    /* Use RAM image, not ROM BAR   */

/*********************************************************************************************************
  PCI control bits.  Shares PCI_IORESOURCE_BITS with above PCI ROM
*********************************************************************************************************/
#define PCI_IORESOURCE_PCI_FIXED            (1 << 4)                    /* Do not move resource         */

/*********************************************************************************************************
  �豸����
*********************************************************************************************************/
#define PCI_DEV_NAME_MAX                    (32 + 1)                    /* �豸�������ֵ               */
#define PCI_DEV_IRQ_NAME_MAX                (32 + 1)                    /* �豸�ж��������ֵ           */

/*********************************************************************************************************
  ID ����
*********************************************************************************************************/
#define PCI_ANY_ID                          (~0)                        /* �κ� ID                      */

#define PCI_DEVICE(vend,dev)                (vend),                 \
                                            (dev),                  \
                                            PCI_ANY_ID,             \
                                            PCI_ANY_ID

#define PCI_DEVICE_SUB(vend, dev, subvend, subdev)                  \
                                            (vend),                 \
                                            (dev),                  \
                                            (subvend),              \
                                            (subdev)

#define PCI_DEVICE_CLASS(dev_class,dev_class_mask)                  \
                                            PCI_ANY_ID,             \
                                            PCI_ANY_ID,             \
                                            PCI_ANY_ID,             \
                                            PCI_ANY_ID,             \
                                            (dev_class),            \
                                            (dev_class_mask)        \

#define PCI_VDEVICE(vend, dev)              PCI_VENDOR_ID_##vend,   \
                                            (dev),                  \
                                            PCI_ANY_ID,             \
                                            PCI_ANY_ID,             \
                                            0,                      \
                                            0

/*********************************************************************************************************
  ����ģʽ����
*********************************************************************************************************/
#if PCI_DEBUG_EN > 0                                                    /* �Ƿ�ʹ�� PCI ����ģʽ        */
#define PCI_DEBUG_MSG                       _DebugHandle
#define PCI_DEBUG_MSG_FMT                   _DebugFormat
#else                                                                   /* PCI_DEBUG_EN                 */
#define PCI_DEBUG_MSG(level, msg)
#define PCI_DEBUG_MSG_FMT(level, fmt, ...)
#endif                                                                  /* PCI_DEBUG_EN                 */

/*********************************************************************************************************
  PCI ����
*********************************************************************************************************/

#if LW_CFG_PCI_64 > 0
typedef UINT64          pci_addr_t;
typedef UINT64          pci_size_t;
typedef UINT64          pci_resource_addr_t;
typedef UINT64          pci_resource_size_t;
typedef UINT64          pci_bus_addr_t;
#else
typedef UINT32          pci_addr_t;
typedef UINT32          pci_size_t;
typedef UINT32          pci_resource_addr_t;
typedef UINT32          pci_resource_size_t;
typedef UINT32          pci_bus_addr_t;
#endif                                                                  /*  LW_CFG_PCI_64 > 0           */

/*********************************************************************************************************
   PCI Ӳ�����ӽṹ��ͼ
   
                      +-------+
                      |       |
                      |  CPU  |
                      |       |
                      +-------+
                          |
       Host bus           |
    ----------------------+-------------------------- ...
                          |
                 +------------------+
                 |     Bridge 0     |
                 |  (host adapter)  |
                 +------------------+
                          |
       PCI bus segment 0  |  (primary bus segment)
    ----------------------+-------------------------- ...
             |            |    |                  |
           dev 0          |   dev 1              dev 2
                          |
                 +------------------+
                 |     Bridge 1     |
                 |   (PCI to PCI)   |
                 +------------------+
                          |
       PCI bus segment 1  |  (secondary bus segment)
    ----------------------+-------------------------- ...
             |            |    |                  |
           dev 0          |   dev 1              dev 2
                          |
                 +------------------+
                 |     Bridge 2     |
                 |   (PCI to PCI)   |
                 +------------------+
                          |
       PCI bus segment 2  |  (tertiary bus segment)
    ----------------------+-------------------------- ...
             |                 |                  |
           dev 0              dev 1              dev 2
*********************************************************************************************************/
/*********************************************************************************************************
  PCI �豸ͷ
*********************************************************************************************************/

typedef struct {
    UINT16          PCID_usVendorId;                                    /* vendor ID                    */
    UINT16          PCID_usDeviceId;                                    /* device ID                    */
    UINT16          PCID_usCommand;                                     /* command register             */
    UINT16          PCID_usStatus;                                      /* status register              */
    
    UINT8           PCID_ucRevisionId;                                  /* revision ID                  */
    UINT8           PCID_ucProgIf;                                      /* programming interface        */
    UINT8           PCID_ucSubClass;                                    /* sub class code               */
    UINT8           PCID_ucClassCode;                                   /* class code                   */
    
    UINT8           PCID_ucCacheLine;                                   /* cache line                   */
    UINT8           PCID_ucLatency;                                     /* latency time                 */
    UINT8           PCID_ucHeaderType;                                  /* header type                  */
    UINT8           PCID_ucBist;                                        /* BIST                         */
    
    UINT32          PCID_uiBase[PCI_DEV_BAR_MAX];                       /* base address                 */
    
    UINT32          PCID_uiCis;                                         /* cardBus CIS pointer          */
    UINT16          PCID_usSubVendorId;                                 /* sub system vendor ID         */
    UINT16          PCID_usSubSystemId;                                 /* sub system ID                */
    UINT32          PCID_uiRomBase;                                     /* expansion ROM base address   */

    UINT32          PCID_uiReserved0;                                   /* reserved                     */
    UINT32          PCID_uiReserved1;                                   /* reserved                     */

    UINT8           PCID_ucIntLine;                                     /* interrupt line               */
    UINT8           PCID_ucIntPin;                                      /* interrupt pin                */
    
    UINT8           PCID_ucMinGrant;                                    /* min Grant                    */
    UINT8           PCID_ucMaxLatency;                                  /* max Latency                  */
} PCI_DEV_HDR;

/*********************************************************************************************************
  PCI ���豸ͷ
*********************************************************************************************************/

typedef struct {
    UINT16          PCIB_usVendorId;                                    /* vendor ID                    */
    UINT16          PCIB_usDeviceId;                                    /* device ID                    */
    UINT16          PCIB_usCommand;                                     /* command register             */
    UINT16          PCIB_usStatus;                                      /* status register              */

    UINT8           PCIB_ucRevisionId;                                  /* revision ID                  */
    UINT8           PCIB_ucProgIf;                                      /* programming interface        */
    UINT8           PCIB_ucSubClass;                                    /* sub class code               */
    UINT8           PCIB_ucClassCode;                                   /* class code                   */
    
    UINT8           PCIB_ucCacheLine;                                   /* cache line                   */
    UINT8           PCIB_ucLatency;                                     /* latency time                 */
    UINT8           PCIB_ucHeaderType;                                  /* header type                  */
    UINT8           PCIB_ucBist;                                        /* BIST                         */

    UINT32          PCIB_uiBase[PCI_BRG_BAR_MAX];                       /* base address                 */

    UINT8           PCIB_ucPriBus;                                      /* primary bus number           */
    UINT8           PCIB_ucSecBus;                                      /* secondary bus number         */
    UINT8           PCIB_ucSubBus;                                      /* subordinate bus number       */
    UINT8           PCIB_ucSecLatency;                                  /* secondary latency timer      */
    UINT8           PCIB_ucIoBase;                                      /* IO base                      */
    UINT8           PCIB_ucIoLimit;                                     /* IO limit                     */

    UINT16          PCIB_usSecStatus;                                   /* secondary status             */

    UINT16          PCIB_usMemBase;                                     /* memory base                  */
    UINT16          PCIB_usMemLimit;                                    /* memory limit                 */
    UINT16          PCIB_usPreBase;                                     /* prefetchable memory base     */
    UINT16          PCIB_usPreLimit;                                    /* prefetchable memory limit    */

    UINT32          PCIB_uiPreBaseUpper;                                /* prefetchable memory base     */
                                                                        /* upper 32 bits                */
    UINT32          PCIB_uiPreLimitUpper;                               /* prefetchable memory limit    */
                                                                        /* upper 32 bits                */

    UINT16          PCIB_usIoBaseUpper;                                 /* IO base upper 16 bits        */
    UINT16          PCIB_usIoLimitUpper;                                /* IO limit upper 16 bits       */

    UINT32          PCIB_uiReserved;                                    /* reserved                     */
    UINT32          PCIB_uiRomBase;                                     /* expansion ROM base address   */

    UINT8           PCIB_ucIntLine;                                     /* interrupt line               */
    UINT8           PCIB_ucIntPin;                                      /* interrupt pin                */

    UINT16          PCIB_usControl;                                     /* bridge control               */
} PCI_BRG_HDR;

/*********************************************************************************************************
  PCI �����豸ͷ
*********************************************************************************************************/

typedef struct {
    UINT16          PCICB_usVendorId;                                   /* vendor ID                    */
    UINT16          PCICB_usDeviceId;                                   /* device ID                    */
    UINT16          PCICB_usCommand;                                    /* command register             */
    UINT16          PCICB_usStatus;                                     /* status register              */

    UINT8           PCICB_ucRevisionId;                                 /* revision ID                  */
    UINT8           PCICB_ucProgIf;                                     /* programming interface        */
    UINT8           PCICB_ucSubClass;                                   /* sub class code               */
    UINT8           PCICB_ucClassCode;                                  /* class code                   */
    
    UINT8           PCICB_ucCacheLine;                                  /* cache line                   */
    UINT8           PCICB_ucLatency;                                    /* latency time                 */
    UINT8           PCICB_ucHeaderType;                                 /* header type                  */
    UINT8           PCICB_ucBist;                                       /* BIST                         */
    
    UINT32          PCICB_uiBase[PCI_CBUS_BAR_MAX];                     /* base address                 */

    UINT8           PCICB_ucCapPtr;                                     /* capabilities pointer         */
    UINT8           PCICB_ucReserved;                                   /* reserved                     */

    UINT16          PCICB_usSecStatus;                                  /* secondary status             */

    UINT8           PCICB_ucPriBus;                                     /* primary bus number           */
    UINT8           PCICB_ucSecBus;                                     /* secondary bus number         */
    UINT8           PCICB_ucSubBus;                                     /* subordinate bus number       */
    UINT8           PCICB_ucSecLatency;                                 /* secondary latency timer      */

    UINT32          PCICB_uiMemBase0;                                   /* memory base 0                */
    UINT32          PCICB_uiMemLimit0;                                  /* memory limit 0               */
    UINT32          PCICB_uiMemBase1;                                   /* memory base 1                */
    UINT32          PCICB_uiMemLimit1;                                  /* memory limit 1               */

    UINT32          PCICB_uiIoBase0;                                    /* IO base 0                    */
    UINT32          PCICB_uiIoLimit0;                                   /* IO limit 0                   */
    UINT32          PCICB_uiIoBase1;                                    /* IO base 1                    */
    UINT32          PCICB_uiIoLimit1;                                   /* IO limit 1                   */

    UINT8           PCICB_ucIntLine;                                    /* interrupt line               */
    UINT8           PCICB_ucIntPin;                                     /* interrupt pin                */

    UINT16          PCICB_usControl;                                    /* bridge control               */
    UINT16          PCICB_usSubVendorId;                                /* sub system vendor ID         */
    UINT16          PCICB_usSubSystemId;                                /* sub system ID                */

    UINT32          PCICB_uiLegacyBase;                                 /* pccard 16bit legacy mode base*/
} PCI_CBUS_HDR;

/*********************************************************************************************************
  PCI ��׼�豸ͷ
*********************************************************************************************************/

typedef struct {
    UINT8               PCIH_ucType;                                    /*  PCI ����                    */
    /*
     * PCI_HEADER_TYPE_NORMAL
     * PCI_HEADER_TYPE_BRIDGE
     * PCI_HEADER_TYPE_CARDBUS
     */
    union {
        PCI_DEV_HDR     PCIHH_pcidHdr;
        PCI_BRG_HDR     PCIHH_pcibHdr;
        PCI_CBUS_HDR    PCIHH_pcicbHdr;
    } hdr;
#define PCIH_pcidHdr    hdr.PCIHH_pcidHdr
#define PCIH_pcibHdr    hdr.PCIHH_pcibHdr
#define PCIH_pcicbHdr   hdr.PCIHH_pcicbHdr
} PCI_HDR;

/*********************************************************************************************************
  PCI I/O Drv
  
  ע��: irqGet() ������ iMsiEn == 0 ʱ pvIrq Ϊ ULONG *pulVect
                     �� iMsiEn != 0 ʱ pvIrq Ϊ PCI_MSI_DESC *pmsidesc
*********************************************************************************************************/

typedef struct pci_drv_funcs0 {
    INT     (*cfgRead)(INT iBus, INT iSlot, INT iFunc, INT iOft, INT iLen, PVOID pvRet);
    INT     (*cfgWrite)(INT iBus, INT iSlot, INT iFunc, INT iOft, INT iLen, UINT32 uiData);
    INT     (*vpdRead)(INT iBus, INT iSlot, INT iFunc, INT iPos, UINT8 *pucBuf, INT iLen);
    INT     (*irqGet)(INT iBus, INT iSlot, INT iFunc, INT iMsiEn, INT iLine, INT iPin, PVOID pvIrq);
    INT     (*cfgSpcl)(INT iBus, UINT32 uiMsg);
} PCI_DRV_FUNCS0;                                                       /*  PCI_MECHANISM_0             */

typedef struct pci_drv_funcs12 {
    UINT8   (*ioInByte)(addr_t ulAddr);
    UINT16  (*ioInWord)(addr_t ulAddr);
    UINT32  (*ioInDword)(addr_t ulAddr);
    VOID    (*ioOutByte)(UINT8 ucValue, addr_t ulAddr);
    VOID    (*ioOutWord)(UINT16 usValue, addr_t ulAddr);
    VOID    (*ioOutDword)(UINT32 uiValue, addr_t ulAddr);
    INT     (*irqGet)(INT iBus, INT iSlot, INT iFunc, INT iMsiEn, INT iLine, INT iPin, PVOID pvIrq);
    INT     (*mmCfgRead)(INT iBus, INT iSlot, INT iFunc, INT iOft, INT iLen, PVOID pvRet);
    INT     (*mmCfgWrite)(INT iBus, INT iSlot, INT iFunc, INT iOft, INT iLen, UINT32 uiData);
} PCI_DRV_FUNCS12;                                                      /*  PCI_MECHANISM_1 , 2         */

/*********************************************************************************************************
  PCI �豸�Զ�������Դ���ƿ�
*********************************************************************************************************/
typedef struct {
    pci_bus_addr_t      PCIAUTOREG_addrBusStart;                        /* ��������ʼ��ַ               */
    pci_bus_addr_t      PCIAUTOREG_addrBusLower;                        /* ������ǰ��ַ               */

    ULONG               PCIAUTOREG_ulFlags;                             /* ������Ϣ                     */
    pci_size_t          PCIAUTOREG_stSize;                              /* �����С                     */
    pci_addr_t          PCIAUTOREG_addrPhyStart;                        /* ��ʼ�����ַ                 */
} PCI_AUTO_REGION_CB;

typedef PCI_AUTO_REGION_CB     *PCI_AUTO_REGION_HANDLE;

/*********************************************************************************************************
  PCI �豸�Զ�����
*********************************************************************************************************/
typedef struct {
    INT                     PCIAUTO_iConfigEn;                          /* �Ƿ�ʹ���Զ�����             */
    INT                     PCIAUTO_iHostBridegCfgEn;                   /* �Ƿ�������ŵ�����           */
    UINT32                  PCIAUTO_uiFirstBusNo;                       /* ��ʼ���ߺ�                   */
    UINT32                  PCIAUTO_uiLastBusNo;                        /* �������ߺ�                   */
    UINT32                  PCIAUTO_uiCurrentBusNo;                     /* ��ǰ���ߺ�                   */

    UINT8                   PCIAUTO_ucCacheLineSize;                    /* ���ٻ����С                 */
    UINT8                   PCIAUTO_ucLatencyTimer;                     /* ʱ�����                     */

    /*
     *  �����豸���ж�
     */
    VOID                  (*PCIAUTO_pfuncDevFixup)(PVOID pvCtrl, PCI_AUTO_DEV_HANDLE hAutoDev,
                                                   UINT16 usVendor, UINT16 usDevice, UINT16 usClass);
    VOID                  (*PCIAUTO_pfuncDevIrqFixup)(PVOID pvCtrl, PCI_AUTO_DEV_HANDLE hAutoDev);

    /*
     *  ��Դ��Ϣ
     */
    UINT32                  PCIAUTO_uiRegionCount;                      /* ��Դ��Ŀ                     */
    PCI_AUTO_REGION_CB      PCIAUTO_tRegion[PCI_AUTO_REGION_MAX];       /* I/O and memory  + ROMs       */
    PCI_AUTO_REGION_HANDLE  PCIAUTO_hRegionIo;                          /* �����Զ����õ� IO            */
    PCI_AUTO_REGION_HANDLE  PCIAUTO_hRegionMem;                         /* �����Զ����õ� MEM           */
    PCI_AUTO_REGION_HANDLE  PCIAUTO_hRegionPre;                         /* �����Զ����õ� PRE           */

    PVOID                   PCIAUTO_pvPriv;                             /* ˽������                     */
} PCI_AUTO_CB;

typedef PCI_AUTO_CB        *PCI_AUTO_HANDLE;

/*********************************************************************************************************
  PCI access config
*********************************************************************************************************/

typedef struct {
    LW_LIST_LINE            PCI_lineDevNode;                            /* �豸����ڵ�                 */
    INT                     PCI_iIndex;                                 /* ����������(��ǰ֧�ֵ�������) */
    INT                     PCI_iBusMax;
    LW_SPINLOCK_DEFINE     (PCI_slLock);                                /* �ײ����������               */

    union {
        PCI_DRV_FUNCS0     *PCIF_pDrvFuncs0;
        PCI_DRV_FUNCS12    *PCIF_pDrvFuncs12;
    } f;
#define PCI_pDrvFuncs0      f.PCIF_pDrvFuncs0
#define PCI_pDrvFuncs12     f.PCIF_pDrvFuncs12
    
#define PCI_MECHANISM_0     0
#define PCI_MECHANISM_1     1
#define PCI_MECHANISM_2     2
    UINT8                   PCI_ucMechanism;
    
    /*
     * �����ַ�������� PCI_MECHANISM_1 �� PCI_MECHANISM_2
     */
    addr_t                  PCI_ulConfigAddr;
    addr_t                  PCI_ulConfigData;
    addr_t                  PCI_ulConfigBase;                           /* only for PCI_MECHANISM_2     */

    PCI_AUTO_CB             PCI_tAutoConfig;                            /* �����Զ�����                 */

    PVOID                   PCI_pvPriv;                                 /* ˽������                     */
} PCI_CTRL_CB;
typedef PCI_CTRL_CB        *PCI_CTRL_HANDLE;

/*********************************************************************************************************
  PCI �豸��Դ���ƿ�
*********************************************************************************************************/
typedef struct {
    pci_resource_size_t     PCIRS_stStart;
    pci_resource_size_t     PCIRS_stEnd;
    PCHAR                   PCIRS_pcName;
    ULONG                   PCIRS_ulFlags;
    ULONG                   PCIRS_ulDesc;
} PCI_RESOURCE_CB;
typedef PCI_RESOURCE_CB    *PCI_RESOURCE_HANDLE;

#define PCI_DEV_RESOURCE_START(dev, index)  ((PCI_DEV_HANDLE)(dev))->PCIDEV_tResource[index].PCIRS_stStart
#define PCI_DEV_RESOURCE_END(dev, index)    ((PCI_DEV_HANDLE)(dev))->PCIDEV_tResource[index].PCIRS_stEnd
#define PCI_DEV_RESOURCE_NAME(dev, index)   ((PCI_DEV_HANDLE)(dev))->PCIDEV_tResource[index].PCIRS_cpcName
#define PCI_DEV_RESOURCE_FLAG(dev, index)   ((PCI_DEV_HANDLE)(dev))->PCIDEV_tResource[index].PCIRS_ulFlags
#define PCI_DEV_RESOURCE_DESC(dev, index)   ((PCI_DEV_HANDLE)(dev))->PCIDEV_tResource[index].PCIRS_ulDesc

#define PCI_RESOURCE_TYPE(handle)           (((PCI_RESOURCE_HANDLE)handle)->PCIRS_ulFlags &     \
                                             PCI_IORESOURCE_TYPE_BITS)

#define PCI_RESOURCE_START(handle)          (((PCI_RESOURCE_HANDLE)handle)->PCIRS_stStart)
#define PCI_RESOURCE_END(handle)            (((PCI_RESOURCE_HANDLE)handle)->PCIRS_stEnd)
#define PCI_RESOURCE_SIZE(handle)           (((PCI_RESOURCE_HANDLE)handle)->PCIRS_stEnd -       \
                                             ((PCI_RESOURCE_HANDLE)handle)->PCIRS_stStart + 1)

#define PCI_DEV_BASE_START(dev, index)      ((!dev) ? ((UINT32)0x0) :       \
                                            ((PCI_DEV_HANDLE)(dev))->PCIDEV_phDevHdr.PCIH_pcidHdr.PCID_uiBase[index])

/*********************************************************************************************************
  PCI �豸 ID
*********************************************************************************************************/
#define PCI_DEV_VENDOR_ID(dev)              ((!dev) ? ((UINT16)0xFFFF) :    \
                                            ((PCI_DEV_HANDLE)(dev))->PCIDEV_phDevHdr.PCIH_pcidHdr.PCID_usVendorId)
#define PCI_DEV_DEVICE_ID(dev)              ((!dev) ? ((UINT16)0xFFFF) :    \
                                            ((PCI_DEV_HANDLE)(dev))->PCIDEV_phDevHdr.PCIH_pcidHdr.PCID_usDeviceId)

#define PCI_DEV_SUB_VENDOR_ID(dev)          ((!dev) ? ((UINT16)0xFFFF) :    \
                                            ((PCI_DEV_HANDLE)(dev))->PCIDEV_phDevHdr.PCIH_pcidHdr.PCID_usSubVendorId)
#define PCI_DEV_SUB_SYSTEM_ID(dev)          ((!dev) ? ((UINT16)0xFFFF) :    \
                                            ((PCI_DEV_HANDLE)(dev))->PCIDEV_phDevHdr.PCIH_pcidHdr.PCID_usSubSystemId)

/*********************************************************************************************************
  PCI �豸���ƿ� (ֻ���� PCI_HEADER_TYPE_NORMAL �����豸)
*********************************************************************************************************/
typedef struct {
    LW_LIST_LINE        PCIDEV_lineDevNode;                             /* �豸����ڵ�                 */
    LW_OBJECT_HANDLE    PCIDEV_hDevLock;                                /* �豸���������               */

    UINT32              PCIDEV_uiDevVersion;                            /* �豸�汾                     */
    UINT32              PCIDEV_uiUnitNumber;                            /* �豸���                     */
    CHAR                PCIDEV_cDevName[PCI_DEV_NAME_MAX];              /* �豸����                     */

    INT                 PCIDEV_iDevBus;                                 /* ���ߺ�                       */
    INT                 PCIDEV_iDevDevice;                              /* �豸��                       */
    INT                 PCIDEV_iDevFunction;                            /* ���ܺ�                       */
    PCI_HDR             PCIDEV_phDevHdr;                                /* �豸ͷ                       */

    /*
     *  PCI_HEADER_TYPE_NORMAL  PCI_HEADER_TYPE_BRIDGE  PCI_HEADER_TYPE_CARDBUS
     */
    INT                 PCIDEV_iType;                                   /* �豸����                     */
    UINT8               PCIDEV_ucPin;                                   /* �ж�����                     */
    UINT8               PCIDEV_ucLine;                                  /* �ж���                       */
    UINT32              PCIDEV_uiIrq;                                   /* �����ж�������               */

    UINT8               PCIDEV_ucRomBaseReg;
    UINT32              PCIDEV_uiResourceNum;
    PCI_RESOURCE_CB     PCIDEV_tResource[PCI_NUM_RESOURCES];            /* I/O and memory  + ROMs       */

    INT                 PCIDEV_iDevIrqMsiEn;                            /* �Ƿ�ʹ�� MSI                 */
    ULONG               PCIDEV_ulDevIrqVector;                          /* MSI �� INTx �ж�����         */
    UINT32              PCIDEV_uiDevIrqMsiNum;                          /* MSI �ж�����                 */
    PCI_MSI_DESC        PCIDEV_pmdDevIrqMsiDesc;                        /* MSI �ж�����                 */

    CHAR                PCIDEV_cDevIrqName[PCI_DEV_IRQ_NAME_MAX];       /* �ж�����                     */
    PINT_SVR_ROUTINE    PCIDEV_pfuncDevIrqHandle;                       /* �жϷ�����                 */
    PVOID               PCIDEV_pvDevIrqArg;                             /* �жϷ������                 */

    PVOID               PCIDEV_pvDevDriver;                             /* �������                     */
    PVOID               PCIDEV_pvPrivate;                               /* �û�����ʹ��                 */
} PCI_DEV_CB;
typedef PCI_DEV_CB     *PCI_DEV_HANDLE;

#define PCI_DEV_MSI_IS_EN(handle)           (((PCI_DEV_HANDLE)handle)->PCIDEV_iDevIrqMsiEn == LW_TRUE)

/*********************************************************************************************************
  ����������
  API_PciCtrlCreate
  ������ BSP ��ʼ������ϵͳʱ������, ���ұ��뱣֤�ǵ�һ������ȷ���õ� PCI ϵͳ����.
*********************************************************************************************************/

LW_API VOID             API_PciCtrlReset(INT  iRebootType);
LW_API PCI_CTRL_HANDLE  API_PciCtrlCreate(PCI_CTRL_HANDLE hCtrl);

/*********************************************************************************************************
  ��������
*********************************************************************************************************/

LW_API INT              API_PciConfigInByte(INT iBus, INT iSlot, INT iFunc, INT iOft, UINT8 *pucValue);
LW_API INT              API_PciConfigInWord(INT iBus, INT iSlot, INT iFunc, INT iOft, UINT16 *pusValue);
LW_API INT              API_PciConfigInDword(INT iBus, INT iSlot, INT iFunc, INT iOft, UINT32 *puiValue);
LW_API INT              API_PciConfigOutByte(INT iBus, INT iSlot, INT iFunc, INT iOft, UINT8 ucValue);
LW_API INT              API_PciConfigOutWord(INT iBus, INT iSlot, INT iFunc, INT iOft, UINT16 usValue);
LW_API INT              API_PciConfigOutDword(INT iBus, INT iSlot, INT iFunc, INT iOft, UINT32  uiValue);
LW_API INT              API_PciConfigModifyByte(INT iBus, INT iSlot, INT iFunc, INT iOft,
                                                UINT8 ucMask, UINT8 ucValue);
LW_API INT              API_PciConfigModifyWord(INT  iBus, INT  iSlot, INT  iFunc, INT  iOft,
                                                UINT16  usMask, UINT16  usValue);
LW_API INT              API_PciConfigModifyDword(INT iBus, INT iSlot, INT iFunc, INT iOft,
                                                 UINT32 uiMask, UINT32 uiValue);

LW_API INT              API_PciFindDev(UINT16  usVendorId, UINT16  usDeviceId, INT  iInstance,
                                       INT *piBus, INT *piSlot, INT *piFunc);
LW_API INT              API_PciFindClass(UINT16  usClassCode, INT  iInstance,
                                         INT *piBus, INT *piSlot, INT *piFunc);

LW_API INT              API_PciDevConfigRead(PCI_DEV_HANDLE hHandle,
                                             UINT uiPos, UINT8 *pucBuf, UINT uiLen);
LW_API INT              API_PciDevConfigWrite(PCI_DEV_HANDLE  hHandle,
                                              UINT uiPos, UINT8 *pucBuf, UINT uiLen);
LW_API INT              API_PciDevConfigReadByte(PCI_DEV_HANDLE hHandle, UINT uiPos, UINT8 *pucValue);
LW_API INT              API_PciDevConfigReadWord(PCI_DEV_HANDLE hHandle, UINT uiPos, UINT16 *pusValue);
LW_API INT              API_PciDevConfigReadDword(PCI_DEV_HANDLE hHandle, UINT uiPos, UINT32 *puiValue);
LW_API INT              API_PciDevConfigWriteByte(PCI_DEV_HANDLE hHandle, UINT uiPos, UINT8 ucValue);
LW_API INT              API_PciDevConfigWriteWord(PCI_DEV_HANDLE hHandle, UINT uiPos, UINT16 usValue);
LW_API INT              API_PciDevConfigWriteDword(PCI_DEV_HANDLE hHandle, UINT uiPos, UINT32 uiValue);

/*********************************************************************************************************
  �豸����
*********************************************************************************************************/

LW_API PCI_RESOURCE_HANDLE  API_PciResourceGet(INT iBus, INT iDevice, INT iFunc, UINT uiType, UINT uiNum);
LW_API PCI_RESOURCE_HANDLE  API_PciDevResourceGet(PCI_DEV_HANDLE hDevHandle, UINT uiType, UINT uiNum);
LW_API PCI_RESOURCE_HANDLE  API_PciDevStdResourceGet(PCI_DEV_HANDLE hDevHandle, UINT uiType, UINT uiNum);
LW_API PCI_RESOURCE_HANDLE  API_PciDevStdResourceFind(PCI_DEV_HANDLE hDevHandle, UINT uiType,
                                                      pci_resource_size_t stStart);

LW_API PVOID                API_PciDevIoRemap(PVOID pvPhysicalAddr, size_t stSize);
LW_API PVOID                API_PciDevIoRemap2(phys_addr_t paPhysicalAddr, size_t stSize);
LW_API PVOID                API_PciDevIoRemapEx(PVOID pvPhysicalAddr, size_t stSize, ULONG ulFlags);
LW_API PVOID                API_PciDevIoRemapEx2(phys_addr_t paPhysicalAddr, size_t stSize, ULONG ulFlags);
LW_API VOID                 API_PciDevIoUnmap(PVOID pvVirtualAddr);

LW_API INT                  API_PciDevMasterEnable(PCI_DEV_HANDLE hDevHandle, BOOL bEnable);

LW_API INT                  API_PciDevInterDisableEx(PCI_DEV_HANDLE   hHandle,
                                                     ULONG            ulVector,
                                                     PINT_SVR_ROUTINE pfuncIsr,
                                                     PVOID            pvArg,
                                                     INT              iMaxServCnt);
LW_API INT                  API_PciDevInterDisable(PCI_DEV_HANDLE   hHandle,
                                                   ULONG            ulVector,
                                                   PINT_SVR_ROUTINE pfuncIsr,
                                                   PVOID            pvArg);
LW_API INT                  API_PciDevInterEnable(PCI_DEV_HANDLE   hHandle,
                                                  ULONG            ulVector,
                                                  PINT_SVR_ROUTINE pfuncIsr,
                                                  PVOID            pvArg);
LW_API INT                  API_PciDevInterDisconnect(PCI_DEV_HANDLE   hHandle,
                                                      ULONG            ulVector,
                                                      PINT_SVR_ROUTINE pfuncIsr,
                                                      PVOID            pvArg);
LW_API INT                  API_PciDevInterConnect(PCI_DEV_HANDLE   hHandle,
                                                   ULONG            ulVector,
                                                   PINT_SVR_ROUTINE pfuncIsr,
                                                   PVOID            pvArg,
                                                   CPCHAR           pcName);
LW_API INT                  API_PciDevInterServiceCnt(PCI_DEV_HANDLE    hHandle,
                                                      ULONG             ulVector,
                                                      INT              *piCnt);

LW_API INT                  API_PciDevIntxEnableSet(PCI_DEV_HANDLE hHandle, INT iEnable);

LW_API INT                  API_PciDevMsiRangeEnable(PCI_DEV_HANDLE hHandle,
                                                     UINT uiVecMin, UINT uiVecMax);
LW_API INT                  API_PciDevMsiVecCountGet(PCI_DEV_HANDLE hHandle, UINT32 *puiVecCount);
LW_API INT                  API_PciDevMsiEnableGet(PCI_DEV_HANDLE hHandle, INT *piEnable);
LW_API INT                  API_PciDevMsiEnableSet(PCI_DEV_HANDLE hHandle, INT iEnable);

LW_API INT                  API_PciDevMsixRangeEnable(PCI_DEV_HANDLE      hHandle,
                                                      PCI_MSI_DESC_HANDLE hMsgHandle,
                                                      UINT                uiVecMin,
                                                      UINT                uiVecMax);
LW_API INT                  API_PciDevMsixVecCountGet(PCI_DEV_HANDLE hHandle, UINT32 *puiVecCount);
LW_API INT                  API_PciDevMsixEnableGet(PCI_DEV_HANDLE hHandle, INT *piEnable);
LW_API INT                  API_PciDevMsixEnableSet(PCI_DEV_HANDLE hHandle, INT iEnable);

LW_API PCI_DEV_HANDLE       API_PciDevParentHandleGet(INT iBus, INT iDevice, INT iFunction);
LW_API PCI_DEV_HANDLE       API_PciDevHandleGet(INT iBus, INT iDevice, INT iFunction);

LW_API INT                  API_PciDevSetupAll(VOID);

/*********************************************************************************************************
  �Զ�����
*********************************************************************************************************/
LW_API INT                  API_PciAutoCtrlRegionSet(PCI_CTRL_HANDLE hCtrl,
                                                     UINT            uiIndex,
                                                     pci_bus_addr_t  addrBusStart,
                                                     pci_addr_t      addrPhyStart,
                                                     pci_size_t      stSize,
                                                     ULONG           ulFlags);

/*********************************************************************************************************
  API Macro
*********************************************************************************************************/

#define pciCtrlReset            API_PciCtrlReset
#define pciCtrlCreate           API_PciCtrlCreate

#define pciConfigInByte         API_PciConfigInByte
#define pciConfigInWord         API_PciConfigInWord
#define pciConfigInDword        API_PciConfigInDword
#define pciConfigOutByte        API_PciConfigOutByte
#define pciConfigOutWord        API_PciConfigOutWord
#define pciConfigOutDword       API_PciConfigOutDword
#define pciConfigModifyByte     API_PciConfigModifyByte
#define pciConfigModifyWord     API_PciConfigModifyWord
#define pciConfigModifyDword    API_PciConfigModifyDword

#define pciFindDev              API_PciFindDev
#define pciFindClass            API_PciFindClass

#define pciDevConfigRead        API_PciDevConfigRead
#define pciDevConfigWrite       API_PciDevConfigWrite
#define pciDevConfigReadByte    API_PciDevConfigReadByte
#define pciDevConfigReadWord    API_PciDevConfigReadWord
#define pciDevConfigReadDword   API_PciDevConfigReadDword
#define pciDevConfigWriteByte   API_PciDevConfigWriteByte
#define pciDevConfigWriteWord   API_PciDevConfigWriteWord
#define pciDevConfigWriteDword  API_PciDevConfigWriteDword

#define pciResourceGet          API_PciResourceGet
#define pciDevResourceGet       API_PciDevResourceGet

#define pciDevIoRemap           API_PciDevIoRemap
#define pciDevIoRemap2          API_PciDevIoRemap2
#define pciDevIoRemapEx         API_PciDevIoRemapEx
#define pciDevIoRemapEx2        API_PciDevIoRemapEx2

#define pciDevMasterEnable      API_PciDevMasterEnable

#define pciDevInterDisableEx    API_PciDevInterDisableEx
#define pciDevInterDisable      API_PciDevInterDisable
#define pciDevInterEnable       API_PciDevInterEnable
#define pciDevInterDisconnect   API_PciDevInterDisconnect
#define pciDevInterConnect      API_PciDevInterConnect
#define pciDevInterServiceCnt   API_PciDevInterServiceCnt

#define pciDevIntxEnableSet     API_PciDevIntxEnableSet

#define pciDevMsiRangeEnable    API_PciDevMsiRangeEnable
#define pciDevMsiVecCountGet    API_PciDevMsiVecCountGet
#define pciDevMsiEnableGet      API_PciDevMsiEnableGet
#define pciDevMsiEnableSet      API_PciDevMsiEnableSet

#define pciDevMsixRangeEnable   API_PciDevMsixRangeEnable
#define pciDevMsixVecCountGet   API_PciDevMsixVecCountGet
#define pciDevMsixEnableGet     API_PciDevMsixEnableGet
#define pciDevMsixEnableSet     API_PciDevMsixEnableSet

#define pciDevParentHandleGet   API_PciDevParentHandleGet
#define pciDevHandleGet         API_PciDevHandleGet

#define pciDevSetupAll          API_PciDevSetupAll
#define pciAutoCtrlRegionSet    API_PciAutoCtrlRegionSet

#endif                                                                  /*  (LW_CFG_DEVICE_EN > 0) &&   */
                                                                        /*  (LW_CFG_PCI_EN > 0)         */
#endif                                                                  /*  __PCIDEV_H                  */
/*********************************************************************************************************
  END
*********************************************************************************************************/
