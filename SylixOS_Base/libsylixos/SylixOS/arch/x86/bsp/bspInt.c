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
** ��   ��   ��: bspInt.c
**
** ��   ��   ��: Jiao.JinXing (������)
**
** �ļ���������: 2016 �� 7 �� 31 ��
**
** ��        ��: ��������ҪΪ SylixOS �ṩ�Ĺ���֧��.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "SylixOS.h"
#include "arch/x86/asm/hwcap.h"
#include "arch/x86/common/x86Topology.h"
#include "driver/int/i8259a.h"
/*********************************************************************************************************
  �ж����

  x86 Int Vector:
  +-----------------+
  |  0              |
  |                 |
  |  x86 exception  |
  |                 |
  |  31             |           SylixOS Int Vector:
  +-----------------+ --------> +-----------------+
  |  32             |           |  0              |
  |                 |           |                 |
  |   x86 IO IRQ    |  INT MAP  |     IO IRQ      | ------> 16
  |                 |           |                 | PCI x 8 Intx
  |                 |           |                 | ------> 23
  +-----------------+           +-----------------+
  |                 |           |                 |
  |  Local APIC IRQ |  INT MAP  |  Local APIC IRQ |
  |                 |           |                 |
  +-----------------+           +-----------------+
  |                 |           |                 |
  |   x86 SMP IPI   |  INT MAP  |     SMP IPI     |
  |                 |           |                 |
  +-----------------+           +-----------------+
  |                 |           |                 |
  | x86 MSI & other |  INT MAP  |   MSI & other   |
  |                 |           |                 |
  +-----------------+ --------> +-----------------+

*********************************************************************************************************/
/*********************************************************************************************************
  �궨��
*********************************************************************************************************/
/*********************************************************************************************************
  ����ʹ���������
*********************************************************************************************************/
#if LW_CFG_SMP_EN > 0
#define VECTOR_OP_LOCK()            LW_SPIN_LOCK_IGNIRQ(&_K_slcaVectorTable.SLCA_sl)
#define VECTOR_OP_UNLOCK()          LW_SPIN_UNLOCK_IGNIRQ(&_K_slcaVectorTable.SLCA_sl)
#else
#define VECTOR_OP_LOCK()
#define VECTOR_OP_UNLOCK()
#endif                                                                  /*  LW_CFG_SMP_EN > 0           */
/*********************************************************************************************************
  ȫ�ֱ�������
*********************************************************************************************************/
static I8259A_CTL   _G_i8259aData = {                                   /*  i8259A ƽ̨����             */
    .iobase_master  = PIC1_BASE_ADR,
    .iobase_slave   = PIC2_BASE_ADR,
    .trigger        = 0,
    .manual_eoi     = 0,
    .vector_base    = X86_IRQ_BASE,
};

UINT                _G_uiX86IntMode = X86_INT_MODE_PIC;                 /*  �ж�ģʽ                    */

UINT                _G_uiX86IoVectorNr;                                 /*  IO �ж���Ŀ                 */

UINT                _G_uiX86LocalApicVectorBase;                        /*  Local APIC �жϿ�ʼ         */
UINT                _G_uiX86LocalApicVectorNr;                          /*  Local APIC �ж���Ŀ         */

UINT                _G_uiX86IpiVectorBase;                              /*  IPI �жϿ�ʼ                */
UINT                _G_uiX86IpiVectorNr;                                /*  IPI �ж���Ŀ                */

UINT                _G_uiX86MsiVectorBase;                              /*  MSI �жϿ�ʼ                */
UINT                _G_uiX86MsiVectorNr;                                /*  MSI �ж���Ŀ                */
/*********************************************************************************************************
** ��������: __x86Is8259Vector
** ��������: �ж�һ���ж��Ƿ�Ϊ 8259 �ж�
** ��  ��  : ulVector     �ж�����
** �䡡��  : LW_TRUE OR LW_FALSE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static BOOL  __x86Is8259Vector (ULONG  ulVector)
{
    if (_G_uiX86IntMode != X86_INT_MODE_SYMMETRIC_IO) {
        if (ulVector < N_PIC_IRQS) {
            return  (LW_TRUE);
        }
    }

    return  (LW_FALSE);
}
/*********************************************************************************************************
** ��������: __x86IsIoApicVector
** ��������: �ж�һ���ж��Ƿ�Ϊ IOAPIC �ж�
** ��  ��  : ulVector     �ж�����
** �䡡��  : LW_TRUE OR LW_FALSE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static  BOOL  __x86IsIoApicVector (ULONG  ulVector)
{
    if (_G_uiX86IntMode == X86_INT_MODE_SYMMETRIC_IO) {
        if (ulVector < _G_uiX86IoVectorNr) {
            return  (LW_TRUE);
        }
    }

    return  (LW_FALSE);
}
/*********************************************************************************************************
** ��������: __x86IsIpiVector
** ��������: �ж�һ���ж��Ƿ�Ϊ Local APIC �ж�
** ��  ��  : ulVector     �ж�����
** �䡡��  : LW_TRUE OR LW_FALSE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static BOOL  __x86IsLocalApicVector (ULONG  ulVector)
{
    if (_G_uiX86IntMode != X86_INT_MODE_PIC) {
        if ((ulVector >= _G_uiX86LocalApicVectorBase) &&
            (ulVector < (_G_uiX86LocalApicVectorBase + _G_uiX86LocalApicVectorNr))) {
            return  (LW_TRUE);
        }
    }

    return  (LW_FALSE);
}
/*********************************************************************************************************
** ��������: __x86IsIpiVector
** ��������: �ж�һ���ж��Ƿ�Ϊ IPI �ж�
** ��  ��  : ulVector     �ж�����
** �䡡��  : LW_TRUE OR LW_FALSE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static BOOL  __x86IsIpiVector (ULONG  ulVector)
{
    if (_G_uiX86IntMode == X86_INT_MODE_SYMMETRIC_IO) {
        if ((ulVector >= _G_uiX86IpiVectorBase) &&
            (ulVector < (_G_uiX86IpiVectorBase + _G_uiX86IpiVectorNr))) {
            return  (LW_TRUE);
        }
    }

    return  (LW_FALSE);
}
/*********************************************************************************************************
** ��������: __x86IsMsiVector
** ��������: �ж�һ���ж��Ƿ�Ϊ MSI �ж�
** ��  ��  : ulVector     �ж�����
** �䡡��  : LW_TRUE OR LW_FALSE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static BOOL  __x86IsMsiVector (ULONG  ulVector)
{
    if (_G_uiX86IntMode != X86_INT_MODE_PIC) {
        if ((ulVector >= _G_uiX86MsiVectorBase) &&
            (ulVector < (_G_uiX86MsiVectorBase + _G_uiX86MsiVectorNr))) {
            return  (LW_TRUE);
        }
    }

    return  (LW_FALSE);
}
/*********************************************************************************************************
** ��������: bspIntInit
** ��������: �ж�ϵͳ��ʼ��
** ��  ��  : NONE
** ��  ��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
LW_WEAK VOID  bspIntInit (VOID)
{
    /*
     * ���ĳ�ж�Ϊ��ʽ�жϣ����������:
     * API_InterVectorSetFlag(LW_IRQ_4, LW_IRQ_FLAG_QUEUE);
     * �Ĵ���.
     *
     * ���ĳ�жϿ�������ʼ����������ӣ����������:
     * API_InterVectorSetFlag(LW_IRQ_0, LW_IRQ_FLAG_SAMPLE_RAND);
     * �Ĵ���.
     */

    _G_uiX86IntMode = X86_INT_MODE_PIC;                                 /*  ��λ��Ĭ�� PIC ģʽ         */

    i8259aInit(&_G_i8259aData);                                         /*  ��ʼ�� 8259A                */
    i8259aIrqEnable(&_G_i8259aData, LW_IRQ_2);                          /*  ʹ�� IRQ2(Slave PIC)        */

    if (LW_NCPUS > 1) {                                                 /*  ���һ������ APIC           */
        _G_uiX86IntMode = X86_INT_MODE_SYMMETRIC_IO;                    /*  Ϊ��֧�ֶ��                */
                                                                        /*  �� SYMMETRIC_IO ģʽ        */
    } else if (X86_FEATURE_HAS_APIC) {                                  /*  ���˵�Ҳ�� APIC             */
        /*
         * MP ���ñ������ PCI �豸�жϿ����� 16 ~ 23, ������ʹ�� IOAPIC,
         * ��ʱ������ SYMMETRIC_IO ģʽ, ������������ģʽ
         */
        _G_uiX86IntMode = X86_INT_MODE_SYMMETRIC_IO;                    /*  �� SYMMETRIC_IO ģʽ        */
    }

    if (_G_uiX86IntMode != X86_INT_MODE_PIC) {                          /*  �� PIC ģʽ                 */

        x86LocalApicInit(&_G_uiX86LocalApicVectorNr);                   /*  ��ʼ�� Local APIC           */

        if (_G_uiX86IntMode == X86_INT_MODE_SYMMETRIC_IO) {             /*  SYMMETRIC_IO ģʽ           */
            x86IoApicInitAll(&_G_uiX86IoVectorNr);                      /*  ��ʼ�����е� IOAPIC         */

        } else {                                                        /*  ������ģʽ                  */
            /*
             * ע��: ������ģʽֻ֧ Over PIC, ��֧�� Over IOAPIC
             */
            _G_uiX86IoVectorNr = N_PIC_IRQS;
        }

        _G_uiX86LocalApicVectorBase = _G_uiX86IoVectorNr;               /*  Local APIC �ж�������ʼ     */

        _G_uiX86IpiVectorBase = _G_uiX86IoVectorNr + \
                                _G_uiX86LocalApicVectorNr;              /*  IPI �ж�������ʼ            */
        if (_G_uiX86IntMode == X86_INT_MODE_SYMMETRIC_IO) {             /*  SYMMETRIC_IO ģʽ           */
            _G_uiX86IpiVectorNr = LW_NCPUS;                             /*  IPI �ж���Ŀ���� LW_NCPUS   */

        } else {                                                        /*  ������ģʽ                  */
            _G_uiX86IpiVectorNr = 0;                                    /*  ���ˣ��� IPI                */
        }

        _G_uiX86MsiVectorBase = _G_uiX86IpiVectorBase + \
                                _G_uiX86IpiVectorNr;                    /*  MSI �ж�������ʼ            */
        _G_uiX86MsiVectorNr   = min(X86_IRQ_NUM - _G_uiX86IoVectorNr - \
                                    _G_uiX86LocalApicVectorNr - _G_uiX86IpiVectorNr,
                                    16);                                /*  MSI �ж���Ŀ                */

        if (_G_uiX86IntMode == X86_INT_MODE_SYMMETRIC_IO) {             /*  SYMMETRIC_IO ģʽ           */
            /*
             * 8 �� PCI �ж�����Ϊ��ʽ�ж�
             */
            API_InterVectorSetFlag(LW_IRQ_16, LW_IRQ_FLAG_QUEUE);
            API_InterVectorSetFlag(LW_IRQ_17, LW_IRQ_FLAG_QUEUE);
            API_InterVectorSetFlag(LW_IRQ_18, LW_IRQ_FLAG_QUEUE);
            API_InterVectorSetFlag(LW_IRQ_19, LW_IRQ_FLAG_QUEUE);
            API_InterVectorSetFlag(LW_IRQ_20, LW_IRQ_FLAG_QUEUE);
            API_InterVectorSetFlag(LW_IRQ_21, LW_IRQ_FLAG_QUEUE);
            API_InterVectorSetFlag(LW_IRQ_22, LW_IRQ_FLAG_QUEUE);
            API_InterVectorSetFlag(LW_IRQ_23, LW_IRQ_FLAG_QUEUE);
        }
    }
}
/*********************************************************************************************************
** ��������: bspIntModeGet
** ��������: ����ж�ģʽ
** ��  ��  : NONE
** ��  ��  : �ж�ģʽ
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
LW_WEAK UINT  bspIntModeGet (VOID)
{
    return  (_G_uiX86IntMode);
}
/*********************************************************************************************************
** ��������: bspIntHandle
** ��������: �жϴ�����
** ��  ��  : ulVector     �ж�����
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
LW_WEAK VOID  bspIntHandle (ULONG  ulVector)
{
    archIntHandle(ulVector, LW_FALSE);                                  /*  �������ж�Ƕ��(MSI �ж��޷� */
                                                                        /*  ����)                       */
    VECTOR_OP_LOCK();
    if (__x86Is8259Vector(ulVector)) {
        if (_G_i8259aData.manual_eoi) {                                 /*  ��Ҫ�ֶ����ж�              */
            i8259aIrqEoi(&_G_i8259aData, (UINT)ulVector);               /*  �� 8259 �ж�                */
        }

    } else if (__x86IsIoApicVector(ulVector)) {
        x86IoApicIrqEoi(ulVector);                                      /*  �� IOAPIC �ж�              */
    }

    if (X86_FEATURE_HAS_APIC) {                                         /*  �� APIC                     */
        x86LocalApicEoi();                                              /*  �� Local APIC �ж�          */
    }
    VECTOR_OP_UNLOCK();
}
/*********************************************************************************************************
** ��������: bspIntVectorEnable
** ��������: ʹ��ָ�����ж�����
** ��  ��  : ulVector     �ж�����
** ��  ��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
LW_WEAK VOID  bspIntVectorEnable (ULONG  ulVector)
{
    if (__x86Is8259Vector(ulVector)) {
        i8259aIrqEnable(&_G_i8259aData, ulVector);

    } else if (__x86IsIoApicVector(ulVector)) {
        x86IoApicIrqEnable(ulVector);

    } else if (__x86IsLocalApicVector(ulVector)) {
        x86LocalApicIrqEnable(ulVector - _G_uiX86LocalApicVectorBase);
    }

    /*
     * IPI MSI ���ܿ���
     */
}
/*********************************************************************************************************
** ��������: bspIntVectorDisable
** ��������: ����ָ�����ж�����
** ��  ��  : ulVector     �ж�����
** ��  ��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
LW_WEAK VOID  bspIntVectorDisable (ULONG  ulVector)
{
    if (__x86Is8259Vector(ulVector)) {
        i8259aIrqDisable(&_G_i8259aData, ulVector);

    } else if (__x86IsIoApicVector(ulVector)) {
        x86IoApicIrqDisable(ulVector);

    } else if (__x86IsLocalApicVector(ulVector)) {
        x86LocalApicIrqDisable(ulVector - _G_uiX86LocalApicVectorBase);
    }

    /*
     * IPI MSI ���ܿ���
     */
}
/*********************************************************************************************************
** ��������: bspIntVectorIsEnable
** ��������: ���ָ�����ж������Ƿ�ʹ��
** ��  ��  : ulVector     �ж�����
** ��  ��  : LW_FALSE �� LW_TRUE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
LW_WEAK BOOL  bspIntVectorIsEnable (ULONG  ulVector)
{
    if (__x86Is8259Vector(ulVector)) {
        return  (i8259aIrqIsEnable(&_G_i8259aData, ulVector));

    } else if (__x86IsIoApicVector(ulVector)) {
        return  (x86IoApicIrqIsEnable(ulVector));

    } else if (__x86IsLocalApicVector(ulVector)) {
        return  (x86LocalApicIrqIsEnable(ulVector - _G_uiX86LocalApicVectorBase));

    } else if (__x86IsIpiVector(ulVector)) {
        /*
         * IPI MSI ���ܿ���
         */
        return  (LW_TRUE);

    } else if (__x86IsMsiVector(ulVector)) {
        return  (LW_TRUE);
    }

    return  (LW_FALSE);
}
/*********************************************************************************************************
** ��������: bspIntVectorSetPriority
** ��������: ����ָ�����ж����������ȼ�
** ��  ��  : ulVector     �ж�����
**           uiPrio       ���ȼ�
** �䡡��  : ERROR CODE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
#if LW_CFG_INTER_PRIO > 0

LW_WEAK ULONG   bspIntVectorSetPriority (ULONG  ulVector, UINT  uiPrio)
{
    /*
     * �����������ȼ�
     */
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: bspIntVectorGetPriority
** ��������: ��ȡָ�����ж����������ȼ�
** ��  ��  : ulVector     �ж�����
**           puiPrio      ���ȼ�
** �䡡��  : ERROR CODE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
LW_WEAK ULONG   bspIntVectorGetPriority (ULONG  ulVector, UINT  *puiPrio)
{
    *puiPrio = ulVector;                                                /*  Vector Խ�����ȼ�Խ��     */
    return  (ERROR_NONE);
}

#endif                                                                  /*  LW_CFG_INTER_PRIO > 0       */
/*********************************************************************************************************
** ��������: bspIntVectorSetTarget
** ��������: ����ָ�����ж�������Ŀ�� CPU
** �䡡��  : ulVector      �ж�����
**           stSize        CPU ���뼯�ڴ��С
**           pcpuset       CPU ����
** �䡡��  : ERROR CODE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
#if LW_CFG_INTER_TARGET > 0

LW_WEAK ULONG   bspIntVectorSetTarget (ULONG  ulVector, size_t  stSize, const PLW_CLASS_CPUSET  pcpuset)
{
    if (__x86IsIoApicVector(ulVector)) {
        ULONG   i;
        ULONG   ulNumChk;

        ulNumChk = ((ULONG)stSize << 3);
        ulNumChk = (ulNumChk > LW_NCPUS) ? LW_NCPUS : ulNumChk;

        for (i = 0; i < ulNumChk; i++) {
            if (LW_CPU_ISSET(i, pcpuset)) {
                x86IoApicIrqSetTarget(ulVector, X86_LOGICID_TO_APICID(i));
                break;                                                  /*  ֻ������һ�� Target CPU     */
            }
        }
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: bspIntVectorGetTarget
** ��������: ��ȡָ�����ж�������Ŀ�� CPU
** �䡡��  : ulVector      �ж�����
**           stSize        CPU ���뼯�ڴ��С
**           pcpuset       CPU ����
** �䡡��  : ERROR CODE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
LW_WEAK ULONG   bspIntVectorGetTarget (ULONG  ulVector, size_t  stSize, PLW_CLASS_CPUSET  pcpuset)
{
    LW_CPU_ZERO(pcpuset);

    if (__x86IsIoApicVector(ulVector)) {
        UINT8  ucTargetLocalApicId;

        x86IoApicIrqGetTarget(ulVector, &ucTargetLocalApicId);
        LW_CPU_SET(X86_APICID_TO_LOGICID(ucTargetLocalApicId), pcpuset);

    } else if (__x86IsLocalApicVector(ulVector)) {
        LW_CPU_SET(LW_CPU_GET_CUR_ID(), pcpuset);

    } else if (__x86IsIpiVector(ulVector)) {
        LW_CPU_SET((ulVector - _G_uiX86IpiVectorBase), pcpuset);
    }

    return  (ERROR_NONE);
}

#endif                                                                  /*  LW_CFG_INTER_TARGET > 0     */
/*********************************************************************************************************
  END
*********************************************************************************************************/
