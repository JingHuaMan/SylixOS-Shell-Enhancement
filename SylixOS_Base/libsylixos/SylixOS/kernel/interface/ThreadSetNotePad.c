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
** ��   ��   ��: ThreadSetNotePad.c
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2007 �� 07 �� 19 ��
**
** ��        ��: �����̼߳��±�

** BUG
2007.07.19  ���� _DebugHandle() ����
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
/*********************************************************************************************************
** ��������: API_ThreadSetNotePad
** ��������: �����̼߳��±�
** �䡡��  : 
**           ulId                          �߳�ID
**           ucNoteIndex                   �̼߳��±�����
**           ulVal                         �̼߳��±�ֵ
** �䡡��  : 
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
#if (LW_CFG_THREAD_NOTE_PAD_EN > 0) && (LW_CFG_MAX_NOTEPADS > 0)

LW_API  
ULONG  API_ThreadSetNotePad (LW_OBJECT_HANDLE  ulId,
                             UINT8             ucNoteIndex,
                             ULONG             ulVal)
{
             INTREG                iregInterLevel;
    REGISTER UINT16                usIndex;
    REGISTER PLW_CLASS_TCB         ptcb;
	
    usIndex = _ObjectGetIndex(ulId);
	
#if LW_CFG_ARG_CHK_EN > 0
    if (ucNoteIndex >= LW_CFG_MAX_NOTEPADS) {                           /*  �����±�����              */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "notepad invalidate.\r\n");
        _ErrorHandle(ERROR_THREAD_NOTEPAD_INDEX);
        return  (ERROR_THREAD_NOTEPAD_INDEX);
    }
    
    if (!_ObjectClassOK(ulId, _OBJECT_THREAD)) {                        /*  ��� ID ������Ч��          */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "thread handle invalidate.\r\n");
        _ErrorHandle(ERROR_KERNEL_HANDLE_NULL);
        return  (ERROR_KERNEL_HANDLE_NULL);
    }
    
    if (_Thread_Index_Invalid(usIndex)) {                               /*  ����߳���Ч��              */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "thread handle invalidate.\r\n");
        _ErrorHandle(ERROR_THREAD_NULL);
        return  (ERROR_THREAD_NULL);
    }
#endif

    __KERNEL_ENTER();                                                   /*  �����ں�                    */
    if (_Thread_Invalid(usIndex)) {
        __KERNEL_EXIT();                                                /*  �˳��ں�                    */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "thread handle invalidate.\r\n");
        _ErrorHandle(ERROR_THREAD_NULL);
        return  (ERROR_THREAD_NULL);
    }
    
    iregInterLevel = KN_INT_DISABLE();                                  /*  �ر��ж�                    */
    
    ptcb = _K_ptcbTCBIdTable[usIndex];
    
    ptcb->TCB_notepadThreadNotePad.NOTEPAD_ulNotePad[ucNoteIndex] = ulVal;
    
    __KERNEL_EXIT_IRQ(iregInterLevel);                                  /*  �˳��ں˲����ж�          */
    
    MONITOR_EVT_LONG3(MONITOR_EVENT_ID_THREAD, MONITOR_EVENT_THREAD_NOTEPAD, 
                      ulId, ucNoteIndex, ulVal, LW_NULL);
    
    return  (ERROR_NONE);
}

#endif                                                                  /* (LW_CFG_THREAD_NOTE_P...     */
                                                                        /* (LW_CFG_MAX_NOTEPADS > 0)    */
/*********************************************************************************************************
  END
*********************************************************************************************************/
