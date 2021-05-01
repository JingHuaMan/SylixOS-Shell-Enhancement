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
** ��   ��   ��: x86IoApic.c
**
** ��   ��   ��: Jiao.JinXing (������)
**
** �ļ���������: 2016 �� 07 �� 29 ��
**
** ��        ��: x86 ��ϵ���� IOAPIC ���Դ�ļ�.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "SylixOS.h"
#include "arch/x86/mpconfig/x86MpApic.h"
/*********************************************************************************************************
  The I/O APIC manages hardware interrupts for an SMP system.

  http://www.intel.com/design/chipsets/datashts/29056601.pdf
*********************************************************************************************************/
/*********************************************************************************************************
  �궨��
*********************************************************************************************************/
/*********************************************************************************************************
  IO APIC direct register offset
*********************************************************************************************************/
#define IOAPIC_IND              0x00                                /*  Index Register                  */
#define IOAPIC_DATA             0x10                                /*  IO window (data)                */
#define IOAPIC_IRQPA            0x20                                /*  IRQ Pin Assertion Register      */
#define IOAPIC_EOI              0x40                                /*  EOI Register                    */
/*********************************************************************************************************
  IO APIC indirect register offset
*********************************************************************************************************/
#define IOAPIC_ID               0x00                                /*  IOAPIC ID                       */
#define IOAPIC_VERS             0x01                                /*  IOAPIC Version                  */
#define IOAPIC_ARB              0x02                                /*  IOAPIC Arbitration ID           */
#define IOAPIC_BOOT             0x03                                /*  IOAPIC Boot Configuration       */
#define IOAPIC_REDTBL           0x10                                /*  Redirection Table (24 * 64bit)  */
/*********************************************************************************************************
  ID register bits
*********************************************************************************************************/
#define IOAPIC_ID_SHIFT         24
/*********************************************************************************************************
  Version register bits
*********************************************************************************************************/
#define IOAPIC_MRE_MASK         0x00ff0000                          /*  Max Red. entry mask             */
#define IOAPIC_MRE_SHIFT        16                                  /*  Max Red. entry shift            */
#define IOAPIC_PRQ              0x00008000                          /*  This has IRQ reg                */
#define IOAPIC_VERSION_MASK     0x000000ff                          /*  Version number                  */
/*********************************************************************************************************
  Interrupt delivery type
*********************************************************************************************************/
#define IOAPIC_DT_APIC          0x0                                 /*  APIC serial bus                 */
#define IOAPIC_DT_FS            0x1                                 /*  Front side bus message          */
/*********************************************************************************************************
  ���Ͷ���
*********************************************************************************************************/
typedef struct {
    addr_t          IOAPIC_ulBase;                                  /*  IOAPIC ����ַ                   */
    UINT            IOAPIC_uiIdteBase;                              /*  IDTE �����ſ�ʼ                 */
    UINT8           IOAPIC_ucId;                                    /*  IOAPIC ID                       */
    UINT32          IOAPIC_uiVersion;                               /*  Version �Ĵ���ֵ                */
} X86_IOAPIC_INTR, *PX86_IOAPIC_INTR;                               /*  IOAPIC �жϿ�����               */
/*********************************************************************************************************
  ȫ�ֱ�������
*********************************************************************************************************/
static LW_SPINLOCK_CA_DEFINE_CACHE_ALIGN(_G_slcaX86IoApic);         /*  ���ʼĴ�����������              */
static X86_IOAPIC_INTR  _G_x86IoApicIntrs[IOAPIC_MAX_NR];           /*  IOAPIC �жϿ�����               */
static UINT             _G_uiX86IoApicNr           = 1;             /*  IOAPIC �жϿ�������Ŀ           */
static UINT             _G_uiX86IoApicRedEntriesNr = 24;            /*  Redirection ��Ŀ��Ŀ            */
/*********************************************************************************************************
** ��������: __x86IoApicRegGet
** ��������: IOAPIC ���Ĵ���
** �䡡��  : pIoApicIntr       IOAPIC �жϿ�����
**           ucRegIndex        �Ĵ�������
** �䡡��  : ֵ
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static UINT32  __x86IoApicRegGet (PX86_IOAPIC_INTR  pIoApicIntr, UINT8  ucRegIndex)
{
    UINT32  uiValue;
    INTREG  iregInterLevel;

    LW_SPIN_LOCK_QUICK(&_G_slcaX86IoApic.SLCA_sl, &iregInterLevel);

    write32(ucRegIndex, pIoApicIntr->IOAPIC_ulBase + IOAPIC_IND);
    uiValue = read32(pIoApicIntr->IOAPIC_ulBase    + IOAPIC_DATA);

    LW_SPIN_UNLOCK_QUICK(&_G_slcaX86IoApic.SLCA_sl, iregInterLevel);

    return  (uiValue);
}
/*********************************************************************************************************
** ��������: __x86IoApicRegSet
** ��������: IOAPIC д�Ĵ���
** �䡡��  : pIoApicIntr       IOAPIC �жϿ�����
**           ucRegIndex        �Ĵ�������
**           uiValue           ֵ
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static VOID  __x86IoApicRegSet (PX86_IOAPIC_INTR  pIoApicIntr, UINT8  ucRegIndex, UINT32  uiValue)
{
    INTREG  iregInterLevel;

    LW_SPIN_LOCK_QUICK(&_G_slcaX86IoApic.SLCA_sl, &iregInterLevel);

    write32(ucRegIndex, pIoApicIntr->IOAPIC_ulBase + IOAPIC_IND);
    write32(uiValue,    pIoApicIntr->IOAPIC_ulBase + IOAPIC_DATA);

    LW_SPIN_UNLOCK_QUICK(&_G_slcaX86IoApic.SLCA_sl, iregInterLevel);
}
/*********************************************************************************************************
** ��������: __x86IoApicRedGetLo
** ��������: ��� Red ����Ŀ�ĵ� 32 λ
** �䡡��  : pIoApicIntr       IOAPIC �жϿ�����
**           ucRedNum          Redirection ��Ŀ��
** �䡡��  : ֵ
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static UINT32  __x86IoApicRedGetLo (PX86_IOAPIC_INTR  pIoApicIntr, UINT8  ucRedNum)
{
    UINT8  ucRegIndex = IOAPIC_REDTBL + (ucRedNum << 1);

    return  (__x86IoApicRegGet(pIoApicIntr, ucRegIndex));
}
/*********************************************************************************************************
** ��������: __x86IoApicRedGetHi
** ��������: ��� Red ����Ŀ�ĸ� 32 λ
** �䡡��  : pIoApicIntr       IOAPIC �жϿ�����
**           ucRedNum          Redirection ��Ŀ��
** �䡡��  : ֵ
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static UINT32  __x86IoApicRedGetHi (PX86_IOAPIC_INTR  pIoApicIntr, UINT8  ucRedNum)
{
    UINT8  ucRegIndex = IOAPIC_REDTBL + (ucRedNum << 1) + 1;

    return  (__x86IoApicRegGet(pIoApicIntr, ucRegIndex));
}
/*********************************************************************************************************
** ��������: __x86IoApicRedSetLo
** ��������: ���� Red ����Ŀ�ĵ� 32 λ
** �䡡��  : pIoApicIntr       IOAPIC �жϿ�����
**           ucRedNum          Redirection ��Ŀ��
**           uiValue           ֵ
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static VOID  __x86IoApicRedSetLo (PX86_IOAPIC_INTR  pIoApicIntr, UINT8  ucRedNum, UINT32  uiValue)
{
    UINT8  ucRegIndex = IOAPIC_REDTBL + (ucRedNum << 1);

    __x86IoApicRegSet(pIoApicIntr, ucRegIndex, uiValue);
}
/*********************************************************************************************************
** ��������: __x86IoApicRedSetHi
** ��������: ���� Red ����Ŀ�ĸ� 32 λ
** �䡡��  : pIoApicIntr       IOAPIC �жϿ�����
**           ucRedNum          Redirection ��Ŀ��
**           uiValue           ֵ
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static VOID  __x86IoApicRedSetHi (PX86_IOAPIC_INTR  pIoApicIntr, UINT8  ucRedNum, UINT32  uiValue)
{
    UINT8  ucRegIndex = IOAPIC_REDTBL + (ucRedNum << 1) + 1;

    __x86IoApicRegSet(pIoApicIntr, ucRegIndex, uiValue);
}
/*********************************************************************************************************
** ��������: __x86IoApicRedUpdateLo
** ��������: ���� Red ����Ŀ�ĵ� 32 λ
** �䡡��  : pIoApicIntr       IOAPIC �жϿ�����
**           ucRedNum          Redirection ��Ŀ��
**           uiValue           ֵ
**           uiMask            ����
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static VOID  __x86IoApicRedUpdateLo (PX86_IOAPIC_INTR  pIoApicIntr,
                                     UINT8             ucRedNum,
                                     UINT32            uiValue,
                                     UINT32            uiMask)
{
    __x86IoApicRedSetLo(pIoApicIntr, ucRedNum,
            (__x86IoApicRedGetLo(pIoApicIntr, ucRedNum) & ~uiMask) | (uiValue & uiMask));
}
/*********************************************************************************************************
** ��������: __x86IoApicIntrLookup
** ��������: ͨ�� IRQ �Ų��� IOAPIC �жϿ������� Redirection ��Ŀ��
** �䡡��  : ucIrq             IRQ ��
**           pucRedNum         Redirection ��Ŀ��
** �䡡��  : IOAPIC �жϿ����� �� LW_NULL
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static PX86_IOAPIC_INTR  __x86IoApicIntrLookup (UINT8  ucIrq, UINT8  *pucRedNum)
{
    PX86_IOAPIC_INTR  pIoApicIntr;
    UINT8             uiIoApicIdOffset;
    INT               i;

    for (i = 0; i < _G_uiX86IoApicNr; i++) {
        pIoApicIntr      = &_G_x86IoApicIntrs[i];
        uiIoApicIdOffset = pIoApicIntr->IOAPIC_ucId - _G_ucX86MpApicIoBaseId;

        if ((ucIrq >= (uiIoApicIdOffset * _G_uiX86IoApicRedEntriesNr)) &&
            (ucIrq < ((uiIoApicIdOffset + 1) * _G_uiX86IoApicRedEntriesNr))) {
            *pucRedNum = ucIrq - (uiIoApicIdOffset * _G_uiX86IoApicRedEntriesNr);
            return  (pIoApicIntr);
        }
    }

    return  (LW_NULL);
}
/*********************************************************************************************************
** ��������: x86IoApicRedGetLo
** ��������: ��� Red ����Ŀ�ĵ� 32 λ
** �䡡��  : ucIrq             IRQ ��
** �䡡��  : ֵ OR ~0
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
UINT32  x86IoApicRedGetLo (UINT8  ucIrq)
{
    PX86_IOAPIC_INTR  pIoApicIntr;
    UINT8             ucRedNum;

    pIoApicIntr = __x86IoApicIntrLookup(ucIrq, &ucRedNum);
    if (pIoApicIntr) {
        return  (__x86IoApicRedGetLo(pIoApicIntr, ucRedNum));
    } else {
        return  ((UINT32)~0);
    }
}
/*********************************************************************************************************
** ��������: x86IoApicRedGetHi
** ��������: ��� Red ����Ŀ�ĸ� 32 λ
** �䡡��  : ucIrq             IRQ ��
** �䡡��  : ֵ OR ~0
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
UINT32  x86IoApicRedGetHi (UINT8  ucIrq)
{
    PX86_IOAPIC_INTR  pIoApicIntr;
    UINT8             ucRedNum;

    pIoApicIntr = __x86IoApicIntrLookup(ucIrq, &ucRedNum);
    if (pIoApicIntr) {
        return  (__x86IoApicRedGetHi(pIoApicIntr, ucRedNum));
    } else {
        return  ((UINT32)~0);
    }
}
/*********************************************************************************************************
** ��������: x86IoApicRedSetLo
** ��������: ���� Red ����Ŀ�ĵ� 32 λ
** �䡡��  : ucIrq             IRQ ��
**           uiValue           ֵ
** �䡡��  : ERROR CODE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
INT  x86IoApicRedSetLo (UINT8  ucIrq, UINT32  uiValue)
{
    PX86_IOAPIC_INTR  pIoApicIntr;
    UINT8             ucRedNum;

    pIoApicIntr = __x86IoApicIntrLookup(ucIrq, &ucRedNum);
    if (pIoApicIntr) {
        __x86IoApicRedSetLo(pIoApicIntr, ucRedNum, uiValue);
        return  (ERROR_NONE);
    } else {
        return  (PX_ERROR);
    }
}
/*********************************************************************************************************
** ��������: x86IoApicRedSetLo
** ��������: ���� Red ����Ŀ�ĸ� 32 λ
** �䡡��  : ucIrq             IRQ ��
**           uiValue           ֵ
** �䡡��  : ERROR CODE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
INT  x86IoApicRedSetHi (UINT8  ucIrq, UINT32  uiValue)
{
    PX86_IOAPIC_INTR  pIoApicIntr;
    UINT8             ucRedNum;

    pIoApicIntr = __x86IoApicIntrLookup(ucIrq, &ucRedNum);
    if (pIoApicIntr) {
        __x86IoApicRedSetHi(pIoApicIntr, ucRedNum, uiValue);
        return  (ERROR_NONE);
    } else {
        return  (PX_ERROR);
    }
}
/*********************************************************************************************************
** ��������: x86IoApicRedUpdateLo
** ��������: ���� Red ����Ŀ�ĵ� 32 λ
** �䡡��  : ucIrq             IRQ ��
**           uiValue           ֵ
**           uiMask            ����
** �䡡��  : ERROR CODE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
INT  x86IoApicRedUpdateLo (UINT8  ucIrq, UINT32  uiValue, UINT32  uiMask)
{
    PX86_IOAPIC_INTR  pIoApicIntr;
    UINT8             ucRedNum;

    pIoApicIntr = __x86IoApicIntrLookup(ucIrq, &ucRedNum);
    if (pIoApicIntr) {
        __x86IoApicRedUpdateLo(pIoApicIntr, ucRedNum, uiValue, uiMask);
        return  (ERROR_NONE);
    } else {
        return  (PX_ERROR);
    }
}
/*********************************************************************************************************
** ��������: x86IoApicIrqEnable
** ��������: IOAPIC ʹ�� IRQ
** �䡡��  : ucIrq             IRQ ��
** �䡡��  : ERROR CODE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
INT  x86IoApicIrqEnable (UINT8  ucIrq)
{
    PX86_IOAPIC_INTR  pIoApicIntr;
    UINT8             ucRedNum;

    pIoApicIntr = __x86IoApicIntrLookup(ucIrq, &ucRedNum);
    if (pIoApicIntr) {
        __x86IoApicRedUpdateLo(pIoApicIntr, ucRedNum, 0, IOAPIC_INT_MASK);
        return  (ERROR_NONE);
    } else {
        return  (PX_ERROR);
    }
}
/*********************************************************************************************************
** ��������: x86IoApicIrqDisable
** ��������: IOAPIC ���� IRQ
** �䡡��  : ucIrq             IRQ ��
** �䡡��  : ERROR CODE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
INT  x86IoApicIrqDisable (UINT8  ucIrq)
{
    PX86_IOAPIC_INTR  pIoApicIntr;
    UINT8             ucRedNum;

    pIoApicIntr = __x86IoApicIntrLookup(ucIrq, &ucRedNum);
    if (pIoApicIntr) {
        UINT32  uiLo = __x86IoApicRedGetLo(pIoApicIntr, ucRedNum);
        UINT32  uiIsLevel;

        /*
         * We need to set the trigger mode to edge first, to workaround following
         * situation: when the mask bit is set after the interrupt message has
         * been accepted by a local APIC unit but before the interrupt is
         * dispensed to the processor. In that case, the I/O APIC will always
         * waiting for EOI even re-enable the interrupt(Remote IRR is always set).
         * We switch to edge first, and then the Remote IRR will be cleared, and
         * then we switch back to level if original mode is level.
         */
        uiIsLevel = uiLo & IOAPIC_LEVEL;
        uiLo     &= ~IOAPIC_LEVEL;                                      /*  Set to edge first           */

        __x86IoApicRedSetLo(pIoApicIntr, ucRedNum, uiLo | IOAPIC_INT_MASK);

        if (uiIsLevel) {
            uiLo |= uiIsLevel;
            __x86IoApicRedSetLo(pIoApicIntr, ucRedNum, uiLo | IOAPIC_INT_MASK);
        }

        return  (ERROR_NONE);
    } else {
        return  (PX_ERROR);
    }
}
/*********************************************************************************************************
** ��������: x86IoApicIrqIsEnable
** ��������: IOAPIC �Ƿ�ʹ��ָ���� IRQ
** �䡡��  : ucIrq             IRQ ��
** �䡡��  : LW_TRUE OR LW_FALSE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
BOOL  x86IoApicIrqIsEnable (UINT8  ucIrq)
{
    PX86_IOAPIC_INTR  pIoApicIntr;
    UINT8             ucRedNum;

    pIoApicIntr = __x86IoApicIntrLookup(ucIrq, &ucRedNum);
    if (pIoApicIntr) {
        return  ((__x86IoApicRedGetLo(pIoApicIntr, ucRedNum) & IOAPIC_INT_MASK) ? (LW_FALSE) : (LW_TRUE));

    } else {
        return  (LW_FALSE);
    }
}
/*********************************************************************************************************
** ��������: x86IoApicIrqSetTarget
** ��������: IOAPIC ���� IRQ �ж�Ŀ�� CPU
** �䡡��  : ucIrq                 IRQ ��
**           ucTargetLocalApicId   Ŀ�� CPU Local APIC ID
** �䡡��  : ERROR CODE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
INT  x86IoApicIrqSetTarget (UINT8  ucIrq, UINT8  ucTargetLocalApicId)
{
    PX86_IOAPIC_INTR  pIoApicIntr;
    UINT8             ucRedNum;

    pIoApicIntr = __x86IoApicIntrLookup(ucIrq, &ucRedNum);
    if (pIoApicIntr) {
        __x86IoApicRedSetHi(pIoApicIntr, ucRedNum,                      /*  �ַ�����ָ���� CPU          */
                            ucTargetLocalApicId << IOAPIC_DESTINATION_SHIFT);
        return  (ERROR_NONE);
    } else {
        return  (PX_ERROR);
    }
}
/*********************************************************************************************************
** ��������: x86IoApicIrqGetTarget
** ��������: IOAPIC ��� IRQ �ж�Ŀ�� CPU
** �䡡��  : ucIrq                 IRQ ��
**           pucTargetLocalApicId  Ŀ�� CPU Local APIC ID
** �䡡��  : ERROR CODE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
INT  x86IoApicIrqGetTarget (UINT8  ucIrq, UINT8  *pucTargetLocalApicId)
{
    PX86_IOAPIC_INTR  pIoApicIntr;
    UINT8             ucRedNum;

    pIoApicIntr = __x86IoApicIntrLookup(ucIrq, &ucRedNum);
    if (pIoApicIntr) {
        *pucTargetLocalApicId = __x86IoApicRedGetHi(pIoApicIntr, ucRedNum) >> IOAPIC_DESTINATION_SHIFT;
        return  (ERROR_NONE);
    } else {
        return  (PX_ERROR);
    }
}
/*********************************************************************************************************
** ��������: __x86IoApicInit
** ��������: ��ʼ�� IOAPIC
** �䡡��  : pIoApicIntr       IOAPIC �жϿ�����
** �䡡��  : ERROR CODE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  __x86IoApicInit (PX86_IOAPIC_INTR  pIoApicIntr)
{
    UINT32  uiRteValue;
    UINT8   ucRedNum;

    /*
     * Set the IOAPIC bus arbitration ID
     */
    __x86IoApicRegSet(pIoApicIntr, IOAPIC_ID, (UINT32)pIoApicIntr->IOAPIC_ucId << IOAPIC_ID_SHIFT);

    /*
     * Boot Config register does not exist in I/O APIC (version 0x1X).
     * It may or may not exist in I/O xAPIC (version 0x2X). Some Intel
     * chipsets with I/O xAPIC don't have Boot Config reg.
     *
     * Attempt to set Interrupt Delivery Mechanism to Front Side Bus
     * (ie. MSI capable), or APIC Serial Bus. Some I/O xAPIC allow this
     * bit to take effect. Some I/O xAPIC allow the bit to be written
     * (for software compat reason), but it has no effect as the mode is
     * hardwired to FSB delivery.
     */
    if (((pIoApicIntr->IOAPIC_uiVersion & IOAPIC_VERSION_MASK) >= 0x20)) {
#if LW_CFG_CPU_X86_APIC_BUS_INT > 0
        /*
         * Pentium up to and including P6 use APIC bus
         */
        __x86IoApicRegSet(pIoApicIntr, IOAPIC_BOOT, IOAPIC_DT_APIC);
#else
        /*
         * Pentium4 and later use FSB for interrupt delivery
         */
        __x86IoApicRegSet(pIoApicIntr, IOAPIC_BOOT, IOAPIC_DT_FS);
#endif                                                                  /*  X86_APIC_BUS_INT > 0        */
    }

    uiRteValue = IOAPIC_EDGE     |                                      /*  �����źŴ���                */
                 IOAPIC_HIGH     |                                      /*  �ߵ�ƽ��Ч(PCIΪ�͵�ƽ)     */
                 IOAPIC_FIXED    |                                      /*  �̶��ַ��� DEST ������� CPU*/
                 IOAPIC_INT_MASK |                                      /*  �����ж�                    */
                 IOAPIC_PHYSICAL;                                       /*  ����ģʽ, DEST = APIC ID    */

    for (ucRedNum = 0; ucRedNum < _G_uiX86IoApicRedEntriesNr; ucRedNum++) {
        __x86IoApicRedSetLo(pIoApicIntr, ucRedNum,
                            uiRteValue |
                            (pIoApicIntr->IOAPIC_uiIdteBase + ucRedNum));   /*  �� 8 λΪ x86 IDTE ��   */

        __x86IoApicRedSetHi(pIoApicIntr,
                            ucRedNum, 0 << IOAPIC_DESTINATION_SHIFT);   /*  DEST ��Ϊ 0(Ĭ�Ϸַ��� BSP) */
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: x86IoApicInit
** ��������: ��ʼ�� IOAPIC
** �䡡��  : puiIoIntNr        IO �ж���Ŀ
** �䡡��  : ERROR CODE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
INT  x86IoApicInitAll (UINT  *puiIoIntNr)
{
    PX86_IOAPIC_INTR  pIoApicIntr;
    UINT8            *pucLogicalTable;
    UINT32           *puiAddrTable;
    UINT32            uiIoApicNr;
    INT               i;

    LW_SPIN_INIT(&_G_slcaX86IoApic.SLCA_sl);
    
    if (x86MpApicIoApicNrGet(&uiIoApicNr) != ERROR_NONE) {
        return  (PX_ERROR);
    }
    _G_uiX86IoApicNr = uiIoApicNr;

    if (x86MpApicAddrTableGet(&puiAddrTable) != ERROR_NONE) {
        return  (PX_ERROR);
    }

    if (x86MpApicLogicalTableGet(&pucLogicalTable) != ERROR_NONE) {
        return  (PX_ERROR);
    }

    for (i = 0; i < uiIoApicNr; i++) {
        pIoApicIntr = &_G_x86IoApicIntrs[i];

        /*
         * Typically assume that the IOAPIC_ID register has been
         * set by the BIOS, however in some cases this isn't what
         * actually happens. If the register has not been initialized,
         * then we must do it ourselves based on system configuration
         * data, either from MPS or ACPI.
         */
        pIoApicIntr->IOAPIC_ucId            = pucLogicalTable[i];
        pIoApicIntr->IOAPIC_ulBase          = puiAddrTable[i];

        pIoApicIntr->IOAPIC_uiVersion       = __x86IoApicRegGet(pIoApicIntr, IOAPIC_VERS);
        _G_uiX86IoApicRedEntriesNr          = ((pIoApicIntr->IOAPIC_uiVersion &
                                                IOAPIC_MRE_MASK) >> IOAPIC_MRE_SHIFT) + 1;

        pIoApicIntr->IOAPIC_uiIdteBase      = X86_IRQ_BASE + \
                 ((pIoApicIntr->IOAPIC_ucId - _G_ucX86MpApicIoBaseId) * _G_uiX86IoApicRedEntriesNr);

        __x86IoApicInit(pIoApicIntr);
    }

    if (puiIoIntNr) {
        *puiIoIntNr = _G_uiX86IoApicNr * _G_uiX86IoApicRedEntriesNr;
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: x86IoApicIrqEoi
** ��������: IOAPIC �����ж�
** �䡡��  : ucIrq             IRQ ��
** �䡡��  : ERROR CODE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
INT  x86IoApicIrqEoi (UINT8  ucIrq)
{
    PX86_IOAPIC_INTR  pIoApicIntr;
    UINT8             ucRedNum;

    pIoApicIntr = __x86IoApicIntrLookup(ucIrq, &ucRedNum);
    if (pIoApicIntr) {
        if (((pIoApicIntr->IOAPIC_uiVersion & IOAPIC_VERSION_MASK) >= 0x20)) {
            UINT8  ucX86Vector = __x86IoApicRedGetLo(pIoApicIntr, ucIrq) & IOAPIC_VEC_MASK;
            write32(ucX86Vector, pIoApicIntr->IOAPIC_ulBase + IOAPIC_EOI);
        }
        return  (ERROR_NONE);
    } else {
        return  (PX_ERROR);
    }
}
/*********************************************************************************************************
** ��������: x86IoApicPciIrqGet
** ��������: ͨ�� Dest APID ID �� IRQ �Ż�������� IRQ ��
** �䡡��  : ucApicId          IOAPIC ID
**           ucIrq             IRQ ��
** �䡡��  : ������ IRQ ��
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
UINT8  x86IoApicPciIrqGet (UINT8  ucDestIoApicId, UINT8  ucIrq)
{
    if ((ucDestIoApicId > _G_ucX86MpApicIoBaseId) &&
        (ucDestIoApicId < (_G_ucX86MpApicIoBaseId + _G_uiX86IoApicNr))) {
        ucIrq += ((ucDestIoApicId - _G_ucX86MpApicIoBaseId) * _G_uiX86IoApicRedEntriesNr);
    }

    return  (ucIrq);
}
/*********************************************************************************************************
  END
*********************************************************************************************************/
