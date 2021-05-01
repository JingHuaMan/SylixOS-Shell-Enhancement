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
** ��   ��   ��: mipsFpu.c
**
** ��   ��   ��: Jiao.JinXing (������)
**
** �ļ���������: 2015 �� 11 �� 17 ��
**
** ��        ��: MIPS ��ϵ�ܹ�Ӳ������������ (VFP).
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "SylixOS.h"
/*********************************************************************************************************
  �ü�֧��
*********************************************************************************************************/
#if LW_CFG_CPU_FPU_EN > 0
#include "mipsFpu.h"
#include "fpu32/mipsVfp32.h"
#include "vfpnone/mipsVfpNone.h"
#include "arch/mips/common/cp0/mipsCp0.h"
/*********************************************************************************************************
  ȫ�ֱ���
*********************************************************************************************************/
static LW_FPU_CONTEXT   _G_fpuCtxInit;
static PMIPS_FPU_OP     _G_pfpuop;
static UINT32           _G_uiFpuFIR;
/*********************************************************************************************************
  Fpu ģ����֡��
*********************************************************************************************************/
static MIPS_FPU_EMU_FRAME   *_G_pmipsfpuePool;
/*********************************************************************************************************
** ��������: archFpuEmuInit
** ��������: Fpu ģ������ʼ�� (������ MMU ��ʼ���󱻵���)
** �䡡��  : NONE
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
VOID  archFpuEmuInit (VOID)
{
#if LW_CFG_VMM_EN > 0
    _G_pmipsfpuePool = (MIPS_FPU_EMU_FRAME *)API_VmmMallocEx(sizeof(MIPS_FPU_EMU_FRAME) * LW_CFG_MAX_THREADS,
                                                             LW_VMM_FLAG_RDWR | LW_VMM_FLAG_EXEC);
#else
    _G_pmipsfpuePool = (MIPS_FPU_EMU_FRAME *)__KHEAP_ALLOC(sizeof(MIPS_FPU_EMU_FRAME) * LW_CFG_MAX_THREADS);
#endif                                                                  /*  LW_CFG_VMM_EN > 0           */

    _BugHandle(!_G_pmipsfpuePool, LW_TRUE, "can not allocate FPU emulator pool.\r\n");

    _DebugHandle(__LOGMESSAGE_LEVEL, "FPU emulator initialization.\r\n");
}
/*********************************************************************************************************
** ��������: archFpuEmuFrameGet
** ��������: Fpu ģ����֡��ȡ (�ڸ����쳣��ִ��)
** �䡡��  : ptcbCur       ��ǰ���� TCB
** �䡡��  : ģ����֡
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
MIPS_FPU_EMU_FRAME  *archFpuEmuFrameGet (PLW_CLASS_TCB  ptcbCur)
{
    MIPS_FPU_EMU_FRAME  *pmipsfpue;

    pmipsfpue = &_G_pmipsfpuePool[_ObjectGetIndex(ptcbCur->TCB_ulId)];

    return  (pmipsfpue);
}
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
    UINT32  uiConfig1;

    _DebugFormat(__LOGMESSAGE_LEVEL, "%s %s %s FPU pri-core initialization.\r\n",
                 LW_CFG_CPU_ARCH_FAMILY, pcMachineName, pcFpuName);

    uiConfig1 = mipsCp0Config1Read();
    if (uiConfig1 & MIPS_CONF1_FP) {
        if (lib_strcmp(pcFpuName, MIPS_FPU_NONE) == 0) {                /*  ѡ�� VFP �ܹ�               */
            _G_pfpuop = mipsVfpNonePrimaryInit(pcMachineName, pcFpuName);

        } else if ((lib_strcmp(pcFpuName, MIPS_FPU_VFP32) == 0) ||
                   (lib_strcmp(pcFpuName, MIPS_FPU_AUTO)  == 0)) {
            _G_pfpuop = mipsVfp32PrimaryInit(pcMachineName, pcFpuName);

        } else {
            _DebugHandle(__ERRORMESSAGE_LEVEL, "unknown fpu name.\r\n");
            return;
        }
    } else {
        _G_pfpuop = mipsVfpNonePrimaryInit(pcMachineName, MIPS_FPU_NONE);
    }

    if (_G_pfpuop == LW_NULL) {
        return;
    }

    lib_bzero(&_G_fpuCtxInit, sizeof(LW_FPU_CONTEXT));

    MIPS_VFP_ENABLE(_G_pfpuop);

    MIPS_VFP_SAVE(_G_pfpuop, (PVOID)&_G_fpuCtxInit);

    _G_uiFpuFIR = (UINT32)MIPS_VFP_GETFIR(_G_pfpuop);

    MIPS_VFP_DISABLE(_G_pfpuop);
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
    UINT32  uiConfig1;

    _DebugFormat(__LOGMESSAGE_LEVEL, "%s %s %s FPU sec-core initialization.\r\n",
                 LW_CFG_CPU_ARCH_FAMILY, pcMachineName, pcFpuName);

    uiConfig1 = mipsCp0Config1Read();
    if (uiConfig1 & MIPS_CONF1_FP) {
        if (lib_strcmp(pcFpuName, MIPS_FPU_NONE) == 0) {                /*  ѡ�� VFP �ܹ�               */
            mipsVfpNoneSecondaryInit(pcMachineName, pcFpuName);

        } else if ((lib_strcmp(pcFpuName, MIPS_FPU_VFP32) == 0) ||
                   (lib_strcmp(pcFpuName, MIPS_FPU_AUTO)  == 0)) {
            mipsVfp32SecondaryInit(pcMachineName, pcFpuName);

        } else {
            _DebugHandle(__ERRORMESSAGE_LEVEL, "unknown fpu name.\r\n");
            return;
        }
    } else {
        mipsVfpNoneSecondaryInit(pcMachineName, MIPS_FPU_NONE);
    }

    MIPS_VFP_DISABLE(_G_pfpuop);
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
    MIPS_VFP_ENABLE(_G_pfpuop);
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
    MIPS_VFP_DISABLE(_G_pfpuop);
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
    MIPS_VFP_SAVE(_G_pfpuop, pvFpuCtx);
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
    MIPS_VFP_RESTORE(_G_pfpuop, pvFpuCtx);
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
    MIPS_VFP_CTXSHOW(_G_pfpuop, iFd, pvFpuCtx);
}
/*********************************************************************************************************
** ��������: archFpuGetFIR
** ��������: ��ø���ʵ�ּĴ�����ֵ.
** �䡡��  : NONE
** �䡡��  : ����ʵ�ּĴ�����ֵ
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
UINT32  archFpuGetFIR (VOID)
{
    return  (_G_uiFpuFIR);
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
    UINT32  uiConfig1;

    if (LW_CPU_GET_CUR_NESTING() > 1) {                                 /*  �ж��з����쳣, ���س���    */
        return  (PX_ERROR);
    }

    if (ptcbCur->TCB_ulOption & LW_OPTION_THREAD_USED_FP) {
        return  (PX_ERROR);
    }

    uiConfig1 = mipsCp0Config1Read();
    if (uiConfig1 & MIPS_CONF1_FP) {                                    /*  �� FPU                      */
        MIPS_VFP_ENABLE_TASK(_G_pfpuop, ptcbCur);                       /*  ����ʹ�� FPU                */
    
	} else {
        return  (PX_ERROR);                                             /*  û�� FPU                    */
    }

    ptcbCur->TCB_ulOption |= LW_OPTION_THREAD_USED_FP;
    MIPS_VFP_RESTORE(_G_pfpuop, ptcbCur->TCB_pvStackFP);                /*  ʹ�� FPU, ��ʼ�� FPU �Ĵ��� */

    return  (ERROR_NONE);
}

#endif                                                                  /*  LW_CFG_CPU_FPU_EN > 0       */
/*********************************************************************************************************
  END
*********************************************************************************************************/
