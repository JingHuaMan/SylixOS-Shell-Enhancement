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
** ��   ��   ��: riscvUnaligned.h
**
** ��   ��   ��: Jiao.JinXing (������)
**
** �ļ���������: 2018 �� 11 �� 16 ��
**
** ��        ��: RISC-V ��ϵ���ܷǶ��봦��.
*********************************************************************************************************/

#ifndef __ARCH_RISCVUNALIGNED_H
#define __ARCH_RISCVUNALIGNED_H

#if LW_CFG_RISCV_M_LEVEL > 0

extern VOID  riscvLoadUnalignedHandle(ARCH_REG_CTX  *pregctx, LW_VMM_ABORT *pabtInfo);
extern VOID  riscvStoreUnalignedHandle(ARCH_REG_CTX  *pregctx, LW_VMM_ABORT *pabtInfo);

#endif                                                                  /*  LW_CFG_RISCV_M_LEVEL > 0    */

#endif                                                                  /*  __ARCH_RISCVUNALIGNED_H     */
/*********************************************************************************************************
  END
*********************************************************************************************************/
