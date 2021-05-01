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
** ��   ��   ��: ThreadCancel.c
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2008 �� 06 �� 07 ��
**
** ��        ��: ȡ��һ��ָ�����߳�.

** BUG:
2011.02.24  ���ٶ��ź����뼯���в���.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
#if LW_CFG_SIGNAL_EN > 0
#include "../SylixOS/system/signal/signal.h"
#endif
/*********************************************************************************************************
  �ü�����
*********************************************************************************************************/
#if LW_CFG_THREAD_CANCEL_EN > 0
/*********************************************************************************************************
** ��������: API_ThreadCancel
** ��������: ȡ��һ��ָ�����߳�
** �䡡��  : pulId     �߳̾��
** �䡡��  : ERROR_NONE or ESRCH
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
ULONG  API_ThreadCancel (LW_OBJECT_HANDLE  *pulId)
{
             INTREG                iregInterLevel;
             LW_OBJECT_HANDLE      ulId;
    REGISTER UINT16                usIndex;
    REGISTER PLW_CLASS_TCB         ptcbDel;

    ulId = *pulId;
    
    usIndex = _ObjectGetIndex(ulId);
    
#if LW_CFG_ARG_CHK_EN > 0
    if (!_ObjectClassOK(ulId, _OBJECT_THREAD)) {                        /*  ��� ID ������Ч��          */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "thread handle invalidate.\r\n");
        _ErrorHandle(ERROR_KERNEL_HANDLE_NULL);
        return  (ERROR_KERNEL_HANDLE_NULL);
    }
    
    if (_Thread_Index_Invalid(usIndex)) {                               /*  ����߳���Ч��              */
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
    
    iregInterLevel = KN_INT_DISABLE();
    
    ptcbDel = _K_ptcbTCBIdTable[usIndex];
    if (ptcbDel->TCB_iCancelState == LW_THREAD_CANCEL_ENABLE) {         /*  ���� CANCEL                 */
        if (ptcbDel->TCB_iCancelType == LW_THREAD_CANCEL_DEFERRED) {    /*  �ӳ� CANCEL                 */
            ptcbDel->TCB_bCancelRequest = LW_TRUE;                      /*  Ŀ���߳̽�����һ�� TEST ��  */
            __KERNEL_EXIT_IRQ(iregInterLevel);                          /*  �˳��ں˲����ж�          */
        
        } else {                                                        /*  �첽ȡ��                    */
            __KERNEL_EXIT_IRQ(iregInterLevel);                          /*  �˳��ں˲����ж�          */
#if LW_CFG_SIGNAL_EN > 0
            kill(ulId, SIGCANCEL);                                      /*  ��������ȡ���ź�            */
#else
            return  (API_ThreadDelete(&ulId, LW_NULL));
#endif                                                                  /*  LW_CFG_SIGNAL_EN > 0        */
        }
    
    } else {
        __KERNEL_EXIT_IRQ(iregInterLevel);                              /*  �˳��ں˲����ж�          */
        _ErrorHandle(ERROR_THREAD_DISCANCEL);                           /*  ������ CACNCEL              */
        return  (ERROR_THREAD_DISCANCEL);
    }
    
    return  (ERROR_NONE);
}

#endif                                                                  /*  LW_CFG_THREAD_CANCEL_EN > 0 */
/*********************************************************************************************************
  END
*********************************************************************************************************/
