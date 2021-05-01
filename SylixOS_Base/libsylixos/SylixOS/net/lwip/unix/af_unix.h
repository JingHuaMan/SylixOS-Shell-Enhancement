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
** ��   ��   ��: af_unix.h
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2012 �� 12 �� 18 ��
**
** ��        ��: AF_UNIX ֧��
*********************************************************************************************************/

#ifndef __AF_UNIX_H
#define __AF_UNIX_H

/*********************************************************************************************************
  AF_UNIX �������ݽڵ���չ����
*********************************************************************************************************/
#if LW_CFG_NET_EN > 0 && LW_CFG_NET_UNIX_EN > 0

typedef struct af_unix_node_ex {
    PCHAR               UNIE_pcMsgEx;                                   /*  ��չ��Ϣ                    */
    socklen_t           UNIE_uiLenEx;                                   /*  ��չ��Ϣ����                */
    pid_t               UNIE_pid;                                       /*  ������Ϣ���� pid            */
    BOOL                UNIE_bNeedUnProc;                               /*  ��չ�����Ƿ���Ҫunproc����  */
    BOOL                UNIE_bValid;                                    /*  �����Ƿ���Ч                */
} AF_UNIX_NEX;

/*********************************************************************************************************
  AF_UNIX �������ݶ��� (ʣ����Ϣ����Ϊ UNIM_stLen - UNIM_stOffset)
*********************************************************************************************************/

typedef struct af_unix_node {                                           /*  һ����Ϣ�ڵ�                */
    LW_LIST_MONO        UNIM_monoManage;                                /*  ������������                */
    
    PCHAR               UNIM_pcMsg;                                     /*  ���յ���Ϣ                  */
    size_t              UNIM_stLen;                                     /*  ��Ϣ����                    */
    size_t              UNIM_stOffset;                                  /*  ��ʼ��Ϣƫ��                */
    
    AF_UNIX_NEX        *UNIM_punie;                                     /*  ��չ����, ��������          */
    
    CHAR                UNIM_cPath[1];                                  /*  ���ͷ��ĵ�ַ                */
} AF_UNIX_N;

typedef struct af_unix_queue {
    PLW_LIST_MONO       UNIQ_pmonoHeader;                               /*  ��Ϣ����ͷ                  */
    PLW_LIST_MONO       UNIQ_pmonoTail;                                 /*  ��Ϣ���н�β                */
    size_t              UNIQ_stTotal;                                   /*  ����Ч�����ֽ���            */
} AF_UNIX_Q;

/*********************************************************************************************************
  AF_UNIX ���ƿ�
  
  ���� UNIX_hCanRead  ��ʾ��ǰ socket ������������ʱ�ȴ����ź���
  ���� UNIX_hCanWrite ��ʾ���� socket д���ڵ�ʱ��Ҫ�ȴ����ź���
*********************************************************************************************************/

typedef struct af_unix_t {
    LW_LIST_LINE        UNIX_lineManage;
    
    LW_LIST_RING        UNIX_ringConnect;                               /*  ���Ӷ���                    */
    LW_LIST_RING_HEADER UNIX_pringConnect;                              /*  �ȴ����ӵĶ���              */
    INT                 UNIX_iConnNum;                                  /*  �ȴ����ӵ�����              */
    
    INT                 UNIX_iReuse;                                    /*  REUSE flag                  */
    INT                 UNIX_iFlag;                                     /*  NONBLOCK or NOT             */
    INT                 UNIX_iType;                                     /*  STREAM / DGRAM / SEQPACKET  */
    
#define __AF_UNIX_STATUS_NONE       0                                   /*  SOCK_DGRAM ��Զ���ڴ�״̬   */
#define __AF_UNIX_STATUS_LISTEN     1
#define __AF_UNIX_STATUS_CONNECT    2
#define __AF_UNIX_STATUS_ESTAB      3
    INT                 UNIX_iStatus;                                   /*  ��ǰ״̬ (����� STREAM)    */
    
#define __AF_UNIX_SHUTD_R           0x01
#define __AF_UNIX_SHUTD_W           0x02
    INT                 UNIX_iShutDFlag;                                /*  ��ǰ shutdown ״̬          */
    INT                 UNIX_iBacklog;                                  /*  �ȴ��ı�����������          */
    
    struct af_unix_t   *UNIX_pafunxPeer;                                /*  ���ӵ�Զ�̽ڵ�              */
    AF_UNIX_Q           UNIX_unixq;                                     /*  �������ݶ���                */
    size_t              UNIX_stMaxBufSize;                              /*  �����ջ����С            */
                                                                        /*  ����������д��ʱ, ��Ҫ����  */
    LW_OBJECT_HANDLE    UNIX_hCanRead;                                  /*  �ɶ�                        */
    LW_OBJECT_HANDLE    UNIX_hCanWrite;                                 /*  ��д (Ҳ������ accept)      */
    
    ULONG               UNIX_ulSendTimeout;                             /*  ���ͳ�ʱ tick               */
    ULONG               UNIX_ulRecvTimeout;                             /*  ��ȡ��ʱ tick               */
    ULONG               UNIX_ulConnTimeout;                             /*  ���ӳ�ʱ tick               */
    
    struct linger       UNIX_linger;                                    /*  �ӳٹر�                    */
    INT                 UNIX_iPassCred;                                 /*  �Ƿ���������֤��Ϣ        */
    
    PVOID               UNIX_sockFile;                                  /*  socket file                 */
    CHAR                UNIX_cFile[MAX_FILENAME_LENGTH];
} AF_UNIX_T;

/*********************************************************************************************************
  �ڲ�����
*********************************************************************************************************/

VOID        unix_init(VOID);
VOID        unix_traversal(VOIDFUNCPTR pfunc, PVOID pvArg0, PVOID pvArg1,
                           PVOID pvArg2, PVOID  pvArg3, PVOID  pvArg4, PVOID pvArg5);
AF_UNIX_T  *unix_socket(INT  iDomain, INT  iType, INT  iProtocol);
INT         unix_bind(AF_UNIX_T  *pafunix, const struct sockaddr *name, socklen_t namelen);
INT         unix_listen(AF_UNIX_T  *pafunix, INT  backlog);
AF_UNIX_T  *unix_accept(AF_UNIX_T  *pafunix, struct sockaddr *addr, socklen_t *addrlen);
INT         unix_connect(AF_UNIX_T  *pafunix, const struct sockaddr *name, socklen_t namelen);
INT         unix_connect2(AF_UNIX_T  *pafunix0, AF_UNIX_T  *pafunix1);
ssize_t     unix_recvfrom(AF_UNIX_T  *pafunix, void *mem, size_t len, int flags,
                          struct sockaddr *from, socklen_t *fromlen);
ssize_t     unix_recv(AF_UNIX_T  *pafunix, void *mem, size_t len, int flags);
ssize_t     unix_recvmsg(AF_UNIX_T  *pafunix, struct msghdr *msg, int flags);
ssize_t     unix_sendto(AF_UNIX_T  *pafunix, const void *data, size_t size, int flags,
                        const struct sockaddr *to, socklen_t tolen);
ssize_t     unix_send(AF_UNIX_T  *pafunix, const void *data, size_t size, int flags);
ssize_t     unix_sendmsg(AF_UNIX_T  *pafunix, const struct msghdr *msg, int flags);
INT         unix_close(AF_UNIX_T  *pafunix);
INT         unix_shutdown(AF_UNIX_T  *pafunix, int how);
INT         unix_getsockname(AF_UNIX_T  *pafunix, struct sockaddr *name, socklen_t *namelen);
INT         unix_getpeername(AF_UNIX_T  *pafunix, struct sockaddr *name, socklen_t *namelen);
INT         unix_setsockopt(AF_UNIX_T  *pafunix, int level, int optname, 
                            const void *optval, socklen_t optlen);
INT         unix_getsockopt(AF_UNIX_T  *pafunix, int level, int optname, 
                            void *optval, socklen_t *optlen);
INT         unix_ioctl(AF_UNIX_T  *pafunix, INT  iCmd, PVOID  pvArg);

#endif                                                                  /*  LW_CFG_NET_EN > 0           */
                                                                        /*  LW_CFG_NET_UNIX_EN > 0      */
#endif                                                                  /*  __AF_UNIX_H                 */
/*********************************************************************************************************
  END
*********************************************************************************************************/
