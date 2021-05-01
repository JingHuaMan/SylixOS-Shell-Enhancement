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
** ��   ��   ��: if.h
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2011 �� 03 �� 10 ��
**
** ��        ��: posix net/if.h
*********************************************************************************************************/

#ifndef __IF_H
#define __IF_H

#include <sys/types.h>

/*********************************************************************************************************
  �ü�����
*********************************************************************************************************/
#if LW_CFG_NET_EN > 0

#include <sys/socket.h>
#include <sys/ioctl.h>

#include <netinet/in.h>
#include <netinet6/in6.h>

/*********************************************************************************************************
  if_nameindex
*********************************************************************************************************/
#ifdef __cplusplus
extern "C" {
#endif                                                          /*  __cplusplus                         */

#define IF_NAMESIZE     16

struct if_nameindex {
    unsigned    if_index;                                       /* Numeric index of interface           */
    char       *if_name;                                        /* Null-terminated name of the          */
                                                                /* interface.                           */
    char        if_name_buf[IF_NAMESIZE];
};

/*********************************************************************************************************
  posix if api
*********************************************************************************************************/

#if defined(__SYLIXOS_EXTEND) || defined(__SYLIXOS_KERNEL)
LW_API INT                   if_down(const char *ifname);       /* net interface down                   */
LW_API INT                   if_up(const char *ifname);         /* net interface up                     */
LW_API INT                   if_isup(const char *ifname);       /* net interface is up                  */
LW_API INT                   if_islink(const char *ifname);     /* net interface is on link             */
LW_API INT                   if_get_dhcp(const char *ifname);   /* get net if dhcp flag                 */
LW_API INT                   if_set_dhcp(const char *ifname, int en);   
                                                                /* set net if dhcp flag                 */
LW_API INT                   if_set_dhcp6(const char *ifname, int en, int stateless);
LW_API INT                   if_get_dhcp6(const char *ifname);
#endif                                                          /* __SYLIXOS_EXTEND                     */

LW_API unsigned              if_nametoindex(const char *ifname);
LW_API char                 *if_indextoname(unsigned  ifindex, char *ifname);
LW_API struct if_nameindex  *if_nameindex(void);
LW_API void                  if_freenameindex(struct if_nameindex *ptr);

#ifdef __SYLIXOS_KERNEL                                         /* user DO NOT use these API            */
LW_API size_t                if_nameindex_bufsize(void);
LW_API struct if_nameindex  *if_nameindex_rnp(void *buffer, size_t bufsize);
#endif                                                          /* __SYLIXOS_KERNEL                     */

/*********************************************************************************************************
  posix if flags
*********************************************************************************************************/

#define IFF_UP               0x0001                             /* Interface is enable                  */
#define IFF_BROADCAST        0x0002                             /* Interface support broadcast          */
#define IFF_POINTOPOINT      0x0004                             /* Interface is point to point          */
#define IFF_RUNNING          0x0010                             /* Interface is linked                  */
#define IFF_MULTICAST        0x0080                             /* Interface support multicast          */
#define IFF_LOOPBACK         0x0100                             /* Loop back interface                  */
#define IFF_NOARP            0x0200                             /* Do not use ARP protocol              */
#define IFF_PROMISC          0x0400                             /* Receive all packets                  */
#define IFF_ALLMULTI         0x0800                             /* Receive all multicast packets        */

/*********************************************************************************************************
  if priv flags
*********************************************************************************************************/

#define IFF_802_1Q_VLAN      0x0001                             /* Vlan device                          */
#define IFF_EBRIDGE          0x0002                             /* Ethernet bridge device               */
#define IFF_BONDING          0x0004                             /* Bonding device                       */

/*********************************************************************************************************
  posix if structures
*********************************************************************************************************/

#ifndef IFNAMSIZ
#define IFNAMSIZ            IF_NAMESIZE
#endif

struct ifreq {
#define IFHWADDRLEN         6
    union {
        char                ifrn_name[IFNAMSIZ];                /* if name, e.g. "en1"                  */
    } ifr_ifrn;
    union {
        struct sockaddr     ifru_addr;
        struct sockaddr     ifru_dstaddr;
        struct sockaddr     ifru_broadaddr;
        struct sockaddr     ifru_netmask;
        struct sockaddr     ifru_gateway;
        struct sockaddr     ifru_hwaddr;
        short               ifru_flags;
        int                 ifru_ifindex;
        int                 ifru_mtu;
        int                 ifru_metric;
        int                 ifru_type;
        int                 ifru_tcpaf;
        int                 ifru_tcpwnd;
        void               *ifru_data;
    } ifr_ifru;
};

#define ifr_name            ifr_ifrn.ifrn_name
#define ifr_addr            ifr_ifru.ifru_addr
#define ifr_dstaddr         ifr_ifru.ifru_dstaddr
#define ifr_netmask         ifr_ifru.ifru_netmask
#define ifr_gateway         ifr_ifru.ifru_gateway
#define ifr_broadaddr       ifr_ifru.ifru_broadaddr
#define ifr_hwaddr          ifr_ifru.ifru_hwaddr
#define ifr_flags           ifr_ifru.ifru_flags
#define ifr_ifindex         ifr_ifru.ifru_ifindex
#define ifr_mtu             ifr_ifru.ifru_mtu
#define ifr_metric          ifr_ifru.ifru_metric
#define ifr_type            ifr_ifru.ifru_type
#define ifr_tcpaf           ifr_ifru.ifru_tcpaf                 /* 2 ~ 127                              */
#define ifr_tcpwnd          ifr_ifru.ifru_tcpwnd
#define ifr_data            ifr_ifru.ifru_data

struct ifaliasreq {
    char                    ifra_name[IFNAMSIZ];                /* if name, e.g. "en1"                  */
    struct sockaddr         ifra_addr;
    struct sockaddr         ifra_broadaddr;
    struct sockaddr         ifra_mask;
};

struct ifconf {
    int                     ifc_len;                            /* size of buffer in bytes              */
    union {
        char               *ifcu_buf;
        struct ifreq       *ifcu_req;
    } ifc_ifcu;
};

#define ifc_buf             ifc_ifcu.ifcu_buf                   /* buffer address                       */
#define ifc_req             ifc_ifcu.ifcu_req                   /* array of structures                  */

/*********************************************************************************************************
  posix if ioctl
*********************************************************************************************************/

#define SIOCGSIZIFCONF      _IOR('i', 106, int)
#define SIOCGIFCONF         _IOWR('i', 20, struct ifconf)
#define SIOCGIFNUM          _IOR('i',  20, int)

#define SIOCSIFADDR         _IOW('i', 12, struct ifreq)
#define SIOCSIFNETMASK      _IOW('i', 22, struct ifreq)
#define SIOCSIFDGWADDR      _IOW('i', 15, struct ifreq)
#define SIOCSIFDSTADDR      _IOW('i', 14, struct ifreq)
#define SIOCSIFBRDADDR      _IOW('i', 19, struct ifreq)
#define SIOCSIFFLAGS        _IOW('i', 16, struct ifreq)

#define SIOCGIFADDR         _IOWR('i', 33, struct ifreq)
#define SIOCGIFNETMASK      _IOWR('i', 37, struct ifreq)
#define SIOCGIFDGWADDR      _IOWR('i', 16, struct ifreq)
#define SIOCGIFDSTADDR      _IOWR('i', 34, struct ifreq)
#define SIOCGIFBRDADDR      _IOWR('i', 35, struct ifreq)
#define SIOCGIFFLAGS        _IOWR('i', 17, struct ifreq)

#define SIOCGIFMETRIC       _IOWR('i', 23, struct ifreq)
#define SIOCSIFMETRIC       _IOW( 'i', 24, struct ifreq)

#define SIOCDIFADDR         _IOW('i', 25, struct ifreq)
#define SIOCAIFADDR         _IOW('i', 26, struct ifaliasreq)

#define SIOCGIFTYPE         _IOR('i',  49, struct ifreq)
#define SIOCGIFNAME         _IOWR('i', 50, struct ifreq)
#define SIOCGIFINDEX        _IOWR('i', 51, struct ifreq)

#define SIOCGIFMTU          _IOWR('i', 52, struct ifreq)
#define SIOCSIFMTU          _IOW('i',  53, struct ifreq)

#define SIOCGIFHWADDR       _IOWR('i', 54, struct ifreq)
#define SIOCSIFHWADDR       _IOW('i',  55, struct ifreq)

#define SIOCADDMULTI        _IOW('i', 60, struct ifreq)
#define SIOCDELMULTI        _IOW('i', 61, struct ifreq)

#define SIOCGIFTCPAF        _IOWR('i', 62, struct ifreq)
#define SIOCSIFTCPAF        _IOW('i',  63, struct ifreq)

#define SIOCGIFTCPWND       _IOWR('i', 64, struct ifreq)
#define SIOCSIFTCPWND       _IOW('i',  65, struct ifreq)

#define SIOCGIFPFLAGS       _IOWR('i', 66, struct ifreq)
#define SIOCSIFPFLAGS       _IOW('i',  67, struct ifreq)

/*********************************************************************************************************
  sylixos if6 structures
*********************************************************************************************************/

struct in6_ifr_addr {
    struct in6_addr      ifr6a_addr;
    uint32_t             ifr6a_prefixlen;
};

struct in6_ifreq {
    int                  ifr6_ifindex;
    int                  ifr6_len;                              /* size of buffer in bytes              */
    struct in6_ifr_addr *ifr6_addr_array;
};

#define ifr6_addr        ifr6_addr_array->ifr6a_addr
#define ifr6_prefixlen   ifr6_addr_array->ifr6a_prefixlen

/*********************************************************************************************************
  sylixos if6 ioctl 
  
  SIOCSIFADDR6, SIOCSIFNETMASK6, SIOCSIFDSTADDR6, SIOCDIFADDR6
  
  ifr6_len must == sizeof(struct in6_ifr_addr))
*********************************************************************************************************/

#define SIOCGSIZIFREQ6      _IOR('i', 106, struct in6_ifreq)    /* size of buffer in bytes              */

#define SIOCSIFADDR6        _IOW('i', 12, struct in6_ifreq)
#define SIOCSIFNETMASK6     _IOW('i', 22, struct in6_ifreq)
#define SIOCSIFDSTADDR6     _IOW('i', 14, struct in6_ifreq)

#define SIOCGIFADDR6        _IOWR('i', 33, struct in6_ifreq)
#define SIOCGIFNETMASK6     _IOWR('i', 37, struct in6_ifreq)
#define SIOCGIFDSTADDR6     _IOWR('i', 34, struct in6_ifreq)

#define SIOCDIFADDR6        _IOW('i', 35, struct in6_ifreq)

/*********************************************************************************************************
  sylixos if statistics
*********************************************************************************************************/

struct ifstatreq {
    char        ifrs_name[IFNAMSIZ];                            /* if name, e.g. "en1"                  */
    u_long      ifrs_mtu;                                       /* maximum transmission unit            */
    u_long      ifrs_collisions;                                /* collisions on csma interfaces        */
    uint64_t    ifrs_baudrate;                                  /* linespeed                            */
    uint64_t    ifrs_ipackets;                                  /* packets received on interface        */
    uint64_t    ifrs_ierrors;                                   /* input errors on interface            */
    uint64_t    ifrs_opackets;                                  /* packets sent on interface            */
    uint64_t    ifrs_oerrors;                                   /* output errors on interface           */
    uint64_t    ifrs_ibytes;                                    /* total number of octets received      */
    uint64_t    ifrs_obytes;                                    /* total number of octets sent          */
    uint64_t    ifrs_imcasts;                                   /* packets received via multicast       */
    uint64_t    ifrs_omcasts;                                   /* packets sent via multicast           */
    uint64_t    ifrs_iqdrops;                                   /* dropped on input, this interface     */
    uint64_t    ifrs_noproto;                                   /* destined for unsupported protocol    */
    uint64_t    ifrs_reserved[8];
};

/*********************************************************************************************************
  sylixos if statistics ioctl
*********************************************************************************************************/

#define SIOCGIFSTATS        _IOWR('i', 80, struct ifstatreq)

/*********************************************************************************************************
  6lowpan IEEE802.15.4
*********************************************************************************************************/

struct ieee802154_ifreq {
    union {
        char                ifrn_name[IFNAMSIZ];                /* if name, e.g. "LP1"                  */
    } ifr802154_ifrn;
    union {
        uint16_t            ifru_panid;
        uint16_t            ifru_shortaddr;
        uint8_t             ifru_aeskey[16];
        struct {
            uint8_t         ifrc_index;                         /* 0 ~ 15                               */
            struct in6_addr ifrc_ctxaddr;
        } ifru_ctx;
    } ifr802154_ifru;
};

#define ifr802154_name      ifr802154_ifrn.ifrn_name
#define ifr802154_pandid    ifr802154_ifru.ifru_panid
#define ifr802154_shortaddr ifr802154_ifru.ifru_shortaddr
#define ifr802154_aeskey    ifr802154_ifru.ifru_aeskey
#define ifr802154_ctxindex  ifr802154_ifru.ifru_ctx.ifrc_index
#define ifr802154_ctxaddr   ifr802154_ifru.ifru_ctx.ifrc_ctxaddr

#define SIOCG802154PANID    _IOR('i', 200, struct ieee802154_ifreq)
#define SIOCS802154PANID    _IOW('i', 200, struct ieee802154_ifreq)

#define SIOCG802154SHRTADDR _IOR('i', 201, struct ieee802154_ifreq)
#define SIOCS802154SHRTADDR _IOW('i', 201, struct ieee802154_ifreq)

#define SIOCG802154CTX      _IOR('i', 202, struct ieee802154_ifreq)
#define SIOCS802154CTX      _IOW('i', 202, struct ieee802154_ifreq)

#define SIOCG802154AESKEY   _IOR('i', 203, struct ieee802154_ifreq)
#define SIOCS802154AESKEY   _IOW('i', 203, struct ieee802154_ifreq)

/*********************************************************************************************************
  6lowpan RFC7668
*********************************************************************************************************/

struct rfc7668_ifreq {
    union {
        char                ifrn_name[IFNAMSIZ];                /* if name, e.g. "LP1"                  */
    } ifr7668_ifrn;
    union {
        struct in6_addr     ifru_dstaddr;
        uint8_t             ifru_aeskey[16];
        struct {
            uint8_t         ifrc_index;                         /* 0 ~ 15                               */
            struct in6_addr ifrc_ctxaddr;
        } ifru_ctx;
    } ifr7668_ifru;
};

#define ifr7668_name        ifr7668_ifrn.ifrn_name
#define ifr7668_dstaddr     ifr7668_ifru.ifru_dstaddr
#define ifr7668_aeskey      ifr7668_ifru.ifru_aeskey
#define ifr7668_ctxindex    ifr7668_ifru.ifru_ctx.ifrc_index
#define ifr7668_ctxaddr     ifr7668_ifru.ifru_ctx.ifrc_ctxaddr

#define SIOCG7668DSTADDR    _IOR('i', 211, struct rfc7668_ifreq)
#define SIOCS7668DSTADDR    _IOW('i', 211, struct rfc7668_ifreq)

#define SIOCG7668CTX        _IOR('i', 212, struct rfc7668_ifreq)
#define SIOCS7668CTX        _IOW('i', 212, struct rfc7668_ifreq)

#define SIOCG7668AESKEY     _IOR('i', 213, struct rfc7668_ifreq)
#define SIOCS7668AESKEY     _IOW('i', 213, struct rfc7668_ifreq)

/*********************************************************************************************************
  proto private 0x89e0 ~ 0x89ef
*********************************************************************************************************/

#define SIOCPROTOPRIVATE    0x89e0                              /* to 0x89ef                            */

/*********************************************************************************************************
  device private 0x89f0 ~ 0x89ff
*********************************************************************************************************/

#define SIOCDEVPRIVATE      0x89f0                              /* to 0x89ff                            */

/*********************************************************************************************************
  Structure describing information about an interface
  which may be of interest to management entities.
*********************************************************************************************************/

struct if_data {
                                                                /* generic interface information        */
    u_char  ifi_type;                                           /* ethernet, tokenring, etc             */
    u_char  ifi_physical;                                       /* e.g., AUI, Thinnet, 10base-T, etc    */
    u_char  ifi_addrlen;                                        /* media address length                 */
    u_char  ifi_hdrlen;                                         /* media header length                  */
    u_char  ifi_recvquota;                                      /* polling quota for receive intrs      */
    u_char  ifi_xmitquota;                                      /* polling quota for xmit intrs         */
    u_long  ifi_mtu;                                            /* maximum transmission unit            */
    u_long  ifi_metric;                                         /* routing metric (external only)       */
    u_long  ifi_baudrate;                                       /* linespeed                            */
                                                                /* volatile statistics                  */
    u_long  ifi_ipackets;                                       /* packets received on interface        */
    u_long  ifi_ierrors;                                        /* input errors on interface            */
    u_long  ifi_opackets;                                       /* packets sent on interface            */
    u_long  ifi_oerrors;                                        /* output errors on interface           */
    u_long  ifi_collisions;                                     /* collisions on csma interfaces        */
    u_long  ifi_ibytes;                                         /* total number of octets received      */
    u_long  ifi_obytes;                                         /* total number of octets sent          */
    u_long  ifi_imcasts;                                        /* packets received via multicast       */
    u_long  ifi_omcasts;                                        /* packets sent via multicast           */
    u_long  ifi_iqdrops;                                        /* dropped on input, this interface     */
    u_long  ifi_noproto;                                        /* destined for unsupported protocol    */
    u_long  ifi_recvtiming;                                     /* usec spent receiving when timing     */
    u_long  ifi_xmittiming;                                     /* usec spent xmitting when timing      */
    struct  timeval ifi_lastchange;                             /* time of last administrative change   */
};

/*********************************************************************************************************
  Message format for use in obtaining information about interfaces
  from getkerninfo and the routing socket
*********************************************************************************************************/

struct if_msghdr {
    u_short ifm_msglen;                                         /* to skip over non-understood messages */
    u_char  ifm_version;                                        /* future binary compatibility          */
    u_char  ifm_type;                                           /* message type                         */
    
    int     ifm_addrs;                                          /* like rtm_addrs                       */
    int     ifm_flags;                                          /* value of if_flags                    */
    u_short ifm_index;                                          /* index for associated ifp             */
    struct  if_data ifm_data;                                   /* statistics and other data about if   */
};

/*********************************************************************************************************
  Message format for use in obtaining information about interface addresses
  from getkerninfo and the routing socket
*********************************************************************************************************/

struct ifa_msghdr {
    u_short ifam_msglen;                                        /* to skip over non-understood messages */
    u_char  ifam_version;                                       /* future binary compatibility          */
    u_char  ifam_type;                                          /* message type                         */
    
    int     ifam_addrs;                                         /* like rtm_addrs                       */
    int     ifam_flags;                                         /* value of ifa_flags                   */
    u_short ifam_index;                                         /* index for associated ifp             */
    int     ifam_metric;                                        /* value of ifa_metric                  */
};

/*********************************************************************************************************
  RTM_NEWMADDR & RTM_DELMADDR
*********************************************************************************************************/

struct ifma_msghdr {
    u_short ifmam_msglen;                                       /* to skip over non-understood messages */
    u_char  ifmam_version;                                      /* future binary compatibility          */
    u_char  ifmam_type;                                         /* message type                         */

    int     ifmam_addrs;                                        /* like rtm_addrs                       */
    int     ifmam_flags;                                        /* value of ifa_flags                   */
    u_short ifmam_index;                                        /* index for associated ifp             */
};

/*********************************************************************************************************
  RTM_IFANNOUNCE
*********************************************************************************************************/

struct if_announcemsghdr {
    u_short ifan_msglen;                                        /* to skip over non-understood messages */
    u_char  ifan_version;                                       /* future binary compatibility          */
    u_char  ifan_type;                                          /* message type                         */

    u_short ifan_index;                                         /* index for associated ifp             */
    char    ifan_name[IFNAMSIZ];                                /* if name, e.g. "en0"                  */
    u_short ifan_what;                                          /* what type of announcement            */
};

#define IFAN_ARRIVAL        0                                   /* interface arrival                    */
#define IFAN_DEPARTURE      1                                   /* interface departure                  */

#ifdef __cplusplus
}
#endif                                                          /*  __cplusplus                         */

#endif                                                          /*  LW_CFG_NET_EN                       */
#endif                                                          /*  __IF_H                              */
/*********************************************************************************************************
  END
*********************************************************************************************************/
