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
** ��   ��   ��: x86AcpiLib.c
**
** ��   ��   ��: Jiao.JinXing (������)
**
** �ļ���������: 2017 �� 04 �� 14 ��
**
** ��        ��: x86 ��ϵ���� ACPI ��Դ�ļ�.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "SylixOS.h"
#include "x86AcpiLib.h"
#include "arch/x86/mpconfig/x86MpApic.h"
/*********************************************************************************************************
  ȫ�ֱ�������
*********************************************************************************************************/
ACPI_MODULE_NAME("acpi_lib")

static BOOL     _G_bAcpiInited = LW_FALSE;

ULONG           _G_ulAcpiMcfgBaseAddress = 0;
BOOL            _G_bAcpiPciConfigAccess  = LW_TRUE;
BOOL            _G_bAcpiEarlyAccess      = LW_TRUE;
/*********************************************************************************************************
** ��������: acpiLibSetModel
** ��������: ���� ACPI �ж�ģ��
** �䡡��  : uiModel           �ж�ģ��
** �䡡��  : ACPI ״̬
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
ACPI_STATUS  acpiLibSetModel (UINT32  uiModel)
{
    ACPI_OBJECT       arg;
    ACPI_OBJECT_LIST  args;

    arg.Type          = ACPI_TYPE_INTEGER;
    arg.Integer.Value = uiModel;
    args.Count        = 1;
    args.Pointer      = &arg;

    return  (AcpiEvaluateObject(ACPI_ROOT_OBJECT, "_PIC", &args, LW_NULL));
}
/*********************************************************************************************************
** ��������: __acpiDummyExceptionHandler
** ��������: �ٵ� ACPI �쳣�������
** �䡡��  : NONE
** �䡡��  : AE_OK
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static ACPI_STATUS  __acpiDummyExceptionHandler (ACPI_STATUS  AmlStatus,
                                                 ACPI_NAME    Name,
                                                 UINT16       usOpcode,
                                                 UINT32       uiAmlOffset,
                                                 VOID        *pvContext)
{
    return  (AE_OK);
}
/*********************************************************************************************************
** ��������: __acpiNotifyHandler
** ��������: ACPI ֪ͨ�������
** �䡡��  : hDevice       �豸���
**           uiValue       ֵ
**           pvContext     ������
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static VOID  __acpiNotifyHandler (ACPI_HANDLE  hDevice, UINT32  uiValue, VOID  *pvContext)
{
    ACPI_INFO((AE_INFO, "Received a notify %X", uiValue));
}
/*********************************************************************************************************
** ��������: __acpiShutdownHandler
** ��������: ACPI �ػ�����
** �䡡��  : NONE
** �䡡��  : AE_OK
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static UINT32  __acpiShutdownHandler (VOID  *pvContext)
{
    ACPI_EVENT_STATUS     eStatus;
    ACPI_STATUS __unused  Status;

    /*
     * Get Event Data
     */
    Status = AcpiGetEventStatus(ACPI_EVENT_POWER_BUTTON, &eStatus);

    if (eStatus & ACPI_EVENT_FLAG_ENABLED) {
        AcpiClearEvent(ACPI_EVENT_POWER_BUTTON);
    }

    API_KernelReboot(LW_REBOOT_SHUTDOWN);

    return  (AE_OK);
}
/*********************************************************************************************************
** ��������: __acpiSleepHandler
** ��������: ACPI ˯�ߴ���
** �䡡��  : NONE
** �䡡��  : AE_OK
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static UINT32  __acpiSleepHandler (VOID  *pvContext)
{
    return  (AE_OK);
}
/*********************************************************************************************************
** ��������: __acpiEventHandler
** ��������: ACPI �¼�����
** �䡡��  : NONE
** �䡡��  : AE_OK
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static VOID  __acpiEventHandler (UINT32       uiEventType,
                                 ACPI_HANDLE  hDevice,
                                 UINT32       uiEventNumber,
                                 VOID        *pvContext)
{
}
/*********************************************************************************************************
** ��������: acpiLibInstallHandlers
** ��������: ��װ ACPI �������
** �䡡��  : NONE
** �䡡��  : ERROR CODE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
INT  acpiLibInstallHandlers (VOID)
{
    ACPI_STATUS  status;

    status = AcpiInstallNotifyHandler(ACPI_ROOT_OBJECT, ACPI_SYSTEM_NOTIFY,
                                      __acpiNotifyHandler, LW_NULL);
    if (ACPI_FAILURE(status)) {
        ACPI_EXCEPTION((AE_INFO, status, "While installing Notify handler"));
        return  (PX_ERROR);
    }

    status = AcpiInstallGlobalEventHandler(__acpiEventHandler, LW_NULL);
    if (ACPI_FAILURE(status)) {
        ACPI_EXCEPTION((AE_INFO, status, "While installing Global Event handler"));
        return  (PX_ERROR);
    }

    /*
     * ����鷵��ֵ, ����� IBM-PC ���ݻ���û�����߰�ť
     */
    AcpiInstallFixedEventHandler(ACPI_EVENT_POWER_BUTTON, __acpiShutdownHandler, LW_NULL);
    AcpiInstallFixedEventHandler(ACPI_EVENT_SLEEP_BUTTON, __acpiSleepHandler,    LW_NULL);

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: acpiLibInit
** ��������: ACPI ���ʼ��
** �䡡��  : bEarlyInit        �Ƿ����ڳ�ʼ��
** �䡡��  : ERROR CODE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
INT  acpiLibInit (BOOL  bEarlyInit)
{
    ACPI_STATUS         status;
    ACPI_SYSTEM_INFO    systemInfoBuffer;
    ACPI_BUFFER         acpiBuffer;
    UINT32              uiLocalAcpiDbgLevel;
    UINT32              uiMode = 0;

    _G_bAcpiEarlyAccess = bEarlyInit;

    __ACPI_DEBUG_LOG("\n**** acpiLibInit ****\n");

    if (!ACPI_DEBUG_ENABLED) {
        uiLocalAcpiDbgLevel = AcpiDbgLevel;
        AcpiDbgLevel = 0;                                               /*  No debug text output        */
    }

    /*
     * Init PCIE ECAM base addess
     */
    _G_ulAcpiMcfgBaseAddress = acpiGetMcfg();

    /*
     * Init ACPI and start debugger thread
     */

    /*
     * Init the ACPI system information
     */
    __ACPI_DEBUG_LOG("\n  AcpiInitializeSubsystem \n");
    status = AcpiInitializeSubsystem();                                 /*  ACPI CAPR 3.2.1.1 (266)     */
    if (ACPI_FAILURE(status)) {
        __ACPI_DEBUG_LOG("\n---> Could not initialize subsystem, %s <---\n",
                         AcpiFormatException(status));
        return  (status);
    }

    if (!ACPI_DEBUG_ENABLED) {
        AcpiInstallExceptionHandler(__acpiDummyExceptionHandler);       /*  Report no errors            */
    }

    __ACPI_DEBUG_LOG("\n  AcpiInitializeTables\n");
    status = AcpiInitializeTables(LW_NULL, 24, 1);                      /*  Not there                   */
    if (ACPI_FAILURE(status)) {
        __ACPI_DEBUG_LOG("\n---> Could not initialize ACPI tables, %s <---\n",
                         AcpiFormatException(status));
        return  (status);
    }

    __ACPI_DEBUG_LOG("\n  AcpiLoadTables\n");                           /*  ACPI CAPR 3.2.2.2           */
    status = AcpiLoadTables();                                          /*  372                         */
    if (ACPI_FAILURE(status)) {
        __ACPI_DEBUG_LOG("\n---> Could not load ACPI tables, %s <---\n",
                         AcpiFormatException(status));
        return  (status);
    }

    __ACPI_DEBUG_LOG("\n  AcpiEvInstallRegionHandlers\n");
    status = AcpiEvInstallRegionHandlers();
    if (ACPI_FAILURE(status)) {
        __ACPI_DEBUG_LOG("\n---> Unable to install the ACPI region handler, %s <---\n",
                         AcpiFormatException(status));
        goto    __error_handle;
    }

    /*
     * Complete namespace initialization by initializing device
     * objects and executing AML code for Regions, buffers, etc.
     * ACPI CAPR 3.2.2.3
     */
    __ACPI_DEBUG_LOG("\n  AcpiEnableSubsystem\n");                      /*  ACPI CAPR 3.2.4.1, 389      */
    status = AcpiEnableSubsystem(ACPI_NO_ACPI_ENABLE);
    if (ACPI_FAILURE(status)) {
        __ACPI_DEBUG_LOG("\n---> Unable to start the ACPI Interpreter, %s <---\n",
                         AcpiFormatException(status));
        goto    __error_handle;
    }

    __ACPI_DEBUG_LOG("\n  AcpiInitializeObjects\n");
    status = AcpiInitializeObjects(ACPI_NO_DEVICE_INIT);                /*  376                         */
    if (ACPI_FAILURE(status)) {
        __ACPI_DEBUG_LOG("\n---> Unable to initialize ACPI objects, %s <---\n",
                         AcpiFormatException(status));
        goto    __error_handle;
    }

    __ACPI_DEBUG_LOG("\n  Interpreter enabled\n");

    /*
     * TODO: ���⻯ GUEST ϵͳ����Ҫ���� acpiLibSetModel
     */
    status = acpiLibSetModel(1);
    if (ACPI_FAILURE(status)) {
        __ACPI_DEBUG_LOG("\n---> Unable to set ACPI Model: %s <---\n",
                         AcpiFormatException(status));
    }

    acpiBuffer.Length  = sizeof(ACPI_SYSTEM_INFO);
    acpiBuffer.Pointer = &systemInfoBuffer;

    status = AcpiGetSystemInfo(&acpiBuffer);
    if (ACPI_FAILURE(status)) {
        __ACPI_DEBUG_LOG("\n---> Unable to get ACPI system information, %s <---\n",
                         AcpiFormatException(status));
        goto    __error_handle;
    }

    __ACPI_DEBUG_LOG("\n  ACPI system info:\n");
    __ACPI_DEBUG_LOG("\n    AcpiCaVersion - 0x%x\n", systemInfoBuffer.AcpiCaVersion);
    __ACPI_DEBUG_LOG("    Flags - 0x%x\n",           systemInfoBuffer.Flags);
    __ACPI_DEBUG_LOG("    TimerResolution - 0x%x\n", systemInfoBuffer.TimerResolution);
    __ACPI_DEBUG_LOG("    DebugLevel - 0x%x\n",      systemInfoBuffer.DebugLevel);
    __ACPI_DEBUG_LOG("    DebugLayer - 0x%x\n",      systemInfoBuffer.DebugLayer);

    /*
     * Full ACPI subsystem init is needed by SylixOS for power management.
     */
    if (ACPI_DEBUG_ENABLED) {
        uiMode = AcpiHwGetMode();
        if (uiMode == ACPI_SYS_MODE_ACPI) {
            __ACPI_DEBUG_LOG("\n  Current operating state of system : ACPI_SYS_MODE_ACPI %d \n",
                             uiMode);
        } else {
            if (uiMode == ACPI_SYS_MODE_LEGACY) {
                __ACPI_DEBUG_LOG("\n  Current operating state of system : ACPI_SYS_MODE_LEGACY %d \n",
                                 uiMode);
            } else {
                __ACPI_DEBUG_LOG("\n  Current operating state of system : ACPI_SYS_MODE_UNKNOWN %d \n",
                                 uiMode);
            }
        }
    }

    if (!_G_bAcpiEarlyAccess) {
        __ACPI_DEBUG_LOG("\n  AcpiEnable\n");
        status = AcpiEnable();                                          /*  ACPI CAPR 6.3.1             */
        if (ACPI_FAILURE(status)) {
            __ACPI_DEBUG_LOG("\n---> Unable to enable ACPI system %s <--\n",
                             AcpiFormatException(status));
        }
    }

    AcpiOsInfoShow();                                                   /*  ��ӡ����ϵͳ��ص���Ϣ      */

    __ACPI_DEBUG_LOG("\n**** acpiLibInit: completed ERROR_NONE ****\n");

    if (!ACPI_DEBUG_ENABLED) {
        AcpiDbgLevel = uiLocalAcpiDbgLevel;
    }

    return  (ERROR_NONE);

__error_handle:
    AcpiOsTerminate();
    __ACPI_DEBUG_LOG("\n---> acpiLibInit FAILED <---\n");
    return  (PX_ERROR);
}
/*********************************************************************************************************
** ��������: acpiLibDisable
** ��������: ACPI ��ر�
** �䡡��  : NONE
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
VOID  acpiLibDisable (VOID)
{
    __ACPI_DEBUG_LOG("\n**** acpiLibDisable: AcpiTerminate ****\n");
    AcpiTerminate();
}
/*********************************************************************************************************
** ��������: acpiGetMcfg
** ��������: �� ACPI ���� MCFG ��ַ
** �䡡��  : NONE
** �䡡��  : MCFG ��ַ�� 0
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
ULONG  acpiGetMcfg (VOID)
{
    ACPI_TABLE_RSDP   *pRsdp;
    ACPI_TABLE_HEADER *pHeader = LW_NULL;
    ACPI_TABLE_RSDT   *pRsdt;
    ACPI_TABLE_XSDT   *pXsdt;
    INT                iTableEntriesNr, i;

    pRsdp = acpiFindRsdp();
    if (pRsdp == LW_NULL) {
        return  (0);
    }

    /*
     * There are two possible table pointers
     */

    /*
     * If we have a 32 bit long address table
     */
    if (_G_pAcpiRsdt != LW_NULL) {
        pRsdt = AcpiOsMapMemory((ACPI_PHYSICAL_ADDRESS)_G_pAcpiRsdt, sizeof(ACPI_TABLE_RSDT));
        if (pRsdt == LW_NULL) {
            return  (0);
        }

        /*
         * Compute number of tables
         */
        iTableEntriesNr = (INT)((pRsdt->Header.Length - sizeof(ACPI_TABLE_HEADER)) / sizeof(UINT32));

        /*
         * We need to verify the length
         */
        if (iTableEntriesNr >= 0) {
            for (i = 0; i < iTableEntriesNr; i++) {
                pHeader = AcpiOsMapMemory((ACPI_PHYSICAL_ADDRESS)pRsdt->TableOffsetEntry[i],
                                          sizeof(ACPI_TABLE_HEADER));
                if (pHeader == LW_NULL) {
                    continue;
                }

                if (ACPI_NAME_COMPARE(pHeader->Signature, "MCFG")) {
                    AcpiOsUnmapMemory(pRsdt, sizeof(ACPI_TABLE_RSDT));
                    return  ((ULONG)pHeader);
                }
            }

            /*
             * Unmap header for both 32-bit and 64-bit if not found
             */
            AcpiOsUnmapMemory(pHeader, sizeof(ACPI_TABLE_HEADER));
        }
        AcpiOsUnmapMemory(pRsdt, sizeof(ACPI_TABLE_RSDT));
    }

    /*
     * If we have a 64 bit long address table
     */
    if (_G_pAcpiXsdt != LW_NULL) {
        pXsdt = AcpiOsMapMemory((ACPI_PHYSICAL_ADDRESS)_G_pAcpiXsdt, sizeof(ACPI_TABLE_XSDT));
        if (pXsdt == LW_NULL) {
            return  (0);
        }

        /*
         * Compute number of tables
         */
        iTableEntriesNr = (INT)((pXsdt->Header.Length - sizeof(ACPI_TABLE_HEADER)) / sizeof(UINT64));
        if (iTableEntriesNr >= 0) {
            for (i = 0; i < iTableEntriesNr; i++) {
                pHeader = AcpiOsMapMemory((ACPI_PHYSICAL_ADDRESS)pXsdt->TableOffsetEntry[i],
                                          sizeof(ACPI_TABLE_HEADER));
                if (pHeader == LW_NULL) {
                    continue;
                }

                if (ACPI_NAME_COMPARE(pHeader->Signature, "MCFG")) {
                    AcpiOsUnmapMemory(pXsdt, sizeof(ACPI_TABLE_XSDT));
                    return  ((ULONG)pHeader);
                }
            }
            AcpiOsUnmapMemory(pHeader, sizeof(ACPI_TABLE_HEADER));
        }
        AcpiOsUnmapMemory(pXsdt, sizeof(ACPI_TABLE_XSDT));
    }

    return  (0);
}
/*********************************************************************************************************
** ��������: x86AcpiAvailable
** ��������: �ж� ACPI �Ƿ����
** �䡡��  : NONE
** �䡡��  : LW_TRUE OR LW_FALSE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
BOOL  x86AcpiAvailable (VOID)
{
    return  (_G_bAcpiInited);
}
/*********************************************************************************************************
** ��������: x86AcpiInit
** ��������: ACPI �����ʼ��
** �䡡��  : NONE
** �䡡��  : ERROR CODE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
INT  x86AcpiInit (VOID)
{
    INT  iError;

    /*
     * ���� ACPI �ı�ӳ�����ǵ��ڴ�
     */
    iError = acpiTableInit();
    if (iError != ERROR_NONE) {
        return  (PX_ERROR);
    }

    /*
     * ���ڳ�ʼ�� ACPI ��
     */
    iError = acpiLibInit(LW_FALSE);
    if (iError != ERROR_NONE) {
        return  (PX_ERROR);
    }

    /*
     * ��װ ACPI �������
     */
    iError = acpiLibInstallHandlers();
    if (iError != ERROR_NONE) {
        return  (PX_ERROR);
    }

    /*
     * ��Ϊ�����Ѿ�ɨ��� ACPI �豸, �������������Ϣ(���ж�·�ɱ����������)�� ����䵽 MPAPIC_DATA
     * �������ﲻ����Ҫ���û����� acpiLibDevScan ����
     */

    _G_bAcpiInited = LW_TRUE;

    return  (ERROR_NONE);
}
/*********************************************************************************************************
  END
*********************************************************************************************************/
