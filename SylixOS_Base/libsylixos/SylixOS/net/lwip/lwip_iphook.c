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
** ��   ��   ��: lwip_iphook.c
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2017 �� 05 �� 12 ��
**
** ��        ��: lwip IP HOOK.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
#include "../SylixOS/system/include/s_system.h"
/*********************************************************************************************************
  �ü�����
*********************************************************************************************************/
#if LW_CFG_NET_EN > 0
#include "lwip/tcpip.h"
#include "net/if.h"
#include "net/if_flags.h"
#include "lwip_iphook.h"
/*********************************************************************************************************
  �ص���
*********************************************************************************************************/
typedef struct {
    LW_LIST_LINE        IPHOOK_lineManage;
    FUNCPTR             IPHOOK_pfuncHook;
    CHAR                IPHOOK_cName[1];
} IP_HOOK_NODE;
typedef IP_HOOK_NODE   *PIP_HOOK_NODE;
/*********************************************************************************************************
  �ص���
*********************************************************************************************************/
static LW_LIST_LINE_HEADER   _G_plineIpHook;
static struct pbuf        *(*_G_pfuncIpHookNat)();
static struct netif       *(*_G_pfuncIpHookRoute)();
/*********************************************************************************************************
** ��������: lwip_ip_hook
** ��������: lwip ip �ص�����
** �䡡��  : ip_type       ip ���� IP_HOOK_V4 / IP_HOOK_V6
**           hook_type     �ص����� IP_HT_PRE_ROUTING / IP_HT_POST_ROUTING / IP_HT_LOCAL_IN ...
**           p             ���ݰ�
**           in            ��������ӿ�
**           out           �������ӿ�
** �䡡��  : 1: ����
**           0: ͨ��
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
int  lwip_ip_hook (int ip_type, int hook_type, struct pbuf *p, struct netif *in, struct netif *out)
{
    PLW_LIST_LINE   pline;
    PIP_HOOK_NODE   pipnod;
    
    for (pline  = _G_plineIpHook;
         pline != LW_NULL;
         pline  = _list_line_get_next(pline)) {
        pipnod = _LIST_ENTRY(pline, IP_HOOK_NODE, IPHOOK_lineManage);
        if (pipnod->IPHOOK_pfuncHook(ip_type, hook_type, p, in, out)) {
            return  (1);
        }
    }
    
    return  (0);
}
/*********************************************************************************************************
** ��������: lwip_ip_nat_hook
** ��������: lwip ip NAT ר�ûص�����
** �䡡��  : ip_type       ip ���� IP_HOOK_V4
**           hook_type     �ص����� IP_HT_NAT_PRE_ROUTING
**           p             ���ݰ�
**           in            ��������ӿ�
**           out           �������ӿ�
** �䡡��  : pbuf
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
struct pbuf *lwip_ip_nat_hook (int ip_type, int hook_type, struct pbuf *p, struct netif *in, struct netif *out)
{
    if (_G_pfuncIpHookNat) {
        return  (_G_pfuncIpHookNat(ip_type, hook_type, p, in, out));
    } else {
        return  (p);
    }
}
/*********************************************************************************************************
** ��������: lwip_ip_route_hook
** ��������: lwip ROUTE �ص�����
** �䡡��  : ip_type       ip ���� IP_HOOK_V4 / IP_HOOK_V6
**           src           Դ��ַ
**           dest          Ŀ�ĵ�ַ
** �䡡��  : netif
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
struct netif  *lwip_ip_route_hook (int ip_type, const void *src, const void *dest)
{
    if (_G_pfuncIpHookRoute) {
        return  (_G_pfuncIpHookRoute(ip_type, src, dest));
    } else {
        return  (LW_NULL);
    }
}
/*********************************************************************************************************
** ��������: net_ip_hook_add
** ��������: lwip ip ��ӻص�����
** �䡡��  : name          ip �ص�����
**           hook          �ص�����
** �䡡��  : -1: ʧ��
**            0: �ɹ�
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
int  net_ip_hook_add (const char *name, int (*hook)(int ip_type, int hook_type, struct pbuf *p, 
                                                    struct netif *in, struct netif *out))
{
    PLW_LIST_LINE   pline;
    PIP_HOOK_NODE   pipnod;
    PIP_HOOK_NODE   pipnodTemp;
    
    if (!name || !hook) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    
    pipnod = (PIP_HOOK_NODE)__SHEAP_ALLOC(sizeof(IP_HOOK_NODE) + lib_strlen(name));
    if (!pipnod) {
        _ErrorHandle(ENOMEM);
        return  (PX_ERROR);
    }

    pipnod->IPHOOK_pfuncHook = hook;
    lib_strcpy(pipnod->IPHOOK_cName, name);
    
    LOCK_TCPIP_CORE();
    for (pline  = _G_plineIpHook;
         pline != LW_NULL;
         pline  = _list_line_get_next(pline)) {
        pipnodTemp = _LIST_ENTRY(pline, IP_HOOK_NODE, IPHOOK_lineManage);
        if (pipnodTemp->IPHOOK_pfuncHook == hook) {
            break;
        }
    }
    if (pline) {
        UNLOCK_TCPIP_CORE();
        __SHEAP_FREE(pipnod);
        _ErrorHandle(EALREADY);
        return  (PX_ERROR);
    }
    _List_Line_Add_Ahead(&pipnod->IPHOOK_lineManage, &_G_plineIpHook);
    UNLOCK_TCPIP_CORE();
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: net_ip_hook_delete
** ��������: lwip ip ɾ���ص�����
** �䡡��  : hook          �ص�����
** �䡡��  : -1: ʧ��
**            0: �ɹ�
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
int  net_ip_hook_delete (int (*hook)(int ip_type, int hook_type, struct pbuf *p, 
                                     struct netif *in, struct netif *out))
{
    PLW_LIST_LINE   pline;
    PIP_HOOK_NODE   pipnod;
    
    if (!hook) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    
    LOCK_TCPIP_CORE();
    for (pline  = _G_plineIpHook;
         pline != LW_NULL;
         pline  = _list_line_get_next(pline)) {
        pipnod = _LIST_ENTRY(pline, IP_HOOK_NODE, IPHOOK_lineManage);
        if (pipnod->IPHOOK_pfuncHook == hook) {
            _List_Line_Del(&pipnod->IPHOOK_lineManage, &_G_plineIpHook);
            break;
        }
    }
    UNLOCK_TCPIP_CORE();
    
    if (pline) {
        __SHEAP_FREE(pipnod);
        return  (ERROR_NONE);
    
    } else {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
}
/*********************************************************************************************************
** ��������: net_ip_hook_isadd
** ��������: lwip ip �ص������Ƿ��Ѿ���װ
** �䡡��  : hook          �ص�����
**           pbIsAdd       �Ƿ��Ѿ������
** �䡡��  : -1: ʧ��
**            0: �ɹ�
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
int  net_ip_hook_isadd (int (*hook)(int ip_type, int hook_type, struct pbuf *p,
                                    struct netif *in, struct netif *out), BOOL *pbIsAdd)
{
    PLW_LIST_LINE   pline;
    PIP_HOOK_NODE   pipnod;

    if (!hook || !pbIsAdd) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }

    LOCK_TCPIP_CORE();
    for (pline  = _G_plineIpHook;
         pline != LW_NULL;
         pline  = _list_line_get_next(pline)) {
        pipnod = _LIST_ENTRY(pline, IP_HOOK_NODE, IPHOOK_lineManage);
        if (pipnod->IPHOOK_pfuncHook == hook) {
            break;
        }
    }
    UNLOCK_TCPIP_CORE();

    if (pline) {
        *pbIsAdd = LW_TRUE;

    } else {
        *pbIsAdd = LW_FALSE;
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: net_ip_hook_nat_add
** ��������: lwip ip ��� NAT ר�ûص�����
** �䡡��  : hook          �ص�����
** �䡡��  : -1: ʧ��
**            0: �ɹ�
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
int  net_ip_hook_nat_add (struct pbuf *(*hook)(int ip_type, int hook_type, struct pbuf *p, 
                                               struct netif *in, struct netif *out))
{
    if (!hook) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }

    LOCK_TCPIP_CORE();
    if (_G_pfuncIpHookNat) {
        UNLOCK_TCPIP_CORE();
        _ErrorHandle(EBUSY);
        return  (PX_ERROR);
    }
    
    _G_pfuncIpHookNat = hook;
    UNLOCK_TCPIP_CORE();
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: net_ip_hook_nat_delete
** ��������: lwip ip ɾ�� NAT ר�ûص�����
** �䡡��  : hook          �ص�����
** �䡡��  : -1: ʧ��
**            0: �ɹ�
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
int  net_ip_hook_nat_delete (struct pbuf *(*hook)(int ip_type, int hook_type, struct pbuf *p, 
                                                  struct netif *in, struct netif *out))
{
    if (!hook) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }

    LOCK_TCPIP_CORE();
    if (_G_pfuncIpHookNat != hook) {
        UNLOCK_TCPIP_CORE();
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    
    _G_pfuncIpHookNat = LW_NULL;
    UNLOCK_TCPIP_CORE();
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: net_ip_hook_nat_isadd
** ��������: lwip ip �ص������Ƿ��Ѿ���װ
** �䡡��  : hook          �ص�����
**           pbIsAdd       �Ƿ��Ѿ������
** �䡡��  : -1: ʧ��
**            0: �ɹ�
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
int  net_ip_hook_nat_isadd (struct pbuf *(*hook)(int ip_type, int hook_type, struct pbuf *p,
                                                 struct netif *in, struct netif *out), BOOL *pbIsAdd)
{
    if (!hook || !pbIsAdd) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    
    LOCK_TCPIP_CORE();
    if (_G_pfuncIpHookNat != hook) {
        *pbIsAdd = LW_FALSE;
        
    } else {
        *pbIsAdd = LW_TRUE;
    }
    UNLOCK_TCPIP_CORE();

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: net_ip_hook_route_add
** ��������: lwip ip ��ӻص�����
** �䡡��  : hook          �ص�����
** �䡡��  : -1: ʧ��
**            0: �ɹ�
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
int  net_ip_hook_route_add (struct netif *(*hook)(int ip_type, const void *src, const void *dest))
{
    if (!hook) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }

    LOCK_TCPIP_CORE();
    if (_G_pfuncIpHookRoute) {
        UNLOCK_TCPIP_CORE();
        _ErrorHandle(EBUSY);
        return  (PX_ERROR);
    }

    _G_pfuncIpHookRoute = hook;
    UNLOCK_TCPIP_CORE();

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: net_ip_hook_route_delete
** ��������: lwip ip ɾ�� ROUTE ר�ûص�����
** �䡡��  : hook          �ص�����
** �䡡��  : -1: ʧ��
**            0: �ɹ�
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
int  net_ip_hook_route_delete (struct netif *(*hook)(int ip_type, const void *src, const void *dest))
{
    if (!hook) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }

    LOCK_TCPIP_CORE();
    if (_G_pfuncIpHookRoute != hook) {
        UNLOCK_TCPIP_CORE();
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }

    _G_pfuncIpHookRoute = LW_NULL;
    UNLOCK_TCPIP_CORE();

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: net_ip_hook_route_isadd
** ��������: lwip ip �ص������Ƿ��Ѿ���װ
** �䡡��  : hook          �ص�����
**           pbIsAdd       �Ƿ��Ѿ������
** �䡡��  : -1: ʧ��
**            0: �ɹ�
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
int  net_ip_hook_route_isadd (struct netif *(*hook)(int ip_type, const void *src, const void *dest), BOOL *pbIsAdd)
{
    if (!hook || !pbIsAdd) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }

    LOCK_TCPIP_CORE();
    if (_G_pfuncIpHookRoute != hook) {
        *pbIsAdd = LW_FALSE;

    } else {
        *pbIsAdd = LW_TRUE;
    }
    UNLOCK_TCPIP_CORE();

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: net_ip_hook_pbuf_set_ifout 
** ��������: IP_HT_LOCAL_OUT �ص�������ͨ���˺����ı����ݰ���������
** �䡡��  : p             ���ݰ�
**           pnetif        ���õķ�������ӿ�
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
void  net_ip_hook_pbuf_set_ifout (struct pbuf *p, struct netif *pnetif)
{
    if (p && pnetif) {
        p->if_out = (void *)pnetif;
    }
}
/*********************************************************************************************************
** ��������: net_ip_hook_netif_get_netdev
** ��������: ��ȡ netdev �ṹ
** �䡡��  : pnetif        ����ӿ�
** �䡡��  : �����豸
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
netdev_t  *net_ip_hook_netif_get_netdev (struct netif *pnetif)
{
    netdev_t  *netdev;
    
    if (pnetif) {
        netdev = (netdev_t *)(pnetif->state);
        if (netdev && (netdev->magic_no == NETDEV_MAGIC)) {
            return  (netdev);
        }
    }
    
    return  (LW_NULL);
}
/*********************************************************************************************************
** ��������: net_ip_hook_netif_get_ipaddr
** ��������: ��ȡ ipv4 ��ַ
** �䡡��  : pnetif        ����ӿ�
** �䡡��  : ipv4 ��ַ
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
const ip4_addr_t  *net_ip_hook_netif_get_ipaddr (struct netif *pnetif)
{
    return  (pnetif ? netif_ip4_addr(pnetif) : LW_NULL);
}
/*********************************************************************************************************
** ��������: net_ip_hook_netif_get_netmask
** ��������: ��ȡ ipv4 ����
** �䡡��  : pnetif        ����ӿ�
** �䡡��  : ipv4 ����
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
const ip4_addr_t  *net_ip_hook_netif_get_netmask (struct netif *pnetif)
{
    return  (pnetif ? netif_ip4_netmask(pnetif) : LW_NULL);
}
/*********************************************************************************************************
** ��������: net_ip_hook_netif_get_gw
** ��������: ��ȡ ipv4 ����Ĭ������
** �䡡��  : pnetif        ����ӿ�
** �䡡��  : ipv4 ����Ĭ������
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
const ip4_addr_t  *net_ip_hook_netif_get_gw (struct netif *pnetif)
{
    return  (pnetif ? netif_ip4_gw(pnetif) : LW_NULL);
}
/*********************************************************************************************************
** ��������: net_ip_hook_netif_get_ip6addr
** ��������: ��ȡ���� ipv6 ��ַ
** �䡡��  : pnetif        ����ӿ�
**           addr_index    �ڼ��� ipv6 ��ַ
**           addr_state    ��ַ״̬ IP6_ADDR_INVALID / IP6_ADDR_VALID / IP6_ADDR_TENTATIVE ...
** �䡡��  : ipv6 ��ַ
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
#if LWIP_IPV6

const ip6_addr_t  *net_ip_hook_netif_get_ip6addr (struct netif *pnetif, int  addr_index, int *addr_state)
{
    if (pnetif && (addr_index >= 0 && (addr_index < LWIP_IPV6_NUM_ADDRESSES))) {
        if (addr_state) {
            *addr_state = netif_ip6_addr_state(pnetif, addr_index);
        }
        
        return  (netif_ip6_addr(pnetif, addr_index));
    }
    
    return  (LW_NULL);
}

#endif                                                                  /*  LWIP_IPV6                   */
/*********************************************************************************************************
** ��������: net_ip_hook_netif_get_hwaddr
** ��������: ��ȡ���������ַ
** �䡡��  : pnetif        ����ӿ�
**           hwaddr_len    �����ַ����
** �䡡��  : �����ַ
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
UINT8  *net_ip_hook_netif_get_hwaddr (struct netif *pnetif, int *hwaddr_len)
{
    if (pnetif) {
        if (hwaddr_len) {
            *hwaddr_len = pnetif->hwaddr_len;
        }
        
        return  (pnetif->hwaddr);
    }
    
    return  (LW_NULL);
}
/*********************************************************************************************************
** ��������: net_ip_hook_netif_get_index
** ��������: ��ȡ���� index
** �䡡��  : pnetif        ����ӿ�
** �䡡��  : index
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
int  net_ip_hook_netif_get_index (struct netif *pnetif)
{
    return  (pnetif ? netif_get_index(pnetif) : 0);
}
/*********************************************************************************************************
** ��������: net_ip_hook_netif_get_name
** ��������: ��ȡ���� name
** �䡡��  : pnetif        ����ӿ�
**           name          ����
**           size          ����������
** �䡡��  : ���ֳ���
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
int  net_ip_hook_netif_get_name (struct netif *pnetif, char *name, size_t size)
{
    if (pnetif && name && (size < IF_NAMESIZE)) {
        return  (PX_ERROR);
    }
    
    netif_get_name(pnetif, name);
    
    return  (lib_strlen(name));
}
/*********************************************************************************************************
** ��������: net_ip_hook_netif_get_type
** ��������: ��ȡ��������
** �䡡��  : pnetif        ����ӿ�
**           type          �ӿ����� IFT_PPP / IFT_ETHER / IFT_LOOP / IFT_OTHER ...
** �䡡��  : �Ƿ�ɹ�
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
int  net_ip_hook_netif_get_type (struct netif *pnetif, int *type)
{
    if (pnetif && type) {
        *type = pnetif->link_type;
        return  (ERROR_NONE);
    }
    
    return  (PX_ERROR);
}
/*********************************************************************************************************
** ��������: net_ip_hook_netif_get_flags
** ��������: ��ȡ���� flags
** �䡡��  : pnetif        ����ӿ�
**           flags         �ӿ�״̬ IFF_UP / IFF_BROADCAST / IFF_POINTOPOINT / IFF_RUNNING ...
** �䡡��  : �Ƿ�ɹ�
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
int  net_ip_hook_netif_get_flags (struct netif *pnetif, int *flags)
{
    if (pnetif && flags) {
        *flags = netif_get_flags(pnetif);
        return  (ERROR_NONE);
    }
    
    return  (PX_ERROR);
}
/*********************************************************************************************************
** ��������: net_ip_hook_netif_get_linkspeed
** ��������: ��ȡ���������ٶ�
** �䡡��  : pnetif        ����ӿ�
** �䡡��  : �����ٶ�
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
UINT64  net_ip_hook_netif_get_linkspeed (struct netif *pnetif)
{
    netdev_t  *netdev;
    
    if (pnetif) {
        netdev = (netdev_t *)(pnetif->state);
        if (netdev && (netdev->magic_no == NETDEV_MAGIC)) {
            return  (netdev->speed);
        } else {
            return  (pnetif->link_speed);
        }
    }
    
    return  (0);
}

#endif                                                                  /*  LW_CFG_NET_EN               */
/*********************************************************************************************************
  END
*********************************************************************************************************/
