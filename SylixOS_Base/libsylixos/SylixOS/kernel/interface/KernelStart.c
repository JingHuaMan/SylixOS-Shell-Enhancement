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
** ��   ��   ��: KernelStart.c
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2006 �� 12 �� 13 ��
**
** ��        ��: ����ϵͳ�ں���ں�����

** BUG
2007.03.22  ȥ����ԭʼ��ϵͳδ����ʱ�Ĵ���ػ����
2007.07.11  _KernelEntry() �н� OSInitTickHook() ���� _SchedSeekPriority() ֮ǰ���Ա�֤���ȼ�������ȷ
2007.07.13  ���� _DebugHandle() ��Ϣ���ܡ�
2008.07.27  ���붯̬�ѵ�ַȷ������.
2008.12.03  ���жϳ�ʼ����ǰ.
2009.04.17  �� OSInitTickHook() �������.
2011.03.08  �ڵ��� pfuncStartHook ֮ǰ, ��ʼ�� c++ ����ʱ��.
2012.03.19  _KernelEntry() �����״ε���Ǯ, ��ʼ�����д�������������.
2012.03.24  ����������������.
2012.04.06  ���� _KernelSMPStartup() �������������� CPU ��ʼ��ת.
2012.09.11  ���� kfpu ��������
2012.09.12  OSStartHighRdy() ����������ǰ, ��Ҫ��ʼ������������� FPU ����.
2013.05.01  �ں���������������ڴ�Խ�����ؼ��.
2013.07.17  �����µĶ�˳�ʼ������.
2014.08.08  ���ں����������������ڶ������ļ���.
2015.05.07  �Ż� SMP ϵͳ���Ӻ���������.
2016.11.25  ����� MIPS ��ϵ�ṹ, ������ʱ��ʼ�� FPU ģ����, ����ģ�� CPU ��֧�ֵĸ�������ָ��.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
/*********************************************************************************************************
  SMP ͬ������
*********************************************************************************************************/
#if LW_CFG_SMP_EN > 0
static volatile BOOL            _K_bPrimaryGo = LW_FALSE;               /*  �����Ƿ��Ѿ���ʼ������״̬  */
#define KN_PRIMARY_GO()         _K_bPrimaryGo = LW_TRUE;
#define KN_PRIMARY_IS_GO()      _K_bPrimaryGo

static volatile ULONG           _K_ulSecondaryHold  = 0ul;              /*  �Ӻ˵ȴ�                    */
#define KN_SECONDARY_GO()       _K_ulSecondaryHold  = 0xdeadbeef;
#define KN_SECONDARY_WAIT()     _K_ulSecondaryHold != 0xdeadbeef
#endif                                                                  /*  LW_CFG_SMP_EN               */
/*********************************************************************************************************
  ��������
*********************************************************************************************************/
extern INT  _cppRtInit(VOID);
/*********************************************************************************************************
** ��������: _KernelPrimaryCoreStartup
** ��������: ϵͳ������ (�����ʼ���ĺ�) ���������״̬, ���������״̬��, ���к˾��ԵȲ�������
** �䡡��  : pcpuCur   ��ǰ CPU
** �䡡��  : 
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static  VOID  _KernelPrimaryCoreStartup (PLW_CLASS_CPU   pcpuCur)
{
    PLW_CLASS_TCB   ptcbOrg;
    
    LW_SPIN_KERN_LOCK_IGNIRQ();
    LW_TCB_GET_CUR(ptcbOrg);
    _CpuActive(pcpuCur);                                                /*  CPU ����                    */
    LW_SPIN_KERN_UNLOCK_SCHED(ptcbOrg);
    
#if LW_CFG_SMP_EN > 0
    KN_SMP_MB();                                                        /*  �ڴ�����, ȷ��֮ǰ�����Ѵ���*/
    KN_PRIMARY_GO();                                                    /*  ֪ͨ�Ӻ˿��Խ��������ģʽ  */
    LW_SPINLOCK_NOTIFY();
#endif                                                                  /*  LW_CFG_SMP_EN               */

    pcpuCur->CPU_iKernelCounter = 0;                                    /*  �������                    */
    KN_SMP_MB();

    _DebugHandle(__LOGMESSAGE_LEVEL, "primary cpu multi-task start...\r\n");
    
#if LW_CFG_CPU_FPU_EN > 0
    _ThreadFpuSwitch(LW_FALSE);                                         /*  ��ʼ����Ҫ�������� FPU ���� */
#endif

#if LW_CFG_CPU_DSP_EN > 0
    _ThreadDspSwitch(LW_FALSE);                                         /*  ��ʼ����Ҫ�������� DSP ���� */
#endif

    errno = ERROR_NONE;                                                 /*  �������                    */
    archTaskCtxStart(pcpuCur);                                          /*  ��ǰ CPU ��������״̬       */
}
/*********************************************************************************************************
** ��������: _KernelSecondaryCoreStartup
** ��������: ϵͳ�ĴӺ� (�ǳ�ʼ����) ���������״̬, ���������״̬��, ���к˾��ԵȲ�������
** �䡡��  : pcpuCur   ��ǰ CPU
** �䡡��  : 
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
#if LW_CFG_SMP_EN > 0

static  VOID  _KernelSecondaryCoreStartup (PLW_CLASS_CPU   pcpuCur)
{
    PLW_CLASS_TCB   ptcbOrg;
    
    KN_SMP_MB();
    while (KN_PRIMARY_IS_GO() == LW_FALSE) {                            /*  �ȴ���������                */
        LW_SPINLOCK_DELAY();                                            /*  ���ӳٲ��ͷ�����            */
    }

    LW_SPIN_KERN_LOCK_IGNIRQ();
    LW_TCB_GET_CUR(ptcbOrg);
    _CpuActive(pcpuCur);                                                /*  CPU ����                    */
    LW_SPIN_KERN_UNLOCK_SCHED(ptcbOrg);
    
    pcpuCur->CPU_iKernelCounter = 0;                                    /*  �������                    */
    KN_SMP_MB();
    
    _DebugHandle(__LOGMESSAGE_LEVEL, "secondary cpu multi-task start...\r\n");
    
#if LW_CFG_CPU_FPU_EN > 0
    _ThreadFpuSwitch(LW_FALSE);                                         /*  ��ʼ����Ҫ�������� FPU ���� */
#endif

#if LW_CFG_CPU_DSP_EN > 0
    _ThreadDspSwitch(LW_FALSE);                                         /*  ��ʼ����Ҫ�������� DSP ���� */
#endif

    errno = ERROR_NONE;                                                 /*  �������                    */
    archTaskCtxStart(pcpuCur);                                          /*  ��ǰ CPU ��������״̬       */
}

#endif                                                                  /*  LW_CFG_SMP_EN               */
/*********************************************************************************************************
** ��������: _KernelPrimaryEntry
** ��������: ����ϵͳ�ں��ڲ����
** �䡡��  : pcpuCur   ��ǰ CPU
** �䡡��  : 
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static  VOID  _KernelPrimaryEntry (PLW_CLASS_CPU   pcpuCur)
{
    if (LW_SYS_STATUS_IS_RUNNING()) {                                   /*  ϵͳ����û������            */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "kernel is already run! can not re-enter\r\n");
        _ErrorHandle(ERROR_KERNEL_RUNNING);
        return;
    }
    
    _DebugHandle(__LOGMESSAGE_LEVEL, "kernel ticks initialize...\r\n");
    bspTickInit();                                                      /*  ��ʼ������ϵͳʱ��          */

    LW_SYS_STATUS_SET(LW_SYS_STATUS_RUNNING);                           /*  ������������              */
    
    _KernelPrimaryCoreStartup(pcpuCur);                                 /*  ����(��ǰ��)����            */
}
/*********************************************************************************************************
** ��������: _KernelBootSecondary
** ��������: ϵͳ�����˼��ж�, �Ӻ�����
** �䡡��  : NONE
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
#if LW_CFG_SMP_EN > 0

static VOID  _KernelBootSecondary (VOID)
{
    KN_SMP_MB();                                                        /*  �ڴ�����, ȷ��֮ǰ�����Ѵ���*/
    KN_SECONDARY_GO();                                                  /*  ֪ͨ�Ӻ˿��Խ��л�����ʼ��  */
    LW_SPINLOCK_NOTIFY();
}

#endif                                                                  /*  LW_CFG_SMP_EN               */
/*********************************************************************************************************
** ��������: API_KernelPrimaryStart
** ��������: ϵͳ�ں���� (ֻ����ϵͳ�߼����˵���, һ��Ϊ CPU 0, ��ǰΪ���ж�״̬)
** �䡡��  : pfuncStartHook     ϵͳ�����е��û��ص�
**           pvKernelHeapMem    �ں˶��ڴ��׵�ַ
**           stKernelHeapSize   �ں˶Ѵ�С
**           pvSystemHeapMem    ϵͳ���ڴ��׵�ַ
**           stSystemHeapSize   ϵͳ�Ѵ�С
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
                                       API ����
*********************************************************************************************************/
#if LW_CFG_MEMORY_HEAP_CONFIG_TYPE > 0
LW_API
VOID  API_KernelPrimaryStart (PKERNEL_START_ROUTINE  pfuncStartHook,
                              PVOID                  pvKernelHeapMem,
                              size_t                 stKernelHeapSize,
                              PVOID                  pvSystemHeapMem,
                              size_t                 stSystemHeapSize)
#else
LW_API
VOID  API_KernelPrimaryStart (PKERNEL_START_ROUTINE  pfuncStartHook)
#endif                                                                  /*  LW_CFG_MEMORY_HEAP_...      */
{
    INT     iError;
    
    _BugHandle(LW_SYS_STATUS_IS_RUNNING(), LW_TRUE, "kernel is already start.\r\n");
    _BugHandle(LW_NCPUS > LW_CFG_MAX_PROCESSORS, LW_TRUE, "LW_NCPUS > LW_CFG_MAX_PROCESSORS.\r\n");
    _DebugHandle(__LOGMESSAGE_LEVEL, "longwing(TM) kernel initialize...\r\n");
    
#if LW_CFG_MEMORY_HEAP_CONFIG_TYPE > 0
    _BugHandle(!pvKernelHeapMem, LW_TRUE, "heap memory invalidate.\r\n");
    _BugHandle(!_Addresses_Is_Aligned(pvKernelHeapMem), LW_TRUE, "heap memory is not align.\r\n");
    _BugHandle(stKernelHeapSize < LW_KERNEL_HEAP_MIN_SIZE, LW_TRUE, "heap memory is too little.\r\n");
    
    if (pvSystemHeapMem && stSystemHeapSize) {
        _BugHandle(!pvSystemHeapMem, LW_TRUE, "heap memory invalidate.\r\n");
        _BugHandle(!_Addresses_Is_Aligned(pvSystemHeapMem), LW_TRUE, "heap memory is not align.\r\n");
        _BugHandle(stSystemHeapSize < LW_SYSTEM_HEAP_MIN_SIZE, LW_TRUE, "heap memory is too little.\r\n");
    }
    _KernelPrimaryLowLevelInit(pvKernelHeapMem, stKernelHeapSize,
                               pvSystemHeapMem, stSystemHeapSize);      /*  �ں˵ײ��ʼ��              */
#else
    _KernelPrimaryLowLevelInit();                                       /*  �ں˵ײ��ʼ��              */
#endif                                                                  /*  LW_CFG_MEMORY_HEAP_...      */
    
    _DebugHandle(__LOGMESSAGE_LEVEL, "kernel interrupt vector initialize...\r\n");
    bspIntInit();                                                       /*  ��ʼ���ж�ϵͳ              */
    
    _KernelHighLevelInit();                                             /*  �ں˸߼���ʼ��              */
    
    iError = _cppRtInit();                                              /*  CPP ����ʱ���ʼ��          */
    if (iError != ERROR_NONE) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "c++ run time lib initialized error!\r\n");
    
    } else {
        _DebugHandle(__LOGMESSAGE_LEVEL, "c++ run time lib initialized.\r\n");
    }
    
    _DebugHandle(__LOGMESSAGE_LEVEL, "kernel primary cpu usrStartup...\r\n");
    if (pfuncStartHook) {                                               /*  �û��Ƿ�Ҫ����Ҫ��ʼ��      */
        pfuncStartHook();                                               /*  �û�ϵͳ��ʼ��              */
    }
    
#if defined(LW_CFG_CPU_ARCH_MIPS) && (LW_CFG_CPU_FPU_EN > 0)
    archFpuEmuInit();                                                   /*  FPU ģ������ʼ��            */
#endif                                                                  /*  LW_CFG_CPU_ARCH_MIPS        */
    
#if LW_CFG_MODULELOADER_EN > 0
    _resInit();                                                         /*  �����ڿ�ʼ��¼��Դ���      */
#endif                                                                  /*  LW_CFG_MODULELOADER_EN > 0  */
    
#if LW_CFG_SMP_EN > 0
    _KernelBootSecondary();                                             /*  �Ӻ˿��Խ��г�ʼ������      */
#endif                                                                  /*  LW_CFG_SMP_EN               */

    _KernelPrimaryEntry(LW_CPU_GET_CUR());                              /*  �����ں�                    */
                                                                        /*  ��˽��ڵ�һ�ε��ȱ�����    */
}
/*********************************************************************************************************
** ��������: API_KernelSecondaryStart
** ��������: �Ӻ�ϵͳ�ں���� (ֻ����ϵͳ���˵���, Ҳ���Ƿ����� CPU, ��ǰΪ���ж�״̬)
** �䡡��  : pfuncStartHook            ��ʼ���� CPU ������Դ, ���� MMU, CACHE, FPU ��
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
                                       API ����
*********************************************************************************************************/
#if LW_CFG_SMP_EN > 0

VOID  API_KernelSecondaryStart (PKERNEL_START_ROUTINE  pfuncStartHook)
{
    KN_SMP_MB();
    while (KN_SECONDARY_WAIT()) {
        LW_SPINLOCK_DELAY();                                            /*  ���ӳٲ��ͷ�����            */
    }
    
    _KernelSecondaryLowLevelInit();                                     /*  �Ӻ˵ײ��ʼ��              */
    
    _DebugHandle(__LOGMESSAGE_LEVEL, "kernel secondary cpu usrStartup...\r\n");
    if (pfuncStartHook) {                                               /*  �û��Ƿ�Ҫ����Ҫ��ʼ��      */
        pfuncStartHook();                                               /*  �û�ϵͳ��ʼ��              */
    }

    _KernelSecondaryCoreStartup(LW_CPU_GET_CUR());                      /*  ���˳�ʼ�����ֱ������������*/
}

#endif                                                                  /*  LW_CFG_SMP_EN               */
/*********************************************************************************************************
  END
*********************************************************************************************************/
