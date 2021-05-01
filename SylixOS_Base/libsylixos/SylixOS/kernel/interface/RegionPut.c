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
** ��   ��   ��: RegionPut.c
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2007 �� 01 �� 22 ��
**
** ��        ��: �ͷ�һ���ڴ滺����

** BUG
2007.07.25  ����ǿ���ڴ���
2008.01.13  ���� _DebugHandle() ���ܡ�
2008.01.24  �������͵��ڴ����.
2008.02.05  ��ǰ�˽�����������ʱ��.
2008.05.18  ʹ�� __KERNEL_ENTER() ���� ThreadLock();
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
/*********************************************************************************************************
** ��������: API_RegionPut
** ��������: �ͷ�һ���ڴ滺����
** �䡡��  : 
**           ulId                         REGION ���
**           pvSegmentData                �ֶ�ָ��
** �䡡��  : 
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
#if (LW_CFG_REGION_EN > 0) && (LW_CFG_MAX_REGIONS > 0)

LW_API  
PVOID  API_RegionPut (LW_OBJECT_HANDLE  ulId, PVOID  pvSegmentData)
{
    REGISTER PLW_CLASS_HEAP            pheap;
    REGISTER UINT16                    usIndex;
    
    usIndex = _ObjectGetIndex(ulId);
    
    if (LW_CPU_GET_CUR_NESTING()) {                                     /*  �������ж��е���            */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "called from ISR.\r\n");
        _ErrorHandle(ERROR_KERNEL_IN_ISR);
        return  (pvSegmentData);
    }
    
#if LW_CFG_ARG_CHK_EN > 0
    if (!pvSegmentData) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "pvSegmentData invalidate\r\n");
        _ErrorHandle(ERROR_REGION_NULL);
        return  (pvSegmentData);
    }
    
    if (!_ObjectClassOK(ulId, _OBJECT_REGION)) {                        /*  �������ͼ��                */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "region handle invalidate.\r\n");
        _ErrorHandle(ERROR_KERNEL_HANDLE_NULL);
        return  (pvSegmentData);
    }
    
    __KERNEL_ENTER();                                                   /*  �����ں�                    */
    if (_Heap_Index_Invalid(usIndex)) {                                 /*  �������������              */
        __KERNEL_EXIT();                                                /*  �˳��ں�                    */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "region handle invalidate.\r\n");
        _ErrorHandle(ERROR_KERNEL_HANDLE_NULL);
        return  (pvSegmentData);
    }
#else
    __KERNEL_ENTER();                                                   /*  �����ں�                    */
#endif

    pheap = &_K_heapBuffer[usIndex];
    
    __KERNEL_EXIT();                                                    /*  �˳��ں�                    */
    
#if LW_CFG_USER_HEAP_SAFETY > 0
    pvSegmentData = _HeapFree(pheap, pvSegmentData, LW_TRUE, __func__); /*  �����ڴ���Ľ���          */
#else
    pvSegmentData = _HeapFree(pheap, pvSegmentData, LW_FALSE, __func__);
#endif                                                                  /*  LW_CFG_USER_HEAP_SAFETY     */
    
    if (pvSegmentData) {
        _ErrorHandle(ERROR_REGION_NULL);
    }
    
    return  (pvSegmentData);
}

#endif                                                                  /*  (LW_CFG_REGION_EN > 0) &&   */
                                                                        /*  (LW_CFG_MAX_REGIONS > 0)    */
/*********************************************************************************************************
  END
*********************************************************************************************************/
