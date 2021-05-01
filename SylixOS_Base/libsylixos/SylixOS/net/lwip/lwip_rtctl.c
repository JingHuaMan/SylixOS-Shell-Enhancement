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
** ��   ��   ��: lwip_rtctl.c
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2017 �� 12 �� 08 ��
**
** ��        ��: ioctl ·�ɱ�֧��.
*********************************************************************************************************/
#define  __SYLIXOS_RTHOOK
#define  __SYLIXOS_KERNEL
#include "SylixOS.h"
/*********************************************************************************************************
  �ü�����
*********************************************************************************************************/
#if LW_CFG_NET_EN > 0 && LW_CFG_NET_ROUTER > 0
#include "socket.h"
#include "net/if_iphook.h"
#include "lwip/ip.h"
#include "lwip/mem.h"
#include "lwip/tcpip.h"
#include "route/af_route.h"
#include "route/tcp_mss_adj.h"
/*********************************************************************************************************
** ��������: __rtAdd4
** ��������: ���һ�� IPv4 ·����Ϣ
** �䡡��  : prtentry  ·����Ϣ
** �䡡��  : ERROR or OK
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static INT  __rtAdd4 (const struct rtentry  *prtentry)
{
    INT                 iRet;
    struct rt_entry    *pentry4;
    
    pentry4 = (struct rt_entry *)mem_malloc(sizeof(struct rt_entry));
    if (!pentry4) {
        _ErrorHandle(ENOMEM);
        return  (PX_ERROR);
    }
    lib_bzero(pentry4, sizeof(struct rt_entry));
    
    rt_rtentry_to_entry(pentry4, prtentry);                             /*  ת��Ϊ rt_entry             */
    
    if (pentry4->rt_dest.addr == IPADDR_ANY) {                          /*  ����Ĭ��·��                */
        if ((pentry4->rt_gateway.addr == IPADDR_ANY) && 
            (pentry4->rt_ifname[0] == '\0')) {
            mem_free(pentry4);
            _ErrorHandle(ENETUNREACH);
            return  (PX_ERROR);
        }
        
        RT_LOCK();
        iRet = rt_change_default(&pentry4->rt_gateway, pentry4->rt_ifname);
        RT_UNLOCK();
        if (iRet < 0) {
            mem_free(pentry4);
            _ErrorHandle(ENETUNREACH);
            return  (PX_ERROR);
        }
        
        route_hook_rt_ipv4(RTM_ADD, pentry4, 0);                        /*  ��ӽ��ջ���                */
        mem_free(pentry4);
        
    } else {
        RT_LOCK();
        if (rt_find_entry(&pentry4->rt_dest, &pentry4->rt_netmask, &pentry4->rt_gateway, 
                          pentry4->rt_ifname, pentry4->rt_flags)) {
            RT_UNLOCK();
            mem_free(pentry4);
            _ErrorHandle(EEXIST);
            return  (PX_ERROR);
        }
        
        iRet = rt_add_entry(pentry4);
        if (iRet < 0) {
            RT_UNLOCK();
            mem_free(pentry4);
            _ErrorHandle(ENETUNREACH);
            return  (PX_ERROR);
        
        } else {
            route_hook_rt_ipv4(RTM_ADD, pentry4, 1);                    /*  ��ӽ��ջ���                */
            RT_UNLOCK();
        }
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: __rtAdd6
** ��������: ���һ�� IPv6 ·����Ϣ
** �䡡��  : prtentry  ·����Ϣ
** �䡡��  : ERROR or OK
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
#if LWIP_IPV6

static INT  __rtAdd6 (const struct rtentry  *prtentry)
{
    INT                 iRet;
    struct rt6_entry   *pentry6;
    
    pentry6 = (struct rt6_entry *)mem_malloc(sizeof(struct rt6_entry));
    if (!pentry6) {
        _ErrorHandle(ENOMEM);
        return  (PX_ERROR);
    }
    lib_bzero(pentry6, sizeof(struct rt6_entry));
    
    rt6_rtentry_to_entry(pentry6, prtentry);                            /*  ת��Ϊ rt6_entry            */
    
    if (ip6_addr_isany_val(pentry6->rt6_dest)) {                        /*  ����Ĭ��·��                */
        if (pentry6->rt6_ifname[0] == '\0') {
            mem_free(pentry6);
            _ErrorHandle(ENETUNREACH);
            return  (PX_ERROR);
        }
        
        RT_LOCK();
        iRet = rt6_change_default(&pentry6->rt6_gateway, pentry6->rt6_ifname);
        RT_UNLOCK();
        if (iRet < 0) {
            mem_free(pentry6);
            _ErrorHandle(ENETUNREACH);
            return  (PX_ERROR);
        }
        
        route_hook_rt_ipv6(RTM_ADD, pentry6, 0);                        /*  ��ӽ��ջ���                */
        mem_free(pentry6);
    
    } else {                                                            /*  ���·�ɱ���                */
        RT_LOCK();
        if (rt6_find_entry(&pentry6->rt6_dest, &pentry6->rt6_netmask, &pentry6->rt6_gateway, 
                           pentry6->rt6_ifname, pentry6->rt6_flags)) {
            RT_UNLOCK();
            mem_free(pentry6);
            _ErrorHandle(EEXIST);
            return  (PX_ERROR);
        }
        
        iRet = rt6_add_entry(pentry6);
        if (iRet < 0) {
            RT_UNLOCK();
            mem_free(pentry6);
            _ErrorHandle(ENETUNREACH);
            return  (PX_ERROR);
        
        } else {
            route_hook_rt_ipv6(RTM_ADD, pentry6, 1);                    /*  ��ӽ��ջ���                */
            RT_UNLOCK();
        }
    }

    return  (ERROR_NONE);
}

#endif                                                                  /*  LWIP_IPV6                   */
/*********************************************************************************************************
** ��������: __rtDel4
** ��������: ɾ��һ�� IPv4 ·����Ϣ
** �䡡��  : prtentry  ·����Ϣ
** �䡡��  : ERROR or OK
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static INT  __rtDel4 (const struct rtentry  *prtentry)
{
    INT                 iRet;
    struct rt_entry    *pentry4;
    ip4_addr_t          ipaddr;
    ip4_addr_t          ipnetmask;
    ip4_addr_t          ipgateway;
    
    inet_addr_to_ip4addr(&ipaddr,    &((struct sockaddr_in *)&prtentry->rt_dst)->sin_addr);
    inet_addr_to_ip4addr(&ipnetmask, &((struct sockaddr_in *)&prtentry->rt_genmask)->sin_addr);
    inet_addr_to_ip4addr(&ipgateway, &((struct sockaddr_in *)&prtentry->rt_gateway)->sin_addr);
    
    if (ipaddr.addr == IPADDR_ANY) {
        if ((ipgateway.addr == IPADDR_ANY) && (prtentry->rt_ifname[0] == '\0')) {
            _ErrorHandle(ENETUNREACH);
            return  (PX_ERROR);
        }
        
        pentry4 = (struct rt_entry *)mem_malloc(sizeof(struct rt_entry));
        if (pentry4) {
            RT_LOCK();
            iRet = rt_get_default(pentry4);
            if (iRet == 0) {
                iRet = rt_delete_default(&ipgateway, prtentry->rt_ifname);
                RT_UNLOCK();
                if (iRet == 0) {
                    route_hook_rt_ipv4(RTM_DELETE, pentry4, 0);
                }
            
            } else {
                RT_UNLOCK();
            }
            mem_free(pentry4);
            if (iRet) {
                _ErrorHandle(ENETUNREACH);
                return  (PX_ERROR);
            }
            
        } else {
            _ErrorHandle(ENOMEM);
            return  (PX_ERROR);
        }
        
    } else {
        RT_LOCK();
        pentry4 = rt_find_entry(&ipaddr, &ipnetmask, &ipgateway, 
                                prtentry->rt_ifname, prtentry->rt_flags);
        if (!pentry4 || pentry4->rt_type) {
            RT_UNLOCK();
            _ErrorHandle(ENETUNREACH);
            return  (PX_ERROR);
        }
        rt_delete_entry(pentry4);
        RT_UNLOCK();
        
        route_hook_rt_ipv4(RTM_DELETE, pentry4, 0);
        mem_free(pentry4);
    }
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: __rtDel6
** ��������: ɾ��һ�� IPv6 ·����Ϣ
** �䡡��  : prtentry  ·����Ϣ
** �䡡��  : ERROR or OK
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
#if LWIP_IPV6

static INT  __rtDel6 (const struct rtentry  *prtentry)
{
    INT                  iRet;
    struct rt6_entry    *pentry6;
    ip6_addr_t           ip6addr;
    ip6_addr_t           ip6netmask;
    ip6_addr_t           ip6gateway;
    
    inet6_addr_to_ip6addr(&ip6addr,    &((struct sockaddr_in6 *)&prtentry->rt_dst)->sin6_addr);
    inet6_addr_to_ip6addr(&ip6netmask, &((struct sockaddr_in6 *)&prtentry->rt_genmask)->sin6_addr);
    inet6_addr_to_ip6addr(&ip6gateway, &((struct sockaddr_in6 *)&prtentry->rt_gateway)->sin6_addr);
    
    if (ip6_addr_isany_val(ip6addr)) {
        if (ip6_addr_isany_val(ip6gateway) && (prtentry->rt_ifname[0] == '\0')) {
            _ErrorHandle(ENETUNREACH);
            return  (PX_ERROR);
        }
        
        pentry6 = (struct rt6_entry *)mem_malloc(sizeof(struct rt6_entry));
        if (pentry6) {
            RT_LOCK();
            iRet = rt6_get_default(pentry6);
            if (iRet == 0) {
                iRet = rt6_delete_default(&ip6gateway, prtentry->rt_ifname);
                RT_UNLOCK();
                if (iRet == 0) {
                    route_hook_rt_ipv6(RTM_DELETE, pentry6, 0);
                }
            
            } else {
                RT_UNLOCK();
            }
            mem_free(pentry6);
            if (iRet) {
                _ErrorHandle(ENETUNREACH);
                return  (PX_ERROR);
            }
            
        } else {
            _ErrorHandle(ENOMEM);
            return  (PX_ERROR);
        }
        
    } else {
        RT_LOCK();
        pentry6 = rt6_find_entry(&ip6addr, &ip6netmask, &ip6gateway, 
                                 prtentry->rt_ifname, prtentry->rt_flags);
        if (!pentry6 || pentry6->rt6_type) {
            RT_UNLOCK();
            _ErrorHandle(ENETUNREACH);
            return  (PX_ERROR);
        }
        rt6_delete_entry(pentry6);
        RT_UNLOCK();
        
        route_hook_rt_ipv6(RTM_DELETE, pentry6, 0);
        mem_free(pentry6);
    }
    
    return  (ERROR_NONE);
}

#endif                                                                  /*  LWIP_IPV6                   */
/*********************************************************************************************************
** ��������: __rtChg4
** ��������: �޸�һ�� IPv4 ·����Ϣ
** �䡡��  : prtentry  ·����Ϣ
** �䡡��  : ERROR or OK
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static INT  __rtChg4 (const struct rtentry  *prtentry)
{
    INT                 iRet;
    struct rt_entry    *pentry4;
    ip4_addr_t          ipaddr;
    ip4_addr_t          ipnetmask;
    ip4_addr_t          ipgateway;
    
    inet_addr_to_ip4addr(&ipaddr,    &((struct sockaddr_in *)&prtentry->rt_dst)->sin_addr);
    inet_addr_to_ip4addr(&ipnetmask, &((struct sockaddr_in *)&prtentry->rt_genmask)->sin_addr);
    inet_addr_to_ip4addr(&ipgateway, &((struct sockaddr_in *)&prtentry->rt_gateway)->sin_addr);
    
    if (ipaddr.addr == IPADDR_ANY) {                                    /*  ����Ĭ��·��                */
        if ((ipgateway.addr == IPADDR_ANY) && (prtentry->rt_ifname[0] == '\0')) {
            _ErrorHandle(ENETUNREACH);
            return  (PX_ERROR);
        }
        
        RT_LOCK();
        iRet = rt_change_default(&ipgateway, prtentry->rt_ifname);
        RT_UNLOCK();
        if (iRet < 0) {
            _ErrorHandle(ENETUNREACH);
            return  (PX_ERROR);
        }
        
        pentry4 = (struct rt_entry *)mem_malloc(sizeof(struct rt_entry));
        if (pentry4) {
            RT_LOCK();
            iRet = rt_get_default(pentry4);
            RT_UNLOCK();
            if (iRet == 0) {
                route_hook_rt_ipv4(RTM_CHANGE, pentry4, 0);             /*  ��ӽ��ջ���                */
            }
            mem_free(pentry4);
        }
        
    } else {                                                            /*  �޸�·�ɱ���                */
        RT_LOCK();
        pentry4 = rt_find_entry(&ipaddr, &ipnetmask, &ipgateway, 
                                prtentry->rt_ifname, prtentry->rt_flags);
        if (!pentry4 || pentry4->rt_type) {
            RT_UNLOCK();
            _ErrorHandle(ENETUNREACH);
            return  (PX_ERROR);
        }
        rt_delete_entry(pentry4);
        
        rt_rtentry_to_entry(pentry4, prtentry);                         /*  ת��Ϊ rt_entry             */
        
        iRet = rt_add_entry(pentry4);
        if (iRet < 0) {
            RT_UNLOCK();
            mem_free(pentry4);
            _ErrorHandle(ENETUNREACH);
            return  (PX_ERROR);
        
        } else {
            route_hook_rt_ipv4(RTM_CHANGE, pentry4, 1);                 /*  ��ӽ��ջ���                */
            RT_UNLOCK();
        }
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: __rtChg6
** ��������: ���һ�� IPv6 ·����Ϣ
** �䡡��  : prtentry  ·����Ϣ
** �䡡��  : ERROR or OK
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
#if LWIP_IPV6

static INT  __rtChg6 (const struct rtentry  *prtentry)
{
    INT                 iRet;
    struct rt6_entry   *pentry6;
    ip6_addr_t          ip6addr;
    ip6_addr_t          ip6netmask;
    ip6_addr_t          ip6gateway;
    
    inet6_addr_to_ip6addr(&ip6addr,    &((struct sockaddr_in6 *)&prtentry->rt_dst)->sin6_addr);
    inet6_addr_to_ip6addr(&ip6netmask, &((struct sockaddr_in6 *)&prtentry->rt_genmask)->sin6_addr);
    inet6_addr_to_ip6addr(&ip6gateway, &((struct sockaddr_in6 *)&prtentry->rt_gateway)->sin6_addr);
    
    if (ip6_addr_isany_val(ip6addr)) {                                  /*  ����Ĭ��·��                */
        if (prtentry->rt_ifname[0] == '\0') {
            _ErrorHandle(ENETUNREACH);
            return  (PX_ERROR);
        }
        
        RT_LOCK();
        iRet = rt6_change_default(&ip6gateway, prtentry->rt_ifname);
        RT_UNLOCK();
        if (iRet < 0) {
            _ErrorHandle(ENETUNREACH);
            return  (PX_ERROR);
        }
        
        pentry6 = (struct rt6_entry *)mem_malloc(sizeof(struct rt6_entry));
        if (pentry6) {
            RT_LOCK();
            iRet = rt6_get_default(pentry6);
            RT_UNLOCK();
            if (iRet == 0) {
                route_hook_rt_ipv6(RTM_CHANGE, pentry6, 0);             /*  ��ӽ��ջ���                */
            }
            mem_free(pentry6);
        }
    
    } else {                                                            /*  ���·�ɱ���                */
        RT_LOCK();
        pentry6 = rt6_find_entry(&ip6addr, &ip6netmask, &ip6gateway, 
                                 prtentry->rt_ifname, prtentry->rt_flags);
        if (!pentry6 || pentry6->rt6_type) {
            RT_UNLOCK();
            _ErrorHandle(ENETUNREACH);
            return  (PX_ERROR);
        }
        rt6_delete_entry(pentry6);
        
        rt6_rtentry_to_entry(pentry6, prtentry);                        /*  ת��Ϊ rt6_entry            */
        
        iRet = rt6_add_entry(pentry6);
        if (iRet < 0) {
            RT_UNLOCK();
            mem_free(pentry6);
            _ErrorHandle(ENETUNREACH);
            return  (PX_ERROR);
        
        } else {
            route_hook_rt_ipv6(RTM_CHANGE, pentry6, 1);                 /*  ��ӽ��ջ���                */
            RT_UNLOCK();
        }
    }

    return  (ERROR_NONE);
}

#endif                                                                  /*  LWIP_IPV6                   */
/*********************************************************************************************************
** ��������: __rtGet4Walk
** ��������: ���� IPv4 ·����Ϣ
** �䡡��  : pentry4   �ڲ�·�ɱ���
**           prtentry  ·����Ϣ
**           ipaddr    ƥ���ַ
**           iRet      ����ֵ
** �䡡��  : ERROR or OK
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static VOID  __rtGet4Walk (struct rt_entry  *pentry4, 
                           struct rtentry   *prtentry,
                           ip4_addr_t       *ipaddr, 
                           INT              *iRet)
{
    if (pentry4->rt_dest.addr == ipaddr->addr) {
        rt_entry_to_rtentry(prtentry, pentry4);                         /*  ת��Ϊ rtentry              */
        *iRet = ERROR_NONE;
    }
}
/*********************************************************************************************************
** ��������: __rtGet4
** ��������: ��ȡһ�� IPv4 ·����Ϣ
** �䡡��  : prtentry  ·����Ϣ
** �䡡��  : ERROR or OK
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static INT  __rtGet4 (struct rtentry  *prtentry)
{
    INT         iRet = PX_ERROR;
    ip4_addr_t  ipaddr;
    
    inet_addr_to_ip4addr(&ipaddr, &((struct sockaddr_in *)&prtentry->rt_dst)->sin_addr);
    
    lib_bzero(prtentry, sizeof(struct rtentry));
    
    RT_LOCK();
    rt_traversal_entry(__rtGet4Walk, prtentry, &ipaddr, &iRet, LW_NULL, LW_NULL, LW_NULL);
    RT_UNLOCK();
    if (iRet < 0) {
        _ErrorHandle(ENETUNREACH);
        return  (PX_ERROR);
    }
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: __rtGet6Walk
** ��������: ���� IPv6 ·����Ϣ
** �䡡��  : pentry6   �ڲ�·�ɱ���
**           prtentry  ·����Ϣ
**           ip6addr   ƥ���ַ
**           iRet      ����ֵ
** �䡡��  : ERROR or OK
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
#if LWIP_IPV6

static VOID  __rtGet6Walk (struct rt6_entry *pentry6, 
                           struct rtentry   *prtentry,
                           ip6_addr_t       *ip6addr, 
                           INT              *iRet)
{
    if (ip6_addr_cmp(&pentry6->rt6_dest, ip6addr)) {
        rt6_entry_to_rtentry(prtentry, pentry6);                        /*  ת��Ϊ rtentry              */
        *iRet = ERROR_NONE;
    }
}
/*********************************************************************************************************
** ��������: __rtGet6
** ��������: ��ȡһ�� IPv6 ·����Ϣ
** �䡡��  : prtentry  ·����Ϣ
** �䡡��  : ERROR or OK
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static INT  __rtGet6 (struct rtentry  *prtentry)
{
    INT         iRet = PX_ERROR;
    ip6_addr_t  ip6addr;
    
    inet6_addr_to_ip6addr(&ip6addr, &((struct sockaddr_in6 *)&prtentry->rt_dst)->sin6_addr);
    
    lib_bzero(prtentry, sizeof(struct rtentry));
    
    RT_LOCK();
    rt6_traversal_entry(__rtGet6Walk, prtentry, &ip6addr, &iRet, LW_NULL, LW_NULL, LW_NULL);
    RT_UNLOCK();
    if (iRet < 0) {
        _ErrorHandle(ENETUNREACH);
        return  (PX_ERROR);
    }
    
    return  (ERROR_NONE);
}

#endif                                                                  /*  LWIP_IPV6                   */
/*********************************************************************************************************
** ��������: __rtLst4Walk
** ��������: ���� IPv4 ·����Ϣ
** �䡡��  : pentry4    �ڲ�·�ɱ���
**           prtelist   ·����Ϣ����
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static VOID  __rtLst4Walk (struct rt_entry     *pentry4, 
                           struct rtentry_list *prtelist)
{
    if (prtelist->rtl_num < prtelist->rtl_bcnt) {
        struct rtentry  *prtentry = &prtelist->rtl_buf[prtelist->rtl_num];
        rt_entry_to_rtentry(prtentry, pentry4);                         /*  ת��Ϊ rtentry              */
        prtelist->rtl_num++;
    }
}
/*********************************************************************************************************
** ��������: __rtLst4
** ��������: ��ȡ���� IPv4 ·����Ϣ
** �䡡��  : prtelist  ·����Ϣ����
** �䡡��  : ERROR or OK
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static INT  __rtLst4 (struct rtentry_list *prtelist)
{
    UINT    uiTotal;
    
    prtelist->rtl_num = 0;
    
    RT_LOCK();
    rt_total_entry(&uiTotal);
    prtelist->rtl_total = uiTotal;
    if (!prtelist->rtl_bcnt || !prtelist->rtl_buf) {
        RT_UNLOCK();
        return  (ERROR_NONE);
    }
    rt_traversal_entry(__rtLst4Walk, prtelist, LW_NULL, LW_NULL, LW_NULL, LW_NULL, LW_NULL);
    RT_UNLOCK();
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: __rtLst6Walk
** ��������: ���� IPv6 ·����Ϣ
** �䡡��  : pentry6   �ڲ�·�ɱ���
**           prtelist  ·����Ϣ����
** �䡡��  : ERROR or OK
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
#if LWIP_IPV6

static VOID  __rtLst6Walk (struct rt6_entry    *pentry6, 
                           struct rtentry_list *prtelist)
{
    if (prtelist->rtl_num < prtelist->rtl_bcnt) {
        struct rtentry  *prtentry = &prtelist->rtl_buf[prtelist->rtl_num];
        rt6_entry_to_rtentry(prtentry, pentry6);                        /*  ת��Ϊ rtentry              */
        prtelist->rtl_num++;
    }
}
/*********************************************************************************************************
** ��������: __rtLst6
** ��������: ��ȡ���� IPv6 ·����Ϣ
** �䡡��  : prtelist  ·����Ϣ����
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static INT  __rtLst6 (struct rtentry_list *prtelist)
{
    UINT    uiTotal;
    
    prtelist->rtl_num = 0;
    
    RT_LOCK();
    rt6_total_entry(&uiTotal);
    prtelist->rtl_total = uiTotal;
    if (!prtelist->rtl_bcnt || !prtelist->rtl_buf) {
        RT_UNLOCK();
        return  (ERROR_NONE);
    }
    rt6_traversal_entry(__rtLst6Walk, prtelist, LW_NULL, LW_NULL, LW_NULL, LW_NULL, LW_NULL);
    RT_UNLOCK();
    
    return  (ERROR_NONE);
}

#endif                                                                  /*  LWIP_IPV6                   */
/*********************************************************************************************************
** ��������: __rtmLst4Walk
** ��������: ���� IPv4 ·����Ϣ
** �䡡��  : pentry4    �ڲ�·�ɱ���
**           prtmlist   ·����Ϣ����
** �䡡��  : ERROR or OK
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static VOID  __rtmLst4Walk (struct rt_entry       *pentry4, 
                            struct rt_msghdr_list *prtmlist)
{
    size_t  stMsgLen = rt_entry_to_msghdr(LW_NULL, pentry4, RTM_GET);

    if ((prtmlist->rtml_rsize + stMsgLen) <= prtmlist->rtml_bsize) {
        struct rt_msghdr   *pmsghdr;
        pmsghdr = (struct rt_msghdr *)((char *)prtmlist->rtml_buf + prtmlist->rtml_rsize);
        rt_entry_to_msghdr(pmsghdr, pentry4, RTM_GET);
        prtmlist->rtml_rsize += stMsgLen;
    }
}
/*********************************************************************************************************
** ��������: __rtLst4
** ��������: ��ȡ���� IPv4 ·����Ϣ
** �䡡��  : prtmlist  ·����Ϣ����
** �䡡��  : ERROR or OK
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static INT  __rtmLst4 (struct rt_msghdr_list *prtmlist)
{
    UINT    uiTotal;
    
    prtmlist->rtml_rsize = 0;
    
    RT_LOCK();
    rt_total_entry(&uiTotal);
    prtmlist->rtml_tsize = uiTotal * (sizeof(struct rt_msghdr) 
                         + (SO_ROUND_UP(sizeof(struct sockaddr_in)) * 4) 
                         +  SO_ROUND_UP(sizeof(struct sockaddr_dl)));
    if (!prtmlist->rtml_bsize || !prtmlist->rtml_buf) {
        RT_UNLOCK();
        return  (ERROR_NONE);
    }
    rt_traversal_entry(__rtmLst4Walk, prtmlist, LW_NULL, LW_NULL, LW_NULL, LW_NULL, LW_NULL);
    RT_UNLOCK();
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: __rtLst6Walk
** ��������: ���� IPv6 ·����Ϣ
** �䡡��  : pentry6   �ڲ�·�ɱ���
**           prtmlist  ·����Ϣ����
** �䡡��  : ERROR or OK
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
#if LWIP_IPV6

static VOID  __rtmLst6Walk (struct rt6_entry      *pentry6, 
                            struct rt_msghdr_list *prtmlist)
{
    size_t  stMsgLen = rt6_entry_to_msghdr(LW_NULL, pentry6, RTM_GET);
    
    if ((prtmlist->rtml_rsize + stMsgLen) <= prtmlist->rtml_bsize) {
        struct rt_msghdr   *pmsghdr;
        pmsghdr = (struct rt_msghdr *)((char *)prtmlist->rtml_buf + prtmlist->rtml_rsize);
        rt6_entry_to_msghdr(pmsghdr, pentry6, RTM_GET);
        prtmlist->rtml_rsize += stMsgLen;
    }
}
/*********************************************************************************************************
** ��������: __rtLst6
** ��������: ��ȡ���� IPv6 ·����Ϣ
** �䡡��  : prtmlist  ·����Ϣ����
** �䡡��  : ERROR or OK
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static INT  __rtmLst6 (struct rt_msghdr_list *prtmlist)
{
    UINT    uiTotal;
    
    prtmlist->rtml_rsize = 0;
    
    RT_LOCK();
    rt6_total_entry(&uiTotal);
    prtmlist->rtml_tsize = uiTotal * (sizeof(struct rt_msghdr) 
                         + (SO_ROUND_UP(sizeof(struct sockaddr_in6)) * 4) 
                         +  SO_ROUND_UP(sizeof(struct sockaddr_dl)));
    if (!prtmlist->rtml_bsize || !prtmlist->rtml_buf) {
        RT_UNLOCK();
        return  (ERROR_NONE);
    }
    rt6_traversal_entry(__rtmLst6Walk, prtmlist, LW_NULL, LW_NULL, LW_NULL, LW_NULL, LW_NULL);
    RT_UNLOCK();
    
    return  (ERROR_NONE);
}

#endif                                                                  /*  LWIP_IPV6                   */
/*********************************************************************************************************
** ��������: __rtSetTcpMssAdj
** ��������: ���� ipv4 forward TCP MSS adjust status
** �䡡��  : *piEnable  �Ƿ�ʹ��
** �䡡��  : ERROR or OK
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  __rtSetTcpMssAdj (INT *piEnable)
{
    INT  iRet;

    if (*piEnable) {
        iRet = net_ip_hook_add("tcp_forward_mss_adj", tcp_forward_mss_adj);

    } else {
        iRet = net_ip_hook_delete(tcp_forward_mss_adj);
    }

    return  (iRet);
}
/*********************************************************************************************************
** ��������: __rtGetTcpMssAdj
** ��������: ��ȡ ipv4 forward TCP MSS adjust status
** �䡡��  : *piEnable  �Ƿ�ʹ��
** �䡡��  : ERROR or OK
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  __rtGetTcpMssAdj (INT *piEnable)
{
    BOOL  bIsAdd;

    net_ip_hook_isadd(tcp_forward_mss_adj, &bIsAdd);
    *piEnable = (bIsAdd) ? 1 : 0;

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: __rtGetForwardOpt
** ��������: ��ȡ ipv4 forward ʹ������
** �䡡��  : *prtf  ת��״̬
** �䡡��  : ERROR or OK
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  __rtGetForwardOpt (struct rt_forward *prtf)
{
    prtf->rtf_ip4fw = ip4_forward_get();
#if LWIP_IPV6
    prtf->rtf_ip6fw = ip6_forward_get();
#else
    prtf->rtf_ip6fw = 0;
#endif

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: __rtSetForwardOpt
** ��������: ���� ipv4 forward ʹ������
** �䡡��  : *prtf  ת��״̬
** �䡡��  : ERROR or OK
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  __rtSetForwardOpt (const struct rt_forward *prtf)
{
    if (geteuid()) {
        _ErrorHandle(EACCES);
        return  (PX_ERROR);
    }
    
    RT_LOCK();
    ip4_forward_set((UINT8)(prtf->rtf_ip4fw ? 1 : 0));
#if LWIP_IPV6
    ip6_forward_set((UINT8)(prtf->rtf_ip6fw ? 1 : 0));
#endif
    RT_UNLOCK();

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: __rtIoctlInet
** ��������: SIOCADDRT / SIOCDELRT ... �����ӿ�
** �䡡��  : iFamily    AF_INET / AF_INET6
**           iCmd       SIOCADDRT / SIOCDELRT
**           pvArg      struct arpreq
** �䡡��  : ERROR or OK
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
INT  __rtIoctlInet (INT  iFamily, INT  iCmd, PVOID  pvArg)
{
    struct rtentry  *prtentry = (struct rtentry  *)pvArg;
    INT              iRet     = PX_ERROR;

    if (!prtentry) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    
    switch (iCmd) {
    
    case SIOCADDRT:                                                     /*  ���·����Ϣ                */
        if (iFamily == AF_INET) {
            iRet = __rtAdd4(prtentry);
        } 
#if LWIP_IPV6
          else {
            iRet = __rtAdd6(prtentry);
        }
#endif
        break;
    
    case SIOCDELRT:                                                     /*  ɾ��·����Ϣ                */
        if (iFamily == AF_INET) {
            iRet = __rtDel4(prtentry);
        }
#if LWIP_IPV6
          else {
            iRet = __rtDel6(prtentry);
        }
#endif
        break;

   case SIOCCHGRT:                                                      /*  �޸�·����Ϣ                */
        if (iFamily == AF_INET) {
            iRet = __rtChg4(prtentry);
        } 
#if LWIP_IPV6
          else {
            iRet = __rtChg6(prtentry);
        }
#endif
        break;
        
    case SIOCGETRT:                                                     /*  ��ѯ·����Ϣ                */
        if (iFamily == AF_INET) {
            iRet = __rtGet4(prtentry);
        } 
#if LWIP_IPV6
          else {
            iRet = __rtGet6(prtentry);
        }
#endif
        break;
        
    case SIOCLSTRT:                                                     /*  ·���б�                    */
        if (iFamily == AF_INET) {
            iRet = __rtLst4((struct rtentry_list *)pvArg);
        } 
#if LWIP_IPV6
          else {
            iRet = __rtLst6((struct rtentry_list *)pvArg);
        }
#endif
        break;

    case SIOCLSTRTM:                                                    /*  ·���б�                    */
        if (iFamily == AF_INET) {
            iRet = __rtmLst4((struct rt_msghdr_list *)pvArg);
        } 
#if LWIP_IPV6
          else {
            iRet = __rtmLst6((struct rt_msghdr_list *)pvArg);
        }
#endif
        break;

    case SIOCGTCPMSSADJ:
        iRet = __rtGetTcpMssAdj((INT *)pvArg);
        break;

    case SIOCSTCPMSSADJ:
        iRet = __rtSetTcpMssAdj((INT *)pvArg);
        break;

    case SIOCGFWOPT:
        iRet = __rtGetForwardOpt((struct rt_forward *)pvArg);
        break;
        
    case SIOCSFWOPT:
        iRet = __rtSetForwardOpt((struct rt_forward *)pvArg);
        break;

    default:
        _ErrorHandle(ENOSYS);
        break;
    }
    
    return  (iRet);
}

#endif                                                                  /*  LW_CFG_NET_EN > 0           */
                                                                        /*  LW_CFG_NET_ROUTER > 0       */
/*********************************************************************************************************
  END
*********************************************************************************************************/
