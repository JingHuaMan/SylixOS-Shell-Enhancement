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
** ��   ��   ��: KernelTicks.c
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2006 �� 12 �� 18 ��
**
** ��        ��: ����ϵͳ�ں�ʵʱʱ�Ӻ�����

** ע�⣺
            ������߳��е��ã��̱߳�����������ȼ���
          
** BUG
2007.04.10  ��̬ HOOK �������뵽 LW_CFG_TIME_TICK_HOOK_EN �ж���
2007.05.07  ��ɨ�� tcb ��������ʱ���رյ��������������Է�ֹ����������£����� kernelTick ʱ���������ƻ�
2007.05.24  ���߳�ģʽ����ʱ���޷���ȡʱ���жϷ���ʱ�����е��̣߳��Ӷ��޷�����ʱ��Ƭ���������
2007.05.27  ���ﲻ��Ҫ��ʱ��Ƭ�ļ�����������ȫ���� API_KernelTicksContext() ��ɡ�
2007.05.27  ����ϵͳ�Ƿ��������жϷ����ʼ��
2007.05.27  CPU������ˢ��Ҳ����������У�ȫ���� API_KernelTicksContext() ��ɡ�
2007.05.27  �����ﲻ�ñ��浱ǰ���жϵ��̵߳��߳̿��ƿ飬ֱ�ӽ���ǰ�߳�ʱ��Ƭ��һ������
            CPU�����ʴ������������
2008.03.29  ʹ��ȫ�µ� wake up �� watch dog ����.
2009.04.14  Ϊ��֧�� SMP ���ϵͳ, ʹ�� _SchedSliceTick ����ʱ��Ƭ����.
2009.09.18  �����µĵ��ж����� CPU �����ʲ�������.
2009.10.12  ��ʱ���ж�ʱ, ��Ҫ�������е� SMP CPU, ���������������е��߳�.
2010.01.04  ʹ���µ� TOD ʱ�ӻ���.
2012.07.07  �ϲ� API_KernelTicksContext ������.
2013.08.28  �����ں��¼������.
2014.01.01  API_KernelTicksContext() ��Ҫ����������.
2014.07.04  ϵͳ�ں�֧�� tod ʱ��΢��.
2015.04.17  tod ʱ��ĸ��±�������ж���������.
2015.05.07  CLOCK_MONOTONIC ���� adjust ����.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
/*********************************************************************************************************
  �������ʱ��Ƭ����
*********************************************************************************************************/
#if LW_CFG_MODULELOADER_EN > 0
#include "../SylixOS/loader/include/loader_vppatch.h"
#endif                                                                  /*  LW_CFG_MODULELOADER_EN > 0  */
/*********************************************************************************************************
  ʱ�����
*********************************************************************************************************/
#define TOD_UPDATE(tod, step_nsec)                      \
        do {                                            \
            (tod)->tv_nsec += step_nsec;                \
            if ((tod)->tv_nsec >= __TIMEVAL_NSEC_MAX) { \
                (tod)->tv_nsec -= __TIMEVAL_NSEC_MAX;   \
                (tod)->tv_sec++;                        \
            }                                           \
        } while (0)
        
#if LW_CFG_CPU_ATOMIC64_EN > 0
#define TICK_UPDATE()                                   \
        do {                                            \
            __LW_ATOMIC64_INC(&_K_atomic64KernelTime);  \
        } while (0)
        
#else                                                                   /*  LW_CFG_CPU_ATOMIC64_EN      */
#define TICK_UPDATE()                                   \
        do {                                            \
            _K_atomic64KernelTime.counter++;            \
        } while (0)
#endif                                                                  /*  !LW_CFG_CPU_ATOMIC64_EN     */
/*********************************************************************************************************
** ��������: __kernelTODUpdate
** ��������: ֪ͨһ��ʱ�ӵ���, ���� TOD ʱ��
** �䡡��  : NONE
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
** ע  ��  : _K_i64TODDelta Ϊ adjtime ������, ��������ϵͳ tod ʱ��.
*********************************************************************************************************/
#if LW_CFG_RTC_EN > 0

static LW_INLINE VOID __kernelTODUpdate (VOID)
{
    LONG    lNsec;
    
    TOD_UPDATE(&_K_tvTODMono, LW_NSEC_PER_TICK);                        /*  CLOCK_MONOTONIC             */
    
    if (_K_iTODDelta > 0) {                                             /*  ��Ҫ�ӿ�ϵͳʱ��һ�� TICK   */
        _K_iTODDelta--;
        lNsec = LW_NSEC_PER_TICK << 1;                                  /*  �ӿ�һ�� tick               */
    
    } else if (_K_iTODDelta < 0) {
        _K_iTODDelta++;
        return;                                                         /*  ϵͳ tod ʱ��ֹͣһ�� tick  */
    
    } else if (_K_iTODDeltaNs) {
        lNsec = LW_NSEC_PER_TICK + _K_iTODDeltaNs;                      /*  ����΢��                    */
        _K_iTODDeltaNs = 0;
    
    } else {
        lNsec = LW_NSEC_PER_TICK;
    }
    
    TOD_UPDATE(&_K_tvTODCurrent, lNsec);                                /*  CLOCK_REALTIME              */
}

#endif                                                                  /*  LW_CFG_RTC_EN > 0           */
/*********************************************************************************************************
** ��������: __kernelTickUpdate
** ��������: ֪ͨһ��ʱ�ӵ���, ���� TICK ʱ��
** �䡡��  : NONE
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static LW_INLINE VOID  __kernelTickUpdate (VOID)
{
    TICK_UPDATE();
    
#if LW_CFG_TIME_TICK_HOOK_EN > 0
    bspTickHook(_K_atomic64KernelTime.counter);                         /*  ����ϵͳʱ�ӹ��Ӻ���        */
    __LW_THREAD_TICK_HOOK(_K_atomic64KernelTime.counter);
#endif
}
/*********************************************************************************************************
** ��������: API_KernelTicks
** ��������: ֪ͨϵͳ����һ��ʵʱʱ�� TICK �������ж��е��ã�Ҳ�������߳��е���
** �䡡��  : 
** �䡡��  : 
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API
VOID  API_KernelTicks (VOID)
{
#if LW_CFG_SOFTWARE_WATCHDOG_EN > 0
    _WatchDogTick();                                                    /*  �����Ź�����              */
#endif                                                                  /*  LW_CFG_SOFTWARE_WATCHDOG... */
    _ThreadTick();                                                      /*  ����ȴ�����                */
    
    MONITOR_EVT(MONITOR_EVENT_ID_KERNEL, MONITOR_EVENT_KERNEL_TICK, LW_NULL);
}
/*********************************************************************************************************
** ��������: API_KernelTicksContext
** ��������: ����ϵͳʱ���ж�. (�˺����������ж��б�����)
** �䡡��  : 
** �䡡��  : 
** ȫ�ֱ���: 
** ����ģ��: 
** ע  ��  : vprocTickHook() ���ܻἤ���µ�����, �����ж��˳�ʱ�᳢�Ե���. 
             �������������� QUICK �����д��ж�.

                                           API ����
*********************************************************************************************************/
LW_API
VOID  API_KernelTicksContext (VOID)
{
             INTREG         iregInterLevel;
    REGISTER INT            i;
             PLW_CLASS_CPU  pcpu;
             PLW_CLASS_TCB  ptcb;
             
    LW_SPIN_KERN_TIME_LOCK_QUICK(&iregInterLevel);                      /*  �����ں�ʱ�䲢�ر��ж�      */
    
#if LW_CFG_RTC_EN > 0
    __kernelTODUpdate();                                                /*  ���� TOD ʱ��               */
#endif                                                                  /*  LW_CFG_RTC_EN > 0           */
    __kernelTickUpdate();                                               /*  ���� TICK ʱ��              */
    
    LW_SPIN_KERN_TIME_UNLOCK_QUICK(iregInterLevel);                     /*  �����ں�ʱ�䲢���ж�      */

    LW_SPIN_KERN_LOCK_QUICK(&iregInterLevel);                           /*  �����ں˲��ر��ж�          */
    
    KN_INT_ENABLE(iregInterLevel);                                      /*  ���������жϽ���            */

#if LW_CFG_SMP_EN > 0
    LW_CPU_FOREACH (i) {                                                /*  �������еĺ�                */
#else
    i = 0;
#endif                                                                  /*  LW_CFG_SMP_EN               */

        pcpu = LW_CPU_GET(i);
        if (LW_CPU_IS_ACTIVE(pcpu)) {                                   /*  CPU ���뱻����              */
            ptcb = pcpu->CPU_ptcbTCBCur;
            ptcb->TCB_ulCPUTicks++;
            if (pcpu->CPU_iKernelCounter) {
                ptcb->TCB_ulCPUKernelTicks++;
            }
            __LW_TICK_CPUUSAGE_UPDATE(ptcb, pcpu);                      /*  �������� CPU ������         */
        }
        
#if LW_CFG_SMP_EN > 0
    }
#endif                                                                  /*  LW_CFG_SMP_EN               */
    
#if LW_CFG_MODULELOADER_EN > 0
    if (!LW_KERN_NO_ITIMER_EN_GET()) {
        vprocTickHook();                                                /*  �������ִ��ʱ��            */
    }
#endif
    
    iregInterLevel = KN_INT_DISABLE();                                  /*  �ر��ж�                    */
    
    _SchedTick();                                                       /*  �������� CPU �̵߳�ʱ��Ƭ   */
    
    LW_SPIN_KERN_UNLOCK_QUICK(iregInterLevel);                          /*  �˳��ں˲����ж�          */
}
/*********************************************************************************************************
** ��������: API_KernelTicksAdjust
** ��������: ÿ�� tick ������΢��
**           BSP ��ϵͳ����ǰ��ʼ�� tick ʱ����, ���������� HZ ��ƫ��ʱ������ʱ��ƫ��
** �䡡��  : lNs       ����������
**                     ���Ϊ������� +: ��ʾÿһ�� tick ��Ҫ�Ӵ��������,
**                                    -: ��ʾÿһ�� tick ��Ҫ��С��������.
**           bRelative �Ƿ�Ϊ�������
** �䡡��  :
** ȫ�ֱ���:
** ����ģ��:

                                           API ����
*********************************************************************************************************/
LW_API
INT  API_KernelTicksAdjust (LONG  lNs, BOOL  bRelative)
{
    if (LW_SYS_STATUS_IS_RUNNING()) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "kernel is already start.\r\n");
        return  (PX_ERROR);
    }

    if (bRelative) {
        if (lib_labs(lNs) >= LW_NSEC_PER_TICK) {
            _DebugHandle(__ERRORMESSAGE_LEVEL, "adjust value range error.\r\n");
            return  (PX_ERROR);
        }

        LW_NSEC_PER_TICK += lNs;

    } else {
        if (lNs < 1) {
            _DebugHandle(__ERRORMESSAGE_LEVEL, "adjust value range error.\r\n");
            return  (PX_ERROR);
        }

        LW_NSEC_PER_TICK = lNs;
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
  END
*********************************************************************************************************/
