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
** ��   ��   ��: ip_qos.h
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2018 �� 07 �� 17 ��
**
** ��        ��: IPv4 QoS support.
*********************************************************************************************************/

#ifndef __NETINET_IP_QOS_H
#define __NETINET_IP_QOS_H

#include <sys/types.h>
#include <sys/ioctl.h>

#include <lwip/opt.h>
#include <lwip/def.h>

#if LWIP_IPV4

#include <lwip/inet.h>

#ifdef __cplusplus
extern "C" {
#endif

/*********************************************************************************************************
  Configuration structure for SIOCSETIPQOS and SIOCGETIPQOS ioctls.
*********************************************************************************************************/

struct ipqosreq {
    u_int    ip_qos_en;
    u_int    ip_qos_reserved[3];                                /*  reserved for furture                */
};

#define SIOCSETIPQOS    _IOWR('i', 60, struct ipqosreq)
#define SIOCGETIPQOS    _IOWR('i', 61, struct ipqosreq)

#ifdef __cplusplus
}
#endif

#endif                                                          /*  LWIP_IPV4                           */
#endif                                                          /*  __NETINET_IP_QOS_H                  */
/*********************************************************************************************************
  END
*********************************************************************************************************/
