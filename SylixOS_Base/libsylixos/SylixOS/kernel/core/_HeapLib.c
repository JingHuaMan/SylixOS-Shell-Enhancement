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
** ��   ��   ��: _HeapLib.c
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2007 �� 07 �� 25 ��
**
** ��        ��: ���ڴ������

** BUG
2007.04.09  LINE 291 if (bIsMergeOk == LW_TRUE) {...} ��Ϊ��if (bIsMergeOk) {...}
2007.06.04  LINE 184 , 185 �����С��Ӧ���������ֶδ��С��
2007.07.25  ���� _Heap_Verify() �ڽ����ڴ�ʱ��ѡ���ԵĽ����ڴ��飬KHEAP �� SHEAP ����Ҫ��������������Ҫ
2007.11.04  ������ PCHAR ���͸�Ϊ UINT8 * ����.
2007.11.13  ʹ���������������������ȫ��װ.
2007.11.18  ����ע��.
2008.01.17  ��ȫ���д������ع�, ���ҶԶѵĻ�������˸����ԵĸĽ�.
2008.05.18  ʹ�� __KERNEL_ENTER() ���� ThreadLock();
2008.05.31  ʹ�� __KERNEL_MODE_...().
2008.06.17  �������ŵ�ָ�������ڴ��������ĺ���.
2009.03.16  HEAP ���ƿ��� HEAP_bIsSemLock û�д��ڵı�Ҫ, LW_CFG_SEMB_EN -> LW_CFG_SEMM_EN.
            REALLOC ���Ƶ����ݳ��ȴ���. ֻ�е�����ɹ�ʱ�ſ����ͷ�ԭ�е��ڴ�.
2009.03.17  �����ڴ����ʱ, ��ߵķֶ��޷��ٷ�ʱ, ���صĵ�ַ���������ֶε��������׵�ַ, ��ʱ, �ڷ��ص�ַ��
            ��һ���ռ��¼�ֶ��ײ���ַ, Ϊ free ʱ�ṩ���ٵ��ж��ֶ�.
2009.04.08  ���� SMP ���֧��.
2009.05.21  ������������ͳ�ƵĹ���.
2009.07.03  �ܹ� GCC һ����ν�ľ���.
2009.07.14  ���� TRACE ����ѡ���ӡ������ڴ��ַ. (�ڴ�й¶����ʹ��)
2009.07.28  �������ĳ�ʼ�����ڳ�ʼ�����еĿ��ƿ���, ����ȥ����ز���.
2009.11.23  ���������� 3 ������ָ�����ڸ����ڴ�й¶.
2009.11.24  free �ڴ�ʱ, ���û����ָ�����ڴ����, ��Ҫ _DebugHandle() ����.
2010.07.10  ���� realloc ע��.
2010.08.19  _HeapRealloc() ֧�ִ��� alloc_align ���ص��ڴ�.
            ��ǿ���ڴ���ٽӿ�.
2011.03.05  Ϊ����ǿ�ڴ���ٹ���, ����������ڴ������;˵��.
2011.07.15  ���ڴ�ѹ���, �����봴��, ɾ������, �������Բ���Ҫ���ں˷�����ƿ�Ϳ��Դ���һ���ڲ�ʹ�õĶ�.
2011.07.19  realloc() ����Ը���������ж�.
2011.07.24  ���� _HeapZallocate() ����.
2011.08.07  _HeapFree() �ͷ� NULL ʱֱ���˳�, ��Ϊ linux kfree() �ж� null �Ĵ����ж�.
2011.11.03  heap ���зֶ�ʹ�û����������, ����ʹ�� free ���ڴ�, �������½��ķֶ�.
            �˷�����Ҫ��Ϊ����Ͻ�������ռ��ʹ�õ�ȱҳ�жϻ���.
2012.11.09  _HeapRealloc() �ж������ָ�������ϵ���ڴ��������֧��, û��ʵ������.
2013.02.22  ���ٷֶι����ڴ濪��.
2013.05.01  �����ڴ�Խ����.
2013.10.09  ����һЩ�����ӡ��Ϣ������.
2014.05.03  �ڴ����ʱ, ��ӡ�ѵ�����.
2014.07.03  ���Ը� heap ������ڴ�.
            free ����ȷ�����Ҳ�ֶ�Ӧ���� freelist �����, Ϊ��Ƽ�����Ķ�.
2014.08.15  �����µ��ж϶����ڴ��ͷ��㷨.
2014.10.27  ��������ϵͳͨ�Žӿ�.
2016.04.13  ���� _HeapGetMax() ��ȡ����ڴ���жδ�С.
2016.12.20  ������ heap ����ڴ��ɾ�� heap ʱ, ʹ������жϴ���.
2017.06.23  �����ⲿ�ڴ���ٽӿ�.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
/*********************************************************************************************************
  TRACE
*********************************************************************************************************/
#if LW_CFG_SHELL_HEAP_TRACE_EN > 0

VOIDFUNCPTR _K_pfuncHeapTraceAlloc;
VOIDFUNCPTR _K_pfuncHeapTraceFree;

#define __HEAP_TRACE_ALLOC(pheap, pvAddr, stLen, pcPurpose)   do {          \
            if (_K_pfuncHeapTraceAlloc) {                                   \
                _K_pfuncHeapTraceAlloc(pheap, pvAddr, stLen, pcPurpose);    \
            }                                                               \
        } while (0)
#define __HEAP_TRACE_ALLOC_ALIGN(pheap, pvAddr, stLen, pcPurpose) do {      \
            if (_K_pfuncHeapTraceAlloc) {                                   \
                _K_pfuncHeapTraceAlloc(pheap, pvAddr, stLen, pcPurpose);    \
            }                                                               \
        } while (0)
#define __HEAP_TRACE_FREE(pheap, pvAddr)    do {                            \
            if (_K_pfuncHeapTraceFree) {                                    \
                _K_pfuncHeapTraceFree(pheap, pvAddr);                       \
            }                                                               \
        } while (0)
#else
#define __HEAP_TRACE_ALLOC(pheap, pvAddr, stLen, pcPurpose)
#define __HEAP_TRACE_ALLOC_ALIGN(pheap, pvAddr, stLen, pcPurpose)
#define __HEAP_TRACE_FREE(pheap, pvAddr)
#endif                                                                  /*  LW_CFG_SHELL_HEAP_TRACE_EN  */  
/*********************************************************************************************************
  ���ź��������ѡ��
*********************************************************************************************************/
#define __HEAP_LOCK_OPT     (LW_OPTION_WAIT_PRIORITY | LW_OPTION_INHERIT_PRIORITY | \
                             LW_OPTION_DELETE_SAFE | LW_OPTION_OBJECT_DEBUG_UNPEND | \
                             LW_OPTION_OBJECT_GLOBAL)
/*********************************************************************************************************
  ���ʹ����ͳ��
*********************************************************************************************************/
#define __HEAP_UPDATA_MAX_USED(pheap)       do {                                        \
            if ((pheap)->HEAP_stMaxUsedByteSize < (pheap)->HEAP_stUsedByteSize) {       \
                (pheap)->HEAP_stMaxUsedByteSize = (pheap)->HEAP_stUsedByteSize;         \
            }                                                                           \
        } while (0)
/*********************************************************************************************************
  allocate �·ֶμ���������� 
  ����µķֶ��Ǹߵ�ַ, ������������������, ʹ��������Ҫ��ʹ��, �������ʹ����ȱҳ�������, 
  ������Լ����ҳ���ʹ����.
*********************************************************************************************************/
#define __HEAP_ADD_NEW_SEG_TO_FREELIST(psegment, pheap)    do {                         \
            if (_list_line_get_next(&psegment->SEGMENT_lineManage)) {                   \
                _List_Ring_Add_Ahead(&psegment->SEGMENT_ringFreeList,                   \
                                     &pheap->HEAP_pringFreeSegment);                    \
            } else {                                                                    \
                _List_Ring_Add_Last(&psegment->SEGMENT_ringFreeList,                    \
                                    &pheap->HEAP_pringFreeSegment);                     \
            }                                                                           \
        } while (0)
/*********************************************************************************************************
  �ֶ�ʹ�ñ�־ (���ڿ���������ʱ, �ֶ�Ϊ���зֶ�, ���ڿ���������, ��Ϊ����ʹ�õķֶ�)
*********************************************************************************************************/
#define __HEAP_SEGMENT_IS_USED(psegment)    (!_list_ring_get_prev(&((psegment)->SEGMENT_ringFreeList)))
/*********************************************************************************************************
  �ֶ��Ƿ���Ч
*********************************************************************************************************/
#define __HEAP_SEGMENT_IS_REAL(psegment)    ((psegment)->SEGMENT_stMagic == LW_SEG_MAGIC_REAL)
/*********************************************************************************************************
  �ֶ�����ָ��
*********************************************************************************************************/
#define __HEAP_SEGMENT_DATA_PTR(psegment)   ((UINT8 *)(psegment) + __SEGMENT_BLOCK_SIZE_ALIGN)
/*********************************************************************************************************
  ͨ������ָ�뷵�طֶ�ָ��
*********************************************************************************************************/
#define __HEAP_SEGMENT_SEG_PTR(pvData)      ((PLW_CLASS_SEGMENT)((UINT8 *)(pvData) - \
                                                                 __SEGMENT_BLOCK_SIZE_ALIGN))
/*********************************************************************************************************
  ��������һ���ֶε�ַ
*********************************************************************************************************/
#define __HEAP_SEGMENT_NEXT_PTR(psegment)   ((PLW_CLASS_SEGMENT)(__HEAP_SEGMENT_DATA_PTR(psegment) + \
                                                                 psegment->SEGMENT_stByteSize))
/*********************************************************************************************************
  �Ƿ���Ժϲ��ֶ��ж�
*********************************************************************************************************/
#define __HEAP_SEGMENT_CAN_MR(psegment, psegmentRight)  \
        (__HEAP_SEGMENT_NEXT_PTR(psegment) == psegmentRight)
#define __HEAP_SEGMENT_CAN_ML(psegment, psegmentLeft)   \
        (__HEAP_SEGMENT_NEXT_PTR(psegmentLeft) == psegment)
/*********************************************************************************************************
  ��������Ҫ�黹���ڴ治���ڴ��ʱ, ��Ҫ��ӡ������Ϣ
*********************************************************************************************************/
#define __DEBUG_MEM_ERROR(caller, heap, error, addr)  \
        _DebugFormat(__ERRORMESSAGE_LEVEL, "\'%s\' heap %s memory is %s, address %p.\r\n",  \
                     caller, heap, error, addr);
/*********************************************************************************************************
  ��Ҫ�ڴ�Խ����
*********************************************************************************************************/
#define __HEAP_SEGMENT_MARK_FLAG            0xA5A54321

static LW_INLINE size_t __heap_crossbord_size (size_t  stSize)
{
    if (_K_bHeapCrossBorderEn) {
        return  (stSize + sizeof(size_t));
    } else {
        return  (stSize);
    }
}

static LW_INLINE VOID __heap_crossbord_mark (PLW_CLASS_SEGMENT psegment)
{
    size_t  *pstMark;
    
    if (_K_bHeapCrossBorderEn) {
        pstMark = (size_t *)(__HEAP_SEGMENT_DATA_PTR(psegment)
                + (psegment->SEGMENT_stByteSize - sizeof(size_t)));
        *pstMark = __HEAP_SEGMENT_MARK_FLAG;
    }
}

static LW_INLINE BOOL __heap_crossbord_check (PLW_CLASS_SEGMENT psegment)
{
    size_t  *pstMark;
    
    if (_K_bHeapCrossBorderEn) {
        pstMark = (size_t *)(__HEAP_SEGMENT_DATA_PTR(psegment)
                + (psegment->SEGMENT_stByteSize - sizeof(size_t)));
        if (*pstMark != __HEAP_SEGMENT_MARK_FLAG) {
            return  (LW_FALSE);
        }
    }
    
    return  (LW_TRUE);
}
/*********************************************************************************************************
** ��������: __heap_lock
** ��������: ����һ���ڴ��
** �䡡��  : pheap                 �ڴ��
** �䡡��  : �������
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
#if (LW_CFG_SEMM_EN > 0) && (LW_CFG_MAX_EVENTS > 0)

static ULONG    __heap_lock (PLW_CLASS_HEAP  pheap)
{
    if (pheap->HEAP_ulLock) {
        return  (API_SemaphoreMPend(pheap->HEAP_ulLock,
                                    LW_OPTION_WAIT_INFINITE));
    } else {
        LW_SPIN_LOCK(&pheap->HEAP_slLock);                              /*  ����������������Դ          */
        return  (ERROR_NONE);
    }
}

#else 
static ULONG    __heap_lock (PLW_CLASS_HEAP  pheap)
{
    LW_SPIN_LOCK(&pheap->HEAP_slLock);                                  /*  ����������������Դ          */
    return  (ERROR_NONE);
}

#endif                                                                  /*  (LW_CFG_SEMM_EN > 0) &&     */
                                                                        /*  (LW_CFG_MAX_EVENTS > 0)     */
/*********************************************************************************************************
** ��������: __heap_unlock
** ��������: ����һ���ڴ��
** �䡡��  : pheap                 �ڴ��
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
#if (LW_CFG_SEMM_EN > 0) && (LW_CFG_MAX_EVENTS > 0)

static VOID    __heap_unlock (PLW_CLASS_HEAP  pheap)
{
    if (pheap->HEAP_ulLock) {
        API_SemaphoreMPost(pheap->HEAP_ulLock);
    } else {
        LW_SPIN_UNLOCK(&pheap->HEAP_slLock);                            /*  �ͷ�������                  */
    }
}

#else
static VOID    __heap_unlock (PLW_CLASS_HEAP  pheap)
{
    LW_SPIN_UNLOCK(&pheap->HEAP_slLock);                                /*  �ͷ�������                  */
}

#endif                                                                  /*  (LW_CFG_SEMM_EN > 0) &&     */
                                                                        /*  (LW_CFG_MAX_EVENTS > 0)     */
/*********************************************************************************************************
** ��������: _HeapTraceAlloc
** ��������: ��������ٺ���
** �䡡��  : pheap              �ѿ��ƿ�
**           pvMem              ����ָ��
**           stByteSize         ������ֽ���
**           pcPurpose          �����ڴ����;
** �䡡��  :
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
VOID  _HeapTraceAlloc (PLW_CLASS_HEAP  pheap, PVOID  pvMem, size_t  stByteSize, CPCHAR  cpcPurpose)
{
    __HEAP_TRACE_ALLOC(pheap, pvMem, stByteSize, cpcPurpose);
}
/*********************************************************************************************************
** ��������: _HeapTraceFree
** ��������: ���ͷŸ��ٺ���
** �䡡��  : pheap              �ѿ��ƿ�
**           pvMem              �ڴ�ָ��
** �䡡��  :
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
VOID  _HeapTraceFree (PLW_CLASS_HEAP  pheap, PVOID  pvMem)
{
    __HEAP_TRACE_FREE(pheap, pvMem);
}
/*********************************************************************************************************
** ��������: _HeapCtor
** ��������: ����һ���ڴ��
** �䡡��  : pheapToBuild          ��Ҫ�����Ķ�
**           pvStartAddress        ��ʼ�ڴ��ַ
**           stByteSize            �ڴ�ѵĴ�С
**           bIsMosHeap            �Ƿ�Ϊ�����ϵͳ�ڴ��
** �䡡��  : �����õ��ڴ�ѿ��ƿ�
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
PLW_CLASS_HEAP  _HeapCtorEx (PLW_CLASS_HEAP    pheapToBuild,
                             PVOID             pvStartAddress, 
                             size_t            stByteSize, 
                             BOOL              bIsMosHeap)
{
    REGISTER PLW_CLASS_SEGMENT  psegment;
    REGISTER addr_t             ulStart      = (addr_t)pvStartAddress;
    REGISTER addr_t             ulStartAlign = ROUND_UP(ulStart, LW_CFG_HEAP_ALIGNMENT);
    
    if (ulStartAlign > ulStart) {
        stByteSize -= (size_t)(ulStartAlign - ulStart);                 /*  ȥ��ǰ��Ĳ����볤��        */
    }
    
    stByteSize = ROUND_DOWN(stByteSize, LW_CFG_HEAP_ALIGNMENT);         /*  �ֶδ�С����                */
    psegment   = (PLW_CLASS_SEGMENT)ulStartAlign;                       /*  ��һ���ֶ���ʼ��ַ          */
    
    _LIST_LINE_INIT_IN_CODE(psegment->SEGMENT_lineManage);              /*  ��ʼ����һ���ֶ�            */
    _LIST_RING_INIT_IN_CODE(psegment->SEGMENT_ringFreeList);
    
    psegment->SEGMENT_stByteSize = stByteSize
                                 - __SEGMENT_BLOCK_SIZE_ALIGN;          /*  ��һ���ֶεĴ�С            */
    psegment->SEGMENT_stMagic    = LW_SEG_MAGIC_REAL;
                                         
    pheapToBuild->HEAP_pringFreeSegment  = LW_NULL;
    _List_Ring_Add_Ahead(&psegment->SEGMENT_ringFreeList, 
                         &pheapToBuild->HEAP_pringFreeSegment);         /*  ������б�                  */
    
    pheapToBuild->HEAP_pvStartAddress    = (PVOID)ulStartAlign;
    pheapToBuild->HEAP_ulSegmentCounter  = 1;                           /*  ��ǰ�ķֶ���                */
    
    pheapToBuild->HEAP_stTotalByteSize   = stByteSize;                  /*  �����ܴ�С                  */
    pheapToBuild->HEAP_stUsedByteSize    = __SEGMENT_BLOCK_SIZE_ALIGN;  /*  ʹ�õ��ֽ���                */
    pheapToBuild->HEAP_stFreeByteSize    = stByteSize
                                         - __SEGMENT_BLOCK_SIZE_ALIGN;  /*  ���е��ֽ���                */
    pheapToBuild->HEAP_stMaxUsedByteSize = pheapToBuild->HEAP_stUsedByteSize;

    if (bIsMosHeap) {
        pheapToBuild->HEAP_ulLock = LW_OBJECT_HANDLE_INVALID;
        
    } else {
#if (LW_CFG_SEMM_EN > 0) && (LW_CFG_MAX_EVENTS > 0)
        pheapToBuild->HEAP_ulLock = API_SemaphoreMCreate("heap_lock",
                                                         LW_PRIO_DEF_CEILING, 
                                                         __HEAP_LOCK_OPT, 
                                                         LW_NULL);      /*  ������                      */
#endif                                                                  /*  (LW_CFG_SEMM_EN > 0) &&     */
    }                                                                   /*  (LW_CFG_MAX_EVENTS > 0)     */
    
    MONITOR_EVT_LONG3(MONITOR_EVENT_ID_REGION, MONITOR_EVENT_REGION_CREATE,
                      pheapToBuild, pvStartAddress, stByteSize, LW_NULL);
    
    return  (pheapToBuild);
}
/*********************************************************************************************************
** ��������: _HeapCtor
** ��������: ����һ���ڴ��
** �䡡��  : pheapToBuild          ��Ҫ�����Ķ�
**           pvStartAddress        ��ʼ�ڴ��ַ
**           stByteSize            �ڴ�ѵĴ�С
** �䡡��  : �����õ��ڴ�ѿ��ƿ�
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
PLW_CLASS_HEAP  _HeapCtor (PLW_CLASS_HEAP    pheapToBuild,
                           PVOID             pvStartAddress, 
                           size_t            stByteSize)
{
    return  (_HeapCtorEx(pheapToBuild, pvStartAddress, stByteSize, LW_FALSE));
}
/*********************************************************************************************************
** ��������: _HeapCreate
** ��������: ����һ���ڴ��
** �䡡��  : pvStartAddress        ��ʼ�ڴ��ַ
**           stByteSize            �ڴ�ѵĴ�С
** �䡡��  : �����õ��ڴ�ѿ��ƿ�
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
PLW_CLASS_HEAP  _HeapCreate (PVOID             pvStartAddress, 
                             size_t            stByteSize)
{
    REGISTER PLW_CLASS_HEAP     pheapToBuild;

    __KERNEL_MODE_PROC(
        pheapToBuild = _Allocate_Heap_Object();                         /*  ������ƿ�                  */
    );
    
    if (!pheapToBuild) {                                                /*  ʧ��                        */
        return  (LW_NULL);
    }
    
    return  (_HeapCtor(pheapToBuild, pvStartAddress, stByteSize));      /*  �����ڴ��                  */
}
/*********************************************************************************************************
** ��������: _HeapDtor
** ��������: ����һ���ڴ��
** �䡡��  : pheap             �Ѿ������Ŀ��ƿ�
**           bIsCheckUsed      ��û���ͷ���ʱ, �Ƿ������˶�
** �䡡��  : ɾ���ɹ����� LW_NULL
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
PLW_CLASS_HEAP _HeapDtor (PLW_CLASS_HEAP  pheap, BOOL  bIsCheckUsed)
{
    REGISTER ULONG              ulLockErr;
    REGISTER PLW_LIST_LINE      plineTemp;
    REGISTER PLW_CLASS_SEGMENT  psegment;

    ulLockErr = __heap_lock(pheap);
    if (ulLockErr) {                                                    /*  ����������                  */
        return  (pheap);
    }
    if (bIsCheckUsed) {                                                 /*  �������Ƿ����            */
        psegment = (PLW_CLASS_SEGMENT)pheap->HEAP_pvStartAddress;
        for (plineTemp  = &psegment->SEGMENT_lineManage;
             plineTemp != LW_NULL;
             plineTemp  = _list_line_get_next(plineTemp)) {             /*  �����ֶ�                    */
            psegment = _LIST_ENTRY(plineTemp, LW_CLASS_SEGMENT, SEGMENT_lineManage);
            if (__HEAP_SEGMENT_IS_USED(psegment)) {
                __heap_unlock(pheap);
                return  (pheap);                                        /*  �������ڱ�ʹ��              */
            }
        }
    }
    pheap->HEAP_stTotalByteSize = 0;                                    /*  �������������©��          */
    pheap->HEAP_stFreeByteSize  = 0;                                    /*  ��ʣ����ֽ�������          */
                                                                        /*  �����󲻿����ڽ��з���      */
#if (LW_CFG_SEMM_EN > 0) && (LW_CFG_MAX_EVENTS > 0)
    if (pheap->HEAP_ulLock) {                                           /*  �Ƿ�������                */
        API_SemaphoreMDelete(&pheap->HEAP_ulLock);                      /*  ɾ����                      */
    } else 
#endif                                                                  /*  (LW_CFG_SEMM_EN > 0) &&     */
                                                                        /*  (LW_CFG_MAX_EVENTS > 0)     */
    {
        __heap_unlock(pheap);                                           /*  �� HEAP_bIsSemLock ȷ������ */
                                                                        /*  ���ֽ�����ʽ�����ش���      */
    }
    
    MONITOR_EVT_LONG1(MONITOR_EVENT_ID_REGION, MONITOR_EVENT_REGION_DELETE,
                      pheap, LW_NULL);
    
    return  (LW_NULL);
}
/*********************************************************************************************************
** ��������: _HeapDelete
** ��������: ɾ��һ���ڴ��
** �䡡��  : pheap             �Ѿ������Ŀ��ƿ�
**           bIsCheckUsed      �Ƿ���ʹ�����
** �䡡��  : ɾ���ɹ����� LW_NULL
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
PLW_CLASS_HEAP  _HeapDelete (PLW_CLASS_HEAP  pheap, BOOL  bIsCheckUsed)
{
    if (_HeapDtor(pheap, bIsCheckUsed) == LW_NULL) {                    /*  �����Ƿ����ڴ�              */
        __KERNEL_MODE_PROC(
            _Free_Heap_Object(pheap);                                   /*  �ͷſ��ƿ�                  */
        );
        return  (LW_NULL);
    
    } else {
        return  (pheap);
    }
}
/*********************************************************************************************************
** ��������: _HeapAddMemory
** ��������: ��һ����������ڴ�
** �䡡��  : pheap                 �ѿ��ƿ�
**           pvMemory              ��Ҫ��ӵ��ڴ� (���뱣֤����)
**           stSize                �ڴ��С (�������һ���ֶδ�С��Сֵ)
** �䡡��  : �Ƿ���ӳɹ�
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
ULONG  _HeapAddMemory (PLW_CLASS_HEAP  pheap, PVOID  pvMemory, size_t  stSize)
{
    REGISTER PLW_LIST_LINE      plineTemp;
    REGISTER PLW_CLASS_SEGMENT  psegmentLast;
    REGISTER PLW_CLASS_SEGMENT  psegmentNew;
    REGISTER ULONG              ulLockErr;
    REGISTER addr_t             ulStart      = (addr_t)pvMemory;
    REGISTER addr_t             ulStartAlign = ROUND_UP(ulStart, LW_CFG_HEAP_ALIGNMENT);
    REGISTER addr_t             ulEnd, ulSeg;
    
    if (ulStartAlign > ulStart) {
        stSize -= (size_t)(ulStartAlign - ulStart);                     /*  ȥ��ǰ��Ĳ����볤��        */
    }
    
    stSize = ROUND_DOWN(stSize, LW_CFG_HEAP_ALIGNMENT);                 /*  �ֶδ�С����                */
    ulEnd  = ulStartAlign + stSize - 1;

    ulLockErr = __heap_lock(pheap);
    if (ulLockErr) {                                                    /*  ����������                  */
        return  (ulLockErr);
    }
    
    psegmentLast = (PLW_CLASS_SEGMENT)pheap->HEAP_pvStartAddress;
    plineTemp    = &psegmentLast->SEGMENT_lineManage;
    
    do {
        if (_list_line_get_next(plineTemp) == LW_NULL) {
            break;
        }
                                                                        /*  �жϵ�ַ�Ƿ����ص�        */
        psegmentLast = _LIST_ENTRY(plineTemp, LW_CLASS_SEGMENT, SEGMENT_lineManage);
        ulSeg = (addr_t)__HEAP_SEGMENT_DATA_PTR(psegmentLast);
        if ((ulStartAlign >= ulSeg) && 
            (ulStartAlign < (ulSeg + psegmentLast->SEGMENT_stByteSize))) {
            __heap_unlock(pheap);
            return  (EFAULT);
        }
        if ((ulEnd >= ulSeg) && 
            (ulEnd < (ulSeg + psegmentLast->SEGMENT_stByteSize))) {
            __heap_unlock(pheap);
            return  (EFAULT);
        }
        
        plineTemp = _list_line_get_next(plineTemp);
    } while (1);
    
    psegmentLast = _LIST_ENTRY(plineTemp, LW_CLASS_SEGMENT, SEGMENT_lineManage);
    
    psegmentNew = (PLW_CLASS_SEGMENT)ulStartAlign;
    psegmentNew->SEGMENT_stByteSize = stSize - __SEGMENT_BLOCK_SIZE_ALIGN;
    psegmentNew->SEGMENT_stMagic    = LW_SEG_MAGIC_REAL;
    
    _List_Line_Add_Right(&psegmentNew->SEGMENT_lineManage,
                         &psegmentLast->SEGMENT_lineManage);
                         
    __HEAP_ADD_NEW_SEG_TO_FREELIST(psegmentNew, pheap);
    
    pheap->HEAP_ulSegmentCounter++;
    pheap->HEAP_stTotalByteSize += stSize;
    pheap->HEAP_stUsedByteSize  += __SEGMENT_BLOCK_SIZE_ALIGN;
    pheap->HEAP_stFreeByteSize  += psegmentNew->SEGMENT_stByteSize;
    
    __heap_unlock(pheap);
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: _HeapGetInfo
** ��������: ���һ���ѵ�״̬
** �䡡��  : pheap                 �ѿ��ƿ�
**           psegmentList[]        ����ֶ�ָ��ı��
**           iMaxCounter           ������������
** �䡡��  : SEGMENT ������
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
ULONG  _HeapGetInfo (PLW_CLASS_HEAP  pheap, PLW_CLASS_SEGMENT  psegmentList[], INT  iMaxCounter)
{
    REGISTER ULONG              ulI = 0;
    REGISTER PLW_LIST_LINE      plineSegmentList;
    REGISTER PLW_CLASS_SEGMENT  psegmentFrist;
    
    REGISTER ULONG              ulLockErr;
    
    psegmentFrist = (PLW_CLASS_SEGMENT)pheap->HEAP_pvStartAddress;
    
    ulLockErr = __heap_lock(pheap);
    if (ulLockErr) {                                                    /*  ����������                  */
        return  (0);
    }
    
    for (plineSegmentList   = &psegmentFrist->SEGMENT_lineManage;
         (plineSegmentList != LW_NULL) && (iMaxCounter > 0);
         plineSegmentList   = _list_line_get_next(plineSegmentList)) {  /*  ��ѯ���еķֶ�              */
         
         psegmentList[ulI++] = _LIST_ENTRY(plineSegmentList, 
                                           LW_CLASS_SEGMENT, 
                                           SEGMENT_lineManage);         /*  ��÷ֶο��ƿ�              */
         iMaxCounter--;
    }
    __heap_unlock(pheap);
    
    return  (ulI);
}
/*********************************************************************************************************
** ��������: _HeapGetMax
** ��������: �����������ڴ��С
** �䡡��  : pheap                 �ѿ��ƿ�
** �䡡��  : �������ڴ�ֶδ�С
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
size_t  _HeapGetMax (PLW_CLASS_HEAP  pheap)
{
             PLW_LIST_RING      pringFreeSegment;
    REGISTER PLW_CLASS_SEGMENT  psegment;
    REGISTER ULONG              ulLockErr;
             size_t             stMax = 0;
    
    ulLockErr = __heap_lock(pheap);
    if (ulLockErr) {                                                    /*  ����������                  */
        return  (0);
    }
    
    pringFreeSegment = pheap->HEAP_pringFreeSegment;
    if (pringFreeSegment) {
        do {
            psegment = _LIST_ENTRY(pringFreeSegment, 
                                   LW_CLASS_SEGMENT, 
                                   SEGMENT_ringFreeList);
            if (stMax < psegment->SEGMENT_stByteSize) {
                stMax = psegment->SEGMENT_stByteSize;
            }
            
            pringFreeSegment = _list_ring_get_next(pringFreeSegment);
        } while (pringFreeSegment != pheap->HEAP_pringFreeSegment);
    }
    __heap_unlock(pheap);
    
    return  (stMax);
}
/*********************************************************************************************************
** ��������: _HeapVerify
** ��������: ����Ǹ��ڴ��ַ�Ƿ���ָ���ڴ���в����ѱ�ռ��(����ʱ�Ѿ����뻤��״̬)
** �䡡��  : pheap                        �ѿ��ƿ�ָ��
**           pvStartAddress               �ڴ���ʼ��ַ
**           ppsegmentUsed                ��д���ڷֶο��ƿ���ʼ��ַ
**           pcCaller                     ��ӡ debug ��Ϣʱ�ĺ�����
** �䡡��  : ����Ƿ�ɹ�
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
BOOL    _HeapVerify (PLW_CLASS_HEAP     pheap, 
                     PVOID              pvStartAddress, 
                     PLW_CLASS_SEGMENT *ppsegmentUsed,
                     CPCHAR             pcCaller)
{
    REGISTER PLW_CLASS_SEGMENT  psegment;
    REGISTER PLW_LIST_LINE      plineSegment;
    
    if ((pvStartAddress < pheap->HEAP_pvStartAddress) ||
        (pvStartAddress >= (PVOID)((UINT8 *)pheap->HEAP_pvStartAddress
                         + pheap->HEAP_stTotalByteSize))) {
        __DEBUG_MEM_ERROR(pcCaller, pheap->HEAP_cHeapName, 
                          "not in this heap", pvStartAddress);
        return  (LW_FALSE);
    }
    
    psegment = (PLW_CLASS_SEGMENT)pheap->HEAP_pvStartAddress;           /*  ��õ�һ���ֶ�              */
    
    for (plineSegment = &psegment->SEGMENT_lineManage;
         ;
         plineSegment = _list_line_get_next(plineSegment)) {            /*  ɨ������ֶ�                */
         
        if (plineSegment == LW_NULL) {                                  /*  ɨ�����                    */
            __DEBUG_MEM_ERROR(pcCaller, pheap->HEAP_cHeapName, 
                              "in heap control block", pvStartAddress);
            return  (LW_FALSE);
        }
        
        psegment = _LIST_ENTRY(plineSegment,  LW_CLASS_SEGMENT, SEGMENT_lineManage);
        
        if ((pvStartAddress >  (PVOID)psegment) &&
            (pvStartAddress <= (PVOID)(__HEAP_SEGMENT_DATA_PTR(psegment)
                             + psegment->SEGMENT_stByteSize))) {        /*  �����ֶ�                    */
            
            if (__HEAP_SEGMENT_IS_USED(psegment)) {
                *ppsegmentUsed = psegment;                              /*  �ҵ��ֶ�                    */
                return  (LW_TRUE);
            } else {                                                    /*  �ֶ�ʹ�ñ�־����            */
                __DEBUG_MEM_ERROR(pcCaller, pheap->HEAP_cHeapName, 
                                  "not in used or double free", pvStartAddress);
                return  (LW_FALSE);
            }
        }
    }
    
    return  (LW_FALSE);                                                 /*  �������е�����              */
}
/*********************************************************************************************************
** ��������: _HeapAllocate
** ��������: �Ӷ��������ֽڳ� (�״���Ӧ�㷨)
** �䡡��  : pheap              �ѿ��ƿ�
**           stByteSize         ������ֽ���
**           pcPurpose          �����ڴ����;
** �䡡��  : ������ڴ��ַ
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
PVOID  _HeapAllocate (PLW_CLASS_HEAP  pheap, size_t  stByteSize, CPCHAR  pcPurpose)
{
    REGISTER size_t             stSize;
    REGISTER size_t             stNewSegSize;
    REGISTER PLW_CLASS_SEGMENT  psegment;
    REGISTER PLW_CLASS_SEGMENT  psegmentNew;
    
             PLW_LIST_LINE      plineHeader;
             PLW_LIST_RING      pringFreeSegment;
             
    REGISTER ULONG              ulLockErr;
    
    stByteSize = __heap_crossbord_size(stByteSize);                     /*  Խ�������                */
    
    stSize = ROUND_UP(stByteSize, LW_CFG_HEAP_SEG_MIN_SIZE);            /*  ���ҳ�����ڴ��С          */
    
    if (pheap->HEAP_stFreeByteSize < stSize) {                          /*  û�пռ�                    */
        return  (LW_NULL);
    }
    
    ulLockErr = __heap_lock(pheap);
    if (ulLockErr) {                                                    /*  ����������                  */
        return  (LW_NULL);
    }
    
    pringFreeSegment = pheap->HEAP_pringFreeSegment;                    /*  ��һ�����зֶ�              */
    if (pringFreeSegment == LW_NULL) {
        __heap_unlock(pheap);
        return  (LW_NULL);                                              /*  û�п��зֶ�                */
    }
    
    do {
        psegment = _LIST_ENTRY(pringFreeSegment, 
                               LW_CLASS_SEGMENT, 
                               SEGMENT_ringFreeList);
        if (psegment->SEGMENT_stByteSize >=  stSize) {
            break;                                                      /*  �ҵ��ֶ�                    */
        }
        
        pringFreeSegment = _list_ring_get_next(pringFreeSegment);
        
        if (pringFreeSegment == pheap->HEAP_pringFreeSegment) {         /*  �������, û�к��ʵĿ��зֶ�*/
            __heap_unlock(pheap);
            return  (LW_NULL);
        }
    } while (1);
    
    _List_Ring_Del(&psegment->SEGMENT_ringFreeList, 
                   &pheap->HEAP_pringFreeSegment);                      /*  ����ǰ�ֶδӿ��ж�����ɾ��  */
    
    if ((psegment->SEGMENT_stByteSize - stSize) > 
        (__SEGMENT_BLOCK_SIZE_ALIGN + LW_CFG_HEAP_SEG_MIN_SIZE)) {      /*  �Ƿ���Էֳ��¶�            */
                     
        stNewSegSize = psegment->SEGMENT_stByteSize - stSize;           /*  �����·ֶε��ܴ�С          */
        
        psegment->SEGMENT_stByteSize = stSize;                          /*  ����ȷ����ǰ�ֶδ�С        */
        
        psegmentNew = (PLW_CLASS_SEGMENT)(__HEAP_SEGMENT_DATA_PTR(psegment)
                    + stSize);                                          /*  ��д�·ֶε������Ϣ        */
        psegmentNew->SEGMENT_stByteSize = stNewSegSize
                                        - __SEGMENT_BLOCK_SIZE_ALIGN;
        psegmentNew->SEGMENT_stMagic    = LW_SEG_MAGIC_REAL;
        
        plineHeader = &psegment->SEGMENT_lineManage;
        _List_Line_Add_Tail(&psegmentNew->SEGMENT_lineManage,
                            &plineHeader);                              /*  ���·ֶ������ھ�����        */
                            
        __HEAP_ADD_NEW_SEG_TO_FREELIST(psegmentNew, pheap);             /*  ���·ֶ���������зֶ�����  */
                             
        pheap->HEAP_ulSegmentCounter++;                                 /*  ��д�ڴ�ѿ��ƿ�            */
        pheap->HEAP_stUsedByteSize += (stSize + __SEGMENT_BLOCK_SIZE_ALIGN);
        pheap->HEAP_stFreeByteSize -= (stSize + __SEGMENT_BLOCK_SIZE_ALIGN);
        
    } else {
        pheap->HEAP_stUsedByteSize += psegment->SEGMENT_stByteSize;
        pheap->HEAP_stFreeByteSize -= psegment->SEGMENT_stByteSize;
    }
    
    __HEAP_UPDATA_MAX_USED(pheap);                                      /*  ����ͳ�Ʊ���                */
    __heap_unlock(pheap);
    
    __HEAP_TRACE_ALLOC(pheap, 
                       (PVOID)__HEAP_SEGMENT_DATA_PTR(psegment),
                       psegment->SEGMENT_stByteSize,
                       pcPurpose);                                      /*  ��ӡ������Ϣ                */
    
    __heap_crossbord_mark(psegment);                                    /*  �����ڴ�Խ���־            */
    
    MONITOR_EVT_LONG4(MONITOR_EVENT_ID_REGION, MONITOR_EVENT_REGION_ALLOC,
                      pheap, __HEAP_SEGMENT_DATA_PTR(psegment), 
                      psegment->SEGMENT_stByteSize, sizeof(LW_STACK), pcPurpose);
    
    return  ((PVOID)__HEAP_SEGMENT_DATA_PTR(psegment));                 /*  ���ط�����ڴ��׵�ַ        */
}
/*********************************************************************************************************
** ��������: _HeapAllocateAlign
** ��������: �Ӷ��������ֽڳ� (�״���Ӧ�㷨) (ָ���ڴ�����ϵ)
** �䡡��  : pheap              �ѿ��ƿ�
**           stByteSize         ������ֽ���
**           stAlign            �ڴ�����ϵ
**           pcPurpose          �����ڴ����;
** �䡡��  : ������ڴ��ַ
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
PVOID  _HeapAllocateAlign (PLW_CLASS_HEAP  pheap, size_t  stByteSize, size_t  stAlign, CPCHAR  pcPurpose)
{
    REGISTER size_t             stSize;
    REGISTER size_t             stNewSegSize;
    REGISTER PLW_CLASS_SEGMENT  psegment;
    REGISTER PLW_CLASS_SEGMENT  psegmentNew;
             PLW_CLASS_SEGMENT  psegmentAlloc = LW_NULL;                /*  �ܹ� GCC һ����ν�ľ���     */
    
             UINT8             *pcAlign;
             addr_t             ulAlignMask = (addr_t)(stAlign - 1);
             size_t             stAddedSize;                            /*  ǰ�˲��븽�����ݴ�С        */
             BOOL               bLeftNewFree;                           /*  ��˵��ڴ��Ƿ���Կ����¶�  */
    
             PLW_LIST_LINE      plineHeader;
             PLW_LIST_RING      pringFreeSegment;
             
    REGISTER ULONG              ulLockErr;
    
    stByteSize = __heap_crossbord_size(stByteSize);                     /*  Խ�������                */
    
    stSize = ROUND_UP(stByteSize, LW_CFG_HEAP_SEG_MIN_SIZE);            /*  ���ҳ�����ڴ��С          */
    
    if (pheap->HEAP_stFreeByteSize < stSize) {                          /*  û�пռ�                    */
        return  (LW_NULL);
    }
    
    ulLockErr = __heap_lock(pheap);
    if (ulLockErr) {                                                    /*  ����������                  */
        return  (LW_NULL);
    }
    
    pringFreeSegment = pheap->HEAP_pringFreeSegment;                    /*  ��һ�����зֶ�              */
    if (pringFreeSegment == LW_NULL) {
        __heap_unlock(pheap);
        return  (LW_NULL);                                              /*  û�п��зֶ�                */
    }
    
    do {
        psegment = _LIST_ENTRY(pringFreeSegment, 
                               LW_CLASS_SEGMENT, 
                               SEGMENT_ringFreeList);
        
        if (((size_t)__HEAP_SEGMENT_DATA_PTR(psegment) & ulAlignMask) == 0) {
            pcAlign     = __HEAP_SEGMENT_DATA_PTR(psegment);            /*  ����������������          */
            stAddedSize = 0;
        } else {                                                        /*  ��ȡ��������������ڴ��    */
            pcAlign     = (UINT8 *)(((addr_t)__HEAP_SEGMENT_DATA_PTR(psegment) | ulAlignMask) + 1);
            stAddedSize = (size_t)(pcAlign - __HEAP_SEGMENT_DATA_PTR(psegment));
        }
        
        if (psegment->SEGMENT_stByteSize >= (stSize + stAddedSize)) {
            if (stAddedSize >= 
                (__SEGMENT_BLOCK_SIZE_ALIGN + LW_CFG_HEAP_SEG_MIN_SIZE)) {
                bLeftNewFree = LW_TRUE;                                 /*  ��˿��Կ����¶�            */
            } else {
                bLeftNewFree = LW_FALSE;
                stSize += stAddedSize;                                  /*  ʹ�������ֶ�                */
                psegmentAlloc = psegment;                               /*  ��¼�����ڴ�                */
            }
            break;                                                      /*  �ҵ��ֶ�                    */
        }
    
        pringFreeSegment = _list_ring_get_next(pringFreeSegment);
        
        if (pringFreeSegment == pheap->HEAP_pringFreeSegment) {         /*  �������, û�к��ʵĿ��зֶ�*/
            __heap_unlock(pheap);
            return  (LW_NULL);
        }
    } while (1);
    
    if (bLeftNewFree) {                                                 /*  ���Ƚ�����β��            */
        psegmentNew = (PLW_CLASS_SEGMENT)(__HEAP_SEGMENT_DATA_PTR(psegment)
                    + stAddedSize - __SEGMENT_BLOCK_SIZE_ALIGN);
        psegmentNew->SEGMENT_stByteSize = psegment->SEGMENT_stByteSize
                                        - stAddedSize;
        psegmentNew->SEGMENT_stMagic    = LW_SEG_MAGIC_REAL;
                                               
        plineHeader = &psegment->SEGMENT_lineManage;
        _List_Line_Add_Tail(&psegmentNew->SEGMENT_lineManage,
                            &plineHeader);                              /*  ���·ֶ������ھ�����        */
                            
        __HEAP_ADD_NEW_SEG_TO_FREELIST(psegmentNew, pheap);             /*  ���·ֶ���������зֶ�����  */
        
        psegment->SEGMENT_stByteSize = stAddedSize 
                                     - __SEGMENT_BLOCK_SIZE_ALIGN;
        
        psegment = psegmentNew;                                         /*  ȷ��������ķ���            */
        
        pheap->HEAP_ulSegmentCounter++;                                 /*  ��д�ڴ�ѿ��ƿ�            */
        pheap->HEAP_stUsedByteSize += __SEGMENT_BLOCK_SIZE_ALIGN;
        pheap->HEAP_stFreeByteSize -= __SEGMENT_BLOCK_SIZE_ALIGN;
    }
    
    _List_Ring_Del(&psegment->SEGMENT_ringFreeList, 
                   &pheap->HEAP_pringFreeSegment);                      /*  ����ǰ�ֶδӿ��ж�����ɾ��  */
    
    if ((psegment->SEGMENT_stByteSize - stSize) > 
        (__SEGMENT_BLOCK_SIZE_ALIGN + LW_CFG_HEAP_SEG_MIN_SIZE)) {      /*  �Ƿ���Էֳ��¶�            */
                     
        stNewSegSize = psegment->SEGMENT_stByteSize - stSize;           /*  �����·ֶε��ܴ�С          */
        
        psegment->SEGMENT_stByteSize = stSize;                          /*  ����ȷ����ǰ�ֶδ�С        */
        
        psegmentNew = (PLW_CLASS_SEGMENT)(__HEAP_SEGMENT_DATA_PTR(psegment)
                    + stSize);                                          /*  ��д�·ֶε������Ϣ        */
        psegmentNew->SEGMENT_stByteSize = stNewSegSize
                                        - __SEGMENT_BLOCK_SIZE_ALIGN;
        psegmentNew->SEGMENT_stMagic    = LW_SEG_MAGIC_REAL;
                                        
        plineHeader = &psegment->SEGMENT_lineManage;
        _List_Line_Add_Tail(&psegmentNew->SEGMENT_lineManage,
                            &plineHeader);                              /*  ���·ֶ������ھ�����        */
               
        __HEAP_ADD_NEW_SEG_TO_FREELIST(psegmentNew, pheap);             /*  ���·ֶ���������зֶ�����  */
                             
        pheap->HEAP_ulSegmentCounter++;                                 /*  ��д�ڴ�ѿ��ƿ�            */
        pheap->HEAP_stUsedByteSize += (stSize + __SEGMENT_BLOCK_SIZE_ALIGN);
        pheap->HEAP_stFreeByteSize -= (stSize + __SEGMENT_BLOCK_SIZE_ALIGN);
        
    } else {
        pheap->HEAP_stUsedByteSize += psegment->SEGMENT_stByteSize;
        pheap->HEAP_stFreeByteSize -= psegment->SEGMENT_stByteSize;
    }
    
    if ((bLeftNewFree == LW_FALSE) && stAddedSize) {                    /*  �����Щ�ڴ�û��ʹ��        */
        PLW_CLASS_SEGMENT    psegmentFake = __HEAP_SEGMENT_SEG_PTR(pcAlign);
        size_t              *pstLeft      = (((size_t *)pcAlign) - 1);
        *pstLeft = (size_t)psegmentAlloc;                               /*  ��¼�����ֶο��ƿ�λ��      */
        if (pstLeft != &psegmentFake->SEGMENT_stMagic) {
            psegmentFake->SEGMENT_stMagic = ~LW_SEG_MAGIC_REAL;         /*  ����ʶ�𱾷ֶ�ͷ            */
        }
    }
    
    __HEAP_UPDATA_MAX_USED(pheap);                                      /*  ����ͳ�Ʊ���                */
    __heap_unlock(pheap);
    
    __HEAP_TRACE_ALLOC_ALIGN(pheap, 
                             pcAlign, 
                             psegment->SEGMENT_stByteSize,
                             pcPurpose);                                /*  ��ӡ������Ϣ                */
    
    __heap_crossbord_mark(psegment);                                    /*  �����ڴ�Խ���־            */
    
    MONITOR_EVT_LONG4(MONITOR_EVENT_ID_REGION, MONITOR_EVENT_REGION_ALLOC,
                      pheap, pcAlign, psegment->SEGMENT_stByteSize, 
					  stAlign, pcPurpose);
    
    return  (pcAlign);                                                  /*  ���ط�����ڴ��׵�ַ        */
}
/*********************************************************************************************************
** ��������: _HeapZallocate
** ��������: �Ӷ��������ֽڳز����� (�״���Ӧ�㷨)
** �䡡��  : pheap              �ѿ��ƿ�
**           stByteSize         ������ֽ��� (max bytes is INT_MAX)
**           pcPurpose          �����ڴ����;
** �䡡��  : ������ڴ��ַ
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
PVOID  _HeapZallocate (PLW_CLASS_HEAP  pheap, size_t  stByteSize, CPCHAR  pcPurpose)
{
    PVOID   pvMem = _HeapAllocate(pheap, stByteSize, pcPurpose);
    
    if (pvMem) {
        lib_bzero(pvMem, stByteSize);
    }
    
    return  (pvMem);
}
/*********************************************************************************************************
** ��������: _HeapFree
** ��������: ������Ŀռ��ͷŻض�, ����һ�����ȱ�����! (�����ۺ��㷨)
** �䡡��  : 
**           pheap              �ѿ��ƿ�
**           pvStartAddress     �黹�ĵ�ַ
**           bIsNeedVerify      �Ƿ���Ҫ��ȫ�Լ��
**           pcPurpose          ˭�ͷ��ڴ�
** �䡡��  : ��ȷ���� LW_NULL ���򷵻� pvStartAddress
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
PVOID  _HeapFree (PLW_CLASS_HEAP  pheap, PVOID  pvStartAddress, BOOL  bIsNeedVerify, CPCHAR  pcPurpose)
{
    REGISTER BOOL               bIsMergeOk = LW_FALSE;
    REGISTER size_t             stSegmentByteSizeFree;
             PLW_CLASS_SEGMENT  psegment;
    REGISTER PLW_LIST_LINE      plineLeft;
    REGISTER PLW_LIST_LINE      plineRight;
    REGISTER PLW_CLASS_SEGMENT  psegmentLeft;                           /*  ��ֶ�                      */
    REGISTER PLW_CLASS_SEGMENT  psegmentRight;                          /*  �ҷֶ�                      */
    
             PLW_LIST_LINE      plineDummyHeader = LW_NULL;             /*  ���ڲ������ݵ�ͷ            */
             
    REGISTER BOOL               bVerifyOk;                              /*  �Ƿ���ɹ�                */
    REGISTER ULONG              ulLockErr;
    
    if (pvStartAddress == LW_NULL) {
        return  (pvStartAddress);
    }
    
    ulLockErr = __heap_lock(pheap);
    if (ulLockErr) {                                                    /*  ����������                  */
        return  (pvStartAddress);
    }
    if (bIsNeedVerify) {                                                /*  �Ƿ���Ҫ��ȫ�Լ��          */
        bVerifyOk = _HeapVerify(pheap, pvStartAddress, 
                                &psegment, pcPurpose);                  /*  ���                        */
        if (bVerifyOk == LW_FALSE) {
            __heap_unlock(pheap);
            return  (pvStartAddress);                                   /*  ������                    */
        }
    } else {                                                            /*  ����Ҫ��ȫ�Լ��            */
        psegment = __HEAP_SEGMENT_SEG_PTR(pvStartAddress);              /*  ���Ҷο��ƿ��ַ            */
        if (!__HEAP_SEGMENT_IS_REAL(psegment)) {                        /*  ���ƿ��ַ����ʵ            */
            psegment = (PLW_CLASS_SEGMENT)(*((size_t *)pvStartAddress - 1));
        }
        
        if (!__HEAP_SEGMENT_IS_REAL(psegment) ||
            !__HEAP_SEGMENT_IS_USED(psegment)) {                        /*  �ο��ƿ���Ч���Ѿ����ͷ�    */
            __heap_unlock(pheap);
            __DEBUG_MEM_ERROR(pcPurpose, pheap->HEAP_cHeapName, 
                              "not in used or double free", pvStartAddress);
            return  (pvStartAddress);
        }
    }
    
    if (__heap_crossbord_check(psegment) == LW_FALSE) {                 /*  �ڴ�Խ��                    */
        __DEBUG_MEM_ERROR(pcPurpose, pheap->HEAP_cHeapName, 
                          "cross-border", pvStartAddress);
    }
    
    __HEAP_TRACE_FREE(pheap, pvStartAddress);                           /*  ��ӡ������Ϣ                */
    
    MONITOR_EVT_LONG2(MONITOR_EVENT_ID_REGION, MONITOR_EVENT_REGION_FREE,
                      pheap, pvStartAddress, LW_NULL);
    
    plineLeft  = _list_line_get_prev(&psegment->SEGMENT_lineManage);    /*  ��ֶ�                      */
    plineRight = _list_line_get_next(&psegment->SEGMENT_lineManage);    /*  �ҷֶ�                      */
    
    if (plineLeft) {
        psegmentLeft  = _LIST_ENTRY(plineLeft,  
                                    LW_CLASS_SEGMENT, 
                                    SEGMENT_lineManage);                /*  ��ֶο��ƿ�                */
    } else {
        psegmentLeft  = LW_NULL;
    }
    if (plineRight) {
        psegmentRight = _LIST_ENTRY(plineRight, 
                                    LW_CLASS_SEGMENT, 
                                    SEGMENT_lineManage);                /*  �ҷֶο��ƿ�                */
    } else {
        psegmentRight = LW_NULL;
    }
    
    stSegmentByteSizeFree = psegment->SEGMENT_stByteSize;               /*  ��Ҫ�ͷŵķֶ����ݴ�С      */
        
    if (psegmentLeft) {
        if (__HEAP_SEGMENT_IS_USED(psegmentLeft) ||
            !__HEAP_SEGMENT_CAN_ML(psegment, psegmentLeft)) {           /*  ���ܾۺ�                    */
            goto    __merge_right;                                      /*  �����Ҷξۺ�                */
        }
        
        psegmentLeft->SEGMENT_stByteSize += (stSegmentByteSizeFree
                                          + __SEGMENT_BLOCK_SIZE_ALIGN);
        
        _List_Line_Del(&psegment->SEGMENT_lineManage, 
                       &plineDummyHeader);                              /*  ���ھ�������ɾ����ǰ�ֶ�    */
        
        pheap->HEAP_ulSegmentCounter--;
        pheap->HEAP_stUsedByteSize -= (__SEGMENT_BLOCK_SIZE_ALIGN + stSegmentByteSizeFree);
        pheap->HEAP_stFreeByteSize += (__SEGMENT_BLOCK_SIZE_ALIGN + stSegmentByteSizeFree);
        
        psegment = psegmentLeft;                                        /*  ��ǰ�ֶα����ֶ�          */
    
        if (plineRight == LW_NULL) {                                    /*  ���Ҳ�ֶ�                  */
            _List_Ring_Del(&psegment->SEGMENT_ringFreeList, 
                           &pheap->HEAP_pringFreeSegment);
            __HEAP_ADD_NEW_SEG_TO_FREELIST(psegment, pheap);            /*  ���²����ʵ���λ��(��Լ�ڴ�)*/
        }
    
        bIsMergeOk = LW_TRUE;                                           /*  �ɹ���������˺ϲ�          */
    }
    
__merge_right:
    
    if (psegmentRight) {                                                /*  �ҷֶξۺ�                  */
        if (__HEAP_SEGMENT_IS_USED(psegmentRight) ||
            !__HEAP_SEGMENT_CAN_MR(psegment, psegmentRight)) {          /*  ���ܾۺ�                    */
            goto    __right_merge_fail;                                 /*  �����Ҷξۺ�ʧ��            */
        }
        
        psegment->SEGMENT_stByteSize += (psegmentRight->SEGMENT_stByteSize
                                      + __SEGMENT_BLOCK_SIZE_ALIGN);
        
        _List_Ring_Del(&psegmentRight->SEGMENT_ringFreeList, 
                       &pheap->HEAP_pringFreeSegment);                  /*  ���ҷֶδӿ��б���ɾ��      */
        
        _List_Line_Del(&psegmentRight->SEGMENT_lineManage, 
                       &plineDummyHeader);                              /*  ���ҷֶδ��ھ�������ɾ��    */
        
        pheap->HEAP_ulSegmentCounter--;                                 /*  �����ֶ�����--              */
        
        if (bIsMergeOk == LW_FALSE) {                                   /*  ��û�в�����ϲ�ʱ          */
            __HEAP_ADD_NEW_SEG_TO_FREELIST(psegment, pheap);            /*  ����嵽���б�ͷ            */
            
            pheap->HEAP_stUsedByteSize -= (__SEGMENT_BLOCK_SIZE_ALIGN + stSegmentByteSizeFree);
            pheap->HEAP_stFreeByteSize += (__SEGMENT_BLOCK_SIZE_ALIGN + stSegmentByteSizeFree);
        
        } else {                                                        /*  ��ֶκϲ��ɹ�              */
            if (!_list_line_get_next(&psegment->SEGMENT_lineManage)) {  /*  �ϲ���Ϊ���Ҳ�ֶ�          */
                _List_Ring_Del(&psegment->SEGMENT_ringFreeList, 
                               &pheap->HEAP_pringFreeSegment);
                __HEAP_ADD_NEW_SEG_TO_FREELIST(psegment, pheap);        /*  ���²����ʵ���λ��(��Լ�ڴ�)*/
            }
        
            pheap->HEAP_stUsedByteSize -= (__SEGMENT_BLOCK_SIZE_ALIGN);
            pheap->HEAP_stFreeByteSize += (__SEGMENT_BLOCK_SIZE_ALIGN);
        }
        
        __heap_unlock(pheap);
        return  (LW_NULL);
    }
    
__right_merge_fail:                                                     /*  �ҷֶκϲ�����              */
    
    if (bIsMergeOk == LW_FALSE) {                                       /*  ��ֶκϲ�ʧ��              */
        __HEAP_ADD_NEW_SEG_TO_FREELIST(psegment, pheap);                /*  ����嵽���б�ͷ            */
        
        pheap->HEAP_stUsedByteSize -= (stSegmentByteSizeFree);
        pheap->HEAP_stFreeByteSize += (stSegmentByteSizeFree);
    }
    
    __heap_unlock(pheap);
    return  (LW_NULL);
}
/*********************************************************************************************************
** ��������: _HeapRealloc
** ��������: �Ӷ������������ֽڳ�
** �䡡��  : pheap              �ѿ��ƿ�
**           pvStartAddress     ԭʼ���ڴ滺���� (������Ҫ�ͷ�)
**           stNewByteSize      ��Ҫ�µ��ڴ��С
**           bIsNeedVerify      �Ƿ���Ҫ��ȫ���
**           pcPurpose          �����ڴ����;
** �䡡��  : ������ڴ��ַ
** ȫ�ֱ���: 
** ����ģ��: 
** ע  ��  : ����� pvStartAddress �� ulNewByteSize ������ʵ��Ч, ����������ڲ�ʹ�õ�.
*********************************************************************************************************/
PVOID  _HeapRealloc (PLW_CLASS_HEAP  pheap, 
                     PVOID           pvStartAddress, 
                     size_t          stNewByteSize,
                     BOOL            bIsNeedVerify,
                     CPCHAR          pcPurpose)
{
    REGISTER size_t             stSize;
    REGISTER size_t             stNewSegSize;
             PLW_CLASS_SEGMENT  psegment;
    REGISTER PLW_CLASS_SEGMENT  psegmentNew;
    
             PLW_LIST_LINE      plineHeader;
             
    REGISTER BOOL               bVerifyOk;                              /*  �Ƿ���ɹ�                */
    REGISTER ULONG              ulLockErr;
    
    if (stNewByteSize == 0) {
        if (pvStartAddress) {
            return  (_HeapFree(pheap, pvStartAddress, 
                               bIsNeedVerify, pcPurpose));              /*  �ͷ���ǰ������ڴ漴��      */
        } else {
            return  (LW_NULL);
        }
    }
    
    if (pvStartAddress == LW_NULL) {
        return  (_HeapAllocate(pheap, stNewByteSize, pcPurpose));       /*  ���������ڴ漴��            */
    }
    
    stNewByteSize = __heap_crossbord_size(stNewByteSize);               /*  Խ�������                */
    
    stSize = ROUND_UP(stNewByteSize, LW_CFG_HEAP_SEG_MIN_SIZE);         /*  ���ҳ�����ڴ��С          */
    
    ulLockErr = __heap_lock(pheap);
    if (ulLockErr) {                                                    /*  ����������                  */
        return  (LW_NULL);
    }
    if (bIsNeedVerify) {                                                /*  �Ƿ���Ҫ��ȫ�Լ��          */
        bVerifyOk = _HeapVerify(pheap, pvStartAddress, 
                                &psegment, __func__);                   /*  ���                        */
        if (bVerifyOk == LW_FALSE) {
            __heap_unlock(pheap);
            return  (LW_NULL);                                          /*  ������                    */
        }
    } else {                                                            /*  ����Ҫ��ȫ�Լ��            */
        psegment = __HEAP_SEGMENT_SEG_PTR(pvStartAddress);              /*  ���Ҷο��ƿ��ַ            */
        if (!__HEAP_SEGMENT_IS_REAL(psegment)) {                        /*  ���ƿ��ַ����ʵ            */
            psegment = (PLW_CLASS_SEGMENT)(*((size_t *)pvStartAddress - 1));
        }
        
        if (!__HEAP_SEGMENT_IS_REAL(psegment) ||
            !__HEAP_SEGMENT_IS_USED(psegment)) {                        /*  �ο��ƿ���Ч���Ѿ����ͷ�    */
            __heap_unlock(pheap);
            __DEBUG_MEM_ERROR(pcPurpose, pheap->HEAP_cHeapName, 
                              "not in used or double free", pvStartAddress);
            return  (LW_NULL);
        }
    }
    
    if (__heap_crossbord_check(psegment) == LW_FALSE) {                 /*  �ڴ�Խ��                    */
        __DEBUG_MEM_ERROR(pcPurpose, pheap->HEAP_cHeapName, 
                          "cross-border", pvStartAddress);
    }
    
    if (pvStartAddress != __HEAP_SEGMENT_DATA_PTR(psegment)) {          /*  realloc ��֧�ִ��ж�������  */
        __heap_unlock(pheap);
        __DEBUG_MEM_ERROR(pcPurpose, pheap->HEAP_cHeapName,
                          "bigger aligned", pvStartAddress);
        return  (LW_NULL);
    }
    
    if (stSize == psegment->SEGMENT_stByteSize) {                       /*  ��Сû�иı�                */
        __heap_unlock(pheap);
        return  (pvStartAddress);
    }
    
    if (stSize > psegment->SEGMENT_stByteSize) {                        /*  ϣ�����䵽����Ŀռ�        */
        REGISTER PLW_LIST_LINE      plineRight;
        REGISTER PLW_CLASS_SEGMENT  psegmentRight;                      /*  �ҷֶ�                      */
                 PLW_LIST_LINE      plineDummyHeader = LW_NULL;         /*  ���ڲ������ݵ�ͷ            */
        
        plineRight = _list_line_get_next(&psegment->SEGMENT_lineManage);/*  �ҷֶ�                      */
        if (plineRight) {
            psegmentRight = _LIST_ENTRY(plineRight, 
                                        LW_CLASS_SEGMENT, 
                                        SEGMENT_lineManage);            /*  �ҷֶο��ƿ�                */
            if (!__HEAP_SEGMENT_IS_USED(psegmentRight) &&
                __HEAP_SEGMENT_CAN_MR(psegment, psegmentRight)) {
                                                                        /*  �ҷֶ�û��ʹ��              */
                if ((psegmentRight->SEGMENT_stByteSize + 
                     __SEGMENT_BLOCK_SIZE_ALIGN) >=
                    (stSize - psegment->SEGMENT_stByteSize)) {          /*  ���Խ��ҷֶ��뱾�κϲ�      */
                    
                    _List_Ring_Del(&psegmentRight->SEGMENT_ringFreeList, 
                                   &pheap->HEAP_pringFreeSegment);      /*  ���ҷֶδӿ��б���ɾ��      */
                    _List_Line_Del(&psegmentRight->SEGMENT_lineManage, 
                                   &plineDummyHeader);                  /*  ���ҷֶδ��ھ�������ɾ��    */
                    
                    psegment->SEGMENT_stByteSize += 
                        (psegmentRight->SEGMENT_stByteSize +
                         __SEGMENT_BLOCK_SIZE_ALIGN);                   /*  ���±��δ�С                */
                                   
                    pheap->HEAP_ulSegmentCounter--;                     /*  �����ֶ�����--              */
                    pheap->HEAP_stUsedByteSize += psegmentRight->SEGMENT_stByteSize;
                    pheap->HEAP_stFreeByteSize -= psegmentRight->SEGMENT_stByteSize;
                    goto    __split_segment;
                }
            }
        }
        
        __heap_unlock(pheap);
        pvStartAddress = _HeapAllocate(pheap, stSize, pcPurpose);       /*  ���¿���                    */
        if (pvStartAddress) {                                           /*  ����ɹ�                    */
            lib_memcpy(pvStartAddress, 
                       __HEAP_SEGMENT_DATA_PTR(psegment), 
                       (UINT)psegment->SEGMENT_stByteSize);             /*  ����ԭʼ��Ϣ                */
            _HeapFree(pheap, 
                      __HEAP_SEGMENT_DATA_PTR(psegment), 
                      LW_FALSE, pcPurpose);                             /*  �ͷű���ռ�                */
        }
        
        ulLockErr = __heap_lock(pheap);                                 /*  ���� heap ���Բ��ܱ�ɾ��    */
        __HEAP_UPDATA_MAX_USED(pheap);                                  /*  ����ͳ�Ʊ���                */
        __heap_unlock(pheap);
        return  (pvStartAddress);
    }
    
__split_segment:
    stNewSegSize = psegment->SEGMENT_stByteSize - stSize;               /*  ��ȡʣ����ֽ���            */
    
    if (stNewSegSize < 
        (__SEGMENT_BLOCK_SIZE_ALIGN + LW_CFG_HEAP_SEG_MIN_SIZE)) {      /*  ʣ��ռ�̫С, �޷������¶�  */
        __HEAP_UPDATA_MAX_USED(pheap);                                  /*  ����ͳ�Ʊ���                */
        __heap_unlock(pheap);
        
        __heap_crossbord_mark(psegment);                                /*  �����ڴ�Խ���־            */
        
        MONITOR_EVT_LONG4(MONITOR_EVENT_ID_REGION, MONITOR_EVENT_REGION_REALLOC,
                          pheap, pvStartAddress, 
                          pvStartAddress, psegment->SEGMENT_stByteSize, pcPurpose);
                      
        return  (pvStartAddress);
    }
    
    psegment->SEGMENT_stByteSize = stSize;                              /*  ����ԭʼ�ֶεĴ�С          */
    
    psegmentNew = (PLW_CLASS_SEGMENT)(__HEAP_SEGMENT_DATA_PTR(psegment)
                + stSize);                                              /*  ȷ���·ֶεĵص�            */
                
    psegmentNew->SEGMENT_stByteSize = stNewSegSize 
                                    - __SEGMENT_BLOCK_SIZE_ALIGN;
    psegmentNew->SEGMENT_stMagic    = LW_SEG_MAGIC_REAL;
                                           
    _INIT_LIST_RING_HEAD(&psegmentNew->SEGMENT_ringFreeList);           /*  ����ʹ�õķֶ�              */
                                           
    plineHeader = &psegment->SEGMENT_lineManage;
    _List_Line_Add_Tail(&psegmentNew->SEGMENT_lineManage,
                        &plineHeader);                                  /*  ���������ھ���������        */
    
    pheap->HEAP_ulSegmentCounter++;                                     /*  ���ж���һ���ֶ�            */
    __heap_unlock(pheap);
    
    __heap_crossbord_mark(psegmentNew);                                 /*  �����ڴ�Խ���־            */
    
    _HeapFree(pheap, __HEAP_SEGMENT_DATA_PTR(psegmentNew), 
              LW_FALSE, pcPurpose);                                     /*  �����ڴ�����                */
    
    ulLockErr = __heap_lock(pheap);                                     /*  ���� heap ���Բ��ܱ�ɾ��    */
    __HEAP_UPDATA_MAX_USED(pheap);                                      /*  ����ͳ�Ʊ���                */
    __heap_unlock(pheap);
    
    __heap_crossbord_mark(psegment);                                    /*  �����ڴ�Խ���־            */
    
    MONITOR_EVT_LONG4(MONITOR_EVENT_ID_REGION, MONITOR_EVENT_REGION_REALLOC,
                      pheap, __HEAP_SEGMENT_DATA_PTR(psegment), 
                      pvStartAddress, stSize, pcPurpose);
    
    return  ((PVOID)__HEAP_SEGMENT_DATA_PTR(psegment));                 /*  ����ԭʼ�ڴ��              */
}
/*********************************************************************************************************
  END
*********************************************************************************************************/
