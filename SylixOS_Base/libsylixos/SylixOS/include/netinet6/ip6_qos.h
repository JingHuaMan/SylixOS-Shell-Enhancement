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
** ��   ��   ��: ip6_qos.h
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2018 �� 07 �� 17 ��
**
** ��        ��: IPv6 QoS support.
*********************************************************************************************************/

#ifndef __NETINET_IP6_QOS_H
#define __NETINET_IP6_QOS_H

#include <sys/types.h>
#include <sys/ioctl.h>

#include <lwip/opt.h>
#include <lwip/def.h>

#if LWIP_IPV6

#include <lwip/inet.h>

#ifdef __cplusplus
extern "C" {
#endif

/*********************************************************************************************************
  Configuration structure for SIOCSETIP6QOS and SIOCGETIP6QOS ioctls.
*********************************************************************************************************/

struct ip6qosreq {
    u_int    ip6_qos_en;
    u_int    ip6_qos_reserved[3];                               /*  reserved for furture                */
};

#define SIOCSETIP6QOS   _IOWR('i', 60, struct ip6qosreq)        /*  same as SIOCSETIPQOS                */
#define SIOCGETIP6QOS   _IOWR('i', 61, struct ip6qosreq)        /*  same as SIOCGETIPQOS                */

#endif                                                          /*  LWIP_IPV6                           */
#endif                                                          /*  __NETINET_IP6_QOS_H                 */
/*********************************************************************************************************
  END
*********************************************************************************************************/
