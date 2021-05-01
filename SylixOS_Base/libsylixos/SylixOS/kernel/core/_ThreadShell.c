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
** ��   ��   ��: _ThreadShell.c
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2006 �� 12 �� 13 ��
**
** ��        ��: ����ϵͳ�̵߳���ǡ�

** BUG
2007.11.04  �� 0xFFFFFFFF ��Ϊ __ARCH_ULONG_MAX.
2008.01.16  API_ThreadDelete() -> API_ThreadForceDelete();
2012.03.20  ���ٶ� _K_ptcbTCBCur ������, �������þֲ�����, ���ٶԵ�ǰ CPU ID ��ȡ�Ĵ���.
2015.12.05  ����̴߳���ʱ�Ѿ�ʹ���� FPU ��ֱ�Ӵ� FPU.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
/*********************************************************************************************************
** ��������: _ThreadShell
** ��������: �߳���Ǻ���
** �䡡��  : pvThreadStartAddress              �̴߳������ʼ��ַ
** �䡡��  : LW_NULL
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
PVOID  _ThreadShell (PVOID  pvThreadStartAddress)
{
    INTREG              iregInterLevel;
    PLW_CLASS_TCB       ptcbCur;
    PVOID               pvReturnVal;
    LW_OBJECT_HANDLE    ulId;
    
    LW_TCB_GET_CUR_SAFE(ptcbCur);                                       /*  ��ǰ������ƿ�              */
    
#if LW_CFG_CPU_FPU_EN > 0
    if (ptcbCur->TCB_ulOption & LW_OPTION_THREAD_USED_FP) {             /*  ǿ��ʹ�� FPU                */
        iregInterLevel = KN_INT_DISABLE();                              /*  �ر��ж�                    */
        __ARCH_FPU_ENABLE();                                            /*  ʹ�� FPU                    */
        KN_INT_ENABLE(iregInterLevel);                                  /*  ���ж�                    */
    }
#endif                                                                  /*  LW_CFG_CPU_FPU_EN > 0       */
    
#if LW_CFG_CPU_DSP_EN > 0
    if (ptcbCur->TCB_ulOption & LW_OPTION_THREAD_USED_DSP) {            /*  ǿ��ʹ�� DSP                */
        iregInterLevel = KN_INT_DISABLE();                              /*  �ر��ж�                    */
        __ARCH_DSP_ENABLE();                                            /*  ʹ�� DSP                    */
        KN_INT_ENABLE(iregInterLevel);                                  /*  ���ж�                    */
    }
#endif                                                                  /*  LW_CFG_CPU_DSP_EN > 0       */

    LW_SOFUNC_PREPARE(pvThreadStartAddress);
    pvReturnVal = ((PTHREAD_START_ROUTINE)pvThreadStartAddress)
                  (ptcbCur->TCB_pvArg);                                 /*  ִ���߳�                    */

    ulId = ptcbCur->TCB_ulId;

#if LW_CFG_THREAD_DEL_EN > 0
    API_ThreadForceDelete(&ulId, pvReturnVal);                          /*  ɾ���߳�                    */
#endif

#if LW_CFG_THREAD_SUSPEND_EN > 0
    API_ThreadSuspend(ulId);                                            /*  �����߳�                    */
#endif

    for (;;) {
        API_TimeSleep(__ARCH_ULONG_MAX);                                /*  ˯��                        */
    }
    
    return  (LW_NULL);
}
/*********************************************************************************************************
  END
*********************************************************************************************************/
