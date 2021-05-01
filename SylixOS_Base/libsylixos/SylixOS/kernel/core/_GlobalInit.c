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
** ��   ��   ��: _GlobalInit.c
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2006 �� 12 �� 13 ��
**
** ��        ��: ����ϵͳ��ʼ�������⡣

** BUG
2007.03.22  ����ϵͳû������ʱ�Ĵ��������
2007.04.12  �����ж϶�ջ������ַ��ʼ��
2007.04.12  ����ж϶�ջ
2007.06.06  ����ж�Ƕ�ײ�������
2008.01.20  ȡ�����̵߳���������ȫ�ֱ�����ʼ��.
2008.03.29  ��ʼ���ֻ������ĵȴ�����Ϳ��Ź����������ͷ.
2009.04.29  ����� SMP ����ں����ĳ�ʼ��.
2009.10.12  ����� CPU ID �ĳ�ʼ��.
2009.11.01  ����ע��.
            10.31��, �ҹ�ΰ��Ŀ�ѧ��Ǯѧɭ����, ����98��. ���߶�Ǯ���ޱȾ���! 
            ����<ʿ��ͻ��>�и߳ϵ�һ�仰׷˼Ǯ��, "�����ⶫ��, ���治��˵������, ����������!". 
            Ҳ��������Լ�.
2010.01.22  �����ں��߳�ɨ����β�ĳ�ʼ��.
2010.08.03  ���� tick spinlock ��ʼ��.
2012.07.04  �ϲ� hook ��ʼ��.
2012.09.11  _GlobalInit() �м���� FPU �ĳ�ʼ��.
2013.12.19  ȥ�� FPU �ĳ�ʼ��, ���� bsp �ں˳�ʼ���ص��н���, �û���Ҫָ�� fpu ������.
2017.08.17  �ж϶�ջ ARCH_STK_ALIGN_SIZE �ֽڶ���, ȷ����ͬ��ϵ�ܹ���ȫ.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#define  __KERNEL_NCPUS_SET
#define  __KERNEL_MAIN_FILE                                             /*  ����ϵͳ���ļ�              */
#include "../SylixOS/kernel/include/k_kernel.h"
/*********************************************************************************************************
  �ж϶�ջ����
*********************************************************************************************************/
LW_STACK    _K_stkInterruptStack[LW_CFG_MAX_PROCESSORS][LW_CFG_INT_STK_SIZE / sizeof(LW_STACK)];
/*********************************************************************************************************
** ��������: __interPrimaryStackInit
** ��������: ��ʼ���ж϶�ջ, (SylixOS �� SMP ��ÿһ�� CPU �����Խ����ж�)
** �䡡��  : NONE
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static VOID  __interPrimaryStackInit (VOID)
{
    REGISTER INT            i;
             PLW_CLASS_CPU  pcpu;
             PLW_STACK      pstk;
    
    LW_SPIN_INIT(&_K_slcaVectorTable.SLCA_sl);
    
    for (i = 0; i < LW_CFG_MAX_PROCESSORS; i++) {
        pcpu = LW_CPU_GET(i);
#if CPU_STK_GROWTH == 0
        pstk = &_K_stkInterruptStack[i][0];
        pcpu->CPU_pstkInterBase = (PLW_STACK)ROUND_UP(pstk, ARCH_STK_ALIGN_SIZE);
#else
        pstk = &_K_stkInterruptStack[i][(LW_CFG_INT_STK_SIZE / sizeof(LW_STACK)) - 1];
        pcpu->CPU_pstkInterBase = (PLW_STACK)ROUND_DOWN(pstk, ARCH_STK_ALIGN_SIZE);
#endif                                                                  /*  CPU_STK_GROWTH              */
        lib_memset(_K_stkInterruptStack[i], LW_CFG_STK_EMPTY_FLAG, LW_CFG_INT_STK_SIZE);
    }
}
/*********************************************************************************************************
** ��������: __interSecondaryStackInit
** ��������: ��ʼ���ж϶�ջ
** �䡡��  : ulCPUId   CPU ID
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
#if LW_CFG_SMP_EN > 0

static VOID  __interSecondaryStackInit (ULONG   ulCPUId)
{
    lib_memset(_K_stkInterruptStack[ulCPUId], LW_CFG_STK_EMPTY_FLAG, LW_CFG_INT_STK_SIZE);
}

#endif                                                                  /*  LW_CFG_SMP_EN > 0           */
/*********************************************************************************************************
** ��������: __cpuPrimaryInit
** ��������: ����ϵͳ CPU ���ƿ�ṹ��ʼ��
** �䡡��  : NONE
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
** ע  ��  : Ϊ�˷�ֹ��Щ�������� CACHE ��֮ǰ����ʹ��ԭ��ָ��, ����ֱ��ʹ�ø�ֵ��ʽ.
*********************************************************************************************************/
static VOID  __cpuPrimaryInit (VOID)
{
    REGISTER INT    i;
    
    for (i = 0; i < LW_CFG_MAX_PROCESSORS; i++) {
        LW_CPU_GET(i)->CPU_ulStatus = 0ul;                              /*  CPU INACTIVE                */
        LW_SPIN_INIT(&_K_tcbDummy[i].TCB_slLock);                       /*  ��ʼ��������                */
        
#if LW_CFG_SMP_EN > 0
        LW_CPU_ONLY_AFFINITY_SET(LW_CPU_GET(i), LW_FALSE);
        LW_CPU_GET(i)->CPU_iIPIPend.counter = 0;                        /*  ��������жϱ�־            */
#endif                                                                  /*  LW_CFG_SMP_EN > 0           */
    }
}
/*********************************************************************************************************
** ��������: __cpuSecondaryInit
** ��������: ����ϵͳ CPU ���ƿ�ṹ��ʼ��
** �䡡��  : ulCPUId   CPU ID
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
#if LW_CFG_SMP_EN > 0

static VOID  __cpuSecondaryInit (ULONG   ulCPUId)
{
    LW_CPU_GET(ulCPUId)->CPU_ulStatus = 0ul;                            /*  CPU INACTIVE                */
    LW_SPIN_INIT(&_K_tcbDummy[ulCPUId].TCB_slLock);                     /*  ��ʼ��������                */
    
#if LW_CFG_SMP_EN > 0
    LW_CPU_ONLY_AFFINITY_SET(LW_CPU_GET(ulCPUId), LW_FALSE);
    LW_CPU_GET(ulCPUId)->CPU_iIPIPend.counter = 0;                      /*  ��������жϱ�־            */
#endif                                                                  /*  LW_CFG_SMP_EN > 0           */
}

#endif                                                                  /*  LW_CFG_SMP_EN > 0           */
/*********************************************************************************************************
** ��������: __miscPrimarySmpInit
** ��������: �� SMP �йص�ȫ�ֱ�����ʼ��
** �䡡��  : NONE
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static VOID  __miscPrimarySmpInit (VOID)
{
    REGISTER INT            i;
#if (LW_CFG_SMP_EN > 0) && (LW_CFG_CPU_ARCH_SMT > 0)
             ULONG          ulMaxPhyId = 0;
#endif                                                                  /*  LW_CFG_CPU_ARCH_SMT > 0     */
             PLW_CLASS_CPU  pcpu;
    
    for (i = 0; i < LW_CFG_MAX_PROCESSORS; i++) {
        pcpu = LW_CPU_GET(i);
        
        LW_CAND_TCB(pcpu) = LW_NULL;                                    /*  ��ѡ���б�Ϊ��              */
        LW_CAND_ROT(pcpu) = LW_FALSE;                                   /*  û�����ȼ�����              */
        
        pcpu->CPU_ulCPUId = (ULONG)i;
        pcpu->CPU_iKernelCounter = 1;                                   /*  ��ʼ�� 1, ��ǰ���������    */

#if LW_CFG_SMP_EN > 0
        LW_CPU_ONLY_AFFINITY_SET(pcpu, LW_FALSE);
#if LW_CFG_CACHE_EN > 0
        pcpu->CPU_bCacheBarStart = LW_FALSE;
        pcpu->CPU_bCacheBarEnd   = LW_FALSE;
#endif                                                                  /*  LW_CFG_CACHE_EN > 0         */
        pcpu->CPU_ulIPIVector = __ARCH_ULONG_MAX;                       /*  Ŀǰ��ȷ���˼��ж�����      */
        LW_SPIN_INIT(&pcpu->CPU_slIpi);                                 /*  ��ʼ�� CPU spinlock         */
#endif                                                                  /*  LW_CFG_SMP_EN > 0           */
    }
    
#if (LW_CFG_SMP_EN > 0) && (LW_CFG_CPU_ARCH_SMT > 0)
    LW_CPU_FOREACH (i) {
        pcpu = LW_CPU_GET(i);
        pcpu->CPU_ulPhyId = bspCpuLogic2Physical((ULONG)i);             /*  ������� CPU ID             */
        _BugFormat(pcpu->CPU_ulPhyId >= LW_NCPUS, LW_TRUE, 
                   "Physical CPU ID error: %lu\r\n", pcpu->CPU_ulPhyId);
        if (ulMaxPhyId < pcpu->CPU_ulPhyId) {
            ulMaxPhyId = pcpu->CPU_ulPhyId;
        }
    }
    _K_ulNPhyCpus = ulMaxPhyId + 1;                                     /*  ͳ������ CPU ����           */
#endif                                                                  /*  LW_CFG_CPU_ARCH_SMT > 0     */
}
/*********************************************************************************************************
** ��������: __miscSecondarySmpInit
** ��������: �� SMP �йص�ȫ�ֱ�����ʼ��
** �䡡��  : ulCPUId   CPU ID
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
#if LW_CFG_SMP_EN > 0

static VOID  __miscSecondarySmpInit (ULONG   ulCPUId)
{
    PLW_CLASS_CPU  pcpu = LW_CPU_GET(ulCPUId);
    
    LW_CAND_TCB(pcpu) = LW_NULL;                                        /*  ��ѡ���б�Ϊ��              */
    LW_CAND_ROT(pcpu) = LW_FALSE;                                       /*  û�����ȼ�����              */
    
    pcpu->CPU_iKernelCounter = 1;                                       /*  ��ʼ�� 1, ��ǰ���������    */
    pcpu->CPU_ulIPIVector    = __ARCH_ULONG_MAX;                        /*  Ŀǰ��ȷ���˼��ж�����      */
    LW_SPIN_INIT(&pcpu->CPU_slIpi);                                     /*  ��ʼ�� CPU spinlock         */
}

#endif                                                                  /*  LW_CFG_SMP_EN > 0           */
/*********************************************************************************************************
** ��������: _GlobalPrimaryInit
** ��������: ��ʼ����ɢȫ�ֱ���
** �䡡��  : NONE
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
VOID _GlobalPrimaryInit (VOID)
{
    LW_SYS_STATUS_SET(LW_SYS_STATUS_INIT);                              /*  ϵͳ״̬Ϊ��ʼ��״̬        */

    /*
     *  �ں˹ؼ�����������ʼ��
     */
    LW_SPIN_INIT(&_K_klKernel.KERN_slLock);                             /*  ��ʼ���ں�������            */
    LW_SPIN_INIT(&_K_slKernTime.SLCA_sl);                               /*  ��ʼ���ں�ʱ��������        */
    LW_SPIN_INIT(&_K_slcaAtomic.SLCA_sl);                               /*  ��ʼ��ԭ�Ӳ���������        */

    /*
     *  �ں˹ؼ������ݽṹ��ʼ��
     */
    __cpuPrimaryInit();                                                 /*  CPU �ṹ��ʼ��              */
    __interPrimaryStackInit();                                          /*  ���ȳ�ʼ���ж϶�ջ          */
    __miscPrimarySmpInit();                                             /*  SMP ��س�ʼ��              */
    
    /*
     *  �ں˹ؼ���״̬������ʼ��
     */
    _K_atomic64KernelTime.counter = 0;
    
#if LW_CFG_THREAD_CPU_USAGE_CHK_EN > 0
    _K_ulCPUUsageTicks       = 1ul;                                     /*  ����� 0 ����               */
    _K_ulCPUUsageKernelTicks = 0ul;
#endif

    _K_usThreadCounter = 0;                                             /*  �߳�����                    */
    _K_plineTCBHeader  = LW_NULL;                                       /*  TCB ��������ͷ              */
    _K_ulNotRunError   = 0ul;                                           /*  ϵͳδ����ʱ�����ű���    */
    
    __WAKEUP_INIT(&_K_wuDelay, LW_NULL, LW_NULL);
#if LW_CFG_SOFTWARE_WATCHDOG_EN > 0
    __WAKEUP_INIT(&_K_wuWatchDog, LW_NULL, LW_NULL);
#endif                                                                  /*  LW_CFG_SOFTWARE_WATCHDOG_EN */
    
#if LW_CFG_THREAD_CPU_USAGE_CHK_EN > 0
    __LW_TICK_CPUUSAGE_DISABLE();                                       /*  �ر� CPU �����ʲ���         */
#endif                                                                  /*  LW_CFG_THREAD_CPU_USAGE_... */
}
/*********************************************************************************************************
** ��������: _GlobalSecondaryInit
** ��������: ��ʼ����ɢȫ�ֱ���
** �䡡��  : NONE
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
#if LW_CFG_SMP_EN > 0

VOID _GlobalSecondaryInit (VOID)
{
    ULONG   ulCPUId = LW_CPU_GET_CUR_ID();
    
    __cpuSecondaryInit(ulCPUId);                                        /*  CPU �ṹ��ʼ��              */
    __interSecondaryStackInit(ulCPUId);                                 /*  ���ȳ�ʼ���ж϶�ջ          */
    __miscSecondarySmpInit(ulCPUId);                                    /*  SMP ��س�ʼ��              */
}

#endif
/*********************************************************************************************************
  END
*********************************************************************************************************/
