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
** ��   ��   ��: lwip_netstat.c
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2013 �� 08 �� 23 ��
**
** ��        ��: lwip netstat ����ʵ�ֺ���.

** BUG:
2013.09.11  ����ӿڴ�ӡֱ�ӷ��� /proc/net/dev �ļ�.
2014.05.06  tcp listen ��ӡ, ����� IPv6 ���ӡ�Ƿ�ֻ���� IPv6 ��������.
*********************************************************************************************************/
#define  __SYLIXOS_STDIO
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
#include "../SylixOS/system/include/s_system.h"
/*********************************************************************************************************
  �ü�����
*********************************************************************************************************/
#if (LW_CFG_NET_EN > 0) && (LW_CFG_SHELL_EN > 0)
#include "lwip/sockets.h"
#include "lwip/tcpip.h"
#include "lwip/netif.h"
#include "lwip/stats.h"
/*********************************************************************************************************
  TCP
*********************************************************************************************************/
#include "lwip/tcp.h"
#include "lwip/priv/tcp_priv.h"
extern struct tcp_pcb           *tcp_active_pcbs;
extern union  tcp_listen_pcbs_t  tcp_listen_pcbs;
extern struct tcp_pcb           *tcp_tw_pcbs;
/*********************************************************************************************************
  UDP
*********************************************************************************************************/
#include "lwip/udp.h"
/*********************************************************************************************************
  RAW
*********************************************************************************************************/
#include "lwip/raw.h"
/*********************************************************************************************************
  IGMP
*********************************************************************************************************/
#include "lwip/igmp.h"
#include "lwip/mld6.h"
/*********************************************************************************************************
  �����
*********************************************************************************************************/
#define __NETSTAT_INC_UNIX(type)    ((type == 0) || (type == 1))
#define __NETSTAT_INC_IPV4(type)    ((type == 0) || (type == 2))
#define __NETSTAT_INC_IPV6(type)    ((type == 0) || (type == 3))
/*********************************************************************************************************
** ��������: __tshellNetstatIf
** ��������: ϵͳ���� "netstat -i"
** �䡡��  : NONE
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
VOID  __tshellNetstatIf (VOID)
{
    INT     iFd;
    CHAR    cBuffer[512];
    ssize_t sstNum;
    
    iFd = open("/proc/net/dev", O_RDONLY);
    if (iFd < 0) {
        fprintf(stderr, "can not open /proc/net/dev : %s\n", lib_strerror(errno));
        return;
    }
    
    do {
        sstNum = read(iFd, cBuffer, sizeof(cBuffer));
        if (sstNum > 0) {
            write(STDOUT_FILENO, cBuffer, (size_t)sstNum);
        }
    } while (sstNum > 0);
    
    close(iFd);
}
/*********************************************************************************************************
** ��������: __GroupPrint
** ��������: ��ӡ���� group �ļ�
** �䡡��  : group         group ���ƿ�
**           netif         ����ӿ�
**           pcBuffer      ����
**           stTotalSize   ��������С
**           pstOft        ��ǰƫ����
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
#if LWIP_IGMP > 0

static VOID  __GroupPrint (struct igmp_group *group, struct netif *netif,
                           PCHAR  pcBuffer, size_t  stTotalSize, size_t *pstOft)
{
    CHAR    cBuffer[INET_ADDRSTRLEN];
    CHAR    cIfName[NETIF_NAMESIZE];

    *pstOft = bnprintf(pcBuffer, stTotalSize, *pstOft,
                       "%-4s %-15s %d\n",
                       netif_get_name(netif, cIfName),
                       ip4addr_ntoa_r(&group->group_address, cBuffer, sizeof(cBuffer)),
                       (u32_t)group->use);
}
/*********************************************************************************************************
** ��������: __Group6Print
** ��������: ��ӡ���� group6 �ļ�
** �䡡��  : mld_group     group6 ���ƿ�
**           netif         ����ӿ�
**           pcBuffer      ����
**           stTotalSize   ��������С
**           pstOft        ��ǰƫ����
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
#if LWIP_IPV6

static VOID  __Group6Print (struct mld_group *mld_group, struct netif *netif,
                            PCHAR  pcBuffer, size_t  stTotalSize, size_t *pstOft)
{
    CHAR    cBuffer[INET6_ADDRSTRLEN];
    CHAR    cIfName[NETIF_NAMESIZE];
    
    *pstOft = bnprintf(pcBuffer, stTotalSize, *pstOft,
                       "%-4s %-39s %d\n",
                       netif_get_name(netif, cIfName),
                       ip6addr_ntoa_r(&mld_group->group_address, cBuffer, sizeof(cBuffer)),
                       (u32_t)mld_group->use);
}

#endif                                                                  /*  LWIP_IPV6                   */
/*********************************************************************************************************
** ��������: __tshellNetstatGroup
** ��������: ϵͳ���� "netstat -g"
** �䡡��  : iNetType      ��������
** �䡡��  : ERROR_NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
VOID  __tshellNetstatGroup (INT  iNetType)
{
    const CHAR           cIgmpInfoHdr[] = 
    "DEV  GROUP           COUNT\n";
    
    const CHAR           cIgmp6InfoHdr[] = 
    "\nDEV  GROUP6                                  COUNT\n";

    size_t               stNeedBufferSize = 0;
    struct igmp_group   *group;
    struct netif        *netif;
    PCHAR                pcPrintBuf;
    size_t               stRealSize = 0;

#if LWIP_IPV6
    struct mld_group    *mld_group;
#endif                                                                  /*  LWIP_IPV6                   */
    
    LOCK_TCPIP_CORE();
    if (__NETSTAT_INC_IPV4(iNetType)) {
        NETIF_FOREACH(netif) {
            for (group = netif_igmp_data(netif); group != NULL; group = group->next) {
                stNeedBufferSize += 128;
            }
        }
    }
#if LWIP_IPV6
    if (__NETSTAT_INC_IPV6(iNetType)) {
        NETIF_FOREACH(netif) {
            for (mld_group = netif_mld6_data(netif); mld_group != NULL; mld_group = mld_group->next) {
                stNeedBufferSize += 256;
            }
        }
    }
#endif                                                                  /*  LWIP_IPV6                   */
    UNLOCK_TCPIP_CORE();
    
    stNeedBufferSize += sizeof(cIgmpInfoHdr) + sizeof(cIgmp6InfoHdr);
    
    pcPrintBuf = (PCHAR)__SHEAP_ALLOC(stNeedBufferSize);
    if (!pcPrintBuf) {
        fprintf(stderr, "no memory!\n");
        _ErrorHandle(ENOMEM);
        return;
    }
    
    LOCK_TCPIP_CORE();
    if (__NETSTAT_INC_IPV4(iNetType)) {
        stRealSize = bnprintf(pcPrintBuf, stNeedBufferSize, stRealSize, cIgmpInfoHdr);
        NETIF_FOREACH(netif) {
            for (group = netif_igmp_data(netif); group != NULL; group = group->next) {
                __GroupPrint(group, netif, pcPrintBuf, stNeedBufferSize, &stRealSize);
            }
        }
    }
#if LWIP_IPV6
    if (__NETSTAT_INC_IPV6(iNetType)) {
        stRealSize = bnprintf(pcPrintBuf, stNeedBufferSize, stRealSize, cIgmp6InfoHdr);
        NETIF_FOREACH(netif) {
            for (mld_group = netif_mld6_data(netif); mld_group != NULL; mld_group = mld_group->next) {
                __Group6Print(mld_group, netif, pcPrintBuf, stNeedBufferSize, &stRealSize);
            }
        }
    }
#endif                                                                  /*  LWIP_IPV6                   */
    UNLOCK_TCPIP_CORE();
    
    printf("%s\n", pcPrintBuf);
    
    __SHEAP_FREE(pcPrintBuf);
}

#endif                                                                  /*  LWIP_IGMP > 0               */
/*********************************************************************************************************
** ��������: __tshellNetstatStat
** ��������: ϵͳ���� "netstat -s"
** �䡡��  : NONE
** �䡡��  : ERROR_NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
VOID  __tshellNetstatStat (VOID)
{
    INT     iFd = open("/proc/net/tcpip_stat", O_RDONLY);
    CHAR    cBuffer[1024];
    ssize_t sstNum;
    
    fflush(stdout);
    
    if (iFd < 0) {
        fprintf(stderr, "can not open /proc/net/tcpip_stat : %s\n", lib_strerror(errno));
        return;
    }
    
    do {
        sstNum = read(iFd, cBuffer, sizeof(cBuffer));
        if (sstNum > 0) {
            write(STDOUT_FILENO, cBuffer, (size_t)sstNum);
        }
    } while (sstNum > 0);
    
    close(iFd);
}
/*********************************************************************************************************
** ��������: __RawGetProto
** ��������: ���Э����Ϣ
** �䡡��  : state     ״̬
** �䡡��  : ״̬�ִ�
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
#if LWIP_RAW

static CPCHAR  __RawGetProto (u8_t  proto)
{
    switch (proto) {
    
    case IPPROTO_IP:
        return  ("ip");
        
    case IPPROTO_IPV6:
        return  ("ipv6");
        
    case IPPROTO_ICMP:
        return  ("icmp");
        
    case IPPROTO_ICMPV6:
        return  ("icmpv6");

    case IPPROTO_TCP:
        return  ("tcp");
        
    case IPPROTO_UDP:
        return  ("udp");
        
    case IPPROTO_UDPLITE:
        return  ("udplite");
        
    default:
        return  ("unknown");
    }
}
/*********************************************************************************************************
** ��������: __RawPrint
** ��������: ��ӡ���� raw �ļ�
** �䡡��  : pcb           raw ���ƿ�
**           pcBuffer      ����
**           stTotalSize   ��������С
**           pstOft        ��ǰƫ����
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static VOID  __RawPrint (struct raw_pcb  *pcb, PCHAR  pcBuffer, 
                         size_t  stTotalSize, size_t *pstOft)
{
    CHAR    cBuffer1[INET6_ADDRSTRLEN];
    CHAR    cBuffer2[INET6_ADDRSTRLEN];
    
    if (pcb->netif_idx != NETIF_NO_INDEX) {
        if (IP_IS_V4_VAL(pcb->local_ip)) {
            *pstOft = bnprintf(pcBuffer, stTotalSize, *pstOft,
                               "%Ifidx_%-9d %-15s %s\n",
                               pcb->netif_idx,
                               (ip_2_ip4(&pcb->remote_ip)->addr == IPADDR_ANY) ?
                               "*" : ip4addr_ntoa_r(ip_2_ip4(&pcb->remote_ip), cBuffer2, sizeof(cBuffer2)),
                               __RawGetProto(pcb->protocol));
#if LWIP_IPV6
        } else {
            *pstOft = bnprintf(pcBuffer, stTotalSize, *pstOft,
                               "%Ifidx_%-33d %-39s %s\n",
                               pcb->netif_idx,
                               ip6_addr_isany(ip_2_ip6(&pcb->remote_ip)) ?
                               "*" : ip6addr_ntoa_r(ip_2_ip6(&pcb->remote_ip), cBuffer2, sizeof(cBuffer2)),
                               __RawGetProto(pcb->protocol));
#endif                                                                  /*  LWIP_IPV6                   */
        }

    } else {
        if (IP_IS_V4_VAL(pcb->local_ip)) {
            *pstOft = bnprintf(pcBuffer, stTotalSize, *pstOft,
                               "%-15s %-15s %s\n",
                               (ip_2_ip4(&pcb->local_ip)->addr == IPADDR_ANY) ?
                               "*" : ip4addr_ntoa_r(ip_2_ip4(&pcb->local_ip), cBuffer1, sizeof(cBuffer1)),
                               (ip_2_ip4(&pcb->remote_ip)->addr == IPADDR_ANY) ?
                               "*" : ip4addr_ntoa_r(ip_2_ip4(&pcb->remote_ip), cBuffer2, sizeof(cBuffer2)),
                               __RawGetProto(pcb->protocol));
#if LWIP_IPV6
        } else {
            *pstOft = bnprintf(pcBuffer, stTotalSize, *pstOft,
                               "%-39s %-39s %s\n",
                               ip6_addr_isany(ip_2_ip6(&pcb->local_ip)) ?
                               "*" : ip6addr_ntoa_r(ip_2_ip6(&pcb->local_ip), cBuffer1, sizeof(cBuffer1)),
                               ip6_addr_isany(ip_2_ip6(&pcb->remote_ip)) ?
                               "*" : ip6addr_ntoa_r(ip_2_ip6(&pcb->remote_ip), cBuffer2, sizeof(cBuffer2)),
                               __RawGetProto(pcb->protocol));
#endif                                                                  /*  LWIP_IPV6                   */
        }
    }
}

#endif                                                                  /*  LWIP_RAW                    */
/*********************************************************************************************************
** ��������: __tshellNetstatRaw
** ��������: ϵͳ���� "netstat -w"
** �䡡��  : iNetType      ��������
** �䡡��  : ERROR_NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
VOID  __tshellNetstatRaw (INT  iNetType)
{
#if LWIP_RAW
    const CHAR      cRawInfoHdr[] = 
    "LOCAL           REMOTE          PROTO\n";
    
    const CHAR      cRaw6InfoHdr[] = 
    "\nLOCAL6                                  REMOTE6                                 PROTO\n";
    
    struct raw_pcb  *pcb;
    PCHAR            pcPrintBuf;
    size_t           stRealSize = 0;
    size_t           stNeedBufferSize = 0;
    BOOL             b4 = LW_FALSE, b6 = LW_FALSE;
    
    LOCK_TCPIP_CORE();
    if (__NETSTAT_INC_IPV4(iNetType)) {
        for (pcb = raw_pcbs; pcb != NULL; pcb = pcb->next) {
            if (IP_IS_V4_VAL(pcb->local_ip)) {
                stNeedBufferSize += 128;
                b4 = LW_TRUE;
            }
        }
    }
    if (__NETSTAT_INC_IPV6(iNetType)) {
        for (pcb = raw_pcbs; pcb != NULL; pcb = pcb->next) {
            if (!IP_IS_V4_VAL(pcb->local_ip)) {
                stNeedBufferSize += 192;
                b6 = LW_TRUE;
            }
        }
    }
    UNLOCK_TCPIP_CORE();
    
    if (stNeedBufferSize == 0) {
        return;
    }
    
    stNeedBufferSize += sizeof(cRawInfoHdr) + sizeof(cRaw6InfoHdr);
    
    pcPrintBuf = (PCHAR)__SHEAP_ALLOC(stNeedBufferSize);
    if (!pcPrintBuf) {
        fprintf(stderr, "no memory!\n");
        _ErrorHandle(ENOMEM);
        return;
    }
    
    printf("--RAW--:\n");
    
    LOCK_TCPIP_CORE();
    if (__NETSTAT_INC_IPV4(iNetType) && b4) {
        stRealSize = bnprintf(pcPrintBuf, stNeedBufferSize, stRealSize, cRawInfoHdr);
        for (pcb = raw_pcbs; pcb != NULL; pcb = pcb->next) {
            if (IP_IS_V4_VAL(pcb->local_ip)) {
                __RawPrint(pcb, pcPrintBuf, stNeedBufferSize, &stRealSize);
            }
        }
    }
    if (__NETSTAT_INC_IPV6(iNetType) && b6) {
        stRealSize = bnprintf(pcPrintBuf, stNeedBufferSize, stRealSize, cRaw6InfoHdr);
        for (pcb = raw_pcbs; pcb != NULL; pcb = pcb->next) {
            if (!IP_IS_V4_VAL(pcb->local_ip)) {
                __RawPrint(pcb, pcPrintBuf, stNeedBufferSize, &stRealSize);
            }
        }
    }
    UNLOCK_TCPIP_CORE();
    
    printf("%s\n", pcPrintBuf);
    
    __SHEAP_FREE(pcPrintBuf);
#endif                                                                  /*  LWIP_RAW                    */
}
/*********************************************************************************************************
** ��������: __ProtoAddrBuild
** ��������: ����Э���ַ�ֶ�
** �䡡��  : addr      ��ַ
**           idx       ���豸
**           usPort    �˿�
**           pcBuffer  ������
**           stSize    ��������С
** �䡡��  : ���
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
#if LWIP_UDP || LWIP_TCP

static PCHAR  __ProtoAddrBuild (ip_addr_t  *addr, u8_t idx, u16_t usPort, PCHAR  pcBuffer, size_t  stSize)
{
    CHAR    cBuffer[INET6_ADDRSTRLEN];

    if (idx != NETIF_NO_INDEX) {
        bnprintf(pcBuffer, stSize, 0, "Ifidx_%d:%d", idx, usPort);

    } else {
        if (IP_IS_V4(addr)) {
            bnprintf(pcBuffer, stSize, 0, "%s:%d",
                     (ip_2_ip4(addr)->addr == IPADDR_ANY) ?
                     "*" : ip4addr_ntoa_r(ip_2_ip4(addr), cBuffer, sizeof(cBuffer)),
                     usPort);
#if LWIP_IPV6
        } else {
            bnprintf(pcBuffer, stSize, 0, "%s:%d",
                     ip6_addr_isany(ip_2_ip6(addr)) ?
                     "*" : ip6addr_ntoa_r(ip_2_ip6(addr), cBuffer, sizeof(cBuffer)),
                     usPort);
#endif                                                                  /*  LWIP_IPV6                   */
        }
    }
    
    return  (pcBuffer);
}

#endif                                                                  /*  LWIP_UDP || LWIP_TCP        */
/*********************************************************************************************************
** ��������: __TcpGetStat
** ��������: ��� tcp ״̬��Ϣ
** �䡡��  : state     ״̬
** �䡡��  : ״̬�ִ�
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
#if LWIP_TCP

static CPCHAR  __TcpGetStat (u8_t  state)
{
    static const PCHAR cTcpState[] = {
        "close",
        "listen",
        "syn_send",
        "syn_rcvd",
        "estab",
        "fin_w_1",
        "fin_w_2",
        "close_w",
        "closeing",
        "last_ack",
        "time_w",
        "unknown"
    };
    
    if (state > 11) {
        state = 11;
    }
    
    return  (cTcpState[state]);
}
/*********************************************************************************************************
** ��������: __TcpPrint
** ��������: ��ӡ���� tcp �ļ�
** �䡡��  : pcb           tcp ���ƿ�
**           pcBuffer      ����
**           stTotalSize   ��������С
**           pstOft        ��ǰƫ����
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static VOID  __TcpPrint (struct tcp_pcb *pcb, PCHAR  pcBuffer, 
                         size_t  stTotalSize, size_t *pstOft)
{
    CHAR    cBuffer1[INET6_ADDRSTRLEN];
    CHAR    cBuffer2[INET6_ADDRSTRLEN];

    if (IP_IS_V4_VAL(pcb->local_ip)) {
        if (pcb->state == LISTEN) {
            *pstOft = bnprintf(pcBuffer, stTotalSize, *pstOft,
                               "%-21s %-21s %-8s %7d %7d %7d\n",
                               __ProtoAddrBuild(&pcb->local_ip, pcb->netif_idx, pcb->local_port,
                                                cBuffer1, sizeof(cBuffer1)),
                               "*:*",
                               __TcpGetStat((u8_t)pcb->state),
                               0, 0, 0);
        } else {
            *pstOft = bnprintf(pcBuffer, stTotalSize, *pstOft,
                               "%-21s %-21s %-8s %-2s %7d %7d %7d\n",
                               __ProtoAddrBuild(&pcb->local_ip, pcb->netif_idx, pcb->local_port,
                                                cBuffer1, sizeof(cBuffer1)),
                               __ProtoAddrBuild(&pcb->remote_ip, pcb->netif_idx, pcb->remote_port,
                                                cBuffer2, sizeof(cBuffer2)),
                               __TcpGetStat((u8_t)pcb->state),
                               ip_get_option(pcb, SO_KEEPALIVE) ? "*" : "",
                               (u32_t)pcb->nrtx, (u32_t)pcb->rcv_wnd, (u32_t)pcb->snd_wnd);
        }
    
#if LWIP_IPV6
    } else {
        if (pcb->state == LISTEN) {
            *pstOft = bnprintf(pcBuffer, stTotalSize, *pstOft,
                               "%-44s %-44s %-8s %-9s %7d %7d %7d\n",
                               __ProtoAddrBuild(&pcb->local_ip, pcb->netif_idx, pcb->local_port,
                                                cBuffer1, sizeof(cBuffer1)),
                               "*:*",
                               __TcpGetStat((u8_t)pcb->state),
                               IP_IS_V6_VAL(pcb->local_ip) ? "YES" : "NO",
                               0, 0, 0);
        } else {
            *pstOft = bnprintf(pcBuffer, stTotalSize, *pstOft,
                               "%-44s %-44s %-8s %-2s %7d %7d %7d\n",
                               __ProtoAddrBuild(&pcb->local_ip, pcb->netif_idx, pcb->local_port,
                                                cBuffer1, sizeof(cBuffer1)),
                               __ProtoAddrBuild(&pcb->remote_ip, pcb->netif_idx, pcb->remote_port,
                                                cBuffer2, sizeof(cBuffer2)),
                               __TcpGetStat((u8_t)pcb->state),
                               ip_get_option(pcb, SO_KEEPALIVE) ? "*" : "",
                               (u32_t)pcb->nrtx, (u32_t)pcb->rcv_wnd, (u32_t)pcb->snd_wnd);
        }
#endif                                                                  /*  LWIP_IPV6                   */
    }
}
/*********************************************************************************************************
** ��������: __TcpKaPrint
** ��������: ��ӡ���� tcp KEEPALIVE �ļ�
** �䡡��  : pcb           tcp ���ƿ�
**           pcBuffer      ����
**           stTotalSize   ��������С
**           pstOft        ��ǰƫ����
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static VOID  __TcpKaPrint (struct tcp_pcb *pcb, PCHAR  pcBuffer,
                           size_t  stTotalSize, size_t *pstOft)
{
    CHAR    cBuffer1[INET6_ADDRSTRLEN];
    CHAR    cBuffer2[INET6_ADDRSTRLEN];

    if (IP_IS_V4_VAL(pcb->local_ip)) {
        *pstOft = bnprintf(pcBuffer, stTotalSize, *pstOft,
                           "%-21s %-21s %-8s %7d %8d %3d\n",
                           __ProtoAddrBuild(&pcb->local_ip, pcb->netif_idx, pcb->local_port,
                                            cBuffer1, sizeof(cBuffer1)),
                           __ProtoAddrBuild(&pcb->remote_ip, pcb->netif_idx, pcb->remote_port,
                                            cBuffer2, sizeof(cBuffer2)),
                           __TcpGetStat((u8_t)pcb->state),
                           (u32_t)pcb->keep_idle, (u32_t)pcb->keep_intvl, (u32_t)pcb->keep_cnt);

#if LWIP_IPV6
    } else {
        *pstOft = bnprintf(pcBuffer, stTotalSize, *pstOft,
                           "%-44s %-44s %-8s %7d %8d %3d\n",
                           __ProtoAddrBuild(&pcb->local_ip, pcb->netif_idx, pcb->local_port,
                                            cBuffer1, sizeof(cBuffer1)),
                           __ProtoAddrBuild(&pcb->remote_ip, pcb->netif_idx, pcb->remote_port,
                                            cBuffer2, sizeof(cBuffer2)),
                           __TcpGetStat((u8_t)pcb->state),
                           (u32_t)pcb->keep_idle, (u32_t)pcb->keep_intvl, (u32_t)pcb->keep_cnt);
#endif                                                                  /*  LWIP_IPV6                   */
    }
}

#endif                                                                  /*  LWIP_TCP                    */
/*********************************************************************************************************
** ��������: __tshellNetstatTcpListen
** ��������: ϵͳ���� "netstat -tl" (ֻ��ʾ listen ״̬ tcp)
** �䡡��  : iNetType      �������� 
** �䡡��  : ERROR_NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
VOID  __tshellNetstatTcpListen (INT  iNetType)
{
#if LWIP_TCP
    const CHAR      cTcpInfoHdr[] = 
    "LOCAL                 REMOTE                STATUS   RETRANS RCV_WND SND_WND\n";
    
    const CHAR      cTcp6InfoHdr[] = 
    "\nLOCAL6                                       REMOTE6                                      "
    "STATUS   IPv6-ONLY RETRANS RCV_WND SND_WND\n";
    
    struct tcp_pcb  *pcb;
    PCHAR            pcPrintBuf;
    size_t           stRealSize = 0;
    size_t           stNeedBufferSize = 0;
    BOOL             b4 = LW_FALSE, b6 = LW_FALSE;
    
    LOCK_TCPIP_CORE();
    if (__NETSTAT_INC_IPV4(iNetType)) {
        for (pcb = tcp_listen_pcbs.pcbs; pcb != NULL; pcb = pcb->next) {
            if (IP_IS_V4_VAL(pcb->local_ip)) {
                stNeedBufferSize += 128;
                b4 = LW_TRUE;
            }
        }
    }
    if (__NETSTAT_INC_IPV6(iNetType)) {
        for (pcb = tcp_listen_pcbs.pcbs; pcb != NULL; pcb = pcb->next) {
            if (!IP_IS_V4_VAL(pcb->local_ip)) {
                stNeedBufferSize += 192;
                b6 = LW_TRUE;
            }
        }
    }
    UNLOCK_TCPIP_CORE();
    
    if (stNeedBufferSize == 0) {
        return;
    }
    
    stNeedBufferSize += sizeof(cTcpInfoHdr) + sizeof(cTcp6InfoHdr);
    
    pcPrintBuf = (PCHAR)__SHEAP_ALLOC(stNeedBufferSize);
    if (!pcPrintBuf) {
        fprintf(stderr, "no memory!\n");
        _ErrorHandle(ENOMEM);
        return;
    }
    
    printf("--TCP LISTEN--:\n");
    
    LOCK_TCPIP_CORE();
    if (__NETSTAT_INC_IPV4(iNetType) && b4) {
        stRealSize = bnprintf(pcPrintBuf, stNeedBufferSize, stRealSize, 
                              cTcpInfoHdr);
        for (pcb = tcp_listen_pcbs.pcbs; pcb != NULL; pcb = pcb->next) {
            if (IP_IS_V4_VAL(pcb->local_ip)) {
                __TcpPrint(pcb, pcPrintBuf, stNeedBufferSize, &stRealSize);
            }
        }
    }
    if (__NETSTAT_INC_IPV6(iNetType) && b6) {
        stRealSize = bnprintf(pcPrintBuf, stNeedBufferSize, stRealSize, 
                              cTcp6InfoHdr);
        for (pcb = tcp_listen_pcbs.pcbs; pcb != NULL; pcb = pcb->next) {
            if (!IP_IS_V4_VAL(pcb->local_ip)) {
                __TcpPrint(pcb, pcPrintBuf, stNeedBufferSize, &stRealSize);
            }
        }
    }
    UNLOCK_TCPIP_CORE();
    
    printf("%s\n", pcPrintBuf);
    
    __SHEAP_FREE(pcPrintBuf);
#endif                                                                  /*  LWIP_TCP                    */
}
/*********************************************************************************************************
** ��������: __tshellNetstatTcp
** ��������: ϵͳ���� "netstat -t"
** �䡡��  : iNetType      ��������
** �䡡��  : ERROR_NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
VOID  __tshellNetstatTcp (INT  iNetType)
{
#if LWIP_TCP
    const CHAR      cTcpInfoHdr[] = 
    "LOCAL                 REMOTE                STATUS   KA RETRANS RCV_WND SND_WND\n";
    
    const CHAR      cTcp6InfoHdr[] = 
    "\nLOCAL6                                       REMOTE6                                      "
    "STATUS   KA RETRANS RCV_WND SND_WND\n";
    
    struct tcp_pcb  *pcb;
    PCHAR            pcPrintBuf;
    size_t           stRealSize = 0;
    size_t           stNeedBufferSize = 0;
    BOOL             b4 = LW_FALSE, b6 = LW_FALSE;
    
    LOCK_TCPIP_CORE();
    if (__NETSTAT_INC_IPV4(iNetType)) {
        for (pcb = tcp_active_pcbs; pcb != NULL; pcb = pcb->next) {
            if (IP_IS_V4_VAL(pcb->local_ip)) {
                stNeedBufferSize += 128;
                b4 = LW_TRUE;
            }
        }
        for (pcb = tcp_tw_pcbs; pcb != NULL; pcb = pcb->next) {
            if (IP_IS_V4_VAL(pcb->local_ip)) {
                stNeedBufferSize += 192;
                b4 = LW_TRUE;
            }
        }
    }
    if (__NETSTAT_INC_IPV6(iNetType)) {
        for (pcb = tcp_active_pcbs; pcb != NULL; pcb = pcb->next) {
            if (!IP_IS_V4_VAL(pcb->local_ip)) {
                stNeedBufferSize += 128;
                b6 = LW_TRUE;
            }
        }
        for (pcb = tcp_tw_pcbs; pcb != NULL; pcb = pcb->next) {
            if (!IP_IS_V4_VAL(pcb->local_ip)) {
                stNeedBufferSize += 192;
                b6 = LW_TRUE;
            }
        }
    }
    UNLOCK_TCPIP_CORE();
    
    if (stNeedBufferSize == 0) {
        return;
    }
    
    stNeedBufferSize += sizeof(cTcpInfoHdr) + sizeof(cTcp6InfoHdr);
    
    pcPrintBuf = (PCHAR)__SHEAP_ALLOC(stNeedBufferSize);
    if (!pcPrintBuf) {
        fprintf(stderr, "no memory!\n");
        _ErrorHandle(ENOMEM);
        return;
    }
    
    printf("--TCP--:\n");
    
    LOCK_TCPIP_CORE();
    if (__NETSTAT_INC_IPV4(iNetType) && b4) {
        stRealSize = bnprintf(pcPrintBuf, stNeedBufferSize, stRealSize, 
                              cTcpInfoHdr);
        for (pcb = tcp_active_pcbs; pcb != NULL; pcb = pcb->next) {
            if (IP_IS_V4_VAL(pcb->local_ip)) {
                __TcpPrint(pcb, pcPrintBuf, stNeedBufferSize, &stRealSize);
            }
        }
        for (pcb = tcp_tw_pcbs; pcb != NULL; pcb = pcb->next) {
            if (IP_IS_V4_VAL(pcb->local_ip)) {
                __TcpPrint(pcb, pcPrintBuf, stNeedBufferSize, &stRealSize);
            }
        }
    }
    if (__NETSTAT_INC_IPV6(iNetType) && b6) {
        stRealSize = bnprintf(pcPrintBuf, stNeedBufferSize, stRealSize, 
                              cTcp6InfoHdr);
        for (pcb = tcp_active_pcbs; pcb != NULL; pcb = pcb->next) {
            if (!IP_IS_V4_VAL(pcb->local_ip)) {
                __TcpPrint(pcb, pcPrintBuf, stNeedBufferSize, &stRealSize);
            }
        }
        for (pcb = tcp_tw_pcbs; pcb != NULL; pcb = pcb->next) {
            if (!IP_IS_V4_VAL(pcb->local_ip)) {
                __TcpPrint(pcb, pcPrintBuf, stNeedBufferSize, &stRealSize);
            }
        }
    }
    UNLOCK_TCPIP_CORE();
    
    printf("%s\n", pcPrintBuf);
    
    __SHEAP_FREE(pcPrintBuf);
#endif                                                                  /*  LWIP_TCP                    */
}
/*********************************************************************************************************
** ��������: __UdpPrint
** ��������: ��ӡ���� udp �ļ�
** �䡡��  : iNetType      ��������
** �䡡��  : ERROR_NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
VOID  __tshellNetstatKeepalive (INT  iNetType)
{
#if LWIP_TCP
    const CHAR      cTcpInfoHdr[] =
    "LOCAL                 REMOTE                STATUS      IDLE INTERVAL CNT\n";

    const CHAR      cTcp6InfoHdr[] =
    "\nLOCAL6                                       REMOTE6                                      "
    "STATUS      IDLE INTERVAL CNT\n";

    struct tcp_pcb  *pcb;
    PCHAR            pcPrintBuf;
    size_t           stRealSize = 0;
    size_t           stNeedBufferSize = 0;
    BOOL             b4 = LW_FALSE, b6 = LW_FALSE;

    LOCK_TCPIP_CORE();
    if (__NETSTAT_INC_IPV4(iNetType)) {
        for (pcb = tcp_active_pcbs; pcb != NULL; pcb = pcb->next) {
            if (IP_IS_V4_VAL(pcb->local_ip) && ip_get_option(pcb, SO_KEEPALIVE)) {
                stNeedBufferSize += 128;
                b4 = LW_TRUE;
            }
        }
    }
    if (__NETSTAT_INC_IPV6(iNetType)) {
        for (pcb = tcp_active_pcbs; pcb != NULL; pcb = pcb->next) {
            if (!IP_IS_V4_VAL(pcb->local_ip) && ip_get_option(pcb, SO_KEEPALIVE)) {
                stNeedBufferSize += 128;
                b6 = LW_TRUE;
            }
        }
    }
    UNLOCK_TCPIP_CORE();

    if (stNeedBufferSize == 0) {
        return;
    }

    stNeedBufferSize += sizeof(cTcpInfoHdr) + sizeof(cTcp6InfoHdr);

    pcPrintBuf = (PCHAR)__SHEAP_ALLOC(stNeedBufferSize);
    if (!pcPrintBuf) {
        fprintf(stderr, "no memory!\n");
        _ErrorHandle(ENOMEM);
        return;
    }

    printf("--KEEPALIVE--:\n");

    LOCK_TCPIP_CORE();
    if (__NETSTAT_INC_IPV4(iNetType) && b4) {
        stRealSize = bnprintf(pcPrintBuf, stNeedBufferSize, stRealSize,
                              cTcpInfoHdr);
        for (pcb = tcp_active_pcbs; pcb != NULL; pcb = pcb->next) {
            if (IP_IS_V4_VAL(pcb->local_ip) && ip_get_option(pcb, SO_KEEPALIVE)) {
                __TcpKaPrint(pcb, pcPrintBuf, stNeedBufferSize, &stRealSize);
            }
        }
    }
    if (__NETSTAT_INC_IPV6(iNetType) && b6) {
        stRealSize = bnprintf(pcPrintBuf, stNeedBufferSize, stRealSize,
                              cTcp6InfoHdr);
        for (pcb = tcp_active_pcbs; pcb != NULL; pcb = pcb->next) {
            if (!IP_IS_V4_VAL(pcb->local_ip) && ip_get_option(pcb, SO_KEEPALIVE)) {
                __TcpKaPrint(pcb, pcPrintBuf, stNeedBufferSize, &stRealSize);
            }
        }
    }
    UNLOCK_TCPIP_CORE();

    printf("%s\n", pcPrintBuf);

    __SHEAP_FREE(pcPrintBuf);
#endif                                                                  /*  LWIP_TCP                    */
}
/*********************************************************************************************************
** ��������: __UdpPrint
** ��������: ��ӡ���� udp �ļ�
** �䡡��  : pcb           tcp ���ƿ�
**           pcBuffer      ����
**           stTotalSize   ��������С
**           pstOft        ��ǰƫ����
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
#if LWIP_UDP

static VOID  __UdpPrint (struct udp_pcb *pcb, PCHAR  pcBuffer, 
                         size_t  stTotalSize, size_t *pstOft)
{
    CHAR    cBuffer1[INET6_ADDRSTRLEN];
    CHAR    cBuffer2[INET6_ADDRSTRLEN];

    if (IP_IS_V4_VAL(pcb->local_ip)) {
        *pstOft = bnprintf(pcBuffer, stTotalSize, *pstOft,
                           "%-21s %-21s %s\n",
                           __ProtoAddrBuild(&pcb->local_ip, pcb->netif_idx, pcb->local_port,
                                            cBuffer1, sizeof(cBuffer1)),
                           __ProtoAddrBuild(&pcb->remote_ip, pcb->netif_idx, pcb->remote_port,
                                            cBuffer2, sizeof(cBuffer2)),
                           (pcb->flags & UDP_FLAGS_UDPLITE) ? "yes" : "no");
#if LWIP_IPV6
    } else {
        *pstOft = bnprintf(pcBuffer, stTotalSize, *pstOft,
                           "%-44s %-44s %s\n",
                           __ProtoAddrBuild(&pcb->local_ip, pcb->netif_idx, pcb->local_port,
                                            cBuffer1, sizeof(cBuffer1)),
                           __ProtoAddrBuild(&pcb->remote_ip, pcb->netif_idx, pcb->remote_port,
                                            cBuffer2, sizeof(cBuffer2)),
                           (pcb->flags & UDP_FLAGS_UDPLITE) ? "yes" : "no");
#endif                                                                  /*  LWIP_IPV6                   */
    }
}

#endif                                                                  /*  LWIP_UDP                    */
/*********************************************************************************************************
** ��������: __tshellNetstatUdp
** ��������: ϵͳ���� "netstat -u"
** �䡡��  : iNetType      ��������
** �䡡��  : ERROR_NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
VOID  __tshellNetstatUdp (INT  iNetType)
{
#if LWIP_UDP
    const CHAR      cUdpInfoHdr[] = 
    "LOCAL                 REMOTE                UDPLITE\n";
    
    const CHAR      cUdp6InfoHdr[] = 
    "\nLOCAL6                                       REMOTE6                                      UDPLITE\n";
    
    struct udp_pcb  *pcb;
    PCHAR            pcPrintBuf;
    size_t           stRealSize = 0;
    size_t           stNeedBufferSize = 0;
    BOOL             b4 = LW_FALSE, b6 = LW_FALSE;

    LOCK_TCPIP_CORE();
    if (__NETSTAT_INC_IPV4(iNetType)) {
        for (pcb = udp_pcbs; pcb != NULL; pcb = pcb->next) {
            if (IP_IS_V4_VAL(pcb->local_ip)) {
                stNeedBufferSize += 64;
                b4 = LW_TRUE;
            }
        }
    }
    if (__NETSTAT_INC_IPV6(iNetType)) {
        for (pcb = udp_pcbs; pcb != NULL; pcb = pcb->next) {
            if (!IP_IS_V4_VAL(pcb->local_ip)) {
                stNeedBufferSize += 128;
                b6 = LW_TRUE;
            }
        }
    }
    UNLOCK_TCPIP_CORE();
    
    if (stNeedBufferSize == 0) {
        return;
    }
    
    stNeedBufferSize += sizeof(cUdpInfoHdr) + sizeof(cUdp6InfoHdr);
    
    pcPrintBuf = (PCHAR)__SHEAP_ALLOC(stNeedBufferSize);
    if (!pcPrintBuf) {
        fprintf(stderr, "no memory!\n");
        _ErrorHandle(ENOMEM);
        return;
    }

    printf("--UDP--:\n");
    
    LOCK_TCPIP_CORE();
    if (__NETSTAT_INC_IPV4(iNetType) && b4) {
        stRealSize = bnprintf(pcPrintBuf, stNeedBufferSize, stRealSize, cUdpInfoHdr);
        for (pcb = udp_pcbs; pcb != NULL; pcb = pcb->next) {
            if (IP_IS_V4_VAL(pcb->local_ip)) {
                __UdpPrint(pcb, pcPrintBuf, stNeedBufferSize, &stRealSize);
            }
        }
    }
    if (__NETSTAT_INC_IPV6(iNetType) && b6) {
        stRealSize = bnprintf(pcPrintBuf, stNeedBufferSize, stRealSize, 
                              cUdp6InfoHdr);
        for (pcb = udp_pcbs; pcb != NULL; pcb = pcb->next) {
            if (!IP_IS_V4_VAL(pcb->local_ip)) {
                __UdpPrint(pcb, pcPrintBuf, stNeedBufferSize, &stRealSize);
            }
        }
    }
    UNLOCK_TCPIP_CORE();
    
    printf("%s\n", pcPrintBuf);
    
    __SHEAP_FREE(pcPrintBuf);
#endif                                                                  /*  LWIP_UDP                    */
}
/*********************************************************************************************************
** ��������: __tshellNetstatUnix
** ��������: ϵͳ���� "netstat -x"
** �䡡��  : iNetType      ��������
** �䡡��  : ERROR_NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
VOID  __tshellNetstatUnix (INT  iNetType)
{
    INT     iFd;
    CHAR    cBuffer[1024];
    ssize_t sstNum;
    
    if (!__NETSTAT_INC_UNIX(iNetType)) {
        return;
    }
    
    printf("--UNIX--:\n");
    fflush(stdout);
    
    iFd = open("/proc/net/unix", O_RDONLY);
    if (iFd < 0) {
        fprintf(stderr, "can not open /proc/net/unix : %s\n", lib_strerror(errno));
        return;
    }
    
    do {
        sstNum = read(iFd, cBuffer, sizeof(cBuffer));
        if (sstNum > 0) {
            write(STDOUT_FILENO, cBuffer, (size_t)sstNum);
        }
    } while (sstNum > 0);
    
    close(iFd);

    printf("\n");
}
/*********************************************************************************************************
** ��������: __tshellNetstatPacket
** ��������: ϵͳ���� "netstat -p"
** �䡡��  : iNetType      ��������
** �䡡��  : ERROR_NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
VOID  __tshellNetstatPacket (INT  iNetType)
{
    INT     iFd;
    CHAR    cBuffer[1024];
    ssize_t sstNum;
    
    printf("--PACKET--:\n");
    fflush(stdout);
    
    iFd = open("/proc/net/packet", O_RDONLY);
    if (iFd < 0) {
        fprintf(stderr, "can not open /proc/net/packet : %s\n", lib_strerror(errno));
        return;
    }
    
    do {
        sstNum = read(iFd, cBuffer, sizeof(cBuffer));
        if (sstNum > 0) {
            write(STDOUT_FILENO, cBuffer, (size_t)sstNum);
        }
    } while (sstNum > 0);
    
    close(iFd);

    printf("\n");
}

#endif                                                                  /*  LW_CFG_NET_EN > 0           */
                                                                        /*  LW_CFG_SHELL_EN > 0         */
/*********************************************************************************************************
  END
*********************************************************************************************************/
