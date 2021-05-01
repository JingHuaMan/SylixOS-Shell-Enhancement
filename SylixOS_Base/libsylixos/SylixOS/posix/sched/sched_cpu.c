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
** ��   ��   ��: sched_cpu.c
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2018 �� 06 �� 07 ��
**
** ��        ��: CPU ǿ�׺Ͷȵ��� (posix ��չ).
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../include/px_sched.h"                                        /*  �Ѱ�������ϵͳͷ�ļ�        */
/*********************************************************************************************************
** ��������: sched_cpuaffinity_enable_np
** ��������: ʹ�� CPU ǿ�׺Ͷȵ���.
** �䡡��  : setsize       CPU ���ϴ�С
**           set           CPU ����
** �䡡��  : ERROR or OK
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
#if (LW_CFG_SMP_EN > 0) && (LW_CFG_POSIX_EN > 0) && (LW_CFG_POSIXEX_EN > 0)

LW_API 
int  sched_cpuaffinity_enable_np (size_t setsize, const cpu_set_t *set)
{
    ULONG      i, num;
    cpu_set_t  cpuset_cur;
    
    if (!setsize || !set) {
        errno = EINVAL;
        return  (PX_ERROR);
    }
    
    if (API_CpuGetSchedAffinity(sizeof(cpu_set_t), &cpuset_cur)) {
        return  (PX_ERROR);
    }
    
    num = ((ULONG)setsize << 3);
    num = (num > LW_NCPUS) ? LW_NCPUS : num;
    
    for (i = 1; i < num; i++) {                                         /*  CPU 0 ��������              */
        if (LW_CPU_ISSET(i, set)) {
            LW_CPU_SET(i, &cpuset_cur);
        }
    }
    
    if (API_CpuSetSchedAffinity(sizeof(cpu_set_t), &cpuset_cur)) {
        return  (PX_ERROR);
    }
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: sched_cpuaffinity_disable_np
** ��������: ���� CPU ǿ�׺Ͷȵ���.
** �䡡��  : setsize       CPU ���ϴ�С
**           set           CPU ����
** �䡡��  : ERROR or OK
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API 
int  sched_cpuaffinity_disable_np (size_t setsize, const cpu_set_t *set)
{
    ULONG      i, num;
    cpu_set_t  cpuset_cur;
    
    if (!setsize || !set) {
        errno = EINVAL;
        return  (PX_ERROR);
    }
    
    if (API_CpuGetSchedAffinity(sizeof(cpu_set_t), &cpuset_cur)) {
        return  (PX_ERROR);
    }
    
    num = ((ULONG)setsize << 3);
    num = (num > LW_NCPUS) ? LW_NCPUS : num;
    
    for (i = 1; i < num; i++) {                                         /*  CPU 0 ��������              */
        if (LW_CPU_ISSET(i, set)) {
            LW_CPU_CLR(i, &cpuset_cur);
        }
    }
    
    if (API_CpuSetSchedAffinity(sizeof(cpu_set_t), &cpuset_cur)) {
        return  (PX_ERROR);
    }
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: sched_cpuaffinity_set_np
** ��������: ���� CPU ǿ�׺Ͷȵ���.
** �䡡��  : setsize       CPU ���ϴ�С
**           set           CPU ����
** �䡡��  : ERROR or OK
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API 
int  sched_cpuaffinity_set_np (size_t setsize, const cpu_set_t *set)
{
    if (!setsize || !set) {
        errno = EINVAL;
        return  (PX_ERROR);
    }

    if (API_CpuSetSchedAffinity(setsize, (PLW_CLASS_CPUSET)set)) {
        return  (PX_ERROR);
    }
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: sched_cpuaffinity_get_np
** ��������: ��ȡ CPU ǿ�׺Ͷȵ���.
** �䡡��  : setsize       CPU ���ϴ�С
**           set           CPU ����
** �䡡��  : ERROR or OK
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API 
int  sched_cpuaffinity_get_np (size_t setsize, cpu_set_t *set)
{
    if (!setsize || !set) {
        errno = EINVAL;
        return  (PX_ERROR);
    }

    if (API_CpuGetSchedAffinity(setsize, set)) {
        return  (PX_ERROR);
    }
    
    return  (ERROR_NONE);
}

#endif                                                                  /*  (LW_CFG_SMP_EN > 0)         */
                                                                        /*  (LW_CFG_POSIX_EN > 0)       */
                                                                        /*  (LW_CFG_POSIXEX_EN > 0)     */
/*********************************************************************************************************
  END
*********************************************************************************************************/
