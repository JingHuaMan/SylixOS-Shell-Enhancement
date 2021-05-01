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
** ��   ��   ��: KernelIpi.c
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2014 �� 11 �� 08 ��
**
** ��        ��: SMP ϵͳ�˼��ж�.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
/*********************************************************************************************************
** ��������: API_KernelSmpCall
** ��������: ָ�� CPU ����ָ������
** �䡡��  : ulCPUId       CPU ID
**           pfunc         ͬ��ִ�к��� (�����ú����ڲ������������ں˲���, ������ܲ�������)
**           pvArg         ͬ������
**           pfuncAsync    �첽ִ�к��� (�����ú����ڲ������������ں˲���, ������ܲ�������)
**           pvAsync       �첽ִ�в���
**           iOpt          ѡ�� IPIM_OPT_NORMAL / IPIM_OPT_NOKERN
** �䡡��  : ͬ�����÷���ֵ
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
#if LW_CFG_SMP_EN > 0

LW_API  
INT  API_KernelSmpCall (ULONG        ulCPUId, 
                        FUNCPTR      pfunc, 
                        PVOID        pvArg,
                        VOIDFUNCPTR  pfuncAsync,
                        PVOID        pvAsync, 
                        INT          iOpt)
{
    BOOL    bLock;
    INT     iRet;
    
    if (LW_CPU_GET_CUR_NESTING()) {                                     /*  �������ж��е���            */
        _ErrorHandle(ERROR_KERNEL_IN_ISR);
        return  (PX_ERROR);
    }
    
    bLock = __SMP_CPU_LOCK();                                           /*  ������ǰ CPU ִ��           */
    
    if (ulCPUId == LW_CPU_GET_CUR_ID()) {                               /*  �����Լ������Լ�            */
        iRet = PX_ERROR;
    
    } else {
        iRet = _SmpCallFunc(ulCPUId, pfunc, pvArg, pfuncAsync, pvAsync, iOpt);
    }
    
    __SMP_CPU_UNLOCK(bLock);                                            /*  ������ǰ CPU ִ��           */
    
    return  (iRet);
}
/*********************************************************************************************************
** ��������: API_KernelSmpCallAll
** ��������: ���м���� CPU ����ָ������
** �䡡��  : pfunc         ͬ��ִ�к��� (�����ú����ڲ������������ں˲���, ������ܲ�������)
**           pvArg         ͬ������
**           pfuncAsync    �첽ִ�к��� (�����ú����ڲ������������ں˲���, ������ܲ�������)
**           pvAsync       �첽ִ�в���
**           iOpt          ѡ�� IPIM_OPT_NORMAL / IPIM_OPT_NOKERN
** �䡡��  : �Ƿ�ɹ�
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
INT  API_KernelSmpCallAll (FUNCPTR      pfunc, 
                           PVOID        pvArg,
                           VOIDFUNCPTR  pfuncAsync,
                           PVOID        pvAsync,
                           INT          iOpt)
{
    BOOL    bLock;
    
    if (LW_CPU_GET_CUR_NESTING()) {                                     /*  �������ж��е���            */
        _ErrorHandle(ERROR_KERNEL_IN_ISR);
        return  (PX_ERROR);
    }
    
    bLock = __SMP_CPU_LOCK();                                           /*  ������ǰ CPU ִ��           */
    
    _SmpCallFuncAllOther(pfunc, pvArg, pfuncAsync, pvAsync, iOpt);
    
    if (pfunc) {
        pfunc(pvArg);
    }
    
    if (pfuncAsync) {
        pfuncAsync(pvAsync);
    }
    
    __SMP_CPU_UNLOCK(bLock);                                            /*  ������ǰ CPU ִ��           */
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: API_KernelSmpCallAllOther
** ��������: �������м���� CPU ����ָ������
** �䡡��  : pfunc         ͬ��ִ�к��� (�����ú����ڲ������������ں˲���, ������ܲ�������)
**           pvArg         ͬ������
**           pfuncAsync    �첽ִ�к��� (�����ú����ڲ������������ں˲���, ������ܲ�������)
**           pvAsync       �첽ִ�в���
**           iOpt          ѡ�� IPIM_OPT_NORMAL / IPIM_OPT_NOKERN
** �䡡��  : �Ƿ�ɹ�
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
INT  API_KernelSmpCallAllOther (FUNCPTR      pfunc, 
                                PVOID        pvArg,
                                VOIDFUNCPTR  pfuncAsync,
                                PVOID        pvAsync,
                                INT          iOpt)
{
    BOOL    bLock;
    
    if (LW_CPU_GET_CUR_NESTING()) {                                     /*  �������ж��е���            */
        _ErrorHandle(ERROR_KERNEL_IN_ISR);
        return  (PX_ERROR);
    }
    
    bLock = __SMP_CPU_LOCK();                                           /*  ������ǰ CPU ִ��           */
    
    _SmpCallFuncAllOther(pfunc, pvArg, pfuncAsync, pvAsync, iOpt);
    
    __SMP_CPU_UNLOCK(bLock);                                            /*  ������ǰ CPU ִ��           */
    
    return  (ERROR_NONE);
}

#endif                                                                  /*  LW_CFG_SMP_EN > 0           */
/*********************************************************************************************************
  END
*********************************************************************************************************/
