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
** ��   ��   ��: sparcVfp.c
**
** ��   ��   ��: Jiao.JinXing (������)
**
** �ļ���������: 2017 �� 09 �� 29 ��
**
** ��        ��: SPARC ��ϵ�ܹ� FPU ֧��.
*********************************************************************************************************/
#define  __SYLIXOS_STDIO
#define  __SYLIXOS_KERNEL
#include "SylixOS.h"
/*********************************************************************************************************
  �ü�֧��
*********************************************************************************************************/
#if LW_CFG_CPU_FPU_EN > 0
#include "../sparcFpu.h"
/*********************************************************************************************************
  ȫ�ֱ���
*********************************************************************************************************/
static SPARC_FPU_OP     _G_fpuopVfp;
/*********************************************************************************************************
  ʵ�ֺ���
*********************************************************************************************************/
extern VOID     sparcVfpEnable(VOID);
extern VOID     sparcVfpDisable(VOID);
extern BOOL     sparcVfpIsEnable(VOID);
extern VOID     sparcVfpSave(PVOID  pvFpuCtx);
extern VOID     sparcVfpRestore(PVOID  pvFpuCtx);
/*********************************************************************************************************
** ��������: sparcVfpCtxShow
** ��������: ��ʾ VFP ������
** �䡡��  : iFd       ����ļ�������
**           pvFpuCtx  VFP ������
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static VOID  sparcVfpCtxShow (INT  iFd, PVOID  pvFpuCtx)
{
#if (LW_CFG_DEVICE_EN > 0) && (LW_CFG_FIO_LIB_EN > 0)
    LW_FPU_CONTEXT *pfpuCtx    = (LW_FPU_CONTEXT *)pvFpuCtx;
    ARCH_FPU_CTX   *pcpufpuCtx = &pfpuCtx->FPUCTX_fpuctxContext;
    INT             i;

    fdprintf(iFd, "FPSCR   = 0x%08x\n", pcpufpuCtx->FPUCTX_uiFpscr);

    for (i = 0; i < FP_DREG_NR; i += 2) {
        fdprintf(iFd, "FPR%02d = %lf, FPR%02d = %lf\n",
                 i,     pcpufpuCtx->FPUCTX_dfDreg[i],
                 i + 1, pcpufpuCtx->FPUCTX_dfDreg[i + 1]);
    }
#endif
}
/*********************************************************************************************************
** ��������: sparcVfpEnableTask
** ��������: ϵͳ���� undef �쳣ʱ, ʹ������� VFP
** �䡡��  : ptcbCur    ��ǰ������ƿ�
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static VOID  sparcVfpEnableTask (PLW_CLASS_TCB  ptcbCur)
{
    ARCH_REG_CTX  *pregctx;
    ARCH_FPU_CTX  *pfpuctx;

    pregctx = &ptcbCur->TCB_archRegCtx;
    pregctx->REG_uiPsr |= PSR_EF;

    pfpuctx = &ptcbCur->TCB_fpuctxContext.FPUCTX_fpuctxContext;
    /*
     * FSR_rounding_direction     = 0 (Nearest (even, if tie))
     * FSR_trap_enable_mask (TEM) = 0 (disable fp_exception trap)
     * FSR_nonstandard_fp (NS)    = 0 (FPU no produce implementation-defined results that
     *                                 may not correspond to ANSI/IEEE Standard 754-1985)
     */
    /*
     * <<LEON user��s manual>> 2.11 FPU interface:
     * The direct FPU interface does not implement a floating-point queue, the processor is stopped
     * during the execution of floating-point instructions. This means that QNE bit in the %fsr
     * register always is zero, and any attempts of executing the STDFQ instruction will generate a
     * FPU exception trap.
     */
    pfpuctx->FPUCTX_uiFpscr = 0ul;
}
/*********************************************************************************************************
** ��������: sparcVfpPrimaryInit
** ��������: ��ʼ������ȡ VFP ����������������
** �䡡��  : pcMachineName ������
**           pcFpuName     ������������
** �䡡��  : ����������
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
PSPARC_FPU_OP  sparcVfpPrimaryInit (CPCHAR  pcMachineName, CPCHAR  pcFpuName)
{
    _G_fpuopVfp.SFPU_pfuncSave       = sparcVfpSave;
    _G_fpuopVfp.SFPU_pfuncRestore    = sparcVfpRestore;
    _G_fpuopVfp.SFPU_pfuncEnable     = sparcVfpEnable;
    _G_fpuopVfp.SFPU_pfuncDisable    = sparcVfpDisable;
    _G_fpuopVfp.SFPU_pfuncIsEnable   = sparcVfpIsEnable;
    _G_fpuopVfp.SFPU_pfuncCtxShow    = sparcVfpCtxShow;
    _G_fpuopVfp.SFPU_pfuncEnableTask = sparcVfpEnableTask;

    return  (&_G_fpuopVfp);
}
/*********************************************************************************************************
** ��������: sparcVfpSecondaryInit
** ��������: ��ʼ�� VFP ������
** �䡡��  : pcMachineName ������
**           pcFpuName     ������������
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
VOID  sparcVfpSecondaryInit (CPCHAR  pcMachineName, CPCHAR  pcFpuName)
{
    (VOID)pcMachineName;
    (VOID)pcFpuName;
}

#endif                                                                  /*  LW_CFG_CPU_FPU_EN > 0       */
/*********************************************************************************************************
  END
*********************************************************************************************************/
