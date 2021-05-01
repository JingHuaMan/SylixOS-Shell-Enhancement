/*********************************************************************************************************
**
**                                    中国软件开源组织
**
**                                   嵌入式实时操作系统
**
**                                SylixOS(TM)  LW : long wing
**
**                               Copyright All Rights Reserved
**
**--------------文件信息--------------------------------------------------------------------------------
**
** 文   件   名: lwip_ifctl.c
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2017 年 12 月 08 日
**
** 描        述: ioctl 网络接口支持.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "SylixOS.h"
/*********************************************************************************************************
  裁剪控制
*********************************************************************************************************/
#if LW_CFG_NET_EN > 0
#include "net/if.h"
#include "net/if_arp.h"
#include "net/if_hwaddr.h"
#include "net/if_types.h"
#include "net/if_lock.h"
#include "net/if_flags.h"
#include "sys/socket.h"
#include "lwip/netif.h"
#include "lwip/netifapi.h"
#include "lwip/tcpip.h"
#include "netif/lowpan6.h"
#include "netif/lowpan6_ble.h"
#if LW_CFG_NET_WIRELESS_EN > 0
#include "net/if_wireless.h"
#include "./wireless/lwip_wl.h"
#endif                                                                  /*  LW_CFG_NET_WIRELESS_EN > 0  */
#include "netdev/netdev.h"
#if LW_CFG_NET_NETDEV_MIP_EN > 0
#include "netdev/netdev_mip.h"
#endif
/*********************************************************************************************************
** 函数名称: __ifConfSize
** 功能描述: 获得网络接口列表保存所需要的内存大小
** 输　入  : piSize    所需内存大小
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static VOID __ifConfSize (INT  *piSize)
{
           INT       iNum = 0;
    struct netif    *pnetif;
    
    NETIF_FOREACH(pnetif) {
        iNum += sizeof(struct ifreq);
    }
    
    *piSize = iNum;
}
/*********************************************************************************************************
** 函数名称: __ifConf
** 功能描述: 获得网络接口列表操作
** 输　入  : pifconf   列表保存缓冲
** 输　出  : PX_ERROR or ERROR_NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static INT __ifConf (struct ifconf  *pifconf)
{
           INT           iSize;
           INT           iNum = 0;
    struct netif        *pnetif;
    struct ifreq        *pifreq;
    struct sockaddr_in  *psockaddrin;
    
    if (!pifconf->ifc_req) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    
    iSize  = pifconf->ifc_len / sizeof(struct ifreq);                   /*  缓冲区个数                  */
    pifreq = pifconf->ifc_req;
    
    NETIF_FOREACH(pnetif) {
        if (iNum >= iSize) {
            break;
        }
        netif_get_name(pnetif, pifreq->ifr_name);
        psockaddrin = (struct sockaddr_in *)&(pifreq->ifr_addr);
        psockaddrin->sin_len    = sizeof(struct sockaddr_in);
        psockaddrin->sin_family = AF_INET;
        psockaddrin->sin_port   = 0;
        psockaddrin->sin_addr.s_addr = netif_ip4_addr(pnetif)->addr;
        
        iNum++;
        pifreq++;
    }
    
    pifconf->ifc_len = iNum * sizeof(struct ifreq);                     /*  读取个数                    */

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: __ifReq6Size
** 功能描述: 获得网络接口 IPv6 地址数目
** 输　入  : ifreq6    ifreq6 请求控制块
** 输　出  : 处理结果
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
#if LWIP_IPV6

static INT __ifReq6Size (struct in6_ifreq  *pifreq6)
{
    INT              i;
    INT              iNum   = 0;
    struct netif    *pnetif = netif_get_by_index(pifreq6->ifr6_ifindex);
    
    if (pnetif == LW_NULL) {
        _ErrorHandle(EADDRNOTAVAIL);                                    /*  未找到指定的网络接口        */
        return  (PX_ERROR);
    }
    
    for (i = 0; i < LWIP_IPV6_NUM_ADDRESSES; i++) {
        if (ip6_addr_isvalid(pnetif->ip6_addr_state[i])) {
            iNum++;
        }
    }
    
    pifreq6->ifr6_len = iNum * sizeof(struct in6_ifr_addr);             /*  回写实际大小                */
    
    return  (ERROR_NONE);
}

#endif                                                                  /*  LWIP_IPV6                   */
/*********************************************************************************************************
** 函数名称: __ifSubIoctlIf
** 功能描述: 网络接口 ioctl 操作 (针对网卡接口)
** 输　入  : iCmd      命令
**           pvArg     参数
** 输　出  : 处理结果
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static INT  __ifSubIoctlIf (INT  iCmd, PVOID  pvArg)
{
    INT              i, iIsUp, iFlags;
    INT              iRet   = PX_ERROR;
    struct ifreq    *pifreq = LW_NULL;
    struct netif    *pnetif;
    
    pifreq = (struct ifreq *)pvArg;
    
    pnetif = netif_find(pifreq->ifr_name);
    if (pnetif == LW_NULL) {
        _ErrorHandle(EADDRNOTAVAIL);                                    /*  未找到指定的网络接口        */
        return  (iRet);
    }

    switch (iCmd) {
    
    case SIOCGIFFLAGS:                                                  /*  获取网卡 flag               */
        pifreq->ifr_flags = netif_get_flags(pnetif);
        iRet = ERROR_NONE;
        break;
        
    case SIOCSIFFLAGS:                                                  /*  设置网卡 flag               */
        iFlags = netif_get_flags(pnetif);
        if (iFlags != pifreq->ifr_flags) {
            if (!pnetif->ioctl) {
                break;
            }
            iRet = pnetif->ioctl(pnetif, SIOCSIFFLAGS, pvArg);
            if (iRet < ERROR_NONE) {
                break;
            }
            if (pifreq->ifr_flags & IFF_PROMISC) {
                pnetif->flags2 |= NETIF_FLAG2_PROMISC;
            } else {
                pnetif->flags2 |= ~NETIF_FLAG2_PROMISC;
            }
            if (pifreq->ifr_flags & IFF_ALLMULTI) {
                pnetif->flags2 |= NETIF_FLAG2_ALLMULTI;
            } else {
                pnetif->flags2 |= ~NETIF_FLAG2_ALLMULTI;
            }
            if (pifreq->ifr_flags & IFF_UP) {
                netifapi_netif_set_up(pnetif);
            } else {
                netifapi_netif_set_down(pnetif);
            }
        }
        iRet = ERROR_NONE;
        break;
        
    case SIOCGIFTYPE:                                                   /*  获得网卡类型                */
        pifreq->ifr_type = pnetif->link_type;                           /*  与 SNMP 类型相同            */
        iRet = ERROR_NONE;
        break;
        
    case SIOCGIFINDEX:                                                  /*  获得网卡 index              */
        pifreq->ifr_ifindex = netif_get_index(pnetif);
        iRet = ERROR_NONE;
        break;
        
    case SIOCGIFMTU:                                                    /*  获得网卡 mtu                */
        pifreq->ifr_mtu = pnetif->mtu;
        iRet = ERROR_NONE;
        break;
        
    case SIOCSIFMTU:                                                    /*  设置网卡 mtu                */
        if (pifreq->ifr_mtu != pnetif->mtu) {
            if (pnetif->ioctl) {
                if (pnetif->ioctl(pnetif, SIOCSIFMTU, pvArg) == 0) {
                    pnetif->mtu = pifreq->ifr_mtu;
                    iRet = ERROR_NONE;
                }
            } else {
                _ErrorHandle(ENOSYS);
            }
        } else {
            iRet = ERROR_NONE;
        }
        break;
        
    case SIOCGIFHWADDR:                                                 /*  获得物理地址                */
        if (pnetif->hwaddr_len && (pnetif->hwaddr_len <= IFHWADDR_MAXLEN)) {
            if (pnetif->ar_hrd != ARPHRD_VOID) {
                pifreq->ifr_hwaddr.sa_len    = pnetif->ar_hrd >> 8;
                pifreq->ifr_hwaddr.sa_family = pnetif->ar_hrd & 0xff;   /*  设置网卡地址类型            */
            } else {
                pifreq->ifr_hwaddr.sa_len    = 0;
                pifreq->ifr_hwaddr.sa_family = ARPHRD_ETHER;
            }
            for (i = 0; i < pnetif->hwaddr_len; i++) {
                pifreq->ifr_hwaddr.sa_data[i] = pnetif->hwaddr[i];
            }
            HALALEN_FROM_SA(&pifreq->ifr_hwaddr) = pnetif->hwaddr_len;
            iRet = ERROR_NONE;
        } else {
            _ErrorHandle(EINVAL);
        }
        break;
        
    case SIOCSIFHWADDR:                                                 /*  设置 mac 地址               */
        if (pnetif->hwaddr_len && (pnetif->hwaddr_len <= IFHWADDR_MAXLEN)) {
            iIsUp = netif_is_up(pnetif);
            if (pnetif->ioctl) {
                netifapi_netif_set_down(pnetif);                        /*  关闭网口                    */
                iRet = pnetif->ioctl(pnetif, SIOCSIFHWADDR, pvArg);
                if (iRet == ERROR_NONE) {
                    for (i = 0; i < pnetif->hwaddr_len; i++) {
                        pnetif->hwaddr[i] = pifreq->ifr_hwaddr.sa_data[i];
                    }
                }
                if (iIsUp) {
                    netifapi_netif_set_up(pnetif);                      /*  重启网口                    */
                }
            } else {
                _ErrorHandle(ENOTSUP);
            }
        } else {
            _ErrorHandle(EINVAL);
        }
        break;
        
    case SIOCGIFMETRIC:                                                 /*  获得网卡测度                */
        pifreq->ifr_metric = pnetif->metric;
        iRet = ERROR_NONE;
        break;
        
    case SIOCSIFMETRIC:                                                 /*  设置网卡测度                */
        pnetif->metric = pifreq->ifr_metric;                            /*  目前不做处理                */
        iRet = ERROR_NONE;
        break;
        
    case SIOCADDMULTI:                                                  /*  设置组播滤波器              */
    case SIOCDELMULTI:
        if (pnetif->ioctl) {
            iRet = pnetif->ioctl(pnetif, iCmd, pvArg);
        }
        break;
        
    case SIOCGIFTCPAF:                                                  /*  获得 tcp ack freq           */
        pifreq->ifr_tcpaf = netif_get_tcp_ack_freq(pnetif);
        iRet = ERROR_NONE;
        break;
        
    case SIOCSIFTCPAF:                                                  /*  设置 tcp ack freq           */
        if ((pifreq->ifr_tcpaf >= LWIP_NETIF_TCP_ACK_FREQ_MIN) && 
            (pifreq->ifr_tcpaf <= LWIP_NETIF_TCP_ACK_FREQ_MAX)) {
            netif_set_tcp_ack_freq(pnetif, (u8_t)pifreq->ifr_tcpaf);
            iRet = ERROR_NONE;
        } else {
            _ErrorHandle(EINVAL);
        }
        break;
        
    case SIOCGIFTCPWND:                                                 /*  获得 tcp window             */
        pifreq->ifr_tcpwnd = netif_get_tcp_wnd(pnetif);
        iRet = ERROR_NONE;
        break;
        
    case SIOCSIFTCPWND:                                                 /*  设置 tcp window             */
        if ((pifreq->ifr_tcpwnd <= (0xffffu << TCP_RCV_SCALE)) && 
            (pifreq->ifr_tcpwnd >= (2 * TCP_MSS))) {
            netif_set_tcp_wnd(pnetif, (u32_t)pifreq->ifr_tcpaf);
            iRet = ERROR_NONE;
        } else {
            _ErrorHandle(EINVAL);
        }
        break;
        
    case SIOCGIFPFLAGS:                                                 /*  获取私有 flags              */
        pifreq->ifr_flags = netif_get_priv_flags(pnetif);
        iRet = ERROR_NONE;
        break;
        
    case SIOCSIFPFLAGS:                                                 /*  设置私有 flags              */
        _ErrorHandle(ENOSYS);
        break;
    }
    
    return  (iRet);
}
/*********************************************************************************************************
** 函数名称: __ifSubIoctl4
** 功能描述: 网络接口 ioctl 操作 (针对 ipv4)
** 输　入  : iCmd      命令
**           pvArg     参数
** 输　出  : 处理结果
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static INT  __ifSubIoctl4 (INT  iCmd, PVOID  pvArg)
{
           INT           iRet   = PX_ERROR;
    struct ifreq        *pifreq = LW_NULL;
    struct netif        *pnetif;
    struct sockaddr_in  *psockaddrin;
    
    pifreq = (struct ifreq *)pvArg;
    
    pnetif = netif_find(pifreq->ifr_name);
    if (pnetif == LW_NULL) {
        _ErrorHandle(EADDRNOTAVAIL);                                    /*  未找到指定的网络接口        */
        return  (iRet);
    }
    
    switch (iCmd) {                                                     /*  命令预处理                  */
    
    case SIOCGIFADDR:                                                   /*  获取地址操作                */
    case SIOCGIFNETMASK:
    case SIOCGIFDGWADDR:
    case SIOCGIFDSTADDR:
    case SIOCGIFBRDADDR:
        psockaddrin = (struct sockaddr_in *)&(pifreq->ifr_addr);
        psockaddrin->sin_len    = sizeof(struct sockaddr_in);
        psockaddrin->sin_family = AF_INET;
        psockaddrin->sin_port   = 0;
        break;
        
    case SIOCSIFADDR:                                                   /*  设置地址操作                */
    case SIOCSIFNETMASK:
    case SIOCSIFDGWADDR:
    case SIOCSIFDSTADDR:
    case SIOCSIFBRDADDR:
        psockaddrin = (struct sockaddr_in *)&(pifreq->ifr_addr);
        break;
        
    default:
        return  (iRet);
    }
    
    switch (iCmd) {                                                     /*  命令处理器                  */
        
    case SIOCGIFADDR:                                                   /*  获取网卡 IP                 */
        psockaddrin->sin_addr.s_addr = netif_ip4_addr(pnetif)->addr;
        iRet = ERROR_NONE;
        break;
    
    case SIOCGIFNETMASK:                                                /*  获取网卡 mask               */
        psockaddrin->sin_addr.s_addr = netif_ip4_netmask(pnetif)->addr;
        iRet = ERROR_NONE;
        break;
        
    case SIOCGIFDGWADDR:                                                /*  默认网关地址                */
        psockaddrin->sin_addr.s_addr = netif_ip4_gw(pnetif)->addr;
        iRet = ERROR_NONE;
        break;

    case SIOCGIFDSTADDR:                                                /*  获取网卡目标地址            */
        if ((pnetif->flags & NETIF_FLAG_BROADCAST) == 0) {
            psockaddrin->sin_addr.s_addr = netif_ip4_gw(pnetif)->addr;
            iRet = ERROR_NONE;
        } else {
            _ErrorHandle(EINVAL);
        }
        break;
        
    case SIOCGIFBRDADDR:                                                /*  获取网卡广播地址            */
        if (pnetif->flags & NETIF_FLAG_BROADCAST) {
            psockaddrin->sin_addr.s_addr = (netif_ip4_addr(pnetif)->addr 
                                         | (~(netif_ip4_netmask(pnetif)->addr)));
            iRet = ERROR_NONE;
        } else {
            _ErrorHandle(EINVAL);
        }
        break;
        
    case SIOCSIFADDR:                                                   /*  设置网卡地址                */
        if (psockaddrin->sin_family == AF_INET) {
            ip4_addr_t ipaddr;
            ipaddr.addr = psockaddrin->sin_addr.s_addr;
            LOCK_TCPIP_CORE();                                          /*  必须 lock 协议栈            */
            netif_set_ipaddr(pnetif, &ipaddr);
            UNLOCK_TCPIP_CORE();
            iRet = ERROR_NONE;
        } else {
            _ErrorHandle(EAFNOSUPPORT);
        }
        break;
        
    case SIOCSIFNETMASK:                                                /*  设置网卡掩码                */
        if (psockaddrin->sin_family == AF_INET) {
            ip4_addr_t ipaddr;
            ipaddr.addr = psockaddrin->sin_addr.s_addr;
            LOCK_TCPIP_CORE();                                          /*  必须 lock 协议栈            */
            netif_set_netmask(pnetif, &ipaddr);
            UNLOCK_TCPIP_CORE();
            iRet = ERROR_NONE;
        } else {
            _ErrorHandle(EAFNOSUPPORT);
        }
        break;
        
    case SIOCSIFDGWADDR:                                                /*  设置默认网关地址            */
        if (psockaddrin->sin_family == AF_INET) {
            ip4_addr_t ipaddr;
            ipaddr.addr = psockaddrin->sin_addr.s_addr;
            LOCK_TCPIP_CORE();                                          /*  必须 lock 协议栈            */
            netif_set_gw(pnetif, &ipaddr);
            UNLOCK_TCPIP_CORE();
            iRet = ERROR_NONE;
        } else {
            _ErrorHandle(EAFNOSUPPORT);
        }
        break;

    case SIOCSIFDSTADDR:                                                /*  设置网卡目标地址            */
        if ((pnetif->flags & NETIF_FLAG_BROADCAST) == 0) {
            ip4_addr_t ipaddr;
            ipaddr.addr = psockaddrin->sin_addr.s_addr;
            LOCK_TCPIP_CORE();                                          /*  必须 lock 协议栈            */
            netif_set_gw(pnetif, &ipaddr);
            UNLOCK_TCPIP_CORE();
            iRet = ERROR_NONE;
        } else {
            _ErrorHandle(EINVAL);
        }
        break;
        
    case SIOCSIFBRDADDR:                                                /*  设置网卡广播地址            */
        if (pnetif->flags & NETIF_FLAG_BROADCAST) {
            iRet = ERROR_NONE;
        } else {
            _ErrorHandle(EINVAL);
        }
        break;
    }
    
    return  (iRet);
}
/*********************************************************************************************************
** 函数名称: __ifSubIoctlAlias4
** 功能描述: 网络接口 ioctl 操作 (针对添加删除 IP 操作)
** 输　入  : iCmd      命令
**           pvArg     参数
** 输　出  : 处理结果
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static INT  __ifSubIoctlAlias4 (INT  iCmd, PVOID  pvArg)
{
#if LW_CFG_NET_NETDEV_MIP_EN > 0
           INT           iRet    = PX_ERROR;
    struct ifreq        *pifreq  = LW_NULL;
    struct ifaliasreq   *pifareq = LW_NULL;
    struct netif        *pnetif;
    struct netdev       *pnetdev;
    struct sockaddr_in  *psockaddrin;
    
    pifreq = (struct ifreq *)pvArg;                                     /*  两个结构接口名偏移量相同    */
    
    pnetif = netif_find(pifreq->ifr_name);
    if (pnetif == LW_NULL) {
        _ErrorHandle(EADDRNOTAVAIL);                                    /*  未找到指定的网络接口        */
        return  (iRet);
    }
    
    pnetdev = netdev_find_by_ifname(pifreq->ifr_name);
    if (pnetdev == LW_NULL) {
        _ErrorHandle(EADDRNOTAVAIL);                                    /*  未找到指定的网络设备        */
        return  (iRet);
    }
    
    switch (iCmd) {                                                     /*  命令处理器                  */
    
    case SIOCDIFADDR:                                                   /*  删除一个辅助 IP             */
        psockaddrin = (struct sockaddr_in *)&(pifreq->ifr_addr);
        if (psockaddrin->sin_family == AF_INET) {
            ip4_addr_t ipaddr;
            ipaddr.addr = psockaddrin->sin_addr.s_addr;
            iRet = netdev_mipif_delete(pnetdev, &ipaddr);
        } else {
            _ErrorHandle(EAFNOSUPPORT);
        }
        break;
        
    case SIOCAIFADDR:                                                   /*  添加一个辅助 IP             */
        pifareq = (struct ifaliasreq *)pvArg;
        if (pifareq->ifra_addr.sa_family == AF_INET) {
            ip4_addr_t ipaddr, netmask;
            psockaddrin = (struct sockaddr_in *)&(pifareq->ifra_addr);
            ipaddr.addr = psockaddrin->sin_addr.s_addr;
            psockaddrin = (struct sockaddr_in *)&(pifareq->ifra_mask);
            netmask.addr = psockaddrin->sin_addr.s_addr;
            iRet = netdev_mipif_add(pnetdev, &ipaddr, &netmask, LW_NULL);
        } else {
            _ErrorHandle(EAFNOSUPPORT);
        }
        break;
    }
    
    return  (iRet);
    
#else
    _ErrorHandle(ENOSYS);
    return  (PX_ERROR);
#endif                                                                  /*  LW_CFG_NET_NETDEV_MIP_EN    */
}
/*********************************************************************************************************
** 函数名称: __ifSubIoctl6
** 功能描述: 网络接口 ioctl 操作 (针对 ipv6)
** 输　入  : iCmd      命令
**           pvArg     参数
** 输　出  : 处理结果
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
#if LWIP_IPV6

static INT  __ifSubIoctl6 (INT  iCmd, PVOID  pvArg)
{
           INT           i;
           INT           iSize;
           INT           iNum = 0;
           INT           iRet = PX_ERROR;
           ip6_addr_t    ip6addr;
    struct in6_ifreq    *pifreq6 = LW_NULL;
    struct in6_ifr_addr *pifr6addr;
    struct netif        *pnetif;
           s8_t          idx;
           err_t         err;
    
    pifreq6 = (struct in6_ifreq *)pvArg;
    
    pnetif = netif_get_by_index(pifreq6->ifr6_ifindex);
    if (pnetif == LW_NULL) {
        _ErrorHandle(EADDRNOTAVAIL);                                    /*  未找到指定的网络接口        */
        return  (iRet);
    }
    
    if (netif_is_mipif(pnetif)) {
        _ErrorHandle(ENOSYS);
        return  (iRet);
    }
    
    iSize = pifreq6->ifr6_len / sizeof(struct in6_ifr_addr);            /*  缓冲区个数                  */
    pifr6addr = pifreq6->ifr6_addr_array;
    
    switch (iCmd) {                                                     /*  命令处理器                  */
    
    case SIOCGIFADDR6:                                                  /*  获取网卡 IP                 */
        for (i = 0; i < LWIP_IPV6_NUM_ADDRESSES; i++) {
            if (iNum >= iSize) {
                break;
            }
            if (ip6_addr_isvalid(pnetif->ip6_addr_state[i])) {
                inet6_addr_from_ip6addr(&pifr6addr->ifr6a_addr, ip_2_ip6(&pnetif->ip6_addr[i]));
                if (ip6_addr_isloopback(ip_2_ip6(&pnetif->ip6_addr[i]))) {
                    pifr6addr->ifr6a_prefixlen = 128;
                } else if (ip6_addr_islinklocal(ip_2_ip6(&pnetif->ip6_addr[i]))) {
                    pifr6addr->ifr6a_prefixlen = 6;
                } else {
                    pifr6addr->ifr6a_prefixlen = 64;                    /*  TODO: 目前无法获取          */
                }
                iNum++;
                pifr6addr++;
            }
        }
        pifreq6->ifr6_len = iNum * sizeof(struct in6_ifr_addr);         /*  回写实际大小                */
        iRet = ERROR_NONE;
        break;
        
    case SIOCSIFADDR6:                                                  /*  添加一个 IPv6 地址          */
        if (iSize != 1) {                                               /*  每次只能设置一个 IP 地址    */
            _ErrorHandle(EOPNOTSUPP);
            break;
        }
        if (IN6_IS_ADDR_LOOPBACK(&pifr6addr->ifr6a_addr) ||
            IN6_IS_ADDR_LINKLOCAL(&pifr6addr->ifr6a_addr)) {            /*  不能手动设置这两种类型的地址*/
            _ErrorHandle(EADDRNOTAVAIL);
            break;
        }
        inet6_addr_to_ip6addr(&ip6addr, &pifr6addr->ifr6a_addr);
        LOCK_TCPIP_CORE();                                              /*  必须 lock 协议栈            */
        err = netif_add_ip6_address(pnetif, &ip6addr, LW_NULL);
        UNLOCK_TCPIP_CORE();
        if (err) {
            _ErrorHandle(ENOSPC);
            break;
        }
        iRet = ERROR_NONE;
        break;
        
    case SIOCDIFADDR6:                                                  /*  删除一个 IPv6 地址          */
        if (iSize != 1) {                                               /*  每次只能设置一个 IP 地址    */
            _ErrorHandle(EOPNOTSUPP);
            break;
        }
        inet6_addr_to_ip6addr(&ip6addr, &pifr6addr->ifr6a_addr);
        LOCK_TCPIP_CORE();                                              /*  必须 lock 协议栈            */
        idx = netif_get_ip6_addr_match(pnetif, &ip6addr);
        if (idx >= 0) {
            netif_ip6_addr_set_state(pnetif, idx, IP6_ADDR_INVALID);
        }
        UNLOCK_TCPIP_CORE();
        if (idx < 0) {
            _ErrorHandle(EADDRNOTAVAIL);
            break;
        }
        iRet = ERROR_NONE;
        break;
        
    default:
        _ErrorHandle(ENOSYS);                                           /*  TODO: 其他命令暂未实现      */
        break;
    }
    
    return  (iRet);
}

#endif                                                                  /*  LWIP_IPV6                   */
/*********************************************************************************************************
** 函数名称: __ifSubIoctlStats
** 功能描述: INET 网络统计接口 ioctl 操作
** 输　入  : iCmd      命令
**           pvArg     参数
** 输　出  : 处理结果
** 全局变量:
** 调用模块:
*********************************************************************************************************/
INT  __ifSubIoctlStats (INT  iCmd, PVOID  pvArg)
{
#define MIB2_NETIF(netif)   (&((netif)->mib2_counters))

    INT                 iRet = PX_ERROR;
    struct ifstatreq   *pifstat;
    struct netif       *pnetif;
    struct netdev      *pnetdev;

    pifstat = (struct ifstatreq *)pvArg;

    pnetif = netif_find(pifstat->ifrs_name);
    if (pnetif == LW_NULL) {
        _ErrorHandle(EADDRNOTAVAIL);                                    /*  未找到指定的网络接口        */
        return  (iRet);
    }

    if (netif_is_mipif(pnetif)) {
        pnetif = netif_get_masterif(pnetif);
        if (pnetif == LW_NULL) {
            _ErrorHandle(EADDRNOTAVAIL);                                /*  未找到指定的网络接口        */
            return  (iRet);
        }
    }

    switch (iCmd) {

    case SIOCGIFSTATS:
        pifstat->ifrs_mtu        = pnetif->mtu;
        pifstat->ifrs_collisions = MIB2_NETIF(pnetif)->ifcollisions;
        pifstat->ifrs_baudrate   = pnetif->link_speed;
        pifstat->ifrs_ipackets   = MIB2_NETIF(pnetif)->ifinucastpkts + MIB2_NETIF(pnetif)->ifinnucastpkts;
        pifstat->ifrs_ierrors    = MIB2_NETIF(pnetif)->ifinerrors;
        pifstat->ifrs_opackets   = MIB2_NETIF(pnetif)->ifoutucastpkts + MIB2_NETIF(pnetif)->ifoutnucastpkts;
        pifstat->ifrs_oerrors    = MIB2_NETIF(pnetif)->ifouterrors;
        pifstat->ifrs_ibytes     = MIB2_NETIF(pnetif)->ifinoctets;
        pifstat->ifrs_obytes     = MIB2_NETIF(pnetif)->ifoutoctets;
        pifstat->ifrs_imcasts    = MIB2_NETIF(pnetif)->ifinnucastpkts;
        pifstat->ifrs_omcasts    = MIB2_NETIF(pnetif)->ifoutnucastpkts;
        pifstat->ifrs_iqdrops    = MIB2_NETIF(pnetif)->ifindiscards;
        pifstat->ifrs_noproto    = MIB2_NETIF(pnetif)->ifinunknownprotos;
        pnetdev = (netdev_t *)(pnetif->state);
        if (pnetdev && (pnetdev->magic_no == NETDEV_MAGIC)) {
            pifstat->ifrs_baudrate = pnetdev->speed;
        }
        iRet = ERROR_NONE;
        break;
    }

    return  (iRet);
}
/*********************************************************************************************************
** 函数名称: __ifSubIoctlCommon
** 功能描述: 通用网络接口 ioctl 操作
** 输　入  : iCmd      命令
**           pvArg     参数
** 输　出  : 处理结果
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static INT  __ifSubIoctlCommon (INT  iCmd, PVOID  pvArg)
{
    INT     iRet = PX_ERROR;
    
    switch (iCmd) {
    
    case SIOCGIFCONF: {                                                 /*  获得网卡列表                */
            struct ifconf *pifconf = (struct ifconf *)pvArg;
            iRet = __ifConf(pifconf);
            break;
        }
        
    case SIOCGIFNUM: {                                                  /*  获得网络接口数量            */
            if (pvArg) {
                *(INT *)pvArg = (INT)netif_get_total();
                iRet = ERROR_NONE;
            } else {
                _ErrorHandle(EINVAL);
            }
            break;
        }
    
    case SIOCGSIZIFCONF: {                                              /*  SIOCGIFCONF 所需缓冲大小    */
            INT  *piSize = (INT *)pvArg;
            __ifConfSize(piSize);
            iRet = ERROR_NONE;
            break;
        }
    
#if LWIP_IPV6
    case SIOCGSIZIFREQ6: {                                              /*  获得指定网口 ipv6 地址数量  */
            struct in6_ifreq *pifreq6 = (struct in6_ifreq *)pvArg;
            iRet = __ifReq6Size(pifreq6);
            break;
        }
#endif                                                                  /*  LWIP_IPV6                   */
    }
    
    return  (iRet);
}
/*********************************************************************************************************
** 函数名称: __ifIoctlInet
** 功能描述: INET 网络接口 ioctl 操作
** 输　入  : iCmd      命令
**           pvArg     参数
** 输　出  : 处理结果
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
INT  __ifIoctlInet (INT  iCmd, PVOID  pvArg)
{
    INT     iRet = PX_ERROR;
    
    if (pvArg == LW_NULL) {
        _ErrorHandle(EINVAL);
        return  (iRet);
    }
    
    if (iCmd == SIOCGIFNAME) {                                          /*  获得网卡名                  */
        struct ifreq *pifreq = (struct ifreq *)pvArg;
        if (if_indextoname(pifreq->ifr_ifindex, pifreq->ifr_name)) {
            iRet = ERROR_NONE;
        }
        return  (iRet);
    }
    
    switch (iCmd) {                                                     /*  命令预处理                  */
    
    case SIOCGIFCONF:                                                   /*  通用网络接口操作            */
    case SIOCGIFNUM:
    case SIOCGSIZIFCONF:
#if LWIP_IPV6
    case SIOCGSIZIFREQ6:
#endif
        LWIP_IF_LIST_LOCK(LW_FALSE);                                    /*  进入临界区                  */
        iRet = __ifSubIoctlCommon(iCmd, pvArg);
        LWIP_IF_LIST_UNLOCK();                                          /*  退出临界区                  */
        break;
    
    case SIOCSIFFLAGS:                                                  /*  基本网络接口操作            */
    case SIOCGIFFLAGS:
    case SIOCGIFTYPE:
    case SIOCGIFINDEX:
    case SIOCGIFMTU:
    case SIOCSIFMTU:
    case SIOCGIFHWADDR:
    case SIOCSIFHWADDR:
    case SIOCGIFMETRIC:
    case SIOCSIFMETRIC:
    case SIOCADDMULTI:
    case SIOCDELMULTI:
    case SIOCGIFTCPAF:
    case SIOCSIFTCPAF:
    case SIOCGIFTCPWND:
    case SIOCSIFTCPWND:
    case SIOCGIFPFLAGS:
    case SIOCSIFPFLAGS:
        LWIP_IF_LIST_LOCK(LW_FALSE);                                    /*  进入临界区                  */
        iRet = __ifSubIoctlIf(iCmd, pvArg);
        LWIP_IF_LIST_UNLOCK();                                          /*  退出临界区                  */
        break;
    
    case SIOCGIFADDR:                                                   /*  ipv4 操作                   */
    case SIOCGIFNETMASK:
    case SIOCGIFDGWADDR:
    case SIOCGIFDSTADDR:
    case SIOCGIFBRDADDR:
    case SIOCSIFADDR:
    case SIOCSIFNETMASK:
    case SIOCSIFDGWADDR:
    case SIOCSIFDSTADDR:
    case SIOCSIFBRDADDR:
        LWIP_IF_LIST_LOCK(LW_FALSE);                                    /*  进入临界区                  */
        iRet = __ifSubIoctl4(iCmd, pvArg);
        LWIP_IF_LIST_UNLOCK();                                          /*  退出临界区                  */
        break;
        
    case SIOCDIFADDR:
        LWIP_IF_LIST_LOCK(LW_TRUE);                                     /*  进入临界区                  */
        iRet = __ifSubIoctlAlias4(iCmd, pvArg);
        LWIP_IF_LIST_UNLOCK();                                          /*  退出临界区                  */
        break;
    
    case SIOCAIFADDR:
        LWIP_IF_LIST_LOCK(LW_FALSE);                                    /*  进入临界区                  */
        iRet = __ifSubIoctlAlias4(iCmd, pvArg);
        LWIP_IF_LIST_UNLOCK();                                          /*  退出临界区                  */
        break;
        
#if LWIP_IPV6
    case SIOCGIFADDR6:                                                  /*  ipv6 操作                   */
    case SIOCGIFNETMASK6:
    case SIOCGIFDSTADDR6:
    case SIOCSIFADDR6:
    case SIOCSIFNETMASK6:
    case SIOCSIFDSTADDR6:
    case SIOCDIFADDR6:
        LWIP_IF_LIST_LOCK(LW_FALSE);                                    /*  进入临界区                  */
        iRet = __ifSubIoctl6(iCmd, pvArg);
        LWIP_IF_LIST_UNLOCK();                                          /*  退出临界区                  */
        break;
#endif                                                                  /*  LWIP_IPV6                   */
    
    case SIOCGIFSTATS:                                                  /*  网络统计                    */
        LWIP_IF_LIST_LOCK(LW_FALSE);                                    /*  进入临界区                  */
        iRet = __ifSubIoctlStats(iCmd, pvArg);
        LWIP_IF_LIST_UNLOCK();                                          /*  退出临界区                  */
        break;

    default:
        _ErrorHandle(ENOSYS);
        break;
    }
    
    return  (iRet);
}
/*********************************************************************************************************
** 函数名称: __ifIoctlWireless
** 功能描述: WEXT 网络接口 ioctl 操作
** 输　入  : iCmd      命令
**           pvArg     参数
** 输　出  : 处理结果
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
#if LW_CFG_NET_WIRELESS_EN > 0

INT  __ifIoctlWireless (INT  iCmd, PVOID  pvArg)
{
    INT     iRet = PX_ERROR;
    
    if ((iCmd >= SIOCIWFIRST) && (iCmd <= SIOCIWLASTPRIV)) {            /*  无线连接设置                */
        LWIP_IF_LIST_LOCK(LW_FALSE);                                    /*  进入临界区                  */
        iRet = wext_handle_ioctl(iCmd, (struct ifreq *)pvArg);
        LWIP_IF_LIST_UNLOCK();                                          /*  退出临界区                  */
        if (iRet) {
            _ErrorHandle(lib_abs(iRet));
            iRet = PX_ERROR;
        }
    } else {
        _ErrorHandle(ENOSYS);
    }
    
    return  (iRet);
}

#endif                                                                  /*  LW_CFG_NET_WIRELESS_EN > 0  */
/*********************************************************************************************************
** 函数名称: __ifIoctlPrivate
** 功能描述: 网络接口私有 ioctl 操作
** 输　入  : iCmd      命令
**           pvArg     参数
** 输　出  : 处理结果
** 全局变量:
** 调用模块:
*********************************************************************************************************/
INT  __ifIoctlPrivate (INT  iCmd, PVOID  pvArg)
{
    INT            iRet = PX_ERROR;
    struct ifreq  *pifreq;
    struct netif  *pnetif;

    if (pvArg == LW_NULL) {
        _ErrorHandle(EINVAL);
        return  (iRet);
    }

    pifreq = (struct ifreq *)pvArg;

    LWIP_IF_LIST_LOCK(LW_FALSE);                                        /*  进入临界区                  */
    pnetif = netif_find(pifreq->ifr_name);
    if (pnetif == LW_NULL) {
        _ErrorHandle(EADDRNOTAVAIL);

    } else {
        if (pnetif->ioctl) {
            iRet = pnetif->ioctl(pnetif, iCmd, pvArg);
        } else {
            _ErrorHandle(ENOSYS);
        }
    }
    LWIP_IF_LIST_UNLOCK();                                              /*  退出临界区                  */

    return  (iRet);
}
/*********************************************************************************************************
** 函数名称: __ifIoctlPacket
** 功能描述: PACKET 网络接口 ioctl 操作
** 输　入  : iCmd      命令
**           pvArg     参数
** 输　出  : 处理结果
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
INT  __ifIoctlPacket (INT  iCmd, PVOID  pvArg)
{
    INT     iRet = PX_ERROR;
    
    if (pvArg == LW_NULL) {
        _ErrorHandle(EINVAL);
        return  (iRet);
    }
    
    if (iCmd == SIOCGIFNAME) {                                          /*  获得网卡名                  */
        struct ifreq *pifreq = (struct ifreq *)pvArg;
        if (if_indextoname(pifreq->ifr_ifindex, pifreq->ifr_name)) {
            iRet = ERROR_NONE;
        }
        return  (iRet);
    }
    
    switch (iCmd) {                                                     /*  命令预处理                  */
    
    case SIOCGIFCONF:                                                   /*  通用网络接口操作            */
    case SIOCGIFNUM:
    case SIOCGSIZIFCONF:
#if LWIP_IPV6
    case SIOCGSIZIFREQ6:
#endif
        LWIP_IF_LIST_LOCK(LW_FALSE);                                    /*  进入临界区                  */
        iRet = __ifSubIoctlCommon(iCmd, pvArg);
        LWIP_IF_LIST_UNLOCK();                                          /*  退出临界区                  */
        break;
    
    case SIOCSIFFLAGS:                                                  /*  基本网络接口操作            */
    case SIOCGIFFLAGS:
    case SIOCGIFTYPE:
    case SIOCGIFINDEX:
    case SIOCGIFMTU:
    case SIOCSIFMTU:
    case SIOCGIFHWADDR:
    case SIOCSIFHWADDR:
    case SIOCGIFPFLAGS:
    case SIOCSIFPFLAGS:
        LWIP_IF_LIST_LOCK(LW_FALSE);                                    /*  进入临界区                  */
        iRet = __ifSubIoctlIf(iCmd, pvArg);
        LWIP_IF_LIST_UNLOCK();                                          /*  退出临界区                  */
        break;
    
    case SIOCGIFSTATS:                                                  /*  网络统计                    */
        LWIP_IF_LIST_LOCK(LW_FALSE);                                    /*  进入临界区                  */
        iRet = __ifSubIoctlStats(iCmd, pvArg);
        LWIP_IF_LIST_UNLOCK();                                          /*  退出临界区                  */
        break;

    default:
        _ErrorHandle(ENOSYS);
        break;
    }
    
    return  (iRet);
}
/*********************************************************************************************************
** 函数名称: __ifIoctlLp802154
** 功能描述: 6LowPAN 网络接口 ioctl 操作
** 输　入  : iCmd      命令
**           pifreq    参数
** 输　出  : 处理结果
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
#if LWIP_IPV6

static INT  __ifIoctlLp802154 (INT  iCmd, struct ieee802154_ifreq *pifreq)
{
    INT              iRet = PX_ERROR;
    UINT8            ucHigh, ucLow;
    struct netif    *pnetif;
    struct netdev   *pnetdev;
    ip6_addr_t       ip6addr;
    
    pnetif = netif_find(pifreq->ifr802154_name);
    if (pnetif == LW_NULL) {
        _ErrorHandle(EADDRNOTAVAIL);                                    /*  未找到指定的网络接口        */
        return  (iRet);
    }
    
    pnetdev = netdev_find_by_ifname(pifreq->ifr802154_name);
    if ((pnetdev == LW_NULL) || 
        (pnetdev->net_type != NETDEV_TYPE_LOWPAN)) {
        _ErrorHandle(EADDRNOTAVAIL);                                    /*  未找到指定的网络设备        */
        return  (iRet);
    }
    
    switch (iCmd) {                                                     /*  命令预处理                  */
    
    case SIOCG802154PANID:
        lowpan6_get_pan_id(&pifreq->ifr802154_pandid);
        iRet = ERROR_NONE;
        break;
    
    case SIOCS802154PANID:
        lowpan6_set_pan_id(pifreq->ifr802154_pandid);
        iRet = ERROR_NONE;
        break;
        
    case SIOCG802154SHRTADDR:
        lowpan6_get_short_addr(&ucHigh, &ucLow);
        pifreq->ifr802154_shortaddr = (uint16_t)((ucHigh << 16) + ucLow);
        iRet = ERROR_NONE;
        break;
    
    case SIOCS802154SHRTADDR:
        ucHigh = (UINT8)((pifreq->ifr802154_shortaddr >> 8) & 0xff);
        ucLow = (UINT8)(pifreq->ifr802154_shortaddr & 0xff);
        lowpan6_set_short_addr(ucHigh, ucLow);
        iRet = ERROR_NONE;
        break;
        
    case SIOCG802154CTX:
        if (lowpan6_get_context(pifreq->ifr802154_ctxindex, &ip6addr)) {
            _ErrorHandle(EINVAL);
        } else {
            inet6_addr_from_ip6addr(&pifreq->ifr802154_ctxaddr, &ip6addr);
            iRet = ERROR_NONE;
        }
        break;
    
    case SIOCS802154CTX:
        inet6_addr_to_ip6addr(&ip6addr, &pifreq->ifr802154_ctxaddr);
        if (lowpan6_set_context(pifreq->ifr802154_ctxindex, &ip6addr)) {
            _ErrorHandle(EINVAL);
        } else {
            iRet = ERROR_NONE;
        }
        break;

    default:
        _ErrorHandle(ENOSYS);
        break;
    }
    
    return  (iRet);
}
/*********************************************************************************************************
** 函数名称: __ifIoctlLp7668
** 功能描述: 6LowPAN 网络接口 ioctl 操作
** 输　入  : iCmd      命令
**           pifreq    参数
** 输　出  : 处理结果
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static INT  __ifIoctlLp7668 (INT  iCmd, struct rfc7668_ifreq *pifreq)
{
    INT              iRet = PX_ERROR;
    struct netif    *pnetif;
    struct netdev   *pnetdev;
    ip6_addr_t       ip6addr;
    UINT8            ucAddr[8];
    
    pnetif = netif_find(pifreq->ifr7668_name);
    if (pnetif == LW_NULL) {
        _ErrorHandle(EADDRNOTAVAIL);                                    /*  未找到指定的网络接口        */
        return  (iRet);
    }
    
    pnetdev = netdev_find_by_ifname(pifreq->ifr7668_name);
    if ((pnetdev == LW_NULL) || 
        (pnetdev->net_type != NETDEV_TYPE_LOWPAN_BLE)) {
        _ErrorHandle(EADDRNOTAVAIL);                                    /*  未找到指定的网络设备        */
        return  (iRet);
    }
    
    switch (iCmd) {                                                     /*  命令预处理                  */
    
    case SIOCG7668DSTADDR:
        break;
        
    case SIOCS7668DSTADDR:
        ucAddr[0] = (UINT8)((pifreq->ifr7668_dstaddr.s6_addr32[2] >> 24) & 0xff);
        ucAddr[1] = (UINT8)((pifreq->ifr7668_dstaddr.s6_addr32[2] >> 16) & 0xff);
        ucAddr[2] = (UINT8)((pifreq->ifr7668_dstaddr.s6_addr32[2] >> 8)  & 0xff);
        ucAddr[3] = (UINT8)((pifreq->ifr7668_dstaddr.s6_addr32[2])       & 0xff);
        ucAddr[4] = (UINT8)((pifreq->ifr7668_dstaddr.s6_addr32[3] >> 24) & 0xff);
        ucAddr[5] = (UINT8)((pifreq->ifr7668_dstaddr.s6_addr32[3] >> 16) & 0xff);
        ucAddr[6] = (UINT8)((pifreq->ifr7668_dstaddr.s6_addr32[3] >>  8) & 0xff);
        ucAddr[7] = (UINT8)((pifreq->ifr7668_dstaddr.s6_addr32[3])       & 0xff);
        rfc7668_set_peer_addr_eui64(pnetif, ucAddr, 8);
        iRet = ERROR_NONE;
        break;
            
    case SIOCG7668CTX:
        if (rfc7668_get_context(pifreq->ifr7668_ctxindex, &ip6addr)) {
            _ErrorHandle(EINVAL);
        } else {
            inet6_addr_from_ip6addr(&pifreq->ifr7668_ctxaddr, &ip6addr);
            iRet = ERROR_NONE;
        }
        break;
        
    case SIOCS7668CTX:
        inet6_addr_to_ip6addr(&ip6addr, &pifreq->ifr7668_ctxaddr);
        if (rfc7668_set_context(pifreq->ifr7668_ctxindex, &ip6addr)) {
            _ErrorHandle(EINVAL);
        } else {
            iRet = ERROR_NONE;
        }
        break;
        
    default:
        _ErrorHandle(ENOSYS);
        break;
    }
    
    return  (iRet);
}

#endif                                                                  /*  LWIP_IPV6                   */
/*********************************************************************************************************
** 函数名称: __ifIoctlLp
** 功能描述: 6LowPAN 网络接口 ioctl 操作
** 输　入  : iCmd      命令
**           pvArg     参数
** 输　出  : 处理结果
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
INT  __ifIoctlLp (INT  iCmd, PVOID  pvArg)
{
    INT  iRet = PX_ERROR;

    if (pvArg == LW_NULL) {
        _ErrorHandle(EINVAL);
        return  (iRet);
    }
    
    switch (iCmd) {                                                     /*  命令预处理                  */
    
#if LWIP_IPV6
    case SIOCG802154PANID:
    case SIOCS802154PANID:
    case SIOCG802154SHRTADDR:
    case SIOCS802154SHRTADDR:
    case SIOCG802154CTX:
    case SIOCS802154CTX:
        LWIP_IF_LIST_LOCK(LW_FALSE);                                    /*  进入临界区                  */
        iRet = __ifIoctlLp802154(iCmd, (struct ieee802154_ifreq *)pvArg);
        LWIP_IF_LIST_UNLOCK();                                          /*  退出临界区                  */
        break;
    
    case SIOCG7668DSTADDR:
    case SIOCS7668DSTADDR:
    case SIOCG7668CTX:
    case SIOCS7668CTX:
        LWIP_IF_LIST_LOCK(LW_FALSE);                                    /*  进入临界区                  */
        iRet = __ifIoctlLp7668(iCmd, (struct rfc7668_ifreq *)pvArg);
        LWIP_IF_LIST_UNLOCK();                                          /*  退出临界区                  */
        break;
#endif                                                                  /*  LWIP_IPV6                   */

    default:
        _ErrorHandle(ENOSYS);
        break;
    }
    
    return  (iRet);
}

#endif                                                                  /*  LW_CFG_NET_EN               */
/*********************************************************************************************************
  END
*********************************************************************************************************/
