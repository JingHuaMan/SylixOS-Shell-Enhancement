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
** ��   ��   ��: if_param.h
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2015 �� 09 �� 20 ��
**
** ��        ��: ����ӿ����ò�����ȡ.
*********************************************************************************************************/

#ifndef __IF_PARAM_H
#define __IF_PARAM_H

#include <netinet/in.h>

/*********************************************************************************************************
  �ü�����
*********************************************************************************************************/
#if LW_CFG_NET_EN > 0

/*********************************************************************************************************
  ������ϵͳ, ���������������ʼ������ʹ��, ������Ϊ��Ҫ������������ʵ��.
  
  ��������ļ���ʽ���� /etc/ifparam.ini

  [dm9000a]
  enable=1
  ipaddr=192.168.1.2
  netmask=255.255.255.0
  gateway=192.168.1.1
  default=1
  mac=00:11:22:33:44:55   # ��������û�� MAC ��ַ, ���򲻽������� MAC
  ipv6_auto_cfg=1         # ����� SylixOS ��Ϊ IPv6 ·����, �� ipv6_auto_cfg=0
  tcp_ack_freq=2          # TCP Delay ACK ��ӦƵ�� (2~127), Ĭ��Ϊ 2, 
                            �Ƚ��������ܺʹ��� MSS �������ݰ��������� ACK
  tcp_wnd=8192            # TCP window (tcp_wnd > 2 * MSS) && (tcp_wnd < (0xffffu << TCP_RCV_SCALE))
  
  txqueue=0               # >0 ��ʾʹ���첽���з��͹��� (16 ~ 4096)
  txqblock=1              # 1: ���Ͷ�����������ʱ���еȴ� (ͨ��Ϊ 1, խ�����������Ϊ 0)
  
  ipaddr_6=fec0::c0a8:10  # ���һ�� IPv6 ��ַ
  gateway_6=fec0::c0a8:1  # ���һ�� IPv6 ���ص�ַ
  
  mipaddr=10.0.0.2        # ���һ������ IP ��ַ
  mnetmask=255.0.0.0
  mgateway=10.0.0.1
  
  mipaddr=172.168.0.2     # ���һ������ IP ��ַ
  mnetmask=255.255.0.0
  mgateway=172.168.0.2
  
  ����
  
  [dm9000a]
  enable=1
  dhcp=1
  dhcp6=1
  mac=00:11:22:33:44:55   # ��������û�� MAC ��ַ, ���򲻽������� MAC
*********************************************************************************************************/
/*********************************************************************************************************
  resolver ��������ļ����� /etc/resolv.conf

  nameserver x.x.x.x
*********************************************************************************************************/

#ifdef __cplusplus
extern "C" {
#endif                                                                  /*  __cplusplus                 */

LW_API void  *if_param_load(const char *name);
LW_API void   if_param_unload(void *pifparam);
LW_API int    if_param_getenable(void *pifparam, int *enable);
LW_API int    if_param_getdefault(void *pifparam, int *def);
LW_API int    if_param_getdhcp(void *pifparam, int *dhcp);
LW_API int    if_param_getdhcp6(void *pifparam, int *dhcp);
LW_API int    if_param_getaodv(void *pifparam, int *aodv);
LW_API int    if_param_gettxqueue(void *pifparam, int *txqueue);
LW_API int    if_param_gettxqblock(void *pifparam, int *txqblock);
LW_API int    if_param_ipv6autocfg(void *pifparam, int *autocfg);
LW_API int    if_param_tcpackfreq(void *pifparam, int *tcpaf);
LW_API int    if_param_tcpwnd(void *pifparam, int *tcpwnd);
LW_API int    if_param_getipaddr(void *pifparam, ip4_addr_t *ipaddr);
LW_API int    if_param_getinaddr(void *pifparam, struct in_addr *inaddr);
LW_API int    if_param_getnetmask(void *pifparam, ip4_addr_t *mask);
LW_API int    if_param_getinnetmask(void *pifparam, struct in_addr *mask);
LW_API int    if_param_getgw(void *pifparam, ip4_addr_t *gw);
LW_API int    if_param_getingw(void *pifparam, struct in_addr *gw);
LW_API int    if_param_getmac(void *pifparam, char *mac, size_t  sz);
LW_API void   if_param_syncdns(void);

#if LW_CFG_NET_IPV6 > 0
LW_API int    if_param_getipaddr_6(void *pifparam, int  idx, ip6_addr_t *ipaddr);
LW_API int    if_param_getgw_6(void *pifparam, ip6_addr_t *ipaddr);
#endif                                                                  /*  LW_CFG_NET_IPV6 > 0         */

#if LW_CFG_NET_NETDEV_MIP_EN > 0
LW_API int    if_param_getmipaddr(void *pifparam, int  idx, ip4_addr_t *ipaddr);
LW_API int    if_param_getmnetmask(void *pifparam, int  idx, ip4_addr_t *mask);
LW_API int    if_param_getmgw(void *pifparam, int  idx, ip4_addr_t *gw);
#endif                                                                  /*  LW_CFG_NET_NETDEV_MIP_EN    */

#ifdef __cplusplus
}
#endif                                                                  /*  __cplusplus                 */

#endif                                                                  /*  LW_CFG_NET_EN               */
#endif                                                                  /*  __IF_ETHER_H                */
/*********************************************************************************************************
  END
*********************************************************************************************************/
