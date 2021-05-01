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
** ��   ��   ��: selectLib.c
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2007 �� 11 �� 07 ��
**
** ��        ��:  IO ϵͳ select ��ϵͳ API.

** BUG
2007.11.11  �����ýڵ���ִ���ʱ. һ��Ҫ�˳�.
2007.11.20  �� pselctx->SELCTX_bPendedOnSelect = LW_TRUE; ��ǰ, ��ʹ FIOSELECT ���е���;�̱߳�ɾ��ʱ
            ����ʹ delete hook ��������, �������� WAKE NODE ɾ��.
2009.07.17  ��������ļ��Ų���Ӧ����ǰ, ��֤ delete hook �ܹ�������ɾ�������Ľڵ�.
2009.09.03  ���ȫ���ļ���, ����ע��.
2009.11.25  select() ���ж� ERROR_IO_UNKNOWN_REQUEST ʱҲҪ�ж� ENOSYS. ���������ȼ۵Ĵ�������.
2011.03.11  ȷ����ʱ���Է��ض�Ӧ�� errno ���.
2011.08.15  �ش����: ������ posix ����ĺ����Ժ�����ʽ(�Ǻ�)����.
2013.03.15  ���� pselect api.
2017.08.18  �ȴ�ʱ����Ӿ�ȷ.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
#include "../SylixOS/system/include/s_system.h"
/*********************************************************************************************************
  �ü�����
*********************************************************************************************************/
#if (LW_CFG_DEVICE_EN > 0) && (LW_CFG_SELECT_EN > 0)
#if LW_CFG_POSIX_EN > 0
#include "../SylixOS/posix/include/posixLib.h"
#endif                                                                  /*  LW_CFG_POSIX_EN > 0         */
#include "select.h"
/*********************************************************************************************************
** ��������: __selIsAllFdsetEmpty
** ��������: ����Ƿ����е��ļ�����Ϊ�� (2010.01.21 �˺�������, �ܶ����ʹ�� select �����ӳ�...)
** �䡡��  : iWidthInBytes     �ļ����е�����ļ��ŵ�ƫ����ռ�˶����ֽ�, ����: fd = 10, ��ռ�� 2 �� Byte
**           pfdsetRead        �û����ĵĿɶ��ļ���.
**           pfdsetWrite       �û����ĵĿ�д�ļ���.
**           pfdsetExcept      �û����ĵ��쳣�ļ���.
** �䡡��  : LW_TRUE  Ϊ��
**           LW_FALSE ��Ϊ��
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static BOOL  __selIsAllFdsetEmpty (INT               iWidthInBytes,
                                   fd_set           *pfdsetRead,
                                   fd_set           *pfdsetWrite,
                                   fd_set           *pfdsetExcept)
{
    (VOID)iWidthInBytes;
    
    return  (LW_FALSE);
}
/*********************************************************************************************************
** ��������: __selFdsetInit
** ��������: ��ʼ�� select context �е�ԭʼ�ļ���
** �䡡��  : iWidthInBytes     �ļ����е�����ļ��ŵ�ƫ����ռ�˶����ֽ�, ����: fd = 10, ��ռ�� 2 �� Byte
**           pfdsetRead        �û����ĵĿɶ��ļ���.
**           pfdsetWrite       �û����ĵĿ�д�ļ���.
**           pfdsetExcept      �û����ĵ��쳣�ļ���.
**           pselctx           select context
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
VOID  __selFdsetInit (INT               iWidthInBytes,
                      fd_set           *pfdsetRead,
                      fd_set           *pfdsetWrite,
                      fd_set           *pfdsetExcept,
                      LW_SEL_CONTEXT   *pselctx)
{
    /*
     *  ��ͬ�� SylixOS �汾, FD_SETSIZE ���ܲ�ͬ, ���� fd_set ��С���ܲ�ͬ, 
     *  ��������ֻ�ܲ��� iWidthInBytes λ����
     */
    if (pfdsetRead) {                                                   /*  �����ȴ������ļ���          */
        lib_bcopy(pfdsetRead, &pselctx->SELCTX_fdsetOrigReadFds,
                  iWidthInBytes);
    } else {
        lib_bzero(&pselctx->SELCTX_fdsetOrigReadFds, iWidthInBytes);
    }

    if (pfdsetWrite) {                                                  /*  �����ȴ�д���ļ���          */
        lib_bcopy(pfdsetWrite, &pselctx->SELCTX_fdsetOrigWriteFds,
                  iWidthInBytes);
    } else {
        lib_bzero(&pselctx->SELCTX_fdsetOrigWriteFds, iWidthInBytes);
    }

    if (pfdsetExcept) {                                                 /*  �����ȴ��쳣���ļ���        */
        lib_bcopy(pfdsetExcept, &pselctx->SELCTX_fdsetOrigExceptFds,
                  iWidthInBytes);
    } else {
        lib_bzero(&pselctx->SELCTX_fdsetOrigExceptFds, iWidthInBytes);
    }
}
/*********************************************************************************************************
** ��������: __selFdsetClear
** ��������: ����ļ����е�����λ
** �䡡��  : iWidthInBytes     �ļ����е�����ļ��ŵ�ƫ����ռ�˶����ֽ�, ����: fd = 10, ��ռ�� 2 �� Byte
**           pfdsetRead        �û����ĵĿɶ��ļ���.
**           pfdsetWrite       �û����ĵĿ�д�ļ���.
**           pfdsetExcept      �û����ĵ��쳣�ļ���.
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static VOID  __selFdsetClear (INT               iWidthInBytes,
                              fd_set           *pfdsetRead,
                              fd_set           *pfdsetWrite,
                              fd_set           *pfdsetExcept)
{
    /*
     *  ��ͬ�� SylixOS �汾, FD_SETSIZE ���ܲ�ͬ, ���� fd_set ��С���ܲ�ͬ, 
     *  ��������ֻ�ܲ��� iWidthInBytes λ����
     */
    if (pfdsetRead) {                                                   /*  ��յ�ǰ�ȴ��Ķ��ļ���      */
        lib_bzero(pfdsetRead, iWidthInBytes);
    }

    if (pfdsetWrite) {                                                  /*  ��յ�ǰ�ȴ���д�ļ���      */
        lib_bzero(pfdsetWrite, iWidthInBytes);
    }

    if (pfdsetExcept) {                                                 /*  ��յ�ǰ�ȴ����쳣�ļ���    */
        lib_bzero(pfdsetExcept, iWidthInBytes);
    }
}
/*********************************************************************************************************
** ��������: __selFdsetGetFileCounter
** ��������: ���ָ���ļ������ļ�������
** �䡡��  : iWidth            �ļ�����,�����ļ���.
**           pfdsetRead        �û����ĵĿɶ��ļ���.
**           pfdsetWrite       �û����ĵĿ�д�ļ���.
**           pfdsetExcept      �û����ĵ��쳣�ļ���.
** �䡡��  : �ļ������ļ�������
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static INT   __selFdsetGetFileCounter (INT               iWidth, 
                                       fd_set           *pfdsetRead,
                                       fd_set           *pfdsetWrite,
                                       fd_set           *pfdsetExcept)
{
    REGISTER INT    iFd;                                                /*  ��ʱ�ļ�������              */
    REGISTER ULONG  ulPartMask;                                         /*  ��������                    */
    REGISTER INT    iFoundCounter = 0;
    
    /*
     *  ��ͬ�� SylixOS �汾, FD_SETSIZE ���ܲ�ͬ, ���� fd_set ��С���ܲ�ͬ, 
     *  ��������ֻ�ܲ��� iWidthInBytes λ����
     */
    if (pfdsetRead) {                                                   /*  ��ÿɶ��ļ�����            */
        for (iFd = 0; iFd < iWidth; iFd++) {
            ulPartMask = pfdsetRead->fds_bits[((unsigned)iFd) / NFDBITS];
            if (ulPartMask == 0) {
                iFd += NFDBITS - 1;
            
            } else if (ulPartMask & (ULONG)(1 << (((unsigned)iFd) % NFDBITS))) {
                iFoundCounter++;
            }
        }
    }
    
    if (pfdsetWrite) {                                                  /*  ��ÿ�д�ļ�����            */
        for (iFd = 0; iFd < iWidth; iFd++) {
            ulPartMask = pfdsetWrite->fds_bits[((unsigned)iFd) / NFDBITS];
            if (ulPartMask == 0) {
                iFd += NFDBITS - 1;
            
            } else if (ulPartMask & (ULONG)(1 << (((unsigned)iFd) % NFDBITS))) {
                iFoundCounter++;
            }
        }
    }
    
    if (pfdsetExcept) {                                                 /*  ����쳣�ļ�����            */
        for (iFd = 0; iFd < iWidth; iFd++) {
            ulPartMask = pfdsetExcept->fds_bits[((unsigned)iFd) / NFDBITS];
            if (ulPartMask == 0) {
                iFd += NFDBITS - 1;
            
            } else if (ulPartMask & (ULONG)(1 << (((unsigned)iFd) % NFDBITS))) {
                iFoundCounter++;
            }
        }
    }
    
    return  (iFoundCounter);
}
/*********************************************************************************************************
** ��������: pselect
** ��������: pselect() ��· I/O ����.
** �䡡��  : iWidth            ���õ��ļ�����,�����ļ��� + 1.
**           pfdsetRead        �û����ĵĿɶ��ļ���.
**           pfdsetWrite       �û����ĵĿ�д�ļ���.
**           pfdsetExcept      �û����ĵ��쳣�ļ���.
**           ptmspecTO         �ȴ���ʱʱ��, LW_NULL ��ʾ��Զ�ȴ�.
**           sigsetMask        �ȴ�ʱ�������ź�
** �䡡��  : �������صȴ������ļ����� ,���󷵻� PX_ERROR.
**           errno == ERROR_IO_SELECT_UNSUPPORT_IN_DRIVER       ��������֧��
**           errno == ERROR_IO_SELECT_CONTEXT                   �̲߳����� context
**           errno == ERROR_IO_SELECT_WIDTH                     ����ļ��Ŵ���
**           errno == ERROR_KERNEL_IN_ISR                       ���ж��е���
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
INT     pselect (INT                     iWidth, 
                 fd_set                 *pfdsetRead,
                 fd_set                 *pfdsetWrite,
                 fd_set                 *pfdsetExcept,
                 const struct timespec  *ptmspecTO,
                 const sigset_t         *sigsetMask)
{
             INT                 iCnt;
    REGISTER INT                 iIsOk = ERROR_NONE;                    /*  ��ʼ��Ϊû�д���            */
    REGISTER INT                 iWidthInBytes;                         /*  ��Ҫ����λ��ռ�����ֽ�    */
    REGISTER ULONG               ulWaitTime;                            /*  �ȴ�ʱ��                    */
    REGISTER LW_SEL_CONTEXT     *pselctx;
             PLW_CLASS_TCB       ptcbCur;
             
#if LW_CFG_SIGNAL_EN > 0
             sigset_t            sigsetMaskOld;
#endif                                                                  /*  LW_CFG_SIGNAL_EN > 0        */
             LW_SEL_WAKEUPNODE   selwunNode;                            /*  ���ɵ� NODE ģ��            */
             ULONG               ulError;
             BOOL                bBadFd;
    
    if (LW_CPU_GET_CUR_NESTING()) {
        _ErrorHandle(ERROR_KERNEL_IN_ISR);                              /*  �������ж��е���            */
        return  (PX_ERROR);
    }
    
    __THREAD_CANCEL_POINT();                                            /*  ����ȡ����                  */
    
    LW_TCB_GET_CUR_SAFE(ptcbCur);
    
    pselctx = ptcbCur->TCB_pselctxContext;
    
    if (!pselctx) {                                                     /*  û�� select context         */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "no select context.\r\n");
        _ErrorHandle(ERROR_IO_SELECT_CONTEXT);
        return  (PX_ERROR);
    }
    
    if ((iWidth < 0) || (iWidth > FD_SETSIZE)) {                        /*  iWidth ��������             */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "iWidth out of range.\r\n");
        _ErrorHandle(ERROR_IO_SELECT_WIDTH);
        return  (PX_ERROR);
    }
    
    iWidthInBytes = __HOWMANY(iWidth, NFDBITS) * sizeof(fd_mask);       /*  ��Ҫ����λ��ռ�����ֽ�    */
    
    if (__selIsAllFdsetEmpty(iWidthInBytes, pfdsetRead, pfdsetWrite, 
                             pfdsetExcept)) {                           /*  ��������ļ����Ƿ���Ч      */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "fdset is empty.\r\n");
        _ErrorHandle(ERROR_IO_SELECT_FDSET_NULL);
        return  (PX_ERROR);
    }
    
    __THREAD_CANCEL_POINT();                                            /*  ����ȡ����                  */
    
    __selFdsetInit(iWidthInBytes, pfdsetRead, pfdsetWrite, 
                   pfdsetExcept, pselctx);                              /*  ��ʼ�� select context       */
                                                                        /*  �е�ԭʼ�ļ���              */
    __selFdsetClear(iWidthInBytes, pfdsetRead, pfdsetWrite, 
                    pfdsetExcept);                                      /*  ��ղ����ļ���              */
                                                                        
    if (!ptmspecTO) {                                                   /*  ����ȴ�ʱ��                */
        ulWaitTime = LW_OPTION_WAIT_INFINITE;                           /*  ���޵ȴ�                    */

    } else {
        ulWaitTime = LW_TS_TIMEOUT_TICK(LW_TRUE, ptmspecTO);
    }
    
    API_SemaphoreBClear(pselctx->SELCTX_hSembWakeup);                   /*  ����ź���                  */
    
    selwunNode.SELWUN_hThreadId = ptcbCur->TCB_ulId;
    
    LW_THREAD_SAFE();                                                   /*  ���밲ȫģʽ                */
    
    pselctx->SELCTX_iWidth          = iWidth;                           /*  ��������ļ���              */
    pselctx->SELCTX_bPendedOnSelect = LW_TRUE;                          /*  ��Ҫ delete hook ��� NODE  */
    pselctx->SELCTX_bBadFd          = LW_FALSE;
    
    if (pfdsetRead) {                                                   /*  �����ļ���                */
        selwunNode.SELWUN_seltypType = SELREAD;
        if (__selDoIoctls(&pselctx->SELCTX_fdsetOrigReadFds, 
                          pfdsetRead, iWidth, FIOSELECT, 
                          &selwunNode, LW_TRUE)) {                      /*  ��������,�����˳�           */
            iIsOk = PX_ERROR;
        }
    }
    
    if (iIsOk != PX_ERROR && pfdsetWrite) {                             /*  ���д�ļ���                */
        selwunNode.SELWUN_seltypType = SELWRITE;
        if (__selDoIoctls(&pselctx->SELCTX_fdsetOrigWriteFds, 
                          pfdsetWrite, iWidth, FIOSELECT, 
                          &selwunNode, LW_TRUE)) {                      /*  ��������,�����˳�           */
            iIsOk = PX_ERROR;
        }
    }
    
    if (iIsOk != PX_ERROR && pfdsetExcept) {                            /*  ����쳣�ļ���              */
        selwunNode.SELWUN_seltypType = SELEXCEPT;
        if (__selDoIoctls(&pselctx->SELCTX_fdsetOrigExceptFds, 
                          pfdsetExcept, iWidth, FIOSELECT, 
                          &selwunNode, LW_TRUE)) {                      /*  ��������,�����˳�           */
            iIsOk = PX_ERROR;
        }
    }
    
    if (iIsOk != ERROR_NONE) {                                          /*  �����˴���                  */
        ulError = API_GetLastError();
        
        if (pfdsetRead) {
            selwunNode.SELWUN_seltypType = SELREAD;
            __selDoIoctls(&pselctx->SELCTX_fdsetOrigReadFds, 
                          LW_NULL, iWidth, FIOUNSELECT, 
                          &selwunNode, LW_FALSE);                       /*  �ͷŽڵ�                    */
        }
        
        if (pfdsetWrite) {
            selwunNode.SELWUN_seltypType = SELWRITE;
            __selDoIoctls(&pselctx->SELCTX_fdsetOrigWriteFds, 
                          LW_NULL, iWidth, FIOUNSELECT, 
                          &selwunNode, LW_FALSE);                       /*  �ͷŽڵ�                    */
        }
        
        if (pfdsetExcept) {
            selwunNode.SELWUN_seltypType = SELEXCEPT;
            __selDoIoctls(&pselctx->SELCTX_fdsetOrigExceptFds, 
                          LW_NULL, iWidth, FIOUNSELECT, 
                          &selwunNode, LW_FALSE);                       /*  �ͷŽڵ�                    */
        }
        
        if (ulError == ENOSYS) {
            _ErrorHandle(ERROR_IO_SELECT_UNSUPPORT_IN_DRIVER);          /*  ��������֧��              */
        }
        
        pselctx->SELCTX_bPendedOnSelect = LW_FALSE;                     /*  ������� NODE ���          */
        
        LW_THREAD_UNSAFE();                                             /*  �˳���ȫģʽ                */
        return  (PX_ERROR);
    }
    
    LW_THREAD_UNSAFE();                                                 /*  �˳���ȫģʽ                */
    
#if LW_CFG_SIGNAL_EN > 0
    if (sigsetMask) {
        sigprocmask(SIG_SETMASK, sigsetMask, &sigsetMaskOld);
    }
#endif                                                                  /*  LW_CFG_SIGNAL_EN > 0        */
    
    ulError = API_SemaphoreBPend(pselctx->SELCTX_hSembWakeup,
                                 ulWaitTime);                           /*  ��ʼ�ȴ�                    */
    
#if LW_CFG_SIGNAL_EN > 0
    if (sigsetMask) {
        sigprocmask(SIG_SETMASK, &sigsetMaskOld, LW_NULL);
    }
#endif                                                                  /*  LW_CFG_SIGNAL_EN > 0        */
    
    LW_THREAD_SAFE();                                                   /*  ���밲ȫģʽ                */
    
    if (pfdsetRead) {                                                   /*  �����ļ���                */
        selwunNode.SELWUN_seltypType = SELREAD;
        if (__selDoIoctls(&pselctx->SELCTX_fdsetOrigReadFds, 
                          pfdsetRead, iWidth, FIOUNSELECT, 
                          &selwunNode, LW_FALSE)) {                     /*  ������ڽڵ�,ɾ���ڵ�       */
            iIsOk = PX_ERROR;
        }
    }

    if (pfdsetWrite) {                                                  /*  ���д�ļ���                */
        selwunNode.SELWUN_seltypType = SELWRITE;
        if (__selDoIoctls(&pselctx->SELCTX_fdsetOrigWriteFds, 
                          pfdsetWrite, iWidth, FIOUNSELECT, 
                          &selwunNode, LW_FALSE)) {                     /*  ������ڽڵ�,ɾ���ڵ�       */
            iIsOk = PX_ERROR;
        }
    }
    
    if (pfdsetExcept) {                                                 /*  ����쳣�ļ���              */
        selwunNode.SELWUN_seltypType = SELEXCEPT;
        if (__selDoIoctls(&pselctx->SELCTX_fdsetOrigExceptFds, 
                          pfdsetExcept, iWidth, FIOUNSELECT, 
                          &selwunNode, LW_FALSE)) {                     /*  ��������,�����˳�           */
            iIsOk = PX_ERROR;
        }
    }
    
    bBadFd = pselctx->SELCTX_bBadFd;
    pselctx->SELCTX_bPendedOnSelect = LW_FALSE;
    
    LW_THREAD_UNSAFE();                                                 /*  �˳���ȫģʽ                */
    
    if (bBadFd || (iIsOk == PX_ERROR)) {                                /*  ���ִ���                    */
        _ErrorHandle(EBADF);
        return  (PX_ERROR);
    }
    
    iCnt = __selFdsetGetFileCounter(iWidth, pfdsetRead, pfdsetWrite, pfdsetExcept);
    if ((iCnt == 0) && (ulError != ERROR_THREAD_WAIT_TIMEOUT)) {        /*  û���ļ���Ч                */
        _ErrorHandle(ulError);
        return  (PX_ERROR);                                             /*  ���ִ���                    */
    }

    return  (iCnt);
}
/*********************************************************************************************************
** ��������: select
** ��������: select() ��· I/O ����.
** �䡡��  : iWidth            ���õ��ļ�����,�����ļ��� + 1.
**           pfdsetRead        �û����ĵĿɶ��ļ���.
**           pfdsetWrite       �û����ĵĿ�д�ļ���.
**           pfdsetExcept      �û����ĵ��쳣�ļ���.
**           ptmvalTO          �ȴ���ʱʱ��, LW_NULL ��ʾ��Զ�ȴ�. (���ܱ��ı�)
** �䡡��  : �������صȴ������ļ����� ,���󷵻� PX_ERROR.
**           errno == ERROR_IO_SELECT_UNSUPPORT_IN_DRIVER       ��������֧��
**           errno == ERROR_IO_SELECT_CONTEXT                   �̲߳����� context
**           errno == ERROR_IO_SELECT_WIDTH                     ����ļ��Ŵ���
**           errno == ERROR_THREAD_WAIT_TIMEOUT                 �ȴ���ʱ
**           errno == ERROR_KERNEL_IN_ISR                       ���ж��е���
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
INT     select (INT               iWidth, 
                fd_set           *pfdsetRead,
                fd_set           *pfdsetWrite,
                fd_set           *pfdsetExcept,
                struct timeval   *ptmvalTO)
{
    struct timespec   tmspec;
    struct timespec  *ptmspec;
           INT        iRet;
           
#if LW_CFG_POSIX_EN > 0
    __PX_VPROC_CONTEXT  *pvpCtx = _posixVprocCtxGet();
    struct timespec      tmspec1, tmspec2;
#endif                                                                  /*  LW_CFG_POSIX_EN > 0         */

    if (LW_CPU_GET_CUR_NESTING()) {
        _ErrorHandle(ERROR_KERNEL_IN_ISR);                              /*  �������ж��е���            */
        return  (PX_ERROR);
    }
    
    if (ptmvalTO) {
        LW_TIMEVAL_TO_TIMESPEC(ptmvalTO, &tmspec);
        ptmspec = &tmspec;

#if LW_CFG_POSIX_EN > 0
        if (pvpCtx->PVPCTX_iSelectMethod == SELECT_METHOD_LINUX) {
            lib_clock_gettime(CLOCK_MONOTONIC, &tmspec1);
        }
#endif                                                                  /*  LW_CFG_POSIX_EN > 0         */

    } else {
        ptmspec = LW_NULL;
    }
    
    iRet = pselect(iWidth, pfdsetRead, pfdsetWrite, pfdsetExcept, ptmspec, LW_NULL);
    
#if LW_CFG_POSIX_EN > 0
    if ((pvpCtx->PVPCTX_iSelectMethod == SELECT_METHOD_LINUX) && ptmspec) {
        lib_clock_gettime(CLOCK_MONOTONIC, &tmspec2);
        if (__timespecLeftTime(&tmspec1, &tmspec2)) {
            __timespecSub(&tmspec2, &tmspec1);
            
            if (__timespecLeftTime(&tmspec2, ptmspec)) {
                __timespecSub(ptmspec, &tmspec2);
                LW_TIMESPEC_TO_TIMEVAL(ptmvalTO, ptmspec);
            
            } else {
                ptmvalTO->tv_sec  = 0;
                ptmvalTO->tv_usec = 0;
            }
        }
    }
#endif                                                                  /*  LW_CFG_POSIX_EN > 0         */
    
    return  (iRet);
}
/*********************************************************************************************************
** ��������: select_method
** ��������: select() ����ѡ��.
** �䡡��  : iMethod           �����µķ���.
**           piOldMethod       ֮ǰ���õķ���.
** �䡡��  : ERROR_NONE
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
INT  select_method (INT  iMethod, INT  *piOldMethod)
{
#if LW_CFG_POSIX_EN > 0
    __PX_VPROC_CONTEXT  *pvpCtx = _posixVprocCtxGet();
    
    if (piOldMethod) {
        *piOldMethod = pvpCtx->PVPCTX_iSelectMethod;
    }
    
    if ((iMethod == SELECT_METHOD_BSD) ||
        (iMethod == SELECT_METHOD_LINUX)) {
        pvpCtx->PVPCTX_iSelectMethod = iMethod;
    }
    
    return  (ERROR_NONE);
    
#else
    _ErrorHandle(ENOSYS);
    return  (PX_ERROR);
#endif                                                                  /*  LW_CFG_POSIX_EN > 0         */
}

#endif                                                                  /*  LW_CFG_DEVICE_EN > 0        */
                                                                        /*  LW_CFG_SELECT_EN > 0        */
/*********************************************************************************************************
  END
*********************************************************************************************************/
