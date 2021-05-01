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
** ��   ��   ��: InterStack.c
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2007 �� 02 �� 02 ��
**
** ��        ��: ����ж϶�ջ����

** BUG
2007.04.12  LINE45 ����±�����Ӧ��Ϊ��(LW_CFG_INT_STK_SIZE / __STACK_ALIGN_DIV) - 1
2009.04.11  �����˵Ĳ���.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
/*********************************************************************************************************
  �ж϶�ջ����
*********************************************************************************************************/
extern LW_STACK     _K_stkInterruptStack[LW_CFG_MAX_PROCESSORS][LW_CFG_INT_STK_SIZE / sizeof(LW_STACK)];
/*********************************************************************************************************
** ��������: API_InterStackBaseGet
** ��������: ����ж϶�ջջ�� (��ǰ���������ڵ� CPU, ���ж�ģʽ�±�����)
** �䡡��  : NONE
** �䡡��  : �ж�ջ��
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
PVOID  API_InterStackBaseGet (VOID)
{
    return  (LW_CPU_GET_CUR()->CPU_pstkInterBase);
}
/*********************************************************************************************************
** ��������: API_InterStackCheck
** ��������: ����ж϶�ջʹ����
** �䡡��  : ulCPUId                       CPU ��
**           pstFreeByteSize               ���ж�ջ��С   (��Ϊ LW_NULL)
**           pstUsedByteSize               ʹ�ö�ջ��С   (��Ϊ LW_NULL)
** �䡡��  : 
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API
VOID  API_InterStackCheck (ULONG   ulCPUId,
                           size_t *pstFreeByteSize,
                           size_t *pstUsedByteSize)
{
    REGISTER size_t                stFree = 0;
    REGISTER PLW_STACK             pstkButtom;
    
    if (ulCPUId >= LW_NCPUS) {
        _ErrorHandle(ERROR_KERNEL_CPU_NULL);
        return;
    }
    
#if CPU_STK_GROWTH == 0
    for (pstkButtom = &_K_stkInterruptStack[ulCPUId][(LW_CFG_INT_STK_SIZE / sizeof(LW_STACK)) - 1];
         ((*pstkButtom == _K_stkFreeFlag) && (stFree < LW_CFG_INT_STK_SIZE / sizeof(LW_STACK)));
         pstkButtom--,
         stFree++);
#else
    for (pstkButtom = &_K_stkInterruptStack[ulCPUId][0];
         ((*pstkButtom == _K_stkFreeFlag) && (stFree < LW_CFG_INT_STK_SIZE / sizeof(LW_STACK)));
         pstkButtom++,
         stFree++);
#endif

    if (pstFreeByteSize) {
        *pstFreeByteSize = stFree * sizeof(LW_STACK);
    }
    
    if (pstUsedByteSize) {
        *pstUsedByteSize = LW_CFG_INT_STK_SIZE - (stFree * sizeof(LW_STACK));
    }
}
/*********************************************************************************************************
  END
*********************************************************************************************************/
