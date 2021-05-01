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
** ��   ��   ��: af_route.c
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2017 �� 12 �� 08 ��
**
** ��        ��: AF_ROUTE ֧��.
*********************************************************************************************************/
#define  __SYLIXOS_RTHOOK
#define  __SYLIXOS_KERNEL
#include "SylixOS.h"
/*********************************************************************************************************
  �ü�����
*********************************************************************************************************/
#if LW_CFG_NET_EN > 0 && LW_CFG_NET_ROUTER > 0
#include "net/if.h"
#include "net/if_dl.h"
#include "net/if_flags.h"
#include "net/if_types.h"
#include "net/if_ether.h"
#include "net/route.h"
#include "lwip/mem.h"
#include "lwip/netif.h"
#include "lwip/tcpip.h"
#include "netdev.h"
#include "af_route.h"
/*********************************************************************************************************
  ��������
*********************************************************************************************************/
extern void  __socketEnotify(void *file, LW_SEL_TYPE type, INT  iSoErr);
/*********************************************************************************************************
  ȫ�ֱ���
*********************************************************************************************************/
static LW_LIST_LINE_HEADER          _G_plineAfRoute;
/*********************************************************************************************************
  AF_ROUTE ��
*********************************************************************************************************/
#define __AF_ROUTE_LOCK()           RT_LOCK()
#define __AF_ROUTE_UNLOCK()         RT_UNLOCK()
/*********************************************************************************************************
  AF_ROUTE ������
*********************************************************************************************************/
#define __AF_ROUTE_DEF_BUFSIZE      (LW_CFG_KB_SIZE * 64)               /*  Ĭ��Ϊ 64K ���ջ���         */
#define __AF_ROUTE_DEF_BUFMAX       (LW_CFG_KB_SIZE * 256)              /*  Ĭ��Ϊ 256K ���ջ���        */
#define __AF_ROUTE_DEF_BUFMIN       (LW_CFG_KB_SIZE * 8)                /*  ��С���ջ����С            */
/*********************************************************************************************************
  AF_ROUTE ֪ͨ
*********************************************************************************************************/
#define __AF_ROUTE_WAIT(pafroute)   API_SemaphoreBPend(pafroute->ROUTE_hCanRead, LW_OPTION_WAIT_INFINITE)
#define __AF_ROUTE_NOTIFY(pafroute) API_SemaphoreBPost(pafroute->ROUTE_hCanRead)
/*********************************************************************************************************
  �ȴ��ж�
*********************************************************************************************************/
#define __AF_ROUTE_IS_NBIO(pafroute, flags)  \
        ((pafroute->ROUTE_iFlag & O_NONBLOCK) || (flags & MSG_DONTWAIT))
/*********************************************************************************************************
** ��������: __routeMsToTicks
** ��������: ����ת��Ϊ ticks
** �䡡��  : ulMs        ����
** �䡡��  : tick
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static ULONG  __routeMsToTicks (ULONG  ulMs)
{
    ULONG   ulTicks;
    
    if (ulMs == 0) {
        ulTicks = LW_OPTION_WAIT_INFINITE;
    } else {
        ulTicks = LW_MSECOND_TO_TICK_1(ulMs);
    }
    
    return  (ulTicks);
}
/*********************************************************************************************************
** ��������: __routeTvToTicks
** ��������: timeval ת��Ϊ ticks
** �䡡��  : ptv       ʱ��
** �䡡��  : tick
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static ULONG  __routeTvToTicks (struct timeval  *ptv)
{
    ULONG   ulTicks;

    if ((ptv->tv_sec == 0) && (ptv->tv_usec == 0)) {
        return  (LW_OPTION_WAIT_INFINITE);
    }
    
    ulTicks = __timevalToTick(ptv);
    if (ulTicks == 0) {
        ulTicks = 1;
    }
    
    return  (ulTicks);
}
/*********************************************************************************************************
** ��������: __routeTicksToMs
** ��������: ticks ת��Ϊ����
** �䡡��  : ulTicks       tick
** �䡡��  : ����
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static ULONG  __routeTicksToMs (ULONG  ulTicks)
{
    ULONG  ulMs;
    
    if (ulTicks == LW_OPTION_WAIT_INFINITE) {
        ulMs = 0;
    } else {
        ulMs = (ulTicks * 1000) / LW_TICK_HZ;
    }
    
    return  (ulMs);
}
/*********************************************************************************************************
** ��������: __routeTicksToTv
** ��������: ticks ת��Ϊ timeval
** �䡡��  : ulTicks       tick
** �䡡��  : timeval
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static VOID __routeTicksToTv (ULONG  ulTicks, struct timeval *ptv)
{
    if (ulTicks == LW_OPTION_WAIT_INFINITE) {
        ptv->tv_sec  = 0;
        ptv->tv_usec = 0;
    } else {
        __tickToTimeval(ulTicks, ptv);
    }
}
/*********************************************************************************************************
** ��������: __routeDeleteBuf
** ��������: ɾ�� route �׽��ֻ���
** �䡡��  : pafroute  route file
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static VOID  __routeDeleteBuf (AF_ROUTE_T  *pafroute)
{
    AF_ROUTE_N    *prouten;
    AF_ROUTE_Q    *prouteq;
    
    prouteq = &pafroute->ROUTE_rtq;
    
    while (prouteq->RTQ_pmonoHeader) {
        prouten = (AF_ROUTE_N *)prouteq->RTQ_pmonoHeader;
        _list_mono_allocate_seq(&prouteq->RTQ_pmonoHeader,
                                &prouteq->RTQ_pmonoTail);
        mem_free(prouten);
    }
    
    prouteq->RTQ_stTotal = 0;
}
/*********************************************************************************************************
** ��������: __routeUpdateReader
** ��������: ֪ͨ route �׽��ֿɶ�
** �䡡��  : pafroute  route file
**           iSoErr    ��������
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static VOID  __routeUpdateReader (AF_ROUTE_T  *pafroute, INT  iSoErr)
{
    __AF_ROUTE_NOTIFY(pafroute);
    
    __socketEnotify(pafroute->ROUTE_sockFile, SELREAD, iSoErr);         /*  ���� select �ɶ�            */
}
/*********************************************************************************************************
** ��������: __routeRtmAdd4
** ��������: ���һ�� IPv4 ·��
** �䡡��  : pmsghdr   ·����Ϣ
** �䡡��  : д��������
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static ssize_t  __routeRtmAdd4 (struct rt_msghdr  *pmsghdr)
{
    INT               iRet;
    struct rt_entry  *pentry;
    
    if ((pmsghdr->rtm_addrs & (RTA_DST | RTA_GATEWAY)) != (RTA_DST | RTA_GATEWAY)) {
        _ErrorHandle(EINVAL);
        return  (0);
    }
    
    pentry = (struct rt_entry *)mem_malloc(sizeof(struct rt_entry));
    if (!pentry) {
        _ErrorHandle(ENOMEM);
        return  (0);
    }
    lib_bzero(pentry, sizeof(struct rt_entry));
    
    if (rt_msghdr_to_entry(pentry, pmsghdr) < 0) {                      /*  ת��Ϊ rt_entry             */
        mem_free(pentry);
        _ErrorHandle(EINVAL);
        return  (0);
    }
    
    if (pentry->rt_dest.addr == IPADDR_ANY) {
        if ((pentry->rt_gateway.addr == IPADDR_ANY) && (pentry->rt_ifname[0] == '\0')) {
            mem_free(pentry);
            _ErrorHandle(ENETUNREACH);
            return  (0);
        }
        
        iRet = rt_change_default(&pentry->rt_gateway, pentry->rt_ifname);
        if (iRet < 0) {
            mem_free(pentry);
            _ErrorHandle(ENETUNREACH);
            return  (0);
        }
        
        route_hook_rt_ipv4(RTM_ADD, pentry, 1);                         /*  ��ӽ��ջ���                */
        mem_free(pentry);
    
    } else {
        if (rt_find_entry(&pentry->rt_dest, &pentry->rt_netmask, &pentry->rt_gateway, 
                          pentry->rt_ifname, pentry->rt_flags)) {
            mem_free(pentry);
            _ErrorHandle(EEXIST);
            return  (0);
        }
        
        iRet = rt_add_entry(pentry);                                    /*  ���·����Ŀ                */
        if (iRet < 0) {
            mem_free(pentry);
            _ErrorHandle(ENETUNREACH);
            return  (0);
        
        } else {
            route_hook_rt_ipv4(RTM_ADD, pentry, 1);                     /*  ��ӽ��ջ���                */
        }
    }
        
    return  ((ssize_t)pmsghdr->rtm_msglen);
}
/*********************************************************************************************************
** ��������: __routeRtmAdd6
** ��������: ���һ�� IPv6 ·��
** �䡡��  : pmsghdr   ·����Ϣ
** �䡡��  : д��������
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
#if LWIP_IPV6

static ssize_t  __routeRtmAdd6 (struct rt_msghdr  *pmsghdr)
{
    INT                iRet;
    struct rt6_entry  *pentry6;
    
    if ((pmsghdr->rtm_addrs & (RTA_DST | RTA_GATEWAY)) != (RTA_DST | RTA_GATEWAY)) {
        _ErrorHandle(EINVAL);
        return  (0);
    }
    
    pentry6 = (struct rt6_entry *)mem_malloc(sizeof(struct rt6_entry));
    if (!pentry6) {
        _ErrorHandle(ENOMEM);
        return  (0);
    }
    lib_bzero(pentry6, sizeof(struct rt6_entry));
    
    if (rt6_msghdr_to_entry(pentry6, pmsghdr) < 0) {                    /*  ת��Ϊ rt6_entry            */
        mem_free(pentry6);
        _ErrorHandle(EINVAL);
        return  (0);
    }
    
    if (ip6_addr_isany_val(pentry6->rt6_dest)) {                        /*  ����Ĭ������                */
        if (pentry6->rt6_ifname[0] == '\0') {
            mem_free(pentry6);
            _ErrorHandle(ENETUNREACH);
            return  (0);
        }
        
        iRet = rt6_change_default(&pentry6->rt6_gateway, pentry6->rt6_ifname);
        if (iRet < 0) {
            mem_free(pentry6);
            _ErrorHandle(ENETUNREACH);
            return  (0);
        }
        
        route_hook_rt_ipv6(RTM_ADD, pentry6, 1);                        /*  ��ӽ��ջ���                */
        mem_free(pentry6);
    
    } else {
        if (rt6_find_entry(&pentry6->rt6_dest, &pentry6->rt6_netmask, &pentry6->rt6_gateway, 
                           pentry6->rt6_ifname, pentry6->rt6_flags)) {
            mem_free(pentry6);
            _ErrorHandle(EEXIST);
            return  (0);
        }
        
        iRet = rt6_add_entry(pentry6);                                  /*  ���·����Ŀ                */
        if (iRet < 0) {
            mem_free(pentry6);
            _ErrorHandle(ENETUNREACH);
            return  (0);
        
        } else {
            route_hook_rt_ipv6(RTM_ADD, pentry6, 1);                    /*  ��ӽ��ջ���                */
        }
    }
        
    return  ((ssize_t)pmsghdr->rtm_msglen);
}

#endif                                                                  /*  LWIP_IPV6                   */
/*********************************************************************************************************
** ��������: __routeRtmChange4
** ��������: �޸�һ�� IPv4 ·��
** �䡡��  : pmsghdr   ·����Ϣ
** �䡡��  : д��������
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static ssize_t  __routeRtmChange4 (struct rt_msghdr  *pmsghdr)
{
    INT                 iRet;
    struct netif       *pnetif;
    struct rt_entry    *pentry;
    struct sockaddr_in *psockaddrin;
    ip4_addr_t          ipaddr;
    ip4_addr_t          ipnetmask;
    ip4_addr_t          ipgateway;
    CHAR                cIfName[IF_NAMESIZE] = "";
    
    if ((pmsghdr->rtm_addrs & (RTA_DST | RTA_GATEWAY)) != (RTA_DST | RTA_GATEWAY)) {
        _ErrorHandle(EINVAL);
        return  (0);
    }
    
    pnetif = netif_get_by_index(pmsghdr->rtm_index);
    if (pnetif) {
        netif_get_name(pnetif, cIfName);
    }
    
    psockaddrin = (struct sockaddr_in *)(pmsghdr + 1);
    ipaddr.addr = psockaddrin->sin_addr.s_addr;
    
    psockaddrin = SA_NEXT(struct sockaddr_in *, psockaddrin);
    ipgateway.addr = psockaddrin->sin_addr.s_addr;
    
    if (pmsghdr->rtm_addrs & RTA_NETMASK) {
        psockaddrin = SA_NEXT(struct sockaddr_in *, psockaddrin);
        ipnetmask.addr = psockaddrin->sin_addr.s_addr;
    } else {
        ipnetmask.addr = IPADDR_ANY;
    }
    
    if (ipaddr.addr == IPADDR_ANY) {                                    /*  ����Ĭ��·��                */
        if ((ipgateway.addr == IPADDR_ANY) && (cIfName[0] == '\0')) {
            _ErrorHandle(ENETUNREACH);
            return  (0);
        }
        
        iRet = rt_change_default(&ipgateway, cIfName);
        if (iRet < 0) {
            _ErrorHandle(ENETUNREACH);
            return  (0);
        }
        
        pentry = (struct rt_entry *)mem_malloc(sizeof(struct rt_entry));
        if (pentry) {
            iRet = rt_get_default(pentry);
            if (iRet == 0) {
                route_hook_rt_ipv4(RTM_CHANGE, pentry, 1);              /*  ��ӽ��ջ���                */
            }
            mem_free(pentry);
        }
        
    } else {
        pentry = rt_find_entry(&ipaddr, &ipnetmask, &ipgateway, 
                               cIfName, pmsghdr->rtm_flags);
        if (!pentry || pentry->rt_type) {
            _ErrorHandle(ENETUNREACH);
            return  (0);
        }
        rt_delete_entry(pentry);
        
        if (rt_msghdr_to_entry(pentry, pmsghdr) < 0) {                  /*  ת��Ϊ rt_entry             */
            _ErrorHandle(EINVAL);
            return  (0);
        }
        
        iRet = rt_add_entry(pentry);
        if (iRet < 0) {
            mem_free(pentry);
            _ErrorHandle(ENETUNREACH);
            return  (0);
        
        } else {
            route_hook_rt_ipv4(RTM_CHANGE, pentry, 1);                  /*  ��ӽ��ջ���                */
        }
    }
        
    return  ((ssize_t)pmsghdr->rtm_msglen);
}
/*********************************************************************************************************
** ��������: __routeRtmChange6
** ��������: �޸�һ�� IPv6 ·��
** �䡡��  : pmsghdr   ·����Ϣ
** �䡡��  : д��������
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
#if LWIP_IPV6

static ssize_t  __routeRtmChange6 (struct rt_msghdr  *pmsghdr)
{
    INT                  iRet;
    struct netif        *pnetif;
    struct rt6_entry    *pentry6;
    struct sockaddr_in6 *psockaddrin6;
    ip6_addr_t           ip6addr;
    ip6_addr_t           ip6netmask;
    ip6_addr_t           ip6gateway;
    CHAR                 cIfName[IF_NAMESIZE] = "";
    
    if ((pmsghdr->rtm_addrs & (RTA_DST | RTA_GATEWAY)) != (RTA_DST | RTA_GATEWAY)) {
        _ErrorHandle(EINVAL);
        return  (0);
    }
    
    pnetif = netif_get_by_index(pmsghdr->rtm_index);
    if (pnetif) {
        netif_get_name(pnetif, cIfName);
    }
    
    psockaddrin6 = (struct sockaddr_in6 *)(pmsghdr + 1);
    inet6_addr_to_ip6addr(&ip6addr, &psockaddrin6->sin6_addr);
    
    psockaddrin6 = SA_NEXT(struct sockaddr_in6 *, psockaddrin6);
    inet6_addr_to_ip6addr(&ip6gateway, &psockaddrin6->sin6_addr);
    
    if (pmsghdr->rtm_addrs & RTA_NETMASK) {
        psockaddrin6 = SA_NEXT(struct sockaddr_in6 *, psockaddrin6);
        inet6_addr_to_ip6addr(&ip6netmask, &psockaddrin6->sin6_addr);
    } else {
        ip6_addr_set_any(&ip6netmask);
    }
    
    if (ip6_addr_isany_val(ip6addr)) {
        if (cIfName[0] == '\0') {
            _ErrorHandle(ENETUNREACH);
            return  (0);
        }
        
        iRet = rt6_change_default(&ip6gateway, cIfName);
        if (iRet < 0) {
            _ErrorHandle(ENETUNREACH);
            return  (0);
        }
    
        pentry6 = (struct rt6_entry *)mem_malloc(sizeof(struct rt6_entry));
        if (pentry6) {
            iRet = rt6_get_default(pentry6);
            if (iRet == 0) {
                route_hook_rt_ipv6(RTM_CHANGE, pentry6, 1);             /*  ��ӽ��ջ���                */
            }
            mem_free(pentry6);
        }

    } else {
        pentry6 = rt6_find_entry(&ip6addr, &ip6netmask, &ip6gateway, 
                                 cIfName, pmsghdr->rtm_flags);
        if (!pentry6 || pentry6->rt6_type) {
            _ErrorHandle(ENETUNREACH);
            return  (0);
        }
        rt6_delete_entry(pentry6);
        
        if (rt6_msghdr_to_entry(pentry6, pmsghdr) < 0) {                /*  ת��Ϊ rt6_entry            */
            _ErrorHandle(EINVAL);
            return  (0);
        }
        
        iRet = rt6_add_entry(pentry6);
        if (iRet < 0) {
            mem_free(pentry6);
            _ErrorHandle(ENETUNREACH);
            return  (0);
        
        } else {
            route_hook_rt_ipv6(RTM_CHANGE, pentry6, 1);                 /*  ��ӽ��ջ���                */
        }
    }
        
    return  ((ssize_t)pmsghdr->rtm_msglen);
}

#endif                                                                  /*  LWIP_IPV6                   */
/*********************************************************************************************************
** ��������: __routeRtmDelete4
** ��������: ɾ��һ�� IPv4 ·��
** �䡡��  : pmsghdr   ·����Ϣ
** �䡡��  : д��������
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static ssize_t  __routeRtmDelete4 (struct rt_msghdr  *pmsghdr)
{
    INT                 iRet;
    struct netif       *pnetif;
    struct rt_entry    *pentry;
    struct sockaddr_in *psockaddrin;
    ip4_addr_t          ipaddr;
    ip4_addr_t          ipnetmask;
    ip4_addr_t          ipgateway;
    CHAR                cIfName[IF_NAMESIZE] = "";
    
    if (!(pmsghdr->rtm_addrs & RTA_DST)) {
        _ErrorHandle(EINVAL);
        return  (0);
    }
    
    pnetif = netif_get_by_index(pmsghdr->rtm_index);
    if (pnetif) {
        netif_get_name(pnetif, cIfName);
    }
    
    psockaddrin = (struct sockaddr_in *)(pmsghdr + 1);
    ipaddr.addr = psockaddrin->sin_addr.s_addr;
    
    if (pmsghdr->rtm_addrs & RTA_GATEWAY) {
        psockaddrin = SA_NEXT(struct sockaddr_in *, psockaddrin);
        ipgateway.addr = psockaddrin->sin_addr.s_addr;
    } else {
        ipgateway.addr = IPADDR_ANY;
    }
    
    if (pmsghdr->rtm_addrs & RTA_NETMASK) {
        psockaddrin = SA_NEXT(struct sockaddr_in *, psockaddrin);
        ipnetmask.addr = psockaddrin->sin_addr.s_addr;
    } else {
        ipnetmask.addr = IPADDR_ANY;
    }
    
    if (ipaddr.addr == IPADDR_ANY) {                                    /*  ɾ��Ĭ��·��                */
        if ((ipgateway.addr == IPADDR_ANY) && (cIfName[0] == '\0')) {
            _ErrorHandle(ENETUNREACH);
            return  (0);
        }
        
        pentry = (struct rt_entry *)mem_malloc(sizeof(struct rt_entry));
        if (pentry) {
            iRet = rt_get_default(pentry);
            if (iRet == 0) {
                iRet = rt_delete_default(&ipgateway, cIfName);
                if (iRet == 0) {
                    route_hook_rt_ipv4(RTM_DELETE, pentry, 1);          /*  ��ӽ��ջ���                */
                }
            }
            mem_free(pentry);
            if (iRet) {
                _ErrorHandle(ENETUNREACH);
                return  (0);
            }
        
        } else {
            _ErrorHandle(ENOMEM);
            return  (0);
        }
    
    } else {
        pentry = rt_find_entry(&ipaddr, &ipnetmask, &ipgateway, 
                               cIfName, pmsghdr->rtm_flags);
        if (!pentry || pentry->rt_type) {
            _ErrorHandle(ENETUNREACH);
            return  (0);
        }
        rt_delete_entry(pentry);
        
        route_hook_rt_ipv4(RTM_DELETE, pentry, 1);                      /*  ��ӽ��ջ���                */
        mem_free(pentry);
    }
    
    return  ((ssize_t)pmsghdr->rtm_msglen);
}
/*********************************************************************************************************
** ��������: __routeRtmDelete6
** ��������: ɾ��һ�� IPv6 ·��
** �䡡��  : pmsghdr   ·����Ϣ
** �䡡��  : д��������
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
#if LWIP_IPV6

static ssize_t  __routeRtmDelete6 (struct rt_msghdr  *pmsghdr)
{
    INT                  iRet;
    struct netif        *pnetif;
    struct rt6_entry    *pentry6;
    struct sockaddr_in6 *psockaddrin6;
    ip6_addr_t           ip6addr;
    ip6_addr_t           ip6netmask;
    ip6_addr_t           ip6gateway;
    CHAR                 cIfName[IF_NAMESIZE] = "";
    
    if (!(pmsghdr->rtm_addrs & RTA_DST)) {
        _ErrorHandle(EINVAL);
        return  (0);
    }
    
    pnetif = netif_get_by_index(pmsghdr->rtm_index);
    if (pnetif) {
        netif_get_name(pnetif, cIfName);
    }
    
    psockaddrin6 = (struct sockaddr_in6 *)(pmsghdr + 1);
    inet6_addr_to_ip6addr(&ip6addr, &psockaddrin6->sin6_addr);
    
    if (pmsghdr->rtm_addrs & RTA_GATEWAY) {
        psockaddrin6 = SA_NEXT(struct sockaddr_in6 *, psockaddrin6);
        inet6_addr_to_ip6addr(&ip6gateway, &psockaddrin6->sin6_addr);
    } else {
        ip6_addr_set_any(&ip6gateway);
    }
    
    if (pmsghdr->rtm_addrs & RTAX_NETMASK) {
        psockaddrin6 = SA_NEXT(struct sockaddr_in6 *, psockaddrin6);
        inet6_addr_to_ip6addr(&ip6netmask, &psockaddrin6->sin6_addr);
    } else {
        ip6_addr_set_any(&ip6netmask);
    }
    
    if (ip6_addr_isany_val(ip6addr)) {
        if (ip6_addr_isany_val(ip6gateway) && (cIfName[0] == '\0')) {
            _ErrorHandle(ENETUNREACH);
            return  (0);
        }
        
        pentry6 = (struct rt6_entry *)mem_malloc(sizeof(struct rt6_entry));
        if (pentry6) {
            iRet = rt6_get_default(pentry6);
            if (iRet == 0) {
                iRet = rt6_delete_default(&ip6gateway, cIfName);
                if (iRet == 0) {
                    route_hook_rt_ipv6(RTM_DELETE, pentry6, 1);         /*  ��ӽ��ջ���                */
                }
            }
            mem_free(pentry6);
            if (iRet) {
                _ErrorHandle(ENETUNREACH);
                return  (0);
            }
        
        } else {
            _ErrorHandle(ENOMEM);
            return  (0);
        }
    
    } else {
        pentry6 = rt6_find_entry(&ip6addr, &ip6netmask, &ip6gateway, 
                                 cIfName, pmsghdr->rtm_flags);
        if (!pentry6 || pentry6->rt6_type) {
            _ErrorHandle(ENETUNREACH);
            return  (0);
        }
        rt6_delete_entry(pentry6);
        
        route_hook_rt_ipv6(RTM_DELETE, pentry6, 1);                     /*  ��ӽ��ջ���                */
        mem_free(pentry6);
    }
    
    return  ((ssize_t)pmsghdr->rtm_msglen);
}

#endif                                                                  /*  LWIP_IPV6                   */
/*********************************************************************************************************
** ��������: __routeRtmGet4Walk
** ��������: ��ȡһ�� IPv4 ·�ɱ���
** �䡡��  : pentry     ·����Ŀ
**           ipaddr     ��ַ
**           ipgateway  ���ص�ַ
**           pcIfname   ����ӿ���
**           iRet       ������
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static VOID  __routeRtmGet4Walk (struct rt_entry *pentry, 
                                 ip4_addr_t      *ipaddr, 
                                 ip4_addr_t      *ipgateway, 
                                 char            *pcIfname,
                                 INT             *iRet)
{
    if (pentry->rt_dest.addr != ipaddr->addr) {
        return;
    }
    
    if ((ipgateway->addr != IPADDR_ANY) && 
        (pentry->rt_gateway.addr != ipgateway->addr)) {
        return;
    }
    
    if ((pcIfname[0] != PX_EOS) && 
        lib_strcmp(pentry->rt_ifname, pcIfname)) {
        return;
    }

    route_hook_rt_ipv4(RTM_GET, pentry, 1);
    *iRet = ERROR_NONE;
}
/*********************************************************************************************************
** ��������: __routeRtmGet4
** ��������: ��ȡһ�� IPv4 ·��
** �䡡��  : pmsghdr   ·����Ϣ
** �䡡��  : д��������
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static ssize_t  __routeRtmGet4 (struct rt_msghdr  *pmsghdr)
{
    INT                 iRet = PX_ERROR;
    struct sockaddr_in *psockaddrin;
    struct sockaddr_dl *psockaddrdl;
    ip4_addr_t          ipaddr;
    ip4_addr_t          ipgateway;
    char                cIfname[IF_NAMESIZE] = "";
    
    if (!(pmsghdr->rtm_addrs & RTA_DST)) {
        _ErrorHandle(EINVAL);
        return  (0);
    }
    
    psockaddrin = (struct sockaddr_in *)(pmsghdr + 1);
    ipaddr.addr = psockaddrin->sin_addr.s_addr;
    
    if (pmsghdr->rtm_addrs & RTA_GATEWAY) {
        psockaddrin = SA_NEXT(struct sockaddr_in *, psockaddrin);
        ipgateway.addr = psockaddrin->sin_addr.s_addr;
    
    } else {
        ipgateway.addr = IPADDR_ANY;
    }
    
    if (pmsghdr->rtm_addrs & RTA_NETMASK) {
        psockaddrin = SA_NEXT(struct sockaddr_in *, psockaddrin);
    }
    
    if (pmsghdr->rtm_addrs & RTA_GENMASK) {
        psockaddrin = SA_NEXT(struct sockaddr_in *, psockaddrin);
    }
    
    if (pmsghdr->rtm_addrs & RTA_IFP) {
        psockaddrdl = SA_NEXT(struct sockaddr_dl *, psockaddrin);
        if (psockaddrdl->sdl_family == AF_LINK) {
            lib_strlcpy(cIfname, psockaddrdl->sdl_data, IF_NAMESIZE);
        }
    }
    
    rt_traversal_entry(__routeRtmGet4Walk, &ipaddr, &ipgateway, cIfname, &iRet, LW_NULL, LW_NULL);
    if (iRet < 0) {
        _ErrorHandle(ENETUNREACH);
        return  (0);
    }
    
    return  ((ssize_t)pmsghdr->rtm_msglen);
}
/*********************************************************************************************************
** ��������: __routeRtmGet6Walk
** ��������: ��ȡһ�� IPv6 ·�ɱ���
** �䡡��  : pentry      ·����Ŀ
**           ip6addr     ��ַ
**           ip6gateway  ���ص�ַ
**           pcIfname    ����ӿ���
**           iRet        ������
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
#if LWIP_IPV6

static VOID  __routeRtmGet6Walk (struct rt6_entry *pentry6, 
                                 ip6_addr_t       *ip6addr, 
                                 ip6_addr_t       *ip6gateway, 
                                 char             *pcIfname,
                                 INT              *iRet)
{
    if (!ip6_addr_cmp(&pentry6->rt6_dest, ip6addr)) {
        return;
    }
    
    if (!ip6_addr_isany(ip6gateway) && 
        !ip6_addr_cmp(&pentry6->rt6_gateway, ip6gateway)) {
        return;
    }
    
    if ((pcIfname[0] != PX_EOS) && 
        lib_strcmp(pentry6->rt6_ifname, pcIfname)) {
        return;
    }
    
    route_hook_rt_ipv6(RTM_GET, pentry6, 1);
    *iRet = ERROR_NONE;
}
/*********************************************************************************************************
** ��������: __routeRtmGet6
** ��������: ��ȡһ�� IPv6 ·��
** �䡡��  : pmsghdr   ·����Ϣ
** �䡡��  : д��������
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static ssize_t  __routeRtmGet6 (struct rt_msghdr  *pmsghdr)
{
    INT                  iRet = PX_ERROR;
    struct sockaddr_in6 *psockaddrin6;
    struct sockaddr_dl  *psockaddrdl;
    ip6_addr_t           ip6addr;
    ip6_addr_t           ip6gateway;
    char                 cIfname[IF_NAMESIZE] = "";
    
    if (!(pmsghdr->rtm_addrs & RTA_DST)) {
        _ErrorHandle(EINVAL);
        return  (0);
    }
    
    psockaddrin6 = (struct sockaddr_in6 *)(pmsghdr + 1);
    inet6_addr_to_ip6addr(&ip6addr, &psockaddrin6->sin6_addr);
    
    if (pmsghdr->rtm_addrs & RTA_GATEWAY) {
        psockaddrin6 = SA_NEXT(struct sockaddr_in6 *, psockaddrin6);
        inet6_addr_to_ip6addr(&ip6gateway, &psockaddrin6->sin6_addr);
    
    } else {
        ip6_addr_set_any(&ip6gateway);
    }
    
    if (pmsghdr->rtm_addrs & RTA_NETMASK) {
        psockaddrin6 = SA_NEXT(struct sockaddr_in6 *, psockaddrin6);
    }
    
    if (pmsghdr->rtm_addrs & RTA_GENMASK) {
        psockaddrin6 = SA_NEXT(struct sockaddr_in6 *, psockaddrin6);
    }
    
    if (pmsghdr->rtm_addrs & RTA_IFP) {
        psockaddrdl = SA_NEXT(struct sockaddr_dl *, psockaddrin6);
        if (psockaddrdl->sdl_family == AF_LINK) {
            lib_strlcpy(cIfname, psockaddrdl->sdl_data, IF_NAMESIZE);
        }
    }
    
    rt6_traversal_entry(__routeRtmGet6Walk, &ip6addr, &ip6gateway, cIfname, &iRet, LW_NULL, LW_NULL);
    if (iRet < 0) {
        _ErrorHandle(ENETUNREACH);
        return  (0);
    }
    
    return  ((ssize_t)pmsghdr->rtm_msglen);
}

#endif                                                                  /*  LWIP_IPV6                   */
/*********************************************************************************************************
** ��������: route_hook_rt_ipv4
** ��������: ���� / ɾ�� / �޸�һ�� IPv4 ·�ɻص�
** �䡡��  : type      RTM_ADD / RTM_DELETE / RTM_CHANGE / RTM_GET
**           pentry    IPv4 ·�ɱ���
**           nolock    �Ƿ����
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
VOID  route_hook_rt_ipv4 (u_char type, const struct rt_entry *pentry, int nolock)
{
    size_t              stMsgLen;
    struct rt_msghdr   *pmsghdr;
    
    PLW_LIST_LINE       pline;
    AF_ROUTE_T         *pafroute;
    AF_ROUTE_Q         *prouteq;
    AF_ROUTE_N         *prouten;
    
    if (!_G_plineAfRoute) {
        return;
    }
    
    stMsgLen = rt_entry_to_msghdr(LW_NULL, pentry, type);
    if (!nolock) {
        __AF_ROUTE_LOCK();
    }
    for (pline = _G_plineAfRoute; pline != LW_NULL; pline = _list_line_get_next(pline)) {
        pafroute = _LIST_ENTRY(pline, AF_ROUTE_T, ROUTE_lineManage);
        if (((pafroute->ROUTE_rtq.RTQ_stTotal + stMsgLen) <= pafroute->ROUTE_stMaxBufSize) &&
            !(pafroute->ROUTE_iShutDFlag & __AF_ROUTE_SHUTD_R)) {
            prouten = (AF_ROUTE_N *)mem_malloc(sizeof(AF_ROUTE_N) + stMsgLen);
            if (!prouten) {
                break;
            }
            
            prouten->RTM_stLen = stMsgLen;
            pmsghdr = (struct rt_msghdr *)(prouten + 1);
            rt_entry_to_msghdr(pmsghdr, pentry, type);
            
            prouteq = &pafroute->ROUTE_rtq;
            _list_mono_free_seq(&prouteq->RTQ_pmonoHeader,
                                &prouteq->RTQ_pmonoTail,
                                &prouten->RTM_monoManage);
            prouteq->RTQ_stTotal += prouten->RTM_stLen;
            
            __routeUpdateReader(pafroute, ERROR_NONE);
        }
    }
    if (!nolock) {
        __AF_ROUTE_UNLOCK();
    }
}
/*********************************************************************************************************
** ��������: route_hook_rt_ipv6
** ��������: ���� / ɾ�� / �޸�һ�� IPv4 ·�ɻص�
** �䡡��  : type      RTM_ADD / RTM_DELETE / RTM_CHANGE / RTM_GET
**           pentry    IPv6 ·�ɱ���
**           nolock    �Ƿ����
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
#if LWIP_IPV6

VOID  route_hook_rt_ipv6 (u_char type, const struct rt6_entry *pentry, int nolock)
{
    size_t               stMsgLen;
    struct rt_msghdr    *pmsghdr;
    
    PLW_LIST_LINE        pline;
    AF_ROUTE_T          *pafroute;
    AF_ROUTE_Q          *prouteq;
    AF_ROUTE_N          *prouten;
    
    if (!_G_plineAfRoute) {
        return;
    }
    
    stMsgLen = rt6_entry_to_msghdr(LW_NULL, pentry, type);
    if (!nolock) {
        __AF_ROUTE_LOCK();
    }
    for (pline = _G_plineAfRoute; pline != LW_NULL; pline = _list_line_get_next(pline)) {
        pafroute = _LIST_ENTRY(pline, AF_ROUTE_T, ROUTE_lineManage);
        if (((pafroute->ROUTE_rtq.RTQ_stTotal + stMsgLen) <= pafroute->ROUTE_stMaxBufSize) &&
            !(pafroute->ROUTE_iShutDFlag & __AF_ROUTE_SHUTD_R)) {
            prouten = (AF_ROUTE_N *)mem_malloc(sizeof(AF_ROUTE_N) + stMsgLen);
            if (!prouten) {
                break;
            }
            
            prouten->RTM_stLen = stMsgLen;
            pmsghdr = (struct rt_msghdr *)(prouten + 1);
            rt6_entry_to_msghdr(pmsghdr, pentry, type);
            
            prouteq = &pafroute->ROUTE_rtq;
            _list_mono_free_seq(&prouteq->RTQ_pmonoHeader,
                                &prouteq->RTQ_pmonoTail,
                                &prouten->RTM_monoManage);
            prouteq->RTQ_stTotal += prouten->RTM_stLen;
            
            __routeUpdateReader(pafroute, ERROR_NONE);
        }
    }
    if (!nolock) {
        __AF_ROUTE_UNLOCK();
    }
}

#endif                                                                  /*  LWIP_IPV6                   */
/*********************************************************************************************************
** ��������: route_hook_netif_ipv4
** ��������: ����ӿ��޸� IPv4 ��ַ
** �䡡��  : pnetif      ����ӿ�
**           pipaddr     ��ַ
**           pnetmask    ����
**           type        RTM_DELADDR / RTM_ADDADDR
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
VOID  route_hook_netif_ipv4 (struct netif *pnetif, const ip4_addr_t *pipaddr, const ip4_addr_t *pnetmask, u_char type)
{
    size_t               stMsgLen;
    struct sockaddr_in  *psockaddrin;
    struct sockaddr_dl  *psockaddrdl;
    struct ifa_msghdr   *pmsghdr;
    int                  iFlags;
    
    PLW_LIST_LINE        pline;
    AF_ROUTE_T          *pafroute;
    AF_ROUTE_Q          *prouteq;
    AF_ROUTE_N          *prouten;
    
    if (!_G_plineAfRoute) {
        return;
    }
    
    iFlags   = netif_get_flags(pnetif);
    stMsgLen = sizeof(struct ifa_msghdr) 
             + (SO_ROUND_UP(sizeof(struct sockaddr_in)) * 3)
             + SO_ROUND_UP(sizeof(struct sockaddr_dl));
             
    __AF_ROUTE_LOCK();
    for (pline = _G_plineAfRoute; pline != LW_NULL; pline = _list_line_get_next(pline)) {
        pafroute = _LIST_ENTRY(pline, AF_ROUTE_T, ROUTE_lineManage);
        if (((pafroute->ROUTE_rtq.RTQ_stTotal + (stMsgLen * 2)) <= pafroute->ROUTE_stMaxBufSize) &&
            !(pafroute->ROUTE_iShutDFlag & __AF_ROUTE_SHUTD_R)) {
            prouten = (AF_ROUTE_N *)mem_malloc(sizeof(AF_ROUTE_N) + stMsgLen);
            if (!prouten) {
                break;
            }
            
            prouten->RTM_stLen = stMsgLen;
            pmsghdr = (struct ifa_msghdr *)(prouten + 1);
            
            pmsghdr->ifam_msglen  = (u_short)stMsgLen;
            pmsghdr->ifam_version = RTM_VERSION;
            pmsghdr->ifam_type    = type;
            
            pmsghdr->ifam_addrs  = RTA_NETMASK | RTA_IFP | RTA_IFA | RTA_BRD;
            pmsghdr->ifam_flags  = iFlags;
            pmsghdr->ifam_index  = netif_get_index(pnetif);
            pmsghdr->ifam_metric = 0;
            
            psockaddrin = (struct sockaddr_in *)(pmsghdr + 1);
            psockaddrin->sin_len         = sizeof(struct sockaddr_in);
            psockaddrin->sin_family      = AF_INET;
            psockaddrin->sin_addr.s_addr = pnetmask->addr;
            
            psockaddrdl = SA_NEXT(struct sockaddr_dl *, psockaddrin);
            lib_bzero(psockaddrdl, sizeof(struct sockaddr_dl));
            rt_build_sockaddr_dl(psockaddrdl, pnetif);
            
            psockaddrin = SA_NEXT(struct sockaddr_in *, psockaddrdl);
            psockaddrin->sin_len         = sizeof(struct sockaddr_in);
            psockaddrin->sin_family      = AF_INET;
            psockaddrin->sin_addr.s_addr = pipaddr->addr;
            
            psockaddrin = SA_NEXT(struct sockaddr_in *, psockaddrin);
            if (pnetif->flags & NETIF_FLAG_BROADCAST) {
                psockaddrin->sin_len         = sizeof(struct sockaddr_in);
                psockaddrin->sin_family      = AF_INET;
                psockaddrin->sin_addr.s_addr = (pipaddr->addr | (~pnetmask->addr));
            
            } else {
                psockaddrin->sin_len         = sizeof(struct sockaddr_in);
                psockaddrin->sin_family      = AF_INET;
                psockaddrin->sin_addr.s_addr = netif_ip4_gw(pnetif)->addr;
            }
            
            prouteq = &pafroute->ROUTE_rtq;
            _list_mono_free_seq(&prouteq->RTQ_pmonoHeader,
                                &prouteq->RTQ_pmonoTail,
                                &prouten->RTM_monoManage);
            prouteq->RTQ_stTotal += prouten->RTM_stLen;
            
            __routeUpdateReader(pafroute, ERROR_NONE);
        }
    }
    __AF_ROUTE_UNLOCK();
}
/*********************************************************************************************************
** ��������: route_hook_netif_ipv6
** ��������: ����ӿ���� / ɾ��һ�� IPv6 ��ַ�ص�
** �䡡��  : pnetif         ����ӿ�
**           pip6addr       ��ַ
**           type           RTM_DELADDR / RTM_ADDADDR
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
#if LWIP_IPV6

VOID  route_hook_netif_ipv6 (struct netif *pnetif, const ip6_addr_t *pip6addr, u_char type)
{
    size_t               stMsgLen;
    struct sockaddr_in6 *psockaddrin6;
    struct sockaddr_dl  *psockaddrdl;
    struct ifa_msghdr   *pmsghdr;
    int                  iFlags;
    ip6_addr_t           ip6addr;
    
    PLW_LIST_LINE        pline;
    AF_ROUTE_T          *pafroute;
    AF_ROUTE_Q          *prouteq;
    AF_ROUTE_N          *prouten;
    
    if (!_G_plineAfRoute) {
        return;
    }
    
    iFlags   = netif_get_flags(pnetif);
    stMsgLen = sizeof(struct ifa_msghdr) 
             + (SO_ROUND_UP(sizeof(struct sockaddr_in6)) * 3)
             + SO_ROUND_UP(sizeof(struct sockaddr_dl));
             
    __AF_ROUTE_LOCK();
    for (pline = _G_plineAfRoute; pline != LW_NULL; pline = _list_line_get_next(pline)) {
        pafroute = _LIST_ENTRY(pline, AF_ROUTE_T, ROUTE_lineManage);
        if (((pafroute->ROUTE_rtq.RTQ_stTotal + stMsgLen) <= pafroute->ROUTE_stMaxBufSize) &&
            !(pafroute->ROUTE_iShutDFlag & __AF_ROUTE_SHUTD_R)) {
            prouten = (AF_ROUTE_N *)mem_malloc(sizeof(AF_ROUTE_N) + stMsgLen);
            if (!prouten) {
                break;
            }
            
            prouten->RTM_stLen = stMsgLen;
            pmsghdr = (struct ifa_msghdr *)(prouten + 1);
            
            pmsghdr->ifam_msglen  = (u_short)stMsgLen;
            pmsghdr->ifam_version = RTM_VERSION;
            pmsghdr->ifam_type    = type;
            
            pmsghdr->ifam_addrs  = RTA_NETMASK | RTA_IFP | RTA_IFA | RTA_BRD;
            pmsghdr->ifam_flags  = iFlags;
            pmsghdr->ifam_index  = netif_get_index(pnetif);
            pmsghdr->ifam_metric = 0;
            
            psockaddrin6 = (struct sockaddr_in6 *)(pmsghdr + 1);
            psockaddrin6->sin6_len    = sizeof(struct sockaddr_in6);
            psockaddrin6->sin6_family = AF_INET6;
            rt6_netmask_from_prefix(&ip6addr, 64);
            inet6_addr_from_ip6addr(&psockaddrin6->sin6_addr, &ip6addr);
            
            psockaddrdl = SA_NEXT(struct sockaddr_dl *, psockaddrin6);
            lib_bzero(psockaddrdl, sizeof(struct sockaddr_dl));
            rt6_build_sockaddr_dl(psockaddrdl, pnetif);
            
            psockaddrin6 = SA_NEXT(struct sockaddr_in6 *, psockaddrdl);
            psockaddrin6->sin6_len    = sizeof(struct sockaddr_in6);
            psockaddrin6->sin6_family = AF_INET6;
            inet6_addr_from_ip6addr(&psockaddrin6->sin6_addr, pip6addr);
            
            psockaddrin6 = SA_NEXT(struct sockaddr_in6 *, psockaddrin6);
            psockaddrin6->sin6_len    = sizeof(struct sockaddr_in6);
            psockaddrin6->sin6_family = AF_INET6;
            IP6_ADDR(&ip6addr, 0xff, 0, 0, 0);
            inet6_addr_from_ip6addr(&psockaddrin6->sin6_addr, &ip6addr);
            
            prouteq = &pafroute->ROUTE_rtq;
            _list_mono_free_seq(&prouteq->RTQ_pmonoHeader,
                                &prouteq->RTQ_pmonoTail,
                                &prouten->RTM_monoManage);
            prouteq->RTQ_stTotal += prouten->RTM_stLen;
            
            __routeUpdateReader(pafroute, ERROR_NONE);
        }
    }
    __AF_ROUTE_UNLOCK();
}

#endif                                                                  /*  LWIP_IPV6                   */
/*********************************************************************************************************
** ��������: route_hook_maddr_ipv4
** ��������: ����ӿ� ���� / ɾ�� һ�� IPv4 �鲥��ַ
** �䡡��  : pnetif    ����ӿ�
**           pipaddr   �鲥��ַ
**           type      RTM_NEWMADDR / RTM_DELMADDR
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
VOID  route_hook_maddr_ipv4 (struct netif *pnetif, const ip4_addr_t *pipaddr, u_char type)
{
    size_t              stMsgLen;
    struct sockaddr_in *psockaddrin;
    struct sockaddr_dl *psockaddrdl;
    struct ifma_msghdr *pmsghdr;
    INT                 iFlags;
    
    PLW_LIST_LINE       pline;
    AF_ROUTE_T         *pafroute;
    AF_ROUTE_Q         *prouteq;
    AF_ROUTE_N         *prouten;
    
    if (!_G_plineAfRoute) {
        return;
    }
    
    iFlags   = netif_get_flags(pnetif);
    stMsgLen = sizeof(struct ifma_msghdr) 
             + SO_ROUND_UP(sizeof(struct sockaddr_in))
             + SO_ROUND_UP(sizeof(struct sockaddr_dl));
    
    __AF_ROUTE_LOCK();
    for (pline = _G_plineAfRoute; pline != LW_NULL; pline = _list_line_get_next(pline)) {
        pafroute = _LIST_ENTRY(pline, AF_ROUTE_T, ROUTE_lineManage);
        if (((pafroute->ROUTE_rtq.RTQ_stTotal + stMsgLen) <= pafroute->ROUTE_stMaxBufSize) &&
            !(pafroute->ROUTE_iShutDFlag & __AF_ROUTE_SHUTD_R)) {
            prouten = (AF_ROUTE_N *)mem_malloc(sizeof(AF_ROUTE_N) + stMsgLen);
            if (!prouten) {
                break;
            }
            
            prouten->RTM_stLen = stMsgLen;
            pmsghdr = (struct ifma_msghdr *)(prouten + 1);
            
            pmsghdr->ifmam_msglen  = (u_short)stMsgLen;
            pmsghdr->ifmam_version = RTM_VERSION;
            pmsghdr->ifmam_type    = type;
            
            pmsghdr->ifmam_addrs = RTA_DST | RTA_IFP;
            pmsghdr->ifmam_flags = iFlags;
            pmsghdr->ifmam_index = netif_get_index(pnetif);
            
            psockaddrin = (struct sockaddr_in *)(pmsghdr + 1);
            psockaddrin->sin_len         = sizeof(struct sockaddr_in);
            psockaddrin->sin_family      = AF_INET;
            psockaddrin->sin_addr.s_addr = pipaddr->addr;
            
            psockaddrdl = SA_NEXT(struct sockaddr_dl *, psockaddrin);
            lib_bzero(psockaddrdl, sizeof(struct sockaddr_dl));
            rt_build_sockaddr_dl(psockaddrdl, pnetif);
            
            prouteq = &pafroute->ROUTE_rtq;
            _list_mono_free_seq(&prouteq->RTQ_pmonoHeader,
                                &prouteq->RTQ_pmonoTail,
                                &prouten->RTM_monoManage);
            prouteq->RTQ_stTotal += prouten->RTM_stLen;
            
            __routeUpdateReader(pafroute, ERROR_NONE);
        }
    }
    __AF_ROUTE_UNLOCK();
}
/*********************************************************************************************************
** ��������: route_hook_maddr_ipv6
** ��������: ����ӿ� ���� / ɾ�� һ�� IPv6 �鲥��ַ
** �䡡��  : pnetif    ����ӿ�
**           pip6addr  �鲥��ַ
**           type      RTM_NEWMADDR / RTM_DELMADDR
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
#if LWIP_IPV6

VOID  route_hook_maddr_ipv6 (struct netif *pnetif, const ip6_addr_t *pip6addr, u_char type)
{
    size_t               stMsgLen;
    struct sockaddr_in6 *psockaddrin6;
    struct sockaddr_dl  *psockaddrdl;
    struct ifma_msghdr  *pmsghdr;
    INT                  iFlags;
    
    PLW_LIST_LINE        pline;
    AF_ROUTE_T          *pafroute;
    AF_ROUTE_Q          *prouteq;
    AF_ROUTE_N          *prouten;
    
    if (!_G_plineAfRoute) {
        return;
    }
    
    iFlags   = netif_get_flags(pnetif);
    stMsgLen = sizeof(struct ifma_msghdr) 
             + SO_ROUND_UP(sizeof(struct sockaddr_in6))
             + SO_ROUND_UP(sizeof(struct sockaddr_dl));
    
    __AF_ROUTE_LOCK();
    for (pline = _G_plineAfRoute; pline != LW_NULL; pline = _list_line_get_next(pline)) {
        pafroute = _LIST_ENTRY(pline, AF_ROUTE_T, ROUTE_lineManage);
        if (((pafroute->ROUTE_rtq.RTQ_stTotal + stMsgLen) <= pafroute->ROUTE_stMaxBufSize) &&
            !(pafroute->ROUTE_iShutDFlag & __AF_ROUTE_SHUTD_R)) {
            prouten = (AF_ROUTE_N *)mem_malloc(sizeof(AF_ROUTE_N) + stMsgLen);
            if (!prouten) {
                break;
            }
            
            prouten->RTM_stLen = stMsgLen;
            pmsghdr = (struct ifma_msghdr *)(prouten + 1);
            
            pmsghdr->ifmam_msglen  = (u_short)stMsgLen;
            pmsghdr->ifmam_version = RTM_VERSION;
            pmsghdr->ifmam_type    = type;
            
            pmsghdr->ifmam_addrs = RTA_DST | RTA_IFP;
            pmsghdr->ifmam_flags = iFlags;
            pmsghdr->ifmam_index = netif_get_index(pnetif);
            
            psockaddrin6 = (struct sockaddr_in6 *)(pmsghdr + 1);
            psockaddrin6->sin6_len    = sizeof(struct sockaddr_in6);
            psockaddrin6->sin6_family = AF_INET6;
            inet6_addr_from_ip6addr(&psockaddrin6->sin6_addr, pip6addr);
            
            psockaddrdl = SA_NEXT(struct sockaddr_dl *, psockaddrin6);
            lib_bzero(psockaddrdl, sizeof(struct sockaddr_dl));
            rt6_build_sockaddr_dl(psockaddrdl, pnetif);
            
            prouteq = &pafroute->ROUTE_rtq;
            _list_mono_free_seq(&prouteq->RTQ_pmonoHeader,
                                &prouteq->RTQ_pmonoTail,
                                &prouten->RTM_monoManage);
            prouteq->RTQ_stTotal += prouten->RTM_stLen;
            
            __routeUpdateReader(pafroute, ERROR_NONE);
        }
    }
    __AF_ROUTE_UNLOCK();
}

#endif                                                                  /*  LWIP_IPV6                   */
/*********************************************************************************************************
** ��������: route_hook_netif_ann
** ��������: ���� / ɾ�� ����ӿ�
** �䡡��  : pnetif    ����ӿ�
**           what      IFAN_ARRIVAL / IFAN_DEPARTURE
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
VOID  route_hook_netif_ann (struct netif *pnetif, int what)
{
    size_t                    stMsgLen;
    struct if_announcemsghdr *pmsghdr;
    
    PLW_LIST_LINE             pline;
    AF_ROUTE_T               *pafroute;
    AF_ROUTE_Q               *prouteq;
    AF_ROUTE_N               *prouten;
    
    if (!_G_plineAfRoute) {
        return;
    }
    
    stMsgLen = sizeof(struct if_announcemsghdr);
    
    __AF_ROUTE_LOCK();
    for (pline = _G_plineAfRoute; pline != LW_NULL; pline = _list_line_get_next(pline)) {
        pafroute = _LIST_ENTRY(pline, AF_ROUTE_T, ROUTE_lineManage);
        if (((pafroute->ROUTE_rtq.RTQ_stTotal + stMsgLen) <= pafroute->ROUTE_stMaxBufSize) &&
            !(pafroute->ROUTE_iShutDFlag & __AF_ROUTE_SHUTD_R)) {
            prouten = (AF_ROUTE_N *)mem_malloc(sizeof(AF_ROUTE_N) + stMsgLen);
            if (!prouten) {
                break;
            }
            
            prouten->RTM_stLen = stMsgLen;
            pmsghdr = (struct if_announcemsghdr *)(prouten + 1);
            
            pmsghdr->ifan_msglen  = (u_short)stMsgLen;
            pmsghdr->ifan_version = RTM_VERSION;
            pmsghdr->ifan_type    = RTM_IFANNOUNCE;
            
            pmsghdr->ifan_index = netif_get_index(pnetif);
            pmsghdr->ifan_what  = (u_short)what;
            
            netif_get_name(pnetif, pmsghdr->ifan_name);
            
            prouteq = &pafroute->ROUTE_rtq;
            _list_mono_free_seq(&prouteq->RTQ_pmonoHeader,
                                &prouteq->RTQ_pmonoTail,
                                &prouten->RTM_monoManage);
            prouteq->RTQ_stTotal += prouten->RTM_stLen;
            
            __routeUpdateReader(pafroute, ERROR_NONE);
        }
    }
    __AF_ROUTE_UNLOCK();
}
/*********************************************************************************************************
** ��������: route_hook_netif_updown
** ��������: ���� / ֹͣ / ���� / ��������ӿ�
** �䡡��  : pnetif    ����ӿ�
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
VOID  route_hook_netif_updown (struct netif *pnetif)
{
#define MIB2_NETIF(netif)   (&((netif)->mib2_counters))

    size_t              stMsgLen;
    struct netdev      *pnetdev;
    struct if_msghdr   *pmsghdr;
    struct sockaddr_dl *psockaddrdl;
    struct if_data     *pifdata;
    struct timespec     ts;
    INT                 iFlags;
    
    PLW_LIST_LINE       pline;
    AF_ROUTE_T         *pafroute;
    AF_ROUTE_Q         *prouteq;
    AF_ROUTE_N         *prouten;
    
    if (!_G_plineAfRoute) {
        return;
    }
    
    iFlags   = netif_get_flags(pnetif);
    stMsgLen = sizeof(struct if_msghdr) +
             + SO_ROUND_UP(sizeof(struct sockaddr_dl));
    
    __AF_ROUTE_LOCK();
    for (pline = _G_plineAfRoute; pline != LW_NULL; pline = _list_line_get_next(pline)) {
        pafroute = _LIST_ENTRY(pline, AF_ROUTE_T, ROUTE_lineManage);
        if (((pafroute->ROUTE_rtq.RTQ_stTotal + stMsgLen) <= pafroute->ROUTE_stMaxBufSize) &&
            !(pafroute->ROUTE_iShutDFlag & __AF_ROUTE_SHUTD_R)) {
            prouten = (AF_ROUTE_N *)mem_malloc(sizeof(AF_ROUTE_N) + stMsgLen);
            if (!prouten) {
                break;
            }
            
            prouten->RTM_stLen = stMsgLen;
            pmsghdr = (struct if_msghdr *)(prouten + 1);
            
            pmsghdr->ifm_msglen  = (u_short)stMsgLen;
            pmsghdr->ifm_version = RTM_VERSION;
            pmsghdr->ifm_type    = RTM_IFINFO;
            
            pmsghdr->ifm_addrs = RTA_IFP;
            pmsghdr->ifm_flags = iFlags;
            pmsghdr->ifm_index = netif_get_index(pnetif);
            
            pifdata = &pmsghdr->ifm_data;
            lib_bzero(pifdata, sizeof(struct if_data));
            
            pifdata->ifi_mtu  = pnetif->mtu;
            pifdata->ifi_type = pnetif->link_type;

            if (pnetif->flags & NETIF_FLAG_ETHERNET) {
                pifdata->ifi_addrlen = ETH_ALEN;
                pifdata->ifi_hdrlen  = ETH_HLEN;

            } else if (!(pnetif->flags & NETIF_FLAG_BROADCAST)) {
                pifdata->ifi_hdrlen  = 4;
            }
            
            pnetdev = (netdev_t *)(pnetif->state);
            if (pnetdev && (pnetdev->magic_no == NETDEV_MAGIC)) {
                pifdata->ifi_baudrate = pnetdev->speed;
            } else {
                pifdata->ifi_baudrate = pnetif->link_speed;
            }

            pifdata->ifi_ipackets   = MIB2_NETIF(pnetif)->ifinucastpkts + MIB2_NETIF(pnetif)->ifinnucastpkts;
            pifdata->ifi_ierrors    = MIB2_NETIF(pnetif)->ifinerrors;
            pifdata->ifi_opackets   = MIB2_NETIF(pnetif)->ifoutucastpkts + MIB2_NETIF(pnetif)->ifoutnucastpkts;
            pifdata->ifi_oerrors    = MIB2_NETIF(pnetif)->ifouterrors;
            pifdata->ifi_collisions = MIB2_NETIF(pnetif)->ifcollisions;
            pifdata->ifi_ibytes     = (u_long)MIB2_NETIF(pnetif)->ifinoctets;
            pifdata->ifi_obytes     = (u_long)MIB2_NETIF(pnetif)->ifoutoctets;
            pifdata->ifi_imcasts    = MIB2_NETIF(pnetif)->ifinnucastpkts;
            pifdata->ifi_omcasts    = MIB2_NETIF(pnetif)->ifoutnucastpkts;
            pifdata->ifi_iqdrops    = MIB2_NETIF(pnetif)->ifindiscards;
            pifdata->ifi_noproto    = MIB2_NETIF(pnetif)->ifinunknownprotos;
            
            lib_clock_gettime(CLOCK_REALTIME, &ts);
            pifdata->ifi_lastchange.tv_sec  = ts.tv_sec;
            pifdata->ifi_lastchange.tv_usec = ts.tv_nsec / 1000;
            
            psockaddrdl = (struct sockaddr_dl *)(pmsghdr + 1);
            lib_bzero(psockaddrdl, sizeof(struct sockaddr_dl));
            rt_build_sockaddr_dl(psockaddrdl, pnetif);

            prouteq = &pafroute->ROUTE_rtq;
            _list_mono_free_seq(&prouteq->RTQ_pmonoHeader,
                                &prouteq->RTQ_pmonoTail,
                                &prouten->RTM_monoManage);
            prouteq->RTQ_stTotal += prouten->RTM_stLen;
            
            __routeUpdateReader(pafroute, ERROR_NONE);
        }
    }
    __AF_ROUTE_UNLOCK();
}
/*********************************************************************************************************
** ��������: route_init
** ��������: ��ʼ�� route ��Э��
** �䡡��  : NONE
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
VOID  route_init (VOID)
{
}
/*********************************************************************************************************
** ��������: route_socket
** ��������: route socket
** �䡡��  : iDomain        ��, ������ AF_ROUTE
**           iType          ������ SOCK_RAW
**           iProtocol      Э��
** �䡡��  : unix socket
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
AF_ROUTE_T  *route_socket (INT  iDomain, INT  iType, INT  iProtocol)
{
    AF_ROUTE_T   *pafroute;
    
    if ((iDomain != AF_ROUTE) || (iType != SOCK_RAW)) {
        _ErrorHandle(EINVAL);
        return  (LW_NULL);
    }
    
    pafroute = (AF_ROUTE_T *)__SHEAP_ALLOC(sizeof(AF_ROUTE_T));
    if (pafroute == LW_NULL) {
        _ErrorHandle(ENOMEM);
        return  (LW_NULL);
    }
    lib_bzero(pafroute, sizeof(AF_ROUTE_T));
    
    pafroute->ROUTE_iFlag         = O_RDWR;
    pafroute->ROUTE_ulRecvTimeout = LW_OPTION_WAIT_INFINITE;
    pafroute->ROUTE_stMaxBufSize  = (LW_CFG_KB_SIZE * 64);
    
    pafroute->ROUTE_hCanRead = API_SemaphoreBCreate("route_rlock", LW_FALSE, 
                                                    LW_OPTION_OBJECT_GLOBAL, LW_NULL);
    if (pafroute->ROUTE_hCanRead == LW_OBJECT_HANDLE_INVALID) {
        __SHEAP_FREE(pafroute);
        return  (LW_NULL);
    }
    
    __AF_ROUTE_LOCK();
    _List_Line_Add_Ahead(&pafroute->ROUTE_lineManage, &_G_plineAfRoute);
    __AF_ROUTE_UNLOCK();
    
    return  (pafroute);
}
/*********************************************************************************************************
** ��������: route_close
** ��������: close
** �䡡��  : pafroute  route file
** �䡡��  : ERROR
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
INT  route_close (AF_ROUTE_T  *pafroute)
{
    if (pafroute) {
        __AF_ROUTE_LOCK();
        __routeDeleteBuf(pafroute);
        __routeUpdateReader(pafroute, ENOTCONN);
        _List_Line_Del(&pafroute->ROUTE_lineManage, &_G_plineAfRoute);
        __AF_ROUTE_UNLOCK();
        
        __SHEAP_FREE(pafroute);
        return  (ERROR_NONE);
    
    } else {
        return  (PX_ERROR);
    }
}
/*********************************************************************************************************
** ��������: route_send
** ��������: д·�ɱ�
** �䡡��  : pafroute  route file
**           data      send buffer
**           size      send len
**           flags     flag
** �䡡��  : ERROR
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
ssize_t  route_send (AF_ROUTE_T  *pafroute, const void *data, size_t size, int flags)
{
    ssize_t             sstSend = 0;
    BOOL                bNeedUpdateReader = LW_FALSE;
    struct rt_msghdr   *pmsghdr;
    struct sockaddr    *psockaddr;

    if (!data || !size) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    
    __AF_ROUTE_LOCK();
    if (pafroute->ROUTE_iShutDFlag & __AF_ROUTE_SHUTD_W) {
        __AF_ROUTE_UNLOCK();
        _ErrorHandle(ESHUTDOWN);                                        /*  �����Ѿ��ر�                */
        return  (sstSend);
    }

    pmsghdr   = (struct rt_msghdr *)data;
    psockaddr = (struct sockaddr *)(pmsghdr + 1);
    
    switch (pmsghdr->rtm_type) {
    
    case RTM_ADD:
        if (psockaddr->sa_family == AF_INET) {
            sstSend = __routeRtmAdd4(pmsghdr);
            
#if LWIP_IPV6
        } else if (psockaddr->sa_family == AF_INET6) {
            sstSend = __routeRtmAdd6(pmsghdr);
#endif
        } else {
            _ErrorHandle(EAFNOSUPPORT);
        }
        break;
        
    case RTM_CHANGE:
        if (psockaddr->sa_family == AF_INET) {
            sstSend = __routeRtmChange4(pmsghdr);
            
#if LWIP_IPV6
        } else if (psockaddr->sa_family == AF_INET6) {
            sstSend = __routeRtmChange6(pmsghdr);
#endif
        } else {
            _ErrorHandle(EAFNOSUPPORT);
        }
        break;
        
    case RTM_DELETE:
        if (psockaddr->sa_family == AF_INET) {
            sstSend = __routeRtmDelete4(pmsghdr);
            
#if LWIP_IPV6
        } else if (psockaddr->sa_family == AF_INET6) {
            sstSend = __routeRtmDelete6(pmsghdr);
#endif
        } else {
            _ErrorHandle(EAFNOSUPPORT);
        }
        break;
        
    case RTM_GET:
        if (psockaddr->sa_family == AF_INET) {
            sstSend = __routeRtmGet4(pmsghdr);
            
#if LWIP_IPV6
        } else if (psockaddr->sa_family == AF_INET6) {
            sstSend = __routeRtmGet6(pmsghdr);
#endif
        } else {
            _ErrorHandle(EAFNOSUPPORT);
        }
        break;
        
    default:
        _ErrorHandle(ENOSYS);                                           /*  ϵͳδʵ��                  */
        break;
    }
    
    if (bNeedUpdateReader) {
        __routeUpdateReader(pafroute, ERROR_NONE);                      /*  update remote reader        */
    }
    __AF_ROUTE_UNLOCK();
    
    return  (sstSend);
}
/*********************************************************************************************************
** ��������: route_sendmsg
** ��������: sendmsg
** �䡡��  : pafroute  route file
**           msg       ��Ϣ
**           flags     flag
** �䡡��  : NUM ���ݳ���
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
ssize_t  route_sendmsg (AF_ROUTE_T  *pafroute, const struct msghdr *msg, int flags)
{
    ssize_t     sstSendLen;
    ssize_t     sstTotal;
    
    if (!msg) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    
    if (msg->msg_iovlen == 1) {
        sstTotal = route_send(pafroute, msg->msg_iov->iov_base, msg->msg_iov->iov_len, flags);
        
    } else {
        int             i;
        struct iovec   *msg_iov;
        size_t          msg_iovlen;
        
        msg_iov    = msg->msg_iov;
        msg_iovlen = msg->msg_iovlen;
    
        for (i = 0; i < msg_iovlen; i++) {
            if ((msg_iov[i].iov_len == 0) || (msg_iov[i].iov_base == LW_NULL)) {
                _ErrorHandle(EINVAL);
                return  (PX_ERROR);
            }
        }
        
        for (i = 0, sstTotal = 0; i < msg_iovlen; i++) {
            sstSendLen = route_send(pafroute, msg_iov[i].iov_base, msg_iov[i].iov_len, flags);
            if (sstSendLen != msg_iov[i].iov_len) {
                return  (sstTotal);
            }
            sstTotal += sstSendLen;
        }
    }
    
    return  (sstTotal);
}
/*********************************************************************************************************
** ��������: route_recv
** ��������: ��·�ɱ�
** �䡡��  : pafroute  route file
**           mem       buffer
**           len       buffer len
**           flags     flag
** �䡡��  : NUM
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
ssize_t  route_recv (AF_ROUTE_T  *pafroute, void *mem, size_t len, int flags)
{
    ssize_t             sstRecv = 0;
    ULONG               ulError;
    AF_ROUTE_N         *prouten;
    AF_ROUTE_Q         *prouteq;
    struct rt_msghdr   *pmsghdr;
    
    if (!mem || !len) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    
    if (flags & MSG_WAITALL) {                                          /*  Ŀǰ���ǲ������            */
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }

    __AF_ROUTE_LOCK();
    do {
        if (pafroute->ROUTE_iShutDFlag & __AF_ROUTE_SHUTD_R) {
            __AF_ROUTE_UNLOCK();
            _ErrorHandle(ENOTCONN);                                     /*  �����Ѿ��ر�                */
            return  (sstRecv);
        }
    
        prouteq = &pafroute->ROUTE_rtq;
        
        if (prouteq->RTQ_pmonoHeader) {
            prouten = (AF_ROUTE_N *)prouteq->RTQ_pmonoHeader;
            if (prouten->RTM_stLen > len) {
                __AF_ROUTE_UNLOCK();
                _ErrorHandle(ENOBUFS);                                  /*  ������̫С                  */
                return  (sstRecv);
            }
            
            pmsghdr = (struct rt_msghdr *)(prouten + 1);
            lib_memcpy(mem, pmsghdr, prouten->RTM_stLen);
            sstRecv = (ssize_t)prouten->RTM_stLen;
            
            if (!(flags & MSG_PEEK)) {
                _list_mono_allocate_seq(&prouteq->RTQ_pmonoHeader,
                                        &prouteq->RTQ_pmonoTail);
                prouteq->RTQ_stTotal -= prouten->RTM_stLen;
                mem_free(prouten);
            }
            break;
        
        } else {
            if (__AF_ROUTE_IS_NBIO(pafroute, flags)) {                  /*  ������ IO                   */
                __AF_ROUTE_UNLOCK();
                _ErrorHandle(EWOULDBLOCK);                              /*  ��Ҫ���¶�                  */
                return  (sstRecv);
            }
            
            __AF_ROUTE_UNLOCK();
            ulError = __AF_ROUTE_WAIT(pafroute);                        /*  �ȴ�����                    */
            if (ulError) {
                _ErrorHandle(EWOULDBLOCK);                              /*  �ȴ���ʱ                    */
                return  (sstRecv);
            }
            __AF_ROUTE_LOCK();
        }
    } while (1);
    __AF_ROUTE_UNLOCK();
    
    return  (sstRecv);
}
/*********************************************************************************************************
** ��������: route_recvmsg
** ��������: recvmsg
** �䡡��  : pafroute  route file
**           msg       ��Ϣ
**           flags     flag
** �䡡��  : NUM ���ݳ���
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
ssize_t  route_recvmsg (AF_ROUTE_T  *pafroute, struct msghdr *msg, int flags)
{
    ssize_t     sstRecvLen;
    ssize_t     sstTotal;

    if (!msg) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    
    msg->msg_flags = 0;
    
    if (msg->msg_iovlen == 1) {
        sstTotal = route_recv(pafroute, msg->msg_iov->iov_base, msg->msg_iov->iov_len, flags);
        
    } else {
        int             i;
        struct iovec   *msg_iov;
        size_t          msg_iovlen;
        
        msg_iov    = msg->msg_iov;
        msg_iovlen = msg->msg_iovlen;
    
        for (i = 0; i < msg_iovlen; i++) {
            if ((msg_iov[i].iov_len == 0) || (msg_iov[i].iov_base == LW_NULL)) {
                _ErrorHandle(EINVAL);
                return  (PX_ERROR);
            }
        }
        
        for (i = 0, sstTotal = 0; i < msg_iovlen; i++) {
            sstRecvLen = route_recv(pafroute, msg_iov[i].iov_base, msg_iov[i].iov_len, flags);
            if (sstRecvLen != msg_iov[i].iov_len) {
                return  (sstTotal);
            }
            sstTotal += sstRecvLen;
        }
    }
    
    return  (sstTotal);
}
/*********************************************************************************************************
** ��������: route_setsockopt
** ��������: setsockopt
** �䡡��  : pafroute  route file
**           level     level
**           optname   option
**           optval    option value
**           optlen    option value len
** �䡡��  : ERROR
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
INT  route_setsockopt (AF_ROUTE_T  *pafroute, int level, int optname, const void *optval, socklen_t optlen)
{
    INT   iRet = PX_ERROR;
    
    if (!optval || optlen < sizeof(INT)) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }

    __AF_ROUTE_LOCK();
    switch (level) {
    
    case SOL_SOCKET:
        switch (optname) {
        
        case SO_RCVBUF:
            pafroute->ROUTE_stMaxBufSize = *(INT *)optval;
            if (pafroute->ROUTE_stMaxBufSize < __AF_ROUTE_DEF_BUFMIN) {
                pafroute->ROUTE_stMaxBufSize = __AF_ROUTE_DEF_BUFMIN;
            } else if (pafroute->ROUTE_stMaxBufSize > __AF_ROUTE_DEF_BUFMAX) {
                pafroute->ROUTE_stMaxBufSize = __AF_ROUTE_DEF_BUFMAX;
            }
            iRet = ERROR_NONE;
            break;
            
        case SO_RCVTIMEO:
            if (optlen == sizeof(struct timeval)) {
                pafroute->ROUTE_ulRecvTimeout = __routeTvToTicks((struct timeval *)optval);
            } else {
                pafroute->ROUTE_ulRecvTimeout = __routeMsToTicks(*(INT *)optval);
            }
            iRet = ERROR_NONE;
            break;
    
        default:
            _ErrorHandle(ENOSYS);
            break;
        }
        break;
        
    default:
        _ErrorHandle(ENOPROTOOPT);
        break;
    }
    __AF_ROUTE_UNLOCK();
    
    return  (iRet);
}
/*********************************************************************************************************
** ��������: route_getsockopt
** ��������: getsockopt
** �䡡��  : pafroute  route file
**           level     level
**           optname   option
**           optval    option value
**           optlen    option value len
** �䡡��  : ERROR
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
INT  route_getsockopt (AF_ROUTE_T  *pafroute, int level, int optname, void *optval, socklen_t *optlen)
{
    INT   iRet = PX_ERROR;
    
    if (!optval || *optlen < sizeof(INT)) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }

    __AF_ROUTE_LOCK();
    switch (level) {
    
    case SOL_SOCKET:
        switch (optname) {
        
        case SO_RCVBUF:
            *(INT *)optval = pafroute->ROUTE_stMaxBufSize;
            iRet = ERROR_NONE;
            break;
            
        case SO_RCVTIMEO:
            if (*optlen == sizeof(struct timeval)) {
                __routeTicksToTv(pafroute->ROUTE_ulRecvTimeout, (struct timeval *)optval);
            } else {
                *(INT *)optval = (INT)__routeTicksToMs(pafroute->ROUTE_ulRecvTimeout);
            }
            iRet = ERROR_NONE;
            break;
            
        default:
            _ErrorHandle(ENOSYS);
            break;
        }
        break;
        
    default:
        _ErrorHandle(ENOPROTOOPT);
        break;
    }
    __AF_ROUTE_UNLOCK();
    
    return  (iRet);
}
/*********************************************************************************************************
** ��������: route_shutdown
** ��������: shutdown
** �䡡��  : pafroute  route file
**           how       SHUT_RD  SHUT_WR  SHUT_RDWR
** �䡡��  : ERROR
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
INT  route_shutdown (AF_ROUTE_T  *pafroute, int how)
{
    _ErrorHandle(ENOSYS);
    return  (PX_ERROR);
}
/*********************************************************************************************************
** ��������: route_ioctl
** ��������: ioctl
** �䡡��  : pafroute  route file
**           iCmd      ����
**           pvArg     ����
** �䡡��  : ERROR
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
INT  route_ioctl (AF_ROUTE_T  *pafroute, INT  iCmd, PVOID  pvArg)
{
    INT     iRet = ERROR_NONE;
    
    switch (iCmd) {
    
    case FIOGETFL:
        if (pvArg) {
            *(INT *)pvArg = pafroute->ROUTE_iFlag;
        }
        break;
        
    case FIOSETFL:
        if ((INT)(LONG)pvArg & O_NONBLOCK) {
            pafroute->ROUTE_iFlag |= O_NONBLOCK;
        } else {
            pafroute->ROUTE_iFlag &= ~O_NONBLOCK;
        }
        break;
        
    case FIONREAD:
        if (pvArg) {
            *(INT *)pvArg = (INT)pafroute->ROUTE_rtq.RTQ_stTotal;
        }
        break;
        
    case FIONBIO:
        if (pvArg && *(INT *)pvArg) {
            pafroute->ROUTE_iFlag |= O_NONBLOCK;
        } else {
            pafroute->ROUTE_iFlag &= ~O_NONBLOCK;
        }
        break;
        
    default:
        _ErrorHandle(ENOSYS);
        iRet = PX_ERROR;
        break;
    }

    return  (iRet);
}
/*********************************************************************************************************
** ��������: __route_have_event
** ��������: ����Ӧ�Ŀ��ƿ��Ƿ�ɶ�
** �䡡��  : pafroute  route file
**           type      �¼�����
**           piSoErr   ����ȴ����¼���Ч����� SO_ERROR
** �䡡��  : ERROR
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
int __route_have_event (AF_ROUTE_T *pafroute, int type, int  *piSoErr)
{
    INT     iEvent = 0;

    switch (type) {

    case SELREAD:                                                       /*  �Ƿ�ɶ�                    */
        __AF_ROUTE_LOCK();
        if (pafroute->ROUTE_iFlag & __AF_ROUTE_SHUTD_R) {
            *piSoErr = ENOTCONN;                                        /*  ���Ѿ���ֹͣ��              */
            iEvent   = 1;
        
        } else if (pafroute->ROUTE_rtq.RTQ_pmonoHeader) {
            *piSoErr = ERROR_NONE;
            iEvent   = 1;
        }
        __AF_ROUTE_UNLOCK();
        break;
        
    case SELWRITE:                                                      /*  �Ƿ��д                    */
        __AF_ROUTE_LOCK();
        if (pafroute->ROUTE_iFlag & __AF_ROUTE_SHUTD_W) {
            *piSoErr = ESHUTDOWN;                                       /*  д�Ѿ���ֹͣ��              */
            
        } else {
            *piSoErr = ERROR_NONE;
        }
        iEvent = 1;
        __AF_ROUTE_UNLOCK();
        break;
        
    case SELEXCEPT:                                                     /*  �Ƿ��쳣                    */
        break;
    }
    
    return  (iEvent);
}
/*********************************************************************************************************
** ��������: __route_set_sockfile
** ��������: ���ö�Ӧ�� socket �ļ�
** �䡡��  : pafroute  route file
**           file      �ļ�
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
void  __route_set_sockfile (AF_ROUTE_T *pafroute, void *file)
{
    pafroute->ROUTE_sockFile = file;
}

#endif                                                                  /*  LW_CFG_NET_EN               */
                                                                        /*  LW_CFG_NET_ROUTE_EN > 0     */
/*********************************************************************************************************
  END
*********************************************************************************************************/
