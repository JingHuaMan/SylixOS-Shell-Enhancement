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
** ��   ��   ��: KernelHook.c
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2006 �� 12 �� 14 ��
**
** ��        ��: ����ں�ָ�� HOOK

** BUG
2008.03.02  ������ reboot �Ļص�����.
2011.07.29  �������µĻص���������, �����ں˶�����ļ��Ĵ��������.
2012.07.04  �ϲ� API_KernelHookSet() ������
2012.09.22  ��������Դ����� HOOK.
2013.03.16  ������̻ص�.
2014.08.10  ����ϵͳ����ص�.
2015.11.21  ���� API_KernelHookGet() ����.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
/*********************************************************************************************************
** ��������: API_KernelHookGet
** ��������: ����ں�ָ�� HOOK
** �䡡��  : ulOpt                         HOOK ����
** �䡡��  : HOOK ���ܺ���
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
LW_HOOK_FUNC  API_KernelHookGet (ULONG  ulOpt)
{
    INTREG         iregInterLevel;
    LW_HOOK_FUNC   hookfuncPtr;
        
    iregInterLevel = __KERNEL_ENTER_IRQ();                              /*  �����ں�ͬʱ�ر��ж�        */
    
    switch (ulOpt) {
    
    case LW_OPTION_THREAD_CREATE_HOOK:                                  /*  �߳̽�������                */
        hookfuncPtr = _K_hookKernel.HOOK_ThreadCreate;
        break;
    
    case LW_OPTION_THREAD_DELETE_HOOK:                                  /*  �߳�ɾ������                */
        hookfuncPtr = _K_hookKernel.HOOK_ThreadDelete;
        break;
    
    case LW_OPTION_THREAD_SWAP_HOOK:                                    /*  �߳��л�����                */
        hookfuncPtr = _K_hookKernel.HOOK_ThreadSwap;
        break;
    
    case LW_OPTION_THREAD_TICK_HOOK:                                    /*  ϵͳʱ���жϹ���            */
        hookfuncPtr = _K_hookKernel.HOOK_ThreadTick;
        break;
    
    case LW_OPTION_THREAD_INIT_HOOK:                                    /*  �̳߳�ʼ������              */
        hookfuncPtr = _K_hookKernel.HOOK_ThreadInit;
        break;
    
    case LW_OPTION_THREAD_IDLE_HOOK:                                    /*  �����̹߳���                */
        hookfuncPtr = _K_hookKernel.HOOK_ThreadIdle;
        break;
    
    case LW_OPTION_KERNEL_INITBEGIN:                                    /*  �ں˳�ʼ����ʼ����          */
        hookfuncPtr = _K_hookKernel.HOOK_KernelInitBegin;
        break;
    
    case LW_OPTION_KERNEL_INITEND:                                      /*  �ں˳�ʼ����������          */
        hookfuncPtr = _K_hookKernel.HOOK_KernelInitEnd;
        break;
    
    case LW_OPTION_KERNEL_REBOOT:                                       /*  �ں�������������            */
        hookfuncPtr = _K_hookKernel.HOOK_KernelReboot;
        break;
        
    case LW_OPTION_WATCHDOG_TIMER:                                      /*  ���Ź���ʱ������            */
        hookfuncPtr = _K_hookKernel.HOOK_WatchDogTimer;
        break;
    
    case LW_OPTION_OBJECT_CREATE_HOOK:                                  /*  �����ں˶�����            */
        hookfuncPtr = _K_hookKernel.HOOK_ObjectCreate;
        break;
    
    case LW_OPTION_OBJECT_DELETE_HOOK:                                  /*  ɾ���ں˶�����            */
        hookfuncPtr = _K_hookKernel.HOOK_ObjectDelete;
        break;
    
    case LW_OPTION_FD_CREATE_HOOK:                                      /*  �ļ���������������          */
        hookfuncPtr = _K_hookKernel.HOOK_FdCreate;
        break;
    
    case LW_OPTION_FD_DELETE_HOOK:                                      /*  �ļ�������ɾ������          */
        hookfuncPtr = _K_hookKernel.HOOK_FdDelete;
        break;
    
    case LW_OPTION_CPU_IDLE_ENTER:                                      /*  CPU �������ģʽ            */
        hookfuncPtr = _K_hookKernel.HOOK_CpuIdleEnter;
        break;
    
    case LW_OPTION_CPU_IDLE_EXIT:                                       /*  CPU �˳�����ģʽ            */
        hookfuncPtr = _K_hookKernel.HOOK_CpuIdleExit;
        break;
    
    case LW_OPTION_CPU_INT_ENTER:                                       /*  CPU �����ж�(�쳣)ģʽ      */
        hookfuncPtr = _K_hookKernel.HOOK_CpuIntEnter;
        break;
    
    case LW_OPTION_CPU_INT_EXIT:                                        /*  CPU �˳��ж�(�쳣)ģʽ      */
        hookfuncPtr = _K_hookKernel.HOOK_CpuIntExit;
        break;
        
    case LW_OPTION_STACK_OVERFLOW_HOOK:                                 /*  ��ջ���                    */
        hookfuncPtr = _K_hookKernel.HOOK_StkOverflow;
        break;
    
    case LW_OPTION_FATAL_ERROR_HOOK:                                    /*  ��������                    */
        hookfuncPtr = _K_hookKernel.HOOK_FatalError;
        break;
        
    case LW_OPTION_VPROC_CREATE_HOOK:                                   /*  ���̽�������                */
        hookfuncPtr = _K_hookKernel.HOOK_VpCreate;
        break;
    
    case LW_OPTION_VPROC_DELETE_HOOK:                                   /*  ����ɾ������                */
        hookfuncPtr = _K_hookKernel.HOOK_VpDelete;
        break;
    
    default:
        __KERNEL_EXIT_IRQ(iregInterLevel);                              /*  �˳��ں�ͬʱ���ж�        */
        _ErrorHandle(ERROR_KERNEL_OPT_NULL);
        return  (LW_NULL);
    }
    
    __KERNEL_EXIT_IRQ(iregInterLevel);                                  /*  �˳��ں�ͬʱ���ж�        */
    return  (hookfuncPtr);
}
/*********************************************************************************************************
** ��������: API_KernelHookSet
** ��������: �����ں�ָ�� HOOK
** �䡡��  : 
**           hookfuncPtr                   HOOK ���ܺ���
**           ulOpt                         HOOK ����
** �䡡��  : ERROR
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
ULONG  API_KernelHookSet (LW_HOOK_FUNC  hookfuncPtr, ULONG  ulOpt)
{
    INTREG      iregInterLevel;
    
    iregInterLevel = __KERNEL_ENTER_IRQ();                              /*  �����ں�ͬʱ�ر��ж�        */
    
    switch (ulOpt) {
    
    case LW_OPTION_THREAD_CREATE_HOOK:                                  /*  �߳̽�������                */
        _K_hookKernel.HOOK_ThreadCreate = hookfuncPtr;
        break;
    
    case LW_OPTION_THREAD_DELETE_HOOK:                                  /*  �߳�ɾ������                */
        _K_hookKernel.HOOK_ThreadDelete = hookfuncPtr;
        break;
    
    case LW_OPTION_THREAD_SWAP_HOOK:                                    /*  �߳��л�����                */
        _K_hookKernel.HOOK_ThreadSwap = hookfuncPtr;
        break;
    
    case LW_OPTION_THREAD_TICK_HOOK:                                    /*  ϵͳʱ���жϹ���            */
        _K_hookKernel.HOOK_ThreadTick = hookfuncPtr;
        break;
    
    case LW_OPTION_THREAD_INIT_HOOK:                                    /*  �̳߳�ʼ������              */
        _K_hookKernel.HOOK_ThreadInit = hookfuncPtr;
        break;
    
    case LW_OPTION_THREAD_IDLE_HOOK:                                    /*  �����̹߳���                */
        _K_hookKernel.HOOK_ThreadIdle = hookfuncPtr;
        break;
    
    case LW_OPTION_KERNEL_INITBEGIN:                                    /*  �ں˳�ʼ����ʼ����          */
        _K_hookKernel.HOOK_KernelInitBegin = hookfuncPtr;
        break;
    
    case LW_OPTION_KERNEL_INITEND:                                      /*  �ں˳�ʼ����������          */
        _K_hookKernel.HOOK_KernelInitEnd = hookfuncPtr;
        break;
    
    case LW_OPTION_KERNEL_REBOOT:                                       /*  �ں�������������            */
        _K_hookKernel.HOOK_KernelReboot = hookfuncPtr;
        break;
    
    case LW_OPTION_WATCHDOG_TIMER:                                      /*  ���Ź���ʱ������            */
        _K_hookKernel.HOOK_WatchDogTimer = hookfuncPtr;
        break;
    
    case LW_OPTION_OBJECT_CREATE_HOOK:                                  /*  �����ں˶�����            */
        _K_hookKernel.HOOK_ObjectCreate = hookfuncPtr;
        break;
    
    case LW_OPTION_OBJECT_DELETE_HOOK:                                  /*  ɾ���ں˶�����            */
        _K_hookKernel.HOOK_ObjectDelete = hookfuncPtr;
        break;
    
    case LW_OPTION_FD_CREATE_HOOK:                                      /*  �ļ���������������          */
        _K_hookKernel.HOOK_FdCreate = hookfuncPtr;
        break;
    
    case LW_OPTION_FD_DELETE_HOOK:                                      /*  �ļ�������ɾ������          */
        _K_hookKernel.HOOK_FdDelete = hookfuncPtr;
        break;
    
    case LW_OPTION_CPU_IDLE_ENTER:                                      /*  CPU �������ģʽ            */
        _K_hookKernel.HOOK_CpuIdleEnter = hookfuncPtr;
        break;
    
    case LW_OPTION_CPU_IDLE_EXIT:                                       /*  CPU �˳�����ģʽ            */
        _K_hookKernel.HOOK_CpuIdleExit = hookfuncPtr;
        break;
    
    case LW_OPTION_CPU_INT_ENTER:                                       /*  CPU �����ж�(�쳣)ģʽ      */
        _K_hookKernel.HOOK_CpuIntEnter = hookfuncPtr;
        break;
    
    case LW_OPTION_CPU_INT_EXIT:                                        /*  CPU �˳��ж�(�쳣)ģʽ      */
        _K_hookKernel.HOOK_CpuIntExit = hookfuncPtr;
        break;
        
    case LW_OPTION_STACK_OVERFLOW_HOOK:                                 /*  ��ջ���                    */
        _K_hookKernel.HOOK_StkOverflow = hookfuncPtr;
        break;
    
    case LW_OPTION_FATAL_ERROR_HOOK:                                    /*  ��������                    */
        _K_hookKernel.HOOK_FatalError = hookfuncPtr;
        break;
        
    case LW_OPTION_VPROC_CREATE_HOOK:                                   /*  ���̽�������                */
        _K_hookKernel.HOOK_VpCreate = hookfuncPtr;
        break;
    
    case LW_OPTION_VPROC_DELETE_HOOK:                                   /*  ����ɾ������                */
        _K_hookKernel.HOOK_VpDelete = hookfuncPtr;
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
