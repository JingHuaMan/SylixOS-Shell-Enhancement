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
** ��   ��   ��: KernelHookDelete.c
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2006 �� 12 �� 14 ��
**
** ��        ��: ɾ���ں�ָ�� HOOK

** BUG
2007.10.28  �޸�ע��.
2010.08.03  ֧�� SMP ���.
2012.09.22  ��������Դ����� HOOK.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
/*********************************************************************************************************
** ��������: API_KernelHookDelete
** ��������: ɾ���ں�ָ�� HOOK
** �䡡��  : 
**           ulOpt                         HOOK ����
** �䡡��  : ERROR
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
ULONG  API_KernelHookDelete (ULONG  ulOpt)
{
    INTREG  iregInterLevel;
        
    iregInterLevel = __KERNEL_ENTER_IRQ();                              /*  �����ں�ͬʱ�ر��ж�        */
    
    switch (ulOpt) {
    
    case LW_OPTION_THREAD_CREATE_HOOK:                                  /*  �߳̽�������                */
        _K_hookKernel.HOOK_ThreadCreate = LW_NULL;
        break;
    
    case LW_OPTION_THREAD_DELETE_HOOK:                                  /*  �߳�ɾ������                */
        _K_hookKernel.HOOK_ThreadDelete = LW_NULL;
        break;
    
    case LW_OPTION_THREAD_SWAP_HOOK:                                    /*  �߳��л�����                */
        _K_hookKernel.HOOK_ThreadSwap = LW_NULL;
        break;
    
    case LW_OPTION_THREAD_TICK_HOOK:                                    /*  ϵͳʱ���жϹ���            */
        _K_hookKernel.HOOK_ThreadTick = LW_NULL;
        break;
    
    case LW_OPTION_THREAD_INIT_HOOK:                                    /*  �̳߳�ʼ������              */
        _K_hookKernel.HOOK_ThreadInit = LW_NULL;
        break;
    
    case LW_OPTION_THREAD_IDLE_HOOK:                                    /*  �����̹߳���                */
        _K_hookKernel.HOOK_ThreadIdle = LW_NULL;
        break;
    
    case LW_OPTION_KERNEL_INITBEGIN:                                    /*  �ں˳�ʼ����ʼ����          */
        _K_hookKernel.HOOK_KernelInitBegin = LW_NULL;
        break;
    
    case LW_OPTION_KERNEL_INITEND:                                      /*  �ں˳�ʼ����������          */
        _K_hookKernel.HOOK_KernelInitEnd = LW_NULL;
        break;
    
    case LW_OPTION_WATCHDOG_TIMER:                                      /*  ���Ź���ʱ������            */
        _K_hookKernel.HOOK_WatchDogTimer = LW_NULL;
        break;
        
    case LW_OPTION_OBJECT_CREATE_HOOK:                                  /*  �����ں˶�����            */
        _K_hookKernel.HOOK_ObjectCreate = LW_NULL;
        break;
    
    case LW_OPTION_OBJECT_DELETE_HOOK:                                  /*  ɾ���ں˶�����            */
        _K_hookKernel.HOOK_ObjectDelete = LW_NULL;
        break;
    
    case LW_OPTION_FD_CREATE_HOOK:                                      /*  �ļ���������������          */
        _K_hookKernel.HOOK_FdCreate = LW_NULL;
        break;
    
    case LW_OPTION_FD_DELETE_HOOK:                                      /*  �ļ�������ɾ������          */
        _K_hookKernel.HOOK_FdDelete = LW_NULL;
        break;
        
    case LW_OPTION_CPU_IDLE_ENTER:                                      /*  CPU �������ģʽ            */
        _K_hookKernel.HOOK_CpuIdleEnter = LW_NULL;
        break;
    
    case LW_OPTION_CPU_IDLE_EXIT:                                       /*  CPU �˳�����ģʽ            */
        _K_hookKernel.HOOK_CpuIdleExit = LW_NULL;
        break;
    
    case LW_OPTION_CPU_INT_ENTER:                                       /*  CPU �����ж�(�쳣)ģʽ      */
        _K_hookKernel.HOOK_CpuIntEnter = LW_NULL;
        break;
    
    case LW_OPTION_CPU_INT_EXIT:                                        /*  CPU �˳��ж�(�쳣)ģʽ      */
        _K_hookKernel.HOOK_CpuIntExit = LW_NULL;
        break;
    
    default:
        __KERNEL_EXIT_IRQ(iregInterLevel);                              /*  �˳��ں�ͬʱ���ж�        */
        _ErrorHandle(ERROR_KERNEL_OPT_NULL);
        return  (ERROR_KERNEL_OPT_NULL);
    }
    
    __KERNEL_EXIT_IRQ(iregInterLevel);                                  /*  �˳��ں�ͬʱ���ж�        */
    return  (ERROR_NONE);
}
/*********************************************************************************************************
  END
*********************************************************************************************************/
