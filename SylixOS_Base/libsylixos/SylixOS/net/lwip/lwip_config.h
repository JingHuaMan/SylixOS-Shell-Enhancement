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
** ��   ��   ��: lwip_config.h
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2009 �� 05 �� 06 ��
**
** ��        ��: lwip ����.

** BUG:
2009.07.02  ������ ETHARP_TRUST_IP_MAC ������. windows ϵͳĬ�ϲ��������� IP �� MAC ��ַ.
2009.07.28  �������ļ������������ָ�Ϊ /socket
2010.02.01  ��̬��ַת�����ʼ�� none-host ��Ӧ 0 ��Ч��ַ.
2010.02.03  MEMP_NUM_NETDB Ĭ��Ϊ 10.
2010.05.27  ���ϵͳ֧�� SSL VPN ����, ����ʹ�� LWIP_TCPIP_CORE_LOCKING ʹ lwip ֧�ְ�ȫ�Ķ��̲߳���.
            (SSL VPN ��������ʹ�ö��߳�ͬʱ����ͬһ�� socket. LWIP_TCPIP_CORE_LOCKING �Դ��ڲ��Խ׶�!)
2011.05.24  TCP_SND_BUF ʼ������Ϊ 64KB - 1, ������ non-blocking ʱ����������ݰ�.
2017.12.06  ����� LW_CFG_LWIP_MEM_TLSF ��֧��.
*********************************************************************************************************/

#ifndef __LWIP_CONFIG_H
#define __LWIP_CONFIG_H

#include "SylixOS.h"
#include "string.h"                                                     /*  memcpy                      */

/*********************************************************************************************************
  prof config
*********************************************************************************************************/

#include "../SylixOS/config/net/net_perf_cfg.h"

#ifdef __cplusplus
extern "C" {
#endif                                                                  /*  __cplusplus                 */

/*********************************************************************************************************
  Platform specific locking
*********************************************************************************************************/

#define SYS_LIGHTWEIGHT_PROT                    1
#define LWIP_DONT_PROVIDE_BYTEORDER_FUNCTIONS   1

/*********************************************************************************************************
  Thread stack safe
*********************************************************************************************************/

#if LW_CFG_NET_SAFE > 1
#define LWIP_MPU_COMPATIBLE                     1
#else
#define LWIP_MPU_COMPATIBLE                     0
#endif

/*********************************************************************************************************
  Platform memcpy smemcpy
*********************************************************************************************************/

extern PVOID  lwip_platform_memcpy(PVOID  pvDest, CPVOID  pvSrc, size_t  stCount);
extern PVOID  lwip_platform_smemcpy(PVOID  pvDest, CPVOID  pvSrc, size_t  stCount);

#define MEMCPY      lwip_platform_memcpy
#define SMEMCPY     lwip_platform_smemcpy

/*********************************************************************************************************
  pbuf ref type 
  For compatibility, choose 'u16_t'
  For speed, choose 'int' to use atomic operate.
*********************************************************************************************************/

#define LWIP_PBUF_REF_T         int

/*********************************************************************************************************
  pbuf custom data
  For IP_HT_LOCAL_OUT hook only, the hook function can change this variable to
  change the output netif, NULL means use system route table (if_out -> struct netif point type)
*********************************************************************************************************/

#define LWIP_PBUF_CUSTOM_DATA   void *if_out;

/*********************************************************************************************************
  Memory options
*********************************************************************************************************/

#if LW_CFG_LWIP_POOL_SIZE < 256
#error "LW_CFG_LWIP_POOL_SIZE must bigger than 256!"
#endif

#if LW_CFG_NET_DEV_MAX > 256
#error "LW_CFG_NET_DEV_MAX must less than 256!"
#endif

#if LW_CFG_NET_FLOWCTL_EN > 0
#define MEM_SIZE                        (LW_CFG_NET_FLOWCTL_MEM_SIZE + LW_CFG_LWIP_MEM_SIZE)
#else
#define MEM_SIZE                        LW_CFG_LWIP_MEM_SIZE            /*  mem_malloc �Ѵ�С           */
#endif                                                                  /*  LW_CFG_NET_FLOWCTL_EN > 0   */

#define MEM_ALIGNMENT                   (LW_CFG_CPU_WORD_LENGHT / NBBY) /*  �ڴ�������                */
#define MEMP_NUM_PBUF                   LW_CFG_LWIP_NUM_PBUFS           /*  npbufs                      */
#define PBUF_POOL_SIZE                  LW_CFG_LWIP_NUM_POOLS           /*  pool num                    */
#define PBUF_POOL_BUFSIZE               LW_CFG_LWIP_POOL_SIZE           /*  pool block size             */

#if MEM_SIZE >= (1 * LW_CFG_MB_SIZE)
#define MEMP_NUM_REASSDATA              512                             /*  ͬʱ��������� IP ���ݰ�    */
#elif MEM_SIZE >= (512 * LW_CFG_KB_SIZE)
#define MEMP_NUM_REASSDATA              256
#elif MEM_SIZE >= (256 * LW_CFG_KB_SIZE)
#define MEMP_NUM_REASSDATA              128
#elif MEM_SIZE >= (128 * LW_CFG_KB_SIZE)
#define MEMP_NUM_REASSDATA              64
#else
#define MEMP_NUM_REASSDATA              16
#endif                                                                  /*  MEM_SIZE >= ...             */

#if LW_CFG_LWIP_TX_SINGLE_PBUF > 0
#define MEMP_NUM_FRAG_PBUF              16
#else
#define MEMP_NUM_FRAG_PBUF              MEMP_NUM_REASSDATA              /*  !TX_SINGLE_PBUF ��Ƭ����    */
#endif

/*********************************************************************************************************
  Memory method
*********************************************************************************************************/

#if LW_CFG_LWIP_MEM_TLSF > 0
#define MEM_LIBC_MALLOC                 1
#define MEM_STATS                       1
#endif                                                                  /*  LW_CFG_LWIP_MEM_TLSF        */

/*********************************************************************************************************
  ...PCB
*********************************************************************************************************/

#define MEMP_NUM_RAW_PCB                LW_CFG_LWIP_RAW_PCB
#define MEMP_NUM_UDP_PCB                LW_CFG_LWIP_UDP_PCB
#define MEMP_NUM_TCP_PCB                LW_CFG_LWIP_TCP_PCB
#define MEMP_NUM_TCP_PCB_LISTEN         LW_CFG_LWIP_TCP_PCB

#if LW_CFG_LWIP_TCP_PCB + LW_CFG_LWIP_UDP_PCB + LW_CFG_LWIP_RAW_PCB
#define MEMP_NUM_NETCONN                ((2 * LW_CFG_LWIP_TCP_PCB) + LW_CFG_LWIP_UDP_PCB + LW_CFG_LWIP_RAW_PCB)
#else
#define MEMP_NUM_NETCONN                1
#endif

#define MEMP_NUM_TCPIP_MSG_API          MEMP_NUM_NETCONN
#define MEMP_NUM_TCPIP_MSG_INPKT        LW_CFG_LWIP_NUM_POOLS           /*  tcp input msgqueue use      */

/*********************************************************************************************************
  PBUF
*********************************************************************************************************/

#define LWIP_NETIF_TX_SINGLE_PBUF       LW_CFG_LWIP_TX_SINGLE_PBUF
#define LWIP_SUPPORT_CUSTOM_PBUF        1

/*********************************************************************************************************
  check sum
*********************************************************************************************************/

#define LWIP_CHECKSUM_CTRL_PER_NETIF    1

#define CHECKSUM_GEN_IP                 1
#define CHECKSUM_GEN_UDP                1
#define CHECKSUM_GEN_TCP                1
#define CHECKSUM_GEN_ICMP               1
#define CHECKSUM_GEN_ICMP6              1

#define CHECKSUM_CHECK_IP               1
#define CHECKSUM_CHECK_UDP              1
#define CHECKSUM_CHECK_TCP              1
#define CHECKSUM_CHECK_ICMP             1
#define CHECKSUM_CHECK_ICMP6            1

#define LWIP_CHECKSUM_ON_COPY           LW_CFG_LWIP_CHECKSUM_ON_COPY    /*  �������ݰ�ͬʱ���� chksum   */

/*********************************************************************************************************
  sylixos do not use MEMP_NUM_NETCONN, because sylixos use another socket interface.
*********************************************************************************************************/

#define IP_FORWARD                      LW_CFG_NET_ROUTER               /*  �Ƿ����� IP ת��            */
#define IP_REASSEMBLY                   LW_CFG_LWIP_IPFRAG
#define IP_FRAG                         LW_CFG_LWIP_IPFRAG
#define IP_REASS_MAX_PBUFS              MEMP_NUM_REASSDATA
#define IP_REASS_MAXAGE                 LW_CFG_LWIP_IP_REASS_MAXAGE

#define IP_SOF_BROADCAST                1                               /*  Use the SOF_BROADCAST       */
#define IP_SOF_BROADCAST_RECV           1

#if LW_CFG_NET_ADHOC_ROUTER > 0
#define IP_FORWARD_ALLOW_TX_ON_RX_NETIF 1
#else
#define IP_FORWARD_ALLOW_TX_ON_RX_NETIF 0
#endif                                                                  /*  LW_CFG_NET_ADHOC_ROUTER > 0 */

/*********************************************************************************************************
  IPv6
*********************************************************************************************************/

#define LWIP_IPV6                       LW_CFG_NET_IPV6
#define LWIP_IPV6_MLD                   1
#define LWIP_IPV6_MLD_V2                1
#define LWIP_IPV6_FORWARD               LW_CFG_NET_ROUTER
#define LWIP_ICMP6                      1
#define LWIP_IPV6_FRAG                  LW_CFG_LWIP_IPFRAG
#define LWIP_IPV6_REASS                 LW_CFG_LWIP_IPFRAG
#define LWIP_IPV6_SCOPES                1

#define MEMP_NUM_MLD6_GROUP             LW_CFG_LWIP_GROUP_MAX

#if LW_CFG_LWIP_ARP_TABLE_SIZE > 127
#define LWIP_ND6_NUM_NEIGHBORS          127                             /*  TODO: Max 127 Now!          */
#define LWIP_ND6_NUM_DESTINATIONS       127
#else
#define LWIP_ND6_NUM_NEIGHBORS          LW_CFG_LWIP_ARP_TABLE_SIZE
#define LWIP_ND6_NUM_DESTINATIONS       LW_CFG_LWIP_ARP_TABLE_SIZE
#endif                                                                  /*  ARP_TABLE_SIZE > 127        */

#define LWIP_IPV6_NUM_ADDRESSES         5                               /*  one face max 5 ipv6 addr    */

#if LW_CFG_CPU_WORD_LENGHT > 32
#define IPV6_FRAG_COPYHEADER            1
#endif                                                                  /*  LW_CFG_CPU_WORD_LENGHT > 32 */

/*********************************************************************************************************
  timeouts (default + 10, aodv, lowpan ...)
*********************************************************************************************************/

#define MEMP_NUM_SYS_TIMEOUT            (LWIP_NUM_SYS_TIMEOUT_INTERNAL + 11 + LW_CFG_NET_FLOWCTL_EN + \
                                         (LW_CFG_NET_MROUTER * 2))

#define MEMP_NUM_NETBUF                 LW_CFG_LWIP_NUM_NETBUF

/*********************************************************************************************************
  ICMP
*********************************************************************************************************/

#define LWIP_ICMP                       1

/*********************************************************************************************************
  RAW
*********************************************************************************************************/

#if LW_CFG_LWIP_RAW_PCB > 0
#define LWIP_RAW                        1
#else
#define LWIP_RAW                        0
#endif

/*********************************************************************************************************
  SNMP
*********************************************************************************************************/

#if LW_CFG_NET_SNMP_V1_EN || LW_CFG_NET_SNMP_V2_EN || LW_CFG_NET_SNMP_V3_EN
#define LWIP_SNMP                       1

#define SNMP_USE_RAW                    0
#define SNMP_USE_NETCONN                1

#define SNMP_LWIP_MIB2_SYSDESC          "lwIP"
#define SNMP_LWIP_MIB2_SYSNAME          "SylixOS"
#define SNMP_LWIP_MIB2_SYSCONTACT       "acoinfo@acoinfo.com"
#define SNMP_LWIP_MIB2_SYSLOCATION      "@universe"

#if LW_CFG_SHELL_PASS_CRYPT_EN > 0 && LW_CFG_NET_SNMP_V3_EN > 0
#define LWIP_SNMP_V3                    1
#define LWIP_SNMP_V3_CRYPTO             1
#define LWIP_SNMP_V3_MBEDTLS            1
#endif                                                                  /*  LW_CFG_SHELL_PASS_CRYPT_EN  */
                                                                        /*  LW_CFG_NET_SNMP_V3_EN       */
#define LWIP_SNMP_CONFIGURE_VERSIONS    1

#else
#define LWIP_SNMP                       0
#endif                                                                  /*  LW_CFG_NET_SNMP_V123_EN     */

#define MIB2_STATS                      1

/*********************************************************************************************************
  IGMP
*********************************************************************************************************/

#define LWIP_IGMP                       1
#define LWIP_IGMP_V3                    1
#define MEMP_NUM_IGMP_GROUP             (LW_CFG_LWIP_GROUP_MAX + LW_CFG_NET_DEV_MAX)

/*********************************************************************************************************
  multicast router.
*********************************************************************************************************/

#if !IP_FORWARD || !LWIP_IGMP
#undef LW_CFG_NET_MROUTER
#define LW_CFG_NET_MROUTER              0
#else
#define LWIP_MULTICAST_TX_OPTIONS       1
#endif

/*********************************************************************************************************
  TCP UDP ����˿ڷ�Χ
*********************************************************************************************************/

#if (LW_CFG_NET_ROUTER > 0) && (LW_CFG_NET_NAT_EN > 0)
#if LW_CFG_NET_NAT_MAX_SESSION < 128
#error "LW_CFG_NET_NAT_MAX_SESSION must bigger than 128!"
#endif                                                                  /*  MAX_SESSION < 128           */
#if LW_CFG_NET_NAT_MAX_SESSION > 24576
#error "LW_CFG_NET_NAT_MAX_SESSION must less than 24576!"
#endif                                                                  /*  MAX_SESSION > 24576         */

#define LW_CFG_NET_NAT_MIN_PORT  (0xffff - LW_CFG_NET_NAT_MAX_SESSION + 1)
#define LW_CFG_NET_NAT_MAX_PORT  (0xffff)                               /*  NAT �˿�ӳ�䷶Χ            */

#ifdef __SYLIXOS_NET_PORT_RNG                                           /*  0x37ff = 0x3fff - 2048      */
#define TCP_LOCAL_PORT_RANGE_START        (LW_CFG_NET_NAT_MIN_PORT - 0x37ff)
#define TCP_LOCAL_PORT_RANGE_END          (LW_CFG_NET_NAT_MIN_PORT - 1)
#define TCP_ENSURE_LOCAL_PORT_RANGE(port) ((port % (TCP_LOCAL_PORT_RANGE_END - TCP_LOCAL_PORT_RANGE_START)) + \
                                           TCP_LOCAL_PORT_RANGE_START)

#define UDP_LOCAL_PORT_RANGE_START        (LW_CFG_NET_NAT_MIN_PORT - 0x37ff)
#define UDP_LOCAL_PORT_RANGE_END          (LW_CFG_NET_NAT_MIN_PORT - 1)
#define UDP_ENSURE_LOCAL_PORT_RANGE(port) ((port % (UDP_LOCAL_PORT_RANGE_END - UDP_LOCAL_PORT_RANGE_START)) + \
                                           UDP_LOCAL_PORT_RANGE_START)
#endif                                                                  /*  __SYLIXOS_NET_PORT_RNG      */
#endif                                                                  /*  LW_CFG_NET_ROUTER           */
                                                                        /*  LW_CFG_NET_NAT_EN           */
/*********************************************************************************************************
  mbox size
*********************************************************************************************************/

#define TCPIP_MBOX_SIZE                 LW_CFG_LWIP_NUM_INPKT           /*  ������                    */
#define DEFAULT_ACCEPTMBOX_SIZE         256

/*********************************************************************************************************
  TCP basic
*********************************************************************************************************/

#if LW_CFG_LWIP_TCP_SIG_EN > 0
#define LWIP_TCP_PCB_NUM_EXT_ARGS       1
#endif

#if LW_CFG_LWIP_TCP_MAXRTX > 0
#define TCP_MAXRTX                      LW_CFG_LWIP_TCP_MAXRTX
#endif

#if LW_CFG_LWIP_TCP_SYNMAXRTX > 0
#define TCP_SYNMAXRTX                   LW_CFG_LWIP_TCP_SYNMAXRTX
#endif

/*********************************************************************************************************
  transmit layer
*********************************************************************************************************/

#if LW_CFG_LWIP_UDP_PCB > 0
#define LWIP_UDP                        1
#define LWIP_UDPLITE                    1
#else
#define LWIP_UDP                        0
#define LWIP_UDPLITE                    0
#endif

#if LW_CFG_LWIP_TCP_PCB > 0
#define LWIP_TCP                        1
#else
#define LWIP_TCP                        0
#endif

#define TCP_LISTEN_MULTI                LW_CFG_LWIP_TCP_MULTI_PORTS     /*  SylixOS Add Listen mports   */
#define TCP_LISTEN_BACKLOG              1
#define LWIP_TCP_TIMESTAMPS             1

#define TCP_QUEUE_OOSEQ                 1                               /*  Out of Order support        */

#define LWIP_TCP_SACK_OUT               1                               /*  Full SACK support           */
#define LWIP_TCP_SACK_IN                LWIP_TCP_SACK_OUT

#ifndef TCP_MSS
#ifdef __SYLIXOS_LITE
#define TCP_MSS                         1460
#else
#define TCP_MSS                         4096                            /*  WND Min is 8K               */
#endif
#endif                                                                  /*  TCP_MSS                     */

#define TCP_CALCULATE_EFF_SEND_MSS      1                               /*  use effective send MSS      */

/*********************************************************************************************************
  TCP_WND & TCP_SND_BUF
*********************************************************************************************************/

#define LWIP_WND_SCALE                  1

#define TCP_WND                         LW_CFG_LWIP_TCP_WND
#define TCP_SND_BUF                     LWIP_MIN(LW_CFG_LWIP_TCP_SND, 0xffff)
#define TCP_RCV_SCALE                   LW_CFG_LWIP_TCP_SCALE
#define TCP_WND_UPDATE_THRESHOLD        (pcb->if_wnd >> 1)              /*  1/2 window size             */

/*********************************************************************************************************
  TCP Other
*********************************************************************************************************/

#define MEMP_NUM_TCP_SEG                MEMP_NUM_PBUF                   /*  Max num of tcp segments     */

#define LWIP_TCP_KEEPALIVE              1
#define LWIP_NETCONN_FULLDUPLEX         1
#define LWIP_NETBUF_RECVINFO            1

#if LW_CFG_NET_SAFE > 0
#define LWIP_NETCONN_SEM_PER_THREAD     1
#else
#define LWIP_NETCONN_SEM_PER_THREAD     0
#endif

#define LWIP_SO_LINGER                  1
#define LWIP_SO_SNDTIMEO                1
#define LWIP_SO_RCVTIMEO                1
#define LWIP_SO_SNDBUF                  1
#define LWIP_SO_RCVBUF                  1

#define SO_REUSE                        1                               /*  enable SO_REUSEADDR         */
#define SO_REUSE_RXTOALL                1

#define LWIP_FIONREAD_LINUXMODE         1                               /*  linux FIONREAD compatibility*/

/*********************************************************************************************************
  DNS
*********************************************************************************************************/

#define LWIP_DNS                        LWIP_UDP

#ifndef MEMP_NUM_NETDB
#define MEMP_NUM_NETDB                  12
#endif                                                                  /*  MEMP_NUM_NETDB              */

#if LW_CFG_NET_IPV6 > 0
#define DNS_MAX_SERVERS                 4
#else
#define DNS_MAX_SERVERS                 2
#endif

#define DNS_MAX_NAME_LENGTH             PATH_MAX
#define DNS_LOCAL_HOSTLIST              1

extern INT  __inetHostTableGetItem(CPCHAR  pcHost, size_t  stLen, PVOID  pvAddr, UINT8  ucAddrType);
                                                                        /*  ���ص�ַӳ����ѯ          */
                                                                        /*  ��Χ IPv4 �����ֽ����ַ    */
#define DNS_LOCAL_HOSTLIST_IS_DYNAMIC   1
#define DNS_LOOKUP_LOCAL_EXTERN(name, len, addr, type)   \
        __inetHostTableGetItem(name, len, addr, type)

/*********************************************************************************************************
  dhcp & autoip
*********************************************************************************************************/

#define LWIP_DHCP                       LWIP_UDP                        /*  DHCP                        */
#define LWIP_DHCP_CHECK_LINK_UP         1
#define LWIP_DHCP_BOOTP_FILE            0                               /*  not include bootp file now  */
#define LWIP_AUTOIP                     1

#if (LWIP_DHCP > 0) && (LWIP_AUTOIP > 0)
#define LWIP_DHCP_AUTOIP_COOP           1
#endif                                                                  /*  (LWIP_DHCP > 0)             */
                                                                        /*  (LWIP_AUTOIP > 0)           */
#define LWIP_IPV6_DHCP6                 LWIP_IPV6                       /*  DHCPv6                      */

/*********************************************************************************************************
  network interfaces options
*********************************************************************************************************/

#define LWIP_SINGLE_NETIF               0
#define LWIP_NETIF_HOSTNAME             1                               /*  netif have nostname member  */
#define LWIP_NETIF_API                  1                               /*  support netif api           */
#define LWIP_NETIF_STATUS_CALLBACK      1                               /*  interface status change     */
#define LWIP_NETIF_REMOVE_CALLBACK      1                               /*  interface remove hook       */
#define LWIP_NETIF_LINK_CALLBACK        1                               /*  link status change          */
#define LWIP_NETIF_HWADDRHINT           1                               /*  XXX                         */
#define LWIP_NETIF_LOOPBACK             1
#define LWIP_NETIF_EXT_STATUS_CALLBACK  1

/*********************************************************************************************************
  network interfaces max hardware address len
*********************************************************************************************************/

#define NETIF_MAX_HWADDR_LEN            8                               /*  max 8bytes physical addr len*/

/*********************************************************************************************************
  ethernet net options
*********************************************************************************************************/

#define LWIP_ARP                        1
#define ARP_QUEUEING                    1
#define ARP_QUEUE_LEN                   LW_CFG_LWIP_ARP_QUEUE_LEN       /*  ÿ����ѯ�ɻ������ݰ�����    */
#define ARP_TABLE_SIZE                  LW_CFG_LWIP_ARP_TABLE_SIZE

#define ETHARP_SUPPORT_VLAN             1                               /*  IEEE 802.1q VLAN            */
#define ETH_PAD_SIZE                    2
#define ETHARP_SUPPORT_STATIC_ENTRIES   1
#define ETHARP_TABLE_MATCH_NETIF        1

#if LW_CFG_NET_ROUTER > 0
#if LW_CFG_LWIP_NUM_ARP_QUEUE < (LW_CFG_LWIP_ARP_TABLE_SIZE * 2)
#undef LW_CFG_LWIP_NUM_ARP_QUEUE
#define LW_CFG_LWIP_NUM_ARP_QUEUE       (LW_CFG_LWIP_ARP_TABLE_SIZE * 2)
#endif
#endif                                                                  /*  LW_CFG_NET_ROUTER > 0       */

#define MEMP_NUM_ARP_QUEUE              LW_CFG_LWIP_NUM_ARP_QUEUE
#define MEMP_NUM_ND6_QUEUE              LW_CFG_LWIP_NUM_ARP_QUEUE

/*********************************************************************************************************
  loop interface
*********************************************************************************************************/

#define LWIP_HAVE_LOOPIF                1                               /*  127.0.0.1                   */

/*********************************************************************************************************
  slip interface
*********************************************************************************************************/

#define LWIP_HAVE_SLIPIF                1

/*********************************************************************************************************
  inet thread
*********************************************************************************************************/

#define TCPIP_THREAD_NAME               "t_netproto"
#define TCPIP_THREAD_STACKSIZE          LW_CFG_LWIP_DEF_STK_SIZE
#define TCPIP_THREAD_PRIO               LW_PRIO_T_NETPROTO

#define SLIPIF_THREAD_NAME              "t_slip"
#define SLIPIF_THREAD_STACKSIZE         LW_CFG_LWIP_DEF_STK_SIZE
#define SLIPIF_THREAD_PRIO              LW_PRIO_T_NETPROTO

#define PPP_THREAD_NAME                 "t_ppp"
#define PPP_THREAD_STACKSIZE            LW_CFG_LWIP_DEF_STK_SIZE
#define PPP_THREAD_PRIO                 LW_PRIO_T_NETPROTO

#define DEFAULT_THREAD_NAME             "t_netdef"
#define DEFAULT_THREAD_STACKSIZE        LW_CFG_LWIP_DEF_STK_SIZE
#define DEFAULT_THREAD_PRIO             LW_PRIO_T_NETPROTO

#define NETDEVTXQ_THREAD_NAME           "t_nettxq"
#define NETDEVTXQ_THREAD_STACKSIZE      LW_CFG_LWIP_DEF_STK_SIZE
#define NETDEVTXQ_THREAD_PRIO           LW_PRIO_T_NETPROTO

#define NETBONDING_THREAD_NAME          "t_bonding"
#define NETBONDING_THREAD_STACKSIZE     LW_CFG_LWIP_DEF_STK_SIZE
#define NETBONDING_THREAD_PRIO          LW_PRIO_T_NETPROTO

/*********************************************************************************************************
  Socket options 
*********************************************************************************************************/

#define LWIP_SOCKET                     1
#define LWIP_TIMEVAL_PRIVATE            0                               /*  SylixOS has already defined */
#define LWIP_COMPAT_SOCKETS             0                               /*  do not us socket macro      */
#define LWIP_PREFIX_BYTEORDER_FUNCS     1                               /*  do not define macro hto??() */
#define LWIP_SOCKET_SELECT              1
#define LWIP_SOCKET_POLL                0

#define LWIP_POSIX_SOCKETS_IO_NAMES     0                               /*  do not have this!!!         */

#if LW_CFG_LWIP_DEF_RECV_BUFSIZE == 0
#define RECV_BUFSIZE_DEFAULT            INT_MAX
#else
#define RECV_BUFSIZE_DEFAULT            LW_CFG_LWIP_DEF_RECV_BUFSIZE
#endif                                                                  /*  LW_CFG_LWIP_DEF_RECV_BUFSIZE*/

/*********************************************************************************************************
  DNS config
*********************************************************************************************************/

#define LWIP_DNS_API_DEFINE_ERRORS      0                               /*  must use sylixos dns errors */
#define LWIP_DNS_API_HOSTENT_STORAGE    1                               /*  have bsd DNS                */
#define LWIP_DNS_API_DECLARE_STRUCTS    1                               /*  use lwip DNS struct         */

/*********************************************************************************************************
  Statistics options
*********************************************************************************************************/

#define LWIP_STATS                      1
#define LWIP_STATS_DISPLAY              1
#define LWIP_STATS_LARGE                1                               /*  must 32bits                 */

/*********************************************************************************************************
  PPP options
*********************************************************************************************************/

#if LW_CFG_SHELL_PASS_CRYPT_EN > 0
#define LWIP_USE_EXTERNAL_MBEDTLS       1                               /*  we use mbedtls now          */
#endif                                                                  /*  LW_CFG_SHELL_PASS_CRYPT_EN  */

#define PPP_SUPPORT                     LW_CFG_LWIP_PPP
#define PPPOS_SUPPORT                   LW_CFG_LWIP_PPP
#define PPPOE_SUPPORT                   LW_CFG_LWIP_PPPOE
#define PPPOE_SCNAME_SUPPORT            LW_CFG_LWIP_PPPOE
#define PPPOL2TP_SUPPORT                LW_CFG_LWIP_PPPOL2TP

/*********************************************************************************************************
  [ppp-new] branch
*********************************************************************************************************/

#define __LWIP_USE_PPP_NEW              1
#define PPP_SERVER                      0

/*********************************************************************************************************
  ppp api
*********************************************************************************************************/

#if PPP_SUPPORT > 0 || PPPOE_SUPPORT > 0

#define LWIP_PPP_API                    1

/*********************************************************************************************************
  ppp num and interface
*********************************************************************************************************/

#define MEMP_NUM_PPPOE_INTERFACES       LW_CFG_LWIP_NUM_PPP
#define MEMP_NUM_PPPOL2TP_INTERFACES    LW_CFG_LWIP_NUM_PPP
#define NUM_PPP                         LW_CFG_LWIP_NUM_PPP
#define MEMP_NUM_PPP_PCB                LW_CFG_LWIP_NUM_PPP

/*********************************************************************************************************
  ppp option
*********************************************************************************************************/

#define PPP_INPROC_IRQ_SAFE             1                               /*  pppos use pppfd recv-thread */
#define PPP_NOTIFY_PHASE                1
#define PPP_FCS_TABLE                   1
#define PPP_OUR_NAME                    "SylixOS"

#define PAP_SUPPORT                     1
#define CHAP_SUPPORT                    1

#if __LWIP_USE_PPP_NEW > 0
#define MSCHAP_SUPPORT                  1
#else
#define MSCHAP_SUPPORT                  0
#endif                                                                  /*  __LWIP_USE_PPP_NEW > 0      */

#define EAP_SUPPORT                     1
#define CBCP_SUPPORT                    0
#define CCP_SUPPORT                     0
#define ECP_SUPPORT                     0
#define LQR_SUPPORT                     1
#define VJ_SUPPORT                      1
#define MD5_SUPPORT                     1

#define PPP_MD5_RANDM                   1
#define PPP_IPV6_SUPPORT                LWIP_IPV6

/*********************************************************************************************************
  ppp timeouts
*********************************************************************************************************/

#define FSM_DEFTIMEOUT                  LW_CFG_NET_PPP_TIMEOUT
#define UPAP_DEFTIMEOUT                 LW_CFG_NET_PPP_TIMEOUT
#define CHAP_DEFTIMEOUT                 LW_CFG_NET_PPP_TIMEOUT
#define EAP_DEFREQTIME                  LW_CFG_NET_PPP_TIMEOUT
#define EAP_DEFTIMEOUT                  LW_CFG_NET_PPP_TIMEOUT

#define FSM_DEFMAXCONFREQS              LW_CFG_NET_PPP_RETRANSMITS
#define UPAP_DEFTRANSMITS               LW_CFG_NET_PPP_RETRANSMITS
#define CHAP_DEFTRANSMITS               LW_CFG_NET_PPP_RETRANSMITS
#define EAP_DEFALLOWREQ                 LW_CFG_NET_PPP_RETRANSMITS
#define LCP_DEFLOOPBACKFAIL             LW_CFG_NET_PPP_RETRANSMITS

#define LCP_ECHOINTERVAL                LW_CFG_NET_PPP_ECHOINTERVAL     /*  echo timer                  */

/*********************************************************************************************************
  ppp api
*********************************************************************************************************/
#else                                                                   /*  no ppp support              */

#define LWIP_PPP_API                    0

#endif                                                                  /*  PPP_SUPPORT > 0 ||          */
                                                                        /*  PPPOE_SUPPORT > 0           */
/*********************************************************************************************************
  sylixos ��Ҫʹ�� lwip core lock ģʽ, ��֧��ȫ˫�� socket
*********************************************************************************************************/

#define LWIP_TCPIP_CORE_LOCKING         1
#define LWIP_TCPIP_TIMEOUT              0                               /*  ��ʱ����Ҫ                  */
                                                                        
/*********************************************************************************************************
  Debugging options (TCP UDP IP debug & only can print message)
*********************************************************************************************************/

#define LWIP_DBG_TYPES_ON               LWIP_DBG_ON

#if LW_CFG_LWIP_DEBUG > 0
#define LWIP_DEBUG
#define LWIP_DBG_MIN_LEVEL              LWIP_DBG_LEVEL_OFF              /*  ����ȫ��������ӡ��Ϣ        */
#define TCP_DEBUG                       (LWIP_DBG_ON | 0)               /*  ������ TCP UDP IP debug     */
#define UDP_DEBUG                       (LWIP_DBG_ON | 0)
#define IP_DEBUG                        (LWIP_DBG_ON | 0)
#else
#define LWIP_DBG_MIN_LEVEL              LWIP_DBG_LEVEL_SEVERE           /*  ������������ӡ��Ϣ          */
#endif                                                                  /*  LW_CFG_LWIP_DEBUG > 0       */

/*********************************************************************************************************
  get_opt ��ͻ
*********************************************************************************************************/

#ifdef  opterr
#undef  opterr
#endif                                                                  /*  opterr                      */

/*********************************************************************************************************
  sylixos msgqueue
*********************************************************************************************************/

#define LWIP_MSGQUEUE_SIZE          LW_CFG_LWIP_MSG_SIZE                /*  sys_mbox_new size           */

/*********************************************************************************************************
  sylixos tty (serial port) file name
*********************************************************************************************************/

#define LWIP_SYLIXOS_TTY_NAME       "/dev/ttyS"                         /*  ���� TTY �ж��ļ���ǰ׺     */

/*********************************************************************************************************
  lwip virtual device file name
*********************************************************************************************************/

#define LWIP_SYLIXOS_SOCKET_NAME    "/dev/socket"                       /*  socket file name            */

/*********************************************************************************************************
  lwip internal hooks
*********************************************************************************************************/

#define LWIP_HOOK_FILENAME          "network/arch/hook.h"               /*  hook ����                   */

/*********************************************************************************************************
  lwip ip route hook
*********************************************************************************************************/

#define LWIP_HOOK_IP4_ROUTE_SRC     ip_route_src_hook
#define LWIP_HOOK_IP4_ROUTE_DEFAULT ip_route_default_hook
#define LWIP_HOOK_ETHARP_GET_GW     ip_gw_hook

#if LWIP_IPV6 > 0
#define LWIP_HOOK_IP6_ROUTE         ip6_route_src_hook
#define LWIP_HOOK_IP6_ROUTE_DEFAULT ip6_route_default_hook
#define LWIP_HOOK_ND6_GET_GW        ip6_gw_hook
#endif                                                                  /*  LWIP_IPV6 > 0               */

/*********************************************************************************************************
  lwip ip input hook (return is the packet has been eaten)
*********************************************************************************************************/

#define LWIP_HOOK_IP4_INPUT         ip_input_hook
#define LWIP_HOOK_IP6_INPUT         ip6_input_hook

/*********************************************************************************************************
  lwip link input/output hook (for AF_PACKET and Net Defender)
*********************************************************************************************************/

#define LWIP_HOOK_LINK_INPUT        link_input_hook
#define LWIP_HOOK_LINK_OUTPUT       link_output_hook

/*********************************************************************************************************
  lwip vlan hook (for AF_PACKET and Net Defender)
*********************************************************************************************************/

#if LW_CFG_NET_VLAN_EN > 0
#define LWIP_HOOK_VLAN_SET          ethernet_vlan_set_hook
#define LWIP_HOOK_VLAN_CHECK        ethernet_vlan_check_hook
#endif                                                                  /*  LW_CFG_NET_VLAN_EN > 0      */

/*********************************************************************************************************
  TCP hook
*********************************************************************************************************/

#if LW_CFG_LWIP_TCP_SIG_EN > 0
#define LWIP_HOOK_TCP_INPACKET_PCB          tcp_md5_check_inpacket
#define LWIP_HOOK_TCP_OPT_LENGTH_SEGMENT    tcp_md5_get_additional_option_length
#define LWIP_HOOK_TCP_ADD_TX_OPTIONS        tcp_md5_add_tx_options
#endif                                                                  /*  LW_CFG_LWIP_TCP_SIG_EN > 0  */

/*********************************************************************************************************
  lwip socket.h ��غ��ض���, �Լ��ݾ��������Ӧ��
*********************************************************************************************************/

#define SHUT_RD     0
#define SHUT_WR     1
#define SHUT_RDWR   2

#ifdef __cplusplus
}
#endif                                                                  /*  __cplusplus                 */

#endif                                                                  /*  __LWIP_CONFIG_H             */
/*********************************************************************************************************
  END
*********************************************************************************************************/
