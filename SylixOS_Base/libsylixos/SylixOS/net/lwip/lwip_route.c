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
** ��   ��   ��: lwip_route.c
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2011 �� 07 �� 01 ��
**
** ��        ��: lwip sylixos ·�ɱ�.
                 lwip ·�ɱ�ӿڵ�������, �����: http://savannah.nongnu.org/bugs/?33634

** BUG:
2011.07.07  �� net safe ״̬�²�������� printf ��ʹ�� IO �����.
2011.08.17  __rtSafeRun() �ڰ�װ lo0 ����ʱ, lwip core lock ��û�д���, ����������Ҫ�ж�һ��!
2012.03.29  __aodvEntryPrint() hcnt > 0 ������ת���ڵ�, Ӧ��ʾΪ G.
2013.01.15  �����·�ɲ����ӿ�.
2013.01.16  LW_RT_FLAG_G ��־Ϊ��̬��, ÿ�α���ʱ�Ż����, ��Ϊ���������ĸĶ����ܻ�����˱�־�ı仯.
2013.01.24  route ����ٴ�ӡ aodv ·�ɱ�, ���� aodvs �����ӡ aodv ·�ɱ�.
2013.06.21  adovs ��������Ŀ�������͵������ӵ���ʾ.
2013.08.22  route_msg ���� metric �ֶ�, ����ǰ����.
2014.07.02  �����ڽ�·�ɱ���� ppp ���ӵĴ�����ʾ.
2016.07.16  ÿһ��·����Ϣ������������.
2017.02.13  ���� AODV �ü�֧��.
*********************************************************************************************************/
#define  __SYLIXOS_STDIO
#define  __SYLIXOS_KERNEL
#include "SylixOS.h"
/*********************************************************************************************************
  �ü�����
*********************************************************************************************************/
#if LW_CFG_NET_EN > 0 && LW_CFG_NET_ROUTER > 0
#include "sys/route.h"
#include "net/route.h"
#if LW_CFG_NET_BALANCING > 0
#include "net/sroute.h"
#endif                                                                  /*  LW_CFG_NET_BALANCING > 0    */
#if LW_CFG_NET_AODV_EN > 0
#include "src/netif/aodv/aodv_route.h"                                  /*  AODV ·�ɱ�                 */
#endif
/*********************************************************************************************************
** ��������: __route_show_ipv4
** ��������: ��ӡ IPv4 ·�ɱ�
** �䡡��  : NONE
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
#if LW_CFG_SHELL_EN > 0

static VOID  __route_show_ipv4 (VOID)
{
    INT                  iRet, i;
    INT                  iSock;
    CHAR                 cFlags[10];
    CHAR                 cStrDest[IP4ADDR_STRLEN_MAX];
    CHAR                 cStrGateway[IP4ADDR_STRLEN_MAX];
    CHAR                 cStrNetmask[IP4ADDR_STRLEN_MAX];
    struct rtentry_list  rtentrylist;
    struct rtentry      *prtentry;
    
    rtentrylist.rtl_bcnt  = 0;
    rtentrylist.rtl_num   = 0;
    rtentrylist.rtl_total = 0;
    rtentrylist.rtl_buf   = LW_NULL;
    
    printf("IPv4 Route Table:\n");
    printf("Destination     Gateway         Genmask         Flags Metric Ref    Use Iface\n");
    
    iSock = socket(AF_INET, SOCK_DGRAM, 0);
    if (iSock < 0) {
        fprintf(stderr, "can not create socket error: %s!\n", lib_strerror(errno));
        return;
    }
    
    iRet = ioctl(iSock, SIOCLSTRT, &rtentrylist);
    if (iRet < 0) {
        fprintf(stderr, "command 'SIOCLSTRT' error: %s!\n", lib_strerror(errno));
        close(iSock);
        return;
    }
    
    if (rtentrylist.rtl_total == 0) {
        close(iSock);
        printf("\n");
        return;
    }
    
    rtentrylist.rtl_buf = (struct rtentry *)__SHEAP_ALLOC(sizeof(struct rtentry) * rtentrylist.rtl_total);
    if (!rtentrylist.rtl_buf) {
        fprintf(stderr, "error: %s!\n", lib_strerror(errno));
        close(iSock);
        return;
    }
    
    rtentrylist.rtl_bcnt = rtentrylist.rtl_total;
    iRet = ioctl(iSock, SIOCLSTRT, &rtentrylist);
    if (iRet < 0) {
        fprintf(stderr, "command 'SIOCLSTRT' error: %s!\n", lib_strerror(errno));
        __SHEAP_FREE(rtentrylist.rtl_buf);
        close(iSock);
        return;
    }
    close(iSock);
    
    for (i = 0; i < rtentrylist.rtl_num; i++) {
        prtentry = &rtentrylist.rtl_buf[i];
        inet_ntoa_r(((struct sockaddr_in *)&prtentry->rt_dst)->sin_addr,     cStrDest,    IP4ADDR_STRLEN_MAX);
        inet_ntoa_r(((struct sockaddr_in *)&prtentry->rt_gateway)->sin_addr, cStrGateway, IP4ADDR_STRLEN_MAX);
        inet_ntoa_r(((struct sockaddr_in *)&prtentry->rt_genmask)->sin_addr, cStrNetmask, IP4ADDR_STRLEN_MAX);
        
        cFlags[0] = '\0';
        if (prtentry->rt_flags & RTF_UP) {
            lib_strcat(cFlags, "U");
        }
        if (prtentry->rt_flags & RTF_GATEWAY) {
            lib_strcat(cFlags, "G");
        }
        if (prtentry->rt_flags & RTF_HOST) {
            lib_strcat(cFlags, "H");
        }
        if (prtentry->rt_flags & RTF_DYNAMIC) {
            lib_strcat(cFlags, "D");
        }
        
        printf("%-15s %-15s %-15s %-5s %-6d %-6d %3d %s\n", 
               cStrDest, cStrGateway, cStrNetmask, cFlags,
               prtentry->rt_metric, prtentry->rt_refcnt, 
               0, prtentry->rt_ifname);
    }
    
    __SHEAP_FREE(rtentrylist.rtl_buf);
    printf("\n");
}
/*********************************************************************************************************
** ��������: __route_show_ipv6
** ��������: ��ӡ IPv6 ·�ɱ�
** �䡡��  : NONE
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
#if LWIP_IPV6

static VOID  __route_show_ipv6 (VOID)
{
    extern int  rt6_netmask_to_prefix(struct ip6_addr *netmask);

    INT                  iRet, i;
    INT                  iSock, iPrefix;
    CHAR                 cFlags[10];
    CHAR                 cStrDest[IP6ADDR_STRLEN_MAX];
    CHAR                 cStrGateway[IP6ADDR_STRLEN_MAX];
    struct ip6_addr      ip6addr;
    struct rtentry_list  rtentrylist;
    struct rtentry      *prtentry;
    
    rtentrylist.rtl_bcnt  = 0;
    rtentrylist.rtl_num   = 0;
    rtentrylist.rtl_total = 0;
    rtentrylist.rtl_buf   = LW_NULL;
    
    printf("IPv6 Route Table:\n");
    printf("Destination                      Gateway                          Prefix Flags Metric Ref    Use Iface\n");
    
    iSock = socket(AF_INET6, SOCK_DGRAM, 0);
    if (iSock < 0) {
        fprintf(stderr, "can not create socket error: %s!\n", lib_strerror(errno));
        return;
    }
    
    iRet = ioctl(iSock, SIOCLSTRT, &rtentrylist);
    if (iRet < 0) {
        fprintf(stderr, "command 'SIOCLSTRT' error: %s!\n", lib_strerror(errno));
        close(iSock);
        return;
    }
    
    if (rtentrylist.rtl_total == 0) {
        close(iSock);
        printf("\n");
        return;
    }
    
    rtentrylist.rtl_buf = (struct rtentry *)__SHEAP_ALLOC(sizeof(struct rtentry) * rtentrylist.rtl_total);
    if (!rtentrylist.rtl_buf) {
        fprintf(stderr, "error: %s!\n", lib_strerror(errno));
        close(iSock);
        return;
    }
    
    rtentrylist.rtl_bcnt = rtentrylist.rtl_total;
    iRet = ioctl(iSock, SIOCLSTRT, &rtentrylist);
    if (iRet < 0) {
        fprintf(stderr, "command 'SIOCLSTRT' error: %s!\n", lib_strerror(errno));
        __SHEAP_FREE(rtentrylist.rtl_buf);
        close(iSock);
        return;
    }
    close(iSock);
    
    for (i = 0; i < rtentrylist.rtl_num; i++) {
        prtentry = &rtentrylist.rtl_buf[i];
        inet6_ntoa_r(((struct sockaddr_in6 *)&prtentry->rt_dst)->sin6_addr,     cStrDest,    IP6ADDR_STRLEN_MAX);
        inet6_ntoa_r(((struct sockaddr_in6 *)&prtentry->rt_gateway)->sin6_addr, cStrGateway, IP6ADDR_STRLEN_MAX);
        inet6_addr_to_ip6addr(&ip6addr, &((struct sockaddr_in6 *)&prtentry->rt_genmask)->sin6_addr);
        iPrefix = rt6_netmask_to_prefix(&ip6addr);
        
        cFlags[0] = '\0';
        if (prtentry->rt_flags & RTF_UP) {
            lib_strcat(cFlags, "U");
        }
        if (prtentry->rt_flags & RTF_GATEWAY) {
            lib_strcat(cFlags, "G");
        }
        if (prtentry->rt_flags & RTF_HOST) {
            lib_strcat(cFlags, "H");
        }
        if (prtentry->rt_flags & RTF_DYNAMIC) {
            lib_strcat(cFlags, "D");
        }
        
        printf("%-32s %-32s %-6d %-5s %-6d %-6d %3d %s\n", 
               cStrDest, cStrGateway, iPrefix, cFlags,
               prtentry->rt_metric, prtentry->rt_refcnt, 
               0, prtentry->rt_ifname);
    }
    
    __SHEAP_FREE(rtentrylist.rtl_buf);
    printf("\n");
}

#endif                                                                  /*  LWIP_IPV6                   */
/*********************************************************************************************************
** ��������: __route_show_aodv
** ��������: ��ӡ AODV ·�ɱ�
** �䡡��  : NONE
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
#if LW_CFG_NET_AODV_EN > 0

static VOID  __route_show_aodv (VOID)
{
    INT     iFd = open("/proc/net/mesh-adhoc/aodv_rt", O_RDONLY);
    CHAR    cBuf[128];
    ssize_t sstNum;
    
    if (iFd < 0) {
        fprintf(stderr, "can not open '/proc/net/mesh-adhoc/aodv_rt' error: %s!\n", lib_strerror(errno));
        return;
    }
    
    printf("AODV Route Table:\n");
    
    for (;;) {
        sstNum = read(iFd, cBuf, sizeof(cBuf));
        if (sstNum > 0) {
            write(STD_OUT, cBuf, (size_t)sstNum);
        } else {
            break;
        }
    }
    
    close(iFd);
    printf("\n");
}

#endif                                                                  /*  LW_CFG_NET_AODV_EN > 0      */
/*********************************************************************************************************
** ��������: __route_default
** ��������: shell �������Ĭ��·��
** �䡡��  : iArgC         ��������
**           ppcArgV       ������
**           prtentry      ·����Ŀ
** �䡡��  : ERROR or OK
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static INT  __route_default (INT  iArgC, PCHAR  *ppcArgV, struct rtentry *prtentry)
{
    if ((iArgC == 5) && (!lib_strcmp(ppcArgV[2], "default")) && (!lib_strcmp(ppcArgV[3], "dev"))) {
        if (!inet_aton("0.0.0.0", &((struct sockaddr_in *)&prtentry->rt_gateway)->sin_addr)) {
            goto    __arg_error;
        }
        lib_strlcpy(prtentry->rt_ifname, ppcArgV[4], IF_NAMESIZE);
    
    } else if ((iArgC >= 5) && (!lib_strcmp(ppcArgV[2], "default")) && (!lib_strcmp(ppcArgV[3], "gw"))) {
        if (!inet_aton(ppcArgV[4], &((struct sockaddr_in *)&prtentry->rt_gateway)->sin_addr)) {
            goto    __arg_error;
        }
        
        if ((iArgC >= 7) && (!lib_strcmp(ppcArgV[5], "dev"))) {
            lib_strlcpy(prtentry->rt_ifname, ppcArgV[6], IF_NAMESIZE);
        }
        
    } else {
__arg_error:
        fprintf(stderr, "arguments error!\n");
        return  (-ERROR_TSHELL_EPARAM);
    }
    
    if (route_add(prtentry) < 0) {
        fprintf(stderr, "add default route fail, error: %s!\n", lib_strerror(errno));
        return  (PX_ERROR);
    }
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: __route_add
** ��������: shell �������һ��·��
** �䡡��  : iArgC         ��������
**           ppcArgV       ������
** �䡡��  : ERROR or OK
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static INT  __route_add (INT  iArgC, PCHAR  *ppcArgV)
{
    INT             i, iMetric = 0;
    struct rtentry  rtentry;
    
    lib_bzero(&rtentry, sizeof(struct rtentry));
    
    rtentry.rt_flags |= RTF_UP;
    
    rtentry.rt_dst.sa_len    = sizeof(struct sockaddr_in);
    rtentry.rt_dst.sa_family = AF_INET;
    
    rtentry.rt_genmask.sa_len    = sizeof(struct sockaddr_in);
    rtentry.rt_genmask.sa_family = AF_INET;
    
    rtentry.rt_gateway.sa_len    = sizeof(struct sockaddr_in);
    rtentry.rt_gateway.sa_family = AF_INET;
    
    if ((iArgC > 3) && (!lib_strcmp(ppcArgV[2], "default"))) {
        return  (__route_default(iArgC, ppcArgV, &rtentry));
    }
    
    if ((iArgC < 7) || !lib_strstr(ppcArgV[4], "mask")) {
__arg_error:
        fprintf(stderr, "arguments error!\n");
        return  (-ERROR_TSHELL_EPARAM);
    }

    if (!lib_strcmp(ppcArgV[2], "-host")) {
        rtentry.rt_flags |= RTF_HOST;
    
    } else if (!lib_strcmp(ppcArgV[2], "-gateway")) {
        rtentry.rt_flags |= RTF_HOST | RTF_GATEWAY;
        
    } else if (lib_strcmp(ppcArgV[2], "-net")) {
        goto    __arg_error;
    }
    
    if (!inet_aton(ppcArgV[3], &((struct sockaddr_in *)&rtentry.rt_dst)->sin_addr)) {
        goto    __arg_error;
    }
    
    if (!inet_aton(ppcArgV[5], &((struct sockaddr_in *)&rtentry.rt_genmask)->sin_addr)) {
        goto    __arg_error;
    }
    
    if (!inet_aton(ppcArgV[6], &((struct sockaddr_in *)&rtentry.rt_gateway)->sin_addr)) {
        goto    __arg_error;
    }
    
    i = 7;
    if (iArgC > (i + 1)) {
        if (lib_strstr(ppcArgV[i], "metric")) {
            if (sscanf(ppcArgV[i + 1], "%d", &iMetric) != 1) {
                goto    __arg_error;
            }
            i += 2;
        }
    }
    
    if (iArgC > (i + 1)) {
        if (lib_strstr(ppcArgV[i], "dev")) {
            lib_strlcpy(rtentry.rt_ifname, ppcArgV[i + 1], IF_NAMESIZE);
        }
    }
    
    rtentry.rt_metric = (short)iMetric;
    
    if (route_add(&rtentry) < 0) {
        fprintf(stderr, "add route fail, error: %s!\n", lib_strerror(errno));
        return  (PX_ERROR);
    }
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: __route_delete
** ��������: shell �������һ��·��
** �䡡��  : iArgC         ��������
**           ppcArgV       ������
** �䡡��  : ERROR or OK
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static INT  __route_delete (INT  iArgC, PCHAR  *ppcArgV)
{
    struct rtentry  rtentry;

    if (iArgC < 4) {
__arg_error:
        fprintf(stderr, "arguments error!\n");
        return  (-ERROR_TSHELL_EPARAM);
    }
    
    lib_bzero(&rtentry, sizeof(struct rtentry));
    
    rtentry.rt_dst.sa_len    = sizeof(struct sockaddr_in);
    rtentry.rt_dst.sa_family = AF_INET;
    
    rtentry.rt_genmask.sa_len    = sizeof(struct sockaddr_in);
    rtentry.rt_genmask.sa_family = AF_INET;
    
    rtentry.rt_gateway.sa_len    = sizeof(struct sockaddr_in);
    rtentry.rt_gateway.sa_family = AF_INET;
    
    if (!lib_strcmp(ppcArgV[2], "-host")) {
        rtentry.rt_flags |= RTF_HOST;
        
    } else if (lib_strcmp(ppcArgV[2], "-net")) {
        goto    __arg_error;
    }
    
    if (!inet_aton(ppcArgV[3], &((struct sockaddr_in *)&rtentry.rt_dst)->sin_addr)) {
        goto    __arg_error;
    }
    
    if (rtentry.rt_flags & RTF_HOST) {
        if (iArgC > 5) {
            if (lib_strstr(ppcArgV[4], "dev")) {
                lib_strlcpy(rtentry.rt_ifname, ppcArgV[5], IF_NAMESIZE);
            
            } else if (lib_strstr(ppcArgV[4], "gw")) {
                if (!inet_aton(ppcArgV[5], &((struct sockaddr_in *)&rtentry.rt_gateway)->sin_addr)) {
                    goto    __arg_error;
                }
            }
        }
        
    } else {
        if (iArgC < 6) {
            goto    __arg_error;
        }
        if (!lib_strstr(ppcArgV[4], "mask")) {
            goto    __arg_error;
        }
        if (!inet_aton(ppcArgV[5], &((struct sockaddr_in *)&rtentry.rt_genmask)->sin_addr)) {
            goto    __arg_error;
        }
    }
    
    if (route_delete(&rtentry) < 0) {
        fprintf(stderr, "delete route fail, error: %s!\n", lib_strerror(errno));
        return  (PX_ERROR);
    }
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: __route_change
** ��������: shell �������һ��·��
** �䡡��  : iArgC         ��������
**           ppcArgV       ������
** �䡡��  : ERROR or OK
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static INT  __route_change (INT  iArgC, PCHAR  *ppcArgV)
{
    INT             i, iMetric = 0;
    struct rtentry  rtentry;
    
    lib_bzero(&rtentry, sizeof(struct rtentry));
    
    rtentry.rt_flags |= RTF_UP;
    
    rtentry.rt_dst.sa_len    = sizeof(struct sockaddr_in);
    rtentry.rt_dst.sa_family = AF_INET;
    
    rtentry.rt_genmask.sa_len    = sizeof(struct sockaddr_in);
    rtentry.rt_genmask.sa_family = AF_INET;
    
    rtentry.rt_gateway.sa_len    = sizeof(struct sockaddr_in);
    rtentry.rt_gateway.sa_family = AF_INET;
    
    if ((iArgC > 3) && (!lib_strcmp(ppcArgV[2], "default"))) {
        return  (__route_default(iArgC, ppcArgV, &rtentry));
    }
    
    if ((iArgC < 7) || !lib_strstr(ppcArgV[4], "mask")) {
__arg_error:
        fprintf(stderr, "arguments error!\n");
        return  (-ERROR_TSHELL_EPARAM);
    }

    if (!lib_strcmp(ppcArgV[2], "-host")) {
        rtentry.rt_flags |= RTF_HOST;
    
    } else if (!lib_strcmp(ppcArgV[2], "-gateway")) {
        rtentry.rt_flags |= RTF_HOST | RTF_GATEWAY;
        
    } else if (lib_strcmp(ppcArgV[2], "-net")) {
        goto    __arg_error;
    }
    
    if (!inet_aton(ppcArgV[3], &((struct sockaddr_in *)&rtentry.rt_dst)->sin_addr)) {
        goto    __arg_error;
    }
    
    if (!inet_aton(ppcArgV[5], &((struct sockaddr_in *)&rtentry.rt_genmask)->sin_addr)) {
        goto    __arg_error;
    }
    
    if (!inet_aton(ppcArgV[6], &((struct sockaddr_in *)&rtentry.rt_gateway)->sin_addr)) {
        goto    __arg_error;
    }
    
    i = 7;
    if (iArgC > (i + 1)) {
        if (lib_strstr(ppcArgV[i], "metric")) {
            if (sscanf(ppcArgV[i + 1], "%d", &iMetric) != 1) {
                goto    __arg_error;
            }
            i += 2;
        }
    }
    
    if (iArgC > (i + 1)) {
        if (lib_strstr(ppcArgV[i], "dev")) {
            lib_strlcpy(rtentry.rt_ifname, ppcArgV[i + 1], IF_NAMESIZE);
        }
    }
    
    rtentry.rt_metric = (short)iMetric;
    
    if (route_change(&rtentry) < 0) {
        fprintf(stderr, "change route fail, error: %s!\n", lib_strerror(errno));
        return  (PX_ERROR);
    }
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: __tshellRoute
** ��������: ϵͳ���� "route"
** �䡡��  : iArgC         ��������
**           ppcArgV       ������
** �䡡��  : ERROR
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
INT  __tshellRoute (INT  iArgC, PCHAR  *ppcArgV)
{
    INT  iRet;

    if (iArgC == 1) {
        __route_show_ipv4();
#if LWIP_IPV6
        __route_show_ipv6();
#endif                                                                  /*  LWIP_IPV6                   */
#if LW_CFG_NET_AODV_EN > 0
        __route_show_aodv();
#endif                                                                  /*  LW_CFG_NET_AODV_EN > 0      */
        return  (ERROR_NONE);
    
    } else if (iArgC > 1) {
        if (!lib_strcmp(ppcArgV[1], "add")) {
            iRet = __route_add(iArgC, ppcArgV);
            
        } else if (!lib_strcmp(ppcArgV[1], "del") || !lib_strcmp(ppcArgV[1], "delete")) {
            iRet = __route_delete(iArgC, ppcArgV);
            
        } else if (!lib_strcmp(ppcArgV[1], "chg") || !lib_strcmp(ppcArgV[1], "change")) {
            iRet = __route_change(iArgC, ppcArgV);
            
        } else {
            goto    __arg_error;
        }
        
    } else {
__arg_error:
        fprintf(stderr, "arguments error!\n");
        return  (-ERROR_TSHELL_EPARAM);
    }
    
    return  (iRet);
}
/*********************************************************************************************************
** ��������: __tshellTcpMssAdj
** ��������: ϵͳ���� "rtmssadj"
** �䡡��  : iArgC         ��������
**           ppcArgV       ������
** �䡡��  : ERROR
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  __tshellTcpMssAdj (INT  iArgC, PCHAR  *ppcArgV)
{
    INT   iSock, iRet, iEnbale;

    iSock = socket(AF_INET, SOCK_DGRAM, 0);
    if (iSock < 0) {
        fprintf(stderr, "can not create route socket, error: %s!\n", lib_strerror(errno));
        return  (PX_ERROR);
    }

    if (iArgC <= 1) {
        iRet = ioctl(iSock, SIOCGTCPMSSADJ, &iEnbale);
        if (iRet < 0) {
            fprintf(stderr, "command 'SIOCGTCPMSSADJ', error: %s!\n", lib_strerror(errno));
            close(iSock);
            return  (PX_ERROR);
        }

        printf("TCP forward MSS adjust: %s\n", iEnbale ? "On" : "Off");

    } else {
        if (sscanf(ppcArgV[1], "%d", &iEnbale) != 1) {
            fprintf(stderr, "arguments error!\n");
            close(iSock);
            return  (-ERROR_TSHELL_EPARAM);
        }

        iRet = ioctl(iSock, SIOCSTCPMSSADJ, &iEnbale);
        if (iRet < 0) {
            fprintf(stderr, "command 'SIOCSTCPMSSADJ', error: %s!\n", lib_strerror(errno));
            close(iSock);
            return  (PX_ERROR);
        }

        printf("TCP forward MSS adjust: %s\n", iEnbale ? "On" : "Off");
    }

    close(iSock);
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: __tshellIpForward
** ��������: ϵͳ���� "ipforward"
** �䡡��  : iArgC         ��������
**           ppcArgV       ������
** �䡡��  : ERROR
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  __tshellIpForward (INT  iArgC, PCHAR  *ppcArgV)
{
    INT                 iSock, iRet;
    struct rt_forward   rtf;

    if (iArgC == 1) {
        iSock = socket(AF_INET, SOCK_DGRAM, 0);
        if (iSock < 0) {
            fprintf(stderr, "can not create route socket, error: %s!\n", lib_strerror(errno));
            return  (PX_ERROR);
        }
        
        iRet = ioctl(iSock, SIOCGFWOPT, &rtf);
        if (iRet < 0) {
            fprintf(stderr, "command 'SIOCGFWOPT', error: %s!\n", lib_strerror(errno));
            close(iSock);
            return  (PX_ERROR);
        }
        
        printf("IPv4 forward: %s IPv6 forward: %s\n", 
               rtf.rtf_ip4fw ? "On" : "Off",
               rtf.rtf_ip6fw ? "On" : "Off");
               
        close(iSock);
    
    } else if (iArgC == 2) {
        iSock = socket(AF_INET, SOCK_DGRAM, 0);
        if (iSock < 0) {
            fprintf(stderr, "can not create route socket, error: %s!\n", lib_strerror(errno));
            return  (PX_ERROR);
        }
        
        if (sscanf(ppcArgV[1], "%d", &rtf.rtf_ip4fw) != 1) {
            fprintf(stderr, "arguments error!\n");
            close(iSock);
            return  (-ERROR_TSHELL_EPARAM);
        }
        
        rtf.rtf_ip6fw = rtf.rtf_ip4fw;
        
        iRet = ioctl(iSock, SIOCSFWOPT, &rtf);
        if (iRet < 0) {
            fprintf(stderr, "command 'SIOCSFWOPT', error: %s!\n", lib_strerror(errno));
            close(iSock);
            return  (PX_ERROR);
        }
        
        close(iSock);
    
    } else if (iArgC == 3) {
        iSock = socket(AF_INET, SOCK_DGRAM, 0);
        if (iSock < 0) {
            fprintf(stderr, "can not create route socket, error: %s!\n", lib_strerror(errno));
            return  (PX_ERROR);
        }
        
        iRet = ioctl(iSock, SIOCGFWOPT, &rtf);
        if (iRet < 0) {
            fprintf(stderr, "command 'SIOCGFWOPT', error: %s!\n", lib_strerror(errno));
            close(iSock);
            return  (PX_ERROR);
        }
    
        if (!lib_strcasecmp(ppcArgV[1], "ipv4")) {
            if (sscanf(ppcArgV[2], "%d", &rtf.rtf_ip4fw) != 1) {
                fprintf(stderr, "arguments error!\n");
                close(iSock);
                return  (-ERROR_TSHELL_EPARAM);
            }
        
        } else if (!lib_strcasecmp(ppcArgV[1], "ipv6")) {
            if (sscanf(ppcArgV[2], "%d", &rtf.rtf_ip6fw) != 1) {
                fprintf(stderr, "arguments error!\n");
                close(iSock);
                return  (-ERROR_TSHELL_EPARAM);
            }
            
        } else {
            fprintf(stderr, "arguments error!\n");
            close(iSock);
            return  (-ERROR_TSHELL_EPARAM);
        }
        
        iRet = ioctl(iSock, SIOCSFWOPT, &rtf);
        if (iRet < 0) {
            fprintf(stderr, "command 'SIOCSFWOPT', error: %s!\n", lib_strerror(errno));
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
/*********************************************************************************************************
** ��������: __tshellRouteInit
** ��������: ע��·��������
** �䡡��  : NONE
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
VOID __tshellRouteInit (VOID)
{
    API_TShellKeywordAdd("route", __tshellRoute);
    API_TShellFormatAdd("route", " [add | del | chg] {-host | -net | -gateway} [dest] [netmask] [gateway] {metric} [dev]");
    API_TShellHelpAdd("route",   "show, add, delete, change route table\n"
    "eg. route\n"
    "    route add -host(-net) 123.123.123.123 mask 255.0.0.0 0.0.0.0 metric 5 dev en1  (add a route and use netif default gateway set)\n"
    "    route add -host(-net) 123.123.123.123 mask 255.0.0.0 123.0.0.1 dev en1         (add a route and use specified gateway set)\n"
    "    route add -host(-net) 0.0.0.0 mask 0.0.0.0 123.0.0.1 dev en1                   (set netif default gateway: 123.0.0.1)\n"
    "    route chg -host(-net) 123.123.123.123 mask 255.0.0.0 0.0.0.0 dev en2           (change a route and use netif default gateway set)\n"
    "    route chg -host(-net) 123.123.123.123 mask 255.0.0.0 123.0.0.1 dev en1         (change a route and use specified gateway set)\n"
    "    route chg -host(-net) 0.0.0.0 mask 255.0.0.0 123.0.0.1 dev en1                 (set netif default gateway: 123.0.0.1)\n"
    "    route add -host(-net) 0.0.0.0 mask 0.0.0.0 192.168.1.1 dev en2                 (set default netif)\n"
    "    route add default dev en2                                                      (set default netif)\n"
    "    route add default gw 192.168.1.1                                               (set default netif)\n"
    "    route add default gw 192.168.1.1 dev en1                                       (set default netif)\n"
    "    route del -net  123.0.0.0 mask 255.0.0.0                                       (delete a net route)\n"
    "    route del -host 145.26.122.35 gw 192.168.1.1                                   (delete a host route)\n"
    "    route del -host 145.26.122.35 dev en1                                          (delete a host route)\n");

    API_TShellKeywordAdd("rtmssadj", __tshellTcpMssAdj);
    API_TShellFormatAdd("rtmssadj", " [0 / 1]");
    API_TShellHelpAdd("rtmssadj", "Set/Get TCP forward MSS adjust status\n");
    
    API_TShellKeywordAdd("ipforward", __tshellIpForward);
    API_TShellFormatAdd("ipforward", " [[ipv4 / ipv6] 0 / 1]");
    API_TShellHelpAdd("ipforward", "Set/Get IP forward setting\n"
    "eg. ipforward\n"
    "    ipforward          (show IP forward setting)\n"
    "    ipforward 1        (enable IP forward)\n"
    "    ipforward 0        (disable IP forward)\n"
    "    ipforward ipv4 1   (enable IPv4 forward)\n"
    "    ipforward ipv6 0   (disable IPv6 forward)\n");
}
/*********************************************************************************************************
** ��������: __sroute_show_ipv4
** ��������: ��ӡ IPv4 Դ·�ɱ�
** �䡡��  : NONE
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
#if LW_CFG_NET_BALANCING > 0

static VOID  __sroute_show_ipv4 (VOID)
{
    INT                  iRet, i;
    INT                  iSock;
    CHAR                 cFlags[10];
    CHAR                 cStrSSrc[IP4ADDR_STRLEN_MAX];
    CHAR                 cStrESrc[IP4ADDR_STRLEN_MAX];
    CHAR                 cStrSDest[IP4ADDR_STRLEN_MAX];
    CHAR                 cStrEDest[IP4ADDR_STRLEN_MAX];
    struct srtentry_list srtentrylist;
    struct srtentry     *psrtentry;
    
    srtentrylist.srtl_bcnt  = 0;
    srtentrylist.srtl_num   = 0;
    srtentrylist.srtl_total = 0;
    srtentrylist.srtl_buf   = LW_NULL;
    
    printf("IPv4 Source route Table:\n");
    printf("Source(s)       Source(e)       Flags Destination(s)  Destination(e)  Mode Prio Iface\n");
    
    iSock = socket(AF_INET, SOCK_DGRAM, 0);
    if (iSock < 0) {
        fprintf(stderr, "can not create socket error: %s!\n", lib_strerror(errno));
        return;
    }
    
    iRet = ioctl(iSock, SIOCLSTSRT, &srtentrylist);
    if (iRet < 0) {
        fprintf(stderr, "command 'SIOCLSTSRT' error: %s!\n", lib_strerror(errno));
        close(iSock);
        return;
    }
    
    if (srtentrylist.srtl_total == 0) {
        close(iSock);
        printf("\n");
        return;
    }
    
    srtentrylist.srtl_buf = (struct srtentry *)__SHEAP_ALLOC(sizeof(struct srtentry) * srtentrylist.srtl_total);
    if (!srtentrylist.srtl_buf) {
        fprintf(stderr, "error: %s!\n", lib_strerror(errno));
        close(iSock);
        return;
    }
    
    srtentrylist.srtl_bcnt = srtentrylist.srtl_total;
    iRet = ioctl(iSock, SIOCLSTSRT, &srtentrylist);
    if (iRet < 0) {
        fprintf(stderr, "command 'SIOCLSTSRT' error: %s!\n", lib_strerror(errno));
        close(iSock);
        return;
    }
    close(iSock);
    
    for (i = 0; i < srtentrylist.srtl_num; i++) {
        psrtentry = &srtentrylist.srtl_buf[i];
        inet_ntoa_r(((struct sockaddr_in *)&psrtentry->srt_ssrc)->sin_addr, cStrSSrc, IP4ADDR_STRLEN_MAX);
        inet_ntoa_r(((struct sockaddr_in *)&psrtentry->srt_esrc)->sin_addr, cStrESrc, IP4ADDR_STRLEN_MAX);
        inet_ntoa_r(((struct sockaddr_in *)&psrtentry->srt_sdest)->sin_addr, cStrSDest, IP4ADDR_STRLEN_MAX);
        inet_ntoa_r(((struct sockaddr_in *)&psrtentry->srt_edest)->sin_addr, cStrEDest, IP4ADDR_STRLEN_MAX);

        cFlags[0] = '\0';
        if (psrtentry->srt_flags & RTF_UP) {
            lib_strcat(cFlags, "U");
        }
        if (psrtentry->srt_flags & RTF_DYNAMIC) {
            lib_strcat(cFlags, "D");
        }
        
        printf("%-15s %-15s %-5s %-15s %-15s %-4s %-4s %s\n",
               cStrSSrc, cStrESrc, cFlags, cStrSDest, cStrEDest,
               (psrtentry->srt_mode == SRT_MODE_EXCLUDE) ? "EXC" : "INC",
               (psrtentry->srt_prio == SRT_PRIO_HIGH) ? "HIGH" : "DEF",
               psrtentry->srt_ifname);
    }
    
    __SHEAP_FREE(srtentrylist.srtl_buf);
    printf("\n");
}
/*********************************************************************************************************
** ��������: __sroute_show_ipv6
** ��������: ��ӡ IPv4 Դ·�ɱ�
** �䡡��  : NONE
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
#if LWIP_IPV6

static VOID  __sroute_show_ipv6 (VOID)
{
    INT                  iRet, i;
    INT                  iSock;
    CHAR                 cFlags[10];
    CHAR                 cStrSSrc[IP6ADDR_STRLEN_MAX];
    CHAR                 cStrESrc[IP6ADDR_STRLEN_MAX];
    CHAR                 cStrSDest[IP6ADDR_STRLEN_MAX];
    CHAR                 cStrEDest[IP6ADDR_STRLEN_MAX];
    struct srtentry_list srtentrylist;
    struct srtentry     *psrtentry;
    
    srtentrylist.srtl_bcnt  = 0;
    srtentrylist.srtl_num   = 0;
    srtentrylist.srtl_total = 0;
    srtentrylist.srtl_buf   = LW_NULL;
    
    printf("IPv6 Source route Table:\n");
    printf("Source(s)                        Source(e)                        Flags "
           "Destination(s)                   Destination(e)                   Mode Prio Iface\n");
    
    iSock = socket(AF_INET6, SOCK_DGRAM, 0);
    if (iSock < 0) {
        fprintf(stderr, "can not create socket error: %s!\n", lib_strerror(errno));
        return;
    }
    
    iRet = ioctl(iSock, SIOCLSTSRT, &srtentrylist);
    if (iRet < 0) {
        fprintf(stderr, "command 'SIOCLSTSRT' error: %s!\n", lib_strerror(errno));
        close(iSock);
        return;
    }
    
    if (srtentrylist.srtl_total == 0) {
        close(iSock);
        printf("\n");
        return;
    }
    
    srtentrylist.srtl_buf = (struct srtentry *)__SHEAP_ALLOC(sizeof(struct srtentry) * srtentrylist.srtl_total);
    if (!srtentrylist.srtl_buf) {
        fprintf(stderr, "error: %s!\n", lib_strerror(errno));
        close(iSock);
        return;
    }
    
    srtentrylist.srtl_bcnt = srtentrylist.srtl_total;
    iRet = ioctl(iSock, SIOCLSTSRT, &srtentrylist);
    if (iRet < 0) {
        fprintf(stderr, "command 'SIOCLSTSRT' error: %s!\n", lib_strerror(errno));
        close(iSock);
        return;
    }
    close(iSock);
    
    for (i = 0; i < srtentrylist.srtl_num; i++) {
        psrtentry = &srtentrylist.srtl_buf[i];
        inet6_ntoa_r(((struct sockaddr_in6 *)&psrtentry->srt_ssrc)->sin6_addr, cStrSSrc, IP6ADDR_STRLEN_MAX);
        inet6_ntoa_r(((struct sockaddr_in6 *)&psrtentry->srt_esrc)->sin6_addr, cStrESrc, IP6ADDR_STRLEN_MAX);
        inet6_ntoa_r(((struct sockaddr_in6 *)&psrtentry->srt_sdest)->sin6_addr, cStrSDest, IP6ADDR_STRLEN_MAX);
        inet6_ntoa_r(((struct sockaddr_in6 *)&psrtentry->srt_edest)->sin6_addr, cStrEDest, IP6ADDR_STRLEN_MAX);

        cFlags[0] = '\0';
        if (psrtentry->srt_flags & RTF_UP) {
            lib_strcat(cFlags, "U");
        }
        if (psrtentry->srt_flags & RTF_DYNAMIC) {
            lib_strcat(cFlags, "D");
        }
        
        printf("%-32s %-32s %-5s %-32s %-32s %-4s %-4s %s\n",
               cStrSSrc, cStrESrc, cFlags, cStrSDest, cStrEDest,
               (psrtentry->srt_mode == SRT_MODE_EXCLUDE) ? "EXC" : "INC",
               (psrtentry->srt_prio == SRT_PRIO_HIGH) ? "HIGH" : "DEF",
               psrtentry->srt_ifname);
    }
    
    __SHEAP_FREE(srtentrylist.srtl_buf);
    printf("\n");
}

#endif                                                                  /*  LWIP_IPV6                   */
/*********************************************************************************************************
** ��������: __sroute_add
** ��������: shell �������һ��Դ·��
** �䡡��  : iArgC         ��������
**           ppcArgV       ������
** �䡡��  : ERROR or OK
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static INT  __sroute_add (INT  iArgC, PCHAR  *ppcArgV)
{
    INT              iSock, iRet;
    struct srtentry  srtentry;
    
    lib_bzero(&srtentry, sizeof(struct srtentry));
    
    srtentry.srt_flags |= RTF_UP;
    
    srtentry.srt_ssrc.sa_len    = sizeof(struct sockaddr_in);
    srtentry.srt_ssrc.sa_family = AF_INET;
    
    srtentry.srt_esrc.sa_len    = sizeof(struct sockaddr_in);
    srtentry.srt_esrc.sa_family = AF_INET;
    
    srtentry.srt_sdest.sa_len    = sizeof(struct sockaddr_in);
    srtentry.srt_sdest.sa_family = AF_INET;

    srtentry.srt_edest.sa_len    = sizeof(struct sockaddr_in);
    srtentry.srt_edest.sa_family = AF_INET;

    if ((iArgC < 10) || lib_strcmp(ppcArgV[8], "dev")) {
__arg_error:
        fprintf(stderr, "arguments error!\n");
        return  (-ERROR_TSHELL_EPARAM);
    }
    
    if (!inet_aton(ppcArgV[2], &((struct sockaddr_in *)&srtentry.srt_ssrc)->sin_addr)) {
        goto    __arg_error;
    }
    
    if (!inet_aton(ppcArgV[3], &((struct sockaddr_in *)&srtentry.srt_esrc)->sin_addr)) {
        goto    __arg_error;
    }
    
    if (!inet_aton(ppcArgV[4], &((struct sockaddr_in *)&srtentry.srt_sdest)->sin_addr)) {
        goto    __arg_error;
    }

    if (!inet_aton(ppcArgV[5], &((struct sockaddr_in *)&srtentry.srt_edest)->sin_addr)) {
        goto    __arg_error;
    }

    if (!lib_strcasecmp(ppcArgV[6], "INC")) {
        srtentry.srt_mode = SRT_MODE_INCLUDE;

    } else if (!lib_strcasecmp(ppcArgV[6], "EXC")) {
        srtentry.srt_mode = SRT_MODE_EXCLUDE;

    } else {
        fprintf(stderr, "mode must be INC or EXC!\n");
        goto    __arg_error;
    }

    if (!lib_strcasecmp(ppcArgV[7], "HIGH")) {
        srtentry.srt_prio = SRT_PRIO_HIGH;

    } else if (!lib_strcasecmp(ppcArgV[7], "DEF")) {
        srtentry.srt_prio = SRT_PRIO_DEFAULT;

    } else {
        fprintf(stderr, "prio must be HIGH or DEF!\n");
        goto    __arg_error;
    }

    lib_strlcpy(srtentry.srt_ifname, ppcArgV[9], IF_NAMESIZE);
    
    iSock = socket(AF_INET, SOCK_DGRAM, 0);
    if (iSock < 0) {
        fprintf(stderr, "can not create socket error: %s!\n", lib_strerror(errno));
        return  (PX_ERROR);
    }
    
    iRet = ioctl(iSock, SIOCADDSRT, &srtentry);
    if (iRet < 0) {
        fprintf(stderr, "command 'SIOCADDSRT' error: %s!\n", lib_strerror(errno));
        close(iSock);
        return  (PX_ERROR);
    }
    close(iSock);
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: __sroute_delete
** ��������: shell ����ɾ��һ��Դ·��
** �䡡��  : iArgC         ��������
**           ppcArgV       ������
** �䡡��  : ERROR or OK
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static INT  __sroute_delete (INT  iArgC, PCHAR  *ppcArgV)
{
    INT              iSock, iRet;
    struct srtentry  srtentry;
    
    lib_bzero(&srtentry, sizeof(struct srtentry));
    
    srtentry.srt_flags |= RTF_UP;
    
    srtentry.srt_ssrc.sa_len    = sizeof(struct sockaddr_in);
    srtentry.srt_ssrc.sa_family = AF_INET;
    
    srtentry.srt_esrc.sa_len    = sizeof(struct sockaddr_in);
    srtentry.srt_esrc.sa_family = AF_INET;
    
    srtentry.srt_sdest.sa_len    = sizeof(struct sockaddr_in);
    srtentry.srt_sdest.sa_family = AF_INET;

    srtentry.srt_edest.sa_len    = sizeof(struct sockaddr_in);
    srtentry.srt_edest.sa_family = AF_INET;

    if (iArgC < 6) {
__arg_error:
        fprintf(stderr, "arguments error!\n");
        return  (-ERROR_TSHELL_EPARAM);
    }
    
    if (!inet_aton(ppcArgV[2], &((struct sockaddr_in *)&srtentry.srt_ssrc)->sin_addr)) {
        goto    __arg_error;
    }
    
    if (!inet_aton(ppcArgV[3], &((struct sockaddr_in *)&srtentry.srt_esrc)->sin_addr)) {
        goto    __arg_error;
    }
    
    if (!inet_aton(ppcArgV[4], &((struct sockaddr_in *)&srtentry.srt_sdest)->sin_addr)) {
        goto    __arg_error;
    }

    if (!inet_aton(ppcArgV[5], &((struct sockaddr_in *)&srtentry.srt_edest)->sin_addr)) {
        goto    __arg_error;
    }

    iSock = socket(AF_INET, SOCK_DGRAM, 0);
    if (iSock < 0) {
        fprintf(stderr, "can not create socket error: %s!\n", lib_strerror(errno));
        return  (PX_ERROR);
    }
    
    iRet = ioctl(iSock, SIOCDELSRT, &srtentry);
    if (iRet < 0) {
        fprintf(stderr, "command 'SIOCDELSRT' error: %s!\n", lib_strerror(errno));
        close(iSock);
        return  (PX_ERROR);
    }
    close(iSock);
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: __sroute_change
** ��������: shell �����޸�һ��Դ·��
** �䡡��  : iArgC         ��������
**           ppcArgV       ������
** �䡡��  : ERROR or OK
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static INT  __sroute_change (INT  iArgC, PCHAR  *ppcArgV)
{
    INT              iSock, iRet;
    struct srtentry  srtentry;
    
    lib_bzero(&srtentry, sizeof(struct srtentry));
    
    srtentry.srt_flags |= RTF_UP;
    
    srtentry.srt_ssrc.sa_len    = sizeof(struct sockaddr_in);
    srtentry.srt_ssrc.sa_family = AF_INET;
    
    srtentry.srt_esrc.sa_len    = sizeof(struct sockaddr_in);
    srtentry.srt_esrc.sa_family = AF_INET;
    
    srtentry.srt_sdest.sa_len    = sizeof(struct sockaddr_in);
    srtentry.srt_sdest.sa_family = AF_INET;

    srtentry.srt_edest.sa_len    = sizeof(struct sockaddr_in);
    srtentry.srt_edest.sa_family = AF_INET;

    if ((iArgC < 10) || lib_strcmp(ppcArgV[8], "dev")) {
__arg_error:
        fprintf(stderr, "arguments error!\n");
        return  (-ERROR_TSHELL_EPARAM);
    }
    
    if (!inet_aton(ppcArgV[2], &((struct sockaddr_in *)&srtentry.srt_ssrc)->sin_addr)) {
        goto    __arg_error;
    }
    
    if (!inet_aton(ppcArgV[3], &((struct sockaddr_in *)&srtentry.srt_esrc)->sin_addr)) {
        goto    __arg_error;
    }
    
    if (!inet_aton(ppcArgV[4], &((struct sockaddr_in *)&srtentry.srt_sdest)->sin_addr)) {
        goto    __arg_error;
    }

    if (!inet_aton(ppcArgV[5], &((struct sockaddr_in *)&srtentry.srt_edest)->sin_addr)) {
        goto    __arg_error;
    }

    if (!lib_strcasecmp(ppcArgV[6], "INC")) {
        srtentry.srt_mode = SRT_MODE_INCLUDE;

    } else if (!lib_strcasecmp(ppcArgV[6], "EXC")) {
        srtentry.srt_mode = SRT_MODE_EXCLUDE;

    } else {
        fprintf(stderr, "mode must be INC or EXC!\n");
        goto    __arg_error;
    }

    if (!lib_strcasecmp(ppcArgV[7], "HIGH")) {
        srtentry.srt_prio = SRT_PRIO_HIGH;

    } else if (!lib_strcasecmp(ppcArgV[7], "DEF")) {
        srtentry.srt_prio = SRT_PRIO_DEFAULT;

    } else {
        fprintf(stderr, "prio must be HIGH or DEF!\n");
        goto    __arg_error;
    }

    lib_strlcpy(srtentry.srt_ifname, ppcArgV[9], IF_NAMESIZE);
    
    iSock = socket(AF_INET, SOCK_DGRAM, 0);
    if (iSock < 0) {
        fprintf(stderr, "can not create socket error: %s!\n", lib_strerror(errno));
        return  (PX_ERROR);
    }
    
    iRet = ioctl(iSock, SIOCCHGSRT, &srtentry);
    if (iRet < 0) {
        fprintf(stderr, "command 'SIOCCHGSRT' error: %s!\n", lib_strerror(errno));
        close(iSock);
        return  (PX_ERROR);
    }
    close(iSock);
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: __tshellSroute
** ��������: ϵͳ���� "sroute"
** �䡡��  : iArgC         ��������
**           ppcArgV       ������
** �䡡��  : ERROR
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static INT  __tshellSroute (INT  iArgC, PCHAR  *ppcArgV)
{
    INT  iRet;

    if (iArgC == 1) {
        __sroute_show_ipv4();
#if LWIP_IPV6
        __sroute_show_ipv6();
#endif                                                                  /*  LWIP_IPV6                   */
        return  (ERROR_NONE);
    
    } else if (iArgC > 1) {
        if (!lib_strcmp(ppcArgV[1], "add")) {
            iRet = __sroute_add(iArgC, ppcArgV);
            
        } else if (!lib_strcmp(ppcArgV[1], "del") || !lib_strcmp(ppcArgV[1], "delete")) {
            iRet = __sroute_delete(iArgC, ppcArgV);
            
        } else if (!lib_strcmp(ppcArgV[1], "chg") || !lib_strcmp(ppcArgV[1], "change")) {
            iRet = __sroute_change(iArgC, ppcArgV);
            
        } else {
            goto    __arg_error;
        }
        
    } else {
__arg_error:
        fprintf(stderr, "arguments error!\n");
        return  (-ERROR_TSHELL_EPARAM);
    }
    
    return  (iRet);
}
/*********************************************************************************************************
** ��������: __tshellSrouteInit
** ��������: ע��Դ·��������
** �䡡��  : NONE
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
VOID __tshellSrouteInit (VOID)
{
    API_TShellKeywordAdd("sroute", __tshellSroute);
    API_TShellFormatAdd("sroute", " [add | del | chg] [start src] [end src] [start dest] [end dest] [INC/EXC] [HIGH/DEF] dev [dev]");
    API_TShellHelpAdd("sroute",   "show, add, delete, change source route table\n"
    "eg. sroute\n"
    "    sroute add 192.168.1.1 192.168.1.10 123.0.0.1 126.0.0.1 INC DEF dev en1\n"
    "       add source ip from 192.168.1.1 ~ 192.168.1.10 dest 123.0.0.1 ~ 126.0.0.1 route to en1 as default priority.\n\n"
    "    sroute chg 192.168.1.1 192.168.1.10 0.0.0.0 0.0.0.0 EXC HIGH dev en2\n"
    "       change source ip from 192.168.1.1 ~ 192.168.1.10 route to en2 as high priority.\n\n"
    "    sroute del 192.168.1.1 192.168.1.10 123.0.0.1 126.0.0.1\n"
    "       delete source ip from 192.168.1.1 ~ 192.168.1.10 dest 123.0.0.1 ~ 126.0.0.1 route\n\n");
}

#endif                                                                  /*  LW_CFG_NET_BALANCING > 0    */
#endif                                                                  /*  LW_CFG_SHELL_EN > 0         */
/*********************************************************************************************************
** ��������: route_add
** ��������: ����һ��·����Ϣ
** �䡡��  : prtentry      ·����Ŀ
** �䡡��  : ERROR or OK
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API
int  route_add (const struct rtentry *prtentry)
{
    INT   iSock, iRet, iErrNo;
    
    if (!prtentry) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    
    iSock = socket(prtentry->rt_dst.sa_family, SOCK_DGRAM, 0);
    if (iSock < 0) {
        return  (PX_ERROR);
    }
    
    iRet = ioctl(iSock, SIOCADDRT, prtentry);
    if (iRet < 0) {
        iErrNo = errno;
        close(iSock);
        errno = iErrNo;
    
    } else {
        close(iSock);
    }
    
    return  (iRet);
}
/*********************************************************************************************************
** ��������: route_delete
** ��������: ɾ��һ��·����Ϣ
** �䡡��  : prtentry      ·����Ŀ
** �䡡��  : ERROR or OK
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API
int  route_delete (const struct rtentry *prtentry)
{
    INT   iSock, iRet, iErrNo;
    
    if (!prtentry) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    
    iSock = socket(prtentry->rt_dst.sa_family, SOCK_DGRAM, 0);
    if (iSock < 0) {
        return  (PX_ERROR);
    }
    
    iRet = ioctl(iSock, SIOCDELRT, prtentry);
    if (iRet < 0) {
        iErrNo = errno;
        close(iSock);
        errno = iErrNo;
    
    } else {
        close(iSock);
    }
    
    return  (iRet);
}
/*********************************************************************************************************
** ��������: route_change
** ��������: �޸�һ��·����Ϣ
** �䡡��  : pinaddr       ·�ɵ�ַ
**           pinaddrGw     ���ص�ַ
**           type          HOST / NET
**           ifname        Ŀ������
** �䡡��  : ERROR or OK
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API
int  route_change (const struct rtentry *prtentry)
{
    INT   iSock, iRet, iErrNo;
    
    if (!prtentry) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    
    iSock = socket(prtentry->rt_dst.sa_family, SOCK_DGRAM, 0);
    if (iSock < 0) {
        return  (PX_ERROR);
    }
    
    iRet = ioctl(iSock, SIOCCHGRT, prtentry);
    if (iRet < 0) {
        iErrNo = errno;
        close(iSock);
        errno = iErrNo;
    
    } else {
        close(iSock);
    }
    
    return  (iRet);
}
/*********************************************************************************************************
** ��������: route_get
** ��������: ��ȡָ��·�ɱ���Ŀ
** �䡡��  : prtentry      ��ȡ��·����Ϣ
** �䡡��  : ERROR or OK
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API
int  route_get (struct rtentry *prtentry)
{
    INT   iSock, iRet, iErrNo;
    
    if (!prtentry) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    
    iSock = socket(prtentry->rt_dst.sa_family, SOCK_DGRAM, 0);
    if (iSock < 0) {
        return  (PX_ERROR);
    }
    
    iRet = ioctl(iSock, SIOCGETRT, prtentry);
    if (iRet < 0) {
        iErrNo = errno;
        close(iSock);
        errno = iErrNo;
    
    } else {
        close(iSock);
    }
    
    return  (iRet);
}
/*********************************************************************************************************
** ��������: route_list
** ��������: ��ȡ·�ɱ���Ϣ
** �䡡��  : iFamily        AF_INET / AF_INET6
**           prtentrylist   ·���б���
** �䡡��  : ERROR or OK
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API
int  route_list (INT  iFamily, struct rtentry_list *prtentrylist)
{
    INT   iSock, iRet, iErrNo;
    
    if (!prtentrylist) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    
    iSock = socket(iFamily, SOCK_DGRAM, 0);
    if (iSock < 0) {
        return  (PX_ERROR);
    }
    
    iRet = ioctl(iSock, SIOCLSTRT, prtentrylist);
    if (iRet < 0) {
        iErrNo = errno;
        close(iSock);
        errno = iErrNo;
    
    } else {
        close(iSock);
    }
    
    return  (iRet);
}

#endif                                                                  /*  LW_CFG_NET_EN               */
                                                                        /*  LW_CFG_NET_ROUTER           */
/*********************************************************************************************************
  END
*********************************************************************************************************/
