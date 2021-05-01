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
** ��   ��   ��: ifaddrs.h
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2017 �� 08 �� 11 ��
**
** ��        ��: ����ӿڵ�ַ.
*********************************************************************************************************/

#ifndef __IFADDRS_H
#define __IFADDRS_H

#include <sys/socket.h>

struct ifaddrs {
    struct ifaddrs  *ifa_next;
    char            *ifa_name;
    u_int            ifa_flags;
    struct sockaddr *ifa_addr;
    struct sockaddr *ifa_netmask;
    struct sockaddr *ifa_dstaddr;
    void            *ifa_data;
};

/*********************************************************************************************************
  This may have been defined in <net/if.h>.  Note that if <net/if.h> is
  to be included it must be included before this header file.
*********************************************************************************************************/

#ifndef ifa_broadaddr
#define ifa_broadaddr   ifa_dstaddr                                     /* broadcast address interface  */
#endif

#ifdef __cplusplus
extern "C" {
#endif

int  getifaddrs(struct ifaddrs **);
void freeifaddrs(struct ifaddrs *);

#ifdef __cplusplus
}
#endif                                                                  /*  __cplusplus                 */

#endif                                                                  /*  __IFADDRS_H                 */
/*********************************************************************************************************
  END
*********************************************************************************************************/
