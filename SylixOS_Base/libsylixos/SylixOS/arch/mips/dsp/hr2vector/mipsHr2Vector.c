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
** ��   ��   ��: mipsHr2Vector.c
**
** ��   ��   ��: Jiao.JinXing (������)
**
** �ļ���������: 2018 �� 02 �� 24 ��
**
** ��        ��: ��� 2 �Ŵ������������㵥Ԫ֧��.
*********************************************************************************************************/
#define  __SYLIXOS_STDIO
#define  __SYLIXOS_KERNEL
#include "SylixOS.h"
/*********************************************************************************************************
  �ü�֧��
*********************************************************************************************************/
#if defined(_MIPS_ARCH_HR2) && LW_CFG_CPU_DSP_EN > 0
#include "../mipsDsp.h"
#include "arch/mips/common/cp0/mipsCp0.h"
/*********************************************************************************************************
  ȫ�ֱ���
*********************************************************************************************************/
static MIPS_DSP_OP      _G_dspopHr2Vector;
/*********************************************************************************************************
  �ⲿ��������
*********************************************************************************************************/
extern VOID  mipsHr2VectorInit(VOID);
extern VOID  mipsHr2VectorEnable(VOID);
extern VOID  mipsHr2VectorDisable(VOID);
extern BOOL  mipsHr2VectorIsEnable(VOID);
extern VOID  mipsHr2VectorSave(ARCH_DSP_CTX  *pdspctx);
extern VOID  mipsHr2VectorRestore(ARCH_DSP_CTX  *pdspctx);
/*********************************************************************************************************
** ��������: mipsHr2VectorCtxShow
** ��������: ��ʾ DSP ������
** �䡡��  : iFd       ����ļ�������
**           pvDspCtx  DSP ������
** �䡡��  :
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static VOID  mipsHr2VectorCtxShow (INT  iFd, ARCH_DSP_CTX  *pdspctx)
{
#if (LW_CFG_DEVICE_EN > 0) && (LW_CFG_FIO_LIB_EN > 0)
    /*
     * Not in opensoure version
     */
#endif
}
/*********************************************************************************************************
** ��������: mipsHr2VectorEnableTask
** ��������: ϵͳ���� DSP �������쳣ʱ, ʹ������� DSP
** �䡡��  : ptcbCur    ��ǰ������ƿ�
** �䡡��  :
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static VOID  mipsHr2VectorEnableTask (PLW_CLASS_TCB  ptcbCur)
{
    /*
     * Not in opensoure version
     */
}
/*********************************************************************************************************
** ��������: mipsHr2VectorPrimaryInit
** ��������: ��ȡ DSP ����������������
** �䡡��  : pcMachineName ������
**           pcDspName     DSP ��������
** �䡡��  : ����������
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
PMIPS_DSP_OP  mipsHr2VectorPrimaryInit (CPCHAR  pcMachineName, CPCHAR  pcDspName)
{
    mipsHr2VectorInit();

    _G_dspopHr2Vector.MDSP_pfuncEnable     = mipsHr2VectorEnable;
    _G_dspopHr2Vector.MDSP_pfuncDisable    = mipsHr2VectorDisable;
    _G_dspopHr2Vector.MDSP_pfuncIsEnable   = mipsHr2VectorIsEnable;
    _G_dspopHr2Vector.MDSP_pfuncSave       = mipsHr2VectorSave;
    _G_dspopHr2Vector.MDSP_pfuncRestore    = mipsHr2VectorRestore;
    _G_dspopHr2Vector.MDSP_pfuncCtxShow    = mipsHr2VectorCtxShow;
    _G_dspopHr2Vector.MDSP_pfuncEnableTask = mipsHr2VectorEnableTask;

    return  (&_G_dspopHr2Vector);
}
/*********************************************************************************************************
** ��������: mipsHr2VectorSecondaryInit
** ��������: ��ʼ�� DSP ������
** �䡡��  : pcMachineName ������
**           pcDspName     DSP ��������
** �䡡��  :
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
VOID  mipsHr2VectorSecondaryInit (CPCHAR  pcMachineName, CPCHAR  pcDspName)
{
    mipsHr2VectorInit();
}

#endif                                                                  /*  defined(_MIPS_ARCH_HR2)     */
                                                                        /*  LW_CFG_CPU_DSP_EN > 0       */
/*********************************************************************************************************
  END
*********************************************************************************************************/
