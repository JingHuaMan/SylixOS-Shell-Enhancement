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
** ��   ��   ��: RegionAddMem.c
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2014 �� 07 �� 03 ��
**
** ��        ��: ���ڴ滺����������ڴ�.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
/*********************************************************************************************************
** ��������: API_RegionAddMem
** ��������: ���ڴ滺����������ڴ�
** �䡡��  : 
**           ulId                         REGION ���
**           pvMem                        �ڴ�
**           stByteSize                   �ֶδ�С
** �䡡��  : 
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
                                           
                                       (�������ж��е���)
*********************************************************************************************************/
#if (LW_CFG_REGION_EN > 0) && (LW_CFG_MAX_REGIONS > 0)

LW_API  
ULONG  API_RegionAddMem (LW_OBJECT_HANDLE  ulId, PVOID  pvMem, size_t  stByteSize)
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
    if (!pvMem || !_Addresses_Is_Aligned(pvMem)) {                      /*  ����ַ�Ƿ����            */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "pvLowAddr is not aligned.\r\n");
        _ErrorHandle(ERROR_KERNEL_MEMORY);
        return  (ERROR_KERNEL_MEMORY);
    }
    
    if (_Heap_ByteSize_Invalid(stByteSize)) {                           /*  �ֶ�̫С                    */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "ulRegionByteSize is too low.\r\n");
        _ErrorHandle(ERROR_REGION_SIZE);
        return  (ERROR_REGION_SIZE);
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

    __KERNEL_EXIT();                                                    /*  �˳��ں�                    */
    
    return  (_HeapAddMemory(pheap, pvMem, stByteSize));
}

#endif                                                                  /*  (LW_CFG_REGION_EN > 0) &&   */
                                                                        /*  (LW_CFG_MAX_REGIONS > 0)    */
/*********************************************************************************************************
  END
*********************************************************************************************************/
