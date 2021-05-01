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
** ��   ��   ��: riscvVfp.c
**
** ��   ��   ��: Jiao.JinXing (������)
**
** �ļ���������: 2018 �� 03 �� 20 ��
**
** ��        ��: RISC-V ��ϵ�ܹ� FPU ֧��.
*********************************************************************************************************/
#define  __SYLIXOS_STDIO
#define  __SYLIXOS_KERNEL
#include "SylixOS.h"
/*********************************************************************************************************
  �ü�֧��
*********************************************************************************************************/
#if LW_CFG_CPU_FPU_EN > 0
#include "../riscvFpu.h"
/*********************************************************************************************************
  ȫ�ֱ���
*********************************************************************************************************/
static RISCV_FPU_OP     _G_fpuopVfp;
/*********************************************************************************************************
  ʵ�ֺ���
*********************************************************************************************************/
extern VOID     riscvVfpEnable(VOID);
extern VOID     riscvVfpDisable(VOID);
extern BOOL     riscvVfpIsEnable(VOID);

extern VOID     riscvVfpSaveSp(PVOID  pvFpuCtx);
extern VOID     riscvVfpRestoreSp(PVOID  pvFpuCtx);

extern VOID     riscvVfpSaveDp(PVOID  pvFpuCtx);
extern VOID     riscvVfpRestoreDp(PVOID  pvFpuCtx);

extern VOID     riscvVfpSaveQp(PVOID  pvFpuCtx);
extern VOID     riscvVfpRestoreQp(PVOID  pvFpuCtx);
/*********************************************************************************************************
** ��������: riscvVfpCtxShowSp
** ��������: ��ʾ Single-Precision VFP ������
** �䡡��  : iFd       ����ļ�������
**           pvFpuCtx  VFP ������
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static VOID  riscvVfpCtxShowSp (INT  iFd, PVOID  pvFpuCtx)
{
#if (LW_CFG_DEVICE_EN > 0) && (LW_CFG_FIO_LIB_EN > 0)
    LW_FPU_CONTEXT  *pfpuCtx    = (LW_FPU_CONTEXT *)pvFpuCtx;
    ARCH_FPU_CTX    *pcpufpuCtx = &pfpuCtx->FPUCTX_fpuctxContext;
    INT              i;

    fdprintf(iFd, "FCSR = 0x%08x\n", pcpufpuCtx->f.FPUCTX_uiFcsr);

    for (i = 0; i < FPU_REG_NR; i++) {
        fdprintf(iFd, "FP%02d = 0x%08x\n", i, pcpufpuCtx->f.FPUCTX_reg[i]);
    }
#endif
}
/*********************************************************************************************************
** ��������: riscvVfpCtxShowDp
** ��������: ��ʾ Double-Precision VFP ������
** �䡡��  : iFd       ����ļ�������
**           pvFpuCtx  VFP ������
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static VOID  riscvVfpCtxShowDp (INT  iFd, PVOID  pvFpuCtx)
{
#if (LW_CFG_DEVICE_EN > 0) && (LW_CFG_FIO_LIB_EN > 0)
    LW_FPU_CONTEXT  *pfpuCtx    = (LW_FPU_CONTEXT *)pvFpuCtx;
    ARCH_FPU_CTX    *pcpufpuCtx = &pfpuCtx->FPUCTX_fpuctxContext;
    INT              i;

    fdprintf(iFd, "FCSR = 0x%08x\n", pcpufpuCtx->d.FPUCTX_uiFcsr);

    for (i = 0; i < FPU_REG_NR; i++) {
        fdprintf(iFd, "FP%02d = 0x%016lx\n", i, pcpufpuCtx->d.FPUCTX_reg[i]);
    }
#endif
}
/*********************************************************************************************************
** ��������: riscvVfpCtxShowQp
** ��������: ��ʾ Quad-Precision VFP ������
** �䡡��  : iFd       ����ļ�������
**           pvFpuCtx  VFP ������
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static VOID  riscvVfpCtxShowQp (INT  iFd, PVOID  pvFpuCtx)
{
#if (LW_CFG_DEVICE_EN > 0) && (LW_CFG_FIO_LIB_EN > 0)
    LW_FPU_CONTEXT  *pfpuCtx    = (LW_FPU_CONTEXT *)pvFpuCtx;
    ARCH_FPU_CTX    *pcpufpuCtx = &pfpuCtx->FPUCTX_fpuctxContext;
    INT              i;

    fdprintf(iFd, "FCSR = 0x%08x\n", pcpufpuCtx->q.FPUCTX_uiFcsr);

    for (i = 0; i < (FPU_REG_NR * 2); i += 2) {
        fdprintf(iFd, "FP%02d = 0x%016lx%016lx\n", i,
                 pcpufpuCtx->q.FPUCTX_reg[i], pcpufpuCtx->q.FPUCTX_reg[i + 1]);
    }
#endif
}
/*********************************************************************************************************
** ��������: riscvVfpEnableTask
** ��������: ϵͳ���� undef �쳣ʱ, ʹ������� VFP
** �䡡��  : ptcbCur    ��ǰ������ƿ�
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static VOID  riscvVfpEnableTask (PLW_CLASS_TCB  ptcbCur)
{
    ARCH_REG_CTX  *pregctx;

    pregctx = &ptcbCur->TCB_archRegCtx;
    pregctx->REG_ulStatus |= SSTATUS_FS;
}
/*********************************************************************************************************
** ��������: riscvVfpPrimaryInit
** ��������: ��ʼ������ȡ VFP ����������������
** �䡡��  : pcMachineName ������
**           pcFpuName     ������������
** �䡡��  : ����������
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
PRISCV_FPU_OP  riscvVfpPrimaryInit (CPCHAR  pcMachineName, CPCHAR  pcFpuName)
{
    if (lib_strcmp(pcFpuName, RISCV_FPU_SP) == 0) {
        _G_fpuopVfp.SFPU_pfuncSave    = riscvVfpSaveSp;
        _G_fpuopVfp.SFPU_pfuncRestore = riscvVfpRestoreSp;
        _G_fpuopVfp.SFPU_pfuncCtxShow = riscvVfpCtxShowSp;

    } else if (lib_strcmp(pcFpuName, RISCV_FPU_DP) == 0) {
        _G_fpuopVfp.SFPU_pfuncSave    = riscvVfpSaveDp;
        _G_fpuopVfp.SFPU_pfuncRestore = riscvVfpRestoreDp;
        _G_fpuopVfp.SFPU_pfuncCtxShow = riscvVfpCtxShowDp;

    } else {
        _G_fpuopVfp.SFPU_pfuncSave    = riscvVfpSaveQp;
        _G_fpuopVfp.SFPU_pfuncRestore = riscvVfpRestoreQp;
        _G_fpuopVfp.SFPU_pfuncCtxShow = riscvVfpCtxShowQp;
    }

    _G_fpuopVfp.SFPU_pfuncEnable     = riscvVfpEnable;
    _G_fpuopVfp.SFPU_pfuncDisable    = riscvVfpDisable;
    _G_fpuopVfp.SFPU_pfuncIsEnable   = riscvVfpIsEnable;
    _G_fpuopVfp.SFPU_pfuncEnableTask = riscvVfpEnableTask;

    return  (&_G_fpuopVfp);
}
/*********************************************************************************************************
** ��������: riscvVfpSecondaryInit
** ��������: ��ʼ�� VFP ������
** �䡡��  : pcMachineName ������
**           pcFpuName     ������������
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
VOID  riscvVfpSecondaryInit (CPCHAR  pcMachineName, CPCHAR  pcFpuName)
{
    (VOID)pcMachineName;
    (VOID)pcFpuName;
}

#endif                                                                  /*  LW_CFG_CPU_FPU_EN > 0       */
/*********************************************************************************************************
  END
*********************************************************************************************************/
