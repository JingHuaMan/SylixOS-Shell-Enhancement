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
** ��   ��   ��: lwip_hook.h
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2018 �� 01 �� 05 ��
**
** ��        ��: lwip hook ����.
*********************************************************************************************************/

#ifndef __LWIP_HOOK_H
#define __LWIP_HOOK_H

#define __SYLIXOS_KERNEL

/*********************************************************************************************************
  ����ͷ�ļ�
*********************************************************************************************************/

#if LW_CFG_LWIP_TCP_SIG_EN > 0
#include "tcpsig/tcp_md5.h"
#endif
#if LW_CFG_NET_MROUTER > 0
#include "netinet/in.h"
#include "netinet/ip_mroute.h"
#include "net/lwip/mroute/ip4_mrt.h"
#if LWIP_IPV6 && LWIP_IPV6_MLD
#include "netinet6/in6.h"
#include "netinet6/ip6_mroute.h"
#include "net/lwip/mroute/ip6_mrt.h"
#endif
#endif

#include "net/if_event.h"
#include "net/if_iphook.h"

/*********************************************************************************************************
  netdev ���ӻ�ɾ���鲥��ַ
*********************************************************************************************************/

extern VOID  netif_set_maddr_hook(struct netif *pnetif, const ip4_addr_t *pipaddr, INT iAdd);

#if LWIP_IPV6 && LWIP_IPV6_MLD
extern VOID  netif_set_maddr6_hook(struct netif *pnetif, const ip6_addr_t *pip6addr, INT iAdd);
#endif

/*********************************************************************************************************
  IP ·��
*********************************************************************************************************/

extern struct netif *ip_route_src_hook(const ip4_addr_t *pipsrc, const ip4_addr_t *pipdest);
extern struct netif *ip_route_default_hook(const ip4_addr_t *pipsrc, const ip4_addr_t *pipdest);
extern ip4_addr_t   *ip_gw_hook(struct netif *pnetif, const ip4_addr_t *pipdest);

#if LWIP_IPV6
extern struct netif *ip6_route_src_hook(const ip6_addr_t *pip6src, const ip6_addr_t *pip6dest);
extern struct netif *ip6_route_default_hook(const ip6_addr_t *pip6src, const ip6_addr_t *pip6dest);
extern ip6_addr_t   *ip6_gw_hook(struct netif *pnetif, const ip6_addr_t *pip6dest);
#endif

/*********************************************************************************************************
  lwip ip input hook (return is the packet has been eaten)
*********************************************************************************************************/

extern int ip_input_hook(struct pbuf *p, struct netif *pnetif);

#if LWIP_IPV6
extern int ip6_input_hook(struct pbuf *p, struct netif *pnetif);
#endif

/*********************************************************************************************************
  lwip link input/output hook (for AF_PACKET and Net Defender)
*********************************************************************************************************/

extern int link_input_hook(struct pbuf *p, struct netif *pnetif);
extern int link_output_hook(struct pbuf *p, struct netif *pnetif);

/*********************************************************************************************************
  lwip vlan hook (for AF_PACKET and Net Defender)
*********************************************************************************************************/

#if LW_CFG_NET_VLAN_EN > 0
#ifdef SIZEOF_ETH_HDR
extern int ethernet_vlan_set_hook(struct netif *pnetif, struct pbuf *p, const struct eth_addr *src, 
                                  const struct eth_addr *dst, u16_t eth_type);
#ifdef SIZEOF_VLAN_HDR
extern int ethernet_vlan_check_hook(struct netif *pnetif, const struct eth_hdr *ethhdr, 
                                    const struct eth_vlan_hdr *vlanhdr);
#endif
#endif
#endif

/*********************************************************************************************************
  ip hook
*********************************************************************************************************/

extern int           lwip_ip_hook(int ip_type, int hook_type, struct pbuf *p, struct netif *in, struct netif *out);
extern struct pbuf  *lwip_ip_nat_hook(int ip_type, int hook_type, struct pbuf *p, struct netif *in, struct netif *out);

#endif                                                                  /*  __LWIP_HOOK_H               */
/*********************************************************************************************************
  END
*********************************************************************************************************/
