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
** ��   ��   ��: ppcAltivec.c
**
** ��   ��   ��: Jiao.JinXing (������)
**
** �ļ���������: 2016 �� 05 �� 04 ��
**
** ��        ��: PowerPC ��ϵ�ܹ� ALTIVEC ֧��.
*********************************************************************************************************/
#define  __SYLIXOS_STDIO
#define  __SYLIXOS_KERNEL
#include "SylixOS.h"
/*********************************************************************************************************
  �ü�֧��
*********************************************************************************************************/
#if LW_CFG_CPU_DSP_EN > 0
#include "../ppcDsp.h"
#define  __SYLIXOS_PPC_HAVE_ALTIVEC 1
#include "arch/ppc/arch_604.h"
/*********************************************************************************************************
  ȫ�ֱ���
*********************************************************************************************************/
static PPC_DSP_OP   _G_dspopAltivec;
/*********************************************************************************************************
  ʵ�ֺ���
*********************************************************************************************************/
extern VOID     ppcAltivecEnable(VOID);
extern VOID     ppcAltivecDisable(VOID);
extern BOOL     ppcAltivecIsEnable(VOID);
extern VOID     ppcAltivecSave(PVOID  pvDspCtx);
extern VOID     ppcAltivecRestore(PVOID  pvDspCtx);
/*********************************************************************************************************
  VSCR ��ض���
*********************************************************************************************************/
#define ALTIVEC_VSCR_CONFIG_WORD    3                                   /*  VSCR word with below bits   */
#define ALTIVEC_VSCR_NJ             0x00010000                          /*  Non-Java mode               */
#define ALTIVEC_VSCR_SAT            0x00000001                          /*  Vector Saturation           */
/*********************************************************************************************************
** ��������: ppcAltivecCtxShow
** ��������: ��ʾ ALTIVEC ������
** �䡡��  : iFd       ����ļ�������
**           pvDspCtx  ALTIVEC ������
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static VOID  ppcAltivecCtxShow (INT  iFd, PVOID  pvDspCtx)
{
#if (LW_CFG_DEVICE_EN > 0) && (LW_CFG_FIO_LIB_EN > 0)
    LW_DSP_CONTEXT *pdspctx    = (LW_DSP_CONTEXT *)pvDspCtx;
    ARCH_DSP_CTX   *pcpudspctx = &pdspctx->DSPCTX_dspctxContext;
    UINT32         *puiValue;
    INT             i;

    fdprintf(iFd, "VRSAVE = 0x%08x VSCR = 0x%08x\n",
             pcpudspctx->VECCTX_uiVrsave,
             pcpudspctx->VECCTX_uiVscr[ALTIVEC_VSCR_CONFIG_WORD]);

    for (i = 0; i < ALTIVEC_REG_NR; i++) {
        puiValue = (UINT32 *)&pcpudspctx->VECCTX_regs[i];
        fdprintf(iFd, "AR%02d: 0x%08x_0x%08x_0x%08x_0x%08x\n",
                 i,
                 puiValue[0], puiValue[1], puiValue[2], puiValue[3]);
    }
#endif
}
/*********************************************************************************************************
** ��������: ppcAltivecEnableTask
** ��������: ϵͳ���� undef �쳣ʱ, ʹ������� ALTIVEC
** �䡡��  : ptcbCur    ��ǰ������ƿ�
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static VOID  ppcAltivecEnableTask (PLW_CLASS_TCB  ptcbCur)
{
    ARCH_REG_CTX  *pregctx;
    ARCH_DSP_CTX  *pdspctx;

    pregctx = &ptcbCur->TCB_archRegCtx;
    pregctx->REG_uiSrr1 |= ARCH_PPC_MSR_VEC;

    pdspctx = &ptcbCur->TCB_dspctxContext.DSPCTX_dspctxContext;
    pdspctx->VECCTX_uiVrsave = 0ul;
    pdspctx->VECCTX_uiVscr[ALTIVEC_VSCR_CONFIG_WORD] = 0ul;
}
/*********************************************************************************************************
** ��������: ppcAltivecPrimaryInit
** ��������: ��ȡ ALTIVEC ����������������
** �䡡��  : pcMachineName ������
**           pcDspName     DSP ��������
** �䡡��  : ����������
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
PPPC_DSP_OP  ppcAltivecPrimaryInit (CPCHAR  pcMachineName, CPCHAR  pcDspName)
{
    _G_dspopAltivec.PDSP_pfuncEnable     = ppcAltivecEnable;
    _G_dspopAltivec.PDSP_pfuncDisable    = ppcAltivecDisable;
    _G_dspopAltivec.PDSP_pfuncIsEnable   = ppcAltivecIsEnable;
    _G_dspopAltivec.PDSP_pfuncSave       = ppcAltivecSave;
    _G_dspopAltivec.PDSP_pfuncRestore    = ppcAltivecRestore;
    _G_dspopAltivec.PDSP_pfuncCtxShow    = ppcAltivecCtxShow;
    _G_dspopAltivec.PDSP_pfuncEnableTask = ppcAltivecEnableTask;

    return  (&_G_dspopAltivec);
}
/*********************************************************************************************************
** ��������: ppcAltivecSecondaryInit
** ��������: ��ʼ�� ALTIVEC ������
** �䡡��  : pcMachineName ������
**           pcDspName     DSP ��������
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
VOID  ppcAltivecSecondaryInit (CPCHAR  pcMachineName, CPCHAR  pcDspName)
{
    (VOID)pcMachineName;
    (VOID)pcDspName;
}

#endif                                                                  /*  LW_CFG_CPU_DSP_EN > 0       */
/*********************************************************************************************************
  END
*********************************************************************************************************/
