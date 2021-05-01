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
** ��   ��   ��: armFpu.c
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2013 �� 12 �� 09 ��
**
** ��        ��: ARM ��ϵ�ܹ�Ӳ������������ (VFP).
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "SylixOS.h"
/*********************************************************************************************************
  �ü�֧��
*********************************************************************************************************/
#if LW_CFG_CPU_FPU_EN > 0
#include "armFpu.h"
#include "vfpnone/armVfpNone.h"
#if defined(__SYLIXOS_ARM_ARCH_M__)
#include "v7m/armVfpV7M.h"
#else
#include "vfp9/armVfp9.h"
#include "vfp11/armVfp11.h"
#include "vfpv3/armVfpV3.h"
#include "vfpv4/armVfpV4.h"
#endif                                                                  /*  !__SYLIXOS_ARM_ARCH_M__     */
/*********************************************************************************************************
  ȫ�ֱ���
*********************************************************************************************************/
static LW_FPU_CONTEXT   _G_fpuCtxInit;
static PARM_FPU_OP      _G_pfpuop;
/*********************************************************************************************************
** ��������: archFpuPrimaryInit
** ��������: ���� Fpu ��������ʼ��
** �䡡��  : pcMachineName ��������
**           pcFpuName     fpu ����
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
VOID  archFpuPrimaryInit (CPCHAR  pcMachineName, CPCHAR  pcFpuName)
{
    _DebugFormat(__LOGMESSAGE_LEVEL, "%s %s %s FPU pri-core initialization.\r\n", 
                 LW_CFG_CPU_ARCH_FAMILY, pcMachineName, pcFpuName);

#if defined(__SYLIXOS_ARM_ARCH_M__)
    if (lib_strcmp(pcFpuName, ARM_FPU_NONE) == 0) {                     /*  ѡ�� VFP �ܹ�               */
        _G_pfpuop = armVfpNonePrimaryInit(pcMachineName, pcFpuName);

    } else {
        _G_pfpuop = armVfpV7MPrimaryInit(pcMachineName, pcFpuName);
    }

    lib_bzero(&_G_fpuCtxInit, sizeof(LW_FPU_CONTEXT));

    ARM_VFP_ENABLE(_G_pfpuop);

    ARM_VFP_SAVE(_G_pfpuop, (PVOID)&_G_fpuCtxInit);

    _G_fpuCtxInit.FPUCTX_fpuctxContext.FPUCTX_uiFpscr = 0x01000000;     /*  Set FZ bit in VFP           */

    ARM_VFP_DISABLE(_G_pfpuop);

#else
    if (lib_strcmp(pcFpuName, ARM_FPU_NONE) == 0) {                     /*  ѡ�� VFP �ܹ�               */
        _G_pfpuop = armVfpNonePrimaryInit(pcMachineName, pcFpuName);
    
    } else if ((lib_strcmp(pcFpuName, ARM_FPU_VFP9_D16) == 0) ||
               (lib_strcmp(pcFpuName, ARM_FPU_VFP9_D32) == 0)) {
        _G_pfpuop = armVfp9PrimaryInit(pcMachineName, pcFpuName);
    
    } else if (lib_strcmp(pcFpuName, ARM_FPU_VFP11) == 0) {
        _G_pfpuop = armVfp11PrimaryInit(pcMachineName, pcFpuName);
        
    } else if (lib_strcmp(pcFpuName, ARM_FPU_VFPv3) == 0) {
        _G_pfpuop = armVfpV3PrimaryInit(pcMachineName, pcFpuName);
    
    } else if (lib_strcmp(pcFpuName, ARM_FPU_VFPv4) == 0) {
        _G_pfpuop = armVfpV4PrimaryInit(pcMachineName, pcFpuName);
    
    } else {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "unknown fpu name.\r\n");
        return;
    }
    
    if (_G_pfpuop == LW_NULL) {
        return;
    }

    lib_bzero(&_G_fpuCtxInit, sizeof(LW_FPU_CONTEXT));
    
    ARM_VFP_ENABLE(_G_pfpuop);
    
    _G_fpuCtxInit.FPUCTX_fpuctxContext.FPUCTX_uiFpsid = (UINT32)ARM_VFP_HW_SID(_G_pfpuop);
    ARM_VFP_SAVE(_G_pfpuop, (PVOID)&_G_fpuCtxInit);

    _G_fpuCtxInit.FPUCTX_fpuctxContext.FPUCTX_uiFpscr = 0x01000000;     /*  Set FZ bit in VFP           */
                                                                        /*  Do not enable FPU           */
    ARM_VFP_DISABLE(_G_pfpuop);
#endif                                                                  /*  !__SYLIXOS_ARM_ARCH_M__     */
}
/*********************************************************************************************************
** ��������: archFpuSecondaryInit
** ��������: �Ӻ� Fpu ��������ʼ��
** �䡡��  : pcMachineName ��������
**           pcFpuName     fpu ����
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
#if LW_CFG_SMP_EN > 0

VOID  archFpuSecondaryInit (CPCHAR  pcMachineName, CPCHAR  pcFpuName)
{
    _DebugFormat(__LOGMESSAGE_LEVEL, "%s %s %s FPU sec-core initialization.\r\n", 
                 LW_CFG_CPU_ARCH_FAMILY, pcMachineName, pcFpuName);

#if defined(__SYLIXOS_ARM_ARCH_M__)
    if (lib_strcmp(pcFpuName, ARM_FPU_NONE) == 0) {                     /*  ѡ�� VFP �ܹ�               */
        armVfpNoneSecondaryInit(pcMachineName, pcFpuName);

    } else {
        armVfpV7MSecondaryInit(pcMachineName, pcFpuName);
    }

#else
    if (lib_strcmp(pcFpuName, ARM_FPU_NONE) == 0) {                     /*  ѡ�� VFP �ܹ�               */
        armVfpNoneSecondaryInit(pcMachineName, pcFpuName);
    
    } else if ((lib_strcmp(pcFpuName, ARM_FPU_VFP9_D16) == 0) ||
               (lib_strcmp(pcFpuName, ARM_FPU_VFP9_D32) == 0)) {
        armVfp9SecondaryInit(pcMachineName, pcFpuName);
    
    } else if (lib_strcmp(pcFpuName, ARM_FPU_VFP11) == 0) {
        armVfp11SecondaryInit(pcMachineName, pcFpuName);
        
    } else if (lib_strcmp(pcFpuName, ARM_FPU_VFPv3) == 0) {
        armVfpV3SecondaryInit(pcMachineName, pcFpuName);
    
    } else if (lib_strcmp(pcFpuName, ARM_FPU_VFPv4) == 0) {
        armVfpV4SecondaryInit(pcMachineName, pcFpuName);
    
    } else {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "unknown fpu name.\r\n");
        return;
    }
#endif                                                                  /*  !__SYLIXOS_ARM_ARCH_M__     */
}

#endif                                                                  /*  LW_CFG_SMP_EN               */
/*********************************************************************************************************
** ��������: archFpuCtxInit
** ��������: ��ʼ��һ�� Fpu �����Ŀ��ƿ� (���ﲢû��ʹ�� FPU)
** �䡡��  : pvFpuCtx   FPU ������
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
VOID  archFpuCtxInit (PVOID pvFpuCtx)
{
    lib_memcpy(pvFpuCtx, &_G_fpuCtxInit, sizeof(LW_FPU_CONTEXT));
}
/*********************************************************************************************************
** ��������: archFpuEnable
** ��������: ʹ�� FPU.
** �䡡��  : NONE
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
VOID  archFpuEnable (VOID)
{
    ARM_VFP_ENABLE(_G_pfpuop);
}
/*********************************************************************************************************
** ��������: archFpuDisable
** ��������: ���� FPU.
** �䡡��  : NONE
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
VOID  archFpuDisable (VOID)
{
    ARM_VFP_DISABLE(_G_pfpuop);
}
/*********************************************************************************************************
** ��������: archFpuSave
** ��������: ���� FPU ������.
** �䡡��  : pvFpuCtx  FPU ������
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
VOID  archFpuSave (PVOID pvFpuCtx)
{
    ARM_VFP_SAVE(_G_pfpuop, pvFpuCtx);
}
/*********************************************************************************************************
** ��������: archFpuRestore
** ��������: �ظ� FPU ������.
** �䡡��  : pvFpuCtx  FPU ������
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
VOID  archFpuRestore (PVOID pvFpuCtx)
{
    ARM_VFP_RESTORE(_G_pfpuop, pvFpuCtx);
}
/*********************************************************************************************************
** ��������: archFpuCtxShow
** ��������: ��ʾ FPU ������.
** �䡡��  : iFd       �ļ�������
**           pvFpuCtx  FPU ������
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
VOID  archFpuCtxShow (INT  iFd, PVOID pvFpuCtx)
{
    ARM_VFP_CTXSHOW(_G_pfpuop, iFd, pvFpuCtx);
}
/*********************************************************************************************************
** ��������: archFpuUndHandle
** ��������: ϵͳ���� undef �쳣ʱ, ���ô˺���. 
**           ֻ��ĳ����������ж�, ����ʹ�ø�������ʱ (�����е���������ָ������쳣)
**           ��ʱ�ſ��Դ򿪸��������.
** �䡡��  : ptcbCur   ��ǰ���� TCB
** �䡡��  : ERROR or OK
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
INT  archFpuUndHandle (PLW_CLASS_TCB  ptcbCur)
{
    if (LW_CPU_GET_CUR_NESTING() > 1) {                                 /*  �ж��з����쳣, ���س���    */
        return  (PX_ERROR);
    }

    if (ptcbCur->TCB_ulOption & LW_OPTION_THREAD_USED_FP) {
        return  (PX_ERROR);
    }
    
    ptcbCur->TCB_ulOption |= LW_OPTION_THREAD_USED_FP;
    ARM_VFP_RESTORE(_G_pfpuop, ptcbCur->TCB_pvStackFP);                 /*  ʹ�� FPU, ��ʼ�� FPU �Ĵ��� */
    
    return  (ERROR_NONE);
}

#endif                                                                  /*  LW_CFG_CPU_FPU_EN > 0       */
/*********************************************************************************************************
  END
*********************************************************************************************************/
