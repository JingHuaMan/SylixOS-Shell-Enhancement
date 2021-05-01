/*********************************************************************************************************
**
**                                    �й������Դ��֯
**
**                                   Ƕ��ʽʵʱ����ϵͳ
**
**                                SylixOS(TM)  LW : long wing
**
**                               Copyright All Rights Reserved
**
**--------------�ļ���Ϣ--------------------------------------------------------------------------------
**
** ��   ��   ��: arch_dsp.h
**
** ��   ��   ��: Jiao.JinXing (������)
**
** �ļ���������: 2018 �� 01 �� 10 ��
**
** ��        ��: MIPS DSP ���.
*********************************************************************************************************/

#ifndef __MIPS_ARCH_DSP_H
#define __MIPS_ARCH_DSP_H

#if defined(__SYLIXOS_KERNEL) || defined(__ASSEMBLY__) || defined(ASSEMBLY)

/*********************************************************************************************************
  �����벻������������
*********************************************************************************************************/

#if !defined(__ASSEMBLY__) && !defined(ASSEMBLY)

/*********************************************************************************************************
  ��� 2 ���������㵥Ԫ������
*********************************************************************************************************/

#define HR2_VECTOR_REG_NR               32                              /*  HR2 �������ݼĴ���������    */
#define HR2_VECTOR_REG_WIDTH            256                             /*  HR2 �������ݼĴ�����λ��    */
#define HR2_VECTOR_REG_SIZE             (HR2_VECTOR_REG_WIDTH / 8)      /*  HR2 �������ݼĴ����Ĵ�С    */
#define HR2_VECTOR_CTX_ALIGN            HR2_VECTOR_REG_SIZE             /*  HR2 ���������Ķ����С      */

typedef union hr2_vector_reg {                                          /*  HR2 �������ݼĴ�������      */
    UINT32              val32[HR2_VECTOR_REG_WIDTH / 32];
    UINT64              val64[HR2_VECTOR_REG_WIDTH / 64];
} HR2_VECTOR_REG;

typedef struct hr2_vector_ctx {                                         /*  HR2 �������㵥Ԫ������      */
    HR2_VECTOR_REG      HR2VECCTX_vectorRegs[HR2_VECTOR_REG_NR];        /*  HR2 �������ݼĴ���          */
    UINT32              HR2VECCTX_uiVccr;                               /*  HR2 ����Ŀ�ļĴ���          */
    UINT32              HR2VECCTX_uiPad;
} HR2_VECTOR_CTX;

/*********************************************************************************************************
  MIPS ��׼ DSP ������
*********************************************************************************************************/

#define MIPS_DSP_REGS_NR                6                               /*  MIPS DSP ���ݼĴ���������   */

typedef UINT32  MIPS_DSP_REG;                                           /*  MIPS DSP ���ݼĴ�������     */

typedef struct mips_dsp_ctx {                                           /*  MIPS ��׼ DSP ������        */
    MIPS_DSP_REG        MIPSDSPCTX_dspRegs[MIPS_DSP_REGS_NR];           /*  MIPS DSP ���ݼĴ���         */
    UINT32              MIPSDSPCTX_dspCtrl;                             /*  MIPS DSP ���ƼĴ���         */
} MIPS_DSP_CTX;

/*********************************************************************************************************
  DSP ������
*********************************************************************************************************/

#define ARCH_DSP_CTX_ALIGN              HR2_VECTOR_CTX_ALIGN            /*  DSP �����Ķ����С          */

union arch_dsp_ctx {                                                    /*  DSP ������                  */
    MIPS_DSP_CTX        DSPCTX_dspCtx;
    HR2_VECTOR_CTX      DSPCTX_hr2VectorCtx;
} __attribute__ ((aligned(ARCH_DSP_CTX_ALIGN)));

typedef union arch_dsp_ctx ARCH_DSP_CTX;                                /*  DSP ������                  */

#endif                                                                  /*  __ASSEMBLY__                */
#endif                                                                  /*  __SYLIXOS_KERNEL            */
#endif                                                                  /*  __MIPS_ARCH_DSP_H           */
/*********************************************************************************************************
  END
*********************************************************************************************************/
