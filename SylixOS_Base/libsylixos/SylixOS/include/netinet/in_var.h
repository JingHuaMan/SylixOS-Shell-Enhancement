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
** ��   ��   ��: in_var.h
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2013 �� 01 �� 19 ��
**
** ��        ��: include/netinet/in_var .
*********************************************************************************************************/

#ifndef __NETINET_IN_VAR_H
#define __NETINET_IN_VAR_H

#include <sys/socket.h>
#include <net/if.h>

struct in_aliasreq {
    char                ifra_name[IFNAMSIZ];                            /*  if name, e.g. "en1"         */
    struct sockaddr_in  ifra_addr;
    struct sockaddr_in  ifra_broadaddr;
#define ifra_dstaddr    ifra_broadaddr
    struct sockaddr_in  ifra_mask;
};

#endif                                                                  /*  __NETINET_IN_VAR_H          */
/*********************************************************************************************************
  END
*********************************************************************************************************/
