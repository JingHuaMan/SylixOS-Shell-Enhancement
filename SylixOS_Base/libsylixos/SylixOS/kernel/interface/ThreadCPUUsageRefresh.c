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
** ��   ��   ��: ThreadCPUUsageRefresh.c
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2007 �� 07 �� 18 ��
**
** ��        ��: ˢ���߳�CPU������

** BUG
2007.07.18  ������ _DebugHandle() ����
2007.11.13  ʹ���������������������ȫ��װ.
2008.03.30  �ӹرյ�������Ϊ�ر��ж�. ��Ϊ��Щ�����������ж��б��޸ĵ�. �Դ�����֤׼ȷ��.
2009.09.18  ʹ���µ��㷨, ����ļ����ж�����ʱ��.
2009.12.14  ������ں�ʱ�������.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
/*********************************************************************************************************
** ��������: API_ThreadCPUUsageRefresh
** ��������: ˢ���߳�CPU������
** �䡡��  : NONE
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
                                           
                                       (�������ж��е���)
*********************************************************************************************************/
#if LW_CFG_THREAD_CPU_USAGE_CHK_EN > 0

LW_API  
ULONG  API_ThreadCPUUsageRefresh (VOID)
{
    REGISTER PLW_CLASS_TCB         ptcb;
             PLW_LIST_LINE         plineList;
             BOOL                  bNeedOn = LW_FALSE;
             
    if (LW_CPU_GET_CUR_NESTING()) {                                     /*  �������ж��е���            */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "called from ISR.\r\n");
        _ErrorHandle(ERROR_KERNEL_IN_ISR);
        return  (ERROR_KERNEL_IN_ISR);
    }
    
    if (!LW_SYS_STATUS_IS_RUNNING()) {                                  /*  ϵͳ����û������            */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "kernel is not running.\r\n");
        _ErrorHandle(ERROR_KERNEL_NOT_RUNNING);
        return  (ERROR_KERNEL_NOT_RUNNING);
    }
    
    __KERNEL_ENTER();                                                   /*  �����ں�                    */
    if (__LW_TICK_CPUUSAGE_ISENABLE()) {
        __LW_TICK_CPUUSAGE_DISABLE();                                   /*  �رղ���                    */
        bNeedOn = LW_TRUE;
    }
    
    for (plineList  = _K_plineTCBHeader;
         plineList != LW_NULL;
         plineList  = _list_line_get_next(plineList)) {
         
         ptcb = _LIST_ENTRY(plineList, LW_CLASS_TCB, TCB_lineManage);
         ptcb->TCB_ulCPUUsageTicks       = 0ul;
         ptcb->TCB_ulCPUUsageKernelTicks = 0ul;
    }
    
    _K_ulCPUUsageTicks       = 1ul;                                     /*  ����� 0 ����               */
    _K_ulCPUUsageKernelTicks = 0ul;
    
    if (bNeedOn) {
        __LW_TICK_CPUUSAGE_ENABLE();                                    /*  ���´򿪲���                */
    }
    __KERNEL_EXIT();                                                    /*  �˳��ں�                    */
    
    
    return  (ERROR_NONE);
}

#endif                                                                  /*  LW_CFG_THREAD_CPU_...       */
/*********************************************************************************************************
  END
*********************************************************************************************************/
