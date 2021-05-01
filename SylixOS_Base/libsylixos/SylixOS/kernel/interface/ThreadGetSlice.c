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
** ��   ��   ��: ThreadGetSlice.c
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2007 �� 07 �� 18 ��
**
** ��        ��: ���ָ���߳�ʱ��Ƭ

** BUG
2007.07.18  ���� _DebugHandle() ����
2009.12.30  ������ʱ��Ƭ��չ�ӿ�.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
/*********************************************************************************************************
** ��������: API_ThreadGetSliceEx
** ��������: ���ָ���߳�ʱ��Ƭ(��չ�ӿ�)
** �䡡��  : 
**           ulId            �߳�ID
**           pusSliceTemp    ʱ��Ƭ
**           pusCounter      ʣ��ʱ��Ƭ
** �䡡��  : 
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
ULONG  API_ThreadGetSliceEx (LW_OBJECT_HANDLE  ulId, UINT16  *pusSliceTemp, UINT16  *pusCounter)
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
    
    ptcb = _K_ptcbTCBIdTable[usIndex];

    if (pusSliceTemp) {
        *pusSliceTemp = ptcb->TCB_usSchedSlice;
    }
    if (pusCounter) {
        *pusCounter = ptcb->TCB_usSchedCounter;
    }
    
    __KERNEL_EXIT();                                                    /*  �˳��ں�                    */
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: API_ThreadGetSlice
** ��������: ���ָ���߳�ʱ��Ƭ
** �䡡��  : 
**           ulId            �߳�ID
**           pusSliceTemp    ʱ��Ƭ
** �䡡��  : 
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
ULONG  API_ThreadGetSlice (LW_OBJECT_HANDLE  ulId, UINT16  *pusSliceTemp)
{
    return  (API_ThreadGetSliceEx(ulId, pusSliceTemp, LW_NULL));
}
/*********************************************************************************************************
  END
*********************************************************************************************************/
