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
** ��   ��   ��: ppcExcE500.c
**
** ��   ��   ��: Jiao.JinXing (������)
**
** �ļ���������: 2016 �� 05 �� 04 ��
**
** ��        ��: PowerPC E500 ��ϵ�����쳣����.
**
** ע        ��: Ŀǰû��ʵ�� E.HV(Embedded.Hypervisor) �� E.HV.LRAT(Embedded.Hypervisor.LRAT) ��ص�
**               �쳣����, δ��֧��Ӳ�����⻯ʱ��Ҫ������ص��쳣����.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "SylixOS.h"
#include "dtrace.h"
#include "../ppcSpr.h"
#include "ppcSprE500.h"
#define  __SYLIXOS_PPC_E500__
#define  __SYLIXOS_PPC_E500MC__
#include "arch/ppc/arch_e500.h"
#include "arch/ppc/param/ppcParam.h"
#if LW_CFG_VMM_EN > 0
#include "arch/ppc/mm/mmu/e500/ppcMmuE500.h"
#include "arch/ppc/mm/mmu/ppc460/ppcMmu460.h"
#endif                                                                  /*  LW_CFG_VMM_EN > 0           */
#include "arch/ppc/common/unaligned/ppcUnaligned.h"
/*********************************************************************************************************
  ��ຯ������
*********************************************************************************************************/
extern VOID  archE500CriticalInputExceptionEntry(VOID);
extern VOID  archE500MachineCheckExceptionEntry(VOID);
extern VOID  archE500DataStorageExceptionEntry(VOID);
extern VOID  archE500InstructionStorageExceptionEntry(VOID);
extern VOID  archE500ExternalInterruptEntry(VOID);
extern VOID  archE500AlignmentExceptionEntry(VOID);
extern VOID  archE500ProgramExceptionEntry(VOID);
extern VOID  archE500FpuUnavailableExceptionEntry(VOID);
extern VOID  archE500SystemCallEntry(VOID);
extern VOID  archE500ApUnavailableExceptionEntry(VOID);
extern VOID  archE500DecrementerInterruptEntry(VOID);
extern VOID  archE500TimerInterruptEntry(VOID);
extern VOID  archE500WatchdogInterruptEntry(VOID);
extern VOID  archE500DataTLBErrorEntry(VOID);
extern VOID  archE500InstructionTLBErrorEntry(VOID);
extern VOID  archE500DebugExceptionEntry(VOID);
extern VOID  archE500SpeUnavailableExceptionEntry(VOID);
extern VOID  archE500FpDataExceptionEntry(VOID);
extern VOID  archE500FpRoundExceptionEntry(VOID);
extern VOID  archE500AltiVecUnavailableExceptionEntry(VOID);
extern VOID  archE500AltiVecAssistExceptionEntry(VOID);
extern VOID  archE500PerfMonitorExceptionEntry(VOID);
extern VOID  archE500DoorbellExceptionEntry(VOID);
extern VOID  archE500DoorbellCriticalExceptionEntry(VOID);

extern VOID  arch460DataTLBErrorEntry(VOID);
extern VOID  arch460InstructionTLBErrorEntry(VOID);
/*********************************************************************************************************
  ʵ����غ���ָ��
*********************************************************************************************************/
static UINT32  (*_G_pfuncGetMCAR)(VOID) = ppcE500GetMCAR;
#if LW_CFG_VMM_EN > 0
static ULONG   (*_G_pfuncMmuStorageAbortType)(addr_t, UINT) = ppcE500MmuStorageAbortType;
#endif                                                                  /*  LW_CFG_VMM_EN > 0           */
/*********************************************************************************************************
** ��������: archE500CriticalInputExceptionHandle
** ��������: �ٽ������쳣����
** �䡡��  : ulRetAddr  �쳣���ص�ַ
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
** ע  ��  : �˺����˳�ʱ����Ϊ�жϹر�״̬.
*********************************************************************************************************/
VOID  archE500CriticalInputExceptionHandle (addr_t  ulRetAddr)
{
    PLW_CLASS_TCB   ptcbCur;
    LW_VMM_ABORT    abtInfo;

    LW_TCB_GET_CUR(ptcbCur);

    abtInfo.VMABT_uiType = LW_VMM_ABORT_TYPE_TERMINAL;
    API_VmmAbortIsr(ulRetAddr, ulRetAddr, &abtInfo, ptcbCur);
}
/*********************************************************************************************************
** ��������: archE500MachineCheckExceptionHandle
** ��������: ��������쳣����
** �䡡��  : ulRetAddr  �쳣���ص�ַ
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
** ע  ��  : �˺����˳�ʱ����Ϊ�жϹر�״̬.
*********************************************************************************************************/
VOID  archE500MachineCheckExceptionHandle (addr_t  ulRetAddr)
{
    PLW_CLASS_TCB   ptcbCur;
    LW_VMM_ABORT    abtInfo;
    UINT32          uiMCSR = ppcE500GetMCSR();
#if LW_CFG_CPU_EXC_HOOK_EN > 0
    addr_t          ulAbortAddr = _G_pfuncGetMCAR();
#endif

    LW_TCB_GET_CUR(ptcbCur);

#if LW_CFG_CPU_EXC_HOOK_EN > 0
    if (bspCpuExcHook(ptcbCur, ulRetAddr, ulAbortAddr, ARCH_MACHINE_EXCEPTION, uiMCSR)) {
        return;
    }
#endif

    _DebugFormat(__ERRORMESSAGE_LEVEL, "Machine error detected! MCSR 0x%x.\r\n", uiMCSR);
    abtInfo.VMABT_uiType = LW_VMM_ABORT_TYPE_FATAL_ERROR;
    API_VmmAbortIsr(ulRetAddr, ulRetAddr, &abtInfo, ptcbCur);
}
/*********************************************************************************************************
** ��������: archE500DataStorageExceptionHandle
** ��������: ���ݴ洢�쳣����
** �䡡��  : ulRetAddr  �쳣���ص�ַ
**           pregctx    �Ĵ���������
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
** ע  ��  : �˺����˳�ʱ����Ϊ�жϹر�״̬.
*********************************************************************************************************/
VOID  archE500DataStorageExceptionHandle (addr_t  ulRetAddr, ARCH_REG_CTX  *pregctx)
{
    PLW_CLASS_TCB   ptcbCur;
    addr_t          ulAbortAddr;
    LW_VMM_ABORT    abtInfo;

    ulAbortAddr          = ppcE500GetDEAR();
    abtInfo.VMABT_uiType = LW_VMM_ABORT_TYPE_TERMINAL;

    LW_TCB_GET_CUR(ptcbCur);

#if LW_CFG_VMM_EN > 0
    UINT32  uiESR = ppcE500GetESR();

    if (uiESR & ARCH_PPC_ESR_BO) {                                      /*  a byte-ordering exception   */
        abtInfo.VMABT_uiType = LW_VMM_ABORT_TYPE_TERMINAL;

    } else if (uiESR & ARCH_PPC_ESR_DLK) {                              /*  a DSI occurs because dcbtls,*/
                                                                        /*  dcbtstls, or dcblc is       */
                                                                        /*  executed in user mode       */
        abtInfo.VMABT_uiType = LW_VMM_ABORT_TYPE_TERMINAL;

    } else if (uiESR & ARCH_PPC_ESR_ST) {                               /*  a store or store-class cache*/
                                                                        /*  management instruction      */
        abtInfo.VMABT_uiMethod = LW_VMM_ABORT_METHOD_WRITE;
        abtInfo.VMABT_uiType   = _G_pfuncMmuStorageAbortType(ulAbortAddr,
                                                             LW_VMM_ABORT_METHOD_WRITE);
    } else {
        abtInfo.VMABT_uiMethod = LW_VMM_ABORT_METHOD_READ;
        abtInfo.VMABT_uiType   = _G_pfuncMmuStorageAbortType(ulAbortAddr,
                                                             LW_VMM_ABORT_METHOD_READ);
    }
#endif                                                                  /*  LW_CFG_VMM_EN > 0           */

    if (abtInfo.VMABT_uiType) {
        API_VmmAbortIsr(ulRetAddr, ulAbortAddr, &abtInfo, ptcbCur);
    }
}
/*********************************************************************************************************
** ��������: archE500InstructionStorageExceptionHandle
** ��������: ָ������쳣����
** �䡡��  : ulRetAddr  �쳣���ص�ַ
**           pregctx    �Ĵ���������
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
** ע  ��  : �˺����˳�ʱ����Ϊ�жϹر�״̬.
*********************************************************************************************************/
VOID  archE500InstructionStorageExceptionHandle (addr_t  ulRetAddr, ARCH_REG_CTX  *pregctx)
{
    PLW_CLASS_TCB   ptcbCur;
    addr_t          ulAbortAddr;
    LW_VMM_ABORT    abtInfo;

    ulAbortAddr          = ulRetAddr;
    abtInfo.VMABT_uiType = LW_VMM_ABORT_TYPE_TERMINAL;

    LW_TCB_GET_CUR(ptcbCur);

#if LW_CFG_VMM_EN > 0
    UINT32  uiESR = ppcE500GetESR();
    if (uiESR & ARCH_PPC_ESR_BO) {                                      /*  the instruction fetch caused*/
                                                                        /*  a byte-ordering exception   */
        abtInfo.VMABT_uiType = LW_VMM_ABORT_TYPE_TERMINAL;

    } else {
        abtInfo.VMABT_uiMethod = LW_VMM_ABORT_METHOD_EXEC;
        abtInfo.VMABT_uiType   = _G_pfuncMmuStorageAbortType(ulAbortAddr,
                                                             LW_VMM_ABORT_METHOD_EXEC);
    }
#endif                                                                  /*  LW_CFG_VMM_EN > 0           */

    if (abtInfo.VMABT_uiType) {
        API_VmmAbortIsr(ulRetAddr, ulAbortAddr, &abtInfo, ptcbCur);
    }
}
/*********************************************************************************************************
** ��������: archE500UnalignedHandle
** ��������: �Ƕ����ڴ�����쳣����
** �䡡��  : pabtctx    abort ������
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static VOID  archE500UnalignedHandle (PLW_VMM_ABORT_CTX  pabtctx)
{
    ppcUnalignedHandle(&pabtctx->ABTCTX_archRegCtx,
                       &pabtctx->ABTCTX_abtInfo);

    API_VmmAbortReturn(pabtctx);
}
/*********************************************************************************************************
** ��������: archE500AlignmentExceptionHandle
** ��������: �Ƕ����쳣����
** �䡡��  : ulRetAddr  �쳣���ص�ַ
**           pregctx    �Ĵ���������
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
** ע  ��  : �˺����˳�ʱ����Ϊ�жϹر�״̬.
*********************************************************************************************************/
VOID  archE500AlignmentExceptionHandle (addr_t  ulRetAddr, ARCH_REG_CTX  *pregctx)
{
    PLW_CLASS_TCB  ptcbCur;
    LW_VMM_ABORT   abtInfo;
    addr_t         ulAbortAddr;
    PPC_PARAM     *param = archKernelParamGet();

    LW_TCB_GET_CUR(ptcbCur);

    ulAbortAddr = ppcE500GetDEAR();
    pregctx->REG_uiDar = ulAbortAddr;

    abtInfo.VMABT_uiType   = LW_VMM_ABORT_TYPE_BUS;
    abtInfo.VMABT_uiMethod = BUS_ADRALN;

    if (param->PP_bUnalign) {
        API_VmmAbortIsrEx(ulRetAddr, ulAbortAddr, &abtInfo, ptcbCur, archE500UnalignedHandle);

    } else {
        API_VmmAbortIsr(ulRetAddr, ulAbortAddr, &abtInfo, ptcbCur);
    }
}
/*********************************************************************************************************
** ��������: archE500ProgramExceptionHandle
** ��������: �����쳣����
** �䡡��  : ulRetAddr  �쳣���ص�ַ
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
** ע  ��  : �˺����˳�ʱ����Ϊ�жϹر�״̬.
*********************************************************************************************************/
VOID  archE500ProgramExceptionHandle (addr_t  ulRetAddr)
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
** ��������: archE500FpuUnavailableExceptionHandle
** ��������: FPU �������쳣����
** �䡡��  : ulRetAddr  �쳣���ص�ַ
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
** ע  ��  : �˺����˳�ʱ����Ϊ�жϹر�״̬.
*********************************************************************************************************/
VOID  archE500FpuUnavailableExceptionHandle (addr_t  ulRetAddr)
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
** ��������: archE500ApUnavailableExceptionHandle
** ��������: AP �������쳣����
** �䡡��  : ulRetAddr  �쳣���ص�ַ
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
** ע  ��  : �˺����˳�ʱ����Ϊ�жϹر�״̬.
*********************************************************************************************************/
LW_WEAK VOID  archE500ApUnavailableExceptionHandle (addr_t  ulRetAddr)
{
    PLW_CLASS_TCB   ptcbCur;
    LW_VMM_ABORT    abtInfo;

    LW_TCB_GET_CUR(ptcbCur);

    abtInfo.VMABT_uiType   = LW_VMM_ABORT_TYPE_UNDEF;
    abtInfo.VMABT_uiMethod = LW_VMM_ABORT_METHOD_EXEC;
    API_VmmAbortIsr(ulRetAddr, ulRetAddr, &abtInfo, ptcbCur);
}
/*********************************************************************************************************
** ��������: archE500SystemCallHandle
** ��������: ϵͳ���ô���
** �䡡��  : ulRetAddr  �쳣���ص�ַ
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
** ע  ��  : �˺����˳�ʱ����Ϊ�жϹر�״̬.
*********************************************************************************************************/
VOID  archE500SystemCallHandle (addr_t  ulRetAddr)
{
    PLW_CLASS_TCB   ptcbCur;
    LW_VMM_ABORT    abtInfo;

    LW_TCB_GET_CUR(ptcbCur);

    abtInfo.VMABT_uiType = LW_VMM_ABORT_TYPE_SYS;
    API_VmmAbortIsr(ulRetAddr, ulRetAddr, &abtInfo, ptcbCur);
}
/*********************************************************************************************************
** ��������: archE500DataTLBErrorHandle
** ��������: ���ݷ��� TLB �����쳣����
** �䡡��  : ulRetAddr  �쳣���ص�ַ
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
** ע  ��  : �˺����˳�ʱ����Ϊ�жϹر�״̬.
*********************************************************************************************************/
#if LW_CFG_VMM_EN == 0

VOID  archE500DataTLBErrorHandle (addr_t  ulRetAddr)
{
    PLW_CLASS_TCB   ptcbCur;
    addr_t          ulAbortAddr;
    LW_VMM_ABORT    abtInfo;

    LW_TCB_GET_CUR(ptcbCur);

    ulAbortAddr = ppcE500GetDEAR();

    abtInfo.VMABT_uiType = LW_VMM_ABORT_TYPE_TERMINAL;
    API_VmmAbortIsr(ulRetAddr, ulAbortAddr, &abtInfo, ptcbCur);
}
/*********************************************************************************************************
** ��������: archE500InstructionTLBErrorHandle
** ��������: ָ����� TLB �����쳣����
** �䡡��  : ulRetAddr  �쳣���ص�ַ
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
** ע  ��  : �˺����˳�ʱ����Ϊ�жϹر�״̬.
*********************************************************************************************************/
VOID  archE500InstructionTLBErrorHandle (addr_t  ulRetAddr)
{
    PLW_CLASS_TCB   ptcbCur;
    LW_VMM_ABORT    abtInfo;

    LW_TCB_GET_CUR(ptcbCur);

    abtInfo.VMABT_uiType = LW_VMM_ABORT_TYPE_TERMINAL;
    API_VmmAbortIsr(ulRetAddr, ulRetAddr, &abtInfo, ptcbCur);
}

#endif                                                                 /*  LW_CFG_VMM_EN == 0           */
/*********************************************************************************************************
** ��������: archE500DebugExceptionHandle
** ��������: �����쳣����
** �䡡��  : ulRetAddr  �쳣���ص�ַ
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
** ע  ��  : �˺����˳�ʱ����Ϊ�жϹر�״̬.
*********************************************************************************************************/
VOID  archE500DebugExceptionHandle (addr_t  ulRetAddr)
{
    PLW_CLASS_TCB   ptcbCur;
    LW_VMM_ABORT    abtInfo;

    LW_TCB_GET_CUR(ptcbCur);

    abtInfo.VMABT_uiType = LW_VMM_ABORT_TYPE_TERMINAL;
    API_VmmAbortIsr(ulRetAddr, ulRetAddr, &abtInfo, ptcbCur);
}
/*********************************************************************************************************
** ��������: archE500SpeUnavailableExceptionHandle
** ��������: SPE �������쳣����
** �䡡��  : ulRetAddr  �쳣���ص�ַ
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
** ע  ��  : �˺����˳�ʱ����Ϊ�жϹر�״̬.
*********************************************************************************************************/
VOID  archE500SpeUnavailableExceptionHandle (addr_t  ulRetAddr)
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
** ��������: archE500FpDataExceptionHandle
** ��������: SPE floating-point data �쳣����
** �䡡��  : ulRetAddr  �쳣���ص�ַ
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
** ע  ��  : �˺����˳�ʱ����Ϊ�жϹر�״̬.
*********************************************************************************************************/
VOID  archE500FpDataExceptionHandle (addr_t  ulRetAddr)
{
    PLW_CLASS_TCB   ptcbCur;
    LW_VMM_ABORT    abtInfo;

    LW_TCB_GET_CUR(ptcbCur);

    /*
     * SPEFSCR[FINVE] = 1 and either SPEFSCR[FINVH, FINV] = 1
     * SPEFSCR[FDBZE] = 1 and either SPEFSCR[FDBZH, FDBZ] = 1
     * SPEFSCR[FUNFE] = 1 and either SPEFSCR[FUNFH, FUNF] = 1
     * SPEFSCR[FOVFE] = 1 and either SPEFSCR[FOVFH, FOVF] = 1
     */
    abtInfo.VMABT_uiType   = LW_VMM_ABORT_TYPE_FPE;
    abtInfo.VMABT_uiMethod = FPE_FLTUND;
    API_VmmAbortIsr(ulRetAddr, ulRetAddr, &abtInfo, ptcbCur);
}
/*********************************************************************************************************
** ��������: archE500FpRoundExceptionHandle
** ��������: SPE floating-point round �쳣����
** �䡡��  : ulRetAddr  �쳣���ص�ַ
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
** ע  ��  : �˺����˳�ʱ����Ϊ�жϹر�״̬.
*********************************************************************************************************/
VOID  archE500FpRoundExceptionHandle (addr_t  ulRetAddr)
{
    PLW_CLASS_TCB   ptcbCur;
    LW_VMM_ABORT    abtInfo;

    LW_TCB_GET_CUR(ptcbCur);

    /*
     * SPEFSCR[FINXE] = 1 and any of the SPEFSCR[FGH, FXH, FG, FX] bits = 1
     * SPEFSCR[FRMC]  = 0b10 (+��)
     * SPEFSCR[FRMC]  = 0b11 (-��)
     */
    abtInfo.VMABT_uiType   = LW_VMM_ABORT_TYPE_FPE;
    abtInfo.VMABT_uiMethod = FPE_FLTOVF;
    API_VmmAbortIsr(ulRetAddr, ulRetAddr, &abtInfo, ptcbCur);
}
/*********************************************************************************************************
** ��������: archE500AltiVecUnavailableExceptionHandle
** ��������: AltiVec �������쳣����
** �䡡��  : ulRetAddr  �쳣���ص�ַ
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
** ע  ��  : �˺����˳�ʱ����Ϊ�жϹر�״̬.
*********************************************************************************************************/
VOID  archE500AltiVecUnavailableExceptionHandle (addr_t  ulRetAddr)
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
** ��������: archE500AltiVecAssistExceptionHandle
** ��������: AltiVec Assist �쳣����
** �䡡��  : ulRetAddr  �쳣���ص�ַ
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
** ע  ��  : �˺����˳�ʱ����Ϊ�жϹر�״̬.
*********************************************************************************************************/
VOID  archE500AltiVecAssistExceptionHandle (addr_t  ulRetAddr)
{
    PLW_CLASS_TCB   ptcbCur;
    LW_VMM_ABORT    abtInfo;

    LW_TCB_GET_CUR(ptcbCur);

    abtInfo.VMABT_uiType   = LW_VMM_ABORT_TYPE_DSPE;
    abtInfo.VMABT_uiMethod = FPE_FLTINV;
    API_VmmAbortIsr(ulRetAddr, ulRetAddr, &abtInfo, ptcbCur);
}
/*********************************************************************************************************
** ��������: archE500PerfMonitorExceptionHandle
** ��������: Performance monitor �쳣����
** �䡡��  : ulRetAddr  �쳣���ص�ַ
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
** ע  ��  : �˺����˳�ʱ����Ϊ�жϹر�״̬.
*********************************************************************************************************/
LW_WEAK VOID  archE500PerfMonitorExceptionHandle (addr_t  ulRetAddr)
{
}
/*********************************************************************************************************
** ��������: archE500DoorbellExceptionHandle
** ��������: Processor doorbell �쳣����
** �䡡��  : ulRetAddr  �쳣���ص�ַ
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
** ע  ��  : �˺����˳�ʱ����Ϊ�жϹر�״̬.
*********************************************************************************************************/
LW_WEAK VOID  archE500DoorbellExceptionHandle (addr_t  ulRetAddr)
{
}
/*********************************************************************************************************
** ��������: archE500DoorbellCriticalExceptionHandle
** ��������: Processor doorbell critical �쳣����
** �䡡��  : ulRetAddr  �쳣���ص�ַ
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
** ע  ��  : �˺����˳�ʱ����Ϊ�жϹر�״̬.
*********************************************************************************************************/
LW_WEAK VOID  archE500DoorbellCriticalExceptionHandle (addr_t  ulRetAddr)
{
}
/*********************************************************************************************************
** ��������: archE500DecrementerInterruptAck
** ��������: Decrementer �ж� ACK
** �䡡��  : NONE
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
VOID  archE500DecrementerInterruptAck (VOID)
{
    UINT32  uiTSR;

    uiTSR  = ppcE500GetTSR();
    uiTSR |= ARCH_PPC_TSR_DIS_U << 16;
    ppcE500SetTSR(uiTSR);
}
/*********************************************************************************************************
** ��������: archE500DecrementerInterruptHandle
** ��������: Decrementer �жϴ���
** �䡡��  : NONE
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
** ע  ��  : �˺����˳�ʱ����Ϊ�жϹر�״̬.
*********************************************************************************************************/
LW_WEAK VOID  archE500DecrementerInterruptHandle (VOID)
{
    archE500DecrementerInterruptAck();
}
/*********************************************************************************************************
** ��������: archE500DecrementerInterruptEnable
** ��������: Decrementer �ж�ʹ��
** �䡡��  : NONE
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
VOID  archE500DecrementerInterruptEnable (VOID)
{
    UINT32  uiTCR;

    uiTCR  = ppcE500GetTCR();
    uiTCR |= ARCH_PPC_TCR_DIE_U << 16;
    ppcE500SetTCR(uiTCR);
}
/*********************************************************************************************************
** ��������: archE500DecrementerInterruptDisable
** ��������: Decrementer �жϽ���
** �䡡��  : NONE
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
VOID  archE500DecrementerInterruptDisable (VOID)
{
    UINT32  uiTCR;

    uiTCR  = ppcE500GetTCR();
    uiTCR &= ~(ARCH_PPC_TCR_DIE_U << 16);
    ppcE500SetTCR(uiTCR);
}
/*********************************************************************************************************
** ��������: archE500DecrementerInterruptIsEnable
** ��������: �ж� Decrementer �ж��Ƿ�ʹ��
** �䡡��  : NONE
** �䡡��  : LW_TRUE OR LW_FALSE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
BOOL  archE500DecrementerInterruptIsEnable (VOID)
{
    UINT32  uiTCR;

    uiTCR = ppcE500GetTCR();
    if (uiTCR & (ARCH_PPC_TCR_DIE_U << 16)) {
        return  (LW_TRUE);
    } else {
        return  (LW_FALSE);
    }
}
/*********************************************************************************************************
** ��������: archE500DecrementerAutoReloadEnable
** ��������: Decrementer �ж�ʹ��
** �䡡��  : NONE
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
VOID  archE500DecrementerAutoReloadEnable (VOID)
{
    UINT32  uiTCR;

    uiTCR  = ppcE500GetTCR();
    uiTCR |= ARCH_PPC_TCR_ARE_U << 16;
    ppcE500SetTCR(uiTCR);
}
/*********************************************************************************************************
** ��������: archE500DecrementerAutoReloadDisable
** ��������: Decrementer �жϽ���
** �䡡��  : NONE
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
VOID  archE500DecrementerAutoReloadDisable (VOID)
{
    UINT32  uiTCR;

    uiTCR  = ppcE500GetTCR();
    uiTCR &= ~(ARCH_PPC_TCR_ARE_U << 16);
    ppcE500SetTCR(uiTCR);
}
/*********************************************************************************************************
** ��������: archE500TimerInterruptAck
** ��������: �̶������ʱ���ж� ACK
** �䡡��  : NONE
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
VOID  archE500TimerInterruptAck (VOID)
{
    UINT32  uiTSR;

    uiTSR  = ppcE500GetTSR();
    uiTSR |= ARCH_PPC_TSR_FIS_U << 16;
    ppcE500SetTSR(uiTSR);
}
/*********************************************************************************************************
** ��������: archE500TimerInterruptHandle
** ��������: �̶������ʱ���жϴ���
** �䡡��  : NONE
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
** ע  ��  : �˺����˳�ʱ����Ϊ�жϹر�״̬.
*********************************************************************************************************/
LW_WEAK VOID  archE500TimerInterruptHandle (VOID)
{
    archE500TimerInterruptAck();
}
/*********************************************************************************************************
** ��������: archE500TimerInterruptEnable
** ��������: �̶������ʱ���ж�ʹ��
** �䡡��  : NONE
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
VOID  archE500TimerInterruptEnable (VOID)
{
    UINT32  uiTCR;

    uiTCR  = ppcE500GetTCR();
    uiTCR |= ARCH_PPC_TCR_FIE_U << 16;
    ppcE500SetTCR(uiTCR);
}
/*********************************************************************************************************
** ��������: archE500TimerInterruptDisable
** ��������: �̶������ʱ���жϽ���
** �䡡��  : NONE
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
VOID  archE500TimerInterruptDisable (VOID)
{
    UINT32  uiTCR;

    uiTCR  = ppcE500GetTCR();
    uiTCR &= ~(ARCH_PPC_TCR_FIE_U << 16);
    ppcE500SetTCR(uiTCR);
}
/*********************************************************************************************************
** ��������: archE500TimerInterruptIsEnable
** ��������: �жϹ̶������ʱ���ж��Ƿ�ʹ��
** �䡡��  : NONE
** �䡡��  : LW_TRUE OR LW_FALSE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
BOOL  archE500TimerInterruptIsEnable (VOID)
{
    UINT32  uiTCR;

    uiTCR = ppcE500GetTCR();
    if (uiTCR & (ARCH_PPC_TCR_FIE_U << 16)) {
        return  (LW_TRUE);
    } else {
        return  (LW_FALSE);
    }
}
/*********************************************************************************************************
** ��������: archE500WatchdogInterruptAck
** ��������: ���Ź���ʱ���ж� ACK
** �䡡��  : NONE
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
VOID  archE500WatchdogInterruptAck (VOID)
{
    UINT32  uiTSR;

    uiTSR  = ppcE500GetTSR();
    uiTSR |= ARCH_PPC_TSR_WIS_U << 16;
    ppcE500SetTSR(uiTSR);
}
/*********************************************************************************************************
** ��������: archE500WatchdogInterruptHandle
** ��������: ���Ź���ʱ���жϴ���
** �䡡��  : NONE
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
** ע  ��  : �˺����˳�ʱ����Ϊ�жϹر�״̬.
*********************************************************************************************************/
LW_WEAK VOID  archE500WatchdogInterruptHandle (VOID)
{
    archE500WatchdogInterruptAck();
}
/*********************************************************************************************************
** ��������: archE500WatchdogInterruptEnable
** ��������: ���Ź���ʱ���ж�ʹ��
** �䡡��  : NONE
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
VOID  archE500WatchdogInterruptEnable (VOID)
{
    UINT32  uiTCR;

    uiTCR  = ppcE500GetTCR();
    uiTCR |= ARCH_PPC_TCR_WIE_U << 16;
    ppcE500SetTCR(uiTCR);
}
/*********************************************************************************************************
** ��������: archE500WatchdogInterruptDisable
** ��������: ���Ź���ʱ���жϽ���
** �䡡��  : NONE
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
VOID  archE500WatchdogInterruptDisable (VOID)
{
    UINT32  uiTCR;

    uiTCR  = ppcE500GetTCR();
    uiTCR &= ~(ARCH_PPC_TCR_WIE_U << 16);
    ppcE500SetTCR(uiTCR);
}
/*********************************************************************************************************
** ��������: archE500WatchdogInterruptIsEnable
** ��������: �жϿ��Ź���ʱ���ж��Ƿ�ʹ��
** �䡡��  : NONE
** �䡡��  : LW_TRUE OR LW_FALSE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
BOOL  archE500WatchdogInterruptIsEnable (VOID)
{
    UINT32  uiTCR;

    uiTCR = ppcE500GetTCR();
    if (uiTCR & (ARCH_PPC_TCR_WIE_U << 16)) {
        return  (LW_TRUE);
    } else {
        return  (LW_FALSE);
    }
}
/*********************************************************************************************************
** ��������: archE500VectorInit
** ��������: ��ʼ�� E500 �쳣������
** �䡡��  : pcMachineName         ������
**           ulVectorBase          Interrupt Vector Prefix
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
VOID  archE500VectorInit (CPCHAR  pcMachineName, addr_t  ulVectorBase)
{
#define tostring(rn)    #rn
#define mtspr(rn, v)    asm volatile("mtspr " tostring(rn) ",%0" : : "r" (v))

    archE500WatchdogInterruptDisable();                                 /*  �رտ��Ź��ж�              */
    archE500DecrementerInterruptDisable();                              /*  �ر� DEC �ж�               */
    archE500TimerInterruptDisable();                                    /*  �رչ̶������ʱ���ж�      */

    mtspr(IVPR, ulVectorBase);

    mtspr(IVOR0,  (addr_t)archE500CriticalInputExceptionEntry - ulVectorBase);
    mtspr(IVOR1,  (addr_t)archE500MachineCheckExceptionEntry - ulVectorBase);
    mtspr(IVOR2,  (addr_t)archE500DataStorageExceptionEntry - ulVectorBase);
    mtspr(IVOR3,  (addr_t)archE500InstructionStorageExceptionEntry - ulVectorBase);
    mtspr(IVOR4,  (addr_t)archE500ExternalInterruptEntry - ulVectorBase);
    mtspr(IVOR5,  (addr_t)archE500AlignmentExceptionEntry - ulVectorBase);
    mtspr(IVOR6,  (addr_t)archE500ProgramExceptionEntry - ulVectorBase);
    mtspr(IVOR8,  (addr_t)archE500SystemCallEntry - ulVectorBase);
    mtspr(IVOR9,  (addr_t)archE500ApUnavailableExceptionEntry - ulVectorBase);
    mtspr(IVOR10, (addr_t)archE500DecrementerInterruptEntry - ulVectorBase);
    mtspr(IVOR11, (addr_t)archE500TimerInterruptEntry - ulVectorBase);
    mtspr(IVOR12, (addr_t)archE500WatchdogInterruptEntry - ulVectorBase);
    mtspr(IVOR13, (addr_t)archE500DataTLBErrorEntry - ulVectorBase);
    mtspr(IVOR14, (addr_t)archE500InstructionTLBErrorEntry - ulVectorBase);
    mtspr(IVOR15, (addr_t)archE500DebugExceptionEntry - ulVectorBase);
    mtspr(IVOR35, (addr_t)archE500PerfMonitorExceptionEntry - ulVectorBase);

    if ((lib_strcmp(pcMachineName, PPC_MACHINE_E500)   == 0) ||
        (lib_strcmp(pcMachineName, PPC_MACHINE_E500V1) == 0) ||
        (lib_strcmp(pcMachineName, PPC_MACHINE_E500V2) == 0)) {         /*  E500v1 v2 ʹ�� SPE          */
        mtspr(IVOR32, (addr_t)archE500SpeUnavailableExceptionEntry - ulVectorBase);
        mtspr(IVOR33, (addr_t)archE500FpDataExceptionEntry - ulVectorBase);
        mtspr(IVOR34, (addr_t)archE500FpRoundExceptionEntry - ulVectorBase);

    } else {                                                            /*  E500mc E5500 �Ⱥ���ʹ�� FPU */
        mtspr(IVOR7, (addr_t)archE500FpuUnavailableExceptionEntry - ulVectorBase);

        if (lib_strcmp(pcMachineName, PPC_MACHINE_E6500) == 0) {        /*  E6500 �� AltiVec            */
            mtspr(IVOR32, (addr_t)archE500AltiVecUnavailableExceptionEntry - ulVectorBase);
            mtspr(IVOR33, (addr_t)archE500AltiVecAssistExceptionEntry - ulVectorBase);
        }

        mtspr(IVOR36, (addr_t)archE500DoorbellExceptionEntry - ulVectorBase);
        mtspr(IVOR37, (addr_t)archE500DoorbellCriticalExceptionEntry - ulVectorBase);
    }
}
/*********************************************************************************************************
** ��������: arch460VectorInit
** ��������: ��ʼ�� PPC460 �쳣������
** �䡡��  : pcMachineName         ������
**           ulVectorBase          Interrupt Vector Prefix
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
VOID  arch460VectorInit (CPCHAR  pcMachineName, addr_t  ulVectorBase)
{
    archE500WatchdogInterruptDisable();                                 /*  �رտ��Ź��ж�              */
    archE500DecrementerInterruptDisable();                              /*  �ر� DEC �ж�               */
    archE500TimerInterruptDisable();                                    /*  �رչ̶������ʱ���ж�      */

    _G_pfuncGetMCAR = ppcE500GetDEAR;                                   /*  PPC460 �� MCAR, ʹ�� DEAR   */
#if LW_CFG_VMM_EN > 0
    _G_pfuncMmuStorageAbortType = ppc460MmuStorageAbortType;
#endif                                                                  /*  LW_CFG_VMM_EN > 0           */

    mtspr(IVPR, ulVectorBase);

    mtspr(IVOR0,  (addr_t)archE500CriticalInputExceptionEntry - ulVectorBase);
    mtspr(IVOR1,  (addr_t)archE500MachineCheckExceptionEntry - ulVectorBase);
    mtspr(IVOR2,  (addr_t)archE500DataStorageExceptionEntry - ulVectorBase);
    mtspr(IVOR3,  (addr_t)archE500InstructionStorageExceptionEntry - ulVectorBase);
    mtspr(IVOR4,  (addr_t)archE500ExternalInterruptEntry - ulVectorBase);
    mtspr(IVOR5,  (addr_t)archE500AlignmentExceptionEntry - ulVectorBase);
    mtspr(IVOR6,  (addr_t)archE500ProgramExceptionEntry - ulVectorBase);
    mtspr(IVOR7,  (addr_t)archE500FpuUnavailableExceptionEntry - ulVectorBase);
    mtspr(IVOR8,  (addr_t)archE500SystemCallEntry - ulVectorBase);
    mtspr(IVOR9,  (addr_t)archE500ApUnavailableExceptionEntry - ulVectorBase);
    mtspr(IVOR10, (addr_t)archE500DecrementerInterruptEntry - ulVectorBase);
    mtspr(IVOR11, (addr_t)archE500TimerInterruptEntry - ulVectorBase);
    mtspr(IVOR12, (addr_t)archE500WatchdogInterruptEntry - ulVectorBase);

#if LW_CFG_VMM_EN > 0
    mtspr(IVOR13, (addr_t)arch460DataTLBErrorEntry - ulVectorBase);
    mtspr(IVOR14, (addr_t)arch460InstructionTLBErrorEntry - ulVectorBase);
#else
    mtspr(IVOR13, (addr_t)archE500DataTLBErrorEntry - ulVectorBase);
    mtspr(IVOR14, (addr_t)archE500InstructionTLBErrorEntry - ulVectorBase);
#endif                                                                  /*  LW_CFG_VMM_EN > 0           */

    mtspr(IVOR15, (addr_t)archE500DebugExceptionEntry - ulVectorBase);
}
/*********************************************************************************************************
  END
*********************************************************************************************************/
