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
** ��   ��   ��: GetLastError.c
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2006 �� 12 �� 14 ��
**
** ��        ��: �û����Ե������ API ���ϵͳ���һ������

** BUG
2007.03.22  ����ϵͳû������ʱ�Ĵ��������
2012.03.20  ���ٶ� _K_ptcbTCBCur ������, �������þֲ�����, ���ٶԵ�ǰ CPU ID ��ȡ�Ĵ���.
2013.07.18  ʹ���µĻ�ȡ TCB �ķ���, ȷ�� SMP ϵͳ��ȫ.
2014.06.04  ����� last error �����ù���.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
/*********************************************************************************************************
** ��������: API_GetLastError
** ��������: ���ϵͳ���һ������
** �䡡��  : 
** �䡡��  : ERROR CODE
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
ULONG  API_GetLastError (VOID)
{
    INTREG          iregInterLevel;
    PLW_CLASS_CPU   pcpuCur;
    PLW_CLASS_TCB   ptcbCur;
    ULONG           ulLastError;
    
    iregInterLevel = KN_INT_DISABLE();                                  /*  �ر��ж�, ��ֹ���ȵ����� CPU*/
    
    pcpuCur = LW_CPU_GET_CUR();
    if (pcpuCur->CPU_ulInterNesting) {
        ulLastError = pcpuCur->CPU_ulInterError[pcpuCur->CPU_ulInterNesting];
    
    } else {
        ptcbCur = pcpuCur->CPU_ptcbTCBCur;
        if (ptcbCur) {
            ulLastError = ptcbCur->TCB_ulLastError;
        } else {
            ulLastError = _K_ulNotRunError;
        }
    }
    
    KN_INT_ENABLE(iregInterLevel);

    return  (ulLastError);
}
/*********************************************************************************************************
** ��������: API_GetLastErrorEx
** ��������: ���ָ������� errno
** �䡡��  : ulId      ���� ID
**           pulError  �����
** �䡡��  : ERROR CODE
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
ULONG  API_GetLastErrorEx (LW_OBJECT_HANDLE  ulId, ULONG  *pulError)
{
    REGISTER UINT16             usIndex;
    REGISTER PLW_CLASS_TCB      ptcb;
    
    if (!pulError) {
        _ErrorHandle(EINVAL);
        return  (EINVAL);
    }
    
    usIndex = _ObjectGetIndex(ulId);
    
    if (!_ObjectClassOK(ulId, _OBJECT_THREAD)) {
        _ErrorHandle(ERROR_KERNEL_HANDLE_NULL);
        return  (ERROR_KERNEL_HANDLE_NULL);
    }
    
    if (_Thread_Index_Invalid(usIndex)) {                               /*  ����߳���Ч��              */
        _ErrorHandle(ERROR_THREAD_NULL);
        return  (ERROR_THREAD_NULL);
    }
    
    __KERNEL_ENTER();                                                   /*  �����ں�                    */
    if (_Thread_Invalid(usIndex)) {
        __KERNEL_EXIT();                                                /*  �˳��ں�                    */
        _ErrorHandle(ERROR_THREAD_NULL);
        return  (ERROR_THREAD_NULL);
    }
    
    ptcb = _K_ptcbTCBIdTable[usIndex];
    
    *pulError = ptcb->TCB_ulLastError;
    
    __KERNEL_EXIT();                                                    /*  �˳��ں�                    */
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: API_SetLastError
** ��������: ���õ�ǰ�� LastError
** �䡡��  : ulError       ��ǰ�����
** �䡡��  : ERROR CODE
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
VOID  API_SetLastError (ULONG  ulError)
{
    errno = (INT)ulError;
}
/*********************************************************************************************************
** ��������: API_SetLastErrorEx
** ��������: ����ָ������� errno
** �䡡��  : ulId      ���� ID
**           ulError   �����
** �䡡��  : ERROR CODE
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
ULONG  API_SetLastErrorEx (LW_OBJECT_HANDLE  ulId, ULONG  ulError)
{
    REGISTER UINT16             usIndex;
    REGISTER PLW_CLASS_TCB      ptcb;
    
    usIndex = _ObjectGetIndex(ulId);
    
    if (!_ObjectClassOK(ulId, _OBJECT_THREAD)) {
        _ErrorHandle(ERROR_KERNEL_HANDLE_NULL);
        return  (ERROR_KERNEL_HANDLE_NULL);
    }
    
    if (_Thread_Index_Invalid(usIndex)) {                               /*  ����߳���Ч��              */
        _ErrorHandle(ERROR_THREAD_NULL);
        return  (ERROR_THREAD_NULL);
    }
    
    __KERNEL_ENTER();                                                   /*  �����ں�                    */
    if (_Thread_Invalid(usIndex)) {
        __KERNEL_EXIT();                                                /*  �˳��ں�                    */
        _ErrorHandle(ERROR_THREAD_NULL);
        return  (ERROR_THREAD_NULL);
    }
    
    ptcb = _K_ptcbTCBIdTable[usIndex];
    
    ptcb->TCB_ulLastError = ulError;
    
    __KERNEL_EXIT();                                                    /*  �˳��ں�                    */
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
  END
*********************************************************************************************************/
