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
** ��   ��   ��: _SchedCand.c
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2006 �� 12 �� 14 ��
**
** ��        ��: ���������ѡ����ز���.

** BUG:
2009.04.14  ȫ�潫������������ SMP ���֧��.
2009.04.28  ������ SMP ������, �����㷨�����Ż�.
2009.07.11  _SchedYield() ����Ҫ bIsSeekPriority ����, ���ִ��һ��.
2010.01.04  _SchedSwitchRunningAndReady() �����̵߳� TCB ����Ҫ���¼�¼�µ� CPUID.
2010.01.12  _SchedYield() �� SMP ģʽ�²���Ҫѭ��.
2012.03.27  _SchedSliceTick() ������� RR �߳�, �����κ�����.
            _SchedSeekThread() ����� FIFO �߳�, ���ж�ʱ��Ƭ��Ϣ.
2013.05.07  __LW_SCHEDULER_BUG_TRACE() ��ӡ������ϸ����Ϣ.
2013.07.29  �����ѡ���б�������ȼ�����, �����ȴ������.
2013.12.02  _SchedGetCandidate() ��������������������, ��Ϊ���ô˺���ʱ, ������ܽ����˵�����������.
2014.01.05  ���������ϸ��ٹ��ܲ��ٷ���ɨ���ѡ�������.
2014.01.10  _SchedSeekThread() ���� _CandTable.c ��.
2015.05.17  _SchedGetCand() �᳢��һ�������Լ��� IPI ִ�б��ӳٵ� IPI CALL.
2016.04.27  ���� _SchedIsLock() �жϵ������Ƿ��Ѿ� lock.
2018.07.28  ��ʱ��Ƭ���.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
/*********************************************************************************************************
** ��������: _SchedIsLock
** ��������: ��ǰ CPU �������Ƿ��Ѿ�����
** �䡡��  : ulCurMaxLock      ��ǰ CPU ��������������������.
** �䡡��  : �Ƿ��Ѿ�����
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
BOOL  _SchedIsLock (ULONG  ulCurMaxLock)
{
    INTREG          iregInterLevel;
    PLW_CLASS_CPU   pcpuCur;
    BOOL            bRet;
    
    iregInterLevel = KN_INT_DISABLE();
    
#if LW_CFG_SMP_EN > 0
    if (__KERNEL_ISLOCKBYME(LW_TRUE)) {
        bRet = LW_TRUE;
    
    } else 
#endif                                                                  /*  LW_CFG_SMP_EN > 0           */
    {
        pcpuCur = LW_CPU_GET_CUR();
        if (__COULD_SCHED(pcpuCur, ulCurMaxLock)) {
            bRet = LW_FALSE;
        
        } else {
            bRet = LW_TRUE;
        }
    }
    
    KN_INT_ENABLE(iregInterLevel);
    
    return  (bRet);
}
/*********************************************************************************************************
** ��������: _SchedGetCand
** ��������: �����Ҫ���е��̱߳� (������ʱ�Ѿ��������ں� spinlock)
** �䡡��  : ptcbRunner        ��Ҫ���е� TCB �б� (��С���� CPU ����)
**           ulCPUIdCur        ��ǰ CPU ID
**           ulCurMaxLock      ��ǰ CPU ��������������������.
** �䡡��  : ��ѡ�� TCB
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
PLW_CLASS_TCB  _SchedGetCand (PLW_CLASS_CPU  pcpuCur, ULONG  ulCurMaxLock)
{
    if (!__COULD_SCHED(pcpuCur, ulCurMaxLock)) {                        /*  ��ǰִ���̲߳��ܵ���        */
        return  (pcpuCur->CPU_ptcbTCBCur);
        
    } else {                                                            /*  ����ִ���߳��л�            */
#if LW_CFG_SMP_EN > 0
        LW_CPU_CLR_SCHED_IPI_PEND(pcpuCur);
#endif                                                                  /*  LW_CFG_SMP_EN > 0           */
        if (LW_CAND_ROT(pcpuCur)) {                                     /*  �������ȼ�����              */
            _CandTableUpdate(pcpuCur);                                  /*  ���Ը��º�ѡ��, ��ռ����    */
        }
        return  (LW_CAND_TCB(pcpuCur));
    }
}
/*********************************************************************************************************
** ��������: _SchedTick
** ��������: ʱ��Ƭ���� (tick �жϷ�������б�����, �����ں��ҹر��ж�״̬)
** �䡡��  : NONE
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
VOID  _SchedTick (VOID)
{
    REGISTER PLW_CLASS_CPU  pcpu;
    REGISTER PLW_CLASS_TCB  ptcb;
             INT            i;
             
#if LW_CFG_SMP_EN > 0
    LW_CPU_FOREACH (i) {
#else
    i = 0;
#endif                                                                  /*  LW_CFG_SMP_EN               */
        
        pcpu = LW_CPU_GET(i);
        if (LW_CPU_IS_ACTIVE(pcpu)) {                                   /*  CPU ���뼤��                */
            ptcb = pcpu->CPU_ptcbTCBCur;
            if (ptcb->TCB_ucSchedPolicy == LW_OPTION_SCHED_RR) {        /*  round-robin �߳�            */
                if (ptcb->TCB_usSchedCounter == 0) {                    /*  ʱ��Ƭ�Ѿ��ľ�              */
                    LW_CAND_ROT(pcpu) = LW_TRUE;                        /*  �´ε���ʱ�����ת          */
                    
                } else {
                    ptcb->TCB_usSchedCounter--;
                }
            }
        }
        
#if LW_CFG_SMP_EN > 0
    }
#endif                                                                  /*  LW_CFG_SMP_EN               */
}
/*********************************************************************************************************
** ��������: _SchedYield
** ��������: ָ���߳������ó� CPU ʹ��Ȩ, (ͬ���ȼ���) (�˺���������ʱ�ѽ����ں����ж��Ѿ��ر�)
** �䡡��  : ptcb          �ھ������е��߳�
**           ppcb          ��Ӧ�����ȼ����ƿ�
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
#if LW_CFG_THREAD_SCHED_YIELD_EN > 0

VOID  _SchedYield (PLW_CLASS_TCB  ptcb, PLW_CLASS_PCB  ppcb)
{
    REGISTER PLW_CLASS_CPU  pcpu;

    if (__LW_THREAD_IS_RUNNING(ptcb)) {                                 /*  ��������ִ��                */
        pcpu = LW_CPU_GET(ptcb->TCB_ulCPUId);
        ptcb->TCB_usSchedCounter = 0;                                   /*  û��ʣ��ʱ��Ƭ              */
        LW_CAND_ROT(pcpu) = LW_TRUE;                                    /*  �´ε���ʱ�����ת          */
    }
}

#endif                                                                  /*  LW_CFG_THREAD_SCHED_YIELD_EN*/
/*********************************************************************************************************
  END
*********************************************************************************************************/
