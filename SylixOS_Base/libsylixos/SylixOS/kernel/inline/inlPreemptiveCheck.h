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
** ��   ��   ��: inlPreemptiveCheck.h
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2006 �� 12 �� 14 ��
**
** ��        ��: ����ϵͳ��鵱ǰ�߳��Ƿ�������ռ��

** BUG
2007.11.21  �޸�ע��.
2008.01.20  ����������ȫ������Ϊ�ֲ���.
2008.05.18  ������ں�״̬���ж�.
2013.07.17  SMP ��ȫ����. ������ȡ��ǰ����ķ���.
*********************************************************************************************************/

#ifndef __INLPREEMPTIVECHECK_H
#define __INLPREEMPTIVECHECK_H

/*********************************************************************************************************
  ���������
*********************************************************************************************************/

#define __THREAD_LOCK_GET(ptcb)      ((ptcb)->TCB_ulThreadLockCounter)
#define __THREAD_LOCK_INC(ptcb)      ((ptcb)->TCB_ulThreadLockCounter++)
#define __THREAD_LOCK_DEC(ptcb)      ((ptcb)->TCB_ulThreadLockCounter--)

/*********************************************************************************************************
  ����Ƿ���Ҫ����. (�����ǹ��жϵ������, ȷ����ǰ CPU ���ᷢ������)
*********************************************************************************************************/

static LW_INLINE BOOL __can_preemptive (PLW_CLASS_CPU  pcpu, ULONG  ulMaxLockCounter)
{
    PLW_CLASS_TCB   ptcbCur = pcpu->CPU_ptcbTCBCur;

    /*
     *  �ж��л������ں���ִ��, ���������.
     */
    if (pcpu->CPU_ulInterNesting || pcpu->CPU_iKernelCounter) {
        return  (LW_FALSE);
    }
    
    /*
     *  ��ǰ�߳̾����ұ�����, ���������.
     */
    if ((__THREAD_LOCK_GET(ptcbCur) > ulMaxLockCounter) && __LW_THREAD_IS_READY(ptcbCur)) {
        return  (LW_FALSE);
    }
    
    return  (LW_TRUE);
}

/*********************************************************************************************************
  ����˳��ں˻��ж�ʱ�Ƿ���Ҫ����. (���ں�״̬������)
*********************************************************************************************************/

static LW_INLINE BOOL __can_preemptive_ki (PLW_CLASS_CPU  pcpu, ULONG  ulMaxLockCounter)
{
    PLW_CLASS_TCB   ptcbCur = pcpu->CPU_ptcbTCBCur;

    /*
     *  ��ǰ�߳̾����ұ�����, ���������.
     */
    return  ((__THREAD_LOCK_GET(ptcbCur) > ulMaxLockCounter) && __LW_THREAD_IS_READY(ptcbCur) ?
             LW_FALSE : LW_TRUE);
}

/*********************************************************************************************************
  �жϵ����Ƿ��ִ�к�. (û��Ƕ�׷����������߳�������ȱ�����)
*********************************************************************************************************/

#define __COULD_SCHED(pcpu, ulMaxLockCounter)    __can_preemptive(pcpu, ulMaxLockCounter)
#define __COULD_SCHED_KI(pcpu, ulMaxLockCounter) __can_preemptive_ki(pcpu, ulMaxLockCounter)

/*********************************************************************************************************
  �ж��Ƿ���Ҫ����
*********************************************************************************************************/

#if LW_CFG_SMP_EN > 0
#define __ISNEED_SCHED(pcpu, ulMaxLockCounter)   (__COULD_SCHED(pcpu, ulMaxLockCounter) && \
                                                  LW_CAND_ROT(pcpu))
#endif

/*********************************************************************************************************
  ��� CPU �ڲ�����
*********************************************************************************************************/
#if LW_CFG_SMP_EN > 0

static LW_INLINE BOOL  __smp_cpu_lock (VOID)
{
    BOOL    bSchedLock = _SchedIsLock(0);
    
    if (bSchedLock == LW_FALSE) {
        LW_THREAD_LOCK();                                               /*  ������ǰ CPU ִ��           */
    }
    
    return  (bSchedLock ? LW_FALSE : LW_TRUE);
}

static LW_INLINE VOID  __smp_cpu_unlock (BOOL  bUnlock)
{
    if (bUnlock) {
        LW_THREAD_UNLOCK();                                             /*  ������ǰ CPU ִ��           */
    }
}

#define __SMP_CPU_LOCK()        __smp_cpu_lock()
#define __SMP_CPU_UNLOCK(prev)  __smp_cpu_unlock(prev)

#endif                                                                  /*  LW_CFG_SMP_EN > 0           */

#endif                                                                  /*  __INLPREEMPTIVECHECK_H      */
/*********************************************************************************************************
  END
*********************************************************************************************************/
