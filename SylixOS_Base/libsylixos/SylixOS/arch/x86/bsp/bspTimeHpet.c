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
** ��   ��   ��: bspTimeHpet.c
**
** ��   ��   ��: Jiao.JinXing (������)
**
** �ļ���������: 2017 �� 7 �� 12 ��
**
** ��        ��: HPET ��ʱ��������.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "SylixOS.h"
#include "arch/x86/common/x86Idt.h"
#include "arch/x86/acpi/x86AcpiLib.h"
/*********************************************************************************************************
  HPET ��ַ
*********************************************************************************************************/
static addr_t               _G_ulHpetBase;
/*********************************************************************************************************
  ��ȷʱ�任�����
*********************************************************************************************************/
static UINT32               _G_uiFullCnt;
static UINT32               _G_uiComparatorCur;
static UINT64               _G_ui64NSecPerCnt7;                         /*  ��� 7bit ����              */
/*********************************************************************************************************
  HPET �Ĵ�������
*********************************************************************************************************/
#define HPET_BASE                       _G_ulHpetBase

#define HPET_ID_LO                      (HPET_BASE + 0)
#define HPET_ID_HI                      (HPET_ID_LO + 4)

#define HPET_CONFIG_LO                  (HPET_BASE + 0x10)
#define HPET_CONFIG_HI                  (HPET_CONFIG_LO + 4)

#define HPET_STATUS_LO                  (HPET_BASE + 0x20)
#define HPET_STATUS_HI                  (HPET_STATUS_LO + 4)

#define HPET_COUNTER_LO                 (HPET_BASE + 0xf0)
#define HPET_COUNTER_HI                 (HPET_COUNTER_LO + 4)

#define HPET_TIMER_CONFIG_LO(t)         (HPET_BASE + 0x100 + (t) * 0x20)
#define HPET_TIMER_CONFIG_HI(t)         (HPET_TIMER_CONFIG_LO(t) + 4)

#define HPET_TIMER_COMPARATOR_LO(t)     (HPET_BASE + 0x108 + (t) * 0x20)
#define HPET_TIMER_COMPARATOR_HI(t)     (HPET_TIMER_COMPARATOR_LO(t) + 4)

#define HPET_TIMER_FSB_INTR_LO(t)       (HPET_BASE + 0x110 + (t) * 0x20)
#define HPET_TIMER_FSB_INTR_HI(t)       (HPET_TIMER_FSB_INTR_LO(t) + 4)
/*********************************************************************************************************
** ��������: __bspAcpiHpetMap
** ��������: ӳ�� ACPI HPET ��
** ��  ��  : ulAcpiPhyAddr    ACPI HPET �������ַ
**           ulAcpiSize       ACPI HPET ����
** ��  ��  : ACPI HPET �������ַ
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static VOID  *__bspAcpiHpetMap (addr_t  ulAcpiPhyAddr, size_t  ulAcpiSize)
{
    addr_t  ulPhyBase = ROUND_DOWN(ulAcpiPhyAddr, LW_CFG_VMM_PAGE_SIZE);
    addr_t  ulOffset  = ulAcpiPhyAddr - ulPhyBase;
    addr_t  ulVirBase;

    ulAcpiSize += ulOffset;
    ulAcpiSize  = ROUND_UP(ulAcpiSize, LW_CFG_VMM_PAGE_SIZE);

    ulVirBase = (addr_t)API_VmmIoRemapNocache((PVOID)ulPhyBase, ulAcpiSize);
    if (ulVirBase) {
        return  (VOID *)(ulVirBase + ulOffset);
    } else {
        return  (VOID *)(LW_NULL);
    }
}
/*********************************************************************************************************
** ��������: bspHpetTickInit
** ��������: ��ʼ�� tick ʱ��
** ��  ��  : pHpet         ACPI HPET ��
**           pulVector     �ж�������
** ��  ��  : ERROR CODE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
INT  bspHpetTickInit (ACPI_TABLE_HPET  *pAcpiHpetPhy, ULONG  *pulVector)
{
    ACPI_TABLE_HPET  *pAcpiHpet;
    UINT64            ui64FsPerCnt;
    addr_t            ulHpetPhyBase;

    /*
     * ӳ�� ACPI HPET ��
     */
    pAcpiHpet = __bspAcpiHpetMap((addr_t)pAcpiHpetPhy, LW_CFG_VMM_PAGE_SIZE);
    if (!pAcpiHpet) {
        return  (PX_ERROR);
    }

    ulHpetPhyBase = (addr_t)pAcpiHpet->Address.Address;

    /*
     * ӳ�� HPET �Ĵ���
     */
    _G_ulHpetBase = (addr_t)API_VmmIoRemapNocache((PVOID)ulHpetPhyBase,
                                                  LW_CFG_VMM_PAGE_SIZE);

    /*
     * ���ӳ�� ACPI HPET ��
     */
    API_VmmIoUnmap((PVOID)(((addr_t)pAcpiHpet) & LW_CFG_VMM_PAGE_MASK));

    if (!_G_ulHpetBase) {                                               /*  ӳ�� HPET �Ĵ���ʧ��        */
        return  (PX_ERROR);
    }

    /*
     * ���� HPET ��һ��֧�� 64 λģʽ(�� QEMU), ���Թ̶�ʹ�� 32 λģʽ
     */
    write32(0, HPET_CONFIG_LO);                                         /*  ֹͣ��ʱ��                  */

    ui64FsPerCnt = read32(HPET_ID_HI);

    _G_uiFullCnt       = (1000000000000000ULL / ui64FsPerCnt) / LW_TICK_HZ;
    _G_ui64NSecPerCnt7 = (ui64FsPerCnt << 7) / 1000000;

    write32(0, HPET_COUNTER_LO);                                        /*  ��λ������                  */
    write32(0, HPET_COUNTER_HI);

    write32((1 << 8) |                                                  /*  32 λģʽ                   */
            (1 << 6) |                                                  /*  ����ֵ                      */
            (1 << 3) |                                                  /*  Preiodic ģʽ               */
            (1 << 2) |                                                  /*  �ж�ʹ��                    */
            (0 << 1),                                                   /*  ���ش���ģʽ                */
            HPET_TIMER_CONFIG_LO(0));                                   /*  ���� timer0                 */
    write32(0, HPET_TIMER_CONFIG_HI(0));

    write32(_G_uiFullCnt, HPET_TIMER_COMPARATOR_LO(0));                 /*  ���� timer0 comparator      */
    write32(0,            HPET_TIMER_COMPARATOR_HI(0));

    write32((1 << 0) |                                                  /*  ��ʼ����                    */
            (1 << 1),                                                   /*  legacy replacement mode     */
            HPET_CONFIG_LO);                                            /*  ������ʱ��                  */

    if (bspIntModeGet() == X86_INT_MODE_SYMMETRIC_IO) {                 /*  IOAPIC mapping              */
        *pulVector = LW_IRQ_2;                                          /*  IRQ2                        */

    } else {                                                            /*  PIC mapping                 */
        *pulVector = X86_IRQ_TIMER;                                     /*  IRQ0                        */
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: bspHpetTickHook
** ��������: ÿ������ϵͳʱ�ӽ��ģ�ϵͳ�������������
** ��  ��  : i64Tick      ϵͳ��ǰʱ��
** ��  ��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
VOID  bspHpetTickHook (INT64  i64Tick)
{
    (VOID)i64Tick;

    _G_uiComparatorCur = read32(HPET_TIMER_COMPARATOR_LO(0)) - _G_uiFullCnt;
}
/*********************************************************************************************************
** ��������: bspHpetTickHighResolution
** ��������: ���������һ�� tick ����ǰ�ľ�ȷʱ��.
** �䡡��  : ptv       ��Ҫ������ʱ��
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
VOID  bspHpetTickHighResolution (struct timespec  *ptv)
{
    UINT32  uiCntDiff = read32(HPET_COUNTER_LO) - _G_uiComparatorCur;

    ptv->tv_nsec += (_G_ui64NSecPerCnt7 * uiCntDiff) >> 7;
    if (ptv->tv_nsec >= 1000000000) {
        ptv->tv_nsec -= 1000000000;
        ptv->tv_sec++;
    }
}
/*********************************************************************************************************
  END
*********************************************************************************************************/
