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
** ��   ��   ��: arm64VfpV4.c
**
** ��   ��   ��: Wang.Xuan (���Q)
**
** �ļ���������: 2018 �� 07 �� 03 ��
**
** ��        ��: ARM64 ��ϵ�ܹ� VFPv4 ֧��.
*********************************************************************************************************/
#define  __SYLIXOS_STDIO
#define  __SYLIXOS_KERNEL
#include "SylixOS.h"
/*********************************************************************************************************
  �ü�֧��
*********************************************************************************************************/
#if LW_CFG_CPU_FPU_EN > 0
#include "../arm64Fpu.h"
/*********************************************************************************************************
  ȫ�ֱ���
*********************************************************************************************************/
static ARM64_FPU_OP     _G_fpuopVfpV4;
static INT              _G_iVfpV4DNum;
/*********************************************************************************************************
  ʵ�ֺ���
*********************************************************************************************************/
extern VOID     arm64VfpV4HwInit(VOID);
extern UINT32   arm64VfpV4Mvfr0EL1(VOID);
extern VOID     arm64VfpV4Enable(VOID);
extern VOID     arm64VfpV4Disable(VOID);
extern BOOL     arm64VfpV4IsEnable(VOID);
extern VOID     arm64VfpV4Save(PVOID pvFpuCtx);
extern VOID     arm64VfpV4Restore(PVOID pvFpuCtx);
/*********************************************************************************************************
** ��������: arm64VfpV4CtxShow
** ��������: ��ʾ VFP ������
** �䡡��  : iFd       ����ļ�������
**           pvFpuCtx  VFP ������
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static VOID  arm64VfpV4CtxShow (INT  iFd, PVOID  pvFpuCtx)
{
#if (LW_CFG_DEVICE_EN > 0) && (LW_CFG_FIO_LIB_EN > 0)
    INT              i;
    LW_FPU_CONTEXT  *pfpuCtx    = (LW_FPU_CONTEXT *)pvFpuCtx;
    ARCH_FPU_CTX    *pcpufpuCtx = &pfpuCtx->FPUCTX_fpuctxContext;
    
    fdprintf(iFd,    "FPCR      = 0x%08x  ", pcpufpuCtx->FPUCTX_uiFpcr);
    fdprintf(iFd,    "FPSR      = 0x%08x\n", pcpufpuCtx->FPUCTX_uiFpsr);
  
    for (i = 0; i < _G_iVfpV4DNum; i += 2) {
        fdprintf(iFd, "FPS[%02d] = 0x%08x  ", i,     pcpufpuCtx->FPUCTX_uiDreg[i]);
        fdprintf(iFd, "FPS[%02d] = 0x%08x\n", i + 1, pcpufpuCtx->FPUCTX_uiDreg[i + 1]);
    }
#endif
}
/*********************************************************************************************************
** ��������: arm64VfpV4PrimaryInit
** ��������: ��ʼ������ȡ VFP ����������������
** �䡡��  : pcMachineName ������
**           pcFpuName     ������������
** �䡡��  : ����������
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
PARM64_FPU_OP  arm64VfpV4PrimaryInit (CPCHAR  pcMachineName, CPCHAR  pcFpuName)
{
    UINT32  uiMvfr0;
    
    arm64VfpV4HwInit();
    
    uiMvfr0  = arm64VfpV4Mvfr0EL1();
    uiMvfr0 &= 0xf;
    
    if (uiMvfr0 == 0x2) {
        _G_iVfpV4DNum = 64;
        _G_fpuopVfpV4.AFPU_pfuncSave    = arm64VfpV4Save;
        _G_fpuopVfpV4.AFPU_pfuncRestore = arm64VfpV4Restore;
    
    } else {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "unknown vfp register number.\r\n");
        return  (LW_NULL);
    }
    
    _G_fpuopVfpV4.AFPU_pfuncEnable   = arm64VfpV4Enable;
    _G_fpuopVfpV4.AFPU_pfuncDisable  = arm64VfpV4Disable;
    _G_fpuopVfpV4.AFPU_pfuncIsEnable = arm64VfpV4IsEnable;
    _G_fpuopVfpV4.AFPU_pfuncCtxShow  = arm64VfpV4CtxShow;

    return  (&_G_fpuopVfpV4);
}
/*********************************************************************************************************
** ��������: arm64VfpV4SecondaryInit
** ��������: ��ʼ�� VFP ������
** �䡡��  : pcMachineName ������
**           pcFpuName     ������������
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
VOID  arm64VfpV4SecondaryInit (CPCHAR  pcMachineName, CPCHAR  pcFpuName)
{
    (VOID)pcMachineName;
    (VOID)pcFpuName;
    
    arm64VfpV4HwInit();
}

#endif                                                                  /*  LW_CFG_CPU_FPU_EN > 0       */
/*********************************************************************************************************
  END
*********************************************************************************************************/
