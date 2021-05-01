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
** ��   ��   ��: aio.c
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2011 �� 02 �� 26 ��
**
** ��        ��: posix �첽I/O���ݿ�.
** ע        ��: �� mutex ���� aiorc ��, ���������� aioqueue, ������ܲ�������.

** BUG:
2013.02.22  ���ڲ�ͬ�Ľ����в�ͬ���ļ��������ռ�, �û�������Ҫʹ�� aio �������� cextern �ⲿ aio ��. 
2013.11.28  aio_suspend() �������ȡ����.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../include/px_aio.h"                                          /*  �Ѱ�������ϵͳͷ�ļ�        */
/*********************************************************************************************************
  �ü�֧��
*********************************************************************************************************/
#if LW_CFG_POSIX_EN > 0 && LW_CFG_POSIX_AIO_EN > 0
#include "aio_lib.h"
/*********************************************************************************************************
  aio sigevent notify
*********************************************************************************************************/
#if LW_CFG_SIGNAL_EN > 0
VOID  __aioSigevent(LW_OBJECT_HANDLE  hThread, struct sigevent  *psigevent);
#endif
/*********************************************************************************************************
** ��������: __aioWaitCleanup
** ��������: ���� wait ����
** �䡡��  : paiowt            paiowait ����ͷ
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static VOID  __aioWaitCleanup (AIO_WAIT_CHAIN  *paiowt)
{
    API_SemaphoreMPend(_G_aioqueue.aioq_mutex, LW_OPTION_WAIT_INFINITE);
    __aioDeleteWaitChain(paiowt);
    API_SemaphoreMPost(_G_aioqueue.aioq_mutex);
}
/*********************************************************************************************************
** ��������: aio_suspend
** ��������: �ȴ�ָ����һ�������첽 I/O ����������
** �䡡��  : list              aio ������ƿ����� (��������� NULL, ���򽫺���)
**           nent              ����Ԫ�ظ���
**           timeout           ��ʱʱ��
** �䡡��  : ERROR or OK
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API 
int  aio_suspend (const struct aiocb * const list[], int nent, const struct timespec *timeout)
{
    AIO_WAIT_CHAIN     *paiowc;
    struct aiowait     *paiowait;
    ULONG               ulTimeout;
    
    BOOL                bNeedWait = LW_FALSE;
    
    int                 iWait = 0;                                      /*  paiowait �±�               */
    int                 iCnt;                                           /*  list �±�                   */
    int                 iRet = ERROR_NONE;
    
#ifdef __SYLIXOS_KERNEL
    if (__PROC_GET_PID_CUR() != 0) {                                    /*  ��Ҫʹ���ⲿ���ṩ�� aio    */
        errno = ENOSYS;
        return  (PX_ERROR);
    }
#endif                                                                  /*  __SYLIXOS_KERNEL            */

    __THREAD_CANCEL_POINT();                                            /*  ����ȡ����                  */

    if ((nent <= 0) || (list == LW_NULL)) {
        errno = EINVAL;
        return  (PX_ERROR);
    }
    
    if (timeout == LW_NULL) {                                           /*  ���õȴ�                    */
        ulTimeout = LW_OPTION_WAIT_INFINITE;
    } else {
        ulTimeout = __timespecToTick(timeout);
    }
    
    paiowc = __aioCreateWaitChain(nent, LW_TRUE);                       /*  �����ȴ�����                */
    if (paiowc == LW_NULL) {
        errno = ENOMEM;                                                 /*  should be EAGAIN ?          */
        return  (PX_ERROR);
    }
    
    paiowait = paiowc->aiowc_paiowait;
    
    API_SemaphoreMPend(_G_aioqueue.aioq_mutex, LW_OPTION_WAIT_INFINITE);
    
    for (iCnt = 0; iCnt < nent; iCnt++) {
        if (list[iCnt]) {
            if (list[iCnt]->aio_req.aioreq_error == EINPROGRESS) {      /*  ����Ϊ���ڴ���״̬          */
                
                paiowait[iWait].aiowt_pcond     = &paiowc->aiowc_cond;
                paiowait[iWait].aiowt_pcnt      = LW_NULL;
                paiowait[iWait].aiowt_psigevent = LW_NULL;
                paiowait[iWait].aiowt_paiowc    = LW_NULL;              /*  ����Ҫ�����߳��ͷ�          */
                
                iRet = __aioAttachWait(list[iCnt], &paiowait[iWait]);
                if (iRet != ERROR_NONE) {
                    errno = EINVAL;
                    break;
                }
                
                _List_Line_Add_Tail(&paiowait[iWait].aiowt_line, 
                                    &paiowc->aiowc_pline);              /*  ���� wait ����              */
                iWait++;
                
                bNeedWait = LW_TRUE;                                    /*  �� wait ����ڵ�����,       */
            } else {
                break;
            }
        }
    }
    
    if (iRet != ERROR_NONE) {                                           /*  __aioAttachWait ʧ��        */
        __aioDeleteWaitChain(paiowc);
        API_SemaphoreMPost(_G_aioqueue.aioq_mutex);
        errno = EINVAL;
        return  (PX_ERROR);
    }
    
    if ((iCnt == nent) && bNeedWait) {                                  /*  �Ƿ���Ҫ�ȴ�                */
        API_ThreadCleanupPush(__aioWaitCleanup, paiowc);
        
        iRet = (INT)API_ThreadCondWait(&paiowc->aiowc_cond, _G_aioqueue.aioq_mutex, ulTimeout);
        
        API_ThreadCleanupPop(LW_FALSE);                                 /*  �����Ѿ����� _G_aioqueue    */
                                                                        /*  �������� __aioWaitCleanup   */
        __aioDeleteWaitChain(paiowc);
        
        if (iRet != ERROR_NONE) {
            API_SemaphoreMPost(_G_aioqueue.aioq_mutex);
            errno = EAGAIN;
            return  (PX_ERROR);
        }
    
    } else {                                                            /*  ����Ҫ�ȴ�                  */
        __aioDeleteWaitChain(paiowc);
    }
    
    API_SemaphoreMPost(_G_aioqueue.aioq_mutex);
    
    return  (iRet);
}
/*********************************************************************************************************
** ��������: lio_listio
** ��������: ͬʱ���������� mode ���������� LIO_WAIT �� LIO_NOWAIT��LIO_WAIT �������������,
             ֱ�����е� I/O �����Ϊֹ. �ڲ��������Ŷ�֮��LIO_NOWAIT �ͻ᷵��. 
             sigevent ���ö����������� I/O ���������ʱ�����źŵķ�����
** �䡡��  : mode              LIO_WAIT or LIO_NOWAIT��LIO_WAIT
**           list              aio ������ƿ����� (��������� NULL, ���򽫺���)
**           nent              ����Ԫ�ظ���
**           sig               ���� I/O ���������ʱ�����źŵķ���
                               LIO_WAIT ������Դ˲���!
** �䡡��  : ERROR or OK
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API 
int  lio_listio (int mode, struct aiocb * const list[], int nent, struct sigevent *sig)
{
    AIO_WAIT_CHAIN     *paiowc;
    struct aiowait     *paiowait;
    
    int                 iNotify;
    int                 iWait = 0;                                      /*  paiowait �±�               */
    int                 iCnt;                                           /*  list �±�                   */
    int                 iRet = ERROR_NONE;
    
#ifdef __SYLIXOS_KERNEL
    if (__PROC_GET_PID_CUR() != 0) {                                    /*  ��Ҫʹ���ⲿ���ṩ�� aio    */
        errno = ENOSYS;
        return  (PX_ERROR);
    }
#endif                                                                  /*  __SYLIXOS_KERNEL            */
    
    if ((mode != LIO_WAIT) && (mode != LIO_NOWAIT)) {
        errno = EINVAL;
        return  (PX_ERROR);
    }
    
    if ((nent <= 0) || (list == LW_NULL)) {
        errno = EINVAL;
        return  (PX_ERROR);
    }
    
    if (mode == LIO_WAIT) {
        paiowc = __aioCreateWaitChain(nent, LW_TRUE);                   /*  �����ȴ�����                */
    } else {
        paiowc = __aioCreateWaitChain(nent, LW_FALSE);
    }
    if (paiowc == LW_NULL) {
        errno = ENOMEM;                                                 /*  should be EAGAIN ?          */
        return  (PX_ERROR);
    }
    
    if (sig) {
        paiowc->aiowc_sigevent = *sig;
    } else {
        paiowc->aiowc_sigevent.sigev_signo = 0;                         /*  ��Ч�źŲ�Ͷ��              */
    }
    
    paiowait = paiowc->aiowc_paiowait;
    
    API_SemaphoreMPend(_G_aioqueue.aioq_mutex, LW_OPTION_WAIT_INFINITE);
    
    for (iCnt = 0; iCnt < nent; iCnt++) {
        if (list[iCnt]) {
            if ((list[iCnt]->aio_lio_opcode != LIO_NOP) &&
                (list[iCnt]->aio_req.aioreq_error != EINPROGRESS)) {
                
                iNotify  = list[iCnt]->aio_sigevent.sigev_notify;
                iNotify &= ~SIGEV_THREAD_ID;
                if ((iNotify != SIGEV_SIGNAL) && 
                    (iNotify != SIGEV_THREAD)) {
                    list[iCnt]->aio_sigevent.sigev_notify = SIGEV_NONE;
                }
                
                list[iCnt]->aio_req.aioreq_thread = API_ThreadIdSelf(); /*  ���յ�������ź�����        */
                
                if (mode == LIO_WAIT) {
                    paiowait[iWait].aiowt_pcond     = &paiowc->aiowc_cond;
                    paiowait[iWait].aiowt_pcnt      = LW_NULL;
                    paiowait[iWait].aiowt_psigevent = LW_NULL;
                    paiowait[iWait].aiowt_paiowc    = LW_NULL;          /*  ����Ҫ�����߳��ͷ�          */
                
                } else {
                    paiowait[iWait].aiowt_pcond     = LW_NULL;
                    paiowait[iWait].aiowt_pcnt      = LW_NULL;
                    paiowait[iWait].aiowt_psigevent = &paiowc->aiowc_sigevent;
                    paiowait[iWait].aiowt_paiowc    = (void *)paiowc;
                }
                
                __aioAttachWaitNoCheck(list[iCnt], &paiowait[iWait]);   /*  aiocb and aiowait attach    */
                
                if (__aioEnqueue(list[iCnt]) == ERROR_NONE) {
                    _List_Line_Add_Tail(&paiowait[iWait].aiowt_line, 
                                        &paiowc->aiowc_pline);          /*  ���� wait ����              */
                    iWait++;
                
                } else {
                    __aioDetachWait(LW_NULL, &paiowait[iWait]);         /*  ɾ��������ϵ                */
                    
                    iRet = PX_ERROR;
                }
            }
        }
    }
    
    if (iWait == 0) {                                                   /*  û��һ���ڵ�������        */
        __aioDeleteWaitChain(paiowc);
        API_SemaphoreMPost(_G_aioqueue.aioq_mutex);
        
        if (mode == LIO_NOWAIT) {
            if (sig && __issig(sig->sigev_signo)) {
#if LW_CFG_SIGNAL_EN > 0
                __aioSigevent(API_ThreadIdSelf(), sig);
#endif                                                                  /*  LW_CFG_SIGNAL_EN > 0        */
            }
        }
        return  (ERROR_NONE);
    
    } else if (mode == LIO_WAIT) {                                      /*  ��Ҫ�ȴ�                    */
        __aioWaitChainSetCnt(paiowc, &iWait);                           /*  ���ü�������                */
        
        while (iWait > 0) {
            if (API_ThreadCondWait(&paiowc->aiowc_cond, 
                                   _G_aioqueue.aioq_mutex, 
                                   LW_OPTION_WAIT_INFINITE)) {          /*  �ȴ���������ִ�����        */
                break;
            }
        }
        
        __aioDeleteWaitChain(paiowc);                                   /*  ��� wait ����              */
        
    } else {                                                            /*  LIO_NOWAIT                  */
        paiowc->aiowc_icnt = iWait;                                     /*  ����ʹ��ȫ�ֱ���            */
        
        __aioWaitChainSetCnt(paiowc, &paiowc->aiowc_icnt);              /*  ���ü�������                */
    }
    
    API_SemaphoreMPost(_G_aioqueue.aioq_mutex);
                
    return  (iRet);
}
/*********************************************************************************************************
** ��������: aio_cancel
** ��������: ȡ��һ�� aio ����
** �䡡��  : fildes            �ļ�������
**           paiocb            aio ������ƿ� (����Ϊ NULL)
** �䡡��  : ERROR or OK
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API 
int  aio_cancel (int fildes, struct aiocb *paiocb)
{
    AIO_REQ_CHAIN          *paiorc;
    INT                     iRet;
    
#ifdef __SYLIXOS_KERNEL
    if (__PROC_GET_PID_CUR() != 0) {                                    /*  ��Ҫʹ���ⲿ���ṩ�� aio    */
        errno = ENOSYS;
        return  (PX_ERROR);
    }
#endif                                                                  /*  __SYLIXOS_KERNEL            */

    if (iosFdGetName(fildes, LW_NULL, 0)) {                             /*  ����� errno �Զ���Ϊ EBADF */
        return  (PX_ERROR);
    }

    API_SemaphoreMPend(_G_aioqueue.aioq_mutex, LW_OPTION_WAIT_INFINITE);
    
    if (paiocb == LW_NULL) {
        /*
         *  ����� cancel �����ļ�������, ��ô������� __aioCancelFd() ����, __aioCancelFd() �ڲ�������
         *  �ȴ������Ľڵ��ͷ�, Ȼ��֪ͨ�����߳�ɾ�� paiorc �ṹ, �������ȫ�Ĳ�����ʽ, �������ֱ��ɾ��
         *  ��ô���ڲ����� paiorc ���ܻ���Σ��, 
         *  ��Ӧ�ò�Ƕ��ǿ�����������̵�! ���Ǳ�֤�ں�������ǰ, �����е� aiocb �������!
         */
        paiorc = __aioSearchFd(_G_aioqueue.aioq_plinework, 
                               fildes);                                 /*  ����������������û����ͬ��  */
        if (paiorc == LW_NULL) {
            if (_G_aioqueue.aioq_plineidle) {
                paiorc = __aioSearchFd(_G_aioqueue.aioq_plineidle, 
                                       fildes);                         /*  �������ж���                */
                if (paiorc == LW_NULL) {
                    API_SemaphoreMPost(_G_aioqueue.aioq_mutex);
                    return  (AIO_ALLDONE);
                
                } else {
                    __aioCancelFd(paiorc);                              /*  ɾ�����нڵ��֪ͨ����ɾ��  */
                    API_SemaphoreMPost(_G_aioqueue.aioq_mutex);
                    return  (AIO_CANCELED);
                }
            
            } else {
                API_SemaphoreMPost(_G_aioqueue.aioq_mutex);
                return  (AIO_ALLDONE);
            }
        
        } else {
            __aioCancelFd(paiorc);                                      /*  ɾ�����нڵ��֪ͨ����ɾ��  */
            API_SemaphoreMPost(_G_aioqueue.aioq_mutex);
            return  (AIO_CANCELED);
        }
    
    } else {
        if (paiocb->aio_fildes != fildes) {
            API_SemaphoreMPost(_G_aioqueue.aioq_mutex);
            errno = EINVAL;
            return  (PX_ERROR);
        }
        
        paiorc = __aioSearchFd(_G_aioqueue.aioq_plinework, 
                               fildes);                                 /*  ����������������û����ͬ��  */
        if (paiorc == LW_NULL) {
            if (_G_aioqueue.aioq_plineidle) {
                paiorc = __aioSearchFd(_G_aioqueue.aioq_plineidle, 
                                       fildes);                         /*  �������ж���                */
                if (paiorc == LW_NULL) {
                    API_SemaphoreMPost(_G_aioqueue.aioq_mutex);
                    return  (AIO_ALLDONE);
                
                } else {
                    API_SemaphoreMPend(paiorc->aiorc_mutex, LW_OPTION_WAIT_INFINITE);
                    iRet = __aioRemoveAiocb(paiorc, paiocb, 
                                            PX_ERROR, ECANCELED);       /*  �Ƴ� aiocb                  */
                    API_SemaphoreMPost(paiorc->aiorc_mutex);
                    API_SemaphoreMPost(_G_aioqueue.aioq_mutex);
                    return  (iRet);
                }
            
            } else {
                API_SemaphoreMPost(_G_aioqueue.aioq_mutex);
                return  (AIO_ALLDONE);
            }
        
        } else {
            API_SemaphoreMPend(paiorc->aiorc_mutex, LW_OPTION_WAIT_INFINITE);
            iRet = __aioRemoveAiocb(paiorc, paiocb,
                                    PX_ERROR, ECANCELED);               /*  �Ƴ� aiocb                  */
            API_SemaphoreMPost(paiorc->aiorc_mutex);
            API_SemaphoreMPost(_G_aioqueue.aioq_mutex);
            return  (iRet);
        }
    }
    
    API_SemaphoreMPost(_G_aioqueue.aioq_mutex);
    return  (AIO_ALLDONE);
}
/*********************************************************************************************************
** ��������: aio_error
** ��������: ��� aio ������ɺ�� errno
** �䡡��  : paiocb            aio ������ƿ�
** �䡡��  : ��������ֵ
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API 
int  aio_error (const struct aiocb *paiocb)
{
#ifdef __SYLIXOS_KERNEL
    if (__PROC_GET_PID_CUR() != 0) {                                    /*  ��Ҫʹ���ⲿ���ṩ�� aio    */
        errno = ENOSYS;
        return  (PX_ERROR);
    }
#endif                                                                  /*  __SYLIXOS_KERNEL            */
    
    if (!paiocb) {
        errno = EINVAL;
        return  (PX_ERROR);
    }
    
    if (!_ObjectClassOK(paiocb->aio_req.aioreq_thread, _OBJECT_THREAD)) {
        errno = EINVAL;
        return  (EINVAL);
    }
    
    return  (paiocb->aio_req.aioreq_error);
}
/*********************************************************************************************************
** ��������: aio_return
** ��������: ��� aio ������ɺ�ķ���ֵ
** �䡡��  : paiocb            aio ������ƿ�
** �䡡��  : ��������ֵ
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API 
ssize_t  aio_return (struct aiocb *paiocb)
{
    ssize_t  ret;

#ifdef __SYLIXOS_KERNEL
    if (__PROC_GET_PID_CUR() != 0) {                                    /*  ��Ҫʹ���ⲿ���ṩ�� aio    */
        errno = ENOSYS;
        return  (PX_ERROR);
    }
#endif                                                                  /*  __SYLIXOS_KERNEL            */
    
    if (!paiocb) {
        errno = EINVAL;
        return  (PX_ERROR);
    }
    
    if (!_ObjectClassOK(paiocb->aio_req.aioreq_thread, _OBJECT_THREAD)) {
        errno = EINVAL;
        return  (PX_ERROR);
    }
    
    if (paiocb->aio_req.aioreq_flags & AIO_REQ_FREE) {
        errno = EINVAL;
        return  (PX_ERROR);
    }
    
    if (paiocb->aio_req.aioreq_error == EINPROGRESS) {
        errno = EINPROGRESS;
        return  (PX_ERROR);
    }
    
    ret = paiocb->aio_req.aioreq_return;
    
    paiocb->aio_req.aioreq_return = PX_ERROR;
    paiocb->aio_req.aioreq_flags |= AIO_REQ_FREE;
    
    return  (ret);
}
/*********************************************************************************************************
** ��������: aio_read
** ��������: aio ��ȡ�ļ�����
** �䡡��  : paiocb            aio ������ƿ�
** �䡡��  : ERROR or OK
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API 
int  aio_read (struct aiocb *paiocb)
{
    INT     iRet;
    
#ifdef __SYLIXOS_KERNEL
    if (__PROC_GET_PID_CUR() != 0) {                                    /*  ��Ҫʹ���ⲿ���ṩ�� aio    */
        errno = ENOSYS;
        return  (PX_ERROR);
    }
#endif                                                                  /*  __SYLIXOS_KERNEL            */

    if (!paiocb) {
        errno = EINVAL;
        return  (PX_ERROR);
    }
    
    if ((paiocb->aio_offset < 0) ||
        (!paiocb->aio_buf) ||
        (!paiocb->aio_nbytes)) {
        errno = EINVAL;
        return  (PX_ERROR);
    }
    
    if (iosFdGetName(paiocb->aio_fildes, LW_NULL, 0)) {                 /*  ����� errno �Զ���Ϊ EBADF */
        return  (PX_ERROR);
    }
    
    paiocb->aio_lio_opcode = LIO_READ;
    
    paiocb->aio_req.aioreq_thread = API_ThreadIdSelf();
    paiocb->aio_pwait = LW_NULL;
    
    API_SemaphoreMPend(_G_aioqueue.aioq_mutex, LW_OPTION_WAIT_INFINITE);
    iRet = __aioEnqueue(paiocb);
    API_SemaphoreMPost(_G_aioqueue.aioq_mutex);
    
    return  (iRet);
}
/*********************************************************************************************************
** ��������: aio_wirte
** ��������: aio д���ļ�����
** �䡡��  : paiocb            aio ������ƿ�
** �䡡��  : ERROR or OK
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API 
int  aio_write (struct aiocb *paiocb)
{
    INT     iRet;
    
#ifdef __SYLIXOS_KERNEL
    if (__PROC_GET_PID_CUR() != 0) {                                    /*  ��Ҫʹ���ⲿ���ṩ�� aio    */
        errno = ENOSYS;
        return  (PX_ERROR);
    }
#endif                                                                  /*  __SYLIXOS_KERNEL            */

    if (!paiocb) {
        errno = EINVAL;
        return  (PX_ERROR);
    }
    
    if ((paiocb->aio_offset < 0) ||
        (!paiocb->aio_buf) ||
        (!paiocb->aio_nbytes)) {
        errno = EINVAL;
        return  (PX_ERROR);
    }
    
    if (iosFdGetName(paiocb->aio_fildes, LW_NULL, 0)) {                 /*  ����� errno �Զ���Ϊ EBADF */
        return  (PX_ERROR);
    }
    
    paiocb->aio_lio_opcode = LIO_WRITE;
    
    paiocb->aio_req.aioreq_thread = API_ThreadIdSelf();
    paiocb->aio_pwait = LW_NULL;
    
    API_SemaphoreMPend(_G_aioqueue.aioq_mutex, LW_OPTION_WAIT_INFINITE);
    iRet = __aioEnqueue(paiocb);
    API_SemaphoreMPost(_G_aioqueue.aioq_mutex);
    
    return  (iRet);
}
/*********************************************************************************************************
** ��������: aio_fsync
** ��������: aio ͬ���ļ�������, �������ڲ�������д���ļ�
** �䡡��  : op                ����ѡ�� (O_SYNC or O_DSYNC)
**           paiocb            aio ������ƿ�
** �䡡��  : ERROR or OK
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API 
int  aio_fsync (int op, struct aiocb *paiocb)
{
    INT     iRet;
    
#ifdef __SYLIXOS_KERNEL
    if (__PROC_GET_PID_CUR() != 0) {                                    /*  ��Ҫʹ���ⲿ���ṩ�� aio    */
        errno = ENOSYS;
        return  (PX_ERROR);
    }
#endif                                                                  /*  __SYLIXOS_KERNEL            */

    if (!paiocb) {
        errno = EINVAL;
        return  (PX_ERROR);
    }
    
    if (iosFdGetName(paiocb->aio_fildes, LW_NULL, 0)) {                 /*  ����� errno �Զ���Ϊ EBADF */
        return  (PX_ERROR);
    }
    
    if (!_ObjectClassOK(paiocb->aio_req.aioreq_thread, _OBJECT_THREAD)) {
        errno = EINVAL;
        return  (PX_ERROR);
    }
    
    paiocb->aio_lio_opcode = LIO_SYNC;
    
    paiocb->aio_req.aioreq_thread = API_ThreadIdSelf();
    paiocb->aio_pwait = LW_NULL;
    
    API_SemaphoreMPend(_G_aioqueue.aioq_mutex, LW_OPTION_WAIT_INFINITE);
    iRet = __aioEnqueue(paiocb);
    API_SemaphoreMPost(_G_aioqueue.aioq_mutex);
    
    return  (iRet);
}

#endif                                                                  /*  LW_CFG_POSIX_EN > 0         */
                                                                        /*  LW_CFG_POSIX_AIO_EN > 0     */
/*********************************************************************************************************
  END
*********************************************************************************************************/
