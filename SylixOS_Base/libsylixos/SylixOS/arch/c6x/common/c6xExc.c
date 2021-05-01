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
** ��   ��   ��: c6xExc.c
**
** ��   ��   ��: Jiao.JinXing (������)
**
** �ļ���������: 2017 �� 03 �� 17 ��
**
** ��        ��: c6x ��ϵ�����쳣/�жϴ���.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "SylixOS.h"
#include "dtrace.h"
#include <linux/compat.h>
/*********************************************************************************************************
  �ⲿ���ñ�������
*********************************************************************************************************/
extern LW_CLASS_CPU _K_cpuTable[];                                      /*  CPU ��                      */
extern LW_STACK     _K_stkInterruptStack[LW_CFG_MAX_PROCESSORS][LW_CFG_INT_STK_SIZE / sizeof(LW_STACK)];
/*********************************************************************************************************
  �ж���ر�������
*********************************************************************************************************/
addr_t  _G_ulIntSafeStack;
addr_t  _G_ulIntNesting;
addr_t  _G_ulCpu;
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
  �쳣��ؼĴ���
*********************************************************************************************************/
extern volatile cregister UINT32  EFR;
extern volatile cregister UINT32  ECR;
extern volatile cregister UINT32  IERR;
/*********************************************************************************************************
  �쳣��ز�����
*********************************************************************************************************/
#define disable_exception()
#define get_except_type()       EFR
#define ack_exception(type)     ECR = 1ul << (type)
#define get_iexcept()           IERR
#define set_iexcept(mask)       IERR = (mask)
/*********************************************************************************************************
  �쳣����
*********************************************************************************************************/
#define EXCEPT_TYPE_NXF         31                                      /*  NMI                         */
#define EXCEPT_TYPE_EXC         30                                      /*  external exception          */
#define EXCEPT_TYPE_IXF         1                                       /*  internal exception          */
#define EXCEPT_TYPE_SXF         0                                       /*  software exception          */

#define EXCEPT_CAUSE_LBX        (1 << 7)                                /*  loop buffer exception       */
#define EXCEPT_CAUSE_PRX        (1 << 6)                                /*  privilege exception         */
#define EXCEPT_CAUSE_RAX        (1 << 5)                                /*  resource access exception   */
#define EXCEPT_CAUSE_RCX        (1 << 4)                                /*  resource conflict exception */
#define EXCEPT_CAUSE_OPX        (1 << 3)                                /*  opcode exception            */
#define EXCEPT_CAUSE_EPX        (1 << 2)                                /*  execute packet exception    */
#define EXCEPT_CAUSE_FPX        (1 << 1)                                /*  fetch packet exception      */
#define EXCEPT_CAUSE_IFX        (1 << 0)                                /*  instruction fetch exception */
/*********************************************************************************************************
  �쳣��Ϣ
*********************************************************************************************************/
typedef struct {
    CPCHAR      EXCI_pcName;                                            /*  ����                        */
    INT         EXCI_iType;                                             /*  ����                        */
    INT         EXCI_iMethod;                                           /*  ����                        */
} ARCH_C6X_EXC_INFO;
/*********************************************************************************************************
  �ڲ��쳣��Ϣ
*********************************************************************************************************/
static ARCH_C6X_EXC_INFO    _G_c6xIntExcTbl[11] = {
    { "instruction fetch",     LW_VMM_ABORT_TYPE_BUS,   LW_VMM_ABORT_METHOD_READ },
    { "fetch packet",          LW_VMM_ABORT_TYPE_BUS,   LW_VMM_ABORT_METHOD_READ },
    { "execute packet",        LW_VMM_ABORT_TYPE_UNDEF, LW_VMM_ABORT_METHOD_EXEC },
    { "undefined instruction", LW_VMM_ABORT_TYPE_UNDEF, LW_VMM_ABORT_METHOD_EXEC },
    { "resource conflict",     LW_VMM_ABORT_TYPE_UNDEF, LW_VMM_ABORT_METHOD_EXEC },
    { "resource access",       LW_VMM_ABORT_TYPE_UNDEF, LW_VMM_ABORT_METHOD_EXEC },
    { "privilege",             LW_VMM_ABORT_TYPE_UNDEF, LW_VMM_ABORT_METHOD_EXEC },
    { "loops buffer",          LW_VMM_ABORT_TYPE_UNDEF, LW_VMM_ABORT_METHOD_EXEC },
    { "software exception",    LW_VMM_ABORT_TYPE_UNDEF, LW_VMM_ABORT_METHOD_EXEC },
    { "unknown exception",     LW_VMM_ABORT_TYPE_UNDEF, LW_VMM_ABORT_METHOD_EXEC },
    { "fatal error",           LW_VMM_ABORT_TYPE_FATAL_ERROR, LW_VMM_ABORT_METHOD_EXEC }
};
/*********************************************************************************************************
  �ⲿ�쳣��Ϣ
*********************************************************************************************************/
static ARCH_C6X_EXC_INFO    _G_c6xExtExcTbl[128] = {
    { "external exception", LW_VMM_ABORT_TYPE_BUS, LW_VMM_ABORT_METHOD_READ },
    { "external exception", LW_VMM_ABORT_TYPE_BUS, LW_VMM_ABORT_METHOD_READ },
    { "external exception", LW_VMM_ABORT_TYPE_BUS, LW_VMM_ABORT_METHOD_READ },
    { "external exception", LW_VMM_ABORT_TYPE_BUS, LW_VMM_ABORT_METHOD_READ },
    { "external exception", LW_VMM_ABORT_TYPE_BUS, LW_VMM_ABORT_METHOD_READ },
    { "external exception", LW_VMM_ABORT_TYPE_BUS, LW_VMM_ABORT_METHOD_READ },
    { "external exception", LW_VMM_ABORT_TYPE_BUS, LW_VMM_ABORT_METHOD_READ },
    { "external exception", LW_VMM_ABORT_TYPE_BUS, LW_VMM_ABORT_METHOD_READ },
    { "external exception", LW_VMM_ABORT_TYPE_BUS, LW_VMM_ABORT_METHOD_READ },
    { "external exception", LW_VMM_ABORT_TYPE_BUS, LW_VMM_ABORT_METHOD_READ },
    { "external exception", LW_VMM_ABORT_TYPE_BUS, LW_VMM_ABORT_METHOD_READ },
    { "external exception", LW_VMM_ABORT_TYPE_BUS, LW_VMM_ABORT_METHOD_READ },
    { "external exception", LW_VMM_ABORT_TYPE_BUS, LW_VMM_ABORT_METHOD_READ },
    { "external exception", LW_VMM_ABORT_TYPE_BUS, LW_VMM_ABORT_METHOD_READ },
    { "external exception", LW_VMM_ABORT_TYPE_BUS, LW_VMM_ABORT_METHOD_READ },
    { "external exception", LW_VMM_ABORT_TYPE_BUS, LW_VMM_ABORT_METHOD_READ },
    { "external exception", LW_VMM_ABORT_TYPE_BUS, LW_VMM_ABORT_METHOD_READ },
    { "external exception", LW_VMM_ABORT_TYPE_BUS, LW_VMM_ABORT_METHOD_READ },
    { "external exception", LW_VMM_ABORT_TYPE_BUS, LW_VMM_ABORT_METHOD_READ },
    { "external exception", LW_VMM_ABORT_TYPE_BUS, LW_VMM_ABORT_METHOD_READ },
    { "external exception", LW_VMM_ABORT_TYPE_BUS, LW_VMM_ABORT_METHOD_READ },
    { "external exception", LW_VMM_ABORT_TYPE_BUS, LW_VMM_ABORT_METHOD_READ },
    { "external exception", LW_VMM_ABORT_TYPE_BUS, LW_VMM_ABORT_METHOD_READ },
    { "external exception", LW_VMM_ABORT_TYPE_BUS, LW_VMM_ABORT_METHOD_READ },
    { "external exception", LW_VMM_ABORT_TYPE_BUS, LW_VMM_ABORT_METHOD_READ },
    { "external exception", LW_VMM_ABORT_TYPE_BUS, LW_VMM_ABORT_METHOD_READ },
    { "external exception", LW_VMM_ABORT_TYPE_BUS, LW_VMM_ABORT_METHOD_READ },
    { "external exception", LW_VMM_ABORT_TYPE_BUS, LW_VMM_ABORT_METHOD_READ },
    { "external exception", LW_VMM_ABORT_TYPE_BUS, LW_VMM_ABORT_METHOD_READ },
    { "external exception", LW_VMM_ABORT_TYPE_BUS, LW_VMM_ABORT_METHOD_READ },
    { "external exception", LW_VMM_ABORT_TYPE_BUS, LW_VMM_ABORT_METHOD_READ },
    { "external exception", LW_VMM_ABORT_TYPE_BUS, LW_VMM_ABORT_METHOD_READ },

    { "external exception", LW_VMM_ABORT_TYPE_BUS, LW_VMM_ABORT_METHOD_READ },
    { "external exception", LW_VMM_ABORT_TYPE_BUS, LW_VMM_ABORT_METHOD_READ },
    { "external exception", LW_VMM_ABORT_TYPE_BUS, LW_VMM_ABORT_METHOD_READ },
    { "external exception", LW_VMM_ABORT_TYPE_BUS, LW_VMM_ABORT_METHOD_READ },
    { "external exception", LW_VMM_ABORT_TYPE_BUS, LW_VMM_ABORT_METHOD_READ },
    { "external exception", LW_VMM_ABORT_TYPE_BUS, LW_VMM_ABORT_METHOD_READ },
    { "external exception", LW_VMM_ABORT_TYPE_BUS, LW_VMM_ABORT_METHOD_READ },
    { "external exception", LW_VMM_ABORT_TYPE_BUS, LW_VMM_ABORT_METHOD_READ },
    { "external exception", LW_VMM_ABORT_TYPE_BUS, LW_VMM_ABORT_METHOD_READ },
    { "external exception", LW_VMM_ABORT_TYPE_BUS, LW_VMM_ABORT_METHOD_READ },
    { "external exception", LW_VMM_ABORT_TYPE_BUS, LW_VMM_ABORT_METHOD_READ },
    { "external exception", LW_VMM_ABORT_TYPE_BUS, LW_VMM_ABORT_METHOD_READ },
    { "external exception", LW_VMM_ABORT_TYPE_BUS, LW_VMM_ABORT_METHOD_READ },
    { "external exception", LW_VMM_ABORT_TYPE_BUS, LW_VMM_ABORT_METHOD_READ },
    { "external exception", LW_VMM_ABORT_TYPE_BUS, LW_VMM_ABORT_METHOD_READ },
    { "external exception", LW_VMM_ABORT_TYPE_BUS, LW_VMM_ABORT_METHOD_READ },
    { "external exception", LW_VMM_ABORT_TYPE_BUS, LW_VMM_ABORT_METHOD_READ },
    { "external exception", LW_VMM_ABORT_TYPE_BUS, LW_VMM_ABORT_METHOD_READ },
    { "external exception", LW_VMM_ABORT_TYPE_BUS, LW_VMM_ABORT_METHOD_READ },
    { "external exception", LW_VMM_ABORT_TYPE_BUS, LW_VMM_ABORT_METHOD_READ },
    { "external exception", LW_VMM_ABORT_TYPE_BUS, LW_VMM_ABORT_METHOD_READ },
    { "external exception", LW_VMM_ABORT_TYPE_BUS, LW_VMM_ABORT_METHOD_READ },
    { "external exception", LW_VMM_ABORT_TYPE_BUS, LW_VMM_ABORT_METHOD_READ },
    { "external exception", LW_VMM_ABORT_TYPE_BUS, LW_VMM_ABORT_METHOD_READ },
    { "external exception", LW_VMM_ABORT_TYPE_BUS, LW_VMM_ABORT_METHOD_READ },
    { "external exception", LW_VMM_ABORT_TYPE_BUS, LW_VMM_ABORT_METHOD_READ },
    { "external exception", LW_VMM_ABORT_TYPE_BUS, LW_VMM_ABORT_METHOD_READ },
    { "external exception", LW_VMM_ABORT_TYPE_BUS, LW_VMM_ABORT_METHOD_READ },
    { "external exception", LW_VMM_ABORT_TYPE_BUS, LW_VMM_ABORT_METHOD_READ },
    { "external exception", LW_VMM_ABORT_TYPE_BUS, LW_VMM_ABORT_METHOD_READ },
    { "external exception", LW_VMM_ABORT_TYPE_BUS, LW_VMM_ABORT_METHOD_READ },
    { "external exception", LW_VMM_ABORT_TYPE_BUS, LW_VMM_ABORT_METHOD_READ },

    { "external exception", LW_VMM_ABORT_TYPE_BUS, LW_VMM_ABORT_METHOD_READ },
    { "external exception", LW_VMM_ABORT_TYPE_BUS, LW_VMM_ABORT_METHOD_READ },
    { "external exception", LW_VMM_ABORT_TYPE_BUS, LW_VMM_ABORT_METHOD_READ },
    { "external exception", LW_VMM_ABORT_TYPE_BUS, LW_VMM_ABORT_METHOD_READ },
    { "external exception", LW_VMM_ABORT_TYPE_BUS, LW_VMM_ABORT_METHOD_READ },
    { "external exception", LW_VMM_ABORT_TYPE_BUS, LW_VMM_ABORT_METHOD_READ },
    { "external exception", LW_VMM_ABORT_TYPE_BUS, LW_VMM_ABORT_METHOD_READ },
    { "external exception", LW_VMM_ABORT_TYPE_BUS, LW_VMM_ABORT_METHOD_READ },
    { "external exception", LW_VMM_ABORT_TYPE_BUS, LW_VMM_ABORT_METHOD_READ },
    { "external exception", LW_VMM_ABORT_TYPE_BUS, LW_VMM_ABORT_METHOD_READ },
    { "external exception", LW_VMM_ABORT_TYPE_BUS, LW_VMM_ABORT_METHOD_READ },
    { "external exception", LW_VMM_ABORT_TYPE_BUS, LW_VMM_ABORT_METHOD_READ },
    { "external exception", LW_VMM_ABORT_TYPE_BUS, LW_VMM_ABORT_METHOD_READ },
    { "external exception", LW_VMM_ABORT_TYPE_BUS, LW_VMM_ABORT_METHOD_READ },
    { "external exception", LW_VMM_ABORT_TYPE_BUS, LW_VMM_ABORT_METHOD_READ },
    { "external exception", LW_VMM_ABORT_TYPE_BUS, LW_VMM_ABORT_METHOD_READ },
    { "external exception", LW_VMM_ABORT_TYPE_BUS, LW_VMM_ABORT_METHOD_READ },
    { "external exception", LW_VMM_ABORT_TYPE_BUS, LW_VMM_ABORT_METHOD_READ },
    { "external exception", LW_VMM_ABORT_TYPE_BUS, LW_VMM_ABORT_METHOD_READ },
    { "external exception", LW_VMM_ABORT_TYPE_BUS, LW_VMM_ABORT_METHOD_READ },
    { "external exception", LW_VMM_ABORT_TYPE_BUS, LW_VMM_ABORT_METHOD_READ },
    { "external exception", LW_VMM_ABORT_TYPE_BUS, LW_VMM_ABORT_METHOD_READ },
    { "external exception", LW_VMM_ABORT_TYPE_BUS, LW_VMM_ABORT_METHOD_READ },
    { "external exception", LW_VMM_ABORT_TYPE_BUS, LW_VMM_ABORT_METHOD_READ },
    { "external exception", LW_VMM_ABORT_TYPE_BUS, LW_VMM_ABORT_METHOD_READ },
    { "external exception", LW_VMM_ABORT_TYPE_BUS, LW_VMM_ABORT_METHOD_READ },
    { "external exception", LW_VMM_ABORT_TYPE_BUS, LW_VMM_ABORT_METHOD_READ },
    { "external exception", LW_VMM_ABORT_TYPE_BUS, LW_VMM_ABORT_METHOD_READ },
    { "external exception", LW_VMM_ABORT_TYPE_BUS, LW_VMM_ABORT_METHOD_READ },
    { "external exception", LW_VMM_ABORT_TYPE_BUS, LW_VMM_ABORT_METHOD_READ },
    { "external exception", LW_VMM_ABORT_TYPE_BUS, LW_VMM_ABORT_METHOD_READ },
    { "external exception", LW_VMM_ABORT_TYPE_BUS, LW_VMM_ABORT_METHOD_READ },

    { "external exception", LW_VMM_ABORT_TYPE_BUS, LW_VMM_ABORT_METHOD_READ },
    { "external exception", LW_VMM_ABORT_TYPE_BUS, LW_VMM_ABORT_METHOD_READ },
    { "external exception", LW_VMM_ABORT_TYPE_BUS, LW_VMM_ABORT_METHOD_READ },
    { "external exception", LW_VMM_ABORT_TYPE_BUS, LW_VMM_ABORT_METHOD_READ },
    { "external exception", LW_VMM_ABORT_TYPE_BUS, LW_VMM_ABORT_METHOD_READ },
    { "external exception", LW_VMM_ABORT_TYPE_BUS, LW_VMM_ABORT_METHOD_READ },
    { "external exception", LW_VMM_ABORT_TYPE_BUS, LW_VMM_ABORT_METHOD_READ },
    { "external exception", LW_VMM_ABORT_TYPE_BUS, LW_VMM_ABORT_METHOD_READ },
    { "external exception", LW_VMM_ABORT_TYPE_BUS, LW_VMM_ABORT_METHOD_READ },
    { "external exception", LW_VMM_ABORT_TYPE_BUS, LW_VMM_ABORT_METHOD_READ },
    { "external exception", LW_VMM_ABORT_TYPE_BUS, LW_VMM_ABORT_METHOD_READ },
    { "external exception", LW_VMM_ABORT_TYPE_BUS, LW_VMM_ABORT_METHOD_READ },
    { "external exception", LW_VMM_ABORT_TYPE_BUS, LW_VMM_ABORT_METHOD_READ },
    { "external exception", LW_VMM_ABORT_TYPE_BUS, LW_VMM_ABORT_METHOD_READ },
    { "external exception", LW_VMM_ABORT_TYPE_BUS, LW_VMM_ABORT_METHOD_READ },
    { "external exception", LW_VMM_ABORT_TYPE_BUS, LW_VMM_ABORT_METHOD_READ },
    { "external exception", LW_VMM_ABORT_TYPE_BUS, LW_VMM_ABORT_METHOD_READ },
    { "external exception", LW_VMM_ABORT_TYPE_BUS, LW_VMM_ABORT_METHOD_READ },
    { "external exception", LW_VMM_ABORT_TYPE_BUS, LW_VMM_ABORT_METHOD_READ },
    { "external exception", LW_VMM_ABORT_TYPE_BUS, LW_VMM_ABORT_METHOD_READ },
    { "external exception", LW_VMM_ABORT_TYPE_BUS, LW_VMM_ABORT_METHOD_READ },
    { "external exception", LW_VMM_ABORT_TYPE_BUS, LW_VMM_ABORT_METHOD_READ },
    { "external exception", LW_VMM_ABORT_TYPE_BUS, LW_VMM_ABORT_METHOD_READ },

    { "CPU memory protection fault",        LW_VMM_ABORT_TYPE_MAP, LW_VMM_ABORT_METHOD_READ },
    { "CPU memory protection fault in L1P", LW_VMM_ABORT_TYPE_MAP, LW_VMM_ABORT_METHOD_READ },
    { "DMA memory protection fault in L1P", LW_VMM_ABORT_TYPE_MAP, LW_VMM_ABORT_METHOD_READ },
    { "CPU memory protection fault in L1D", LW_VMM_ABORT_TYPE_MAP, LW_VMM_ABORT_METHOD_READ },
    { "DMA memory protection fault in L1D", LW_VMM_ABORT_TYPE_MAP, LW_VMM_ABORT_METHOD_READ },
    { "CPU memory protection fault in L2",  LW_VMM_ABORT_TYPE_MAP, LW_VMM_ABORT_METHOD_READ },
    { "DMA memory protection fault in L2",  LW_VMM_ABORT_TYPE_MAP, LW_VMM_ABORT_METHOD_READ },
    { "EMC CPU memory protection fault",    LW_VMM_ABORT_TYPE_MAP, LW_VMM_ABORT_METHOD_READ },

    { "EMC bus error",      LW_VMM_ABORT_TYPE_BUS, LW_VMM_ABORT_METHOD_READ }
};
/*********************************************************************************************************
** ��������: archExcInit
** ��������: ��ʼ���쳣
** �䡡��  : NONE
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
VOID  archExcInit (VOID)
{
    _G_ulCpu          = (addr_t)&_K_cpuTable[0];
    _G_ulIntNesting   = (addr_t)&_K_cpuTable[0].CPU_ulInterNesting;

#if CPU_STK_GROWTH == 0
    _G_ulIntSafeStack = (addr_t)&_K_stkInterruptStack[0][0];
    _G_ulIntSafeStack = ROUND_UP(_G_ulIntSafeStack, ARCH_STK_ALIGN_SIZE);
#else
    _G_ulIntSafeStack = (addr_t)&_K_stkInterruptStack[0][(LW_CFG_INT_STK_SIZE / sizeof(LW_STACK)) - 1];
    _G_ulIntSafeStack = ROUND_DOWN(_G_ulIntSafeStack, ARCH_STK_ALIGN_SIZE);
#endif                                                                  /*  CPU_STK_GROWTH              */

    ack_exception(EXCEPT_TYPE_NXF);                                     /*  ����������쳣              */
    ack_exception(EXCEPT_TYPE_EXC);
    ack_exception(EXCEPT_TYPE_IXF);
    ack_exception(EXCEPT_TYPE_SXF);

    archExcEnable();                                                    /*  ʹ���쳣                    */
}
/*********************************************************************************************************
** ��������: archExcProcess
** ��������: �쳣����
** �䡡��  : pregctx       �Ĵ���������
**           pExcInfo      �쳣��Ϣ
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static VOID  archExcProcess (ARCH_REG_CTX  *pregctx, ARCH_C6X_EXC_INFO  *pExcInfo)
{
    PLW_CLASS_TCB      ptcbCur;
    LW_VMM_ABORT       abtInfo;

    LW_TCB_GET_CUR(ptcbCur);

    abtInfo.VMABT_uiType   = pExcInfo->EXCI_iType;
    abtInfo.VMABT_uiMethod = pExcInfo->EXCI_iMethod;
    if (abtInfo.VMABT_uiType) {
        API_VmmAbortIsr(pregctx->REG_uiIrp,
                        ((abtInfo.VMABT_uiType == LW_VMM_ABORT_TYPE_UNDEF) ? pregctx->REG_uiIrp : 0),
                        &abtInfo, ptcbCur);
    }
}
/*********************************************************************************************************
** ��������: archIntExcProcess
** ��������: ����һ���ڲ��쳣 (���ܱ�����)
** �䡡��  : pregctx       �Ĵ���������
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static VOID  archIntExcProcess (ARCH_REG_CTX  *pregctx)
{
#if LW_CFG_GDB_EN > 0
#define C6X_BREAKPOINT_INS  0x56454314

    addr_t             ulRetAddr;
#endif                                                                  /*  LW_CFG_GDB_EN > 0           */

    INT  iIntExcReport = get_iexcept();
    INT  iIntExcNum;

    ack_exception(EXCEPT_TYPE_IXF);                                     /*  ����쳣                    */

    if (iIntExcReport) {                                                /*  TODO: Ŀǰֻ�ܴ���һ���쳣  */
        iIntExcNum     = __ffs(iIntExcReport);
        iIntExcReport &= ~(1 << iIntExcNum);
        set_iexcept(iIntExcReport);

#if LW_CFG_GDB_EN > 0
        ulRetAddr = pregctx->REG_uiIrp;
        if (*(ULONG *)ulRetAddr == C6X_BREAKPOINT_INS) {
            UINT    uiBpType = archDbgTrapType(pregctx->REG_uiIrp,
                                               (PVOID)LW_NULL);         /*  �ϵ�ָ��̽��                */
            if (uiBpType) {
                if (API_DtraceBreakTrap(pregctx->REG_uiIrp,
                                        uiBpType) == ERROR_NONE) {      /*  ������Խӿڶϵ㴦��        */
                    return;
                }
            }
        }
#endif                                                                  /*  LW_CFG_GDB_EN > 0           */

        archExcProcess(pregctx, &_G_c6xIntExcTbl[iIntExcNum]);
    }
}
/*********************************************************************************************************
** ��������: archExtExcProcess
** ��������: ����һ���ⲿ�쳣 (�ܱ�����)
** �䡡��  : pregctx       �Ĵ���������
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static VOID  archExtExcProcess (ARCH_REG_CTX  *pregctx)
{
    INT  iExtExcNum;

    if ((iExtExcNum = bspExtExcGet()) >= 0) {                           /*  BSP ��ѯ�쳣��              */
        archExcProcess(pregctx, &_G_c6xExtExcTbl[iExtExcNum]);          /*  TODO: Ŀǰֻ�ܴ���һ���쳣  */
    }

    ack_exception(EXCEPT_TYPE_EXC);                                     /*  ����쳣                    */
}
/*********************************************************************************************************
** ��������: archExcHandle
** ��������: �쳣����
** �䡡��  : pregctx       �Ĵ���������
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
VOID  archExcHandle (ARCH_REG_CTX  *pregctx)
{
    UINT  uiExcType;
    UINT  uiTypeNum;
    UINT  uiIntExcNum = 9;                                              /*  ȱʡΪδ֪�쳣              */

    if ((uiExcType = get_except_type()) != 0) {                         /*  �쳣����                    */
                                                                        /*  TODO: Ŀǰֻ�ܴ���һ���쳣  */
        uiTypeNum = fls(uiExcType) - 1;                                 /*  ����쳣���ͺ�              */

        switch (uiTypeNum) {

        case EXCEPT_TYPE_NXF:                                           /*  NMI �쳣                    */
            ack_exception(EXCEPT_TYPE_NXF);                             /*  ����쳣                    */
            if (bspNmiExcHandler() < 0) {                               /*  ���� BSP ����               */
                uiIntExcNum = 10;                                       /*  �����ɹ�: fatal error     */
                archExcProcess(pregctx, &_G_c6xIntExcTbl[uiIntExcNum]);
            }
            break;

        case EXCEPT_TYPE_IXF:                                           /*  �ڲ��쳣                    */
            archIntExcProcess(pregctx);
            break;

        case EXCEPT_TYPE_EXC:                                           /*  �ⲿ�쳣                    */
            archExtExcProcess(pregctx);
            break;

        case EXCEPT_TYPE_SXF:
            uiIntExcNum = 8;                                            /*  ����쳣                    */

        default:
            ack_exception(uiTypeNum);                                   /*  ����쳣                    */
            archExcProcess(pregctx, &_G_c6xIntExcTbl[uiIntExcNum]);
            break;
        }
    }

    /*
     * TODO: ����Ŀǰֻ�ܴ���һ���쳣, ��������Ҫ����������쳣
     */
    ack_exception(EXCEPT_TYPE_NXF);                                     /*  ����������쳣              */
    ack_exception(EXCEPT_TYPE_EXC);
    ack_exception(EXCEPT_TYPE_IXF);
    ack_exception(EXCEPT_TYPE_SXF);
    set_iexcept(0);
}
/*********************************************************************************************************
  END
*********************************************************************************************************/
