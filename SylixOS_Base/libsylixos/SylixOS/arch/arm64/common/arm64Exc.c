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
** ��   ��   ��: arm64Exc.c
**
** ��   ��   ��: Wang.Xuan (���Q)
**
** �ļ���������: 2018 �� 06 �� 29 ��
**
** ��        ��: ARM64 ��ϵ�����쳣����.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "SylixOS.h"
#include "dtrace.h"
#include "../mm/mmu/arm64Mmu.h"
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
  �쳣���Ͷ���
*********************************************************************************************************/
#define EXC_UNKNOWN_REASON          0x00                                /*  000000                      */
#define EXC_TRAP_WFI_WFE            0x01                                /*  000001                      */
#define EXC_TRAP_MCR_MRC_CO1111     0x03                                /*  000011                      */
#define EXC_TRAP_MCRR_MRRC_CO1111   0x04                                /*  000100                      */
#define EXC_TRAP_MCR_MRC_CO1110     0x05                                /*  000101                      */
#define EXC_TRAP_LDC_STC            0x06                                /*  000110                      */
#define EXC_ACCESS_SIMD_FP          0x07                                /*  000111                      */
#define EXC_TRAP_VMRS               0x08                                /*  001000                      */
#define EXC_TRAP_MRRC_CO1110        0x0c                                /*  001100                      */
#define EXC_ILLEGAL_EXEC            0x0e                                /*  001110                      */
#define EXC_SVC_AARCH32             0x11                                /*  010001                      */
#define EXC_HVC_AARCH32             0x12                                /*  010010                      */
#define EXC_SMC_AARCH32             0x13                                /*  010011                      */
#define EXC_SVC_AARCH64             0x15                                /*  010101                      */
#define EXC_HVC_AARCH64             0x16                                /*  010110                      */
#define EXC_SMC_AARCH64             0x17                                /*  010111                      */
#define EXC_MSR_MRS_AARCH64         0x18                                /*  011000                      */
#define EXC_EL3                     0x1f                                /*  011111                      */
#define EXC_INSTRUCTION_ABORT_LO    0x20                                /*  100000                      */
#define EXC_INSTRUCTION_ABORT       0x21                                /*  100001                      */
#define EXC_PC_ALIGNMENT_FAULT      0x22                                /*  100010                      */
#define EXC_DATA_ABORT_LO           0x24                                /*  100100                      */
#define EXC_DATA_ABORT              0x25                                /*  100101                      */
#define EXC_SP_ALIGNMENT_FAULT      0x26                                /*  100110                      */
#define EXC_TRAP_FP_AARCH32         0x28                                /*  101000                      */
#define EXC_TRAP_FP_AARCH64         0x2c                                /*  101100                      */
#define EXC_SERROR_INT              0x2f                                /*  101111                      */
#define EXC_BREAKPOINT_LO           0x30                                /*  110000                      */
#define EXC_BREAKPOINT              0x31                                /*  110001                      */
#define EXC_SOFTWARE_STEP_LO        0x32                                /*  110010                      */
#define EXC_SOFTWARE_STEP           0x33                                /*  110011                      */
#define EXC_WATCHPOINT_LO           0x34                                /*  110100                      */
#define EXC_WATCHPOINT              0x35                                /*  110101                      */
#define EXC_BKPT_AARCH32            0x38                                /*  111000                      */
#define EXC_VECTOR_CATCH_AARCH32    0x3a                                /*  111010                      */
#define EXC_BRK_AARCH64             0x3c                                /*  111100                      */
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
LW_WEAK VOID  archIntHandle (ULONG  ulVector, BOOL  bPreemptive)
{
    REGISTER irqreturn_t irqret;

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
** ��������: bspCpuExcHook
** ��������: �������쳣�ص�
** �䡡��  : ptcb       �쳣������
**           ulRetAddr  �쳣���ص�ַ
**           ulExcAddr  �쳣��ַ
**           iExcType   �쳣����
**           iExcInfo   ��ϵ�ṹ����쳣��Ϣ
** �䡡��  : 0
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
#if LW_CFG_CPU_EXC_HOOK_EN > 0

LW_WEAK INT  bspCpuExcHook (PLW_CLASS_TCB   ptcb,
                            addr_t          ulRetAddr,
                            addr_t          ulExcAddr,
                            INT             iExcType, 
                            INT             iExcInfo)
{
    return  (0);
}

#endif                                                                  /*  LW_CFG_CPU_EXC_HOOK_EN      */
/*********************************************************************************************************
** ��������: archSyncExcHandle
** ��������: ���� Sync �쳣
** �䡡��  : pregctx    ������
**           uiExcType  �쳣����
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
VOID  archSyncExcHandle (ARCH_REG_CTX  *pregctx, UINT32  uiExcType)
{
#define  ARM64_EXC_TYPE_UNKNOWN   10

    PLW_CLASS_TCB   ptcbCur;
    LW_VMM_ABORT    abtInfo;
    UINT            uiExcClass;
    UINT            uiExcISS;
    ULONG           ulAbortAddr;
    UINT            uiBpType;
    
    LW_TCB_GET_CUR(ptcbCur);
     
    uiExcClass  = (uiExcType >> 26) & 0x3f;
    uiExcISS    = uiExcType & 0x1ffffff;
    ulAbortAddr = pregctx->REG_ulPc;

    switch (uiExcClass) {

    case EXC_UNKNOWN_REASON:
    case EXC_TRAP_WFI_WFE:
    case EXC_EL3:
        abtInfo.VMABT_uiMethod = 0;
        abtInfo.VMABT_uiType   = ARM64_EXC_TYPE_UNKNOWN;                /*  δ֪����                    */
        break;

    case EXC_TRAP_MCR_MRC_CO1111:
    case EXC_TRAP_MCRR_MRRC_CO1111:
    case EXC_TRAP_MCR_MRC_CO1110:
    case EXC_TRAP_LDC_STC:
    case EXC_TRAP_VMRS:
    case EXC_TRAP_MRRC_CO1110:
    case EXC_MSR_MRS_AARCH64:
        uiExcISS               = uiExcType & 0x1;
        abtInfo.VMABT_uiMethod = uiExcISS ? LW_VMM_ABORT_METHOD_READ : LW_VMM_ABORT_METHOD_WRITE;
        abtInfo.VMABT_uiType   = LW_VMM_ABORT_TYPE_PERM;                /*  ���ʴ���                    */
        break;

    case EXC_ACCESS_SIMD_FP:
    case EXC_TRAP_FP_AARCH32:
    case EXC_TRAP_FP_AARCH64:
        abtInfo.VMABT_uiMethod = LW_VMM_ABORT_METHOD_EXEC;
        abtInfo.VMABT_uiType   = LW_VMM_ABORT_TYPE_FPE;                 /*  �������                    */
#if LW_CFG_CPU_FPU_EN > 0
        if (archFpuUndHandle(ptcbCur) == ERROR_NONE) {                  /*  ���� FPU ָ��̽��           */
            return;
        }
#endif                                                                  /*  LW_CFG_CPU_FPU_EN > 0       */
        break;

    case EXC_ILLEGAL_EXEC:
    case EXC_PC_ALIGNMENT_FAULT:
    case EXC_SP_ALIGNMENT_FAULT:
        abtInfo.VMABT_uiMethod = LW_VMM_ABORT_METHOD_EXEC;
        abtInfo.VMABT_uiType   = LW_VMM_ABORT_TYPE_BUS;                 /*  �Ƕ�����ʴ���              */
        break;

    case EXC_SVC_AARCH32:
    case EXC_HVC_AARCH32:
    case EXC_SMC_AARCH32:
    case EXC_SVC_AARCH64:
    case EXC_HVC_AARCH64:
    case EXC_SMC_AARCH64:
       abtInfo.VMABT_uiMethod = LW_VMM_ABORT_METHOD_EXEC;
       abtInfo.VMABT_uiType   = LW_VMM_ABORT_TYPE_SYS;                  /*  ϵͳ���ô���                */
       break;

    case EXC_INSTRUCTION_ABORT_LO:
    case EXC_INSTRUCTION_ABORT:
        abtInfo.VMABT_uiMethod = LW_VMM_ABORT_METHOD_EXEC;
        abtInfo.VMABT_uiType   = LW_VMM_ABORT_TYPE_UNDEF;               /*  ָ�����                    */
        break;

    case EXC_DATA_ABORT_LO:
    case EXC_DATA_ABORT:
        ulAbortAddr            = arm64MmuAbtFaultAddr();
        abtInfo.VMABT_uiMethod = (uiExcISS & 0x40) ? LW_VMM_ABORT_METHOD_WRITE : LW_VMM_ABORT_METHOD_READ;
        abtInfo.VMABT_uiType   = LW_VMM_ABORT_TYPE_MAP;                 /*  ���ݴ���                    */
        if (((uiExcISS & 0xf) == 0xf) && (abtInfo.VMABT_uiMethod == LW_VMM_ABORT_METHOD_WRITE)) {
            abtInfo.VMABT_uiType = LW_VMM_ABORT_TYPE_PERM;              /*  Ȩ�޴���                    */
        }
        break;

    case EXC_SERROR_INT:
        abtInfo.VMABT_uiMethod = 0;
        abtInfo.VMABT_uiType   = LW_VMM_ABORT_TYPE_FATAL_ERROR;         /*  ��������                    */
        break;

    case EXC_SOFTWARE_STEP_LO:
    case EXC_SOFTWARE_STEP:
#if LW_CFG_GDB_EN > 0
        uiBpType = archDbgTrapType(ulAbortAddr,
                                   (PVOID)ARM64_DBG_TRAP_STEP);         /*  ����ָ��̽��                */
        if (uiBpType) {
            if (API_DtraceBreakTrap(ulAbortAddr, uiBpType)
                == ERROR_NONE) {                                        /*  ������Խӿڶϵ㴦��        */
                return;
            }
        }
#endif
        break;

    case EXC_BREAKPOINT_LO:
    case EXC_BREAKPOINT:
    case EXC_WATCHPOINT_LO:
    case EXC_WATCHPOINT:
    case EXC_BKPT_AARCH32:
    case EXC_VECTOR_CATCH_AARCH32:
    case EXC_BRK_AARCH64:
        abtInfo.VMABT_uiMethod = 0;
        abtInfo.VMABT_uiType   = LW_VMM_ABORT_TYPE_BREAK;               /*  �ϵ��쳣                    */

#if LW_CFG_GDB_EN > 0
        uiBpType = archDbgTrapType(ulAbortAddr, LW_NULL);               /*  �ϵ�ָ��̽��                */
        if (uiBpType) {
            if (API_DtraceBreakTrap(ulAbortAddr, uiBpType)
                == ERROR_NONE) {                                        /*  ������Խӿڶϵ㴦��        */
                return;
            }
        }
#endif                                                                  /*  LW_CFG_GDB_EN > 0           */
        break;

    default:
        break;
    }
    
#if LW_CFG_CPU_EXC_HOOK_EN > 0
    if (bspCpuExcHook(ptcbCur, 
                      pregctx->REG_ulPc, 
                      ulAbortAddr,
                      abtInfo.VMABT_uiType, 
                      abtInfo.VMABT_uiMethod)) {
        return;
    }
#endif
  
    API_VmmAbortIsr(pregctx->REG_ulPc, ulAbortAddr, &abtInfo, ptcbCur);
}
/*********************************************************************************************************
** ��������: archInvalidExcHandle
** ��������: ���� Invalid �쳣
** �䡡��  : pregctx    ������
**           uiExcType  �쳣����
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
VOID  archInvalidExcHandle (ARCH_REG_CTX  *pregctx, UINT32  uiExcType, UINT32  uiType)
{
    CPCHAR  pcException;
     
    _DebugFormat(__ERRORMESSAGE_LEVEL, "FATAL ERROR: exception.");      /*  �ؼ��Դ���                  */
     
    switch (uiType) {

    case EL1_SYN_INVALID:
        pcException = "el1SyncInvalid";
        break;
            
    case EL1_IRQ_INVALID:
        pcException = "el1IrqInvalid";
        break;
         
    case EL1_FIQ_INVALID:
        pcException = "el1FiqInvalid";
        break;
         
    case EL1_ERR_INVALID:
        pcException = "el1ErrorInvalid";
        break;

    case EL2_IRQ_AARCH64_INVALID:
        pcException = "el2IrqAArch64Invalid";
        break;

    case EL2_FIQ_AARCH64_INVALID:
        pcException = "el2FiqAArch64Invalid";
        break;

    case EL2_ERR_AARCH64_INVALID:
        pcException = "el2ErrAArch64Invalid";
        break;

    case EL2_SYN_AARCH32_INVALID:
        pcException = "el2SynAArch32Invalid";
        break;

    case EL2_IRQ_AARCH32_INVALID:
        pcException = "el2IrqAArch32Invalid";
        break;

    case EL2_FIQ_AARCH32_INVALID:
        pcException = "el2FiqAArch32Invalid";
        break;

    case EL2_ERR_AARCH32_INVALID:
        pcException = "el2ErrAArch32Invalid";
        break;

    default:
        pcException = "unknown";
        break;
    }
                         
    _DebugFormat(__ERRORMESSAGE_LEVEL,
                 "Invalid exception in %s handler detected on CPU%d, code 0x%08x\r\n",
                 pcException, LW_CPU_GET_CUR_ID(), uiExcType);

    archTaskCtxPrint(LW_NULL, 0, pregctx);

    API_KernelReboot(LW_REBOOT_FORCE);                                  /*  ֱ��������������ϵͳ        */
}
/*********************************************************************************************************
  END
*********************************************************************************************************/
