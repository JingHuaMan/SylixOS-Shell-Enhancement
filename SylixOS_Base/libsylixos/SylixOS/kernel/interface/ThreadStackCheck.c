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
** ��   ��   ��: ThreadStackCheck.c
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2007 �� 07 �� 19 ��
**
** ��        ��: �߳�����ջʹ�������

** BUG
2007.04.12  ����������̶߳�ջ
2007.07.19  ���� _DebugHandle() ����
2008.05.18  ʹ�� __KERNEL_ENTER() ���� ThreadLock();
2009.04.06  ʹ���µĿ����ñ�־���жϿ��ж�ջ��, �������׼ȷ.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
/*********************************************************************************************************
** ��������: API_ThreadStackCheck
** ��������: �߳�����ջʹ�������
** �䡡��  : 
**           ulId                          �߳�ID
**           pstFreeByteSize               ���ж�ջ��С   (��Ϊ LW_NULL)
**           pstUsedByteSize               ʹ�ö�ջ��С   (��Ϊ LW_NULL)
**           pstTcbByteSize                �߳̿��ƿ��С (��Ϊ LW_NULL)
** �䡡��  : 
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
                                           
                                       (�������ж��е���)
*********************************************************************************************************/
LW_API  
ULONG  API_ThreadStackCheck (LW_OBJECT_HANDLE  ulId,
                             size_t           *pstFreeByteSize,
                             size_t           *pstUsedByteSize,
                             size_t           *pstTcbByteSize)
{
    REGISTER UINT16                usIndex;
    REGISTER PLW_CLASS_TCB         ptcb;
    
    REGISTER size_t                stTotal;
    REGISTER size_t                stFree = 0;
    
    REGISTER PLW_STACK             pstkButtom;
	
    usIndex = _ObjectGetIndex(ulId);
    
    if (LW_CPU_GET_CUR_NESTING()) {                                     /*  �������ж��е���            */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "called from ISR.\r\n");
        _ErrorHandle(ERROR_KERNEL_IN_ISR);
        return  (ERROR_KERNEL_IN_ISR);
    }
    
#if LW_CFG_ARG_CHK_EN > 0
    if (!_ObjectClassOK(ulId, _OBJECT_THREAD)) {                        /*  ��� ID ������Ч��          */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "thread handle invalidate.\r\n");
        _ErrorHandle(ERROR_KERNEL_HANDLE_NULL);
        return  (ERROR_KERNEL_HANDLE_NULL);
    }
    
    if (usIndex >= LW_CFG_MAX_THREADS) {                                /*  �����ÿ����̶߳�ջ��Ϣ    */
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
    
    if (ptcb->TCB_ulOption & LW_OPTION_THREAD_STK_CHK) {
        stTotal = ptcb->TCB_stStackSize;                                /*  �ܴ�С                      */
        
#if CPU_STK_GROWTH == 0                                                 /*  Ѱ�Ҷ�ջͷβ                */
        for (pstkButtom = ptcb->TCB_pstkStackBottom;
             (pstkButtom >= ptcb->TCB_pstkStackTop) &&
             (*pstkButtom == _K_stkFreeFlag);
             pstkButtom--,
             stFree++);
#else
        for (pstkButtom = ptcb->TCB_pstkStackBottom;
             (pstkButtom <= ptcb->TCB_pstkStackTop) &&
             (*pstkButtom == _K_stkFreeFlag);
             pstkButtom++,
             stFree++);
#endif
        __KERNEL_EXIT();                                                /*  �˳��ں�                    */
        
        if (pstFreeByteSize) {
            *pstFreeByteSize = stFree * sizeof(LW_STACK);
        }
        
        if (pstUsedByteSize) {
            *pstUsedByteSize = (stTotal - stFree) * sizeof(LW_STACK);
        }
        
        if (pstTcbByteSize) {
            *pstTcbByteSize = sizeof(LW_CLASS_TCB);
        }
        return  (ERROR_NONE);
    
    } else {
        __KERNEL_EXIT();                                                /*  �˳��ں�                    */
        
        _DebugHandle(__ERRORMESSAGE_LEVEL, "thread not has this opt.\r\n");
        _ErrorHandle(ERROR_THREAD_OPTION);
        return  (ERROR_THREAD_OPTION);
    }
}
/*********************************************************************************************************
  END
*********************************************************************************************************/
