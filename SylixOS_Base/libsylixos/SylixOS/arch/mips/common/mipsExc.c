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
** ��   ��   ��: mipsExc.c
**
** ��   ��   ��: Jiao.JinXing (������)
**
** �ļ���������: 2015 �� 09 �� 01 ��
**
** ��        ��: MIPS ��ϵ�ܹ��쳣����.
*********************************************************************************************************/
#define  __SYLIXOS_STDIO
#define  __SYLIXOS_KERNEL
#include "SylixOS.h"
#include "dtrace.h"
#include "arch/mips/param/mipsParam.h"
#include "arch/mips/common/cp0/mipsCp0.h"
#include "arch/mips/common/unaligned/mipsUnaligned.h"
#if LW_CFG_VMM_EN > 0
#include "arch/mips/mm/mmu/mipsMmuCommon.h"
#endif                                                                  /*  LW_CFG_VMM_EN > 0           */
#if LW_CFG_CPU_FPU_EN > 0
#include "arch/mips/fpu/emu/mipsFpuEmu.h"
#include "arch/mips/fpu/fpu32/mipsVfp32.h"
#endif                                                                  /*  LW_CFG_CPU_FPU_EN > 0       */
/*********************************************************************************************************
  MIPS �쳣����������
*********************************************************************************************************/
typedef VOID  (*MIPS_EXCEPT_HANDLE)(addr_t         ulRetAddr,           /*  ���ص�ַ                    */
                                    addr_t         ulAbortAddr,         /*  ��ֹ��ַ                    */
                                    ARCH_REG_CTX  *pregctx);            /*  �Ĵ���������                */
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
** ��������: archCacheErrorHandle
** ��������: Cache ������
** �䡡��  : NONE
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
#if LW_CFG_CACHE_EN > 0

VOID  archCacheErrorHandle (ARCH_REG_CTX  *pregctx)
{
    REGISTER UINT32         uiFiled  = 2 * sizeof(UINT32);
    REGISTER ARCH_REG_T     ulRegVal;
             PLW_CLASS_TCB  ptcbCur;
             LW_VMM_ABORT   abtInfo;

    ulRegVal = mipsCp0ConfigRead();
    mipsCp0ConfigWrite((ulRegVal & ~M_ConfigK0) | MIPS_UNCACHED);

    _PrintFormat("Cache error exception:\r\n");
    _PrintFormat("cp0_error epc == %lx\r\n", uiFiled, mipsCp0ERRPCRead());

    ulRegVal = mipsCp0CacheErrRead();

    _PrintFormat("cp0_cache error == 0x%08x\r\n", ulRegVal);
    _PrintFormat("Decoded cp0_cache error: %s cache fault in %s reference.\r\n",
                 (ulRegVal & M_CacheLevel) ? "secondary" : "primary",
                 (ulRegVal & M_CacheType)  ? "d-cache"   : "i-cache");

    _PrintFormat("Error bits: %s%s%s%s%s%s%s\r\n",
                 (ulRegVal & M_CacheData) ? "ED " : "",
                 (ulRegVal & M_CacheTag)  ? "ET " : "",
                 (ulRegVal & M_CacheECC)  ? "EE " : "",
                 (ulRegVal & M_CacheBoth) ? "EB " : "",
                 (ulRegVal & M_CacheEI)   ? "EI " : "",
                 (ulRegVal & M_CacheE1)   ? "E1 " : "",
                 (ulRegVal & M_CacheE0)   ? "E0 " : "");

    _PrintFormat("IDX: 0x%08x\r\n", ulRegVal & (M_CacheE0 - 1));

    LW_TCB_GET_CUR(ptcbCur);

    abtInfo.VMABT_uiMethod = 0;
    abtInfo.VMABT_uiType   = LW_VMM_ABORT_TYPE_FATAL_ERROR;
    API_VmmAbortIsr(pregctx->REG_ulCP0Epc, pregctx->REG_ulCP0Epc, &abtInfo, ptcbCur);
}

#endif                                                                  /*  LW_CFG_CACHE_EN > 0         */
/*********************************************************************************************************
** ��������: archTlbModExceptHandle
** ��������: TLB modified �쳣����
** �䡡��  : ulRetAddr     ���ص�ַ
**           ulAbortAddr   ��ֹ��ַ
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static VOID  archTlbModExceptHandle (addr_t  ulRetAddr, addr_t  ulAbortAddr)
{
    PLW_CLASS_TCB  ptcbCur;
    LW_VMM_ABORT   abtInfo;

    /*
     * A TLB modify exception occurs when a TLB entry matches a store reference to a mapped address,
     * but the matched entry is not dirty.
     */

    LW_TCB_GET_CUR(ptcbCur);

    abtInfo.VMABT_uiMethod = LW_VMM_ABORT_METHOD_WRITE;
    abtInfo.VMABT_uiType   = LW_VMM_ABORT_TYPE_PERM;
    API_VmmAbortIsr(ulRetAddr, ulAbortAddr, &abtInfo, ptcbCur);
}
/*********************************************************************************************************
** ��������: archTlbLoadExceptHandle
** ��������: TLB load or ifetch �쳣����
** �䡡��  : ulRetAddr     ���ص�ַ
**           ulAbortAddr   ��ֹ��ַ
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static VOID  archTlbLoadExceptHandle (addr_t  ulRetAddr, addr_t  ulAbortAddr)
{
    PLW_CLASS_TCB  ptcbCur;
    LW_VMM_ABORT   abtInfo;

    LW_TCB_GET_CUR(ptcbCur);

    abtInfo.VMABT_uiMethod = LW_VMM_ABORT_METHOD_READ;
#if LW_CFG_VMM_EN > 0
    abtInfo.VMABT_uiType   = mipsMmuTlbLoadStoreExcHandle(ulAbortAddr, LW_FALSE);
#else
    abtInfo.VMABT_uiType   = LW_VMM_ABORT_TYPE_TERMINAL;
#endif                                                                  /*  LW_CFG_VMM_EN > 0           */
    if (abtInfo.VMABT_uiType) {
        API_VmmAbortIsr(ulRetAddr, ulAbortAddr, &abtInfo, ptcbCur);
    }
}
/*********************************************************************************************************
** ��������: archTlbStoreExceptHandle
** ��������: TLB store �쳣����
** �䡡��  : ulRetAddr     ���ص�ַ
**           ulAbortAddr   ��ֹ��ַ
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static VOID  archTlbStoreExceptHandle (addr_t  ulRetAddr, addr_t  ulAbortAddr)
{
    PLW_CLASS_TCB  ptcbCur;
    LW_VMM_ABORT   abtInfo;

    LW_TCB_GET_CUR(ptcbCur);

    abtInfo.VMABT_uiMethod = LW_VMM_ABORT_METHOD_WRITE;
#if LW_CFG_VMM_EN > 0
    abtInfo.VMABT_uiType   = mipsMmuTlbLoadStoreExcHandle(ulAbortAddr, LW_TRUE);
#else
    abtInfo.VMABT_uiType   = LW_VMM_ABORT_TYPE_TERMINAL;
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
    mipsUnalignedHandle(&pabtctx->ABTCTX_archRegCtx,
                        &pabtctx->ABTCTX_abtInfo);

    API_VmmAbortReturn(pabtctx);
}
/*********************************************************************************************************
** ��������: archAddrLoadExceptHandle
** ��������: Address load or ifetch �쳣����
** �䡡��  : ulRetAddr     ���ص�ַ
**           ulAbortAddr   ��ֹ��ַ
**           pregctx       �Ĵ���������
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static VOID  archAddrLoadExceptHandle (addr_t  ulRetAddr, addr_t  ulAbortAddr, ARCH_REG_CTX  *pregctx)
{
    PLW_CLASS_TCB  ptcbCur;
    LW_VMM_ABORT   abtInfo;
    MIPS_PARAM    *param = archKernelParamGet();

    LW_TCB_GET_CUR(ptcbCur);

    /*
     *  An address error exception occurs in following cases:
     *  1. An instruction fetch or a data load/store accesses a word from a misaligned word address
     *     boundary.
     *  2. A data load/store accesses a half-word from a misaligned half-word address boundary.
     *  3. Reference kernel address space in user mode.
     *
     *  TODO: ��������δʹ���û�ģʽ, ���Ե���������ݲ��ô���
     */

    abtInfo.VMABT_uiMethod = BUS_ADRALN;
    abtInfo.VMABT_uiType   = LW_VMM_ABORT_TYPE_BUS;

    if (param->MP_bUnalign) {
        API_VmmAbortIsrEx(ulRetAddr, ulAbortAddr, &abtInfo, ptcbCur, archUnalignedHandle);

    } else {
        API_VmmAbortIsr(ulRetAddr, ulAbortAddr, &abtInfo, ptcbCur);
    }
}
/*********************************************************************************************************
** ��������: archAddrStoreExceptHandle
** ��������: Address store �쳣����
** �䡡��  : ulRetAddr     ���ص�ַ
**           ulAbortAddr   ��ֹ��ַ
**           pregctx       �Ĵ���������
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static VOID  archAddrStoreExceptHandle (addr_t  ulRetAddr, addr_t  ulAbortAddr, ARCH_REG_CTX  *pregctx)
{
    PLW_CLASS_TCB  ptcbCur;
    LW_VMM_ABORT   abtInfo;
    MIPS_PARAM    *param = archKernelParamGet();

    LW_TCB_GET_CUR(ptcbCur);

    abtInfo.VMABT_uiMethod = BUS_ADRALN;
    abtInfo.VMABT_uiType   = LW_VMM_ABORT_TYPE_BUS;

    if (param->MP_bUnalign) {
        API_VmmAbortIsrEx(ulRetAddr, ulAbortAddr, &abtInfo, ptcbCur, archUnalignedHandle);

    } else {
        API_VmmAbortIsr(ulRetAddr, ulAbortAddr, &abtInfo, ptcbCur);
    }
}
/*********************************************************************************************************
** ��������: archInstBusExceptHandle
** ��������: Instruction Bus �쳣����
** �䡡��  : ulRetAddr     ���ص�ַ
**           ulAbortAddr   ��ֹ��ַ
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static VOID  archInstBusExceptHandle (addr_t  ulRetAddr, addr_t  ulAbortAddr)
{
    PLW_CLASS_TCB  ptcbCur;
    LW_VMM_ABORT   abtInfo;

    LW_TCB_GET_CUR(ptcbCur);

#if LW_CFG_CPU_EXC_HOOK_EN > 0
    if (bspCpuExcHook(ptcbCur, ulRetAddr, ulAbortAddr, ARCH_BUS_EXCEPTION, 0)) {
        return;
    }
#endif                                                                  /*  LW_CFG_CPU_EXC_HOOK_EN > 0  */

    abtInfo.VMABT_uiMethod = BUS_ADRERR;
    abtInfo.VMABT_uiType   = LW_VMM_ABORT_TYPE_BUS;
    API_VmmAbortIsr(ulRetAddr, ulAbortAddr, &abtInfo, ptcbCur);
}
/*********************************************************************************************************
** ��������: archDataBusExceptHandle
** ��������: Data Bus �쳣����
** �䡡��  : ulRetAddr     ���ص�ַ
**           ulAbortAddr   ��ֹ��ַ
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static VOID  archDataBusExceptHandle (addr_t  ulRetAddr, addr_t  ulAbortAddr)
{
    PLW_CLASS_TCB  ptcbCur;
    LW_VMM_ABORT   abtInfo;

    LW_TCB_GET_CUR(ptcbCur);

#if LW_CFG_CPU_EXC_HOOK_EN > 0
    if (bspCpuExcHook(ptcbCur, ulRetAddr, ulAbortAddr, ARCH_BUS_EXCEPTION, 1)) {
        return;
    }
#endif                                                                  /*  LW_CFG_CPU_EXC_HOOK_EN > 0  */

    abtInfo.VMABT_uiMethod = BUS_ADRERR;
    abtInfo.VMABT_uiType   = LW_VMM_ABORT_TYPE_BUS;
    API_VmmAbortIsr(ulRetAddr, ulAbortAddr, &abtInfo, ptcbCur);
}
/*********************************************************************************************************
** ��������: archSysCallHandle
** ��������: System Call ����
** �䡡��  : ulRetAddr     ���ص�ַ
**           ulAbortAddr   ��ֹ��ַ
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static VOID  archSysCallHandle (addr_t  ulRetAddr, addr_t  ulAbortAddr)
{
    PLW_CLASS_TCB  ptcbCur;
    LW_VMM_ABORT   abtInfo;

    LW_TCB_GET_CUR(ptcbCur);

    abtInfo.VMABT_uiMethod = 0;
    abtInfo.VMABT_uiType   = LW_VMM_ABORT_TYPE_SYS;
    API_VmmAbortIsr(ulRetAddr, ulRetAddr, &abtInfo, ptcbCur);
}
/*********************************************************************************************************
** ��������: archBreakPointHandle
** ��������: Break Point ����
** �䡡��  : ulRetAddr     ���ص�ַ
**           ulAbortAddr   ��ֹ��ַ
**           pregctx       �Ĵ���������
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static VOID  archBreakPointHandle (addr_t  ulRetAddr, addr_t  ulAbortAddr, ARCH_REG_CTX  *pregctx)
{
    PLW_CLASS_TCB  ptcbCur;
    LW_VMM_ABORT   abtInfo;
#if LW_CFG_GDB_EN > 0
    UINT           uiBpType;
#endif                                                                  /*  LW_CFG_GDB_EN > 0           */

    LW_TCB_GET_CUR(ptcbCur);

#if LW_CFG_CPU_FPU_EN > 0
    if (*(MIPS_INSTRUCTION *)ulRetAddr == BRK_MEMU) {
        if (do_dsemulret(ptcbCur, pregctx) == ERROR_NONE) {
            return;                                                     /*  FPU ģ�ⷵ��                */
        }
    }
#endif                                                                  /*  LW_CFG_CPU_FPU_EN > 0       */

#if LW_CFG_GDB_EN > 0
    uiBpType = archDbgTrapType(ulRetAddr, LW_NULL);                     /*  �ϵ�ָ��̽��                */
    if (uiBpType) {
        if (API_DtraceBreakTrap(ulRetAddr, uiBpType) == ERROR_NONE) {
            return;                                                     /*  ������Խӿڶϵ㴦��        */
        }
    }
#endif                                                                  /*  LW_CFG_GDB_EN > 0           */

    abtInfo.VMABT_uiMethod = 0;
    abtInfo.VMABT_uiType   = LW_VMM_ABORT_TYPE_BREAK;
    API_VmmAbortIsr(ulRetAddr, ulRetAddr, &abtInfo, ptcbCur);
}
/*********************************************************************************************************
** ��������: archTrapInstHandle
** ��������: Trap instruction ����
** �䡡��  : ulRetAddr     ���ص�ַ
**           ulAbortAddr   ��ֹ��ַ
**           pregctx       �Ĵ���������
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static VOID  archTrapInstHandle (addr_t  ulRetAddr, addr_t  ulAbortAddr, ARCH_REG_CTX  *pregctx)
{
    /*
     * �� TGE��TGUE��TLT��TLTU��TEQ��TNE��TGEI��TGEUI��TLTI��TLTUI��TEQI��TNEI ָ��ִ�У�
     * �������Ϊ��ʱ��������������
     */
    PLW_CLASS_TCB  ptcbCur;
    LW_VMM_ABORT   abtInfo;

    LW_TCB_GET_CUR(ptcbCur);

    abtInfo.VMABT_uiMethod = 0;
    abtInfo.VMABT_uiType   = LW_VMM_ABORT_TYPE_TERMINAL;
    API_VmmAbortIsr(ulRetAddr, ulAbortAddr, &abtInfo, ptcbCur);
}
/*********************************************************************************************************
** ��������: archResvInstHandle
** ��������: Reserved instruction ����
** �䡡��  : ulRetAddr     ���ص�ַ
**           ulAbortAddr   ��ֹ��ַ
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static VOID  archResvInstHandle (addr_t  ulRetAddr, addr_t  ulAbortAddr)
{
    PLW_CLASS_TCB  ptcbCur;
    LW_VMM_ABORT   abtInfo;

    LW_TCB_GET_CUR(ptcbCur);

    abtInfo.VMABT_uiMethod = LW_VMM_ABORT_METHOD_EXEC;
    abtInfo.VMABT_uiType   = LW_VMM_ABORT_TYPE_UNDEF;
    API_VmmAbortIsr(ulRetAddr, ulRetAddr, &abtInfo, ptcbCur);
}
/*********************************************************************************************************
** ��������: archFloatPointUnImplHandle
** ��������: FPU δʵ���쳣����
** �䡡��  : pabtctx    abort ������
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static VOID  archFloatPointUnImplHandle (PLW_VMM_ABORT_CTX  pabtctx)
{
    PLW_CLASS_TCB  ptcbCur;
    ARCH_FPU_CTX  *pFpuCtx;
    INT            iSignal;

    LW_NONSCHED_MODE_PROC(
        LW_TCB_GET_CUR(ptcbCur);
        pFpuCtx = ptcbCur->TCB_pvStackFP;
        __ARCH_FPU_ENABLE();
        __ARCH_FPU_SAVE(pFpuCtx);                                       /*  ���浱ǰ FPU CTX            */
    );

    iSignal = fpu_emulator_cop1Handler(&pabtctx->ABTCTX_archRegCtx,     /*  FPU ģ��                    */
                                       pFpuCtx,
                                       LW_TRUE,
                                       (PVOID *)&pabtctx->ABTCTX_ulAbortAddr);
    switch (iSignal) {

    case 0:                                                             /*  �ɹ�ģ��                    */
        pabtctx->ABTCTX_abtInfo.VMABT_uiType = LW_VMM_ABORT_TYPE_NOINFO;
        LW_NONSCHED_MODE_PROC(
            __ARCH_FPU_RESTORE(pFpuCtx);                                /*  �ָ���ǰ FPU CTX            */
        );
        break;

    case SIGILL:                                                        /*  δ����ָ��                  */
        pabtctx->ABTCTX_abtInfo.VMABT_uiMethod = LW_VMM_ABORT_METHOD_EXEC;
        pabtctx->ABTCTX_abtInfo.VMABT_uiType   = LW_VMM_ABORT_TYPE_UNDEF;
        pabtctx->ABTCTX_ulAbortAddr = pabtctx->ABTCTX_ulRetAddr;
        break;

    case SIGBUS:                                                        /*  ���ߴ���                    */
        pabtctx->ABTCTX_abtInfo.VMABT_uiMethod = BUS_ADRERR;
        pabtctx->ABTCTX_abtInfo.VMABT_uiType   = LW_VMM_ABORT_TYPE_BUS;
        break;

    case SIGSEGV:                                                       /*  ��ַ���Ϸ�                  */
        pabtctx->ABTCTX_abtInfo.VMABT_uiMethod = LW_VMM_ABORT_METHOD_WRITE;
        pabtctx->ABTCTX_abtInfo.VMABT_uiType   = LW_VMM_ABORT_TYPE_TERMINAL;
        break;

    case SIGFPE:                                                        /*  FPU ����                    */
    default:
        break;
    }

    LW_NONSCHED_MODE_PROC(
       __ARCH_FPU_DISABLE();
    );

    API_VmmAbortReturn(pabtctx);
}
/*********************************************************************************************************
** ��������: archFloatPointExceptHandle
** ��������: �����쳣����
** �䡡��  : ulRetAddr     ���ص�ַ
**           ulAbortAddr   ��ֹ��ַ
**           pregctx       �Ĵ���������
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static VOID  archFloatPointExceptHandle (addr_t  ulRetAddr, addr_t  ulAbortAddr, ARCH_REG_CTX  *pregctx)
{
    PLW_CLASS_TCB  ptcbCur;
    LW_VMM_ABORT   abtInfo;
#if LW_CFG_CPU_FPU_EN > 0
    UINT32         uiConfig1;
    UINT32         uiFCSR;
    UINT32         uiFEXR;
#endif                                                                  /*  LW_CFG_CPU_FPU_EN > 0       */

    LW_TCB_GET_CUR(ptcbCur);

    abtInfo.VMABT_uiMethod = FPE_FLTINV;
    abtInfo.VMABT_uiType   = LW_VMM_ABORT_TYPE_FPE;                     /*  ��ֹ����Ĭ��Ϊ FPU �쳣     */

#if LW_CFG_CPU_FPU_EN > 0
#define FEXR_MASK       ((0x3f << 12) | (0x1f << 2))
#define FENR_MASK       ((0x1f <<  7) | (0x07 << 0))
#define FCCR_MASK       ((0xff <<  0))

    uiConfig1 = mipsCp0Config1Read();
    if (uiConfig1 & MIPS_CONF1_FP) {                                    /*  �� FPU                      */

        uiFCSR = mipsVfp32GetFCSR();
        mipsVfp32SetFCSR(uiFCSR & (~FEXR_MASK));                        /*  ��������쳣                */

        uiFEXR = uiFCSR & FEXR_MASK;                                    /*  ��ø����쳣����            */
        if (uiFEXR & (1 << 17)) {                                       /*  δʵ���쳣                  */
            abtInfo.VMABT_uiMethod = FPE_FLTINV;
            API_VmmAbortIsrEx(ulRetAddr, ulAbortAddr, &abtInfo, ptcbCur, archFloatPointUnImplHandle);
            return;

        } else if (uiFEXR & (1 << 16)) {                                /*  �Ƿ������쳣                */
            abtInfo.VMABT_uiMethod = FPE_FLTINV;

        } else if (uiFEXR & (1 << 15)) {                                /*  �����쳣                    */
            abtInfo.VMABT_uiMethod = FPE_FLTDIV;

        } else if (uiFEXR & (1 << 14)) {                                /*  �����쳣                    */
            abtInfo.VMABT_uiMethod = FPE_FLTOVF;

        } else if (uiFEXR & (1 << 13)) {
            abtInfo.VMABT_uiMethod = FPE_FLTUND;                        /*  �����쳣                    */

        } else if (uiFEXR & (1 << 12)) {                                /*  ����ȷ�쳣                  */
            abtInfo.VMABT_uiMethod = FPE_FLTRES;
        }
    }
#endif                                                                  /*  LW_CFG_CPU_FPU_EN > 0       */

    API_VmmAbortIsr(ulRetAddr, ulAbortAddr, &abtInfo, ptcbCur);
}
/*********************************************************************************************************
** ��������: archCoProc2ExceptHandle
** ��������: Э������ 2 �쳣����
** �䡡��  : ulRetAddr     ���ص�ַ
**           ulAbortAddr   ��ֹ��ַ
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
#if defined(_MIPS_ARCH_HR2)

static VOID  archCoProc2ExceptHandle (addr_t  ulRetAddr, addr_t  ulAbortAddr)
{
    PLW_CLASS_TCB  ptcbCur;
    LW_VMM_ABORT   abtInfo;

    LW_TCB_GET_CUR(ptcbCur);

    abtInfo.VMABT_uiMethod = 0;
    abtInfo.VMABT_uiType   = LW_VMM_ABORT_TYPE_DSPE;
    API_VmmAbortIsr(ulRetAddr, ulAbortAddr, &abtInfo, ptcbCur);
}

#endif                                                                  /*  defined(_MIPS_ARCH_HR2)     */
/*********************************************************************************************************
** ��������: archCoProcUnusableExceptHandle
** ��������: Э�������������쳣����
** �䡡��  : ulRetAddr     ���ص�ַ
**           ulAbortAddr   ��ֹ��ַ
**           pregctx       �Ĵ���������
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static VOID  archCoProcUnusableExceptHandle (addr_t  ulRetAddr, addr_t  ulAbortAddr, ARCH_REG_CTX  *pregctx)
{
    PLW_CLASS_TCB  ptcbCur;
    LW_VMM_ABORT   abtInfo;

    LW_TCB_GET_CUR(ptcbCur);

    switch (((pregctx->REG_ulCP0Cause & CAUSEF_CE) >> CAUSEB_CE)) {     /*  ��ò�����Э���������      */

#if LW_CFG_CPU_FPU_EN > 0
    case 1:
        if (archFpuUndHandle(ptcbCur) == ERROR_NONE) {                  /*  ���� FPU ָ��̽��           */
            return;
        }
        break;
#endif                                                                  /*  LW_CFG_CPU_FPU_EN > 0       */

#if defined(_MIPS_ARCH_HR2) && (LW_CFG_CPU_DSP_EN > 0)
    case 2:
        if (archDspUndHandle(ptcbCur) == ERROR_NONE) {                  /*  ���� DSP ָ��̽��           */
            return;
        }
        break;
#endif                                                                  /*  defined(_MIPS_ARCH_HR2)     */
                                                                        /*  LW_CFG_CPU_DSP_EN > 0       */
    default:
        break;
    }

    abtInfo.VMABT_uiMethod = LW_VMM_ABORT_METHOD_EXEC;
    abtInfo.VMABT_uiType   = LW_VMM_ABORT_TYPE_UNDEF;
    API_VmmAbortIsr(ulRetAddr, ulRetAddr, &abtInfo, ptcbCur);
}
/*********************************************************************************************************
** ��������: archDspDisableExceptHandle
** ��������: DSP �ر��쳣����
** �䡡��  : ulRetAddr     ���ص�ַ
**           ulAbortAddr   ��ֹ��ַ
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static VOID  archDspDisableExceptHandle (addr_t  ulRetAddr, addr_t  ulAbortAddr)
{
    PLW_CLASS_TCB  ptcbCur;
    LW_VMM_ABORT   abtInfo;

    LW_TCB_GET_CUR(ptcbCur);

#if LW_CFG_CPU_DSP_EN > 0
    if (archDspUndHandle(ptcbCur) == ERROR_NONE) {                      /*  ���� DSP ָ��̽��           */
        return;
    }
#endif                                                                  /*  LW_CFG_CPU_DSP_EN > 0       */

    abtInfo.VMABT_uiMethod = LW_VMM_ABORT_METHOD_EXEC;
    abtInfo.VMABT_uiType   = LW_VMM_ABORT_TYPE_UNDEF;
    API_VmmAbortIsr(ulRetAddr, ulRetAddr, &abtInfo, ptcbCur);
}
/*********************************************************************************************************
** ��������: archDefaultExceptHandle
** ��������: ȱʡ���쳣����
** �䡡��  : ulRetAddr     ���ص�ַ
**           ulAbortAddr   ��ֹ��ַ
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static VOID  archDefaultExceptHandle (addr_t  ulRetAddr, addr_t  ulAbortAddr)
{
    PLW_CLASS_TCB  ptcbCur;
    LW_VMM_ABORT   abtInfo;

    LW_TCB_GET_CUR(ptcbCur);

    abtInfo.VMABT_uiMethod = 0;
    abtInfo.VMABT_uiType   = LW_VMM_ABORT_TYPE_TERMINAL;
    API_VmmAbortIsr(ulRetAddr, ulAbortAddr, &abtInfo, ptcbCur);
}
/*********************************************************************************************************
** ��������: archExecInhibitExceptHandle
** ��������: ִ�н�ֹ���쳣����
** �䡡��  : ulRetAddr     ���ص�ַ
**           ulAbortAddr   ��ֹ��ַ
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static VOID  archExecInhibitExceptHandle (addr_t  ulRetAddr, addr_t  ulAbortAddr)
{
    PLW_CLASS_TCB  ptcbCur;
    LW_VMM_ABORT   abtInfo;

    LW_TCB_GET_CUR(ptcbCur);

    abtInfo.VMABT_uiMethod = LW_VMM_ABORT_METHOD_EXEC;
    abtInfo.VMABT_uiType   = LW_VMM_ABORT_TYPE_PERM;
    API_VmmAbortIsr(ulRetAddr, ulAbortAddr, &abtInfo, ptcbCur);
}
/*********************************************************************************************************
** ��������: archMachineCheckExceptHandle
** ��������: ��������쳣����
** �䡡��  : ulRetAddr     ���ص�ַ
**           ulAbortAddr   ��ֹ��ַ
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static VOID  archMachineCheckExceptHandle (addr_t  ulRetAddr, addr_t  ulAbortAddr)
{
    PLW_CLASS_TCB  ptcbCur;
    LW_VMM_ABORT   abtInfo;

    /*
     * The machine check exception occurs when executing of TLBWI or TLBWR instruction
     * detects multiple matching entries in the JTLB.
     */

    LW_TCB_GET_CUR(ptcbCur);

    abtInfo.VMABT_uiMethod = 0;
    abtInfo.VMABT_uiType   = LW_VMM_ABORT_TYPE_FATAL_ERROR;
    API_VmmAbortIsr(ulRetAddr, ulAbortAddr, &abtInfo, ptcbCur);
}
/*********************************************************************************************************
  MIPS �쳣����������
*********************************************************************************************************/
static MIPS_EXCEPT_HANDLE   _G_mipsExceptHandle[32] = {
    [EXCCODE_INT]      = (PVOID)bspIntHandle,                           /*  Interrupt pending           */
    [EXCCODE_MOD]      = (PVOID)archTlbModExceptHandle,                 /*  TLB modified fault          */
    [EXCCODE_TLBL]     = (PVOID)archTlbLoadExceptHandle,                /*  TLB miss on load or ifetch  */
    [EXCCODE_TLBS]     = (PVOID)archTlbStoreExceptHandle,               /*  TLB miss on a store         */
    [EXCCODE_ADEL]     = (PVOID)archAddrLoadExceptHandle,               /*  Address error(load or ifetch*/
    [EXCCODE_ADES]     = (PVOID)archAddrStoreExceptHandle,              /*  Address error(store)        */
    [EXCCODE_IBE]      = (PVOID)archInstBusExceptHandle,                /*  Instruction bus error       */
    [EXCCODE_DBE]      = (PVOID)archDataBusExceptHandle,                /*  Data bus error              */
    [EXCCODE_SYS]      = (PVOID)archSysCallHandle,                      /*  System call                 */
    [EXCCODE_BP]       = (PVOID)archBreakPointHandle,                   /*  Breakpoint                  */
    [EXCCODE_RI]       = (PVOID)archResvInstHandle,                     /*  Reserved instruction        */
    [EXCCODE_CPU]      = (PVOID)archCoProcUnusableExceptHandle,         /*  Coprocessor unusable        */
    [EXCCODE_OV]       = (PVOID)archDefaultExceptHandle,                /*  Arithmetic overflow         */
    [EXCCODE_TR]       = (PVOID)archTrapInstHandle,                     /*  Trap instruction            */
    [EXCCODE_MSAFPE]   = (PVOID)archDefaultExceptHandle,                /*  MSA floating point exception*/
    [EXCCODE_FPE]      = (PVOID)archFloatPointExceptHandle,             /*  Floating point exception    */
    [EXCCODE_GSEXC]    = (PVOID)archDefaultExceptHandle,                /*  Loongson cpu exception      */
#if defined(_MIPS_ARCH_HR2)
    [EXCCODE_C2E]      = (PVOID)archCoProc2ExceptHandle,                /*  Coprocessor 2 exception     */
#endif                                                                  /*  defined(_MIPS_ARCH_HR2)     */
    /*
     * ����ֹ����ûʹ��, ִ����ֹ���⸴�� TLBL ����
     */
    [EXCCODE_TLBRI]    = (PVOID)archDefaultExceptHandle,                /*  TLB Read-Inhibit exception  */
    [EXCCODE_TLBXI]    = (PVOID)archExecInhibitExceptHandle,            /*  TLB Exec-Inhibit exception  */
    [EXCCODE_MSADIS]   = (PVOID)archDefaultExceptHandle,                /*  MSA disabled exception      */
    [EXCCODE_MDMX]     = (PVOID)archDefaultExceptHandle,                /*  MDMX unusable exception     */
    [EXCCODE_WATCH]    = (PVOID)archDefaultExceptHandle,                /*  Watch address reference     */
    [EXCCODE_MCHECK]   = (PVOID)archMachineCheckExceptHandle,           /*  Machine check exception     */
    [EXCCODE_THREAD]   = (PVOID)archDefaultExceptHandle,                /*  Thread exception (MT)       */
    [EXCCODE_DSPDIS]   = (PVOID)archDspDisableExceptHandle,             /*  DSP disabled exception      */
    [EXCCODE_CACHEERR] = (PVOID)archCacheErrorHandle,                   /*  Cache error exception       */
};
/*********************************************************************************************************
** ��������: archExceptionHandle
** ��������: ͨ���쳣����
** �䡡��  : NONE
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
VOID  archExceptionHandle (ARCH_REG_CTX  *pregctx)
{
    ULONG               ulExcCode = ((pregctx->REG_ulCP0Cause & CAUSEF_EXCCODE) >> CAUSEB_EXCCODE);
    MIPS_EXCEPT_HANDLE  pfuncExceptHandle;

    if (ulExcCode == EXCCODE_INT) {                                     /*  �Ż���ͨ�жϴ���            */
        bspIntHandle();
        return;
    }

    pfuncExceptHandle = _G_mipsExceptHandle[ulExcCode];
    if (pfuncExceptHandle == LW_NULL) {
        _BugFormat(LW_TRUE, LW_TRUE, "Unknown exception: %d\r\n", ulExcCode);

    } else {
        pfuncExceptHandle(pregctx->REG_ulCP0Epc, pregctx->REG_ulCP0BadVAddr, pregctx);
    }
}
/*********************************************************************************************************
  END
*********************************************************************************************************/
