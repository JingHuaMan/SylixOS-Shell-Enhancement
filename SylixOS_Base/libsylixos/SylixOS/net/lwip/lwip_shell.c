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
** ��   ��   ��: lwip_shell.c
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2009 �� 05 �� 06 ��
**
** ��        ��: lwip shell ����.

** BUG:
2009.05.22  ��������Ĭ��·���� shell ����.
2009.05.27  ��������ӿ� linkup ����ʾ��Ϣ.
2009.06.02  ������ tftp �� put �� get ��Դ�ļ���Ŀ���˳��.
2009.06.03  ����������ʱ����� DHCP ��ȡ IP, ������Ҫ�������ַ����.
2009.06.08  ifup ������ -nodhcp ѡ��, ����ǿ�Ʋ�ʹ�� DHCP ��ȡ IP ��ַ.
2009.06.26  ���� shell ��������.
2009.07.29  ���� ifconfig ����, �����ӽ� linux bash.
2009.09.14  ���� ifrouter ��Ĭ��·�ɽӿڵ���ʾ.
2009.11.09  ��Щ�������� api ��Ҫʹ�� netifapi_... ���.
2009.11.21  tftp ������е���, ���� tftp������.
2009.12.11  ifconfig �м��� metric ����ʾ.
2010.11.04  ���� arp ����֧��.
2011.06.08  ifconfig ��ʾ inet6 ��ַ�����Ϣ.
2011.07.02  ���� route ����.
2012.08.21  ���ڳ�ʼ�� ppp ��� shell.
2013.05.14  ��ӡ ipv6 ��ַʱ��Ҫ��ӡ��ַ״̬.
2013.09.12  ���� ifconfig ��������ӿ�ʱ�İ�ȫ��.
            ���� arp ����İ�ȫ��.
2014.12.23  ���� ifconfig ��ʾ.
*********************************************************************************************************/
#define  __SYLIXOS_STDIO
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
#include "../SylixOS/system/include/s_system.h"
/*********************************************************************************************************
  �ü�����
*********************************************************************************************************/
#if (LW_CFG_NET_EN > 0) && (LW_CFG_SHELL_EN > 0)
#include "net/if_lock.h"
#include "net/if_flags.h"
#include "lwip/opt.h"
#include "lwip/snmp.h"
#include "lwip/netif.h"
#include "lwip/netifapi.h"
#include "lwip/dns.h"
#include "lwip/inet.h"
#include "lwip/dhcp.h"
#include "lwip/tcpip.h"
#if LWIP_SNMP
#include "lwip/snmp/snmp.h"
#endif                                                                  /*  LWIP_SNMP                   */
#include "lwip_route.h"
#include "lwip_flowsh.h"
#if LW_CFG_NET_LOGINBL_EN > 0
#include "lwip_loginbl.h"
#endif                                                                  /*  LW_CFG_NET_LOGINBL_EN > 0   */
/*********************************************************************************************************
  ARP ��Э����غ���
*********************************************************************************************************/
#include "netif/etharp.h"
/*********************************************************************************************************
  netdev ��غ���
*********************************************************************************************************/
#include "netdev/netdev.h"
/*********************************************************************************************************
  QoS ��غ���
*********************************************************************************************************/
#include "netinet/ip_qos.h"
#include "netinet6/ip6_qos.h"
/*********************************************************************************************************
  netstat ������Ϣ
*********************************************************************************************************/
static const CHAR   _G_cNetstatHelp[] = {
    "show net status\n\n"
    "-h, --help             display this message\n\n"
    "-r, --route            display route table\n"
    "-i, --interface        display interface table\n"
    "-g, --groups           display multicast group memberships\n"
    "-s, --statistics       display networking statistics (like SNMP)\n\n"
    "-w, --raw              display raw socket information\n"
    "-t, --tcp              display tcp socket information\n"
    "-u, --udp              display udp socket information\n"
    "-p, --packet           display packet socket information\n"
    "-x, --unix             display unix socket information\n\n"
    "-l, --listening        display listening server sockets\n"
    "-k, --keepalive        display tcp keepalive information\n"
    "-a, --all              display all sockets\n\n"
    "-A <net type>, --<net type>    select <net type>, <net type>=inet, inet6 or unix\n"
};
extern VOID  __tshellNetstatIf(VOID);
#if LWIP_IGMP > 0
extern VOID  __tshellNetstatGroup(INT  iNetType);
#endif
extern VOID  __tshellNetstatStat(VOID);
extern VOID  __tshellNetstatRaw(INT  iNetType);
extern VOID  __tshellNetstatTcp(INT  iNetType);
extern VOID  __tshellNetstatTcpListen(INT  iNetType);
extern VOID  __tshellNetstatUdp(INT  iNetType);
extern VOID  __tshellNetstatUnix(INT  iNetType);
extern VOID  __tshellNetstatPacket(INT  iNetType);
extern VOID  __tshellNetstatKeepalive(INT  iNetType);
/*********************************************************************************************************
** ��������: __tshellNetstat
** ��������: ϵͳ���� "netstat"
** �䡡��  : iArgC         ��������
**           ppcArgV       ������
** �䡡��  : 0
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  __tshellNetstat (INT  iArgC, PCHAR  *ppcArgV)
{
    int             iC;
    const  char     cShortOpt[] = "hrigswtpuxlakA:";
    struct option   optionNetstat[] = {
        {"help",       0, LW_NULL, 'h'},
        {"route",      0, LW_NULL, 'r'},
        {"interface",  0, LW_NULL, 'i'},
        {"groups",     0, LW_NULL, 'g'},
        {"statistics", 0, LW_NULL, 's'},
        {"raw",        0, LW_NULL, 'r'},
        {"tcp",        0, LW_NULL, 't'},
        {"udp",        0, LW_NULL, 'u'},
        {"packet",     0, LW_NULL, 'p'},
        {"unix",       0, LW_NULL, 'x'},
        {"listening",  0, LW_NULL, 'l'},
        {"all",        0, LW_NULL, 'a'},
        {"keepalive",  0, LW_NULL, 'k'},
        {"unix",       0, LW_NULL, 1},
        {"inet",       0, LW_NULL, 2},
        {"inet6",      0, LW_NULL, 3},
        {LW_NULL,      0, LW_NULL, 0},
    };
    
    BOOL    bPacket  = LW_FALSE;
    BOOL    bRaw     = LW_FALSE;
    BOOL    bUdp     = LW_FALSE;
    BOOL    bTcp     = LW_FALSE;
    BOOL    bUnix    = LW_FALSE;
    BOOL    bListen  = LW_FALSE;
    INT     iNetType = 0;                                               /* 0:all 1:unix 2:inet 3:inet6  */
    CHAR    cNettype[10];
    
    while ((iC = getopt_long(iArgC, ppcArgV, cShortOpt, optionNetstat, LW_NULL)) != -1) {
        switch (iC) {
        
        case 'h':                                                       /*  ��ʾ����                    */
            printf(_G_cNetstatHelp);
            return  (ERROR_NONE);
            
        case 'r':                                                       /*  ��ʾ·�ɱ�                  */
            ppcArgV[1] = LW_NULL;
#if LW_CFG_NET_ROUTER > 0
            __tshellRoute(1, ppcArgV);
#endif
            return  (ERROR_NONE);
            
        case 'i':                                                       /*  ��ʾ����ӿ���Ϣ            */
            __tshellNetstatIf();
            return  (ERROR_NONE);
            
        case 'g':                                                       /*  ��ʾ�鲥�����              */
#if LWIP_IGMP > 0
            __tshellNetstatGroup(iNetType);
#endif
            return  (ERROR_NONE);
        
        case 'k':                                                       /*  ��ʾ KEEPALIVE ��Ϣ         */
            __tshellNetstatKeepalive(iNetType);
            return  (ERROR_NONE);

        case 's':                                                       /*  ��ʾͳ����Ϣ                */
            __tshellNetstatStat();
            return  (ERROR_NONE);
            
        case 'p':                                                       /*  ��ʾ packet socket          */
            bPacket = LW_TRUE;
            break;
            
        case 'w':                                                       /*  ��ʾ raw socket             */
            bRaw = LW_TRUE;
            break;
            
        case 't':                                                       /*  ��ʾ tcp socket             */
            bTcp = LW_TRUE;
            break;
            
        case 'u':                                                       /*  ��ʾ udp socket             */
            bUdp = LW_TRUE;
            break;
        
        case 'x':                                                       /*  ��ʾ unix socket            */
            bUnix = LW_TRUE;
            break;
        
        case 'l':                                                       /*  ��ʾ listen socket          */
            bListen = LW_TRUE;
            break;
            
        case 'a':                                                       /*  ��ʾ�б�                    */
            goto    __show;
            
        case 'A':                                                       /*  ��������                    */
            lib_strlcpy(cNettype, optarg, 10);
            if (lib_strcmp(cNettype, "unix") == 0) {
                iNetType = 1;
            } else if (lib_strcmp(cNettype, "inet") == 0) {
                iNetType = 2;
            } else if (lib_strcmp(cNettype, "inet6") == 0) {
                iNetType = 3;
            }
            break;
            
        case 1:
            iNetType = 1;
            break;
            
        case 2:
            iNetType = 2;
            break;
            
        case 3:
            iNetType = 3;
            break;
        }
    }
    getopt_free();
    
__show:
    if ((bRaw    == LW_FALSE) && (bUdp    == LW_FALSE) &&
        (bTcp    == LW_FALSE) && (bUnix   == LW_FALSE) &&
        (bListen == LW_FALSE) && (bPacket == LW_FALSE)) {
        bRaw    = LW_TRUE;
        bUdp    = LW_TRUE;
        bTcp    = LW_TRUE;
        bUnix   = LW_TRUE;
        bPacket = LW_TRUE;
    }
    if (bUnix) {
        __tshellNetstatUnix(iNetType);
    }
    if (bPacket) {
        __tshellNetstatPacket(iNetType);
    }
    if (bTcp || bListen) {
        __tshellNetstatTcpListen(iNetType);
        if (bTcp) {
            __tshellNetstatTcp(iNetType);
        }
    }
    if (bUdp) {
        __tshellNetstatUdp(iNetType);
    }
    if (bRaw) {
        __tshellNetstatRaw(iNetType);
    }
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: __netIfSpeed
** ��������: ��ʾָ��������ӿ���Ϣ (ip v4)
** �䡡��  : pcIfName      ����ӿ���
**           netifShow     ����ӿڽṹ
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static VOID  __netIfSpeed (struct netif  *netif, PCHAR  pcSpeedStr, size_t  stSize)
{
    netdev_t  *pnetdev;
    UINT64     u64Speed;
    
    pnetdev = netdev_find_by_index(netif_get_index(netif));
    if (pnetdev) {
        u64Speed = pnetdev->speed;
    } else {
        u64Speed = (UINT64)netif->link_speed;
    }

    if (u64Speed == 0) {
        lib_strlcpy(pcSpeedStr, "N/A", stSize);
        
    } else if (u64Speed < 1000ull) {
        snprintf(pcSpeedStr, stSize, "%qu bps", u64Speed);
    
    } else if (u64Speed < 5000000ull) {
        snprintf(pcSpeedStr, stSize, "%qu Kbps", u64Speed / 1000);
    
    } else if (u64Speed < 5000000000ull) {
        snprintf(pcSpeedStr, stSize, "%qu Mbps", u64Speed / 1000000);
    
    } else {
        snprintf(pcSpeedStr, stSize, "%qu Gbps", u64Speed / 1000000000);
    }
}
/*********************************************************************************************************
** ��������: __netIfOctets
** ��������: ��ʾָ��������ӿ�����ͳ��
** �䡡��  : ullValue      ����
**           pcBuffer      ����
**           stSize        �����С
** �䡡��  : ��ʾ����
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static PCHAR  __netIfOctets (UINT64  ullValue, PCHAR  pcBuffer, size_t  stSize)
{
    if (ullValue > LW_CFG_GB_SIZE) {
        ullValue = (ullValue >> 20);
        snprintf(pcBuffer, stSize, "%qu.%qu GB", (ullValue >> 10), (ullValue & 0x3ff) / 102);
        
    } else if (ullValue > LW_CFG_MB_SIZE) {
        ullValue = (ullValue >> 10);
        snprintf(pcBuffer, stSize, "%qu.%qu MB", (ullValue >> 10), (ullValue & 0x3ff) / 102);
    
    } else if (ullValue > LW_CFG_KB_SIZE) {
        snprintf(pcBuffer, stSize, "%qu.%qu KB", (ullValue >> 10), (ullValue & 0x3ff) / 102);
    
    } else {
        snprintf(pcBuffer, stSize, "%qu.0 B", ullValue);
    }
    
    return  (pcBuffer);
}
/*********************************************************************************************************
** ��������: __netIfShow
** ��������: ��ʾָ��������ӿ���Ϣ (ip v4)
** �䡡��  : pcIfName      ����ӿ���
**           netifShow     ����ӿڽṹ
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static VOID  __netIfShow (CPCHAR  pcIfName, const struct netif  *netifShow)
{
#define MIB2_NETIF(netif)   (&((netif)->mib2_counters))

    struct netif    *netif;
    struct netdev   *netdev;
    CHAR             cIfName[NETIF_NAMESIZE];
    CHAR             cBuffer1[32];
    CHAR             cBuffer2[32];
    PCHAR            pcDevName = "N/A";
    ip4_addr_t       ipaddrBroadcast;
    INT              i, iFlags;

    if ((pcIfName == LW_NULL) && (netifShow == LW_NULL)) {
        return;
    }

    if (netifShow) {
        netif = (struct netif *)netifShow;
    } else {
        netif = netif_find((PCHAR)pcIfName);
    }

    if (netif == LW_NULL) {
        return;
    }
    
    netdev = (netdev_t *)(netif->state);
    if (netdev && (netdev->magic_no == NETDEV_MAGIC)) {
        pcDevName = netdev->dev_name;
    } else {
        netdev = LW_NULL;
    }

    printf("%-5s%4s ", netif_get_name(netif, cIfName), "");             /*  ��������                    */
    
    if (netif->flags & (NETIF_FLAG_ETHARP | NETIF_FLAG_ETHERNET)) {     /*  ��̫����                    */
        printf("Link encap: Ethernet HWaddr: ");
        for (i = 0; i < netif->hwaddr_len - 1; i++) {
            printf("%02x:", netif->hwaddr[i]);
        }
        printf("%02x\n", netif->hwaddr[netif->hwaddr_len - 1]);
        
    } else if (netdev && (netdev->net_type != NETDEV_TYPE_RAW)) {       /*  netdev �豸                 */
        switch (netdev->net_type) {
        
        case NETDEV_TYPE_LOWPAN:
            printf("Link encap: LowPAN HWaddr: ");
            for (i = 0; i < netif->hwaddr_len - 1; i++) {
                printf("%02x:", netif->hwaddr[i]);
            }
            printf("%02x\n", netif->hwaddr[netif->hwaddr_len - 1]);
            break;
            
        case NETDEV_TYPE_LOWPAN_BLE:
            printf("Link encap: LowPAN-BLE HWaddr: ");
            for (i = 0; i < netif->hwaddr_len - 1; i++) {
                printf("%02x:", netif->hwaddr[i]);
            }
            printf("%02x\n", netif->hwaddr[netif->hwaddr_len - 1]);
            break;
            
        default:                                                        /*  �������е�����              */
            break;
        }
    
    } else {
        if ((netif->flags & NETIF_FLAG_BROADCAST) == 0) {               /*  ��Ե�����ӿ�              */
            if (netif->link_type == snmp_ifType_softwareLoopback) {
                printf("Link encap: Local Loopback\n");
            } else if (netif->link_type == snmp_ifType_ppp) {
                printf("Link encap: PPP Link\n");
            } else if (netif->link_type == snmp_ifType_slip) {
                printf("Link encap: SLIP Link\n");
            } else if (netif->link_type == snmp_ifType_tunnel) {
                printf("Link encap: Tunnel Link\n");
            } else {
                printf("Link encap: General\n");
            }
            
        } else {                                                        /*  ͨ������ӿ�                */
            printf("Link encap: General\n");
        }
    }
    
    __netIfSpeed(netif, cBuffer1, sizeof(cBuffer1));
    
#if LW_CFG_NET_NETDEV_MIP_EN > 0
    if (netif_is_mipif(netif)) {
        printf("%9s Mif: %s Ifidx: %d ", "",
               netif_get_name(netif_get_masterif(netif), cIfName), netif_get_index(netif));
    } else 
#endif                                                                  /*  LW_CFG_NET_NETDEV_MIP_EN    */
    {
        printf("%9s Dev: %s Ifidx: %d ", "",
               pcDevName, netif_get_index(netif));
    }

#if LWIP_DHCP
    printf("DHCP: %s%s %s%s Spd: %s\n", 
           (netif->flags2 & NETIF_FLAG2_DHCP) ? "E4" : "D4",
           (netif->flags2 & NETIF_FLAG2_DHCP) ? ((netif_dhcp_data(netif)) ? "(On)" : "(Off)") : "",
#if LWIP_IPV6_DHCP6
           (netif->flags2 & NETIF_FLAG2_DHCP6) ? "E6" : "D6",
           (netif->flags2 & NETIF_FLAG2_DHCP6) ? ((netif_dhcp6_data(netif)) ? "(On)" : "(Off)") : "",
#else
           "", "", 
#endif                                                                  /*  LWIP_IPV6_DHCP6             */
           cBuffer1);
#else
    printf("Spd: %s\n", cBuffer1);
#endif                                                                  /*  LWIP_DHCP                   */
    
    printf("%9s inet addr: %d.%d.%d.%d ", "",
           ip4_addr1(netif_ip4_addr(netif)), ip4_addr2(netif_ip4_addr(netif)),
           ip4_addr3(netif_ip4_addr(netif)), ip4_addr4(netif_ip4_addr(netif)));
    printf("netmask: %d.%d.%d.%d\n", 
           ip4_addr1(netif_ip4_netmask(netif)), ip4_addr2(netif_ip4_netmask(netif)),
           ip4_addr3(netif_ip4_netmask(netif)), ip4_addr4(netif_ip4_netmask(netif)));
    
    if ((netif->flags & NETIF_FLAG_BROADCAST) == 0) {
        printf("%9s P-to-P: %d.%d.%d.%d ", "",
               ip4_addr1(netif_ip4_gw(netif)), ip4_addr2(netif_ip4_gw(netif)),
               ip4_addr3(netif_ip4_gw(netif)), ip4_addr4(netif_ip4_gw(netif)));
        printf("broadcast: N/A\n");
    
    } else {
        printf("%9s gateway: %d.%d.%d.%d ", "",
               ip4_addr1(netif_ip4_gw(netif)), ip4_addr2(netif_ip4_gw(netif)),
               ip4_addr3(netif_ip4_gw(netif)), ip4_addr4(netif_ip4_gw(netif)));
        ipaddrBroadcast.addr = (netif_ip4_addr(netif)->addr | (~netif_ip4_netmask(netif)->addr));
        printf("broadcast: %d.%d.%d.%d\n", 
               ip4_addr1(&ipaddrBroadcast), ip4_addr2(&ipaddrBroadcast),
               ip4_addr3(&ipaddrBroadcast), ip4_addr4(&ipaddrBroadcast));
    }
    
#if LWIP_IPV6
    for (i = 0; i < LWIP_IPV6_NUM_ADDRESSES; i++) {
        PCHAR       pcAddrType;
        CHAR        cBuffer[64];
        
        if (ip6_addr_isglobal(ip_2_ip6(&netif->ip6_addr[i]))) {
            pcAddrType = "Global";
        } else if (ip6_addr_islinklocal(ip_2_ip6(&netif->ip6_addr[i]))) {
            pcAddrType = "Link";
        } else if (ip6_addr_issitelocal(ip_2_ip6(&netif->ip6_addr[i]))) {
            pcAddrType = "Site";
        } else if (ip6_addr_isuniquelocal(ip_2_ip6(&netif->ip6_addr[i]))) {
            pcAddrType = "Unique";
        } else if (ip6_addr_isloopback(ip_2_ip6(&netif->ip6_addr[i]))) {
            pcAddrType = "Loopback";
        } else {
            pcAddrType = "Unknown";
        }
        
        if (ip6_addr_isvalid(netif->ip6_addr_state[i])) {
            printf("%9s inet6 addr: %s Scope:%s\n", "",
                   ip6addr_ntoa_r(ip_2_ip6(&netif->ip6_addr[i]), cBuffer, sizeof(cBuffer)),
                   pcAddrType);
            
        } else if (ip6_addr_istentative(netif->ip6_addr_state[i])) {
            printf("%9s inet6 addr: %s Scope:%s<T%d>\n", "",
                   ip6addr_ntoa_r(ip_2_ip6(&netif->ip6_addr[i]), cBuffer, sizeof(cBuffer)),
                   pcAddrType, (netif->ip6_addr_state[i] & IP6_ADDR_TENTATIVE_7) - 8);
        }
    }
#endif                                                                  /*  LWIP_IPV6                   */
    
    printf("%9s ", "");
    iFlags = netif_get_flags(netif);
    if (iFlags & IFF_UP) {
        printf("UP ");
    }
    if (iFlags & IFF_BROADCAST) {
        printf("BROADCAST ");
    }
    if (iFlags & IFF_LOOPBACK) {
        printf("LOOPBACK ");
    }
    if (iFlags & IFF_RUNNING) {
        printf("RUNNING ");
    }
    if (iFlags & IFF_MULTICAST) {
        printf("MULTICAST ");
    }
    if (netif->flags & (NETIF_FLAG_ETHARP | NETIF_FLAG_ETHERNET)) {
        if (iFlags & IFF_NOARP) {
            printf("NOARP ");
        }
    }
    printf(" MTU:%d  Metric:%d\n", netif->mtu, netif->metric);
    
    if (netif_is_mipif(netif)) {
        printf("\n");
        return;
    }
    
#if LW_CFG_NET_DEV_TXQ_EN > 0
    printf("%9s collisions:%u txqueue:%u tcpaf:%u tcpwnd:%u\n", "", 
           MIB2_NETIF(netif)->ifcollisions,
           (netdev) ? netdev_txq_length(netdev) : 0,
           netif_get_tcp_ack_freq(netif), netif_get_tcp_wnd(netif));
#else
    printf("%9s collisions:%u noproto:%u tcpaf:%u tcpwnd:%u\n", "", 
           MIB2_NETIF(netif)->ifcollisions,
           MIB2_NETIF(netif)->ifinunknownprotos,
           netif_get_tcp_ack_freq(netif), netif_get_tcp_wnd(netif));
#endif
    
    printf("%9s RX ucast packets:%u nucast packets:%u dropped:%u\n", "",
           MIB2_NETIF(netif)->ifinucastpkts, MIB2_NETIF(netif)->ifinnucastpkts, MIB2_NETIF(netif)->ifindiscards);
    printf("%9s TX ucast packets:%u nucast packets:%u dropped:%u\n", "",
           MIB2_NETIF(netif)->ifoutucastpkts, MIB2_NETIF(netif)->ifoutnucastpkts, MIB2_NETIF(netif)->ifoutdiscards);
    printf("%9s RX bytes:%qu (%s)  TX bytes:%qu (%s)\n", "",
           MIB2_NETIF(netif)->ifinoctets, 
           __netIfOctets(MIB2_NETIF(netif)->ifinoctets, cBuffer1, sizeof(cBuffer1)),
           MIB2_NETIF(netif)->ifoutoctets,
           __netIfOctets(MIB2_NETIF(netif)->ifoutoctets, cBuffer2, sizeof(cBuffer2)));
    printf("\n");
}
/*********************************************************************************************************
** ��������: __netIfShowAll
** ��������: ��ʾ��������ӿ���Ϣ (ip v4)
** �䡡��  : iFlags   ���� flags
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static VOID  __netIfShowAll (INT  iFlags)
{
    struct netif *netif;
    INT           iCounter = 0;
    INT           i;
    CHAR          cName[NETIF_NAMESIZE] = "null";                       /*  ��ǰĬ��·������ӿ���      */

    NETIF_FOREACH(netif) {
        if ((netif_get_flags(netif) & iFlags) == iFlags) {
            __netIfShow(LW_NULL, netif);
            iCounter++;
        }
    }

#if LWIP_DNS > 0
    for (i = 0; i < DNS_MAX_SERVERS; i++) {
        ip_addr_t ipaddr = *dns_getserver((u8_t)i);
        CHAR      cStrBuf[48];
        printf("dns%d: %s\n", i, ipaddr_ntoa_r(&ipaddr, cStrBuf, sizeof(cStrBuf)));
    }
#endif                                                                  /*  LWIP_DNS                    */

    if (netif_default) {
        netif_get_name(netif_default, cName);
    }
    
    printf("default device is: %s\n", cName);                           /*  ��ʾ·�ɶ˿�                */
    printf("list net interface: %d\n", iCounter);
}
/*********************************************************************************************************
** ��������: __netIfSet
** ��������: ����ָ������ӿ���Ϣ (ip v4)
** �䡡��  : netif     ����ӿ�
**           pcItem    ������Ŀ����
**           ipaddr    ��ַ��Ϣ
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static VOID  __netIfSet (struct netif  *netif, CPCHAR  pcItem, ip4_addr_t *ipaddr)
{
    ip4_addr_t  ipaddrInet;
    ip4_addr_t  ipaddrMask;
    ip4_addr_t  ipaddrGw;

    if (netif == LW_NULL) {
        return;
    }
    
    ipaddrInet = *netif_ip4_addr(netif);
    ipaddrMask = *netif_ip4_netmask(netif);
    ipaddrGw   = *netif_ip4_gw(netif);

    if (lib_strcmp(pcItem, "inet") == 0) {
        netifapi_netif_set_addr(netif, ipaddr, &ipaddrMask, &ipaddrGw);
    
    } else if (lib_strcmp(pcItem, "netmask") == 0) {
        netifapi_netif_set_addr(netif, &ipaddrInet, ipaddr, &ipaddrGw);
    
    } else if (lib_strcmp(pcItem, "gateway") == 0) {
        netifapi_netif_set_addr(netif, &ipaddrInet, &ipaddrMask, ipaddr);
    
    } else {
        fprintf(stderr, "arguments error!\n");
    }
}
/*********************************************************************************************************
** ��������: __tshellIfconfig
** ��������: ϵͳ���� "ifconfig"
** �䡡��  : iArgC         ��������
**           ppcArgV       ������
** �䡡��  : 0
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  __tshellIfconfig (INT  iArgC, PCHAR  *ppcArgV)
{
    struct netif    *netif;
    struct in_addr   inaddr;
    ip_addr_t        ipaddr;
    INT              iFlags = 0;

    if (iArgC == 1) {
        LWIP_IF_LIST_LOCK(LW_FALSE);
        __netIfShowAll(iFlags);                                         /*  ��ӡ����������Ϣ            */
        LWIP_IF_LIST_UNLOCK();
        return  (ERROR_NONE);
    
    } else if (iArgC == 2) {
        if (*ppcArgV[1] == '-') {
            if (lib_strcmp(ppcArgV[1], "-u") == 0) {
                iFlags |= IFF_UP;
            } else if (lib_strcmp(ppcArgV[1], "-r") == 0) {
                iFlags |= IFF_UP | IFF_RUNNING;
            } else {
                fprintf(stderr, "arguments error!\n");
            }
            LWIP_IF_LIST_LOCK(LW_FALSE);
            __netIfShowAll(iFlags);                                     /*  ��ӡ���з���������������Ϣ  */
            LWIP_IF_LIST_UNLOCK();
            return  (ERROR_NONE);
        
        } else {
            LWIP_IF_LIST_LOCK(LW_FALSE);
            __netIfShow(ppcArgV[1], LW_NULL);                           /*  ��ӡָ��������Ϣ            */
            LWIP_IF_LIST_UNLOCK();
            return  (ERROR_NONE);
        }
    }

    if (iArgC >= 4) {                                                   /*  ����ӿڲ�������            */
        if (lib_strcmp(ppcArgV[1], "dns") == 0) {                       /*  ָ�� DNS ����               */
            INT     iDnsIndex = 0;
            
            sscanf(ppcArgV[2], "%d", &iDnsIndex);
            if (iDnsIndex >= DNS_MAX_SERVERS) {
                fprintf(stderr, "arguments error!\n");
                return  (-ERROR_TSHELL_EPARAM);
            }
            if (ipaddr_aton(ppcArgV[3], &ipaddr) == 0) {                /*  ��� IP ��ַ                */
                fprintf(stderr, "address error.\n");
                return  (-ERROR_TSHELL_EPARAM);
            }
            dns_setserver((u8_t)iDnsIndex, &ipaddr);                    /*  ���� DNS                    */
        
        } else {                                                        /*  ָ������ӿ�����            */
            INT     iIndex;
            
            LWIP_IF_LIST_LOCK(LW_FALSE);
            netif = netif_find(ppcArgV[1]);                             /*  ��ѯ����ӿ�                */
            if (netif == LW_NULL) {
                LWIP_IF_LIST_UNLOCK();
                fprintf(stderr, "can not find net interface.\n");
                return  (-ERROR_TSHELL_EPARAM);
            }
            for (iIndex = 2; iIndex < (iArgC - 1); iIndex += 2) {       /*  �������ò���                */
                if (inet_aton(ppcArgV[iIndex + 1], &inaddr) == 0) {     /*  ��� IP ��ַ                */
                    fprintf(stderr, "address error.\n");
                    return  (-ERROR_TSHELL_EPARAM);
                }
                ip_2_ip4(&ipaddr)->addr = inaddr.s_addr;
                IP_SET_TYPE_VAL(ipaddr, IPADDR_TYPE_V4);
                __netIfSet(netif, ppcArgV[iIndex], ip_2_ip4(&ipaddr));  /*  ��������ӿ�                */
            }
            LWIP_IF_LIST_UNLOCK();
        }
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: __tshellIfUp
** ��������: ϵͳ���� "ifup"
** �䡡��  : iArgC         ��������
**           ppcArgV       ������
** �䡡��  : 0
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  __tshellIfUp (INT  iArgC, PCHAR  *ppcArgV)
{
    INT           i;
    struct netif *netif;
    BOOL          bUseDHCP       = LW_FALSE;                            /*  �Ƿ�ʹ���Զ���ȡ IP         */
    BOOL          bShutDownDHCP  = LW_FALSE;                            /*  �Ƿ�ǿ�ƹر� DHCP           */

#if LWIP_IPV6_DHCP6 > 0
    BOOL          bUseDHCP6      = LW_FALSE;                            /*  �Ƿ�ʹ���Զ���ȡ IP         */
    BOOL          bShutDownDHCP6 = LW_FALSE;                            /*  �Ƿ�ǿ�ƹر� DHCP           */
#endif                                                                  /*  LWIP_IPV6_DHCP6             */

    if (iArgC < 2) {
        fprintf(stderr, "arguments error!\n");
        return  (-ERROR_TSHELL_EPARAM);
    
    } else if (iArgC > 2) {
        for (i = 2; i < iArgC; i++) {
            if (lib_strcmp(ppcArgV[i], "-dhcp") == 0) {
                bUseDHCP = LW_TRUE;                                     /*  ʹ�� DHCP ����              */
            } else if (lib_strcmp(ppcArgV[i], "-nodhcp") == 0) {
                bShutDownDHCP = LW_TRUE;
            } 
#if LWIP_IPV6_DHCP6 > 0
              else if (lib_strcmp(ppcArgV[i], "-dhcp6") == 0) {
                bUseDHCP6 = LW_TRUE;
            } else if (lib_strcmp(ppcArgV[i], "-nodhcp6") == 0) {
                bShutDownDHCP6 = LW_TRUE;
            }
#endif                                                                  /*  LWIP_IPV6_DHCP6             */
        }
    }

    LWIP_IF_LIST_LOCK(LW_FALSE);
    netif = netif_find(ppcArgV[1]);                                     /*  ��ѯ����ӿ�                */
    if (netif == LW_NULL) {
        LWIP_IF_LIST_UNLOCK();
        fprintf(stderr, "can not find net interface.\n");
        return  (-ERROR_TSHELL_EPARAM);
    }

    if (netif_is_up(netif)) {                                           /*  �����Ƿ��Ѿ�����            */
#if LWIP_DHCP > 0                                                       /*  ���ȹر�����                */
        if (netif_dhcp_data(netif)) {
            netifapi_dhcp_release_and_stop(netif);                      /*  ��� DHCP ��Լ���ͷ���Դ    */
        }
#endif                                                                  /*  LWIP_DHCP > 0               */
#if LWIP_IPV6_DHCP6 > 0
        if (netif_dhcp6_data(netif)) {
            netifapi_dhcp6_disable(netif);
        }
#endif                                                                  /*  LWIP_IPV6_DHCP6             */
        netifapi_netif_set_down(netif);                                 /*  ��������                    */
    }

    netifapi_netif_set_up(netif);                                       /*  ��������                    */

#if LWIP_DHCP > 0
    if (bUseDHCP && !netif_is_mipif(netif) &&
        (netif->flags & NETIF_FLAG_BROADCAST)) {
        netif->flags2 |= NETIF_FLAG2_DHCP;                              /*  ʹ�� DHCP ����              */
    } else if (bShutDownDHCP) {
        netif->flags2 &= ~NETIF_FLAG2_DHCP;                             /*  ǿ�ƹر� DHCP               */
    }

    if (netif->flags2 & NETIF_FLAG2_DHCP) {
        ip4_addr_t  inaddrNone;

        lib_bzero(&inaddrNone, sizeof(ip4_addr_t));
        netifapi_netif_set_addr(netif, &inaddrNone, &inaddrNone, &inaddrNone);
                                                                        /*  ���е�ַ����Ϊ 0            */
        printf("DHCP client starting...\n");
        if (netifapi_dhcp_start(netif) < ERR_OK) {
            printf("DHCP client serious error.\n");
        } else {
            printf("DHCP client start.\n");
        }
    }
#endif                                                                  /*  LWIP_DHCP > 0               */

#if LWIP_IPV6_DHCP6 > 0
    if (bUseDHCP6 && !netif_is_mipif(netif) &&
        (netif->flags & NETIF_FLAG_BROADCAST)) {
        netif->flags2 |= NETIF_FLAG2_DHCP6;                             /*  ʹ�� DHCP ����              */
    } else if (bShutDownDHCP6) {
        netif->flags2 &= ~NETIF_FLAG2_DHCP6;                            /*  ǿ�ƹر� DHCP               */
    }
    
    if (netif->flags2 & NETIF_FLAG2_DHCP6) {
        printf("DHCPv6 client starting...\n");
        if (netifapi_dhcp6_enable_stateless(netif) < ERR_OK) {
            printf("DHCPv6 client serious error.\n");
        } else {
            printf("DHCPv6 client start.\n");
        }
    }
#endif                                                                  /*  LWIP_IPV6_DHCP6             */
    LWIP_IF_LIST_UNLOCK();
    
    printf("net interface \"%s\" set up.\n", ppcArgV[1]);
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: __tshellIfDown
** ��������: ϵͳ���� "ifdown"
** �䡡��  : iArgC         ��������
**           ppcArgV       ������
** �䡡��  : 0
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  __tshellIfDown (INT  iArgC, PCHAR  *ppcArgV)
{
    struct netif *netif;

    if (iArgC != 2) {
        fprintf(stderr, "arguments error!\n");
        return  (-ERROR_TSHELL_EPARAM);
    }

    LWIP_IF_LIST_LOCK(LW_FALSE);
    netif = netif_find(ppcArgV[1]);                                     /*  ��ѯ����ӿ�                */
    if (netif == LW_NULL) {
        LWIP_IF_LIST_UNLOCK();
        fprintf(stderr, "can not find net interface.\n");
        return  (-ERROR_TSHELL_EPARAM);
    }

    if (!netif_is_up(netif)) {
        LWIP_IF_LIST_UNLOCK();
        fprintf(stderr, "net interface already set down.\n");
        return  (PX_ERROR);
    }

#if LWIP_DHCP > 0
    if (netif_dhcp_data(netif)) {
        netifapi_dhcp_release_and_stop(netif);                          /*  ��� DHCP ��Լ���ͷ���Դ    */
    }
#endif                                                                  /*  LWIP_DHCP > 0               */

#if LWIP_IPV6_DHCP6 > 0
    if (netif_dhcp6_data(netif)) {
        netifapi_dhcp6_disable(netif);
    }
#endif                                                                  /*  LWIP_IPV6_DHCP6             */

    netifapi_netif_set_down(netif);                                     /*  ��������                    */
    LWIP_IF_LIST_UNLOCK();
    
    printf("net interface \"%s\" set down.\n", ppcArgV[1]);
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: __tshellIfMip
** ��������: ϵͳ���� "ifmip"
** �䡡��  : iArgC         ��������
**           ppcArgV       ������
** �䡡��  : 0
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
#if LW_CFG_NET_NETDEV_MIP_EN > 0

static INT  __tshellIfMip (INT  iArgC, PCHAR  *ppcArgV)
{
    INT                  iSock, iRet;
    struct sockaddr_in  *psockaddrin;

    if ((iArgC == 4) && (lib_strcmp(ppcArgV[2], "del") == 0)) {
        struct ifreq  ifreq;
    
        iSock = socket(AF_INET, SOCK_DGRAM, 0);
        if (iSock < 0) {
            fprintf(stderr, "can not create socket, error: %s!\n", lib_strerror(errno));
            return  (PX_ERROR);
        }
        
        lib_strlcpy(ifreq.ifr_name, ppcArgV[1], IFNAMSIZ);
        psockaddrin = (struct sockaddr_in *)&(ifreq.ifr_addr);
        lib_bzero(psockaddrin, sizeof(struct sockaddr_in));
        
        psockaddrin->sin_len         = sizeof(struct sockaddr_in);
        psockaddrin->sin_family      = AF_INET;
        psockaddrin->sin_addr.s_addr = inet_addr(ppcArgV[3]);
        
        iRet = ioctl(iSock, SIOCDIFADDR, &ifreq);
        if (iRet < 0) {
            fprintf(stderr, "command 'SIOCDIFADDR', error: %s!\n", lib_strerror(errno));
            close(iSock);
            return  (PX_ERROR);
        }
        
        close(iSock);
        
    } else if ((iArgC == 5) && (lib_strcmp(ppcArgV[2], "add") == 0)) {
        struct ifaliasreq  ifaliasreq;
    
        iSock = socket(AF_INET, SOCK_DGRAM, 0);
        if (iSock < 0) {
            fprintf(stderr, "can not create socket, error: %s!\n", lib_strerror(errno));
            return  (PX_ERROR);
        }
        
        lib_strlcpy(ifaliasreq.ifra_name, ppcArgV[1], IFNAMSIZ);
        psockaddrin = (struct sockaddr_in *)&(ifaliasreq.ifra_addr);
        lib_bzero(psockaddrin, sizeof(struct sockaddr_in));
        
        psockaddrin->sin_len         = sizeof(struct sockaddr_in);
        psockaddrin->sin_family      = AF_INET;
        psockaddrin->sin_addr.s_addr = inet_addr(ppcArgV[3]);
        
        psockaddrin = (struct sockaddr_in *)&(ifaliasreq.ifra_broadaddr);
        lib_bzero(psockaddrin, sizeof(struct sockaddr_in));
        psockaddrin = (struct sockaddr_in *)&(ifaliasreq.ifra_mask);
        lib_bzero(psockaddrin, sizeof(struct sockaddr_in));
        
        psockaddrin->sin_len         = sizeof(struct sockaddr_in);
        psockaddrin->sin_family      = AF_INET;
        psockaddrin->sin_addr.s_addr = inet_addr(ppcArgV[4]);
        
        iRet = ioctl(iSock, SIOCAIFADDR, &ifaliasreq);
        if (iRet < 0) {
            fprintf(stderr, "command 'SIOCAIFADDR', error: %s!\n", lib_strerror(errno));
            close(iSock);
            return  (PX_ERROR);
        }
        
        close(iSock);
        
    } else {
        fprintf(stderr, "arguments error!\n");
        return  (-ERROR_TSHELL_EPARAM);
    }
    
    return  (ERROR_NONE);
}

#endif                                                                  /*  LW_CFG_NET_NETDEV_MIP_EN    */
/*********************************************************************************************************
** ��������: __tshellIfRouter
** ��������: ϵͳ���� "ifrouter"
** �䡡��  : iArgC         ��������
**           ppcArgV       ������
** �䡡��  : 0
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  __tshellIfRouter (INT  iArgC, PCHAR  *ppcArgV)
{
    printf("this command has been removed! please use 'route' command instead.\n");

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: __tshellArp
** ��������: ϵͳ���� "arp"
** �䡡��  : iArgC         ��������
**           ppcArgV       ������
** �䡡��  : 0
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  __tshellArp (INT  iArgC, PCHAR  *ppcArgV)
{
    if (iArgC < 2) {
        fprintf(stderr, "arguments error!\n");
        return  (-ERROR_TSHELL_EPARAM);
    }
    
    if (lib_strcmp(ppcArgV[1], "-a") == 0) {                            /*  ��ʾ arp ��                 */
        INT     iFd;
        CHAR    cBuffer[512];
        ssize_t sstNum;
        
        iFd = open("/proc/net/arp", O_RDONLY);
        if (iFd < 0) {
            fprintf(stderr, "can not open /proc/net/arp : %s\n", lib_strerror(errno));
            return  (PX_ERROR);
        }
        
        do {
            sstNum = read(iFd, cBuffer, sizeof(cBuffer));
            if (sstNum > 0) {
                write(STDOUT_FILENO, cBuffer, (size_t)sstNum);
            }
        } while (sstNum > 0);
        
        close(iFd);
        
        return  (ERROR_NONE);
    
    } else if (lib_strcmp(ppcArgV[1], "-s") == 0) {                     /*  ����һ����̬ת����ϵ        */
        INT             i;
        INT             iTemp[6];
        ip4_addr_t      ipaddr;
        struct eth_addr ethaddr;
        err_t           err;
        
        if (iArgC != 4) {
            fprintf(stderr, "arguments error!\n");
            return  (-ERROR_TSHELL_EPARAM);
        }
        
        ipaddr.addr = ipaddr_addr(ppcArgV[2]);
        if ((ipaddr.addr == IPADDR_NONE) || (ipaddr.addr == IPADDR_ANY)) {
            fprintf(stderr, "bad inet address: %s\n", ppcArgV[2]);
            return  (-ERROR_TSHELL_EPARAM);
        }
        
        if (sscanf(ppcArgV[3], "%02x:%02x:%02x:%02x:%02x:%02x",
                   &iTemp[0], &iTemp[1], &iTemp[2], 
                   &iTemp[3], &iTemp[4], &iTemp[5]) != 6) {
            fprintf(stderr, "bad physical address: %s\n", ppcArgV[3]);
            return  (-ERROR_TSHELL_EPARAM);
        }
        
        for (i = 0; i < 6; i++) {
            ethaddr.addr[i] = (u8_t)iTemp[i];
        }
        
        err = netifapi_arp_add(&ipaddr, &ethaddr, NETIFAPI_ARP_PERM);
        return  (err ? PX_ERROR : ERROR_NONE);
    
    } else if (lib_strcmp(ppcArgV[1], "-d") == 0) {                     /*  ɾ��һ����̬ת����ϵ        */
        ip4_addr_t      ipaddr;
        err_t           err;
        
        if (iArgC != 3) {                                               /*  ɾ��ȫ��ת����ϵ            */
            struct netif *netif;
            
            LWIP_IF_LIST_LOCK(LW_FALSE);
            NETIF_FOREACH(netif) {
                if (netif->flags & NETIF_FLAG_ETHARP) {
                    netifapi_arp_cleanup(netif);
                }
            }
            LWIP_IF_LIST_UNLOCK();
            
            return  (ERROR_NONE);
        }
        
        ipaddr.addr = ipaddr_addr(ppcArgV[2]);
        if ((ipaddr.addr == IPADDR_NONE) || (ipaddr.addr == IPADDR_ANY)) {
            fprintf(stderr, "bad inet address: %s\n", ppcArgV[2]);
            return  (-ERROR_TSHELL_EPARAM);
        }
        
        err = netifapi_arp_remove(&ipaddr, NETIFAPI_ARP_PERM);
        return  (err ? PX_ERROR : ERROR_NONE);
    
    } else {
        fprintf(stderr, "arguments error!\n");
        return  (-ERROR_TSHELL_EPARAM);
    }
}
/*********************************************************************************************************
** ��������: __tshellLoginBl
** ��������: ϵͳ���� "loginbl"
** �䡡��  : iArgC         ��������
**           ppcArgV       ������
** �䡡��  : 0
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
#if LW_CFG_NET_LOGINBL_EN > 0

static INT  __tshellLoginBl (INT  iArgC, PCHAR  *ppcArgV)
{
    struct sockaddr      addr;
    struct sockaddr_in  *pinaddr  = (struct sockaddr_in *)&addr;

    if (iArgC == 1) {
        API_LoginBlShow();
        return  (ERROR_NONE);
    
    } else if (iArgC != 3) {
        goto    __error;
    }
    
    lib_bzero(&addr, sizeof(struct sockaddr));
    
    pinaddr->sin_addr.s_addr = inet_addr(ppcArgV[2]);
    if (pinaddr->sin_addr.s_addr == INADDR_NONE) {
#if LWIP_IPV6
        struct sockaddr_in6 *pin6addr = (struct sockaddr_in6 *)&addr;

        if (inet6_aton(ppcArgV[2], &pin6addr->sin6_addr) == 0) {
            fprintf(stderr, "ipaddr error!\n");
            return  (-ERROR_TSHELL_EPARAM);
        
        } else {
            pin6addr->sin6_len    = sizeof(struct sockaddr_in6);
            pin6addr->sin6_family = AF_INET6;
        }
#else
        fprintf(stderr, "ipaddr error!\n");
        return  (-ERROR_TSHELL_EPARAM);
#endif
    
    } else {
        pinaddr->sin_len    = sizeof(struct sockaddr_in);
        pinaddr->sin_family = AF_INET;
    }
    
    if (lib_strcmp(ppcArgV[1], "add") == 0) {
        API_LoginBlAdd(&addr, 1, 0);
        return  (ERROR_NONE);
        
    } else if (lib_strcmp(ppcArgV[1], "del") == 0) {
        API_LoginBlDelete(&addr);
        return  (ERROR_NONE);
    }
    
__error:
    fprintf(stderr, "arguments error!\n");
    return  (-ERROR_TSHELL_EPARAM);
}
/*********************************************************************************************************
** ��������: __tshellLoginWl
** ��������: ϵͳ���� "loginwl"
** �䡡��  : iArgC         ��������
**           ppcArgV       ������
** �䡡��  : 0
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  __tshellLoginWl (INT  iArgC, PCHAR  *ppcArgV)
{
    struct sockaddr      addr;
    struct sockaddr_in  *pinaddr  = (struct sockaddr_in *)&addr;

    if (iArgC == 1) {
        API_LoginWlShow();
        return  (ERROR_NONE);
    
    } else if (iArgC != 3) {
        goto    __error;
    }
    
    lib_bzero(&addr, sizeof(struct sockaddr));
    
    pinaddr->sin_addr.s_addr = inet_addr(ppcArgV[2]);
    if (pinaddr->sin_addr.s_addr == INADDR_NONE) {
#if LWIP_IPV6
        struct sockaddr_in6 *pin6addr = (struct sockaddr_in6 *)&addr;

        if (inet6_aton(ppcArgV[2], &pin6addr->sin6_addr) == 0) {
            fprintf(stderr, "ipaddr error!\n");
            return  (-ERROR_TSHELL_EPARAM);
        
        } else {
            pin6addr->sin6_len    = sizeof(struct sockaddr_in6);
            pin6addr->sin6_family = AF_INET6;
        }
#else
        fprintf(stderr, "ipaddr error!\n");
        return  (-ERROR_TSHELL_EPARAM);
#endif

    } else {
        pinaddr->sin_len    = sizeof(struct sockaddr_in);
        pinaddr->sin_family = AF_INET;
    }
    
    if (lib_strcmp(ppcArgV[1], "add") == 0) {
        API_LoginWlAdd(&addr);
        return  (ERROR_NONE);
        
    } else if (lib_strcmp(ppcArgV[1], "del") == 0) {
        API_LoginWlDelete(&addr);
        return  (ERROR_NONE);
    }
    
__error:
    fprintf(stderr, "arguments error!\n");
    return  (-ERROR_TSHELL_EPARAM);
}

#endif                                                                  /*  LW_CFG_NET_LOGINBL_EN > 0   */
/*********************************************************************************************************
** ��������: __tshellServbind
** ��������: ϵͳ���� "servbind"
** �䡡��  : iArgC         ��������
**           ppcArgV       ������
** �䡡��  : 0
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  __tshellServbind (INT  iArgC, PCHAR  *ppcArgV)
{
    UINT  uiIndex;

    if (iArgC == 2) {
        if (sscanf(ppcArgV[1], "%d", &uiIndex) != 1) {
            goto    __error;
        }

        if (uiIndex >= LW_CFG_NET_DEV_MAX) {
            fprintf(stderr, "no such device!\n");
            return  (-ERROR_TSHELL_EPARAM);
        }

#if LW_CFG_NET_FTPD_EN > 0
        API_INetFtpServerBindDev(uiIndex);
#endif
#if LW_CFG_NET_TELNET_EN > 0
        API_INetTelnetBindDev(uiIndex);
#endif
#if LW_CFG_NET_NETBIOS_EN > 0
        API_INetNetBiosBindDev(uiIndex);
#endif
#if LW_CFG_NET_TFTP_EN > 0
        API_INetTftpServerBindDev(uiIndex);
#endif
#if LWIP_SNMP
        snmp_bind_if((UINT8)uiIndex);
#endif

    } else if (iArgC == 3) {
        if (sscanf(ppcArgV[2], "%d", &uiIndex) != 1) {
            goto    __error;
        }

        if (uiIndex >= LW_CFG_NET_DEV_MAX) {
            fprintf(stderr, "no such device!\n");
            return  (-ERROR_TSHELL_EPARAM);
        }

        if (lib_strcmp(ppcArgV[1], "ftpd") == 0) {
#if LW_CFG_NET_FTPD_EN > 0
            API_INetFtpServerBindDev(uiIndex);
#endif
        } else if (lib_strcmp(ppcArgV[1], "telnetd") == 0) {
#if LW_CFG_NET_TELNET_EN > 0
            API_INetTelnetBindDev(uiIndex);
#endif
        } else if (lib_strcmp(ppcArgV[1], "netbiosd") == 0) {
#if LW_CFG_NET_NETBIOS_EN > 0
            API_INetNetBiosBindDev(uiIndex);
#endif
        } else if (lib_strcmp(ppcArgV[1], "tftpd") == 0) {
#if LW_CFG_NET_TFTP_EN > 0
            API_INetTftpServerBindDev(uiIndex);
#endif
        } else if (lib_strcmp(ppcArgV[1], "snmpd") == 0) {
#if LWIP_SNMP
            snmp_bind_if((UINT8)uiIndex);
#endif
        } else {
            goto    __error;
        }
    }

    return  (ERROR_NONE);

__error:
    fprintf(stderr, "arguments error!\n");
    return  (-ERROR_TSHELL_EPARAM);
}
/*********************************************************************************************************
** ��������: __tshellIpQos
** ��������: ϵͳ���� "ipqos"
** �䡡��  : iArgC         ��������
**           ppcArgV       ������
** �䡡��  : 0
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  __tshellIpQos (INT  iArgC, PCHAR  *ppcArgV)
{
    INT                 iSock, iRet;
    struct ipqosreq     ipqos;
#if LWIP_IPV6
    struct ip6qosreq    ip6qos;
#endif
    
    if (iArgC == 1) {
        iSock = socket(AF_INET, SOCK_DGRAM, 0);
        if (iSock < 0) {
            fprintf(stderr, "can not create socket, error: %s!\n", lib_strerror(errno));
            return  (PX_ERROR);
        }
        
        iRet = ioctl(iSock, SIOCGETIPQOS, &ipqos);
        if (iRet < 0) {
            fprintf(stderr, "command 'SIOCGETIPQOS', error: %s!\n", lib_strerror(errno));
            close(iSock);
            return  (PX_ERROR);
        }
        
        close(iSock);
        
#if LWIP_IPV6
        iSock = socket(AF_INET6, SOCK_DGRAM, 0);
        if (iSock < 0) {
            fprintf(stderr, "can not create socket, error: %s!\n", lib_strerror(errno));
            return  (PX_ERROR);
        }
        
        iRet = ioctl(iSock, SIOCGETIP6QOS, &ip6qos);
        if (iRet < 0) {
            fprintf(stderr, "command 'SIOCGETIP6QOS', error: %s!\n", lib_strerror(errno));
            close(iSock);
            return  (PX_ERROR);
        }
        
        close(iSock);
#endif
        
#if LWIP_IPV6
        printf("IPv4 QoS: %s IPv6 QoS: %s\n", 
               ipqos.ip_qos_en ? "On" : "Off",
               ip6qos.ip6_qos_en ? "On" : "Off");
#else
        printf("IPv4 QoS: %s\n", ipqos.ip_qos_en ? "On" : "Off");
#endif
    
    } else if (iArgC == 2) {
        iSock = socket(AF_INET, SOCK_DGRAM, 0);
        if (iSock < 0) {
            fprintf(stderr, "can not create socket, error: %s!\n", lib_strerror(errno));
            return  (PX_ERROR);
        }
        
        if (sscanf(ppcArgV[1], "%d", &ipqos.ip_qos_en) != 1) {
            fprintf(stderr, "arguments error!\n");
            close(iSock);
            return  (-ERROR_TSHELL_EPARAM);
        }
        
        iRet = ioctl(iSock, SIOCSETIPQOS, &ipqos);
        if (iRet < 0) {
            fprintf(stderr, "command 'SIOCSETIPQOS', error: %s!\n", lib_strerror(errno));
            close(iSock);
            return  (PX_ERROR);
        }
        
        close(iSock);
    
    } else if (iArgC == 3) {
        if (!lib_strcasecmp(ppcArgV[1], "ipv4")) {
            if (sscanf(ppcArgV[2], "%d", &ipqos.ip_qos_en) != 1) {
                fprintf(stderr, "arguments error!\n");
                return  (-ERROR_TSHELL_EPARAM);
            }
            
            iSock = socket(AF_INET, SOCK_DGRAM, 0);
            if (iSock < 0) {
                fprintf(stderr, "can not create socket, error: %s!\n", lib_strerror(errno));
                return  (PX_ERROR);
            }
            
            iRet = ioctl(iSock, SIOCSETIPQOS, &ipqos);
            if (iRet < 0) {
                fprintf(stderr, "command 'SIOCSETIPQOS', error: %s!\n", lib_strerror(errno));
                close(iSock);
                return  (PX_ERROR);
            }
        
#if LWIP_IPV6
        } else if (!lib_strcasecmp(ppcArgV[1], "ipv6")) {
            if (sscanf(ppcArgV[2], "%d", &ip6qos.ip6_qos_en) != 1) {
                fprintf(stderr, "arguments error!\n");
                return  (-ERROR_TSHELL_EPARAM);
            }
            
            iSock = socket(AF_INET6, SOCK_DGRAM, 0);
            if (iSock < 0) {
                fprintf(stderr, "can not create socket, error: %s!\n", lib_strerror(errno));
                return  (PX_ERROR);
            }
            
            iRet = ioctl(iSock, SIOCSETIP6QOS, &ip6qos);
            if (iRet < 0) {
                fprintf(stderr, "command 'SIOCSETIP6QOS', error: %s!\n", lib_strerror(errno));
                close(iSock);
                return  (PX_ERROR);
            }
#endif
            
        } else {
            fprintf(stderr, "arguments error!\n");
            return  (-ERROR_TSHELL_EPARAM);
        }
        
        close(iSock);
    
    } else {
        fprintf(stderr, "arguments error!\n");
        return  (-ERROR_TSHELL_EPARAM);
    }
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: __tshellIfTcpaf
** ��������: ϵͳ���� "iftcpaf"
** �䡡��  : iArgC         ��������
**           ppcArgV       ������
** �䡡��  : 0
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  __tshellIfTcpaf (INT  iArgC, PCHAR  *ppcArgV)
{
    INT           iSock, iRet;
    struct ifreq  ifreq;
    
    if (iArgC < 2) {
__error:
        fprintf(stderr, "arguments error!\n");
        return  (-ERROR_TSHELL_EPARAM);
    }
    
    if (iArgC > 2) {
        if (sscanf(ppcArgV[2], "%d", &ifreq.ifr_tcpaf) != 1) {
            goto    __error;
        }
    }

    iSock = socket(AF_INET, SOCK_DGRAM, 0);
    if (iSock < 0) {
        fprintf(stderr, "can not create socket, error: %s!\n", lib_strerror(errno));
        return  (PX_ERROR);
    }
    
    lib_strlcpy(ifreq.ifr_name, ppcArgV[1], IFNAMSIZ);
    
    if (iArgC > 2) {
        iRet = ioctl(iSock, SIOCSIFTCPAF, &ifreq);
        if (iRet < ERROR_NONE) {
            fprintf(stderr, "command 'SIOCSIFTCPAF', error: %s!\n", lib_strerror(errno));
            close(iSock);
            return  (PX_ERROR);
        }
        
    } else {
        iRet = ioctl(iSock, SIOCGIFTCPAF, &ifreq);
        if (iRet < ERROR_NONE) {
            fprintf(stderr, "command 'SIOCGIFTCPAF', error: %s!\n", lib_strerror(errno));
            close(iSock);
            return  (PX_ERROR);
        }
    }
    
    close(iSock);
    
    printf("TCP ACK Frequency: %d\n", ifreq.ifr_tcpaf);
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: __tshellIfTcpwnd
** ��������: ϵͳ���� "iftcpwnd"
** �䡡��  : iArgC         ��������
**           ppcArgV       ������
** �䡡��  : 0
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  __tshellIfTcpwnd (INT  iArgC, PCHAR  *ppcArgV)
{
    INT           iSock, iRet;
    struct ifreq  ifreq;
    
    if (iArgC < 2) {
__error:
        fprintf(stderr, "arguments error!\n");
        return  (-ERROR_TSHELL_EPARAM);
    }
    
    if (iArgC > 2) {
        if (sscanf(ppcArgV[2], "%d", &ifreq.ifr_tcpwnd) != 1) {
            goto    __error;
        }
    }

    iSock = socket(AF_INET, SOCK_DGRAM, 0);
    if (iSock < 0) {
        fprintf(stderr, "can not create socket, error: %s!\n", lib_strerror(errno));
        return  (PX_ERROR);
    }
    
    lib_strlcpy(ifreq.ifr_name, ppcArgV[1], IFNAMSIZ);
    
    if (iArgC > 2) {
        iRet = ioctl(iSock, SIOCSIFTCPWND, &ifreq);
        if (iRet < ERROR_NONE) {
            fprintf(stderr, "command 'SIOCSIFTCPWND', error: %s!\n", lib_strerror(errno));
            close(iSock);
            return  (PX_ERROR);
        }
        
    } else {
        iRet = ioctl(iSock, SIOCGIFTCPWND, &ifreq);
        if (iRet < ERROR_NONE) {
            fprintf(stderr, "command 'SIOCGIFTCPWND', error: %s!\n", lib_strerror(errno));
            close(iSock);
            return  (PX_ERROR);
        }
    }
    
    close(iSock);
    
    printf("TCP Window size: %d\n", ifreq.ifr_tcpwnd);
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: __tshellNetInit
** ��������: ע����������
** �䡡��  : NONE
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
VOID  __tshellNetInit (VOID)
{
#if LW_CFG_NET_ROUTER > 0
    __tshellRouteInit();                                                /*  ע�� route ����             */
#if LW_CFG_NET_BALANCING > 0
    __tshellSrouteInit();
#endif                                                                  /*  LW_CFG_NET_BALANCING > 0    */
#endif                                                                  /*  LW_CFG_NET_ROUTER > 0       */

#if LW_CFG_NET_FLOWCTL_EN > 0
    __tshellFlowctlInit();
#endif                                                                  /*  LW_CFG_NET_FLOWCTL_EN > 0   */

    API_TShellKeywordAdd("netstat", __tshellNetstat);
    API_TShellFormatAdd("netstat",  " {[-wtux --A] -i | [-hrigsapl]}");
    API_TShellHelpAdd("netstat",    _G_cNetstatHelp);

    API_TShellKeywordAdd("ifconfig", __tshellIfconfig);
    API_TShellFormatAdd("ifconfig",  " [ifname] [{inet | netmask | gateway}] [address]");
    API_TShellHelpAdd("ifconfig",    "show or set net interface parameter.\n"
                                     "if there are no arguments, it will show all interface parameter\n"
                                     "ifconfig -u (show all interface with 'IFF_UP' state)\n"
                                     "ifconfig -r (show all interface with 'IFF_UP' & 'IFF_RUNNING' state)\n"
                                     "set interface like following:\n"
                                     "ifconfig en1 inet    192.168.0.3\n"
                                     "ifconfig en1 netmask 255.255.255.0\n"
                                     "ifconfig en1 gateway 192.168.0.1\n"
                                     "ifconfig dns 0       192.168.0.2\n");

    API_TShellKeywordAdd("ifup", __tshellIfUp);
    API_TShellFormatAdd("ifup", " [ifname] [{-dhcp | -nodhcp}] [{-dhcp6 | -nodhcp6}]");
    API_TShellHelpAdd("ifup",   "set net interface enable\n"
                                "\"-dncp\"   mean use dhcp client get net address.\n"
                                "\"-nodncp\" mean MUST NOT use dhcp.\n");

    API_TShellKeywordAdd("ifdown", __tshellIfDown);
    API_TShellFormatAdd("ifdown", " [ifname]");
    API_TShellHelpAdd("ifdown",   "set net interface disable.\n");
    
#if LW_CFG_NET_NETDEV_MIP_EN > 0
    API_TShellKeywordAdd("ifmip", __tshellIfMip);
    API_TShellFormatAdd("ifmip", " [ifname] [{add | del}] [ipv4 address [netmask]]");
    API_TShellHelpAdd("ifmip",   "net interface add / delete secondary IPv4 address.\n"
                                 "NOTICE: a fake interface 'mi*' will be create / delete.\n");
#endif                                                                  /*  LW_CFG_NET_NETDEV_MIP_EN    */

    API_TShellKeywordAdd("ifrouter", __tshellIfRouter);
    API_TShellFormatAdd("ifrouter", " [ifname]");
    API_TShellHelpAdd("ifrouter",   "set default router net interface.\n");
    
    API_TShellKeywordAdd("arp", __tshellArp);
    API_TShellFormatAdd("arp", " [-a | -s inet_address physical_address | -d inet_address]");
    API_TShellHelpAdd("arp",   "display ro modifies ARP table.\n"
                               "-a      display the ARP table.\n"
                               "-s      add or set a static arp entry.\n"
                               "        eg. arp -s 192.168.1.100 00:11:22:33:44:55\n"
                               "-d      delete a STATIC arp entry.\n"
                               "        eg. arp -d 192.168.1.100\n");
#if LW_CFG_NET_LOGINBL_EN > 0
    API_TShellKeywordAdd("loginbl", __tshellLoginBl);
    API_TShellFormatAdd("loginbl", " [{add | del}] [ipaddr]");
    API_TShellHelpAdd("loginbl", "show remote login blacklist.\n");
    
    API_TShellKeywordAdd("loginwl", __tshellLoginWl);
    API_TShellFormatAdd("loginwl", " [{add | del}] [ipaddr]");
    API_TShellHelpAdd("loginwl", "show remote login whitelist.\n");
#endif                                                                  /*  LW_CFG_NET_LOGINBL_EN > 0   */

    API_TShellKeywordAdd("servbind", __tshellServbind);
    API_TShellFormatAdd("servbind", " [{ftpd | telnetd | netbiosd | tftpd | snmpd}] [if_index]");
    API_TShellHelpAdd("servbind", "bind SylixOS build-in service to specified network interface.\n"
                                  "eg. servbind 2       bind ALL build-in service to network interface 2\n"
                                  "    servbind ftpd 2  bind 'ftpd' service to network interface 2\n");

    API_TShellKeywordAdd("ipqos", __tshellIpQos);
    API_TShellFormatAdd("ipqos", " [[ipv4 / ipv6] 0 / 1]");
    API_TShellHelpAdd("ipqos", "Set/Get IP QoS support setting\n"
    "eg. ipqos\n"
    "    ipqos          (show IP QoS support setting)\n"
    "    ipqos 1        (enable IP QoS support)\n"
    "    ipqos 0        (disable IP QoS support)\n"
    "    ipqos ipv4 1   (enable IPv4 QoS support)\n"
    "    ipqos ipv6 0   (disable IPv6 QoS support)\n");
    
    API_TShellKeywordAdd("iftcpaf", __tshellIfTcpaf);
    API_TShellFormatAdd("iftcpaf", " [ifname] [tcpaf (2~127)]");
    API_TShellHelpAdd("iftcpaf", "Set/Get TCP ACK Frequency for net interface\n"
    "eg. iftcpaf lo0    (get 'lo0' TCP ACK Frequency setting)\n"
    "    iftcpaf en1 4  (set 'en1' TCP ACK Frequency to 4)\n");
    
    API_TShellKeywordAdd("iftcpwnd", __tshellIfTcpwnd);
    API_TShellFormatAdd("iftcpwnd", " [ifname] [tcp_wnd]");
    API_TShellHelpAdd("iftcpwnd", "Set/Get TCP Window size for net interface\n"
    "eg. iftcpwnd lo0        (get 'lo0' TCP Window size setting)\n"
    "    iftcpwnd en1 65535  (set 'en1' TCP Window size to 65535)\n");
}

#endif                                                                  /*  LW_CFG_NET_EN > 0           */
                                                                        /*  LW_CFG_SHELL_EN > 0         */
/*********************************************************************************************************
  END
*********************************************************************************************************/
