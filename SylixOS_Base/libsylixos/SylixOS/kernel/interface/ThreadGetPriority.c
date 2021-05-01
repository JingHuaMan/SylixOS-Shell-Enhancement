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
** ��   ��   ��: ThreadGetPriority.c
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2007 �� 07 �� 18 ��
**
** ��        ��: ����߳����ȼ���

** BUG
2007.07.18  ���� _DebugHandle() ����
2008.05.18  ʹ�� __KERNEL_ENTER() ���� ThreadLock();
2009.03.10  ��Ϊ������ errno ϵͳ, һЩ API ��Ҫ����.
2012.08.28  �����ȡ idle �߳�
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
/*********************************************************************************************************
** ��������: API_ThreadGetPriority
** ��������: ����߳����ȼ���              �ж��в����ܷ������ȼ���ת�䣬�������ﲻ�ع��ж�
** �䡡��  : 
**           ulId            �߳�ID
**           pucPriority     ���ȼ�
** �䡡��  : ERROR CODE
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
#if LW_CFG_THREAD_CHANGE_PRIO_EN > 0

LW_API
ULONG  API_ThreadGetPriority (LW_OBJECT_HANDLE    ulId, UINT8  *pucPriority)
{
    REGISTER UINT16                usIndex;
    REGISTER PLW_CLASS_TCB         ptcb;
	
    usIndex = _ObjectGetIndex(ulId);
    
#if LW_CFG_ARG_CHK_EN > 0
    if (!_ObjectClassOK(ulId, _OBJECT_THREAD)) {                        /*  ��� ID ������Ч��          */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "thread handle invalidate.\r\n");
        _ErrorHandle(ERROR_KERNEL_HANDLE_NULL);
        return  (ERROR_KERNEL_HANDLE_NULL);
    }
    
    if (usIndex >= LW_CFG_MAX_THREADS) {                                /*  ����߳���Ч��              */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "thread handle invalidate.\r\n");
        _ErrorHandle(ERROR_THREAD_NULL);
        return  (ERROR_THREAD_NULL);
    }
#endif

    __KERNEL_ENTER();                                                   /*  �����ں�                    */
    if (_Thread_Invalid(usIndex)) {
        __KERNEL_EXIT();                                                /*  �˳��ں�                    */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "thread handle invalidate.\r\n");
        _ErrorHandle(ERROR_THREAD_NULL);
        return  (ERROR_THREAD_NULL);
    }
    
    ptcb = _K_ptcbTCBIdTable[usIndex];
    
    if (pucPriority) {
        *pucPriority = ptcb->TCB_ucPriority;
    }
    
    __KERNEL_EXIT();                                                    /*  �˳��ں�                    */

    return  (ERROR_NONE);
}

#endif                                                                  /*  LW_CFG_THREAD_CHANGE_PRIO_EN*/
/*********************************************************************************************************
  END
*********************************************************************************************************/
