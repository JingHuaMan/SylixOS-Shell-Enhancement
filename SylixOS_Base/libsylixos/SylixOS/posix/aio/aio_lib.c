/*********************************************************************************************************
**
**                                    �й������Դ��֯
**
**                                   Ƕ��ʽʵʱ����ϵͳ
**
**                                       SylixOS(TM)
**
**                               Copyright  All Rights Reserved
**
**--------------�ļ���Ϣ--------------------------------------------------------------------------------
**
** ��   ��   ��: aio_lib.c
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2011 �� 02 �� 26 ��
**
** ��        ��: posix �첽I/O�ڲ���.
** ע        ��: �� mutex ���� aiorc ��, ���������� aioqueue, ������ܲ�������.

** BUG:
2013.02.22  __aioThread() ����ʹ�� lseek ����, ����ʹ�� pread �� pwrite ���ж�д.
2017.08.11  ���� AIO_REQ_BUSY ��ʶ, �����ļ����ڱ�����.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../include/px_pthread.h"
#include "../include/px_aio.h"                                          /*  �Ѱ�������ϵͳͷ�ļ�        */
/*********************************************************************************************************
  �ü�֧��
*********************************************************************************************************/
#if LW_CFG_POSIX_EN > 0 && LW_CFG_POSIX_AIO_EN > 0
#include "aio_lib.h"
/*********************************************************************************************************
  aio �����߳�����
*********************************************************************************************************/
#define __PX_AIO_TIMEOUT    (3 * LW_TICK_HZ)
/*********************************************************************************************************
  aio �ź��������
*********************************************************************************************************/
#if LW_CFG_SIGNAL_EN > 0

struct sigevent_notify_arg {
    VOIDFUNCPTR     SNA_pfunc;
    union sigval    SNA_sigvalue;
};

#endif                                                                  /*  LW_CFG_SIGNAL_EN > 0        */
/*********************************************************************************************************
  aio �����̴߳�������
*********************************************************************************************************/
static LW_CLASS_THREADATTR  _G_threadattrAio = {
    LW_NULL, LW_CFG_THREAD_DEFAULT_GUARD_SIZE, LW_CFG_POSIX_AIO_STK_SIZE, LW_PRIO_NORMAL, 
    LW_OPTION_THREAD_STK_CHK | LW_OPTION_THREAD_DETACHED,
    LW_NULL, LW_NULL
};
/*********************************************************************************************************
  aio ȫ�ֱ���
*********************************************************************************************************/
AIO_QUEUE                   _G_aioqueue;                                /*  aio ���й���                */
/*********************************************************************************************************
  aio �ڲ���������
*********************************************************************************************************/
static INT  __aioRemoveAllAiocb(AIO_REQ_CHAIN  *paiorc, INT  iError, INT  iErrNo);
/*********************************************************************************************************
** ��������: _posixAioInit
** ��������: ��ʼ�� AIO ������
** �䡡��  : NONE
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
VOID  _posixAioInit (VOID)
{
    _G_aioqueue.aioq_mutex = API_SemaphoreMCreate("aio_mutex", LW_PRIO_DEF_CEILING, 
                                                  LW_OPTION_WAIT_PRIORITY |
                                                  LW_OPTION_DELETE_SAFE | 
                                                  LW_OPTION_INHERIT_PRIORITY, LW_NULL);
    if (_G_aioqueue.aioq_mutex == 0) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "can not initialize mutex.\r\n");
        return;
    }
    
    if (API_ThreadCondInit(&_G_aioqueue.aioq_newreq, LW_THREAD_PROCESS_SHARED) != ERROR_NONE) {
        API_SemaphoreMDelete(&_G_aioqueue.aioq_mutex);
        _DebugHandle(__ERRORMESSAGE_LEVEL, "can not initialize thread cond.\r\n");
        return;
    }
    
    _G_aioqueue.aioq_plinework = LW_NULL;
    _G_aioqueue.aioq_plineidle = LW_NULL;
    
    _G_aioqueue.aioq_idlethread = 0;
    _G_aioqueue.aioq_actthread  = 0;
}
/*********************************************************************************************************
** ��������: aio_setstacksize
** ��������: ���� aio �����̵߳Ķ�ջ��С (����δ���������߳���Ч)
** �䡡��  : newsize       �µĴ�С
** �䡡��  : ERROR_NONE or ERROR
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API 
int  aio_setstacksize (size_t  newsize)
{
    if (_StackSizeCheck(newsize)) {
        errno = EINVAL;
        return  (EINVAL);
    }
    _G_threadattrAio.THREADATTR_stStackByteSize = newsize;
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: aio_getstacksize
** ��������: ��ȡ aio �����̵߳Ķ�ջ��С
** �䡡��  : NONE
** �䡡��  : ��ջ��С
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API 
size_t  aio_getstacksize (void)
{
    return  (_G_threadattrAio.THREADATTR_stStackByteSize);
}
/*********************************************************************************************************
** ��������: __aioSearchFd
** ��������: ��ָ���� AIO_REQ_CHAIN ��������ָ���ļ���������ͬ�� aio req chain �ڵ�. 
             (����������� _G_aioqueue)
** �䡡��  : plineHeader       ��������ͷ
**           iFd               �ļ�������
** �䡡��  : ���ҵ��� aio req chain �ڵ�
** ȫ�ֱ���: 
** ����ģ��: 
** ע  ��  : ������� search ���� aiorc ����û�б� cancel ��. ����� cancel ��, һ�����д����߳̽� cancel
             �� aiorc ɾ��. 
*********************************************************************************************************/
AIO_REQ_CHAIN  *__aioSearchFd (LW_LIST_LINE_HEADER  plineHeader, int  iFd)
{
    AIO_REQ_CHAIN  *paiorc;
    PLW_LIST_LINE   plineTemp;
    
    for (plineTemp  = plineHeader;
         plineTemp != LW_NULL;
         plineTemp  = _list_line_get_next(plineTemp)) {
         
        paiorc = _LIST_ENTRY(plineTemp, AIO_REQ_CHAIN, aiorc_line);
        if ((paiorc->aiorc_fildes == iFd) &&
            (paiorc->aiorc_iscancel == 0)) {                            /*  ע��, cancel��paiorc������  */
            break;
        }
    }
    
    if (plineTemp) {
        return  (paiorc);
    } else {
        return  (LW_NULL);
    }
}
/*********************************************************************************************************
** ��������: __aioCreateFd
** ��������: ����һ�� AIO_REQ_CHAIN �ڵ�, ������ָ������ (����������� _G_aioqueue)
** �䡡��  : pplineHeader      ���������������ͷ��ַ
**           iFd               �ļ�������
** �䡡��  : �������� aio req chain �ڵ�
** ȫ�ֱ���: 
** ����ģ��: 
** ע  ��  : �������� AIO_REQ_CHAIN �ڵ�, һ�����ɴ����߳� delete ��. 
*********************************************************************************************************/
static AIO_REQ_CHAIN  *__aioCreateFd (LW_LIST_LINE_HEADER  *pplineHeader, int  iFd)
{
    AIO_REQ_CHAIN  *paiorc;
    
    paiorc = (AIO_REQ_CHAIN *)__SHEAP_ALLOC(sizeof(AIO_REQ_CHAIN));
    if (paiorc == LW_NULL) {
        return  (LW_NULL);
    }
    
    paiorc->aiorc_mutex = API_SemaphoreMCreate("aiorc_mutex", LW_PRIO_DEF_CEILING, 
                                               LW_OPTION_WAIT_PRIORITY |
                                               LW_OPTION_DELETE_SAFE | 
                                               LW_OPTION_INHERIT_PRIORITY, LW_NULL);
    if (paiorc->aiorc_mutex == 0) {
        __SHEAP_FREE(paiorc);
        return  (LW_NULL);
    }
    
    if (API_ThreadCondInit(&paiorc->aiorc_cond, LW_THREAD_PROCESS_SHARED) != ERROR_NONE) {
        API_SemaphoreMDelete(&paiorc->aiorc_mutex);
        __SHEAP_FREE(paiorc);
        return  (LW_NULL);
    }
    
    paiorc->aiorc_plineaiocb = LW_NULL;
    paiorc->aiorc_fildes     = iFd;
    paiorc->aiorc_iscancel   = 0;                                       /*  û�б� cancel               */
    
    _List_Line_Add_Ahead(&paiorc->aiorc_line, pplineHeader);
    
    return  (paiorc);
}
/*********************************************************************************************************
** ��������: __aioDeleteFd
** ��������: ��ָ���� AIO_REQ_CHAIN ��ɾ��һ�� AIO_REQ_CHAIN �ڵ� (����������� _G_aioqueue)
** �䡡��  : paiorc            AIO_REQ_CHAIN �ڵ�
**           pplineHeader      ���������������ͷ��ַ
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static VOID  __aioDeleteFd (AIO_REQ_CHAIN  *paiorc, LW_LIST_LINE_HEADER  *pplineHeader)
{
    API_SemaphoreMPend(paiorc->aiorc_mutex, LW_OPTION_WAIT_INFINITE);
    __aioRemoveAllAiocb(paiorc, PX_ERROR, ECANCELED);                   /*  ɾ������ͬ�ļ��������ڵ�    */
    API_SemaphoreMPost(paiorc->aiorc_mutex);
    
    _List_Line_Del(&paiorc->aiorc_line, pplineHeader);

    API_SemaphoreMDelete(&paiorc->aiorc_mutex);
    API_ThreadCondDestroy(&paiorc->aiorc_cond);
    
    __SHEAP_FREE(paiorc);
}
/*********************************************************************************************************
** ��������: __aioCancelFd
** ��������: ȡ��һ�� AIO_REQ_CHAIN �ڵ�, 
** �䡡��  : paiorc            AIO_REQ_CHAIN �ڵ�
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
VOID  __aioCancelFd (AIO_REQ_CHAIN  *paiorc)
{
    API_SemaphoreMPend(paiorc->aiorc_mutex, LW_OPTION_WAIT_INFINITE);
    __aioRemoveAllAiocb(paiorc, PX_ERROR, ECANCELED);                   /*  ɾ������ͬ�ļ��������ڵ�    */
    paiorc->aiorc_iscancel = 1;
    API_ThreadCondSignal(&paiorc->aiorc_cond);                          /*  ֪ͨ�����߳�ɾ�� aiorc      */
    API_SemaphoreMPost(paiorc->aiorc_mutex);
}
/*********************************************************************************************************
** ��������: __aioInsertAiocb
** ��������: ��ָ���� aiocb �����ȼ����뵽 aio req chain �ڵ������Ķ����� (����������� paiorc)
** �䡡��  : paiorc            Ŀ�� aio req chain �ڵ�
**           paiocb            ָ���� aiocb 
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
** ע  ��  : ���ȼ���ֵԽ��, Խ������ͷ, ����Խ�ȱ�ִ��.
*********************************************************************************************************/
static VOID  __aioInsertAiocb (AIO_REQ_CHAIN  *paiorc, struct aiocb *paiocb)
{
    PLW_LIST_LINE   plineTemp;
    struct aioreq  *paioreqTemp;
    struct aiocb   *paiocbTemp;
    
    if (paiorc->aiorc_plineaiocb == LW_NULL) {                          /*  �����, �����ͷ����        */
        _List_Line_Add_Ahead(&paiocb->aio_req.aioreq_line, &paiorc->aiorc_plineaiocb);
        return;
    }
    
    plineTemp = paiorc->aiorc_plineaiocb;
    do {
        paioreqTemp = _LIST_ENTRY(plineTemp, struct aioreq, aioreq_line);
        paiocbTemp  = _LIST_ENTRY(paioreqTemp, struct aiocb, aio_req);
        if (paiocbTemp->aio_reqprio < paiocb->aio_reqprio) {            /*  �Ƿ���Բ���                */
            if (plineTemp == paiorc->aiorc_plineaiocb) {                /*  �Ƿ�Ϊ��ͷλ��              */
                _List_Line_Add_Ahead(&paiocb->aio_req.aioreq_line, 
                                     &paiorc->aiorc_plineaiocb);        /*  �����ͷ����                */
                break;
            } else {
                _List_Line_Add_Left(&paiocb->aio_req.aioreq_line, 
                                    plineTemp);                         /*  ���뵱ǰ�ڵ�����          */
            }
            break;
        }
        if (_list_line_get_next(plineTemp) == LW_NULL) {                /*  ���һ��                    */
            _List_Line_Add_Right(&paiocb->aio_req.aioreq_line, 
                                 plineTemp);                            /*  ���뵱ǰ�ڵ���Ҳ�          */
            break;
        } else {
            plineTemp = _list_line_get_next(plineTemp);                 /*  ���������һ��              */
        }
    } while (1);
}
/*********************************************************************************************************
** ��������: __aioRemoveAiocb
** ��������: ��ָ���� aiocb �� aio req chain �ڵ������Ķ������Ƴ� (����������� _G_aioqueue �� paiorc)
** �䡡��  : paiorc            aio req chain �ڵ�
**           paiocb            ָ���� aiocb 
**           iError            ����ķ���ֵ
**           iErrNo            ����Ĵ����
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
INT  __aioRemoveAiocb (AIO_REQ_CHAIN  *paiorc, struct aiocb *paiocb, INT  iError, INT  iErrNo)
{
    PLW_LIST_LINE   plineTemp;
    struct aioreq  *paioreqTemp;
    struct aiocb   *paiocbTemp;
    
    for (plineTemp  = paiorc->aiorc_plineaiocb;
         plineTemp != LW_NULL;
         plineTemp  = _list_line_get_next(plineTemp)) {
        
        paioreqTemp = _LIST_ENTRY(plineTemp, struct aioreq, aioreq_line);
        paiocbTemp  = _LIST_ENTRY(paioreqTemp, struct aiocb, aio_req);
        
        if (paiocbTemp == paiocb) {
            if (paiocbTemp->aio_req.aioreq_flags & AIO_REQ_BUSY) {
                return  (AIO_NOTCANCELED);
            }
        
            _List_Line_Del(plineTemp, 
                           &paiorc->aiorc_plineaiocb);
            
            paioreqTemp->aioreq_error  = iErrNo;
            paioreqTemp->aioreq_return = iError;
            
            if (paiocbTemp->aio_pwait) {
                __aioDetachWait(paiocbTemp, LW_NULL);                   /*  detach wait ����            */
            }
            
            return  (AIO_CANCELED);
        }
    }
    
    return  (AIO_ALLDONE);
}
/*********************************************************************************************************
** ��������: __aioRemoveAllAiocb
** ��������: ��ָ���� aiocb ���������� aio req chain �ڵ��Ƴ� (����������� _G_aioqueue �� paiorc)
** �䡡��  : paiorc            aio req chain �ڵ�
**           iError            ����ķ���ֵ
**           iErrNo            ����Ĵ����
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static INT  __aioRemoveAllAiocb (AIO_REQ_CHAIN  *paiorc, INT  iError, INT  iErrNo)
{
    struct aioreq  *paioreqTemp;
    struct aiocb   *paiocbTemp;

    while (paiorc->aiorc_plineaiocb) {
        paioreqTemp = _LIST_ENTRY(paiorc->aiorc_plineaiocb, struct aioreq, aioreq_line);
        paiocbTemp  = _LIST_ENTRY(paioreqTemp, struct aiocb, aio_req);
        
        _List_Line_Del(paiorc->aiorc_plineaiocb, 
                       &paiorc->aiorc_plineaiocb);
        
        paioreqTemp->aioreq_error  = iErrNo;
        paioreqTemp->aioreq_return = iError;
        
        if (paiocbTemp->aio_pwait) {
            __aioDetachWait(paiocbTemp, LW_NULL);                       /*  detach wait ����            */
        }
    }
    return  (AIO_CANCELED);
}
/*********************************************************************************************************
** ��������: __aioAttachWait
** ��������: ��ָ���� aiocb ������ wait ��Ϣ (����������� _G_aioqueue)
** �䡡��  : paiocb            ָ���� aiocb 
**           paiowait          wait ��Ϣ
** �䡡��  : ERROR or OK
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
INT  __aioAttachWait (const struct aiocb  *paiocb, struct aiowait  *paiowait)
{
    PLW_LIST_LINE   plineTempAiorc;
    PLW_LIST_LINE   plineTempAiocb;
    
    AIO_REQ_CHAIN   *paiorc;
    struct aioreq   *paioreqTemp;
    struct aiocb    *paiocbTemp;
    
    BOOL             bIsOk = LW_FALSE;
    
    for (plineTempAiorc  = _G_aioqueue.aioq_plinework;
         plineTempAiorc != LW_NULL;
         plineTempAiorc  = _list_line_get_next(plineTempAiorc)) {
         
        paiorc = _LIST_ENTRY(plineTempAiorc, AIO_REQ_CHAIN, aiorc_line);
        if (paiorc->aiorc_fildes == paiocb->aio_fildes) {
            API_SemaphoreMPend(paiorc->aiorc_mutex, LW_OPTION_WAIT_INFINITE);
            for (plineTempAiocb  = paiorc->aiorc_plineaiocb;
                 plineTempAiocb != LW_NULL;
                 plineTempAiocb  = _list_line_get_next(plineTempAiocb)) {
                 
                paioreqTemp = _LIST_ENTRY(plineTempAiocb, struct aioreq, aioreq_line);
                paiocbTemp  = _LIST_ENTRY(paioreqTemp, struct aiocb, aio_req);
                
                if (paiocbTemp == paiocb) {
                    if (paiocbTemp->aio_pwait) {                        /*  �������ظ�����              */
                        API_SemaphoreMPost(paiorc->aiorc_mutex);
                        return  (PX_ERROR);
                    }
                    paiocbTemp->aio_pwait  = paiowait;
                    paiowait->aiowt_paiocb = paiocbTemp;
                    bIsOk = LW_TRUE;
                    break;
                }
            }
            API_SemaphoreMPost(paiorc->aiorc_mutex);
            break;
        }
    }
    
    if (bIsOk) {
        return  (ERROR_NONE);
    }
    
    for (plineTempAiorc  = _G_aioqueue.aioq_plineidle;
         plineTempAiorc != LW_NULL;
         plineTempAiorc  = _list_line_get_next(plineTempAiorc)) {
         
        paiorc = _LIST_ENTRY(plineTempAiorc, AIO_REQ_CHAIN, aiorc_line);
        if (paiorc->aiorc_fildes == paiocb->aio_fildes) {
            API_SemaphoreMPend(paiorc->aiorc_mutex, LW_OPTION_WAIT_INFINITE);
            for (plineTempAiocb  = paiorc->aiorc_plineaiocb;
                 plineTempAiocb != LW_NULL;
                 plineTempAiocb  = _list_line_get_next(plineTempAiocb)) {
                 
                paioreqTemp = _LIST_ENTRY(plineTempAiocb, struct aioreq, aioreq_line);
                paiocbTemp  = _LIST_ENTRY(paioreqTemp, struct aiocb, aio_req);
                
                if (paiocbTemp == paiocb) {
                    if (paiocbTemp->aio_pwait) {                        /*  �������ظ�����              */
                        API_SemaphoreMPost(paiorc->aiorc_mutex);
                        return  (PX_ERROR);
                    }
                    paiocbTemp->aio_pwait  = paiowait;
                    paiowait->aiowt_paiocb = paiocbTemp;
                    bIsOk = LW_TRUE;
                    break;
                }
            }
            API_SemaphoreMPost(paiorc->aiorc_mutex);
            break;
        }
    }
    
    if (bIsOk) {
        return  (ERROR_NONE);
    }
    
    return  (PX_ERROR);
}
/*********************************************************************************************************
** ��������: __aioAttachWaitNoCheck
** ��������: ��ָ���� aiocb ������ wait ��Ϣ, ����� aiocb �Ƿ��ڶ����� (����������� _G_aioqueue)
** �䡡��  : paiocb            ָ���� aiocb 
**           paiowait          wait ��Ϣ
** �䡡��  : ERROR or OK
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
VOID  __aioAttachWaitNoCheck (struct aiocb  *paiocb, struct aiowait  *paiowait)
{
    paiocb->aio_pwait = paiowait;
    paiowait->aiowt_paiocb = paiocb;
}
/*********************************************************************************************************
** ��������: __aioDetachWait
** ��������: ��ָ���� aiocb ���� wait ��Ϣ���� (����������� _G_aioqueue)
** �䡡��  : paiocb            ָ���� aiocb   (����Ϊ NULL)
**           paiowait          wait ��Ϣ      (����Ϊ NULL , ���� paiocb ����ͬʱΪ NULL)
** �䡡��  : ERROR or OK
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
INT  __aioDetachWait (struct aiocb  *paiocb, struct aiowait  *paiowait)
{
    if (paiocb) {
        paiowait = paiocb->aio_pwait;
        if (paiowait) {
            paiocb->aio_pwait      = LW_NULL;
            paiowait->aiowt_paiocb = LW_NULL;
        }
        return  (ERROR_NONE);
    }
    
    if (paiowait) {
        paiocb = paiowait->aiowt_paiocb;
        if (paiocb) {
            paiocb->aio_pwait      = LW_NULL;
            paiowait->aiowt_paiocb = LW_NULL;
        }
        return  (ERROR_NONE);
    }
    
    return  (PX_ERROR);
}
/*********************************************************************************************************
** ��������: __aioCreateWaitChain
** ��������: ����һ���ȴ�����
** �䡡��  : iNum          ������ wait �ڵ�ĸ���
**           bIsNeedCond   �Ƿ񴴽� cond
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
AIO_WAIT_CHAIN  *__aioCreateWaitChain (int  iNum, BOOL  bIsNeedCond)
{
    AIO_WAIT_CHAIN  *paiowt;
    
    paiowt = (AIO_WAIT_CHAIN *)__SHEAP_ALLOC(sizeof(AIO_WAIT_CHAIN));
    if (paiowt == LW_NULL) {
        return  (LW_NULL);
    }
    lib_bzero(paiowt, sizeof(AIO_WAIT_CHAIN));
    
    paiowt->aiowc_paiowait = (struct aiowait *)__SHEAP_ALLOC(sizeof(struct aiowait) * (size_t)iNum);
    if (paiowt->aiowc_paiowait == LW_NULL) {
        __SHEAP_FREE(paiowt);
        return  (LW_NULL);
    }
    lib_bzero(paiowt->aiowc_paiowait, sizeof(struct aiowait) * iNum);
    
    if (bIsNeedCond) {
        if (API_ThreadCondInit(&paiowt->aiowc_cond, LW_THREAD_PROCESS_SHARED) != ERROR_NONE) {
            __SHEAP_FREE(paiowt->aiowc_paiowait);
            __SHEAP_FREE(paiowt);
            return  (LW_NULL);
        }
    }

    return  (paiowt);
}
/*********************************************************************************************************
** ��������: __aioDeleteWaitChain
** ��������: ɾ��һ���ȴ�����   (����������� _G_aioqueue)
** �䡡��  : paiowt        �ȴ����п��ƿ�
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
VOID  __aioDeleteWaitChain (AIO_WAIT_CHAIN  *paiowt)
{
    struct aiowait     *paiowait;
    PLW_LIST_LINE       plineTemp;

    for (plineTemp  = paiowt->aiowc_pline;
         plineTemp != LW_NULL;
         plineTemp  = _list_line_get_next(plineTemp)) {
        
        paiowait = _LIST_ENTRY(plineTemp, struct aiowait, aiowt_line);
        __aioDetachWait(LW_NULL, paiowait);
    }

    if (paiowt->aiowc_paiowait->aiowt_pcond) {
        API_ThreadCondDestroy(&paiowt->aiowc_cond);
    }
    
    __SHEAP_FREE(paiowt->aiowc_paiowait);
    __SHEAP_FREE(paiowt);
}
/*********************************************************************************************************
** ��������: __aioDeleteWaitChainByAgent
** ��������: �����߳�ɾ��һ���ȴ�����, ���ﲻ����Ҫ __aioDetachWait.  (����������� _G_aioqueue)
** �䡡��  : paiowt        �ȴ����п��ƿ�
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static VOID  __aioDeleteWaitChainByAgent (AIO_WAIT_CHAIN  *paiowt)
{
    if (paiowt->aiowc_paiowait->aiowt_pcond) {
        API_ThreadCondDestroy(&paiowt->aiowc_cond);
    }
    
    __SHEAP_FREE(paiowt->aiowc_paiowait);
    __SHEAP_FREE(paiowt);
}
/*********************************************************************************************************
** ��������: __aioWaitChainSetCnt
** ��������: ����һ���ȴ��������г�Ա�ļ�������ַ (����������� _G_aioqueue)
** �䡡��  : paiowt        �ȴ����п��ƿ�
**           piCnt         ��������ַ
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
VOID  __aioWaitChainSetCnt (AIO_WAIT_CHAIN  *paiowt, INT  *piCnt)
{
    struct aiowait     *paiowait;
    PLW_LIST_LINE       plineTemp;
    
    for (plineTemp  = paiowt->aiowc_pline;
         plineTemp != LW_NULL;
         plineTemp  = _list_line_get_next(plineTemp)) {
        
        paiowait = _LIST_ENTRY(plineTemp, struct aiowait, aiowt_line);
        paiowait->aiowt_pcnt = piCnt;
    }
}
/*********************************************************************************************************
** ��������: __aioNotifyWrapper
** ��������: SIGEV_THREAD �����ź���Ǻ���
** �䡡��  : hThread       Ŀ���߳�
**           psigevent     �ź��¼�
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
#if LW_CFG_SIGNAL_EN > 0

static PVOID  __aioNotifyWrapper (PVOID  pvArg)
{
    VOIDFUNCPTR                  pfunc;
    union sigval                 sigvalue;
    struct sigevent_notify_arg  *psna;

    psna     = (struct sigevent_notify_arg *)pvArg;
    pfunc    = psna->SNA_pfunc;
    sigvalue = psna->SNA_sigvalue;
    __SHEAP_FREE(psna);

    pfunc(sigvalue);
    return  (LW_NULL);
}
/*********************************************************************************************************
** ��������: __aioSigevent
** ��������: ��������ź�
** �䡡��  : hThread       Ŀ���߳�
**           psigevent     �ź��¼�
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
VOID  __aioSigevent (LW_OBJECT_HANDLE  hThread, struct sigevent  *psigevent)
{
    pthread_t                    tid;
    pthread_attr_t               attr, *pattr;
    struct sigevent_notify_arg  *psna;

    if ((psigevent->sigev_notify == SIGEV_THREAD) &&
        (psigevent->sigev_notify_function)) {
        pattr = (pthread_attr_t *)psigevent->sigev_notify_attributes;
        if (!pattr) {
            pthread_attr_init(&attr);
            pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
            pattr = &attr;
        }

        psna = (struct sigevent_notify_arg *)__SHEAP_ALLOC(sizeof(struct sigevent_notify_arg));
        if (!psna) {
            return;
        }

        psna->SNA_pfunc    = psigevent->sigev_notify_function;
        psna->SNA_sigvalue = psigevent->sigev_value;

        if (pthread_create(&tid, pattr, __aioNotifyWrapper, psna)) {
            __SHEAP_FREE(psna);
            return;
        }

    } else if ((psigevent->sigev_notify & SIGEV_NOTIFY_MASK) == SIGEV_SIGNAL) {
        _doSigEvent(hThread, psigevent, SI_ASYNCIO);                    /*  notify                      */
    }
}

#endif                                                                  /*  LW_CFG_SIGNAL_EN > 0        */
/*********************************************************************************************************
** ��������: __aioThread
** ��������: aio �����߳�
** �䡡��  : pvArg         ����
** �䡡��  : ��ʵ������
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static PVOID  __aioThread (PVOID  pvArg)
{
    AIO_REQ_CHAIN   *paiorc = (AIO_REQ_CHAIN *)pvArg;
    struct aioreq   *paioreq;
    struct aiocb    *paiocb;
    ULONG            ulError;

    for (;;) {
        API_SemaphoreMPend(paiorc->aiorc_mutex, LW_OPTION_WAIT_INFINITE);
        
        if ((paiorc->aiorc_plineaiocb) &&
            (paiorc->aiorc_iscancel == 0)) {                            /*  paiorc �Ƿ��д�����Ľڵ�   */
            
            paioreq = _LIST_ENTRY(paiorc->aiorc_plineaiocb, 
                                  struct aioreq, aioreq_line);
            paiocb  = _LIST_ENTRY(paioreq, struct aiocb, aio_req);
            
            API_ThreadSetPriority(API_ThreadIdSelf(), 
                                  paioreq->aioreq_prio);                /*  �����������ȼ�������������ͬ*/
            
            paioreq->aioreq_flags |= AIO_REQ_BUSY;                      /*  ׼����ʼ�����ļ�            */
            
            API_SemaphoreMPost(paiorc->aiorc_mutex);
            
            /*
             *  ʵ�ʲ����ļ�
             */
            errno = 0;                                                  /*  ע��: һ��Ҫ�� errno ����   */
            if (paiocb->aio_lio_opcode == LIO_SYNC) {
                paioreq->aioreq_return =  fsync(paiocb->aio_fildes);

            } else {
                switch (paiocb->aio_lio_opcode) {
                
                case LIO_READ:
                    paioreq->aioreq_return = pread(paiocb->aio_fildes,
                                                   (void *)paiocb->aio_buf,
                                                   paiocb->aio_nbytes,
                                                   paiocb->aio_offset);
                    break;
                
                case LIO_WRITE:
                    paioreq->aioreq_return = pwrite(paiocb->aio_fildes,
                                                    (const void *)paiocb->aio_buf,
                                                    paiocb->aio_nbytes,
                                                    paiocb->aio_offset);
                    break;
                    
                default:
                    paioreq->aioreq_return = PX_ERROR;
                    errno = EINVAL;
                }
            }
            paioreq->aioreq_error = errno;
            
            API_SemaphoreMPend(paiorc->aiorc_mutex, LW_OPTION_WAIT_INFINITE);

            if (paiorc->aiorc_iscancel) {
                paioreq->aioreq_return = PX_ERROR;
                paioreq->aioreq_error  = ECANCELED;
                paioreq->aioreq_flags &= ~AIO_REQ_BUSY;                 /*  ���æ��־                  */
                API_SemaphoreMPost(paiorc->aiorc_mutex);
                continue;
            }

            _List_Line_Del(paiorc->aiorc_plineaiocb,
                           &paiorc->aiorc_plineaiocb);                  /*  ������ڵ�� paiorc ��ɾ��  */

            API_SemaphoreMPost(paiorc->aiorc_mutex);

            if (__issig(paiocb->aio_sigevent.sigev_signo) &&
                (paiocb->aio_sigevent.sigev_notify != SIGEV_NONE)) {
#if LW_CFG_SIGNAL_EN > 0
                __aioSigevent(paioreq->aioreq_thread, &paiocb->aio_sigevent);
#endif                                                                  /*  LW_CFG_SIGNAL_EN > 0        */
            }
            
            /*
             *  ����ȴ���Ϣ, 
             */
            if (paiocb->aio_pwait) {                                    /*  ��Ҫ���� wait ��Ϣ          */
                API_SemaphoreMPend(_G_aioqueue.aioq_mutex, LW_OPTION_WAIT_INFINITE);
                if (paiocb->aio_pwait) {                                /*  ��Ҫ�ټ��һ��              */
                    struct aiowait  *paiowait = paiocb->aio_pwait;
                    
                    __aioDetachWait(paiocb, LW_NULL);                   /*  ȡ�� paiowait �� paiocb ����*/
                    
                    if (paiowait->aiowt_pcnt) {
                        (*paiowait->aiowt_pcnt)--;                      /*  ������--                    */
                    }
                    
                    if (paiowait->aiowt_pcond) {
                        API_ThreadCondSignal(paiowait->aiowt_pcond);    /*  ���ﲻ��Ҫ���� wait         */
                    
                    } else {
                        if ((paiowait->aiowt_pcnt) &&
                            (*paiowait->aiowt_pcnt == 0)) {             /*  ��Ҫ���� wait ����          */
                            
                            AIO_WAIT_CHAIN *paiowt = (AIO_WAIT_CHAIN *)paiowait->aiowt_paiowc;
                            
                            if (paiowait->aiowt_psigevent &&
                                __issig(paiowait->aiowt_psigevent->sigev_signo)) {
#if LW_CFG_SIGNAL_EN > 0
                                __aioSigevent(paioreq->aioreq_thread, paiowait->aiowt_psigevent);
#endif                                                                  /*  LW_CFG_SIGNAL_EN > 0        */
                            }
                            
                            if (paiowt) {
                                __aioDeleteWaitChainByAgent(paiowt);    /*  �ͷ� wait ����              */
                            }
                        }
                    }
                }
                API_SemaphoreMPost(_G_aioqueue.aioq_mutex);
            }
            
            paioreq->aioreq_flags &= ~AIO_REQ_BUSY;                     /*  ���æ��־                  */
        
        } else if (paiorc->aiorc_iscancel) {                            /*  �� cancel ��                */
        
            API_SemaphoreMPost(paiorc->aiorc_mutex);
            
            /*
             *  paiorc ���屻 cancel ��, ������Ҫɾ�� paiorc.
             */
            API_SemaphoreMPend(_G_aioqueue.aioq_mutex, LW_OPTION_WAIT_INFINITE);
            if (paiorc->aiorc_isworkq) {
                __aioDeleteFd(paiorc, &_G_aioqueue.aioq_plinework);
            } else {
                __aioDeleteFd(paiorc, &_G_aioqueue.aioq_plineidle);
            }
            goto    __check_idle;                                       /*  ��� idle ������û�� aiorc  */
        
        } else {                                                        /*  ���ļ�û����Ҫ���������    */
            if (API_ThreadCondWait(&paiorc->aiorc_cond, 
                                   paiorc->aiorc_mutex,
                                   __PX_AIO_TIMEOUT) == ERROR_NONE) {   /*  �ٵȴ�һ��ʱ��              */
                API_SemaphoreMPost(paiorc->aiorc_mutex);
                continue;                                               /*  ������������ļ�������      */

            } else {
                API_SemaphoreMPost(paiorc->aiorc_mutex);
            }
            
            API_SemaphoreMPend(_G_aioqueue.aioq_mutex, LW_OPTION_WAIT_INFINITE);
            if ((paiorc->aiorc_plineaiocb == LW_NULL) ||
                (paiorc->aiorc_iscancel)) {                             /*  ��Ҫɾ�� paiorc             */
                
                if (paiorc->aiorc_isworkq) {
                    __aioDeleteFd(paiorc, &_G_aioqueue.aioq_plinework);
                } else {
                    __aioDeleteFd(paiorc, &_G_aioqueue.aioq_plineidle);
                }
                
__check_idle:
                if (_G_aioqueue.aioq_plineidle == LW_NULL) {            /*  aioq_plineidle ��Ҫִ��     */
                    
                    _G_aioqueue.aioq_idlethread++;                      /*  ���� idle ģʽ              */
                    ulError = API_ThreadCondWait(&_G_aioqueue.aioq_newreq,
                                                 _G_aioqueue.aioq_mutex,
                                                 __PX_AIO_TIMEOUT);
                    _G_aioqueue.aioq_idlethread--;
                    
                    if ((ulError != ERROR_NONE) || 
                        (_G_aioqueue.aioq_plineidle == LW_NULL)) {      /*  û����Ҫִ�е� I/O ����     */
                        _G_aioqueue.aioq_actthread--;
                        API_SemaphoreMPost(_G_aioqueue.aioq_mutex);
                        return  (LW_NULL);
                    }
                        
                    paiorc = _LIST_ENTRY(_G_aioqueue.aioq_plineidle, 
                                         AIO_REQ_CHAIN, 
                                         aiorc_line);                   /*  ��ȡ�µ� paiorc             */
                    
                    _List_Line_Del(&paiorc->aiorc_line, 
                                   &_G_aioqueue.aioq_plineidle);        /*  �� idle ��ɾ��              */
                    
                    paiorc->aiorc_isworkq = 1;                          /*  ������� aioq_plinework     */
                    _List_Line_Add_Ahead(&paiorc->aiorc_line, 
                                         &_G_aioqueue.aioq_plinework);
                }
            }
            API_SemaphoreMPost(_G_aioqueue.aioq_mutex);
        }
    }

    return  (LW_NULL);
}
/*********************************************************************************************************
** ��������: __aioEnqueue
** ��������: �� aio һ������ڵ����ִ�ж��� (����������� _G_aioqueue)
** �䡡��  : paiocb            aio �������
** �䡡��  : ERROR or OK
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
int  __aioEnqueue (struct aiocb *paiocb)
{
    LW_CLASS_THREADATTR     threadattr;
    AIO_REQ_CHAIN          *paiorc;
    ULONG                   ulError;
    
    API_ThreadGetPriority(API_ThreadIdSelf(), &paiocb->aio_req.aioreq_prio);
    paiocb->aio_req.aioreq_flags  = 0;
    paiocb->aio_req.aioreq_error  = EINPROGRESS;
    paiocb->aio_req.aioreq_return = 0;
    
    if ((_G_aioqueue.aioq_idlethread == 0) && 
        (_G_aioqueue.aioq_actthread < LW_CFG_POSIX_AIO_MAX_THREAD)) {   /*  û�п�������, ���ܴ���������*/
        
        paiorc = __aioSearchFd(_G_aioqueue.aioq_plinework, 
                               paiocb->aio_fildes);                     /*  ����������������û����ͬ��  */
        if (paiorc == LW_NULL) {
            paiorc =  __aioCreateFd(&_G_aioqueue.aioq_plinework, 
                                    paiocb->aio_fildes);                /*  �����ڵ�                    */
            if (paiorc == LW_NULL) {
                paiocb->aio_req.aioreq_error  = EAGAIN;
                paiocb->aio_req.aioreq_return = PX_ERROR;
                errno = ENOMEM;                                         /*  ϵͳȱ���ڴ��˳�            */
                return  (PX_ERROR);
            }
            
            paiorc->aiorc_isworkq = 1;                                  /*  �� aioq_plinework ��        */
            __aioInsertAiocb(paiorc, paiocb);                           /*  �����ļ�����������          */
            
            threadattr = _G_threadattrAio;
            threadattr.THREADATTR_ucPriority = paiocb->aio_req.aioreq_prio;
            threadattr.THREADATTR_pvArg      = (PVOID)paiorc;
            
            ulError = API_ThreadCreate("t_aio", __aioThread, &threadattr, LW_NULL);
            if (ulError == LW_OBJECT_HANDLE_INVALID) {
                return  (PX_ERROR);                                     /*  ϵͳ�޷������µ��߳�        */
            }
            _G_aioqueue.aioq_actthread++;
        
        } else {                                                        /*  �Ѿ�������Ӧ�ļ�����������  */
            API_SemaphoreMPend(paiorc->aiorc_mutex, LW_OPTION_WAIT_INFINITE);
            __aioInsertAiocb(paiorc, paiocb);                           /*  �����ļ�����������          */
            API_ThreadCondSignal(&paiorc->aiorc_cond);                  /*  ֪ͨ�߳�                    */
            API_SemaphoreMPost(paiorc->aiorc_mutex);
        }
    
    } else {                                                            /*  ���ڿ���������߳�����      */
        
        paiorc = __aioSearchFd(_G_aioqueue.aioq_plinework, 
                               paiocb->aio_fildes);                     /*  ����������������û����ͬ��  */
        if (paiorc == LW_NULL) {
            paiorc = __aioSearchFd(_G_aioqueue.aioq_plineidle,
                                   paiocb->aio_fildes);                 /*  �� idlethread �в���        */
            if (paiorc == LW_NULL) {
                paiorc =  __aioCreateFd(&_G_aioqueue.aioq_plineidle,
                                        paiocb->aio_fildes);            /*  �� idlethread ����          */
                if (paiorc == LW_NULL) {
                    paiocb->aio_req.aioreq_error  = EAGAIN;
                    paiocb->aio_req.aioreq_return = PX_ERROR;
                    errno = ENOMEM;                                     /*  ϵͳȱ���ڴ��˳�            */
                    return  (PX_ERROR);
                }
                
                paiorc->aiorc_isworkq = 0;                              /*  û���� aioq_plinework ��    */
                __aioInsertAiocb(paiorc, paiocb);                       /*  �����ļ�����������          */
                
                API_ThreadCondSignal(&_G_aioqueue.aioq_newreq);         /*  ֪ͨ idle ���߳�ȥִ��      */
            
            } else {                                                    /*  idlethread ���ж�Ӧ�� fd    */
                API_SemaphoreMPend(paiorc->aiorc_mutex, LW_OPTION_WAIT_INFINITE);
                __aioInsertAiocb(paiorc, paiocb);                       /*  �����ļ�����������          */
                API_SemaphoreMPost(paiorc->aiorc_mutex);
            }
        
        } else {
            API_SemaphoreMPend(paiorc->aiorc_mutex, LW_OPTION_WAIT_INFINITE);
            __aioInsertAiocb(paiorc, paiocb);                           /*  �����ļ�����������          */
            API_ThreadCondSignal(&paiorc->aiorc_cond);                  /*  ֪ͨ�߳�                    */
            API_SemaphoreMPost(paiorc->aiorc_mutex);
        }
    }
    
    return  (ERROR_NONE);
}

#endif                                                                  /*  LW_CFG_POSIX_EN > 0         */
                                                                        /*  LW_CFG_POSIX_AIO_EN > 0     */
/*********************************************************************************************************
  END
*********************************************************************************************************/
