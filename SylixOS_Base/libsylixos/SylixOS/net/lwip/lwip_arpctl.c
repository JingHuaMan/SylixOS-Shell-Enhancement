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
** ��   ��   ��: lwip_arpctl.c
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2017 �� 12 �� 08 ��
**
** ��        ��: ioctl ARP ֧��.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "SylixOS.h"
/*********************************************************************************************************
  �ü�����
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
  ��������
*********************************************************************************************************/
#ifndef ETHARP_FLAG_TRY_HARD
#define ETHARP_FLAG_TRY_HARD     1
#define ETHARP_FLAG_FIND_ONLY    2
#define ETHARP_FLAG_STATIC_ENTRY 4
#endif
/*********************************************************************************************************
** ��������: __ifArpSearch
** ��������: arp ��������
** �䡡��  : netif      ����ӿ�
**           ipaddr     ip
**           ethaddr    ��̫����ַ
**           iStatic    ��̬ ?
**           ipaddr_s   �Ƚ� ip
**           netif_s    ��������ӿ�
**           flags_s    ���� flags
**           ethaddr_s  �����ַ
** �䡡��  : 0: �������� 1: �˳�����
** ȫ�ֱ���: 
** ����ģ��: 
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
** ��������: __ifArpCount
** ��������: arp ������������
** �䡡��  : netif      ����ӿ�
**           ipaddr     ip
**           ethaddr    ��̫����ַ
**           iStatic    ��̬ ?
**           puiCount   �ۼӱ���
** �䡡��  : 0: �������� 1: �˳�����
** ȫ�ֱ���:
** ����ģ��:
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
** ��������: __ifArpWalk
** ��������: arp ������ȡ���� arp ��Ϣ.
** �䡡��  : netif      ����ӿ�
**           ipaddr     ip
**           ethaddr    ��̫����ַ
**           iStatic    ��̬ ?
**           parplst    �����б�
** �䡡��  : 0: �������� 1: �˳�����
** ȫ�ֱ���:
** ����ģ��:
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
** ��������: __ifArpSet
** ��������: ���� ARP
** �䡡��  : parpreq    ����
** �䡡��  : ERROR or OK
** ȫ�ֱ���: 
** ����ģ��: 
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
** ��������: __ifArpGet
** ��������: ��ȡ ARP
** �䡡��  : parpreq    ����
** �䡡��  : ERROR or OK
** ȫ�ֱ���: 
** ����ģ��: 
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
** ��������: __ifArpDel
** ��������: ɾ�� ARP
** �䡡��  : parpreq    ����
** �䡡��  : ERROR or OK
** ȫ�ֱ���: 
** ����ģ��: 
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
** ��������: __ifArpLst
** ��������: ��ȡ ARP �б�
** �䡡��  : parplst    ����
** �䡡��  : ERROR or OK
** ȫ�ֱ���:
** ����ģ��:
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
** ��������: __ifIoctlArp
** ��������: SIOCSARP / SIOCGARP / SIOCDARP �����ӿ�
** �䡡��  : iCmd       SIOCSARP / SIOCGARP / SIOCDARP
**           pvArg      struct arpreq
** �䡡��  : ERROR or OK
** ȫ�ֱ���: 
** ����ģ��: 
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
