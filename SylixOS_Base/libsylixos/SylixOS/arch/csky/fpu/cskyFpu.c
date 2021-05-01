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
** ��   ��   ��: cskyFpu.c
**
** ��   ��   ��: Wang.Xuan (���Q)
**
** �ļ���������: 2018 �� 05 �� 14 ��
**
** ��        ��: C-SKY ��ϵ�ܹ�Ӳ������������ (VFP).
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "SylixOS.h"
/*********************************************************************************************************
  �ü�֧��
*********************************************************************************************************/
#if LW_CFG_CPU_FPU_EN > 0
#include "cskyFpu.h"
#include "fpu/cskyVfp.h"
#include "vfpnone/cskyVfpNone.h"
#include "arch/csky/inc/cskyregs.h"
/*********************************************************************************************************
  ȫ�ֱ���
*********************************************************************************************************/
static LW_FPU_CONTEXT   _G_fpuCtxInit;
static PCSKY_FPU_OP     _G_pfpuop;
/*********************************************************************************************************
** ��������: archFpuPrimaryInit
** ��������: ���� FPU ��������ʼ��
** �䡡��  : pcMachineName ��������
**           pcFpuName     FPU ����
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
VOID  archFpuPrimaryInit (CPCHAR  pcMachineName, CPCHAR  pcFpuName)
{
    _DebugFormat(__LOGMESSAGE_LEVEL, "%s %s %s FPU pri-core initialization.\r\n",
                 LW_CFG_CPU_ARCH_FAMILY, pcMachineName, pcFpuName);

    if (lib_strcmp(pcFpuName, CSKY_FPU_NONE) == 0) {                    /*  ѡ�� VFP �ܹ�               */
        _G_pfpuop = cskyVfpNonePrimaryInit(pcMachineName, pcFpuName);

    } else if ((lib_strcmp(pcFpuName, CSKY_FPU_FPUV1) == 0) ||
               (lib_strcmp(pcFpuName, CSKY_FPU_FPUV2) == 0)) {
        _G_pfpuop = cskyVfpPrimaryInit(pcMachineName, pcFpuName);

    } else {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "unknown fpu name.\r\n");
        return;
    }

    if (_G_pfpuop == LW_NULL) {
        return;
    }

    lib_bzero(&_G_fpuCtxInit, sizeof(LW_FPU_CONTEXT));

    CSKY_VFP_ENABLE(_G_pfpuop);

    CSKY_VFP_SAVE(_G_pfpuop, (PVOID)&_G_fpuCtxInit);

    CSKY_VFP_DISABLE(_G_pfpuop);
}
/*********************************************************************************************************
** ��������: archFpuSecondaryInit
** ��������: �Ӻ� FPU ��������ʼ��
** �䡡��  : pcMachineName ��������
**           pcFpuName     FPU ����
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
#if LW_CFG_SMP_EN > 0

VOID  archFpuSecondaryInit (CPCHAR  pcMachineName, CPCHAR  pcFpuName)
{
    _DebugFormat(__LOGMESSAGE_LEVEL, "%s %s %s FPU sec-core initialization.\r\n",
                 LW_CFG_CPU_ARCH_FAMILY, pcMachineName, pcFpuName);

    if (lib_strcmp(pcFpuName, CSKY_FPU_NONE) == 0) {                    /*  ѡ�� VFP �ܹ�               */
        cskyVfpNoneSecondaryInit(pcMachineName, pcFpuName);

    } else if ((lib_strcmp(pcFpuName, CSKY_FPU_FPUV1) == 0) ||
               (lib_strcmp(pcFpuName, CSKY_FPU_FPUV2) == 0)) {
        cskyVfpSecondaryInit(pcMachineName, pcFpuName);

    } else {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "unknown fpu name.\r\n");
        return;
    }

    CSKY_VFP_DISABLE(_G_pfpuop);
}

#endif                                                                  /*  LW_CFG_SMP_EN               */
/*********************************************************************************************************
** ��������: archFpuCtxInit
** ��������: ��ʼ��һ�� FPU �����Ŀ��ƿ� (���ﲢû��ʹ�� FPU)
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
    CSKY_VFP_ENABLE(_G_pfpuop);
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
    CSKY_VFP_DISABLE(_G_pfpuop);
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
    CSKY_VFP_SAVE(_G_pfpuop, pvFpuCtx);
}
/*********************************************************************************************************
** ��������: archFpuRestore
** ��������: �ָ� FPU ������.
** �䡡��  : pvFpuCtx  FPU ������
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
VOID  archFpuRestore (PVOID  pvFpuCtx)
{
    CSKY_VFP_RESTORE(_G_pfpuop, pvFpuCtx);
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
    CSKY_VFP_CTXSHOW(_G_pfpuop, iFd, pvFpuCtx);
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
    CSKY_VFP_RESTORE(_G_pfpuop, ptcbCur->TCB_pvStackFP);                /*  ʹ�� FPU, ��ʼ�� FPU �Ĵ��� */

    return  (ERROR_NONE);
}

#endif                                                                  /*  LW_CFG_CPU_FPU_EN > 0       */
/*********************************************************************************************************
  END
*********************************************************************************************************/
