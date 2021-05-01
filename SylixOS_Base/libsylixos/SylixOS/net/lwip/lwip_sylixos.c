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
** ��   ��   ��: lwip_sylixos.c
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2009 �� 05 �� 06 ��
**
** ��        ��: lwip sylixos �ӿ�.

** BUG:
2009.05.20  _netJobqueueInit ��ʼ��Ӧ���������ʼ���ĺ���, ���Ӱ�ȫ.
2009.07.11  API_NetInit() �������ʼ��һ��.
2009.08.19  ȥ�� snmp init ����������� lwip 1.3.1 �Ժ�汾���Զ���ʼ��.
2010.02.22  ���� lwip ��ȥ�� loopif �ĳ�ʼ��(Э��ջ���а�װ�ػ�����).
2010.07.28  ���� snmp ʱ����ص��Ͷ����ĳ�ʼ������.
2010.11.01  __netCloseAll() ���� DHCP ����ʹ�ð�ȫģʽ.
2012.12.18  ���� unix_init() ��ʼ�� AF_UNIX ��Э��.
2013.06.21  �������� proc �ļ�.
2016.10.21  ��ϵͳ��������.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "SylixOS.h"
/*********************************************************************************************************
  �ü�����
*********************************************************************************************************/
#if LW_CFG_NET_EN > 0
#include "net/if_lock.h"
#include "net/if_flags.h"
#include "lwip/tcpip.h"
#include "lwip/inet.h"
#include "lwip/netif.h"
#include "lwip/netifapi.h"
#include "lwip/dhcp.h"
#include "lwip/sockets.h"
#include "lwip/snmp/snmp.h"
#include "lwip/snmp/snmp_mib2.h"
/*********************************************************************************************************
  TCP Ext
*********************************************************************************************************/
#if LW_CFG_LWIP_TCP_SIG_EN > 0
#include "tcpsig/tcp_md5.h"
#endif                                                                  /*  LW_CFG_LWIP_TCP_SIG_EN > 0  */
/*********************************************************************************************************
  �ڲ�ͷ�ļ�
*********************************************************************************************************/
#if LWIP_SNMP > 0 && LWIP_SNMP_V3 > 0
#include "./tools/snmp/snmpv3_sylixos.h"
#endif                                                                  /*  LWIP_SNMP && LWIP_SNMP_V3   */
#include "./unix/af_unix.h"
#include "./route/af_route.h"
#include "./packet/af_packet.h"
#if LW_CFG_PROCFS_EN > 0
#include "./proc/lwip_proc.h"
#endif                                                                  /*  LW_CFG_PROCFS_EN            */
/*********************************************************************************************************
  ���綯̬�ڴ����
*********************************************************************************************************/
#if LW_CFG_LWIP_MEM_TLSF > 0
extern void tlsf_mem_create(void);
#endif                                                                  /*  LW_CFG_LWIP_MEM_TLSF        */
/*********************************************************************************************************
  ���������������к�������
*********************************************************************************************************/
extern INT  _netJobqueueInit(VOID);
/*********************************************************************************************************
  ���纯������
*********************************************************************************************************/
#if LW_CFG_SHELL_EN > 0
extern VOID __tshellNetInit(VOID);
extern VOID __tshellNet6Init(VOID);
#endif                                                                  /*  LW_CFG_SHELL_EN > 0         */
/*********************************************************************************************************
  ��������������
*********************************************************************************************************/
extern VOID __inetHostTableInit(VOID);
/*********************************************************************************************************
  �����¼���ʼ��
*********************************************************************************************************/
extern INT  _netEventInit(VOID);
/*********************************************************************************************************
  ���������豸֧��
*********************************************************************************************************/
#if LW_CFG_NET_VNETDEV_EN > 0
extern INT  _netVndInit(VOID);
#endif                                                                  /*  LW_CFG_NET_VNETDEV_EN > 0   */
/*********************************************************************************************************
  VLAN ֧��
*********************************************************************************************************/
#if LW_CFG_NET_VLAN_EN > 0
extern VOID __netVlanInit(VOID);
#endif                                                                  /*  LW_CFG_NET_VLAN_EN > 0      */
/*********************************************************************************************************
  ����֧��
*********************************************************************************************************/
#if LW_CFG_NET_DEV_BRIDGE_EN > 0
extern INT  _netBridgeInit(VOID);
#endif                                                                  /*  LW_CFG_NET_DEV_BRIDGE_EN    */
/*********************************************************************************************************
  Bonding ֧��
*********************************************************************************************************/
#if LW_CFG_NET_DEV_BONDING_EN > 0
extern INT  _netBondingInit(VOID);
#endif                                                                  /*  LW_CFG_NET_DEV_BONDING_EN   */
/*********************************************************************************************************
  �������纯������
*********************************************************************************************************/
#if LW_CFG_LWIP_PPP > 0 || LW_CFG_LWIP_PPPOE > 0
#if __LWIP_USE_PPP_NEW == 0
extern err_t pppInit(void);
#endif
#endif                                                                  /*  LW_CFG_LWIP_PPP             */
                                                                        /*  LW_CFG_LWIP_PPPOE           */
/*********************************************************************************************************
  NET libc lock
*********************************************************************************************************/
LW_OBJECT_HANDLE    _G_ulNetLibcLock;
/*********************************************************************************************************
** ��������: __netCloseAll
** ��������: ϵͳ������ر�ʱ�ص�����.
** �䡡��  : NONE
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static VOID  __netCloseAll (VOID)
{
#if LWIP_DHCP > 0
    struct netif  *netif;

    NETIF_FOREACH(netif) {
        if (netif_dhcp_data(netif)) {
            netifapi_dhcp_release_and_stop(netif);                      /*  ��� DHCP ��Լ, ͬʱֹͣ����*/
        }
    }
#endif                                                                  /*  LWIP_DHCP > 0               */
}
/*********************************************************************************************************
** ��������: snmp_mib2_threadsync
** ��������: SNMP mib2 �߳�ͬ���ص�����.
** �䡡��  : pfunc     �ص�����
**           pvArg     �ص�����
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
#if LWIP_SNMP > 0

static VOID  snmp_mib2_threadsync (snmp_threadsync_called_fn  pfunc, PVOID  pvArg)
{
    if (pfunc) {
        pfunc(pvArg);
    }
}
/*********************************************************************************************************
** ��������: __netSnmpInit
** ��������: ��ʼ�� SNMP Ĭ����Ϣ.
** �䡡��  : NONE
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static VOID  __netSnmpInit (VOID)
{
    static const u16_t  ucContactLen  = 19;
    static const u16_t  ucLocationLen = 6;
    
    static const u16_t  ucDesrLen = 7;
    static const u16_t  ucNameLen = 23;
    
    snmp_mib2_set_syscontact_readonly((u8_t *)"acoinfo@acoinfo.com", &ucContactLen);
    snmp_mib2_set_syslocation_readonly((u8_t *)"@china", &ucLocationLen);
                                                                        /*  at CHINA ^_^                */
    snmp_mib2_set_sysdescr((u8_t *)"sylixos", &ucDesrLen);
    snmp_mib2_set_sysname_readonly((u8_t *)"device based on sylixos", &ucNameLen);

    snmp_threadsync_init(&snmp_mib2_lwip_locks, snmp_mib2_threadsync);
}

#endif                                                                  /*  LWIP_SNMP > 0               */
/*********************************************************************************************************
** ��������: __netLibcInit
** ��������: ��ʼ�� net libc.
** �䡡��  : NONE
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static VOID  __netLibcInit (VOID)
{
    FILE   *fp;
    
    if (access("/etc/hosts", R_OK) < 0) {
        if ((fp = fopen("/etc/hosts", "w")) != NULL) {
            fprintf(fp, "127.0.0.1    localhost\n");
            fclose(fp);
        }
    }
    
    _G_ulNetLibcLock = API_SemaphoreMCreate("net_libc", LW_PRIO_DEF_CEILING,
                                            LW_OPTION_INHERIT_PRIORITY | LW_OPTION_DELETE_SAFE |
                                            LW_OPTION_OBJECT_GLOBAL, LW_NULL);
    _BugHandle(!_G_ulNetLibcLock, LW_TRUE, "can not create NET Libc lock.\r\n");
}
/*********************************************************************************************************
** ��������: API_NetInit
** ��������: �����ϵͳ�ں�ע���������
** �䡡��  : NONE
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
VOID  API_NetInit (VOID)
{
    static BOOL   bIsInit = LW_FALSE;
    
    if (bIsInit) {
        return;
    }
    
    bIsInit = LW_TRUE;
    
    if_list_init();                                                     /*  ����ӿ�������ʼ��          */
    
#if LW_CFG_LWIP_MEM_TLSF > 0
    tlsf_mem_create();
#endif                                                                  /*  LW_CFG_LWIP_MEM_TLSF        */
    
#if LW_CFG_NET_VLAN_EN > 0
    __netVlanInit();                                                    /*  ��ʼ�� vlan                 */
#endif                                                                  /*  LW_CFG_NET_VLAN_EN > 0      */
    
    _netJobqueueInit();                                                 /*  ��������������ҵ�������    */
    
    _netEventInit();                                                    /*  ��ʼ�������¼�              */
    
    API_SystemHookAdd((LW_HOOK_FUNC)__netCloseAll, 
                      LW_OPTION_KERNEL_REBOOT);                         /*  ��װϵͳ�ر�ʱ, �ص�����    */

    netif_callback_init();                                              /*  ��������ӿڻص�            */

    tcpip_init(LW_NULL, LW_NULL);                                       /*  �Զ�������ʽ��ʼ�� lwip     */
    
#if LW_CFG_LWIP_TCP_SIG_EN > 0
    tcp_md5_init();
#endif                                                                  /*  LW_CFG_LWIP_TCP_SIG_EN > 0  */

#if LW_CFG_NET_UNIX_EN > 0
    unix_init();                                                        /*  ��ʼ�� AF_UNIX ��Э��       */
#endif                                                                  /*  LW_CFG_NET_UNIX_EN > 0      */

#if LW_CFG_NET_ROUTER > 0
    route_init();
#endif                                                                  /*  LW_CFG_NET_ROUTER > 0       */

    packet_init();                                                      /*  ��ʼ�� AF_PACKET Э��       */
    
    __socketInit();                                                     /*  ��ʼ�� socket ϵͳ          */

#if LW_CFG_NET_VNETDEV_EN > 0
    _netVndInit();                                                      /*  ��ʼ�����������豸          */
#endif                                                                  /*  LW_CFG_NET_VNETDEV_EN > 0   */

#if LW_CFG_LWIP_PPP > 0 || LW_CFG_LWIP_PPPOE > 0
#if __LWIP_USE_PPP_NEW == 0
    pppInit();                                                          /*  ��ʼ�� point to point proto */
#endif                                                                  /*  !__LWIP_USE_PPP_NEW         */
#endif                                                                  /*  LW_CFG_LWIP_PPP             */
                                                                        /*  LW_CFG_LWIP_PPPOE           */
#if LWIP_SNMP > 0
    __netSnmpInit();                                                    /*  ��ʼ�� SNMP ������Ϣ        */
#endif                                                                  /*  LWIP_SNMP > 0               */
    
    __netLibcInit();
    
#if LW_CFG_SHELL_EN > 0
    __tshellNetInit();                                                  /*  ע����������                */
#if LWIP_IPV6
    __tshellNet6Init();                                                 /*  ע�� IPv6 ר������          */
#endif
#endif                                                                  /*  LW_CFG_SHELL_EN > 0         */
    
#if LW_CFG_NET_DEV_BRIDGE_EN > 0
    _netBridgeInit();
#endif                                                                  /*  LW_CFG_NET_DEV_BRIDGE_EN    */
#if LW_CFG_NET_DEV_BONDING_EN > 0
    _netBondingInit();
#endif                                                                  /*  LW_CFG_NET_DEV_BONDING_EN   */

    /*
     *  ������ع��߳�ʼ��.
     */
    __inetHostTableInit();                                              /*  ��ʼ�����ص�ַת����        */
    
#if LW_CFG_PROCFS_EN > 0
    __procFsNetInit();
#endif                                                                  /*  LW_CFG_PROCFS_EN > 0        */
}
/*********************************************************************************************************
** ��������: API_NetSnmpInit
** ��������: ���� SNMP ����
** �䡡��  : NONE
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
VOID  API_NetSnmpInit (VOID)
{
#if LWIP_SNMP > 0
    snmp_init();

#if LWIP_SNMP_V3 > 0
    snmpv3_sylixos_init();
#endif                                                                  /*  LWIP_SNMP_V3 > 0            */

    snmp_v1_enable(LW_CFG_NET_SNMP_V1_EN);
    snmp_v2c_enable(LW_CFG_NET_SNMP_V2_EN);
    snmp_v3_enable(LW_CFG_NET_SNMP_V3_EN);
#endif                                                                  /*  LWIP_SNMP > 0               */
}

#endif                                                                  /*  LW_CFG_NET_EN               */
/*********************************************************************************************************
  END
*********************************************************************************************************/
