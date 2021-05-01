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
** ��   ��   ��: _KernelLowLevelInit.c
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2006 �� 12 �� 14 ��
**
** ��        ��: ����ϵͳ�ں˵ײ��ʼ�������⡣

** BUG
2007.04.12  ��ȫ�ֱ�����ʼ�����ڳ�ʼ���ʼ
2007.07.13  ���� _DebugHandle() ��Ϣ���ܡ�
2007.11.08  ���û��Ѹ�Ϊ�ں˶�.
2008.01.24  �޸�������˳��.
2009.04.06  �����˶�ջ����ʼ��.
2014.11.12  �ں˵ײ��ʼ����Ϊ���Ӻ˷ֿ�.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
/*********************************************************************************************************
** ��������: _KernelPrimaryLowLevelInit
** ��������: �ں˵ײ��ʼ��
** �䡡��  : pvKernelHeapMem   �ں˶��׵�ַ
**           stKernelHeapSize  �ں˶Ѵ�С
**           pvSystemHeapMem   ϵͳ���׵�ַ
**           stSystemHeapSize  ϵͳ�Ѵ�С
** �䡡��  : 
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
#if LW_CFG_MEMORY_HEAP_CONFIG_TYPE > 0
VOID  _KernelPrimaryLowLevelInit (PVOID     pvKernelHeapMem,
                                  size_t    stKernelHeapSize,
                                  PVOID     pvSystemHeapMem,
                                  size_t    stSystemHeapSize)
#else
VOID  _KernelPrimaryLowLevelInit (VOID)
#endif                                                                  /*  LW_CFG_MEMORY_HEAP_...      */
{
    PLW_CLASS_CPU   pcpuCur;

    _DebugHandle(__LOGMESSAGE_LEVEL, "kernel low level initialize...\r\n");
    
    _GlobalPrimaryInit();                                               /*  ȫ�ֱ�����ʼ��              */
    
    _ScheduleInit();                                                    /*  ��������ʼ��                */
    _StackCheckInit();                                                  /*  ��ջ����ʼ��              */
    _InterVectInit();                                                   /*  ��ʼ���ж�������            */
    _ReadyTableInit();                                                  /*  �������ʼ��                */
    _ThreadIdInit();                                                    /*  Thread ID ��ʼ��            */
    _EventInit();                                                       /*  �¼���ʼ��                  */
    _ThreadVarInit();                                                   /*  ȫ�ֱ���˽�л�ģ���ʼ��    */
    _TimerInit();                                                       /*  ��ʼ����ʱ��                */
    _TimeCvtInit();                                                     /*  ��ʼ����ʱ���㺯��          */
    _PriorityInit();                                                    /*  ��ʼ�����ȼ����ƿ����      */
    _EventSetInit();                                                    /*  ��ʼ���¼���                */
    _PartitionInit();                                                   /*  ��ʼ�������ڴ����          */
    _MsgQueueInit();                                                    /*  ��ʼ����Ϣ����              */
    _RmsInit();                                                         /*  ��ʼ�����ȵ���������        */
    _RtcInit();                                                         /*  RTC��Ԫ��ʼ��               */
    _HeapInit();                                                        /*  �ѳ�ʼ�����䳤�ڴ����      */
    
    /*
     *  ����ĳ�ʼ�����ܻ��õ���ǰ CPU ����ִ�е��߳���Ϣ, ����ֻ�Ǳ�֤ CPU_ptcbTCBCur != NULL
     *  ע��, ��ǰ�ǹر��ж�״̬, ��ǰ�� CPU ID ��Ϊ���� CPU.
     */
    pcpuCur = LW_CPU_GET_CUR();
    pcpuCur->CPU_ptcbTCBCur = &_K_tcbDummy[LW_CPU_GET_ID(pcpuCur)];     /*  α�ں��߳�                  */
    pcpuCur->CPU_ulStatus  |= LW_CPU_STATUS_RUNNING;
    KN_SMP_WMB();
    
    _DebugHandle(__LOGMESSAGE_LEVEL, "kernel heap build...\r\n");
#if LW_CFG_MEMORY_HEAP_CONFIG_TYPE > 0
    _HeapKernelInit(pvKernelHeapMem, stKernelHeapSize);
#else
    _HeapKernelInit();                                                  /*  �����ں˶�                  */
#endif                                                                  /*  LW_CFG_MEMORY_HEAP_...      */

    _DebugHandle(__LOGMESSAGE_LEVEL, "system heap build...\r\n");
#if LW_CFG_MEMORY_HEAP_CONFIG_TYPE > 0
    _HeapSystemInit(pvSystemHeapMem, stSystemHeapSize);
#else
    _HeapSystemInit();                                                  /*  ����ϵͳ��                  */
#endif                                                                  /*  LW_CFG_MEMORY_HEAP_...      */

    LW_KERNEL_JOB_INIT();                                               /*  ��ʼ���ں˹�������          */
}
/*********************************************************************************************************
** ��������: _KernelSecondaryLowLevelInit
** ��������: �ں˵ײ��ʼ��
** �䡡��  : NONE
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
#if LW_CFG_SMP_EN > 0

VOID  _KernelSecondaryLowLevelInit (VOID)
{
    PLW_CLASS_CPU   pcpuCur;

    _DebugHandle(__LOGMESSAGE_LEVEL, "kernel secondary low level initialize...\r\n");
    
    _GlobalSecondaryInit();                                             /*  ȫ�ֱ�����ʼ��              */
    
    pcpuCur = LW_CPU_GET_CUR();
    pcpuCur->CPU_ptcbTCBCur = &_K_tcbDummy[LW_CPU_GET_ID(pcpuCur)];     /*  α�ں��߳�                  */
    pcpuCur->CPU_ulStatus  |= LW_CPU_STATUS_RUNNING;
    KN_SMP_WMB();
}

#endif                                                                  /*  LW_CFG_SMP_EN > 0           */
/*********************************************************************************************************
  END
*********************************************************************************************************/
