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
** ��   ��   ��: lwip_tftp.c
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2009 �� 05 �� 29 ��
**
** ��        ��: tftp �ͻ���/������, ��Ҫ��������ṩϵͳ���о���.
**               tftp Э���Ϊ��, ����Դ����û�н����Ự��Ϊ����, ���ͻ�����������д������ļ���.

** BUG:
2009.06.04  ����һ����űȽϵ�����. ͬʱ�ͻ���ʧ��ʱ�������Ӧ errno.
2009.06.18  ���� TFTP ������Ŀ¼���� shell ����.
2009.06.19  ʹ�� GCC ������ʱ, ���� char ���� cFrameBuffer ��ʼ��ַ������, 
            ���� ntohs(*(UINT16 *)&cFrameBuffer[0]); ���ܲ�������������쳣.
            ���� __TFTP_HDR *hdr �����黺��ǿת, hdr = (__TFTP_HDR *)cFrameBuffer ��ô:
            ntohs(hdr->TFTPHDR_usOpc) ��һ�������������, (GCC �ڲ��д���)
2009.06.19  API_INetTftpServerPath() ���ټ��·����Ч��.
            ����������ͨ�ų�ʱ����.
2009.07.03  ������ GCC ����.
2009.09.25  ���� gethostbyname() ���ص�ַ���Ƶķ���.
2009.10.29  __inetTftpServerListen() ���� socket �쳣ʱ, ֱ���˳�.
2009.11.21  �� shell ftp ������ڱ�ģ����.
2011.02.21  ����ͷ�ļ�����, ���� POSIX ��׼.  
2011.03.11  ʹ�� getaddrinfo ������������.
2011.04.07  �������̴߳� /etc/services �л�ȡ ftp �ķ���˿�, ���û��, Ĭ��ʹ�� 69 �˿�.
*********************************************************************************************************/
#define  __SYLIXOS_STDIO
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
#include "../SylixOS/system/include/s_system.h"
#include "../SylixOS/net/include/net_net.h"
/*********************************************************************************************************
  �ü�����
*********************************************************************************************************/
#if (LW_CFG_NET_EN > 0) && (LW_CFG_NET_TFTP_EN > 0)
#include "netdb.h"
#include "arpa/inet.h"
#include "net/if.h"
#include "sys/socket.h"
#include "lwip_tftp.h"
/*********************************************************************************************************
  ���Դ���
*********************************************************************************************************/
#define __LWIP_TFTP_RETRY_TIMES     3
/*********************************************************************************************************
  tftp �������ݰ�
*********************************************************************************************************/
const CHAR      _G_cTftpErrorMsg[8][16] = {
    {0x00, LWIP_TFTP_OPC_ERROR, 0x00, LWIP_TFTP_EC_NONE, 'n', 'o', 'n', 'e', 0},
    {0x00, LWIP_TFTP_OPC_ERROR, 0x00, LWIP_TFTP_EC_NOFILE, 'n', 'o', 'f', 'i', 'l', 'e', 0},
    {0x00, LWIP_TFTP_OPC_ERROR, 0x00, LWIP_TFTP_EC_NOACCESS, 'n', 'o', 'a', 'c', 'c', 'e', 's', 's', 0},
    {0x00, LWIP_TFTP_OPC_ERROR, 0x00, LWIP_TFTP_EC_DISKFULL, 'd', 'i', 's', 'k', 'f', 'u', 'l', 'l', 0},
    {0x00, LWIP_TFTP_OPC_ERROR, 0x00, LWIP_TFTP_EC_TFTP, 't', 'f', 't', 'p', 0},
    {0x00, LWIP_TFTP_OPC_ERROR, 0x00, LWIP_TFTP_EC_ID, 'i', 'd', 0},
    {0x00, LWIP_TFTP_OPC_ERROR, 0x00, LWIP_TFTP_EC_FILEEXIST, 'e', 'x', 'i', 's', 't', 0},
    {0x00, LWIP_TFTP_OPC_ERROR, 0x00, LWIP_TFTP_EC_NOUSER, 'n', 'o', 'u', 's', 'e', 'r', 0},
};
/*********************************************************************************************************
  tftp ���ݰ�
*********************************************************************************************************/
PACK_STRUCT_BEGIN
struct __tftp_hdr {
    PACK_STRUCT_FIELD(UINT16    TFTPHDR_usOpc);                         /*  ������                      */
    PACK_STRUCT_FIELD(UINT16    TFTPHDR_usSeq);                         /*  ���                        */
} PACK_STRUCT_STRUCT;
PACK_STRUCT_END
typedef struct __tftp_hdr       __TFTP_HDR;                             /*  TFTP ���ݰ�ͷ               */
typedef struct __tftp_hdr      *__PTFTP_HDR;
/*********************************************************************************************************
  ȫ�ֱ���
*********************************************************************************************************/
static PCHAR    _G_pcTftpRootPath = LW_NULL;
static INT      _G_iListenSocket  = PX_ERROR;
/*********************************************************************************************************
  ��������
*********************************************************************************************************/
#if LW_CFG_SHELL_EN > 0
static INT  __tshellNetTftpdPath(INT  iArgC, PCHAR  *ppcArgV);
static INT  __tshellTftp(INT  iArgC, PCHAR  *ppcArgV);
#endif                                                                  /*  LW_CFG_SHELL_EN > 0         */
/*********************************************************************************************************
** ��������: __inetTftpParseReqFrame
** ��������: ����һ��������
** �䡡��  : ptftphdr          ���ݰ�ͷ
**           piReqType         ��������
**           pcFrameBuffer     ���ݰ�����
**           pcFileName        �ļ���
**           pcMode            ����ģʽ
** �䡡��  : ERROR
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static INT  __inetTftpParseReqFrame (__PTFTP_HDR    ptftphdr,
                                     INT           *piReqType,
                                     PCHAR         *ppcFileName,
                                     PCHAR         *ppcMode)
{
    UINT16      usOpc = ntohs(ptftphdr->TFTPHDR_usOpc);                 /*  ������                      */
    size_t      stFileNameLen;
    PCHAR       pcTemp;
    
    if ((usOpc != LWIP_TFTP_OPC_RDREQ) &&
        (usOpc != LWIP_TFTP_OPC_WRREQ)) {
        return  (PX_ERROR);
    }
    
    *ppcFileName  = (PCHAR)(&ptftphdr->TFTPHDR_usSeq);                  /*  �� seq λ�ÿ�ʼ             */
    stFileNameLen = lib_strnlen(*ppcFileName, PATH_MAX);
    pcTemp        = (*ppcFileName) + stFileNameLen;
    *pcTemp++     = PX_EOS;
    *ppcMode      = pcTemp;                                             /*  ģʽ                        */
    
    *piReqType    = (INT)usOpc;
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: __inetTftpMakeReqFrame
** ��������: ����һ��������
** �䡡��  : iReqType          ��������
**           ptftphdr          tftp ������
**           pcFileName        �ļ���
**           pcMode            ����ģʽ
** �䡡��  : ���ݰ�����
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static INT  __inetTftpMakeReqFrame (INT             iReqType, 
                                    __PTFTP_HDR     ptftphdr,
                                    CPCHAR          pcFileName, 
                                    CPCHAR          pcMode)
{
    REGISTER INT   iIndex;
    REGISTER INT   iLen;
    REGISTER PCHAR pcFileNameBuffer = (PCHAR)(&ptftphdr->TFTPHDR_usSeq);/*  �� seq λ�ÿ�ʼ             */

    ptftphdr->TFTPHDR_usOpc = htons((UINT16)iReqType);                  /*  ��д��������                */

    iIndex  = sprintf(pcFileNameBuffer, "%s", pcFileName);              /*  ��д�ļ���                  */
    pcFileNameBuffer[iIndex++] = PX_EOS;
    
    iLen    = iIndex + 2;                                               /*  �����ͷ����                */
    iLen   += sprintf(&pcFileNameBuffer[iIndex], "%s", pcMode);         /*  ��д��������                */
    
    return  (iLen + 1);                                                 /*  include last '\0'           */
}
/*********************************************************************************************************
** ��������: __inetTftpSendReqFrame
** ��������: ����һ���������ݰ�
** �䡡��  : iSock                 �׽���
**           psockaddrinRemote     Զ�̵�ַ
**           iReqType              ��������
**           pcFileName            �ļ���
** �䡡��  : ERROR
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static INT  __inetTftpSendReqFrame (INT                  iSock, 
                                    struct sockaddr_in  *psockaddrinRemote, 
                                    INT                  iReqType, 
                                    CPCHAR               pcFileName)
{
    CHAR                cSendBuffer[MAX_FILENAME_LENGTH + 8];
    __PTFTP_HDR         ptftphdr;
    
    REGISTER INT        iDataLen;
    REGISTER ssize_t    sstSend;
    
    ptftphdr = (__PTFTP_HDR)cSendBuffer;                                /*  tftp ��ͷλ��               */
                                                                        /*  �Զ�������ʽ�����ļ�        */
    iDataLen = __inetTftpMakeReqFrame(iReqType, ptftphdr, pcFileName, "octet");
    
    sstSend = sendto(iSock, (const void *)cSendBuffer, iDataLen, 0,     /*  �����������ݰ�              */
                     (struct sockaddr *)psockaddrinRemote, sizeof(struct sockaddr_in));
    return  ((INT)sstSend);
}
/*********************************************************************************************************
** ��������: __inetTftpSendAckFrame
** ��������: ����һ��ȷ�����ݰ�
** �䡡��  : iSock               �׽���
**           psockaddrinRemote   Զ�̵�ַ
**           usAck               ACK ��
** �䡡��  : ERROR
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static INT  __inetTftpSendAckFrame (INT  iSock, struct sockaddr_in  *psockaddrinRemote, UINT16  usAck)
{
    __TFTP_HDR          tftphdr;
    REGISTER ssize_t    sstSend;
    
    tftphdr.TFTPHDR_usOpc = htons(LWIP_TFTP_OPC_ACK);
    tftphdr.TFTPHDR_usSeq = htons(usAck);
    
    sstSend = sendto(iSock, (const void *)&tftphdr, 4, 0,               /*  ����ȷ�����ݰ�              */
                   (struct sockaddr *)psockaddrinRemote, sizeof(struct sockaddr_in));
    return  ((INT)sstSend);
}
/*********************************************************************************************************
** ��������: __inetTftpSendDataFrame
** ��������: ����һ���ļ����ݰ�
** �䡡��  : iSock               �׽���
**           psockaddrinRemote   Զ�̵�ַ
**           usSeq               SEQ ��
**           ptftphdr            tftp ���ݰ�ͷ
**           iDataLen            (���ݳ���) ���С�� 512 ��ʾ�ļ�ĩβ
** �䡡��  : ERROR
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static INT  __inetTftpSendDataFrame (INT                  iSock, 
                                     struct sockaddr_in  *psockaddrinRemote, 
                                     UINT16               usSeq,
                                     __PTFTP_HDR          ptftphdr,
                                     INT                  iDataLen)
{
    REGISTER ssize_t  sstSend;
    
    ptftphdr->TFTPHDR_usOpc = htons(LWIP_TFTP_OPC_DATA);
    ptftphdr->TFTPHDR_usSeq = htons(usSeq);
    
    iDataLen += LWIP_TFTP_DATA_HEADERLEN;                               /*  �����ͷ����                */
    
    sstSend = sendto(iSock, (const void *)ptftphdr, iDataLen, 0,        /*  �����ļ����ݰ�              */
                   (struct sockaddr *)psockaddrinRemote, sizeof(struct sockaddr_in));
    return  ((INT)sstSend);
}
/*********************************************************************************************************
** ��������: __inetTftpSendErrorFrame
** ��������: ����һ��������Ϣ���ݰ�
** �䡡��  : iSock               �׽���
**           psockaddrinRemote   Զ�̵�ַ
**           usErrorCode         ������
** �䡡��  : ERROR
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static INT  __inetTftpSendErrorFrame (INT                  iSock, 
                                      struct sockaddr_in  *psockaddrinRemote, 
                                      UINT16               usErrorCode)
{
    REGISTER ssize_t    sstSend;
    REGISTER size_t     stDataLen = lib_strlen(&_G_cTftpErrorMsg[usErrorCode][4]);
    
    stDataLen += 5;                                                     /*  include '\0'                */
    
    sstSend = sendto(iSock, (const void *)_G_cTftpErrorMsg[usErrorCode], 
                     stDataLen, 0,                                      /*  �����ļ����ݰ�              */
                     (struct sockaddr *)psockaddrinRemote, sizeof(struct sockaddr_in));
    return  ((INT)sstSend);
}
/*********************************************************************************************************
** ��������: __inetTftpRecvFrame
** ��������: ����һ�����ݰ�
** �䡡��  : iSock             �׽���
**           psockaddrinRemote Զ�̵�ַ
**           pcRecvBuffer      �������ݰ�������
** �䡡��  : ERROR
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static INT  __inetTftpRecvFrame (INT                  iSock, 
                                 struct sockaddr_in  *psockaddrinRemote, 
                                 PCHAR                pcRecvBuffer, 
                                 INT                  iBufferLen)
{
    struct sockaddr_in  sockaddrinRecv;
    
    REGISTER INT        i;
    REGISTER ssize_t    sstRecv;
             socklen_t  uiAddrLen = sizeof(struct sockaddr_in);
    
    for (i = 0; i < __LWIP_TFTP_RETRY_TIMES; i++) {
        sstRecv = recvfrom(iSock, (void *)pcRecvBuffer, iBufferLen, 0,  /*  ��������                    */
                         (struct sockaddr *)&sockaddrinRecv, &uiAddrLen);
        if (sstRecv > 0) {
            if (lib_memcmp((PVOID)&sockaddrinRecv.sin_addr, 
                           (PVOID)&psockaddrinRemote->sin_addr, 
                           sizeof(struct in_addr))) {                   /*  IP ��ַ��������ַ��ͬ, ���� */
                continue;
            } else {
                psockaddrinRemote->sin_port = sockaddrinRecv.sin_port;  /*  ���¶˿�                    */
            }
        }
        return  ((INT)sstRecv);
    }
    
    return  (PX_ERROR);
}
/*********************************************************************************************************
** ��������: __inetTftpReceive
** ��������: ͨ�� tftp Э���Զ�̼�������ָ���ļ�. (Э�鴦��)
** �䡡��  : iSock      �׽���
**           iFd        �ļ�������
**           pinaddr    Զ�̵�ַ
**           pcFileName �ļ���
** �䡡��  : ERROR
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static INT  __inetTftpReceive (INT  iSock, INT  iFd, struct in_addr  *pinaddr, CPCHAR  pcFileName)
{
#define __LWIP_TFTP_RECVBUFFER_SIZE     (512 + 4)

    struct sockaddr_in  sockaddrinRemote;
    __PTFTP_HDR         ptftphdr;                                       /*  tftp ͷ                     */
             PCHAR      pcFileData;
    
             INT        iRecvError = 0;                                 /*  �������մ���                */
             ssize_t    sstWriteNum;                                    /*  д���ļ��Ĵ�С              */
    
    REGISTER INT        iSend;
    REGISTER INT        iRecv;
             
             UINT16     usOpc;
             UINT16     usSeq = 0;                                      /*  ���յ����к�                */
             UINT16     usAck = 0;                                      /*  �ظ������ݰ�                */

             CHAR       cRecvBuffer[__LWIP_TFTP_RECVBUFFER_SIZE];       /*  ���ջ���                    */
             
             INT        iTempError;
    
    ptftphdr   = (__PTFTP_HDR)cRecvBuffer;
    pcFileData = (cRecvBuffer + LWIP_TFTP_DATA_HEADERLEN);              /*  ����֡����λ��              */
    
    sockaddrinRemote.sin_len    = sizeof(struct sockaddr_in);
    sockaddrinRemote.sin_family = AF_INET;
    sockaddrinRemote.sin_port   = htons(69);
    sockaddrinRemote.sin_addr   = *pinaddr;
                                                                        /*  �����������ݰ�              */
    iSend = __inetTftpSendReqFrame(iSock, &sockaddrinRemote, LWIP_TFTP_OPC_RDREQ, pcFileName);
    if (iSend < 0) {
        return  (iSend);
    }
    
    while (iRecvError < __LWIP_TFTP_RETRY_TIMES) {                      /*  ��������С��ָ��ֵ          */
    
        iRecv = __inetTftpRecvFrame(iSock, 
                                    &sockaddrinRemote, 
                                    cRecvBuffer, 
                                    __LWIP_TFTP_RECVBUFFER_SIZE);       /*  �������ݰ�                  */
        if ((iRecv <= 0) && ((errno == ETIMEDOUT) || (errno == EWOULDBLOCK))) {
                                                                        /*  �޷���������������          */
            iRecvError++;
            goto    __error_handle;
        }
        
        if (iRecv < LWIP_TFTP_DATA_HEADERLEN) {
            return  (PX_ERROR);                                         /*  �޷��յ����ݰ�, ����        */
        }
        
        usOpc = ntohs(ptftphdr->TFTPHDR_usOpc);                         /*  �жϲ�����                  */
        if (usOpc != LWIP_TFTP_OPC_DATA) {                              /*  �޷���ȡ����                */
            _ErrorHandle(ECONNABORTED);                                 /*  ��ֹ                        */
            return  (PX_ERROR);
        }
        
        usSeq = ntohs(ptftphdr->TFTPHDR_usSeq);                         /*  �ж����к�                  */
        if (usSeq != (UINT16)(usAck + 1)) {                             /*  ��������Ҫ�����ݰ�          */
            iRecvError++;
            goto    __error_handle;
        }
        
        iRecvError = 0;                                                 /*  ����û�д���                */
        usAck      = usSeq;                                             /*  ����ȷ�Ϻ�                  */
        __inetTftpSendAckFrame(iSock, &sockaddrinRemote, usAck);        /*  �ظ�                        */
        
        iRecv -= LWIP_TFTP_DATA_HEADERLEN;                              /*  �������ݸ��ش�С            */
        if (iRecv <= 0) {
            return  (ERROR_NONE);                                       /*  ���ݽ������                */
        } else {
            sstWriteNum = write(iFd, pcFileData, iRecv);
            if (sstWriteNum < iRecv) {
                __inetTftpSendErrorFrame(iSock, &sockaddrinRemote, LWIP_TFTP_EC_DISKFULL);
                return  (PX_ERROR);                                     /*  �޷�������д���ļ�          */
            }
            if (iRecv < 512) {
                return  (ERROR_NONE);                                   /*  ���ݽ������                */
            }
        }
        continue;                                                       /*  ������������                */
        
__error_handle:
        iTempError = errno;
        if (usAck == 0) {                                               /*  ��Ҫ���·��������          */
            __inetTftpSendReqFrame(iSock, &sockaddrinRemote, 
                                   LWIP_TFTP_OPC_RDREQ, pcFileName);
        } else {                                                        /*  �ٴη���ȷ�����ݰ�          */
            __inetTftpSendAckFrame(iSock, &sockaddrinRemote, usAck);
        }
        errno = iTempError;
    }

    if (errno == 0) {
        _ErrorHandle(EHOSTDOWN);                                        /*  ��������                    */
    }
    return  (PX_ERROR);                                                 /*  ���ݰ�ʧ��                  */
}
/*********************************************************************************************************
** ��������: __inetTftpSend
** ��������: ͨ�� tftp Э����Զ�̼��������ָ���ļ�. (Э�鴦��)
** �䡡��  : iSock      �׽���
**           iFd        �ļ�������
**           pinaddr    Զ�̵�ַ
**           pcFileName �ļ���
** �䡡��  : ERROR
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static INT  __inetTftpSend (INT  iSock, INT  iFd, struct in_addr  *pinaddr, CPCHAR  pcFileName)
{
#define __LWIP_TFTP_SENDBUFFER_SIZE     (512 + 4)
#define __LWIP_TFTP_READFILE_SIZE       512

    struct sockaddr_in  sockaddrinRemote;
    
    __PTFTP_HDR         ptftphdrRecv;                                   /*  tftp ͷ                     */
    __PTFTP_HDR         ptftphdrSend;
             PCHAR      pcFileData;
             
             INT        iRecvError = 0;                                 /*  �������մ���                */
             ssize_t    sstReadNum = 0;                                 /*  ��ȡ�ļ�����                */
             
             INT        iSendOver = 0;
    
    REGISTER INT        iSend;
    REGISTER INT        iRecv;
             
             UINT16     usOpc;
             UINT16     usSeq = 0;                                      /*  ���͵����к�                */
             UINT16     usAck = 0;                                      /*  ���յ�ȷ�Ϻ�                */

             CHAR       cRecvBuffer[4];                                 /*  ���ջ���                    */
             CHAR       cSendBuffer[__LWIP_TFTP_SENDBUFFER_SIZE];       /*  ���ͻ���                    */
             
             INT        iTempError;
             
    ptftphdrRecv = (__PTFTP_HDR)cRecvBuffer;
    ptftphdrSend = (__PTFTP_HDR)cSendBuffer;
    pcFileData   = (cSendBuffer + LWIP_TFTP_DATA_HEADERLEN);            /*  ����֡����λ��              */
    
    sockaddrinRemote.sin_len    = sizeof(struct sockaddr_in);
    sockaddrinRemote.sin_family = AF_INET;
    sockaddrinRemote.sin_port   = htons(69);
    sockaddrinRemote.sin_addr   = *pinaddr;
                                                                        /*  �����������ݰ�              */
    iSend = __inetTftpSendReqFrame(iSock, &sockaddrinRemote, LWIP_TFTP_OPC_WRREQ, pcFileName);
    if (iSend < 0) {
        return  (iSend);
    }
    
    while (iRecvError < __LWIP_TFTP_RETRY_TIMES) {                      /*  ��������С��ָ��ֵ          */
    
        iRecv = __inetTftpRecvFrame(iSock, 
                                    &sockaddrinRemote, 
                                    cRecvBuffer, 
                                    4);                                 /*  �������ݰ�                  */
        if ((iRecv <= 0) && ((errno == ETIMEDOUT) || (errno == EWOULDBLOCK))) {
                                                                        /*  �޷���������������          */
            iRecvError++;
            goto    __error_handle;
        }
        
        if (iRecv < 4) {
            return  (PX_ERROR);                                         /*  �޷��յ����ݰ�, ����        */
        }
        
        usOpc = ntohs(ptftphdrRecv->TFTPHDR_usOpc);                     /*  �жϲ�����                  */
        if (usOpc != LWIP_TFTP_OPC_ACK) {                               /*  �޷���ȡ����                */
            _ErrorHandle(ECONNABORTED);                                 /*  ��ֹ                        */
            return  (PX_ERROR);
        }
        
        usAck = ntohs(ptftphdrRecv->TFTPHDR_usSeq);                     /*  �ж�ȷ�Ϻ�                  */
        if (usAck != usSeq) {                                           /*  ��������Ҫ��ȷ�ϰ�          */
            iRecvError++;
            goto    __error_handle;
        }
        
        if (iSendOver) {                                                /*  �Ѿ����������һ�����ݰ�    */
            return  (ERROR_NONE);
        }
        
        usSeq++;                                                        /*  ׼��������һ������          */
        sstReadNum = read(iFd, pcFileData, __LWIP_TFTP_READFILE_SIZE);  /*  ���ļ���ȡ����              */
        if (sstReadNum < 0) {
            __inetTftpSendErrorFrame(iSock, &sockaddrinRemote, LWIP_TFTP_EC_NOACCESS);
            return  (PX_ERROR);                                         /*  �޷���ȡ�����ļ�            */
        } else {
            __inetTftpSendDataFrame(iSock, &sockaddrinRemote, usSeq,
                                    ptftphdrSend, (INT)sstReadNum);     /*  �������ݰ�                  */
            if (sstReadNum < __LWIP_TFTP_READFILE_SIZE) {
                iSendOver = 1;                                          /*  �������һ�����ݰ�          */
            }
        }
        continue;                                                       /*  ������������                */
        
__error_handle:
        iTempError = errno;
        if (usSeq == 0) {                                               /*  ��û�з���һ�����ݰ�        */
            __inetTftpSendReqFrame(iSock, &sockaddrinRemote, LWIP_TFTP_OPC_WRREQ, pcFileName);
        } else {                                                        /*  ��Ҫ�ط�����                */
            __inetTftpSendDataFrame(iSock, &sockaddrinRemote, usSeq,
                                    ptftphdrSend, (INT)sstReadNum);     /*  �������ݰ�                  */
        }
        errno = iTempError;
    }
    
    if (errno == 0) {
        _ErrorHandle(EHOSTDOWN);                                        /*  ��������                    */
    }
    return  (PX_ERROR);
}
/*********************************************************************************************************
** ��������: API_INetTftpReceive
** ��������: ͨ�� tftp Э���Զ�̼�������ָ���ļ�.
** �䡡��  : pcRemoteHost      Զ������
**           pcFileName        ��Ҫ��ȡ���ļ��ļ���
**           pcLocalFileName   ת���뱾�ص��ļ���
** �䡡��  : ERROR
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
INT  API_INetTftpReceive (CPCHAR  pcRemoteHost, CPCHAR  pcFileName, CPCHAR  pcLocalFileName)
{
    struct addrinfo      hints;
    struct addrinfo     *phints = LW_NULL;

    struct in_addr  inaddrRemote;
    INT             iTimeout = LWIP_TFTP_TIMEOUT;
    
    INT             iDestFd;
    INT             iSock;
    
    INT             iError;
    ULONG           ulError;
    
    if (!pcRemoteHost || !pcFileName || !pcLocalFileName) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    if (lib_strnlen(pcFileName, MAX_FILENAME_LENGTH) >= MAX_FILENAME_LENGTH) {
        _ErrorHandle(ERROR_IO_NAME_TOO_LONG);
        return  (PX_ERROR);
    }
    
    if (!inet_aton(pcRemoteHost, &inaddrRemote)) {
        hints.ai_family = AF_UNSPEC;
        hints.ai_flags  = AI_CANONNAME;
        getaddrinfo(pcRemoteHost, LW_NULL, &hints, &phints);            /*  ��������                    */
        if (phints == LW_NULL) {
            _ErrorHandle(EHOSTUNREACH);
            return  (PX_ERROR);
        } else {
            if (phints->ai_addr->sa_family == AF_INET) {                /*  ��������ַ                */
                inaddrRemote = ((struct sockaddr_in *)(phints->ai_addr))->sin_addr;
                freeaddrinfo(phints);
            } else {
                freeaddrinfo(phints);
                _ErrorHandle(EAFNOSUPPORT);
                return  (PX_ERROR);
            }
        }
    }
                                                                        /*  ��Ŀ���ļ�                */
    iDestFd = open(pcLocalFileName, (O_WRONLY | O_CREAT | O_TRUNC), DEFAULT_FILE_PERM);
    if (iDestFd < 0) {
        return  (PX_ERROR);
    }
    
    iSock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);                   /*  ���� socket                 */
    if (iSock < 0) {
        close(iDestFd);                                                 /*  �ر��ļ�                    */
        unlink(pcLocalFileName);                                        /*  ɾ���ļ�                    */
        return  (PX_ERROR);
    }                                                                   /*  ���ó�ʱʱ��                */
    setsockopt(iSock, SOL_SOCKET, SO_RCVTIMEO, (const void *)&iTimeout, sizeof(INT));
    
    iError = __inetTftpReceive(iSock, iDestFd, 
                               &inaddrRemote, pcFileName);              /*  ��ʼ�����ļ�                */
    ulError = errno;
    
    close(iDestFd);                                                     /*  �ر��ļ�                    */
    close(iSock);                                                       /*  �ر� socket                 */
    
    if (iError < 0) {
        unlink(pcLocalFileName);                                        /*  ɾ���ļ�                    */
        _ErrorHandle(ulError);
        return  (iError);
    } else {
        return  (ERROR_NONE);
    }
}
/*********************************************************************************************************
** ��������: API_INetTftpSend
** ��������: ͨ�� tftp Э����Զ�̼��������ָ���ļ�.
** �䡡��  : pcRemoteHost      Զ������
**           pcFileName        ����������˵��ļ���
**           pcLocalFileName   �����ļ���
** �䡡��  : ERROR
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
INT  API_INetTftpSend (CPCHAR  pcRemoteHost, CPCHAR  pcFileName, CPCHAR  pcLocalFileName)
{
    struct addrinfo      hints;
    struct addrinfo     *phints = LW_NULL;

    struct in_addr  inaddrRemote;
    INT             iTimeout = LWIP_TFTP_TIMEOUT;
    
    INT             iSrcFd;
    INT             iSock;
    
    INT             iError;
    ULONG           ulError;
    
    if (!pcRemoteHost || !pcFileName || !pcLocalFileName) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    if (lib_strnlen(pcFileName, MAX_FILENAME_LENGTH) >= MAX_FILENAME_LENGTH) {
        _ErrorHandle(ERROR_IO_NAME_TOO_LONG);
        return  (PX_ERROR);
    }
    
    if (!inet_aton(pcRemoteHost, &inaddrRemote)) {
        hints.ai_family = AF_UNSPEC;
        hints.ai_flags  = AI_CANONNAME;
        getaddrinfo(pcRemoteHost, LW_NULL, &hints, &phints);            /*  ��������                    */
        if (phints == LW_NULL) {
            _ErrorHandle(EHOSTUNREACH);
            return  (PX_ERROR);
        } else {
            if (phints->ai_addr->sa_family == AF_INET) {                /*  ��������ַ                */
                inaddrRemote = ((struct sockaddr_in *)(phints->ai_addr))->sin_addr;
                freeaddrinfo(phints);
            } else {
                freeaddrinfo(phints);
                _ErrorHandle(EAFNOSUPPORT);
                return  (PX_ERROR);
            }
        }
    }
                                                                        /*  ��Դ�ļ�                  */
    iSrcFd = open(pcLocalFileName, O_RDONLY);
    if (iSrcFd < 0) {
        return  (PX_ERROR);
    }
    
    iSock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);                   /*  ���� socket                 */
    if (iSock < 0) {
        close(iSrcFd);                                                  /*  �ر��ļ�                    */
        return  (PX_ERROR);
    }                                                                   /*  ���ó�ʱʱ��                */
    setsockopt(iSock, SOL_SOCKET, SO_RCVTIMEO, (const void *)&iTimeout, sizeof(INT));
    
    iError = __inetTftpSend(iSock, iSrcFd, 
                            &inaddrRemote, pcFileName);                 /*  ��ʼ�����ļ�                */
                            
    ulError = errno;
    
    close(iSrcFd);                                                      /*  �ر��ļ�                    */
    close(iSock);                                                       /*  �ر� socket                 */
    
    if (iError < 0) {
        _ErrorHandle(ulError);
        return  (iError);
    } else {
        return  (ERROR_NONE);
    }
}
/*********************************************************************************************************
  tftp ��������
*********************************************************************************************************/
typedef struct {
    INT                     TFTPSA_iReqType;                            /*  ����ʽ                    */
    INT                     TFTPSA_iFd;                                 /*  �ļ�������                  */
    INT                     TFTPSA_iSock;                               /*  ͨ�Žӿ��ļ�������          */
    struct sockaddr_in      TFTPSA_sockaddrinRemote;                    /*  Զ��������ַ                */
    CHAR                    TFTPSA_cFileName[1];                        /*  �ļ���                      */
} __LW_TFTP_SERVERARG;
typedef __LW_TFTP_SERVERARG     *__PLW_TFTP_SERVERARG;
/*********************************************************************************************************
** ��������: __inetTftpServerCleanup
** ��������: ��� tftp �����������ڴ�
** �䡡��  : ptftpsa       ����������
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static VOID  __inetTftpServerCleanup (__PLW_TFTP_SERVERARG  ptftpsa)
{
    if (ptftpsa) {
        close(ptftpsa->TFTPSA_iSock);                                   /*  �ر���ʱͨ�Ŷ˿�            */
        __SHEAP_FREE(ptftpsa);                                          /*  �ͷ��ڴ�                    */
    }
}
/*********************************************************************************************************
** ��������: __inetTftpRdServer
** ��������: tftp �������߳� (��������)
** �䡡��  : ptftpsa       ����������
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static VOID  __inetTftpRdServer (__PLW_TFTP_SERVERARG   ptftpsa)
{
#define __LWIP_TFTP_SENDBUFFER_SIZE     (512 + 4)
#define __LWIP_TFTP_READFILE_SIZE       512

    __PTFTP_HDR         ptftphdrRecv;                                   /*  tftp ͷ                     */
    __PTFTP_HDR         ptftphdrSend;
             PCHAR      pcFileData;

             INT        iRecvError = 0;                                 /*  �������մ���                */
             ssize_t    sstReadNum;                                     /*  ��ȡ�ļ�����                */
             
             INT        iSendOver = 0;
    
    REGISTER INT        iRecv;
             
             UINT16     usOpc;
             UINT16     usSeq = 0;                                      /*  ���͵����к�                */
             UINT16     usAck = 0;                                      /*  ���յ�ȷ�Ϻ�                */

             CHAR       cRecvBuffer[4];                                 /*  ���ջ���                    */
             CHAR       cSendBuffer[__LWIP_TFTP_SENDBUFFER_SIZE];       /*  ���ͻ���                    */
             
    API_ThreadCleanupPush(__inetTftpServerCleanup, (PVOID)ptftpsa);
    
    ptftphdrRecv = (__PTFTP_HDR)cRecvBuffer;
    ptftphdrSend = (__PTFTP_HDR)cSendBuffer;
    pcFileData   = (cSendBuffer + LWIP_TFTP_DATA_HEADERLEN);            /*  ����֡����λ��              */
    
    while (iRecvError < __LWIP_TFTP_RETRY_TIMES) {
        
        usSeq++;                                                        /*  ׼��������һ������          */
        sstReadNum = read(ptftpsa->TFTPSA_iFd, pcFileData, 
                          __LWIP_TFTP_READFILE_SIZE);                   /*  ���ļ���ȡ����              */
        if (sstReadNum < 0) {
            __inetTftpSendErrorFrame(ptftpsa->TFTPSA_iSock, 
                                     &ptftpsa->TFTPSA_sockaddrinRemote, 
                                     LWIP_TFTP_EC_NOACCESS);
            break;
        } else {
            __inetTftpSendDataFrame(ptftpsa->TFTPSA_iSock, 
                                    &ptftpsa->TFTPSA_sockaddrinRemote, 
                                    usSeq,
                                    ptftphdrSend, 
                                    (INT)sstReadNum);                   /*  �������ݰ�                  */
            if (sstReadNum < __LWIP_TFTP_READFILE_SIZE) {
                iSendOver = 1;                                          /*  �������һ�����ݰ�          */
            }
        }
        
        iRecv = __inetTftpRecvFrame(ptftpsa->TFTPSA_iSock, 
                                    &ptftpsa->TFTPSA_sockaddrinRemote, 
                                    cRecvBuffer, 
                                    4);                                 /*  �������ݰ�                  */
        if ((iRecv <= 0) && ((errno == ETIMEDOUT) || (errno == EWOULDBLOCK))) {
                                                                        /*  �޷���������������          */
            iRecvError++;
            goto    __error_handle;
        }
        
        if (iRecv < 4) {
            break;                                                      /*  �޷��յ����ݰ�, ����        */
        }
        
        usOpc = ntohs(ptftphdrRecv->TFTPHDR_usOpc);                     /*  �жϲ�����                  */
        if (usOpc != LWIP_TFTP_OPC_ACK) {                               /*  �޷���ȡ����                */
            break;
        }
        
        usAck = ntohs(ptftphdrRecv->TFTPHDR_usSeq);                     /*  �ж�ȷ�Ϻ�                  */
        if (usAck != usSeq) {                                           /*  ��������Ҫ��ȷ�ϰ�          */
            iRecvError++;
            goto    __error_handle;
        }
        
        if (iSendOver) {                                                /*  �Ѿ����������һ�����ݰ�    */
            close(ptftpsa->TFTPSA_iFd);
            return;                                                     /*  ���ݷ������                */
        }
        continue;                                                       /*  ������������                */
        
        /*
         *  ע��: ������������˳�ʱ�ط�, ����Ϊ�˽�ʡ�ڴ�, ��û�в���Ԥ���ļ����ٻ���.
         */
__error_handle:
        __inetTftpSendDataFrame(ptftpsa->TFTPSA_iSock, 
                                &ptftpsa->TFTPSA_sockaddrinRemote, 
                                usSeq,
                                ptftphdrSend,
                                (INT)sstReadNum);                       /*  �������ݰ�                  */
    }
    
    close(ptftpsa->TFTPSA_iFd);                                         /*  �ر��ļ�                    */
}
/*********************************************************************************************************
** ��������: __inetTftpWrServer
** ��������: tftp �������߳� (д������)
** �䡡��  : ptftpsa       ����������
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static VOID  __inetTftpWrServer (__PLW_TFTP_SERVERARG   ptftpsa)
{
#define __LWIP_TFTP_RECVBUFFER_SIZE     (512 + 4)

    __PTFTP_HDR         ptftphdr;                                       /*  tftp ͷ                     */
             PCHAR      pcFileData;                                     /*  �ļ�����ָ��                */
             
             INT        iRecvError = 0;                                 /*  �������մ���                */
             ssize_t    sstWriteNum;                                    /*  д���ļ��Ĵ�С              */
             
    REGISTER INT        iRecv;
             
             UINT16     usOpc;
             UINT16     usSeq = 0;                                      /*  ���͵����к�                */
             UINT16     usAck = 0;                                      /*  ���յ�ȷ�Ϻ�                */
             
             CHAR       cRecvBuffer[__LWIP_TFTP_RECVBUFFER_SIZE];       /*  ���ջ���                    */
    
    API_ThreadCleanupPush(__inetTftpServerCleanup, (PVOID)ptftpsa);     /*  �߳��˳�����Ҫ��� ptftpsa  */
    
    ptftphdr   = (__PTFTP_HDR)cRecvBuffer;
    pcFileData = (cRecvBuffer + LWIP_TFTP_DATA_HEADERLEN);              /*  ����֡����λ��              */
    
    __inetTftpSendAckFrame(ptftpsa->TFTPSA_iSock, 
                           &ptftpsa->TFTPSA_sockaddrinRemote,
                           usAck);                                      /*  ���� ACK ����׼����������   */
    
    while (iRecvError < __LWIP_TFTP_RETRY_TIMES) {
    
        iRecv = __inetTftpRecvFrame(ptftpsa->TFTPSA_iSock, 
                                    &ptftpsa->TFTPSA_sockaddrinRemote, 
                                    cRecvBuffer, 
                                    __LWIP_TFTP_RECVBUFFER_SIZE);       /*  �������ݰ�                  */
        if ((iRecv <= 0) && ((errno == ETIMEDOUT) || (errno == EWOULDBLOCK))) {
                                                                        /*  �޷���������������          */
            iRecvError++;
            goto    __error_handle;
        }
        
        if (iRecv < LWIP_TFTP_DATA_HEADERLEN) {
            break;                                                      /*  �޷��յ����ݰ�, ����        */
        }
        
        usOpc = ntohs(ptftphdr->TFTPHDR_usOpc);                         /*  �жϲ�����                  */
        if (usOpc != LWIP_TFTP_OPC_DATA) {                              /*  �޷���ȡ����                */
            break;
        }
        
        usSeq = ntohs(ptftphdr->TFTPHDR_usSeq);                         /*  �ж����к�                  */
        if (usSeq != (UINT16)(usAck + 1)) {                             /*  ��������Ҫ�����ݰ�          */
            iRecvError++;
            goto    __error_handle;
        }
        
        iRecvError = 0;                                                 /*  ����û�д���                */
        usAck      = usSeq;                                             /*  ����ȷ�Ϻ�                  */
        __inetTftpSendAckFrame(ptftpsa->TFTPSA_iSock, 
                               &ptftpsa->TFTPSA_sockaddrinRemote, 
                               usAck);                                  /*  �ظ�                        */
        
        iRecv -= LWIP_TFTP_DATA_HEADERLEN;                              /*  �������ݸ��ش�С            */
        if (iRecv <= 0) {
            close(ptftpsa->TFTPSA_iFd);
            return;                                                     /*  ���ݽ������                */
        } else {
            sstWriteNum = write(ptftpsa->TFTPSA_iFd, pcFileData, iRecv);/*  д����                      */
            if (sstWriteNum < iRecv) {
                __inetTftpSendErrorFrame(ptftpsa->TFTPSA_iSock, 
                                         &ptftpsa->TFTPSA_sockaddrinRemote, 
                                         LWIP_TFTP_EC_DISKFULL);        /*  �޷�������д���ļ�          */
                break;
            }
            if (iRecv < 512) {
                close(ptftpsa->TFTPSA_iFd);
                return;                                                 /*  ���ݽ������                */
            }
        }
        continue;
        
__error_handle:
        __inetTftpSendAckFrame(ptftpsa->TFTPSA_iSock, 
                               &ptftpsa->TFTPSA_sockaddrinRemote, 
                               usAck);                                  /*  �ٴη���ȷ�����ݰ�          */
    }
    
    close(ptftpsa->TFTPSA_iFd);                                         /*  �ر������ļ�                */
    unlink(ptftpsa->TFTPSA_cFileName);                                  /*  ���ִ���, ��Ҫɾ���ļ�      */
}
/*********************************************************************************************************
** ��������: __inetTftpServerListen
** ��������: tftp ���������������߳�
** �䡡��  : NONE
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static VOID  __inetTftpServerListen (VOID)
{
    INT                     iSock;                                      /*  ������                      */
    INT                     iTempSock;                                  /*  ��ʱ����                    */
    INT                     iFd;
    
    INT                     iTimeout = LWIP_TFTP_TIMEOUT;               /*  ��ʱ���ӳ�ʱʱ��            */
    
    __PTFTP_HDR             ptftphdr;                                   /*  tftp ͷ                     */
    __PLW_TFTP_SERVERARG    ptftpsa;                                    /*  ����������                  */
    LW_CLASS_THREADATTR     threadattr;                                 /*  �������߳�����              */
    ULONG                   ulThreadHandle;
    
    struct sockaddr_in      sockaddrinLocal;
    struct sockaddr_in      sockaddrinRemote;
    socklen_t               uiAddrLen = sizeof(struct sockaddr_in);
    
    struct servent         *pservent;
    
    INT                     iErrLevel = 0;
    
    REGISTER ssize_t        sstRecv;
    CHAR                    cRecvBuffer[MAX_FILENAME_LENGTH + 8];       /*  ���ջ�����                  */
    CHAR                    cFullFileName[MAX_FILENAME_LENGTH];         /*  �����ļ���                  */
    
    INT                     iReqType;
    PCHAR                   pcFileName;
    PCHAR                   pcMode;
    
    ptftphdr = (__PTFTP_HDR)cRecvBuffer;
    
    API_ThreadAttrBuild(&threadattr,
                        LW_CFG_NET_TFTP_STK_SIZE,
                        LW_PRIO_T_SERVICE,
                        LW_OPTION_THREAD_STK_CHK | LW_OPTION_OBJECT_GLOBAL,
                        LW_NULL);                                       /*  �����߳����Կ�              */
    
    iSock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);                   /*  ���� socket                 */
    if (iSock < 0) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "can not create socket.\r\n");
        return;
    }
    
    sockaddrinLocal.sin_len         = sizeof(struct sockaddr_in);
    sockaddrinLocal.sin_family      = AF_INET;
    sockaddrinLocal.sin_addr.s_addr = INADDR_ANY;
    
    pservent = getservbyname("tftp", "udp");
    if (pservent) {
        sockaddrinLocal.sin_port = (u16_t)pservent->s_port;
    } else {
        sockaddrinLocal.sin_port = htons(69);                           /*  tftp default port           */
    }
                                                                        /*  �󶨷������˿�              */
    if (bind(iSock, (struct sockaddr *)&sockaddrinLocal, sizeof(struct sockaddr_in)) < 0) {
        close(iSock);
        _DebugHandle(__ERRORMESSAGE_LEVEL, "can not bind server port.\r\n");
        return;
    }
    
    _G_iListenSocket = iSock;                                           /*  ��¼���� socket             */

    for (;;) {
        sstRecv = recvfrom(iSock, (void *)cRecvBuffer, (MAX_FILENAME_LENGTH + 8), 0,
                           (struct sockaddr *)&sockaddrinRemote, &uiAddrLen);
        if (sstRecv <= 0) {
            if ((errno != ETIMEDOUT) && (errno != EWOULDBLOCK)) {       /*  69 socket error!            */
                close(iSock);
                _DebugHandle(__ERRORMESSAGE_LEVEL, "socket fd error.\r\n");
                return;
            }
            continue;
        }
        cRecvBuffer[sstRecv] = PX_EOS;
                                                                        /*  ����������                */
        if (__inetTftpParseReqFrame(ptftphdr, &iReqType, &pcFileName, &pcMode)) {
            __inetTftpSendErrorFrame(iSock, &sockaddrinRemote, LWIP_TFTP_EC_TFTP);
            continue;
        }
        
        if (lib_strcasecmp(pcMode, "octet")) {                          /*  ��֧�ֶ�����ģʽ            */
            __inetTftpSendErrorFrame(iSock, &sockaddrinRemote, LWIP_TFTP_EC_TFTP);
            continue;
        }
                                                                        /*  �����ļ���                  */
        snprintf(cFullFileName, MAX_FILENAME_LENGTH, "%s/%s", _G_pcTftpRootPath, pcFileName);
        
        if (iReqType == LWIP_TFTP_OPC_RDREQ) {                          /*  ������                      */
            iFd = open(cFullFileName, O_RDONLY);
            if (iFd < 0) {
                __inetTftpSendErrorFrame(iSock, &sockaddrinRemote, LWIP_TFTP_EC_NOFILE);
                continue;
            }
        } else if (iReqType == LWIP_TFTP_OPC_WRREQ) {                   /*  д����                      */
            iFd = open(cFullFileName, (O_CREAT | O_EXCL | O_WRONLY), DEFAULT_FILE_PERM);
            if (iFd < 0) {
                __inetTftpSendErrorFrame(iSock, &sockaddrinRemote, LWIP_TFTP_EC_FILEEXIST);
                continue;
            }
        } else {
            continue;                                                   /*  ����                        */
        }
        
        iTempSock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);           /*  ������ʱͨ�Žӿ�            */
        if (iTempSock < 0) {
            iErrLevel = 1;
            goto    __error_handle;
        }                                                               /*  ����ͨ�ų�ʱ                */
        setsockopt(iTempSock, SOL_SOCKET, SO_RCVTIMEO, (const void *)&iTimeout, sizeof(INT));
        
        if (iReqType == LWIP_TFTP_OPC_RDREQ) {                          /*  ��������������              */
            ptftpsa = (__PLW_TFTP_SERVERARG)__SHEAP_ALLOC(sizeof(__LW_TFTP_SERVERARG));
            if (ptftpsa == LW_NULL) {
                iErrLevel = 2;
                goto    __error_handle;
            }
            ptftpsa->TFTPSA_cFileName[0] = PX_EOS;                      /*  ��ȡ�ļ�����Ҫ��¼�ļ���    */
        } else {
            ptftpsa = (__PLW_TFTP_SERVERARG)__SHEAP_ALLOC(sizeof(__LW_TFTP_SERVERARG) + 
                                                          lib_strlen(cFullFileName));
            if (ptftpsa == LW_NULL) {
                iErrLevel = 2;
                goto    __error_handle;
            }
            lib_strcpy(ptftpsa->TFTPSA_cFileName, cFullFileName);       /*  д��ʱ��Ҫ��¼�ļ���        */
        }                                                               /*  ������ʧ��ʱ, ��Ҫɾ���ļ�  */
        ptftpsa->TFTPSA_iReqType         = iReqType;
        ptftpsa->TFTPSA_iFd              = iFd;
        ptftpsa->TFTPSA_iSock            = iTempSock;
        ptftpsa->TFTPSA_sockaddrinRemote = sockaddrinRemote;            /*  ��¼Զ�̵�ַ                */
        
        API_ThreadAttrSetArg(&threadattr, (PVOID)ptftpsa);
        if (iReqType == LWIP_TFTP_OPC_RDREQ) {                          /*  ������                      */
            ulThreadHandle = API_ThreadCreate("t_tftprtmp", 
                                              (PTHREAD_START_ROUTINE)__inetTftpRdServer, 
                                              &threadattr, LW_NULL);
        } else {
            ulThreadHandle = API_ThreadCreate("t_tftpwtmp", 
                                              (PTHREAD_START_ROUTINE)__inetTftpWrServer, 
                                              &threadattr, LW_NULL);
        }
        if (ulThreadHandle == LW_OBJECT_HANDLE_INVALID) {
            iErrLevel = 3;
            goto    __error_handle;
        }
        continue;                                                       /*  �ȴ���һ������              */
        
__error_handle:
        if (iErrLevel > 2) {
            __SHEAP_FREE(ptftpsa);
        }
        if (iErrLevel > 1) {
            close(iTempSock);
        }
        if (iErrLevel > 0) {
            close(iFd);
            if (iReqType == LWIP_TFTP_OPC_WRREQ) {                      /*  д����������                */
                unlink(cFullFileName);                                  /*  ɾ���������ļ�              */
            }
        }
        __inetTftpSendErrorFrame(iSock, &sockaddrinRemote, LWIP_TFTP_EC_TFTP);
    }
}
/*********************************************************************************************************
** ��������: API_INetTftpServerInit
** ��������: ��ʼ�� tftp ������
** �䡡��  : pcPath        ����Ŀ¼
** �䡡��  : ERROR
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
VOID  API_INetTftpServerInit (CPCHAR  pcPath)
{
    static BOOL             bIsInit = LW_FALSE;
    LW_CLASS_THREADATTR     threadattr;
           PCHAR            pcNewPath = "\0";
    
    if (bIsInit) {                                                      /*  �Ѿ���ʼ����                */
        return;
    } else {
        bIsInit = LW_TRUE;
    }
    
    if (pcPath) {                                                       /*  ��Ҫ���÷�����Ŀ¼          */
        pcNewPath = (PCHAR)pcPath;
    }
    if (_G_pcTftpRootPath) {
        __SHEAP_FREE(_G_pcTftpRootPath);
    }
    _G_pcTftpRootPath = (PCHAR)__SHEAP_ALLOC(lib_strlen(pcNewPath) + 1);
    if (_G_pcTftpRootPath == LW_NULL) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "system low memory.\r\n");
        _ErrorHandle(ERROR_SYSTEM_LOW_MEMORY);
        goto    __error_handle;
    }
    lib_strcpy(_G_pcTftpRootPath, pcNewPath);                           /*  ���������·��              */
    
    API_ThreadAttrBuild(&threadattr,
                        LW_CFG_NET_TFTP_STK_SIZE,
                        LW_PRIO_T_SERVICE,
                        LW_OPTION_THREAD_STK_CHK | LW_OPTION_OBJECT_GLOBAL,
                        LW_NULL);
    if (API_ThreadCreate("t_tftpd", (PTHREAD_START_ROUTINE)__inetTftpServerListen, 
                         &threadattr, LW_NULL) == LW_OBJECT_HANDLE_INVALID) {
        goto    __error_handle;
    }
    
#if LW_CFG_SHELL_EN > 0
    /*
     *  ���� SHELL ����.
     */
    API_TShellKeywordAdd("tftpdpath", __tshellNetTftpdPath);
    API_TShellFormatAdd("tftpdpath", " [new path]");
    API_TShellHelpAdd("tftpdpath",   "set default tftp server path.\n");
    
    API_TShellKeywordAdd("tftp", __tshellTftp);
    API_TShellFormatAdd("tftp", " [-i] [Host] [{get | put}] [Source] [Destination]");
    API_TShellHelpAdd("tftp",   "exchange file using tftp protocol.\n");
#endif                                                                  /*  LW_CFG_SHELL_EN > 0         */
    
    return;
    
__error_handle:
    bIsInit = LW_FALSE;                                                 /*  ��ʼ��ʧ��                  */
}
/*********************************************************************************************************
** ��������: API_INetTftpServerPath
** ��������: ���� tftp ��������Ŀ¼
** �䡡��  : pcPath        ����Ŀ¼
** �䡡��  : ERROR
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
INT  API_INetTftpServerPath (CPCHAR  pcPath)
{
    REGISTER PCHAR    pcNewPath;
    REGISTER PCHAR    pcTemp = _G_pcTftpRootPath;
    
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
        _G_pcTftpRootPath = pcNewPath;                                  /*  �����µķ�����·��          */
    );
    
    if (pcTemp) {
        __SHEAP_FREE(pcTemp);
    }
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: API_INetTftpServerBindDev
** ��������: ���� tftp ���������豸
** �䡡��  : uiIndex        �����豸����
** �䡡��  : ERROR
** ȫ�ֱ���:
** ����ģ��:
                                           API ����
*********************************************************************************************************/
LW_API
INT  API_INetTftpServerBindDev (UINT  uiIndex)
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
** ��������: __tshellNetTftpdPath
** ��������: ϵͳ���� "tftpdpath"
** �䡡��  : iArgC         ��������
**           ppcArgV       ������
** �䡡��  : 0
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
#if LW_CFG_SHELL_EN > 0

static INT  __tshellNetTftpdPath (INT  iArgC, PCHAR  *ppcArgV)
{
    if (iArgC < 2) {
        printf("tftpd path: %s\n", _G_pcTftpRootPath);                  /*  ��ӡ��ǰ������Ŀ¼          */
        return  (ERROR_NONE);
    }
    
    return  (API_INetTftpServerPath(ppcArgV[1]));
}

#endif                                                                  /*  LW_CFG_SHELL_EN > 0         */
/*********************************************************************************************************
** ��������: __tshellTftp
** ��������: ϵͳ���� "tftp"
** �䡡��  : iArgC         ��������
**           ppcArgV       ������
** �䡡��  : 0
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
#if LW_CFG_SHELL_EN > 0

static INT  __tshellTftp (INT  iArgC, PCHAR  *ppcArgV)
{
    INT     iError;
    PCHAR   pcLocalFile;
    PCHAR   pcRemoteFile;

    if ((iArgC != 6) && (iArgC != 5)) {
        fprintf(stderr, "arguments error!\n");
        return  (-ERROR_TSHELL_EPARAM);
    }
    if (lib_strcmp(ppcArgV[1], "-i")) {
        fprintf(stderr, "must use -i option (binary type)!\n");
        return  (-ERROR_TSHELL_EPARAM);
    }

    if (lib_strcasecmp(ppcArgV[3], "get") == 0) {
        pcRemoteFile = ppcArgV[4];
        if (iArgC == 6) {
            pcLocalFile = ppcArgV[5];
        } else {
            pcLocalFile = pcRemoteFile;
        }
        printf("getting file...\n");
        iError = API_INetTftpReceive(ppcArgV[2], pcRemoteFile, pcLocalFile);
    } else if (lib_strcasecmp(ppcArgV[3], "put") == 0) {
        pcRemoteFile = ppcArgV[5];
        if (iArgC == 6) {
            pcLocalFile = ppcArgV[4];
        } else {
            pcLocalFile = pcRemoteFile;
        }
        printf("sending file...\n");
        iError = API_INetTftpSend(ppcArgV[2], pcRemoteFile, pcLocalFile);
    } else {
        fprintf(stderr, "arguments error!\n");
        return  (-ERROR_TSHELL_EPARAM);
    }

    if (iError < 0) {
        if (errno == EHOSTUNREACH) {
            fprintf(stderr, "can not find host.\n");
        } else {
            fprintf(stderr, "some error occur, error: %s\n", lib_strerror(errno));
        }
    } else {
        printf("file transfer completed.\n");
    }

    return  (ERROR_NONE);
}

#endif                                                                  /*  LW_CFG_SHELL_EN > 0         */

#endif                                                                  /*  LW_CFG_NET_EN > 0           */
                                                                        /*  LW_CFG_NET_TFTP_EN > 0      */
/*********************************************************************************************************
  END
*********************************************************************************************************/
