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
** ��   ��   ��: CpuActive.c
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2014 �� 07 �� 21 ��
**
** ��        ��: SMP ϵͳ����/�ر�һ�� CPU.
**
** ע        ��: �� CPU ��������ֹͣ��������.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
/*********************************************************************************************************
  �ü�����
*********************************************************************************************************/
#if LW_CFG_SMP_EN > 0
/*********************************************************************************************************
** ��������: API_CpuUp
** ��������: ����һ�� CPU. (�� 0 �� CPU)
** �䡡��  : ulCPUId       CPU ID
** �䡡��  : ERROR
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
ULONG  API_CpuUp (ULONG  ulCPUId)
{
    INTREG          iregInterLevel;
    PLW_CLASS_CPU   pcpu;

    if ((ulCPUId == 0) || (ulCPUId >= LW_NCPUS)) {
        _ErrorHandle(EINVAL);
        return  (EINVAL);
    }
    
    KN_SMP_MB();
    pcpu = LW_CPU_GET(ulCPUId);
    if (LW_CPU_IS_ACTIVE(pcpu) || 
        (LW_CPU_GET_IPI_PEND2(pcpu) & LW_IPI_DOWN_MSK)) {
        return  (ERROR_NONE);
    }
    
    iregInterLevel = KN_INT_DISABLE();
    bspCpuUp(ulCPUId);
    KN_INT_ENABLE(iregInterLevel);
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: API_CpuDown
** ��������: �ر�һ�� CPU. (�� 0 �� CPU, ���ú󲢲������Ϲر�, ��Ҫ�ӳ�ʹ�� API_CpuIsUp ̽��)
** �䡡��  : ulCPUId       CPU ID
** �䡡��  : ERROR
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
#if LW_CFG_SMP_CPU_DOWN_EN > 0

LW_API  
ULONG  API_CpuDown (ULONG  ulCPUId)
{
#if LW_CFG_CPU_ATOMIC_EN == 0
    INTREG          iregInterLevel;
#endif
    
    PLW_CLASS_CPU   pcpu;

    if ((ulCPUId == 0) || (ulCPUId >= LW_NCPUS)) {
        _ErrorHandle(EINVAL);
        return  (EINVAL);
    }
    
    __KERNEL_ENTER();
    pcpu = LW_CPU_GET(ulCPUId);
    if (!LW_CPU_IS_ACTIVE(pcpu) || 
        (LW_CPU_GET_IPI_PEND2(pcpu) & LW_IPI_DOWN_MSK)) {
        __KERNEL_EXIT();
        return  (ERROR_NONE);
    }
    
#if LW_CFG_CPU_ATOMIC_EN == 0
    LW_SPIN_LOCK_QUICK(&pcpu->CPU_slIpi, &iregInterLevel);
#endif
    
    LW_CPU_ADD_IPI_PEND2(pcpu, LW_IPI_DOWN_MSK);
    
#if LW_CFG_CPU_ATOMIC_EN == 0
    LW_SPIN_UNLOCK_QUICK(&pcpu->CPU_slIpi, iregInterLevel);
#endif
    
    _ThreadOffAffinity(pcpu);                                           /*  �ر���� CPU �йص��׺Ͷ�   */
    __KERNEL_EXIT();
    
    _SmpSendIpi(ulCPUId, LW_IPI_DOWN, 0, LW_FALSE);                     /*  ʹ�ú˼��ж�֪ͨ CPU ֹͣ   */
    
    return  (ERROR_NONE);
}

#endif                                                                  /*  LW_CFG_SMP_CPU_DOWN_EN > 0  */
/*********************************************************************************************************
** ��������: API_CpuIsUp
** ��������: ָ�� CPU �Ƿ�����.
** �䡡��  : ulCPUId       CPU ID
** �䡡��  : �Ƿ�����
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
BOOL  API_CpuIsUp (ULONG  ulCPUId)
{
    PLW_CLASS_CPU   pcpu;
    
    if (ulCPUId >= LW_NCPUS) {
        _ErrorHandle(EINVAL);
        return  (LW_FALSE);
    }
    
    KN_SMP_MB();
    pcpu = LW_CPU_GET(ulCPUId);
    if (LW_CPU_IS_ACTIVE(pcpu)) {
        return  (LW_TRUE);
    
    } else {
        return  (LW_FALSE);
    }
}

/*********************************************************************************************************
** ��������: API_CpuIsRunning
** ��������: ָ�� CPU �Ƿ�������.
** �䡡��  : ulCPUId       CPU ID
** �䡡��  : �Ƿ�������
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
BOOL  API_CpuIsRunning (ULONG  ulCPUId)
{
    PLW_CLASS_CPU   pcpu;
    
    if (ulCPUId >= LW_NCPUS) {
        _ErrorHandle(EINVAL);
        return  (LW_FALSE);
    }
    
    KN_SMP_MB();
    pcpu = LW_CPU_GET(ulCPUId);
    if (LW_CPU_IS_RUNNING(pcpu)) {
        return  (LW_TRUE);
    
    } else {
        return  (LW_FALSE);
    }
}

#endif                                                                  /*  LW_CFG_SMP_EN > 0           */
/*********************************************************************************************************
** ��������: API_CpuCurId
** ��������: ���ָ�� CPU ���� CPU ID.
** �䡡��  : NONE
** �䡡��  : CPU ID
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API 
ULONG  API_CpuCurId (VOID)
{
#if LW_CFG_SMP_EN > 0
    INTREG  iregInterLevel;
    ULONG   ulCPUId;
    
    iregInterLevel = KN_INT_DISABLE();
    ulCPUId = LW_CPU_GET_CUR_ID();
    KN_INT_ENABLE(iregInterLevel);
    
    return  (ulCPUId);
#else                                                                   /*  LW_CFG_SMP_EN > 0           */
    return  (0ul);
#endif
}
/*********************************************************************************************************
** ��������: API_CpuPhyId
** ��������: ���ָ�� CPU ���� CPU ID.
** �䡡��  : ulCPUId       CPU ID
**           pulPhyId      ���� CPU ID
** �䡡��  : ERROR
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API 
ULONG  API_CpuPhyId (ULONG  ulCPUId, ULONG  *pulPhyId)
{
    if ((ulCPUId >= LW_NCPUS) || !pulPhyId) {
        _ErrorHandle(EINVAL);
        return  (EINVAL);
    }
    
#if (LW_CFG_SMP_EN > 0) && (LW_CFG_CPU_ARCH_SMT > 0)
    *pulPhyId = LW_CPU_GET(ulCPUId)->CPU_ulPhyId;
#else
    *pulPhyId = ulCPUId;
#endif
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: API_CpuNum
** ��������: CPU ����
** �䡡��  : NONE
** �䡡��  : CPU ����
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API 
ULONG  API_CpuNum (VOID)
{
#if LW_CFG_SMP_EN > 0
    return  (LW_NCPUS);
#else
    return  (1ul);
#endif                                                                  /*  LW_CFG_SMP_EN > 0           */
}
/*********************************************************************************************************
** ��������: API_CpuUpNum
** ��������: �Ѿ������� CPU ����
** �䡡��  : NONE
** �䡡��  : �Ѿ������� CPU ����
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API 
ULONG  API_CpuUpNum (VOID)
{
#if LW_CFG_SMP_EN > 0
    INT     i;
    ULONG   ulCnt = 0;
    
    LW_CPU_FOREACH_ACTIVE (i) {
        ulCnt++;
    }
    
    return  (ulCnt);
#else
    return  (1ul);
#endif                                                                  /*  LW_CFG_SMP_EN > 0           */
}
/*********************************************************************************************************
  END
*********************************************************************************************************/
