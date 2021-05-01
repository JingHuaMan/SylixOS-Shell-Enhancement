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
** ��   ��   ��: riscvExc.c
**
** ��   ��   ��: Jiao.JinXing (������)
**
** �ļ���������: 2018 �� 03 �� 20 ��
**
** ��        ��: RISC-V ��ϵ�����쳣����.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "SylixOS.h"
#include "dtrace.h"
#if LW_CFG_RISCV_M_LEVEL > 0
#include "arch/riscv/param/riscvParam.h"
#include "arch/riscv/common/unaligned/riscvUnaligned.h"
#endif                                                                  /*  LW_CFG_RISCV_M_LEVEL > 0    */
#if LW_CFG_VMM_EN > 0
#include "arch/riscv/mm/mmu/riscvMmu.h"
#endif                                                                  /*  LW_CFG_VMM_EN > 0           */
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
LW_WEAK VOID  archIntHandle (ULONG  ulVector, BOOL  bPreemptive)
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
** ��������: archMmuAbortType
** ��������: ��÷�����ֹ����
** �䡡��  : ulAddr        ��ֹ��ַ
**           uiMethod      ���ʷ���(LW_VMM_ABORT_METHOD_XXX)
** �䡡��  : ������ֹ����
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static LW_INLINE ULONG  archMmuAbortType (addr_t  ulAddr, UINT  uiMethod)
{
#if LW_CFG_VMM_EN > 0
    return  (riscvMmuAbortType(ulAddr, uiMethod));
#else
    return  (LW_VMM_ABORT_TYPE_PERM);
#endif                                                                  /*  LW_CFG_VMM_EN > 0           */
}
/*********************************************************************************************************
** ��������: archDefaultTrapHandle
** ��������: ȱʡ�쳣����
** �䡡��  : pregctx       �Ĵ���������
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static VOID  archDefaultTrapHandle (ARCH_REG_CTX  *pregctx)
{
    PLW_CLASS_TCB  ptcbCur;
    LW_VMM_ABORT   abtInfo;

    LW_TCB_GET_CUR(ptcbCur);

    abtInfo.VMABT_uiMethod = 0;
    abtInfo.VMABT_uiType   = LW_VMM_ABORT_TYPE_TERMINAL;
    API_VmmAbortIsr(pregctx->REG_ulEpc, pregctx->REG_ulTrapVal, &abtInfo, ptcbCur);
}
/*********************************************************************************************************
** ��������: archInstAddrMisalignTrapHandle
** ��������: Instruction address misaligned
** �䡡��  : pregctx       �Ĵ���������
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static VOID  archInstAddrMisalignTrapHandle (ARCH_REG_CTX  *pregctx)
{
    PLW_CLASS_TCB  ptcbCur;
    LW_VMM_ABORT   abtInfo;

    LW_TCB_GET_CUR(ptcbCur);

    abtInfo.VMABT_uiMethod = BUS_ADRALN;
    abtInfo.VMABT_uiType   = LW_VMM_ABORT_TYPE_BUS;
    if (abtInfo.VMABT_uiType) {
        API_VmmAbortIsr(pregctx->REG_ulEpc, pregctx->REG_ulTrapVal, &abtInfo, ptcbCur);
    }
}
/*********************************************************************************************************
** ��������: archInstAccessTrapHandle
** ��������: Instruction access fault
** �䡡��  : pregctx       �Ĵ���������
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static VOID  archInstAccessTrapHandle (ARCH_REG_CTX  *pregctx)
{
    PLW_CLASS_TCB  ptcbCur;
    LW_VMM_ABORT   abtInfo;

    LW_TCB_GET_CUR(ptcbCur);

    abtInfo.VMABT_uiMethod = LW_VMM_ABORT_METHOD_EXEC;
    abtInfo.VMABT_uiType   = LW_VMM_ABORT_TYPE_TERMINAL;
    API_VmmAbortIsr(pregctx->REG_ulEpc, pregctx->REG_ulTrapVal, &abtInfo, ptcbCur);
}
/*********************************************************************************************************
** ��������: archIllegalInstTrapHandle
** ��������: Illegal instruction
** �䡡��  : pregctx       �Ĵ���������
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static VOID  archIllegalInstTrapHandle (ARCH_REG_CTX  *pregctx)
{
    PLW_CLASS_TCB   ptcbCur;
    LW_VMM_ABORT    abtInfo;

    LW_TCB_GET_CUR(ptcbCur);

#if LW_CFG_CPU_FPU_EN > 0
    if (archFpuUndHandle(ptcbCur) == ERROR_NONE) {                      /*  ���� FPU ָ��̽��           */
        return;
    }
#endif                                                                  /*  LW_CFG_CPU_FPU_EN > 0       */

    abtInfo.VMABT_uiType   = LW_VMM_ABORT_TYPE_UNDEF;
    abtInfo.VMABT_uiMethod = LW_VMM_ABORT_METHOD_EXEC;

    API_VmmAbortIsr(pregctx->REG_ulEpc, pregctx->REG_ulEpc, &abtInfo, ptcbCur);
}
/*********************************************************************************************************
** ��������: archBreakpointTrapHandle
** ��������: Breakpoint
** �䡡��  : pregctx       �Ĵ���������
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static VOID  archBreakpointTrapHandle (ARCH_REG_CTX  *pregctx)
{
    PLW_CLASS_TCB  ptcbCur;
    LW_VMM_ABORT   abtInfo;

#if LW_CFG_GDB_EN > 0
    addr_t         ulAddr = pregctx->REG_ulEpc;

    UINT    uiBpType = archDbgTrapType(ulAddr, LW_NULL);                /*  �ϵ�ָ��̽��                */
    if (uiBpType) {
        if (API_DtraceBreakTrap(ulAddr, uiBpType) == ERROR_NONE) {      /*  ������Խӿڶϵ㴦��        */
            return;
        }
    }
#endif                                                                  /*  LW_CFG_GDB_EN > 0           */

    LW_TCB_GET_CUR(ptcbCur);

    abtInfo.VMABT_uiMethod = LW_VMM_ABORT_METHOD_EXEC;
    abtInfo.VMABT_uiType   = LW_VMM_ABORT_TYPE_BREAK;
    API_VmmAbortIsr(pregctx->REG_ulEpc, pregctx->REG_ulEpc, &abtInfo, ptcbCur);
}
/*********************************************************************************************************
** ��������: archLoadAccessTrapHandle
** ��������: Load access fault
** �䡡��  : pregctx       �Ĵ���������
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static VOID  archLoadAccessTrapHandle (ARCH_REG_CTX  *pregctx)
{
    PLW_CLASS_TCB  ptcbCur;
    LW_VMM_ABORT   abtInfo;

    LW_TCB_GET_CUR(ptcbCur);

    abtInfo.VMABT_uiMethod = LW_VMM_ABORT_METHOD_READ;
    abtInfo.VMABT_uiType   = LW_VMM_ABORT_TYPE_TERMINAL;
    API_VmmAbortIsr(pregctx->REG_ulEpc, pregctx->REG_ulTrapVal, &abtInfo, ptcbCur);
}
/*********************************************************************************************************
** ��������: archLoadUnalignedHandle
** ��������: �Ƕ����ڴ�����쳣����
** �䡡��  : pabtctx    abort ������
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
#if LW_CFG_RISCV_M_LEVEL > 0

static VOID  archLoadUnalignedHandle (PLW_VMM_ABORT_CTX  pabtctx)
{
    riscvLoadUnalignedHandle(&pabtctx->ABTCTX_archRegCtx,
                             &pabtctx->ABTCTX_abtInfo);

    API_VmmAbortReturn(pabtctx);
}
/*********************************************************************************************************
** ��������: archStoreUnalignedHandle
** ��������: �Ƕ����ڴ洢���쳣����
** �䡡��  : pabtctx    abort ������
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static VOID  archStoreUnalignedHandle (PLW_VMM_ABORT_CTX  pabtctx)
{
    riscvStoreUnalignedHandle(&pabtctx->ABTCTX_archRegCtx,
                              &pabtctx->ABTCTX_abtInfo);

    API_VmmAbortReturn(pabtctx);
}

#endif                                                                  /*  LW_CFG_RISCV_M_LEVEL > 0    */
/*********************************************************************************************************
** ��������: archLoadAddrMisalignTrapHandle
** ��������: Load address misaligned
** �䡡��  : pregctx       �Ĵ���������
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
#if LW_CFG_RISCV_M_LEVEL > 0

static VOID  archLoadAddrMisalignTrapHandle (ARCH_REG_CTX  *pregctx)
{
    PLW_CLASS_TCB  ptcbCur;
    LW_VMM_ABORT   abtInfo;
    RISCV_PARAM   *param = archKernelParamGet();

    LW_TCB_GET_CUR(ptcbCur);

    abtInfo.VMABT_uiMethod = BUS_ADRALN;
    abtInfo.VMABT_uiType   = LW_VMM_ABORT_TYPE_BUS;

    if (param->RISCV_bUnalign) {
        API_VmmAbortIsrEx(pregctx->REG_ulEpc, pregctx->REG_ulTrapVal, &abtInfo, ptcbCur,
                          archLoadUnalignedHandle);

    } else {
        API_VmmAbortIsr(pregctx->REG_ulEpc, pregctx->REG_ulTrapVal, &abtInfo, ptcbCur);
    }
}

#endif                                                                  /*  LW_CFG_RISCV_M_LEVEL > 0    */
/*********************************************************************************************************
** ��������: archStoreAmoAddrMisalignTrapHandle
** ��������: Store/AMO address misaligned
** �䡡��  : pregctx       �Ĵ���������
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static VOID  archStoreAmoAddrMisalignTrapHandle (ARCH_REG_CTX  *pregctx)
{
    PLW_CLASS_TCB  ptcbCur;
    LW_VMM_ABORT   abtInfo;
#if LW_CFG_RISCV_M_LEVEL > 0
    RISCV_PARAM   *param = archKernelParamGet();
#endif                                                                  /*  LW_CFG_RISCV_M_LEVEL > 0    */

    LW_TCB_GET_CUR(ptcbCur);

    abtInfo.VMABT_uiMethod = BUS_ADRALN;
    abtInfo.VMABT_uiType   = LW_VMM_ABORT_TYPE_BUS;

#if LW_CFG_RISCV_M_LEVEL > 0
    if (param->RISCV_bUnalign) {
        API_VmmAbortIsrEx(pregctx->REG_ulEpc, pregctx->REG_ulTrapVal, &abtInfo, ptcbCur,
                          archStoreUnalignedHandle);

    } else
#endif                                                                  /*  LW_CFG_RISCV_M_LEVEL > 0    */
    {
        API_VmmAbortIsr(pregctx->REG_ulEpc, pregctx->REG_ulTrapVal, &abtInfo, ptcbCur);
    }
}
/*********************************************************************************************************
** ��������: archStoreAmoAccessTrapHandle
** ��������: Store/AMO access fault
** �䡡��  : pregctx       �Ĵ���������
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static VOID  archStoreAmoAccessTrapHandle (ARCH_REG_CTX  *pregctx)
{
    PLW_CLASS_TCB  ptcbCur;
    LW_VMM_ABORT   abtInfo;

    LW_TCB_GET_CUR(ptcbCur);

    abtInfo.VMABT_uiMethod = LW_VMM_ABORT_METHOD_WRITE;
    abtInfo.VMABT_uiType   = LW_VMM_ABORT_TYPE_TERMINAL;
    API_VmmAbortIsr(pregctx->REG_ulEpc, pregctx->REG_ulTrapVal, &abtInfo, ptcbCur);
}
/*********************************************************************************************************
** ��������: archEnvironmentCallHandle
** ��������: Environment call
** �䡡��  : pregctx       �Ĵ���������
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static VOID  archEnvironmentCallHandle (ARCH_REG_CTX  *pregctx)
{
    PLW_CLASS_TCB  ptcbCur;
    LW_VMM_ABORT   abtInfo;

    LW_TCB_GET_CUR(ptcbCur);

    abtInfo.VMABT_uiMethod = LW_VMM_ABORT_METHOD_EXEC;
    abtInfo.VMABT_uiType   = LW_VMM_ABORT_TYPE_SYS;
    API_VmmAbortIsr(pregctx->REG_ulEpc, pregctx->REG_ulTrapVal, &abtInfo, ptcbCur);
}
/*********************************************************************************************************
** ��������: archInstPageTrapHandle
** ��������: Instruction page fault
** �䡡��  : pregctx       �Ĵ���������
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static VOID  archInstPageTrapHandle (ARCH_REG_CTX  *pregctx)
{
    PLW_CLASS_TCB  ptcbCur;
    LW_VMM_ABORT   abtInfo;

    LW_TCB_GET_CUR(ptcbCur);

    abtInfo.VMABT_uiMethod = LW_VMM_ABORT_METHOD_EXEC;
    abtInfo.VMABT_uiType   = archMmuAbortType(pregctx->REG_ulTrapVal, LW_VMM_ABORT_METHOD_EXEC);
    API_VmmAbortIsr(pregctx->REG_ulEpc, pregctx->REG_ulTrapVal, &abtInfo, ptcbCur);
}
/*********************************************************************************************************
** ��������: archLoadPageTrapHandle
** ��������: Load page fault
** �䡡��  : pregctx       �Ĵ���������
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static VOID  archLoadPageTrapHandle (ARCH_REG_CTX  *pregctx)
{
    PLW_CLASS_TCB  ptcbCur;
    LW_VMM_ABORT   abtInfo;

    LW_TCB_GET_CUR(ptcbCur);

    abtInfo.VMABT_uiMethod = LW_VMM_ABORT_METHOD_READ;
    abtInfo.VMABT_uiType   = archMmuAbortType(pregctx->REG_ulTrapVal, LW_VMM_ABORT_METHOD_READ);
    API_VmmAbortIsr(pregctx->REG_ulEpc, pregctx->REG_ulTrapVal, &abtInfo, ptcbCur);
}
/*********************************************************************************************************
** ��������: archStoreAmoPageTrapHandle
** ��������: Store/AMO page fault
** �䡡��  : pregctx       �Ĵ���������
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static VOID  archStoreAmoPageTrapHandle (ARCH_REG_CTX  *pregctx)
{
    PLW_CLASS_TCB  ptcbCur;
    LW_VMM_ABORT   abtInfo;

    LW_TCB_GET_CUR(ptcbCur);

    abtInfo.VMABT_uiMethod = LW_VMM_ABORT_METHOD_WRITE;
    abtInfo.VMABT_uiType   = archMmuAbortType(pregctx->REG_ulTrapVal, LW_VMM_ABORT_METHOD_WRITE);
    API_VmmAbortIsr(pregctx->REG_ulEpc, pregctx->REG_ulTrapVal, &abtInfo, ptcbCur);
}
/*********************************************************************************************************
  Machine cause register (mcause) values after trap
  Interrupt Exception Code Description
  1         0              User software interrupt
  1         1              Supervisor software interrupt
  1         2              Reserved
  1         3              Machine software interrupt
  1         4              User timer interrupt
  1         5              Supervisor timer interrupt
  1         6              Reserved
  1         7              Machine timer interrupt
  1         8              User external interrupt
  1         9              Supervisor external interrupt
  1         10             Reserved
  1         11             Machine external interrupt
  1         >=12           Reserved

  0         0              Instruction address misaligned
  0         1              Instruction access fault
  0         2              Illegal instruction
  0         3              Breakpoint
  0         4              Load address misaligned
  0         5              Load access fault
  0         6              Store/AMO address misaligned
  0         7              Store/AMO access fault
  0         8              Environment call from U-mode
  0         9              Environment call from S-mode
  0         10             Reserved
  0         11             Environment call from M-mode
  0         12             Instruction page fault
  0         13             Load page fault
  0         14             Reserved
  0         15             Store/AMO page fault
  0         >=16           Reserved
*********************************************************************************************************/
/*********************************************************************************************************
  Supervisor cause register (scause) values after trap.
  Interrupt Exception Code Description
  1         0              User software interrupt
  1         1              Supervisor software interrupt
  1         2-3            Reserved
  1         4              User timer interrupt
  1         5              Supervisor timer interrupt
  1         6-7            Reserved
  1         8              User external interrupt
  1         9              Supervisor external interrupt
  1         >=10           Reserved

  0         0              Instruction address misaligned
  0         1              Instruction access fault
  0         2              Illegal instruction
  0         3              Breakpoint
  0         4              Reserved
  0         5              Load access fault
  0         6              AMO address misaligned
  0         7              Store/AMO access fault
  0         8              Environment call
  0         9-11           Reserved
  0         12             Instruction page fault
  0         13             Load page fault
  0         14             Reserved
  0         15             Store/AMO page fault
  0         >=16           Reserved
*********************************************************************************************************/
/*********************************************************************************************************
  RISC-V �쳣��������
*********************************************************************************************************/
typedef VOID  (*RISCV_TRAP_HANDLE)(ARCH_REG_CTX  *pregctx);

static RISCV_TRAP_HANDLE   _G_riscvTrapHandle[32] = {
    [0]  = (PVOID)archInstAddrMisalignTrapHandle,                   /*  Instruction address misaligned  */
    [1]  = (PVOID)archInstAccessTrapHandle,                         /*  Instruction access fault        */
    [2]  = (PVOID)archIllegalInstTrapHandle,                        /*  Illegal instruction             */
    [3]  = (PVOID)archBreakpointTrapHandle,                         /*  Breakpoint                      */

#if LW_CFG_RISCV_M_LEVEL > 0
    [4]  = (PVOID)archLoadAddrMisalignTrapHandle,                   /*  Load address misaligned         */
#endif                                                              /*  LW_CFG_RISCV_M_LEVEL > 0        */

    [5]  = (PVOID)archLoadAccessTrapHandle,                         /*  Load access fault               */
    [6]  = (PVOID)archStoreAmoAddrMisalignTrapHandle,               /*  Store/AMO address misaligned    */
    [7]  = (PVOID)archStoreAmoAccessTrapHandle,                     /*  Store/AMO access fault          */
    [8]  = (PVOID)archEnvironmentCallHandle,                        /*  Environment call from U-mode    */

#if LW_CFG_RISCV_M_LEVEL > 0
    [9]  = (PVOID)archEnvironmentCallHandle,                        /*  Environment call from S-mode    */
    [11] = (PVOID)archEnvironmentCallHandle,                        /*  Environment call from M-mode    */
#endif                                                              /*  LW_CFG_RISCV_M_LEVEL > 0        */

    [12] = (PVOID)archInstPageTrapHandle,                           /*  Instruction page fault          */
    [13] = (PVOID)archLoadPageTrapHandle,                           /*  Load page fault                 */
    [15] = (PVOID)archStoreAmoPageTrapHandle,                       /*  Store/AMO page fault            */
};
/*********************************************************************************************************
** ��������: archTrapHandle
** ��������: �쳣�������
** �䡡��  : pregctx       �Ĵ���������
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
VOID  archTrapHandle (ARCH_REG_CTX  *pregctx)
{
    if (pregctx->REG_ulCause & (1ULL << (LW_CFG_CPU_WORD_LENGHT - 1))) {
        bspIntHandle();

    } else {
        RISCV_TRAP_HANDLE  pfuncHandle = _G_riscvTrapHandle[pregctx->REG_ulCause];

        if (pfuncHandle) {
            pfuncHandle(pregctx);

        } else {
            archDefaultTrapHandle(pregctx);
        }
    }
}
/*********************************************************************************************************
  END
*********************************************************************************************************/
