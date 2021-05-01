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
** ��   ��   ��: mosipc_heap.c
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2014 �� 10 �� 28 ��
**
** ��        ��: �����ϵͳͨ���ڴ��.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
#include "../SylixOS/system/include/s_system.h"
/*********************************************************************************************************
  �ü���
*********************************************************************************************************/
#if LW_CFG_MOSIPC_EN > 0
/*********************************************************************************************************
** ��������: __mosipcHeapInit
** ��������: ��ʼ�� MOSIPC �ڴ��.
** �䡡��  : pheapToBuild          ��Ҫ�����Ķ�
**           pvStartAddress        ��ʼ�ڴ��ַ
**           stByteSize            �ڴ�ѵĴ�С
** �䡡��  : ERROR
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
INT  __mosipcHeapInit (PLW_CLASS_HEAP    pheapToBuild,
                       PVOID             pvStartAddress, 
                       size_t            stByteSize)
{
    if (_HeapCtorEx(pheapToBuild, pvStartAddress, stByteSize, LW_TRUE)) {
        return  (ERROR_NONE);
    
    } else {
        return  (PX_ERROR);
    }
    
    lib_strlcpy(pheapToBuild->HEAP_cHeapName, "mosipc_heap", LW_CFG_OBJECT_NAME_SIZE);
}
/*********************************************************************************************************
** ��������: __mosipcHeapAlloc
** ��������: �� MOSIPC �ڴ�ѷ����ڴ�.
** �䡡��  : pheap              �ڴ��
**           stByteSize         ������ֽ���
**           stAlign            �ڴ�����ϵ
**           pcPurpose          �����ڴ����;
** �䡡��  : ������ڴ�
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
PVOID  __mosipcHeapAlloc (PLW_CLASS_HEAP    pheap,
                          size_t            stByteSize, 
                          size_t            stAlign, 
                          CPCHAR            pcPurpose)
{
    return  (_HeapAllocateAlign(pheap, stByteSize, stAlign, pcPurpose));
}
/*********************************************************************************************************
** ��������: __mosipcHeapFree
** ��������: �� MOSIPC �ڴ���ͷ��ڴ�.
** �䡡��  : pheap              �ѿ��ƿ�
**           pvStartAddress     �黹�ĵ�ַ
**           pcPurpose          ˭�ͷ��ڴ�
** �䡡��  : ERROR
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
INT  __mosipcHeapFree (PLW_CLASS_HEAP    pheap,
                       PVOID             pvStartAddress,
                       CPCHAR            pcPurpose)
{
    if (_HeapFree(pheap, pvStartAddress, LW_FALSE, pcPurpose)) {
        return  (PX_ERROR);
    
    } else {
        return  (ERROR_NONE);
    }
}

#endif                                                                  /*  LW_CFG_MOSIPC_EN > 0        */
/*********************************************************************************************************
  END
*********************************************************************************************************/
