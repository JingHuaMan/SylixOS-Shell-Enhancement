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
** ��   ��   ��: x86Exc.c
**
** ��   ��   ��: Jiao.JinXing (������)
**
** �ļ���������: 2016 �� 07 �� 04 ��
**
** ��        ��: x86 ��ϵ�����쳣/�жϴ���.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "SylixOS.h"
#include "dtrace.h"
#include "x86Idt.h"
#include "x86Cr.h"
/*********************************************************************************************************
  �ⲿ��������
*********************************************************************************************************/
extern addr_t   _G_ulX86IntEntryArray[X86_IDTE_NUM];
/*********************************************************************************************************
  ȫ�ֱ�������
*********************************************************************************************************/
typedef VOID (*X86_INT_HANDLER)(ULONG  ulX86Vector, ARCH_REG_CTX  *pregctx);/*  �쳣/�жϴ���������   */

X86_INT_HANDLER _G_pfuncX86IntHandleArray[X86_IDTE_NUM];                /*  �쳣/�жϴ���������       */

static CPCHAR   _G_pcX86ExceName[] = {                                  /*  �쳣���ֱ�                  */
    [X86_EXCEPT_DIVIDE_ERROR]                = "Division by zero",
    [X86_EXCEPT_DEBUG]                       = "Debug",
    [X86_EXCEPT_NMI_INTERRUPT]               = "Non Maskable Interrupt",
    [X86_EXCEPT_BREAKPOINT]                  = "Breakpoint",
    [X86_EXCEPT_OVERFLOW]                    = "Overflow",
    [X86_EXCEPT_BOUND_RANGE_EXCEDEED]        = "Bound Range Exceeded",
    [X86_EXCEPT_INVALID_OPCODE]              = "Invalid Opcode",
    [X86_EXCEPT_DEVICE_NOT_AVAILABLE]        = "Device Unavailable",
    [X86_EXCEPT_DOUBLE_FAULT]                = "Double Fault",
    [X86_EXCEPT_COPROCESSOR_SEGMENT_OVERRUN] = "Coprocessor Segment Overrun",
    [X86_EXCEPT_INVALID_TSS]                 = "Invalid TSS",
    [X86_EXCEPT_SEGMENT_NOT_PRESENT]         = "Segment Not Present",
    [X86_EXCEPT_STACK_SEGMENT_FAULT]         = "Stack-Segment Fault",
    [X86_EXCEPT_GENERAL_PROTECTION]          = "General Protection",
    [X86_EXCEPT_PAGE_FAULT]                  = "Page Fault",
    [X86_EXCEPT_INTEL_RESERVED_1]            = "INTEL1",
    [X86_EXCEPT_FLOATING_POINT_ERROR]        = "X87 FPU FP Error",
    [X86_EXCEPT_ALIGNEMENT_CHECK]            = "Alignment Check",
    [X86_EXCEPT_MACHINE_CHECK]               = "Machine Check",
    [X86_EXCEPT_INTEL_RESERVED_2]            = "INTEL2",
    [X86_EXCEPT_INTEL_RESERVED_3]            = "INTEL3",
    [X86_EXCEPT_INTEL_RESERVED_4]            = "INTEL4",
    [X86_EXCEPT_INTEL_RESERVED_5]            = "INTEL5",
    [X86_EXCEPT_INTEL_RESERVED_6]            = "INTEL6",
    [X86_EXCEPT_INTEL_RESERVED_7]            = "INTEL7",
    [X86_EXCEPT_INTEL_RESERVED_8]            = "INTEL8",
    [X86_EXCEPT_INTEL_RESERVED_9]            = "INTEL9",
    [X86_EXCEPT_INTEL_RESERVED_10]           = "INTEL10",
    [X86_EXCEPT_INTEL_RESERVED_11]           = "INTEL11",
    [X86_EXCEPT_INTEL_RESERVED_12]           = "INTEL12",
    [X86_EXCEPT_INTEL_RESERVED_13]           = "INTEL13",
    [X86_EXCEPT_INTEL_RESERVED_14]           = "INTEL14"
};
/*********************************************************************************************************
  ����ʹ���������
*********************************************************************************************************/
#if LW_CFG_SMP_EN > 0
#define VECTOR_OP_LOCK()    LW_SPIN_LOCK_IGNIRQ(&_K_slcaVectorTable.SLCA_sl)
#define VECTOR_OP_UNLOCK()  LW_SPIN_UNLOCK_IGNIRQ(&_K_slcaVectorTable.SLCA_sl)
#else
#define VECTOR_OP_LOCK()
#define VECTOR_OP_UNLOCK()
#endif                                                                  /*  LW_CFG_SMP_EN > 0           */
/*********************************************************************************************************
** ��������: archIntHandle
** ��������: bspIntHandle ��Ҫ���ô˺��������ж� (�ر��ж����������)
** �䡡��  : ulVector         �ж�����
**           bPreemptive      �ж��Ƿ����ռ
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
** ע  ��  : �˺����˳�ʱ����Ϊ�жϹر�״̬.
*********************************************************************************************************/
VOID  archIntHandle (ULONG  ulVector, BOOL  bPreemptive)
{
    REGISTER irqreturn_t  irqret;

    if (_Inter_Vector_Invalid(ulVector)) {
        return;                                                         /*  �����Ų���ȷ                */
    }

    if (LW_IVEC_GET_FLAG(ulVector) & LW_IRQ_FLAG_PREEMPTIVE) {
        bPreemptive = LW_TRUE;
    }

    if (bPreemptive) {
        VECTOR_OP_LOCK();
        __ARCH_INT_VECTOR_DISABLE(ulVector);                            /*  ���� vector �ж�            */
        VECTOR_OP_UNLOCK();
        KN_INT_ENABLE_FORCE();                                          /*  �����ж�                    */
    }

    irqret = API_InterVectorIsr(ulVector);                              /*  �����жϷ������            */
    
    KN_INT_DISABLE();                                                   /*  �����ж�                    */
    
    if (bPreemptive) {
        if (irqret != LW_IRQ_HANDLED_DISV) {
            VECTOR_OP_LOCK();
            __ARCH_INT_VECTOR_ENABLE(ulVector);                         /*  ���� vector �ж�            */
            VECTOR_OP_UNLOCK();
        }
    
    } else if (irqret == LW_IRQ_HANDLED_DISV) {
        VECTOR_OP_LOCK();
        __ARCH_INT_VECTOR_DISABLE(ulVector);                            /*  ���� vector �ж�            */
        VECTOR_OP_UNLOCK();
    }
}
/*********************************************************************************************************
** ��������: x86DefaultExceptHandle
** ��������: ȱʡ�쳣������
** �䡡��  : ulX86Vector   x86 �쳣����
**           ulESP         �쳣���������ĺ�� ESP
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static VOID  x86DefaultExceptHandle (ULONG  ulX86Vector, ARCH_REG_CTX  *pregctx)
{
    PLW_CLASS_TCB   ptcbCur;
    LW_VMM_ABORT    abtInfo;
    addr_t          ulRetAddr;

    _PrintFormat("EXCEPTION: %s\r\n", _G_pcX86ExceName[ulX86Vector]);

    LW_TCB_GET_CUR(ptcbCur);

    ulRetAddr = pregctx->REG_XIP;

    abtInfo.VMABT_uiType = LW_VMM_ABORT_TYPE_TERMINAL;
    API_VmmAbortIsr(ulRetAddr, ulRetAddr, &abtInfo, ptcbCur);
}
/*********************************************************************************************************
** ��������: x86InvalidOpcodeExceptHandle
** ��������: ��Чָ���쳣������
** �䡡��  : ulX86Vector   x86 �쳣����
**           ulESP         �쳣���������ĺ�� ESP
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static VOID  x86InvalidOpcodeExceptHandle (ULONG  ulX86Vector, ARCH_REG_CTX  *pregctx)
{
    PLW_CLASS_TCB   ptcbCur;
    LW_VMM_ABORT    abtInfo;
    addr_t          ulRetAddr;

    LW_TCB_GET_CUR(ptcbCur);

    ulRetAddr = pregctx->REG_XIP;

    abtInfo.VMABT_uiType = LW_VMM_ABORT_TYPE_UNDEF;
    API_VmmAbortIsr(ulRetAddr, ulRetAddr, &abtInfo, ptcbCur);
}
/*********************************************************************************************************
** ��������: x86FpErrorExceptHandle
** ��������: ��������쳣������
** �䡡��  : ulX86Vector   x86 �쳣����
**           ulESP         �쳣���������ĺ�� ESP
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
#if LW_CFG_CPU_FPU_EN > 0

static VOID  x86FpErrorExceptHandle (ULONG  ulX86Vector, ARCH_REG_CTX  *pregctx)
{
    PLW_CLASS_TCB   ptcbCur;
    LW_VMM_ABORT    abtInfo;
    addr_t          ulRetAddr;

    LW_TCB_GET_CUR(ptcbCur);

    ulRetAddr = pregctx->REG_XIP;

    abtInfo.VMABT_uiType   = LW_VMM_ABORT_TYPE_FPE;
    abtInfo.VMABT_uiMethod = FPE_INTDIV;                                /*  Ĭ��Ϊ�� 0 �쳣             */
    API_VmmAbortIsr(ulRetAddr, ulRetAddr, &abtInfo, ptcbCur);
}

#endif                                                                  /*  LW_CFG_CPU_FPU_EN > 0       */
/*********************************************************************************************************
** ��������: x86DeviceNotAvailExceptHandle
** ��������: �豸�������쳣������
** �䡡��  : ulX86Vector   x86 �쳣����
**           ulESP         �쳣���������ĺ�� ESP
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static VOID  x86DeviceNotAvailExceptHandle (ULONG  ulX86Vector, ARCH_REG_CTX  *pregctx)
{
    PLW_CLASS_TCB   ptcbCur;
    LW_VMM_ABORT    abtInfo;
    addr_t          ulRetAddr;

    LW_TCB_GET_CUR(ptcbCur);

#if LW_CFG_CPU_FPU_EN > 0
    if (archFpuUndHandle(ptcbCur) == ERROR_NONE) {                      /*  ���� FPU ָ��̽��           */
        return;
    }
#endif                                                                  /*  LW_CFG_CPU_FPU_EN > 0       */

    ulRetAddr = pregctx->REG_XIP;

    abtInfo.VMABT_uiType   = LW_VMM_ABORT_TYPE_UNDEF;
    abtInfo.VMABT_uiMethod = 0;
    API_VmmAbortIsr(ulRetAddr, ulRetAddr, &abtInfo, ptcbCur);
}
/*********************************************************************************************************
** ��������: x86MachineCheckExceptHandle
** ��������: �豸�쳣����
** �䡡��  : ulX86Vector   x86 �쳣����
**           ulESP         �쳣���������ĺ�� ESP
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static VOID  x86MachineCheckExceptHandle (ULONG  ulX86Vector, ARCH_REG_CTX  *pregctx)
{
    PLW_CLASS_TCB   ptcbCur;
    LW_VMM_ABORT    abtInfo;
    addr_t          ulRetAddr;

    _PrintFormat("Machine error detected.\r\n");

    LW_TCB_GET_CUR(ptcbCur);

    ulRetAddr = pregctx->REG_XIP;

    abtInfo.VMABT_uiType = LW_VMM_ABORT_TYPE_FATAL_ERROR;
    API_VmmAbortIsr(ulRetAddr, ulRetAddr, &abtInfo, ptcbCur);
}
/*********************************************************************************************************
** ��������: x86PageFaultExceptHandle
** ��������: ҳ����ʴ����쳣������
** �䡡��  : ulX86Vector   x86 �쳣����
**           ulESP         �쳣���������ĺ�� ESP
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static VOID  x86PageFaultExceptHandle (ULONG  ulX86Vector, ARCH_REG_CTX  *pregctx)
{
    PLW_CLASS_TCB   ptcbCur;
    LW_VMM_ABORT    abtInfo;
    addr_t          ulRetAddr;
    addr_t          ulAbortAddr;

    LW_TCB_GET_CUR(ptcbCur);

    ulRetAddr   = pregctx->REG_XIP;
    ulAbortAddr = x86Cr2Get();

#if LW_CFG_VMM_EN > 0
    abtInfo.VMABT_uiType = (pregctx->REG_ERROR & (1 << 0)) ?
                            LW_VMM_ABORT_TYPE_PERM : LW_VMM_ABORT_TYPE_MAP;

    if (pregctx->REG_ERROR & (1 << 4)) {
        abtInfo.VMABT_uiMethod = LW_VMM_ABORT_METHOD_EXEC;

    } else {
        abtInfo.VMABT_uiMethod = (pregctx->REG_ERROR & (1 << 1)) ?
                                  LW_VMM_ABORT_METHOD_WRITE : LW_VMM_ABORT_METHOD_READ;
    }
#else
    abtInfo.VMABT_uiType   = LW_VMM_ABORT_TYPE_TERMINAL;
#endif                                                                  /*  LW_CFG_VMM_EN > 0           */

    API_VmmAbortIsr(ulRetAddr, ulAbortAddr, &abtInfo, ptcbCur);
}
/*********************************************************************************************************
** ��������: x86BreakPointExceptHandle
** ��������: �ϵ��쳣������
** �䡡��  : ulX86Vector   x86 �쳣����
**           ulESP         �쳣���������ĺ�� ESP
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static VOID  x86BreakPointExceptHandle (ULONG  ulX86Vector, ARCH_REG_CTX  *pregctx)
{
    PLW_CLASS_TCB   ptcbCur;
    LW_VMM_ABORT    abtInfo;
    addr_t          ulRetAddr;

    pregctx->REG_XIP -= 1;                                              /*  ���˵�ǰһ���ֽڵ� INT3 ָ��*/

    ulRetAddr = pregctx->REG_XIP;

#if LW_CFG_GDB_EN > 0
    UINT  uiBpType = archDbgTrapType(ulRetAddr, X86_DBG_TRAP_BP);       /*  �ϵ�ָ��̽��                */
    if (uiBpType) {
        if (API_DtraceBreakTrap(ulRetAddr, uiBpType) == ERROR_NONE) {   /*  ������Խӿڶϵ㴦��        */
            return;
        }
    }
#endif                                                                  /*  LW_CFG_GDB_EN > 0           */

    LW_TCB_GET_CUR(ptcbCur);

    abtInfo.VMABT_uiType   = LW_VMM_ABORT_TYPE_BREAK;
    abtInfo.VMABT_uiMethod = LW_VMM_ABORT_METHOD_EXEC;
    API_VmmAbortIsr(ulRetAddr, ulRetAddr, &abtInfo, ptcbCur);
}
/*********************************************************************************************************
** ��������: x86DebugExceptHandle
** ��������: �����ϵ��쳣������
** �䡡��  : ulX86Vector   x86 �쳣����
**           ulESP         �쳣���������ĺ�� ESP
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static VOID  x86DebugExceptHandle (ULONG  ulX86Vector, ARCH_REG_CTX  *pregctx)
{
    PLW_CLASS_TCB   ptcbCur;
    LW_VMM_ABORT    abtInfo;
    addr_t          ulRetAddr;

    ulRetAddr = pregctx->REG_XIP;

#if LW_CFG_GDB_EN > 0
    UINT  uiBpType = archDbgTrapType(ulRetAddr, X86_DBG_TRAP_STEP);     /*  �ϵ�ָ��̽��                */
    if (uiBpType) {
        if (API_DtraceBreakTrap(ulRetAddr, uiBpType) == ERROR_NONE) {   /*  ������Խӿڶϵ㴦��        */
            return;
        }
    }
#endif                                                                  /*  LW_CFG_GDB_EN > 0           */

    LW_TCB_GET_CUR(ptcbCur);

    abtInfo.VMABT_uiType   = LW_VMM_ABORT_TYPE_BREAK;
    abtInfo.VMABT_uiMethod = LW_VMM_ABORT_METHOD_EXEC;
    API_VmmAbortIsr(ulRetAddr, ulRetAddr, &abtInfo, ptcbCur);
}
/*********************************************************************************************************
** ��������: x86IrqHandle
** ��������: IRQ �жϴ�����
** �䡡��  : ulX86Vector   x86 �ж�����
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static VOID  x86IrqHandle (ULONG  ulX86Vector)
{
    ULONG  ulVector = ulX86Vector - X86_IRQ_BASE;                       /*  x86 Vector ת SylixOS Vector*/

    bspIntHandle(ulVector);
}
/*********************************************************************************************************
** ��������: x86ExceptIrqInit
** ��������: ��ʼ���쳣/�ж�
** �䡡��  : NONE
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
VOID  x86ExceptIrqInit (VOID)
{
    ULONG  ulX86Vector;

    for (ulX86Vector = X86_EXCEPT_BASE; ulX86Vector <= X86_EXCEPT_MAX; ulX86Vector++) {
        _G_pfuncX86IntHandleArray[ulX86Vector] = x86DefaultExceptHandle;
    }

    for (ulX86Vector = X86_IRQ_BASE; ulX86Vector <= X86_IRQ_MAX; ulX86Vector++) {
        _G_pfuncX86IntHandleArray[ulX86Vector] = (X86_INT_HANDLER)x86IrqHandle;
    }

    _G_pfuncX86IntHandleArray[X86_EXCEPT_PAGE_FAULT]     = x86PageFaultExceptHandle;
    _G_pfuncX86IntHandleArray[X86_EXCEPT_BREAKPOINT]     = x86BreakPointExceptHandle;
    _G_pfuncX86IntHandleArray[X86_EXCEPT_INVALID_OPCODE] = x86InvalidOpcodeExceptHandle;
    _G_pfuncX86IntHandleArray[X86_EXCEPT_DEBUG]          = x86DebugExceptHandle;
    _G_pfuncX86IntHandleArray[X86_EXCEPT_MACHINE_CHECK]  = x86MachineCheckExceptHandle;

#if LW_CFG_CPU_FPU_EN > 0
    _G_pfuncX86IntHandleArray[X86_EXCEPT_FLOATING_POINT_ERROR] = x86FpErrorExceptHandle;
#endif                                                                  /*  LW_CFG_CPU_FPU_EN > 0       */
    _G_pfuncX86IntHandleArray[X86_EXCEPT_DEVICE_NOT_AVAILABLE] = x86DeviceNotAvailExceptHandle;

    for (ulX86Vector = X86_EXCEPT_BASE; ulX86Vector <= X86_EXCEPT_MAX; ulX86Vector++) {
        x86IdtSetHandler(ulX86Vector, _G_ulX86IntEntryArray[ulX86Vector], 0);
    }

    for (ulX86Vector = X86_IRQ_BASE; ulX86Vector <= X86_IRQ_MAX; ulX86Vector++) {
        x86IdtSetHandler(ulX86Vector, _G_ulX86IntEntryArray[ulX86Vector], 0);
    }
}
/*********************************************************************************************************
  END
*********************************************************************************************************/
