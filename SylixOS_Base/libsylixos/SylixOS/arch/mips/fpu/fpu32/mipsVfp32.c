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
** ��   ��   ��: mipsVfp32.c
**
** ��   ��   ��: Jiao.JinXing (������)
**
** �ļ���������: 2015 �� 11 �� 17 ��
**
** ��        ��: MIPS ��ϵ�ܹ� VFP32 ֧��.
*********************************************************************************************************/
#define  __SYLIXOS_STDIO
#define  __SYLIXOS_KERNEL
#include "SylixOS.h"
/*********************************************************************************************************
  �ü�֧��
*********************************************************************************************************/
#if LW_CFG_CPU_FPU_EN > 0
#include "../mipsFpu.h"
/*********************************************************************************************************
  ȫ�ֱ���
*********************************************************************************************************/
static MIPS_FPU_OP  _G_fpuopVfp32;
/*********************************************************************************************************
  ʵ�ֺ���
*********************************************************************************************************/
extern VOID    mipsVfp32Init(VOID);
extern ULONG   mipsVfp32GetFIR(VOID);
extern VOID    mipsVfp32Enable(VOID);
extern VOID    mipsVfp32Disable(VOID);
extern BOOL    mipsVfp32IsEnable(VOID);
extern VOID    mipsVfp32Save(PVOID  pvFpuCtx);
extern VOID    mipsVfp32Restore(PVOID  pvFpuCtx);
/*********************************************************************************************************
** ��������: mipsVfp32CtxShow
** ��������: ��ʾ VFP ������
** �䡡��  : iFd       ����ļ�������
**           pvFpuCtx  VFP ������
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static VOID  mipsVfp32CtxShow (INT  iFd, PVOID  pvFpuCtx)
{
#if (LW_CFG_DEVICE_EN > 0) && (LW_CFG_FIO_LIB_EN > 0)
    LW_FPU_CONTEXT  *pfpuCtx    = (LW_FPU_CONTEXT *)pvFpuCtx;
    ARCH_FPU_CTX    *pcpufpuCtx = &pfpuCtx->FPUCTX_fpuctxContext;
    INT              i;

    fdprintf(iFd, "FCSR = 0x%08x\n", pcpufpuCtx->FPUCTX_uiFcsr);

    for (i = 0; i < FPU_REG_NR; i++) {
        fdprintf(iFd, "FP%02d = 0x%08x%08x\n", i,
                 pcpufpuCtx->FPUCTX_reg[i].val32[0],
                 pcpufpuCtx->FPUCTX_reg[i].val32[1]);
    }
#endif
}
/*********************************************************************************************************
** ��������: mipsVfp32EnableTask
** ��������: ϵͳ���� FPU �������쳣ʱ, ʹ������� VFP
** �䡡��  : ptcbCur    ��ǰ������ƿ�
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static VOID  mipsVfp32EnableTask (PLW_CLASS_TCB  ptcbCur)
{
    ARCH_REG_CTX  *pregctx;

    pregctx = &ptcbCur->TCB_archRegCtx;
    pregctx->REG_ulCP0Status |= ST0_CU1;

    /*
     * GS464E �еĸ���Ĵ�����Ϯ MIPS R4000/R10000 �������÷����� MIPS64 �淶���в�ͬ���� Status ��
     * �ƼĴ����� FR λΪ 0 ʱ��GS464E ֻ�� 16 �� 32 λ�� 64 λ�ĸ���Ĵ������Ҹ���Ĵ����ű���Ϊż����
     * �� MIPS64 ��ʾ�� 32 �� 32 λ�ĸ���Ĵ����� 16 �� 64 λ�ĸ���Ĵ�����
     * �� Status ���ƼĴ����� FR λΪ 1 ʱ��
     * GS464E ʹ�ø���Ĵ����ķ����� MIPS64 �淶һ�£����� 32 �� 64 λ�ĸ���Ĵ�����
     */

#if LW_CFG_CPU_WORD_LENGHT == 64
    pregctx->REG_ulCP0Status |= ST0_FR;                                 /*  32 ������Ĵ���             */
#else
    pregctx->REG_ulCP0Status &= ~ST0_FR;                                /*  16 ������Ĵ���             */
#endif                                                                  /*  LW_CFG_CPU_WORD_LENGHT == 64*/
}
/*********************************************************************************************************
** ��������: mipsVfp32PrimaryInit
** ��������: ��ʼ������ȡ VFP ����������������
** �䡡��  : pcMachineName ������
**           pcFpuName     ������������
** �䡡��  : ����������
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
PMIPS_FPU_OP  mipsVfp32PrimaryInit (CPCHAR  pcMachineName, CPCHAR  pcFpuName)
{
    (VOID)pcMachineName;
    (VOID)pcFpuName;

    mipsVfp32Init();

    _G_fpuopVfp32.MFPU_pfuncEnable     = mipsVfp32Enable;
    _G_fpuopVfp32.MFPU_pfuncDisable    = mipsVfp32Disable;
    _G_fpuopVfp32.MFPU_pfuncIsEnable   = mipsVfp32IsEnable;
    _G_fpuopVfp32.MFPU_pfuncCtxShow    = mipsVfp32CtxShow;
    _G_fpuopVfp32.MFPU_pfuncSave       = mipsVfp32Save;
    _G_fpuopVfp32.MFPU_pfuncRestore    = mipsVfp32Restore;
    _G_fpuopVfp32.MFPU_pfuncGetFIR     = mipsVfp32GetFIR;
    _G_fpuopVfp32.MFPU_pfuncEnableTask = mipsVfp32EnableTask;

    return  (&_G_fpuopVfp32);
}
/*********************************************************************************************************
** ��������: mipsVfp32SecondaryInit
** ��������: ��ʼ�� VFP ������
** �䡡��  : pcMachineName ������
**           pcFpuName     ������������
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
VOID  mipsVfp32SecondaryInit (CPCHAR  pcMachineName, CPCHAR  pcFpuName)
{
    (VOID)pcMachineName;
    (VOID)pcFpuName;

    mipsVfp32Init();
}

#endif                                                                  /*  LW_CFG_CPU_FPU_EN > 0       */
/*********************************************************************************************************
  END
*********************************************************************************************************/
