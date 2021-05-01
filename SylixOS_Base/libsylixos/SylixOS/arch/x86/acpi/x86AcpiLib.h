/*********************************************************************************************************
**
**                                    �й������Դ��֯
**
**                                   Ƕ��ʽʵʱ����ϵͳ
**
**                                       SylixOS(TM)
**
**                               Copyright  All Rights Reserved
**
**--------------�ļ���Ϣ--------------------------------------------------------------------------------
**
** ��   ��   ��: x86AcpiLib.h
**
** ��   ��   ��: Jiao.JinXing (������)
**
** �ļ���������: 2017 �� 04 �� 14 ��
**
** ��        ��: x86 ��ϵ���� ACPI ��ͷ�ļ�.
*********************************************************************************************************/

#ifndef __X86_ACPILIB_H
#define __X86_ACPILIB_H

#include "acpi.h"
#include "actypes.h"
#include "accommon.h"
#include "acevents.h"
#include "acresrc.h"

/*********************************************************************************************************
  ����
*********************************************************************************************************/
#define ACPI_DEBUG_ENABLED          0
#undef  ACPI_VERBOSE
/*********************************************************************************************************
  �궨��
*********************************************************************************************************/
#if ACPI_DEBUG_ENABLED > 0
#define __ACPI_DEBUG_LOG(...)       AcpiOsPrintf(__VA_ARGS__)
#else
#define __ACPI_DEBUG_LOG(...)
#endif

#define __ACPI_ERROR_LOG(...)       AcpiOsPrintf(__VA_ARGS__)

#ifdef ACPI_VERBOSE
#define ACPI_VERBOSE_PRINT(...)     AcpiOsPrintf(__VA_ARGS__)
#else
#define ACPI_VERBOSE_PRINT(...)
#endif

#define ACPI_NAME_COMPARE(a,b)      (lib_strncmp((CHAR *)a, (CHAR *)b, ACPI_NAME_SIZE) == 0)
/*********************************************************************************************************
  �������Ͷ���
*********************************************************************************************************/
typedef struct {
    UINT8       IRQ_ucSourceIrq;                            /*  Interrupt source (IRQ)                  */
    UINT32      IRQ_uiGlobalIrq;                            /*  Global system interrupt                 */
    UINT16      IRQ_usIntFlags;
    BOOL        IRQ_bPrepend;                               /*  LW_TRUE if prepend to list              */
} X86_IRQ_OVERRIDE;                                         /*  X86_IRQ_OVERRIDE                        */
/*********************************************************************************************************
  ȫ�ֱ�������
*********************************************************************************************************/
extern ULONG                _G_ulAcpiMcfgBaseAddress;
extern BOOL                 _G_bAcpiEarlyAccess;
extern BOOL                 _G_bAcpiPciConfigAccess;

extern CHAR                *_G_pcAcpiRsdpPtr;
extern ACPI_TABLE_HPET     *_G_pAcpiHpet;
extern ACPI_TABLE_MADT     *_G_pAcpiMadt;
extern ACPI_TABLE_FACS     *_G_pAcpiFacs;
extern ACPI_TABLE_RSDT     *_G_pAcpiRsdt;
extern ACPI_TABLE_XSDT     *_G_pAcpiXsdt;
extern ACPI_TABLE_FADT     *_G_pAcpiFadt;

extern PCHAR                _G_pcAcpiOsHeapPtr;
extern PCHAR                _G_pcAcpiOsHeapBase;
extern PCHAR                _G_pcAcpiOsHeapTop;

extern BOOL                 _G_bAcpiPcAtCompatible;
extern UINT8                _G_ucAcpiIsaIntNr;
extern UINT8                _G_ucAcpiIsaApic;
extern UINT8               *_G_pucAcpiGlobalIrqBaseTable;
extern X86_IRQ_OVERRIDE  *(*_G_pfuncAcpiIrqOverride)(INT  iIndex);
extern PVOID              (*_G_pfuncAcpiIrqAppend)(INT iIndex);
/*********************************************************************************************************
  ��������
*********************************************************************************************************/
UINT8               acpiChecksum(const UINT8  *pucBuffer, UINT32  uiLength);
INT                 acpiTableValidate(UINT8  *pucBuffer, UINT32  uiLength, const CHAR  *pcSignature);
ACPI_TABLE_RSDP    *acpiFindRsdp(VOID);
INT                 acpiIsAmlTable(const ACPI_TABLE_HEADER   *pTable);
INT                 acpiIsFacsTable(const ACPI_TABLE_HEADER  *pTable);
INT                 acpiIsRsdpTable(const ACPI_TABLE_RSDP    *pTable);
ACPI_SIZE           acpiGetTableSize(ACPI_PHYSICAL_ADDRESS  where);
INT                 acpiTableInit(VOID);

ACPI_STATUS         acpiLibSetModel(UINT32  uiModel);
INT                 acpiLibInit(BOOL  bEarlyInit);
INT                 acpiLibInstallHandlers(VOID);
VOID                acpiLibDisable(VOID);
ULONG               acpiGetMcfg(VOID);

VOID                acpiConfigInit(PCHAR  pcHeapBase, size_t  stHeapSize);
ACPI_STATUS         acpiLibGetBusNo(ACPI_HANDLE  hObject, UINT32  *puiBus);
VOID                acpiLibGetIsaIrqResources(ACPI_RESOURCE  *pResourceList, BOOL  bFillinTable);
INT                 acpiLibDevScan(BOOL  bEarlyInit, BOOL  bPciAccess, CHAR  *pcBuf, UINT32  uiMpApicBufSize);
VOID                acpiMpTableShow(VOID);

VOID                AcpiOsInfoShow(VOID);
INT                 AcpiShowFacs(const ACPI_TABLE_HEADER  *pAcpiHeader);

#endif                                                                  /*  __X86_ACPILIB_H             */
/*********************************************************************************************************
  END
*********************************************************************************************************/
