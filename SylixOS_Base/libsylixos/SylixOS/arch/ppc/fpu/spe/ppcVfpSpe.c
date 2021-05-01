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
** ��   ��   ��: ppcVfpSpe.c
**
** ��   ��   ��: Jiao.JinXing (������)
**
** �ļ���������: 2016 �� 05 �� 02 ��
**
** ��        ��: PowerPC ��ϵ�ܹ� SPE ֧��.
*********************************************************************************************************/
#define  __SYLIXOS_STDIO
#define  __SYLIXOS_KERNEL
#include "SylixOS.h"
/*********************************************************************************************************
  �ü�֧��
*********************************************************************************************************/
#if LW_CFG_CPU_FPU_EN > 0
#include "../ppcFpu.h"
#include "arch/ppc/arch_e500.h"
/*********************************************************************************************************
  ȫ�ֱ���
*********************************************************************************************************/
static PPC_FPU_OP   _G_fpuopVfpSpe;
/*********************************************************************************************************
  ʵ�ֺ���
*********************************************************************************************************/
extern VOID     ppcVfpSpeEnable(VOID);
extern VOID     ppcVfpSpeDisable(VOID);
extern BOOL     ppcVfpSpeIsEnable(VOID);
extern VOID     ppcVfpSpeSave(PVOID  pvFpuCtx);
extern VOID     ppcVfpSpeRestore(PVOID  pvFpuCtx);
/*********************************************************************************************************
** ��������: ppcVfpSpeCtxShow
** ��������: ��ʾ VFP ������
** �䡡��  : iFd       ����ļ�������
**           pvFpuCtx  VFP ������
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static VOID  ppcVfpSpeCtxShow (INT  iFd, PVOID  pvFpuCtx)
{
#if (LW_CFG_DEVICE_EN > 0) && (LW_CFG_FIO_LIB_EN > 0)
    LW_FPU_CONTEXT *pfpuCtx    = (LW_FPU_CONTEXT *)pvFpuCtx;
    ARCH_FPU_CTX   *pcpufpuCtx = &pfpuCtx->FPUCTX_fpuctxContext;
    INT             i;

    fdprintf(iFd, "Upper 32 bits of GPR\n");

    for (i = 0; i < SPE_GPR_NR; i += 4) {
        fdprintf(iFd, "R%02d 0x%08x R%02d 0x%08x R%02d 0x%08x R%02d 0x%08x\n",
                 i,     pcpufpuCtx->SPECTX_uiGpr[i],
                 i + 1, pcpufpuCtx->SPECTX_uiGpr[i + 1],
                 i + 2, pcpufpuCtx->SPECTX_uiGpr[i + 2],
                 i + 3, pcpufpuCtx->SPECTX_uiGpr[i + 3]);
    }

    fdprintf(iFd, "\nAccL = 0x%08x, AccH = 0x%08x\n",
             pcpufpuCtx->SPECTX_uiAcc[0],
             pcpufpuCtx->SPECTX_uiAcc[1]);
#endif
}
/*********************************************************************************************************
** ��������: ppcVfpSpeEnableTask
** ��������: ϵͳ���� undef �쳣ʱ, ʹ������� VFP
** �䡡��  : ptcbCur    ��ǰ������ƿ�
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static VOID  ppcVfpSpeEnableTask (PLW_CLASS_TCB  ptcbCur)
{
    ARCH_REG_CTX  *pregctx;
    ARCH_FPU_CTX  *pfpuctx;

    pregctx = &ptcbCur->TCB_archRegCtx;
    pregctx->REG_uiSrr1 |= ARCH_PPC_MSR_SPE;

    pfpuctx = &ptcbCur->TCB_fpuctxContext.FPUCTX_fpuctxContext;
    pfpuctx->SPECTX_uiSpefscr = 0ul;
}
/*********************************************************************************************************
** ��������: ppcVfpSpePrimaryInit
** ��������: ��ȡ VFP ����������������
** �䡡��  : pcMachineName ������
**           pcFpuName     ������������
** �䡡��  : ����������
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
PPPC_FPU_OP  ppcVfpSpePrimaryInit (CPCHAR  pcMachineName, CPCHAR  pcFpuName)
{
    _G_fpuopVfpSpe.PFPU_pfuncEnable     = ppcVfpSpeEnable;
    _G_fpuopVfpSpe.PFPU_pfuncDisable    = ppcVfpSpeDisable;
    _G_fpuopVfpSpe.PFPU_pfuncIsEnable   = ppcVfpSpeIsEnable;
    _G_fpuopVfpSpe.PFPU_pfuncSave       = ppcVfpSpeSave;
    _G_fpuopVfpSpe.PFPU_pfuncRestore    = ppcVfpSpeRestore;
    _G_fpuopVfpSpe.PFPU_pfuncCtxShow    = ppcVfpSpeCtxShow;
    _G_fpuopVfpSpe.PFPU_pfuncEnableTask = ppcVfpSpeEnableTask;

    return  (&_G_fpuopVfpSpe);
}
/*********************************************************************************************************
** ��������: ppcVfpSpeSecondaryInit
** ��������: ��ʼ�� VFP ������
** �䡡��  : pcMachineName ������
**           pcFpuName     ������������
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
VOID  ppcVfpSpeSecondaryInit (CPCHAR  pcMachineName, CPCHAR  pcFpuName)
{
    (VOID)pcMachineName;
    (VOID)pcFpuName;
}

#endif                                                                  /*  LW_CFG_CPU_FPU_EN > 0       */
/*********************************************************************************************************
  END
*********************************************************************************************************/
