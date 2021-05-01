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
** ��   ��   ��: waitfile.c
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2007 �� 11 �� 07 ��
**
** ��        ��:  IO ϵͳ select ��ϵͳ���� API. �ṩ�Ե����ļ��Ŀ��ٲ���.

** BUG
2007.11.20  �� pselctx->SELCTX_bPendedOnSelect = LW_TRUE; ��ǰ, ��ʹ FIOSELECT ���е���;�̱߳�ɾ��ʱ
            ����ʹ delete hook ��������, �������� WAKE NODE ɾ��.
2007.11.20  ����� context �� Orig??? �ļ����Ĳ���, �Ա�֤ delete hook ���Է�������.
2007.12.11  ��������ʱ,����ֵӦ��Ϊ -1.
2007.12.22  ����ע��, �޸� _DebugHandle() ������ַ���.
2008.03.01  wait???() ��������ļ����������ж���©��.
2009.07.17  ��������ļ��Ų���Ӧ����ǰ, ��֤ delete hook �ܹ�������ɾ�������Ľڵ�.
2011.03.11  ȷ����ʱ���Է��ض�Ӧ�� errno ���.
2011.08.15  �ش����: ������ posix ����ĺ����Ժ�����ʽ(�Ǻ�)����.
2017.08.18  �ȴ�ʱ����Ӿ�ȷ.
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
** ��������: waitread
** ��������: select() ����,�ȴ������ļ��ɶ�.
** �䡡��  : iFd               �ļ�������
**           ptmvalTO          �ȴ���ʱʱ��, LW_NULL ��ʾ��Զ�ȴ�.
** �䡡��  : �����ȴ������ļ���, Ϊ 1 ��ʾ�ļ����Զ�, Ϊ 0 ��ʾ��ʱ ,���󷵻� PX_ERROR.
**           errno == ERROR_IO_SELECT_UNSUPPORT_IN_DRIVER       ��������֧��
**           errno == ERROR_IO_SELECT_CONTEXT                   �̲߳����� context
**           errno == ERROR_THREAD_WAIT_TIMEOUT                 �ȴ���ʱ
**           errno == ERROR_KERNEL_IN_ISR                       ���ж��е���
**           errno == ERROR_IOS_INVALID_FILE_DESCRIPTOR         �ļ���������Ч.
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
INT     waitread (INT  iFd, struct timeval   *ptmvalTO)
{
    REGISTER INT                 iWidth = iFd + 1;                      /*  iFd + 1                     */
    REGISTER INT                 iWidthInBytes;                         /*  ��Ҫ����λ��ռ�����ֽ�    */
    REGISTER ULONG               ulWaitTime;                            /*  �ȴ�ʱ��                    */
    REGISTER LW_SEL_CONTEXT     *pselctx;
             PLW_CLASS_TCB       ptcbCur;
             struct timespec     tvTO;

    volatile LW_SEL_WAKEUPNODE   selwunNode;                            /*  ���ɵ� NODE ģ��            */
             ULONG               ulError;
             BOOL                bBadFd, bReady;
             
    if (LW_CPU_GET_CUR_NESTING()) {
        _ErrorHandle(ERROR_KERNEL_IN_ISR);                              /*  �������ж��е���            */
        return  (PX_ERROR);
    }
    
    if (iFd > (FD_SETSIZE - 1) || iFd < 0) {                            /*  �ļ��Ŵ���                  */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "file descriptor invalidate..\r\n");
        _ErrorHandle(ERROR_IOS_INVALID_FILE_DESCRIPTOR);                /*  �ļ���������Ч              */
        return  (PX_ERROR);
    }
    
    LW_TCB_GET_CUR_SAFE(ptcbCur);
    
    pselctx = ptcbCur->TCB_pselctxContext;
    
    if (!pselctx) {                                                     /*  û�� select context         */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "no select context.\r\n");
        _ErrorHandle(ERROR_IO_SELECT_CONTEXT);
        return  (PX_ERROR);
    }

    iWidthInBytes = __HOWMANY(iWidth, NFDBITS) * sizeof(fd_mask);       /*  ��Ҫ����λ��ռ�����ֽ�    */
    
    lib_bzero(&pselctx->SELCTX_fdsetOrigReadFds, iWidthInBytes);
    FD_SET(iFd, &pselctx->SELCTX_fdsetOrigReadFds);

    if (!ptmvalTO) {                                                    /*  ����ȴ�ʱ��                */
        ulWaitTime = LW_OPTION_WAIT_INFINITE;                           /*  ���޵ȴ�                    */

    } else {
        LW_TIMEVAL_TO_TIMESPEC(ptmvalTO, &tvTO);
        ulWaitTime = LW_TS_TIMEOUT_TICK(LW_TRUE, &tvTO);
    }

    API_SemaphoreBClear(pselctx->SELCTX_hSembWakeup);                   /*  ����ź���                  */
    
    selwunNode.SELWUN_hThreadId  = ptcbCur->TCB_ulId;
    selwunNode.SELWUN_seltypType = SELREAD;
    selwunNode.SELWUN_iFd        = iFd;
    LW_SELWUN_CLEAR_READY(&selwunNode);
    
    LW_THREAD_SAFE();                                                   /*  ���밲ȫģʽ                */
    
    pselctx->SELCTX_iWidth          = iWidth;                           /*  ��¼����ļ���              */
    pselctx->SELCTX_bPendedOnSelect = LW_TRUE;                          /*  ��Ҫ delete hook ��� NODE  */
    pselctx->SELCTX_bBadFd          = LW_FALSE;
    
    if (ioctl(iFd, FIOSELECT, &selwunNode)) {                           /*  FIOSELECT                   */
        ioctl(iFd, FIOUNSELECT, &selwunNode);                           /*  FIOUNSELECT                 */
        
        bBadFd = pselctx->SELCTX_bBadFd;
        pselctx->SELCTX_bPendedOnSelect = LW_FALSE;                     /*  �����������                */
        
        LW_THREAD_UNSAFE();                                             /*  �˳���ȫģʽ                */
        if (bBadFd) {
            _ErrorHandle(EBADF);
        }
        return  (PX_ERROR);                                             /*  ����                        */
    
    } else {
        if (LW_SELWUN_IS_READY(&selwunNode)) {
            LW_SELWUN_CLEAR_READY(&selwunNode);
            bReady = LW_TRUE;
        
        } else {
            bReady = LW_FALSE;
        }
    }

    LW_THREAD_UNSAFE();                                                 /*  �˳���ȫģʽ                */

    ulError = API_SemaphoreBPend(pselctx->SELCTX_hSembWakeup,
                                 ulWaitTime);                           /*  ��ʼ�ȴ�                    */
    
    LW_THREAD_SAFE();                                                   /*  ���밲ȫģʽ                */
    
    if (ioctl(iFd, FIOUNSELECT, &selwunNode) == ERROR_NONE) {           /*  FIOUNSELECT                 */
        if (!bReady && LW_SELWUN_IS_READY(&selwunNode)) {
            bReady = LW_TRUE;
        }
    }
    
    bBadFd = pselctx->SELCTX_bBadFd;
    pselctx->SELCTX_bPendedOnSelect = LW_FALSE;                         /*  �����������                */
    
    LW_THREAD_UNSAFE();                                                 /*  �˳���ȫģʽ                */
    
    if (bBadFd) {                                                       /*  ���ִ���                    */
        _ErrorHandle(EBADF);
        return  (PX_ERROR);
    }
    
    _ErrorHandle(ulError);
    return  (bReady ? 1 : 0);                                           /*  ����ļ��Ƿ�ɶ�            */
}
/*********************************************************************************************************
** ��������: waitwrite
** ��������: select() ����,�ȴ������ļ���д.
** �䡡��  : iFd               �ļ�������
**           ptmvalTO          �ȴ���ʱʱ��, LW_NULL ��ʾ��Զ�ȴ�.
** �䡡��  : �����ȴ������ļ���, Ϊ 1 ��ʾ�ļ����Զ�, Ϊ 0 ��ʾ��ʱ ,���󷵻� PX_ERROR.
**           errno == ERROR_IO_SELECT_UNSUPPORT_IN_DRIVER       ��������֧��
**           errno == ERROR_IO_SELECT_CONTEXT                   �̲߳����� context
**           errno == ERROR_THREAD_WAIT_TIMEOUT                 �ȴ���ʱ
**           errno == ERROR_KERNEL_IN_ISR                       ���ж��е���
**           errno == ERROR_IOS_INVALID_FILE_DESCRIPTOR         �ļ���������Ч.
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
INT     waitwrite (INT  iFd, struct timeval   *ptmvalTO)
{
    REGISTER INT                 iWidth = iFd + 1;                      /*  iFd + 1                     */
    REGISTER INT                 iWidthInBytes;                         /*  ��Ҫ����λ��ռ�����ֽ�    */
    REGISTER ULONG               ulWaitTime;                            /*  �ȴ�ʱ��                    */
    REGISTER LW_SEL_CONTEXT     *pselctx;
             PLW_CLASS_TCB       ptcbCur;
             struct timespec     tvTO;
    
             LW_SEL_WAKEUPNODE   selwunNode;                            /*  ���ɵ� NODE ģ��            */
             ULONG               ulError;
             BOOL                bBadFd, bReady;
             
    if (LW_CPU_GET_CUR_NESTING()) {
        _ErrorHandle(ERROR_KERNEL_IN_ISR);                              /*  �������ж��е���            */
        return  (PX_ERROR);
    }
    
    if (iFd > (FD_SETSIZE - 1) || iFd < 0) {                            /*  �ļ��Ŵ���                  */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "file descriptor invalidate..\r\n");
        _ErrorHandle(ERROR_IOS_INVALID_FILE_DESCRIPTOR);                /*  �ļ���������Ч              */
        return  (PX_ERROR);
    }
    
    LW_TCB_GET_CUR_SAFE(ptcbCur);
    
    pselctx = ptcbCur->TCB_pselctxContext;
    
    if (!pselctx) {                                                     /*  û�� select context         */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "no select context.\r\n");
        _ErrorHandle(ERROR_IO_SELECT_CONTEXT);
        return  (PX_ERROR);
    }

    iWidthInBytes = __HOWMANY(iWidth, NFDBITS) * sizeof(fd_mask);       /*  ��Ҫ����λ��ռ�����ֽ�    */
    
    lib_bzero(&pselctx->SELCTX_fdsetOrigWriteFds, iWidthInBytes);
    FD_SET(iFd, &pselctx->SELCTX_fdsetOrigWriteFds);                    /*  ָ���ļ���λ                */
    
    if (!ptmvalTO) {                                                    /*  ����ȴ�ʱ��                */
        ulWaitTime = LW_OPTION_WAIT_INFINITE;                           /*  ���޵ȴ�                    */

    } else {
        LW_TIMEVAL_TO_TIMESPEC(ptmvalTO, &tvTO);
        ulWaitTime = LW_TS_TIMEOUT_TICK(LW_TRUE, &tvTO);
    }
    
    API_SemaphoreBClear(pselctx->SELCTX_hSembWakeup);                   /*  ����ź���                  */
    
    selwunNode.SELWUN_hThreadId  = ptcbCur->TCB_ulId;
    selwunNode.SELWUN_seltypType = SELWRITE;
    selwunNode.SELWUN_iFd        = iFd;
    LW_SELWUN_CLEAR_READY(&selwunNode);
    
    LW_THREAD_SAFE();                                                   /*  ���밲ȫģʽ                */
    
    pselctx->SELCTX_iWidth          = iWidth;                           /*  ��¼����ļ���              */
    pselctx->SELCTX_bPendedOnSelect = LW_TRUE;                          /*  ��Ҫ delete hook ��� NODE  */
    pselctx->SELCTX_bBadFd          = LW_FALSE;
    
    if (ioctl(iFd, FIOSELECT, &selwunNode)) {                           /*  FIOSELECT                   */
        ioctl(iFd, FIOUNSELECT, &selwunNode);                           /*  FIOUNSELECT                 */
        
        bBadFd = pselctx->SELCTX_bBadFd;
        pselctx->SELCTX_bPendedOnSelect = LW_FALSE;                     /*  �����������                */
        
        LW_THREAD_UNSAFE();                                             /*  �˳���ȫģʽ                */
        if (bBadFd) {
            _ErrorHandle(EBADF);
        }
        return  (PX_ERROR);                                             /*  ����                        */
    
    } else {
        if (LW_SELWUN_IS_READY(&selwunNode)) {
            LW_SELWUN_CLEAR_READY(&selwunNode);
            bReady = LW_TRUE;
        
        } else {
            bReady = LW_FALSE;
        }
    }

    LW_THREAD_UNSAFE();                                                 /*  �˳���ȫģʽ                */
    
    ulError = API_SemaphoreBPend(pselctx->SELCTX_hSembWakeup,
                                 ulWaitTime);                           /*  ��ʼ�ȴ�                    */
    
    LW_THREAD_SAFE();                                                   /*  ���밲ȫģʽ                */
    
    if (ioctl(iFd, FIOUNSELECT, &selwunNode) == ERROR_NONE) {           /*  FIOUNSELECT                 */
        if (!bReady && LW_SELWUN_IS_READY(&selwunNode)) {
            bReady = LW_TRUE;
        }
    }
    
    bBadFd = pselctx->SELCTX_bBadFd;
    pselctx->SELCTX_bPendedOnSelect = LW_FALSE;                         /*  �����������                */
    
    LW_THREAD_UNSAFE();                                                 /*  �˳���ȫģʽ                */
    
    if (bBadFd) {                                                       /*  ���ִ���                    */
        _ErrorHandle(EBADF);
        return  (PX_ERROR);
    }
    
    _ErrorHandle(ulError);
    return  (bReady ? 1 : 0);                                           /*  ����ļ��Ƿ�ɶ�            */
}
/*********************************************************************************************************
** ��������: waitexcept
** ��������: select() ����,�ȴ������ļ��쳣.
** �䡡��  : iFd               �ļ�������
**           ptmvalTO          �ȴ���ʱʱ��, LW_NULL ��ʾ��Զ�ȴ�.
** �䡡��  : �����ȴ������ļ���, Ϊ 1 ��ʾ�ļ����Զ�, Ϊ 0 ��ʾ��ʱ ,���󷵻� PX_ERROR.
**           errno == ERROR_IO_SELECT_UNSUPPORT_IN_DRIVER       ��������֧��
**           errno == ERROR_IO_SELECT_CONTEXT                   �̲߳����� context
**           errno == ERROR_THREAD_WAIT_TIMEOUT                 �ȴ���ʱ
**           errno == ERROR_KERNEL_IN_ISR                       ���ж��е���
**           errno == ERROR_IOS_INVALID_FILE_DESCRIPTOR         �ļ���������Ч.
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
INT     waitexcept (INT  iFd, struct timeval   *ptmvalTO)
{
    REGISTER INT                 iWidth = iFd + 1;                      /*  iFd + 1                     */
    REGISTER INT                 iWidthInBytes;                         /*  ��Ҫ����λ��ռ�����ֽ�    */
    REGISTER ULONG               ulWaitTime;                            /*  �ȴ�ʱ��                    */
    REGISTER LW_SEL_CONTEXT     *pselctx;
             PLW_CLASS_TCB       ptcbCur;
             struct timespec     tvTO;
    
             LW_SEL_WAKEUPNODE   selwunNode;                            /*  ���ɵ� NODE ģ��            */
             ULONG               ulError;
             BOOL                bBadFd, bReady;
             
    if (LW_CPU_GET_CUR_NESTING()) {
        _ErrorHandle(ERROR_KERNEL_IN_ISR);                              /*  �������ж��е���            */
        return  (PX_ERROR);
    }
    
    if (iFd > (FD_SETSIZE - 1) || iFd < 0) {                            /*  �ļ��Ŵ���                  */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "file descriptor invalidate..\r\n");
        _ErrorHandle(ERROR_IOS_INVALID_FILE_DESCRIPTOR);                /*  �ļ���������Ч              */
        return  (PX_ERROR);
    }
    
    LW_TCB_GET_CUR_SAFE(ptcbCur);
    
    pselctx = ptcbCur->TCB_pselctxContext;
    
    if (!pselctx) {                                                     /*  û�� select context         */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "no select context.\r\n");
        _ErrorHandle(ERROR_IO_SELECT_CONTEXT);
        return  (PX_ERROR);
    }

    iWidthInBytes = __HOWMANY(iWidth, NFDBITS) * sizeof(fd_mask);       /*  ��Ҫ����λ��ռ�����ֽ�    */
    
    lib_bzero(&pselctx->SELCTX_fdsetOrigExceptFds, iWidthInBytes);
    FD_SET(iFd, &pselctx->SELCTX_fdsetOrigExceptFds);                   /*  ָ���ļ���λ                */
    
    if (!ptmvalTO) {                                                    /*  ����ȴ�ʱ��                */
        ulWaitTime = LW_OPTION_WAIT_INFINITE;                           /*  ���޵ȴ�                    */

    } else {
        LW_TIMEVAL_TO_TIMESPEC(ptmvalTO, &tvTO);
        ulWaitTime = LW_TS_TIMEOUT_TICK(LW_TRUE, &tvTO);
    }
    
    API_SemaphoreBClear(pselctx->SELCTX_hSembWakeup);                   /*  ����ź���                  */
    
    selwunNode.SELWUN_hThreadId  = ptcbCur->TCB_ulId;
    selwunNode.SELWUN_seltypType = SELEXCEPT;
    selwunNode.SELWUN_iFd        = iFd;
    LW_SELWUN_CLEAR_READY(&selwunNode);
    
    LW_THREAD_SAFE();                                                   /*  ���밲ȫģʽ                */
    
    pselctx->SELCTX_iWidth          = iWidth;                           /*  ��¼����ļ���              */
    pselctx->SELCTX_bPendedOnSelect = LW_TRUE;                          /*  ��Ҫ delete hook ��� NODE  */
    pselctx->SELCTX_bBadFd          = LW_FALSE;
    
    if (ioctl(iFd, FIOSELECT, &selwunNode)) {                           /*  FIOSELECT                   */
        ioctl(iFd, FIOUNSELECT, &selwunNode);                           /*  FIOUNSELECT                 */
        
        bBadFd = pselctx->SELCTX_bBadFd;
        pselctx->SELCTX_bPendedOnSelect = LW_FALSE;                     /*  �����������                */
        
        LW_THREAD_UNSAFE();                                             /*  �˳���ȫģʽ                */
        if (bBadFd) {
            _ErrorHandle(EBADF);
        }
        return  (PX_ERROR);                                             /*  ����                        */
    
    } else {
        if (LW_SELWUN_IS_READY(&selwunNode)) {
            LW_SELWUN_CLEAR_READY(&selwunNode);
            bReady = LW_TRUE;
        
        } else {
            bReady = LW_FALSE;
        }
    }

    LW_THREAD_UNSAFE();                                                 /*  �˳���ȫģʽ                */
    
    ulError = API_SemaphoreBPend(pselctx->SELCTX_hSembWakeup,
                                 ulWaitTime);                           /*  ��ʼ�ȴ�                    */
    
    LW_THREAD_SAFE();                                                   /*  ���밲ȫģʽ                */
    
    if (ioctl(iFd, FIOUNSELECT, &selwunNode) == ERROR_NONE) {           /*  FIOUNSELECT                 */
        if (!bReady && LW_SELWUN_IS_READY(&selwunNode)) {
            bReady = LW_TRUE;
        }
    }
    
    bBadFd = pselctx->SELCTX_bBadFd;
    pselctx->SELCTX_bPendedOnSelect = LW_FALSE;                         /*  �����������                */
    
    LW_THREAD_UNSAFE();                                                 /*  �˳���ȫģʽ                */
    
    if (bBadFd) {                                                       /*  ���ִ���                    */
        _ErrorHandle(EBADF);
        return  (PX_ERROR);
    }
    
    _ErrorHandle(ulError);
    return  (bReady ? 1 : 0);                                           /*  ����ļ��Ƿ�ɶ�            */
}

#endif                                                                  /*  LW_CFG_DEVICE_EN > 0        */
                                                                        /*  LW_CFG_SELECT_EN > 0        */
/*********************************************************************************************************
  END
*********************************************************************************************************/
