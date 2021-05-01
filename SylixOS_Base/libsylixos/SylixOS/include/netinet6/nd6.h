/*********************************************************************************************************
**
**                                    中国软件开源组织
**
**                                   嵌入式实时操作系统
**
**                                SylixOS(TM)  LW : long wing
**
**                               Copyright All Rights Reserved
**
**--------------文件信息--------------------------------------------------------------------------------
**
** 文   件   名: nd6.h
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2019 年 01 月 03 日
**
** 描        述: include/netinet6/nd6.
*********************************************************************************************************/

#ifndef __NETINET6_ND6_H
#define __NETINET6_ND6_H

#include <net/if.h>
#include <net/route.h>

/*********************************************************************************************************
  See net/route.h
*********************************************************************************************************/

#ifndef RTF_ANNOUNCE
#define RTF_ANNOUNCE    RTF_PROTO2
#endif

#define ND6_LLINFO_NOSTATE      0
#define ND6_LLINFO_INCOMPLETE   1
#define ND6_LLINFO_REACHABLE    2
#define ND6_LLINFO_STALE        3
#define ND6_LLINFO_DELAY        4
#define ND6_LLINFO_PROBE        5

struct nd_ifinfo {
    u_int32_t   linkmtu;                                /* LinkMTU                                      */
    u_int32_t   maxmtu;                                 /* Upper bound of LinkMTU                       */
    u_int32_t   basereachable;                          /* BaseReachableTime                            */
    u_int32_t   reachable;                              /* Reachable Time                               */
    u_int32_t   retrans;                                /* Retrans Timer                                */
    u_int32_t   flags;                                  /* Flags                                        */
    int         recalctm;                               /* BaseReacable re-calculation timer            */
    u_int8_t    chlim;                                  /* CurHopLimit                                  */
    u_int8_t    initialized;                            /* Flag to see the entry is initialized         */
                                                        /* the following 3 members are for privacy      */
                                                        /* extension for addrconf                       */
    u_int8_t    randomseed0[8];                         /* upper 64 bits of MD5 digest                  */
    u_int8_t    randomseed1[8];                         /* lower 64 bits (usually the EUI64 IFID)       */
    u_int8_t    randomid[8];                            /* current random ID                            */
};

#define ND6_IFF_PERFORMNUD          0x1
#define ND6_IFF_ACCEPT_RTADV        0x2
#define ND6_IFF_PREFER_SOURCE       0x4                 /* Not used in FreeBSD.                         */
#define ND6_IFF_IFDISABLED          0x8                 /* IPv6 operation is disabled due to            */
                                                        /* DAD failure.  (XXX: not ND-specific)         */
#define ND6_IFF_DONT_SET_IFROUTE    0x10
#define ND6_IFF_AUTO_LINKLOCAL      0x20
#define ND6_IFF_NO_RADR             0x40
#define ND6_IFF_NO_PREFER_IFACE     0x80                /* XXX: not related to ND.                      */
#define ND6_IFF_NO_DAD              0x100

struct in6_nbrinfo {
    char            ifname[IFNAMSIZ];                   /* if name, e.g. "en1"                          */
    struct in6_addr addr;                               /* IPv6 address of the neighbor                 */
    long            asked;                              /* number of queries already sent for this addr */
    int             isrouter;                           /* if it acts as a router                       */
    int             state;                              /* reachability state                           */
    int             expire;                             /* lifetime for NDP state transition            */
};

#define DRLSTSIZ    10
#define PRLSTSIZ    10

struct in6_drlist {
    char                ifname[IFNAMSIZ];
    struct {
        struct in6_addr rtaddr;
        u_char          flags;
        u_short         rtlifetime;
        u_long          expire;
        u_short         if_index;
    } defrouter[DRLSTSIZ];
};

struct in6_defrouter {
    struct sockaddr_in6 rtaddr;
    u_char              flags;
    u_short             rtlifetime;
    u_long              expire;
    u_short             if_index;
};

struct in6_prlist {
    char                ifname[IFNAMSIZ];
    struct {
        struct in6_addr prefix;
        struct prf_ra   raflags;
        u_char          prefixlen;
        u_char          origin;
        u_int32_t       vltime;
        u_int32_t       pltime;
        time_t          expire;
        u_short         if_index;
        u_short         advrtrs;                        /* number of advertisement routers              */
        struct in6_addr advrtr[DRLSTSIZ];               /* XXX: explicit limit                          */
    } prefix[PRLSTSIZ];
};

struct in6_prefix {
    struct sockaddr_in6 prefix;
    struct prf_ra       raflags;
    u_char              prefixlen;
    u_char              origin;
    u_int32_t           vltime;
    u_int32_t           pltime;
    time_t              expire;
    u_int32_t           flags;
    int                 refcnt;
    u_short             if_index;
    u_short             advrtrs;                        /* number of advertisement routers              */
    /*
     * struct sockaddr_in6 advrtr[]
     */
};

struct in6_ndireq {
    char                ifname[IFNAMSIZ];
    struct nd_ifinfo    ndi;
};

struct in6_ndifreq {
    char    ifname[IFNAMSIZ];
    u_long  ifindex;
};

/*********************************************************************************************************
  Prefix status
*********************************************************************************************************/

#define NDPRF_ONLINK        0x1
#define NDPRF_DETACHED      0x2

/*********************************************************************************************************
  protocol constants
*********************************************************************************************************/

#define MAX_RTR_SOLICITATION_DELAY  1                   /* 1sec                                         */
#define RTR_SOLICITATION_INTERVAL   4                   /* 4sec                                         */
#define MAX_RTR_SOLICITATIONS       3

#define ND6_INFINITE_LIFETIME       0xffffffff

#endif                                                  /* __NETINET6_ND6_H                             */
/*********************************************************************************************************
  END
*********************************************************************************************************/
