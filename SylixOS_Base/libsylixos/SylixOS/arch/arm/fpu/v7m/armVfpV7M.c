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
** ��   ��   ��: armVfpV7M.c
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2017 �� 11 �� 08 ��
**
** ��        ��: ARM ��ϵ�ܹ� Cortex-Mx VFPv4/5 ֧��.
*********************************************************************************************************/
#define  __SYLIXOS_STDIO
#define  __SYLIXOS_KERNEL
#include "SylixOS.h"
/*********************************************************************************************************
  ARMv7M ��ϵ����
*********************************************************************************************************/
#if defined(__SYLIXOS_ARM_ARCH_M__)
/*********************************************************************************************************
  �ü�֧��
*********************************************************************************************************/
#if LW_CFG_CPU_FPU_EN > 0
#include "../armFpu.h"
/*********************************************************************************************************
  Coprocessor Access Control Register: enable CP10 & CP11 for VFP
*********************************************************************************************************/
#define ARMV7M_CPACR            0xe000ed88
#define ARMV7M_CPACR_CP_FA(x)   (3 << (2 * (x)))                /* 0b11 : Full access for this cp       */
/*********************************************************************************************************
  Floating-Point Context Control Register, FPCCR
*********************************************************************************************************/
#define ARMV7M_FPCCR            0xe000ef34
#define ARMV7M_FPCCR_ASPEN      (1 << 31)                       /* enable auto FP context stacking      */
#define ARMV7M_FPCCR_LSPEN      (1 << 30)                       /* enable lazy FP context stacking      */
#define ARMV7M_FPCCR_MONRDY     (1 << 8)
#define ARMV7M_FPCCR_BFRDY      (1 << 6)
#define ARMV7M_FPCCR_MMRDY      (1 << 5)
#define ARMV7M_FPCCR_HFRDY      (1 << 4)
#define ARMV7M_FPCCR_THREAD     (1 << 3)                        /* allocated FP stack in thread         */
#define ARMV7M_FPCCR_USER       (1 << 1)                        /* allocated FP stack in priviledged    */
#define ARMV7M_FPCCR_LSPACT     (1 << 0)                        /* lazy stacking is active              */
/*********************************************************************************************************
  Floating-Point Context Address Register, FPCAR
  this register contains the FP context address in the exception stack
*********************************************************************************************************/
#define ARMV7M_FPCAR            (0xe000ef38)
/*********************************************************************************************************
  ȫ�ֱ���
*********************************************************************************************************/
static ARM_FPU_OP   _G_fpuopVfpV7M;
/*********************************************************************************************************
  ���ʵ�ֺ���
*********************************************************************************************************/
extern VOID    armVfpV7MSave(PVOID pvFpuCtx);
extern VOID    armVfpV7MRestore(PVOID pvFpuCtx);
/*********************************************************************************************************
** ��������: armVfpV7MEnable
** ��������: ʹ�� VFP
** �䡡��  : NONE
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
VOID  armVfpV7MEnable (VOID)
{
    volatile UINT32 *uiCpacr = (volatile UINT32 *)ARMV7M_CPACR;

    *uiCpacr |= ARMV7M_CPACR_CP_FA(10) | ARMV7M_CPACR_CP_FA(11);
}
/*********************************************************************************************************
** ��������: armVfpV7MDisable
** ��������: ���� VFP
** �䡡��  : NONE
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static VOID  armVfpV7MDisable (VOID)
{
    volatile UINT32 *uiCpacr = (volatile UINT32 *)ARMV7M_CPACR;

    *uiCpacr &= ~(ARMV7M_CPACR_CP_FA(10) | ARMV7M_CPACR_CP_FA(11));
}
/*********************************************************************************************************
** ��������: armVfpV7MIsEnable
** ��������: �Ƿ�ʹ�� VFP
** �䡡��  : NONE
** �䡡��  : �Ƿ�ʹ��
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static BOOL  armVfpV7MIsEnable (VOID)
{
    UINT32  uiVal = *(volatile UINT32 *)ARMV7M_CPACR;

    return  (uiVal & ((ARMV7M_CPACR_CP_FA(10) | ARMV7M_CPACR_CP_FA(11)))) ? (LW_TRUE) : (LW_FALSE);
}
/*********************************************************************************************************
** ��������: armVfpV7MCtxShow
** ��������: ��ʾ VFP ������
** �䡡��  : iFd       ����ļ�������
**           pvFpuCtx  VFP ������
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static VOID  armVfpV7MCtxShow (INT iFd, PVOID pvFpuCtx)
{
#if (LW_CFG_DEVICE_EN > 0) && (LW_CFG_FIO_LIB_EN > 0)
    INT i;

    LW_FPU_CONTEXT  *pfpuCtx    = (LW_FPU_CONTEXT *)pvFpuCtx;
    ARCH_FPU_CTX    *pcpufpuCtx = &pfpuCtx->FPUCTX_fpuctxContext;

    fdprintf(iFd, "FPSCR   = 0x%08x\n", pcpufpuCtx->FPUCTX_uiFpscr);

    for (i = 0; i < 16; i += 2) {
        fdprintf(iFd, "FPS[%02d] = 0x%08x  ", i,     pcpufpuCtx->FPUCTX_uiDreg[i]);
        fdprintf(iFd, "FPS[%02d] = 0x%08x\n", i + 1, pcpufpuCtx->FPUCTX_uiDreg[i + 1]);
    }
#endif
}
/*********************************************************************************************************
** ��������: armVfpV7MPrimaryInit
** ��������: ��ʼ������ȡ VFP ����������������
** �䡡��  : pcMachineName ������
**           pcFpuName     ������������
** �䡡��  : ����������
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
PARM_FPU_OP  armVfpV7MPrimaryInit (CPCHAR  pcMachineName, CPCHAR  pcFpuName)
{
             UINT32  uiVal;
    volatile UINT32 *uiCpacr = (volatile UINT32 *)ARMV7M_CPACR;
    volatile UINT32 *uiFpccr = (volatile UINT32 *)ARMV7M_FPCCR;

    uiVal = *uiCpacr;
    if (uiVal & ((ARMV7M_CPACR_CP_FA(10) | ARMV7M_CPACR_CP_FA(11)))) {
        uiVal &= ~(ARMV7M_CPACR_CP_FA(10) | ARMV7M_CPACR_CP_FA(11));
        *uiCpacr = uiVal;                                       /* disable                              */
    }

    *uiFpccr &= ~(ARMV7M_FPCCR_ASPEN | ARMV7M_FPCCR_LSPEN);     /* disable automaically FP ctx saving.  */

    _G_fpuopVfpV7M.AFPU_pfuncEnable   = armVfpV7MEnable;
    _G_fpuopVfpV7M.AFPU_pfuncDisable  = armVfpV7MDisable;
    _G_fpuopVfpV7M.AFPU_pfuncIsEnable = armVfpV7MIsEnable;
    _G_fpuopVfpV7M.AFPU_pfuncSave     = armVfpV7MSave;
    _G_fpuopVfpV7M.AFPU_pfuncRestore  = armVfpV7MRestore;
    _G_fpuopVfpV7M.AFPU_pfuncCtxShow  = armVfpV7MCtxShow;

    return  (&_G_fpuopVfpV7M);
}
/*********************************************************************************************************
** ��������: armVfpV7MSecondaryInit
** ��������: ��ʼ�� VFP ������
** �䡡��  : pcMachineName ������
**           pcFpuName     ������������
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
VOID  armVfpV7MSecondaryInit (CPCHAR  pcMachineName, CPCHAR  pcFpuName)
{
    (VOID)pcMachineName;
    (VOID)pcFpuName;
}

#endif                                                                  /*  LW_CFG_CPU_FPU_EN > 0       */
#endif                                                                  /*  __SYLIXOS_ARM_ARCH_M__      */
/*********************************************************************************************************
  END
*********************************************************************************************************/
