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
** ��   ��   ��: ppcUnaligned.c
**
** ��   ��   ��: Jiao.JinXing (������)
**
** �ļ���������: 2018 �� 08 �� 27 ��
**
** ��        ��: PowerPC ��ϵ���ܷǶ��봦��.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "SylixOS.h"
#include <linux/compat.h>
#include "porting.h"
#include "sstep.h"
#include "disassemble.h"
/*********************************************************************************************************
  ��������
*********************************************************************************************************/
int fix_alignment(ARCH_REG_CTX *regs, enum instruction_type  *inst_type);
/*********************************************************************************************************
** ��������: ppcUnalignedHandle
** ��������: PowerPC �Ƕ��봦��
** �䡡��  : pregctx           �Ĵ���������
**           pabtInfo          ��ֹ��Ϣ
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
VOID  ppcUnalignedHandle (ARCH_REG_CTX  *pregctx, PLW_VMM_ABORT  pabtInfo)
{
    INT                    iFixed;
    enum instruction_type  type;

    if (pregctx->REG_uiDar == pregctx->REG_uiPc) {
        goto  sigbus;
    }

    iFixed = fix_alignment(pregctx, &type);
    if (iFixed == 1) {
        pregctx->REG_uiPc += 4;                                         /*  Skip over emulated inst     */
        pabtInfo->VMABT_uiType = LW_VMM_ABORT_TYPE_NOINFO;
        return;

    } else if (iFixed == -EFAULT) {                                     /*  Operand address was bad     */
        switch (type) {

        case STORE:
        case STORE_MULTI:
        case STORE_FP:
        case STORE_VMX:
        case STORE_VSX:
        case STCX:
            pabtInfo->VMABT_uiType   = LW_VMM_ABORT_TYPE_MAP;
            pabtInfo->VMABT_uiMethod = LW_VMM_ABORT_METHOD_WRITE;
            break;

        case LOAD:
        case LOAD_MULTI:
        case LOAD_FP:
        case LOAD_VMX:
        case LOAD_VSX:
        case LARX:
        default:
            pabtInfo->VMABT_uiType   = LW_VMM_ABORT_TYPE_MAP;
            pabtInfo->VMABT_uiMethod = LW_VMM_ABORT_METHOD_READ;
            break;
        }
        return;
    }

sigbus:
    pabtInfo->VMABT_uiType   = LW_VMM_ABORT_TYPE_BUS;
    pabtInfo->VMABT_uiMethod = BUS_ADRALN;
}
/*********************************************************************************************************
  END
*********************************************************************************************************/
