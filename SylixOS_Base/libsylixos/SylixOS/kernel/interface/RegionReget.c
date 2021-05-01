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
** ��   ��   ��: RegionReget.c
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2008 �� 02 �� 05 ��
**
** ��        ��: ���»��һ���ڴ滺����, ������ realloc() ����

** BUG
2008.05.18  ʹ�� __KERNEL_ENTER() ���� ThreadLock();
2008.09.05  ��ȫ���� realloc �淶.
2011.03.05  �����ڴ�ʱ�����˸��ٺ�������Ϣ.
2011.07.19  �Բ������ж���ȫ���� _HeapRealloc() ����.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
/*********************************************************************************************************
** ��������: API_RegionReget
** ��������: ���»��һ���ڴ滺����
** �䡡��  : 
**           ulId                         REGION ���
**           pvOldMem                     ԭ�ȷ�����ڴ�
**           stNewByteSize                �µķֶδ�С
** �䡡��  : 
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
                                           
                                       (�������ж��е���)
*********************************************************************************************************/
#if (LW_CFG_REGION_EN > 0) && (LW_CFG_MAX_REGIONS > 0)

LW_API  
PVOID  API_RegionReget (LW_OBJECT_HANDLE    ulId, 
                        PVOID               pvOldMem, 
                        size_t              stNewByteSize)
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
    
    pvAllocate = _HeapRealloc(pheap, pvOldMem, stNewByteSize, LW_TRUE, __func__);
    
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
