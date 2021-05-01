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
** ��   ��   ��: _CpuLib.c
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2014 �� 01 �� 10 ��
**
** ��        ��: CPU ������.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
/*********************************************************************************************************
** ��������: _CpuActive
** ��������: �� CPU ����Ϊ����״̬ (�����ں��ҹر��ж�״̬�±�����)
** �䡡��  : pcpu      CPU �ṹ
** �䡡��  : ERROR or OK
** ȫ�ֱ���: 
** ����ģ��: 
** ע  ��  : ���뱣֤ pcpu ��ǰִ���߳���һ����Ч�� TCB ���� _K_tcbDummyKernel ��������.
*********************************************************************************************************/
INT  _CpuActive (PLW_CLASS_CPU   pcpu)
{
    if (LW_CPU_IS_ACTIVE(pcpu)) {
        return  (PX_ERROR);
    }
    
    pcpu->CPU_ulStatus |= LW_CPU_STATUS_ACTIVE;
    
#if (LW_CFG_SMP_EN > 0) && (LW_CFG_CPU_ARCH_SMT > 0)
    LW_PHYCPU_GET(pcpu->CPU_ulPhyId)->PHYCPU_uiLogic++;
#endif
    KN_SMP_MB();
    
    _CandTableUpdate(pcpu);                                             /*  ���º�ѡ�߳�                */

    pcpu->CPU_ptcbTCBCur  = LW_CAND_TCB(pcpu);
    pcpu->CPU_ptcbTCBHigh = LW_CAND_TCB(pcpu);
    
#if (LW_CFG_SMP_EN > 0) && (LW_CFG_CPU_ARCH_SMT > 0)
    if (pcpu->CPU_ptcbTCBCur->TCB_usIndex >= LW_NCPUS) {
        LW_PHYCPU_GET(pcpu->CPU_ulPhyId)->PHYCPU_uiNonIdle++;           /*  ���еķ� Idle ����          */
    }
#endif
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: _CpuInactive
** ��������: �� CPU ����Ϊ�Ǽ���״̬ (�����ں��ҹر��ж�״̬�±�����)
** �䡡��  : pcpu      CPU �ṹ
** �䡡��  : ERROR or OK
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
#if LW_CFG_SMP_CPU_DOWN_EN > 0

INT  _CpuInactive (PLW_CLASS_CPU   pcpu)
{
    INT             i;
    ULONG           ulCPUId;
    PLW_CLASS_CPU   pcpuOther;
    PLW_CLASS_TCB   ptcbOther;
    PLW_CLASS_TCB   ptcb;
    BOOL            bRotIdle;

    if (!LW_CPU_IS_ACTIVE(pcpu)) {
        return  (PX_ERROR);
    }
    
    ulCPUId = LW_CPU_GET_ID(pcpu);
    ptcb    = LW_CAND_TCB(pcpu);
    
    pcpu->CPU_ulStatus &= ~LW_CPU_STATUS_ACTIVE;
    
#if LW_CFG_CPU_ARCH_SMT > 0
    LW_PHYCPU_GET(pcpu->CPU_ulPhyId)->PHYCPU_uiLogic--;
#endif
    KN_SMP_MB();
    
#if LW_CFG_CPU_ARCH_SMT > 0
    if (pcpu->CPU_ptcbTCBCur->TCB_usIndex >= LW_NCPUS) {
        LW_PHYCPU_GET(pcpu->CPU_ulPhyId)->PHYCPU_uiNonIdle--;           /*  ���еķ� Idle ����          */
    }
#endif
    
    _CandTableRemove(pcpu);                                             /*  �Ƴ���ѡִ���߳�            */
    LW_CAND_ROT(pcpu) = LW_FALSE;                                       /*  ������ȼ����Ʊ�־          */

    pcpu->CPU_ptcbTCBCur  = LW_NULL;
    pcpu->CPU_ptcbTCBHigh = LW_NULL;
    
    if (ptcb->TCB_bCPULock) {                                           /*  �������������׺Ͷ�          */
        return  (ERROR_NONE);
    }
    
    bRotIdle = LW_FALSE;
    
    LW_CPU_FOREACH_ACTIVE_EXCEPT (i, ulCPUId) {                         /*  CPU �����Ǽ���״̬          */
        pcpuOther = LW_CPU_GET(i);
        ptcbOther = LW_CAND_TCB(pcpuOther);
        
        if (LW_CPU_ONLY_AFFINITY_GET(pcpuOther)) {                      /*  �������׺Ͷ�����            */
            continue;
        }

        if (!LW_CAND_ROT(pcpuOther) && 
            (LW_PRIO_IS_EQU(LW_PRIO_LOWEST, 
                            ptcbOther->TCB_ucPriority))) {              /*  ���� idle �������ޱ�ע      */
            LW_CAND_ROT(pcpuOther) = LW_TRUE;
            bRotIdle               = LW_TRUE;
            break;                                                      /*  ֻ��עһ�� CPU ����         */
        }
    }
    
    if (!bRotIdle) {
        LW_CPU_FOREACH_ACTIVE_EXCEPT (i, ulCPUId) {                     /*  CPU �����Ǽ���״̬          */
            pcpuOther = LW_CPU_GET(i);
            ptcbOther = LW_CAND_TCB(pcpuOther);
            
            if (LW_CPU_ONLY_AFFINITY_GET(pcpuOther)) {                  /*  �������׺Ͷ�����            */
                continue;
            }

            if (LW_PRIO_IS_HIGH(ptcb->TCB_ucPriority,
                                ptcbOther->TCB_ucPriority)) {
                LW_CAND_ROT(pcpuOther) = LW_TRUE;
            }
        }
    }
    
    _CpuBogoMipsClear(ulCPUId);                                         /*  ���֮ǰ����� BogoMIPS     */
    
    return  (ERROR_NONE);
}

#endif                                                                  /*  LW_CFG_SMP_CPU_DOWN_EN > 0  */
/*********************************************************************************************************
** ��������: _CpuGetCur
** ��������: ��ȡ��ǰ CPU ���ƿ� (�����������ж�״̬�±�����)
** �䡡��  : NONE
** �䡡��  : ��ǰ CPU ���ƿ�
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
#if defined(__SYLIXOS_ARM_ARCH_M__)

PLW_CLASS_CPU  _CpuGetCur (VOID)
{
    return  (LW_CPU_GET_CUR());
}

#endif                                                                  /*  __SYLIXOS_ARM_ARCH_M__      */
/*********************************************************************************************************
** ��������: _CpuGetNesting
** ��������: ��ȡ CPU ��ǰ�ж�Ƕ��ֵ
** �䡡��  : NONE
** �䡡��  : �ж�Ƕ��ֵ
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
ULONG  _CpuGetNesting (VOID)
{
#if LW_CFG_SMP_EN > 0
    INTREG          iregInterLevel;
    ULONG           ulNesting;
    
    iregInterLevel = KN_INT_DISABLE();                                  /*  �ر��ж�                    */
    ulNesting      = LW_CPU_GET_CUR()->CPU_ulInterNesting;
    KN_INT_ENABLE(iregInterLevel);                                      /*  ���ж�                    */
    
    return  (ulNesting);
#else
    return  (LW_CPU_GET_CUR()->CPU_ulInterNesting);
#endif                                                                  /*  LW_CFG_SMP_EN > 0           */
}
/*********************************************************************************************************
** ��������: _CpuGetMaxNesting
** ��������: ��ȡ CPU ����ж�Ƕ��ֵ
** �䡡��  : NONE
** �䡡��  : �ж�Ƕ��ֵ
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
ULONG  _CpuGetMaxNesting (VOID)
{
#if LW_CFG_SMP_EN > 0
    INTREG          iregInterLevel;
    ULONG           ulNesting;
    
    iregInterLevel = KN_INT_DISABLE();                                  /*  �ر��ж�                    */
    ulNesting      = LW_CPU_GET_CUR()->CPU_ulInterNestingMax;
    KN_INT_ENABLE(iregInterLevel);                                      /*  ���ж�                    */
    
    return  (ulNesting);
#else
    return  (LW_CPU_GET_CUR()->CPU_ulInterNestingMax);
#endif                                                                  /*  LW_CFG_SMP_EN > 0           */
}
/*********************************************************************************************************
** ��������: _CpuSetSchedAffinity
** ��������: ����ָ�� CPU Ϊǿ�׺Ͷȵ���ģʽ. (�� 0 �� CPU, �����ں˲����жϱ�����)
** �䡡��  : ulCPUId       CPU ID 
**           bEnable       �Ƿ�ʹ��Ϊǿ�׺Ͷ�ģʽ
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
#if LW_CFG_SMP_EN > 0

VOID  _CpuSetSchedAffinity (ULONG  ulCPUId, BOOL  bEnable)
{
    PLW_CLASS_CPU   pcpu;
    
    pcpu = LW_CPU_GET(ulCPUId);
    if (LW_CPU_ONLY_AFFINITY_GET(pcpu) == bEnable) {
        return;                                                         /*  ����Ҫ�����κδ���          */
    }
    
    LW_CPU_ONLY_AFFINITY_SET(pcpu, bEnable);
    LW_CAND_ROT(pcpu) = LW_TRUE;                                        /*  ��Ҫ���Ե���                */
    
    if ((LW_CPU_GET_CUR_ID() != ulCPUId) &&
        (LW_CPU_GET_IPI_PEND(ulCPUId) & LW_IPI_SCHED_MSK) == 0) {
        _SmpSendIpi(ulCPUId, LW_IPI_SCHED, 0, LW_TRUE);                 /*  �����˼��ж�                */
    }
}
/*********************************************************************************************************
** ��������: _CpuGetSchedAffinity
** ��������: ��ȡָ�� CPU �Ƿ�Ϊǿ�׺Ͷȵ���ģʽ. (�����ں˲����жϱ�����)
** �䡡��  : ulCPUId       CPU ID 
**           pbEnable      �Ƿ�ʹ��Ϊǿ�׺Ͷ�ģʽ
** �䡡��  : ERROR
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
VOID  _CpuGetSchedAffinity (ULONG  ulCPUId, BOOL  *pbEnable)
{
    PLW_CLASS_CPU   pcpu;
    
    pcpu = LW_CPU_GET(ulCPUId);
    *pbEnable = LW_CPU_ONLY_AFFINITY_GET(pcpu);
}

#endif                                                                  /*  LW_CFG_SMP_EN > 0           */
/*********************************************************************************************************
  END
*********************************************************************************************************/
