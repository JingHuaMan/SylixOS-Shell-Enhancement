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
** ��   ��   ��: cskyExc.c
**
** ��   ��   ��: Hui.Kai (�ݿ�)
**
** �ļ���������: 2018 �� 05 �� 11 ��
**
** ��        ��: C-SKY ��ϵ�ܹ��쳣����.
*********************************************************************************************************/
#define  __SYLIXOS_STDIO
#define  __SYLIXOS_KERNEL
#include "SylixOS.h"
#include "dtrace.h"
#include "arch/csky/inc/cskyregs.h"
#include "arch/csky/param/cskyParam.h"
#include "arch/csky/common/unaligned/cskyUnaligned.h"
#if LW_CFG_VMM_EN > 0
#include "arch/csky/mm/mmu/cskyMmu.h"
#endif
#if LW_CFG_CPU_FPU_EN > 0
#include "arch/csky/fpu/fpu/cskyVfp.h"
#endif
/*********************************************************************************************************
  C-SKY ��ϵ�ܹ�
*********************************************************************************************************/
#if !defined(__SYLIXOS_CSKY_ARCH_CK803__)
/*********************************************************************************************************
  ����ʹ���������
*********************************************************************************************************/
#if LW_CFG_SMP_EN > 0
#define VECTOR_OP_LOCK()    LW_SPIN_LOCK_IGNIRQ(&_K_slcaVectorTable.SLCA_sl)
#define VECTOR_OP_UNLOCK()  LW_SPIN_UNLOCK_IGNIRQ(&_K_slcaVectorTable.SLCA_sl)
#else
#define VECTOR_OP_LOCK()
#define VECTOR_OP_UNLOCK()
#endif
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
** ��������: archFastAutoIntHandle
** ��������: �����ж�
** �䡡��  : ulVector  �ж�����
**           pregctx   ������
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
LW_WEAK VOID  archFastAutoIntHandle (ULONG  ulVector, ARCH_REG_CTX  *pregctx)
{
    bspIntHandle(ulVector);
}
/*********************************************************************************************************
** ��������: archAutoIntHandle
** ��������: ��ͨ�ж�
** �䡡��  : ulVector  �ж�����
**           pregctx   ������
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
VOID  archAutoIntHandle (ULONG  ulVector, ARCH_REG_CTX  *pregctx)
{
    bspIntHandle(ulVector);
}
/*********************************************************************************************************
** ��������: archTlbLoadExceptHandle
** ��������: TLB load or ifetch �쳣����
** �䡡��  : ulVector  �ж�����
**           pregctx   ������
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
VOID  archTlbLoadExceptHandle (ULONG  ulVector, ARCH_REG_CTX  *pregctx)
{
    PLW_CLASS_TCB  ptcbCur;
    LW_VMM_ABORT   abtInfo;

    LW_TCB_GET_CUR(ptcbCur);

    abtInfo.VMABT_uiMethod = LW_VMM_ABORT_METHOD_READ;
#if LW_CFG_VMM_EN > 0
    abtInfo.VMABT_uiType   = cskyMmuTlbLdStExceptHandle();
#else
    abtInfo.VMABT_uiType   = LW_VMM_ABORT_TYPE_TERMINAL;
#endif                                                                  /*  LW_CFG_VMM_EN > 0           */

    if (abtInfo.VMABT_uiType) {
        API_VmmAbortIsr(pregctx->REG_ulPc, pregctx->REG_ulMeh, &abtInfo, ptcbCur);
    }
}
/*********************************************************************************************************
** ��������: archTlbStoreExceptHandle
** ��������: TLB store �쳣����
** �䡡��  : ulVector  �ж�����
**           pregctx   ������
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
VOID  archTlbStoreExceptHandle (ULONG  ulVector, ARCH_REG_CTX  *pregctx)
{
    PLW_CLASS_TCB  ptcbCur;
    LW_VMM_ABORT   abtInfo;

    LW_TCB_GET_CUR(ptcbCur);

    abtInfo.VMABT_uiMethod = LW_VMM_ABORT_METHOD_WRITE;
#if LW_CFG_VMM_EN > 0
    abtInfo.VMABT_uiType   = cskyMmuTlbLdStExceptHandle();
#else
    abtInfo.VMABT_uiType   = LW_VMM_ABORT_TYPE_TERMINAL;
#endif                                                                  /*  LW_CFG_VMM_EN > 0           */

    if (abtInfo.VMABT_uiType) {
        API_VmmAbortIsr(pregctx->REG_ulPc, pregctx->REG_ulMeh, &abtInfo, ptcbCur);
    }
}
/*********************************************************************************************************
** ��������: archTlbModExceptHandle
** ��������: TLB modified �쳣����
** �䡡��  : ulRetAddr     ���ص�ַ
**           ulAbortAddr   ��ֹ��ַ
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
VOID  archTlbModExceptHandle (ULONG  ulVector, ARCH_REG_CTX  *pregctx)
{
    PLW_CLASS_TCB  ptcbCur;
    LW_VMM_ABORT   abtInfo;

    LW_TCB_GET_CUR(ptcbCur);

    abtInfo.VMABT_uiMethod = LW_VMM_ABORT_METHOD_WRITE;
    abtInfo.VMABT_uiType   = LW_VMM_ABORT_TYPE_PERM;
    API_VmmAbortIsr(pregctx->REG_ulPc, pregctx->REG_ulMeh, &abtInfo, ptcbCur);
}
/*********************************************************************************************************
** ��������: archTrapExceptHandle
** ��������: �����쳣
** �䡡��  : ulVector  �ж�����
**           pregctx   ������
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
VOID  archTrapExceptHandle (ULONG  ulVector, ARCH_REG_CTX  *pregctx)
{
    PLW_CLASS_TCB  ptcbCur;
    LW_VMM_ABORT   abtInfo;

    LW_TCB_GET_CUR(ptcbCur);

    abtInfo.VMABT_uiMethod = LW_VMM_ABORT_METHOD_EXEC;
    abtInfo.VMABT_uiType   = LW_VMM_ABORT_TYPE_SYS;
    API_VmmAbortIsr(pregctx->REG_ulPc, pregctx->REG_ulMeh, &abtInfo, ptcbCur);
}
/*********************************************************************************************************
** ��������: archTrap0ExceptHandle
** ��������: ����ָ���쳣 0
** �䡡��  : ulVector  �ж�����
**           pregctx   ������
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
VOID  archTrap0ExceptHandle (ULONG  ulVector, ARCH_REG_CTX  *pregctx)
{
    archTrapExceptHandle(ulVector, pregctx);
}
/*********************************************************************************************************
** ��������: archTrap1ExceptHandle
** ��������: ����ָ���쳣 1
** �䡡��  : ulVector  �ж�����
**           pregctx   ������
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
VOID  archTrap1ExceptHandle (ULONG  ulVector, ARCH_REG_CTX  *pregctx)
{
    archTrapExceptHandle(ulVector, pregctx);
}
/*********************************************************************************************************
** ��������: archTrap2ExceptHandle
** ��������: ����ָ���쳣 2
** �䡡��  : ulVector  �ж�����
**           pregctx   ������
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
VOID  archTrap2ExceptHandle (ULONG  ulVector, ARCH_REG_CTX  *pregctx)
{
    archTrapExceptHandle(ulVector, pregctx);
}
/*********************************************************************************************************
** ��������: archTrap3ExceptHandle
** ��������: ����ָ���쳣 3
** �䡡��  : ulVector  �ж�����
**           pregctx   ������
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
VOID  archTrap3ExceptHandle (ULONG  ulVector, ARCH_REG_CTX  *pregctx)
{
    archTrapExceptHandle(ulVector, pregctx);
}
/*********************************************************************************************************
** ��������: archTlbFatalExceptHandle
** ��������: TLB ���ɻָ��쳣
** �䡡��  : ulVector  �ж�����
**           pregctx   ������
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
VOID  archTlbFatalExceptHandle (ULONG  ulVector, ARCH_REG_CTX  *pregctx)
{
    LW_VMM_ABORT    abtInfo;
    PLW_CLASS_TCB   ptcbCur;

    LW_TCB_GET_CUR(ptcbCur);

    abtInfo.VMABT_uiMethod = 0;
    abtInfo.VMABT_uiType   = LW_VMM_ABORT_TYPE_FATAL_ERROR;
    API_VmmAbortIsr(pregctx->REG_ulPc, pregctx->REG_ulMeh, &abtInfo, ptcbCur);
}
/*********************************************************************************************************
** ��������: archTlbRefillExceptHandle
** ��������: TLB �����쳣
** �䡡��  : ulVector  �ж�����
**           pregctx   ������
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
#if LW_CFG_VMM_EN == 0

VOID  archTlbRefillExceptHandle (ULONG  ulVector, ARCH_REG_CTX  *pregctx)
{
    LW_VMM_ABORT    abtInfo;
    PLW_CLASS_TCB   ptcbCur;

    LW_TCB_GET_CUR(ptcbCur);

    abtInfo.VMABT_uiMethod = 0;
    abtInfo.VMABT_uiType   = LW_VMM_ABORT_TYPE_FATAL_ERROR;
    API_VmmAbortIsr(pregctx->REG_ulPc, pregctx->REG_ulMeh, &abtInfo, ptcbCur);
}

#endif                                                                  /*  LW_CFG_VMM_EN == 0          */
/*********************************************************************************************************
** ��������: archReservedExceptHandle
** ��������: Reserved �ж�
** �䡡��  : ulVector  �ж�����
**           pregctx   ������
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
VOID  archReservedExceptHandle (ULONG  ulVector, ARCH_REG_CTX  *pregctx)
{
    LW_VMM_ABORT    abtInfo;
    PLW_CLASS_TCB   ptcbCur;
#if LW_CFG_CPU_FPU_EN > 0
    UINT32          uiFESR;
#endif                                                                  /*  LW_CFG_CPU_FPU_EN > 0       */

    LW_TCB_GET_CUR(ptcbCur);

    abtInfo.VMABT_uiType = LW_VMM_ABORT_TYPE_FATAL_ERROR;               /*  ��ֹ����Ĭ��Ϊ FATAL �쳣   */

#if LW_CFG_CPU_FPU_EN > 0
    uiFESR = cskyVfpGetFESR();
    if (uiFESR & FESR_FEC) {                                            /*  ������ FPU �쳣             */
        abtInfo.VMABT_uiType = LW_VMM_ABORT_TYPE_FPE;

        if (uiFESR & FESR_IDC) {                                        /*  �ǹ��������                */
            abtInfo.VMABT_uiMethod = FPE_FLTSUB;

        } else if(uiFESR & FESR_IXC) {
            abtInfo.VMABT_uiMethod = FPE_FLTRES;                        /*  ����ȷ�쳣                  */

        }  else if(uiFESR & FESR_UFC) {
            abtInfo.VMABT_uiMethod = FPE_FLTUND;                        /*  �����쳣                    */

        } else if(uiFESR & FESR_OFC) {
            abtInfo.VMABT_uiMethod = FPE_FLTOVF;                        /*  �����쳣                    */

        } else if(uiFESR & FESR_DZC) {
            abtInfo.VMABT_uiMethod = FPE_FLTDIV;                        /*  �����쳣                    */

        } else if(uiFESR & FESR_IOC) {
            abtInfo.VMABT_uiMethod = FPE_FLTINV;                        /*  �Ƿ������쳣                */

        } else {
            abtInfo.VMABT_uiMethod = 0;
        }

        cskyVfpSetFESR(0);                                              /*  ��� FPU �쳣               */
    }
#endif                                                                  /*  LW_CFG_CPU_FPU_EN > 0       */

    if (abtInfo.VMABT_uiType) {
        API_VmmAbortIsr(pregctx->REG_ulPc, pregctx->REG_ulPc, &abtInfo, ptcbCur);
    }
}
/*********************************************************************************************************
** ��������: archIdlyExceptHandle
** ��������: Idly �쳣
** �䡡��  : ulVector  �ж�����
**           pregctx   ������
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
VOID  archIdlyExceptHandle (ULONG  ulVector, ARCH_REG_CTX  *pregctx)
{
    PLW_CLASS_TCB  ptcbCur;
    LW_VMM_ABORT   abtInfo;

    LW_TCB_GET_CUR(ptcbCur);

    abtInfo.VMABT_uiMethod = 0;
    abtInfo.VMABT_uiType   = LW_VMM_ABORT_TYPE_FATAL_ERROR;
    API_VmmAbortIsr(pregctx->REG_ulPc, pregctx->REG_ulPc, &abtInfo, ptcbCur);
}
/*********************************************************************************************************
** ��������: archFatalErrExceptHandle
** ��������: ���ɻָ������쳣
** �䡡��  : ulVector  �ж�����
**           pregctx   ������
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
VOID  archFatalErrExceptHandle (ULONG  ulVector, ARCH_REG_CTX  *pregctx)
{
    PLW_CLASS_TCB  ptcbCur;
    LW_VMM_ABORT   abtInfo;

    LW_TCB_GET_CUR(ptcbCur);

    abtInfo.VMABT_uiMethod = 0;
    abtInfo.VMABT_uiType   = LW_VMM_ABORT_TYPE_FATAL_ERROR;
    API_VmmAbortIsr(pregctx->REG_ulPc, pregctx->REG_ulPc, &abtInfo, ptcbCur);
}
/*********************************************************************************************************
** ��������: archBreakPointExceptHandle
** ��������: �ϵ��쳣
** �䡡��  : ulVector  �ж�����
**           pregctx   ������
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
VOID  archBreakPointExceptHandle (ULONG  ulVector, ARCH_REG_CTX  *pregctx)
{
    PLW_CLASS_TCB  ptcbCur;
    LW_VMM_ABORT   abtInfo;
#if LW_CFG_GDB_EN > 0
    UINT           uiBpType;
#endif                                                                  /*  LW_CFG_GDB_EN > 0           */

    LW_TCB_GET_CUR(ptcbCur);

#if LW_CFG_GDB_EN > 0
    uiBpType = archDbgTrapType(pregctx->REG_ulPc, LW_NULL);             /*  �ϵ�ָ��̽��                */
    if (uiBpType) {
        if (API_DtraceBreakTrap(pregctx->REG_ulPc, uiBpType) == ERROR_NONE) {
            return;                                                     /*  ������Խӿڶϵ㴦��        */
        }
    }
#endif                                                                  /*  LW_CFG_GDB_EN > 0           */

    abtInfo.VMABT_uiMethod = LW_VMM_ABORT_METHOD_EXEC;
    abtInfo.VMABT_uiType   = LW_VMM_ABORT_TYPE_BREAK;
    API_VmmAbortIsr(pregctx->REG_ulPc, pregctx->REG_ulPc, &abtInfo, ptcbCur);
}
/*********************************************************************************************************
** ��������: archPrivilegeExceptHandle
** ��������: ��ȨΥ���쳣
** �䡡��  : ulVector  �ж�����
**           pregctx   ������
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
VOID  archPrivilegeExceptHandle (ULONG  ulVector, ARCH_REG_CTX  *pregctx)
{
    PLW_CLASS_TCB  ptcbCur;
    LW_VMM_ABORT   abtInfo;

    LW_TCB_GET_CUR(ptcbCur);

    abtInfo.VMABT_uiMethod = LW_VMM_ABORT_METHOD_EXEC;
    abtInfo.VMABT_uiType   = LW_VMM_ABORT_TYPE_PERM;
    API_VmmAbortIsr(pregctx->REG_ulPc, pregctx->REG_ulPc, &abtInfo, ptcbCur);
}
/*********************************************************************************************************
** ��������: archIllegalInstExceptHandle
** ��������: �Ƿ�ָ���쳣
** �䡡��  : ulVector  �ж�����
**           pregctx   ������
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
VOID  archIllegalInstExceptHandle (ULONG  ulVector, ARCH_REG_CTX  *pregctx)
{
    PLW_CLASS_TCB  ptcbCur;
    LW_VMM_ABORT   abtInfo;

    LW_TCB_GET_CUR(ptcbCur);

    abtInfo.VMABT_uiMethod = LW_VMM_ABORT_METHOD_EXEC;
    abtInfo.VMABT_uiType   = LW_VMM_ABORT_TYPE_UNDEF;
    API_VmmAbortIsr(pregctx->REG_ulPc, pregctx->REG_ulPc, &abtInfo, ptcbCur);
}
/*********************************************************************************************************
** ��������: archDivideZeroExceptHandle
** ��������: �������쳣
** �䡡��  : ulVector  �ж�����
**           pregctx   ������
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
VOID  archDivideZeroExceptHandle (ULONG  ulVector, ARCH_REG_CTX  *pregctx)
{
    PLW_CLASS_TCB  ptcbCur;
    LW_VMM_ABORT   abtInfo;

    LW_TCB_GET_CUR(ptcbCur);

    abtInfo.VMABT_uiMethod = LW_VMM_ABORT_METHOD_EXEC;
    abtInfo.VMABT_uiType   = LW_VMM_ABORT_TYPE_TERMINAL;
    API_VmmAbortIsr(pregctx->REG_ulPc, pregctx->REG_ulPc, &abtInfo, ptcbCur);
}
/*********************************************************************************************************
** ��������: archAccessExceptHandle
** ��������: ���ʴ����쳣
** �䡡��  : ulVector  �ж�����
**           pregctx   ������
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
VOID  archAccessExceptHandle (ULONG  ulVector, ARCH_REG_CTX  *pregctx)
{
    PLW_CLASS_TCB  ptcbCur;
    LW_VMM_ABORT   abtInfo;

    LW_TCB_GET_CUR(ptcbCur);

    abtInfo.VMABT_uiMethod = 0;
    abtInfo.VMABT_uiType   = LW_VMM_ABORT_TYPE_BUS;
    API_VmmAbortIsr(pregctx->REG_ulPc, pregctx->REG_ulMeh, &abtInfo, ptcbCur);
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
    cskyUnalignedHandle(&pabtctx->ABTCTX_archRegCtx,
                        &pabtctx->ABTCTX_abtInfo);

    API_VmmAbortReturn(pabtctx);
}
/*********************************************************************************************************
** ��������: archUnalignedExceptHandle
** ��������: δ��������쳣����
** �䡡��  : ulVector  �ж�����
**           pregctx   ������
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
VOID  archUnalignedExceptHandle (ULONG  ulVector, ARCH_REG_CTX  *pregctx)
{
    PLW_CLASS_TCB  ptcbCur;
    LW_VMM_ABORT   abtInfo;
    CSKY_PARAM    *param = archKernelParamGet();

    LW_TCB_GET_CUR(ptcbCur);

    abtInfo.VMABT_uiType   = LW_VMM_ABORT_TYPE_BUS;
    abtInfo.VMABT_uiMethod = BUS_ADRALN;

    if (param->CP_bUnalign) {
        API_VmmAbortIsrEx(pregctx->REG_ulPc, pregctx->REG_ulPc, &abtInfo, ptcbCur, archUnalignedHandle);

    } else {
        API_VmmAbortIsr(pregctx->REG_ulPc, pregctx->REG_ulPc, &abtInfo, ptcbCur);
    }
}

#endif                                                                  /*  !__SYLIXOS_CSKY_ARCH_CK803__*/
/*********************************************************************************************************
  END
*********************************************************************************************************/
