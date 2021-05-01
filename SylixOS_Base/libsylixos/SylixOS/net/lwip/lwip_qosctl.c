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
** ��   ��   ��: lwip_qosctl.c
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2018 �� 07 �� 17 ��
**
** ��        ��: ioctl QoS ֧��.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "SylixOS.h"
/*********************************************************************************************************
  �ü�����
*********************************************************************************************************/
#if LW_CFG_NET_EN > 0 && LW_CFG_LWIP_IPQOS > 0
#include "netinet/ip_qos.h"
#include "netinet6/ip6_qos.h"
#include "sys/socket.h"
#include "lwip/tcpip.h"
/*********************************************************************************************************
** ��������: __qosIoctlInet
** ��������: SIOCSETIPQOS / SIOCGETIPQOS �����ӿ�
** �䡡��  : iFamily    AF_INET / AF_INET6
**           iCmd       SIOCSETIPQOS / SIOCGETIPQOS
**           pvArg      struct ipqosreq / struct ip6qosreq
** �䡡��  : ERROR or OK
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
INT  __qosIoctlInet (INT  iFamily, INT  iCmd, PVOID  pvArg)
{
    INT  iRet = PX_ERROR;

    if (!pvArg) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    
    switch (iCmd) {
    
    case SIOCSETIPQOS:
        if (iFamily == AF_INET) {
            struct ipqosreq *pipqosreq = (struct ipqosreq *)pvArg;
            
            if (pipqosreq->ip_qos_en) {
                tcpip_qos_set(1);
            
            } else {
                tcpip_qos_set(0);
            }
            
            iRet = ERROR_NONE;
        }
#if LWIP_IPV6
          else {
            struct ip6qosreq *pip6qosreq = (struct ip6qosreq *)pvArg;
            
            if (pip6qosreq->ip6_qos_en) {
                tcpip_qos_set(1);
            
            } else {
                tcpip_qos_set(0);
            }
            
            iRet = ERROR_NONE;
        }
#endif
        break;
        
    case SIOCGETIPQOS:
        if (iFamily == AF_INET) {
            struct ipqosreq *pipqosreq = (struct ipqosreq *)pvArg;
            
            pipqosreq->ip_qos_en = tcpip_qos_get();
            pipqosreq->ip_qos_reserved[0] = 0;
            pipqosreq->ip_qos_reserved[1] = 0;
            pipqosreq->ip_qos_reserved[2] = 0;
            
            iRet = ERROR_NONE;
        } 
#if LWIP_IPV6
          else {
            struct ip6qosreq *pip6qosreq = (struct ip6qosreq *)pvArg;
            
            pip6qosreq->ip6_qos_en = tcpip_qos_get();
            pip6qosreq->ip6_qos_reserved[0] = 0;
            pip6qosreq->ip6_qos_reserved[1] = 0;
            pip6qosreq->ip6_qos_reserved[2] = 0;
            
            iRet = ERROR_NONE;
        }
#endif
        break;
    
    default:
        _ErrorHandle(ENOSYS);
        break;
    }
    
    return  (iRet);
}

#endif                                                                  /*  LW_CFG_NET_EN               */
                                                                        /*  LW_CFG_LWIP_IPQOS           */
/*********************************************************************************************************
  END
*********************************************************************************************************/
