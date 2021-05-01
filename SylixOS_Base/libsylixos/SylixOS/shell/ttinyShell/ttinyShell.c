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
** ��   ��   ��: ttinyShell.c
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2008 �� 07 �� 27 ��
**
** ��        ��: һ����С�͵� shell ϵͳ, ʹ�� tty/pty ���ӿ�, ��Ҫ���ڵ�����򵥽���.

** BUG
2008.08.12  �� linux shell ��, ����ĵ�һ������Ϊ, ���������ؼ���.
2008.10.19  ����ת���ַ������ͺ���.
2009.04.04  ������������, ����Ϊ��������Ӣ����.
            ������ɾ��ʱ, ��Ҫ��� getopt ����ڴ�.
2009.05.27  ���� shell �� ctrl+C ��֧��.
2009.05.30  API_TShellCtlCharSend() ��Ҫ�������.
2009.07.13  ���봴�� shell ����չ��ʽ.
2009.11.23  ���� heap �����ʼ��.
2010.01.16  shell ��ջ��С��������.
2010.07.28  ���� modem ����.
2011.06.03  ��ʼ�� shell ϵͳʱ, �� /etc/profile ͬ����������.
2011.07.08  ȡ�� shell ��ʼ��ʱ, load ��������. ȷ�� etc Ŀ¼���غ�, �� bsp �� load.
2012.12.07  shell Ϊϵͳ�߳�.
2012.12.24  shell ֧�ִӽ��̴���, ���������Զ����������ļ������� dup ���ں���, Ȼ�������� shell.
            API_TShellExec() ������ڽ����е���, �������ͬ��������ʽ����.
            API_TShellExecBg() ����ڱ�������������뽫ָ�����ļ� dup ���ں��������б��� shell.
2013.01.21  �����û� CACHE ����.
2013.07.18  ʹ���µĻ�ȡ TCB �ķ���, ȷ�� SMP ϵͳ��ȫ.
2013.08.26  shell ϵͳ������� format �� help �ڶ�������Ϊ const char *.
2014.07.10  shell ϵͳ������µ���ɫϵͳ�ĳ�ʼ��.
            shell ȥ����ʽɫ�ʿ��ƺ���.
2014.11.22  ����ѡ�����ú���.
2016.03.25  ���� API_TShellExecBg() ���� shell �������񴴽�������������.
*********************************************************************************************************/
#define  __SYLIXOS_STDIO
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
#include "../SylixOS/system/include/s_system.h"
#include "../SylixOS/shell/include/ttiny_shell.h"
/*********************************************************************************************************
  Ӧ�ü� API
*********************************************************************************************************/
#include "../SylixOS/api/Lw_Api_Kernel.h"
#include "../SylixOS/api/Lw_Api_System.h"
/*********************************************************************************************************
  �ü�����
*********************************************************************************************************/
#if LW_CFG_SHELL_EN > 0
#include "ttinyShell.h"
#include "ttinyShellLib.h"
#include "ttinyShellReadline.h"
#include "ttinyShellSysCmd.h"
#include "ttinyShellSysVar.h"
#include "../SylixOS/shell/ttinyVar/ttinyVarLib.h"
#include "../SylixOS/shell/extLib/ttinyShellExtCmd.h"
#include "../SylixOS/shell/fsLib/ttinyShellFsCmd.h"
#include "../SylixOS/shell/tarLib/ttinyShellTarCmd.h"
#include "../SylixOS/shell/modemLib/ttinyShellModemCmd.h"
#include "../SylixOS/shell/heapLib/ttinyShellHeapCmd.h"
#include "../SylixOS/shell/perfLib/ttinyShellPerfCmd.h"
/*********************************************************************************************************
  ȫ�ֱ���
*********************************************************************************************************/
LW_OBJECT_HANDLE        _G_hTShellLock      = LW_OBJECT_HANDLE_INVALID;
static BOOL             _G_bIsInstallSysCmd = LW_FALSE;
/*********************************************************************************************************
  shell ִ���̶߳�ջ��С (Ĭ���� shell ��ͬ)
*********************************************************************************************************/
size_t                  _G_stShellStackSize = LW_CFG_SHELL_THREAD_STK_SIZE;
/*********************************************************************************************************
** ��������: API_TShellTermAlert
** ��������: tty �ն�����
** �䡡��  : iFd       ���Ŀ��
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
VOID  API_TShellTermAlert (INT  iFd)
{
    PLW_CLASS_TCB   ptcbCur;
    
    LW_TCB_GET_CUR_SAFE(ptcbCur);
    
    if (!(__TTINY_SHELL_GET_OPT(ptcbCur) & LW_OPTION_TSHELL_VT100)) {
        return;
    }
    
    fdprintf(iFd, "\a");
}
/*********************************************************************************************************
** ��������: API_TShellSetTitle
** ��������: tty �ն����ñ���
** �䡡��  : iFd       ���Ŀ��
**           pcTitel   ����
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
VOID  API_TShellSetTitle (INT  iFd, CPCHAR  pcTitle)
{
    PLW_CLASS_TCB   ptcbCur;
    
    LW_TCB_GET_CUR_SAFE(ptcbCur);
    
    if (!(__TTINY_SHELL_GET_OPT(ptcbCur) & LW_OPTION_TSHELL_VT100)) {
        return;
    }
    
    fdprintf(iFd, "\x1B]0;%s\x07", pcTitle);
}
/*********************************************************************************************************
** ��������: API_TShellScrClear
** ��������: tty �ն�����
** �䡡��  : iFd       ���Ŀ��
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
VOID  API_TShellScrClear (INT  iFd)
{
    fdprintf(iFd, "\x1B" "\x5B" "\x48" "\x1B" "\x5B" "\x32" "\x4A");
}
/*********************************************************************************************************
** ��������: API_TShellSetStackSize
** ��������: �趨�µ� shell ��ջ��С (���ú���Ժ��������� shell �� ����ִ�� shell ��Ч)
** �䡡��  : stNewSize     �µĶ�ջ��С (0 ��ʾ������)
**           pstOldSize    ��ǰ��ջ��С (����)
** �䡡��  : ERROR_NONE ��ʾû�д���, -1 ��ʾ����,
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
INT  API_TShellSetStackSize (size_t  stNewSize, size_t  *pstOldSize)
{
    if (pstOldSize) {
        *pstOldSize = _G_stShellStackSize;
    }
    
    if (stNewSize) {
        _G_stShellStackSize = stNewSize;
    }
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: API_TShellInit
** ��������: ��װ tshell ����, �൱�ڳ�ʼ�� tshell ƽ̨
** �䡡��  : NONE
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
VOID  API_TShellInit (VOID)
{
    if (_G_hTShellLock == 0) {
        _G_hTShellLock = API_SemaphoreMCreate("tshell_lock", LW_PRIO_DEF_CEILING, 
                         __TTINY_SHELL_LOCK_OPT, LW_NULL);              /*  ������                      */
        if (!_G_hTShellLock) {
            _DebugHandle(__ERRORMESSAGE_LEVEL, 
                         "tshell lock can not create.\r\n");
            _DebugHandle(__LOGMESSAGE_LEVEL,
                         "ttiny shell system is not initialize.\r\n");
            return;
        }
        API_SystemHookAdd(__tShellOptDeleteHook, 
                          LW_OPTION_THREAD_DELETE_HOOK);                /*  ��װ�ص�����                */
        API_SystemHookAdd(__tshellReadlineClean, 
                          LW_OPTION_THREAD_DELETE_HOOK);                /*  readline ��ʷɾ��           */
                          
        __tshellColorInit();                                            /*  ��ʼ�� shell ��ɫϵͳ       */
    }
    
    if (_G_bIsInstallSysCmd == LW_FALSE) {
        _G_bIsInstallSysCmd =  LW_TRUE;
        __tshellSysVarInit();                                           /*  ��ʼ��ϵͳ��������          */
        __tshellSysCmdInit();                                           /*  ��ʼ��ϵͳ����              */
        __tshellFsCmdInit();                                            /*  ��ʼ���ļ�ϵͳ����          */
        __tshellModemCmdInit();                                         /*  ��ʼ�� modem ����           */
        
#if defined(__SYLIXOS_LITE)
        __tshellExtCmdInit();
#endif                                                                  /*  __SYLIXOS_LITE              */
        
#if LW_CFG_SHELL_USER_EN > 0
        __tshellUserCmdInit();                                          /*  ��ʼ���û���������          */
#endif                                                                  /*  LW_CFG_SHELL_USER_EN        */
#if LW_CFG_SHELL_HEAP_TRACE_EN > 0
        __tshellHeapCmdInit();                                          /*  ��ʼ���ڴ������            */
#endif                                                                  /*  LW_CFG_SHELL_HEAP_TRACE_EN  */
#if LW_CFG_SYSPERF_EN > 0 && LW_CFG_SHELL_PERF_TRACE_EN > 0
        __tshellPerfCmdInit();                                          /*  ��ʼ�����ܷ�������          */
#endif                                                                  /*  LW_CFG_SHELL_PERF_TRACE_EN  */
#if LW_CFG_SHELL_TAR_EN > 0
        __tshellTarCmdInit();                                           /*  ��ʼ�� tar ����             */
#endif                                                                  /*  LW_CFG_SHELL_TAR_EN         */
    }
}
/*********************************************************************************************************
** ��������: API_TShellStartup
** ��������: ϵͳ����ʱ shell ���� startup.sh �ű�
** �䡡��  : NONE
** �䡡��  : ���н��
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
INT  API_TShellStartup (VOID)
{
    static BOOL     bInit      = LW_FALSE;
    INT             iOldOption = OPT_TERMINAL;
    INT             iNewOption;
    INT             iRet, iIgn = 0;
    INT             iWaitIgnSec;
    CHAR            cWaitIgnSec[16], cRead;
    struct timeval  tv;
    
    if (bInit) {
        return  (PX_ERROR);
    }
    bInit = LW_TRUE;
    
    if (API_TShellVarGetRt("STARTUP_WAIT_SEC", cWaitIgnSec, 16) > 0) {
        iWaitIgnSec = lib_atoi(cWaitIgnSec);
        if (iWaitIgnSec < 0) {
            iWaitIgnSec = 0;
        
        } else if (iWaitIgnSec > 10) {
            iWaitIgnSec = 10;
        }
        
    } else {
        iWaitIgnSec = 0;
    }
    
    printf("Press <n> to NOT execute /etc/startup.sh (timeout: %d sec(s))\n", iWaitIgnSec);
    
    tv.tv_sec  = iWaitIgnSec;
    tv.tv_usec = 0;
    
    ioctl(STD_IN, FIOGETOPTIONS, &iOldOption);
    iNewOption = iOldOption & ~(OPT_ECHO | OPT_LINE);                   /*  no echo no line mode        */
    ioctl(STD_IN, FIOSETOPTIONS, iNewOption);

    if (waitread(STD_IN, &tv) > 0) {
        if (read(STD_IN, &cRead, 1) == 1) {
            if ((cRead == 'n') || (cRead == 'N')) {
                iIgn = 1;
            }
        }
    }
    
    ioctl(STD_IN, FIOSETOPTIONS, iOldOption);

    if (iIgn) {
        printf("Abort executes /etc/startup.sh\n");
        iRet = PX_ERROR;
        
    } else {
        iRet = API_TShellExec("shfile /etc/startup.sh^");
    }
    
    return  (iRet);
}
/*********************************************************************************************************
** ��������: API_TShellSigEvent
** ��������: ��һ�� shell �ն˷����ź�.
** �䡡��  : ulShell               �ն��߳�
**           psigevent             �ź���Ϣ
**           iSigCode              �ź�
** �䡡��  : �����Ƿ�ɹ�.
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
#if LW_CFG_SIGNAL_EN > 0

LW_API  
ULONG  API_TShellSigEvent (LW_OBJECT_HANDLE  ulShell, struct sigevent *psigevent, INT  iSigCode)
{
    REGISTER PLW_CLASS_TCB    ptcbShell;
    REGISTER PLW_CLASS_TCB    ptcbJoin;
             LW_OBJECT_HANDLE ulJoin = LW_OBJECT_HANDLE_INVALID;
             UINT16           usIndex;
    
    usIndex = _ObjectGetIndex(ulShell);
    
    __KERNEL_ENTER();                                                   /*  �����ں�                    */
    if (_Thread_Invalid(usIndex)) {
        __KERNEL_EXIT();                                                /*  �˳��ں�                    */
        return  (ERROR_THREAD_NULL);
    }
    
    ptcbShell = __GET_TCB_FROM_INDEX(usIndex);
    ptcbJoin  = ptcbShell->TCB_ptcbJoin;
    if (ptcbJoin) {                                                     /*  �ȴ������߳̽���            */
        ulJoin = ptcbJoin->TCB_ulId;
    }
    __KERNEL_EXIT();                                                    /*  �˳��ں�                    */
    
    if (ulJoin) {
        _doSigEvent(ulJoin, psigevent, iSigCode);
    
    } else {
        _doSigEvent(ulShell, psigevent, iSigCode);
    }
    
    return  (ERROR_NONE);
}

#endif                                                                  /*  LW_CFG_SIGNAL_EN > 0        */
/*********************************************************************************************************
** ��������: API_TShellCreateEx
** ��������: ����һ�� ttiny shell ϵͳ, SylixOS ֧�ֶ���ն��豸ͬʱ����.
**           tshell ����ʹ�ñ�׼ tty �豸, ���� pty �����ն��豸.
** �䡡��  : iTtyFd                �ն��豸���ļ�������
**           ulOption              ��������
**           pfuncRunCallback      ��ʼ����Ϻ�, �����ص�
**           pvCbArg               �ص����� (��һ������Ϊ shell stdout, �˲���Ϊ�ڶ�������)
** �䡡��  : shell �̵߳ľ��.
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
LW_OBJECT_HANDLE  API_TShellCreateEx (INT  iTtyFd, ULONG  ulOption,
                                      FUNCPTR  pfuncRunCallback, PVOID  pvCbArg)
{
    LW_CLASS_THREADATTR     threadattrTShell;
    LW_OBJECT_HANDLE        hTShellHandle;
    INT                     iKernelFile;
    ULONG                   ulOldOption = ulOption;
    ULONG                   ulTaskOpt;
    PLW_CLASS_TCB           ptcbShell;
    
    _DebugHandle(__LOGMESSAGE_LEVEL, "ttiny shell system initialize...\r\n");
    
    if (!isatty(iTtyFd)) {                                              /*  ����Ƿ�Ϊ�ն��豸          */
        _DebugHandle(__ERRORMESSAGE_LEVEL,
                     "is not a tty or pty device.\r\n");
        _DebugHandle(__LOGMESSAGE_LEVEL, 
                     "ttiny shell system is not initialize.\r\n");
        return  (0);
    }
    
    if (__PROC_GET_PID_CUR() != 0) {                                    /*  �ڽ���������                */
        if (_G_hTShellLock == 0) {
            _DebugHandle(__ERRORMESSAGE_LEVEL,
                         "shell sub-system not initialization.\r\n");
            return  (0);
        }
        
        iKernelFile = dup2kernel(iTtyFd);                               /*  �����ļ������� dup �� kernel*/
        if (iKernelFile < 0) {
            return  (0);
        }
        
        ulOption         |= LW_OPTION_TSHELL_CLOSE_FD;                  /*  ִ����Ϻ���Ҫ�ر��ļ�      */
        pfuncRunCallback  = LW_NULL;                                    /*  ���̴������ð�װ�ص�        */
    
    } else {
        API_TShellInit();                                               /*  ��ʼ�� shell                */
        
        iKernelFile = iTtyFd;
    }
    
    ulTaskOpt = LW_CFG_SHELL_THREAD_OPTION | LW_OPTION_OBJECT_GLOBAL;
    if (!(ulOption & LW_OPTION_TSHELL_NODETACH)) {
        ulTaskOpt |= LW_OPTION_THREAD_DETACHED;
    }

    API_ThreadAttrBuild(&threadattrTShell,
                        _G_stShellStackSize,                            /*  shell ��ջ��С              */
                        LW_PRIO_T_SHELL,
                        ulTaskOpt,
                        (PVOID)(LONG)iKernelFile);                      /*  �������Կ�                  */
    
    hTShellHandle = API_ThreadInit("t_tshell", __tshellThread,
                                   &threadattrTShell, LW_NULL);         /*  ���� tshell �߳�            */
    if (!hTShellHandle) {
        if (__PROC_GET_PID_CUR() != 0) {                                /*  �ڽ���������                */
            __KERNEL_SPACE_ENTER();
            close(iKernelFile);                                         /*  �ں��йر� dup ���ں˵��ļ� */
            __KERNEL_SPACE_EXIT();
        }
        _DebugHandle(__ERRORMESSAGE_LEVEL, 
                     "tshell thread can not create.\r\n");
        _DebugHandle(__LOGMESSAGE_LEVEL, 
                     "ttiny shell system is not initialize.\r\n");
        return  (0);
        
    } else if ((__PROC_GET_PID_CUR() != 0) && 
               (ulOldOption & LW_OPTION_TSHELL_CLOSE_FD)) {             /*  ����ǽ��̴���, ��ر��ļ�  */
        close(iTtyFd);
    }
    
    ptcbShell = __GET_TCB_FROM_INDEX(_ObjectGetIndex(hTShellHandle));
    __TTINY_SHELL_SET_OPT(ptcbShell, ulOption);                         /*  ��ʼ��ѡ��                  */
    __TTINY_SHELL_SET_CALLBACK(ptcbShell, pfuncRunCallback);            /*  ��ʼ�������ص�              */
    __TTINY_SHELL_SET_CBARG(ptcbShell, pvCbArg);
    
    API_ThreadStart(hTShellHandle);                                     /*  ���� shell �߳�             */
    
    return  (hTShellHandle);
}
/*********************************************************************************************************
** ��������: API_TShellCreate
** ��������: ����һ�� ttiny shell ϵͳ, SylixOS ֧�ֶ���ն��豸ͬʱ����.
**           tshell ����ʹ�ñ�׼ tty �豸, ���� pty �����ն��豸.
** �䡡��  : iTtyFd    �ն��豸���ļ�������
**           ulOption  ��������
** �䡡��  : shell �̵߳ľ��.
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
LW_OBJECT_HANDLE  API_TShellCreate (INT  iTtyFd, ULONG  ulOption)
{
    return  (API_TShellCreateEx(iTtyFd, ulOption, LW_NULL, LW_NULL));
}
/*********************************************************************************************************
** ��������: API_TShellLogout
** ��������: ע����¼.
** �䡡��  : NONE
** �䡡��  : ERROR or OK
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
INT  API_TShellLogout (VOID)
{
    PLW_CLASS_TCB   ptcbCur;
    
    LW_TCB_GET_CUR_SAFE(ptcbCur);
    
    if (__TTINY_SHELL_GET_MAIN(ptcbCur)) {
        return  (__tshellRestartEx(__TTINY_SHELL_GET_MAIN(ptcbCur), LW_TRUE));
        
    } else {
        _ErrorHandle(ERROR_THREAD_NULL);
        return  (PX_ERROR);
    }
}
/*********************************************************************************************************
** ��������: API_TShellSetOption
** ��������: �����µ� shell ѡ��.
** �䡡��  : hTShellHandle   shell �߳�
**           ulNewOpt        �µ� shell ѡ��
** �䡡��  : ERROR or OK
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
INT  API_TShellSetOption (LW_OBJECT_HANDLE  hTShellHandle, ULONG  ulNewOpt)
{
    PLW_CLASS_TCB   ptcbShell;
    UINT16          usIndex;
    
    usIndex = _ObjectGetIndex(hTShellHandle);
    
#if LW_CFG_ARG_CHK_EN > 0
    if (!_ObjectClassOK(hTShellHandle, _OBJECT_THREAD)) {               /*  ��� ID ������Ч��          */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "thread handle invalidate.\r\n");
        _ErrorHandle(ERROR_KERNEL_HANDLE_NULL);
        return  (PX_ERROR);
    }
#endif

    __KERNEL_ENTER();                                                   /*  �����ں�                    */
    if (_Thread_Invalid(usIndex)) {
        __KERNEL_EXIT();                                                /*  �˳��ں�                    */
        _ErrorHandle(ERROR_KERNEL_HANDLE_NULL);
        return  (PX_ERROR);
    }
    
    ptcbShell = __GET_TCB_FROM_INDEX(usIndex);
    __TTINY_SHELL_SET_OPT(ptcbShell, ulNewOpt);
    __KERNEL_EXIT();                                                    /*  �˳��ں�                    */
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: API_TShellGetOption
** ��������: ��ȡ�µ� shell ѡ��.
** �䡡��  : hTShellHandle   shell �߳�
**           pulOpt          shell ѡ��
** �䡡��  : ERROR or OK
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
INT  API_TShellGetOption (LW_OBJECT_HANDLE  hTShellHandle, ULONG  *pulOpt)
{
    PLW_CLASS_TCB   ptcbShell;
    UINT16          usIndex;
    
    if (!pulOpt) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    
    usIndex = _ObjectGetIndex(hTShellHandle);
    
#if LW_CFG_ARG_CHK_EN > 0
    if (!_ObjectClassOK(hTShellHandle, _OBJECT_THREAD)) {               /*  ��� ID ������Ч��          */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "thread handle invalidate.\r\n");
        _ErrorHandle(ERROR_KERNEL_HANDLE_NULL);
        return  (PX_ERROR);
    }
#endif

    __KERNEL_ENTER();                                                   /*  �����ں�                    */
    if (_Thread_Invalid(usIndex)) {
        __KERNEL_EXIT();                                                /*  �˳��ں�                    */
        _ErrorHandle(ERROR_KERNEL_HANDLE_NULL);
        return  (PX_ERROR);
    }
    
    ptcbShell = __GET_TCB_FROM_INDEX(usIndex);
    *pulOpt   = __TTINY_SHELL_GET_OPT(ptcbShell);
    __KERNEL_EXIT();                                                    /*  �˳��ں�                    */
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: API_TShellGetUserName
** ��������: ͨ�� shell �û������ȡһ���û���.
** �䡡��  : uid           �û� id
**           pcName        �û���
**           stSize        ��������С
** �䡡��  : �������.
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
INT  API_TShellGetUserName (uid_t  uid, PCHAR  pcName, size_t  stSize)
{
    if (!pcName || !stSize) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    
    return  (__tshellGetUserName(uid, pcName, stSize, LW_NULL, 0));
}
/*********************************************************************************************************
** ��������: API_TShellGetGrpName
** ��������: ͨ�� shell �û������ȡһ������.
** �䡡��  : gid           �� id
**           pcName        ����
**           stSize        ��������С
** �䡡��  : �������.
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
INT  API_TShellGetGrpName (gid_t  gid, PCHAR  pcName, size_t  stSize)
{
    if (!pcName || !stSize) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    
    return  (__tshellGetGrpName(gid, pcName, stSize));
}
/*********************************************************************************************************
** ��������: API_TShellGetUserHome
** ��������: ���һ���û��� HOME ·��.
** �䡡��  : uid           �û� id
**           pcHome        HOME ·��
**           stSize        ��������С
** �䡡��  : �������.
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
INT  API_TShellGetUserHome (uid_t  uid, PCHAR  pcHome, size_t  stSize)
{
    if (!pcHome || !stSize) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    
    return  (__tshellGetUserName(uid, LW_NULL, 0, pcHome, stSize));
}
/*********************************************************************************************************
** ��������: API_TShellFlushCache
** ��������: ɾ������ shell ������û�������Ϣ (�����û�������䶯ʱ, ��Ҫ�ͷ� CACHE)
** �䡡��  : NONE
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
VOID  API_TShellFlushCache (VOID)
{
    __tshellFlushCache();
}
/*********************************************************************************************************
** ��������: API_TShellKeywordAdd
** ��������: �� ttiny shell ϵͳ���һ���ؼ���.
** �䡡��  : pcKeyword     �ؼ���
**           pfuncCommand  ִ�е� shell ����
** �䡡��  : �������.
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
ULONG  API_TShellKeywordAdd (CPCHAR  pcKeyword, PCOMMAND_START_ROUTINE  pfuncCommand)
{
    return  (API_TShellKeywordAddEx(pcKeyword, pfuncCommand, LW_OPTION_DEFAULT));
}
/*********************************************************************************************************
** ��������: API_TShellKeywordAddEx
** ��������: �� ttiny shell ϵͳ���һ���ؼ���.
** �䡡��  : pcKeyword     �ؼ���
**           pfuncCommand  ִ�е� shell ����
**           ulOption      ѡ��
** �䡡��  : �������.
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
ULONG  API_TShellKeywordAddEx (CPCHAR  pcKeyword, PCOMMAND_START_ROUTINE  pfuncCommand, ULONG  ulOption)
{
    REGISTER size_t    stStrLen;

    if (__PROC_GET_PID_CUR() != 0) {                                    /*  �����в���ע������          */
        _ErrorHandle(ENOTSUP);
        return  (ENOTSUP);
    }
    
    if (!pcKeyword) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "pcKeyword invalidate.\r\n");
        _ErrorHandle(ERROR_TSHELL_EPARAM);
        return  (ERROR_TSHELL_EPARAM);
    }
    
    stStrLen = lib_strnlen(pcKeyword, LW_CFG_SHELL_MAX_KEYWORDLEN);
    
    if (stStrLen >= (LW_CFG_SHELL_MAX_KEYWORDLEN + 1)) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "pcKeyword is too long.\r\n");
        _ErrorHandle(ERROR_TSHELL_EPARAM);
        return  (ERROR_TSHELL_EPARAM);
    }

    return  (__tshellKeywordAdd(pcKeyword, stStrLen, pfuncCommand, ulOption));
}
/*********************************************************************************************************
** ��������: API_TShellHelpAdd
** ��������: Ϊһ���ؼ�����Ӱ�����Ϣ
** �䡡��  : pcKeyword     �ؼ���
**           pcHelp        ������Ϣ�ַ���
** �䡡��  : �������.
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
ULONG  API_TShellHelpAdd (CPCHAR  pcKeyword, CPCHAR  pcHelp)
{
             __PTSHELL_KEYWORD   pskwNode = LW_NULL;
    REGISTER size_t              stStrLen;
    
    if (__PROC_GET_PID_CUR() != 0) {                                    /*  �����в��������            */
        _ErrorHandle(ENOTSUP);
        return  (ENOTSUP);
    }
    
    if (!pcKeyword) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "pcKeyword invalidate.\r\n");
        _ErrorHandle(ERROR_TSHELL_EPARAM);
        return  (ERROR_TSHELL_EPARAM);
    }
    if (!pcHelp) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "pcHelp invalidate.\r\n");
        _ErrorHandle(ERROR_TSHELL_EPARAM);
        return  (ERROR_TSHELL_EPARAM);
    }
    
    stStrLen = lib_strnlen(pcKeyword, LW_CFG_SHELL_MAX_KEYWORDLEN);
    
    if (stStrLen >= (LW_CFG_SHELL_MAX_KEYWORDLEN + 1)) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "pcKeyword is too long.\r\n");
        _ErrorHandle(ERROR_TSHELL_EPARAM);
        return  (ERROR_TSHELL_EPARAM);
    }
    
    if (ERROR_NONE == __tshellKeywordFind(pcKeyword, &pskwNode)) {      /*  ���ҹؼ���                  */
        
        __TTINY_SHELL_LOCK();                                           /*  �������                    */
        if (pskwNode->SK_pcHelpString) {
            __TTINY_SHELL_UNLOCK();                                     /*  �ͷ���Դ                    */
            
            _DebugHandle(__ERRORMESSAGE_LEVEL, "help message overlap.\r\n");
            _ErrorHandle(ERROR_TSHELL_OVERLAP);
            return  (ERROR_TSHELL_OVERLAP);
        }
        
        stStrLen = lib_strlen(pcHelp);                                  /*  Ϊ�����ִ����ٿռ�          */
        pskwNode->SK_pcHelpString = (PCHAR)__SHEAP_ALLOC(stStrLen + 1);
        if (!pskwNode->SK_pcHelpString) {
            _DebugHandle(__ERRORMESSAGE_LEVEL, "system low memory.\r\n");
            _ErrorHandle(ERROR_SYSTEM_LOW_MEMORY);
            return  (ERROR_SYSTEM_LOW_MEMORY);
        }
        lib_strcpy(pskwNode->SK_pcHelpString, pcHelp);
        __TTINY_SHELL_UNLOCK();                                         /*  �ͷ���Դ                    */
        return  (ERROR_NONE);
    
    } else {
        _ErrorHandle(ERROR_TSHELL_EKEYWORD);
        return  (ERROR_TSHELL_EKEYWORD);                                /*  �ؼ��ִ���                  */
    }
}
/*********************************************************************************************************
** ��������: API_TShellFormatAdd
** ��������: Ϊһ���ؼ�����Ӹ�ʽ�ִ���Ϣ
** �䡡��  : pcKeyword     �ؼ���
**           pcFormat      ��ʽ�ִ�
** �䡡��  : �������.
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
ULONG  API_TShellFormatAdd (CPCHAR  pcKeyword, CPCHAR  pcFormat)
{
             __PTSHELL_KEYWORD   pskwNode = LW_NULL;
    REGISTER size_t              stStrLen;
    
    if (__PROC_GET_PID_CUR() != 0) {                                    /*  �����в��������            */
        _ErrorHandle(ENOTSUP);
        return  (ENOTSUP);
    }
    
    if (!pcKeyword) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "pcKeyword invalidate.\r\n");
        _ErrorHandle(ERROR_TSHELL_EPARAM);
        return  (ERROR_TSHELL_EPARAM);
    }
    if (!pcFormat) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "pcHelp invalidate.\r\n");
        _ErrorHandle(ERROR_TSHELL_EPARAM);
        return  (ERROR_TSHELL_EPARAM);
    }
    
    stStrLen = lib_strnlen(pcKeyword, LW_CFG_SHELL_MAX_KEYWORDLEN);
    
    if (stStrLen >= (LW_CFG_SHELL_MAX_KEYWORDLEN + 1)) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "pcKeyword is too long.\r\n");
        _ErrorHandle(ERROR_TSHELL_EPARAM);
        return  (ERROR_TSHELL_EPARAM);
    }
    
    if (ERROR_NONE == __tshellKeywordFind(pcKeyword, &pskwNode)) {      /*  ���ҹؼ���                  */
        
        __TTINY_SHELL_LOCK();                                           /*  �������                    */
        if (pskwNode->SK_pcFormatString) {
            __TTINY_SHELL_UNLOCK();                                     /*  �ͷ���Դ                    */
            
            _DebugHandle(__ERRORMESSAGE_LEVEL, "format string overlap.\r\n");
            _ErrorHandle(ERROR_TSHELL_OVERLAP);
            return  (ERROR_TSHELL_OVERLAP);
        }
        
        stStrLen = lib_strlen(pcFormat);                                /*  Ϊ�����ִ����ٿռ�          */
        pskwNode->SK_pcFormatString = (PCHAR)__SHEAP_ALLOC(stStrLen + 1);
        if (!pskwNode->SK_pcFormatString) {
            _DebugHandle(__ERRORMESSAGE_LEVEL, "system low memory.\r\n");
            _ErrorHandle(ERROR_SYSTEM_LOW_MEMORY);
            return  (ERROR_SYSTEM_LOW_MEMORY);
        }
        lib_strcpy(pskwNode->SK_pcFormatString, pcFormat);
        __TTINY_SHELL_UNLOCK();                                         /*  �ͷ���Դ                    */
        return  (ERROR_NONE);
    
    } else {
        _ErrorHandle(ERROR_TSHELL_EKEYWORD);
        return  (ERROR_TSHELL_EKEYWORD);                                /*  �ؼ��ִ���                  */
    }
}
/*********************************************************************************************************
** ��������: API_TShellExec
** ��������: ttiny shell ϵͳ, ִ��һ�� shell ����
** �䡡��  : pcCommandExec    �����ַ���
** �䡡��  : �����ֵ(����������ʱ, ����ֵΪ��ֵ)
** ȫ�ֱ���: 
** ����ģ��: 
** ע  ��  : �� shell ���������ַ�������ʱ, ���᷵�ظ�ֵ, ��ֵȡ�෴����Ϊ������.

                                           API ����
*********************************************************************************************************/
LW_API  
INT  API_TShellExec (CPCHAR  pcCommandExec)
{
    if (__PROC_GET_PID_CUR() != 0) {
        return  (API_TShellExecBg(pcCommandExec, LW_NULL, LW_NULL, LW_TRUE, LW_NULL));
            
    } else {
        return  (__tshellExec(pcCommandExec, LW_NULL));
    }
}
/*********************************************************************************************************
** ��������: API_TShellExecBg
** ��������: ttiny shell ϵͳ, ����ִ��һ�� shell ���� (�����ɹ���񶼻�ر���Ҫ�رյ��ļ�)
** �䡡��  : pcCommandExec  �����ַ���
**           iFd[3]         ��׼�ļ�
**           bClosed[3]     ִ�н������Ƿ�رն�Ӧ��׼�ļ�
**           bIsJoin        �Ƿ�ȴ�����ִ�н���
**           pulSh          �����߳̾��, (���� bIsJoin = LW_FALSE ʱ����)
** �䡡��  : �����ֵ(����������ʱ, ����ֵΪ��ֵ)
** ȫ�ֱ���: 
** ����ģ��: 
** ע  ��  : �� shell ���������ַ�������ʱ, ���᷵�ظ�ֵ, ��ֵȡ�෴����Ϊ������.

                                           API ����
*********************************************************************************************************/
LW_API  
INT  API_TShellExecBg (CPCHAR  pcCommandExec, INT  iFd[3], BOOL  bClosed[3], 
                       BOOL  bIsJoin, LW_OBJECT_HANDLE *pulSh)
{
    INT     iRet;
    INT     iError;
    INT     iFdArray[3];
    BOOL    bClosedArray[3];
    
    CHAR    cCommand[LW_CFG_SHELL_MAX_COMMANDLEN + 1];
    size_t  stStrLen = lib_strnlen(pcCommandExec, LW_CFG_SHELL_MAX_COMMANDLEN + 1);
    BOOL    bNeedAsyn;
    
    if ((stStrLen > LW_CFG_SHELL_MAX_COMMANDLEN - 1) || (stStrLen < 1)) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    
    lib_strcpy(cCommand, pcCommandExec);
    
    if (__PROC_GET_PID_CUR() != 0) {                                    /*  �����д���                  */
        if (iFd == LW_NULL) {
            iFdArray[0] = dup2kernel(STD_IN);
            iFdArray[1] = dup2kernel(STD_OUT);
            iFdArray[2] = dup2kernel(STD_ERR);
        
        } else {
            iFdArray[0] = dup2kernel(iFd[0]);
            iFdArray[1] = dup2kernel(iFd[1]);
            iFdArray[2] = dup2kernel(iFd[2]);
        }
        
        /*
         *  ��������ļ��Ѿ� dup ���ں���, ����������Թرս����е���ص��ļ�
         */
        if (bClosed && iFd) {
            if (bClosed[0]) {
                close(iFd[0]);
            }
            if (bClosed[1]) {
                close(iFd[1]);
            }
            if (bClosed[2]) {
                close(iFd[2]);
            }
        }
        
        /*
         *  �����Ѿ� dup ���ں���, ������������н�����ر���Щ�ļ�
         */
        bClosedArray[0] = LW_TRUE;                                      /*  ִ����Ϻ���Ҫ�ر���Щ       */
        bClosedArray[1] = LW_TRUE;
        bClosedArray[2] = LW_TRUE;
        
        iFd     = iFdArray;
        bClosed = bClosedArray;                                         /*  �ļ��� dup �������������ȫ��*/
    
        __tshellPreTreatedBg(cCommand, LW_NULL, &bNeedAsyn);            /*  Ԥ������ִ����ز���       */
        
        if (bNeedAsyn) {
            bIsJoin = LW_FALSE;
        }
        
    } else {                                                            /*  �ں��е���                   */
        if (iFd == LW_NULL) {
            LW_OBJECT_HANDLE  ulMe = API_ThreadIdSelf();
            iFd = iFdArray;
            iFdArray[0] = API_IoTaskStdGet(ulMe, STD_IN);
            iFdArray[1] = API_IoTaskStdGet(ulMe, STD_OUT);
            iFdArray[2] = API_IoTaskStdGet(ulMe, STD_ERR);
        }
        
        if (bClosed == LW_NULL) {
            bClosed = bClosedArray;
            bClosedArray[0] = LW_FALSE;
            bClosedArray[1] = LW_FALSE;
            bClosedArray[2] = LW_FALSE;
        }
    }
    
    iError = __tshellBgCreateEx(iFd, bClosed, cCommand, lib_strlen(cCommand), 
                                0ul, bIsJoin, 0, pulSh, &iRet);
    if (iError < 0) {                                                   /*  ��������ʧ��                */
        /*
         *  ����ʧ��, ��ر��ں�����Ҫ�رյ��ļ�
         */
        __KERNEL_SPACE_ENTER();
        if (bClosed[0]) {
            close(iFd[0]);
        }
        if (bClosed[1]) {
            close(iFd[1]);
        }
        if (bClosed[2]) {
            close(iFd[2]);
        }
        __KERNEL_SPACE_EXIT();
    }                                                                   /*  ������гɹ�, ����Զ��ر�  */
    
    return  (iRet);
}
/*********************************************************************************************************
** ��������: API_TShellHookSet
** ��������: �� ttiny shell ����ע��һ���ص�����.
** �䡡��  : pfuncShellHook     �� shell ע��Ļص�����
** �䡡��  : ֮ǰ�Ļص�����
** ȫ�ֱ���:
** ����ģ��:
                                           API ����
*********************************************************************************************************/
#if LW_CFG_SHELL_HOOK_EN > 0

LW_API
FUNCPTR  API_TShellHookSet (FUNCPTR  pfuncShellHook)
{
    return  (__tshellThreadHook(pfuncShellHook));
}

#endif                                                                  /*  LW_CFG_SHELL_HOOK_EN > 0    */
#endif                                                                  /*  LW_CFG_SHELL_EN > 0         */
/*********************************************************************************************************
  END
*********************************************************************************************************/
