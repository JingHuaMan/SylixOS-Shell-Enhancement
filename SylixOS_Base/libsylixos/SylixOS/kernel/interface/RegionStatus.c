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
2009.05.21  ��������ʹ�����Ļ�ȡ.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
/*********************************************************************************************************
** ��������: API_RegionStatus
** ��������: ���һ���ڴ滺����״̬
** �䡡��  : 
**           ulId                         REGION ���
**           pstByteSize                  REGION ��С    ����Ϊ NULL
**           pulSegmentCounter            �ֶ�����       ����Ϊ NULL
**           pstUsedByteSize              ʹ�õ��ֽ���   ����Ϊ NULL
**           pstFreeByteSize              ���е��ֽ���   ����Ϊ NULL
**           pstMaxUsedByteSize           ʹ�÷�ֵ�ֽ��� ����Ϊ NULL
** �䡡��  : 
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
#if (LW_CFG_REGION_EN > 0) && (LW_CFG_MAX_REGIONS > 0)

LW_API  
ULONG  API_RegionStatus (LW_OBJECT_HANDLE    ulId,
                         size_t             *pstByteSize,
                         ULONG              *pulSegmentCounter,
                         size_t             *pstUsedByteSize,
                         size_t             *pstFreeByteSize,
                         size_t             *pstMaxUsedByteSize)
{
    REGISTER PLW_CLASS_HEAP            pheap;
    REGISTER UINT16                    usIndex;
    
    usIndex = _ObjectGetIndex(ulId);
    
#if LW_CFG_ARG_CHK_EN > 0
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
    
    if (pstByteSize) {
        *pstByteSize = pheap->HEAP_stTotalByteSize;
    }
    
    if (pulSegmentCounter) {
        *pulSegmentCounter = pheap->HEAP_ulSegmentCounter;
    }
    
    if (pstUsedByteSize) {
        *pstUsedByteSize = pheap->HEAP_stUsedByteSize;
    }
    
    if (pstFreeByteSize) {
        *pstFreeByteSize = pheap->HEAP_stFreeByteSize;
    }
    
    if (pstMaxUsedByteSize) {
        *pstMaxUsedByteSize = pheap->HEAP_stMaxUsedByteSize;
    }
    
    __KERNEL_EXIT();                                                    /*  �˳��ں�                    */
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: API_RegionGetMax
** ��������: ���һ���ڴ�������������С
** �䡡��  : 
**           ulId                         REGION ���
**           pstMaxFreeSize               ����ڴ������С
** �䡡��  : 
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
ULONG  API_RegionGetMax (LW_OBJECT_HANDLE  ulId, size_t  *pstMaxFreeSize)
{
    REGISTER PLW_CLASS_HEAP            pheap;
    REGISTER UINT16                    usIndex;
    
    usIndex = _ObjectGetIndex(ulId);
    
#if LW_CFG_ARG_CHK_EN > 0
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
    
    if (pstMaxFreeSize) {
        *pstMaxFreeSize = _HeapGetMax(pheap);
    }
    
    __KERNEL_EXIT();                                                    /*  �˳��ں�                    */
    
    return  (ERROR_NONE);
}

#endif                                                                  /*  (LW_CFG_REGION_EN > 0)      */
                                                                        /*  (LW_CFG_MAX_REGIONS > 0)    */
/*********************************************************************************************************
  END
*********************************************************************************************************/
