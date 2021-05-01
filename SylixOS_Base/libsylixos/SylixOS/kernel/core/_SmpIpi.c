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
** ��   ��   ��: _SmpIpi.c
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2013 �� 07 �� 19 ��
**
** ��        ��: CPU �˼��ж�, (���� SMP ���ϵͳ)

** BUG:
2014.04.09  ������û�� ACTIVE �� CPU ���ͺ˼��ж�.
2018.08.09  ���� IPI trace ����.
*********************************************************************************************************/
#define  __SYLIXOS_SMPFMB
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
/*********************************************************************************************************
  �ü�֧��
*********************************************************************************************************/
#if LW_CFG_SMP_EN > 0
/*********************************************************************************************************
  IPI HOOK
*********************************************************************************************************/
#if LW_CFG_SYSPERF_EN > 0
static VOIDFUNCPTR  _K_pfuncIpiPerf;
#endif                                                                  /*  LW_CFG_SYSPERF_EN > 0       */
/*********************************************************************************************************
  IPI LOCK
*********************************************************************************************************/
#define LW_IPI_LOCK(pcpu, bIntLock)                                     \
        if (bIntLock) {                                                 \
            LW_SPIN_LOCK_IGNIRQ(&pcpu->CPU_slIpi);                      \
        } else {                                                        \
            LW_SPIN_LOCK_QUICK(&pcpu->CPU_slIpi, &iregInterLevel);      \
        }
#define LW_IPI_UNLOCK(pcpu, bIntLock)                                   \
        if (bIntLock) {                                                 \
            LW_SPIN_UNLOCK_IGNIRQ(&pcpu->CPU_slIpi);                    \
        } else {                                                        \
            LW_SPIN_UNLOCK_QUICK(&pcpu->CPU_slIpi, iregInterLevel);     \
        }
#define LW_IPI_INT_LOCK(bIntLock)                                       \
        if (bIntLock == LW_FALSE) {                                     \
            iregInterLevel = KN_INT_DISABLE();                          \
        }
#define LW_IPI_INT_UNLOCK(bIntLock)                                     \
        if (bIntLock == LW_FALSE) {                                     \
            KN_INT_ENABLE(iregInterLevel);                              \
        }
/*********************************************************************************************************
** ��������: _SmpPerfIpi
** ��������: ���� IPI ���ٻص�
** �䡡��  : ulIPIVec      �˼��ж����� (���Զ������ͺ����ж�����)
**           pfuncHook     trace ����
** �䡡��  : ֮ǰ�� trace ����
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
#if LW_CFG_SYSPERF_EN > 0

VOIDFUNCPTR  _SmpPerfIpi (VOIDFUNCPTR  pfuncHook)
{
    VOIDFUNCPTR  pfuncOld;
    
    pfuncOld = _K_pfuncIpiPerf;
    _K_pfuncIpiPerf = pfuncHook;
    KN_SMP_MB();
    
    return  (pfuncOld);
}

#endif                                                                  /*  LW_CFG_SYSPERF_EN > 0       */
/*********************************************************************************************************
** ��������: _SmpSendIpi
** ��������: ����һ�����Զ�������ĺ˼��жϸ�ָ���� CPU. (������ȴ����ⲿ�ɲ�������ǰ CPU)
** �䡡��  : ulCPUId       CPU ID
**           ulIPIVec      �˼��ж����� (���Զ������ͺ����ж�����)
**           iWait         �Ƿ�ȴ�������� (LW_IPI_SCHED ��������ȴ�, ���������)
**           bIntLock      �ⲿ�Ƿ���ж���.
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
** ע  ��  : ֪ͨ���� (LW_IPI_SCHED) ��֪ͨ CPU ֹͣ (LW_IPI_DOWN) ����Ҫ�ȴ�����.
*********************************************************************************************************/
VOID  _SmpSendIpi (ULONG  ulCPUId, ULONG  ulIPIVec, INT  iWait, BOOL  bIntLock)
{
    INTREG          iregInterLevel;
    PLW_CLASS_CPU   pcpuDst = LW_CPU_GET(ulCPUId);
    PLW_CLASS_CPU   pcpuCur = LW_CPU_GET_CUR();
    ULONG           ulMask  = (ULONG)(1 << ulIPIVec);
    
    if (!LW_CPU_IS_ACTIVE(pcpuDst)) {                                   /*  CPU ���뱻����              */
        return;
    }
    
#if LW_CFG_CPU_ATOMIC_EN == 0
    LW_IPI_LOCK(pcpuDst, bIntLock);                                     /*  ����Ŀ�� CPU IPI            */
#endif                                                                  /*  !LW_CFG_CPU_ATOMIC_EN       */
    
    LW_CPU_ADD_IPI_PEND(ulCPUId, ulMask);                               /*  ��� PEND λ                */
    archMpInt(ulCPUId);
    
#if LW_CFG_CPU_ATOMIC_EN == 0
    LW_IPI_UNLOCK(pcpuDst, bIntLock);                                   /*  ����Ŀ�� CPU IPI            */
#endif                                                                  /*  !LW_CFG_CPU_ATOMIC_EN       */
    
    if (iWait) {
        while (LW_CPU_GET_IPI_PEND(ulCPUId) & ulMask) {                 /*  �ȴ�����                    */
            LW_IPI_INT_LOCK(bIntLock);
            _SmpTryProcIpi(pcpuCur);                                    /*  ����ִ�������˷����� IPI    */
            LW_IPI_INT_UNLOCK(bIntLock);
        }
    }
}
/*********************************************************************************************************
** ��������: _SmpSendIpiAllOther
** ��������: ����һ�����Զ�������ĺ˼��жϸ��������� CPU, (�ⲿ����������ǰ CPU ����)
** �䡡��  : ulIPIVec      �˼��ж����� (���Զ��������ж�����)
**           iWait         �Ƿ�ȴ�������� (LW_IPI_SCHED ��������ȴ�, ���������)
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
VOID  _SmpSendIpiAllOther (ULONG  ulIPIVec, INT  iWait)
{
    ULONG   i;
    ULONG   ulCPUId;
    
    ulCPUId = LW_CPU_GET_CUR_ID();
    
    KN_SMP_WMB();
    LW_CPU_FOREACH_EXCEPT (i, ulCPUId) {
        _SmpSendIpi(i, ulIPIVec, iWait, LW_FALSE);
    }
}
/*********************************************************************************************************
** ��������: _SmpCallIpi
** ��������: ����һ���Զ���˼��жϸ�ָ���� CPU. (������ȴ����ⲿ�ɲ�������ǰ CPU)
** �䡡��  : ulCPUId       CPU ID
**           pipim         �˼��жϲ���
** �䡡��  : ���÷���ֵ
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static INT  _SmpCallIpi (ULONG  ulCPUId, PLW_IPI_MSG  pipim)
{
    INTREG          iregInterLevel;
    PLW_CLASS_CPU   pcpuDst = LW_CPU_GET(ulCPUId);
    PLW_CLASS_CPU   pcpuCur = LW_CPU_GET_CUR();
    
    if (!LW_CPU_IS_ACTIVE(pcpuDst)) {                                   /*  CPU ���뱻����              */
        return  (ERROR_NONE);
    }
    
    LW_IPI_LOCK(pcpuDst, LW_FALSE);                                     /*  ����Ŀ�� CPU IPI            */
    _List_Ring_Add_Last(&pipim->IPIM_ringManage, &pcpuDst->CPU_pringMsg);
    pcpuDst->CPU_uiMsgCnt++;
    LW_CPU_ADD_IPI_PEND(ulCPUId, LW_IPI_CALL_MSK);
    archMpInt(ulCPUId);
    LW_IPI_UNLOCK(pcpuDst, LW_FALSE);                                   /*  ����Ŀ�� CPU IPI            */
    
    while (pipim->IPIM_iWait) {                                         /*  �ȴ�����                    */
        LW_IPI_INT_LOCK(LW_FALSE);
        _SmpTryProcIpi(pcpuCur);
        LW_IPI_INT_UNLOCK(LW_FALSE);
    }
    
    return  (pipim->IPIM_iRet);
}
/*********************************************************************************************************
** ��������: _SmpCallIpiAllOther
** ��������: ����һ���Զ���˼��жϸ��������� CPU. (�ⲿ����������ǰ CPU ����)
** �䡡��  : pipim         �˼��жϲ���
** �䡡��  : NONE (�޷�ȷ������ֵ)
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static VOID  _SmpCallIpiAllOther (PLW_IPI_MSG  pipim)
{
    ULONG   i;
    ULONG   ulCPUId;
    INT     iWaitSave = pipim->IPIM_iWait;
    
    ulCPUId = LW_CPU_GET_CUR_ID();
    
    KN_SMP_WMB();
    LW_CPU_FOREACH_EXCEPT (i, ulCPUId) {
        _SmpCallIpi(i, pipim);
        pipim->IPIM_iWait = iWaitSave;
        KN_SMP_WMB();
    }
}
/*********************************************************************************************************
** ��������: _SmpCallFunc
** ��������: ���ú˼��ж���ָ���� CPU ����ָ���ĺ���. (�ⲿ����������ǰ CPU ����)
** �䡡��  : ulCPUId       CPU ID
**           pfunc         ͬ��ִ�к��� (�����ú����ڲ������������ں˲���, ������ܲ�������)
**           pvArg         ͬ������
**           pfuncAsync    �첽ִ�к��� (�����ú����ڲ������������ں˲���, ������ܲ�������)
**           pvAsync       �첽ִ�в���
**           iOpt          ѡ�� IPIM_OPT_NORMAL / IPIM_OPT_NOKERN
** �䡡��  : ���÷���ֵ
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
INT  _SmpCallFunc (ULONG        ulCPUId, 
                   FUNCPTR      pfunc, 
                   PVOID        pvArg,
                   VOIDFUNCPTR  pfuncAsync,
                   PVOID        pvAsync,
                   INT          iOpt)
{
    LW_IPI_MSG  ipim;
    
    ipim.IPIM_pfuncCall      = pfunc;
    ipim.IPIM_pvArg          = pvArg;
    ipim.IPIM_pfuncAsyncCall = pfuncAsync;
    ipim.IPIM_pvAsyncArg     = pvAsync;
    ipim.IPIM_iRet           = -1;
    ipim.IPIM_iOption        = iOpt;
    ipim.IPIM_iWait          = 1;
    
    return  (_SmpCallIpi(ulCPUId, &ipim));
}
/*********************************************************************************************************
** ��������: _SmpCallFuncAllOther
** ��������: ���ú˼��ж���ָ���� CPU ����ָ���ĺ���. (�ⲿ����������ǰ CPU ����)
** �䡡��  : pfunc         ͬ��ִ�к��� (�����ú����ڲ������������ں˲���, ������ܲ�������)
**           pvArg         ͬ������
**           pfuncAsync    �첽ִ�к��� (�����ú����ڲ������������ں˲���, ������ܲ�������)
**           pvAsync       �첽ִ�в���
**           iOpt          ѡ�� IPIM_OPT_NORMAL / IPIM_OPT_NOKERN
** �䡡��  : NONE (�޷�ȷ������ֵ)
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
VOID  _SmpCallFuncAllOther (FUNCPTR      pfunc, 
                            PVOID        pvArg,
                            VOIDFUNCPTR  pfuncAsync,
                            PVOID        pvAsync,
                            INT          iOpt)
{
    LW_IPI_MSG  ipim;
    
    ipim.IPIM_pfuncCall      = pfunc;
    ipim.IPIM_pvArg          = pvArg;
    ipim.IPIM_pfuncAsyncCall = pfuncAsync;
    ipim.IPIM_pvAsyncArg     = pvAsync;
    ipim.IPIM_iRet           = -1;
    ipim.IPIM_iOption        = iOpt;
    ipim.IPIM_iWait          = 1;
    
    _SmpCallIpiAllOther(&ipim);
}
/*********************************************************************************************************
** ��������: __smpProcCallfunc
** ��������: ����˼��жϵ��ú���
** �䡡��  : pcpuCur       ��ǰ CPU
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static VOID  __smpProcCallfunc (PLW_CLASS_CPU  pcpuCur)
{
#define LW_KERNEL_OWN_CPU()     (PLW_CLASS_CPU)(_K_klKernel.KERN_pvCpuOwner)

    UINT            i, uiCnt;
    PLW_IPI_MSG     pipim;
    PLW_LIST_RING   pringTemp;
    PLW_LIST_RING   pringDelete;
    VOIDFUNCPTR     pfuncAsync;
    PVOID           pvAsync;
    
    LW_SPIN_LOCK_IGNIRQ(&pcpuCur->CPU_slIpi);                           /*  ���� CPU                    */
    
    pringTemp = pcpuCur->CPU_pringMsg;
    uiCnt     = pcpuCur->CPU_uiMsgCnt;
    
    for (i = 0; i < uiCnt; i++) {
        _BugHandle((!pcpuCur->CPU_pringMsg), LW_TRUE, "ipi call func error!\r\n");
        
        pipim = _LIST_ENTRY(pringTemp, LW_IPI_MSG, IPIM_ringManage);
        if ((LW_KERNEL_OWN_CPU() == pcpuCur) &&
            (pipim->IPIM_iOption & IPIM_OPT_NOKERN)) {                  /*  �˺����������ں�����״ִ̬��*/
            pringTemp = _list_ring_get_next(pringTemp);
            continue;
        }
        
        pringDelete = pringTemp;
        pringTemp   = _list_ring_get_next(pringTemp);
        _List_Ring_Del(pringDelete, &pcpuCur->CPU_pringMsg);            /*  ɾ��һ���ڵ�                */
        pcpuCur->CPU_uiMsgCnt--;
        
        if (pipim->IPIM_pfuncCall) {
            pipim->IPIM_iRet = pipim->IPIM_pfuncCall(pipim->IPIM_pvArg);/*  ִ��ͬ������                */
        }
        
        pfuncAsync = pipim->IPIM_pfuncAsyncCall;
        pvAsync    = pipim->IPIM_pvAsyncArg;
        
        KN_SMP_MB();
        pipim->IPIM_iWait = 0;                                          /*  ���ý���                    */
        KN_SMP_WMB();
        LW_SPINLOCK_NOTIFY();
        
        if (pfuncAsync) {
            pfuncAsync(pvAsync);                                        /*  ִ���첽����                */
        }
    }
    
    KN_SMP_MB();
    if (pcpuCur->CPU_pringMsg == LW_NULL) {
        LW_CPU_CLR_IPI_PEND2(pcpuCur, LW_IPI_CALL_MSK);                 /*  ���                        */
    }
    
    LW_SPIN_UNLOCK_IGNIRQ(&pcpuCur->CPU_slIpi);                         /*  ���� CPU                    */
    
    LW_SPINLOCK_NOTIFY();
}
/*********************************************************************************************************
** ��������: _SmpProcCallfunc
** ��������: ����˼��жϵ��ú���
** �䡡��  : pcpuCur       ��ǰ CPU
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static VOID  _SmpProcCallfunc (PLW_CLASS_CPU  pcpuCur)
{
    INTREG  iregInterLevel;
    
    iregInterLevel = KN_INT_DISABLE();
    __smpProcCallfunc(pcpuCur);
    KN_INT_ENABLE(iregInterLevel);
}
/*********************************************************************************************************
** ��������: _SmpProcCallfuncIgnIrq
** ��������: ����˼��жϵ��ú��� (�Ѿ��ر��ж�)
** �䡡��  : pcpuCur       ��ǰ CPU
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static VOID  _SmpProcCallfuncIgnIrq (PLW_CLASS_CPU  pcpuCur)
{
    __smpProcCallfunc(pcpuCur);
}
/*********************************************************************************************************
** ��������: _SmpProcIpi
** ��������: ����˼��ж� (���ﲻ�����������Ϣ)
** �䡡��  : pcpuCur       ��ǰ CPU
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
VOID  _SmpProcIpi (PLW_CLASS_CPU  pcpuCur)
{
    pcpuCur->CPU_iIPICnt++;                                             /*  �˼��ж����� ++             */

    if (LW_CPU_GET_IPI_PEND2(pcpuCur) & LW_IPI_CALL_MSK) {              /*  �Զ������ ?                */
        _SmpProcCallfunc(pcpuCur);
    }
    
#if LW_CFG_SYSPERF_EN > 0
    if (LW_CPU_GET_IPI_PEND2(pcpuCur) & LW_IPI_PERF_MSK) {
        if (_K_pfuncIpiPerf) {
            _K_pfuncIpiPerf(pcpuCur);
        }
#if LW_CFG_CPU_ATOMIC_EN == 0
        LW_SPIN_LOCK_IGNIRQ(&pcpuCur->CPU_slIpi);                       /*  ���� CPU                    */
#endif
        LW_CPU_CLR_IPI_PEND2(pcpuCur, LW_IPI_PERF_MSK);                 /*  ���                        */
#if LW_CFG_CPU_ATOMIC_EN == 0
        LW_SPIN_UNLOCK_IGNIRQ(&pcpuCur->CPU_slIpi);                     /*  ���� CPU                    */
#endif
    }
#endif                                                                  /*  LW_CFG_SYSPERF_EN > 0       */
}
/*********************************************************************************************************
** ��������: _SmpTryProcIpi
** ��������: ���Դ���˼��ж� (�����ڹ��ж�����µ���, �����������ִ�� call ����)
** �䡡��  : pcpuCur       ��ǰ CPU
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
VOID  _SmpTryProcIpi (PLW_CLASS_CPU  pcpuCur)
{
    if (LW_CPU_GET_IPI_PEND2(pcpuCur) & LW_IPI_CALL_MSK) {              /*  �Զ������ ?                */
        _SmpProcCallfuncIgnIrq(pcpuCur);
    }
}
/*********************************************************************************************************
** ��������: _SmpUpdateIpi
** ��������: ����һ�� IPI
** �䡡��  : pcpuCur   CPU ���ƿ�
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
VOID  _SmpUpdateIpi (PLW_CLASS_CPU  pcpu)
{
    if (!LW_CPU_IS_ACTIVE(pcpu)) {                                      /*  CPU ���뱻����              */
        return;
    }

    archMpInt(LW_CPU_GET_ID(pcpu));
}

#endif                                                                  /*  LW_CFG_SMP_EN               */
/*********************************************************************************************************
  END
*********************************************************************************************************/
