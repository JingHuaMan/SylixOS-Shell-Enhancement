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
** ��   ��   ��: mipsDsp.h
**
** ��   ��   ��: Jiao.JinXing (������)
**
** �ļ���������: 2018 �� 01 �� 10 ��
**
** ��        ��: MIPS ��ϵ�ܹ� DSP.
*********************************************************************************************************/

#ifndef __ARCH_MIPSDSP_H
#define __ARCH_MIPSDSP_H

/*********************************************************************************************************
  �ü�֧��
*********************************************************************************************************/
#if LW_CFG_CPU_DSP_EN > 0

/*********************************************************************************************************
  MIPS dsp ��������
*********************************************************************************************************/

typedef struct {
    VOIDFUNCPTR     MDSP_pfuncEnable;
    VOIDFUNCPTR     MDSP_pfuncDisable;
    BOOLFUNCPTR     MDSP_pfuncIsEnable;
    VOIDFUNCPTR     MDSP_pfuncSave;
    VOIDFUNCPTR     MDSP_pfuncRestore;
    VOIDFUNCPTR     MDSP_pfuncCtxShow;
    VOIDFUNCPTR     MDSP_pfuncEnableTask;
} MIPS_DSP_OP;
typedef MIPS_DSP_OP *PMIPS_DSP_OP;

/*********************************************************************************************************
  MIPS dsp ��������
*********************************************************************************************************/

#define MIPS_DSP_ENABLE(op)              op->MDSP_pfuncEnable()
#define MIPS_DSP_DISABLE(op)             op->MDSP_pfuncDisable()
#define MIPS_DSP_ISENABLE(op)            op->MDSP_pfuncIsEnable()
#define MIPS_DSP_SAVE(op, ctx)           op->MDSP_pfuncSave((ctx))
#define MIPS_DSP_RESTORE(op, ctx)        op->MDSP_pfuncRestore((ctx))
#define MIPS_DSP_CTXSHOW(op, fd, ctx)    op->MDSP_pfuncCtxShow((fd), (ctx))
#define MIPS_DSP_ENABLE_TASK(op, tcb)    op->MDSP_pfuncEnableTask((tcb))

#endif                                                                  /*  LW_CFG_CPU_DSP_EN > 0       */
#endif                                                                  /*  __ARCH_MIPSDSP_H            */
/*********************************************************************************************************
  END
*********************************************************************************************************/
