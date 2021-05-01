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
** ��   ��   ��: armVfpV4.c
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2013 �� 12 �� 09 ��
**
** ��        ��: ARM ��ϵ�ܹ� VFPv4 ֧��.
*********************************************************************************************************/
#define  __SYLIXOS_STDIO
#define  __SYLIXOS_KERNEL
#include "SylixOS.h"
/*********************************************************************************************************
  ARMv7A, R ��ϵ����
*********************************************************************************************************/
#if !defined(__SYLIXOS_ARM_ARCH_M__)
/*********************************************************************************************************
  �ü�֧��
*********************************************************************************************************/
#if LW_CFG_CPU_FPU_EN > 0
#include "../armFpu.h"
/*********************************************************************************************************
  ȫ�ֱ���
*********************************************************************************************************/
static ARM_FPU_OP   _G_fpuopVfpV4;
static INT          _G_iVfpV4DNum;
/*********************************************************************************************************
  ʵ�ֺ���
*********************************************************************************************************/
extern VOID     armVfp11NonSecEn(VOID);
extern VOID     armVfp11HwInit(VOID);
extern UINT32   armVfp11Mvfr0(VOID);
extern ULONG    armVfp9Sid(VOID);
extern VOID     armVfp9Enable(VOID);
extern VOID     armVfp9Disable(VOID);
extern BOOL     armVfp9IsEnable(VOID);
extern VOID     armVfpV3Save16(PVOID pvFpuCtx);
extern VOID     armVfpV3Restore16(PVOID pvFpuCtx);
extern VOID     armVfpV3Save32(PVOID pvFpuCtx);
extern VOID     armVfpV3Restore32(PVOID pvFpuCtx);
/*********************************************************************************************************
** ��������: armVfpV4CtxShow
** ��������: ��ʾ VFP ������
** �䡡��  : iFd       ����ļ�������
**           pvFpuCtx  VFP ������
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static VOID  armVfpV4CtxShow (INT iFd, PVOID pvFpuCtx)
{
#if (LW_CFG_DEVICE_EN > 0) && (LW_CFG_FIO_LIB_EN > 0)
    INT i;
    
    LW_FPU_CONTEXT  *pfpuCtx    = (LW_FPU_CONTEXT *)pvFpuCtx;
    ARCH_FPU_CTX    *pcpufpuCtx = &pfpuCtx->FPUCTX_fpuctxContext;
    
    fdprintf(iFd, "FPSID   = 0x%08x\n", pcpufpuCtx->FPUCTX_uiFpsid);
    fdprintf(iFd, "FPSCR   = 0x%08x  ", pcpufpuCtx->FPUCTX_uiFpscr);
    fdprintf(iFd, "FPEXC   = 0x%08x\n", pcpufpuCtx->FPUCTX_uiFpexc);
    fdprintf(iFd, "FPINST  = 0x%08x  ", pcpufpuCtx->FPUCTX_uiFpinst);
    fdprintf(iFd, "FPINST2 = 0x%08x\n", pcpufpuCtx->FPUCTX_uiFpinst2);
    fdprintf(iFd, "MFVFR0  = 0x%08x  ", pcpufpuCtx->FPUCTX_uiMfvfr0);
    fdprintf(iFd, "MFVFR1  = 0x%08x\n", pcpufpuCtx->FPUCTX_uiMfvfr1);

    for (i = 0; i < _G_iVfpV4DNum; i += 2) {
        fdprintf(iFd, "FPS[%02d] = 0x%08x  ", i,     pcpufpuCtx->FPUCTX_uiDreg[i]);
        fdprintf(iFd, "FPS[%02d] = 0x%08x\n", i + 1, pcpufpuCtx->FPUCTX_uiDreg[i + 1]);
    }
#endif
}
/*********************************************************************************************************
** ��������: armVfpV4PrimaryInit
** ��������: ��ʼ������ȡ VFP ����������������
** �䡡��  : pcMachineName ������
**           pcFpuName     ������������
** �䡡��  : ����������
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
PARM_FPU_OP  armVfpV4PrimaryInit (CPCHAR  pcMachineName, CPCHAR  pcFpuName)
{
    UINT32  uiMvfr0;
    
    armVfp11HwInit();
    
#if LW_CFG_CPU_FPU_NONSEC_EN > 0
    armVfp11NonSecEn();
#endif                                                                  /*  LW_CFG_CPU_FPU_NONSEC_EN    */
    
    uiMvfr0  = armVfp11Mvfr0();
    uiMvfr0 &= 0xf;
    
    if (uiMvfr0 == 0x1) {
        _G_iVfpV4DNum = 32;
        _G_fpuopVfpV4.AFPU_pfuncSave    = armVfpV3Save16;
        _G_fpuopVfpV4.AFPU_pfuncRestore = armVfpV3Restore16;
    
    } else if (uiMvfr0 == 0x2) {
        _G_iVfpV4DNum = 64;
        _G_fpuopVfpV4.AFPU_pfuncSave    = armVfpV3Save32;
        _G_fpuopVfpV4.AFPU_pfuncRestore = armVfpV3Restore32;
    
    } else {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "unknown vfp register number.\r\n");
        return  (LW_NULL);
    }

    _G_fpuopVfpV4.AFPU_pfuncHwSid    = armVfp9Sid;
    _G_fpuopVfpV4.AFPU_pfuncEnable   = armVfp9Enable;
    _G_fpuopVfpV4.AFPU_pfuncDisable  = armVfp9Disable;
    _G_fpuopVfpV4.AFPU_pfuncIsEnable = armVfp9IsEnable;
    _G_fpuopVfpV4.AFPU_pfuncCtxShow  = armVfpV4CtxShow;

    return  (&_G_fpuopVfpV4);
}
/*********************************************************************************************************
** ��������: armVfpV4SecondaryInit
** ��������: ��ʼ�� VFP ������
** �䡡��  : pcMachineName ������
**           pcFpuName     ������������
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
VOID  armVfpV4SecondaryInit (CPCHAR  pcMachineName, CPCHAR  pcFpuName)
{
    (VOID)pcMachineName;
    (VOID)pcFpuName;
    
    armVfp11HwInit();

#if LW_CFG_CPU_FPU_NONSEC_EN > 0
    armVfp11NonSecEn();
#endif                                                                  /*  LW_CFG_CPU_FPU_NONSEC_EN    */
}

#endif                                                                  /*  LW_CFG_CPU_FPU_EN > 0       */
#endif                                                                  /*  !__SYLIXOS_ARM_ARCH_M__     */
/*********************************************************************************************************
  END
*********************************************************************************************************/
