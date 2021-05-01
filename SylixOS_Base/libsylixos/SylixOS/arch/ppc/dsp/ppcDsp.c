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
** ��   ��   ��: ppcDsp.c
**
** ��   ��   ��: Jiao.JinXing (������)
**
** �ļ���������: 2018 �� 01 �� 10 ��
**
** ��        ��: PowerPC ��ϵ�ܹ� DSP.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "SylixOS.h"
/*********************************************************************************************************
  �ü�֧��
*********************************************************************************************************/
#if LW_CFG_CPU_DSP_EN > 0
#include "ppcDsp.h"
#include "altivec/ppcAltivec.h"
#include "dspnone/ppcDspNone.h"
/*********************************************************************************************************
  ȫ�ֱ���
*********************************************************************************************************/
static LW_DSP_CONTEXT   _G_dspCtxInit;
static PPPC_DSP_OP      _G_pdspop;
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

    if (lib_strcmp(pcDspName, PPC_DSP_NONE) == 0) {                     /*  ѡ�� DSP �ܹ�               */
        _G_pdspop = ppcDspNonePrimaryInit(pcMachineName, pcDspName);

    } else if (lib_strcmp(pcDspName, PPC_DSP_ALTIVEC) == 0) {
        _G_pdspop = ppcAltivecPrimaryInit(pcMachineName, pcDspName);

    } else {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "unknown dsp name.\r\n");
        return;
    }

    if (_G_pdspop == LW_NULL) {
        return;
    }

    lib_bzero(&_G_dspCtxInit, sizeof(LW_DSP_CONTEXT));

    PPC_DSP_ENABLE(_G_pdspop);

    PPC_DSP_SAVE(_G_pdspop, (PVOID)&_G_dspCtxInit);

    PPC_DSP_DISABLE(_G_pdspop);
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

    if (lib_strcmp(pcDspName, PPC_DSP_NONE) == 0) {                     /*  ѡ�� DSP �ܹ�               */
        ppcDspNoneSecondaryInit(pcMachineName, pcDspName);

    } else if (lib_strcmp(pcDspName, PPC_DSP_ALTIVEC) == 0) {
        ppcAltivecSecondaryInit(pcMachineName, pcDspName);

    } else {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "unknown dsp name.\r\n");
        return;
    }

    PPC_DSP_DISABLE(_G_pdspop);
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
    PPC_DSP_ENABLE(_G_pdspop);
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
    PPC_DSP_DISABLE(_G_pdspop);
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
    PPC_DSP_SAVE(_G_pdspop, pvDspCtx);
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
    PPC_DSP_RESTORE(_G_pdspop, pvDspCtx);
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
    PPC_DSP_CTXSHOW(_G_pdspop, iFd, pvDspCtx);
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

    if (PPC_DSP_ISENABLE(_G_pdspop)) {                                  /*  �����ǰ������ DSP ʹ��     */
        return  (PX_ERROR);                                             /*  ��δ����ָ���� DSP �޹�     */
    }

    PPC_DSP_ENABLE_TASK(_G_pdspop, ptcbCur);                            /*  ����ʹ�� DSP                */

    ptcbCur->TCB_ulOption |= LW_OPTION_THREAD_USED_DSP;
    PPC_DSP_RESTORE(_G_pdspop, ptcbCur->TCB_pvStackDSP);                /*  ʹ�� DSP, ��ʼ�� DSP �Ĵ��� */

    return  (ERROR_NONE);
}

#endif                                                                  /*  LW_CFG_CPU_DSP_EN > 0       */
/*********************************************************************************************************
  END
*********************************************************************************************************/
