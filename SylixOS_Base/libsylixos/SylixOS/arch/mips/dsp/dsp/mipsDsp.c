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
** ��        ��: MIPS ��ϵ�ܹ� DSP ֧��.
*********************************************************************************************************/
#define  __SYLIXOS_STDIO
#define  __SYLIXOS_KERNEL
#include "SylixOS.h"
/*********************************************************************************************************
  �ü�֧��
*********************************************************************************************************/
#if LW_CFG_CPU_DSP_EN > 0
#include "../mipsDsp.h"
#include "arch/mips/common/cp0/mipsCp0.h"
/*********************************************************************************************************
  ȫ�ֱ���
*********************************************************************************************************/
static MIPS_DSP_OP      _G_dspopDsp;

#define DSP_DEFAULT     0x00000000
#define DSP_MASK        0x3f
/*********************************************************************************************************
** ��������: mipsDspEnable
** ��������: ʹ�� DSP
** �䡡��  :
** �䡡��  :
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static VOID  mipsDspEnable (VOID)
{
    mipsCp0StatusWrite(mipsCp0StatusRead() | ST0_MX);
}
/*********************************************************************************************************
** ��������: mipsDspDisable
** ��������: ���� DSP
** �䡡��  :
** �䡡��  :
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static VOID  mipsDspDisable (VOID)
{
    mipsCp0StatusWrite(mipsCp0StatusRead() & (~ST0_MX));
}
/*********************************************************************************************************
** ��������: mipsDspIsEnable
** ��������: �Ƿ�ʹ���� DSP
** �䡡��  :
** �䡡��  : �Ƿ�ʹ��
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static BOOL  mipsDspIsEnable (VOID)
{
    return  ((mipsCp0StatusRead() & ST0_MX) ? LW_TRUE : LW_FALSE);
}
/*********************************************************************************************************
** ��������: mipsDspSave
** ��������: ���� DSP ������
** �䡡��  : pvDspCtx  DSP ������
** �䡡��  :
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static VOID  mipsDspSave (ARCH_DSP_CTX  *pdspctx)
{
    UINT32  uiStatus = mipsCp0StatusRead();

    mipsCp0StatusWrite(uiStatus | ST0_MX);

    pdspctx->DSPCTX_dspCtx.MIPSDSPCTX_dspRegs[0] = mfhi1();
    pdspctx->DSPCTX_dspCtx.MIPSDSPCTX_dspRegs[1] = mflo1();
    pdspctx->DSPCTX_dspCtx.MIPSDSPCTX_dspRegs[2] = mfhi2();
    pdspctx->DSPCTX_dspCtx.MIPSDSPCTX_dspRegs[3] = mflo2();
    pdspctx->DSPCTX_dspCtx.MIPSDSPCTX_dspRegs[4] = mfhi3();
    pdspctx->DSPCTX_dspCtx.MIPSDSPCTX_dspRegs[5] = mflo3();
    pdspctx->DSPCTX_dspCtx.MIPSDSPCTX_dspCtrl    = rddsp(DSP_MASK);

    mipsCp0StatusWrite(uiStatus);
}
/*********************************************************************************************************
** ��������: mipsDspRestore
** ��������: �ָ� DSP ������
** �䡡��  : pvDspCtx  DSP ������
** �䡡��  :
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static VOID  mipsDspRestore (ARCH_DSP_CTX  *pdspctx)
{
    UINT32  uiStatus = mipsCp0StatusRead();

    mipsCp0StatusWrite(uiStatus | ST0_MX);

    mthi1(pdspctx->DSPCTX_dspCtx.MIPSDSPCTX_dspRegs[0]);
    mtlo1(pdspctx->DSPCTX_dspCtx.MIPSDSPCTX_dspRegs[1]);
    mthi2(pdspctx->DSPCTX_dspCtx.MIPSDSPCTX_dspRegs[2]);
    mtlo2(pdspctx->DSPCTX_dspCtx.MIPSDSPCTX_dspRegs[3]);
    mthi3(pdspctx->DSPCTX_dspCtx.MIPSDSPCTX_dspRegs[4]);
    mtlo3(pdspctx->DSPCTX_dspCtx.MIPSDSPCTX_dspRegs[5]);
    wrdsp(pdspctx->DSPCTX_dspCtx.MIPSDSPCTX_dspCtrl, DSP_MASK);

    mipsCp0StatusWrite(uiStatus);
}
/*********************************************************************************************************
** ��������: mipsDspCtxShow
** ��������: ��ʾ DSP ������
** �䡡��  : iFd       ����ļ�������
**           pvDspCtx  DSP ������
** �䡡��  :
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static VOID  mipsDspCtxShow (INT  iFd, ARCH_DSP_CTX  *pdspctx)
{
#if (LW_CFG_DEVICE_EN > 0) && (LW_CFG_FIO_LIB_EN > 0)
    fdprintf(iFd, "hi1 = 0x%x, lo1 = 0x%x\n", pdspctx->DSPCTX_dspCtx.MIPSDSPCTX_dspRegs[0],
                                              pdspctx->DSPCTX_dspCtx.MIPSDSPCTX_dspRegs[1]);
    fdprintf(iFd, "hi2 = 0x%x, lo2 = 0x%x\n", pdspctx->DSPCTX_dspCtx.MIPSDSPCTX_dspRegs[2],
                                              pdspctx->DSPCTX_dspCtx.MIPSDSPCTX_dspRegs[3]);
    fdprintf(iFd, "hi3 = 0x%x, lo3 = 0x%x\n", pdspctx->DSPCTX_dspCtx.MIPSDSPCTX_dspRegs[4],
                                              pdspctx->DSPCTX_dspCtx.MIPSDSPCTX_dspRegs[5]);
    fdprintf(iFd, "dsp control = 0x%x\n",     pdspctx->DSPCTX_dspCtx.MIPSDSPCTX_dspCtrl);
#endif
}
/*********************************************************************************************************
** ��������: mipsDspEnableTask
** ��������: ϵͳ���� DSP �������쳣ʱ, ʹ������� DSP
** �䡡��  : ptcbCur    ��ǰ������ƿ�
** �䡡��  :
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static VOID  mipsDspEnableTask (PLW_CLASS_TCB  ptcbCur)
{
    ARCH_REG_CTX  *pregctx;

    pregctx = &ptcbCur->TCB_archRegCtx;
    pregctx->REG_ulCP0Status |= ST0_MX;
}
/*********************************************************************************************************
** ��������: mipsDspPrimaryInit
** ��������: ��ȡ DSP ����������������
** �䡡��  : pcMachineName ������
**           pcDspName     DSP ��������
** �䡡��  : ����������
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static VOID  mipsDspInit (VOID)
{
    UINT32  uiStatus = mipsCp0StatusRead();

    mipsCp0StatusWrite(uiStatus | ST0_MX);

    mthi1(0);
    mtlo1(0);
    mthi2(0);
    mtlo2(0);
    mthi3(0);
    mtlo3(0);
    wrdsp(DSP_DEFAULT, DSP_MASK);

    mipsCp0StatusWrite(uiStatus);
}
/*********************************************************************************************************
** ��������: mipsDspPrimaryInit
** ��������: ��ȡ DSP ����������������
** �䡡��  : pcMachineName ������
**           pcDspName     DSP ��������
** �䡡��  : ����������
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
PMIPS_DSP_OP  mipsDspPrimaryInit (CPCHAR  pcMachineName, CPCHAR  pcDspName)
{
    mipsDspInit();

    _G_dspopDsp.MDSP_pfuncEnable     = mipsDspEnable;
    _G_dspopDsp.MDSP_pfuncDisable    = mipsDspDisable;
    _G_dspopDsp.MDSP_pfuncIsEnable   = mipsDspIsEnable;
    _G_dspopDsp.MDSP_pfuncSave       = mipsDspSave;
    _G_dspopDsp.MDSP_pfuncRestore    = mipsDspRestore;
    _G_dspopDsp.MDSP_pfuncCtxShow    = mipsDspCtxShow;
    _G_dspopDsp.MDSP_pfuncEnableTask = mipsDspEnableTask;

    return  (&_G_dspopDsp);
}
/*********************************************************************************************************
** ��������: mipsDspSecondaryInit
** ��������: ��ʼ�� DSP ������
** �䡡��  : pcMachineName ������
**           pcDspName     DSP ��������
** �䡡��  :
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
VOID  mipsDspSecondaryInit (CPCHAR  pcMachineName, CPCHAR  pcDspName)
{
    mipsDspInit();
}

#endif                                                                  /*  LW_CFG_CPU_DSP_EN > 0       */
/*********************************************************************************************************
  END
*********************************************************************************************************/
