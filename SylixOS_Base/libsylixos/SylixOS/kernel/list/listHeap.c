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
** ��   ��   ��: listHeap.c
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2006 �� 12 �� 13 ��
**
** ��        ��: ����ϵͳ Heap ��Դ���������
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
/*********************************************************************************************************
** ��������: _Allocate_Heap_Object
** ��������: �ӿ��� Heap �ؼ�����ȡ��һ������ Heap
** �䡡��  : 
** �䡡��  : ��õ� Heap ��ַ��ʧ�ܷ��� NULL
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
PLW_CLASS_HEAP  _Allocate_Heap_Object (VOID)
{
    REGISTER PLW_LIST_MONO    pmonoFree;
    REGISTER PLW_CLASS_HEAP   pheapFree;
    
    if (_LIST_MONO_IS_EMPTY(_K_resrcHeap.RESRC_pmonoFreeHeader)) {
        return  (LW_NULL);
    }
    
    pmonoFree = _list_mono_allocate_seq(&_K_resrcHeap.RESRC_pmonoFreeHeader, 
                                        &_K_resrcHeap.RESRC_pmonoFreeTail);
                                                                        /*  �����Դ                   */
    pheapFree = _LIST_ENTRY(pmonoFree, LW_CLASS_HEAP, 
                            HEAP_monoResrcList);                        /*  �����Դ��������ַ         */
    
    _K_resrcHeap.RESRC_uiUsed++;
    if (_K_resrcHeap.RESRC_uiUsed > _K_resrcHeap.RESRC_uiMaxUsed) {
        _K_resrcHeap.RESRC_uiMaxUsed = _K_resrcHeap.RESRC_uiUsed;
    }
    
    return  (pheapFree);
}
/*********************************************************************************************************
** ��������: _Free_Heap_Object
** ��������: �� Heap ���ƿ齻�������
** �䡡��  : 
** �䡡��  : 
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
VOID  _Free_Heap_Object (PLW_CLASS_HEAP    pheapFree)
{
    REGISTER PLW_LIST_MONO    pmonoFree;
    
    pmonoFree = &pheapFree->HEAP_monoResrcList;
    
    _list_mono_free_seq(&_K_resrcHeap.RESRC_pmonoFreeHeader, 
                        &_K_resrcHeap.RESRC_pmonoFreeTail, 
                        pmonoFree);
    
    _K_resrcHeap.RESRC_uiUsed--;
}
/*********************************************************************************************************
  END
*********************************************************************************************************/
