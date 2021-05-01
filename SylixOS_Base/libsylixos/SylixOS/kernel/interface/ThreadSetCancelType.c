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
** ��   ��   ��: ThreadSetCancelType.c
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2008 �� 06 �� 07 ��
**
** ��        ��: �����߳�ȡ��������. �������첽������ͬ��.

** BUG:
2011.02.24  ���ٶ��ź����뼯���в���.
2012.03.20  ���ٶ� _K_ptcbTCBCur ������, �������þֲ�����, ���ٶԵ�ǰ CPU ID ��ȡ�Ĵ���.
2013.01,10  ���ȼ�������ɾ��������ɾ��.
2013.07.18  ʹ���µĻ�ȡ TCB �ķ���, ȷ�� SMP ϵͳ��ȫ.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
/*********************************************************************************************************
  �ü�����
*********************************************************************************************************/
#if LW_CFG_THREAD_CANCEL_EN > 0
/*********************************************************************************************************
** ��������: API_ThreadSetCancelType
** ��������: ���õ�ǰ�̱߳���ȡ��ʱ�Ķ���, 
**           ����Ϊ PTHREAD_CANCEL_ASYNCHRONOUS or PTHREAD_CANCEL_DEFERRED
** �䡡��  : 
**           iNewType           ������
**           piOldType          ��������
** �䡡��  : ERRNO
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
INT  API_ThreadSetCancelType (INT  iNewType, INT  *piOldType)
{
    INTREG                iregInterLevel;
    PLW_CLASS_TCB         ptcbCur;

    if (iNewType != LW_THREAD_CANCEL_ASYNCHRONOUS &&
        iNewType != LW_THREAD_CANCEL_DEFERRED) {
        _ErrorHandle(EINVAL);
        return  (EINVAL);
    }
    
    LW_TCB_GET_CUR_SAFE(ptcbCur);                                       /*  ��ǰ������ƿ�              */
    
    if (ptcbCur->TCB_iCancelState == LW_THREAD_CANCEL_ENABLE   &&
        ptcbCur->TCB_iCancelType  == LW_THREAD_CANCEL_DEFERRED &&
        (ptcbCur->TCB_bCancelRequest)) {                                /*  ��Ҫɾ��                    */
        LW_OBJECT_HANDLE    ulId = ptcbCur->TCB_ulId;
        
#if LW_CFG_THREAD_DEL_EN > 0
        API_ThreadDelete(&ulId, LW_THREAD_CANCELED);
#endif                                                                  /*  LW_CFG_THREAD_DEL_EN > 0    */
        return  (ERROR_NONE);
    }
    
    iregInterLevel = __KERNEL_ENTER_IRQ();                              /*  �����ں�ͬʱ�ر��ж�        */
    if (piOldType) {
        *piOldType = ptcbCur->TCB_iCancelType;
    }
    ptcbCur->TCB_iCancelType = iNewType;
    __KERNEL_EXIT_IRQ(iregInterLevel);                                  /*  �˳��ں�ͬʱ���ж�        */
    
    return  (ERROR_NONE);
}

#endif                                                                  /*  LW_CFG_THREAD_CANCEL_EN > 0 */
/*********************************************************************************************************
  END
*********************************************************************************************************/
