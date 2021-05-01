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
** ��   ��   ��: if_arp.h
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2014 �� 05 �� 09 ��
**
** ��        ��: ARP.
*********************************************************************************************************/

#ifndef __IF_ARP_H
#define __IF_ARP_H

#include <sys/types.h>
#include <sys/ioctl.h>

/*********************************************************************************************************
  �ü�����
*********************************************************************************************************/
#if LW_CFG_NET_EN > 0

#ifdef __cplusplus
extern "C" {
#endif                                                      /*  __cplusplus                             */

/*********************************************************************************************************
 * Address Resolution Protocol.
 *
 * See RFC 826 for protocol description.  ARP packets are variable
 * in size; the arphdr structure defines the fixed-length portion.
 * Protocol type values are the same as those for 10 Mb/s Ethernet.
 * It is followed by the variable-sized fields ar_sha, arp_spa,
 * arp_tha and arp_tpa in that order, according to the lengths
 * specified.  Field names used correspond to RFC 826.
*********************************************************************************************************/

struct arphdr {
    u_short     ar_hrd;                                     /* format of hardware address               */
#define ARPHRD_ETHER                1                       /* ethernet hardware format                 */
#define ARPHRD_IEEE802              6                       /* token-ring hardware format               */
#define ARPHRD_FRELAY               15                      /* frame relay hardware format              */
#define ARPHRD_IEEE1394             24                      /* firewire hardware format                 */
#define ARPHRD_INFINIBAND           32                      /* infiniband hardware format               */
#define ARPHRD_IEEE802_TR           800                     /* Magic type ident for TR                  */
#define ARPHRD_IEEE80211            801                     /* IEEE 802.11                              */
#define ARPHRD_IEEE80211_PRISM      802                     /* IEEE 802.11 + Prism2 header              */
#define ARPHRD_IEEE80211_RADIOTAP   803                     /* IEEE 802.11 + radiotap header            */
#define ARPHRD_IEEE802154           804                     /* IEEE 802.15.4                            */
#define ARPHRD_VOID                 0xffff                  /* Void type, nothing is known              */
#define ARPHRD_NONE                 0xfffe                  /* zero header length                       */
    u_short     ar_pro;                                     /* format of protocol address               */
    u_char      ar_hln;                                     /* length of hardware address               */
    u_char      ar_pln;                                     /* length of protocol address               */
    u_short     ar_op;                                      /* one of: */
#define ARPOP_REQUEST       1                               /* request to resolve address               */
#define ARPOP_REPLY         2                               /* response to previous request             */
#define ARPOP_REVREQUEST    3                               /* request protocol address given hardware  */
#define ARPOP_REVREPLY      4                               /* response giving protocol address         */
#define ARPOP_INVREQUEST    8                               /* request to identify peer                 */
#define ARPOP_INVREPLY      9                               /* response identifying peer                */

/*********************************************************************************************************
 * The remaining fields are variable in size,
 * according to the sizes above.
*********************************************************************************************************/

#ifdef COMMENT_ONLY
    u_char    ar_sha[];                                     /* sender hardware address                  */
    u_char    ar_spa[];                                     /* sender protocol address                  */
    u_char    ar_tha[];                                     /* target hardware address                  */
    u_char    ar_tpa[];                                     /* target protocol address                  */
#endif
};

/*********************************************************************************************************
 * ARP ioctl request
*********************************************************************************************************/

struct arpreq {
    struct sockaddr    arp_pa;                              /* protocol address                         */
    struct sockaddr    arp_ha;                              /* hardware address                         */
    int                arp_flags;                           /* flags                                    */
    struct sockaddr    arp_netmask;                         /* netmask (only for proxy arps)            */
    char               arp_dev[16];
};

/*********************************************************************************************************
 * arp_flags and at_flags field values 
*********************************************************************************************************/

#define ATF_INUSE           0x01                            /* entry in use                             */
#define ATF_COM             0x02                            /* completed entry (enaddr valid)           */
#define ATF_PERM            0x04                            /* permanent entry                          */
#define ATF_PUBL            0x08                            /* publish entry (respond for other host)   */
#define ATF_USETRAILERS     0x10                            /* has requested trailers                   */

/*********************************************************************************************************
 * arp ioctl command
*********************************************************************************************************/

#define SIOCSARP            _IOW('i', 30, struct arpreq)    /* set arp entry                            */
#define SIOCGARP            _IOWR('i',38, struct arpreq)    /* get arp entry                            */
#define SIOCDARP            _IOW('i', 32, struct arpreq)    /* delete arp entry                         */

/*********************************************************************************************************
 * This structure gets passed by the SIOCLSTARP calls.
*********************************************************************************************************/

struct arpreq_list {
    u_long          arpl_bcnt;                              /* struct arpreq buffer count               */
    u_long          arpl_num;                               /* system return how many entry in buffer   */
    u_long          arpl_total;                             /* system return total number entry         */
    struct arpreq  *arpl_buf;                               /* arp entry list buffer                    */
};

#define SIOCLSTARP          _IOWR('i', 39, struct arpreq_list)

#ifdef __cplusplus
}
#endif                                                      /*  __cplusplus                             */

#endif                                                      /*  LW_CFG_NET_EN                           */
#endif                                                      /*  __IF_ARP_H                              */
/*********************************************************************************************************
  END
*********************************************************************************************************/
