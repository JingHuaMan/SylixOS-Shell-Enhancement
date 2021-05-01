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
** ��   ��   ��: gdbserver.c
**
** ��   ��   ��: Jiang.Taijin (��̫��)
**
** �ļ���������: 2014 �� 05 �� 06 ��
**
** ��        ��: GDBServer ���س���
**
** BUG:
2014.05.31  ʹ�� LW_VPROC_EXIT_FORCE ɾ������.
2014.06.03  ʹ�� posix_spawnp ��������.
2014.09.02  ��������ͬʱ�������߳�����.
2015.11.18  ���� SMP ���� CPU ���Թ���.
2016.06.14  ֧���ڵ�ǰ�ն��µ���.
2016.08.16  ֧��Ӳ����������.
2016.12.07  GDB �˳�ʱ, ���� kill �������Խ���, Ȼ���ٷ��н���, ȷ�������ִ��һ�����.
2017.02.21  �޸ĵ���������ʱkill������ʧ���Լ��˿�δ�ͷŵ�����.
*********************************************************************************************************/
#define  __SYLIXOS_GDB
#define  __SYLIXOS_STDIO
#define  __SYLIXOS_KERNEL
#include "SylixOS.h"
#include "spawn.h"
#include "dtrace.h"
#include "socket.h"
#include "sys/signalfd.h"
#if LW_CFG_SMP_EN > 0
#include "sched.h"
#endif                                                                  /*  LW_CFG_SMP_EN > 0           */
/*********************************************************************************************************
  �ü�֧��
*********************************************************************************************************/
#if LW_CFG_GDB_EN > 0
#include "arch/arch_gdb.h"
#include "../SylixOS/loader/elf/elf_type.h"
#include "../SylixOS/loader/include/loader.h"
#include "../SylixOS/loader/include/loader_vppatch.h"
/*********************************************************************************************************
  Non-STOP ģʽ֧���ж�
*********************************************************************************************************/
#if defined(LW_CFG_CPU_ARCH_PPC) || (defined(LW_CFG_CPU_ARCH_X86) && (LW_CFG_CPU_WORD_LENGHT == 64))
#define LW_CFG_GDB_NON_STOP_EN      0
#else
#define LW_CFG_GDB_NON_STOP_EN      1
#endif
/*********************************************************************************************************
  �����붨��
*********************************************************************************************************/
#define ERROR_GDB_INIT_SOCK         200000
#define ERROR_GDB_PARAM             200001
#define ERROR_GDB_ATTACH_PROG       200002
/*********************************************************************************************************
  ��������
*********************************************************************************************************/
#define GDB_RSP_MAX_LEN             0x2000                              /*  rsp ��������С              */
#define GDB_MAX_THREAD_NUM          LW_CFG_MAX_THREADS                  /*  ����߳���                  */
/*********************************************************************************************************
  ���� keepalive ��������
*********************************************************************************************************/
#define GDB_TCP_KEEPIDLE            60                                  /*  ����ʱ��, ��λ��            */
#define GDB_TCP_KEEPINTVL           60                                  /*  ����̽����ʱ���, ��λ��  */
#define GDB_TCP_KEEPCNT             3                                   /*  ̽�� N ��ʧ����Ϊ�ǵ���     */
/*********************************************************************************************************
  �ڴ����
*********************************************************************************************************/
#define LW_GDB_SAFEMALLOC(size)     __SHEAP_ALLOC((size_t)size)
#define LW_GDB_SAFEFREE(a)          { if (a) { __SHEAP_FREE((PVOID)a); a = 0; } }
/*********************************************************************************************************
  ��ӡ����
*********************************************************************************************************/
#define LW_GDB_MSG                  printk
/*********************************************************************************************************
  GDB Non-STOP ģʽ BUG
*********************************************************************************************************/
#define GDB_NON_STOP_BUG
/*********************************************************************************************************
  ͨ������
*********************************************************************************************************/
typedef enum {
    COMM_TYPE_TCP = 1,                                                  /* TCP                          */
    COMM_TYPE_TTY                                                       /* ����                         */
}LW_GDB_COMM_TYPE;
/*********************************************************************************************************
  �ϵ����ݽṹ
*********************************************************************************************************/
typedef struct {
    LW_LIST_LINE            BP_plistBpLine;                             /* �ϵ�����                     */
    addr_t                  BP_addr;                                    /* ��������                     */
    size_t                  BP_size;
    ULONG                   BP_ulInstOrg;                               /* �ϵ�����ԭָ��               */
} LW_GDB_BP;
/*********************************************************************************************************
  �߳̽ṹ
*********************************************************************************************************/
typedef struct {
    LW_LIST_LINE            TH_plistThLine;                             /* �߳�����ͷ                   */
    ULONG                   TH_ulId;                                    /* �߳�ID                       */
    CHAR                    TH_cStates;                                 /* �߳�״̬                     */
    addr_t                  TH_addrLow;                                 /* ���򵥲���ʼ��ַ             */
    addr_t                  TH_addrHigh;                                /* ���򵥲�������ַ             */
    BOOL                    TH_bChanged;                                /* ״̬�Ƿ��޸�               */
} LW_GDB_THREAD;
/*********************************************************************************************************
  ���ڵ���ԭʼ����
*********************************************************************************************************/
typedef struct {
    LONG                    GDBSP_lBaud;                                /* ���ڲ�����                   */
    INT                     GDBSP_iHwOpt;                               /* Ӳ������                     */
    INT                     GDBSP_iSioOpt;                              /* TTY ����                     */
} LW_GDB_SERIAL_PARAM;
/*********************************************************************************************************
  ȫ�ֲ����ṹ
*********************************************************************************************************/
typedef struct {
    BYTE                    GDB_byCommType;                             /* ��������                     */
    LW_GDB_SERIAL_PARAM     GDB_spSerial;                               /* ��Ҫ�ظ��Ĵ��ڲ���           */
    BOOL                    GDB_bTerminal;                              /* �Ƿ�Ϊ�ն˵���               */
    INT                     GDB_iCommFd;                                /* ���ڴ���Э��֡���ļ�������   */
    INT                     GDB_iCommListenFd;                          /* ���ڶ˿��������ļ�������     */
    INT                     GDB_iSigFd;                                 /* �ж�signalfd�ļ����         */
    CHAR                    GDB_cProgPath[MAX_FILENAME_LENGTH];         /* ��ִ���ļ�·��               */
    pid_t                   GDB_iPid;                                   /* ���ٵĽ��̺�                 */
    PVOID                   GDB_pvDtrace;                               /* dtrace������               */
    LW_LIST_LINE_HEADER     GDB_plistBpHead;                            /* �ϵ�����                     */
    LONG                    GDB_lOpCThreadId;                           /* c,s�����߳�                  */
    LONG                    GDB_lOpGThreadId;                           /* ���������߳�                 */
    ULONG                   GDB_lOptThreadId;                           /* t����ֹͣ���߳�              */
    BOOL                    GDB_bNonStop;                               /* Non-stopģʽ                 */
    UINT                    GDB_uiThdNum;                               /* �߳���                       */
    ULONG                   GDB_ulThreads[GDB_MAX_THREAD_NUM + 1];      /* �߳�����                     */
    LW_LIST_LINE_HEADER     GDB_plistThd;                               /* �߳�����                     */
    BOOL                    GDB_beNotifing;                             /* �Ƿ����ڴ���notify           */
    BOOL                    GDB_bExited;                                /* �����Ƿ��˳�                 */
    BOOL                    GDB_bStarted;                               /* �������Ѿ�����               */
    BOOL                    GDB_bAttached;                              /* �Ƿ�Ϊattach����             */
    BOOL                    GDB_bLockCpu;                               /* �Ƿ����������Խ���cpu        */
} LW_GDB_PARAM;
/*********************************************************************************************************
  ȫ�ֱ�������
*********************************************************************************************************/
#ifndef LW_DTRACE_HW_ISTEP
static atomic_t     GHookRefCnt = {0};                                  /* gdb hook ���ü���            */
#endif                                                                  /* !LW_DTRACE_HW_ISTEP          */
/*********************************************************************************************************
** ��������: gdbSetStepMode
** ��������: ��������ģʽ
** �䡡��  : pparam       GDB ����
**           ulThread     �߳̾��
** �䡡��  :
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static VOID gdbSetStepMode (LW_GDB_PARAM *pparam, LW_OBJECT_HANDLE ulThread)
{
#ifdef LW_DTRACE_HW_ISTEP
    API_DtraceThreadStepSet(pparam->GDB_pvDtrace, ulThread, LW_TRUE);
    
#else
    GDB_REG_SET regset;
    addr_t      addrNP;

    archGdbRegsGet(pparam->GDB_pvDtrace, ulThread, &regset);
    addrNP = archGdbGetNextPc(pparam->GDB_pvDtrace, ulThread, &regset);
    API_DtraceThreadStepSet(pparam->GDB_pvDtrace, ulThread, addrNP);
#endif                                                                  /*  LW_DTRACE_HW_ISTEP          */
}
/*********************************************************************************************************
** ��������: gdbClearStepMode
** ��������: �������ģʽ
** �䡡��  : pparam       GDB ����
**           ulThread     �߳̾��
** �䡡��  :
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static VOID gdbClearStepMode (LW_GDB_PARAM *pparam, LW_OBJECT_HANDLE ulThread)
{
#ifdef LW_DTRACE_HW_ISTEP
    API_DtraceThreadStepSet(pparam->GDB_pvDtrace, ulThread, LW_FALSE);
    
#else
    API_DtraceThreadStepSet(pparam->GDB_pvDtrace, ulThread, LW_GDB_ADDR_INVAL);
#endif                                                                  /*  LW_DTRACE_HW_ISTEP          */
}
/*********************************************************************************************************
** ��������: gdbIsStepBp
** ��������: �ж��Ƿ�Ϊ�����ϵ�
** �䡡��  : pparam       GDB ����
**           pdmsg        �ϵ���Ϣָ��
** �䡡��  : LW_TRUE--�����ϵ㣬LW_FALSE--�ǵ����ϵ�
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static BOOL gdbIsStepBp (LW_GDB_PARAM *pparam, PLW_DTRACE_MSG pdmsg)
{
#ifdef LW_DTRACE_HW_ISTEP
    if (pdmsg->DTM_uiType == LW_TRAP_ISTEP) {
        return  (LW_TRUE);
    
    } else {
        return  (LW_FALSE);
    }

#else
    addr_t      addrNP;

    API_DtraceThreadStepGet(pparam->GDB_pvDtrace,
                            pdmsg->DTM_ulThread, &addrNP);
    if ((addrNP != PX_ERROR) && (pdmsg->DTM_ulAddr == addrNP)) {
        return  (LW_TRUE);
    
    } else {
        return  (LW_FALSE);
    }
#endif                                                                  /*  LW_DTRACE_HW_ISTEP          */
}
/*********************************************************************************************************
** ��������: gdbRemoveStepBp
** ��������: ɾ���߳����Ѿ������ĵ����ϵ�
** �䡡��  : pparam       GDB ����
**           ulThread     �߳̾��
** �䡡��  :
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static VOID gdbRemoveStepBp (LW_GDB_PARAM *pparam, LW_OBJECT_HANDLE ulThread)
{
#ifdef LW_DTRACE_HW_ISTEP
    API_DtraceDelBreakInfo(pparam->GDB_pvDtrace, ulThread, LW_GDB_ADDR_INVAL, LW_TRUE);
    
#else
    addr_t      addrNP;

    API_DtraceThreadStepGet(pparam->GDB_pvDtrace, ulThread, &addrNP);
    if (addrNP != PX_ERROR) {
        API_DtraceDelBreakInfo(pparam->GDB_pvDtrace, ulThread, addrNP, LW_TRUE);
    }
#endif                                                                  /*  LW_DTRACE_HW_ISTEP          */
}
/*********************************************************************************************************
** ��������: gdbTcpSockInit
** ��������: ��ʼ��socket
** �䡡��  : pparam       GDB ����
**           ui32Ip       ����ip
**           usPort       �����˿�
** �䡡��  : �ɹ�-- fd��������ʧ��-- PX_ERROR.
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT gdbTcpSockInit (LW_GDB_PARAM *pparam, UINT32 ui32Ip, UINT16 usPort)
{
    struct sockaddr_in  addrServer;
    struct sockaddr_in  addrClient;
    INT                 iSockListen;
    INT                 iSockNew;
    INT                 iKeepIdle     = GDB_TCP_KEEPIDLE;               /*  ����ʱ��                    */
    INT                 iKeepInterval = GDB_TCP_KEEPINTVL;              /*  ����̽����ʱ����        */
    INT                 iKeepCount    = GDB_TCP_KEEPCNT;                /*  ̽�� N ��ʧ����Ϊ�ǵ���     */
    INT                 iOpt          = 1;
    socklen_t           iAddrLen      = sizeof(addrClient);
    CHAR                cIpBuff[32]   = {0};

    bzero(&addrServer, sizeof(addrServer));
    if (0 == ui32Ip) {
        ui32Ip = INADDR_ANY;
    }
    
    addrServer.sin_family       = AF_INET;
    addrServer.sin_addr.s_addr  = htonl(ui32Ip);
    addrServer.sin_port         = htons(usPort);
    
    iSockListen = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (iSockListen < 0) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "Create listen sock failed.\r\n");
        return  (PX_ERROR);
    }

    setsockopt(iSockListen, SOL_SOCKET, SO_REUSEADDR, &iOpt, sizeof(iOpt));
    
    if (bind(iSockListen, (struct sockaddr*)&addrServer, sizeof(addrServer))) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "Bind sock failed.\r\n");
        close(iSockListen);
        return  (PX_ERROR);
    }

    if (listen(iSockListen, 1)) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "Bind sock failed.\r\n");
        close(iSockListen);
        return  (PX_ERROR);
    }
    
    pparam->GDB_iCommListenFd = iSockListen;

    /*
     *  �˴�ʹ��printf�����telnet�նˣ�IDE��Ȿ�����ȷ����������gdb����
     */
    printf("[GDB]Waiting for connect...\n");
    
    iSockNew = accept(iSockListen, (struct sockaddr *)&addrClient, &iAddrLen);
    if (iSockNew < 0) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "Accept failed.\r\n");
        return  (PX_ERROR);
    }

    setsockopt(iSockNew, IPPROTO_TCP, TCP_NODELAY,   (const void *)&iOpt,          sizeof(INT));
    setsockopt(iSockNew, SOL_SOCKET,  SO_KEEPALIVE,  (const void *)&iOpt,          sizeof(INT));
    setsockopt(iSockNew, IPPROTO_TCP, TCP_KEEPIDLE,  (const void *)&iKeepIdle,     sizeof(INT));
    setsockopt(iSockNew, IPPROTO_TCP, TCP_KEEPINTVL, (const void *)&iKeepInterval, sizeof(INT));
    setsockopt(iSockNew, IPPROTO_TCP, TCP_KEEPCNT,   (const void *)&iKeepCount,    sizeof(INT));

    printf("[GDB]Connected. host: %s\n",
           inet_ntoa_r(addrClient.sin_addr, cIpBuff, sizeof(cIpBuff)));

    return  (iSockNew);
}
/*********************************************************************************************************
** ��������: gdbSerialInit
** ��������: ��ʼ�� tty
** �䡡��  : pparam       GDB ����
**           pcSerial     tty �豸��
** �䡡��  : �ɹ�-- fd��������ʧ��-- PX_ERROR.
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT gdbSerialInit (LW_GDB_PARAM *pparam, CPCHAR pcSerial)
{
#define SERIAL_PARAM    "115200,n,8,1"
#define SERIAL_BAUD     SIO_BAUD_115200
#define SERIAL_HWOPT    (CLOCAL | CREAD | CS8 | HUPCL)
#define SERIAL_BSIZE    LW_OSIOD_LARG(1024)

    INT  iFd;
    
    if (lib_strcmp(pcSerial, "terminal") == 0) {                        /*  ʹ�õ�ǰ�ն˴��ڵ���        */
        INT  iFdIn, iFdOut;
        LW_OBJECT_HANDLE  ulSelf = API_ThreadIdSelf();
        
        iFdIn  = API_IoTaskStdGet(ulSelf, STD_IN);
        iFdOut = API_IoTaskStdGet(ulSelf, STD_OUT);
        if (iFdIn != iFdOut) {
            _DebugHandle(__ERRORMESSAGE_LEVEL, "Standard 'IN' 'OUT' device not same.\r\n");
            return  (PX_ERROR);
        }
        
        iFd                   = iFdOut;
        pparam->GDB_bTerminal = LW_TRUE;
    
    } else {                                                            /*  �½�����                    */
        iFd = open(pcSerial, O_RDWR);
        if (iFd < 0) {
            _DebugHandle(__ERRORMESSAGE_LEVEL, "Open serial failed.\r\n");
            return  (PX_ERROR);
        }
    }
    
    if (!isatty(iFd)) {
        if (pparam->GDB_bTerminal == LW_FALSE) {
            close(iFd);
        }
        _DebugHandle(__ERRORMESSAGE_LEVEL, "Serial is not a tty device.\r\n");
        return  (PX_ERROR);
    }
    
    if (pparam->GDB_bTerminal) {                                        /*  ���� terminal ����          */
        ioctl(iFd, FIOGETOPTIONS,   &pparam->GDB_spSerial.GDBSP_iSioOpt);
        ioctl(iFd, SIO_BAUD_GET,    &pparam->GDB_spSerial.GDBSP_lBaud);
        ioctl(iFd, SIO_HW_OPTS_GET, &pparam->GDB_spSerial.GDBSP_iHwOpt);
    }
    
    ioctl(iFd, FIOSETOPTIONS,   OPT_RAW);
    ioctl(iFd, SIO_BAUD_SET,    SERIAL_BAUD);
    ioctl(iFd, SIO_HW_OPTS_SET, SERIAL_HWOPT);
    ioctl(iFd, FIORBUFSET,      SERIAL_BSIZE);
    ioctl(iFd, FIOWBUFSET,      SERIAL_BSIZE);
    
    LW_GDB_MSG("[GDB]Serial device: %s %s\n", pcSerial, SERIAL_PARAM);
    
    return  (iFd);
}
/*********************************************************************************************************
** ��������: gdbAsc2Hex
** ��������: ASCת����hexֵ
** �䡡��  : ch       ASC�ַ�
** �䡡��  : hexֵ
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT gdbAsc2Hex (CHAR ch)
{
    if ((ch >= 'a') && (ch <= 'f')) {
        return  (ch - 'a' + 10);
    }
    if ((ch >= '0') && (ch <= '9')) {
        return  (ch - '0');
    }
    if ((ch >= 'A') && (ch <= 'F')) {
        return  (ch - 'A' + 10);
    }

    return  (PX_ERROR);
}
/*********************************************************************************************************
** ��������: gdbByte2Asc
** ��������: byteת����ASC�ַ���
** �䡡��  : iByte       byteֵ
** �䡡��  : pcAsc       ת�����ASC�ַ���
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static VOID gdbByte2Asc (PCHAR pcAsc, BYTE iByte)
{
    BYTE  val = (iByte >> 4) & 0xf;

    pcAsc[0] = val > 9 ? (val - 0xA + 'a') : (val + '0');
    val = iByte & 0xf;
    pcAsc[1] = val > 9 ? (val - 0xA + 'a') : (val + '0');
}
/*********************************************************************************************************
** ��������: gdbAscToByte
** ��������: ASC�ַ���ת����byte�ֽ�
** �䡡��  : pcAsc       ASC�ַ���ֵ
** �䡡��  : ת�������ֵ
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT gdbAscToByte (PCHAR pcAsc)
{
    return  (gdbAsc2Hex(pcAsc[0]) << 4) + gdbAsc2Hex(pcAsc[1]);
}
/*********************************************************************************************************
** ��������: gdbReg2Asc
** ��������: ULONGת����ASC�ַ���
** �䡡��  : ulReg       ULONGֵ
** �䡡��  : pcAsc       ת�����ASC�ַ���
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static VOID gdbReg2Asc (PCHAR pcAsc, ULONG ulReg)
{
    INT i;

#if LW_CFG_CPU_ENDIAN == 0
    for (i = 0; i < sizeof(ulReg); i++, ulReg >>= 8) {
        gdbByte2Asc(pcAsc + (i * 2), ulReg & 0xff);
    }
#else
    for (i = 0; i < sizeof(ulReg); i++, ulReg >>= 8) {
        gdbByte2Asc(pcAsc + ((sizeof(ulReg) - i - 1) * 2), ulReg & 0xff);
    }
#endif
}
/*********************************************************************************************************
** ��������: gdbAscToReg
** ��������: ASC�ַ���ת����ULONG
** �䡡��  : pcAsc       ASC�ַ���ֵ
** �䡡��  : ת�������ֵ
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static ULONG gdbAscToReg (char *pcAsc)
{
    ULONG       ulReg = 0;
    INT         i;

#if LW_CFG_CPU_ENDIAN == 0
    for (i = 0; i < sizeof(ulReg); i++) {
        ulReg <<= 8;
        ulReg += gdbAscToByte(pcAsc + ((sizeof(ulReg) - i - 1) * 2));
    }
#else
    for (i = 0; i < sizeof(ulReg); i++) {
        ulReg <<= 8;
        ulReg += gdbAscToByte(pcAsc + (i * 2));
    }
#endif

    return  (ulReg);
}
/*********************************************************************************************************
** ��������: gdbReplyError
** ��������: ��װrsp�����
** �䡡��  : pcReply       ���������
**           iErrNum       ������
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static VOID gdbReplyError (PCHAR pcReply, INT iErrNum)
{
    pcReply[0] = 'E';
    gdbByte2Asc(pcReply + 1, iErrNum);
    pcReply[3] = 0;
}
/*********************************************************************************************************
** ��������: gdbReplyOk
** ��������: ��װrsp ok��
** �䡡��  : pcReply       ���������
** �䡡��  :
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static VOID gdbReplyOk (PCHAR pcReply)
{
    lib_strcpy(pcReply, "OK");
}
/*********************************************************************************************************
** ��������: gdbRspPkgPut
** ��������: ����rsp���ݰ�
** �䡡��  : pparam       ϵͳ����
**           pcOutBuff    ���ͻ�����
**           bNotify      �Ƿ���notify��
** �䡡��  : ERROR_NONE ��ʾû�д���, PX_ERROR ��ʾ����
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT gdbRspPkgPut (LW_GDB_PARAM *pparam, PCHAR pcOutBuff, BOOL bNotify)
{
    CHAR    cHeader    = '$';
    UCHAR   ucCheckSum = 0;
    INT     iPkgLen    = 0;
    INT     iSendLen   = 0;
    INT     iDataLen   = 0;
    PCHAR   pcWPos;
    CHAR    cAck;

    while (pcOutBuff[iPkgLen] != '\0') {
        ucCheckSum += pcOutBuff[iPkgLen];
        iPkgLen++;
    }

    if (bNotify) {                                                      /* �첽֪ͨ��                   */
        cHeader = '%';
    }

    if (iPkgLen > GDB_RSP_MAX_LEN - 10) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "Package Too long.\r\n");
        return  (PX_ERROR);
    }

    pcOutBuff[iPkgLen++] = '#';
    gdbByte2Asc(&pcOutBuff[iPkgLen], ucCheckSum);
    iPkgLen += 2;

    do {
        if (write(pparam->GDB_iCommFd, &cHeader, 1) < 0) {
            return  (PX_ERROR);
        }

        pcWPos   = pcOutBuff;
        iDataLen = iPkgLen;

        while (iDataLen > 0) {
            iSendLen = write(pparam->GDB_iCommFd, pcWPos, iDataLen);
            if (iSendLen <= 0) {
                return  (PX_ERROR);
            }
            iDataLen -= iSendLen;
            pcWPos  += iSendLen;
            if (iDataLen <= 0) {
                break;
            }
        }

        if (bNotify) {                                                  /* notify����gdb����Ӧ+/-       */
            cAck = '+';
        } else {
            if (read(pparam->GDB_iCommFd, &cAck, 1) <= 0) {
                return  (PX_ERROR);
            }
        }
    } while(cAck != '+');

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: gdbRspPkgGet
** ��������: ����rsp���ݰ�
** �䡡��  : pparam       ϵͳ����
**           pcInBuff     ���ܻ�����
** �䡡��  : pfdsi        ���Ϊ�ж��źţ�������ź���Ϣ
**           ����ֵ       ERROR_NONE ��ʾû�д���, PX_ERROR ��ʾ����
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT gdbRspPkgGet (LW_GDB_PARAM *pparam, PCHAR pcInBuff, struct signalfd_siginfo *pfdsi)
{
    CHAR    cHeader;
    CHAR    cSend;
    INT     iReadLen    = 0;
    INT     iReadChkSum = 0;
    INT     iDataLen    = 0;
    UCHAR   ucCheckSum  = 0;
    UCHAR   ucXmitcSum  = 0;

    fd_set  fdset;
    ssize_t szLen;
    INT     iMaxFd;

    iMaxFd = max(pparam->GDB_iCommFd, pparam->GDB_iSigFd);

    while (1) {
        FD_ZERO(&fdset);
        FD_SET(pparam->GDB_iCommFd, &fdset);
        if (pparam->GDB_bNonStop) {                                     /* nonstop�ж��������첽����    */
            FD_SET(pparam->GDB_iSigFd, &fdset);
        }

        while (select(iMaxFd + 1, &fdset, NULL, NULL, NULL) >= 0) {
            if (FD_ISSET(pparam->GDB_iCommFd, &fdset)) {                /* ��������                     */
                iReadLen = read(pparam->GDB_iCommFd, &cHeader, 1);
                if (iReadLen <= 0) {
                    return  (PX_ERROR);
                }
                if ((cHeader & 0x7F) == '$') {
                    break;
                }
            }

            if (FD_ISSET(pparam->GDB_iSigFd, &fdset)) {                 /* �жϵ���                     */
                szLen = read(pparam->GDB_iSigFd, pfdsi,
                             sizeof(struct signalfd_siginfo));
                if (szLen != sizeof(struct signalfd_siginfo)) {
                    FD_SET(pparam->GDB_iCommFd, &fdset);
                    continue;
                }
                return  (ERROR_NONE);
            }

            FD_SET(pparam->GDB_iCommFd, &fdset);
            if (pparam->GDB_bNonStop) {
                FD_SET(pparam->GDB_iSigFd, &fdset);
            }
        }

        cSend = '+';
        iReadLen = read(pparam->GDB_iCommFd,
                        pcInBuff + iDataLen,
                        GDB_RSP_MAX_LEN - iDataLen);

        while (iReadLen > 0) {
            while (iReadLen > 0) {
                if ((pcInBuff[iDataLen] & 0x7F) == '#') {
                    while (iReadLen < 3 &&
                           (GDB_RSP_MAX_LEN - iDataLen) > 3) {          /* ��ȡδ��ɵ��ֽ�             */
                        iReadChkSum = read(pparam->GDB_iCommFd,
                                           pcInBuff + iDataLen + iReadLen,
                                           3 - iReadLen);
                        if (iReadChkSum <= 0) {
                            return  (PX_ERROR);
                        }
                        iReadLen += iReadChkSum;
                    }
                    ucXmitcSum  = gdbAsc2Hex(pcInBuff[iDataLen + 1] & 0x7f) << 4;
                    ucXmitcSum += gdbAsc2Hex(pcInBuff[iDataLen + 2] & 0x7f);

                    if (ucXmitcSum == ucCheckSum) {                     /* ����ȷ�ϰ�                   */
                        cSend = '+';
                        write(pparam->GDB_iCommFd, &cSend, sizeof(cSend));
                        return  (iDataLen);
                    
                    } else {
                        cSend = '-';
                        write(pparam->GDB_iCommFd, &cSend, sizeof(cSend));
                        break;
                    }

                } else {
                    ucCheckSum += (pcInBuff[iDataLen] & 0x7F);
                }
                iDataLen++;
                iReadLen--;
            }

            if (cSend == '-') {
                break;
            }

            iReadLen = read(pparam->GDB_iCommFd,
                            pcInBuff + iDataLen,
                            GDB_RSP_MAX_LEN - iDataLen);
        }

        if (iReadLen <= 0) {
            return  (PX_ERROR);
        }
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: gdbReportStopReason
** ��������: �ϱ�ֹͣԭ��
** �䡡��  : pdmsg         �ж���Ϣ
**           pcOutBuff     ���������
** �䡡��  : ERROR_NONE ��ʾû�д���, PX_ERROR ��ʾ����
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT gdbReportStopReason (PLW_DTRACE_MSG pdmsg, PCHAR pcOutBuff)
{
    UINT uiSig = SIGSEGV;

    if (pdmsg->DTM_uiType == LW_TRAP_QUIT) {                             /* �����˳�                     */
        sprintf(pcOutBuff, "W00");
        return  (ERROR_NONE);
    }

    uiSig = ((pdmsg->DTM_uiType == LW_TRAP_ABORT) ? SIGSEGV : SIGTRAP);
    sprintf(pcOutBuff, "T%02xthread:%lx;",
            (UCHAR)uiSig, pdmsg->DTM_ulThread);                         /* ����ֹͣԭ��Ĭ��Ϊ�ж�     */

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: gdbNotfyStopReason
** ��������: non-stopģʽ�ϱ�ֹͣԭ��
** �䡡��  : pdmsg         �ж���Ϣ
**           pcOutBuff     ���������
** �䡡��  : ERROR_NONE ��ʾû�д���, PX_ERROR ��ʾ����
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT gdbNotfyStopReason (PLW_DTRACE_MSG pdmsg, PCHAR pcOutBuff)
{
    UINT uiSig = SIGSEGV;

    if (pdmsg->DTM_uiType == LW_TRAP_QUIT) {                            /* �����˳�                     */
        sprintf(pcOutBuff, "W00");
        return  (ERROR_NONE);
    }

#ifdef GDB_NON_STOP_BUG
    API_TimeMSleep(10);                                                 /* Ϊ����ͻ��� bug����ʱ 10ms  */
#endif                                                                  /* GDB_NON_STOP_BUG             */

    uiSig = ((pdmsg->DTM_uiType == LW_TRAP_ABORT) ? SIGSEGV : SIGTRAP);
    sprintf(pcOutBuff, "Stop:T%02xthread:%lx;",
            (UCHAR)uiSig, pdmsg->DTM_ulThread);                         /* ����ֹͣԭ��Ĭ��Ϊ�ж�     */

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: gdbRemoveBP
** ��������: �Ƴ��ϵ�
** �䡡��  : pparam        gdbȫ�ֶ���
**           cInBuff       rsp��������ַ�ͳ���
** �䡡��  : cOutBuff      rsp�������ڴ�����
**           ����ֵ        ERROR_NONE ��ʾû�д���, PX_ERROR ��ʾ����
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT gdbRemoveBP (LW_GDB_PARAM *pparam, PCHAR pcInBuff, PCHAR pcOutBuff)
{
    INT                 iType;
    addr_t              addrBp = 0;
    UINT                uiLen  = 0;
    PLW_LIST_LINE       plineTemp;
    LW_GDB_BP          *pbpItem;

    if (sscanf(pcInBuff, "%d,%lx,%x",
               &iType, (ULONG *)&addrBp, &uiLen) != 3) {
        gdbReplyError(pcOutBuff, 0);
        return  (ERROR_NONE);
    }

    plineTemp = pparam->GDB_plistBpHead;                                /* �ͷŶϵ�����                 */
    while (plineTemp) {
        pbpItem  = _LIST_ENTRY(plineTemp, LW_GDB_BP, BP_plistBpLine);
        plineTemp = _list_line_get_next(plineTemp);
        if (addrBp == pbpItem->BP_addr) {                               /* �ϵ��Ѵ���                   */
            if (API_DtraceBreakpointRemove(pparam->GDB_pvDtrace,
                                           pbpItem->BP_addr,
                                           pbpItem->BP_size,
                                           pbpItem->BP_ulInstOrg) != ERROR_NONE) {
                gdbReplyError(pcOutBuff, 0);
                return  (ERROR_NONE);
            }
            _List_Line_Del(&pbpItem->BP_plistBpLine, &pparam->GDB_plistBpHead);
            LW_GDB_SAFEFREE(pbpItem);
            break;                                                      /* ɾ���ɹ�                     */
        }
    }

    gdbReplyOk(pcOutBuff);

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: gdbInsertBP
** ��������: �Ƴ��ϵ�
** �䡡��  : pparam        gdbȫ�ֶ���
**           cInBuff       �Ĵ���������rsp����ʽ
** �䡡��  : cOutBuff      �Ĵ���ֵ��rsp����ʽ
**           ����ֵ        ERROR_NONE ��ʾû�д���, PX_ERROR ��ʾ����
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT gdbInsertBP (LW_GDB_PARAM *pparam, PCHAR pcInBuff, PCHAR pcOutBuff)
{
    INT                 iType;
    addr_t              addrBp = 0;
    UINT                uiLen  = 0;
    PLW_LIST_LINE       plineTemp;
    LW_GDB_BP          *pbpItem;

    if (sscanf(pcInBuff, "%d,%lx,%x", &iType,
               (ULONG *)&addrBp, &uiLen) != 3) {
        gdbReplyError(pcOutBuff, 0);
        return  (ERROR_NONE);
    }

    plineTemp = pparam->GDB_plistBpHead;
    while (plineTemp) {
        pbpItem  = _LIST_ENTRY(plineTemp, LW_GDB_BP, BP_plistBpLine);
        plineTemp = _list_line_get_next(plineTemp);
        if (addrBp == pbpItem->BP_addr) {                               /* �ϵ��Ѵ���                   */
            return  (ERROR_NONE);
        }
    }

    pbpItem = (LW_GDB_BP*)LW_GDB_SAFEMALLOC(sizeof(LW_GDB_BP));
    if (LW_NULL == pbpItem) {
        gdbReplyError(pcOutBuff, 0);
        return  (ERROR_NONE);
    }

    pbpItem->BP_addr = addrBp;
    pbpItem->BP_size = (uiLen != 0 ? uiLen : (LW_CFG_CPU_WORD_LENGHT / 8));
    if (API_DtraceBreakpointInsert(pparam->GDB_pvDtrace,
                                   pbpItem->BP_addr,
                                   pbpItem->BP_size,
                                   &pbpItem->BP_ulInstOrg) != ERROR_NONE) {
        LW_GDB_SAFEFREE(pbpItem);
        gdbReplyError(pcOutBuff, 0);
        return  (ERROR_NONE);
    }

    _List_Line_Add_Ahead(&pbpItem->BP_plistBpLine, 
                         &pparam->GDB_plistBpHead);                     /*  ����ϵ�����                */

    gdbReplyOk(pcOutBuff);

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: gdbIsBP
** ��������: �Ƴ��ϵ�
** �䡡��  : pparam        gdbȫ�ֶ���
** �䡡��  : adddrPc       ��ַ
**           ����ֵ        ERROR_NONE ��ʾû�д���, PX_ERROR ��ʾ����
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static BOOL gdbIsBP (LW_GDB_PARAM *pparam, addr_t adddrPc)
{
    PLW_LIST_LINE       plineTemp;
    LW_GDB_BP          *pbpItem;

    plineTemp = pparam->GDB_plistBpHead;
    while (plineTemp) {
        pbpItem  = _LIST_ENTRY(plineTemp, LW_GDB_BP, BP_plistBpLine);
        plineTemp = _list_line_get_next(plineTemp);
        if (adddrPc == pbpItem->BP_addr) {
            return  (LW_TRUE);
        }
    }

    return  (LW_FALSE);
}
/*********************************************************************************************************
** ��������: gdbRegGet
** ��������: ��ȡ�Ĵ���ֵ
** �䡡��  : iTid          �߳�id
**           cInBuff       �Ĵ���������rsp����ʽ
** �䡡��  : cOutBuff      �Ĵ���ֵ��rsp����ʽ
**           ����ֵ        ERROR_NONE ��ʾû�д���, PX_ERROR ��ʾ����
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT gdbRegsGet (LW_GDB_PARAM     *pparam,
                       PLW_DTRACE_MSG    pdmsg,
                       PCHAR             pcInBuff,
                       PCHAR             pcOutBuff)
{
    INT         i;
    PCHAR       pcPos;
    GDB_REG_SET regset;
    ULONG       ulThreadId;

    if ((pparam->GDB_lOpGThreadId == 0) || (pparam->GDB_lOpGThreadId == -1)) {
        ulThreadId = pdmsg->DTM_ulThread;
    } else {
        ulThreadId = pparam->GDB_lOpGThreadId;
    }

    pcPos = pcOutBuff;

    archGdbRegsGet(pparam->GDB_pvDtrace, ulThreadId, &regset);

    for (i = 0; i < regset.GDBR_iRegCnt; i++) {
#if defined(LW_CFG_CPU_ARCH_ARM64)
        if ((i == regset.GDBR_iPstateInx) ||
            (i == regset.GDBR_iFpsrInx)   ||
            (i == regset.GDBR_iFpcrInx)) {
            INT  j;
            for (j = 0; j < sizeof(INT); j++, regset.regArr[i].GDBRA_ulValue >>= 8) {
                gdbByte2Asc(pcPos + (j * 2), regset.regArr[i].GDBRA_ulValue & 0xff);
            }
            pcPos += sizeof(ULONG);

        } else {
            gdbReg2Asc(pcPos, regset.regArr[i].GDBRA_ulValue);
            pcPos += (2 * sizeof(ULONG));
        }
#else
        gdbReg2Asc(pcPos, regset.regArr[i].GDBRA_ulValue);
        pcPos += (2 * sizeof(ULONG));
#endif
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: gdbRegSet
** ��������: ���üĴ���ֵ
** �䡡��  : iTid          �߳�id
**           cInBuff       �Ĵ���������rsp����ʽ
** �䡡��  : cOutBuff      �Ĵ���ֵ��rsp����ʽ
**           ����ֵ        ERROR_NONE ��ʾû�д���, PX_ERROR ��ʾ����
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT gdbRegsSet (LW_GDB_PARAM     *pparam,
                       PLW_DTRACE_MSG    pdmsg,
                       PCHAR             pcInBuff,
                       PCHAR             pcOutBuff)
{
    PCHAR       pcPos;
    INT         i;
    INT         iLen;
    GDB_REG_SET regset;
    ULONG       ulThreadId;

    if ((pparam->GDB_lOpGThreadId == 0) || (pparam->GDB_lOpGThreadId == -1)) {
        ulThreadId = pdmsg->DTM_ulThread;
    } else {
        ulThreadId = pparam->GDB_lOpGThreadId;
    }

    iLen = lib_strlen(pcInBuff);

    archGdbRegsGet(pparam->GDB_pvDtrace, ulThreadId, &regset);

    pcPos = pcInBuff;
    for (i = 0; i < regset.GDBR_iRegCnt && iLen >= (i + 1) * 8; i++) {
        regset.regArr[i].GDBRA_ulValue = gdbAscToReg(pcPos);
        pcPos += (2 * sizeof(ULONG));
    }

    archGdbRegsSet(pparam->GDB_pvDtrace, ulThreadId, &regset);

    gdbReplyOk(pcOutBuff);

    return  (ERROR_NONE);
}
/*********************************************************************************************************
 ** ��������: gdbRegGet
 ** ��������: ��ȡ�Ĵ���ֵ
 ** �䡡��  : iTid          �߳�id
 **           cInBuff       �Ĵ���������rsp����ʽ
 ** �䡡��  : cOutBuff      �Ĵ���ֵ��rsp����ʽ
 **           ����ֵ        ERROR_NONE ��ʾû�д���, PX_ERROR ��ʾ����
 ** ȫ�ֱ���:
 ** ����ģ��:
 *********************************************************************************************************/
static INT gdbRegGet (LW_GDB_PARAM      *pparam,
                      PLW_DTRACE_MSG     pdmsg,
                      PCHAR              pcInBuff,
                      PCHAR              pcOutBuff)
 {
     GDB_REG_SET regset;
     INT         iRegIdx;
     ULONG       ulThreadId;

     if ((pparam->GDB_lOpGThreadId == 0) || (pparam->GDB_lOpGThreadId == -1)) {
         ulThreadId = pdmsg->DTM_ulThread;
     } else {
         ulThreadId = pparam->GDB_lOpGThreadId;
     }

     if (sscanf(pcInBuff, "%x", &iRegIdx) != 1) {
         gdbReplyError(pcOutBuff, 0);
         return  (PX_ERROR);
     }

     archGdbRegsGet(pparam->GDB_pvDtrace, ulThreadId, &regset);

     gdbReg2Asc(pcOutBuff, regset.regArr[iRegIdx].GDBRA_ulValue);

     return  (ERROR_NONE);
 }
/*********************************************************************************************************
** ��������: gdbRegSet
** ��������: ���üĴ���ֵ
** �䡡��  : iTid          �߳�id
**           cInBuff       �Ĵ���������rsp����ʽ
** �䡡��  : cOutBuff      �Ĵ���ֵ��rsp����ʽ
**           ����ֵ        ERROR_NONE ��ʾû�д���, PX_ERROR ��ʾ����
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT gdbRegSet (LW_GDB_PARAM      *pparam,
                      PLW_DTRACE_MSG     pdmsg,
                      PCHAR              pcInBuff,
                      PCHAR              pcOutBuff)
 {
     GDB_REG_SET regset;
     INT         iRegIdx;
     INT         iPos;
     ULONG       ulThreadId;

     if ((pparam->GDB_lOpGThreadId == 0) || (pparam->GDB_lOpGThreadId == -1)) {
         ulThreadId = pdmsg->DTM_ulThread;
     } else {
         ulThreadId = pparam->GDB_lOpGThreadId;
     }

     iPos = -1;
     sscanf(pcInBuff, "%x=%n", &iRegIdx, &iPos);
     if (iPos == -1) {
         gdbReplyError(pcOutBuff, 0);
        return  (PX_ERROR);
     }

     archGdbRegsGet(pparam->GDB_pvDtrace, ulThreadId, &regset);

     regset.regArr[iRegIdx].GDBRA_ulValue = gdbAscToReg(pcInBuff + iPos);

     archGdbRegsSet(pparam->GDB_pvDtrace, ulThreadId, &regset);

     return  (ERROR_NONE);
 }
/*********************************************************************************************************
** ��������: gdbMemGet
** ��������: ���üĴ���ֵ
** �䡡��  : cInBuff       rsp��������ַ�ͳ���
** �䡡��  : cOutBuff      rsp�������ڴ�����
**           ����ֵ        ERROR_NONE ��ʾû�д���, PX_ERROR ��ʾ����
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT gdbMemGet (LW_GDB_PARAM *pparam, PCHAR pcInBuff, PCHAR pcOutBuff)
{
    ULONG   ulAddr  = 0;
    UINT    uiLen   = 0;
    INT     i;
    PBYTE   pbyBuff = NULL;

    if (sscanf(pcInBuff, "%lx,%x", &ulAddr, &uiLen) != 2) {
        gdbReplyError(pcOutBuff, 0);
        return  (ERROR_NONE);
    }

    if (uiLen > (GDB_RSP_MAX_LEN - 16) / 2) {
        gdbReplyError(pcOutBuff, 1);
        return  (ERROR_NONE);
    }

    pbyBuff  = (PBYTE)LW_GDB_SAFEMALLOC(uiLen);                         /* �����ڴ�                     */
    if (NULL == pbyBuff) {
        gdbReplyError(pcOutBuff, 20);
        return  (PX_ERROR);
    }

    if (API_DtraceGetMems(pparam->GDB_pvDtrace, ulAddr,
                          pbyBuff, uiLen) != (ERROR_NONE)) {            /* ���ڴ�                       */
        LW_GDB_SAFEFREE(pbyBuff);
        gdbReplyError(pcOutBuff, 20);
        return  (ERROR_NONE);
    }

    for (i = 0; i < uiLen; i++) {
        gdbByte2Asc(pcOutBuff + i * 2, pbyBuff[i]);
    }

    LW_GDB_SAFEFREE(pbyBuff);

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: gdbRegSet
** ��������: ���üĴ���ֵ
** �䡡��  : cInBuff       rsp��������ַ�ͳ���
** �䡡��  : cOutBuff      rsp�������ڴ�����
**           ����ֵ        ERROR_NONE ��ʾû�д���, PX_ERROR ��ʾ����
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT gdbMemSet (LW_GDB_PARAM *pparam, PCHAR pcInBuff, PCHAR pcOutBuff)
{
    ULONG   ulAddr  = 0;
    UINT    uiLen   = 0;
    INT     iPos;
    INT     i;
    PBYTE   pbyBuff = NULL;

    if ((sscanf(pcInBuff, "%lx,%x:%n", &ulAddr, &uiLen, &iPos) < 2) || (iPos == -1)) {
        gdbReplyError(pcOutBuff, 0);
        return  (ERROR_NONE);
    }

    pbyBuff  = (PBYTE)LW_GDB_SAFEMALLOC(uiLen);                         /* �����ڴ�                     */
    if (NULL == pbyBuff) {
        gdbReplyError(pcOutBuff, 20);
        return  (PX_ERROR);
    }

    for (i = 0; i < uiLen; i++) {
        pbyBuff[i] = gdbAscToByte(pcInBuff + iPos + i * 2);
    }

    if (API_DtraceSetMems(pparam->GDB_pvDtrace, ulAddr,
                          pbyBuff, uiLen) != (ERROR_NONE)) {            /* д�ڴ�                       */
        LW_GDB_SAFEFREE(pbyBuff);
        gdbReplyError(pcOutBuff, 20);
        return  (ERROR_NONE);
    }

    gdbReplyOk(pcOutBuff);

    LW_GDB_SAFEFREE(pbyBuff);

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: gdbGoTo
** ��������: �ָ������Գ���ִ��
** �䡡��  : pparam        gdbȫ�ֲ���
**           iTid          �߳�id
**           cInBuff       rsp��������ʼִ�еĵ�ַ
** �䡡��  : cOutBuff      rsp������ʾ����ִ�н��
**           ����ֵ        ERROR_NONE ��ʾû�д���, PX_ERROR ��ʾ����
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT gdbGoTo (LW_GDB_PARAM    *pparam,
                    PLW_DTRACE_MSG   pdmsg,
                    PCHAR            pcInBuff,
                    PCHAR            pcOutBuff,
                    INT              iBeStep)
{
    INT     iSig;
    ULONG   ulAddr = 0;

    if (sscanf(pcInBuff, "%x;%lx", &iSig, &ulAddr) != 2) {
        sscanf(pcInBuff, "%lx", &ulAddr);
    }

    if (ulAddr != 0) {
        archGdbRegSetPc(pparam->GDB_pvDtrace, pdmsg->DTM_ulThread, ulAddr);
    }

    if (iBeStep) {                                                      /* ����������һ��ָ�����öϵ�   */
        gdbSetStepMode(pparam, pdmsg->DTM_ulThread);
    }

    if (pparam->GDB_lOpCThreadId <= 0) {
        API_DtraceContinueProcess(pparam->GDB_pvDtrace);
    } else {
        API_DtraceContinueThread(pparam->GDB_pvDtrace,
                                 pparam->GDB_lOpCThreadId);             /* ��ǰ�߳�ֹͣ������һ         */
    }

    gdbReplyOk(pcOutBuff);

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: gdbGetElfOffset
** ��������: ��ȡ�����ض�λ��ַ
** �䡡��  : pid           ����id
**           pcModPath     ģ��·��
** �䡡��  : addrText      text�ε�ַ
**           addrData      data�ε�ַ
**           addrBss       bss�ε�ַ
**           bSo           �Ƿ�so�ļ�
**           ����ֵ        ERROR_NONE ��ʾû�д���, PX_ERROR ��ʾ����
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT gdbGetElfOffset (pid_t   pid,
                            PCHAR   pcModPath,
                            addr_t *addrText,
                            addr_t *addrData,
                            addr_t *addrBss,
                            BOOL   *bSo)
{
    addr_t          addrBase;
    INT             iFd;
    INT             i;
    INT             iReadLen;

    Elf_Ehdr        ehdr;
    Elf_Shdr       *pshdr;
    Elf_Phdr       *pphdr;
    size_t          stHdSize;
    PCHAR           pcBuf    = NULL;
    PCHAR           pcShName = NULL;

    if (API_ModuleGetBase(pid, pcModPath, &addrBase, LW_NULL) != (ERROR_NONE)) {
        return  (PX_ERROR);
    }

    iFd = open(pcModPath, O_RDONLY);
    if (iFd < 0) {                                                      /*  ���ļ�                    */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "Open file error.\r\n");
        return  (PX_ERROR);
    }

    if (read(iFd, &ehdr, sizeof(ehdr)) < sizeof(ehdr)) {                /*  ��ȡELFͷ                   */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "Read elf header error.\r\n");
        return  (PX_ERROR);
    }

    if (ehdr.e_type == ET_REL) {                                        /*  ko �ļ�                     */
        (*bSo)   = LW_FALSE;
        stHdSize = ehdr.e_shentsize * ehdr.e_shnum;

        pcBuf = (PCHAR)LW_GDB_SAFEMALLOC(stHdSize);
        if (pcBuf == LW_NULL) {
            close(iFd);
            return  (PX_ERROR);
        }

        if (lseek(iFd, ehdr.e_shoff, SEEK_SET) < 0) {
            close(iFd);
            LW_GDB_SAFEFREE(pcBuf);
            return  (PX_ERROR);
        }

        if (read(iFd, pcBuf, stHdSize) < stHdSize) {
            close(iFd);
            LW_GDB_SAFEFREE(pcBuf);
            return  (PX_ERROR);
        }

        pshdr = (Elf_Shdr *)pcBuf;

        pcShName = (PCHAR)LW_GDB_SAFEMALLOC(pshdr[ehdr.e_shstrndx].sh_size);
        if (pcBuf == LW_NULL) {
            close(iFd);
            LW_GDB_SAFEFREE(pcBuf);
            return  (PX_ERROR);
        }

        if (lseek(iFd, pshdr[ehdr.e_shstrndx].sh_offset, SEEK_SET) < 0) {
            close(iFd);
            LW_GDB_SAFEFREE(pcBuf);
            LW_GDB_SAFEFREE(pcShName);
            return  (PX_ERROR);
        }

        iReadLen = read(iFd, pcShName, pshdr[ehdr.e_shstrndx].sh_size);
        if (iReadLen < pshdr[ehdr.e_shstrndx].sh_size) {
            close(iFd);
            LW_GDB_SAFEFREE(pcBuf);
            LW_GDB_SAFEFREE(pcShName);
            return  (PX_ERROR);
        }

        for (i = 0; i < ehdr.e_shnum; i++) {
            if (lib_strcmp(pcShName + pshdr[i].sh_name, ".text") == 0) {
                (*addrText) = addrBase + pshdr[i].sh_addr;
            }

            if (lib_strcmp(pcShName + pshdr[i].sh_name, ".data") == 0) {
                (*addrData) = addrBase + pshdr[i].sh_addr;
            }

            if (strcmp(pcShName + pshdr[i].sh_name, ".bss") == 0) {
                (*addrBss) = addrBase + pshdr[i].sh_addr;
            }
        }
    
    } else {                                                            /*  so �ļ�                     */
        (*bSo)   = LW_TRUE;
        stHdSize = ehdr.e_phentsize * ehdr.e_phnum;
        pcBuf    = (PCHAR)LW_GDB_SAFEMALLOC((stHdSize + ehdr.e_shentsize));

        if (LW_NULL == pcBuf) {
            close(iFd);
            LW_GDB_SAFEFREE(pcBuf);
            return  (PX_ERROR);
        }

        if (lseek(iFd, ehdr.e_phoff, SEEK_SET) < 0) {
            close(iFd);
            LW_GDB_SAFEFREE(pcBuf);
            return  (PX_ERROR);
        }

        if (read(iFd, pcBuf, stHdSize) < stHdSize) {
            close(iFd);
            LW_GDB_SAFEFREE(pcBuf);
            return  (PX_ERROR);
        }

        pphdr = (Elf_Phdr *)pcBuf;
        (*addrText) = 0;
        (*addrData) = 0;
        (*addrBss)  = 0;
        for (i = 0; i < ehdr.e_phnum; i++) {
            if (PT_LOAD != pphdr[i].p_type) {                           /*  ֻ����ɼ��ض�              */
                continue;
            }

            if ((*addrText) == 0) {                                     /*  ��һ���ɼ��ض�λ�����      */
                (*addrText) = addrBase + pphdr[i].p_vaddr;
                continue;
            }

            if ((*addrData) == 0) {                                     /*  �ڶ����ɼ��ض�λ���ݶ�      */
                (*addrData) = addrBase + pphdr[i].p_vaddr;
                (*addrBss)  = addrBase + pphdr[i].p_vaddr;
                break;
            }
        }
    }

    close(iFd);
    LW_GDB_SAFEFREE(pcBuf);
    LW_GDB_SAFEFREE(pcShName);

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: gdbHandleQCmd
** ��������: ��Ӧgdb Q����
** �䡡��  : pparam        gdbȫ�ֲ���
**           cInBuff       rsp��������ʼִ�еĵ�ַ
** �䡡��  : cOutBuff      rsp������ʾ����ִ�н��
**           ����ֵ        ERROR_NONE ��ʾû�д���, PX_ERROR ��ʾ����
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT gdbHandleQCmd (LW_GDB_PARAM *pparam, PCHAR pcInBuff, PCHAR pcOutBuff)
{
    if (lib_strstr(pcInBuff, "NonStop:1") == pcInBuff) {
#if LW_CFG_GDB_NON_STOP_EN > 0
        pparam->GDB_bNonStop = 1;
        gdbReplyOk(pcOutBuff);
#else
        /*
         * PowerPC �� x64 ������ NonStop ģʽ�󣬵���ʱ GDB �´����һ��ָ�ĳ����ִַ�У�
         * �÷����������ת��ָ���������
         */
        pparam->GDB_bNonStop = 0;
        gdbReplyError(pcOutBuff, 0);
#endif

    } else if (lib_strstr(pcInBuff, "NonStop:0") == pcInBuff) {
        pparam->GDB_bNonStop = 0;
        gdbReplyOk(pcOutBuff);
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: gdbCmdQuery
** ��������: ��Ӧgdb q����
** �䡡��  : pparam        gdbȫ�ֲ���
**           cInBuff       rsp��������ʼִ�еĵ�ַ
** �䡡��  : cOutBuff      rsp������ʾ����ִ�н��
**           ����ֵ        ERROR_NONE ��ʾû�д���, PX_ERROR ��ʾ����
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT gdbCmdQuery (LW_GDB_PARAM *pparam, PCHAR pcInBuff, PCHAR pcOutBuff)
{
    addr_t  addrText = 0;
    addr_t  addrData = 0;
    addr_t  addrBss  = 0;
    BOOL    bSo;

    UINT    i;
    UINT    uiStrLen = 0;
    ULONG   ulThreadId;
    CHAR    cThreadInfo[0x100] = {0};

    if (lib_strstr(pcInBuff, "Offsets") == pcInBuff) {                  /* ���س����ض�λ��Ϣ           */
        if (gdbGetElfOffset(pparam->GDB_iPid, pparam->GDB_cProgPath,
                            &addrText, &addrData, &addrBss, &bSo) != ERROR_NONE) {
            pcOutBuff[0] = 0;
        }
        /*
         *  ������ϵ�ṹ��mips��Ҫ����64λ���з���ƫ�ƣ�������32λ����ϵ�ṹ��arm��gdb���Զ��ض�.
         *  Ϊ��չ����λ����ַ��ת����LONG��ת����INT64
         */
        if (addrText) {
            if (bSo) {
                sprintf(pcOutBuff, "TextSeg=%llx;", (INT64)(LONG)addrText);
                if (addrData) {
                    sprintf(pcOutBuff + lib_strlen(pcOutBuff), "DataSeg=%llx", (INT64)(LONG)addrData);
                }
            } else {
                sprintf(pcOutBuff, "Text=%llx;", (INT64)(LONG)addrText);
                if (addrData) {
                    sprintf(pcOutBuff + lib_strlen(pcOutBuff), "Data=%llx;", (INT64)(LONG)addrData);
                    sprintf(pcOutBuff + lib_strlen(pcOutBuff), "Bss=%llx", (INT64)(LONG)addrData);
                }
            }
        } else {
            pcOutBuff[0] = 0;
        }
    
    } else if (lib_strstr(pcInBuff, "Supported") == pcInBuff) {         /* ��ȡ������                   */
#if defined(LW_CFG_CPU_ARCH_MIPS)
        sprintf(pcOutBuff, "PacketSize=2000;"
                           "qXfer:libraries:read+;"
                           "QNonStop+");
#elif defined(LW_CFG_CPU_ARCH_CSKY)
        sprintf(pcOutBuff, "PacketSize=2000;"
                           "qXfer:features:read+;"
                           "qXfer:libraries-svr4:read+;"
                           "QNonStop+");
#else
        sprintf(pcOutBuff, "PacketSize=2000;"
                           "qXfer:features:read+;"
                           "qXfer:libraries:read+;"
                           "QNonStop+");
#endif
    
    } else if (lib_strstr(pcInBuff, "Xfer:features:read:target.xml") == pcInBuff) {
        sprintf(pcOutBuff, archGdbTargetXml());
    
    } else if (lib_strstr(pcInBuff, "Xfer:features:read:arch-core.xml") == pcInBuff) {
        sprintf(pcOutBuff, archGdbCoreXml());
    
    } else if (lib_strstr(pcInBuff, "Xfer:libraries:read") == pcInBuff) {
        pcOutBuff[0] = 'l';
        if (vprocGetModsInfo(pparam->GDB_iPid, pcOutBuff + 1,
                             GDB_RSP_MAX_LEN - 4) <= 0) {
            gdbReplyError(pcOutBuff, 1);
            return  (PX_ERROR);
        }

#if defined(LW_CFG_CPU_ARCH_CSKY)
    } else if (lib_strstr(pcInBuff, "Xfer:libraries-svr4:read") == pcInBuff) {
        pcOutBuff[0] = 'l';
        if (vprocGetModsSvr4Info(pparam->GDB_iPid, pcOutBuff + 1,
                                 GDB_RSP_MAX_LEN - 4) <= 0) {
            gdbReplyError(pcOutBuff, 1);
            return  (PX_ERROR);
        }
#endif                                                                  /*  LW_CFG_CPU_ARCH_CSKY        */

    } else if (lib_strstr(pcInBuff, "fThreadInfo") == pcInBuff) {
        API_DtraceProcessThread(pparam->GDB_pvDtrace,
                                pparam->GDB_ulThreads,
                                GDB_MAX_THREAD_NUM,
                                &pparam->GDB_uiThdNum);                 /* ��ȡ�߳��б�                 */
        if (pparam->GDB_uiThdNum <= 0) {
            gdbReplyError(pcOutBuff, 1);
            return  (PX_ERROR);
        }

        uiStrLen += sprintf(pcOutBuff, "m");
        for (i = 0; i < pparam->GDB_uiThdNum - 1; i++) {
            uiStrLen += sprintf(pcOutBuff + uiStrLen, "%lx,", pparam->GDB_ulThreads[i]);
        }

        uiStrLen += sprintf(pcOutBuff + uiStrLen,
                            "%lx", pparam->GDB_ulThreads[i]);           /* ���һ���̲߳�׷��","        */
    
    } else if (lib_strstr(pcInBuff, "sThreadInfo") == pcInBuff) {
        sprintf(pcOutBuff, "l");
    
    } else if (lib_strstr(pcInBuff, "ThreadExtraInfo") == pcInBuff) {   /* ��ȡ�߳���Ϣ                 */
        if (sscanf(pcInBuff + lib_strlen("ThreadExtraInfo,"), "%lx", &ulThreadId) < 1) {
            gdbReplyError(pcOutBuff, 1);
            return  (PX_ERROR);
        }
        API_DtraceThreadExtraInfo(pparam->GDB_pvDtrace, ulThreadId,
                                  cThreadInfo, sizeof(cThreadInfo) - 1);
        for (i = 0; cThreadInfo[i] != 0; i++) {
            gdbByte2Asc(&pcOutBuff[i * 2], cThreadInfo[i]);
        }
    
    } else if (lib_strstr(pcInBuff, "C") == pcInBuff) {                 /* ��ȡ��ǰ�߳�                 */
        sprintf(pcOutBuff, "QC%lx", pparam->GDB_ulThreads[0]);

    } else {
        pcOutBuff[0] = 0;
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: gdbHcmdHandle
** ��������: ��Ӧgdb H����
** �䡡��  : pparam        gdbȫ�ֲ���
**           cInBuff       rsp��������ʼִ�еĵ�ַ
** �䡡��  : cOutBuff      rsp������ʾ����ִ�н��
**           ����ֵ        ERROR_NONE ��ʾû�д���, PX_ERROR ��ʾ����
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT gdbHcmdHandle (LW_GDB_PARAM *pparam, PCHAR pcInBuff, PCHAR pcOutBuff)
{
    CHAR    cOp;
    LONG    lThread;

    cOp = pcInBuff[0];
    if (pcInBuff[1] == '-') {
        if (sscanf(pcInBuff + 2, "%lx", (ULONG*)&lThread) < 1) {
            gdbReplyError(pcOutBuff, 1);
            return  (PX_ERROR);
        }
        lThread = 0 - lThread;
    
    } else {
        if (sscanf(pcInBuff + 1, "%lx", (ULONG*)&lThread) < 1) {
            gdbReplyError(pcOutBuff, 1);
            return  (PX_ERROR);
        }
    }

    if (cOp == 'c') {
        if (lThread == -1) {                                            /* -1��0����ʽһ��            */
            lThread = 0;
        }
        pparam->GDB_lOpCThreadId = lThread;
    
    } else if (cOp == 'g') {
        pparam->GDB_lOpGThreadId = lThread;
    
    } else {
        gdbReplyError(pcOutBuff, 1);
        return  (PX_ERROR);
    }

    gdbReplyOk(pcOutBuff);

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: gdbUpdateThreadList
** ��������: �����߳��б�
** �䡡��  : pparam        gdbȫ�ֲ���
**           ����ֵ        ERROR_NONE ��ʾû�д���, PX_ERROR ��ʾ����1��ʾ����ѭ��
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT gdbUpdateThreadList (LW_GDB_PARAM  *pparam)
{
    LW_LIST_LINE_HEADER     plistThd = LW_NULL;
    PLW_LIST_LINE           plineTemp;
    LW_GDB_THREAD          *pthItem;
    INT                     i;

    API_DtraceProcessThread(pparam->GDB_pvDtrace,
                            pparam->GDB_ulThreads,
                            GDB_MAX_THREAD_NUM,
                            &pparam->GDB_uiThdNum);                     /* ���»�ȡ�߳���Ϣ             */

    plistThd = pparam->GDB_plistThd;

    pparam->GDB_plistThd = LW_NULL;

    for (i = 0; i < pparam->GDB_uiThdNum; i++) {
        plineTemp = plistThd;
        while (plineTemp) {
            pthItem  = _LIST_ENTRY(plineTemp, LW_GDB_THREAD, TH_plistThLine);
            if (pparam->GDB_ulThreads[i] == pthItem->TH_ulId) {         /* �̴߳���                     */
                _List_Line_Del(&pthItem->TH_plistThLine, &plistThd);
                pthItem->TH_bChanged = LW_FALSE;
                _List_Line_Add_Ahead(&pthItem->TH_plistThLine,
                                     &pparam->GDB_plistThd);
                break;
            }
            plineTemp = _list_line_get_next(plineTemp);
        }

        if (!plineTemp) {                                               /* ���߳�                       */
            pthItem = (LW_GDB_THREAD*)LW_GDB_SAFEMALLOC(sizeof(LW_GDB_THREAD));
            if (pthItem) {
                pthItem->TH_ulId = pparam->GDB_ulThreads[i];
                pthItem->TH_addrHigh = 0;
                pthItem->TH_addrLow  = 0;
                pthItem->TH_cStates  = '\0';
                pthItem->TH_bChanged = LW_FALSE;
                _List_Line_Add_Ahead(&pthItem->TH_plistThLine,
                                     &pparam->GDB_plistThd);
            } else {
                _DebugHandle(__ERRORMESSAGE_LEVEL, "Memory alloc failed.\r\n");
                /*
                 *  ���̷���ʧ�ܣ����ᵼ�³������
                 */
            }
        }
    }

    while (plistThd) {                                                  /* �߳��ѽ���                   */
        pthItem  = _LIST_ENTRY(plistThd, LW_GDB_THREAD, TH_plistThLine);
        _List_Line_Del(&pthItem->TH_plistThLine, &plistThd);
        LW_GDB_SAFEFREE(pthItem);
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: gdbContHandle
** ��������: ��Ӧgdb vCont����
** �䡡��  : pparam        gdbȫ�ֲ���
**           cInBuff       rsp��������ʼִ�еĵ�ַ
** �䡡��  : cOutBuff      rsp������ʾ����ִ�н��
**           ����ֵ        ERROR_NONE ��ʾû�д���, PX_ERROR ��ʾ����1��ʾ����ѭ��
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT gdbContHandle (LW_GDB_PARAM     *pparam,
                          PLW_DTRACE_MSG    pdmsg,
                          PCHAR             pcInBuff,
                          PCHAR             pcOutBuff)
{
    CHAR        chOp  = '\0';
    ULONG       ulTid = 0;
    INT         iSigNo;
    GDB_REG_SET regset;
    PCHAR       pcPos = pcInBuff;
    addr_t      addrRangeLow;
    addr_t      addrRangeHigh;

    PLW_LIST_LINE           plineTemp;
    LW_GDB_THREAD          *pthItem;

    gdbUpdateThreadList(pparam);

    pparam->GDB_lOpCThreadId = PX_ERROR;

    while (pcPos[0] && pcPos[0] != '#') {
        chOp  = pcPos[0];
        ulTid = 0;
        if ('S' == chOp || 'C' == chOp) {
            sscanf(pcPos + 1, "%02x:%lx", &iSigNo, &ulTid);

        } else if ('s' == chOp || 'c' == chOp) {
            sscanf(pcPos + 1, ":%lx", &ulTid);

        } else if ('t' == chOp) {
            sscanf(pcPos + 1, ":%lx", &ulTid);

        } else if ('r' == chOp) {
            sscanf(pcPos + 1, "%lx,%lx:%lx", &addrRangeLow,
                   &addrRangeHigh, &ulTid);

        } else {
            gdbReplyError(pcOutBuff, 1);
            return  (ERROR_NONE);
        }

        plineTemp = pparam->GDB_plistThd;
        while (plineTemp) {
            pthItem  = _LIST_ENTRY(plineTemp, LW_GDB_THREAD, TH_plistThLine);
            if (pthItem->TH_ulId == ulTid ||
                (0 == ulTid && pthItem->TH_bChanged == LW_FALSE)) {
                pthItem->TH_bChanged = LW_TRUE;
                pthItem->TH_cStates  = chOp;
                if ('r' == chOp) {
                    pthItem->TH_addrLow = addrRangeLow;
                    pthItem->TH_addrHigh = addrRangeHigh;
                } else {
                    pthItem->TH_addrLow  = 0;
                    pthItem->TH_addrHigh = 0;
                }
            }

            plineTemp = _list_line_get_next(plineTemp);
        }

        if (('t' != chOp) && (pparam->GDB_lOpCThreadId != 0)) {         /* ��non-stop�ж�ʱ��ֹͣ����   */
            pparam->GDB_lOpCThreadId = ulTid;
        }

        while((*pcPos) && (*pcPos) != ';') {
            pcPos++;
        }

        if ((*pcPos) == ';') {
            pcPos++;
        }
    }

    if (pparam->GDB_lOpCThreadId == PX_ERROR) {
        pparam->GDB_lOpCThreadId = 0;
    }

    plineTemp = pparam->GDB_plistThd;
    while (plineTemp) {
        pthItem  = _LIST_ENTRY(plineTemp, LW_GDB_THREAD, TH_plistThLine);
        if (pthItem->TH_bChanged == LW_FALSE) {                         /* ״̬δ�ı�                   */
            plineTemp = _list_line_get_next(plineTemp);
            continue;
        }

        archGdbRegsGet(pparam->GDB_pvDtrace, pthItem->TH_ulId, &regset);

        /*
         *  TODO:Ŀǰ��ȡ��linux��ͬ�Ķ��̴߳�����ԣ����ϵ���һ��ȫ�����У����к�̨�̵߳����ϵ�
         *  �������
         */
        if (!gdbIsBP(pparam, archGdbRegGetPc(&regset))) {
            gdbRemoveStepBp(pparam, pthItem->TH_ulId);
        }
        gdbClearStepMode(pparam, pthItem->TH_ulId);

        switch (pthItem->TH_cStates) {
        
        case 'S':
        case 's':
        case 'r':
            gdbSetStepMode(pparam, pthItem->TH_ulId);
                                                                        /* ����Ҫbreak                  */
        case 'C':
        case 'c':
            API_DtraceContinueThread(pparam->GDB_pvDtrace,
                                     pthItem->TH_ulId);
            break;

        case 't':
            API_DtraceStopThread(pparam->GDB_pvDtrace,
                                 pthItem->TH_ulId);
            pdmsg->DTM_uiType   = 0;
            pdmsg->DTM_ulAddr   = 0;
            pdmsg->DTM_ulThread = pthItem->TH_ulId;
            pparam->GDB_lOptThreadId = pthItem->TH_ulId;
            if (pparam->GDB_bNonStop && !pparam->GDB_bStarted) {
                API_DtraceContinueProcess(pparam->GDB_pvDtrace);        /* non-stop����������           */
                pparam->GDB_bStarted = LW_TRUE;
            }
            break;

        default:
            break;
        }

        plineTemp = _list_line_get_next(plineTemp);
    }

    gdbReplyOk(pcOutBuff);

    return  (pparam->GDB_bNonStop ? (ERROR_NONE): 1);                   /* non-stopģʽ������ѭ�����˳� */
}
/*********************************************************************************************************
** ��������: gdbHcmdHandle
** ��������: ��Ӧgdb v����
** �䡡��  : pparam        gdbȫ�ֲ���
**           cInBuff       rsp��������ʼִ�еĵ�ַ
** �䡡��  : cOutBuff      rsp������ʾ����ִ�н��
**           ����ֵ        ERROR_NONE ��ʾû�д���, PX_ERROR ��ʾ����, 1��ʾ
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT gdbVcmdHandle (LW_GDB_PARAM     *pparam,
                          PLW_DTRACE_MSG    pdmsg,
                          PCHAR             pcInBuff,
                          PCHAR             pcOutBuff)
{
    PLW_LIST_LINE   plineTemp;
    LW_GDB_THREAD  *pthItem;

    if (lib_strstr(pcInBuff, "Cont?") == pcInBuff) {                    /* ֧�ֵ�vCont�����б�          */
        if (pparam->GDB_bNonStop) {
            lib_strncpy(pcOutBuff, "vCont;c;C;s;S;t", GDB_RSP_MAX_LEN); /* nonstopģʽ�ݲ�֧��steprange */
        } else {
            lib_strncpy(pcOutBuff, "vCont;c;C;s;S;t,r", GDB_RSP_MAX_LEN);
        }

    } else if (lib_strstr(pcInBuff, "Cont;") == pcInBuff) {             /* vCont�����                */
        return gdbContHandle(pparam,
                             pdmsg,
                             pcInBuff + lib_strlen("Cont;"),
                             pcOutBuff);

    } else if (lib_strstr(pcInBuff, "Stopped") == pcInBuff) {           /* ��ѯ��һ���ϵ��߳�           */
        if (API_DtraceGetBreakInfo(pparam->GDB_pvDtrace,
                                   pdmsg, 0) == ERROR_NONE) {
            if (gdbIsStepBp(pparam, pdmsg)) {
                gdbClearStepMode(pparam, pdmsg->DTM_ulThread);
            }

            gdbUpdateThreadList(pparam);
            plineTemp = pparam->GDB_plistThd;
            while (plineTemp) {                                         /* ��¼�߳�״̬                 */
                pthItem  = _LIST_ENTRY(plineTemp, LW_GDB_THREAD, TH_plistThLine);
                if (pthItem->TH_ulId == pdmsg->DTM_ulThread) {
                    pthItem->TH_cStates = 'b';
                }
                plineTemp = _list_line_get_next(plineTemp);
            }

            gdbReportStopReason(pdmsg, pcOutBuff);
            return  (ERROR_NONE);
        }

        pparam->GDB_beNotifing = 0;                                     /* notify�������               */
        gdbReplyOk(pcOutBuff);
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: gdbCheckThread
** ��������: ��Ӧgdb T�����ѯ�߳�״̬
** �䡡��  : pparam        gdbȫ�ֲ���
**           cInBuff       rsp��������ʼִ�еĵ�ַ
** �䡡��  : cOutBuff      rsp������ʾ����ִ�н��
**           ����ֵ        ERROR_NONE ��ʾû�д���, PX_ERROR ��ʾ����
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT gdbCheckThread (LW_GDB_PARAM *pparam, PCHAR pcInBuff, PCHAR pcOutBuff)
{
    ULONG   ulThread;
    
    if (sscanf(pcInBuff, "%lx", (ULONG*)&ulThread) < 1) {
        gdbReplyError(pcOutBuff, 1);
        return  (PX_ERROR);
    }

    if (API_ThreadVerify(ulThread)) {
        gdbReplyOk(pcOutBuff);
        return  (ERROR_NONE);
    
    } else {
        gdbReplyError(pcOutBuff, 10);
        return  (ERROR_NONE);
    }
}
/*********************************************************************************************************
** ��������: gdbRspPkgHandle
** ��������: ����rspЭ����������gdb�Ľ���
** �䡡��  : pparam     ϵͳ����
** �䡡��  : ERROR_NONE ��ʾû�д���, PX_ERROR ��ʾ����
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT gdbRspPkgHandle (LW_GDB_PARAM    *pparam,
                            PLW_DTRACE_MSG   pdmsg,
                            BOOL             bNeedReport)
{
    struct signalfd_siginfo  fdsi;
    INT                      iRet;
    CHAR                    *cInBuff   = LW_NULL;
    CHAR                    *cOutBuff  = LW_NULL;

    PLW_LIST_LINE            plineTemp;
    LW_GDB_THREAD           *pthItem;

    pparam->GDB_lOpGThreadId = 0;                                       /* ÿ�ν�ȥѭ��ǰ��λHg��ֵ     */

    cInBuff  = (CHAR *)LW_GDB_SAFEMALLOC(GDB_RSP_MAX_LEN);
    cOutBuff = (CHAR *)LW_GDB_SAFEMALLOC(GDB_RSP_MAX_LEN);
    if (!cInBuff || !cOutBuff) {
        LW_GDB_SAFEFREE(cInBuff);
        LW_GDB_SAFEFREE(cOutBuff);
        _DebugHandle(__ERRORMESSAGE_LEVEL, "Memory alloc failed.\r\n");
        return  (PX_ERROR);
    }

    if (bNeedReport) {                                                  /* �ϱ�ֹͣԭ��                 */
        gdbReportStopReason(pdmsg, cOutBuff);
        if (gdbRspPkgPut(pparam, cOutBuff, LW_FALSE) < 0) {
            LW_GDB_SAFEFREE(cInBuff);
            LW_GDB_SAFEFREE(cOutBuff);
            LW_GDB_MSG("[GDB]Host close.\n");
            return  (PX_ERROR);
        
        } else {
            if (pdmsg->DTM_uiType == LW_TRAP_QUIT &&
                pparam->GDB_byCommType == COMM_TYPE_TTY) {              /* ����ģʽ���ٵȴ��ͻ���֪ͨ   */
                LW_GDB_SAFEFREE(cInBuff);
                LW_GDB_SAFEFREE(cOutBuff);
                LW_GDB_MSG("[GDB]Host close.\n");
                return  (PX_ERROR);
            }
        }
    }

    while (1) {
        lib_bzero(cInBuff, GDB_RSP_MAX_LEN);
        lib_bzero(cOutBuff, GDB_RSP_MAX_LEN);
        lib_bzero(&fdsi, sizeof(fdsi));

        if (pparam->GDB_lOptThreadId != 0) {                            /* �ϸ�������ͣ�߳�,��: vCont;t */
            gdbNotfyStopReason(pdmsg, cOutBuff);
            gdbRspPkgPut(pparam, cOutBuff, LW_TRUE);
            pparam->GDB_lOptThreadId = 0;
            pparam->GDB_beNotifing = 1;
        }

        iRet = gdbRspPkgGet(pparam, cInBuff, &fdsi);
        if (iRet < 0) {
            LW_GDB_SAFEFREE(cInBuff);
            LW_GDB_SAFEFREE(cOutBuff);
            LW_GDB_MSG("[GDB]Host close.\n");
            return  (PX_ERROR);
        }

        if (fdsi.ssi_signo == SIGTRAP) {                                /* non-stop�첽�ϱ�             */
            if (pparam->GDB_beNotifing == 1) {                          /* ���ڴ�����һ��notify         */
                continue;
            }

            if (API_DtraceGetBreakInfo(pparam->GDB_pvDtrace,
                                       pdmsg, 0) == ERROR_NONE) {
                if (gdbIsStepBp(pparam, pdmsg)) {
                    gdbClearStepMode(pparam, pdmsg->DTM_ulThread);
                    if (archGdbGetStepSkip(pparam->GDB_pvDtrace,
                                           pdmsg->DTM_ulThread,
                                           pdmsg->DTM_ulAddr)) {        /*  �Ƿ���Դ˵����ϵ�          */
                        gdbSetStepMode(pparam, pdmsg->DTM_ulThread);
                        API_DtraceContinueThread(pparam->GDB_pvDtrace, pdmsg->DTM_ulThread);
                        continue;
                    }
                }

                gdbUpdateThreadList(pparam);
                plineTemp = pparam->GDB_plistThd;
                while (plineTemp) {                                     /* ��¼�߳�״̬                 */
                    pthItem  = _LIST_ENTRY(plineTemp, LW_GDB_THREAD, TH_plistThLine);
                    if (pthItem->TH_ulId == pdmsg->DTM_ulThread) {
                        pthItem->TH_cStates = 'b';
                    }
                    plineTemp = _list_line_get_next(plineTemp);
                }

                pparam->GDB_beNotifing = 1;
                gdbNotfyStopReason(pdmsg, cOutBuff);
                gdbRspPkgPut(pparam, cOutBuff, LW_TRUE);
                continue;
            }

        } else if ((fdsi.ssi_signo == SIGCHLD) &&
                    (fdsi.ssi_code == CLD_EXITED)) {                    /* �����˳�                     */
            pdmsg->DTM_ulAddr   = 0;
            pdmsg->DTM_ulThread = LW_OBJECT_HANDLE_INVALID;
            pdmsg->DTM_uiType   = LW_TRAP_QUIT;
            pparam->GDB_bExited = LW_TRUE;
            gdbNotfyStopReason(pdmsg, cOutBuff);
            gdbRspPkgPut(pparam, cOutBuff, LW_FALSE);
            LW_GDB_SAFEFREE(cInBuff);
            LW_GDB_SAFEFREE(cOutBuff);
            LW_GDB_MSG("[GDB]Target process exit.\n");
            return  (PX_ERROR);
        }

        switch (cInBuff[0]) {
        
        case 'g':
            gdbRegsGet(pparam, pdmsg, cInBuff + 1, cOutBuff);
            break;
        
        case 'G':
            gdbRegsSet(pparam, pdmsg, cInBuff + 1, cOutBuff);
            break;
        
        case 'p':
            gdbRegGet(pparam, pdmsg, cInBuff + 1, cOutBuff);
            break;
        
        case 'P':
            gdbRegSet(pparam, pdmsg, cInBuff + 1, cOutBuff);
            break;
        
        case 'm':
            gdbMemGet(pparam, cInBuff + 1, cOutBuff);
            break;
        
        case 'M':
            gdbMemSet(pparam, cInBuff + 1, cOutBuff);
            break;
        
        case 'c':
            gdbGoTo(pparam, pdmsg, cInBuff+1, cOutBuff, 0);
            LW_GDB_SAFEFREE(cInBuff);
            LW_GDB_SAFEFREE(cOutBuff);
            return  (ERROR_NONE);
        
        case 'q':
            gdbCmdQuery(pparam, cInBuff + 1, cOutBuff);
            break;
        
        case 'Q':
            gdbHandleQCmd(pparam, cInBuff + 1, cOutBuff);
            break;
        
        case '?':
            if (pparam->GDB_bNonStop &&
                API_DtraceGetBreakInfo(pparam->GDB_pvDtrace,
                                       pdmsg, 0) == ERROR_NONE) {
                if (gdbIsStepBp(pparam, pdmsg)) {
                    gdbClearStepMode(pparam, pdmsg->DTM_ulThread);
                }

                gdbUpdateThreadList(pparam);
                plineTemp = pparam->GDB_plistThd;
                while (plineTemp) {                                     /* ��¼�߳�״̬                 */
                    pthItem  = _LIST_ENTRY(plineTemp, LW_GDB_THREAD, TH_plistThLine);

                    if (pthItem->TH_ulId == pdmsg->DTM_ulThread) {
                        pthItem->TH_cStates = 'b';
                    }

                    plineTemp = _list_line_get_next(plineTemp);
                }

            }

            gdbReportStopReason(pdmsg, cOutBuff);
            break;
        
        case 'H':
            gdbHcmdHandle(pparam, cInBuff + 1, cOutBuff);
            break;
        
        case 'T':
            gdbCheckThread(pparam, cInBuff + 1, cOutBuff);
            break;
        
        case 's':
            gdbGoTo(pparam, pdmsg, cInBuff + 1, cOutBuff, 1);
            LW_GDB_SAFEFREE(cInBuff);
            LW_GDB_SAFEFREE(cOutBuff);
            return  (ERROR_NONE);
            break;

#if !defined(LW_CFG_CPU_ARCH_C6X)
        case 'z':
            gdbRemoveBP(pparam, cInBuff + 1, cOutBuff);
            break;

        case 'Z':
            gdbInsertBP(pparam, cInBuff + 1, cOutBuff);
            break;
#endif

        case 'v':
            if (gdbVcmdHandle(pparam, pdmsg, cInBuff + 1, cOutBuff) > 0) {
                LW_GDB_SAFEFREE(cInBuff);
                LW_GDB_SAFEFREE(cOutBuff);
                return  (ERROR_NONE);
            }
            break;

        case 'k':
            LW_GDB_SAFEFREE(cInBuff);
            LW_GDB_SAFEFREE(cOutBuff);
            LW_GDB_MSG("[GDB]Host stoped.\n");
            return  (PX_ERROR);

        default:
            cOutBuff[0] = 0;
            break;
        }

        if (gdbRspPkgPut(pparam, cOutBuff, LW_FALSE) < 0) {
            LW_GDB_SAFEFREE(cInBuff);
            LW_GDB_SAFEFREE(cOutBuff);
            LW_GDB_MSG("[GDB]Host close.\n");
            return  (PX_ERROR);
        }
    }

    LW_GDB_SAFEFREE(cInBuff);
    LW_GDB_SAFEFREE(cOutBuff);

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: gdbWaitGdbSig
** ��������: �¼�ѭ��������ϵͳ��Ϣ����Ⲣ����ϵ�
** �䡡��  : pparam       ϵͳ����
** �䡡��  : bClientSig   �Ƿ�ͻ����ź�
**           pfdsi        ���ܵ����ź���Ϣ
**           ����         ERROR_NONE ��ʾû�д���, PX_ERROR ��ʾ����
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT gdbWaitSig (LW_GDB_PARAM            *pparam,
                       BOOL                    *pbClientSig,
                       struct signalfd_siginfo *pfdsi)
{
    fd_set      fdset;
    ssize_t     szLen;
    INT         iMaxFd;
    CHAR        chCmd;

    (*pbClientSig) = LW_FALSE;

    FD_ZERO(&fdset);
    FD_SET(pparam->GDB_iCommFd, &fdset);
    FD_SET(pparam->GDB_iSigFd, &fdset);
    iMaxFd = max(pparam->GDB_iCommFd, pparam->GDB_iSigFd);

    while (select(iMaxFd + 1, &fdset, NULL, NULL, NULL) >= 0) {
        if (FD_ISSET(pparam->GDB_iCommFd, &fdset)) {
            if (read(pparam->GDB_iCommFd,
                     &chCmd,
                     sizeof(chCmd)) <= 0) {                             /* �ж�����0x3                  */
                return  (PX_ERROR);
            }

            if (chCmd == 0x3) {
                (*pbClientSig) = LW_TRUE;
                return  (ERROR_NONE);
            }
        }

        if (FD_ISSET(pparam->GDB_iSigFd, &fdset)) {
            szLen = read(pparam->GDB_iSigFd,
                         pfdsi,
                         sizeof(struct signalfd_siginfo));              /* �ж�                         */
            if (szLen != sizeof(struct signalfd_siginfo)) {
                goto    __re_select;
            }
            return  (ERROR_NONE);
        }

__re_select:
        FD_SET(pparam->GDB_iCommFd, &fdset);
        FD_SET(pparam->GDB_iSigFd, &fdset);
    }

    return  (PX_ERROR);
}
/*********************************************************************************************************
** ��������: gdbSigInit
** ��������: ��ʼ���źž��
** �䡡��  :
** �䡡��  : signalfd���
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT gdbSigInit (VOID)
{
    sigset_t    sigset;

    sigemptyset(&sigset);
    sigaddset(&sigset, SIGTRAP);
    sigaddset(&sigset, SIGCHLD);

    if (sigprocmask(SIG_BLOCK, &sigset, NULL) < ERROR_NONE) {
        return  (PX_ERROR);
    }

    return  (signalfd(-1, &sigset, 0));
}
/*********************************************************************************************************
** ��������: gdbRelease
** ��������: �ͷ���Դ
** �䡡��  : pparam     ϵͳ����
** �䡡��  : ERROR_NONE ��ʾû�д���, PX_ERROR ��ʾ����
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT gdbRelease (LW_GDB_PARAM *pparam)
{
    pparam->GDB_plistBpHead = LW_NULL;

    if (pparam->GDB_pvDtrace != LW_NULL) {
        API_DtraceDelete(pparam->GDB_pvDtrace);
    }

    if (pparam->GDB_iSigFd >= 0) {
        close(pparam->GDB_iSigFd);
    }

    if (pparam->GDB_iCommFd >= 0) {
        if (pparam->GDB_bTerminal) {                                    /* ��ԭ�ն�֮ǰ��ͨ��ģʽ       */
            if (pparam->GDB_spSerial.GDBSP_iHwOpt != SERIAL_HWOPT) {
                ioctl(pparam->GDB_iCommFd, SIO_HW_OPTS_SET, 
                      pparam->GDB_spSerial.GDBSP_iHwOpt);
            }
            if (pparam->GDB_spSerial.GDBSP_lBaud != SERIAL_BAUD) {
                ioctl(pparam->GDB_iCommFd, SIO_BAUD_SET,
                      pparam->GDB_spSerial.GDBSP_lBaud);
            }
            ioctl(pparam->GDB_iCommFd, FIOSETOPTIONS,
                  pparam->GDB_spSerial.GDBSP_iSioOpt);
            
        } else {
            close(pparam->GDB_iCommFd);
        }
    }

    if (pparam->GDB_iCommListenFd >= 0) {
        close(pparam->GDB_iCommListenFd);
    }

    LW_GDB_SAFEFREE(pparam);

    LW_GDB_MSG("[GDB]Server exit.\n");

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: gdbReadEvent
** ��������: ��ȡ�¼���Ϣ
** �䡡��  : pparam     ϵͳ����
** �䡡��  : pdmsg      �¼���Ϣ
**           ERROR_NONE ��ʾû�д���, PX_ERROR ��ʾ����
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT gdbReadEvent (LW_GDB_PARAM *pparam, PLW_DTRACE_MSG pdmsg)
{
    return  ((INT)API_DtraceGetBreakInfo(pparam->GDB_pvDtrace, pdmsg,
                                         pparam->GDB_lOpCThreadId));
}
/*********************************************************************************************************
 ** ��������: gdbInStepRange
 ** ��������: �ж��Ƿ����򵥲�
 ** �䡡��  : pparam     ϵͳ����
 ** �䡡��  : pdmsg      �¼���Ϣ
 **           LW_TRUE ��, LW_FALSE ��
 ** ȫ�ֱ���:
 ** ����ģ��:
 *********************************************************************************************************/
static BOOL gdbInStepRange (LW_GDB_PARAM *pparam, PLW_DTRACE_MSG pdmsg)
{
    PLW_LIST_LINE   plineTemp;
    LW_GDB_THREAD  *pthItem;

    if (gdbIsBP(pparam, pdmsg->DTM_ulAddr)) {
        return  (LW_FALSE);
    }

    plineTemp = pparam->GDB_plistThd;
    while (plineTemp) {
        pthItem  = _LIST_ENTRY(plineTemp, LW_GDB_THREAD, TH_plistThLine);
        if (pthItem->TH_ulId == pdmsg->DTM_ulThread) {
            if (pdmsg->DTM_ulAddr >= pthItem->TH_addrLow &&
                pdmsg->DTM_ulAddr < pthItem->TH_addrHigh) {
                return  (LW_TRUE);
            } else {
                pthItem->TH_addrLow  = 0;
                pthItem->TH_addrHigh = 0;
                return  (LW_FALSE);
            }
        }
        plineTemp = _list_line_get_next(plineTemp);
    }
    
    return  (LW_FALSE);
}
/*********************************************************************************************************
** ��������: gdbEventLoop
** ��������: �¼�ѭ��������ϵͳ��Ϣ����Ⲣ����ϵ�
** �䡡��  : pparam     ϵͳ����
** �䡡��  : ERROR_NONE ��ʾû�д���, PX_ERROR ��ʾ����
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT gdbEventLoop (LW_GDB_PARAM *pparam)
{
    LW_DTRACE_MSG           dmsg;
    BOOL                    bClientSig;
    struct signalfd_siginfo fdsi;

    dmsg.DTM_ulThread = vprocMainThread(pparam->GDB_iPid);
    dmsg.DTM_uiType   = 0;
    dmsg.DTM_ulAddr   = 0;

    if (gdbRspPkgHandle(pparam, &dmsg, LW_FALSE) == PX_ERROR) {         /* ��ʼ��ʱ���̴�����ͣ״̬     */
        return  (ERROR_NONE);
    }

    /*
     *  ���³���ֻ�е���non-stopģʽ�Ž���
     */
    while (gdbWaitSig(pparam, &bClientSig, &fdsi) == ERROR_NONE) {
        if (bClientSig) {                                               /* �ͻ����źŶ��⴦��           */
            API_DtraceStopProcess(pparam->GDB_pvDtrace);                /* ֹͣ����                     */
            dmsg.DTM_ulThread = vprocMainThread(pparam->GDB_iPid);
            if (gdbRspPkgHandle(pparam, &dmsg, LW_TRUE) == PX_ERROR) {  /* С��0��ʾ�ͻ��˹ر�����      */
                return  (ERROR_NONE);
            }
        }

        if ((fdsi.ssi_signo == SIGCHLD) &&
            (fdsi.ssi_code == CLD_EXITED)) {                            /* �����˳�                     */
            dmsg.DTM_ulAddr     = 0;
            dmsg.DTM_ulThread   = LW_OBJECT_HANDLE_INVALID;
            dmsg.DTM_uiType     = LW_TRAP_QUIT;
            pparam->GDB_bExited = LW_TRUE;
            gdbRspPkgHandle(pparam, &dmsg, LW_TRUE);
            LW_GDB_MSG("[GDB]Target process exit.\n");
            return  (ERROR_NONE);
        }

        while (gdbReadEvent(pparam, &dmsg) == ERROR_NONE) {
            if (pparam->GDB_lOpCThreadId <= 0) {
                API_DtraceStopProcess(pparam->GDB_pvDtrace);            /* ֹͣ����                     */
                API_DtraceContinueThread(pparam->GDB_pvDtrace,
                                         dmsg.DTM_ulThread);            /* ��ǰ�߳�ֹͣ������һ         */
            }

            if (gdbIsStepBp(pparam, &dmsg)) {                           /* �Զ��Ƴ������ϵ�             */
                gdbClearStepMode(pparam, dmsg.DTM_ulThread);
                if (gdbInStepRange(pparam, &dmsg) ||
                    archGdbGetStepSkip(pparam->GDB_pvDtrace,
                                       dmsg.DTM_ulThread,
                                       dmsg.DTM_ulAddr)) {              /* �Ƿ���Ҫ���Դ˵����ϵ�       */
                    gdbSetStepMode(pparam, dmsg.DTM_ulThread);
                    if (pparam->GDB_lOpCThreadId <= 0) {
                        API_DtraceContinueProcess(pparam->GDB_pvDtrace);
                    } else {
                        API_DtraceContinueThread(pparam->GDB_pvDtrace, dmsg.DTM_ulThread);
                    }
                    continue;
                }
            }

            if (gdbRspPkgHandle(pparam, &dmsg, LW_TRUE) == PX_ERROR) {  /* С��0��ʾ�ͻ��˹ر�����      */
                return  (ERROR_NONE);
            }
        }
    }

    API_DtraceStopProcess(pparam->GDB_pvDtrace);                        /* ������Ͽ��˳�gdb����ͣ����  */
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: gdbWaitSpawmSig
** ��������: �ȴ���������ź�
** �䡡��  : iSigNo �ź�ֵ
** �䡡��  : ERROR_NONE ��ʾû�д���, PX_ERROR ��ʾ����
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT gdbWaitSpawmSig (INT iSigNo)
{
    sigset_t                sigset;
    INT                     iSigFd;
    ssize_t                 szLen;
    struct signalfd_siginfo fdsi;

    sigemptyset(&sigset);
    sigaddset(&sigset, iSigNo);

    if (sigprocmask(SIG_BLOCK, &sigset, NULL) < ERROR_NONE) {
        return  (PX_ERROR);
    }

    iSigFd = signalfd(-1, &sigset, 0);
    if (iSigFd < 0) {
        return  (PX_ERROR);
    }

    szLen = read(iSigFd, &fdsi, sizeof(struct signalfd_siginfo));
    if (szLen != sizeof(struct signalfd_siginfo)) {
        close(iSigFd);
        return  (PX_ERROR);
    }

    close(iSigFd);

    return  (fdsi.ssi_int);
}
/*********************************************************************************************************
** ��������: gdbExit
** ��������: �˳�������SIGKILL �źŴ�����
** �䡡��  : iSigNo �ź�ֵ
** �䡡��  :
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static VOID gdbExit (INT iSigNo)
{
    LW_GDB_PARAM       *pparam    = LW_NULL;
    PLW_LIST_LINE       plineTemp;
    LW_GDB_BP          *pbpItem;
    LW_GDB_THREAD      *pthItem;
    LW_DTRACE_MSG       dmsg;
    
#if LW_CFG_SMP_EN > 0
    cpu_set_t           cpuset;
#endif                                                                  /* LW_CFG_SMP_EN > 0            */

    pparam = (LW_GDB_PARAM*)API_ThreadGetNotePad(API_ThreadIdSelf(), 0);/* ��ȡ����������               */
    if (LW_NULL == pparam) {
        return;
    }

    plineTemp = pparam->GDB_plistBpHead;                                /* �ͷŶϵ�����                 */
    while (plineTemp) {
        pbpItem  = _LIST_ENTRY(plineTemp, LW_GDB_BP, BP_plistBpLine);
        plineTemp = _list_line_get_next(plineTemp);
        if (!pparam->GDB_bExited) {
            API_DtraceBreakpointRemove(pparam->GDB_pvDtrace,
                                       pbpItem->BP_addr,
                                       pbpItem->BP_size,
                                       pbpItem->BP_ulInstOrg);
        }
        LW_GDB_SAFEFREE(pbpItem);
    }
    pparam->GDB_plistBpHead = LW_NULL;

    /*
     *  �ͷ��߳�����������gdbֹͣ���߳�
     */
    if (!pparam->GDB_bExited) {
        gdbUpdateThreadList(pparam);
    }

    plineTemp = pparam->GDB_plistThd;
    while (plineTemp) {
        pthItem  = _LIST_ENTRY(plineTemp, LW_GDB_THREAD, TH_plistThLine);

        if (!pparam->GDB_bExited) {
            gdbClearStepMode(pparam, pthItem->TH_ulId);

            if (pparam->GDB_bNonStop) {                                 /* nonstopģʽ�赥�������߳�    */
                if (pthItem->TH_cStates == 't' ||
                    pthItem->TH_cStates == 'b') {
                    API_DtraceContinueThread(pparam->GDB_pvDtrace,
                                             pthItem->TH_ulId);
                }
            }
        }

        plineTemp = _list_line_get_next(plineTemp);
        LW_GDB_SAFEFREE(pthItem);
    }
    pparam->GDB_plistThd = LW_NULL;

    while (API_DtraceGetBreakInfo(pparam->GDB_pvDtrace,
                                  &dmsg, 0) == ERROR_NONE) {            /* �������ϵ�ֹͣ���߳�         */
        if (!pparam->GDB_bExited) {
            API_DtraceContinueThread(pparam->GDB_pvDtrace, dmsg.DTM_ulThread);
        }
    }

    if (!pparam->GDB_bAttached && !pparam->GDB_bExited) {               /* �������attch��ֹͣ����      */
        kill(pparam->GDB_iPid, SIGKILL);                                /* ǿ�ƽ���ֹͣ                 */
        LW_GDB_MSG("[GDB]Warning: Process is killed (SIGKILL) by GDB server.\n"
                   "     Restart SylixOS is recommended!\n");
    }
#if LW_CFG_SMP_EN > 0
    else if (pparam->GDB_bLockCpu) {
        CPU_ZERO(&cpuset);
        sched_setaffinity(pparam->GDB_iPid, sizeof(cpu_set_t), &cpuset);/* ��������ȡ������ CPU         */
    }
#endif                                                                  /* LW_CFG_SMP_EN > 0            */

    if (!pparam->GDB_bNonStop && !pparam->GDB_bExited) {
        API_DtraceContinueProcess(pparam->GDB_pvDtrace);                /* �������ִ��                 */
    }

    gdbRelease(pparam);

#ifndef LW_DTRACE_HW_ISTEP
    if (API_AtomicGet(&GHookRefCnt) > 0) {
        if (API_AtomicDec(&GHookRefCnt) == 0) {                         /* ���һ�ε������ʱ���HOOK   */
            API_SystemHookDelete(API_DtraceSchedHook,
                                 LW_OPTION_THREAD_SWAP_HOOK);           /* ɾ���߳��л�HOOK��           */
        }
    }
#endif                                                                  /* !LW_DTRACE_HW_ISTEP          */
}
/*********************************************************************************************************
** ��������: gdbMain
** ��������: gdb stub ������
** �䡡��  : argc         ��������
**           argv         ������
** �䡡��  : ERROR_NONE ��ʾû�д���, PX_ERROR ��ʾ����
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT gdbMain (INT argc, CHAR **argv)
{
    LW_GDB_PARAM       *pparam    = LW_NULL;
    UINT32              ui32Ip    = INADDR_ANY;
    UINT16              usPort    = 0;
    INT                 iArgPos   = 1;
    INT                 i;
    INT                 iPid;
    struct sched_param  schedparam;
    posix_spawnattr_t   spawnattr;
    posix_spawnopt_t    spawnopt  = {0};
    PCHAR               pcSerial  = LW_NULL;
    LW_OBJECT_HANDLE    ulSelf    = API_ThreadIdSelf();
    
#if LW_CFG_SMP_EN > 0
    CHAR                cValue[10];
    cpu_set_t           cpuset;
#endif                                                                  /* #if LW_CFG_SMP_EN > 0        */

    if (argc < 3) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "Parameter error.\r\n");
        _ErrorHandle(ERROR_GDB_PARAM);
        return  (PX_ERROR);
    }

    pparam = (LW_GDB_PARAM *)LW_GDB_SAFEMALLOC(sizeof(LW_GDB_PARAM));
    if (LW_NULL == pparam) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "Malloc mem error.\r\n");
        _ErrorHandle(ENOMEM);
        return  (PX_ERROR);
    }
    lib_bzero(pparam, sizeof(LW_GDB_PARAM));

    pparam->GDB_bTerminal     = LW_FALSE;
    pparam->GDB_iCommFd       = PX_ERROR;
    pparam->GDB_iCommListenFd = PX_ERROR;
    pparam->GDB_iSigFd        = PX_ERROR;

    if (lib_strcmp(argv[iArgPos], "--attach") == 0) {                   /* ���뵽����ִ�еĽ��̵���     */
        iArgPos++;
        pparam->GDB_bAttached = LW_TRUE;
        
        if (argc < 4) {
            _DebugHandle(__ERRORMESSAGE_LEVEL, "Parameter error.\r\n");
            _ErrorHandle(ERROR_GDB_PARAM);
            LW_GDB_SAFEFREE(pparam);
            return  (PX_ERROR);
        }
    }

    pparam->GDB_byCommType = COMM_TYPE_TTY;
    for (i = 0; argv[iArgPos][i] != '\0'; i++) {                        /* ����ip�Ͷ˿�                 */
        if (argv[iArgPos][i] == ':') {
            argv[iArgPos][i] = '\0';
            sscanf(argv[iArgPos] + i + 1, "%hu", &usPort);
            if (lib_strcmp(argv[iArgPos], "localhost") != 0) {
                ui32Ip = inet_addr(argv[iArgPos]);
                if (IPADDR_NONE == ui32Ip) {
                    ui32Ip = INADDR_ANY;
                }
            } else {
                ui32Ip = INADDR_ANY;
            }
            pparam->GDB_byCommType = COMM_TYPE_TCP;
        }
    }
    
    if (pparam->GDB_byCommType == COMM_TYPE_TTY) {                      /* ���ڵ���                     */
        pcSerial = argv[iArgPos];
    }

    iArgPos++;

    if ((pparam->GDB_iSigFd = gdbSigInit()) < 0) {                      /* ��ʼ�������ź�               */
        gdbRelease(pparam);
        _DebugHandle(__ERRORMESSAGE_LEVEL, "Initialize signal failed.\r\n");
        _ErrorHandle(ERROR_GDB_INIT_SOCK);
        return  (PX_ERROR);
    }

    pparam->GDB_pvDtrace = API_DtraceCreate(LW_DTRACE_PROCESS,
                                            LW_DTRACE_F_DEF, ulSelf);   /*  ����dtrace����              */
    if (pparam->GDB_pvDtrace == NULL) {
        gdbRelease(pparam);
        _DebugHandle(__ERRORMESSAGE_LEVEL, "Create dtrace object error.\r\n");
        _ErrorHandle(ERROR_GDB_PARAM);
        return  (PX_ERROR);
    }
    
#if LW_CFG_SMP_EN > 0
    CPU_ZERO(&cpuset);
    if (API_TShellVarGetRt("DEBUG_CPU", cValue, sizeof(cValue)) > 0) {
        INT    iCpu = lib_atoi(cValue);
        if ((iCpu >= 0) && (iCpu < LW_NCPUS)) {
            CPU_SET(iCpu, &cpuset);
            pparam->GDB_bLockCpu = LW_TRUE;
        }
    }
#endif                                                                  /* LW_CFG_SMP_EN > 0            */

    if (pparam->GDB_bAttached) {                                        /* attach�����н���             */
        if (sscanf(argv[iArgPos], "%d", &iPid) != 1) {
            gdbRelease(pparam);
            _DebugHandle(__ERRORMESSAGE_LEVEL, "Parameter error.\r\n");
            _ErrorHandle(ERROR_GDB_PARAM);
            return  (PX_ERROR);
        }

        pparam->GDB_iPid = iPid;
        if (API_DtraceSetPid(pparam->GDB_pvDtrace, iPid) == PX_ERROR) { /* ��������                     */
            gdbRelease(pparam);
            _DebugHandle(__ERRORMESSAGE_LEVEL, "Attach to program failed.\r\n");
            _ErrorHandle(ERROR_GDB_ATTACH_PROG);
            return  (PX_ERROR);
        }

        if (API_DtraceStopProcess(pparam->GDB_pvDtrace) == PX_ERROR) {  /* ֹͣ����                     */
            _DebugHandle(__ERRORMESSAGE_LEVEL, "Stop program failed.\r\n");
            _ErrorHandle(ERROR_GDB_ATTACH_PROG);
            return  (PX_ERROR);
        }
    
    } else {                                                            /* ��������attach             */
        posix_spawnattr_init(&spawnattr);
        
        spawnopt.SPO_iSigNo       = SIGUSR1;
        spawnopt.SPO_ulId         = ulSelf;
        spawnopt.SPO_ulMainOption = LW_OPTION_DEFAULT;
        spawnopt.SPO_stStackSize  = 0;
        posix_spawnattr_setopt(&spawnattr, &spawnopt);
        
        schedparam.sched_priority = PX_PRIORITY_CONVERT(LW_PRIO_NORMAL);/*  �½��������ȼ�Ϊ NORMAL     */
        posix_spawnattr_setschedparam(&spawnattr, &schedparam);
        posix_spawnattr_setflags(&spawnattr, POSIX_SPAWN_SETSCHEDPARAM);
        
        if (posix_spawnp(&pparam->GDB_iPid, argv[iArgPos], NULL,
                         &spawnattr, &argv[iArgPos], NULL) != ERROR_NONE) {
            posix_spawnattr_destroy(&spawnattr);
            gdbRelease(pparam);
            _DebugHandle(__ERRORMESSAGE_LEVEL, "Start program failed.\r\n");
            _ErrorHandle(ERROR_GDB_ATTACH_PROG);
            return  (PX_ERROR);
        
        } else {
            posix_spawnattr_destroy(&spawnattr);
        }

        if (API_DtraceSetPid(pparam->GDB_pvDtrace,
                             pparam->GDB_iPid) == PX_ERROR) {           /* ��������                     */
            gdbRelease(pparam);
            _DebugHandle(__ERRORMESSAGE_LEVEL, "Attach to program failed.\r\n");
            _ErrorHandle(ERROR_GDB_ATTACH_PROG);
            return  (PX_ERROR);
        }

        if (gdbWaitSpawmSig(spawnopt.SPO_iSigNo) == PX_ERROR) {         /* �ȴ����ؾ����ź�             */
            gdbRelease(pparam);
            _DebugHandle(__ERRORMESSAGE_LEVEL, "Load program failed.\r\n");
            return  (PX_ERROR);
        }
    }
    
#if LW_CFG_SMP_EN > 0
    if (pparam->GDB_bLockCpu) {
        sched_setaffinity(pparam->GDB_iPid, sizeof(cpu_set_t), &cpuset);
        API_ThreadSetAffinity(ulSelf, sizeof(cpu_set_t), &cpuset);
    }
#endif                                                                  /* LW_CFG_SMP_EN > 0            */

    if (vprocGetPath(pparam->GDB_iPid, 
                     pparam->GDB_cProgPath, 
                     MAX_FILENAME_LENGTH)) {
        gdbRelease(pparam);
        _DebugHandle(__ERRORMESSAGE_LEVEL, "Get process path fail.\r\n");
        return  (PX_ERROR);
    }

    if (API_ThreadSetNotePad(ulSelf, 0, (ULONG)pparam) != ERROR_NONE) {
                                                                        /* ��������������߳�������   */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "Set thread notepad failed.\r\n");
        gdbRelease(pparam);
        return  (PX_ERROR);
    }

    if (signal(SIGKILL, gdbExit) == SIG_ERR) {                          /* ע��SIGKILL�źŴ�����      */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "Set SIGKILL handle failed.\r\n");
        gdbRelease(pparam);
        return  (PX_ERROR);
    }

#ifndef LW_DTRACE_HW_ISTEP
    if (API_AtomicInc(&GHookRefCnt) == 1) {                             /* ��һ�ε���ʱ���HOOK         */
        API_SystemHookAdd(API_DtraceSchedHook,
                          LW_OPTION_THREAD_SWAP_HOOK);                  /* ����߳��л�HOOK�����ڵ���   */
    }
#endif                                                                  /* !LW_DTRACE_HW_ISTEP          */

    if (pparam->GDB_byCommType == COMM_TYPE_TCP) {
        pparam->GDB_iCommFd = gdbTcpSockInit(pparam, ui32Ip, usPort);   /* ��ʼ��socket����ȡ���Ӿ��   */
    } else {
        pparam->GDB_iCommFd = gdbSerialInit(pparam, pcSerial);
    }
        
    if (pparam->GDB_iCommFd < 0) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "Initialize comunication failed.\r\n");
        _ErrorHandle(ERROR_GDB_INIT_SOCK);
        gdbExit(0);
        return  (PX_ERROR);
    }

    gdbEventLoop(pparam);                                               /* �����¼�ѭ��                 */

    gdbExit(0);                                                         /* ���ٶ���                     */

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: gdbInit
** ��������: ע�� GDBServer ����
** �䡡��  : NONE
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API 
VOID  API_GdbInit (VOID)
{
    API_TShellKeywordAddEx("debug", gdbMain, LW_OPTION_KEYWORD_SYNCBG);
    API_TShellFormatAdd("debug", " [connect options] [program] [arguments...]");
    API_TShellHelpAdd("debug",   "GDB Server (On serial: \"115200,n,8,1\")\n"
                                 "eg. debug localhost:1234 helloworld\n"
                                 "    debug /dev/ttyS1 helloworld\n"
                                 "    debug terminal helloworld\n"
                                 "    debug --attach localhost:1234 1\n");
}

#endif                                                                  /*  LW_CFG_GDB_EN > 0           */
/*********************************************************************************************************
  END
*********************************************************************************************************/
