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
** ��   ��   ��: KernelHeapInfo.c
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2006 �� 12 �� 14 ��
**
** ��        ��: ϵͳ�ں˶�״̬��ѯ

** BUG
2007.11.07  �� LW_OPTION_HEAP_USR ��Ϊ LW_OPTION_HEAP_KERNEL.
            �� �û�������Ϊ �ں˶�.
2008.05.18  ʹ�� __KERNEL_ENTER() ���� ThreadLock();
2009.09.28  �޸�ע��.
2013.03.30  �� HeapInfoEx ��չ�ӿڷ��ڴ˴�.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
/*********************************************************************************************************
** ��������: API_KernelHeapInfo
** ��������: ϵͳ�ں˶�״̬��ѯ
** �䡡��  : 
**           ulOption             ��ѡ��ѡ�� LW_OPTION_HEAP_KERNEL LW_OPTION_HEAP_SYSTEM
**           pstByteSize          ���ܴ�С���ֽ���
**           pulSegmentCounter    �ѷֶ���
**           pstUsedByteSize      ��ʹ�õ��ֽ���
**           pstFreeByteSize      �ѿ��е��ֽ���
**           pstMaxUsedByteSize   �ѿ������ʹ����
** �䡡��  : 
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
ULONG  API_KernelHeapInfo (ULONG    ulOption, 
                           size_t  *pstByteSize,
                           ULONG   *pulSegmentCounter,
                           size_t  *pstUsedByteSize,
                           size_t  *pstFreeByteSize,
                           size_t  *pstMaxUsedByteSize)
{
    switch (ulOption) {
    
    case LW_OPTION_HEAP_KERNEL:
        __KERNEL_ENTER();                                               /*  �����ں�                    */
        if (pstByteSize) {
            *pstByteSize = _K_pheapKernel->HEAP_stTotalByteSize;
        }
        if (pulSegmentCounter) {
            *pulSegmentCounter = _K_pheapKernel->HEAP_ulSegmentCounter;
        }
        if (pstUsedByteSize) {
            *pstUsedByteSize = _K_pheapKernel->HEAP_stUsedByteSize;
        }
        if (pstFreeByteSize) {
            *pstFreeByteSize = _K_pheapKernel->HEAP_stFreeByteSize;
        }
        if (pstMaxUsedByteSize) {
            *pstMaxUsedByteSize = _K_pheapKernel->HEAP_stMaxUsedByteSize;
        }
        __KERNEL_EXIT();                                                /*  �˳��ں�                    */
        return  (ERROR_NONE);
        
    case LW_OPTION_HEAP_SYSTEM:
        if (_K_pheapKernel == _K_pheapSystem) {
            _ErrorHandle(ERROR_KERNEL_OPTION);
            return  (ERROR_KERNEL_OPTION);
        }
        __KERNEL_ENTER();                                               /*  �����ں�                    */
        if (pstByteSize) {
            *pstByteSize = _K_pheapSystem->HEAP_stTotalByteSize;
        }
        if (pulSegmentCounter) {
            *pulSegmentCounter = _K_pheapSystem->HEAP_ulSegmentCounter;
        }
        if (pstUsedByteSize) {
            *pstUsedByteSize = _K_pheapSystem->HEAP_stUsedByteSize;
        }
        if (pstFreeByteSize) {
            *pstFreeByteSize = _K_pheapSystem->HEAP_stFreeByteSize;
        }
        if (pstMaxUsedByteSize) {
            *pstMaxUsedByteSize = _K_pheapSystem->HEAP_stMaxUsedByteSize;
        }
        __KERNEL_EXIT();                                                /*  �˳��ں�                    */
        return  (ERROR_NONE);
        
    default:
        _ErrorHandle(ERROR_KERNEL_OPTION);
        return  (ERROR_KERNEL_OPTION);
    }
}
/*********************************************************************************************************
** ��������: KernelHeapInfoEx
** ��������: ϵͳ�ں˶�״̬��ѯ:�߼��ӿ�
** �䡡��  : 
**           ulOption             ��ѡ��ѡ�� LW_OPTION_HEAP_KERNEL LW_OPTION_HEAP_SYSTEM
**           pstByteSize          ���ܴ�С���ֽ���
**           pulSegmentCounter    �ѷֶ���
**           pstUsedByteSize      ��ʹ�õ��ֽ���
**           pstFreeByteSize      �ѿ��е��ֽ���
**           psegmentList[]       �ֶ�ͷ�ص��
**           iMaxCounter          �ֶ�ͷ�ص�����ܱ�������ֶ���.
** �䡡��  : 
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
ULONG  API_KernelHeapInfoEx (ULONG                ulOption, 
                             size_t              *pstByteSize,
                             ULONG               *pulSegmentCounter,
                             size_t              *pstUsedByteSize,
                             size_t              *pstFreeByteSize,
                             size_t              *pstMaxUsedByteSize,
                             PLW_CLASS_SEGMENT    psegmentList[],
                             INT                  iMaxCounter)
{
    switch (ulOption) {
    
    case LW_OPTION_HEAP_KERNEL:
        __KERNEL_ENTER();                                               /*  �����ں�                    */
        if (pstByteSize) {
            *pstByteSize = _K_pheapKernel->HEAP_stTotalByteSize;
        }
        if (pulSegmentCounter) {
            *pulSegmentCounter = _K_pheapKernel->HEAP_ulSegmentCounter;
        }
        if (pstUsedByteSize) {
            *pstUsedByteSize = _K_pheapKernel->HEAP_stUsedByteSize;
        }
        if (pstFreeByteSize) {
            *pstFreeByteSize = _K_pheapKernel->HEAP_stFreeByteSize;
        }
        if (pstMaxUsedByteSize) {
            *pstMaxUsedByteSize = _K_pheapKernel->HEAP_stMaxUsedByteSize;
        }
        __KERNEL_EXIT();                                                /*  �˳��ں�                    */
        
        if (psegmentList) {
            (VOID)_HeapGetInfo(_K_pheapKernel, psegmentList, iMaxCounter);
        }
        return  (ERROR_NONE);
        
    case LW_OPTION_HEAP_SYSTEM:
        if (_K_pheapKernel == _K_pheapSystem) {
            _ErrorHandle(ERROR_KERNEL_OPTION);
            return  (ERROR_KERNEL_OPTION);
        }
        __KERNEL_ENTER();                                               /*  �����ں�                    */
        if (pstByteSize) {
            *pstByteSize = _K_pheapSystem->HEAP_stTotalByteSize;
        }
        if (pulSegmentCounter) {
            *pulSegmentCounter = _K_pheapSystem->HEAP_ulSegmentCounter;
        }
        if (pstUsedByteSize) {
            *pstUsedByteSize = _K_pheapSystem->HEAP_stUsedByteSize;
        }
        if (pstFreeByteSize) {
            *pstFreeByteSize = _K_pheapSystem->HEAP_stFreeByteSize;
        }
        if (pstMaxUsedByteSize) {
            *pstMaxUsedByteSize = _K_pheapSystem->HEAP_stMaxUsedByteSize;
        }
        __KERNEL_EXIT();                                                /*  �˳��ں�                    */
        
        if (psegmentList) {
            (VOID)_HeapGetInfo(_K_pheapSystem, psegmentList, iMaxCounter);
        }
        return  (ERROR_NONE);
        
    default:
        _ErrorHandle(ERROR_KERNEL_OPTION);
        return  (ERROR_KERNEL_OPTION);
    }
}
/*********************************************************************************************************
  END
*********************************************************************************************************/
