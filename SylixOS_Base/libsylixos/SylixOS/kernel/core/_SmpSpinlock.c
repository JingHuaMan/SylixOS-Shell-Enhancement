/*********************************************************************************************************
**
**                                    �й������Դ��֯
**
**                                   Ƕ��ʽʵʱ����ϵͳ
**
**                                       SylixOS(TM)
**
**                               Copyright  All Rights Reserved
**
**--------------�ļ���Ϣ--------------------------------------------------------------------------------
**
** ��   ��   ��: _SmpSpinlock.c
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2013 �� 07 �� 21 ��
**
** ��        ��: �� CPU ϵͳ������.
**
** BUG:
2015.05.15  ���� CPU_ulSpinNesting ����.
2015.05.16  ���� Task ��������, ���������������������������.
2016.04.26  ���� raw ��������, ���������� atomic ��������.
2018.11.29  raw ���������������һЩ���⴦�������������ж� (���ܻ��м����ݹ鴥�����������ȱ��).
*********************************************************************************************************/
#define  __SYLIXOS_SMPFMB
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
/*********************************************************************************************************
  �ü�֧��
*********************************************************************************************************/
#if LW_CFG_SMP_EN > 0
/*********************************************************************************************************
  ��������������ֵ
*********************************************************************************************************/
#define LW_SPIN_OK      1
#define LW_SPIN_ERROR   0
/*********************************************************************************************************
** ��������: _SmpSpinInit
** ��������: ��������ʼ��
** �䡡��  : psl           ������
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
VOID  _SmpSpinInit (spinlock_t *psl)
{
    __ARCH_SPIN_INIT(psl);
    KN_SMP_WMB();
}
/*********************************************************************************************************
** ��������: _SmpSpinLock
** ��������: ��������������
** �䡡��  : psl           ������
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
VOID  _SmpSpinLock (spinlock_t *psl)
{
    INTREG          iregInterLevel;
    PLW_CLASS_CPU   pcpuCur;
    
    iregInterLevel = KN_INT_DISABLE();
    
    pcpuCur = LW_CPU_GET_CUR();
    if (!pcpuCur->CPU_ulInterNesting) {
        __THREAD_LOCK_INC(pcpuCur->CPU_ptcbTCBCur);                     /*  ���������ڵ�ǰ CPU          */
    }
    
    LW_CPU_SPIN_NESTING_INC(pcpuCur);
    
    __ARCH_SPIN_LOCK(psl, pcpuCur, LW_NULL, LW_NULL);                   /*  ������֤��������ɹ�        */
    KN_SMP_MB();
    
    KN_INT_ENABLE(iregInterLevel);
}
/*********************************************************************************************************
** ��������: _SmpSpinTryLock
** ��������: ���������Լ�������
** �䡡��  : psl           ������
** �䡡��  : LW_TRUE �������� LW_FALSE ����ʧ��
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
BOOL  _SmpSpinTryLock (spinlock_t *psl)
{
    INTREG          iregInterLevel;
    PLW_CLASS_CPU   pcpuCur;
    INT             iRet;
    
    iregInterLevel = KN_INT_DISABLE();
    
    pcpuCur = LW_CPU_GET_CUR();
    if (!pcpuCur->CPU_ulInterNesting) {
        __THREAD_LOCK_INC(pcpuCur->CPU_ptcbTCBCur);                     /*  ���������ڵ�ǰ CPU          */
    }
    
    LW_CPU_SPIN_NESTING_INC(pcpuCur);
    
    iRet = __ARCH_SPIN_TRYLOCK(psl, pcpuCur);
    KN_SMP_MB();
    
    if (iRet != LW_SPIN_OK) {
        if (!pcpuCur->CPU_ulInterNesting) {
            __THREAD_LOCK_DEC(pcpuCur->CPU_ptcbTCBCur);                 /*  ����ʧ��, �����������      */
        }
        
        LW_CPU_SPIN_NESTING_DEC(pcpuCur);
    }
    
    KN_INT_ENABLE(iregInterLevel);
    
    return  ((iRet == LW_SPIN_OK) ? (LW_TRUE) : (LW_FALSE));
}
/*********************************************************************************************************
** ��������: _SmpSpinUnlock
** ��������: ��������������
** �䡡��  : psl           ������
** �䡡��  : ����������ֵ
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
INT  _SmpSpinUnlock (spinlock_t *psl)
{
    INTREG          iregInterLevel;
    PLW_CLASS_CPU   pcpuCur;
    PLW_CLASS_TCB   ptcbCur;
    BOOL            bTrySched = LW_FALSE;
    INT             iRet;
    
    iregInterLevel = KN_INT_DISABLE();
    
    KN_SMP_MB();
    pcpuCur = LW_CPU_GET_CUR();
    iRet    = __ARCH_SPIN_UNLOCK(psl, pcpuCur);
    _BugFormat((iRet != LW_SPIN_OK), LW_TRUE, "unlock error %p!\r\n", psl);
    
    if (!pcpuCur->CPU_ulInterNesting) {
        ptcbCur = pcpuCur->CPU_ptcbTCBCur;
        __THREAD_LOCK_DEC(ptcbCur);                                     /*  �����������                */
        if (__ISNEED_SCHED(pcpuCur, 0)) {
            bTrySched = LW_TRUE;                                        /*  ��Ҫ���Ե���                */
        }
    }
    
    LW_CPU_SPIN_NESTING_DEC(pcpuCur);
    
    KN_INT_ENABLE(iregInterLevel);
    
    if (bTrySched) {
        return  (_ThreadSched(ptcbCur));
    
    } else {
        return  (ERROR_NONE);
    }
}
/*********************************************************************************************************
** ��������: _SmpSpinLockIgnIrq
** ��������: ��������������, �����ж����� (�������жϹرյ�״̬�±�����)
** �䡡��  : psl           ������
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
VOID  _SmpSpinLockIgnIrq (spinlock_t *psl)
{
    PLW_CLASS_CPU   pcpuCur;
    
    pcpuCur = LW_CPU_GET_CUR();
    if (!pcpuCur->CPU_ulInterNesting) {
        __THREAD_LOCK_INC(pcpuCur->CPU_ptcbTCBCur);                     /*  ���������ڵ�ǰ CPU          */
    }
    
    LW_CPU_SPIN_NESTING_INC(pcpuCur);
    
    __ARCH_SPIN_LOCK(psl, pcpuCur, LW_NULL, LW_NULL);                   /*  ������֤��������ɹ�        */
    KN_SMP_MB();
}
/*********************************************************************************************************
** ��������: _SmpSpinTryLockIgnIrq
** ��������: ���������Լ�������, �����ж����� (�������жϹرյ�״̬�±�����)
** �䡡��  : psl           ������
** �䡡��  : LW_TRUE �������� LW_FALSE ����ʧ��
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
BOOL  _SmpSpinTryLockIgnIrq (spinlock_t *psl)
{
    PLW_CLASS_CPU   pcpuCur;
    INT             iRet;
    
    pcpuCur = LW_CPU_GET_CUR();
    if (!pcpuCur->CPU_ulInterNesting) {
        __THREAD_LOCK_INC(pcpuCur->CPU_ptcbTCBCur);                     /*  ���������ڵ�ǰ CPU          */
    }
    
    LW_CPU_SPIN_NESTING_INC(pcpuCur);
    
    iRet = __ARCH_SPIN_TRYLOCK(psl, pcpuCur);
    KN_SMP_MB();
    
    if (iRet != LW_SPIN_OK) {
        if (!pcpuCur->CPU_ulInterNesting) {
            __THREAD_LOCK_DEC(pcpuCur->CPU_ptcbTCBCur);                 /*  ����ʧ��, �����������      */
        }
        
        LW_CPU_SPIN_NESTING_DEC(pcpuCur);
    }
    
    return  ((iRet == LW_SPIN_OK) ? (LW_TRUE) : (LW_FALSE));
}
/*********************************************************************************************************
** ��������: _SmpSpinUnlockIgnIrq
** ��������: ��������������, �����ж����� (�������жϹرյ�״̬�±�����)
** �䡡��  : psl           ������
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
VOID  _SmpSpinUnlockIgnIrq (spinlock_t *psl)
{
    PLW_CLASS_CPU   pcpuCur;
    INT             iRet;
    
    KN_SMP_MB();
    pcpuCur = LW_CPU_GET_CUR();
    iRet    = __ARCH_SPIN_UNLOCK(psl, pcpuCur);
    _BugFormat((iRet != LW_SPIN_OK), LW_TRUE, "unlock error %p!\r\n", psl);
    
    if (!pcpuCur->CPU_ulInterNesting) {
        __THREAD_LOCK_DEC(pcpuCur->CPU_ptcbTCBCur);                     /*  �����������                */
    }
    
    LW_CPU_SPIN_NESTING_DEC(pcpuCur);
}
/*********************************************************************************************************
** ��������: _SmpSpinLockIrq
** ��������: ��������������, ��ͬ�����ж�
** �䡡��  : psl               ������
**           piregInterLevel   �ж�������Ϣ
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
VOID  _SmpSpinLockIrq (spinlock_t *psl, INTREG  *piregInterLevel)
{
    PLW_CLASS_CPU   pcpuCur;
    
    *piregInterLevel = KN_INT_DISABLE();
    
    pcpuCur = LW_CPU_GET_CUR();
    if (!pcpuCur->CPU_ulInterNesting) {
        __THREAD_LOCK_INC(pcpuCur->CPU_ptcbTCBCur);                     /*  ���������ڵ�ǰ CPU          */
    }
    
    LW_CPU_SPIN_NESTING_INC(pcpuCur);
    
    __ARCH_SPIN_LOCK(psl, pcpuCur, LW_NULL, LW_NULL);                   /*  ������֤��������ɹ�        */
    KN_SMP_MB();
}
/*********************************************************************************************************
** ��������: _SmpSpinTryLockIrq
** ��������: ���������Լ�������, ��ͬ�����ж�
** �䡡��  : psl               ������
**           piregInterLevel   �ж�������Ϣ
** �䡡��  : LW_TRUE �������� LW_FALSE ����ʧ��
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
BOOL  _SmpSpinTryLockIrq (spinlock_t *psl, INTREG  *piregInterLevel)
{
    PLW_CLASS_CPU   pcpuCur;
    INT             iRet;
    
    *piregInterLevel = KN_INT_DISABLE();
    
    pcpuCur = LW_CPU_GET_CUR();
    if (!pcpuCur->CPU_ulInterNesting) {
        __THREAD_LOCK_INC(pcpuCur->CPU_ptcbTCBCur);                     /*  ���������ڵ�ǰ CPU          */
    }
    
    LW_CPU_SPIN_NESTING_INC(pcpuCur);
    
    iRet = __ARCH_SPIN_TRYLOCK(psl, pcpuCur);
    KN_SMP_MB();
    
    if (iRet != LW_SPIN_OK) {
        if (!pcpuCur->CPU_ulInterNesting) {
            __THREAD_LOCK_DEC(pcpuCur->CPU_ptcbTCBCur);                 /*  ����ʧ��, �����������      */
        }
        
        LW_CPU_SPIN_NESTING_DEC(pcpuCur);
        
        KN_INT_ENABLE(*piregInterLevel);
    }
    
    return  ((iRet == LW_SPIN_OK) ? (LW_TRUE) : (LW_FALSE));
}
/*********************************************************************************************************
** ��������: _SmpSpinUnlockIrq
** ��������: ��������������, ��ͬ�����ж�
** �䡡��  : psl               ������
**           iregInterLevel    �ж�������Ϣ
** �䡡��  : ����������ֵ
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
INT  _SmpSpinUnlockIrq (spinlock_t *psl, INTREG  iregInterLevel)
{
    PLW_CLASS_CPU   pcpuCur;
    PLW_CLASS_TCB   ptcbCur;
    BOOL            bTrySched = LW_FALSE;
    INT             iRet;
    
    KN_SMP_MB();
    pcpuCur = LW_CPU_GET_CUR();
    iRet    = __ARCH_SPIN_UNLOCK(psl, pcpuCur);
    _BugFormat((iRet != LW_SPIN_OK), LW_TRUE, "unlock error %p!\r\n", psl);
    
    if (!pcpuCur->CPU_ulInterNesting) {
        ptcbCur = pcpuCur->CPU_ptcbTCBCur;
        __THREAD_LOCK_DEC(ptcbCur);                                     /*  �����������                */
        if (__ISNEED_SCHED(pcpuCur, 0)) {
            bTrySched = LW_TRUE;                                        /*  ��Ҫ���Ե���                */
        }
    }
    
    LW_CPU_SPIN_NESTING_DEC(pcpuCur);
    
    KN_INT_ENABLE(iregInterLevel);
    
    if (bTrySched) {
        return  (_ThreadSched(ptcbCur));
    
    } else {
        return  (ERROR_NONE);
    }
}
/*********************************************************************************************************
** ��������: _SmpSpinLockIrqQuick
** ��������: ��������������, ��ͬ�����ж�
** �䡡��  : psl               ������
**           piregInterLevel   �ж�������Ϣ
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
VOID  _SmpSpinLockIrqQuick (spinlock_t *psl, INTREG  *piregInterLevel)
{
    PLW_CLASS_CPU   pcpuCur;
    
    *piregInterLevel = KN_INT_DISABLE();
    
    pcpuCur = LW_CPU_GET_CUR();
    if (!pcpuCur->CPU_ulInterNesting) {
        LW_CPU_LOCK_QUICK_INC(pcpuCur);                                 /*  ������������֮ǰ            */
        KN_SMP_WMB();
        __THREAD_LOCK_INC(pcpuCur->CPU_ptcbTCBCur);                     /*  ���������ڵ�ǰ CPU          */
    }
    
    LW_CPU_SPIN_NESTING_INC(pcpuCur);
    
    __ARCH_SPIN_LOCK(psl, pcpuCur, LW_NULL, LW_NULL);                   /*  ������֤��������ɹ�        */
    KN_SMP_MB();
}
/*********************************************************************************************************
** ��������: _SmpSpinUnlockIrqQuick
** ��������: ��������������, ��ͬ�����ж�, �����г��Ե���
** �䡡��  : psl               ������
**           iregInterLevel    �ж�������Ϣ
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
VOID  _SmpSpinUnlockIrqQuick (spinlock_t *psl, INTREG  iregInterLevel)
{
    PLW_CLASS_CPU   pcpuCur;
    INT             iRet;

    KN_SMP_MB();
    pcpuCur = LW_CPU_GET_CUR();
    iRet    = __ARCH_SPIN_UNLOCK(psl, pcpuCur);
    _BugFormat((iRet != LW_SPIN_OK), LW_TRUE, "unlock error %p!\r\n", psl);

    if (!pcpuCur->CPU_ulInterNesting) {
        __THREAD_LOCK_DEC(pcpuCur->CPU_ptcbTCBCur);                     /*  �����������                */
        KN_SMP_WMB();
        LW_CPU_LOCK_QUICK_DEC(pcpuCur);                                 /*  ������������֮��            */
    }
    
    LW_CPU_SPIN_NESTING_DEC(pcpuCur);
    
    KN_INT_ENABLE(iregInterLevel);
}
/*********************************************************************************************************
** ��������: _SmpSpinLockTask
** ��������: �����������������. (�������ж�, ͬʱ�����������ÿ��ܲ��������Ĳ���)
** �䡡��  : psl               ������
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
VOID  _SmpSpinLockTask (spinlock_t *psl)
{
    INTREG          iregInterLevel;
    PLW_CLASS_CPU   pcpuCur;
    
    iregInterLevel = KN_INT_DISABLE();
    
    pcpuCur = LW_CPU_GET_CUR();
    _BugHandle(pcpuCur->CPU_ulInterNesting, LW_TRUE, "called from ISR.\r\n");
    __THREAD_LOCK_INC(pcpuCur->CPU_ptcbTCBCur);                         /*  ���������ڵ�ǰ CPU          */
    
#if defined(LW_CFG_CPU_ARCH_MIPS) && (LW_CFG_MIPS_NEST_LLSC_BUG > 0)
    __ARCH_SPIN_LOCK_RAW(psl);                                          /*  ������֤��������ɹ�        */
    KN_SMP_MB();
    KN_INT_ENABLE(iregInterLevel);
    
#else                                                                   /*  LW_CFG_MIPS_CPU_LOONGSON2K  */
    KN_INT_ENABLE(iregInterLevel);
    __ARCH_SPIN_LOCK_RAW(psl);                                          /*  ������֤��������ɹ�        */
    KN_SMP_MB();
#endif                                                                  /*  !LW_CFG_MIPS_CPU_LOONGSON2K */
}
/*********************************************************************************************************
** ��������: _SmpSpinTryLockTask
** ��������: ���������������������. (�������ж�, ͬʱ�����������ÿ��ܲ��������Ĳ���)
** �䡡��  : psl               ������
** �䡡��  : LW_TRUE �������� LW_FALSE ����ʧ��
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
BOOL  _SmpSpinTryLockTask (spinlock_t *psl)
{
    INTREG          iregInterLevel;
    PLW_CLASS_CPU   pcpuCur;
    INT             iRet;
    
    iregInterLevel = KN_INT_DISABLE();
    
    pcpuCur = LW_CPU_GET_CUR();
    _BugHandle(pcpuCur->CPU_ulInterNesting, LW_TRUE, "called from ISR.\r\n");
    __THREAD_LOCK_INC(pcpuCur->CPU_ptcbTCBCur);                         /*  ���������ڵ�ǰ CPU          */
    
    iRet = __ARCH_SPIN_TRYLOCK_RAW(psl);                                /*  �������ѭ���ȴ�            */
    KN_SMP_MB();

    if (iRet != LW_SPIN_OK) {
        __THREAD_LOCK_DEC(pcpuCur->CPU_ptcbTCBCur);                     /*  ����ʧ��, �����������      */
    }

    KN_INT_ENABLE(iregInterLevel);
    
    return  ((iRet == LW_SPIN_OK) ? (LW_TRUE) : (LW_FALSE));
}
/*********************************************************************************************************
** ��������: _SmpSpinUnlockTask
** ��������: �����������������. (�������ж�, ͬʱ�����������ÿ��ܲ��������Ĳ���)
** �䡡��  : psl               ������
** �䡡��  : ����������ֵ
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
INT  _SmpSpinUnlockTask (spinlock_t *psl)
{
    INTREG          iregInterLevel;
    PLW_CLASS_CPU   pcpuCur;
    PLW_CLASS_TCB   ptcbCur;
    BOOL            bTrySched = LW_FALSE;
    
#if defined(LW_CFG_CPU_ARCH_MIPS) && (LW_CFG_MIPS_NEST_LLSC_BUG > 0)
    iregInterLevel = KN_INT_DISABLE();
    KN_SMP_MB();
    __ARCH_SPIN_UNLOCK_RAW(psl);
    
#else                                                                   /*  LW_CFG_MIPS_CPU_LOONGSON2K  */
    KN_SMP_MB();
    __ARCH_SPIN_UNLOCK_RAW(psl);
    iregInterLevel = KN_INT_DISABLE();
#endif                                                                  /*  !LW_CFG_MIPS_CPU_LOONGSON2K */

    pcpuCur = LW_CPU_GET_CUR();
    _BugHandle(pcpuCur->CPU_ulInterNesting, LW_TRUE, "called from ISR.\r\n");
    
    ptcbCur = pcpuCur->CPU_ptcbTCBCur;
    if (__THREAD_LOCK_GET(ptcbCur)) {                                   /*  Ӧ��ʹ����Ҫ�ж���Ч��      */
        __THREAD_LOCK_DEC(ptcbCur);                                     /*  �����������                */
    }
    
    if (__ISNEED_SCHED(pcpuCur, 0)) {
        bTrySched = LW_TRUE;                                            /*  ��Ҫ���Ե���                */
    }
    
    KN_INT_ENABLE(iregInterLevel);
    
    if (bTrySched) {
        return  (_ThreadSched(ptcbCur));
    
    } else {
        return  (ERROR_NONE);
    }
}
/*********************************************************************************************************
** ��������: _SmpSpinLockRaw
** ��������: ������ԭʼ��������.
** �䡡��  : psl               ������
**           piregInterLevel   �ж�״̬
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
VOID  _SmpSpinLockRaw (spinlock_t *psl, INTREG  *piregInterLevel)
{
    *piregInterLevel = KN_INT_DISABLE();
    __ARCH_SPIN_LOCK_RAW(psl);                                          /*  ������֤��������ɹ�        */
    KN_SMP_MB();
}
/*********************************************************************************************************
** ��������: _SmpSpinTryLockRaw
** ��������: ����������ԭʼ��������.
** �䡡��  : psl               ������
**           piregInterLevel   �ж�״̬
** �䡡��  : LW_TRUE �������� LW_FALSE ����ʧ��
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
BOOL  _SmpSpinTryLockRaw (spinlock_t *psl, INTREG  *piregInterLevel)
{
    INT     iRet;
    
    *piregInterLevel = KN_INT_DISABLE();
    iRet = __ARCH_SPIN_TRYLOCK_RAW(psl);
    KN_SMP_MB();
    
    if (iRet != LW_SPIN_OK) {
        KN_INT_ENABLE(*piregInterLevel);
    }
    
    return  ((iRet == LW_SPIN_OK) ? (LW_TRUE) : (LW_FALSE));
}
/*********************************************************************************************************
** ��������: _SmpSpinUnlockRaw
** ��������: ������ԭʼ��������.
** �䡡��  : psl               ������
**           iregInterLevel    �ж�״̬
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
VOID  _SmpSpinUnlockRaw (spinlock_t *psl, INTREG  iregInterLevel)
{
    KN_SMP_MB();
    __ARCH_SPIN_UNLOCK_RAW(psl);
    KN_INT_ENABLE(iregInterLevel);
}

#endif                                                                  /*  LW_CFG_SMP_EN               */
/*********************************************************************************************************
  END
*********************************************************************************************************/
