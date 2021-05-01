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
** ��   ��   ��: RegionDelete.c
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2007 �� 01 �� 17 ��
**
** ��        ��: ɾ��һ���ڴ����

** BUG
2008.01.13  ���� _DebugHandle() ���ܡ�
2008.02.05  ʹ���µ�_HeapDelete()��ɾ���ѿ��ƿ�.
2008.05.18  ʹ�� __KERNEL_ENTER() ���� ThreadLock();
2011.07.18  ���� log ��ӡ����.
2011.07.29  ������󴴽�/���ٻص�.
2012.12.06  ����ǿ��ɾ������.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
/*********************************************************************************************************
** ��������: API_RegionDeleteEx
** ��������: ɾ��һ���ڴ����
** �䡡��  : 
**           pulId                         REGION ���ָ��
**           bForce                        ǿ��ɾ��
** �䡡��  : 
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
                                           
                                       (�������ж��е���)
*********************************************************************************************************/
#if (LW_CFG_REGION_EN > 0) && (LW_CFG_MAX_REGIONS > 0)

LW_API  
ULONG  API_RegionDeleteEx (LW_OBJECT_HANDLE   *pulId, BOOL  bForce)
{
    REGISTER PLW_CLASS_HEAP            pheap;
    REGISTER UINT16                    usIndex;
    REGISTER LW_OBJECT_HANDLE          ulId;
    
    ulId = *pulId;
    
    usIndex = _ObjectGetIndex(ulId);
    
    if (LW_CPU_GET_CUR_NESTING()) {                                     /*  �������ж��е���            */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "called from ISR.\r\n");
        _ErrorHandle(ERROR_KERNEL_IN_ISR);
        return  (ERROR_KERNEL_IN_ISR);
    }
    
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
    
    __KERNEL_EXIT();                                                    /*  �˳��ں�                    */
    if (_HeapDelete(pheap, !bForce)) {                                  /*  �ͷŶѿ��ƿ�                */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "region in use.\r\n");
        _ErrorHandle(ERROR_REGION_USED);
        return  (ERROR_REGION_USED);
    }
    
    _ObjectCloseId(pulId);                                              /*  ������                    */
    
    __LW_OBJECT_DELETE_HOOK(ulId);
    
    _DebugFormat(__LOGMESSAGE_LEVEL, "region \"%s\" has been delete.\r\n", pheap->HEAP_cHeapName);
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: API_RegionDelete
** ��������: ɾ��һ���ڴ����
** �䡡��  : 
**           pulId                         REGION ���ָ��
** �䡡��  : 
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
                                           
                                       (�������ж��е���)
*********************************************************************************************************/
LW_API  
ULONG  API_RegionDelete (LW_OBJECT_HANDLE   *pulId)
{
    return  (API_RegionDeleteEx(pulId, LW_FALSE));
}

#endif                                                                  /*  (LW_CFG_REGION_EN > 0) &&   */
                                                                        /*  (LW_CFG_MAX_REGIONS > 0)    */
/*********************************************************************************************************
  END
*********************************************************************************************************/
