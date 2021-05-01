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
** ��   ��   ��: RegionStatus.c
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2007 �� 01 �� 22 ��
**
** ��        ��: ���һ���ڴ滺����״̬

** BUG
2008.01.13  ���� _DebugHandle() ���ܡ�
2008.05.18  ʹ�� __KERNEL_ENTER() ���� ThreadLock();
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
/*********************************************************************************************************
** ��������: API_RegionGetName
** ��������: ����ڴ滺����������
** �䡡��  : 
**           ulId       REGION ���
**           pcName     ���ֻ�����
** �䡡��  : 
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
#if (LW_CFG_REGION_EN > 0) && (LW_CFG_MAX_REGIONS > 0)

LW_API  
ULONG  API_RegionGetName (LW_OBJECT_HANDLE  ulId, PCHAR  pcName)
{
    REGISTER PLW_CLASS_HEAP            pheap;
    REGISTER UINT16                    usIndex;
    
    usIndex = _ObjectGetIndex(ulId);
    
    if (LW_CPU_GET_CUR_NESTING()) {                                     /*  �������ж��е���            */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "called from ISR.\r\n");
        _ErrorHandle(ERROR_KERNEL_IN_ISR);
        return  (ERROR_KERNEL_IN_ISR);
    }
    
#if LW_CFG_ARG_CHK_EN > 0
    if (!pcName) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "name invalidate\r\n");
        _ErrorHandle(ERROR_KERNEL_PNAME_NULL);
        return  (ERROR_KERNEL_PNAME_NULL);
    }
    
    if (!_ObjectClassOK(ulId, _OBJECT_REGION)) {                        /*  �������ͼ��                */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "region handle invalidate.\r\n");
        _ErrorHandle(ERROR_KERNEL_HANDLE_NULL);
        return  (ERROR_KERNEL_HANDLE_NULL);
    }
    
    __KERNEL_ENTER();                                                   /*  �����ں�                    */
    if (_Heap_Index_Invalid(usIndex)) {                                 /*  �������������              */
        __KERNEL_EXIT();                                                /*  �˳��ں�                    */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "region handle invalidate.\r\n");
        _ErrorHandle(ERROR_KERNEL_HANDLE_NULL);
        return  (ERROR_KERNEL_HANDLE_NULL);
    }
#else
    __KERNEL_ENTER();                                                   /*  �����ں�                    */
#endif

    pheap = &_K_heapBuffer[usIndex];
    
    lib_strcpy(pcName, pheap->HEAP_cHeapName);
    
    __KERNEL_EXIT();                                                    /*  �˳��ں�                    */
    
    return  (ERROR_NONE);
}

#endif                                                                  /*  (LW_CFG_REGION_EN > 0)      */
                                                                        /*  (LW_CFG_MAX_REGIONS > 0)    */
/*********************************************************************************************************
  END
*********************************************************************************************************/
