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
** ��   ��   ��: RegionGet.c
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2007 �� 01 �� 22 ��
**
** ��        ��: ���һ���ڴ滺����

** BUG
2008.01.13  ���� _DebugHandle() ���ܡ�
2008.01.24  �������͵��ڴ����.
2008.02.05  ��ǰ�˽�����������ʱ��.
2008.05.18  ʹ�� __KERNEL_ENTER() ���� ThreadLock();
2011.03.05  �����ڴ�ʱ�����˸��ٺ�������Ϣ.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
/*********************************************************************************************************
** ��������: API_RegionGet
** ��������: ���һ���ڴ滺����
** �䡡��  : 
**           ulId                         REGION ���
**           stByteSize                   �ֶδ�С
** �䡡��  : 
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
                                           
                                       (�������ж��е���)
*********************************************************************************************************/
#if (LW_CFG_REGION_EN > 0) && (LW_CFG_MAX_REGIONS > 0)

LW_API  
PVOID  API_RegionGet (LW_OBJECT_HANDLE  ulId, size_t  stByteSize)
{
    REGISTER PLW_CLASS_HEAP            pheap;
    REGISTER UINT16                    usIndex;
    REGISTER PVOID                     pvAllocate;
    
    usIndex = _ObjectGetIndex(ulId);
    
    if (LW_CPU_GET_CUR_NESTING()) {                                     /*  �������ж��е���            */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "called from ISR.\r\n");
        _ErrorHandle(ERROR_KERNEL_IN_ISR);
        return  (LW_NULL);
    }
    
#if LW_CFG_ARG_CHK_EN > 0
    if (!stByteSize) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "ulByteSize invalidate\r\n");
        _ErrorHandle(ERROR_REGION_SIZE);
        return  (LW_NULL);
    }
    
    if (!_ObjectClassOK(ulId, _OBJECT_REGION)) {                        /*  �������ͼ��                */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "region handle invalidate.\r\n");
        _ErrorHandle(ERROR_KERNEL_HANDLE_NULL);
        return  (LW_NULL);
    }
    
    __KERNEL_ENTER();                                                   /*  �����ں�                    */
    if (_Heap_Index_Invalid(usIndex)) {                                 /*  �������������              */
        __KERNEL_EXIT();                                                /*  �˳��ں�                    */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "region handle invalidate.\r\n");
        _ErrorHandle(ERROR_KERNEL_HANDLE_NULL);
        return  (LW_NULL);
    }
#else
    __KERNEL_ENTER();                                                   /*  �����ں�                    */
#endif

    pheap = &_K_heapBuffer[usIndex];

    __KERNEL_EXIT();                                                    /*  �˳��ں�                    */

    pvAllocate = _HeapAllocate(pheap, stByteSize, __func__);
    
    if (!pvAllocate) {                                                  /*  �Ƿ����ɹ�                */
        _ErrorHandle(ERROR_REGION_NOMEM);
    }
    
    return  (pvAllocate);
}

#endif                                                                  /*  (LW_CFG_REGION_EN > 0) &&   */
                                                                        /*  (LW_CFG_MAX_REGIONS > 0)    */
/*********************************************************************************************************
  END
*********************************************************************************************************/
