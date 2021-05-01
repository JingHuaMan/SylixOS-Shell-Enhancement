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
** ��   ��   ��: ppcExc.c
**
** ��   ��   ��: Jiao.JinXing (������)
**
** �ļ���������: 2015 �� 12 �� 15 ��
**
** ��        ��: PowerPC ��ϵ�����쳣����.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "SylixOS.h"
#include "dtrace.h"
#include "ppcSpr.h"
#include "arch/ppc/param/ppcParam.h"
#if LW_CFG_VMM_EN > 0
#include "arch/ppc/mm/mmu/hash/ppcMmuHash.h"
#endif
#include "arch/ppc/common/unaligned/ppcUnaligned.h"
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
  ���� Decrementer ���ȫ�ֱ���
*********************************************************************************************************/
static ULONG    _G_ulDecVector[LW_CFG_MAX_PROCESSORS];
static BOOL     _G_bDecPreemptive[LW_CFG_MAX_PROCESSORS];
static UINT32   _G_uiDecValue[LW_CFG_MAX_PROCESSORS];
static BOOL     _G_bDecInited[LW_CFG_MAX_PROCESSORS];
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
** ��������: archDataStorageExceptionHandle
** ��������: ���ݴ洢�쳣����
** �䡡��  : ulRetAddr  �쳣���ص�ַ
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
** ע  ��  : �˺����˳�ʱ����Ϊ�жϹر�״̬.
*********************************************************************************************************/
VOID  archDataStorageExceptionHandle (addr_t  ulRetAddr)
{
    PLW_CLASS_TCB   ptcbCur;
    addr_t          ulAbortAddr;
    LW_VMM_ABORT    abtInfo;

    ulAbortAddr          = ppcGetDAR();
    abtInfo.VMABT_uiType = LW_VMM_ABORT_TYPE_TERMINAL;

    LW_TCB_GET_CUR(ptcbCur);

#if LW_CFG_VMM_EN > 0
    UINT32  uiDSISR = ppcHashMmuGetDSISR();

    /*
     * See << programming_environment_manual >> Figure 7-16
     */
    if (uiDSISR & (0x1 << (31 - 1))) {
        /*
         * Page fault (no PTE found)
         */
        abtInfo.VMABT_uiType   = ppcHashMmuPteMissHandle(ulAbortAddr);
        abtInfo.VMABT_uiMethod = LW_VMM_ABORT_METHOD_READ;

    } else if (uiDSISR & (0x1 << (31 - 4))) {
        /*
         * Page protection violation
         */
        if (uiDSISR & (0x1 << (31 - 6))) {
            /*
             * If the access is a store
             */
            abtInfo.VMABT_uiType   = LW_VMM_ABORT_TYPE_PERM;
            abtInfo.VMABT_uiMethod = LW_VMM_ABORT_METHOD_WRITE;

        } else {
            /*
             * �������
             */
            abtInfo.VMABT_uiType   = LW_VMM_ABORT_TYPE_PERM;
            abtInfo.VMABT_uiMethod = LW_VMM_ABORT_METHOD_READ;
        }

    } else {
        /*
         * dcbt/dcbtst Instruction
         */
        abtInfo.VMABT_uiType = LW_VMM_ABORT_TYPE_TERMINAL;
    }
#endif                                                                  /*  LW_CFG_VMM_EN > 0           */

    if (abtInfo.VMABT_uiType) {
        API_VmmAbortIsr(ulRetAddr, ulAbortAddr, &abtInfo, ptcbCur);
    }
}
/*********************************************************************************************************
** ��������: archInstructionStorageExceptionHandle
** ��������: ָ������쳣����
** �䡡��  : ulRetAddr  �쳣���ص�ַ
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
** ע  ��  : �˺����˳�ʱ����Ϊ�жϹر�״̬.
*********************************************************************************************************/
VOID  archInstructionStorageExceptionHandle (addr_t  ulRetAddr)
{
    PLW_CLASS_TCB   ptcbCur;
    addr_t          ulAbortAddr;
    LW_VMM_ABORT    abtInfo;

    ulAbortAddr          = ppcGetDAR();
    abtInfo.VMABT_uiType = LW_VMM_ABORT_TYPE_TERMINAL;

    LW_TCB_GET_CUR(ptcbCur);

#if LW_CFG_VMM_EN > 0
    /*
     * See << programming_environment_manual >> Figure 7-16
     */
    UINT32  uiSRR1 = ppcHashMmuGetSRR1();

    if (uiSRR1 & (0x1 << (31 - 1))) {
        /*
         * Page fault (no PTE found)
         */
        abtInfo.VMABT_uiType   = ppcHashMmuPteMissHandle(ulAbortAddr);
        abtInfo.VMABT_uiMethod = LW_VMM_ABORT_METHOD_READ;

    } else if (uiSRR1 & (0x1 << (31 - 4))) {
        /*
         * Page protection violation
         * ������Ԥȡ
         */
        abtInfo.VMABT_uiType   = LW_VMM_ABORT_TYPE_PERM;
        abtInfo.VMABT_uiMethod = LW_VMM_ABORT_METHOD_READ;

    } else if (uiSRR1 & (0x1 << (31 - 3))) {
        /*
         * If the segment is designated as no-execute
         */
        abtInfo.VMABT_uiType   = LW_VMM_ABORT_TYPE_PERM;
        abtInfo.VMABT_uiMethod = LW_VMM_ABORT_METHOD_EXEC;

    } else {
        /*
         * dcbt/dcbtst Instruction
         */
        abtInfo.VMABT_uiType = LW_VMM_ABORT_TYPE_TERMINAL;
    }
#endif                                                                  /*  LW_CFG_VMM_EN > 0           */

    if (abtInfo.VMABT_uiType) {
        API_VmmAbortIsr(ulRetAddr, ulAbortAddr, &abtInfo, ptcbCur);
    }
}
/*********************************************************************************************************
** ��������: archUnalignedHandle
** ��������: �Ƕ����ڴ�����쳣����
** �䡡��  : pabtctx    abort ������
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static VOID  archUnalignedHandle (PLW_VMM_ABORT_CTX  pabtctx)
{
    ppcUnalignedHandle(&pabtctx->ABTCTX_archRegCtx,
                       &pabtctx->ABTCTX_abtInfo);

    API_VmmAbortReturn(pabtctx);
}
/*********************************************************************************************************
** ��������: archAlignmentExceptionHandle
** ��������: �Ƕ����쳣����
** �䡡��  : ulRetAddr  �쳣���ص�ַ
**           pregctx    �Ĵ���������
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
** ע  ��  : �˺����˳�ʱ����Ϊ�жϹر�״̬.
*********************************************************************************************************/
VOID  archAlignmentExceptionHandle (addr_t  ulRetAddr, ARCH_REG_CTX  *pregctx)
{
    PLW_CLASS_TCB  ptcbCur;
    LW_VMM_ABORT   abtInfo;
    addr_t         ulAbortAddr;
    PPC_PARAM     *param = archKernelParamGet();

    LW_TCB_GET_CUR(ptcbCur);

    ulAbortAddr = ppcGetDAR();
    pregctx->REG_uiDar = ulAbortAddr;

    abtInfo.VMABT_uiType   = LW_VMM_ABORT_TYPE_BUS;
    abtInfo.VMABT_uiMethod = BUS_ADRALN;

    if (param->PP_bUnalign) {
        API_VmmAbortIsrEx(ulRetAddr, ulAbortAddr, &abtInfo, ptcbCur, archUnalignedHandle);

    } else {
        API_VmmAbortIsr(ulRetAddr, ulAbortAddr, &abtInfo, ptcbCur);
    }
}
/*********************************************************************************************************
** ��������: archProgramExceptionHandle
** ��������: �����쳣����
** �䡡��  : ulRetAddr  �쳣���ص�ַ
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
** ע  ��  : �˺����˳�ʱ����Ϊ�жϹر�״̬.
*********************************************************************************************************/
VOID  archProgramExceptionHandle (addr_t  ulRetAddr)
{
    PLW_CLASS_TCB   ptcbCur;
    LW_VMM_ABORT    abtInfo;

#if LW_CFG_GDB_EN > 0
    UINT    uiBpType = archDbgTrapType(ulRetAddr, (PVOID)LW_NULL);      /*  �ϵ�ָ��̽��                */
    if (uiBpType) {
        if (API_DtraceBreakTrap(ulRetAddr, uiBpType) == ERROR_NONE) {   /*  ������Խӿڶϵ㴦��        */
            return;
        }
    }
#endif                                                                  /*  LW_CFG_GDB_EN > 0           */

    LW_TCB_GET_CUR(ptcbCur);

    abtInfo.VMABT_uiType   = LW_VMM_ABORT_TYPE_UNDEF;
    abtInfo.VMABT_uiMethod = LW_VMM_ABORT_METHOD_EXEC;
    API_VmmAbortIsr(ulRetAddr, ulRetAddr, &abtInfo, ptcbCur);
}
/*********************************************************************************************************
** ��������: archFpuUnavailableExceptionHandle
** ��������: FPU �������쳣����
** �䡡��  : ulRetAddr  �쳣���ص�ַ
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
** ע  ��  : �˺����˳�ʱ����Ϊ�жϹر�״̬.
*********************************************************************************************************/
VOID  archFpuUnavailableExceptionHandle (addr_t  ulRetAddr)
{
    PLW_CLASS_TCB   ptcbCur;
    LW_VMM_ABORT    abtInfo;

    LW_TCB_GET_CUR(ptcbCur);

#if LW_CFG_CPU_FPU_EN > 0
    if (archFpuUndHandle(ptcbCur) == ERROR_NONE) {                      /*  ���� FPU ָ��̽��           */
        return;
    }
#endif                                                                  /*  LW_CFG_CPU_FPU_EN > 0       */

    abtInfo.VMABT_uiType   = LW_VMM_ABORT_TYPE_FPE;
    abtInfo.VMABT_uiMethod = FPE_FLTINV;                                /*  FPU ������                  */
    API_VmmAbortIsr(ulRetAddr, ulRetAddr, &abtInfo, ptcbCur);
}
/*********************************************************************************************************
** ��������: archSystemCallHandle
** ��������: ϵͳ���ô���
** �䡡��  : ulRetAddr  �쳣���ص�ַ
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
** ע  ��  : �˺����˳�ʱ����Ϊ�жϹر�״̬.
*********************************************************************************************************/
VOID  archSystemCallHandle (addr_t  ulRetAddr)
{
    PLW_CLASS_TCB   ptcbCur;
    LW_VMM_ABORT    abtInfo;

    LW_TCB_GET_CUR(ptcbCur);

    abtInfo.VMABT_uiType = LW_VMM_ABORT_TYPE_SYS;
    API_VmmAbortIsr(ulRetAddr, ulRetAddr, &abtInfo, ptcbCur);
}
/*********************************************************************************************************
** ��������: archTraceHandle
** ��������: Trace ����
** �䡡��  : ulRetAddr  �쳣���ص�ַ
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
** ע  ��  : �˺����˳�ʱ����Ϊ�жϹر�״̬.
*********************************************************************************************************/
VOID  archTraceHandle (addr_t  ulRetAddr)
{
    PLW_CLASS_TCB   ptcbCur;
    LW_VMM_ABORT    abtInfo;

    LW_TCB_GET_CUR(ptcbCur);

    abtInfo.VMABT_uiType = LW_VMM_ABORT_TYPE_TERMINAL;
    API_VmmAbortIsr(ulRetAddr, ulRetAddr, &abtInfo, ptcbCur);
}
/*********************************************************************************************************
** ��������: archFpAssistExceptionHandle
** ��������: Floating-Point Assist �쳣����
** �䡡��  : ulRetAddr  �쳣���ص�ַ
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
** ע  ��  : �˺����˳�ʱ����Ϊ�жϹر�״̬.
*********************************************************************************************************/
VOID  archFpAssistExceptionHandle (addr_t  ulRetAddr)
{
    PLW_CLASS_TCB   ptcbCur;
    LW_VMM_ABORT    abtInfo;

    LW_TCB_GET_CUR(ptcbCur);

    /*
     * Optional. This interrupt can be used to provide software assistance for infrequent and complex
     * floating-point operations such as denormalization.
     *
     * The MPC750 does not generate an exception to this vector. Other PowerPC
     * processors may use this vector for floating-point assist exceptions.
     *
     *  1. Execution of floating-point instructions for which an implementation uses software routines to
     *     perform certain operations, such as those involving denormalization.
     *  2. Execution of floating-point instructions that are not optional and are not implemented in
     *     hardware.In this case, the processor may generate an illegal instruction type program
     *     interrupt instead.
     */

    abtInfo.VMABT_uiType   = LW_VMM_ABORT_TYPE_FPE;
    abtInfo.VMABT_uiMethod = FPE_FLTINV;
    API_VmmAbortIsr(ulRetAddr, ulRetAddr, &abtInfo, ptcbCur);
}
/*********************************************************************************************************
** ��������: archMachineCheckExceptionHandle
** ��������: ��������쳣����
** �䡡��  : ulRetAddr  �쳣���ص�ַ
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
** ע  ��  : �˺����˳�ʱ����Ϊ�жϹر�״̬.
*********************************************************************************************************/
VOID  archMachineCheckExceptionHandle (addr_t  ulRetAddr)
{
    PLW_CLASS_TCB   ptcbCur;
    LW_VMM_ABORT    abtInfo;

#if LW_CFG_CPU_EXC_HOOK_EN > 0
    addr_t          ulAbortAddr = ppcGetDAR();
#endif

    LW_TCB_GET_CUR(ptcbCur);

#if LW_CFG_CPU_EXC_HOOK_EN > 0
    if (bspCpuExcHook(ptcbCur, ulRetAddr, ulAbortAddr, ARCH_MACHINE_EXCEPTION, 0)) {
        return;
    }
#endif

    _DebugHandle(__ERRORMESSAGE_LEVEL, "Machine error detected!\r\n");
    abtInfo.VMABT_uiType = LW_VMM_ABORT_TYPE_FATAL_ERROR;
    API_VmmAbortIsr(ulRetAddr, ulRetAddr, &abtInfo, ptcbCur);
}
/*********************************************************************************************************
** ��������: archDecrementerInterruptHandle
** ��������: Decrementer �жϴ���
** �䡡��  : NONE
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
** ע  ��  : �˺����˳�ʱ����Ϊ�жϹر�״̬.
*********************************************************************************************************/
VOID  archDecrementerInterruptHandle (VOID)
{
    ULONG   ulCPUId = LW_CPU_GET_CUR_ID();

    if (_G_bDecInited[ulCPUId]) {
        ppcSetDEC(_G_uiDecValue[ulCPUId]);
        archIntHandle(_G_ulDecVector[ulCPUId], _G_bDecPreemptive[ulCPUId]);

    } else {
        ppcSetDEC(0x7fffffff);
    }
}
/*********************************************************************************************************
** ��������: archDecrementerInit
** ��������: ��ʼ�� Decrementer
** �䡡��  : ulVector          Decrementer �ж�����
**           bPreemptive       �Ƿ����ռ
**           uiDecValue        Decrementer ֵ
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
** ע  ��  : �����ڶ����񻷾�����ǰ���øú���.
*********************************************************************************************************/
VOID  archDecrementerInit (ULONG    ulVector,
                           BOOL     bPreemptive,
                           UINT32   uiDecValue)
{
    ULONG   ulCPUId = LW_CPU_GET_CUR_ID();

    _G_ulDecVector[ulCPUId]    = ulVector;
    _G_bDecPreemptive[ulCPUId] = bPreemptive;
    _G_uiDecValue[ulCPUId]     = uiDecValue;

    ppcSetDEC(uiDecValue);

    _G_bDecInited[ulCPUId] = LW_TRUE;
}
/*********************************************************************************************************
** ��������: archAltiVecUnavailableExceptionHandle
** ��������: AltiVec �������쳣����
** �䡡��  : ulRetAddr  �쳣���ص�ַ
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
** ע  ��  : �˺����˳�ʱ����Ϊ�жϹر�״̬.
*********************************************************************************************************/
VOID  archAltiVecUnavailableExceptionHandle (addr_t  ulRetAddr)
{
    PLW_CLASS_TCB   ptcbCur;
    LW_VMM_ABORT    abtInfo;

    LW_TCB_GET_CUR(ptcbCur);

#if LW_CFG_CPU_DSP_EN > 0
    if (archDspUndHandle(ptcbCur) == ERROR_NONE) {                      /*  ���� AltiVec ָ��̽��       */
        return;
    }
#endif                                                                  /*  LW_CFG_CPU_DSP_EN > 0       */

    abtInfo.VMABT_uiType   = LW_VMM_ABORT_TYPE_DSPE;
    abtInfo.VMABT_uiMethod = FPE_FLTINV;                                /*  AltiVec ������              */
    API_VmmAbortIsr(ulRetAddr, ulRetAddr, &abtInfo, ptcbCur);
}
/*********************************************************************************************************
** ��������: archAltiVecAssistExceptionHandle
** ��������: AltiVec Assist �쳣����
** �䡡��  : ulRetAddr  �쳣���ص�ַ
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
** ע  ��  : �˺����˳�ʱ����Ϊ�жϹر�״̬.
*********************************************************************************************************/
VOID  archAltiVecAssistExceptionHandle (addr_t  ulRetAddr)
{
    PLW_CLASS_TCB   ptcbCur;
    LW_VMM_ABORT    abtInfo;

    LW_TCB_GET_CUR(ptcbCur);

    abtInfo.VMABT_uiType   = LW_VMM_ABORT_TYPE_DSPE;
    abtInfo.VMABT_uiMethod = FPE_FLTINV;
    API_VmmAbortIsr(ulRetAddr, ulRetAddr, &abtInfo, ptcbCur);
}
/*********************************************************************************************************
  END
*********************************************************************************************************/
