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
** ��   ��   ��: selectInit.c
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2007 �� 11 �� 07 ��
**
** ��        ��:  IO ϵͳ select ��ϵͳ��ʼ��.

** BUG
2008.03.16 �� LW_HANDLE ��Ϊ LW_OBJECT_HANDLE.
2009.07.17 ���� __selTaskDeleteHook() �ص�����.
2017.08.17 ��· IO ����, �ɱ��źŴ��.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
#include "../SylixOS/system/include/s_system.h"
/*********************************************************************************************************
  �ü�����
*********************************************************************************************************/
#if (LW_CFG_DEVICE_EN > 0) && (LW_CFG_SELECT_EN > 0)
#include "select.h"
#include "selectDrv.h"
/*********************************************************************************************************
  �ڲ�����
*********************************************************************************************************/
static VOID     __selTaskCreateHook(LW_OBJECT_HANDLE  ulId, ULONG  ulOption);
static VOID     __selTaskDeleteHook(LW_OBJECT_HANDLE  ulId, PVOID  pvReturnVal, PLW_CLASS_TCB  ptcb);
/*********************************************************************************************************
** ��������: _SelectInit
** ��������: ��ʼ�� select ��
** �䡡��  : NONE
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
INT     _SelectInit (VOID)
{
    REGISTER LW_ERROR       ulError;
    
    ulError = API_SystemHookAdd(__selTaskCreateHook, 
                                LW_OPTION_THREAD_CREATE_HOOK);          /*  ��װ�̴߳������Ӻ���        */
    if (ulError) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "couldn't install task create hook.\r\n");
        return  (PX_ERROR);                                             /*  ��װʧ��                    */
    }
    
    ulError = API_SystemHookAdd(__selTaskDeleteHook, 
                                LW_OPTION_THREAD_DELETE_HOOK);          /*  ��װ�߳����ٹ��Ӻ���        */
    if (ulError) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "couldn't install task delete hook.\r\n");
        return  (PX_ERROR);                                             /*  ��װʧ��                    */
    }
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: __selTaskCreateHook
** ��������: �߳̽���ʱ�Ĺ��Ӻ���
** �䡡��  : ulId                  �߳� Id
             ulOption              �߳̽���ѡ��
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static VOID    __selTaskCreateHook (LW_OBJECT_HANDLE  ulId, ULONG  ulOption)
{
    REGISTER UINT16              usIndex = _ObjectGetIndex(ulId);
    REGISTER PLW_CLASS_TCB       ptcb;
    REGISTER LW_SEL_CONTEXT     *pselctxNew;
    
    ptcb = __GET_TCB_FROM_INDEX(usIndex);
    
    if (ulOption & LW_OPTION_THREAD_UNSELECT) {                         /*  ���̲߳�ʹ�� select ����    */
        ptcb->TCB_pselctxContext = LW_NULL;
        return;
    }

    pselctxNew = (LW_SEL_CONTEXT *)__SHEAP_ALLOC(sizeof(LW_SEL_CONTEXT));
    if (!pselctxNew) {                                                  /*  ����ʧ��                    */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "couldn't allocate memory for select context.\r\n");
        return;
    }
    
    pselctxNew->SELCTX_hSembWakeup = API_SemaphoreBCreate("sel_wakeup",
                                        LW_TRUE,
                                        LW_OPTION_WAIT_FIFO |
#if LW_CFG_SELECT_INTER_EN > 0
                                        LW_OPTION_SIGNAL_INTER |        /*  �ɱ��źŴ��                */
#endif                                                                  /*  LW_CFG_SELECT_INTER_EN > 0  */
                                        LW_OPTION_OBJECT_GLOBAL,
                                        LW_NULL);                       /*  �����ź���                  */
    if (!pselctxNew->SELCTX_hSembWakeup) {  
        __SHEAP_FREE(pselctxNew);                                       /*  �ź�������ʧ��              */
        return;
    }
    
    pselctxNew->SELCTX_bPendedOnSelect = LW_FALSE;                      /*  �߳�û������                */
    
    ptcb->TCB_pselctxContext = pselctxNew;
}
/*********************************************************************************************************
** ��������: __selTaskDeleteHook
** ��������: �߳�����ʱ�Ĺ��Ӻ���
** �䡡��  : ulId                  �߳� Id
**           pvReturnVal           �̷߳���ֵ
**           ptcb                  �߳� TCB
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static VOID    __selTaskDeleteHook (LW_OBJECT_HANDLE  ulId, PVOID  pvReturnVal, PLW_CLASS_TCB  ptcb)
{
    REGISTER LW_SEL_CONTEXT     *pselctxDelete;
             LW_SEL_WAKEUPNODE   selwunNode;
    
    (VOID)pvReturnVal;                                                  /*  ��ʹ�ô˲���                */
    
    pselctxDelete = ptcb->TCB_pselctxContext;
    if (!pselctxDelete) {
        return;                                                         /*  û�� select context         */
    }
    
    if (pselctxDelete->SELCTX_bPendedOnSelect) {                        /*  �� select() ����            */
        selwunNode.SELWUN_hThreadId = ulId;                             /*  �ͷ����нڵ�                */
        
        selwunNode.SELWUN_seltypType = SELREAD;
        __selDoIoctls(&pselctxDelete->SELCTX_fdsetOrigReadFds, LW_NULL,
                      pselctxDelete->SELCTX_iWidth, 
                      FIOUNSELECT, &selwunNode, LW_FALSE);              /*  �ͷ����еȴ���ʹ�ܵĽڵ�    */
        
        selwunNode.SELWUN_seltypType = SELWRITE;
        __selDoIoctls(&pselctxDelete->SELCTX_fdsetOrigWriteFds, LW_NULL,
                      pselctxDelete->SELCTX_iWidth,
                      FIOUNSELECT, &selwunNode, LW_FALSE);              /*  �ͷ����еȴ�дʹ�ܵĽڵ�    */
        
        selwunNode.SELWUN_seltypType = SELEXCEPT;
        __selDoIoctls(&pselctxDelete->SELCTX_fdsetOrigExceptFds, LW_NULL,
                      pselctxDelete->SELCTX_iWidth, 
                      FIOUNSELECT, &selwunNode, LW_FALSE);              /*  �ͷ����еȴ��쳣ʹ�ܵĽڵ�  */
    }
    
    API_SemaphoreBDelete(&pselctxDelete->SELCTX_hSembWakeup);           /*  ɾ���ź���                  */
    
    ptcb->TCB_pselctxContext = LW_NULL;
    __SHEAP_FREE(pselctxDelete);
}

#endif                                                                  /*  LW_CFG_DEVICE_EN > 0        */
                                                                        /*  LW_CFG_SELECT_EN > 0        */
/*********************************************************************************************************
  END
*********************************************************************************************************/
