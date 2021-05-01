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
** ��   ��   ��: ThreadGetNotePad.c
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2007 �� 07 �� 18 ��
**
** ��        ��: ����̼߳��±�

** BUG
2007.07.18  ���� _DebugHandle() ����
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
/*********************************************************************************************************
** ��������: API_ThreadGetNotePad
** ��������: ����̼߳��±�
** �䡡��  : 
**           ulId                          �߳�ID
**           ucNoteIndex                   �̼߳��±�����
** �䡡��  : �̼߳��±�ֵ
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
#if (LW_CFG_THREAD_NOTE_PAD_EN > 0) && (LW_CFG_MAX_NOTEPADS > 0)

LW_API  
ULONG  API_ThreadGetNotePad (LW_OBJECT_HANDLE  ulId,
                             UINT8             ucNoteIndex)
{
             INTREG                iregInterLevel;
    REGISTER UINT16                usIndex;
    REGISTER PLW_CLASS_TCB         ptcb;
    
    REGISTER ULONG                 ulValTemp;
	
    usIndex = _ObjectGetIndex(ulId);
	
#if LW_CFG_ARG_CHK_EN > 0
    if (ucNoteIndex >= LW_CFG_MAX_NOTEPADS) {                           /*  �����±�����              */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "notepad invalidate.\r\n");
        _ErrorHandle(ERROR_THREAD_NOTEPAD_INDEX);
        return  (0);
    }
    
    if (!_ObjectClassOK(ulId, _OBJECT_THREAD)) {                        /*  ��� ID ������Ч��          */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "thread handle invalidate.\r\n");
        _ErrorHandle(ERROR_KERNEL_HANDLE_NULL);
        return  (0);
    }
    
    if (_Thread_Index_Invalid(usIndex)) {                               /*  ����߳���Ч��              */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "thread handle invalidate.\r\n");
        _ErrorHandle(ERROR_THREAD_NULL);
        return  (0);
    }
#endif

    __KERNEL_ENTER();                                                   /*  �����ں�                    */
    if (_Thread_Invalid(usIndex)) {
        __KERNEL_EXIT();                                                /*  �˳��ں�                    */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "thread handle invalidate.\r\n");
        _ErrorHandle(ERROR_THREAD_NULL);
        return  (0);
    }
    
    iregInterLevel = KN_INT_DISABLE();                                  /*  �ر��ж�                    */
    
    ptcb = _K_ptcbTCBIdTable[usIndex];
    
    ulValTemp = ptcb->TCB_notepadThreadNotePad.NOTEPAD_ulNotePad[ucNoteIndex];
    
    __KERNEL_EXIT_IRQ(iregInterLevel);                                  /*  �˳��ں˲����ж�          */
    
    return  (ulValTemp);
}
/*********************************************************************************************************
** ��������: API_ThreadCurNotePad
** ��������: ����̼߳��±�
** �䡡��  :
**           ucNoteIndex                   �̼߳��±�����
** �䡡��  : �̼߳��±�ֵ
** ȫ�ֱ���:
** ����ģ��:
                                           API ����
*********************************************************************************************************/
LW_API
ULONG  API_ThreadCurNotePad (UINT8  ucNoteIndex)
{
    REGISTER PLW_CLASS_TCB  ptcbCur;

#if LW_CFG_ARG_CHK_EN > 0
    if (ucNoteIndex >= LW_CFG_MAX_NOTEPADS) {                           /*  �����±�����              */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "notepad invalidate.\r\n");
        _ErrorHandle(ERROR_THREAD_NOTEPAD_INDEX);
        return  (0);
    }
#endif

    LW_TCB_GET_CUR_SAFE(ptcbCur);

    return  (ptcbCur->TCB_notepadThreadNotePad.NOTEPAD_ulNotePad[ucNoteIndex]);
}
/*********************************************************************************************************
** ��������: API_ThreadFastNotePad
** ��������: ���ٻ���̼߳��±� (����������Ч�Լ��, ��֧�ֶ�����״̬����, �Ͻ����쳣״̬����)
** �䡡��  :
**           ucNoteIndex                   �̼߳��±�����
** �䡡��  : �̼߳��±�ֵ
** ȫ�ֱ���:
** ����ģ��:
                                           API ����
*********************************************************************************************************/
#if defined(LW_CFG_CPU_ARCH_ARM64) && (LW_CFG_ARM64_FAST_TCB_CUR > 0)

LW_API
ULONG  API_ThreadFastNotePad (UINT8  ucNoteIndex)
{
    REGISTER PLW_CLASS_TCB  ptcbCur;

    asm volatile ("mov %0, x18" : "=r"(ptcbCur));

    return  (ptcbCur->TCB_notepadThreadNotePad.NOTEPAD_ulNotePad[ucNoteIndex]);
}

#endif                                                                  /*  LW_CFG_ARM64_FAST_TCB_CUR   */
#endif                                                                  /*  (LW_CFG_THREAD_NOTE_PAD_... */
                                                                        /*  (LW_CFG_MAX_NOTEPADS > 0)   */
/*********************************************************************************************************
  END
*********************************************************************************************************/
