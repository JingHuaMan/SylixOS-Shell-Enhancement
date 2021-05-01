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
** ��   ��   ��: _ReadyTableLib.c
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2014 �� 11 �� 11 ��
**
** ��        ��: ����ϵͳ��������������⡣
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
/*********************************************************************************************************
** ��������: _ReadyTableAdd
** ��������: ָ���̼߳��������
** �䡡��  : ptcb      �߳̿��ƿ�
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
VOID  _ReadyTableAdd (PLW_CLASS_TCB  ptcb)
{
    PLW_CLASS_PCBBMAP   ppcbbmap;
    
#if LW_CFG_SMP_EN > 0
    if (ptcb->TCB_bCPULock) {
        ppcbbmap = LW_CPU_RDY_PCBBMAP(LW_CPU_GET(ptcb->TCB_ulCPULock));
    } else 
#endif                                                                  /*  LW_CFG_SMP_EN > 0           */    
    {
        ppcbbmap = LW_GLOBAL_RDY_PCBBMAP();
    }
    
    _BitmapAdd(&ppcbbmap->PCBM_bmap, ptcb->TCB_ucPriority);
}
/*********************************************************************************************************
** ��������: _ReadyTableDel
** ��������: ָ���߳��˳�������
** �䡡��  : ptcb      �߳̿��ƿ�
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
VOID  _ReadyTableDel (PLW_CLASS_TCB  ptcb)
{
    PLW_CLASS_PCBBMAP   ppcbbmap;
    
#if LW_CFG_SMP_EN > 0
    if (ptcb->TCB_bCPULock) {
        ppcbbmap = LW_CPU_RDY_PCBBMAP(LW_CPU_GET(ptcb->TCB_ulCPULock));
    } else 
#endif                                                                  /*  LW_CFG_SMP_EN > 0           */    
    {
        ppcbbmap = LW_GLOBAL_RDY_PCBBMAP();
    }
    
    _BitmapDel(&ppcbbmap->PCBM_bmap, ptcb->TCB_ucPriority);
}
/*********************************************************************************************************
  END
*********************************************************************************************************/
