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
** ��   ��   ��: lwip_telnet.c
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2009 �� 05 �� 06 ��
**
** ��        ��: lwip telnet server ����.

** BUG:
2009.07.18  �� __telnetInput() �̵߳����ȼ�Ҫ�� shell ��ͬ, ����ȷ�� ascii ��������һ�ε������Ĵ��� shell.
            ��ЩӦ�ó�����Ҫ�����Ĳ���. ����: vi ���о����� ty ����ʱ, �ȼ����ͬ���ͼ���select�о�������
            �� shell ��ռ. 
            __telnetInput() ���� FIFO ���� (���ᱻͬ���ȼ���ռ).
2009.07.18  �������������. Ĭ�ϴ�� 4 ����(60 * 4)�����жϵ���.
2009.07.28  ����� LW_CFG_NET_TELNET_MAX_LINKS �������������֧��.
2009.09.04  ��Ҫ����, lwip ���߳�ͬʱ����һ�� socket ʱ, �б����Ŀ�����, ������ύ�� #bug:27377 
            (savannah.nongnu.org/bugs/index.php?27377) �����Ѿ������Ƽ�����ķ���.
            ֱ��1.3.1�汾�ǻ�û�н���������, ���Ե�ǰʹ�ö�· I/O ������ʽ��� telnet ȫ˫��ͨ������.
            ����ͬʱ������ fullduplex �汾(����Ŀ¼�� lwip_telnet.c.fulduplex �ļ�)
2009.09.05  �޸� telnet �����������, ȷ��ϵͳ������Ϊ�ļ�����ǰ�رձ���. ��֤�ȶ���.
2009.10.13  ���Ӽ��������� atomic_t ����.
2009.11.21  �����̴߳���ʧ�ܺ�, ��Ҫ�����Ӽ�����--.
2009.12.14  _G_cTelnetPtyStartName ��ʼ�����ܼ���ּ�Ŀ¼.
2011.02.21  ����ͷ�ļ�����, ���� POSIX ��׼.
2011.04.07  �������̴߳� /etc/services �л�ȡ telnet �ķ���˿�, ���û��, Ĭ��ʹ�� 23 �˿�.
2011.08.06  ������������Ϊ t_telnetd.
2012.03.26  __telnetCommunication() �Ż��˳�����. ���� t_ptyproc �̲߳���Ҫ��ʹ�� FIFO ����.
2013.05.08  pty Ĭ��Ŀ¼Ϊ /dev/pty Ŀ¼.
2014.10.22  ����Բ��� IAC ����Ĵ���.
2017.01.09  ���������¼����������.
2017.04.18  �������ڸı��źŷ��͵Ĵ���.
*********************************************************************************************************/
#define  __SYLIXOS_STDIO
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
#include "../SylixOS/system/include/s_system.h"
#include "../SylixOS/net/include/net_net.h"
/*********************************************************************************************************
  �ü�����
*********************************************************************************************************/
#if (LW_CFG_NET_EN > 0) && (LW_CFG_NET_TELNET_EN > 0) && (LW_CFG_SHELL_EN > 0)
#include "netdb.h"
#include "arpa/inet.h"
#include "net/if.h"
#include "sys/socket.h"
#include "sys/ioctl.h"
#if LW_CFG_NET_LOGINBL_EN > 0
#include "sys/loginbl.h"
#endif                                                                  /*  LW_CFG_NET_LOGINBL_EN > 0   */
#include "../iac/lwip_iac.h"
#include "../SylixOS/shell/ttinyShell/ttinyShellLib.h"
/*********************************************************************************************************
  ���� keepalive ��������
*********************************************************************************************************/
#define __LW_TELNET_TCP_KEEPIDLE            60                          /*  ����ʱ��, ��λ��            */
#define __LW_TELNET_TCP_KEEPINTVL           60                          /*  ����̽����ʱ���, ��λ��  */
#define __LW_TELNET_TCP_KEEPCNT             3                           /*  ̽�� N ��ʧ����Ϊ�ǵ���     */
/*********************************************************************************************************
  ��������
*********************************************************************************************************/
#define __LW_TELNET_PTY_MAX_FILENAME_LENGTH     (MAX_FILENAME_LENGTH - 10)
/*********************************************************************************************************
  ȫ�ֱ���
*********************************************************************************************************/
static CHAR           _G_cTelnetPtyStartName[MAX_FILENAME_LENGTH] = "/dev/pty";
static const CHAR     _G_cTelnetAbort[] = __TTINY_SHELL_FORCE_ABORT "\n";
static atomic_t       _G_atomicTelnetLinks;
static INT            _G_iListenSocket  = PX_ERROR;
/*********************************************************************************************************
  ��¼������
*********************************************************************************************************/
#if LW_CFG_NET_LOGINBL_EN > 0
static UINT16         _G_uiLoginFailPort  = 0;
static UINT           _G_uiLoginFailBlSec = 120;                        /*  ��������ʱ, Ĭ�� 120 ��     */
static UINT           _G_uiLoginFailBlRep = 3;                          /*  ������̽��Ĭ��Ϊ 3 �δ���   */
#endif                                                                  /*  LW_CFG_NET_LOGINBL_EN > 0   */
/*********************************************************************************************************
  IAC�����ʽ��
  FMT1 = IAC(BYTE) + CMD(BYTE)
  FMT2 = IAC(BYTE) + CMD(BYTE) + OPT(BYTE)
  FMT3 = IAC(BYTE) + SB(BYTE)  + PARAM(...) + IAC(BYTE) + SE(BYTE)
*********************************************************************************************************/
/*********************************************************************************************************
** ��������: __telnetIacProcesser
** ��������: telnet IAC ����ص���������Ӧ�ͻ��˷������� IAC ����
** �䡡��  : ulShell         shell �߳�.
**           pucIACBuff      IAC������ʵλ��
**           pucIACBuffEnd   ����������λ��
** �䡡��  : ����ռ�ó��ȣ�����������ʱӦ�������ĳ���
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT __telnetIacProcesser (LW_OBJECT_HANDLE   ulShell,
                                 PUCHAR             pucIACBuff,
                                 PUCHAR             pucIACBuffEnd)
{
    INT     iCmd;
    INT     iOpt;
    INT     iBuffLen;
    PUCHAR  pucTemp;
    
    if ((pucIACBuff    == LW_NULL)       ||
        (pucIACBuffEnd == LW_NULL)       ||
        (pucIACBuffEnd - pucIACBuff) < 2 ||
        (pucIACBuff[0] != LW_IAC_IAC)) {                                /*  ֻ����IAC����               */
        return  (1);                                                    /*  �������ֻ������ǰ�ַ�    */
    }
    
    iCmd     = pucIACBuff[1];
    iBuffLen = pucIACBuffEnd - pucIACBuff;

    switch (iCmd) {
    
    case LW_IAC_AYT:                                                    /*  Are you there����           */
        __inetIacSend(STD_OUT, LW_IAC_AYT, -1);
        return  (2);

    case LW_IAC_WILL:                                                   /*  Э��ѡ��                    */
    case LW_IAC_WONT:
    case LW_IAC_DO:
    case LW_IAC_DONT:
        if (iBuffLen >= 3) {
            iOpt = pucIACBuff[2];                                       /*  ѡ����                      */
        }
        return  (__MIN(3, iBuffLen));                                   /*  ����3���ֽڣ���ѡ������     */

    case LW_IAC_SB:                                                     /*  ����ѡ�����                */
        pucTemp = pucIACBuff + 2;
        while (pucTemp < pucIACBuffEnd) {
            if ((*pucTemp == LW_IAC_SE) &&
                (*(pucTemp - 1) == LW_IAC_IAC)) {                       /*  ��ѡ���� IAC SE����         */
                pucTemp++;
                break;
            }
            pucTemp++;
        }

        if ((pucIACBuff + 2) < pucTemp) {
            iOpt = pucIACBuff[2];                                       /*  ѡ����                      */
        } else {
            iOpt = LW_IAC_OPT_INVAL;
        }

        switch (iOpt) {
        
        case LW_IAC_OPT_NAWS:                                           /*  ���ڴ�С                    */
            if ((pucIACBuff + 4) < pucTemp) {
                INT             iFd = API_IoTaskStdGet(ulShell, STD_OUT);
                struct winsize  ws;
                
#if LW_CFG_SIGNAL_EN > 0
                struct sigevent sigevent;
#endif                                                                  /*  LW_CFG_SIGNAL_EN > 0        */
                
                ws.ws_row = (unsigned short)(((INT)pucIACBuff[5] << 8) + pucIACBuff[6]);
                ws.ws_col = (unsigned short)(((INT)pucIACBuff[3] << 8) + pucIACBuff[4]);
                ws.ws_xpixel = (unsigned short)(ws.ws_col *  8);
                ws.ws_ypixel = (unsigned short)(ws.ws_row * 16);
                ioctl(iFd, TIOCSWINSZ, &ws);                            /*  �����µĴ��ڴ�С            */
                
#if LW_CFG_SIGNAL_EN > 0
                sigevent.sigev_signo           = SIGWINCH;
                sigevent.sigev_value.sival_ptr = LW_NULL;
                sigevent.sigev_notify          = SIGEV_SIGNAL;
                API_TShellSigEvent(ulShell, &sigevent, SIGWINCH);       /*  ֪ͨӦ�ó���                */
#endif                                                                  /*  LW_CFG_SIGNAL_EN > 0        */
            }
            break;
        }
        return  (pucTemp - pucIACBuff);                                 /*  �����������                */

    case LW_IAC_USER_SDK:                                               /*  �ر���ɫ��ʾ�ͻ���          */
        {
            ULONG  ulOpt;
            API_TShellGetOption(ulShell, &ulOpt);
            ulOpt &= ~LW_OPTION_TSHELL_VT100;
            ulOpt |=  LW_OPTION_TSHELL_NOECHO;
            API_TShellSetOption(ulShell, ulOpt);
        }
        return  (2);

    default:
        return  (2);
    }
}
/*********************************************************************************************************
** ��������: __telnetCommunication
** ��������: telnet ��������˫���������. (lwip �ݲ�֧��ȫ˫�����̶߳�дͬ�׽��ֲ���)
**                                        (ȫ˫���� telnet ��������� lwip_telnet.c.fulduplex)
** �䡡��  : iDevFd      pty �豸��
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static VOID  __telnetCommunication (INT  iDevFd)
{
    CHAR        cPtyRBuffer[256];                                       /*  pty ���ջ���                */
    CHAR        cSockRBuffer[256];                                      /*  socket ���ջ���             */
    INT         iPtyReadNum;
    PCHAR       pcPtyBuffer;
    
    INT         iPtyLen      = 0;
    INT         iProcessLen  = 0;
    INT         iSockReadNum = 0;
    INT         iOverplus    = 0;
    
    fd_set      fdset;
    INT         iTemp;
    
    INT                 i;
    ssize_t             sstWriteDevNum = 0;
    LW_OBJECT_HANDLE    ulShell = API_ThreadGetNotePad(API_ThreadIdSelf(), 0);

    API_ThreadSetCancelState(LW_THREAD_CANCEL_ENABLE, LW_NULL);         /*  ����ȡ��                    */
    API_ThreadSetCancelType(LW_THREAD_CANCEL_DEFERRED, LW_NULL);        /*  ��������ɾ������            */
    
    FD_ZERO(&fdset);                                                    /*  �ļ������                  */
    
    for (;;) {
        FD_SET(STD_IN, &fdset);                                         /*  ����ӿڶ�                  */
        FD_SET(iDevFd, &fdset);                                         /*  pty �豸�˶�                */
    
        /*
         *  iDevFd һ������ STD_IN, ���� width = iDevFd + 1;
         */
        iTemp = select(iDevFd + 1, &fdset, LW_NULL, LW_NULL, LW_NULL);  /*  ���õȴ�                    */
        if (iTemp < 0) {
            if (errno != EINTR) {
                break;                                                  /*  ��������! ֱ���˳�          */
            }
            continue;

        } else if (iTemp == 0) {                                        /*  ��ʱ����?                   */
            continue;
        }
        
        API_ThreadTestCancel();                                         /*  ����Ƿ�ɾ��              */
        
        /*
         *  ��⴦�� pty �¼�
         */
        if (FD_ISSET(iDevFd, &fdset)) {
            iPtyReadNum = (INT)read(iDevFd, cPtyRBuffer, sizeof(cPtyRBuffer));
                                                                        /*  �� pty �豸�˶�������       */
            if (iPtyReadNum > 0) {
                write(STD_OUT, cPtyRBuffer, iPtyReadNum);               /*  д�� telnet ����            */

            } else {                                                    /*  ֹͣ shell ����             */
                write(iDevFd, (CPVOID)_G_cTelnetAbort, sizeof(_G_cTelnetAbort));
                break;
            }
        }
        
        /*
         *  ��⴦�����ڿɶ��¼�
         */
        if (FD_ISSET(STD_IN, &fdset)) {
            if (iOverplus > 0) {                                        /*  �ϴ� IAC �����Ƿ���ʣ���ַ� */
                lib_memcpy(cSockRBuffer, 
                           &cSockRBuffer[iSockReadNum - iOverplus],
                           iOverplus);                                  /*  ��û�д���������ƶ���ǰ��  */
            }
            iSockReadNum = (INT)read(STD_IN, 
                                     &cSockRBuffer[iOverplus], 
                                     sizeof(cSockRBuffer) - iOverplus); /*  ��������                    */
            if (iSockReadNum > 0) {
                iSockReadNum += iOverplus;                              /*  �ϲ���һ��û�н��������ݳ���*/
                pcPtyBuffer = __inetIacFilter(cSockRBuffer, 
                                              iSockReadNum, 
                                              &iPtyLen, 
                                              &iProcessLen, 
                                              __telnetIacProcesser, 
                                              (PVOID)ulShell);          /*  �˳� IAC �ֶ�               */
                if (pcPtyBuffer && (iPtyLen > 0)) {
                    if ((iPtyLen >= 2) &&
                        (pcPtyBuffer[iPtyLen - 2] == '\r') &&
                        (pcPtyBuffer[iPtyLen - 1] == '\n')) {           /*  ���� \n pty ���Զ��� \r ת��*/
                        iPtyLen--;
                    }
                    write(iDevFd, pcPtyBuffer, iPtyLen);                /*  д�� pty �ն�               */
                }
                if (iProcessLen > 0) {
                    iOverplus = iSockReadNum - iProcessLen;             /*  ����δ����������ݳ���      */
                }

            } else {
                if ((errno != ETIMEDOUT) && (errno != EWOULDBLOCK)) {   /*  ����Ͽ�                    */
                    break;                                              /*  ��Ҫ�˳�ѭ��                */
                }
            }
        }
    }
    
    /*
     *  ��������� shell �����˳�, ���һ�ε��� test cancel �ͻᱻɾ��
     *  ����������ж��˳�, ����Ҫ����һ�������� force abort ��ֹ shell �����˳�.
     */
    for (i = 0; ; i++) {
        API_ThreadTestCancel();                                         /*  ����Ƿ�ɾ��              */
                                                                        /*  ��� shell �˳�����ֱ��ɾ�� */
        if (sstWriteDevNum != sizeof(_G_cTelnetAbort)) {
            ioctl(iDevFd, FIOFLUSH);                                    /*  ��� PTY ������������       */
            sstWriteDevNum = write(iDevFd, (CPVOID)_G_cTelnetAbort, 
                                   sizeof(_G_cTelnetAbort));            /*  ֹͣ shell                  */
        }
        
#if LW_CFG_SIGNAL_EN > 0
        if (i == (LW_TICK_HZ * 3)) {                                    /*  ��� 3 �뻹û���˳�, kill   */
            union sigval        sigvalue;
            sigvalue.sival_int = PX_ERROR;
            sigqueue(ulShell, SIGABRT, sigvalue);                       /*  �����ź�, �쳣��ֹ          */
        }
#endif                                                                  /*  LW_CFG_SIGNAL_EN > 0        */
        
        API_TimeSleep(LW_OPTION_WAIT_A_TICK);
    }
}
/*********************************************************************************************************
** ��������: __telnetShellCallback
** ��������: telnet shell ������ص�
** �䡡��  : iStdOut   PTY ��׼���
**           iSock     ������ socket
** �䡡��  : ERROR_NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  __telnetShellCallback (INT  iStdOut, INT  iSock)
{
    struct sockaddr      sa;
    struct sockaddr_in  *psain;
#if LWIP_IPV6
    struct sockaddr_in6 *psain6;
#endif
    socklen_t            namelen = sizeof(struct sockaddr);
    CHAR                 cLocalAddr[48];
    CHAR                 cTitle[80];

    if (getsockname(iSock, &sa, &namelen) == ERROR_NONE) {
        if (sa.sa_family == AF_INET) {
            psain = (struct sockaddr_in *)&sa;
            if (inet_ntoa_r(psain->sin_addr, cLocalAddr, sizeof(cLocalAddr))) {
                snprintf(cTitle, sizeof(cTitle), "SylixOS Terminal %s", cLocalAddr);
                API_TShellSetTitle(iStdOut, cTitle);
            }

#if LWIP_IPV6
        } else if (sa.sa_family == AF_INET6) {
            psain6 = (struct sockaddr_in6 *)&sa;
            if (inet6_ntoa_r(psain6->sin6_addr, cLocalAddr, sizeof(cLocalAddr))) {
                snprintf(cTitle, sizeof(cTitle), "SylixOS Terminal %s", cLocalAddr);
                API_TShellSetTitle(iStdOut, cTitle);
            }
#endif
        }
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: __telnetServer
** ��������: telnet ������(������ɺ�, �ȴ� shell �߳̽���������Դ)
** �䡡��  : iSock     ������ socket
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static VOID  __telnetServer (INT  iSock)
{
    INT                 iErrLevel = 0;

    CHAR                cPtyName[MAX_FILENAME_LENGTH];
    INT                 iHostFd;
    INT                 iDevFd;
    PVOID               pvRetValue;
    
    ULONG               ulShellOption;
    LW_OBJECT_HANDLE    ulTShell;
    LW_OBJECT_HANDLE    ulCommunicatThread;
    
#if LW_CFG_NET_LOGINBL_EN > 0
    BOOL                bBlAdd = LW_FALSE;
    struct sockaddr     addr;
    socklen_t           namelen = sizeof(struct sockaddr);

    if (getpeername(iSock, &addr, &namelen) == ERROR_NONE) {
        ((struct sockaddr_in *)&addr)->sin_port = _G_uiLoginFailPort;
        bBlAdd = LW_TRUE;
    }
#endif                                                                  /*  LW_CFG_NET_LOGINBL_EN > 0   */
    
    sprintf(cPtyName, "%s/%d", _G_cTelnetPtyStartName, iSock);          /*  ���� PTY �豸��             */

    if (ptyDevCreate(cPtyName, LW_CFG_NET_TELNET_RBUFSIZE, LW_CFG_NET_TELNET_WBUFSIZE) < 0) {
        iErrLevel = 1;
        goto    __error_handle;
    }
    
    {
        CHAR    cPtyHostName[MAX_FILENAME_LENGTH];
        /*
         *  �� host ���ļ�.
         */
        sprintf(cPtyHostName, "%s.hst", cPtyName);
        iHostFd = open(cPtyHostName, O_RDWR);
        if (iHostFd < 0) {
            _DebugHandle(__ERRORMESSAGE_LEVEL, "pty host can not open.\r\n");
            iErrLevel = 2;
            goto    __error_handle;
        }
    }
    {
        CHAR    cPtyDevName[MAX_FILENAME_LENGTH];
        /*
         *  �� dev ���ļ�.
         */
        sprintf(cPtyDevName, "%s.dev", cPtyName);
        iDevFd = open(cPtyDevName, O_RDWR);
        if (iDevFd < 0) {
            _DebugHandle(__ERRORMESSAGE_LEVEL, "pty dev can not open.\r\n");
            iErrLevel = 3;
            goto    __error_handle;
        }
    }
    ioctl(iHostFd, FIOSETOPTIONS, OPT_TERMINAL);                        /*  host ʹ���ն˷�ʽ           */
    
    /*
     *  ���� PTY ��������߳�
     */
    {
        LW_CLASS_THREADATTR    threadattr;
            
        API_ThreadAttrBuild(&threadattr,
                            LW_CFG_NET_TELNET_STK_SIZE,
                            LW_PRIO_T_SERVICE,
                            LW_OPTION_THREAD_STK_CHK |
                            LW_OPTION_OBJECT_GLOBAL |
                            LW_OPTION_THREAD_DETACHED,
                            (PVOID)(LONG)iDevFd);
                                                                        /*  �� shell ���ȼ���ͬ         */
        threadattr.THREADATTR_ucPriority = LW_PRIO_T_SHELL;
        ulCommunicatThread = API_ThreadInit("t_ptyproc", 
                                            (PTHREAD_START_ROUTINE)__telnetCommunication, 
                                            &threadattr, 
                                            LW_NULL);                   /*  ���������߳�                */
        if (ulCommunicatThread == LW_OBJECT_HANDLE_INVALID) {
            iErrLevel = 4;
            goto    __error_handle;
        }
        
        API_IoTaskStdSet(ulCommunicatThread, STD_OUT, iSock);           /*  ��׼���Ϊ socket           */
        API_IoTaskStdSet(ulCommunicatThread, STD_IN,  iSock);           /*  ��׼����Ϊ socket           */
    }

    /*
     *  ���� TSHELL �����߳�
     */
    ulShellOption = LW_OPTION_TSHELL_VT100
                  | LW_OPTION_TSHELL_PROMPT_FULL
                  | LW_OPTION_TSHELL_NODETACH;

#if LW_CFG_NET_TELNET_LOGIN_EN > 0
    ulShellOption |= LW_OPTION_TSHELL_AUTHEN;
#endif                                                                  /*  LW_CFG_NET_TELNET_LOGIN_EN  */

    ulTShell = API_TShellCreateEx(iHostFd, ulShellOption,
                                  __telnetShellCallback,
                                  (PVOID)(LONG)iSock);                  /*  ���� Shell                  */
    if (ulTShell == LW_OBJECT_HANDLE_INVALID) {
        iErrLevel = 5;
        goto    __error_handle;
    }
    
    API_ThreadSetNotePad(ulCommunicatThread, 0, ulTShell);
    
    __inetIacSend(iSock, LW_IAC_DO,   LW_IAC_OPT_ECHO);
    __inetIacSend(iSock, LW_IAC_DO,   LW_IAC_OPT_LFLOW);
    __inetIacSend(iSock, LW_IAC_DO,   LW_IAC_OPT_NAWS);
    __inetIacSend(iSock, LW_IAC_WILL, LW_IAC_OPT_ECHO);                 /*  �������˻���                */
    __inetIacSend(iSock, LW_IAC_WILL, LW_IAC_OPT_SGA);                  /*  ����Զ�̻���ʼͨ��          */
    
    API_ThreadStart(ulCommunicatThread);                                /*  ���������߳�                */
    
#if LW_CFG_NET_TELNET_LOGFD_EN > 0
    API_LogFdAdd(iHostFd);
#endif                                                                  /*  LW_CFG_NET_TELNET_LOGFD_EN  */

    API_ThreadJoin(ulTShell, &pvRetValue);                              /*  �ȴ� shell �߳̽���         */
    
#if LW_CFG_NET_TELNET_LOGFD_EN > 0
    API_LogFdDelete(iHostFd);
#endif                                                                  /*  LW_CFG_NET_TELNET_LOGFD_EN  */

#if LW_CFG_NET_LOGINBL_EN > 0
    if ((INT)(LONG)pvRetValue == -ERROR_TSHELL_EUSER) {                 /*  �û���¼����                */
        if (bBlAdd) {
            API_LoginBlAdd(&addr, _G_uiLoginFailBlRep, _G_uiLoginFailBlSec);
        }
    }
#endif                                                                  /*  LW_CFG_NET_LOGINBL_EN > 0   */
    
    API_ThreadCancel(&ulCommunicatThread);                              /*  ֹ֪ͨͣ t_ptyproc �߳�     */

    /*
     *  ɾ�� pty �豸, ���� t_ptyproc �߳�.
     *  ptyDevRemove() ͬʱ�Ὣ pty �ļ�����ΪԤ�ر�ģʽ, ��֤ select() ��ȷ��.
     *  t_ptyproc �߳����е� cancel ��, ���Զ��ر�.
     */
    ptyDevRemove(cPtyName);
    close(iDevFd);
    close(iHostFd);

    iErrLevel = 0;                                                      /*  û���κδ���, ���ر� socket */
    
__error_handle:
    if (iErrLevel > 4) {
        API_ThreadDelete(&ulCommunicatThread, LW_NULL);
    }
    if (iErrLevel > 3) {
        close(iDevFd);
    }
    if (iErrLevel > 2) {
        close(iHostFd);
    }
    if (iErrLevel > 1) {
        ptyDevRemove(cPtyName);
    }
    close(iSock);
    
    API_AtomicDec(&_G_atomicTelnetLinks);                               /*  ��������--                  */
}
/*********************************************************************************************************
** ��������: __telnetListener
** ��������: telnet �������߳�
** �䡡��  : NONE
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static VOID  __telnetListener (VOID)
{
    INT                 iOne = 1;
    INT                 iSock;
    INT                 iSockNew;
    
    struct sockaddr_in  inaddrLcl;
    struct sockaddr_in  inaddrRmt;
    
    socklen_t           uiLen;
    
    struct servent     *pservent;
    
    inaddrLcl.sin_len         = sizeof(struct sockaddr_in);
    inaddrLcl.sin_family      = AF_INET;
    inaddrLcl.sin_addr.s_addr = INADDR_ANY;
    
    pservent = getservbyname("telnet", "tcp");
    if (pservent) {
        inaddrLcl.sin_port = (u16_t)pservent->s_port;
    } else {
        inaddrLcl.sin_port = htons(23);                                 /*  telnet default port         */
    }
    
#if LW_CFG_NET_LOGINBL_EN > 0
    _G_uiLoginFailPort = inaddrLcl.sin_port;
#endif                                                                  /*  LW_CFG_NET_LOGINBL_EN > 0   */

    iSock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (iSock < 0) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "can not create socket.\r\n");
        return;
    }
                                                                        /*  ��ʹ�� nagle �㷨           */
    setsockopt(iSock, IPPROTO_TCP, TCP_NODELAY, (const void *)&iOne, sizeof(iOne));
    
    if (bind(iSock, (struct sockaddr *)&inaddrLcl, sizeof(inaddrLcl))) {
        _DebugFormat(__ERRORMESSAGE_LEVEL, "can not bind socket %s.\r\n", lib_strerror(errno));
        close(iSock);
        return;
    }
    
    if (listen(iSock, LW_CFG_NET_TELNET_MAX_LINKS)) {
        _DebugFormat(__ERRORMESSAGE_LEVEL, "can not listen socket %s.\r\n", lib_strerror(errno));
        close(iSock);
        return;
    }
    
    _G_iListenSocket = iSock;                                           /*  ��¼���� socket             */

    for (;;) {
        iSockNew = accept(iSock, (struct sockaddr *)&inaddrRmt, &uiLen);
        if (iSockNew < 0) {
            if (errno != ENOTCONN) {
                _DebugFormat(__ERRORMESSAGE_LEVEL, "accept failed: %s.\r\n", lib_strerror(errno));
            }
            sleep(1);                                                   /*  �ӳ� 1 S                    */
            continue;
        }
        if (API_AtomicInc(&_G_atomicTelnetLinks) > LW_CFG_NET_TELNET_MAX_LINKS) {
            API_AtomicDec(&_G_atomicTelnetLinks);
            /*
             *  ��������������, �����ٴ����µ�����.
             */
            write(iSockNew, "server is full of links", lib_strlen("server is full of links"));
            close(iSockNew);
            sleep(1);                                                   /*  �ӳ� 1 S (��ֹ����)         */
            continue;
        }
        
        setsockopt(iSockNew, IPPROTO_TCP, TCP_NODELAY, (const void *)&iOne, sizeof(iOne));
        /*
         *  ���ñ��ʲ���.
         */
        {
            INT     iKeepIdle     = __LW_TELNET_TCP_KEEPIDLE;           /*  ����ʱ��                    */
            INT     iKeepInterval = __LW_TELNET_TCP_KEEPINTVL;          /*  ����̽����ʱ����        */
            INT     iKeepCount    = __LW_TELNET_TCP_KEEPCNT;            /*  ̽�� N ��ʧ����Ϊ�ǵ���     */
            
            setsockopt(iSockNew, SOL_SOCKET,  SO_KEEPALIVE,  (const void *)&iOne,          sizeof(INT));
            setsockopt(iSockNew, IPPROTO_TCP, TCP_KEEPIDLE,  (const void *)&iKeepIdle,     sizeof(INT));
            setsockopt(iSockNew, IPPROTO_TCP, TCP_KEEPINTVL, (const void *)&iKeepInterval, sizeof(INT));
            setsockopt(iSockNew, IPPROTO_TCP, TCP_KEEPCNT,   (const void *)&iKeepCount,    sizeof(INT));
        }
        
        /*
         *  �����������߳�
         */
        {
            LW_CLASS_THREADATTR    threadattr;
            
            API_ThreadAttrBuild(&threadattr,
                                LW_CFG_NET_TELNET_STK_SIZE,
                                LW_PRIO_T_SERVICE,
                                LW_OPTION_THREAD_STK_CHK | LW_OPTION_OBJECT_GLOBAL,
                                (PVOID)(LONG)iSockNew);
            if (API_ThreadCreate("t_ptyserver", 
                                 (PTHREAD_START_ROUTINE)__telnetServer, 
                                 &threadattr, 
                                 LW_NULL) == LW_OBJECT_HANDLE_INVALID) {
                API_AtomicDec(&_G_atomicTelnetLinks);
                close(iSockNew);                                        /*  �ر���ʱ����                */
            }
        }
    }
}
/*********************************************************************************************************
** ��������: API_INetTelnetInit
** ��������: ��ʼ�� telnet ����
** �䡡��  : pcPtyStartName    pty ��ʼ�ļ���, ����: "/dev/pty", ֮��ϵͳ������ pty Ϊ 
                                                     "/dev/pty/?.hst" 
                                                     "/dev/pty/?.dev"
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
VOID  API_INetTelnetInit (const PCHAR  pcPtyStartName)
{
    static   BOOL                   bIsInit = LW_FALSE;
    REGISTER size_t                 stNameLen;
             LW_CLASS_THREADATTR    threadattr;
             LW_OBJECT_HANDLE       ulId;
             
#if LW_CFG_NET_LOGINBL_EN > 0
             CHAR                   cEnvBuf[32];
#endif                                                                  /*  LW_CFG_NET_LOGINBL_EN > 0   */

    if (bIsInit) {
        return;
    }
    
    if (ptyDrv()) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "can not install pty driver.\r\n");
        return;
    }

    if (pcPtyStartName) {
        stNameLen = lib_strnlen(pcPtyStartName, __LW_TELNET_PTY_MAX_FILENAME_LENGTH);
        if (stNameLen >= __LW_TELNET_PTY_MAX_FILENAME_LENGTH) {
            _DebugHandle(__ERRORMESSAGE_LEVEL, "name too long.\r\n");
            return;
        
        } else if (stNameLen > 1) {                                     /*  ��Ϊ��Ŀ¼                  */
            lib_strcpy(_G_cTelnetPtyStartName, pcPtyStartName);         /*  pty �豸·��                */
            if (_G_cTelnetPtyStartName[stNameLen - 1] == PX_DIVIDER) {
                _G_cTelnetPtyStartName[stNameLen - 1] =  PX_EOS;        /*  ���� / ���Ž�β             */
            }
        }
    }
    
    API_AtomicSet(0, &_G_atomicTelnetLinks);                            /*  ������������                */
    
#if LW_CFG_NET_LOGINBL_EN > 0
    if (API_TShellVarGetRt("LOGINBL_TO", cEnvBuf, (INT)sizeof(cEnvBuf)) > 0) {
        _G_uiLoginFailBlSec = lib_atoi(cEnvBuf);
    }
    if (API_TShellVarGetRt("LOGINBL_REP", cEnvBuf, (INT)sizeof(cEnvBuf)) > 0) {
        _G_uiLoginFailBlRep = lib_atoi(cEnvBuf);
    }
#endif                                                                  /*  LW_CFG_NET_LOGINBL_EN > 0   */
    
    API_ThreadAttrBuild(&threadattr, 
                        LW_CFG_NET_TELNET_STK_SIZE, 
                        LW_PRIO_T_SERVICE,
                        (LW_OPTION_THREAD_STK_CHK | LW_OPTION_THREAD_SAFE | LW_OPTION_OBJECT_GLOBAL),
                        LW_NULL);
    ulId = API_ThreadCreate("t_telnetd", (PTHREAD_START_ROUTINE)__telnetListener, &threadattr, LW_NULL);
    if (ulId) {
        bIsInit = LW_TRUE;
    }
}
/*********************************************************************************************************
** ��������: API_INetTelnetBindDev
** ��������: ���� telnet ���������豸
** �䡡��  : uiIndex        �����豸����
** �䡡��  : ERROR
** ȫ�ֱ���:
** ����ģ��:
                                           API ����
*********************************************************************************************************/
LW_API
INT  API_INetTelnetBindDev (UINT  uiIndex)
{
    struct ifreq  ifreq;

    if (_G_iListenSocket < 0) {
        _ErrorHandle(ESRCH);
        return  (PX_ERROR);
    }

    if (uiIndex == 0) {
        ifreq.ifr_name[0] = '\0';

    } else if (uiIndex >= LW_CFG_NET_DEV_MAX) {
        _ErrorHandle(ENODEV);
        return  (PX_ERROR);

    } else {
        if (!if_indextoname(uiIndex, ifreq.ifr_name)) {
            return  (PX_ERROR);
        }
    }

    return  (setsockopt(_G_iListenSocket, SOL_SOCKET, SO_BINDTODEVICE,
                        (const void *)&ifreq, sizeof(ifreq)));
}

#endif                                                                  /*  (LW_CFG_NET_EN > 0)         */
                                                                        /*  (LW_CFG_NET_TELNET_EN > 0)  */
                                                                        /*  (LW_CFG_SHELL_EN > 0)       */
/*********************************************************************************************************
  END
*********************************************************************************************************/
