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
** ��   ��   ��: af_route.h
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2017 �� 12 �� 08 ��
**
** ��        ��: AF_ROUTE ֧��.
*********************************************************************************************************/

#ifndef __AF_ROUTE_H
#define __AF_ROUTE_H

#if LW_CFG_NET_EN > 0 && LW_CFG_NET_ROUTER > 0

#ifdef __SYLIXOS_RTHOOK
#include "ip4_route.h"
#include "ip6_route.h"
#endif                                                                  /*  __SYLIXOS_RTHOOK            */

/*********************************************************************************************************
  AF_ROUTE ���ն���
*********************************************************************************************************/

typedef struct af_route_node {                                          /*  һ����Ϣ�ڵ�                */
    LW_LIST_MONO        RTM_monoManage;                                 /*  ������������                */
    size_t              RTM_stLen;                                      /*  ��Ϣ����                    */
} AF_ROUTE_N;

typedef struct af_route_queue {
    PLW_LIST_MONO       RTQ_pmonoHeader;                                /*  ��Ϣ����ͷ                  */
    PLW_LIST_MONO       RTQ_pmonoTail;                                  /*  ��Ϣ���н�β                */
    size_t              RTQ_stTotal;                                    /*  ����Ч�����ֽ���            */
} AF_ROUTE_Q;

/*********************************************************************************************************
  AF_ROUTE ���ƿ�
*********************************************************************************************************/

typedef struct af_route_t {
    LW_LIST_LINE        ROUTE_lineManage;
    INT                 ROUTE_iFlag;                                    /*  �򿪷�ʽ                    */
    
#define __AF_ROUTE_SHUTD_R      0x01
#define __AF_ROUTE_SHUTD_W      0x02
    INT                 ROUTE_iShutDFlag;                               /*  ��ǰ shutdown ״̬          */
    
    LW_OBJECT_HANDLE    ROUTE_hCanRead;                                 /*  �Ƿ��ܶ�                    */
    ULONG               ROUTE_ulRecvTimeout;                            /*  ���ճ�ʱ                    */
    
    AF_ROUTE_Q          ROUTE_rtq;                                      /*  ������Ϣ����                */
    size_t              ROUTE_stMaxBufSize;                             /*  �����ջ����С            */
    
    PVOID               ROUTE_sockFile;                                 /*  socket file                 */
} AF_ROUTE_T;

/*********************************************************************************************************
  �ڲ�����
*********************************************************************************************************/

VOID         route_init(VOID);
AF_ROUTE_T  *route_socket(INT  iDomain, INT  iType, INT  iProtocol);
INT          route_close(AF_ROUTE_T  *pafroute);
ssize_t      route_send(AF_ROUTE_T  *pafroute, const void *data, size_t size, int flags);
ssize_t      route_sendmsg(AF_ROUTE_T  *pafroute, const struct msghdr *msg, int flags);
ssize_t      route_recv(AF_ROUTE_T  *pafroute, void *mem, size_t len, int flags);
ssize_t      route_recvmsg(AF_ROUTE_T  *pafroute, struct msghdr *msg, int flags);
INT          route_setsockopt(AF_ROUTE_T  *pafroute, int level, int optname, const void *optval, socklen_t optlen);
INT          route_getsockopt(AF_ROUTE_T  *pafroute, int level, int optname, void *optval, socklen_t *optlen);
INT          route_shutdown(AF_ROUTE_T  *pafroute, int how);
INT          route_ioctl(AF_ROUTE_T  *pafroute, INT  iCmd, PVOID  pvArg);

/*********************************************************************************************************
  ��Ϣ�ӿ� (iAddrType: 0: ipaddr 1: netmask)
*********************************************************************************************************/

#ifdef __SYLIXOS_RTHOOK
VOID         route_hook_rt_ipv4(u_char type, const struct rt_entry *pentry, int nolock);
#if LWIP_IPV6
VOID         route_hook_rt_ipv6(u_char type, const struct rt6_entry *pentry, int nolock);
#endif                                                                  /*  LWIP_IPV6                   */

VOID         route_hook_netif_ipv4(struct netif *pnetif, const ip4_addr_t *pipaddr, const ip4_addr_t *pnetmask, u_char type);
#if LWIP_IPV6
VOID         route_hook_netif_ipv6(struct netif *pnetif, const ip6_addr_t *pipaddr, u_char type);
#endif                                                                  /*  LWIP_IPV6                   */

VOID         route_hook_maddr_ipv4(struct netif *pnetif, const ip4_addr_t *pipaddr, u_char type);
#if LWIP_IPV6
VOID         route_hook_maddr_ipv6(struct netif *pnetif, const ip6_addr_t *pip6addr, u_char type);
#endif                                                                  /*  LWIP_IPV6                   */

VOID         route_hook_netif_ann(struct netif *pnetif, int what);

VOID         route_hook_netif_updown(struct netif *pnetif);
#endif                                                                  /*  __SYLIXOS_RTHOOK            */

#endif                                                                  /*  LW_CFG_NET_EN > 0           */
                                                                        /*  LW_CFG_NET_ROUTER > 0       */
#endif                                                                  /*  __AF_ROUTE_H                */
/*********************************************************************************************************
  END
*********************************************************************************************************/
