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
** 文   件   名: lwip_arpctl.c
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2017 年 12 月 08 日
**
** 描        述: ioctl ARP 支持.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "SylixOS.h"
/*********************************************************************************************************
  裁剪控制
*********************************************************************************************************/
#if LW_CFG_NET_EN > 0
#include "sys/socket.h"
#include "net/if_arp.h"
#include "net/if_ether.h"
#include "net/if_lock.h"
#include "lwip/netifapi.h"
#include "lwip/tcpip.h"
#include "lwip/etharp.h"
/*********************************************************************************************************
  函数声明
*********************************************************************************************************/
#ifndef ETHARP_FLAG_TRY_HARD
#define ETHARP_FLAG_TRY_HARD     1
#define ETHARP_FLAG_FIND_ONLY    2
#define ETHARP_FLAG_STATIC_ENTRY 4
#endif
/*********************************************************************************************************
** 函数名称: __ifArpSearch
** 功能描述: arp 遍历查找
** 输　入  : netif      网络接口
**           ipaddr     ip
**           ethaddr    以太网地址
**           iStatic    静态 ?
**           ipaddr_s   比较 ip
**           netif_s    保存网络接口
**           flags_s    保存 flags
**           ethaddr_s  保存地址
** 输　出  : 0: 继续遍历 1: 退出遍历
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static INT  __ifArpSearch (struct netif    *netif, 
                           ip4_addr_t      *ipaddr, 
                           struct eth_addr *ethaddr, 
                           INT              iStatic,
                           ip4_addr_t      *ipaddr_s, 
                           struct netif   **netif_s, 
                           int             *flags_s, 
                           struct eth_addr *ethaddr_s)
{
    if (ipaddr->addr == ipaddr_s->addr) {
        *netif_s = netif;
        if (iStatic) {
            *flags_s = ATF_INUSE | ATF_COM | ATF_PERM;
        } else {
            *flags_s = ATF_INUSE | ATF_COM;
        }
        MEMCPY(ethaddr_s->addr, ethaddr->addr, ETH_HWADDR_LEN);
        return  (1);
    }
    
    return  (0);
}
/*********************************************************************************************************
** 函数名称: __ifArpCount
** 功能描述: arp 遍历计算数量
** 输　入  : netif      网络接口
**           ipaddr     ip
**           ethaddr    以太网地址
**           iStatic    静态 ?
**           puiCount   累加变量
** 输　出  : 0: 继续遍历 1: 退出遍历
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT  __ifArpCount (struct netif    *netif,
                          ip4_addr_t      *ipaddr,
                          struct eth_addr *ethaddr,
                          INT              iStatic,
                          UINT            *puiCount)
{
    (*puiCount)++;
    return  (0);
}
/*********************************************************************************************************
** 函数名称: __ifArpWalk
** 功能描述: arp 遍历获取所有 arp 信息.
** 输　入  : netif      网络接口
**           ipaddr     ip
**           ethaddr    以太网地址
**           iStatic    静态 ?
**           parplst    缓冲列表
** 输　出  : 0: 继续遍历 1: 退出遍历
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT  __ifArpWalk (struct netif       *netif,
                         ip4_addr_t         *ipaddr,
                         struct eth_addr    *ethaddr,
                         INT                 iStatic,
                         struct arpreq_list *parplst)
{
    if (parplst->arpl_num < parplst->arpl_bcnt) {
        struct arpreq  *parpreq = &parplst->arpl_buf[parplst->arpl_num];
        parpreq->arp_pa.sa_family = AF_INET;
        parpreq->arp_pa.sa_len    = sizeof(struct sockaddr_in);
        ((struct sockaddr_in *)&parpreq->arp_pa)->sin_addr.s_addr = ipaddr->addr;
        parpreq->arp_ha.sa_len    = sizeof(struct sockaddr);
        parpreq->arp_ha.sa_family = AF_UNSPEC;
        MEMCPY(parpreq->arp_ha.sa_data, ethaddr->addr, ETH_ALEN);
        netif_get_name(netif, parpreq->arp_dev);
        if (iStatic) {
            parpreq->arp_flags = ATF_INUSE | ATF_COM | ATF_PERM;
        } else {
            parpreq->arp_flags = ATF_INUSE | ATF_COM;
        }
        parplst->arpl_num++;
        return  (0);

    } else {
        return  (1);
    }
}
/*********************************************************************************************************
** 函数名称: __ifArpSet
** 功能描述: 设置 ARP
** 输　入  : parpreq    参数
** 输　出  : ERROR or OK
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static INT  __ifArpSet (const struct arpreq  *parpreq)
{
    err_t                err;
    u8_t                 flags;
    ip4_addr_t           ipaddr;
    struct eth_addr      ethaddr;
    struct netif        *netif;
    
    if (parpreq->arp_pa.sa_family != AF_INET) {
        _ErrorHandle(EPROTOTYPE);
        return  (PX_ERROR);
    }
    if (parpreq->arp_ha.sa_family != AF_UNSPEC) {
        _ErrorHandle(EPROTOTYPE);
        return  (PX_ERROR);
    }
    if (parpreq->arp_flags & ATF_PERM) {
        flags = ETHARP_FLAG_TRY_HARD | ETHARP_FLAG_STATIC_ENTRY;
    } else {
        flags = ETHARP_FLAG_TRY_HARD;
    }
    
    ipaddr.addr = ((struct sockaddr_in *)&parpreq->arp_pa)->sin_addr.s_addr;
    netif = netif_find(parpreq->arp_dev);
    if (!netif) {
        LOCK_TCPIP_CORE();
        netif = ip4_route_src(LW_NULL, &ipaddr);
        UNLOCK_TCPIP_CORE();
        if (!netif) {
            _ErrorHandle(EHOSTUNREACH);
            return  (PX_ERROR);
        }
    }
    MEMCPY(ethaddr.addr, parpreq->arp_ha.sa_data, ETH_ALEN);
    
    err = netifapi_arp_update(netif, &ipaddr, &ethaddr, flags);
    if (err) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: __ifArpGet
** 功能描述: 获取 ARP
** 输　入  : parpreq    参数
** 输　出  : ERROR or OK
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static INT  __ifArpGet (struct arpreq  *parpreq)
{
    int                  arp_flags;
    ip4_addr_t           ipaddr;
    struct eth_addr      ethaddr;
    struct netif        *netif;
    
    if (parpreq->arp_pa.sa_family != AF_INET) {
        _ErrorHandle(EPROTOTYPE);
        return  (PX_ERROR);
    }
    ipaddr.addr = ((struct sockaddr_in *)&parpreq->arp_pa)->sin_addr.s_addr;
    
    netif = LW_NULL;
    netifapi_arp_traversal(LW_NULL, __ifArpSearch, &ipaddr, &netif, &arp_flags, 
                           &ethaddr, LW_NULL, LW_NULL);
    if (!netif) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    
    parpreq->arp_flags        = arp_flags;
    parpreq->arp_ha.sa_len    = sizeof(struct sockaddr);
    parpreq->arp_ha.sa_family = AF_UNSPEC;
    MEMCPY(parpreq->arp_ha.sa_data, ethaddr.addr, ETH_ALEN);
    netif_get_name(netif, parpreq->arp_dev);
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: __ifArpDel
** 功能描述: 删除 ARP
** 输　入  : parpreq    参数
** 输　出  : ERROR or OK
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static INT  __ifArpDel (const struct arpreq  *parpreq)
{
    err_t                err;
    ip4_addr_t           ipaddr;
    
    if (parpreq->arp_pa.sa_family != AF_INET) {
        _ErrorHandle(EPROTOTYPE);
        return  (PX_ERROR);
    }
    ipaddr.addr = ((struct sockaddr_in *)&parpreq->arp_pa)->sin_addr.s_addr;
    
    err = netifapi_arp_remove(&ipaddr, NETIFAPI_ARP_PERM);
    if (err) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: __ifArpLst
** 功能描述: 获取 ARP 列表
** 输　入  : parplst    参数
** 输　出  : ERROR or OK
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT  __ifArpLst (struct arpreq_list *parplst)
{
    UINT  uiTotal = 0;

    parplst->arpl_num = 0;

    netifapi_arp_traversal(LW_NULL, __ifArpCount, &uiTotal,
                           LW_NULL, LW_NULL, LW_NULL, LW_NULL, LW_NULL);
    parplst->arpl_total = uiTotal;
    if (!parplst->arpl_bcnt || !parplst->arpl_buf) {
        return  (ERROR_NONE);
    }
    netifapi_arp_traversal(LW_NULL, __ifArpWalk, parplst,
                           LW_NULL, LW_NULL, LW_NULL, LW_NULL, LW_NULL);

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: __ifIoctlArp
** 功能描述: SIOCSARP / SIOCGARP / SIOCDARP 命令处理接口
** 输　入  : iCmd       SIOCSARP / SIOCGARP / SIOCDARP
**           pvArg      struct arpreq
** 输　出  : ERROR or OK
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
INT  __ifIoctlArp (INT  iCmd, PVOID  pvArg)
{
    struct arpreq   *parpreq = (struct arpreq *)pvArg;
    INT              iRet    = PX_ERROR;
    
    if (!parpreq) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    
    switch (iCmd) {
    
    case SIOCSARP:
        LWIP_IF_LIST_LOCK(LW_FALSE);
        iRet = __ifArpSet(parpreq);
        LWIP_IF_LIST_UNLOCK();
        break;
        
    case SIOCGARP:
        LWIP_IF_LIST_LOCK(LW_FALSE);
        iRet = __ifArpGet(parpreq);
        LWIP_IF_LIST_UNLOCK();
        break;
    
    case SIOCDARP:
        LWIP_IF_LIST_LOCK(LW_FALSE);
        iRet = __ifArpDel(parpreq);
        LWIP_IF_LIST_UNLOCK();
        break;
        
    case SIOCLSTARP:
        LWIP_IF_LIST_LOCK(LW_FALSE);
        iRet = __ifArpLst((struct arpreq_list *)pvArg);
        LWIP_IF_LIST_UNLOCK();
        break;

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
