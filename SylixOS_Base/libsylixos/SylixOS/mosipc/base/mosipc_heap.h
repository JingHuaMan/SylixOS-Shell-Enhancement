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
** ��   ��   ��: mosipc_heap.h
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2014 �� 10 �� 28 ��
**
** ��        ��: �����ϵͳͨ���ڴ��.
*********************************************************************************************************/

#ifndef __MOSIPC_HEAP_H
#define __MOSIPC_HEAP_H

/*********************************************************************************************************
  �����ϵͳͨ���ڴ�ѹ���
*********************************************************************************************************/

INT   __mosipcHeapInit(PLW_CLASS_HEAP pheapToBuild, PVOID pvStartAddress, size_t stByteSize);
PVOID __mosipcHeapAlloc(PLW_CLASS_HEAP pheap, size_t stByteSize, size_t stAlign, CPCHAR pcPurpose);
INT   __mosipcHeapFree(PLW_CLASS_HEAP pheap, PVOID pvStartAddress, CPCHAR pcPurpose);

#define MOSIPC_HEAP_INIT(pheap, addr, size)     __mosipcHeapInit(pheap, addr, size)
#define MOSIPC_HEAP_ALLOC(pheap, size, align)   __mosipcHeapAlloc(pheap, size, align, __func__)
#define MOSIPC_HEAP_FREE(pheap, addr)           __mosipcHeapFree(pheap, addr, __func__)

#endif                                                                  /*  __MOSIPC_HEAP_H             */
/*********************************************************************************************************
  END
*********************************************************************************************************/
