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
** ��   ��   ��: lwip_iphook.h
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2017 �� 05 �� 12 ��
**
** ��        ��: lwip IP HOOK.
*********************************************************************************************************/

#ifndef __LWIP_IPHOOK_H
#define __LWIP_IPHOOK_H

#include "lwip/pbuf.h"
#include "lwip/netif.h"
#include "netdev.h"
#include "net/if.h"
#include "net/if_types.h"

/*********************************************************************************************************
  ���ݰ�����
*********************************************************************************************************/

#define IP_HOOK_V4      4
#define IP_HOOK_V6      6

/*********************************************************************************************************
  ͨ�ûص�����
  
                                               TCP/IP stack  
                                         ^                     |
                                         |                     |
                                         |                     \/
                                     [LOCAL_IN]           [LOCAL_OUT]
                                         ^                     |
                                         |                     |
                                         |                route output
                                         |                     |
                                         |                     |
                                         |                     \/
  packet input --> [PRE_ROUTING] --> route input --> [FORWARD] --> [POST_ROUTING] --> packet output 
  
  NOTICE: The NF_IP_LOCAL_OUT hook is called for packets that are created locally. Here you can see 
          that routing occurs after this hook is called: in fact, the routing code is called first 
          if you want to alter the routing, you must alter the `pbuf->if_out' field yourself.
*********************************************************************************************************/

#define IP_HT_PRE_ROUTING       0
#define IP_HT_LOCAL_IN          1
#define IP_HT_FORWARD           2
#define IP_HT_LOCAL_OUT         3
#define IP_HT_POST_ROUTING      4

/*********************************************************************************************************
  ����ص�����
*********************************************************************************************************/

#define IP_HT_NAT_PRE_ROUTING   0
#define IP_HT_NAT_POST_ROUTING  4

/*********************************************************************************************************
  ����·�ɻص�
*********************************************************************************************************/

#define IP_HT_ROUTING           0

/*********************************************************************************************************
  ͨ�ûص������ں˺���
*********************************************************************************************************/

#ifdef __cplusplus
extern "C" {
#endif                                                                  /*  __cplusplus                 */

int  net_ip_hook_add(const char *name, int (*hook)(int ip_type, int hook_type, struct pbuf *p, 
                                                   struct netif *in, struct netif *out));
int  net_ip_hook_delete(int (*hook)(int ip_type, int hook_type, struct pbuf *p, 
                                    struct netif *in, struct netif *out));
int  net_ip_hook_isadd(int (*hook)(int ip_type, int hook_type, struct pbuf *p,
                                   struct netif *in, struct netif *out), BOOL *pbIsAdd);

/*********************************************************************************************************
  ����ص������ں˺��� (���ܰ�װһ���ص�, ������ IP_HT_NAT_PRE_ROUTING, IP_HT_NAT_POST_ROUTING)
*********************************************************************************************************/

int  net_ip_hook_nat_add(struct pbuf *(*hook)(int ip_type, int hook_type, struct pbuf *p, 
                                              struct netif *in, struct netif *out));
int  net_ip_hook_nat_delete(struct pbuf *(*hook)(int ip_type, int hook_type, struct pbuf *p, 
                                                 struct netif *in, struct netif *out));
int  net_ip_hook_nat_isadd(struct pbuf *(*hook)(int ip_type, int hook_type, struct pbuf *p,
                                                struct netif *in, struct netif *out), BOOL *pbIsAdd);

/*********************************************************************************************************
  ����·�ɻص�
*********************************************************************************************************/

int  net_ip_hook_route_add(struct netif *(*hook)(int ip_type, const void *src, const void *dest));

int  net_ip_hook_route_delete(struct netif *(*hook)(int ip_type, const void *src, const void *dest));

int  net_ip_hook_route_isadd(struct netif *(*hook)(int ip_type, const void *src, const void *dest), BOOL *pbIsAdd);

/*********************************************************************************************************
  ���� pbuf ��Ա
  
  ע��: ���ڼ����Կ���, ������ֱ������ if_out ��Ա����.
*********************************************************************************************************/

void net_ip_hook_pbuf_set_ifout(struct pbuf *p, struct netif *pnetif);

/*********************************************************************************************************
  ��ȡ netif ��Ա
  
  ע��: ���ڼ����Կ���, ������ֱ�Ӵ� netif �ṹ�з�������, ����Ҫͨ�����½ӿں��������� netif �еĳ�Ա.
*********************************************************************************************************/

netdev_t          *net_ip_hook_netif_get_netdev(struct netif *pnetif);
const ip4_addr_t  *net_ip_hook_netif_get_ipaddr(struct netif *pnetif);
const ip4_addr_t  *net_ip_hook_netif_get_netmask(struct netif *pnetif);
const ip4_addr_t  *net_ip_hook_netif_get_gw(struct netif *pnetif);
#if LWIP_IPV6
const ip6_addr_t  *net_ip_hook_netif_get_ip6addr(struct netif *pnetif, int  addr_index, int *addr_state);
#endif
UINT8             *net_ip_hook_netif_get_hwaddr(struct netif *pnetif, int *hwaddr_len);
int                net_ip_hook_netif_get_index(struct netif *pnetif);
int                net_ip_hook_netif_get_name(struct netif *pnetif, char *name, size_t size);
int                net_ip_hook_netif_get_type(struct netif *pnetif, int *type);
int                net_ip_hook_netif_get_flags(struct netif *pnetif, int *flags);
UINT64             net_ip_hook_netif_get_linkspeed(struct netif *pnetif);

#ifdef __cplusplus
}
#endif                                                                  /*  __cplusplus                 */

#endif                                                                  /*  __LWIP_IPHOOK_H             */
/*********************************************************************************************************
  END
*********************************************************************************************************/
