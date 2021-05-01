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
** ��   ��   ��: lwip_socket.c
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2012 �� 12 �� 18 ��
**
** ��        ��: socket �ӿ�. (ʹ�� sylixos io ϵͳ���� lwip �ļ�������)

** BUG:
2013.01.02  �� socket �ṹ�м��� errno ��¼, SO_ERROR ͳһ�� socket �㴦��.
2013.01.10  ���� POSIX Ҫ��������ȷ��ȡ����.
2013.01.20  �������� SO_ERROR ��һ������:
            BSD socket �涨, ���ʹ�� NBIO ִ�� connect ����, ���ܳɹ���񶼻������˳�, �������ִ������
            ����, �� errno Ϊ EINPROGRESS. ��ʱӦ�ó���ʹ�� select ���� poll ��������, ������ӳɹ�, ��
            ��ʱͨ�� getsockopt �� SO_ERROR ѡ���ȡ�Ľ��Ӧ��Ϊ 0, ������Ϊ������������. 
            ����, ����ͨ����ͨ�� select ����ʱ, ��Ҫ�ύ���µĴ����, ���Ը��� socket ��ǰ��¼�Ĵ����.
2013.01.30  ʹ�� hash ��, �ӿ� socket_event �ٶ�.
2013.04.27  ���� hostbyaddr() α����, ���� netdb �����ⲿ C ��ʵ��.
2013.04.28  ���� __IfIoctl() ��ģ�� BSD ϵͳ ifioctl �Ĳ��ֹ���.
2013.04.29  lwip_fcntl() �����⵽ O_NONBLOCK λ�⻹������λ, �����. ������Ҫ����.
2013.09.24  ioctl() ����� SIOCGSIZIFCONF ��֧��.
            ioctl() ֧�� IPv6 ��ַ����.
2013.11.17  ����� SOCK_CLOEXEC �� SOCK_NONBLOCK ��֧��.
2013.11.21  ���� accept4() ����.
2014.03.22  ���� AF_PACKET ֧��.
2014.04.01  ���� socket �ļ��� mmap ��֧��.
2014.05.02  __SOCKET_CHECHK() �жϳ���ʱ��ӡ debug ��Ϣ.
            socket ���� monitor ���������.
            ���� __ifIoctl() �� if_indextoname �����Ĵ���.
2014.05.03  �����ȡ�������͵Ľӿ�.
2014.05.07  __ifIoctlIf() ����ʹ�� netif ioctl.
2014.11.07  AF_PACKET ioctl() ֧�ֻ������ڲ���.
2015.03.02  ���� socket ��λ����, ���� socket ������Ҫ���� SO_LINGER Ϊ�����ر�ģʽ.
2015.03.17  netif ioctl ���������ģʽ������.
2015.04.06  *_socket_event() ������Ҫ socket lock ����.
2015.04.17  ��������չ ioctl ר�ŷ�װ.
2017.12.08  ���� AF_ROUTE ֧��.
2017.12.20  �������ؽӿ�֧��.
2018.07.17  ���� QoS �ӿ�.
2018.08.01  �� select �����㷨, ���Ч��.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "SylixOS.h"
/*********************************************************************************************************
  �ü�����
*********************************************************************************************************/
#if LW_CFG_NET_EN > 0
#include "netdb.h"
#include "sys/socket.h"
#include "sys/un.h"
#include "lwip/mem.h"
#include "lwip/dns.h"
#include "net/if.h"
#include "net/if_arp.h"
#include "net/if_vlan.h"
#include "net/route.h"
#include "lwip_arpctl.h"
#include "lwip_ifctl.h"
#include "lwip_vlanctl.h"
/*********************************************************************************************************
  QoS
*********************************************************************************************************/
#if LW_CFG_LWIP_IPQOS > 0
#include "netinet/ip_qos.h"
#include "lwip_qosctl.h"
#endif                                                                  /*  LW_CFG_LWIP_IPQOS > 0       */
/*********************************************************************************************************
  ·��
*********************************************************************************************************/
#if LW_CFG_NET_ROUTER > 0
#include "lwip_rtctl.h"
#if LW_CFG_NET_BALANCING > 0
#include "net/sroute.h"
#include "lwip_srtctl.h"
#endif                                                                  /*  LW_CFG_NET_BALANCING > 0    */
#endif                                                                  /*  LW_CFG_NET_ROUTER > 0       */
#if LW_CFG_NET_MROUTER > 0
#include "lwip_mrtctl.h"
#endif                                                                  /*  LW_CFG_NET_MROUTER > 0      */
/*********************************************************************************************************
  ����
*********************************************************************************************************/
#if LW_CFG_NET_FLOWCTL_EN > 0
#include "lwip_flowctl.h"
#include "net/flowctl.h"
#endif                                                                  /*  LW_CFG_NET_FLOWCTL_EN > 0   */
/*********************************************************************************************************
  ����
*********************************************************************************************************/
#if LW_CFG_NET_WIRELESS_EN > 0
#include "net/if_wireless.h"
#endif                                                                  /*  LW_CFG_NET_WIRELESS_EN > 0  */
/*********************************************************************************************************
  Э����
*********************************************************************************************************/
#include "./packet/af_packet.h"
#include "./unix/af_unix.h"
#if LW_CFG_NET_ROUTER > 0
#include "./route/af_route.h"
#endif                                                                  /*  LW_CFG_NET_ROUTER > 0       */
/*********************************************************************************************************
  ipv6 extern vars
*********************************************************************************************************/
#if LWIP_IPV6
const struct in6_addr in6addr_loopback           = IN6ADDR_LOOPBACK_INIT;
const struct in6_addr in6addr_nodelocal_allnodes = IN6ADDR_NODELOCAL_ALLNODES_INIT;
const struct in6_addr in6addr_linklocal_allnodes = IN6ADDR_LINKLOCAL_ALLNODES_INIT;
#endif
/*********************************************************************************************************
  �ļ��ṹ (֧�ֵ�Э��� AF_INET AF_INET6 AF_RAW AF_UNIX)
*********************************************************************************************************/
typedef struct {
    LW_LIST_LINE        SOCK_lineManage;                                /*  ��������                    */
    INT                 SOCK_iFamily;                                   /*  Э���                      */
    
    union {
        INT             SOCKF_iLwipFd;                                  /*  lwip �ļ�������             */
#if LW_CFG_NET_UNIX_EN > 0
        AF_UNIX_T      *SOCKF_pafunix;                                  /*  AF_UNIX ���ƿ�              */
#endif
#if LW_CFG_NET_ROUTER > 0
        AF_ROUTE_T     *SOCKF_pafroute;                                 /*  AF_ROUTE ���ƿ�             */
#endif
        AF_PACKET_T    *SOCKF_pafpacket;                                /*  AF_PACKET ���ƿ�            */
    } SOCK_family;
    
    INT                 SOCK_iSoErr;                                    /*  ���һ�δ���                */
    LW_SEL_WAKEUPLIST   SOCK_selwulist;
} SOCKET_T;

#define SOCK_iLwipFd    SOCK_family.SOCKF_iLwipFd
#define SOCK_pafunix    SOCK_family.SOCKF_pafunix
#define SOCK_pafroute   SOCK_family.SOCKF_pafroute
#define SOCK_pafpacket  SOCK_family.SOCKF_pafpacket
/*********************************************************************************************************
  ��������
*********************************************************************************************************/
static LONG             __socketOpen(LW_DEV_HDR *pdevhdr, PCHAR  pcName, INT  iFlag, mode_t  mode);
static INT              __socketClose(SOCKET_T *psock);
static ssize_t          __socketRead(SOCKET_T *psock, PVOID  pvBuffer, size_t  stLen);
static ssize_t          __socketWrite(SOCKET_T *psock, CPVOID  pvBuffer, size_t  stLen);
static INT              __socketIoctl(SOCKET_T *psock, INT  iCmd, PVOID  pvArg);
static INT              __socketMmap(SOCKET_T *psock, PLW_DEV_MMAP_AREA  pdmap);
static INT              __socketUnmap(SOCKET_T *psock, PLW_DEV_MMAP_AREA  pdmap);
/*********************************************************************************************************
  ȫ�ֱ���
*********************************************************************************************************/
static LW_DEV_HDR           _G_devhdrSocket;
static LW_OBJECT_HANDLE     _G_hSockSelMutex;
/*********************************************************************************************************
  lwip �� unix �ڲ����� (�����ʵ�ִ���ֻ�����¼���Чʱ���ܸ��� piSoErr)
*********************************************************************************************************/
extern int              __lwip_have_event(int s, int type, int *piSoErr);
extern void             __lwip_set_sockfile(int s, void *file);
#if LW_CFG_NET_UNIX_EN > 0
extern int              __unix_have_event(AF_UNIX_T *pafunix, int type, int *piSoErr);
extern void             __unix_set_sockfile(AF_UNIX_T *pafunix, void *file);
#endif
#if LW_CFG_NET_ROUTER > 0
extern int              __route_have_event(AF_ROUTE_T *pafroute, int type, int  *piSoErr);
extern void             __route_set_sockfile(AF_ROUTE_T *pafroute, void *file);
#endif
extern int              __packet_have_event(AF_PACKET_T *pafpacket, int type, int  *piSoErr);
extern void             __packet_set_sockfile(AF_PACKET_T *pafpacket, void *file);
/*********************************************************************************************************
  socket fd ��Ч�Լ��
*********************************************************************************************************/
#define __SOCKET_CHECHK()   if (psock == (SOCKET_T *)PX_ERROR) {    \
                                _ErrorHandle(EBADF);    \
                                return  (PX_ERROR); \
                            }   \
                            if (iType != LW_DRV_TYPE_SOCKET) { \
                                _DebugHandle(__ERRORMESSAGE_LEVEL, "not a socket file.\r\n");   \
                                _ErrorHandle(ENOTSOCK);    \
                                return  (PX_ERROR); \
                            }
/*********************************************************************************************************
** ��������: __socketInit
** ��������: ��ʼ�� sylixos socket ϵͳ
** �䡡��  : NONE
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
VOID  __socketInit (VOID)
{
    static INT              iDrv = 0;
    struct file_operations  fileop;
    
    if (iDrv > 0) {
        return;
    }
    
    lib_bzero(&fileop, sizeof(struct file_operations));
    
    fileop.owner       = THIS_MODULE;
    fileop.fo_create   = __socketOpen;
    fileop.fo_release  = LW_NULL;
    fileop.fo_open     = __socketOpen;
    fileop.fo_close    = __socketClose;
    fileop.fo_read     = __socketRead;
    fileop.fo_write    = __socketWrite;
    fileop.fo_ioctl    = __socketIoctl;
    fileop.fo_mmap     = __socketMmap;
    fileop.fo_unmap    = __socketUnmap;
    
    iDrv = iosDrvInstallEx2(&fileop, LW_DRV_TYPE_SOCKET);
    if (iDrv < 0) {
        return;
    }
    
    DRIVER_LICENSE(iDrv,     "GPL->Ver 2.0");
    DRIVER_AUTHOR(iDrv,      "Han.hui");
    DRIVER_DESCRIPTION(iDrv, "socket driver v2.0");
    
    iosDevAddEx(&_G_devhdrSocket, LWIP_SYLIXOS_SOCKET_NAME, iDrv, DT_SOCK);
    
    _G_hSockSelMutex = API_SemaphoreMCreate("socksel_lock", LW_PRIO_DEF_CEILING, 
                                            LW_OPTION_WAIT_PRIORITY | LW_OPTION_DELETE_SAFE |
                                            LW_OPTION_INHERIT_PRIORITY | LW_OPTION_OBJECT_GLOBAL,
                                            LW_NULL);
    _BugHandle(!_G_hSockSelMutex, LW_TRUE, "socksel_lock create error!\r\n");
}
/*********************************************************************************************************
** ��������: __socketOpen
** ��������: socket open ����
** �䡡��  : pdevhdr   �豸
**           pcName    ����
**           iFlag     ѡ��
**           mode      δʹ��
** �䡡��  : socket �ṹ
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static LONG  __socketOpen (LW_DEV_HDR *pdevhdr, PCHAR  pcName, INT  iFlag, mode_t  mode)
{
    SOCKET_T    *psock;
    
    psock = (SOCKET_T *)__SHEAP_ALLOC(sizeof(SOCKET_T));
    if (psock == LW_NULL) {
        _ErrorHandle(ENOMEM);
        return  (PX_ERROR);
    }
    
    psock->SOCK_iFamily = AF_UNSPEC;
    psock->SOCK_iLwipFd = PX_ERROR;
    psock->SOCK_iSoErr  = 0;
    
    lib_bzero(&psock->SOCK_selwulist, sizeof(LW_SEL_WAKEUPLIST));
    psock->SOCK_selwulist.SELWUL_hListLock = _G_hSockSelMutex;
    
    return  ((LONG)psock);
}
/*********************************************************************************************************
** ��������: __socketClose
** ��������: socket close ����
** �䡡��  : psock     socket �ṹ
** �䡡��  : ERROR
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static INT  __socketClose (SOCKET_T *psock)
{
    switch (psock->SOCK_iFamily) {
    
#if LW_CFG_NET_UNIX_EN > 0
    case AF_UNIX:                                                       /*  UNIX ��Э��                 */
        if (psock->SOCK_pafunix) {
            unix_close(psock->SOCK_pafunix);
        }
        break;
#endif                                                                  /*  LW_CFG_NET_UNIX_EN > 0      */
        
#if LW_CFG_NET_ROUTER > 0
    case AF_ROUTE:
        if (psock->SOCK_pafroute) {
            route_close(psock->SOCK_pafroute);
        }
        break;
#endif                                                                  /*  LW_CFG_NET_ROUTER > 0       */
        
    case AF_PACKET:                                                     /*  PACKET                      */
        if (psock->SOCK_pafpacket) {
            packet_close(psock->SOCK_pafpacket);
        }
        break;
        
    default:                                                            /*  ����ʹ�� lwip               */
        if (psock->SOCK_iLwipFd >= 0) {
            lwip_close(psock->SOCK_iLwipFd);
        }
        break;
    }
    
    SEL_WAKE_UP_TERM(&psock->SOCK_selwulist);
    
    __SHEAP_FREE(psock);
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: __socketClose
** ��������: socket read ����
** �䡡��  : psock     socket �ṹ
**           pvBuffer  �����ݻ���
**           stLen     ��������С
** �䡡��  : ���ݸ���
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static ssize_t  __socketRead (SOCKET_T *psock, PVOID  pvBuffer, size_t  stLen)
{
    ssize_t     sstNum = 0;

    switch (psock->SOCK_iFamily) {

#if LW_CFG_NET_UNIX_EN > 0
    case AF_UNIX:                                                       /*  UNIX ��Э��                 */
        if (psock->SOCK_pafunix) {
            sstNum = unix_recvfrom(psock->SOCK_pafunix, pvBuffer, stLen, 0, LW_NULL, LW_NULL);
        }
        break;
#endif                                                                  /*  LW_CFG_NET_UNIX_EN > 0      */

#if LW_CFG_NET_ROUTER > 0
    case AF_ROUTE:                                                      /*  ROUTE                       */
        if (psock->SOCK_pafroute) {
            sstNum = route_recv(psock->SOCK_pafroute, pvBuffer, stLen, 0);
        }
        break;
#endif                                                                  /*  LW_CFG_NET_ROUTER > 0       */

    case AF_PACKET:                                                     /*  PACKET                      */
        if (psock->SOCK_pafpacket) {
            sstNum = packet_recvfrom(psock->SOCK_pafpacket, pvBuffer, stLen, 0, LW_NULL, LW_NULL);
        }
        break;
        
    default:                                                            /*  ����ʹ�� lwip               */
        if (psock->SOCK_iLwipFd >= 0) {
            sstNum = lwip_read(psock->SOCK_iLwipFd, pvBuffer, stLen);
        }
        break;
    }
    
    if (sstNum <= 0) {
        psock->SOCK_iSoErr = errno;
    
    } else {
        MONITOR_EVT_LONG3(MONITOR_EVENT_ID_NETWORK, MONITOR_EVENT_NETWORK_RECV, 
                          0, 0, sstNum, LW_NULL);                       /*  Ŀǰ�޷���ȡ fd             */
    }
    
    return  (sstNum);
}
/*********************************************************************************************************
** ��������: __socketClose
** ��������: socket write ����
** �䡡��  : psock     socket �ṹ
**           pvBuffer  д���ݻ���
**           stLen     д���ݴ�С
** �䡡��  : ���ݸ���
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static ssize_t  __socketWrite (SOCKET_T *psock, CPVOID  pvBuffer, size_t  stLen)
{
    ssize_t     sstNum = 0;

    switch (psock->SOCK_iFamily) {

#if LW_CFG_NET_UNIX_EN > 0
    case AF_UNIX:                                                       /*  UNIX ��Э��                 */
        if (psock->SOCK_pafunix) {
            sstNum = unix_sendto(psock->SOCK_pafunix, pvBuffer, stLen, 0, LW_NULL, 0);
        }
        break;
#endif                                                                  /*  LW_CFG_NET_UNIX_EN > 0      */

#if LW_CFG_NET_ROUTER > 0
    case AF_ROUTE:                                                      /*  ROUTE                       */
        if (psock->SOCK_pafroute) {
            sstNum = route_send(psock->SOCK_pafroute, pvBuffer, stLen, 0);
        }
        break;
#endif                                                                  /*  LW_CFG_NET_ROUTER > 0       */

    case AF_PACKET:                                                     /*  PACKET                      */
        if (psock->SOCK_pafpacket) {
            sstNum = packet_sendto(psock->SOCK_pafpacket, pvBuffer, stLen, 0, LW_NULL, 0);
        }
        break;
        
    default:                                                            /*  ����ʹ�� lwip               */
        if (psock->SOCK_iLwipFd >= 0) {
            sstNum = lwip_write(psock->SOCK_iLwipFd, pvBuffer, stLen);
        }
        break;
    }
    
    if (sstNum <= 0) {
        psock->SOCK_iSoErr = errno;
    
    } else {
        MONITOR_EVT_LONG3(MONITOR_EVENT_ID_NETWORK, MONITOR_EVENT_NETWORK_SEND, 
                          0, 0, sstNum, LW_NULL);                       /*  Ŀǰ�޷���ȡ fd             */
    }
    
    return  (sstNum);
}
/*********************************************************************************************************
** ��������: __socketClose
** ��������: socket ioctl ����
** �䡡��  : psock     socket �ṹ
**           iCmd      ����
**           pvArg     ����
** �䡡��  : ���ݸ���
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static INT  __socketIoctl (SOCKET_T *psock, INT  iCmd, PVOID  pvArg)
{
           INT                 iRet = PX_ERROR;
    struct stat               *pstatGet;
           PLW_SEL_WAKEUPNODE  pselwun;
           
    if (iCmd == FIOFSTATGET) {
        pstatGet = (struct stat *)pvArg;
        pstatGet->st_dev     = LW_DEV_MAKE_STDEV(&_G_devhdrSocket);
        pstatGet->st_ino     = (ino_t)0;                                /*  �൱��Ψһ�ڵ�              */
        pstatGet->st_mode    = 0666 | S_IFSOCK;
        pstatGet->st_nlink   = 1;
        pstatGet->st_uid     = 0;
        pstatGet->st_gid     = 0;
        pstatGet->st_rdev    = 1;
        pstatGet->st_size    = 0;
        pstatGet->st_blksize = 0;
        pstatGet->st_blocks  = 0;
        pstatGet->st_atime   = API_RootFsTime(LW_NULL);
        pstatGet->st_mtime   = API_RootFsTime(LW_NULL);
        pstatGet->st_ctime   = API_RootFsTime(LW_NULL);
        return  (ERROR_NONE);
    }
    
    switch (psock->SOCK_iFamily) {
    
    case AF_UNSPEC:                                                     /*  ��Ч                        */
        _ErrorHandle(ENOSYS);
        break;
    
#if LW_CFG_NET_UNIX_EN > 0
    case AF_UNIX:                                                       /*  UNIX ��Э��                 */
        if (psock->SOCK_pafunix) {
            switch (iCmd) {
            
            case FIOSELECT:
                pselwun = (PLW_SEL_WAKEUPNODE)pvArg;
                SEL_WAKE_NODE_ADD(&psock->SOCK_selwulist, pselwun);
                if (__unix_have_event(psock->SOCK_pafunix, 
                                      pselwun->SELWUN_seltypType,
                                      &psock->SOCK_iSoErr)) {
                    SEL_WAKE_UP(pselwun);
                }
                iRet = ERROR_NONE;
                break;
                
            case FIOUNSELECT:
                SEL_WAKE_NODE_DELETE(&psock->SOCK_selwulist, (PLW_SEL_WAKEUPNODE)pvArg);
                iRet = ERROR_NONE;
                break;
                
            default:
                iRet = unix_ioctl(psock->SOCK_pafunix, iCmd, pvArg);
                break;
            }
        }
        break;
#endif                                                                  /*  LW_CFG_NET_UNIX_EN > 0      */

#if LW_CFG_NET_ROUTER > 0
    case AF_ROUTE:                                                      /*  ROUTE                       */
        if (psock->SOCK_pafroute) {
            switch (iCmd) {
            
            case FIOSELECT:
                pselwun = (PLW_SEL_WAKEUPNODE)pvArg;
                SEL_WAKE_NODE_ADD(&psock->SOCK_selwulist, pselwun);
                if (__route_have_event(psock->SOCK_pafroute, 
                                       pselwun->SELWUN_seltypType,
                                       &psock->SOCK_iSoErr)) {
                    SEL_WAKE_UP(pselwun);
                }
                iRet = ERROR_NONE;
                break;
                
            case FIOUNSELECT:
                SEL_WAKE_NODE_DELETE(&psock->SOCK_selwulist, (PLW_SEL_WAKEUPNODE)pvArg);
                iRet = ERROR_NONE;
                break;
                
            default:
                iRet = route_ioctl(psock->SOCK_pafroute, iCmd, pvArg);
                break;
            }
        }
        break;
#endif                                                                  /*  LW_CFG_NET_ROUTER > 0       */

    case AF_PACKET:                                                     /*  PACKET                      */
        if (psock->SOCK_pafpacket) {
            switch (iCmd) {
            
            case FIOSELECT:
                pselwun = (PLW_SEL_WAKEUPNODE)pvArg;
                SEL_WAKE_NODE_ADD(&psock->SOCK_selwulist, pselwun);
                if (__packet_have_event(psock->SOCK_pafpacket, 
                                        pselwun->SELWUN_seltypType,
                                        &psock->SOCK_iSoErr)) {
                    SEL_WAKE_UP(pselwun);
                }
                iRet = ERROR_NONE;
                break;
                
            case FIOUNSELECT:
                SEL_WAKE_NODE_DELETE(&psock->SOCK_selwulist, (PLW_SEL_WAKEUPNODE)pvArg);
                iRet = ERROR_NONE;
                break;
            
            case SIOCGIFCONF:                                           /*  ͨ������ӿڲ���            */
            case SIOCGIFNUM:
            case SIOCGSIZIFCONF:
            case SIOCGSIZIFREQ6:
            case SIOCSIFFLAGS:
            case SIOCGIFFLAGS:
            case SIOCGIFTYPE:
            case SIOCGIFINDEX:
            case SIOCGIFMTU:
            case SIOCSIFMTU:
            case SIOCGIFHWADDR:
            case SIOCSIFHWADDR:
            case SIOCGIFPFLAGS:
            case SIOCSIFPFLAGS:
            case SIOCGIFSTATS:
                iRet = __ifIoctlPacket(iCmd, pvArg);
                break;
            
            case SIOCG802154PANID:
            case SIOCS802154PANID:
            case SIOCG802154SHRTADDR:
            case SIOCS802154SHRTADDR:
            case SIOCG802154CTX:
            case SIOCS802154CTX:
            case SIOCG7668DSTADDR:
            case SIOCS7668DSTADDR:
            case SIOCG7668CTX:
            case SIOCS7668CTX:
                iRet = __ifIoctlLp(iCmd, pvArg);
                break;
                
            case SIOCSARP:
            case SIOCGARP:
            case SIOCDARP:
            case SIOCLSTARP:
                iRet = __ifIoctlArp(iCmd, pvArg);
                break;
                
            default:
                iRet = packet_ioctl(psock->SOCK_pafpacket, iCmd, pvArg);
                break;
            }
        }
        break;
        
    default:                                                            /*  ����ʹ�� lwip               */
        if (psock->SOCK_iLwipFd >= 0) {
            switch (iCmd) {
            
            case FIOSELECT:
                pselwun = (PLW_SEL_WAKEUPNODE)pvArg;
                SEL_WAKE_NODE_ADD(&psock->SOCK_selwulist, pselwun);
                if (__lwip_have_event(psock->SOCK_iLwipFd, 
                                      pselwun->SELWUN_seltypType,
                                      &psock->SOCK_iSoErr)) {
                    SEL_WAKE_UP(pselwun);
                }
                iRet = ERROR_NONE;
                break;
            
            case FIOUNSELECT:
                SEL_WAKE_NODE_DELETE(&psock->SOCK_selwulist, (PLW_SEL_WAKEUPNODE)pvArg);
                iRet = ERROR_NONE;
                break;
                
            case FIOGETFL:
                if (pvArg) {
                    *(int *)pvArg  = lwip_fcntl(psock->SOCK_iLwipFd, F_GETFL, 0);
                    *(int *)pvArg |= O_RDWR;
                }
                iRet = ERROR_NONE;
                break;
                
            case FIOSETFL:
                {
                    INT iIsNonBlk = ((INT)(LONG)pvArg & O_NONBLOCK);    /*  ����λ���ܴ���              */
                    iRet = lwip_fcntl(psock->SOCK_iLwipFd, F_SETFL, iIsNonBlk);
                }
                break;
                
            case FIONREAD:
                if (pvArg) {
                    *(INT *)pvArg = 0;
                }
                iRet = lwip_ioctl(psock->SOCK_iLwipFd, (long)iCmd, pvArg);
                break;
                
            case SIOCGSIZIFCONF:
            case SIOCGIFNUM:
            case SIOCGIFCONF:
            case SIOCSIFADDR:
            case SIOCSIFNETMASK:
            case SIOCSIFDGWADDR:
            case SIOCSIFDSTADDR:
            case SIOCSIFBRDADDR:
            case SIOCSIFFLAGS:
            case SIOCGIFADDR:
            case SIOCGIFNETMASK:
            case SIOCGIFDGWADDR:
            case SIOCGIFDSTADDR:
            case SIOCGIFBRDADDR:
            case SIOCGIFFLAGS:
            case SIOCGIFTYPE:
            case SIOCGIFNAME:
            case SIOCGIFINDEX:
            case SIOCGIFMTU:
            case SIOCSIFMTU:
            case SIOCGIFHWADDR:
            case SIOCSIFHWADDR:
            case SIOCGIFMETRIC:
            case SIOCSIFMETRIC:
            case SIOCDIFADDR:
            case SIOCAIFADDR:
            case SIOCADDMULTI:
            case SIOCDELMULTI:
            case SIOCGIFTCPAF:
            case SIOCSIFTCPAF:
            case SIOCGIFTCPWND:
            case SIOCSIFTCPWND:
            case SIOCGIFPFLAGS:
            case SIOCSIFPFLAGS:
            case SIOCGSIZIFREQ6:
            case SIOCSIFADDR6:
            case SIOCSIFNETMASK6:
            case SIOCSIFDSTADDR6:
            case SIOCGIFADDR6:
            case SIOCGIFNETMASK6:
            case SIOCGIFDSTADDR6:
            case SIOCDIFADDR6:
            case SIOCGIFSTATS:
                iRet = __ifIoctlInet(iCmd, pvArg);
                break;
                
            case SIOCG802154PANID:
            case SIOCS802154PANID:
            case SIOCG802154SHRTADDR:
            case SIOCS802154SHRTADDR:
            case SIOCG802154CTX:
            case SIOCS802154CTX:
            case SIOCG7668DSTADDR:
            case SIOCS7668DSTADDR:
            case SIOCG7668CTX:
            case SIOCS7668CTX:
                iRet = __ifIoctlLp(iCmd, pvArg);
                break;
                
            case SIOCSARP:
            case SIOCGARP:
            case SIOCDARP:
            case SIOCLSTARP:
                iRet = __ifIoctlArp(iCmd, pvArg);
                break;
                
#if LW_CFG_LWIP_IPQOS > 0
            case SIOCSETIPQOS:
            case SIOCGETIPQOS:
                iRet = __qosIoctlInet(psock->SOCK_iFamily, iCmd, pvArg);
                break;
#endif                                                                  /*  LW_CFG_LWIP_IPQOS > 0       */
                
#if LW_CFG_NET_VLAN_EN > 0
            case SIOCSETVLAN:
            case SIOCGETVLAN:
            case SIOCLSTVLAN:
                iRet = __ifIoctlVlan(iCmd, pvArg);
                break;
#endif                                                                  /*  LW_CFG_NET_VLAN_EN > 0      */
                
#if LW_CFG_NET_ROUTER > 0
            case SIOCADDRT:
            case SIOCDELRT:
            case SIOCCHGRT:
            case SIOCGETRT:
            case SIOCLSTRT:
            case SIOCLSTRTM:
            case SIOCGTCPMSSADJ:
            case SIOCSTCPMSSADJ:
            case SIOCGFWOPT:
            case SIOCSFWOPT:
                iRet = __rtIoctlInet(psock->SOCK_iFamily, iCmd, pvArg);
                break;
                
#if LW_CFG_NET_BALANCING > 0
            case SIOCADDSRT:
            case SIOCDELSRT:
            case SIOCCHGSRT:
            case SIOCGETSRT:
            case SIOCLSTSRT:
                iRet = __srtIoctlInet(psock->SOCK_iFamily, iCmd, pvArg);
                break;
#endif                                                                  /*  LW_CFG_NET_BALANCING > 0    */

#if LW_CFG_NET_MROUTER > 0
            case SIOCGETVIFCNT:
            case SIOCGETSGCNT:
                iRet = __mrtIoctlInet(psock->SOCK_iFamily, iCmd, pvArg);
                break;
#endif                                                                  /*  LW_CFG_NET_MROUTER > 0      */

#if LW_CFG_NET_FLOWCTL_EN > 0
            case SIOCADDFC:
            case SIOCDELFC:
            case SIOCCHGFC:
            case SIOCGETFC:
            case SIOCLSTFC:
                iRet = __fcIoctlInet(psock->SOCK_iFamily, iCmd, pvArg);
                break;
#endif                                                                  /*  LW_CFG_NET_FLOWCTL_EN > 0   */
#endif                                                                  /*  LW_CFG_NET_ROUTER > 0       */
            
            default:
#if LW_CFG_NET_WIRELESS_EN > 0
                if ((iCmd >= SIOCIWFIRST) &&
                    (iCmd <= SIOCIWLASTPRIV)) {                         /*  ������������                */
                    iRet = __ifIoctlWireless(iCmd, pvArg);
                } else 
#endif                                                                  /*  LW_CFG_NET_WIRELESS_EN > 0  */
                if ((iCmd >= SIOCDEVPRIVATE) &&
                    (iCmd <= SIOCDEVPRIVATE + 0xf)) {                   /*  ����˽������                */
                    iRet = __ifIoctlPrivate(iCmd, pvArg);

                } else {
                    iRet = lwip_ioctl(psock->SOCK_iLwipFd, (long)iCmd, pvArg);
                }
                break;
            }
        }
        break;
    }
    
    return  (iRet);
}
/*********************************************************************************************************
** ��������: __socketMmap
** ��������: socket mmap ����
** �䡡��  : psock         socket �ṹ
**           pdmap         ����ռ���Ϣ
** �䡡��  : �������.
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static INT  __socketMmap (SOCKET_T *psock, PLW_DEV_MMAP_AREA  pdmap)
{
    INT     iRet;

    if (!pdmap) {
        return  (PX_ERROR);
    }

    switch (psock->SOCK_iFamily) {
        
    case AF_PACKET:                                                     /*  PACKET                      */
        if (psock->SOCK_pafpacket) {
            iRet = packet_mmap(psock->SOCK_pafpacket, pdmap);
        } else {
            iRet = PX_ERROR;
        }
        break;
        
    default:
        _ErrorHandle(ENOTSUP);
        iRet = PX_ERROR;
    }
    
    return  (iRet);
}
/*********************************************************************************************************
** ��������: __socketUnmap
** ��������: socket unmap ����
** �䡡��  : psock         socket �ṹ
**           pdmap         ����ռ���Ϣ
** �䡡��  : �������.
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static INT  __socketUnmap (SOCKET_T *psock, PLW_DEV_MMAP_AREA  pdmap)
{
    INT     iRet;

    if (!pdmap) {
        return  (PX_ERROR);
    }

    switch (psock->SOCK_iFamily) {
        
    case AF_PACKET:                                                     /*  PACKET                      */
        if (psock->SOCK_pafpacket) {
            iRet = packet_unmap(psock->SOCK_pafpacket, pdmap);
        } else {
            iRet = PX_ERROR;
        }
        break;
        
    default:
        _ErrorHandle(ENOTSUP);
        iRet = PX_ERROR;
    }
    
    return  (iRet);
}
/*********************************************************************************************************
** ��������: __socketReset
** ��������: socket ��λ����
** �䡡��  : psock         socket �ṹ
** �䡡��  : NONE.
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
VOID  __socketReset (PLW_FD_ENTRY  pfdentry)
{
    struct linger   lingerReset = {1, 0};
    SOCKET_T       *psock       = (SOCKET_T *)pfdentry->FDENTRY_lValue;
    
    if (psock && 
        ((psock->SOCK_iFamily == AF_INET) || 
        ((psock->SOCK_iFamily == AF_INET6)))) {
        INT         iAccept = 1, iType = SOCK_DGRAM;                    /*  ������� LISTEN ���͵� TCP  */
        socklen_t   socklen = sizeof(INT);
        
        lwip_getsockopt(psock->SOCK_iLwipFd, SOL_SOCKET, SO_ACCEPTCONN, &iAccept, &socklen);
        lwip_getsockopt(psock->SOCK_iLwipFd, SOL_SOCKET, SO_TYPE,       &iType,   &socklen);
        
        if (!iAccept && (iType == SOCK_STREAM)) {
            lwip_setsockopt(psock->SOCK_iLwipFd, SOL_SOCKET, SO_LINGER, 
                            &lingerReset, sizeof(struct linger));
        }
    }
}
/*********************************************************************************************************
** ��������: __socketEnotify
** ��������: socket �¼�֪ͨ
** �䡡��  : file        SOCKET_T
**           type        �¼�����
**           iSoErr      ���µ� SO_ERROR ��ֵ
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
void  __socketEnotify (void *file, LW_SEL_TYPE type, INT  iSoErr)
{
    SOCKET_T *psock = (SOCKET_T *)file;
    
    if (psock) {
        psock->SOCK_iSoErr = iSoErr;                                    /*  ���� SO_ERROR               */
        SEL_WAKE_UP_ALL(&psock->SOCK_selwulist, type);
    }
}
/*********************************************************************************************************
** ��������: __socketEnotify2
** ��������: socket �¼�֪ͨ
** �䡡��  : file        SOCKET_T
**           uiSelFlags  �¼����� LW_SEL_TYPE_FLAG_READ / LW_SEL_TYPE_FLAG_WRITE / LW_SEL_TYPE_FLAG_EXCEPT
**           iSoErr      ���µ� SO_ERROR ��ֵ
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
void  __socketEnotify2 (void *file, UINT uiSelFlags, INT  iSoErr)
{
    SOCKET_T *psock = (SOCKET_T *)file;
    
    if (psock) {
        psock->SOCK_iSoErr = iSoErr;                                    /*  ���� SO_ERROR               */
        if (uiSelFlags) {
            SEL_WAKE_UP_ALL_BY_FLAGS(&psock->SOCK_selwulist, uiSelFlags);
        }
    }
}
/*********************************************************************************************************
** ��������: socketpair
** ��������: BSD socketpair
** �䡡��  : domain        ��
**           type          ����
**           protocol      Э��
**           sv            ���������ɶ��ļ�������
** �䡡��  : ERROR
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API 
int  socketpair (int domain, int type, int protocol, int sv[2])
{
#if LW_CFG_NET_UNIX_EN > 0
    INT          iError;
    SOCKET_T    *psock[2];
    
    if (!sv) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    
    if (domain != AF_UNIX) {                                            /*  ��֧�� unix ��Э��          */
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    
    sv[0] = socket(AF_UNIX, type, protocol);                            /*  ���� socket                 */
    if (sv[0] < 0) {
        return  (PX_ERROR);
    }
    
    sv[1] = socket(AF_UNIX, type, protocol);                            /*  �����ڶ��� socket           */
    if (sv[1] < 0) {
        close(sv[0]);
        return  (PX_ERROR);
    }
    
    psock[0] = (SOCKET_T *)iosFdValue(sv[0]);
    psock[1] = (SOCKET_T *)iosFdValue(sv[1]);
    
    __KERNEL_SPACE_ENTER();
    iError = unix_connect2(psock[0]->SOCK_pafunix, psock[1]->SOCK_pafunix);
    __KERNEL_SPACE_EXIT();
    
    if (iError < 0) {
        close(sv[0]);
        close(sv[1]);
        return  (PX_ERROR);
    }
    
    MONITOR_EVT_INT5(MONITOR_EVENT_ID_NETWORK, MONITOR_EVENT_NETWORK_SOCKPAIR, 
                     domain, type, protocol, sv[0], sv[1], LW_NULL);
    
    return  (ERROR_NONE);
    
#else
    _ErrorHandle(ENOSYS);
    return  (PX_ERROR);
#endif                                                                  /*  LW_CFG_NET_UNIX_EN > 0      */
}
/*********************************************************************************************************
** ��������: socket
** ��������: BSD socket
** �䡡��  : domain    Э����
**           type      ����
**           protocol  Э��
** �䡡��  : fd
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
int  socket (int domain, int type, int protocol)
{
    INT          iFd     = PX_ERROR;
    INT          iLwipFd = PX_ERROR;
    
    SOCKET_T    *psock     = LW_NULL;
#if LW_CFG_NET_UNIX_EN > 0
    AF_UNIX_T   *pafunix   = LW_NULL;
#endif                                                                  /*  LW_CFG_NET_UNIX_EN > 0      */
#if LW_CFG_NET_ROUTER > 0
    AF_ROUTE_T  *pafroute  = LW_NULL;
#endif                                                                  /*  LW_CFG_NET_ROUTER > 0       */
    AF_PACKET_T *pafpacket = LW_NULL;
    
    INT          iCloExec;
    BOOL         iNonBlock;
    
    if (type & SOCK_CLOEXEC) {                                          /*  SOCK_CLOEXEC ?              */
        type &= ~SOCK_CLOEXEC;
        iCloExec = FD_CLOEXEC;
    } else {
        iCloExec = 0;
    }
    
    if (type & SOCK_NONBLOCK) {                                         /*  SOCK_NONBLOCK ?             */
        type &= ~SOCK_NONBLOCK;
        iNonBlock = 1;
    } else {
        iNonBlock = 0;
    }

    __KERNEL_SPACE_ENTER();
    switch (domain) {

#if LW_CFG_NET_UNIX_EN > 0
    case AF_UNIX:                                                       /*  UNIX ��Э��                 */
        pafunix = unix_socket(domain, type, protocol);
        if (pafunix == LW_NULL) {
            __KERNEL_SPACE_EXIT();
            goto    __error_handle;
        }
        break;
#endif                                                                  /*  LW_CFG_NET_UNIX_EN > 0      */

#if LW_CFG_NET_ROUTER > 0
    case AF_ROUTE:                                                      /*  AF_ROUTE ��Э��             */
        pafroute = route_socket(domain, type, protocol);
        if (pafroute == LW_NULL) {
            __KERNEL_SPACE_EXIT();
            goto    __error_handle;
        }
        break;
#endif                                                                  /*  LW_CFG_NET_ROUTER > 0       */

    case AF_PACKET:                                                     /*  PACKET                      */
        pafpacket = packet_socket(domain, type, protocol);
        if (pafpacket == LW_NULL) {
            __KERNEL_SPACE_EXIT();
            goto    __error_handle;
        }
        break;
    
    case AF_INET:                                                       /*  IPv4 / v6                   */
    case AF_INET6:
        iLwipFd = lwip_socket(domain, type, protocol);
        if (iLwipFd < 0) {
            __KERNEL_SPACE_EXIT();
            goto    __error_handle;
        }
        break;
        
    default:
        _ErrorHandle(EAFNOSUPPORT);
        __KERNEL_SPACE_EXIT();
        goto    __error_handle;
    }
    __KERNEL_SPACE_EXIT();
    
    iFd = open(LWIP_SYLIXOS_SOCKET_NAME, O_RDWR);
    if (iFd < 0) {
        goto    __error_handle;
    }
    psock = (SOCKET_T *)iosFdValue(iFd);
    if (psock == (SOCKET_T *)PX_ERROR) {
        goto    __error_handle;
    }
    psock->SOCK_iFamily = domain;
    
    switch (domain) {

#if LW_CFG_NET_UNIX_EN > 0
    case AF_UNIX:                                                       /*  UNIX ��Э��                 */
        psock->SOCK_pafunix = pafunix;
        __unix_set_sockfile(pafunix, psock);
        break;
#endif                                                                  /*  LW_CFG_NET_UNIX_EN > 0      */

#if LW_CFG_NET_ROUTER > 0
    case AF_ROUTE:                                                      /*  AF_ROUTE ��Э��             */
        psock->SOCK_pafroute = pafroute;
        __route_set_sockfile(pafroute, psock);
        break;
#endif                                                                  /*  LW_CFG_NET_ROUTER > 0       */

    case AF_PACKET:                                                     /*  PACKET                      */
        psock->SOCK_pafpacket = pafpacket;
        __packet_set_sockfile(pafpacket, psock);
        break;
        
    default:
        psock->SOCK_iLwipFd = iLwipFd;                                  /*  save lwip fd                */
        __lwip_set_sockfile(iLwipFd, psock);
        break;
    }
    
    if (iCloExec) {
        API_IosFdSetCloExec(iFd, iCloExec);
    }
    
    if (iNonBlock) {
        __KERNEL_SPACE_ENTER();
        __socketIoctl(psock, FIONBIO, &iNonBlock);
        __KERNEL_SPACE_EXIT();
    }
    
    MONITOR_EVT_INT4(MONITOR_EVENT_ID_NETWORK, MONITOR_EVENT_NETWORK_SOCKET, 
                     domain, type, protocol, iFd, LW_NULL);
    
    return  (iFd);
    
__error_handle:
    if (iFd >= 0) {
        close(iFd);
    }
    
#if LW_CFG_NET_UNIX_EN > 0
    if (pafunix) {
        unix_close(pafunix);
    }
#endif                                                                  /*  LW_CFG_NET_UNIX_EN > 0      */

#if LW_CFG_NET_ROUTER > 0
    if (pafroute) {
        route_close(pafroute);
    }
#endif                                                                  /*  LW_CFG_NET_ROUTER > 0       */

    if (pafpacket) {
        packet_close(pafpacket);
        
    } else if (iLwipFd >= 0) {
        lwip_close(iLwipFd);
    }
    
    return  (PX_ERROR);
}
/*********************************************************************************************************
** ��������: accept4
** ��������: BSD accept4
** �䡡��  : s         socket fd
**           addr      address
**           addrlen   address len
**           flags     SOCK_CLOEXEC, SOCK_NONBLOCK
** �䡡��  : new socket fd
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
int  accept4 (int s, struct sockaddr *addr, socklen_t *addrlen, int flags)
{
    SOCKET_T    *psock;
    SOCKET_T    *psockNew;
    INT          iType;
    
#if LW_CFG_NET_UNIX_EN > 0
    AF_UNIX_T   *pafunix   = LW_NULL;
#endif                                                                  /*  LW_CFG_NET_UNIX_EN > 0      */

    AF_PACKET_T *pafpacket = LW_NULL;
    INT          iRet      = PX_ERROR;
    INT          iFdNew    = PX_ERROR;
    
    INT          iCloExec;
    BOOL         iNonBlock;
    
    psock = (SOCKET_T *)iosFdValueType(s, &iType);
    
    __SOCKET_CHECHK();
    
    __THREAD_CANCEL_POINT();                                            /*  ����ȡ����                  */
    
    if (flags & SOCK_CLOEXEC) {                                         /*  SOCK_CLOEXEC ?              */
        iCloExec = FD_CLOEXEC;
    } else {
        iCloExec = 0;
    }
    
    if (flags & SOCK_NONBLOCK) {                                        /*  SOCK_NONBLOCK ?             */
        iNonBlock = 1;
    } else {
        iNonBlock = 0;
    }
    
    __KERNEL_SPACE_ENTER();
    switch (psock->SOCK_iFamily) {
    
#if LW_CFG_NET_UNIX_EN > 0
    case AF_UNIX:                                                       /*  UNIX ��Э��                 */
        pafunix = unix_accept(psock->SOCK_pafunix, addr, addrlen);
        if (pafunix == LW_NULL) {
            __KERNEL_SPACE_EXIT();
            goto    __error_handle;
        }
        break;
#endif                                                                  /*  LW_CFG_NET_UNIX_EN > 0      */
        
    case AF_PACKET:                                                     /*  PACKET                      */
        pafpacket = packet_accept(psock->SOCK_pafpacket, addr, addrlen);
        if (pafpacket == LW_NULL) {
            __KERNEL_SPACE_EXIT();
            goto    __error_handle;
        }
        break;
        
    default:
        iRet = lwip_accept(psock->SOCK_iLwipFd, addr, addrlen);         /*  lwip_accept                 */
        if (iRet < 0) {
            __KERNEL_SPACE_EXIT();
            goto    __error_handle;
        }
        break;
    }
    __KERNEL_SPACE_EXIT();
    
    iFdNew = open(LWIP_SYLIXOS_SOCKET_NAME, O_RDWR);                    /*  new fd                      */
    if (iFdNew < 0) {
        goto    __error_handle;
    }
    psockNew = (SOCKET_T *)iosFdValue(iFdNew);
    psockNew->SOCK_iFamily = psock->SOCK_iFamily;
    
    switch (psockNew->SOCK_iFamily) {
    
#if LW_CFG_NET_UNIX_EN > 0
    case AF_UNIX:                                                       /*  UNIX ��Э��                 */
        psockNew->SOCK_pafunix = pafunix;
        __unix_set_sockfile(pafunix, psockNew);
        break;
#endif                                                                  /*  LW_CFG_NET_UNIX_EN > 0      */

    case AF_PACKET:                                                     /*  PACKET                      */
        psockNew->SOCK_pafpacket = pafpacket;
        __packet_set_sockfile(pafpacket, psockNew);
        break;
        
    default:
        psockNew->SOCK_iLwipFd = iRet;                                  /*  save lwip fd                */
        __lwip_set_sockfile(iRet, psockNew);
        break;
    }
    
    if (iCloExec) {
        API_IosFdSetCloExec(iFdNew, iCloExec);
    }
    
    if (iNonBlock) {
        __KERNEL_SPACE_ENTER();
        __socketIoctl(psockNew, FIONBIO, &iNonBlock);
        __KERNEL_SPACE_EXIT();
    }
    
    MONITOR_EVT_INT2(MONITOR_EVENT_ID_NETWORK, MONITOR_EVENT_NETWORK_ACCEPT, 
                     s, iFdNew, LW_NULL);
    
    return  (iFdNew);
    
__error_handle:
    psock->SOCK_iSoErr = errno;
    
#if LW_CFG_NET_UNIX_EN > 0
    if (pafunix) {
        unix_close(pafunix);
    }
#endif                                                                  /*  LW_CFG_NET_UNIX_EN > 0      */
    
    if (pafpacket) {
        packet_close(pafpacket);
    
    } else if (iRet >= 0) {
        lwip_close(iRet);
    }
    
    return  (PX_ERROR);
}
/*********************************************************************************************************
** ��������: accept
** ��������: BSD accept
** �䡡��  : s         socket fd
**           addr      address
**           addrlen   address len
** �䡡��  : new socket fd
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
int  accept (int s, struct sockaddr *addr, socklen_t *addrlen)
{
    return  (accept4(s, addr, addrlen, 0));
}
/*********************************************************************************************************
** ��������: bind
** ��������: BSD bind
** �䡡��  : s         socket fd
**           name      address
**           namelen   address len
** �䡡��  : ERROR
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
int  bind (int s, const struct sockaddr *name, socklen_t namelen)
{
    SOCKET_T   *psock;
    INT         iType;
    INT         iRet = PX_ERROR;
    
    psock = (SOCKET_T *)iosFdValueType(s, &iType);
    
    __SOCKET_CHECHK();
    
    __KERNEL_SPACE_ENTER();
    switch (psock->SOCK_iFamily) {

#if LW_CFG_NET_UNIX_EN > 0
    case AF_UNIX:                                                       /*  UNIX ��Э��                 */
        iRet = unix_bind(psock->SOCK_pafunix, name, namelen);
        break;
#endif                                                                  /*  LW_CFG_NET_UNIX_EN > 0      */

    case AF_PACKET:                                                     /*  PACKET                      */
        iRet = packet_bind(psock->SOCK_pafpacket, name, namelen);
        break;
        
    default:
        iRet = lwip_bind(psock->SOCK_iLwipFd, name, namelen);
        break;
    }
    __KERNEL_SPACE_EXIT();
    
    if (iRet < ERROR_NONE) {
        psock->SOCK_iSoErr = errno;
    
    } else {
        MONITOR_EVT_INT1(MONITOR_EVENT_ID_NETWORK, MONITOR_EVENT_NETWORK_BIND, 
                         s, LW_NULL);
    }
    
    return  (iRet);
}
/*********************************************************************************************************
** ��������: bind
** ��������: BSD bind
** �䡡��  : s         socket fd
**           how       SHUT_RD  SHUT_RDWR  SHUT_WR
** �䡡��  : ERROR
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
int  shutdown (int s, int how)
{
    SOCKET_T   *psock;
    INT         iType;
    INT         iRet = PX_ERROR;
    
    psock = (SOCKET_T *)iosFdValueType(s, &iType);
    
    __SOCKET_CHECHK();
    
    __KERNEL_SPACE_ENTER();
    switch (psock->SOCK_iFamily) {
    
#if LW_CFG_NET_UNIX_EN > 0
    case AF_UNIX:                                                       /*  UNIX ��Э��                 */
        iRet = unix_shutdown(psock->SOCK_pafunix, how);
        break;
#endif                                                                  /*  LW_CFG_NET_UNIX_EN > 0      */

#if LW_CFG_NET_ROUTER > 0
    case AF_ROUTE:                                                      /*  ROUTE                       */
        iRet = route_shutdown(psock->SOCK_pafroute, how);
        break;
#endif                                                                  /*  LW_CFG_NET_ROUTER > 0       */

    case AF_PACKET:                                                     /*  PACKET                      */
        iRet = packet_shutdown(psock->SOCK_pafpacket, how);
        break;
        
    default:
        iRet = lwip_shutdown(psock->SOCK_iLwipFd, how);
        break;
    }
    __KERNEL_SPACE_EXIT();
    
    if (iRet < ERROR_NONE) {
        psock->SOCK_iSoErr = errno;
    
    } else {
        MONITOR_EVT_INT2(MONITOR_EVENT_ID_NETWORK, MONITOR_EVENT_NETWORK_SHUTDOWN, 
                         s, how, LW_NULL);
    }
    
    return  (iRet);
}
/*********************************************************************************************************
** ��������: connect
** ��������: BSD connect
** �䡡��  : s         socket fd
**           name      address
**           namelen   address len
** �䡡��  : ERROR
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
int  connect (int s, const struct sockaddr *name, socklen_t namelen)
{
    SOCKET_T   *psock;
    INT         iType;
    INT         iRet = PX_ERROR;
    
    psock = (SOCKET_T *)iosFdValueType(s, &iType);
    
    __SOCKET_CHECHK();
    
    __THREAD_CANCEL_POINT();                                            /*  ����ȡ����                  */
    
    __KERNEL_SPACE_ENTER();
    switch (psock->SOCK_iFamily) {
    
#if LW_CFG_NET_UNIX_EN > 0
    case AF_UNIX:                                                       /*  UNIX ��Э��                 */
        iRet = unix_connect(psock->SOCK_pafunix, name, namelen);
        break;
#endif                                                                  /*  LW_CFG_NET_UNIX_EN > 0      */

    case AF_PACKET:                                                     /*  PACKET                      */
        iRet = packet_connect(psock->SOCK_pafpacket, name, namelen);
        break;
        
    default:
        iRet = lwip_connect(psock->SOCK_iLwipFd, name, namelen);
        break;
    }
    __KERNEL_SPACE_EXIT();
    
    if (iRet < ERROR_NONE) {
        psock->SOCK_iSoErr = errno;
    
    } else {
        MONITOR_EVT_INT1(MONITOR_EVENT_ID_NETWORK, MONITOR_EVENT_NETWORK_CONNECT, 
                         s, LW_NULL);
    }
    
    return  (iRet);
}
/*********************************************************************************************************
** ��������: getsockname
** ��������: BSD getsockname
** �䡡��  : s         socket fd
**           name      address
**           namelen   address len
** �䡡��  : ERROR
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
int  getsockname (int s, struct sockaddr *name, socklen_t *namelen)
{
    SOCKET_T   *psock;
    INT         iType;
    INT         iRet = PX_ERROR;
    
    psock = (SOCKET_T *)iosFdValueType(s, &iType);
    
    __SOCKET_CHECHK();
    
    __KERNEL_SPACE_ENTER();
    switch (psock->SOCK_iFamily) {
    
#if LW_CFG_NET_UNIX_EN > 0
    case AF_UNIX:                                                       /*  UNIX ��Э��                 */
        iRet = unix_getsockname(psock->SOCK_pafunix, name, namelen);
        break;
#endif                                                                  /*  LW_CFG_NET_UNIX_EN > 0      */

    case AF_PACKET:                                                     /*  PACKET                      */
        iRet = packet_getsockname(psock->SOCK_pafpacket, name, namelen);
        break;
        
    default:
        iRet = lwip_getsockname(psock->SOCK_iLwipFd, name, namelen);
        break;
    }
    __KERNEL_SPACE_EXIT();
    
    if (iRet < ERROR_NONE) {
        psock->SOCK_iSoErr = errno;
    }
    
    return  (iRet);
}
/*********************************************************************************************************
** ��������: getpeername
** ��������: BSD getpeername
** �䡡��  : s         socket fd
**           name      address
**           namelen   address len
** �䡡��  : ERROR
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
int  getpeername (int s, struct sockaddr *name, socklen_t *namelen)
{
    SOCKET_T   *psock;
    INT         iType;
    INT         iRet = PX_ERROR;
    
    psock = (SOCKET_T *)iosFdValueType(s, &iType);
    
    __SOCKET_CHECHK();
    
    __KERNEL_SPACE_ENTER();
    switch (psock->SOCK_iFamily) {
    
#if LW_CFG_NET_UNIX_EN > 0
    case AF_UNIX:                                                       /*  UNIX ��Э��                 */
        iRet = unix_getpeername(psock->SOCK_pafunix, name, namelen);
        break;
#endif                                                                  /*  LW_CFG_NET_UNIX_EN > 0      */

    case AF_PACKET:                                                     /*  PACKET                      */
        iRet = packet_getpeername(psock->SOCK_pafpacket, name, namelen);
        break;
        
    default:
        iRet = lwip_getpeername(psock->SOCK_iLwipFd, name, namelen);
        break;
    }
    __KERNEL_SPACE_EXIT();
    
    if (iRet < ERROR_NONE) {
        psock->SOCK_iSoErr = errno;
    }
    
    return  (iRet);
}
/*********************************************************************************************************
** ��������: setsockopt
** ��������: BSD setsockopt
** �䡡��  : s         socket fd
**           level     level
**           optname   option
**           optval    option value
**           optlen    option value len
** �䡡��  : ERROR
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
int  setsockopt (int s, int level, int optname, const void *optval, socklen_t optlen)
{
    SOCKET_T   *psock;
    INT         iType;
    INT         iRet = PX_ERROR;
    
    psock = (SOCKET_T *)iosFdValueType(s, &iType);
    
    __SOCKET_CHECHK();
    
    __KERNEL_SPACE_ENTER();
    switch (psock->SOCK_iFamily) {
    
#if LW_CFG_NET_UNIX_EN > 0
    case AF_UNIX:                                                       /*  UNIX ��Э��                 */
        iRet = unix_setsockopt(psock->SOCK_pafunix, level, optname, optval, optlen);
        break;
#endif                                                                  /*  LW_CFG_NET_UNIX_EN > 0      */

#if LW_CFG_NET_ROUTER > 0
    case AF_ROUTE:                                                      /*  ROUTE                       */
        iRet = route_setsockopt(psock->SOCK_pafroute, level, optname, optval, optlen);
        break;
#endif                                                                  /*  LW_CFG_NET_ROUTER > 0       */

    case AF_PACKET:                                                     /*  PACKET                      */
        iRet = packet_setsockopt(psock->SOCK_pafpacket, level, optname, optval, optlen);
        break;
        
    default:
        iRet = lwip_setsockopt(psock->SOCK_iLwipFd, level, optname, optval, optlen);
        break;
    }
    __KERNEL_SPACE_EXIT();
    
    if (iRet < ERROR_NONE) {
        psock->SOCK_iSoErr = errno;
    
    } else {
        MONITOR_EVT_INT3(MONITOR_EVENT_ID_NETWORK, MONITOR_EVENT_NETWORK_SOCKOPT, 
                         s, level, optname, LW_NULL);
    }
    
    return  (iRet);
}
/*********************************************************************************************************
** ��������: getsockopt
** ��������: BSD getsockopt
** �䡡��  : s         socket fd
**           level     level
**           optname   option
**           optval    option value
**           optlen    option value len
** �䡡��  : ERROR
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
int  getsockopt (int s, int level, int optname, void *optval, socklen_t *optlen)
{
    SYS_ARCH_DECL_PROTECT(lev);
    SOCKET_T   *psock;
    INT         iType;
    INT         iRet = PX_ERROR;
    
    psock = (SOCKET_T *)iosFdValueType(s, &iType);
    
    __SOCKET_CHECHK();
    
    if (level == SOL_SOCKET) {                                          /*  ͳһ���� SO_ERROR           */
        if (optname == SO_ERROR) {
            if (!optval || *optlen < sizeof(INT)) {
                _ErrorHandle(EINVAL);
                return  (iRet);
            }

            SYS_ARCH_PROTECT(lev);
            *(INT *)optval = psock->SOCK_iSoErr;
            psock->SOCK_iSoErr = ERROR_NONE;
            SYS_ARCH_UNPROTECT(lev);

            return  (ERROR_NONE);
        }
    }
    
    __KERNEL_SPACE_ENTER();
    switch (psock->SOCK_iFamily) {
    
#if LW_CFG_NET_UNIX_EN > 0
    case AF_UNIX:                                                       /*  UNIX ��Э��                 */
        iRet = unix_getsockopt(psock->SOCK_pafunix, level, optname, optval, optlen);
        break;
#endif                                                                  /*  LW_CFG_NET_UNIX_EN > 0      */

#if LW_CFG_NET_ROUTER > 0
    case AF_ROUTE:                                                      /*  ROUTE                       */
        iRet = route_getsockopt(psock->SOCK_pafroute, level, optname, optval, optlen);
        break;
#endif                                                                  /*  LW_CFG_NET_ROUTER > 0       */
    
    case AF_PACKET:                                                     /*  PACKET                      */
        iRet = packet_getsockopt(psock->SOCK_pafpacket, level, optname, optval, optlen);
        break;
        
    default:
        iRet = lwip_getsockopt(psock->SOCK_iLwipFd, level, optname, optval, optlen);
        break;
    }
    __KERNEL_SPACE_EXIT();
    
    if (iRet < ERROR_NONE) {
        psock->SOCK_iSoErr = errno;
    }
    
    return  (iRet);
}
/*********************************************************************************************************
** ��������: listen
** ��������: BSD listen
** �䡡��  : s         socket fd
**           backlog   back log num
** �䡡��  : ERROR
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
int listen (int s, int backlog)
{
    SOCKET_T   *psock;
    INT         iType;
    INT         iRet = PX_ERROR;
    
    psock = (SOCKET_T *)iosFdValueType(s, &iType);
    
    __SOCKET_CHECHK();
    
    __KERNEL_SPACE_ENTER();
    switch (psock->SOCK_iFamily) {
    
#if LW_CFG_NET_UNIX_EN > 0
    case AF_UNIX:                                                       /*  UNIX ��Э��                 */
        iRet = unix_listen(psock->SOCK_pafunix, backlog);
        break;
#endif                                                                  /*  LW_CFG_NET_UNIX_EN > 0      */

    case AF_PACKET:                                                     /*  PACKET                      */
        iRet = packet_listen(psock->SOCK_pafpacket, backlog);
        break;
        
    default:
        iRet = lwip_listen(psock->SOCK_iLwipFd, backlog);
        break;
    }
    __KERNEL_SPACE_EXIT();
    
    if (iRet < ERROR_NONE) {
        psock->SOCK_iSoErr = errno;
    
    } else {
        MONITOR_EVT_INT2(MONITOR_EVENT_ID_NETWORK, MONITOR_EVENT_NETWORK_LISTEN, 
                         s, backlog, LW_NULL);
    }
    
    return  (iRet);
}
/*********************************************************************************************************
** ��������: recv
** ��������: BSD recv
** �䡡��  : s         socket fd
**           mem       buffer
**           len       buffer len
**           flags     flag
** �䡡��  : NUM
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
ssize_t  recv (int s, void *mem, size_t len, int flags)
{
    SOCKET_T   *psock;
    INT         iType;
    ssize_t     sstRet = PX_ERROR;
    
    psock = (SOCKET_T *)iosFdValueType(s, &iType);
    
    __SOCKET_CHECHK();
    
    __THREAD_CANCEL_POINT();                                            /*  ����ȡ����                  */
    
    __KERNEL_SPACE_ENTER();
    switch (psock->SOCK_iFamily) {
    
#if LW_CFG_NET_UNIX_EN > 0
    case AF_UNIX:                                                       /*  UNIX ��Э��                 */
        sstRet = (ssize_t)unix_recv(psock->SOCK_pafunix, mem, len, flags);
        break;
#endif                                                                  /*  LW_CFG_NET_UNIX_EN > 0      */

#if LW_CFG_NET_ROUTER > 0
    case AF_ROUTE:                                                      /*  ROUTE                       */
        sstRet = (ssize_t)route_recv(psock->SOCK_pafroute, mem, len, flags);
        break;
#endif                                                                  /*  LW_CFG_NET_ROUTER > 0       */

    case AF_PACKET:                                                     /*  PACKET                      */
        sstRet = (ssize_t)packet_recv(psock->SOCK_pafpacket, mem, len, flags);
        break;
        
    default:
        sstRet = (ssize_t)lwip_recv(psock->SOCK_iLwipFd, mem, len, flags);
        break;
    }
    __KERNEL_SPACE_EXIT();
    
    if (sstRet <= 0) {
        psock->SOCK_iSoErr = errno;
    
    } else {
        MONITOR_EVT_LONG3(MONITOR_EVENT_ID_NETWORK, MONITOR_EVENT_NETWORK_RECV, 
                          s, flags, sstRet, LW_NULL);
    }
    
    return  (sstRet);
}
/*********************************************************************************************************
** ��������: recvfrom
** ��������: BSD recvfrom
** �䡡��  : s         socket fd
**           mem       buffer
**           len       buffer len
**           flags     flag
**           from      packet from
**           fromlen   name len
** �䡡��  : NUM
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
ssize_t  recvfrom (int s, void *mem, size_t len, int flags,
                   struct sockaddr *from, socklen_t *fromlen)
{
    SOCKET_T   *psock;
    INT         iType;
    ssize_t     sstRet = PX_ERROR;
    
    psock = (SOCKET_T *)iosFdValueType(s, &iType);
    
    __SOCKET_CHECHK();
    
    __THREAD_CANCEL_POINT();                                            /*  ����ȡ����                  */
    
    __KERNEL_SPACE_ENTER();
    switch (psock->SOCK_iFamily) {
    
#if LW_CFG_NET_UNIX_EN > 0
    case AF_UNIX:                                                       /*  UNIX ��Э��                 */
        sstRet = (ssize_t)unix_recvfrom(psock->SOCK_pafunix, mem, len, flags, from, fromlen);
        break;
#endif                                                                  /*  LW_CFG_NET_UNIX_EN > 0      */

#if LW_CFG_NET_ROUTER > 0
    case AF_ROUTE:                                                      /*  ROUTE                       */
        sstRet = (ssize_t)route_recv(psock->SOCK_pafroute, mem, len, flags);
        break;
#endif                                                                  /*  LW_CFG_NET_ROUTER > 0       */

    case AF_PACKET:                                                     /*  PACKET                      */
        sstRet = (ssize_t)packet_recvfrom(psock->SOCK_pafpacket, mem, len, flags, from, fromlen);
        break;
        
    default:
        sstRet = (ssize_t)lwip_recvfrom(psock->SOCK_iLwipFd, mem, len, flags, from, fromlen);
        break;
    }
    __KERNEL_SPACE_EXIT();
    
    if (sstRet <= 0) {
        psock->SOCK_iSoErr = errno;
    
    } else {
        MONITOR_EVT_LONG3(MONITOR_EVENT_ID_NETWORK, MONITOR_EVENT_NETWORK_RECV, 
                          s, flags, sstRet, LW_NULL);
    }
    
    return  (sstRet);
}
/*********************************************************************************************************
** ��������: recvmsg
** ��������: BSD recvmsg
** �䡡��  : s             �׽���
**           msg           ��Ϣ
**           flags         �����־
** �䡡��  : NUM (�˳��Ȳ�����������Ϣ����)
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API 
ssize_t  recvmsg (int  s, struct msghdr *msg, int flags)
{
    SOCKET_T   *psock;
    INT         iType;
    ssize_t     sstRet = PX_ERROR;
    
    psock = (SOCKET_T *)iosFdValueType(s, &iType);
    
    __SOCKET_CHECHK();
    
    __THREAD_CANCEL_POINT();                                            /*  ����ȡ����                  */
    
    __KERNEL_SPACE_ENTER();
    switch (psock->SOCK_iFamily) {
    
#if LW_CFG_NET_UNIX_EN > 0
    case AF_UNIX:                                                       /*  UNIX ��Э��                 */
        sstRet = (ssize_t)unix_recvmsg(psock->SOCK_pafunix, msg, flags);
        break;
#endif                                                                  /*  LW_CFG_NET_UNIX_EN > 0      */

#if LW_CFG_NET_ROUTER > 0
    case AF_ROUTE:                                                      /*  ROUTE                       */
        sstRet = (ssize_t)route_recvmsg(psock->SOCK_pafroute, msg, flags);
        break;
#endif                                                                  /*  LW_CFG_NET_ROUTER > 0       */

    case AF_PACKET:                                                     /*  PACKET                      */
        sstRet = (ssize_t)packet_recvmsg(psock->SOCK_pafpacket, msg, flags);
        break;
        
    default:
        sstRet = (ssize_t)lwip_recvmsg(psock->SOCK_iLwipFd, msg, flags);
        break;
    }
    __KERNEL_SPACE_EXIT();
    
    if (sstRet <= 0) {
        psock->SOCK_iSoErr = errno;
    
    } else {
        MONITOR_EVT_LONG3(MONITOR_EVENT_ID_NETWORK, MONITOR_EVENT_NETWORK_RECV, 
                          s, flags, sstRet, LW_NULL);
    }
    
    return  (sstRet);
}
/*********************************************************************************************************
** ��������: send
** ��������: BSD send
** �䡡��  : s         socket fd
**           data      send buffer
**           size      send len
**           flags     flag
** �䡡��  : NUM
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
ssize_t  send (int s, const void *data, size_t size, int flags)
{
    SOCKET_T   *psock;
    INT         iType;
    ssize_t     sstRet = PX_ERROR;
    
    psock = (SOCKET_T *)iosFdValueType(s, &iType);
    
    __SOCKET_CHECHK();
    
    __THREAD_CANCEL_POINT();                                            /*  ����ȡ����                  */
    
    __KERNEL_SPACE_ENTER();
    switch (psock->SOCK_iFamily) {
    
#if LW_CFG_NET_UNIX_EN > 0
    case AF_UNIX:                                                       /*  UNIX ��Э��                 */
        sstRet = (ssize_t)unix_send(psock->SOCK_pafunix, data, size, flags);
        break;
#endif                                                                  /*  LW_CFG_NET_UNIX_EN > 0      */

#if LW_CFG_NET_ROUTER > 0
    case AF_ROUTE:                                                      /*  ROUTE                       */
        sstRet = (ssize_t)route_send(psock->SOCK_pafroute, data, size, flags);
        break;
#endif                                                                  /*  LW_CFG_NET_ROUTER > 0       */

    case AF_PACKET:                                                     /*  PACKET                      */
        sstRet = (ssize_t)packet_send(psock->SOCK_pafpacket, data, size, flags);
        break;
        
    default:
        sstRet = (ssize_t)lwip_send(psock->SOCK_iLwipFd, data, size, flags);
        break;
    }
    __KERNEL_SPACE_EXIT();
    
    if (sstRet <= 0) {
        psock->SOCK_iSoErr = errno;
    
    } else {
        MONITOR_EVT_LONG3(MONITOR_EVENT_ID_NETWORK, MONITOR_EVENT_NETWORK_SEND, 
                          s, flags, sstRet, LW_NULL);
    }
    
    return  (sstRet);
}
/*********************************************************************************************************
** ��������: sendto
** ��������: BSD sendto
** �䡡��  : s         socket fd
**           data      send buffer
**           size      send len
**           flags     flag
**           to        packet to
**           tolen     name len
** �䡡��  : NUM
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
ssize_t  sendto (int s, const void *data, size_t size, int flags,
                 const struct sockaddr *to, socklen_t tolen)
{
    SOCKET_T   *psock;
    INT         iType;
    ssize_t     sstRet = PX_ERROR;
    
    psock = (SOCKET_T *)iosFdValueType(s, &iType);
    
    __SOCKET_CHECHK();
    
    __KERNEL_SPACE_ENTER();
    switch (psock->SOCK_iFamily) {
    
#if LW_CFG_NET_UNIX_EN > 0
    case AF_UNIX:                                                       /*  UNIX ��Э��                 */
        sstRet = (ssize_t)unix_sendto(psock->SOCK_pafunix, data, size, flags, to, tolen);
        break;
#endif                                                                  /*  LW_CFG_NET_UNIX_EN > 0      */

#if LW_CFG_NET_ROUTER > 0
    case AF_ROUTE:                                                      /*  ROUTE                       */
        sstRet = (ssize_t)route_send(psock->SOCK_pafroute, data, size, flags);
        break;
#endif                                                                  /*  LW_CFG_NET_ROUTER > 0       */

    case AF_PACKET:                                                     /*  PACKET                      */
        sstRet = (ssize_t)packet_sendto(psock->SOCK_pafpacket, data, size, flags, to, tolen);
        break;
        
    default:
        sstRet = (ssize_t)lwip_sendto(psock->SOCK_iLwipFd, data, size, flags, to, tolen);
        break;
    }
    __KERNEL_SPACE_EXIT();
    
    if (sstRet <= 0) {
        psock->SOCK_iSoErr = errno;
    
    } else {
        MONITOR_EVT_LONG3(MONITOR_EVENT_ID_NETWORK, MONITOR_EVENT_NETWORK_SEND, 
                          s, flags, sstRet, LW_NULL);
    }
    
    return  (sstRet);
}
/*********************************************************************************************************
** ��������: sendmsg
** ��������: BSD sendmsg
** �䡡��  : s             �׽���
**           msg           ��Ϣ
**           flags         �����־
** �䡡��  : NUM (�˳��Ȳ�����������Ϣ����)
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API 
ssize_t  sendmsg (int  s, const struct msghdr *msg, int flags)
{
    SOCKET_T   *psock;
    INT         iType;
    ssize_t     sstRet = PX_ERROR;
    
    psock = (SOCKET_T *)iosFdValueType(s, &iType);
    
    __SOCKET_CHECHK();
    
    __THREAD_CANCEL_POINT();                                            /*  ����ȡ����                  */
    
    __KERNEL_SPACE_ENTER();
    switch (psock->SOCK_iFamily) {
    
#if LW_CFG_NET_UNIX_EN > 0
    case AF_UNIX:                                                       /*  UNIX ��Э��                 */
        sstRet = (ssize_t)unix_sendmsg(psock->SOCK_pafunix, msg, flags);
        break;
#endif                                                                  /*  LW_CFG_NET_UNIX_EN > 0      */

#if LW_CFG_NET_ROUTER > 0
    case AF_ROUTE:                                                      /*  ROUTE                       */
        sstRet = (ssize_t)route_sendmsg(psock->SOCK_pafroute, msg, flags);
        break;
#endif                                                                  /*  LW_CFG_NET_ROUTER > 0       */

    case AF_PACKET:                                                     /*  PACKET                      */
        sstRet = (ssize_t)packet_sendmsg(psock->SOCK_pafpacket, msg, flags);
        break;
        
    default:
        sstRet = (ssize_t)lwip_sendmsg(psock->SOCK_iLwipFd, msg, flags);
        break;
    }
    __KERNEL_SPACE_EXIT();
    
    if (sstRet <= 0) {
        psock->SOCK_iSoErr = errno;
    
    } else {
        MONITOR_EVT_LONG3(MONITOR_EVENT_ID_NETWORK, MONITOR_EVENT_NETWORK_SEND, 
                          s, flags, sstRet, LW_NULL);
    }
    
    return  (sstRet);
}
/*********************************************************************************************************
** ��������: gethostbyname
** ��������: BSD gethostbyname
** �䡡��  : name      domain name
** �䡡��  : hostent
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
struct hostent  *gethostbyname (const char *name)
{
    return  (lwip_gethostbyname(name));
}
/*********************************************************************************************************
** ��������: gethostbyname_r
** ��������: BSD gethostbyname_r
** �䡡��  : name      domain name
**           ret       hostent buffer
**           buf       result buffer
**           buflen    buf len
**           result    result return
**           h_errnop  error
** �䡡��  : ERROR
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
int gethostbyname_r (const char *name, struct hostent *ret, char *buf,
                     size_t buflen, struct hostent **result, int *h_errnop)
{
    return  (lwip_gethostbyname_r(name, ret, buf, buflen, result, h_errnop));
}
/*********************************************************************************************************
** ��������: gethostbyaddr_r
** ��������: BSD gethostbyname_r
** �䡡��  : addr      domain addr
**           length    socketaddr len
**           type      AF_INET
** �䡡��  : hostent
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
struct hostent *gethostbyaddr (const void *addr, socklen_t length, int type)
{
    errno = ENOSYS;
    return  (LW_NULL);
}
/*********************************************************************************************************
** ��������: gethostbyaddr_r
** ��������: BSD gethostbyname_r
** �䡡��  : addr      domain addr
**           length    socketaddr len
**           type      AF_INET
**           ret       hostent buffer
**           buf       result buffer
**           buflen    buf len
**           result    result return
**           h_errnop  error
** �䡡��  : hostent
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
struct hostent *gethostbyaddr_r (const void *addr, socklen_t length, int type,
                                 struct hostent *ret, char  *buffer, int buflen, int *h_errnop)
{
    errno = ENOSYS;
    return  (LW_NULL);
}
/*********************************************************************************************************
** ��������: freeaddrinfo
** ��������: BSD freeaddrinfo
** �䡡��  : ai        addrinfo
** �䡡��  : ERROR
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
void  freeaddrinfo (struct addrinfo *ai)
{
    lwip_freeaddrinfo(ai);
}
/*********************************************************************************************************
** ��������: getaddrinfo
** ��������: BSD getaddrinfo
** �䡡��  : nodename  node name
**           servname  server name
**           hints     addrinfo
**           res       result
** �䡡��  : ERROR
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
int  getaddrinfo (const char *nodename, const char *servname,
                  const struct addrinfo *hints, struct addrinfo **res)
{
    return  (lwip_getaddrinfo(nodename, servname, hints, res));
}
/*********************************************************************************************************
** ��������: get_dns_server_info_4
** ��������: get lwip dns server for IPv4
** �䡡��  : iIndex   dns server index
**           inaddr   DNS Server
** �䡡��  : ERROR or OK
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
INT  get_dns_server_info_4 (UINT iIndex, struct in_addr *inaddr)
{
    const ip_addr_t *ipdns;

    if ((iIndex >= DNS_MAX_SERVERS) || !inaddr) {
        return  (PX_ERROR);
    }
    
    ipdns = dns_getserver((u8_t)iIndex);
    if (!ipdns || (IP_GET_TYPE(ipdns) == IPADDR_TYPE_V6)) {
        return  (PX_ERROR);
    }
    
    inet_addr_from_ip4addr(inaddr, ip_2_ip4(ipdns));

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: get_dns_server_info_6
** ��������: get lwip dns server for IPv6
** �䡡��  : iIndex   dns server index
**           in6addr  DNS Server
** �䡡��  : ERROR or OK
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
#if LWIP_IPV6

LW_API  
INT  get_dns_server_info_6 (UINT iIndex, struct in6_addr *in6addr)
{
    const ip_addr_t *ipdns;

    if ((iIndex >= DNS_MAX_SERVERS) || !in6addr) {
        return  (PX_ERROR);
    }
    
    ipdns = dns_getserver((u8_t)iIndex);
    if (!ipdns || (IP_GET_TYPE(ipdns) == IPADDR_TYPE_V4)) {
        return  (PX_ERROR);
    }
    
    inet6_addr_from_ip6addr(in6addr, ip_2_ip6(ipdns));

    return  (ERROR_NONE);
}

#endif                                                                  /*  LWIP_IPV6                   */
#endif                                                                  /*  LW_CFG_NET_EN               */
/*********************************************************************************************************
  END
*********************************************************************************************************/
