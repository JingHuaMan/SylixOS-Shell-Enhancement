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
** ��   ��   ��: CpuAffinity.c
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2018 �� 06 �� 06 ��
**
** ��        ��: SMP ϵͳ���� CPU Ϊǿ�׺Ͷȵ���ģʽ.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
/*********************************************************************************************************
  �ü�����
*********************************************************************************************************/
#if LW_CFG_SMP_EN > 0
/*********************************************************************************************************
** ��������: API_CpuSetSchedAffinity
** ��������: ����ָ�� CPU Ϊǿ�׺Ͷȵ���ģʽ. (�� 0 �� CPU)
** �䡡��  : stSize        CPU ���뼯�ڴ��С
**           pcpuset       CPU ����
** �䡡��  : ERROR
** ȫ�ֱ���: 
** ����ģ��: 
** ע  ��  : ��������
                                           API ����
*********************************************************************************************************/
LW_API 
ULONG  API_CpuSetSchedAffinity (size_t  stSize, const PLW_CLASS_CPUSET  pcpuset)
{
    INTREG  iregInterLevel;
    ULONG   i;
    ULONG   ulNumChk;

    if (!LW_SYS_STATUS_IS_RUNNING()) {                                  /*  ϵͳ�����Ѿ�����            */
        _ErrorHandle(ERROR_KERNEL_NOT_RUNNING);
        return  (ERROR_KERNEL_NOT_RUNNING);
    }
    
    if (!stSize || !pcpuset) {
        _ErrorHandle(EINVAL);
        return  (EINVAL);
    }

    ulNumChk = ((ULONG)stSize << 3);
    ulNumChk = (ulNumChk > LW_NCPUS) ? LW_NCPUS : ulNumChk;
    
    for (i = 1; i < ulNumChk; i++) {                                    /*  CPU 0 ��������              */
        iregInterLevel = __KERNEL_ENTER_IRQ();                          /*  �����ں�                    */
        if (LW_CPU_ISSET(i, pcpuset)) {
            _CpuSetSchedAffinity(i, LW_TRUE);
        
        } else {
            _CpuSetSchedAffinity(i, LW_FALSE);
        }
        __KERNEL_EXIT_IRQ(iregInterLevel);                              /*  �˳��ں�                    */
    }
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: API_CpuGetSchedAffinity
** ��������: ��ȡָ�� CPU �Ƿ�Ϊǿ�׺Ͷȵ���ģʽ.
** �䡡��  : stSize        CPU ���뼯�ڴ��С
**           pcpuset       CPU ����
** �䡡��  : ERROR
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API 
ULONG  API_CpuGetSchedAffinity (size_t  stSize, PLW_CLASS_CPUSET  pcpuset)
{
    INTREG  iregInterLevel;
    BOOL    bEnable;
    ULONG   i;
    ULONG   ulNumChk;

    if (!LW_SYS_STATUS_IS_RUNNING()) {                                  /*  ϵͳ�����Ѿ�����            */
        _ErrorHandle(ERROR_KERNEL_NOT_RUNNING);
        return  (ERROR_KERNEL_NOT_RUNNING);
    }
    
    if (!stSize || !pcpuset) {
        _ErrorHandle(EINVAL);
        return  (EINVAL);
    }

    ulNumChk = ((ULONG)stSize << 3);
    ulNumChk = (ulNumChk > LW_NCPUS) ? LW_NCPUS : ulNumChk;
    
    LW_CPU_ZERO(pcpuset);
    
    for (i = 1; i < ulNumChk; i++) {                                    /*  CPU 0 ��������              */
        iregInterLevel = __KERNEL_ENTER_IRQ();                          /*  �����ں�                    */
        _CpuGetSchedAffinity(i, &bEnable);
        if (bEnable) {
            LW_CPU_SET(i, pcpuset);
            
        } else {
            LW_CPU_CLR(i, pcpuset);
        }
        __KERNEL_EXIT_IRQ(iregInterLevel);                              /*  �˳��ں�                    */
    }
    
    return  (ERROR_NONE);
}

#endif                                                                  /*  LW_CFG_SMP_EN > 0           */
/*********************************************************************************************************
  END
*********************************************************************************************************/
