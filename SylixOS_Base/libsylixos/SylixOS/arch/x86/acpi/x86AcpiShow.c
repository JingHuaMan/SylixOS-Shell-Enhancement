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
** ��   ��   ��: x86AcpiShow.c
**
** ��   ��   ��: Jiao.JinXing (������)
**
** �ļ���������: 2017 �� 04 �� 17 ��
**
** ��        ��: x86 ��ϵ���� ACPI ��Ϣ��ʾ.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "SylixOS.h"
#include "x86AcpiLib.h"
#include "arch/x86/mpconfig/x86MpApic.h"
/*********************************************************************************************************
  ȫ�ֱ�������
*********************************************************************************************************/
ACPI_MODULE_NAME("acpi_show")
/*********************************************************************************************************
** ��������: __acpiPrintableString
** ��������: �������ַ�����Ϊ�ɴ�ӡ���ַ���
** �䡡��  : pcOutString       ����ַ���
**           pcInString        �����ַ���
**           iLength           ����
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
#ifdef ACPI_VERBOSE

static VOID  __acpiPrintableString (CHAR  *pcOutString, const CHAR  *pcInString, INT  iLength)
{
    INT  i;

    for (i = 0; i < (iLength - 1); i++) {
        if (lib_isalnum(pcInString[i])) {
            pcOutString[i] = pcInString[i];
        } else {
            pcOutString[i] = ' ';
        }
    }
    pcOutString[i] = '\0';
}
/*********************************************************************************************************
** ��������: acpiShowFacs
** ��������: ��ʾ FACS
** �䡡��  : pAcpiHeader       ACPI ��ͷ
** �䡡��  : FACS ����
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
INT  AcpiShowFacs (const ACPI_TABLE_HEADER  *pAcpiHeader)
{
    ACPI_TABLE_FACS  *pFacs = (ACPI_TABLE_FACS *)pAcpiHeader;
    CHAR              acString[10];

    if (LW_SYS_STATUS_IS_RUNNING()) {
        /*
         * FACS is a special case, doesn't have an ACPI header
         */
        __acpiPrintableString(acString, pFacs->Signature, 5);

        ACPI_VERBOSE_PRINT("\n\n  ACPI message (%p) %4s\n",      pFacs, acString);
        ACPI_VERBOSE_PRINT("    Length        %d\n",             pFacs->Length);
        ACPI_VERBOSE_PRINT("    HardwareSignature     0x%x\n",   pFacs->HardwareSignature);
        ACPI_VERBOSE_PRINT("    FirmwareWakingVec     0x%x\n",   pFacs->FirmwareWakingVector);
        ACPI_VERBOSE_PRINT("    GlobalLock    0x%x\n",           pFacs->GlobalLock);
        ACPI_VERBOSE_PRINT("    Flags         0x%x\n",           pFacs->Flags);
        ACPI_VERBOSE_PRINT("    XFirmwareWakingVect   0x%llx\n", pFacs->XFirmwareWakingVector);
        ACPI_VERBOSE_PRINT("    Version       0x%x\n",           pFacs->Version);
    }

    return  (pFacs->Length);
}

#endif                                                                  /*  ACPI_VERBOSE                */
/*********************************************************************************************************
  END
*********************************************************************************************************/
