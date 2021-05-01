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
** ��   ��   ��: _HeapInit.c
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2007 �� 07 �� 21 ��
**
** ��        ��: ���ڴ��ʼ��

** BUG
2007.01.10  û�г�ʼ�� index
2007.07.21  ���� _DebugHandle() ��Ϣ���ܡ�
2007.11.07  ���û�������Ϊ�ں˶�.
2009.06.30  ������صĲü�����.
2009.07.28  ������������ĳ�ʼ��.
2009.11.23  ��������.
2009.12.11  ����ע��.
2013.11.14  ʹ�ö�����Դ�������ṹ���������Դ.
2017.12.01  ���� cdump ��ʼ��.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
/*********************************************************************************************************
  ʹ���Զ�������ʱ, �ں˶Ѻ�ϵͳ�ѵ��ڴ�.
*********************************************************************************************************/
#if LW_CFG_MEMORY_HEAP_CONFIG_TYPE == 0
#if LW_CFG_MEMORY_KERNEL_HEAP_ADDRESS == 0
static LW_STACK _K_stkKernelHeap[LW_CFG_MEMORY_KERNEL_HEAP_SIZE_BYTE / sizeof(LW_STACK)];
#endif
#if LW_CFG_MEMORY_SYSTEM_HEAP_ADDRESS == 0
static LW_STACK _K_stkSystemHeap[LW_CFG_MEMORY_SYSTEM_HEAP_SIZE_BYTE / sizeof(LW_STACK)];
#endif
#endif                                                                  /*  LW_CFG_MEMORY_HEAP_CONFIG_..*/
/*********************************************************************************************************
** ��������: _HeapInit
** ��������: ���ڴ��ʼ��
** �䡡��  : 
** �䡡��  : 
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
VOID  _HeapInit (VOID)
{
    REGISTER ULONG                  ulI;
    REGISTER PLW_LIST_MONO          pmonoTemp1;
    REGISTER PLW_LIST_MONO          pmonoTemp2;
    REGISTER PLW_CLASS_HEAP         heapTemp1;
    REGISTER PLW_CLASS_HEAP         heapTemp2;
    
    _K_resrcHeap.RESRC_pmonoFreeHeader = &_K_heapBuffer[0].HEAP_monoResrcList;
    
    heapTemp1 = &_K_heapBuffer[0];                                      /*  ָ�򻺳���׵�ַ            */
    heapTemp2 = &_K_heapBuffer[1];

    for (ulI = 0; ulI < (LW_CFG_MAX_REGIONS + 1); ulI++) {              /*  LW_CFG_MAX_REGIONS + 2 ��   */
        pmonoTemp1 = &heapTemp1->HEAP_monoResrcList;
        pmonoTemp2 = &heapTemp2->HEAP_monoResrcList;
        
        heapTemp1->HEAP_usIndex = (UINT16)ulI;
        LW_SPIN_INIT(&heapTemp1->HEAP_slLock);
        
        _LIST_MONO_LINK(pmonoTemp1, pmonoTemp2);
        
        heapTemp1++;
        heapTemp2++;
    }
    
    heapTemp1->HEAP_usIndex = (UINT16)ulI;
    LW_SPIN_INIT(&heapTemp1->HEAP_slLock);
    
    pmonoTemp1 = &heapTemp1->HEAP_monoResrcList;
    
    _INIT_LIST_MONO_HEAD(pmonoTemp1);
    
    _K_resrcHeap.RESRC_pmonoFreeTail = pmonoTemp1;
    
    _K_resrcHeap.RESRC_uiUsed    = 0;
    _K_resrcHeap.RESRC_uiMaxUsed = 0;
}
/*********************************************************************************************************
** ��������: _HeapKernelInit
** ��������: �ں˶��ڴ��ʼ��
** �䡡��  : pvKernelHeapMem   �ں˶ѵ���ʼ��ַ
**           stKernelHeapSize  �ں˶ѵĴ�С
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
#if LW_CFG_MEMORY_HEAP_CONFIG_TYPE > 0
VOID  _HeapKernelInit (PVOID     pvKernelHeapMem,
                       size_t    stKernelHeapSize)
#else
VOID  _HeapKernelInit (VOID)
#endif                                                                  /*  LW_CFG_MEMORY_HEAP_...      */
{
    PVOID   pvHeap;
    size_t  stSize;

#if LW_CFG_MEMORY_HEAP_CONFIG_TYPE > 0
    pvHeap = pvKernelHeapMem;
    stSize = stKernelHeapSize;
    
    _K_pheapKernel = _HeapCreate(LW_KERNEL_HEAP_START(pvKernelHeapMem), 
                                 LW_KERNEL_HEAP_SIZE(stKernelHeapSize));
#else
#if LW_CFG_MEMORY_KERNEL_HEAP_ADDRESS == 0
    pvHeap = (PVOID)_K_stkKernelHeap;
    stSize = LW_CFG_MEMORY_KERNEL_HEAP_SIZE_BYTE;

    _K_pheapKernel = _HeapCreate(LW_KERNEL_HEAP_START(_K_stkKernelHeap), 
                                 LW_KERNEL_HEAP_SIZE(LW_CFG_MEMORY_KERNEL_HEAP_SIZE_BYTE));
#else
    pvHeap = (PVOID)LW_CFG_MEMORY_KERNEL_HEAP_ADDRESS;
    stSize = LW_CFG_MEMORY_KERNEL_HEAP_SIZE_BYTE;

    _K_pheapKernel = _HeapCreate(LW_KERNEL_HEAP_START(LW_CFG_MEMORY_KERNEL_HEAP_ADDRESS), 
                                 LW_KERNEL_HEAP_SIZE(LW_CFG_MEMORY_KERNEL_HEAP_SIZE_BYTE));
#endif
#endif                                                                  /*  LW_CFG_MEMORY_HEAP_...      */

#if (LW_CFG_CDUMP_EN > 0) && (LW_CFG_DEVICE_EN > 0)
    _CrashDumpSet(LW_KERNEL_CDUMP_START(pvHeap, stSize), LW_CFG_CDUMP_BUF_SIZE);
#endif                                                                  /*  LW_CFG_CDUMP_EN > 0         */
                                                                        /*  LW_CFG_DEVICE_EN > 0        */
#if (LW_CFG_DEVICE_EN > 0) && (LW_CFG_FIO_LIB_EN > 0)
#if LW_CFG_ERRORMESSAGE_EN > 0 || LW_CFG_LOGMESSAGE_EN > 0
#if LW_CFG_MEMORY_HEAP_CONFIG_TYPE > 0
    _DebugFormat(__LOGMESSAGE_LEVEL, "kernel heap has been create 0x%lx (%zd Bytes).\r\n",
                 (addr_t)pvKernelHeapMem, stKernelHeapSize);
#else
    _DebugFormat(__LOGMESSAGE_LEVEL, "kernel heap has been create 0x%lx (%zd Bytes).\r\n",
                 (LW_CFG_MEMORY_KERNEL_HEAP_ADDRESS == 0) ? 
                 (addr_t)_K_stkKernelHeap : (addr_t)LW_CFG_MEMORY_KERNEL_HEAP_ADDRESS,
                 (size_t)LW_CFG_MEMORY_KERNEL_HEAP_SIZE_BYTE);
#endif                                                                  /*  LW_CFG_MEMORY_HEAP_...      */
#endif                                                                  /*  LW_CFG_ERRORMESSAGE_EN...   */
#endif                                                                  /*  LW_CFG_DEVICE_EN...         */

    lib_strcpy(_K_pheapKernel->HEAP_cHeapName, "kernel");
}
/*********************************************************************************************************
** ��������: _HeapSystemInit
** ��������: ϵͳ���ڴ��ʼ��
** �䡡��  : pvSystemHeapMem   ϵͳ�ѵ���ʼ��ַ
**           stSystemHeapSize  ϵͳ�ѵĴ�С
** �䡡��  : NONE
** �䡡��  : 
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
#if LW_CFG_MEMORY_HEAP_CONFIG_TYPE > 0
VOID  _HeapSystemInit (PVOID     pvSystemHeapMem,
                       size_t    stSystemHeapSize)
#else
VOID  _HeapSystemInit (VOID)
#endif                                                                  /*  LW_CFG_MEMORY_HEAP_...      */
{
#if LW_CFG_MEMORY_HEAP_CONFIG_TYPE > 0
    if (pvSystemHeapMem && stSystemHeapSize) {
        _K_pheapSystem = _HeapCreate(pvSystemHeapMem, stSystemHeapSize);
    } else
#else
#if LW_CFG_MEMORY_SYSTEM_HEAP_ADDRESS == 0
    if (LW_CFG_MEMORY_SYSTEM_HEAP_SIZE_BYTE) {
        _K_pheapSystem = _HeapCreate((PVOID)_K_stkSystemHeap, LW_CFG_MEMORY_SYSTEM_HEAP_SIZE_BYTE);
    } else
#else
    if (LW_CFG_MEMORY_SYSTEM_HEAP_SIZE_BYTE) {
        _K_pheapSystem = _HeapCreate((PVOID)LW_CFG_MEMORY_SYSTEM_HEAP_ADDRESS, 
                                     LW_CFG_MEMORY_SYSTEM_HEAP_SIZE_BYTE);
    } else
#endif
#endif                                                                  /*  LW_CFG_MEMORY_HEAP_...      */
    {
        _K_pheapSystem = _K_pheapKernel;                                /*  ֻʹ�� kernel heap          */
    }

#if (LW_CFG_DEVICE_EN > 0) && (LW_CFG_FIO_LIB_EN > 0)
#if (LW_CFG_ERRORMESSAGE_EN > 0) || (LW_CFG_LOGMESSAGE_EN > 0)
#if LW_CFG_MEMORY_HEAP_CONFIG_TYPE > 0
    _DebugFormat(__LOGMESSAGE_LEVEL, "system heap has been create 0x%lx (%zd Bytes).\r\n",
                 (addr_t)pvSystemHeapMem, stSystemHeapSize);
#else
    _DebugFormat(__LOGMESSAGE_LEVEL, "system heap has been create 0x%lx (%zd Bytes).\r\n",
                 (LW_CFG_MEMORY_SYSTEM_HEAP_ADDRESS == 0) ? 
                 (addr_t)_K_stkSystemHeap : (addr_t)LW_CFG_MEMORY_SYSTEM_HEAP_ADDRESS,
                 (size_t)LW_CFG_MEMORY_SYSTEM_HEAP_SIZE_BYTE);
#endif                                                                  /*  LW_CFG_MEMORY_HEAP_...      */
#endif                                                                  /*  LW_CFG_ERRORMESSAGE_EN...   */
#endif                                                                  /*  LW_CFG_DEVICE_EN...         */

    if (_K_pheapSystem == _K_pheapKernel) {
        lib_strcpy(_K_pheapSystem->HEAP_cHeapName, "kersys");
    
    } else {
        lib_strcpy(_K_pheapSystem->HEAP_cHeapName, "system");
    }
}
/*********************************************************************************************************
  END
*********************************************************************************************************/
