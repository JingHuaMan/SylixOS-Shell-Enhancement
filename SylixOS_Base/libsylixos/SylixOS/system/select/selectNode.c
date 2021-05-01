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
** ��   ��   ��: selectNode.c
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2007 �� 11 �� 07 ��
**
** ��        ��:  IO ϵͳ select ��ϵͳ WAKEUP �ڵ��������.

** BUG
2007.11.13  ʹ���������������������ȫ��װ.
2007.11.20  API_SelWakeup() ��Ҫ����߳���Ч��.
2007.11.21  API_SelNodeDelete() �����Ƿ� FREE ,ֻҪѰ���ڵ�������˳�.
2007.11.21  API_SelNodeDeleteAll() ���ٵ��� API_SelNodeDelete() ����ɾ��, ��ʹ������ɾ���ڵ�.
2007.12.11  �������������,���Բ���һ��������.
2009.12.09  ������ API_SelWakeupAllError() exceJobAdd() ����ָ��.
2011.03.06  ���� gcc 4.5.1 ��� warning.
2014.03.03  �Ż�����.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
#include "../SylixOS/system/include/s_system.h"
/*********************************************************************************************************
  �ü�����
*********************************************************************************************************/
#if (LW_CFG_DEVICE_EN > 0) && (LW_CFG_SELECT_EN > 0)
#include "select.h"
/*********************************************************************************************************
  ��
*********************************************************************************************************/
#define SEL_LIST_LOCK(pselwulList)      \
        API_SemaphoreMPend(pselwulList->SELWUL_hListLock, LW_OPTION_WAIT_INFINITE)
#define SEL_LIST_UNLOCK(pselwulList)    \
        API_SemaphoreMPost(pselwulList->SELWUL_hListLock)
/*********************************************************************************************************
** ��������: API_SelWakeupType
** ��������: ��ýڵ�ĵȴ�����
** �䡡��  : pselwunNode        select wake up node ���ƽṹ
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
LW_SEL_TYPE  API_SelWakeupType (PLW_SEL_WAKEUPNODE   pselwunNode)
{
    if (!pselwunNode) {
        return  (SELEXCEPT);                                            /*  �ڵ����                    */
    }
    
    return  (pselwunNode->SELWUN_seltypType);                           /*  ���ؽڵ�����                */
}
/*********************************************************************************************************
** ��������: API_SelWakeup
** ��������: ����һ���ȴ����߳�
** �䡡��  : pselwunNode        select wake up node ���ƽṹ
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
VOID    API_SelWakeup (PLW_SEL_WAKEUPNODE   pselwunNode)
{
    REGISTER UINT16         usIndex;
    REGISTER PLW_CLASS_TCB  ptcb;
    
    PLW_SEL_CONTEXT         pselctxContext;
    
    if (!pselwunNode) {                                                 /*  ָ�����                    */
        return;
    }
    
    usIndex = _ObjectGetIndex(pselwunNode->SELWUN_hThreadId);
    
    ptcb = __GET_TCB_FROM_INDEX(usIndex);
    if (!ptcb || !ptcb->TCB_pselctxContext) {                           /*  �̲߳�����                  */
        return;
    }
    
    LW_SELWUN_SET_READY(pselwunNode);
    
    pselctxContext = ptcb->TCB_pselctxContext;
    API_SemaphoreBPost(pselctxContext->SELCTX_hSembWakeup);             /*  ��ǰ������ȴ��߳�        */
}
/*********************************************************************************************************
** ��������: API_SelWakeupError
** ��������: ���ڲ����˴���, ����һ���ȴ����߳�
** �䡡��  : pselwunNode        select wake up node ���ƽṹ
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
VOID    API_SelWakeupError (PLW_SEL_WAKEUPNODE   pselwunNode)
{
    REGISTER UINT16         usIndex;
    REGISTER PLW_CLASS_TCB  ptcb;
    
    PLW_SEL_CONTEXT         pselctxContext;
    
    if (!pselwunNode) {                                                 /*  ָ�����                    */
        return;
    }
    
    usIndex = _ObjectGetIndex(pselwunNode->SELWUN_hThreadId);
    
    ptcb = __GET_TCB_FROM_INDEX(usIndex);
    if (!ptcb || !ptcb->TCB_pselctxContext) {                           /*  �̲߳�����                  */
        return;
    }
    
    pselctxContext = ptcb->TCB_pselctxContext;
    pselctxContext->SELCTX_bBadFd = LW_TRUE;                            /*  �ļ�����������              */
    
    API_SemaphoreBPost(pselctxContext->SELCTX_hSembWakeup);             /*  ��ǰ������ȴ��߳�        */
}
/*********************************************************************************************************
** ��������: API_SelWakeupAll
** ��������: �������еȴ�ָ���������߳�
** �䡡��  : pselwulList        select wake up list ���ƽṹ
             seltyp             �ȴ�����
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
VOID    API_SelWakeupAll (PLW_SEL_WAKEUPLIST  pselwulList, LW_SEL_TYPE  seltyp)
{
    REGISTER PLW_SEL_WAKEUPNODE     pselwunNode;
    REGISTER PLW_LIST_LINE          plineNode;
    
    if (!pselwulList || !pselwulList->SELWUL_plineHeader) {             /*  û�нڵ�                    */
        return;
    }
    
    if (LW_CPU_GET_CUR_NESTING()) {                                     /*  ���ж���                    */
        _excJobAdd((VOIDFUNCPTR)API_SelWakeupAll, (PVOID)pselwulList, (PVOID)seltyp, 
                   0, 0, 0, 0);                                         /*  �װ��жϴ���                */
        return;
    }
    
    SEL_LIST_LOCK(pselwulList);
    
    for (plineNode  = pselwulList->SELWUL_plineHeader;
         plineNode != LW_NULL;
         plineNode  = _list_line_get_next(plineNode)) {                 /*  �������нڵ�                */
        
        pselwunNode = _LIST_ENTRY(plineNode, LW_SEL_WAKEUPNODE, SELWUN_lineManage);
        if (pselwunNode->SELWUN_seltypType == seltyp) {                 /*  �������                    */
            API_SelWakeup(pselwunNode);                                 /*  �ͷ�                        */
        }
    }
    
    SEL_LIST_UNLOCK(pselwulList);
}
/*********************************************************************************************************
** ��������: API_SelWakeupFifo
** ��������: ��������ȴ�ָ���������߳�
** �䡡��  : pselwulList        select wake up list ���ƽṹ
             seltyp             �ȴ�����
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
                                           API ����
*********************************************************************************************************/
#if LW_CFG_SEMFD_EN > 0

LW_API
VOID    API_SelWakeupFifo (PLW_SEL_WAKEUPLIST  pselwulList, LW_SEL_TYPE  seltyp)
{
    REGISTER PLW_SEL_WAKEUPNODE     pselwunNode;
    REGISTER PLW_SEL_WAKEUPNODE     pselwunOld = LW_NULL;
    REGISTER PLW_LIST_LINE          plineNode;

    if (!pselwulList || !pselwulList->SELWUL_plineHeader) {             /*  û�нڵ�                    */
        return;
    }

    if (LW_CPU_GET_CUR_NESTING()) {                                     /*  ���ж���                    */
        _excJobAdd((VOIDFUNCPTR)API_SelWakeupFifo, (PVOID)pselwulList, (PVOID)seltyp,
                   0, 0, 0, 0);                                         /*  �װ��жϴ���                */
        return;
    }

    SEL_LIST_LOCK(pselwulList);

    for (plineNode  = pselwulList->SELWUL_plineHeader;
         plineNode != LW_NULL;
         plineNode  = _list_line_get_next(plineNode)) {                 /*  �������нڵ�                */

        pselwunNode = _LIST_ENTRY(plineNode, LW_SEL_WAKEUPNODE, SELWUN_lineManage);
        if (pselwunNode->SELWUN_seltypType == seltyp) {                 /*  �������                    */
            pselwunOld = pselwunNode;
        }
    }

    if (pselwunOld) {
        API_SelWakeup(pselwunOld);                                      /*  �ͷ�                        */
    }

    SEL_LIST_UNLOCK(pselwulList);
}
/*********************************************************************************************************
** ��������: API_SelWakeupPrio
** ��������: �������ȼ���ߵȴ�ָ���������߳�
** �䡡��  : pselwulList        select wake up list ���ƽṹ
             seltyp             �ȴ�����
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
                                           API ����
*********************************************************************************************************/
LW_API
VOID    API_SelWakeupPrio (PLW_SEL_WAKEUPLIST  pselwulList, LW_SEL_TYPE  seltyp)
{
    REGISTER PLW_SEL_WAKEUPNODE     pselwunNode;
    REGISTER PLW_SEL_WAKEUPNODE     pselwunPrio = LW_NULL;
    REGISTER PLW_LIST_LINE          plineNode;
             UINT8                  ucPrio, ucPrioHigh = LW_PRIO_LOWEST;

    if (!pselwulList || !pselwulList->SELWUL_plineHeader) {             /*  û�нڵ�                    */
        return;
    }

    if (LW_CPU_GET_CUR_NESTING()) {                                     /*  ���ж���                    */
        _excJobAdd((VOIDFUNCPTR)API_SelWakeupPrio, (PVOID)pselwulList, (PVOID)seltyp,
                   0, 0, 0, 0);                                         /*  �װ��жϴ���                */
        return;
    }

    SEL_LIST_LOCK(pselwulList);

    for (plineNode  = pselwulList->SELWUL_plineHeader;
         plineNode != LW_NULL;
         plineNode  = _list_line_get_next(plineNode)) {                 /*  �������нڵ�                */

        pselwunNode = _LIST_ENTRY(plineNode, LW_SEL_WAKEUPNODE, SELWUN_lineManage);
        if (pselwunNode->SELWUN_seltypType == seltyp) {                 /*  �������                    */
            API_ThreadGetPriority(pselwunNode->SELWUN_hThreadId, &ucPrio);
            if (pselwunPrio == LW_NULL || (ucPrio <= ucPrioHigh)) {
                pselwunPrio = pselwunNode;
                ucPrioHigh  = ucPrio;
            }
        }
    }

    if (pselwunPrio) {
        API_SelWakeup(pselwunPrio);                                     /*  �ͷ�                        */
    }

    SEL_LIST_UNLOCK(pselwulList);
}

#endif                                                                  /*  LW_CFG_SEMFD_EN > 0         */
/*********************************************************************************************************
** ��������: API_SelWakeupAllByFlags
** ��������: �������еȴ�ָ���������߳�
** �䡡��  : pselwulList        select wake up list ���ƽṹ
             uiFlags            �ȴ����� 
                                LW_SEL_TYPE_FLAG_READ / LW_SEL_TYPE_FLAG_WRITE / LW_SEL_TYPE_FLAG_EXCEPT
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
VOID    API_SelWakeupAllByFlags (PLW_SEL_WAKEUPLIST  pselwulList, UINT  uiFlags)
{
    REGISTER PLW_SEL_WAKEUPNODE     pselwunNode;
    REGISTER PLW_LIST_LINE          plineNode;
    
    if (!pselwulList || !pselwulList->SELWUL_plineHeader) {             /*  û�нڵ�                    */
        return;
    }
    
    if (LW_CPU_GET_CUR_NESTING()) {                                     /*  ���ж���                    */
        _excJobAdd((VOIDFUNCPTR)API_SelWakeupAllByFlags, (PVOID)pselwulList, (PVOID)(LONG)uiFlags,
                   0, 0, 0, 0);                                         /*  �װ��жϴ���                */
        return;
    }
    
    SEL_LIST_LOCK(pselwulList);
    
    for (plineNode  = pselwulList->SELWUL_plineHeader;
         plineNode != LW_NULL;
         plineNode  = _list_line_get_next(plineNode)) {                 /*  �������нڵ�                */
        
        pselwunNode = _LIST_ENTRY(plineNode, LW_SEL_WAKEUPNODE, SELWUN_lineManage);
        switch (pselwunNode->SELWUN_seltypType) {
        
        case SELREAD:
            if (uiFlags & LW_SEL_TYPE_FLAG_READ) {
                API_SelWakeup(pselwunNode);                             /*  �ͷ�                        */
            }
            break;
            
        case SELWRITE:
            if (uiFlags & LW_SEL_TYPE_FLAG_WRITE) {
                API_SelWakeup(pselwunNode);                             /*  �ͷ�                        */
            }
            break;
        
        case SELEXCEPT:
            if (uiFlags & LW_SEL_TYPE_FLAG_EXCEPT) {
                API_SelWakeup(pselwunNode);                             /*  �ͷ�                        */
            }
            break;
            
        default:
            break;
        }
    }
    
    SEL_LIST_UNLOCK(pselwulList);
}
/*********************************************************************************************************
** ��������: API_SelWakeupTerm
** ��������: ���ڲ����˴���, �������еȴ�ָ���������߳�
** �䡡��  : pselwulList        select wake up list ���ƽṹ
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
VOID    API_SelWakeupTerm (PLW_SEL_WAKEUPLIST  pselwulList)
{
    REGISTER PLW_SEL_WAKEUPNODE     pselwunNode;
    REGISTER PLW_LIST_LINE          plineNode;
    
    if (!pselwulList) {
        return;
    }
    
    SEL_LIST_LOCK(pselwulList);
    
    plineNode = pselwulList->SELWUL_plineHeader;
    while (plineNode) {                                                 /*  ��һɾ��                    */
        pselwunNode = _LIST_ENTRY(plineNode, LW_SEL_WAKEUPNODE, SELWUN_lineManage);
        plineNode   = _list_line_get_next(plineNode);                   /*  �����һ���ڵ�              */
        
        API_SelWakeupError(pselwunNode);                                /*  �ͷ�                        */
        
        _List_Line_Del(&pselwunNode->SELWUN_lineManage, 
                       &pselwulList->SELWUL_plineHeader);               /*  ɾ���˽ڵ�                  */
        pselwulList->SELWUL_ulWakeCounter--;                            /*  �ȴ��߳�����--              */
        
        if (!LW_SELWUN_IS_DFREE(pselwunNode)) {                         /*  ��Ҫ�ͷ��ڴ�                */
            __SHEAP_FREE(pselwunNode);
        }
    }
    
    SEL_LIST_UNLOCK(pselwulList);
}
/*********************************************************************************************************
** ��������: API_SelNodeAdd
** ��������: ��ָ���� wake up list ���һ���ȴ��ڵ�.
** �䡡��  : pselwulList        select wake up list ���ƽṹ
             pselwunNode        select wake up node ���ƽṹ
** �䡡��  : ���� ERROR_NONE ,������ϵͳ�ڴ��ѷ�ʱ,���� PX_ERROR.
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
INT     API_SelNodeAdd (PLW_SEL_WAKEUPLIST  pselwulList, PLW_SEL_WAKEUPNODE   pselwunNode)
{
    REGISTER PLW_SEL_WAKEUPNODE   pselwunNodeClone;                     /*  �����Ľڵ�                  */  
    REGISTER BOOL                 bDontFree;                            /*  ����Ҫ�ͷ�                  */
    
    if (!pselwulList || !pselwunNode) {                                 /*  �ڴ����                    */
        return  (PX_ERROR);
    }
    
    SEL_LIST_LOCK(pselwulList);
    
    if (pselwulList->SELWUL_plineHeader == LW_NULL) {                   /*  û�нڵ�                    */
        pselwunNodeClone = &pselwulList->SELWUL_selwunFrist;
        bDontFree = LW_TRUE;
    
    } else {                                                            /*  �Ѿ����ڽڵ�                */
        pselwunNodeClone = (PLW_SEL_WAKEUPNODE)__SHEAP_ALLOC(sizeof(LW_SEL_WAKEUPNODE));
        bDontFree = LW_FALSE;
    }
    
    if (pselwunNodeClone == LW_NULL) {                                  /*  ����ʧ��                    */
        SEL_LIST_UNLOCK(pselwulList);
        return  (PX_ERROR);
    }
    
    *pselwunNodeClone = *pselwunNode;                                   /*  �����ڵ���Ϣ                */
    
    if (bDontFree) {
        LW_SELWUN_SET_DFREE(pselwunNodeClone);
    
    } else {
        LW_SELWUN_CLEAR_DFREE(pselwunNodeClone);
    }
    
    _List_Line_Add_Ahead(&pselwunNodeClone->SELWUN_lineManage, 
                         &pselwulList->SELWUL_plineHeader);             /*  ����ȴ�����                */
    pselwulList->SELWUL_ulWakeCounter++;                                /*  �ȴ��߳�����++              */
                         
    SEL_LIST_UNLOCK(pselwulList);
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: API_SelNodeDelete
** ��������: ��ָ���� wake up list ��,ɾ��һ���ȴ��ڵ�.
** �䡡��  : pselwulList        select wake up list ���ƽṹ
             pselwunDelete      select wake up node ���ƽṹ
** �䡡��  : ���� ERROR_NONE, ��û���ҵ��ڵ�ʱ,���� PX_ERROR.
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
INT     API_SelNodeDelete (PLW_SEL_WAKEUPLIST  pselwulList, PLW_SEL_WAKEUPNODE   pselwunDelete)
{
    REGISTER PLW_SEL_WAKEUPNODE   pselwunNode;                          /*  �����ڵ�                    */
    REGISTER PLW_LIST_LINE        plineNode;
    
    SEL_LIST_LOCK(pselwulList);
    
    for (plineNode  = pselwulList->SELWUL_plineHeader;
         plineNode != LW_NULL;
         plineNode  = _list_line_get_next(plineNode)) {                 /*  �������нڵ�                */
         
        pselwunNode = _LIST_ENTRY(plineNode, LW_SEL_WAKEUPNODE, SELWUN_lineManage);
        if ((pselwunNode->SELWUN_hThreadId  == pselwunDelete->SELWUN_hThreadId) &&
            (pselwunNode->SELWUN_seltypType == pselwunDelete->SELWUN_seltypType)) {
            
            _List_Line_Del(&pselwunNode->SELWUN_lineManage, 
                           &pselwulList->SELWUL_plineHeader);
            pselwulList->SELWUL_ulWakeCounter--;                        /*  �ȴ��߳�����--              */
            
            if (LW_SELWUN_IS_READY(pselwunNode)) {
                LW_SELWUN_SET_READY(pselwunDelete);
            }
            
            if (!LW_SELWUN_IS_DFREE(pselwunNode)) {                     /*  ��Ҫ�ͷ��ڴ�                */
                __SHEAP_FREE(pselwunNode);
            }
            
            SEL_LIST_UNLOCK(pselwulList);
            return  (ERROR_NONE);                                       /*  �˳�                        */
        }
    }
    
    SEL_LIST_UNLOCK(pselwulList);
    
    return  (PX_ERROR);                                                 /*  û���ҵ��ڵ�                */
}

#endif                                                                  /*  LW_CFG_DEVICE_EN > 0        */
                                                                        /*  LW_CFG_SELECT_EN > 0        */
/*********************************************************************************************************
  END
*********************************************************************************************************/
