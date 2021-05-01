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
** ��   ��   ��: lwip_shell6.c
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2013 �� 09 �� 27 ��
**
** ��        ��: lwip ipv6 shell ����.
*********************************************************************************************************/
#define  __SYLIXOS_STDIO
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
#include "../SylixOS/system/include/s_system.h"
/*********************************************************************************************************
  �ü�����
*********************************************************************************************************/
#if (LW_CFG_NET_EN > 0) && (LW_CFG_SHELL_EN > 0)
#include "socket.h"
#include "net/if.h"
#include "netinet6/in6.h"
/*********************************************************************************************************
  ipv6 ������Ϣ
*********************************************************************************************************/
#if LWIP_IPV6
static const CHAR   _G_cIpv6Help[] = {
    "add/delete IPv6 address\n"
    "address   [ifname [address%prefixlen]]  add an ipv6 address for given interface\n"
    "noaddress [ifname [address%prefixlen]]  delete an ipv6 address for given interface\n"
};
/*********************************************************************************************************
** ��������: __ifreq6Init
** ��������: ͨ��������ʼ�� in6_ifreq �ṹ
** �䡡��  : pifeq6        ��Ҫ��ʼ���Ľṹ
**           pcIf          ����ӿ�
**           pcIpv6        IPv6 ��ַ
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static VOID  __ifreq6Init (struct in6_ifreq *pifeq6, CPCHAR pcIf, PCHAR pcIpv6)
{
    PCHAR pcDiv;

    pifeq6->ifr6_ifindex = if_nametoindex(pcIf);
    
    pcDiv = lib_strchr(pcIpv6, '%');
    if (pcDiv) {
        *pcDiv = PX_EOS;
        pcDiv++;
        inet6_aton(pcIpv6, &pifeq6->ifr6_addr_array->ifr6a_addr);
        pifeq6->ifr6_addr_array->ifr6a_prefixlen = lib_atoi(pcDiv);
    
    } else {
        inet6_aton(pcIpv6, &pifeq6->ifr6_addr_array->ifr6a_addr);
        pifeq6->ifr6_addr_array->ifr6a_prefixlen = 64;                  /*  default prefixlen           */
    }
}
/*********************************************************************************************************
** ��������: __tshellIpv6Address
** ��������: ϵͳ���� "ipv6 address"
** �䡡��  : iArgC         ��������
**           ppcArgV       ������
** �䡡��  : 0
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  __tshellIpv6Address (INT  iArgC, PCHAR  *ppcArgV)
{
    struct in6_ifreq    ifeq6;
    struct in6_ifr_addr ifraddr6;
    
    INT iSock;
    
    if (iArgC < 4) {
        printf("%s", _G_cIpv6Help);
        return  (ERROR_NONE);
    }
    
    ifeq6.ifr6_len = sizeof(struct in6_ifr_addr);
    ifeq6.ifr6_addr_array = &ifraddr6;
    
    __ifreq6Init(&ifeq6, ppcArgV[2], ppcArgV[3]);
    
    iSock = socket(AF_INET6, SOCK_DGRAM, IPPROTO_UDP);
    if (iSock < 0) {
        fprintf(stderr, "can not create socket error: %s\n", lib_strerror(errno));
        return  (ERROR_NONE);
    }
    
    if (ioctl(iSock, SIOCSIFADDR6, &ifeq6)) {
        INT   iErrNo = errno;
        close(iSock);
        fprintf(stderr, "can not set/add ipv6 address error: %s\n", lib_strerror(iErrNo));
        return  (ERROR_NONE);
    }
    
    close(iSock);
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: __tshellIpv6Noaddress
** ��������: ϵͳ���� "ipv6 noaddress"
** �䡡��  : iArgC         ��������
**           ppcArgV       ������
** �䡡��  : 0
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  __tshellIpv6Noaddress (INT  iArgC, PCHAR  *ppcArgV)
{
    struct in6_ifreq    ifeq6;
    struct in6_ifr_addr ifraddr6;
    
    INT iSock;
    
    if (iArgC < 4) {
        printf("%s", _G_cIpv6Help);
        return  (ERROR_NONE);
    }
    
    ifeq6.ifr6_len = sizeof(struct in6_ifr_addr);
    ifeq6.ifr6_addr_array = &ifraddr6;
    
    __ifreq6Init(&ifeq6, ppcArgV[2], ppcArgV[3]);
    
    iSock = socket(AF_INET6, SOCK_DGRAM, IPPROTO_UDP);
    if (iSock < 0) {
        fprintf(stderr, "can not create socket error: %s\n", lib_strerror(errno));
        return  (ERROR_NONE);
    }
    
    if (ioctl(iSock, SIOCDIFADDR6, &ifeq6)) {
        INT   iErrNo = errno;
        close(iSock);
        fprintf(stderr, "can not delete ipv6 address error: %s\n", lib_strerror(iErrNo));
        return  (ERROR_NONE);
    }
    
    close(iSock);
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: __tshellIpv6
** ��������: ϵͳ���� "ipv6"
** �䡡��  : iArgC         ��������
**           ppcArgV       ������
** �䡡��  : 0
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  __tshellIpv6 (INT  iArgC, PCHAR  *ppcArgV)
{
    if (iArgC < 2) {
        printf("%s", _G_cIpv6Help);
        return  (ERROR_NONE);
    }
    
    if (lib_strcmp(ppcArgV[1], "address") == 0) {                       /*  ���� ipv6 ��ַ              */
        return  (__tshellIpv6Address(iArgC, ppcArgV));
    
    } else if (lib_strcmp(ppcArgV[1], "noaddress") == 0) {              /*  ɾ�� ipv6 ��ַ              */
        return  (__tshellIpv6Noaddress(iArgC, ppcArgV));
    
    } else {
        printf("%s", _G_cIpv6Help);
        return  (ERROR_NONE);
    }
}
/*********************************************************************************************************
** ��������: __tshellNet6Init
** ��������: ע�� IPv6 ר������
** �䡡��  : NONE
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
VOID  __tshellNet6Init (VOID)
{
    API_TShellKeywordAdd("ipv6", __tshellIpv6);
    API_TShellFormatAdd("ipv6",  " ...");
    API_TShellHelpAdd("ipv6",    _G_cIpv6Help);
}

#endif                                                                  /*  LWIP_IPV6                   */
#endif                                                                  /*  LW_CFG_NET_EN > 0           */
                                                                        /*  LW_CFG_SHELL_EN > 0         */
/*********************************************************************************************************
  END
*********************************************************************************************************/
