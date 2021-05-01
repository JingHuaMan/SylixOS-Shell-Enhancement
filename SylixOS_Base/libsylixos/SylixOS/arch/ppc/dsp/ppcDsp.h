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
** ��        ��: PowerPC ��ϵ�ܹ� DSP.
*********************************************************************************************************/

#ifndef __ARCH_PPCDSP_H
#define __ARCH_PPCDSP_H

/*********************************************************************************************************
  �ü�֧��
*********************************************************************************************************/
#if LW_CFG_CPU_DSP_EN > 0

/*********************************************************************************************************
  PowerPC dsp ��������
*********************************************************************************************************/

typedef struct {
    VOIDFUNCPTR     PDSP_pfuncEnable;
    VOIDFUNCPTR     PDSP_pfuncDisable;
    BOOLFUNCPTR     PDSP_pfuncIsEnable;
    VOIDFUNCPTR     PDSP_pfuncSave;
    VOIDFUNCPTR     PDSP_pfuncRestore;
    VOIDFUNCPTR     PDSP_pfuncCtxShow;
    VOIDFUNCPTR     PDSP_pfuncEnableTask;
} PPC_DSP_OP;
typedef PPC_DSP_OP *PPPC_DSP_OP;

/*********************************************************************************************************
  PowerPC dsp ��������
*********************************************************************************************************/

#define PPC_DSP_ENABLE(op)              op->PDSP_pfuncEnable()
#define PPC_DSP_DISABLE(op)             op->PDSP_pfuncDisable()
#define PPC_DSP_ISENABLE(op)            op->PDSP_pfuncIsEnable()
#define PPC_DSP_SAVE(op, ctx)           op->PDSP_pfuncSave((ctx))
#define PPC_DSP_RESTORE(op, ctx)        op->PDSP_pfuncRestore((ctx))
#define PPC_DSP_CTXSHOW(op, fd, ctx)    op->PDSP_pfuncCtxShow((fd), (ctx))
#define PPC_DSP_ENABLE_TASK(op, tcb)    op->PDSP_pfuncEnableTask((tcb))

#endif                                                                  /*  LW_CFG_CPU_DSP_EN > 0       */
#endif                                                                  /*  __ARCH_PPCDSP_H             */
/*********************************************************************************************************
  END
*********************************************************************************************************/
