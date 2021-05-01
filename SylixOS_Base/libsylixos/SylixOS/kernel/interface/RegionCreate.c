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
** ��   ��   ��: RegionCreate.c
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2007 �� 01 �� 11 ��
**
** ��        ��: ����һ���ڴ�ɱ����

** BUG
2007.08.29  ����ԷǶ����ַ�ļ�鹦�ܡ�
2008.01.13  ���� _DebugHandle() ����.
2011.07.29  ������󴴽�/���ٻص�.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
/*********************************************************************************************************
** ��������: API_RegionCreate
** ��������: ����һ���ڴ�ɱ����
** �䡡��  : 
**           pcName                        ����
**           pvLowAddr                     �ڴ������ʼ��ַ
**           stRegionByteSize              �ڴ������С
**           ulOption                      ѡ��
**           pulId                         Id ��
** �䡡��  : 
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
                                           
                                       (�������ж��е���)
*********************************************************************************************************/
#if (LW_CFG_REGION_EN > 0) && (LW_CFG_MAX_REGIONS > 0)

LW_API  
LW_OBJECT_HANDLE API_RegionCreate (CPCHAR             pcName,
                                   PVOID              pvLowAddr,
                                   size_t             stRegionByteSize,
                                   ULONG              ulOption,
                                   LW_OBJECT_ID      *pulId)
{
    REGISTER PLW_CLASS_HEAP     pheap;
    REGISTER ULONG              ulIdTemp;
    
    if (LW_CPU_GET_CUR_NESTING()) {                                     /*  �������ж��е���            */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "called from ISR.\r\n");
        _ErrorHandle(ERROR_KERNEL_IN_ISR);
        return  (LW_OBJECT_HANDLE_INVALID);
    }
    
#if LW_CFG_ARG_CHK_EN > 0
    if (!pvLowAddr) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "pvLowAddr invalidate.\r\n");
        _ErrorHandle(ERROR_REGION_NULL);
        return  (LW_OBJECT_HANDLE_INVALID);
    }
    if (!_Addresses_Is_Aligned(pvLowAddr)) {                            /*  ����ַ�Ƿ����            */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "pvLowAddr is not aligned.\r\n");
        _ErrorHandle(ERROR_KERNEL_MEMORY);
        return  (LW_OBJECT_HANDLE_INVALID);
    }
    if (_Heap_ByteSize_Invalid(stRegionByteSize)) {                     /*  �ֶ�̫С                    */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "ulRegionByteSize is too low.\r\n");
        _ErrorHandle(ERROR_REGION_SIZE);
        return  (LW_OBJECT_HANDLE_INVALID);
    }
#endif

    if (_Object_Name_Invalid(pcName)) {                                 /*  ���������Ч��              */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "name too long.\r\n");
        _ErrorHandle(ERROR_KERNEL_PNAME_TOO_LONG);
        return  (LW_OBJECT_HANDLE_INVALID);
    }
    
    pheap = _HeapCreate(pvLowAddr, stRegionByteSize);
    
    if (!pheap) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "there is no ID to build a region.\r\n");
        _ErrorHandle(ERROR_REGION_NULL);
        return  (LW_OBJECT_HANDLE_INVALID);
    }
    
    if (pcName) {                                                       /*  ��������                    */
        lib_strcpy(pheap->HEAP_cHeapName, pcName);
    } else {
        pheap->HEAP_cHeapName[0] = PX_EOS;                              /*  �������                    */
    }
    
    ulIdTemp = _MakeObjectId(_OBJECT_REGION, 
                             LW_CFG_PROCESSOR_NUMBER, 
                             pheap->HEAP_usIndex);                      /*  �������� id                 */
    
    if (pulId) {
        *pulId = ulIdTemp;
    }
    
    __LW_OBJECT_CREATE_HOOK(ulIdTemp, ulOption);
    
    _DebugFormat(__LOGMESSAGE_LEVEL, "region \"%s\" has been create.\r\n", (pcName ? pcName : ""));
    
    return  (ulIdTemp);
}

#endif                                                                  /*  (LW_CFG_REGION_EN > 0)      */
                                                                        /*  (LW_CFG_MAX_REGIONS > 0)    */
/*********************************************************************************************************
  END
*********************************************************************************************************/
