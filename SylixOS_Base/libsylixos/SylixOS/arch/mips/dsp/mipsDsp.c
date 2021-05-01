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
** ��   ��   ��: mipsDsp.c
**
** ��   ��   ��: Jiao.JinXing (������)
**
** �ļ���������: 2018 �� 01 �� 10 ��
**
** ��        ��: MIPS ��ϵ�ܹ� DSP.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "SylixOS.h"
/*********************************************************************************************************
  �ü�֧��
*********************************************************************************************************/
#if LW_CFG_CPU_DSP_EN > 0
#include "mipsDsp.h"
#include "dsp/mipsDsp.h"
#include "dspnone/mipsDspNone.h"
#include "hr2vector/mipsHr2Vector.h"
#include "arch/mips/common/cp0/mipsCp0.h"
/*********************************************************************************************************
  ȫ�ֱ���
*********************************************************************************************************/
static LW_DSP_CONTEXT   _G_dspCtxInit;
static PMIPS_DSP_OP     _G_pdspop;
/*********************************************************************************************************
** ��������: archDspPrimaryInit
** ��������: ���� DSP ��������ʼ��
** �䡡��  : pcMachineName ��������
**           pcDspName     dsp ����
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
VOID  archDspPrimaryInit (CPCHAR  pcMachineName, CPCHAR  pcDspName)
{
    _DebugFormat(__LOGMESSAGE_LEVEL, "%s %s %s DSP pri-core initialization.\r\n",
                 LW_CFG_CPU_ARCH_FAMILY, pcMachineName, pcDspName);

    if (lib_strcmp(pcDspName, MIPS_DSP_NONE) == 0) {                    /*  ѡ�� DSP �ܹ�               */
        _G_pdspop = mipsDspNonePrimaryInit(pcMachineName, pcDspName);

    } else if ((lib_strcmp(pcDspName, MIPS_DSP_V1) == 0) ||
               (lib_strcmp(pcDspName, MIPS_DSP_V2) == 0) ||
               (lib_strcmp(pcDspName, MIPS_DSP_V3) == 0)) {
        _G_pdspop = mipsDspPrimaryInit(pcMachineName, pcDspName);

#if defined(_MIPS_ARCH_HR2)
    } else if (lib_strcmp(pcDspName, MIPS_DSP_HR2_VECTOR) == 0) {
        _G_pdspop = mipsHr2VectorPrimaryInit(pcMachineName, pcDspName);
#endif                                                                  /*  defined(_MIPS_ARCH_HR2)     */

    } else {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "unknown dsp name.\r\n");
        return;
    }

    if (_G_pdspop == LW_NULL) {
        return;
    }

    lib_bzero(&_G_dspCtxInit, sizeof(LW_DSP_CONTEXT));

    MIPS_DSP_ENABLE(_G_pdspop);

    MIPS_DSP_SAVE(_G_pdspop, (PVOID)&_G_dspCtxInit);

    MIPS_DSP_DISABLE(_G_pdspop);
}
/*********************************************************************************************************
** ��������: archDspSecondaryInit
** ��������: �Ӻ� DSP ��������ʼ��
** �䡡��  : pcMachineName ��������
**           pcDspName     dsp ����
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
#if LW_CFG_SMP_EN > 0

VOID  archDspSecondaryInit (CPCHAR  pcMachineName, CPCHAR  pcDspName)
{
    _DebugFormat(__LOGMESSAGE_LEVEL, "%s %s %s DSP sec-core initialization.\r\n",
                 LW_CFG_CPU_ARCH_FAMILY, pcMachineName, pcDspName);

    if (lib_strcmp(pcDspName, MIPS_DSP_NONE) == 0) {                    /*  ѡ�� DSP �ܹ�               */
        mipsDspNoneSecondaryInit(pcMachineName, pcDspName);

    } else if ((lib_strcmp(pcDspName, MIPS_DSP_V1) == 0) ||
               (lib_strcmp(pcDspName, MIPS_DSP_V2) == 0) ||
               (lib_strcmp(pcDspName, MIPS_DSP_V3) == 0)) {
        mipsDspSecondaryInit(pcMachineName, pcDspName);

#if defined(_MIPS_ARCH_HR2)
    } else if (lib_strcmp(pcDspName, MIPS_DSP_HR2_VECTOR) == 0) {
        mipsHr2VectorSecondaryInit(pcMachineName, pcDspName);
#endif                                                                  /*  defined(_MIPS_ARCH_HR2)     */

    } else {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "unknown dsp name.\r\n");
        return;
    }

    MIPS_DSP_DISABLE(_G_pdspop);
}

#endif                                                                  /*  LW_CFG_SMP_EN               */
/*********************************************************************************************************
** ��������: archDspCtxInit
** ��������: ��ʼ��һ�� DSP �����Ŀ��ƿ� (���ﲢû��ʹ�� DSP)
** �䡡��  : pvDspCtx   DSP ������
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
VOID  archDspCtxInit (PVOID  pvDspCtx)
{
    lib_memcpy(pvDspCtx, &_G_dspCtxInit, sizeof(LW_DSP_CONTEXT));
}
/*********************************************************************************************************
** ��������: archDspEnable
** ��������: ʹ�� DSP.
** �䡡��  : NONE
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
VOID  archDspEnable (VOID)
{
    MIPS_DSP_ENABLE(_G_pdspop);
}
/*********************************************************************************************************
** ��������: archDspDisable
** ��������: ���� DSP.
** �䡡��  : NONE
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
VOID  archDspDisable (VOID)
{
    MIPS_DSP_DISABLE(_G_pdspop);
}
/*********************************************************************************************************
** ��������: archDspSave
** ��������: ���� DSP ������.
** �䡡��  : pvDspCtx  DSP ������
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
VOID  archDspSave (PVOID  pvDspCtx)
{
    MIPS_DSP_SAVE(_G_pdspop, pvDspCtx);
}
/*********************************************************************************************************
** ��������: archDspRestore
** ��������: �ָ� DSP ������.
** �䡡��  : pvDspCtx  DSP ������
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
VOID  archDspRestore (PVOID  pvDspCtx)
{
    MIPS_DSP_RESTORE(_G_pdspop, pvDspCtx);
}
/*********************************************************************************************************
** ��������: archDspCtxShow
** ��������: ��ʾ DSP ������.
** �䡡��  : iFd       �ļ�������
**           pvDspCtx  DSP ������
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
VOID  archDspCtxShow (INT  iFd, PVOID  pvDspCtx)
{
    MIPS_DSP_CTXSHOW(_G_pdspop, iFd, pvDspCtx);
}
/*********************************************************************************************************
** ��������: archDspUndHandle
** ��������: ϵͳ���� undef �쳣ʱ, ���ô˺���.
**           ֻ��ĳ����������ж�, ����ʹ�� DSP ʱ (�����е� DSP ָ������쳣)
**           ��ʱ�ſ��Դ� DSP.
** �䡡��  : ptcbCur   ��ǰ���� TCB
** �䡡��  : ERROR or OK
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
INT  archDspUndHandle (PLW_CLASS_TCB  ptcbCur)
{
    if (LW_CPU_GET_CUR_NESTING() > 1) {                                 /*  �ж��з����쳣, ���س���    */
        return  (PX_ERROR);
    }

    if (MIPS_DSP_ISENABLE(_G_pdspop)) {                                 /*  �����ǰ������ DSP ʹ��     */
        return  (PX_ERROR);                                             /*  ��δ����ָ���� DSP �޹�     */
    }

    MIPS_DSP_ENABLE_TASK(_G_pdspop, ptcbCur);                           /*  ����ʹ�� DSP                */

    ptcbCur->TCB_ulOption |= LW_OPTION_THREAD_USED_DSP;
    MIPS_DSP_RESTORE(_G_pdspop, ptcbCur->TCB_pvStackDSP);               /*  ʹ�� DSP, ��ʼ�� DSP �Ĵ��� */

    return  (ERROR_NONE);
}

#endif                                                                  /*  LW_CFG_CPU_DSP_EN > 0       */
/*********************************************************************************************************
  END
*********************************************************************************************************/
