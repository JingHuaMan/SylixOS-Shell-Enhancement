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
** ��   ��   ��: riscvUnaligned.c
**
** ��   ��   ��: Jiao.JinXing (������)
**
** �ļ���������: 2018 �� 08 �� 27 ��
**
** ��        ��: RISC-V ��ϵ���ܷǶ��봦��.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "SylixOS.h"
#if LW_CFG_RISCV_M_LEVEL > 0
/*********************************************************************************************************
  ��������
*********************************************************************************************************/
extern int  misaligned_load_trap(ARCH_REG_CTX  *pregctx);
extern int  misaligned_store_trap(ARCH_REG_CTX  *pregctx);
/*********************************************************************************************************
** ��������: riscvLoadUnalignedHandle
** ��������: RISC-V load ָ��Ƕ��봦��
** �䡡��  : pregctx           �Ĵ���������
**           pabtInfo          �쳣��Ϣ
** �䡡��  : ��ֹ��Ϣ
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
VOID  riscvLoadUnalignedHandle (ARCH_REG_CTX  *pregctx, LW_VMM_ABORT *pabtInfo)
{
    if (misaligned_load_trap(pregctx) == ERROR_NONE) {
        pabtInfo->VMABT_uiType = LW_VMM_ABORT_TYPE_NOINFO;

    } else {
        pabtInfo->VMABT_uiMethod = BUS_ADRALN;
        pabtInfo->VMABT_uiType   = LW_VMM_ABORT_TYPE_BUS;
    }
}
/*********************************************************************************************************
** ��������: riscvStoreUnalignedHandle
** ��������: RISC-V store ָ��Ƕ��봦��
** �䡡��  : pregctx           �Ĵ���������
**           pabtInfo          �쳣��Ϣ
** �䡡��  : ��ֹ��Ϣ
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
VOID  riscvStoreUnalignedHandle (ARCH_REG_CTX  *pregctx, LW_VMM_ABORT *pabtInfo)
{
    if (misaligned_store_trap(pregctx) == ERROR_NONE) {
        pabtInfo->VMABT_uiType = LW_VMM_ABORT_TYPE_NOINFO;

    } else {
        pabtInfo->VMABT_uiMethod = BUS_ADRALN;
        pabtInfo->VMABT_uiType   = LW_VMM_ABORT_TYPE_BUS;
    }
}

#endif                                                                  /*  LW_CFG_RISCV_M_LEVEL > 0    */
/*********************************************************************************************************
  END
*********************************************************************************************************/
