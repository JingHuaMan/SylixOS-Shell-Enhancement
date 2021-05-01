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
** ��   ��   ��: lwip_ftpd.c
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2009 �� 11 �� 21 ��
**
** ��        ��: ftp ������. (������ʾ���� SylixOS Ŀ¼)
** ע        ��: ftp ������ 1 ���Ӳ�����, �����Զ��Ͽ�, ���ͷ���Դ.
                 ��� ftp �������ĸ�Ŀ¼����ϵͳ��Ŀ¼, ��Щ�汾 windows ���ʴ˷�������Щ����������, 
                 ����: ������. �Ƽ��� ftp ��Ŀ¼����Ϊ /
                 �Ƽ�ʹ�� CuteFTP, FileZilla ��רҵ FTP �ͻ��˷��ʴ˷�����.
** BUG:
2009.11.29  ���ͻ���ʹ�� PORT ��ʽ����ʱ, ���� socket һ��Ҫʹ�� SO_REUSEADDR �������� 20 �˿ڹ��´�ʹ��.
2009.11.30  PWD ����ִ����ȷʱ, ��Ҫ�ظ� 257 ��Ϊ��ȷ����.
2010.07.10  ��ӡʱ����Ϣʱ, ʹ�� asctime_r() �����뺯��.
2011.02.13  �޸�ע��.
2011.02.21  ����ͷ�ļ�����, ���� POSIX ��׼.  
2011.03.04  ftp ֻ�ܴ��� S_IFREG �ļ�.
            ֧�ִ��� proc �ڵ��ļ�.
2011.04.07  �������̴߳� /etc/services �л�ȡ ftp �ķ���˿�, ���û��, Ĭ��ʹ�� 21 �˿�.
2012.01.06  ���� scanf �޷���ȡ���пո��·������.
2013.01.10  ���� LIST �������, ��֧����󳤶��ļ�.
2013.01.19  ֧�� REST �� APPE ����. ֧�ֶϵ�����.
            Ѹ�׵� FTP ����������, �����Ƕ�����ģʽ����ASCIIģʽ, ����Ҫ������ظ���βΪ \r\n. ������ FTP
            �ͻ���û�д�Ҫ��.
2013.05.10  �ϵ��ϴ�ʱ, ��׷��ģʽ�������ļ�������� O_TRUNC ѡ��, �򿪺��Զ����.
2015.03.18  �������Ӳ���ʹ�� SO_LINGER reset ģʽ�ر�.
2016.12.07  Ĭ��ʹ�ö�����ģʽ����.
2017.01.09  ���������¼����������.
*********************************************************************************************************/
#define  __SYLIXOS_STDIO
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
#include "../SylixOS/system/include/s_system.h"
#include "../SylixOS/net/include/net_net.h"
/*********************************************************************************************************
  �ü�����
*********************************************************************************************************/
#if (LW_CFG_NET_EN > 0) && (LW_CFG_NET_FTPD_EN > 0)
#include "ctype.h"
#include "shadow.h"
#include "netdb.h"
#include "arpa/inet.h"
#include "net/if.h"
#include "sys/socket.h"
#if LW_CFG_NET_LOGINBL_EN > 0
#include "sys/loginbl.h"
#endif                                                                  /*  LW_CFG_NET_LOGINBL_EN > 0   */
#include "lwip_ftp.h"
#include "lwip_ftplib.h"
/*********************************************************************************************************
  FTPD ������ѡ�� 
*********************************************************************************************************/
#define __LWIP_FTPD_ASCII_REPLY     1                                   /*  Ѹ����Ҫ������              */
/*********************************************************************************************************
  FTPD ����
*********************************************************************************************************/
#if LW_CFG_NET_FTPD_LOG_EN > 0
#define __LWIP_FTPD_LOG(s)          printf s                            /*  ��ӡ log ��Ϣ               */
#else
#define __LWIP_FTPD_LOG(s)
#endif                                                                  /*  LW_CFG_NET_FTPD_LOG_EN > 0  */
#define __LWIP_FTPD_BUFFER_SIZE     1024                                /*  FTP ������ջ�������С      */
#define __LWIP_FTPD_PATH_SIZE       MAX_FILENAME_LENGTH                 /*  �ļ�Ŀ¼�����С            */
#define __LWIP_FTPD_MAX_USER_LEN    (LW_CFG_SHELL_MAX_KEYWORDLEN + 1)   /*  �û�����󳤶�              */
#define __LWIP_FTPD_MAX_RETRY       10                                  /*  ������·�������Դ���        */
#define __LWIP_FTPD_SEND_SIZE       (32 * LW_CFG_KB_SIZE)               /*  �ļ����ͻ����С            */
#define __LWIP_FTPD_RECV_SIZE       (12 * LW_CFG_KB_SIZE)               /*  �ļ����ջ���                */
/*********************************************************************************************************
  FTPD Ŀ¼����
*********************************************************************************************************/
#define __LWIP_FTPD_ITEM__STR(s)    #s
#define __LWIP_FTPD_ITEM_STR(s)     __LWIP_FTPD_ITEM__STR(s)
#define __LWIP_FTPD_PATH_MAX_STR    __LWIP_FTPD_ITEM_STR(LW_CFG_PATH_MAX)
/*********************************************************************************************************
  FTPD �Ự״̬
*********************************************************************************************************/
#define __FTPD_SESSION_STATUS_LOGIN     0x0001                          /*  �û��ѵ�¼                  */
/*********************************************************************************************************
  FTPD �Ự���ƿ� (rtems ftpd like!)
*********************************************************************************************************/
typedef struct {
    LW_LIST_LINE                FTPDS_lineManage;                       /*  �Ự��������                */
    INT                         FTPDS_iStatus;                          /*  ��ǰ�Ự״̬                */
    CHAR                        FTPDS_cUser[__LWIP_FTPD_MAX_USER_LEN];  /*  ��½�û���                  */
    CHAR                        FTPDS_cPathBuf[MAX_FILENAME_LENGTH];    /*  ·������                    */
    off_t                       FTPDS_oftOffset;                        /*  �ϵ�����ƫ����              */
    
    struct sockaddr_in          FTPDS_sockaddrinCtrl;                   /*  �������ӵ�ַ                */
    struct sockaddr_in          FTPDS_sockaddrinData;                   /*  �������ӵ�ַ                */
    struct sockaddr_in          FTPDS_sockaddrinDefault;                /*  Ĭ�����ӵ�ַ                */
    
    BOOL                        FTPDS_bUseDefault;                      /*  �Ƿ�ʹ��Ĭ�ϵ�ַ������������*/
    struct in_addr              FTPDS_inaddrRemote;                     /*  Զ�̵�ַ, show ����ʹ��     */
    
    FILE                       *FTPDS_pfileCtrl;                        /*  �������ӻ���IO              */
    INT                         FTPDS_iSockCtrl;                        /*  �Ự socket                 */
    INT                         FTPDS_iSockPASV;                        /*  PASV socket                 */
    INT                         FTPDS_iSockData;                        /*  ���� socket                 */
    
    INT                         FTPDS_iTransMode;                       /*  ��֧�� ASCII / BIN          */
#define __FTP_TRANSMODE_ASCII   1
#define __FTP_TRANSMODE_EBCDIC  2
#define __FTP_TRANSMODE_BIN     3
#define __FTP_TRANSMODE_LOCAL   4
    
    LW_OBJECT_HANDLE            FTPDS_ulThreadId;                       /*  �����߳̾��                */
    time_t                      FTPDS_timeStart;                        /*  ����ʱʱ��                  */
} __FTPD_SESSION;
typedef __FTPD_SESSION         *__PFTPD_SESSION;
/*********************************************************************************************************
  ȫ�ֱ���
*********************************************************************************************************/
static PCHAR                    _G_pcFtpdRootPath         = LW_NULL;    /*  FTPd ��������Ŀ¼           */
static LW_LIST_LINE_HEADER      _G_plineFtpdSessionHeader = LW_NULL;    /*  �Ự���������ͷ            */
static atomic_t                 _G_atomicFtpdLinks;                     /*  ��������                    */
static INT                      _G_iListenSocket          = PX_ERROR;   /*  ���� socket                 */
static INT                      _G_iFtpdDefaultTimeout    = 20 * 1000;  /*  Ĭ���������ӳ�ʱʱ��        */
static INT                      _G_iFtpdIdleTimeout       = 60 * 1000;  /*  1 ���Ӳ����ʽ���رտ�������*/
static INT                      _G_iFtpdNoLoginTimeout    = 20 * 1000;  /*  20 ��ȴ���������           */
static LW_OBJECT_HANDLE         _G_ulFtpdSessionLock;                   /*  �Ự������                  */
#define __FTPD_SESSION_LOCK()   API_SemaphoreMPend(_G_ulFtpdSessionLock, LW_OPTION_WAIT_INFINITE)
#define __FTPD_SESSION_UNLOCK() API_SemaphoreMPost(_G_ulFtpdSessionLock)
/*********************************************************************************************************
  ��¼������
*********************************************************************************************************/
#if LW_CFG_NET_LOGINBL_EN > 0
static UINT16                   _G_uiLoginFailPort  = 0;
static UINT                     _G_uiLoginFailBlSec = 120;              /*  ��������ʱ, Ĭ�� 120 ��     */
static UINT                     _G_uiLoginFailBlRep = 3;                /*  ������̽��Ĭ��Ϊ 3 �δ���   */
#endif                                                                  /*  LW_CFG_NET_LOGINBL_EN > 0   */
/*********************************************************************************************************
  ���Ӳ�������
*********************************************************************************************************/
#define __LW_FTPD_TCP_KEEPIDLE   60                                     /*  ����ʱ��, ��λ��            */
#define __LW_FTPD_TCP_KEEPINTVL  60                                     /*  ����̽����ʱ���, ��λ��  */
#define __LW_FTPD_TCP_KEEPCNT    3                                      /*  ̽�� N ��ʧ����Ϊ�ǵ���     */
#define __LW_FTPD_TCP_BACKLOG    LW_CFG_NET_FTPD_MAX_LINKS              /*  listen backlog              */
/*********************************************************************************************************
  shell ��������
*********************************************************************************************************/
#if LW_CFG_SHELL_EN > 0
static INT  __tshellNetFtpdShow(INT  iArgC, PCHAR  *ppcArgV);
static INT  __tshellNetFtpdPath(INT  iArgC, PCHAR  *ppcArgV);
#endif                                                                  /*  LW_CFG_SHELL_EN > 0         */
/*********************************************************************************************************
  �ڲ���������
*********************************************************************************************************/
static VOID  __ftpdCloseSessionData(__PFTPD_SESSION  pftpds);
static VOID  __ftpdCommandAnalyse(PCHAR  pcBuffer, PCHAR *ppcCmd, PCHAR *ppcOpts, PCHAR *ppcArgs);
/*********************************************************************************************************
** ��������: __ftpdSendReply
** ��������: ftp ���ͻظ������ִ�
** �䡡��  : pftpds        �Ự���ƿ�
**           iCode         ��Ӧ����
**           cpcMessage    ������Ϣ
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static VOID  __ftpdSendReply (__PFTPD_SESSION  pftpds, INT  iCode, CPCHAR  cpcMessage)
{
#if __LWIP_FTPD_ASCII_REPLY > 0                                         /*  �ظ���Զ�� \r\n ��Ϊ��β    */
    if (cpcMessage != LW_NULL) {
        fprintf(pftpds->FTPDS_pfileCtrl, "%d %s\r\n", iCode, cpcMessage);
    } else {
        fprintf(pftpds->FTPDS_pfileCtrl, "%d\r\n", iCode);
    }

#else                                                                   /*  ����ģʽѡ���β����        */
    PCHAR  pcTail = (pftpds->FTPDS_iTransMode == __FTP_TRANSMODE_ASCII) ? "\r" : "";
    
    if (cpcMessage != LW_NULL) {
        fprintf(pftpds->FTPDS_pfileCtrl, "%d %s%s\n", iCode, cpcMessage, pcTail);
    } else {
        fprintf(pftpds->FTPDS_pfileCtrl, "%d%s\n", iCode, pcTail);
    }
#endif
    
    fflush(pftpds->FTPDS_pfileCtrl);                                    /*  ����Ϣ����                  */
}
/*********************************************************************************************************
** ��������: __ftpdSendModeReply
** ��������: ftp ���͵�ǰ����ģʽ�ִ�
** �䡡��  : pftpds        �Ự���ƿ�
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static VOID  __ftpdSendModeReply (__PFTPD_SESSION  pftpds)
{
    if (pftpds->FTPDS_iTransMode == __FTP_TRANSMODE_BIN) {
        __ftpdSendReply(pftpds, __FTPD_RETCODE_SERVER_FILE_OK, 
                        "Opening BINARY mode data connection.");
    } else {
        __ftpdSendReply(pftpds, __FTPD_RETCODE_SERVER_FILE_OK, 
                        "Opening ASCII mode data connection.");
    }
}
/*********************************************************************************************************
** ��������: __ftpdSendDirLine
** ��������: ftp ��� LIST ����, ����һ���������ļ���Ϣ�ظ�
** �䡡��  : iSock        ���Ͷ� socket
**           bWide        �Ƿ����ļ�������Ϣ (LW_FALSE ��ʾ�������ļ���)
**           timeNow      ��ǰʱ��
**           pcPath       Ŀ¼
**           pcAdd        ׷��Ŀ¼ (���������Ҫ��Ŀ¼ pcPath ������������Ŀ¼)
**           pcFileName   ��Ҫ������ļ���
**           pcBuffer     ��ʱ����
**           stSize       ��ʱ�����С
** �䡡��  : ���͵��ļ����� 1: ����һ�� 0: ���ʹ���
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static INT  __ftpdSendDirLine (INT      iSock, 
                               BOOL     bWide, 
                               time_t   timeNow, 
                               CPCHAR   pcPath, 
                               CPCHAR   pcAdd,
                               CPCHAR   pcFileName,
                               PCHAR    pcBuffer,
                               size_t   stSize)
{
    if (bWide) {                                                        /*  ��Ҫ����������Ϣ            */
        struct stat   statFile;
        size_t        stPathLen = lib_strlen(pcPath);
        size_t        stAddLen  = lib_strlen(pcAdd);
        
        if (stPathLen == 0) {
            pcBuffer[0] = PX_EOS;
        } else {
            lib_strcpy(pcBuffer, pcPath);
            if (stAddLen > 0 && pcBuffer[stPathLen - 1] != PX_DIVIDER) {
                pcBuffer[stPathLen++] = PX_DIVIDER;
                if (stPathLen >= stSize) {
                    return  (0);                                        /*  too long                    */
                }
                pcBuffer[stPathLen] = PX_EOS;
            }
        }
        if (stPathLen + stAddLen >= stSize) {
            return  (0);                                                /*  too long                    */
        }
        lib_strcpy(pcBuffer + stPathLen, pcAdd);                        /*  �ϳ�����Ŀ¼                */
        
        if (stat(pcBuffer, &statFile) == 0) {                           /*  �鿴Ŀ¼��Ϣ                */
            size_t      stLen;
            struct tm   tmFile;
            time_t      timeFile = statFile.st_mtime;                   /*  �ļ�ʱ�� (UTC)              */
            CHAR        cMode0;
            CHAR        cMode3;
            CHAR        cMode6;
            
            enum { 
                SIZE = 80 
            };
            enum { 
                SIX_MONTHS = 365 * 24 * 60 * 60 / 2
            };
            
            char  cTimeBuf[SIZE];
            
            lib_localtime_r(&timeFile, &tmFile);                        /*  ���� tm ʱ��ṹ            */
            
            if ((timeNow > timeFile + SIX_MONTHS) || (timeFile > timeNow + SIX_MONTHS)) {
                lib_strftime(cTimeBuf, SIZE, "%b %d  %Y", &tmFile);
            } else {
                lib_strftime(cTimeBuf, SIZE, "%b %d %H:%M", &tmFile);
            }
            
            switch (statFile.st_mode & S_IFMT) {                        /*  ��ʾ�ļ�����                */
            
            case S_IFIFO:  cMode0 = 'f'; break;
            case S_IFCHR:  cMode0 = 'c'; break;
            case S_IFDIR:  cMode0 = 'd'; break;
            case S_IFBLK:  cMode0 = 'b'; break;
            case S_IFREG:  cMode0 = '-'; break;
            case S_IFLNK:  cMode0 = 'l'; break;
            case S_IFSOCK: cMode0 = 's'; break;
            default: cMode0 = '-'; break;
            }
            
            if (statFile.st_mode & S_IXUSR) {                           /*  SETUID λ��Ϣ               */
                if (statFile.st_mode & S_ISUID) {
                    cMode3 = 's';
                } else {
                    cMode3 = 'x';
                }
            } else {
                if (statFile.st_mode & S_ISUID) {
                    cMode3 = 'S';
                } else {
                    cMode3 = '-';
                }
            }
            
            if (statFile.st_mode & S_IXGRP) {                           /*  SETGID λ��Ϣ               */
                if (statFile.st_mode & S_ISGID) {
                    cMode6 = 's';
                } else {
                    cMode6 = 'x';
                }
            } else {
                if (statFile.st_mode & S_ISGID) {
                    cMode6 = 'S';
                } else {
                    cMode6 = '-';
                }
            }
            
            /*
             *  �������͸�ʽ���ݰ�
             */
            stLen = bnprintf(pcBuffer, stSize, 0, 
                             "%c%c%c%c%c%c%c%c%c%c  1 %5d %5d %11qu %s %s\r\n",
                             cMode0,
                             (statFile.st_mode & S_IRUSR) ? ('r') : ('-'),
                             (statFile.st_mode & S_IWUSR) ? ('w') : ('-'),
                             cMode3,
                             (statFile.st_mode & S_IRGRP) ? ('r') : ('-'),
                             (statFile.st_mode & S_IWGRP) ? ('w') : ('-'),
                             cMode6,
                             (statFile.st_mode & S_IROTH) ? ('r') : ('-'),
                             (statFile.st_mode & S_IWOTH) ? ('w') : ('-'),
                             (statFile.st_mode & S_IXOTH) ? ('x') : ('-'),
                             (int)statFile.st_uid,
                             (int)statFile.st_gid,
                             statFile.st_size,
                             cTimeBuf,
                             pcFileName);
            if (send(iSock, pcBuffer, stLen, 0) != (ssize_t)stLen) {    /*  ������ϸ��Ϣ                */
                return  (0);
            }
        }
    } else {
        INT stLen = bnprintf(pcBuffer, stSize, 0, "%s\r\n", pcFileName);
        
        if (send(iSock, pcBuffer, stLen, 0) != (ssize_t)stLen) {        /*  �����ļ���                  */
            return  (0);
        }
    }
    
    return  (1);                                                        /*  �ɹ����͵�һ��              */
}
/*********************************************************************************************************
** ��������: __ftpdDatasocket
** ��������: ftp ������������
** �䡡��  : pftpds        �Ự���ƿ�
** �䡡��  : socket
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static INT  __ftpdDatasocket (__PFTPD_SESSION  pftpds)
{
    REGISTER INT    iSock = pftpds->FTPDS_iSockPASV;

    if (iSock < 0) {                                                    /*  ���û�н��� pasv ��������  */
        iSock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        if (iSock < 0) {
            __ftpdSendReply(pftpds, __FTPD_RETCODE_SERVER_DATALINK_FAILED,
                            "Can't create data socket.");
        } else {
            struct sockaddr_in  sockaddrinLocalData;
            INT                 i;
            INT                 iOn = 1;
            
            sockaddrinLocalData = pftpds->FTPDS_sockaddrinCtrl;
            sockaddrinLocalData.sin_port = htons(20);                   /*  �������ݶ˿ں�              */
        
            setsockopt(iSock, SOL_SOCKET, SO_REUSEADDR, &iOn, sizeof(iOn));
                                                                        /*  �������ص�ַ���´�ʹ��      */
            /*
             *  �������������ӿͻ���, ���ض˿ڱ����Ϊ 20
             */
            for (i = 0; i < __LWIP_FTPD_MAX_RETRY; i++) {
                errno = 0;
                if (bind(iSock, (struct sockaddr *)&sockaddrinLocalData, 
                         sizeof(sockaddrinLocalData)) >= 0) {
                    break;
                } else if (errno != EADDRINUSE) {                       /*  ������ǵ�ַ��ռ��          */
                    i = __LWIP_FTPD_MAX_RETRY;                          /*  ֱ�Ӵ����˳�                */
                } else {
                    sleep(1);                                           /*  �ȴ�һ��                    */
                }
            }
            
            if (i >= 10) {
                __ftpdSendReply(pftpds, __FTPD_RETCODE_SERVER_DATALINK_FAILED,
                                "Can't bind data socket.");
                close(iSock);
                iSock = -1;
            
            } else {
                /*
                 *  ����ͻ����Ѿ�ͨ�� PORT ����֪ͨ���Լ���������·��ַ, ��ô������������ָ���ĵ�ַ
                 *  ����ͻ���û��ָ��������·��ַ, ��ô������������Ĭ�����ݵ�ַ.
                 */
                struct sockaddr_in  *psockaddrinDataDest = (pftpds->FTPDS_bUseDefault) 
                                                         ? (&pftpds->FTPDS_sockaddrinDefault)
                                                         : (&pftpds->FTPDS_sockaddrinData);
                if (connect(iSock, (struct sockaddr *)psockaddrinDataDest, 
                            sizeof(struct sockaddr_in)) < 0) {
                    __ftpdSendReply(pftpds, __FTPD_RETCODE_SERVER_DATALINK_FAILED, 
                                    "Can't connect data socket.");
                    close(iSock);
                    iSock = -1;
                }
            }
        }
    }
    
    pftpds->FTPDS_iSockData   = iSock;
    pftpds->FTPDS_bUseDefault = LW_TRUE;
    if (iSock >= 0) {
        setsockopt(iSock, SOL_SOCKET, SO_RCVTIMEO, 
                   (const void *)&_G_iFtpdDefaultTimeout, 
                   sizeof(INT));                                        /*  ������������ղ���          */
    }
    
    return  (iSock);
}
/*********************************************************************************************************
** ��������: __ftpdCmdList
** ��������: ����: �ļ��б�
** �䡡��  : pftpds        �Ự���ƿ�
**           pcDir         ����Ŀ¼
** �䡡��  : ERROR
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static INT  __ftpdCmdList (__PFTPD_SESSION  pftpds, CPCHAR  pcFileName, INT  iWide)
{
    INT             iSock;
    struct stat     statFile;
    CHAR            cBuffer[__LWIP_FTPD_PATH_SIZE + 64];                /*  �������Գ���                */
    CHAR            cPath[__LWIP_FTPD_PATH_SIZE];
    
    DIR            *pdir    = LW_NULL;
    struct dirent  *pdirent = LW_NULL;
    time_t          timeNow;
    INT             iSuc = 1;

    __ftpdSendReply(pftpds, __FTPD_RETCODE_SERVER_FILE_OK, 
                    "Opening ASCII mode data connection for LIST.");
                    
    iSock = __ftpdDatasocket(pftpds);                                   /*  ����������� socket         */
    if (iSock < 0) {
        __LWIP_FTPD_LOG(("ftpd: error connecting to data socket. errno: %d\n", errno));
        return  (ERROR_NONE);
    }
    
    if (pcFileName[0] != PX_EOS) {                                      /*  ָ��Ŀ¼                    */
        lib_strcpy(cPath, pcFileName);
    } else {
        lib_strcpy(cPath, ".");                                         /*  ��ǰĿ¼                    */
    }
    
    if (stat(cPath, &statFile) < 0) {                                   /*  �鿴�ļ�����                */
        snprintf(cBuffer, sizeof(cBuffer),
                 "%s: No such file or directory.\r\n", pcFileName);
        send(iSock, cBuffer, lib_strlen(cBuffer), 0);
    
    } else if (S_ISDIR(statFile.st_mode) && 
               (LW_NULL == (pdir = opendir(pcFileName)))) {             /*  ��Ŀ¼�����޷���          */
        snprintf(cBuffer, sizeof(cBuffer),
                 "%s: Can not open directory.\r\n", pcFileName);
        send(iSock, cBuffer, lib_strlen(cBuffer), 0);
    
    } else {
        timeNow = lib_time(LW_NULL);
        if (!lib_strcmp(cPath, ".")) {
            cPath[0] = PX_EOS;                                          /*  ���Ϊ��ǰĿ¼����Ҫ      */
        }
        if (!pdir && *pcFileName) {
            iSuc = iSuc && __ftpdSendDirLine(iSock, iWide, timeNow, cPath, 
                                             pcFileName, pcFileName, 
                                             cBuffer, sizeof(cBuffer));
        } else {
            do {
                pdirent = readdir(pdir);
                if (pdirent == LW_NULL) {
                    break;
                }
                iSuc = iSuc && __ftpdSendDirLine(iSock, iWide, timeNow, cPath, 
                                                 pdirent->d_name, pdirent->d_name, 
                                                 cBuffer, sizeof(cBuffer));
            } while (iSuc && pdirent);
        }
    }
    
    if (pdir) {
        closedir(pdir);
    }
    __ftpdCloseSessionData(pftpds);
    
    if (iSuc) {
        __ftpdSendReply(pftpds, __FTPD_RETCODE_SERVER_DATACLOSE_NOERR, "Transfer complete.");
    } else {
        __ftpdSendReply(pftpds, __FTPD_RETCODE_SERVER_DATALINK_ABORT, "Connection aborted.");
    }
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: __ftpdCmdCwd
** ��������: ����: ���õ�ǰ����Ŀ¼
** �䡡��  : pftpds        �Ự���ƿ�
**           pcDir         ����Ŀ¼
** �䡡��  : ERROR
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static INT  __ftpdCmdCwd (__PFTPD_SESSION  pftpds, PCHAR  pcDir)
{
    INT     iError = ERROR_NONE;

    if (pcDir) {
        iError = access(pcDir, 0);                                      /*  ����̽���Ƿ���Է���        */
        if (iError == ERROR_NONE) {
            iError = cd(pcDir);                                         /*  ����Ŀ¼                    */
        }
    }
    
    if (iError == ERROR_NONE) {                                         /*  Ŀ¼����                    */
        __ftpdSendReply(pftpds, __FTPD_RETCODE_SERVER_FILE_OP_OK, "CWD command successful.");
    } else {
        __ftpdSendReply(pftpds, __FTPD_RETCODE_SERVER_REQ_FAILED, "CWD command failed.");
    }
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: __ftpdCmdCdup
** ��������: ����: CDUP
** �䡡��  : pftpds        �Ự���ƿ�
** �䡡��  : ERROR
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static INT  __ftpdCmdCdup (__PFTPD_SESSION  pftpds)
{
    getcwd(pftpds->FTPDS_cPathBuf, PATH_MAX);
    if (lib_strcmp(pftpds->FTPDS_cPathBuf, PX_STR_ROOT)) {              /*  ���Ǹ�Ŀ¼                  */
        cd("..");
        __ftpdSendReply(pftpds, __FTPD_RETCODE_SERVER_FILE_OP_OK, "CDUP command successful.");
    } else {
        __ftpdSendReply(pftpds, __FTPD_RETCODE_SERVER_REQ_FAILED, "CDUP command failed.");
    }
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: __ftpdCmdPort
** ��������: ����: PORT
** �䡡��  : pftpds        �Ự���ƿ�
**           pcArg         �������
** �䡡��  : ERROR
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static INT  __ftpdCmdPort (__PFTPD_SESSION  pftpds, PCHAR  pcArg)
{
    enum { 
        NUM_FIELDS = 6 
    };
    UINT    uiTemp[NUM_FIELDS];
    INT     iNum;
    
    __ftpdCloseSessionData(pftpds);                                     /*  �ر���������                */
    
    iNum = sscanf(pcArg, "%u,%u,%u,%u,%u,%u", uiTemp + 0, 
                                              uiTemp + 1, 
                                              uiTemp + 2, 
                                              uiTemp + 3, 
                                              uiTemp + 4, 
                                              uiTemp + 5);              /*  ע��: ����������Ϳ�        */
    if (NUM_FIELDS == iNum) {
        INT     i;
        UCHAR   ucTemp[NUM_FIELDS];

        for (i = 0; i < NUM_FIELDS; i++) {
            if (uiTemp[i] > 255) {
                break;
            }
            ucTemp[i] = (UCHAR)uiTemp[i];
        }
        if (i == NUM_FIELDS) {
            /*
             *  ���ﲻ���� port ������� IP ��ַ��ԭʼ�ͻ��˲����.
             */
            u32_t   uiIp = (u32_t)((ucTemp[0] << 24)
                         |         (ucTemp[1] << 16)
                         |         (ucTemp[2] <<  8)
                         |         (ucTemp[3]));                        /*  ���� IP ��ַ                */
            
            u16_t   usPort = (u16_t)((ucTemp[4] << 8)
                           |         (ucTemp[5]));                      /*  �����˿ں�                  */
            
            uiIp   = ntohl(uiIp);
            usPort = ntohs(usPort);
            
            if (uiIp == pftpds->FTPDS_sockaddrinDefault.sin_addr.s_addr) {
                pftpds->FTPDS_sockaddrinData.sin_addr.s_addr = uiIp;
                pftpds->FTPDS_sockaddrinData.sin_port        = usPort;  /*  �Ѿ�Ϊ������, ����ת��      */
                pftpds->FTPDS_sockaddrinData.sin_family      = AF_INET;
                lib_bzero(pftpds->FTPDS_sockaddrinData.sin_zero,
                          sizeof(pftpds->FTPDS_sockaddrinData.sin_zero));
               
                /*
                 *  ֮��������������ӽ�ʹ�ÿͻ���ָ���ĵ�ַ.
                 */
                pftpds->FTPDS_bUseDefault = LW_FALSE;                   /*  ����ʹ��Ĭ�ϵ�ַ            */
                __ftpdSendReply(pftpds, __FTPD_RETCODE_SERVER_CMD_OK, 
                                "PORT command successful.");
                
                return  (ERROR_NONE);
            
            } else {
                __ftpdSendReply(pftpds, __FTPD_RETCODE_SERVER_DATALINK_FAILED, 
                                "Address doesn't match peer's IP.");
                return  (ERROR_NONE);
            }
        }
    }

    __ftpdSendReply(pftpds, __FTPD_RETCODE_SERVER_SYNTAX_ERR, "Syntax error.");
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: __ftpdCmdPasv
** ��������: ����: PASV
** �䡡��  : pftpds        �Ự���ƿ�
**           pcArg         �������
** �䡡��  : ERROR
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static INT  __ftpdCmdPasv (__PFTPD_SESSION  pftpds)
{
    INT     iErrNo = 1;
    INT     iSock;

    __ftpdCloseSessionData(pftpds);                                     /*  �ر���������                */
    
    iSock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);                  /*  �����������Ӷ˵�            */
    if (iSock < 0) {
        __ftpdSendReply(pftpds, __FTPD_RETCODE_SERVER_DATALINK_FAILED, 
                        "Can't open passive socket.");
        __LWIP_FTPD_LOG(("ftpd: error creating PASV socket. errno: %d\n", errno));
        return  (PX_ERROR);
        
    } else {
        struct sockaddr_in  sockaddrin;
        socklen_t           uiAddrLen = sizeof(sockaddrin);

        sockaddrin          = pftpds->FTPDS_sockaddrinCtrl;
        sockaddrin.sin_port = htons(0);                                 /*  ��Ҫ�Զ�����˿�            */
        
        if (bind(iSock, (struct sockaddr *)&sockaddrin, uiAddrLen) < 0) {
            __LWIP_FTPD_LOG(("ftpd: error binding PASV socket. errno: %d\n", errno));
        
        } else if (listen(iSock, 1) < 0) {
            __LWIP_FTPD_LOG(("ftpd: error listening on PASV socket. errno: %d\n", errno));
        
        } else {
            CHAR            cArgBuffer[65];
            ip4_addr_t      ipaddr;
            u16_t           usPort;
            
            setsockopt(iSock, SOL_SOCKET, SO_RCVTIMEO, 
                       (const void *)&_G_iFtpdDefaultTimeout, 
                       sizeof(INT));                                    /*  ������������ղ���          */
                       
            getsockname(iSock, 
                        (struct sockaddr *)&sockaddrin, 
                        &uiAddrLen);                                    /*  ����������ӵ�ַ            */
            /*
             *  ������Ӧ����
             */
            ipaddr.addr = sockaddrin.sin_addr.s_addr;
            usPort      = ntohs(sockaddrin.sin_port);
            snprintf(cArgBuffer, sizeof(cArgBuffer), "Entering passive mode (%u,%u,%u,%u,%u,%u).",
                     ip4_addr1(&ipaddr), ip4_addr2(&ipaddr), ip4_addr3(&ipaddr), ip4_addr4(&ipaddr), 
                     (usPort >> 8), (usPort & 0xFF));
            
            __ftpdSendReply(pftpds, __FTPD_RETCODE_SERVER_INTO_PASV, cArgBuffer);
            
            /*
             *  ��ʼ�ȴ��ͻ���������
             */
            pftpds->FTPDS_iSockPASV = accept(iSock, (struct sockaddr *)&sockaddrin, &uiAddrLen);
            if (pftpds->FTPDS_iSockPASV < 0) {
                __LWIP_FTPD_LOG(("ftpd: error accepting PASV connection. errno: %d\n", errno));
            } else {                                                    /*  ���ӳɹ�                    */
                /*
                 *  �ر� listen socket �������������� socket
                 */
                close(iSock);
                iSock  = -1;
                iErrNo = 0;
                
                /*
                 *  ���ñ��ʲ���.
                 */
                {
                    INT     iOne          = 1;
                    INT     iKeepIdle     = __LW_FTPD_TCP_KEEPIDLE;     /*  ����ʱ��                    */
                    INT     iKeepInterval = __LW_FTPD_TCP_KEEPINTVL;    /*  ����̽����ʱ����        */
                    INT     iKeepCount    = __LW_FTPD_TCP_KEEPCNT;      /*  ̽�� N ��ʧ����Ϊ�ǵ���     */
                    
                    setsockopt(pftpds->FTPDS_iSockPASV, SOL_SOCKET,  SO_KEEPALIVE,  
                               (const void *)&iOne, sizeof(INT));
                    setsockopt(pftpds->FTPDS_iSockPASV, IPPROTO_TCP, TCP_KEEPIDLE,  
                               (const void *)&iKeepIdle, sizeof(INT));
                    setsockopt(pftpds->FTPDS_iSockPASV, IPPROTO_TCP, TCP_KEEPINTVL, 
                               (const void *)&iKeepInterval, sizeof(INT));
                    setsockopt(pftpds->FTPDS_iSockPASV, IPPROTO_TCP, TCP_KEEPCNT,
                               (const void *)&iKeepCount, sizeof(INT));
                }
            }
        }
    }
    
    if (iErrNo) {
        __ftpdSendReply(pftpds, __FTPD_RETCODE_SERVER_DATALINK_FAILED, 
                        "Can't open passive connection.");
        close(iSock);                                                   /*  �ر���ʱ����                */
    }
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: __ftpdCmdPwd
** ��������: ����: PWD
** �䡡��  : pftpds        �Ự���ƿ�
**           pcArg         �������
** �䡡��  : ERROR
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static INT  __ftpdCmdPwd (__PFTPD_SESSION  pftpds)
{
    CHAR    cBuffer[__LWIP_FTPD_PATH_SIZE + 40];
    PCHAR   pcCwd;
    
    errno = 0;
    cBuffer[0] = '"';
    pcCwd = getcwd(cBuffer + 1, __LWIP_FTPD_PATH_SIZE);                 /*  ��õ�ǰĿ¼                */
    if (pcCwd) {
        lib_strlcat(cBuffer, "\" is the current directory.", __LWIP_FTPD_PATH_SIZE + 40);
        __ftpdSendReply(pftpds, __FTPD_RETCODE_SERVER_MAKE_DIR_OK, cBuffer);
        
    } else {
        snprintf(cBuffer, __LWIP_FTPD_PATH_SIZE, "Error: %s\n", lib_strerror(errno));
        __ftpdSendReply(pftpds, __FTPD_RETCODE_SERVER_DONOT_RUN_REQ, cBuffer);
    }
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: __ftpdCmdMdtm
** ��������: ����: MDTM �����ļ�����ʱ����Ϣ
** �䡡��  : pftpds        �Ự���ƿ�
**           pcFileName    �ļ���
** �䡡��  : ERROR
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static INT  __ftpdCmdMdtm (__PFTPD_SESSION  pftpds, CPCHAR  pcFileName)
{
    struct stat   statBuf;
    CHAR          cBuffer[__LWIP_FTPD_PATH_SIZE];

    if (0 > stat(pcFileName, &statBuf)) {
        snprintf(cBuffer, __LWIP_FTPD_PATH_SIZE, "%s: error: %s.", pcFileName, lib_strerror(errno));
        __ftpdSendReply(pftpds, __FTPD_RETCODE_SERVER_REQ_FAILED, cBuffer);
    } else {
        struct tm   tmFile;
        
        lib_localtime_r(&statBuf.st_mtime, &tmFile);                    /*  get localtime               */
        snprintf(cBuffer, __LWIP_FTPD_PATH_SIZE, "%04d%02d%02d%02d%02d%02d",
                 1900 + tmFile.tm_year,
                 tmFile.tm_mon+1, tmFile.tm_mday,
                 tmFile.tm_hour, tmFile.tm_min, tmFile.tm_sec);
        __ftpdSendReply(pftpds, __FTPD_RETCODE_SERVER_FILE_STATUS, cBuffer);
    }
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: __ftpdCmdRnfr
** ��������: ����: RNFR �ļ�������
** �䡡��  : pftpds        �Ự���ƿ�
**           pcFileName    ��Ҫ���������ļ���
** �䡡��  : ERROR
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static INT  __ftpdCmdRnfr (__PFTPD_SESSION  pftpds, CPCHAR  pcFileName)
{
    CHAR            cBuffer[__LWIP_FTPD_PATH_SIZE + 32];                /*  �����µ��ļ���              */
    struct timeval  tmvalTO = {3, 0};                                   /*  �ȴ� 3 S                    */
    
    __ftpdSendReply(pftpds, __FTPD_RETCODE_SERVER_NEED_INFO, "RNFR need RNTO info.");
    
    if (1 == waitread(pftpds->FTPDS_iSockCtrl, &tmvalTO)) {             /*  �ȴ� RNTO ����              */
        PCHAR   pcCmd;
        PCHAR   pcOpts;
        PCHAR   pcArgs = LW_NULL;
        
        if (fgets(cBuffer, (__LWIP_FTPD_PATH_SIZE + 32), 
                  pftpds->FTPDS_pfileCtrl) != LW_NULL) {                /*  ���տ����ַ���              */
            
            __ftpdCommandAnalyse(cBuffer, &pcCmd, &pcOpts, &pcArgs);    /*  ��������                    */
            if (!lib_strcmp(pcCmd, "RNTO")) {
                if (pcArgs) {
                    if (rename(pcFileName, pcArgs) == 0) {
                        __ftpdSendReply(pftpds, __FTPD_RETCODE_SERVER_FILE_OP_OK, "RNTO complete.");
                    } else {
                        __ftpdSendReply(pftpds, __FTPD_RETCODE_SERVER_FILE_NAME_ERROR, "File name error.");
                    }
                    return  (ERROR_NONE);
                }
            }
        }
    }
    
    __ftpdSendReply(pftpds, __FTPD_RETCODE_SERVER_SYNTAX_ERR, "RNTO not follow by RNFR.");
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: __ftpdCmdSize
** ��������: ����: SIZE ��ȡ�ļ���С
** �䡡��  : pftpds        �Ự���ƿ�
**           pcFileName    �ļ���
** �䡡��  : ERROR
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static INT  __ftpdCmdSize (__PFTPD_SESSION  pftpds, CPCHAR  pcFileName)
{
    CHAR          cBuffer[__LWIP_FTPD_PATH_SIZE];
    struct stat   statBuf;
    
    if (0 > stat(pcFileName, &statBuf)) {
        snprintf(cBuffer, __LWIP_FTPD_PATH_SIZE, "%s: error: %s.", pcFileName, lib_strerror(errno));
        __ftpdSendReply(pftpds, __FTPD_RETCODE_SERVER_REQ_FAILED, cBuffer);
    } else {
        snprintf(cBuffer, __LWIP_FTPD_PATH_SIZE, "%lld", statBuf.st_size);
        __ftpdSendReply(pftpds, __FTPD_RETCODE_SERVER_FILE_STATUS, cBuffer);
    }
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: __ftpdCmdRest
** ��������: ����: REST �����ļ���ȡƫ����
** �䡡��  : pftpds        �Ự���ƿ�
**           pcOffset      ƫ����
** �䡡��  : ERROR
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static INT  __ftpdCmdRest (__PFTPD_SESSION  pftpds, CPCHAR  pcOffset)
{
    CHAR    cReplyBuffer[64];

    pftpds->FTPDS_oftOffset = lib_strtoll(pcOffset, LW_NULL, 10);
    
    snprintf(cReplyBuffer, 64,
             "Restarting at %lld. Send STORE or RETRIEVE to initiate transfer.", 
             pftpds->FTPDS_oftOffset);                                  /*  �ظ��ͻ���, �ļ�ָ���Ѿ�����*/
    
    __ftpdSendReply(pftpds, __FTPD_RETCODE_SERVER_NEED_INFO, cReplyBuffer);
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: __ftpdCmdRter
** ��������: ����: RETR ����һ���ļ����ͻ���
** �䡡��  : pftpds        �Ự���ƿ�
**           pcFileName    �ļ���
** �䡡��  : ERROR
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static INT  __ftpdCmdRter (__PFTPD_SESSION  pftpds, CPCHAR  pcFileName)
{
    INT         iSock;
    INT         iFd;
    struct stat statBuf;
    CHAR        cLocalBuffer[__LWIP_FTPD_BUFFER_SIZE];
    PCHAR       pcTransBuffer = LW_NULL;
    
    size_t      stBufferSize;
    INT         iResult = 0;
    off_t       oftOffset = pftpds->FTPDS_oftOffset;
    off_t       oftNiceSize;
    
    pftpds->FTPDS_oftOffset = 0;                                        /*  ��λƫ����                  */
    
    iFd = open(pcFileName, O_RDONLY);
    if (iFd < 0) {
        if (errno == EACCES) {
            __ftpdSendReply(pftpds, __FTPD_RETCODE_SERVER_REQ_FAILED, "Insufficient permissions.");
        } else {
            __ftpdSendReply(pftpds, __FTPD_RETCODE_SERVER_REQ_FAILED, "Error opening file.");
        }
        return  (ERROR_NONE);
    }
    if (0 > fstat(iFd, &statBuf)) {
        close(iFd);
        __ftpdSendReply(pftpds, __FTPD_RETCODE_SERVER_REQ_FAILED, "Error stating file.");
        return  (ERROR_NONE);
    }
    if (!S_ISREG(statBuf.st_mode)) {
        close(iFd);
        __ftpdSendReply(pftpds, __FTPD_RETCODE_SERVER_REQ_FAILED, "not a REG file.");
        return  (ERROR_NONE);
    }
    
    if (oftOffset) {
        if (oftOffset > statBuf.st_size) {                              /*  ƫ���������ļ�����С      */
            close(iFd);
            __ftpdSendReply(pftpds, __FTPD_RETCODE_SERVER_REQ_FAILED, "Offset overflow.");
            return  (ERROR_NONE);
        }
        if (lseek(iFd, oftOffset, SEEK_SET) == (off_t)PX_ERROR) {
            close(iFd);
            __ftpdSendReply(pftpds, __FTPD_RETCODE_SERVER_REQ_FAILED, "Offset invalidate.");
            return  (ERROR_NONE);
        }
    }
    
    oftNiceSize = statBuf.st_size - oftOffset;
    
    if (oftNiceSize < __LWIP_FTPD_BUFFER_SIZE) {
        pcTransBuffer = cLocalBuffer;                                   /*  ʹ�þֲ����崫��            */
        stBufferSize  = __LWIP_FTPD_BUFFER_SIZE;
    } else {
        stBufferSize  = (size_t)__MIN(__LWIP_FTPD_SEND_SIZE, oftNiceSize);
        pcTransBuffer = (PCHAR)__SHEAP_ALLOC(stBufferSize);             /*  ʹ�����Ż�������С          */
        if (pcTransBuffer == LW_NULL) {
            pcTransBuffer =  cLocalBuffer;
            stBufferSize  = __LWIP_FTPD_BUFFER_SIZE;                    /*  ����ʧ��, ʹ�ñ��ػ���      */
        }
    }
    
    __ftpdSendModeReply(pftpds);                                        /*  ֪ͨ�Է����䷽ʽ            */
    
    iSock = __ftpdDatasocket(pftpds);                                   /*  ����������� socket         */
    if (iSock >= 0) {
        INT     iN = -1;
        
        if (pftpds->FTPDS_iTransMode == __FTP_TRANSMODE_BIN) {          /*  ���������ʹ���              */
            while ((iN = (INT)read(iFd, (PVOID)pcTransBuffer, stBufferSize)) > 0) {
                if (send(iSock, pcTransBuffer, iN, 0) != iN) {
                    break;                                              /*  ����ʧ��                    */
                }
            }
            
        } else if (pftpds->FTPDS_iTransMode == __FTP_TRANSMODE_ASCII) { /*  �ı�����                    */
            INT     iReset = 0;
            
            while (iReset == 0 && (iN = (INT)read(iFd, (PVOID)pcTransBuffer, stBufferSize)) > 0) {
                PCHAR   e = pcTransBuffer;
                PCHAR   b;
                INT     i;
                
                iReset = iN;
                do {
                    CHAR  cLf = PX_EOS;

                    b = e;
                    for (i = 0; i < iReset; i++, e++) {
                        if (*e == '\n') {                               /*  �н����任                  */
                            cLf = '\n';
                            break;
                        }
                    }
                    if (send(iSock, b, i, 0) != i) {                    /*  ���͵�������                */
                        break;
                    }
                    if (cLf == '\n') {
                        if (send(iSock, "\r\n", 2, 0) != 2) {           /*  ������β                    */
                            break;
                        }
                        e++;
                        i++;
                    }
                } while ((iReset -= i) > 0);
            }
        }
        
        if (0 == iN) {                                                  /*  ��ȡ�������                */
            if (0 == close(iFd)) {
                iFd     = -1;
                iResult = 1;
            }
        }
    }
    
    if (iFd >= 0) {
        close(iFd);
    }
    
    if (pcTransBuffer && (pcTransBuffer != cLocalBuffer)) {
        __SHEAP_FREE(pcTransBuffer);
    }
    __ftpdCloseSessionData(pftpds);
    
    if (0 == iResult) {
        __ftpdSendReply(pftpds, __FTPD_RETCODE_SERVER_REQ_ABORT, "File read error.");
    } else {
        __ftpdSendReply(pftpds, __FTPD_RETCODE_SERVER_DATACLOSE_NOERR, "Transfer complete.");
    }
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: __ftpdCmdStor
** ��������: ����: STOR ����������һ���ļ�
** �䡡��  : pftpds        �Ự���ƿ�
**           pcFileName    �ļ���
**           iAppe         �Ƿ��� APPE ����
** �䡡��  : ERROR
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static INT  __ftpdCmdStor (__PFTPD_SESSION  pftpds, CPCHAR  pcFileName, INT  iAppe)
{
    INT         iSock;
    INT         iFd;
    struct stat statGet;
    CHAR        cLocalBuffer[__LWIP_FTPD_BUFFER_SIZE];
    PCHAR       pcTransBuffer = LW_NULL;
    INT         iBufferSize;
    INT         iResult = 0;
    
    INT         iN = 0;
    
    pftpds->FTPDS_oftOffset = 0;                                        /*  ��λƫ����                  */
    
    __ftpdSendModeReply(pftpds);                                        /*  ֪ͨ�Է����䷽ʽ            */
    
    iSock = __ftpdDatasocket(pftpds);                                   /*  ����������� socket         */
    if (iSock < 0) {
        __ftpdSendReply(pftpds, __FTPD_RETCODE_SERVER_REQ_FAILED, "Error creating data link.");
        return  (ERROR_NONE);
    }
    
    if (iAppe) {                                                        /*  ׷�ӷ�ʽ                    */
        iFd = open(pcFileName, 
                   (O_WRONLY | O_CREAT | O_APPEND), 
                   DEFAULT_FILE_PERM);                                  /*  û���ļ�Ҳ�½�              */
    } else {
        iFd = open(pcFileName, 
                   (O_WRONLY | O_CREAT | O_TRUNC), 
                   DEFAULT_FILE_PERM);                                  /*  �򿪻򴴽��ļ�(���)        */
    }
    if (iFd < 0) {
        if (errno == EACCES) {
            __ftpdSendReply(pftpds, __FTPD_RETCODE_SERVER_REQ_FAILED, "Insufficient permissions.");
        } else {
            __ftpdSendReply(pftpds, __FTPD_RETCODE_SERVER_REQ_FAILED, "Error creating file.");
        }
        __ftpdCloseSessionData(pftpds);
        return  (ERROR_NONE);
    }
    if ((fstat(iFd, &statGet) < 0) || S_ISDIR(statGet.st_mode)) {       /*  �������� DIR ͬ��           */
        close(iFd);                                                     /*  �ر��ļ�                    */
        __ftpdSendReply(pftpds, __FTPD_RETCODE_SERVER_REQ_FAILED, "Error creating file.");
        __ftpdCloseSessionData(pftpds);
        return  (ERROR_NONE);
    }
    
    pcTransBuffer = (PCHAR)__SHEAP_ALLOC(__LWIP_FTPD_RECV_SIZE);        /*  ���ٽ��ջ���                */
    if (pcTransBuffer) {
        iBufferSize = __LWIP_FTPD_RECV_SIZE;
    } else {
        pcTransBuffer = cLocalBuffer;
        iBufferSize = __LWIP_FTPD_BUFFER_SIZE;
    }
    
    if (pftpds->FTPDS_iTransMode == __FTP_TRANSMODE_BIN) {              /*  ���������ʹ���              */
        while ((iN = (INT)read(iSock, (PVOID)pcTransBuffer, (ssize_t)iBufferSize)) > 0) {
            if (write(iFd, pcTransBuffer, iN) != iN) {
                break;                                                  /*  ����ʧ��                    */
            }
        }
    
    } else if (pftpds->FTPDS_iTransMode == __FTP_TRANSMODE_ASCII) {     /*  �ı�����                    */
        while ((iN = (INT)read(iSock, (PVOID)pcTransBuffer, (ssize_t)iBufferSize)) > 0) {
            PCHAR   e = pcTransBuffer;
            PCHAR   b;
            INT     i;
            INT     iCounter = 0;
            
            do {
                INT     iCr = 0;
                b = e;
                for (i = 0; i < (iN - iCounter); i++, e++) {
                    if (*e == '\r') {
                        iCr = 1;
                        break;
                    }
                }
                if (i > 0) {
                    if (write(iFd, b, i) != i) {                        /*  ���浥������                */
                        goto    __recv_over;                            /*  ���մ���                    */
                    }
                }
                iCounter += (i + iCr);                                  /*  �Ѿ����������              */
                if (iCr) {
                    e++;                                                /*  ���� \r                     */
                }
            } while (iCounter < iN);
        }
    } else {
        iN = 1;                                                         /*  �����־                    */
    }
    
__recv_over:
    close(iFd);                                                         /*  �ر��ļ�                    */

    if (0 >= iN) {                                                      /*  �������                    */
        iResult = 1;
    } else if (!iAppe) {                                                /*  ��׷��ģʽ                  */
        unlink(pcFileName);                                             /*  ����ʧ��ɾ���ļ�            */
    }
    
    if (pcTransBuffer && (pcTransBuffer != cLocalBuffer)) {
        __SHEAP_FREE(pcTransBuffer);                                    /*  �ͷŻ�����                  */
    }
    
#if LW_CFG_NET_FTPD_AUTO_SYNC > 0
    sync();                                                             /*  ִ������ͬ��                */
#endif                                                                  /*  LW_CFG_NET_FTPD_AUTO_SYNC   */

    __ftpdCloseSessionData(pftpds);                                     /*  �ر���������                */
    
    if (0 == iResult) {
        __ftpdSendReply(pftpds, __FTPD_RETCODE_SERVER_DREQ_ABORT, "File write error.");
    } else {
        __ftpdSendReply(pftpds, __FTPD_RETCODE_SERVER_DATACLOSE_NOERR, "Transfer complete.");
    }
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: __ftpdCommandExec
** ��������: ִ������
** �䡡��  : pftpds        �Ự���ƿ�
**           pcCmd         ����
**           pcArg         ����
** �䡡��  : ERROR
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static INT  __ftpdCommandExec (__PFTPD_SESSION  pftpds, PCHAR  pcCmd, PCHAR  pcArg)
{
    CHAR    cFileName[MAX_FILENAME_LENGTH];                             /*  �ļ���                      */

#if (LW_CFG_NET_FTPD_LOGIN_EN > 0) && (LW_CFG_NET_LOGINBL_EN > 0)
    struct sockaddr_in  addr;
#endif                                                                  /*  LW_CFG_NET_LOGINBL_EN > 0   */

    if (!lib_strcmp("USER", pcCmd)) {                                   /*  �û���                      */
        lib_strlcpy(pftpds->FTPDS_cUser, pcArg, __LWIP_FTPD_MAX_USER_LEN);
        __ftpdSendReply(pftpds, __FTPD_RETCODE_SERVER_PW_REQ, 
                        "Password required.");                          /*  �ȴ���������                */
        pftpds->FTPDS_iStatus &= ~__FTPD_SESSION_STATUS_LOGIN;
        
    } else if (!lib_strcmp("PASS", pcCmd)) {                            /*  ��������                    */
#if LW_CFG_NET_FTPD_LOGIN_EN > 0
        if (userlogin(pftpds->FTPDS_cUser, pcArg, 1) < 0) {
            __ftpdSendReply(pftpds, __FTPD_RETCODE_SERVER_LOGIN_FAILED,
                            "Login failed.");                           /*  �û���֤ʧ��                */
            pftpds->FTPDS_iStatus &= ~__FTPD_SESSION_STATUS_LOGIN;
        
#if LW_CFG_NET_LOGINBL_EN > 0
            addr.sin_len    = sizeof(struct sockaddr_in);
            addr.sin_family = AF_INET;
            addr.sin_port   = _G_uiLoginFailPort;
            addr.sin_addr   = pftpds->FTPDS_inaddrRemote;
            API_LoginBlAdd((struct sockaddr *)&addr, _G_uiLoginFailBlRep, _G_uiLoginFailBlSec);
#endif                                                                  /*  LW_CFG_NET_LOGINBL_EN > 0   */
        } else
#endif                                                                  /*  LW_CFG_NET_FTPD_LOGIN_EN    */
        {
            setsockopt(pftpds->FTPDS_iSockCtrl, SOL_SOCKET, SO_RCVTIMEO, 
                       (const void *)&_G_iFtpdIdleTimeout, sizeof(INT));
            __ftpdSendReply(pftpds, __FTPD_RETCODE_SERVER_USER_LOGIN,
                            "User logged in.");
            pftpds->FTPDS_iStatus |= __FTPD_SESSION_STATUS_LOGIN;
        }
        
    } else if (!lib_strcmp("HELP", pcCmd)) {                            /*  ������Ϣ                    */
        /*
         *  TODO: Ŀǰ�ݲ���ӡ�����嵥.
         */
        goto    __command_not_understood;                               /*  ��ʱ��ʶ�������            */
         
    } else {
        /*
         *  ����Ĳ�����Ҫ��½
         */
        if ((pftpds->FTPDS_iStatus & __FTPD_SESSION_STATUS_LOGIN) == 0) {
            __ftpdSendReply(pftpds, __FTPD_RETCODE_SERVER_LOGIN_FAILED,
                            "USER and PASS required.");                 /*  ��Ҫ�û���¼                */
        } else {
            if (!lib_strcmp("CWD", pcCmd)) {                            /*  ���ù���Ŀ¼                */
                if ((pcArg != LW_NULL) && (pcArg[0] != PX_EOS)) {
                    lib_strlcpy(cFileName, pcArg, MAX_FILENAME_LENGTH);
                } else {
                    lib_strcpy(cFileName, ".");                         /*  ��ǰ�ļ�                    */
                }
                return  (__ftpdCmdCwd(pftpds, cFileName));
            
            } else if (!lib_strcmp("CDUP", pcCmd)) {                    /*  �����ϼ�Ŀ¼                */
                return  (__ftpdCmdCdup(pftpds));
            
            } else if (!lib_strcmp("PWD", pcCmd)) {                     /*  ��ǰĿ¼                    */
                return  (__ftpdCmdPwd(pftpds));
                
            } else if (!lib_strcmp("ALLO", pcCmd) ||
                       !lib_strcmp("ACCT", pcCmd)) {                    /*  �û�����ʶ��                */
                __ftpdSendReply(pftpds, __FTPD_RETCODE_SERVER_CMD_UNSUPPORT, 
                                "Allocate and Account not required.");
                return  (ERROR_NONE);
                
            } else if (!lib_strcmp("PORT", pcCmd)) {                    /*  ����������Ϣ                */
                return  (__ftpdCmdPort(pftpds, pcArg));
                
            } else if (!lib_strcmp("PASV", pcCmd)) {                    /*  PASV ����������Ϣ           */
                return  (__ftpdCmdPasv(pftpds));
                
            } else if (!lib_strcmp("TYPE", pcCmd)) {                    /*  �趨����ģʽ                */
                if (pcArg[0] == 'I') {
                    __ftpdSendReply(pftpds, __FTPD_RETCODE_SERVER_CMD_OK, 
                                    "Type set to I.");                  /*  ���֮ǰΪ A ģʽ, ����Ҫ \r*/
                    pftpds->FTPDS_iTransMode = __FTP_TRANSMODE_BIN;
                                    
                } else if (pcArg[0] == 'A') {
                    __ftpdSendReply(pftpds, __FTPD_RETCODE_SERVER_CMD_OK, 
                                    "Type set to A.");
                    pftpds->FTPDS_iTransMode = __FTP_TRANSMODE_ASCII;
                    
                } else {
                    __ftpdSendReply(pftpds, __FTPD_RETCODE_SERVER_UNSUP_WITH_ARG, 
                                    "Type not implemented. Set to I.");
                    pftpds->FTPDS_iTransMode = __FTP_TRANSMODE_BIN;
                }
                return  (ERROR_NONE);
                
            } else if (!lib_strcmp("LIST", pcCmd)) {                    /*  ��ӡ��ǰĿ¼�б�            */
                if ((pcArg != LW_NULL) && (pcArg[0] != PX_EOS)) {
                    lib_strlcpy(cFileName, pcArg, MAX_FILENAME_LENGTH);
                } else {
                    lib_strcpy(cFileName, ".");                         /*  ��ǰ�ļ�                    */
                }
                return  (__ftpdCmdList(pftpds, cFileName, 1));
                
            } else if (!lib_strcmp("NLST", pcCmd)) {                    /*  ��ӡ��ǰĿ¼�б�            */
                if ((pcArg != LW_NULL) && (pcArg[0] != PX_EOS)) {
                    lib_strlcpy(cFileName, pcArg, MAX_FILENAME_LENGTH);
                } else {
                    lib_strcpy(cFileName, ".");                         /*  ��ǰ�ļ�                    */
                }
                return  (__ftpdCmdList(pftpds, cFileName, 0));          /*  ����ӡ                    */
                
            } else if (!lib_strcmp("RNFR", pcCmd)) {                    /*  �ļ��ø���                  */
                return  (__ftpdCmdRnfr(pftpds, pcArg));
                
            } else if (!lib_strcmp("REST", pcCmd)) {                    /*  �ļ�ƫ����                  */
                return  (__ftpdCmdRest(pftpds, pcArg));
                
            } else if (!lib_strcmp("RETR", pcCmd)) {                    /*  �ļ�����                    */
                return  (__ftpdCmdRter(pftpds, pcArg));
            
            } else if (!lib_strcmp("STOR", pcCmd)) {                    /*  �ļ�����                    */
                return  (__ftpdCmdStor(pftpds, pcArg, 0));
                
            } else if (!lib_strcmp("APPE", pcCmd)) {                    /*  �ļ�׷�ӽ���                */
                return  (__ftpdCmdStor(pftpds, pcArg, 1));
            
            } else if (!lib_strcmp("SYST", pcCmd)) {                    /*  ѯ��ϵͳ����                */
                __ftpdSendReply(pftpds, __FTPD_RETCODE_SERVER_NAME_SYS_TYPE, 
                                __FTPD_SYSTYPE);
                return  (ERROR_NONE);
            
            } else if (!lib_strcmp("MKD", pcCmd)) {                     /*  ����һ��Ŀ¼                */
                if (mkdir(pcArg, DEFAULT_DIR_PERM) < 0) {
                    __ftpdSendReply(pftpds, __FTPD_RETCODE_SERVER_REQ_FAILED, 
                                    "MKD failed.");
                } else {
                    __ftpdSendReply(pftpds, __FTPD_RETCODE_SERVER_MAKE_DIR_OK, 
                                    "MKD successful.");
                }
                return  (ERROR_NONE);
                
            } else if (!lib_strcmp("RMD", pcCmd)) {                     /*  ɾ��һ��Ŀ¼                */
                if (rmdir(pcArg) < 0) {
                    __ftpdSendReply(pftpds, __FTPD_RETCODE_SERVER_REQ_FAILED, 
                                    "RMD failed.");
                } else {
                    __ftpdSendReply(pftpds, __FTPD_RETCODE_SERVER_MAKE_DIR_OK, 
                                    "RMD successful.");
                }
                return  (ERROR_NONE);
            } else if (!lib_strcmp("DELE", pcCmd)) {                    /*  ɾ��һ���ļ�                */
                if (unlink(pcArg) < 0) {
                    __ftpdSendReply(pftpds, __FTPD_RETCODE_SERVER_REQ_FAILED, 
                                    "DELE failed.");
                } else {
                    __ftpdSendReply(pftpds, __FTPD_RETCODE_SERVER_MAKE_DIR_OK, 
                                    "DELE successful.");
                }
                return  (ERROR_NONE);
                
            } else if (!lib_strcmp("MDTM", pcCmd)) {                    /*  �ļ�ʱ�����                */
                return  (__ftpdCmdMdtm(pftpds, pcArg));
                
            } else if (!lib_strcmp("SIZE", pcCmd)) {                    /*  ��ȡ�ļ���С                */
                return  (__ftpdCmdSize(pftpds, pcArg));
                
            } else if (!lib_strcmp("SITE", pcCmd)) {                    /*  վ�����                    */
                PCHAR   pcOpts;
                
                __ftpdCommandAnalyse(pcArg, &pcCmd, &pcOpts, &pcArg);
                if (!lib_strcmp("CHMOD", pcCmd)) {                      /*  �޸��ļ� mode               */
                    INT     iMask = DEFAULT_FILE_PERM;
                    if ((2 == sscanf(pcArg, "%o %" __LWIP_FTPD_PATH_MAX_STR "[^\n]", 
                                     &iMask, cFileName)) &&
                        (0 == chmod(cFileName, (mode_t)iMask))) {
                        __ftpdSendReply(pftpds, __FTPD_RETCODE_SERVER_MAKE_DIR_OK, 
                                        "CHMOD successful.");
                    } else {
                        __ftpdSendReply(pftpds, __FTPD_RETCODE_SERVER_REQ_FAILED, 
                                        "CHMOD failed.");
                    }
                    return  (ERROR_NONE);
                
                } else if (!lib_strcmp("SYNC", pcCmd)) {                /*  ��д����                    */
                    sync();
                    __ftpdSendReply(pftpds, __FTPD_RETCODE_SERVER_CMD_OK, 
                                    "SYNC successful.");
                    return  (ERROR_NONE);
                }
                goto    __command_not_understood;
                
            } else if (!lib_strcmp("NOOP", pcCmd)) {                    /*  ʲô������                  */
                __ftpdSendReply(pftpds, __FTPD_RETCODE_SERVER_CMD_OK, 
                                "NOOP -- did nothing as requested.");
                return  (ERROR_NONE);
            }

__command_not_understood:
            __ftpdSendReply(pftpds, __FTPD_RETCODE_SERVER_CMD_ERROR, 
                            "Command not understood.");                 /*  �ݲ��ɱ�ʶ�������          */
        }
    }
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: __ftpdCloseSessionCtrl
** ��������: �ر�һ���Ự���ƿ��������
** �䡡��  : pftpds        �Ự���ƿ�
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static VOID  __ftpdCloseSessionCtrl (__PFTPD_SESSION  pftpds)
{
    /*
     *  fclose() ���Զ����� close() �ر�����.
     */
    if (pftpds->FTPDS_pfileCtrl) {
        fclose(pftpds->FTPDS_pfileCtrl);                                /*  �رտ��ƶ��ļ�              */
    } else {
        close(pftpds->FTPDS_iSockCtrl);                                 /*  �رտ��� socket             */
    }
    
    pftpds->FTPDS_pfileCtrl = LW_NULL;
    pftpds->FTPDS_iSockCtrl = -1;
}
/*********************************************************************************************************
** ��������: __ftpdCloseSessionData
** ��������: �ر�һ���Ự���ƿ���������
** �䡡��  : pftpds        �Ự���ƿ�
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static VOID  __ftpdCloseSessionData (__PFTPD_SESSION  pftpds)
{
    /*
     *  ֻ���ܴ���һ����������ģʽ.
     */
    if (pftpds->FTPDS_iSockData > 0) {
        close(pftpds->FTPDS_iSockData);                                 /*  �ر���������                */
    
    } else if (pftpds->FTPDS_iSockPASV > 0) {
        close(pftpds->FTPDS_iSockPASV);                                 /*  �ر� PASV ��������          */
    }
    
    pftpds->FTPDS_iSockData   = -1;
    pftpds->FTPDS_iSockPASV   = -1;
    pftpds->FTPDS_bUseDefault = LW_TRUE;
}
/*********************************************************************************************************
** ��������: __ftpdSkipOpt
** ��������: ����ѡ���ֶ�
** �䡡��  : pcTemp        ѡ��ָ���ַ
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static VOID  __ftpdSkipOpt (PCHAR  *ppcTemp)
{
    PCHAR     pcBuf  = *ppcTemp;
    PCHAR     pcLast = LW_NULL;
  
    for (;;) {
        while (isspace(*pcBuf)) {                                       /*  ����                        */
            ++pcBuf;
        }
        
        if (*pcBuf == '-') {
            if (*++pcBuf == '-') {                                      /* `--' should terminate options*/
                if (isspace(*++pcBuf)) {
                    pcLast = pcBuf;
                    do {
                        ++pcBuf;
                    } while (isspace(*pcBuf));
                    break;
                }
            }
            while (*pcBuf && !isspace(*pcBuf)) {
                ++pcBuf;
            }
            pcLast = pcBuf;
        } else {
            break;
        }
    }
    if (pcLast) {
        *pcLast = PX_EOS;
    }
    *ppcTemp = pcBuf;
}
/*********************************************************************************************************
** ��������: __ftpdCommandAnalyse
** ��������: ��������
** �䡡��  : pcBuffer        �����ִ�
**           ppcCmd          ����ָ��
**           ppcOpts         ѡ��ָ��
**           ppcArgs         ����ָ��
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static VOID  __ftpdCommandAnalyse (PCHAR  pcBuffer, PCHAR *ppcCmd, PCHAR *ppcOpts, PCHAR *ppcArgs)
{
    PCHAR     pcEoc;
    PCHAR     pcTemp = pcBuffer;
  
    while (isspace(*pcTemp)) {                                          /*  ����ǰ�������ַ�            */
        ++pcTemp;
    }
    
    *ppcCmd = pcTemp;                                                   /*  ��¼������ʵ��ַ            */
    while (*pcTemp && !isspace(*pcTemp)) {
        *pcTemp = (CHAR)toupper(*pcTemp);                               /*  ת��Ϊ��д�ַ�              */
        ++pcTemp;
    }
    
    pcEoc = pcTemp;
    if (*pcTemp) {
        *pcTemp++ = PX_EOS;                                             /*  �������                    */
    }
    
    while (isspace(*pcTemp)) {                                          /*  ����ǰ�������ַ�            */
        ++pcTemp;
    }
    
    *ppcOpts = pcTemp;                                                  /*  ��¼ѡ��                    */
    __ftpdSkipOpt(&pcTemp);
  
    *ppcArgs = pcTemp;
    if (*ppcOpts == pcTemp) {
        *ppcOpts = pcEoc;
    }
    
    while (*pcTemp && *pcTemp != '\r' && *pcTemp != '\n') {             /*  ����ĩβ                    */
        ++pcTemp;
    }
    
    if (*pcTemp) {
        *pcTemp++ = PX_EOS;                                             /*  ȥ�����з�                  */
    }
}
/*********************************************************************************************************
** ��������: __inetFtpSession
** ��������: ftp �Ự�߳�
** �䡡��  : pftpds        �Ự���ƿ�
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static VOID  __inetFtpdSession (__PFTPD_SESSION  pftpds)
{
#if LW_CFG_NET_FTPD_LOG_EN > 0
    CHAR    cAddr[INET_ADDRSTRLEN];
#endif                                                                  /*  LW_CFG_NET_FTPD_LOG_EN > 0  */
    CHAR    cBuffer[__LWIP_FTPD_BUFFER_SIZE];                           /*  ��Ҫռ�ýϴ��ջ�ռ�        */

#if LW_CFG_NET_LOGINBL_EN > 0
    struct sockaddr_in  addr;
#endif                                                                  /*  LW_CFG_NET_LOGINBL_EN > 0   */

    pftpds->FTPDS_ulThreadId = API_ThreadIdSelf();
    pftpds->FTPDS_pfileCtrl  = fdopen(pftpds->FTPDS_iSockCtrl, "r+");   /*  �������� IO                 */
    if (pftpds->FTPDS_pfileCtrl == LW_NULL) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "fdopen() on socket failed.\r\n");
        API_AtomicDec(&_G_atomicFtpdLinks);
        close(pftpds->FTPDS_iSockCtrl);                                 /*  �رտ��ƶ� socket           */
        __SHEAP_FREE(pftpds);                                           /*  �ͷſ��ƿ��ڴ�              */
        return;
    }
    
    __FTPD_SESSION_LOCK();
    _List_Line_Add_Ahead(&pftpds->FTPDS_lineManage, 
                         &_G_plineFtpdSessionHeader);                   /*  ����Ự��                  */
    __FTPD_SESSION_UNLOCK();
    
    if (ioPrivateEnv() < 0) {                                           /*  ʹ���߳�˽�� io ����        */
        goto    __error_handle;
    }
    chdir(pftpds->FTPDS_cPathBuf);                                      /*  ת����������Ŀ¼            */
    
    __LWIP_FTPD_LOG(("ftpd: session create, remote: %s\n", 
                    inet_ntoa_r(pftpds->FTPDS_inaddrRemote, cAddr, INET_ADDRSTRLEN)));
                    
    __ftpdSendReply(pftpds, __FTPD_RETCODE_SERVER_READY, __FTPD_SERVER_MESSAGE);
                                                                        /*  ���ͷ�����������Ϣ          */

    for (;;) {                                                          /*  ftp ������ѭ��              */
        PCHAR   pcCmd;
        PCHAR   pcOpts;
        PCHAR   pcArgs;
        
        if (fgets(cBuffer, __LWIP_FTPD_BUFFER_SIZE, 
                  pftpds->FTPDS_pfileCtrl) == LW_NULL) {                /*  ���տ����ַ���              */
            __LWIP_FTPD_LOG(("ftpd: session connection aborted\n"));

#if LW_CFG_NET_LOGINBL_EN > 0                                           /*  û�е�¼, ��ʼ���������    */
            if (!(pftpds->FTPDS_iStatus & __FTPD_SESSION_STATUS_LOGIN)) {
                addr.sin_len    = sizeof(struct sockaddr_in);
                addr.sin_family = AF_INET;
                addr.sin_port   = _G_uiLoginFailPort;
                addr.sin_addr   = pftpds->FTPDS_inaddrRemote;
                API_LoginBlAdd((struct sockaddr *)&addr, _G_uiLoginFailBlRep, _G_uiLoginFailBlSec);
            }
#endif                                                                  /*  LW_CFG_NET_LOGINBL_EN > 0   */
            break;
        }
        
        __ftpdCommandAnalyse(cBuffer, &pcCmd, &pcOpts, &pcArgs);        /*  ��������                    */
        
        if (!lib_strcmp("QUIT", pcCmd)) {                               /*  ���� ftp �Ự               */
            __ftpdSendReply(pftpds, __FTPD_RETCODE_SERVER_BYEBYE, 
                            "Thank you for using the SylixOS buildin FTP service. Goodbye.");
            break;
        } else {
            if (__ftpdCommandExec(pftpds, pcCmd, pcArgs) < 0) {         /*  ִ������                    */
                break;                                                  /*  �������ֱ�ӽ����Ự        */
            }
        }
    }
                                                                        
__error_handle:
    __ftpdCloseSessionData(pftpds);                                     /*  �ر���������                */
    __ftpdCloseSessionCtrl(pftpds);                                     /*  �رտ�������                */
    
    __FTPD_SESSION_LOCK();
    _List_Line_Del(&pftpds->FTPDS_lineManage, 
                   &_G_plineFtpdSessionHeader);                         /*  �ӻỰ���н���              */
    __FTPD_SESSION_UNLOCK();
    
    API_AtomicDec(&_G_atomicFtpdLinks);                                 /*  ��������--                  */
    __SHEAP_FREE(pftpds);                                               /*  �ͷſ��ƿ�                  */
}
/*********************************************************************************************************
** ��������: __inetFtpServerListen
** ��������: ftp ���������������߳�
** �䡡��  : NONE
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static VOID  __inetFtpServerListen (VOID)
{
    INT                 iOne = 1;
    INT                 iSock;
    INT                 iSockNew;
    __PFTPD_SESSION     pftpdsNew;
    
    struct sockaddr_in  inaddrLcl;
    struct sockaddr_in  inaddrRmt;
    
    socklen_t           uiLen;
    
    struct servent     *pservent;
    
    inaddrLcl.sin_len         = sizeof(struct sockaddr_in);
    inaddrLcl.sin_family      = AF_INET;
    inaddrLcl.sin_addr.s_addr = INADDR_ANY;
    
    pservent = getservbyname("ftp", "tcp");
    if (pservent) {
        inaddrLcl.sin_port = (u16_t)pservent->s_port;
    } else {
        inaddrLcl.sin_port = htons(21);                                 /*  ftp default port            */
    }
    
#if LW_CFG_NET_LOGINBL_EN > 0
    _G_uiLoginFailPort = inaddrLcl.sin_port;
#endif                                                                  /*  LW_CFG_NET_LOGINBL_EN > 0   */
    
    iSock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (iSock < 0) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "can not create socket.\r\n");
        return;
    }
    
    if (bind(iSock, (struct sockaddr *)&inaddrLcl, sizeof(inaddrLcl))) {
        _DebugFormat(__ERRORMESSAGE_LEVEL, "can not bind socket %s.\r\n", lib_strerror(errno));
        close(iSock);
        return;
    }
    
    if (listen(iSock, __LW_FTPD_TCP_BACKLOG)) {
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
        if (API_AtomicInc(&_G_atomicFtpdLinks) > LW_CFG_NET_FTPD_MAX_LINKS) {
            API_AtomicDec(&_G_atomicFtpdLinks);
            /*
             *  ��������������, �����ٴ����µ�����.
             */
            close(iSockNew);
            sleep(1);                                                   /*  �ӳ� 1 S (��ֹ����)         */
            continue;
        }
        
        /*
         *  FTPD �ȴ�����ʱ, �������ӳ�ʱ��û���յ�����, �����Զ��ر��Խ�ʡ��Դ.
         */
        setsockopt(iSockNew, SOL_SOCKET, SO_RCVTIMEO, (const void *)&_G_iFtpdNoLoginTimeout, sizeof(INT));
        
        /*
         *  ��������ʼ���Ự�ṹ
         */
        pftpdsNew = (__PFTPD_SESSION)__SHEAP_ALLOC(sizeof(__FTPD_SESSION));
        if (pftpdsNew == LW_NULL) {
            _DebugHandle(__ERRORMESSAGE_LEVEL, "system low memory.\r\n");
            goto    __error_handle;
        }
        lib_bzero(pftpdsNew, sizeof(__FTPD_SESSION));
        
        pftpdsNew->FTPDS_iSockCtrl         = iSockNew;                  /*  ����ӿ� socket             */
        pftpdsNew->FTPDS_sockaddrinDefault = inaddrRmt;                 /*  Զ�̵�ַ                    */
        pftpdsNew->FTPDS_inaddrRemote      = inaddrRmt.sin_addr;        /*  Զ�� IP                     */
        
        if (getsockname(iSockNew, (struct sockaddr *)&inaddrLcl, &uiLen) < 0) {
            _DebugHandle(__ERRORMESSAGE_LEVEL, "getsockname() failed.\r\n");
            goto    __error_handle;
        } else {
            pftpdsNew->FTPDS_bUseDefault    = LW_TRUE;
            pftpdsNew->FTPDS_sockaddrinCtrl = inaddrLcl;                /*  ���ƶ˿ڱ��ص�ַ            */
            pftpdsNew->FTPDS_iSockPASV      = -1;
            pftpdsNew->FTPDS_iSockData      = -1;
            pftpdsNew->FTPDS_iTransMode     = __FTP_TRANSMODE_BIN;      /*  Ĭ��ʹ�ö�����ģʽ����      */
            pftpdsNew->FTPDS_sockaddrinData.sin_port = 
                htons((UINT16)(ntohs(pftpdsNew->FTPDS_sockaddrinCtrl.sin_port) - 1));
            pftpdsNew->FTPDS_timeStart      = lib_time(LW_NULL);        /*  ��¼����ʱ��                */
        }
        lib_strcpy(pftpdsNew->FTPDS_cPathBuf, _G_pcFtpdRootPath);       /*  ��ʼ��Ϊ�趨�� ftp ��Ŀ¼   */
        
        /*
         *  ���ñ��ʲ���.
         */
        {
            INT     iKeepIdle     = __LW_FTPD_TCP_KEEPIDLE;             /*  ����ʱ��                    */
            INT     iKeepInterval = __LW_FTPD_TCP_KEEPINTVL;            /*  ����̽����ʱ����        */
            INT     iKeepCount    = __LW_FTPD_TCP_KEEPCNT;              /*  ̽�� N ��ʧ����Ϊ�ǵ���     */
            
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
                                LW_CFG_NET_FTPD_STK_SIZE,
                                LW_PRIO_T_SERVICE,
                                LW_OPTION_THREAD_STK_CHK | LW_OPTION_OBJECT_GLOBAL,
                                (PVOID)pftpdsNew);
            if (API_ThreadCreate("t_ftpdsession", 
                                 (PTHREAD_START_ROUTINE)__inetFtpdSession, 
                                 &threadattr, 
                                 LW_NULL) == LW_OBJECT_HANDLE_INVALID) {
                goto    __error_handle;
            }
        }
        continue;                                                       /*  �ȴ��µĿͻ���              */
        
__error_handle:
        API_AtomicDec(&_G_atomicFtpdLinks);
        close(iSockNew);                                                /*  �ر���ʱ����                */
        if (pftpdsNew) {
            __SHEAP_FREE(pftpdsNew);                                    /*  �ͷŻỰ���ƿ�              */
        }
    }
}
/*********************************************************************************************************
** ��������: API_INetFtpServerInit
** ��������: ��ʼ�� ftp ��������Ŀ¼
** �䡡��  : pcPath        ����Ŀ¼
** �䡡��  : ERROR
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
VOID  API_INetFtpServerInit (CPCHAR  pcPath)
{
    static BOOL             bIsInit = LW_FALSE;
    LW_CLASS_THREADATTR     threadattr;
           PCHAR            pcNewPath = "\0";
    
#if LW_CFG_NET_LOGINBL_EN > 0
           CHAR             cEnvBuf[32];
#endif                                                                  /*  LW_CFG_NET_LOGINBL_EN > 0   */

    if (bIsInit) {                                                      /*  �Ѿ���ʼ����                */
        return;
    } else {
        bIsInit = LW_TRUE;
    }
    
#if LW_CFG_NET_LOGINBL_EN > 0
    if (API_TShellVarGetRt("LOGINBL_TO", cEnvBuf, (INT)sizeof(cEnvBuf)) > 0) {
        _G_uiLoginFailBlSec = lib_atoi(cEnvBuf);
    }
    if (API_TShellVarGetRt("LOGINBL_REP", cEnvBuf, (INT)sizeof(cEnvBuf)) > 0) {
        _G_uiLoginFailBlRep = lib_atoi(cEnvBuf);
    }
#endif                                                                  /*  LW_CFG_NET_LOGINBL_EN > 0   */

    if (pcPath) {                                                       /*  ��Ҫ���÷�����Ŀ¼          */
        pcNewPath = (PCHAR)pcPath;
    }
    if (_G_pcFtpdRootPath) {
        __SHEAP_FREE(_G_pcFtpdRootPath);
    }
    _G_pcFtpdRootPath = (PCHAR)__SHEAP_ALLOC(lib_strlen(pcNewPath) + 1);
    if (_G_pcFtpdRootPath == LW_NULL) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "system low memory.\r\n");
        _ErrorHandle(ERROR_SYSTEM_LOW_MEMORY);
        goto    __error_handle;
    }
    lib_strcpy(_G_pcFtpdRootPath, pcNewPath);                           /*  ���������·��              */
    
    API_AtomicSet(0, &_G_atomicFtpdLinks);                              /*  ������������                */
    
    _G_ulFtpdSessionLock = API_SemaphoreMCreate("ftpsession_lock", 
                                                LW_PRIO_T_NETPROTO, 
                                                LW_OPTION_WAIT_PRIORITY |
                                                LW_OPTION_INHERIT_PRIORITY |
                                                LW_OPTION_DELETE_SAFE |
                                                LW_OPTION_OBJECT_GLOBAL,
                                                LW_NULL);               /*  �����Ự��������          */
    if (_G_ulFtpdSessionLock == LW_OBJECT_HANDLE_INVALID) {
        goto    __error_handle;
    }
    
    API_ThreadAttrBuild(&threadattr,
                        LW_CFG_NET_FTPD_STK_SIZE,
                        LW_PRIO_T_SERVICE,
                        LW_OPTION_THREAD_STK_CHK | LW_OPTION_OBJECT_GLOBAL,
                        LW_NULL);
    if (API_ThreadCreate("t_ftpd", (PTHREAD_START_ROUTINE)__inetFtpServerListen, 
                         &threadattr, LW_NULL) == LW_OBJECT_HANDLE_INVALID) {
        goto    __error_handle;
    }
    
#if LW_CFG_SHELL_EN > 0
    /*
     *  ���� SHELL ����.
     */
    API_TShellKeywordAdd("ftpds", __tshellNetFtpdShow);
    API_TShellHelpAdd("ftpds",   "show ftp server session.\n");
    
    API_TShellKeywordAdd("ftpdpath", __tshellNetFtpdPath);
    API_TShellFormatAdd("ftpdpath", " [new path]");
    API_TShellHelpAdd("ftpdpath",   "set default ftp server path.\n");
#endif                                                                  /*  LW_CFG_SHELL_EN > 0         */
    
    return;
    
__error_handle:
    bIsInit = LW_FALSE;                                                 /*  ��ʼ��ʧ��                  */
}
/*********************************************************************************************************
** ��������: API_INetFtpServerShow
** ��������: ��ʾ ftp ��������Ŀ¼
** �䡡��  : pcPath        ����Ŀ¼
** �䡡��  : ERROR
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
VOID  API_INetFtpServerShow (VOID)
{
    static const CHAR   _G_cFtpdInfoHdr[] = "\n"
    "    REMOTE               TIME               ALIVE(s)\n"
    "--------------- ------------------------ ------------\n";

    REGISTER PLW_LIST_LINE      plineTemp;
    REGISTER __PFTPD_SESSION    pftpds;
    
             CHAR               cAddr[INET_ADDRSTRLEN];
             time_t             timeNow = lib_time(LW_NULL);
             INT                iAlive;
             struct tm          tmTime;
             INT                iSessionCounter = 0;
             
             CHAR               cTimeBuffer[32];
             PCHAR              pcN;
    
    printf("ftpd show >>\n");
    printf("ftpd path: %s\n", (_G_pcFtpdRootPath != LW_NULL) ? (_G_pcFtpdRootPath) : "null");
    printf(_G_cFtpdInfoHdr);
    
    __FTPD_SESSION_LOCK();                                              /*  �����Ự����                */
    for (plineTemp  = _G_plineFtpdSessionHeader;
         plineTemp != LW_NULL;
         plineTemp  = _list_line_get_next(plineTemp)) {
         
        pftpds = _LIST_ENTRY(plineTemp, __FTPD_SESSION, FTPDS_lineManage);
        
        inet_ntoa_r(pftpds->FTPDS_inaddrRemote, cAddr, INET_ADDRSTRLEN);/*  ��ʽ����ַ�ִ�              */
        lib_localtime_r(&pftpds->FTPDS_timeStart, &tmTime);             /*  ��ʽ������ʱ��              */
        iAlive = (INT)(timeNow - pftpds->FTPDS_timeStart);              /*  ������ʱ��                */
        
        lib_asctime_r(&tmTime, cTimeBuffer);
        pcN = lib_index(cTimeBuffer, '\n');
        if (pcN) {
            *pcN = PX_EOS;
        }
        
        printf("%-15s %-24s %12d\n", cAddr, cTimeBuffer, iAlive);
                                                                        /*  ��ӡ�Ự��Ϣ                */
        iSessionCounter++;
    }
    __FTPD_SESSION_UNLOCK();                                            /*  �����Ự����                */

    printf("\ntotal ftp session: %d\n", iSessionCounter);               /*  ��ʾ�Ự����                */
}
/*********************************************************************************************************
** ��������: API_INetFtpServerPath
** ��������: ���� ftp ��������Ŀ¼
** �䡡��  : pcPath        ����Ŀ¼
** �䡡��  : ERROR
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
INT  API_INetFtpServerPath (CPCHAR  pcPath)
{
    REGISTER PCHAR    pcNewPath;
    REGISTER PCHAR    pcTemp = _G_pcFtpdRootPath;
    
    if (pcPath == LW_NULL) {                                            /*  Ŀ¼Ϊ��                    */
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    
    pcNewPath = (PCHAR)__SHEAP_ALLOC(lib_strlen(pcPath) + 1);
    if (pcNewPath == LW_NULL) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "system low memory.\r\n");
        _ErrorHandle(ERROR_SYSTEM_LOW_MEMORY);
        return  (PX_ERROR);
    }
    lib_strcpy(pcNewPath, pcPath);                                      /*  �����µ�·��                */
    
    __KERNEL_MODE_PROC(
        _G_pcFtpdRootPath = pcNewPath;                                  /*  �����µķ�����·��          */
    );
    
    if (pcTemp) {
        __SHEAP_FREE(pcTemp);
    }
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: API_INetFtpServerBindDev
** ��������: ���� ftp ���������豸
** �䡡��  : uiIndex        �����豸����
** �䡡��  : ERROR
** ȫ�ֱ���:
** ����ģ��:
                                           API ����
*********************************************************************************************************/
LW_API
INT  API_INetFtpServerBindDev (UINT  uiIndex)
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
/*********************************************************************************************************
** ��������: __tshellNetFtpdShow
** ��������: ϵͳ���� "ftpds"
** �䡡��  : iArgC         ��������
**           ppcArgV       ������
** �䡡��  : 0
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
#if LW_CFG_SHELL_EN > 0

static INT  __tshellNetFtpdShow (INT  iArgC, PCHAR  *ppcArgV)
{
    API_INetFtpServerShow();
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: __tshellNetFtpdPath
** ��������: ϵͳ���� "ftpdpath"
** �䡡��  : iArgC         ��������
**           ppcArgV       ������
** �䡡��  : 0
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static INT  __tshellNetFtpdPath (INT  iArgC, PCHAR  *ppcArgV)
{
    if (iArgC < 2) {
        printf("ftpd path: %s\n", _G_pcFtpdRootPath);                   /*  ��ӡ��ǰ������Ŀ¼          */
        return  (ERROR_NONE);
    }
    
    return  (API_INetFtpServerPath(ppcArgV[1]));
}

#endif                                                                  /*  LW_CFG_SHELL_EN > 0         */
#endif                                                                  /*  LW_CFG_NET_EN > 0           */
                                                                        /*  LW_CFG_NET_FTPD_EN > 0      */
/*********************************************************************************************************
  END
*********************************************************************************************************/
