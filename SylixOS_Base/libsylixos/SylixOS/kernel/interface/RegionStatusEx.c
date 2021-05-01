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
** ��   ��   ��: RegionStatusEx.c
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2007 �� 04 �� 12 ��
**
** ��        ��: ���һ���ڴ滺����״̬:�߼��ӿ�

** BUG
2008.01.13  ���� _DebugHandle() ���ܡ�
2008.01.24  �����������ڴ����ĺ���.
2008.05.18  ʹ�� __KERNEL_ENTER() ���� ThreadLock();
2009.05.21  ��������ʹ�����Ļ�ȡ.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
/*********************************************************************************************************
** ��������: API_RegionStatusEx
** ��������: ���һ���ڴ滺����״̬�߼��ӿ�
** �䡡��  : 
**           ulId                         REGION ���
**           pstByteSize                  REGION ��С    ����Ϊ NULL
**           pulSegmentCounter            �ֶ�����       ����Ϊ NULL
**           pstUsedByteSize              ʹ�õ��ֽ���   ����Ϊ NULL
**           pstFreeByteSize              ���е��ֽ���   ����Ϊ NULL
**           pstMaxUsedByteSize           ���ķ�����   ����Ϊ NULL
**           psegmentList[]               �ֶ�ͷ��ַ��   ����Ϊ NULL
**           iMaxCounter                  �ֶ�ͷ��ַ�������Ա��������
** �䡡��  : 
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
#if (LW_CFG_REGION_EN > 0) && (LW_CFG_MAX_REGIONS > 0)

LW_API  
ULONG  API_RegionStatusEx (LW_OBJECT_HANDLE    ulId,
                           size_t             *pstByteSize,
                           ULONG              *pulSegmentCounter,
                           size_t             *pstUsedByteSize,
                           size_t             *pstFreeByteSize,
                           size_t             *pstMaxUsedByteSize,
                           PLW_CLASS_SEGMENT   psegmentList[],
                           INT                 iMaxCounter)
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

    if (psegmentList) {
        _HeapGetInfo(pheap, psegmentList, iMaxCounter);
    }
    
    __KERNEL_EXIT();                                                    /*  �˳��ں�                    */
    
    return  (ERROR_NONE);
}

#endif                                                                  /*  (LW_CFG_REGION_EN > 0)      */
                                                                        /*  (LW_CFG_MAX_REGIONS > 0)    */
/*********************************************************************************************************
  END
*********************************************************************************************************/
