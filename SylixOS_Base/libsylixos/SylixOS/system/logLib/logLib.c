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
** ��   ��   ��: logLib.c
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2008 �� 06 �� 12 ��
**
** ��        ��: ϵͳ��־����ϵͳ, ��������������жϺ��������źž��������.

** BUG:
2009.04.04  ������־ϵͳ, ֧�� logMsg �� printk ����, ͬʱ�ܾ�ʹ��Σ�յ�ȫ�ֻ���.
2009.04.07  printk ʹ���������ʵ�ֻ���, ������������ printk ���ж��еĴ���.
2009.11.03  API_LogPrintk() �����֮ǰ, ����һ�εȼ��ж�.
2012.03.12  ʹ���Զ� attr 
2014.05.29  ��һ�������ļ�������ʱ������ӡ�߳�.
2014.07.04  ϵͳû�г�ʼ�� log ʱ, ͨ�� bspDebugMsg() ��ӡ.
*********************************************************************************************************/
#define  __SYLIXOS_STDARG
#define  __SYLIXOS_STDIO
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
#include "../SylixOS/system/include/s_system.h"
/*********************************************************************************************************
  MACRO
*********************************************************************************************************/
#define __MAX_MSG_LEN   LW_CFG_LOG_MSG_LEN_MAX                          /*  ϵͳ��ӡ�����Ϣ����      */
#define __MAX_MSG_NUM   LW_CFG_MAX_LOGMSGS                              /*  ��Ϣ������Ϣ����            */
#define __MAX_ARG       10                                              /*  ����������                */
/*********************************************************************************************************
  printk priority
*********************************************************************************************************/
#define DEFAULT_MESSAGE_LOGLEVEL    4                                   /*  KERN_WARNING                */
#define MINIMUM_CONSOLE_LOGLEVEL    0                                   /*  ���û�ʹ�õ���С����        */
#define DEFAULT_CONSOLE_LOGLEVEL    7                                   /*  anything MORE serious than  */
                                                                        /*  KERN_DEBUG                  */
int     console_printk[4] = {
       DEFAULT_CONSOLE_LOGLEVEL,                                        /*  �ն˼���                    */
       DEFAULT_MESSAGE_LOGLEVEL,                                        /*  Ĭ�ϼ���                    */
       MINIMUM_CONSOLE_LOGLEVEL,                                        /*  ���û�ʹ�õ���С����        */
       DEFAULT_CONSOLE_LOGLEVEL,                                        /*  Ĭ���ն˼���                */
};
/*********************************************************************************************************
  CONTRL BLOCK
*********************************************************************************************************/
#if LW_CFG_LOG_LIB_EN > 0

typedef struct {
    CPCHAR                     LOGMSG_pcFormat;                         /*  ��ʽ���ִ�                  */
    
    /*
     *  printk
     */
    PCHAR                      LOGMSG_pcPrintk;                         /*  ֱ����Ҫ��ӡ���ִ� printk   */
    int                        LOGMSG_iLevel;                           /*  printk level                */
    
    /*
     *  log messsage
     */
    PVOID                      LOGMSG_pvArg[__MAX_ARG];                 /*  �ص����� logMsg             */
    LW_OBJECT_HANDLE           LOGMSG_ulThreadId;                       /*  �����߳̾��                */
    BOOL                       LOGMSG_bIsNeedHeader;                    /*  �Ƿ���Ҫ��ӡͷ��            */
} LW_LOG_MSG;
typedef LW_LOG_MSG            *PLW_LOG_MSG;
/*********************************************************************************************************
  INTERNAL CLOBAL
*********************************************************************************************************/
LW_OBJECT_HANDLE               _G_hLogMsgHandle;                        /*  LOG ��Ϣ���о��            */
static INT                     _G_iLogMsgsLost = 0;                     /*  ��ʧ��LOG ��Ϣ����          */
static fd_set                  _G_fdsetLogFd;                           /*  LOG �ļ���                  */
static INT                     _G_iMaxWidth;                            /*  �����ļ��� + 1            */
/*********************************************************************************************************
  printk buffer
*********************************************************************************************************/
static CHAR                    _G_cLogPrintkBuffer[__MAX_MSG_NUM][__MAX_MSG_LEN];
static LW_OBJECT_HANDLE        _G_cLogPartition;                        /*  �ڴ滺�����                */

#define __LOG_PRINTK_GET_BUFFER()           API_PartitionGet(_G_cLogPartition);
#define __LOG_PRINTK_FREE_BUFFER(pvMem)     API_PartitionPut(_G_cLogPartition, pvMem);
/*********************************************************************************************************
  �����߳�
*********************************************************************************************************/
VOID    _LogThread(VOID);                                               /*  LOG �������                */
/*********************************************************************************************************
  INTERNAL FUNC
*********************************************************************************************************/
VOID    __logPrintk(int          iLevel, 
                    PCHAR        pcPrintk);
VOID    __logPrintf(CPCHAR       pcFormat,
                    PVOID        pvArg0,
                    PVOID        pvArg1,
                    PVOID        pvArg2,
                    PVOID        pvArg3,
                    PVOID        pvArg4,
                    PVOID        pvArg5,
                    PVOID        pvArg6,
                    PVOID        pvArg7,
                    PVOID        pvArg8,
                    PVOID        pvArg9);
/*********************************************************************************************************
** ��������: API_LogFdSet
** ��������: ���� LOG ��Ҫ���ĵ��ļ�
** �䡡��  : iWidth                        �����ļ��� + 1  ���� select() ��һ������
**           pfdsetLog                     �µ��ļ���
** �䡡��  : PX_ERROR or ERROR_NONE
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
INT  API_LogFdSet (INT  iWidth, fd_set  *pfdsetLog)
{
    static BOOL  bIsInit = LW_FALSE;
    
    if (!pfdsetLog || !iWidth) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "pfdsetLog invalidate.\r\n");
        _ErrorHandle(ERROR_LOG_FDSET_NULL);
        return  (PX_ERROR);
    }

    __KERNEL_MODE_PROC(
        _G_fdsetLogFd = *pfdsetLog;
        _G_iMaxWidth  = iWidth;
    );
    
    if (bIsInit == LW_FALSE) {
        bIsInit =  LW_TRUE;
        API_ThreadStart(_S_ulThreadLogId);                              /*  �����ں˴�ӡ�߳�            */
    }
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: API_LogFdGet
** ��������: ��� LOG ��Ҫ���ĵ��ļ�
** �䡡��  : piWidth                       �����ļ��� + 1  ���� select() ��һ������
**           pfdsetLog                     �µ��ļ���
** �䡡��  : PX_ERROR or ERROR_NONE
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
INT  API_LogFdGet (INT  *piWidth, fd_set  *pfdsetLog)
{
    if (!pfdsetLog || !piWidth) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "pfdsetLog invalidate.\r\n");
        _ErrorHandle(ERROR_LOG_FDSET_NULL);
        return  (PX_ERROR);
    }

    __KERNEL_MODE_PROC(
        *pfdsetLog = _G_fdsetLogFd;
        *piWidth   = _G_iMaxWidth;
    );
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: API_LogFdAdd
** ��������: ��� LOG ��Ҫ���ĵ��ļ�
** �䡡��  : iFd                      �ļ�������
** �䡡��  : PX_ERROR or ERROR_NONE
** ȫ�ֱ���:
** ����ģ��:
                                           API ����
*********************************************************************************************************/
LW_API
INT  API_LogFdAdd (INT  iFd)
{
    if (iFd < 0) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }

    if (__PROC_GET_PID_CUR() != 0) {
        _ErrorHandle(ENOTSUP);
        return  (PX_ERROR);
    }

    __KERNEL_ENTER();
    FD_SET(iFd, &_G_fdsetLogFd);
    if (iFd >= _G_iMaxWidth) {
        _G_iMaxWidth = iFd + 1;
    }
    __KERNEL_EXIT();

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: API_LogFdDelete
** ��������: ɾ�� LOG ��Ҫ���ĵ��ļ�
** �䡡��  : iFd                      �ļ�������
** �䡡��  : PX_ERROR or ERROR_NONE
** ȫ�ֱ���:
** ����ģ��:
                                           API ����
*********************************************************************************************************/
LW_API
INT  API_LogFdDelete (INT  iFd)
{
    if (iFd < 0) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }

    if (__PROC_GET_PID_CUR() != 0) {
        _ErrorHandle(ENOTSUP);
        return  (PX_ERROR);
    }

    __KERNEL_ENTER();
    FD_CLR(iFd, &_G_fdsetLogFd);
    __KERNEL_EXIT();

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: __logBspMsg
** ��������: log û�г�ʼ��ʱ�� log / printk ��ӡ
** �䡡��  : pcMsg                      ��Ҫ��ӡ����Ϣ
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
** ˵  ��  : ����ϵͳ��ʼ��֮��, ʹ�� bspDebugMsg() ��ӡ, ����������뽫 \n ��Ϊ \r\n ����.
*********************************************************************************************************/
static VOID  __logBspMsg (PCHAR  pcMsg)
{
    PCHAR   pcLn = lib_index(pcMsg, '\n');
    size_t  stLen;
    
    if (pcLn) {
        stLen = pcLn - pcMsg;
        if (stLen && stLen < (__MAX_MSG_LEN - 2)) {
            if (pcMsg[stLen - 1] != '\r') {
                pcMsg[stLen]      = '\r';
                pcMsg[stLen + 1]  = '\n';
                pcMsg[stLen + 2]  = PX_EOS;
            }
        }
    }
    
    _DebugHandle(__PRINTMESSAGE_LEVEL, pcMsg);
}
/*********************************************************************************************************
** ��������: API_LogPrintk
** ��������: ��¼��ʽ����־��Ϣ
** �䡡��  : pcFormat                   ��ʽ���ִ�
**           ...                        �䳤�ִ�
** �䡡��  : ��ӡ����
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
INT  API_LogPrintk (CPCHAR   pcFormat, ...)
{
    static   CHAR          cBspMsgBuf[__MAX_MSG_LEN];                   /*  û�г�ʼ��֮ǰ��ʱʹ��      */
                                                                        /*  �̲߳���ȫ!                 */
             va_list       varlist;
             LW_LOG_MSG    logmsg;
    REGISTER INT           iRet;
    REGISTER PCHAR         pcBuffer;
    REGISTER ULONG         ulError;
             BOOL          bHaveLevel = LW_FALSE;
             BOOL          bBspMsg    = LW_FALSE;
    
    if (_G_hLogMsgHandle == LW_OBJECT_HANDLE_INVALID) {                 /*  log ��û�г�ʼ��            */
        pcBuffer = cBspMsgBuf;
        bBspMsg  = LW_TRUE;
        
    } else {
        pcBuffer = (PCHAR)__LOG_PRINTK_GET_BUFFER();
        if (pcBuffer == LW_NULL) {
            _ErrorHandle(ERROR_SYSTEM_LOW_MEMORY);
            return  (PX_ERROR);
        }
    }
    
    logmsg.LOGMSG_iLevel = default_message_loglevel;
    if (lib_strnlen(pcFormat, 3) >= 3) {
        if ((pcFormat[0] == '<') && (pcFormat[2] == '>')) {
            if ((pcFormat[1] <= '9') && (pcFormat[1] >= '0')) {
                logmsg.LOGMSG_iLevel = pcFormat[1] - '0';
                bHaveLevel = LW_TRUE;
            }
        }
    }
    
    if (logmsg.LOGMSG_iLevel > console_loglevel) {                      /*  ����Ӧ��Ϊ 7                */
        if (bBspMsg == LW_FALSE) {
            __LOG_PRINTK_FREE_BUFFER(pcBuffer);
        }
        return  (ERROR_NONE);                                           /*  �ȼ�̫��, �޷���ӡ          */
    }
    
    if (bHaveLevel) {
        va_start(varlist, pcFormat);
        iRet = vsnprintf(pcBuffer, __MAX_MSG_LEN, &pcFormat[3], varlist);
        va_end(varlist);
    
    } else {
        va_start(varlist, pcFormat);
        iRet = vsnprintf(pcBuffer, __MAX_MSG_LEN, pcFormat, varlist);
        va_end(varlist);
    }
    
    logmsg.LOGMSG_pcPrintk = pcBuffer;
    logmsg.LOGMSG_pcFormat = pcFormat;
    
    logmsg.LOGMSG_bIsNeedHeader = LW_FALSE;                             /*  ����Ҫ��ӡͷ��              */
    logmsg.LOGMSG_ulThreadId    = LW_OBJECT_HANDLE_INVALID;
    
    if (bBspMsg) {                                                      /*  log ��û�г�ʼ��            */
        __logBspMsg(pcBuffer);
    
    } else {
        ulError = API_MsgQueueSend(_G_hLogMsgHandle, &logmsg, sizeof(LW_LOG_MSG));
        if (ulError) {
            __LOG_PRINTK_FREE_BUFFER(pcBuffer);
            _G_iLogMsgsLost++;
            _DebugHandle(__ERRORMESSAGE_LEVEL, "log message lost.\r\n");
            _ErrorHandle(ERROR_LOG_LOST);
            return  (PX_ERROR);
        }
    }
    
    return  (iRet);
}
/*********************************************************************************************************
** ��������: API_LogMsg
** ��������: ��¼��ʽ����־��Ϣ
** �䡡��  : pcFormat                   ��ʽ���ִ�
**           pvArg0                     ��������
**           pvArg1                     ��������
**           pvArg2                     ��������
**           pvArg3                     ��������
**           pvArg4                     ��������
**           pvArg5                     ��������
**           pvArg6                     ��������
**           pvArg7                     ��������
**           pvArg8                     ��������
**           pvArg9                     ��������
**           bIsNeedHeader              �Ƿ���Ҫ��� LOG ͷ��Ϣ
** �䡡��  : PX_ERROR or ERROR_NONE
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
INT  API_LogMsg (CPCHAR       pcFormat,
                 PVOID        pvArg0,
                 PVOID        pvArg1,
                 PVOID        pvArg2,
                 PVOID        pvArg3,
                 PVOID        pvArg4,
                 PVOID        pvArg5,
                 PVOID        pvArg6,
                 PVOID        pvArg7,
                 PVOID        pvArg8,
                 PVOID        pvArg9,
                 BOOL         bIsNeedHeader)
{
             LW_LOG_MSG    logmsg;
    REGISTER ULONG         ulError;
    
    if (!pcFormat) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "log message format.\r\n");
        _ErrorHandle(ERROR_LOG_FMT);
        return  (PX_ERROR);
    }
    
    logmsg.LOGMSG_pcPrintk   = LW_NULL;
    logmsg.LOGMSG_pcFormat   = pcFormat;
    logmsg.LOGMSG_pvArg[0]   = pvArg0;
    logmsg.LOGMSG_pvArg[1]   = pvArg1;
    logmsg.LOGMSG_pvArg[2]   = pvArg2;
    logmsg.LOGMSG_pvArg[3]   = pvArg3;
    logmsg.LOGMSG_pvArg[4]   = pvArg4;
    logmsg.LOGMSG_pvArg[5]   = pvArg5;
    logmsg.LOGMSG_pvArg[6]   = pvArg6;
    logmsg.LOGMSG_pvArg[7]   = pvArg7;
    logmsg.LOGMSG_pvArg[8]   = pvArg8;
    logmsg.LOGMSG_pvArg[9]   = pvArg9;
    
    logmsg.LOGMSG_bIsNeedHeader = bIsNeedHeader;
    if (LW_CPU_GET_CUR_NESTING()) {
        logmsg.LOGMSG_ulThreadId = LW_OBJECT_HANDLE_INVALID;
    } else {
        logmsg.LOGMSG_ulThreadId = API_ThreadIdSelf();
    }
    
    if (_G_hLogMsgHandle == LW_OBJECT_HANDLE_INVALID) {                 /*  log ��û�г�ʼ��            */
        CHAR       cPrintBuffer[__MAX_MSG_LEN];                         /*  �������                    */
        snprintf(cPrintBuffer, __MAX_MSG_LEN, pcFormat,
                 pvArg0, pvArg1, pvArg2, pvArg3, pvArg4, 
                 pvArg5, pvArg6, pvArg7, pvArg8, pvArg9);
        __logBspMsg(cPrintBuffer);
    
    } else {
        ulError = API_MsgQueueSend(_G_hLogMsgHandle, &logmsg, sizeof(LW_LOG_MSG));
        if (ulError) {
            _G_iLogMsgsLost++;
            _DebugHandle(__ERRORMESSAGE_LEVEL, "log message lost.\r\n");
            _ErrorHandle(ERROR_LOG_LOST);
            return  (PX_ERROR);
        }
    }
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: _logInit
** ��������: ��ʼ�� LOG ���� ����
** �䡡��  : 
** �䡡��  : 
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
INT  _logInit (VOID)
{
    LW_CLASS_THREADATTR       threadattr;

    FD_ZERO(&_G_fdsetLogFd);                                            /*  ����ļ���                  */
    
    _G_hLogMsgHandle = API_MsgQueueCreate("log_msg", 
                                          __MAX_MSG_NUM, 
                                          sizeof(LW_LOG_MSG),
                                          LW_OPTION_WAIT_FIFO | LW_OPTION_OBJECT_GLOBAL, 
                                          LW_NULL);                     /*  ���� Msg Queue              */
    if (!_G_hLogMsgHandle) {
        return  (PX_ERROR);
    }
    
    _G_cLogPartition = API_PartitionCreate("printk_pool", 
                                           (PVOID)_G_cLogPrintkBuffer,
                                           __MAX_MSG_NUM, 
                                           __MAX_MSG_LEN, 
                                           LW_OPTION_OBJECT_GLOBAL,
                                           LW_NULL);                    /*  ���� printk ����            */
    if (!_G_cLogPartition) {
        return  (PX_ERROR);
    }
    
    API_ThreadAttrBuild(&threadattr, 
                        LW_CFG_THREAD_LOG_STK_SIZE, 
                        LW_PRIO_T_LOG,
                        (LW_OPTION_THREAD_STK_CHK |
                         LW_OPTION_THREAD_SAFE |
                         LW_OPTION_OBJECT_GLOBAL |
                         LW_OPTION_THREAD_DETACHED),
                        LW_NULL);
    
    _S_ulThreadLogId = API_ThreadInit("t_log",
                                      (PTHREAD_START_ROUTINE)_LogThread,
                                      &threadattr,
                                      LW_NULL);                         /*  ����LOG �����߳�            */
    if (!_S_ulThreadLogId) {
        API_PartitionDelete(&_G_cLogPartition);
        API_MsgQueueDelete(&_G_hLogMsgHandle);
        return  (PX_ERROR);
    }
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: _LogThread
** ��������: ����LOG ��Ϣ���߳�
** �䡡��  : 
** �䡡��  : 
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
VOID  _LogThread (VOID)
{
    static   INT           iOldMsgsLost = 0;
             INT           iNewMsgsLost;
             LW_LOG_MSG    logmsg;
    REGISTER ULONG         ulError;
             size_t        stMsgLen;
             
             CHAR          cThreadName[LW_CFG_OBJECT_NAME_SIZE];
    
    for (;;) {
        ulError = API_MsgQueueReceive(_G_hLogMsgHandle, 
                                      (PVOID)&logmsg,
                                      sizeof(LW_LOG_MSG),
                                      &stMsgLen,
                                      LW_OPTION_WAIT_INFINITE);         /*  �ȴ�LOG ��Ϣ                */
                                      
        if (ulError) {
            /*
             *  ���ǳ�ʼ�����ɹ�, ���򲻻ᵽ����
             */
        } else {
            if (logmsg.LOGMSG_ulThreadId) {                             /*  ������ģʽ����              */
                if (logmsg.LOGMSG_bIsNeedHeader) {                      /*  ��Ҫ��ӡͷ��                */
                    ulError = API_ThreadGetName(logmsg.LOGMSG_ulThreadId,
                                                cThreadName);
                    if (ulError) {
                        __logPrintf("thread id 0x%08x log : ", 
                                    (PVOID)logmsg.LOGMSG_ulThreadId, 
                                    0, 0, 0, 0, 0, 0, 0, 0, 0);
                    } else {
                        __logPrintf("thread \"%s\" log : ", 
                                    (PVOID)cThreadName, 
                                    0, 0, 0, 0, 0, 0, 0, 0, 0);
                    }
                }
            } else {                                                    /*  �ж��з���                  */
                if (logmsg.LOGMSG_bIsNeedHeader) {                      /*  ��Ҫ��ӡͷ��                */
                    __logPrintf("interrupt log : ", 
                                0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
                }
            }
            
            if (logmsg.LOGMSG_pcPrintk) {
                __logPrintk(logmsg.LOGMSG_iLevel,
                            logmsg.LOGMSG_pcPrintk);                    /*  ��ӡ��Ϣ                    */
            } else {
                __logPrintf(logmsg.LOGMSG_pcFormat,
                            logmsg.LOGMSG_pvArg[0],
                            logmsg.LOGMSG_pvArg[1],
                            logmsg.LOGMSG_pvArg[2],
                            logmsg.LOGMSG_pvArg[3],
                            logmsg.LOGMSG_pvArg[4],
                            logmsg.LOGMSG_pvArg[5],
                            logmsg.LOGMSG_pvArg[6],
                            logmsg.LOGMSG_pvArg[7],
                            logmsg.LOGMSG_pvArg[8],
                            logmsg.LOGMSG_pvArg[9]);                    /*  ��ӡ��Ϣ                    */
            }
        }
        
        iNewMsgsLost = _G_iLogMsgsLost;
        if (iNewMsgsLost != iOldMsgsLost) {
            iOldMsgsLost = iNewMsgsLost;
        }
    }
}
/*********************************************************************************************************
** ��������: __logPrintk
** ��������: ��ָ�����ļ�����ӡ LOG ��Ϣ
** �䡡��  : iLevel                     ��ӡ�ȼ�
**           pcPrintk                   ��ʽ����Ϣ
** �䡡��  : 
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
VOID   __logPrintk (int  iLevel, PCHAR  pcPrintk)
{
    REGISTER size_t     stLen = lib_strlen(pcPrintk);
    REGISTER INT        iFdTemp;                                        /*  ��ʱ�ļ�������              */
    REGISTER ULONG      ulPartMask;
    
    if (iLevel > console_loglevel) {
        goto    __printk_over;                                          /*  ������ӡ�ȼ�                */
    }
    
    for (iFdTemp = 0; iFdTemp < _G_iMaxWidth; iFdTemp++) {              /*  ������п�ִ�ж��������ļ�  */
        ulPartMask = 
        _G_fdsetLogFd.fds_bits[((unsigned)iFdTemp) / NFDBITS];          /*  ��� iFdTemp ���ڵ�������   */
        
        if (ulPartMask == 0) {                                          /*  �������������ļ��޹�      */
            iFdTemp += NFDBITS - 1;                                     /*  ������һ���������ж�        */
        
        } else if (ulPartMask & (ULONG)(1 << (((unsigned)iFdTemp) % NFDBITS))) {
            write(iFdTemp, pcPrintk, stLen);                            /*  ��ӡ                        */
        }
    }
    
__printk_over:
    __LOG_PRINTK_FREE_BUFFER(pcPrintk);                                 /*  �ͷ���Ϣ�ռ�                */
}
/*********************************************************************************************************
** ��������: __logPrintf
** ��������: ��ָ�����ļ�����ӡ LOG ��Ϣ
** �䡡��  : pcFormat                   ��ʽ���ִ�
**           pvArg0                     ��������
**           pvArg1                     ��������
**           pvArg2                     ��������
**           pvArg3                     ��������
**           pvArg4                     ��������
**           pvArg5                     ��������
**           pvArg6                     ��������
**           pvArg7                     ��������
**           pvArg8                     ��������
**           pvArg9                     ��������
** �䡡��  : 
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
VOID   __logPrintf (CPCHAR       pcFormat,
                    PVOID        pvArg0,
                    PVOID        pvArg1,
                    PVOID        pvArg2,
                    PVOID        pvArg3,
                    PVOID        pvArg4,
                    PVOID        pvArg5,
                    PVOID        pvArg6,
                    PVOID        pvArg7,
                    PVOID        pvArg8,
                    PVOID        pvArg9)
{
    REGISTER size_t     stLen;
    REGISTER INT        iFdTemp;                                        /*  ��ʱ�ļ�������              */
    REGISTER ULONG      ulPartMask;
             CHAR       cPrintBuffer[__MAX_MSG_LEN];                    /*  �������                    */
    
    stLen = bnprintf(cPrintBuffer, __MAX_MSG_LEN, 0, pcFormat,
                     pvArg0, pvArg1, pvArg2, pvArg3, pvArg4, 
                     pvArg5, pvArg6, pvArg7, pvArg8, pvArg9);           /*  ��ʽ���ִ�                  */
    
    for (iFdTemp = 0; iFdTemp < _G_iMaxWidth; iFdTemp++) {              /*  ������п�ִ�ж��������ļ�  */
        ulPartMask = 
        _G_fdsetLogFd.fds_bits[((unsigned)iFdTemp) / NFDBITS];          /*  ��� iFdTemp ���ڵ�������   */
        
        if (ulPartMask == 0) {                                          /*  �������������ļ��޹�      */
            iFdTemp += NFDBITS - 1;                                     /*  ������һ���������ж�        */
        
        } else if (ulPartMask & (ULONG)(1 << (((unsigned)iFdTemp) % NFDBITS))) {
            write(iFdTemp, cPrintBuffer, stLen);                        /*  ��ӡ                        */
        }
    }
}

#endif                                                                  /*  LW_CFG_LOG_LIB_EN > 0 &&    */
/*********************************************************************************************************
  END
*********************************************************************************************************/
