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
** ��   ��   ��: riscvFpu.c
**
** ��   ��   ��: Jiao.JinXing (������)
**
** �ļ���������: 2018 �� 03 �� 20 ��
**
** ��        ��: RISC-V ��ϵ�ܹ�Ӳ������������ (VFP).
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "SylixOS.h"
/*********************************************************************************************************
  �ü�֧��
*********************************************************************************************************/
#if LW_CFG_CPU_FPU_EN > 0
#include "riscvFpu.h"
#include "vfpnone/riscvVfpNone.h"
#include "vfp/riscvVfp.h"
/*********************************************************************************************************
  ȫ�ֱ���
*********************************************************************************************************/
static LW_FPU_CONTEXT   _G_fpuCtxInit;
static PRISCV_FPU_OP    _G_pfpuop;
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

    if (lib_strcmp(pcFpuName, RISCV_FPU_NONE) == 0) {                   /*  ѡ�� VFP �ܹ�               */
        _G_pfpuop = riscvVfpNonePrimaryInit(pcMachineName, pcFpuName);

    } else if ((lib_strcmp(pcFpuName, RISCV_FPU_SP) == 0) ||
               (lib_strcmp(pcFpuName, RISCV_FPU_DP) == 0) ||
               (lib_strcmp(pcFpuName, RISCV_FPU_QP) == 0)) {
        _G_pfpuop = riscvVfpPrimaryInit(pcMachineName, pcFpuName);

    } else {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "unknown fpu name.\r\n");
        return;
    }

    if (_G_pfpuop == LW_NULL) {
        return;
    }

    lib_bzero(&_G_fpuCtxInit, sizeof(LW_FPU_CONTEXT));

    RISCV_VFP_ENABLE(_G_pfpuop);

    RISCV_VFP_SAVE(_G_pfpuop, (PVOID)&_G_fpuCtxInit);

    RISCV_VFP_DISABLE(_G_pfpuop);
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

    if (lib_strcmp(pcFpuName, RISCV_FPU_NONE) == 0) {                   /*  ѡ�� VFP �ܹ�               */
        riscvVfpNoneSecondaryInit(pcMachineName, pcFpuName);

    } else if ((lib_strcmp(pcFpuName, RISCV_FPU_SP) == 0) ||
               (lib_strcmp(pcFpuName, RISCV_FPU_DP) == 0) ||
               (lib_strcmp(pcFpuName, RISCV_FPU_QP) == 0)) {
        riscvVfpSecondaryInit(pcMachineName, pcFpuName);

    } else {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "unknown fpu name.\r\n");
        return;
    }
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
VOID  archFpuCtxInit (PVOID  pvFpuCtx)
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
    RISCV_VFP_ENABLE(_G_pfpuop);
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
    RISCV_VFP_DISABLE(_G_pfpuop);
}
/*********************************************************************************************************
** ��������: archFpuSave
** ��������: ���� FPU ������.
** �䡡��  : pvFpuCtx  FPU ������
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
VOID  archFpuSave (PVOID  pvFpuCtx)
{
    RISCV_VFP_SAVE(_G_pfpuop, pvFpuCtx);
}
/*********************************************************************************************************
** ��������: archFpuRestore
** ��������: �ظ� FPU ������.
** �䡡��  : pvFpuCtx  FPU ������
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
VOID  archFpuRestore (PVOID  pvFpuCtx)
{
    RISCV_VFP_RESTORE(_G_pfpuop, pvFpuCtx);
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
VOID  archFpuCtxShow (INT  iFd, PVOID  pvFpuCtx)
{
    RISCV_VFP_CTXSHOW(_G_pfpuop, iFd, pvFpuCtx);
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

    RISCV_VFP_ENABLE_TASK(_G_pfpuop, ptcbCur);                          /*  ����ʹ�� FPU                */
    ptcbCur->TCB_ulOption |= LW_OPTION_THREAD_USED_FP;

    RISCV_VFP_RESTORE(_G_pfpuop, ptcbCur->TCB_pvStackFP);               /*  ʹ�� FPU, ��ʼ�� FPU �Ĵ��� */

    return  (ERROR_NONE);
}

#endif                                                                  /*  LW_CFG_CPU_FPU_EN > 0       */
/*********************************************************************************************************
  END
*********************************************************************************************************/
