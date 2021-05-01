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
** ��   ��   ��: CpuPerf.c
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2016 �� 11 �� 25 ��
**
** ��        ��: CPU ���ܼ���.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
/*********************************************************************************************************
  �������洢
*********************************************************************************************************/
static ULONG        _K_ulKInsPerSec[LW_CFG_MAX_PROCESSORS];
/*********************************************************************************************************
** ��������: _CpuBogoMipsCalc
** ��������: ���Լ��� CPU �����ٶ�.
** �䡡��  : ulCPUId           CPU ID
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static VOID  _CpuBogoMipsCalc (ULONG  ulCPUId)
{
    ULONG   ulLoopCnt = 1600000;                                        /*  ��Ϊ����������Ϊ 64MHz      */
    ULONG   ulKInsPerSec;
    clock_t clockTick;
    clock_t clockPer100ms = CLOCKS_PER_SEC / 10;
    
    while (ulLoopCnt <<= 1) {
        clockTick = lib_clock();
        __ARCH_BOGOMIPS_LOOP(ulLoopCnt);
        clockTick = lib_clock() - clockTick;
        
        if (clockTick >= clockPer100ms) {
            ulKInsPerSec = (ulLoopCnt / clockTick / 1000) * CLOCKS_PER_SEC;
            _K_ulKInsPerSec[ulCPUId] = ulKInsPerSec * __ARCH_BOGOMIPS_INS_PER_LOOP;
            break;
        }
    }
}
/*********************************************************************************************************
** ��������: _CpuBogoMipsClear
** ��������: ���Լ��� CPU �����ٶ�.
** �䡡��  : ulCPUId           CPU ID
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
VOID  _CpuBogoMipsClear (ULONG  ulCPUId)
{
    _K_ulKInsPerSec[ulCPUId] = 0ul;
}
/*********************************************************************************************************
** ��������: API_CpuBogoMips
** ��������: ���Լ��� CPU �����ٶ�.
** �䡡��  : ulCPUId           CPU ID
**           pulKInsPerSec     ÿ��ִ�е�ָ������ (ǧ��ָ��Ϊ��λ)
** �䡡��  : ERROR CODE
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
ULONG  API_CpuBogoMips (ULONG  ulCPUId, ULONG  *pulKInsPerSec)
{
#if LW_CFG_SMP_EN > 0
    LW_CLASS_CPUSET     pcpusetNew;
    LW_CLASS_CPUSET     pcpusetOld;
    LW_OBJECT_HANDLE    ulMe = API_ThreadIdSelf();
#endif                                                                  /*  LW_CFG_SMP_EN > 0           */

    if (ulCPUId >= LW_NCPUS) {
        _ErrorHandle(EINVAL);
        return  (EINVAL);
    }
    
    if (!LW_SYS_STATUS_IS_RUNNING()) {
        _ErrorHandle(ERROR_KERNEL_NOT_RUNNING);
        return  (ERROR_KERNEL_NOT_RUNNING);
    }
    
    if (LW_CPU_GET_CUR_NESTING()) {
        _ErrorHandle(ERROR_KERNEL_IN_ISR);
        return  (ERROR_KERNEL_IN_ISR);
    }
    
    if (_K_ulKInsPerSec[ulCPUId]) {                                     /*  ֮ǰ�Ѿ������              */
        if (pulKInsPerSec) {
            *pulKInsPerSec = _K_ulKInsPerSec[ulCPUId];
        }
        return  (ERROR_NONE);
    }
    
#if LW_CFG_SMP_EN > 0
    if (LW_NCPUS > 1) {
        if (API_CpuIsUp(ulCPUId)) {
            API_ThreadGetAffinity(ulMe, sizeof(pcpusetOld), &pcpusetOld);
            LW_CPU_ZERO(&pcpusetNew);
            LW_CPU_SET(ulCPUId, &pcpusetNew);                           /*  ���� CPU                    */
            API_ThreadSetAffinity(ulMe, sizeof(pcpusetNew), &pcpusetNew);
            _CpuBogoMipsCalc(ulCPUId);
            API_ThreadSetAffinity(ulMe, sizeof(pcpusetOld), &pcpusetOld);
        }
    } else 
#endif                                                                  /*  LW_CFG_SMP_EN > 0           */
    {
        _CpuBogoMipsCalc(ulCPUId);
    }
    
    if (pulKInsPerSec) {
        *pulKInsPerSec = _K_ulKInsPerSec[ulCPUId];
    }
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
  END
*********************************************************************************************************/
