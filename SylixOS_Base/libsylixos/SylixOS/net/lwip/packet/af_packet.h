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
** ��   ��   ��: af_packet.h
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2014 �� 03 �� 21 ��
**
** ��        ��: AF_PACKET ֧��
*********************************************************************************************************/

#ifndef __AF_PACKET_H
#define __AF_PACKET_H

#include "netpacket/packet.h"

/*********************************************************************************************************
  AF_PACKET �������ݶ���
*********************************************************************************************************/

typedef struct af_packet_node {                                         /*  һ����Ϣ�ڵ�                */
    LW_LIST_MONO            PKTM_monoManage;                            /*  ������������                */
    
    struct pbuf            *PKTM_p;
    u8_t                    PKTM_ucForme;                               /*  ���͸����������ݰ�          */
    u8_t                    PKTM_ucIndex;                               /*  ��������ӿ�                */
    u8_t                    PKTM_ucOutgo;                               /*  �Ƿ�Ϊ���ͽػ�              */
} AF_PACKET_N;

typedef struct af_packet_queue {
    PLW_LIST_MONO           PKTQ_pmonoHeader;                           /*  ��Ϣ����ͷ                  */
    PLW_LIST_MONO           PKTQ_pmonoTail;                             /*  ��Ϣ���н�β                */
    size_t                  PKTQ_stTotal;                               /*  ����Ч�����ֽ���            */
} AF_PACKET_Q;

/*********************************************************************************************************
  AF_PACKET �������ݶ���
*********************************************************************************************************/
#if LW_CFG_NET_PACKET_MMAP > 0

typedef struct af_packet_mmap {
    struct tpacket_req      PKTB_reqbuf;
    UINT                    PKTB_uiFramePerBlock;
    UINT                    PKTB_uiFramePtr;
    UINT                    PKTB_uiFrameMax;
    PVOID                   PKTB_pvPhyMem;                              /*  �����ڴ������ַ            */
    PVOID                   PKTB_pvVirMem;                              /*  �����ڴ������ַ            */
    size_t                  PKTB_stSize;                                /*  �����ڴ��С                */
} AF_PACKET_MMAP;

#endif                                                                  /*  LW_CFG_NET_PACKET_MMAP > 0  */
/*********************************************************************************************************
  AF_PACKET ���ƿ�
*********************************************************************************************************/

typedef struct af_packet_t {
    LW_LIST_LINE            PACKET_lineManage;
    
    INT                     PACKET_iFlag;                               /*  NONBLOCK or NOT             */
    INT                     PACKET_iType;                               /*  RAW / DGRAM                 */
    INT                     PACKET_iProtocol;                           /*  Э��                        */
    
#define __AF_PACKET_SHUTD_R     0x01
#define __AF_PACKET_SHUTD_W     0x02
    INT                     PACKET_iShutDFlag;                          /*  ��ǰ shutdown ״̬          */
    
    BOOL                    PACKET_bRecvOut;                            /*  �Ƿ����������ݰ�          */
    INT                     PACKET_iIfIndex;                            /*  �󶨵Ľ���                  */
    AF_PACKET_Q             PACKET_pktq;                                /*  ���ջ���                    */
    size_t                  PACKET_stMaxBufSize;                        /*  �����ջ����С            */
    
    struct sockaddr_ll      PACKET_saddrll;                             /*  connect ��Ϣ                */
    enum tpacket_versions   PACKET_tpver;                               /*  RING ͷ���汾               */
    struct tpacket_stats    PACKET_stats;                               /*  ͳ����Ϣ                    */
    
    UINT                    PACKET_uiHdrLen;                            /*  tp_hdrlen                   */
    UINT                    PACKET_uiReserve;                           /*  tp_reserve                  */
    
#if LW_CFG_NET_PACKET_MMAP > 0
    BOOL                    PACKET_bMapBusy;                            /*  �Ƿ��ڷ�æ                  */
    BOOL                    PACKET_bMmap;                               /*  �Ƿ��Ѿ������� MMAP         */
    AF_PACKET_MMAP          PACKET_mmapRx;                              /*  mmap ���ջ���               */
#endif                                                                  /*  LW_CFG_NET_PACKET_MMAP > 0  */

    LW_OBJECT_HANDLE        PACKET_hCanRead;                            /*  �ɶ�                        */
    ULONG                   PACKET_ulRecvTimeout;                       /*  ��ȡ��ʱ tick               */
    
    PVOID                   PACKET_sockFile;                            /*  socket file                 */
} AF_PACKET_T;

/*********************************************************************************************************
  �ڲ�����
*********************************************************************************************************/

INT             packet_link_input(struct pbuf *p, struct netif *inp, BOOL bOutgo);
VOID            packet_init(VOID);
VOID            packet_traversal(VOIDFUNCPTR pfunc, PVOID pvArg0, PVOID pvArg1, PVOID pvArg2,
                                 PVOID pvArg3, PVOID pvArg4, PVOID pvArg5);
AF_PACKET_T    *packet_socket(INT iDomain, INT iType, INT iProtocol);
INT             packet_bind(AF_PACKET_T *pafpacket, const struct sockaddr *name, socklen_t namelen);
INT             packet_listen(AF_PACKET_T *pafpacket, INT backlog);
AF_PACKET_T    *packet_accept(AF_PACKET_T *pafpacket, struct sockaddr *addr, socklen_t *addrlen);
INT             packet_connect(AF_PACKET_T *pafpacket, const struct sockaddr *name, socklen_t namelen);
ssize_t         packet_recvfrom(AF_PACKET_T *pafpacket, void *mem, size_t len, int flags,
                                struct sockaddr *from, socklen_t *fromlen);
ssize_t         packet_recv(AF_PACKET_T *pafpacket, void *mem, size_t len, int flags);
ssize_t         packet_recvmsg(AF_PACKET_T *pafpacket, struct msghdr *msg, int flags);
ssize_t         packet_sendto(AF_PACKET_T *pafpacket, const void *data, size_t size, int flags,
                              const struct sockaddr *to, socklen_t tolen);
ssize_t         packet_send(AF_PACKET_T *pafpacket, const void *data, size_t size, int flags);
ssize_t         packet_sendmsg(AF_PACKET_T *pafpacket, const struct msghdr *msg, int flags);
INT             packet_close(AF_PACKET_T *pafpacket);
INT             packet_shutdown(AF_PACKET_T *pafpacket, int how);
INT             packet_getsockname(AF_PACKET_T *pafpacket, struct sockaddr *name, socklen_t *namelen);
INT             packet_getpeername(AF_PACKET_T *pafpacket, struct sockaddr *name, socklen_t *namelen);
INT             packet_setsockopt(AF_PACKET_T *pafpacket, int level, int optname, 
                                  const void *optval, socklen_t optlen);
INT             packet_getsockopt(AF_PACKET_T *pafpacket, int level, int optname, 
                                  void *optval, socklen_t *optlen);
INT             packet_ioctl(AF_PACKET_T *pafpacket, INT  iCmd, PVOID  pvArg);
INT             packet_mmap(AF_PACKET_T *pafpacket, PLW_DEV_MMAP_AREA  pdmap);
INT             packet_unmap(AF_PACKET_T *pafpacket, PLW_DEV_MMAP_AREA  pdmap);

#endif                                                                  /*  __AF_PACKET_H               */
/*********************************************************************************************************
  END
*********************************************************************************************************/
