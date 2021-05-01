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
** ��   ��   ��: lwip_natlib.c
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2010 �� 03 �� 19 ��
**
** ��        ��: lwip NAT ֧�ְ��ڲ���.

** BUG:
2010.04.08  �����ǹ����³�Ⱥ��һ����������, ����ע��, ���������ʽ.
2010.04.12  ���� nat_ip_input_hook() ����, ϵͳʹ�õ� ip_input_hook().
2010.11.06  �������˻������ٿ�, ����ȴ����ֹͣ��صĻ�������, ת�����Ż������ݻ�������, ����������Ա
            �ǳ����������! ��������ԱΪ�������˵Ľ�����ש����, ȴ�õ���˴���.
            ip nat �������������.
2011.03.06  ���� gcc 4.5.1 ��� warning.
2011.06.23  �Ż�һ���ִ���. 
            �� NAT ת���������������ϸ������.
2011.07.01  lwip ֧�ֱ��������·�ɸĽ�����, ���� NAT ֧�ֵ�����ת��.
            https://savannah.nongnu.org/bugs/?33634
2011.07.30  �� ap ���벻�ڴ���˿ڷ�Χ����, ֱ���˳�.
2013.04.01  ���� GCC 4.7.3 �������� warning.
2015.12.16  ���� NAT ����֧������.
            ������������ӳ�书��.
2016.08.25  ������������ȱ�ٴ�����ƿ������.
2017.12.19  ���� MAP ���ؾ��⹦��.
2018.01.16  ʹ�� iphook ʵ�ָ������� NAT ����.
2018.04.06  NAT ֧����ǰ��Ƭ����.
2019.02.15  ���� TCP SYN, CLOSING ������ 1 ����.
2019.04.09  NAT AP �˿�֧��������ȫ����.
2019.05.20  MAP 0.0.0.0 ��Ϊ����ӳ��.
2020.02.25  ����Э���ʹ�� Hash ��, ���ת�������ٶ�.
2020.06.30  ��������ӿ��˳� NAT ����.
2020.07.22  �����ر��� DHCP �˿�.
*********************************************************************************************************/
#define  __SYLIXOS_STDIO
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
#include "../SylixOS/system/include/s_system.h"
#include "../SylixOS/net/include/net_net.h"
/*********************************************************************************************************
  �ü�����
*********************************************************************************************************/
#if (LW_CFG_NET_EN > 0) && (LW_CFG_NET_ROUTER > 0) && (LW_CFG_NET_NAT_EN > 0)
#include "net/if.h"
#include "net/if_iphook.h"
#include "lwip/opt.h"
#include "lwip/inet.h"
#include "lwip/ip.h"
#include "lwip/ip4_frag.h"
#include "lwip/tcp.h"
#include "lwip/prot/iana.h"
#include "lwip/priv/tcp_priv.h"
#include "lwip/udp.h"
#include "lwip/icmp.h"
#include "lwip/inet_chksum.h"
#include "lwip/err.h"
#include "lwip/netif.h"
#include "lwip/tcpip.h"
#include "lwip_natlib.h"
/*********************************************************************************************************
  NAT ��ȫ����
*********************************************************************************************************/
#define __NAT_STRONG_RULE   1                                           /*  �����Ϲ涨�����ݰ��Ƿ����  */
/*********************************************************************************************************
  NAT ������
*********************************************************************************************************/
#define __NAT_LOCK()        LOCK_TCPIP_CORE()
#define __NAT_UNLOCK()      UNLOCK_TCPIP_CORE()
/*********************************************************************************************************
  NAT �ڵ� Hash ����
*********************************************************************************************************/
#define __NAT_HASH_SIZE     128
#define __NAT_HASH_MASK     (__NAT_HASH_SIZE - 1)
#define __NAT_HASH(port)    ((port) & __NAT_HASH_MASK)
/*********************************************************************************************************
  NAT �ڵ� Hash ����
*********************************************************************************************************/
#define __NAT_HASH_FOR_EACH(i) \
        for (i = 0; i < __NAT_HASH_SIZE; i++)
/*********************************************************************************************************
  NAT �ӿ�����
*********************************************************************************************************/
typedef struct {
    CHAR            NATIF_cIfName[IF_NAMESIZE];
    struct netif   *NATIF_pnetif;
} __NAT_IF;
typedef __NAT_IF   *__PNAT_IF;
/*********************************************************************************************************
  NAT ���������� AP ��������ӿ�
*********************************************************************************************************/
static __NAT_IF     _G_natifAp[LW_CFG_NET_NAT_MAX_AP_IF];
static __NAT_IF     _G_natifLocal[LW_CFG_NET_NAT_MAX_LOCAL_IF];
static BOOL         _G_bNatStart = LW_FALSE;
/*********************************************************************************************************
  NAT ���ƿ黺���
*********************************************************************************************************/
static LW_SPINLOCK_CA_DEFINE_CACHE_ALIGN(_G_slcaNat);
static LW_LIST_MONO_HEADER   _G_pmonoNatcbFree = LW_NULL;
static LW_LIST_MONO_HEADER   _G_pmonoNatcbTail = LW_NULL;
static LW_OBJECT_HANDLE      _G_ulNatcbTimer   = LW_OBJECT_HANDLE_INVALID;
/*********************************************************************************************************
  NAT ���ƿ�����
*********************************************************************************************************/
static LW_LIST_LINE_HEADER  _G_plineNatcbTcp[__NAT_HASH_SIZE];
static LW_LIST_LINE_HEADER  _G_plineNatcbUdp[__NAT_HASH_SIZE];
static LW_LIST_LINE_HEADER  _G_plineNatcbIcmp[__NAT_HASH_SIZE];
/*********************************************************************************************************
  NAT ASS Hash table
*********************************************************************************************************/
static LW_LIST_LINE_HEADER  _G_plineAssHashTcp[__NAT_HASH_SIZE];
static LW_LIST_LINE_HEADER  _G_plineAssHashUdp[__NAT_HASH_SIZE];
static LW_LIST_LINE_HEADER  _G_plineAssHashIcmp[__NAT_HASH_SIZE];
/*********************************************************************************************************
  NAT ��Ƭʹ��
*********************************************************************************************************/
static BOOL  _G_bNatTcpFrag  = LW_FALSE;
static BOOL  _G_bNatUdpFrag  = LW_FALSE;
static BOOL  _G_bNatIcmpFrag = LW_FALSE;
/*********************************************************************************************************
  NAT ����
*********************************************************************************************************/
static LW_LIST_LINE_HEADER  _G_plineNatalias = LW_NULL;
/*********************************************************************************************************
  NAT ������������
*********************************************************************************************************/
static LW_LIST_LINE_HEADER  _G_plineNatmap = LW_NULL;
/*********************************************************************************************************
** ��������: nat_netif_add_hook
** ��������: NAT ����ӿڼ���ص�
** �䡡��  : pnetif    �����������ӿ�
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
VOID  nat_netif_add_hook (struct netif *pnetif)
{
    INT    i;
    CHAR   cIfName[IF_NAMESIZE];
    
    if (!_G_bNatStart) {
        return;
    }
    
    __NAT_LOCK();
    netif_get_name(pnetif, cIfName);
    
    for (i = 0; i < LW_CFG_NET_NAT_MAX_AP_IF; i++) {
        if (_G_natifAp[i].NATIF_pnetif == LW_NULL) {
            if (!lib_strcmp(cIfName, _G_natifAp[i].NATIF_cIfName)) {
                _G_natifAp[i].NATIF_pnetif = pnetif;
                pnetif->nat_mode = NETIF_NAT_AP;
                goto    out;
            }
        }
    }
    for (i = 0; i < LW_CFG_NET_NAT_MAX_LOCAL_IF; i++) {
        if (_G_natifLocal[i].NATIF_pnetif == LW_NULL) {
            if (!lib_strcmp(cIfName, _G_natifLocal[i].NATIF_cIfName)) {
                _G_natifLocal[i].NATIF_pnetif = pnetif;
                pnetif->nat_mode = NETIF_NAT_LOCAL;
                goto    out;
            }
        }
    }
out:
    __NAT_UNLOCK();
}
/*********************************************************************************************************
** ��������: nat_netif_remove_hook
** ��������: NAT ����ӿ��Ƴ��ص�
** �䡡��  : pnetif    ���Ƴ�������ӿ�
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
VOID  nat_netif_remove_hook (struct netif *pnetif)
{
    INT  i;
    
    if (!_G_bNatStart) {
        return;
    }
    
    __NAT_LOCK();
    for (i = 0; i < LW_CFG_NET_NAT_MAX_AP_IF; i++) {
        if (_G_natifAp[i].NATIF_pnetif == pnetif) {
            _G_natifAp[i].NATIF_pnetif =  LW_NULL;
            goto    out;
        }
    }
    for (i = 0; i < LW_CFG_NET_NAT_MAX_LOCAL_IF; i++) {
        if (_G_natifLocal[i].NATIF_pnetif == pnetif) {
            _G_natifLocal[i].NATIF_pnetif =  LW_NULL;
            goto    out;
        }
    }
out:
    __NAT_UNLOCK();
}
/*********************************************************************************************************
** ��������: __natPoolInit
** ��������: ���� NAT ���ƿ黺���.
** �䡡��  : NONE
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static VOID  __natPoolInit (VOID)
{
    INT        i;
    UINT16     usPort = LW_CFG_NET_NAT_MIN_PORT;
    __PNAT_CB  pnatcbFree;
    
    LW_SPIN_INIT(&_G_slcaNat.SLCA_sl);
    
    pnatcbFree = (__PNAT_CB)__SHEAP_ALLOC(sizeof(__NAT_CB) * LW_CFG_NET_NAT_MAX_SESSION);
    _BugHandle((pnatcbFree == LW_NULL), LW_TRUE, "can not create 'nat_pool'\r\n");
    
    for (i = 0; i < LW_CFG_NET_NAT_MAX_SESSION; i++) {
        pnatcbFree->NAT_usAssPort = PP_HTONS(usPort);
        _list_mono_free_seq(&_G_pmonoNatcbFree, &_G_pmonoNatcbTail, &pnatcbFree->NAT_monoFree);
        pnatcbFree++;
        usPort++;
    }
}
/*********************************************************************************************************
** ��������: __natPoolAlloc
** ��������: ����һ�� NAT ���ƿ�.
** �䡡��  : NONE
** �䡡��  : NAT ���ƿ�
** ȫ�ֱ���: 
** ����ģ��: 
** ע  ��  : ���ﲻ�ܳ�ʼ�� ass port
*********************************************************************************************************/
static __PNAT_CB  __natPoolAlloc (VOID)
{
    INTREG     iregInterLevel;
    __PNAT_CB  pnatcb;
    
    LW_SPIN_LOCK_QUICK(&_G_slcaNat.SLCA_sl, &iregInterLevel);
    if (_LIST_MONO_IS_EMPTY(_G_pmonoNatcbFree)) {
        pnatcb = LW_NULL;
    } else {
        pnatcb = (__PNAT_CB)_list_mono_allocate_seq(&_G_pmonoNatcbFree, &_G_pmonoNatcbTail);
    }
    LW_SPIN_UNLOCK_QUICK(&_G_slcaNat.SLCA_sl, iregInterLevel);
    
    if (pnatcb) {
        pnatcb->NAT_ucInAssHash      = 0;
        pnatcb->NAT_ucProto          = 0;
        pnatcb->NAT_usMapHash        = 0;
        pnatcb->NAT_ipaddrLocal.addr = IPADDR_ANY;
        pnatcb->NAT_usAssPortSave    = 0;
        pnatcb->NAT_usLocalPort      = 0;
        pnatcb->NAT_uiLocalSequence  = 0;
        pnatcb->NAT_uiLocalRcvNext   = 0;
        pnatcb->NAT_ulIdleTimer      = 0ul;
        pnatcb->NAT_iStatus          = __NAT_STATUS_OPEN;
    }

    return  (pnatcb);
}
/*********************************************************************************************************
** ��������: __natPoolFree
** ��������: �ͷ�һ�� NAT ���ƿ�.
** �䡡��  : pnatcb            NAT ���ƿ�
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static VOID  __natPoolFree (__PNAT_CB  pnatcb)
{
    INTREG     iregInterLevel;
    
    if (pnatcb) {
        LW_SPIN_LOCK_QUICK(&_G_slcaNat.SLCA_sl, &iregInterLevel);
        if (pnatcb->NAT_usAssPortSave) {
            pnatcb->NAT_usAssPort = pnatcb->NAT_usAssPortSave;          /*  �ָ��˿���Դ                */
        }
        _list_mono_free_seq(&_G_pmonoNatcbFree, &_G_pmonoNatcbTail, &pnatcb->NAT_monoFree);
        LW_SPIN_UNLOCK_QUICK(&_G_slcaNat.SLCA_sl, iregInterLevel);
    }
}
/*********************************************************************************************************
** ��������: __natNewmap
** ��������: ����һ�� NAT ����ӳ�����.
** �䡡��  : pipaddr       ���ػ��� IP
**           usIpCnt       ���� IP ���� (���ؾ���)
**           usPort        ���ػ��� �˿�
**           AssPort       ӳ��˿� 
**           ucProto       ʹ�õ�Э��
** �䡡��  : ERROR or OK
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
INT  __natMapAdd (ip4_addr_t  *pipaddr, u16_t  usIpCnt, u16_t  usPort, u16_t  AssPort, u8_t  ucProto)
{
    __PNAT_MAP  pnatmap;
    
    if ((PP_NTOHS(AssPort) >= LW_CFG_NET_NAT_MIN_PORT) && 
        (PP_NTOHS(AssPort) <= LW_CFG_NET_NAT_MAX_PORT)) {
        _ErrorHandle(EADDRINUSE);
        return  (PX_ERROR);
    }
    
    pnatmap = (__PNAT_MAP)__SHEAP_ALLOC(sizeof(__NAT_MAP));
    if (pnatmap == LW_NULL) {
        _ErrorHandle(ERROR_SYSTEM_LOW_MEMORY);
        return  (PX_ERROR);
    }
    
    pnatmap->NATM_ucProto            = ucProto;
    pnatmap->NATM_ipaddrLocalIp.addr = pipaddr->addr;
    pnatmap->NATM_usLocalCnt         = usIpCnt;
    pnatmap->NATM_usLocalPort        = usPort;
    pnatmap->NATM_usAssPort          = AssPort;
    
    __NAT_LOCK();
    _List_Line_Add_Ahead(&pnatmap->NATM_lineManage, &_G_plineNatmap);
    __NAT_UNLOCK();
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: __natNewmap
** ��������: ɾ��һ�� NAT ����ӳ�����.
** �䡡��  : pipaddr       ���ػ��� IP
**           usPort        ���ػ��� �˿�
**           AssPort       ӳ��˿� 
**           ucProto       ʹ�õ�Э��
** �䡡��  : ERROR or OK
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
INT  __natMapDelete (ip4_addr_t  *pipaddr, u16_t  usPort, u16_t  AssPort, u8_t  ucProto)
{
    PLW_LIST_LINE   plineTemp;
    __PNAT_MAP      pnatmap;
    PVOID           pvFree = LW_NULL;
    
    __NAT_LOCK();
    for (plineTemp  = _G_plineNatmap;
         plineTemp != LW_NULL;
         plineTemp  = _list_line_get_next(plineTemp)) {
        
        pnatmap = _LIST_ENTRY(plineTemp, __NAT_MAP, NATM_lineManage);
        if ((pnatmap->NATM_ucProto            == ucProto)       &&
            (pnatmap->NATM_ipaddrLocalIp.addr == pipaddr->addr) &&
            (pnatmap->NATM_usLocalPort        == usPort)        &&
            (pnatmap->NATM_usAssPort          == AssPort)) {
            break;
        }
    }
    if (plineTemp) {
        _List_Line_Del(&pnatmap->NATM_lineManage, &_G_plineNatmap);
        pvFree = pnatmap;
    }
    __NAT_UNLOCK();
    
    if (pvFree) {
        __SHEAP_FREE(pvFree);
        return  (ERROR_NONE);
    
    } else {
        _ErrorHandle(ESRCH);
        return  (PX_ERROR);
    }
}
/*********************************************************************************************************
** ��������: __natAliasAdd
** ��������: NAT ����������� (���� IP ӳ��)
** �䡡��  : pipaddrAlias      ����
**           ipaddrSLocalIp    ��Ӧ������ʼ IP
**           ipaddrELocalIp    ��Ӧ���ؽ��� IP
** �䡡��  : ERROR or OK
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
INT  __natAliasAdd (const ip4_addr_t  *pipaddrAlias, 
                    const ip4_addr_t  *ipaddrSLocalIp,
                    const ip4_addr_t  *ipaddrELocalIp)
{
    __PNAT_ALIAS  pnatalias;
    
    pnatalias = (__PNAT_ALIAS)__SHEAP_ALLOC(sizeof(__NAT_ALIAS));
    if (pnatalias == LW_NULL) {
        _ErrorHandle(ERROR_SYSTEM_LOW_MEMORY);
        return  (PX_ERROR);
    }
    
    pnatalias->NATA_ipaddrAliasIp.addr  = pipaddrAlias->addr;
    pnatalias->NATA_ipaddrSLocalIp.addr = ipaddrSLocalIp->addr;
    pnatalias->NATA_ipaddrELocalIp.addr = ipaddrELocalIp->addr;
    
    __NAT_LOCK();
    _List_Line_Add_Ahead(&pnatalias->NATA_lineManage, &_G_plineNatalias);
    __NAT_UNLOCK();
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: __natAliasDelete
** ��������: NAT ɾ��������� (���� IP ӳ��)
** �䡡��  : pipaddrAlias      ����
** �䡡��  : ERROR or OK
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
INT  __natAliasDelete (const ip4_addr_t  *pipaddrAlias)
{
    PLW_LIST_LINE   plineTemp;
    __PNAT_ALIAS    pnatalias;
    PVOID           pvFree = LW_NULL;
    
    __NAT_LOCK();
    for (plineTemp  = _G_plineNatalias;
         plineTemp != LW_NULL;
         plineTemp  = _list_line_get_next(plineTemp)) {
        
        pnatalias = _LIST_ENTRY(plineTemp, __NAT_ALIAS, NATA_lineManage);
        if (pnatalias->NATA_ipaddrAliasIp.addr == pipaddrAlias->addr) {
            break;
        }
    }
    if (plineTemp) {
        _List_Line_Del(&pnatalias->NATA_lineManage, &_G_plineNatalias);
        pvFree = pnatalias;
    }
    __NAT_UNLOCK();
    
    if (pvFree) {
        __SHEAP_FREE(pvFree);
        return  (ERROR_NONE);
    
    } else {
        _ErrorHandle(ESRCH);
        return  (PX_ERROR);
    }
}
/*********************************************************************************************************
** ��������: __natAssHashAdd
** ��������: NAT ���ƿ���ӵ� Ass Hash ��.
** �䡡��  : pnatcb   NAT ���ƿ�
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static VOID  __natAssHashAdd (__PNAT_CB  pnatcb)
{
    INT    iAssHash;

    if (pnatcb->NAT_ucInAssHash) {
        return;
    }

    iAssHash = __NAT_HASH(pnatcb->NAT_usAssPort);

    switch (pnatcb->NAT_ucProto) {

    case IP_PROTO_TCP:                                                  /*  TCP                         */
        _List_Line_Add_Ahead(&pnatcb->NAT_lineAssHash, &_G_plineAssHashTcp[iAssHash]);
        break;

    case IP_PROTO_UDP:                                                  /*  UDP                         */
        _List_Line_Add_Ahead(&pnatcb->NAT_lineAssHash, &_G_plineAssHashUdp[iAssHash]);
        break;

    case IP_PROTO_ICMP:                                                 /*  ICMP                        */
        _List_Line_Add_Ahead(&pnatcb->NAT_lineAssHash, &_G_plineAssHashIcmp[iAssHash]);
        break;

    default:                                                            /*  ��Ӧ�����е�����            */
        _BugHandle(LW_TRUE, LW_TRUE, "NAT Protocol error!\r\n");
        break;
    }

    pnatcb->NAT_ucInAssHash = 1;
}
/*********************************************************************************************************
** ��������: __natAssHashDelete
** ��������: NAT ���ƿ���ӵ� Ass Hash ��.
** �䡡��  : pnatcb   NAT ���ƿ�
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static VOID  __natAssHashDelete (__PNAT_CB  pnatcb)
{
    INT    iAssHash;

    if (pnatcb->NAT_ucInAssHash == 0) {
        return;
    }

    iAssHash = __NAT_HASH(pnatcb->NAT_usAssPort);

    switch (pnatcb->NAT_ucProto) {

    case IP_PROTO_TCP:                                                  /*  TCP                         */
        _List_Line_Del(&pnatcb->NAT_lineAssHash, &_G_plineAssHashTcp[iAssHash]);
        break;

    case IP_PROTO_UDP:                                                  /*  UDP                         */
        _List_Line_Del(&pnatcb->NAT_lineAssHash, &_G_plineAssHashUdp[iAssHash]);
        break;

    case IP_PROTO_ICMP:                                                 /*  ICMP                        */
        _List_Line_Del(&pnatcb->NAT_lineAssHash, &_G_plineAssHashIcmp[iAssHash]);
        break;

    default:                                                            /*  ��Ӧ�����е�����            */
        _BugHandle(LW_TRUE, LW_TRUE, "NAT Protocol error!\r\n");
        break;
    }

    pnatcb->NAT_ucInAssHash = 0;
}
/*********************************************************************************************************
** ��������: __natFirst
** ��������: ��� hash ���д��ڿ��ƿ�, �򷵻�һ�����ƿ�.
** �䡡��  : pplineHeader[]   �ڵ� Hash ��
** �䡡��  : NAT ���ƿ�
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static __PNAT_CB  __natFirst (LW_LIST_LINE_HEADER pplineHeader[])
{
    INT                 i;
    LW_LIST_LINE_HEADER plineHeader;
    __PNAT_CB           pnatcb = LW_NULL;

    __NAT_HASH_FOR_EACH(i) {
        plineHeader = pplineHeader[i];
        if (plineHeader) {
            pnatcb = _LIST_ENTRY(plineHeader, __NAT_CB, NAT_lineManage);
            break;
        }
    }

    return  (pnatcb);
}
/*********************************************************************************************************
** ��������: __natNew
** ��������: ����һ�� NAT ����.
** �䡡��  : pipaddr       ���ػ��� IP
**           usPort        ���ػ��� �˿�
**           ucProto       ʹ�õ�Э��
** �䡡��  : NAT ���ƿ�
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static __PNAT_CB  __natNew (ip4_addr_p_t  *pipaddr, u16_t  usPort, u8_t  ucProto)
{
    INT             i, iHash;
    PLW_LIST_LINE   plineTemp;
    __PNAT_CB       pnatcb;
    __PNAT_CB       pnatcbOldest;
    __PNAT_CB       pnatcbSyn     = LW_NULL;
    __PNAT_CB       pnatcbClosing = LW_NULL;
    
    pnatcb = __natPoolAlloc();
    if (pnatcb == LW_NULL) {                                            /*  ��Ҫɾ������ϵ�            */
        pnatcbOldest = __natFirst(_G_plineNatcbIcmp);                   /*  ������̭ ICMP               */
        if (pnatcbOldest) {
            __NAT_HASH_FOR_EACH(i) {
                for (plineTemp  = _G_plineNatcbIcmp[i];
                     plineTemp != LW_NULL;
                     plineTemp  = _list_line_get_next(plineTemp)) {

                    pnatcb = _LIST_ENTRY(plineTemp, __NAT_CB, NAT_lineManage);
                    if (pnatcb->NAT_ulIdleTimer >= pnatcbOldest->NAT_ulIdleTimer) {
                        pnatcbOldest = pnatcb;
                    }
                }
            }
            
        } else {                                                        /*  ��̭ TCP UDP ʱ����õĽڵ� */
            pnatcbOldest = __natFirst(_G_plineNatcbTcp);
            if (pnatcbOldest == LW_NULL) {
                pnatcbOldest = __natFirst(_G_plineNatcbUdp);
            }
            
            __NAT_HASH_FOR_EACH(i) {
                for (plineTemp  = _G_plineNatcbTcp[i];
                     plineTemp != LW_NULL;
                     plineTemp  = _list_line_get_next(plineTemp)) {

                    pnatcb = _LIST_ENTRY(plineTemp, __NAT_CB, NAT_lineManage);
                    if ((pnatcb->NAT_iStatus == __NAT_STATUS_SYN) &&
                        (pnatcb->NAT_ulIdleTimer > 40)) {               /*  ���� 40s ���ϵ� SYN TCP     */
                        if (pnatcbSyn) {
                            if (pnatcb->NAT_ulIdleTimer > pnatcbSyn->NAT_ulIdleTimer) {
                                pnatcbSyn = pnatcb;
                            }

                        } else {
                            pnatcbSyn = pnatcb;
                        }

                    } else if ((pnatcb->NAT_iStatus == __NAT_STATUS_CLOSING) &&
                               (pnatcb->NAT_ulIdleTimer > 30)) {        /*  ���� 30s ���ϵ� CLOSING TCP */
                        if (pnatcbClosing) {
                            if (pnatcb->NAT_ulIdleTimer > pnatcbClosing->NAT_ulIdleTimer) {
                                pnatcbClosing = pnatcb;
                            }

                        } else {
                            pnatcbClosing = pnatcb;
                        }
                    }

                    if (pnatcb->NAT_ulIdleTimer >= pnatcbOldest->NAT_ulIdleTimer) {
                        pnatcbOldest = pnatcb;
                    }
                }
            }

            if (pnatcbClosing) {
                pnatcbOldest = pnatcbClosing;                           /*  ������̭ CLOSING            */

            } else if (pnatcbSyn) {
                pnatcbOldest = pnatcbSyn;                               /*  �����̭ SYN                */

            } else {
                __NAT_HASH_FOR_EACH(i) {                                /*  ������ UDP ���бȽ�         */
                    for (plineTemp  = _G_plineNatcbUdp[i];
                         plineTemp != LW_NULL;
                         plineTemp  = _list_line_get_next(plineTemp)) {

                        pnatcb = _LIST_ENTRY(plineTemp, __NAT_CB, NAT_lineManage);
                        if (pnatcb->NAT_ulIdleTimer >= pnatcbOldest->NAT_ulIdleTimer) {
                            pnatcbOldest = pnatcb;
                        }
                    }
                }
            }
        }
        
        pnatcb = pnatcbOldest;                                          /*  ʹ�����ϵ�                  */
        __natAssHashDelete(pnatcb);

        iHash = __NAT_HASH(pnatcb->NAT_usLocalPort);                    /*  ���� Hash ���              */

        switch (pnatcb->NAT_ucProto) {
        
        case IP_PROTO_TCP:                                              /*  TCP ����                    */
            _List_Line_Del(&pnatcb->NAT_lineManage, &_G_plineNatcbTcp[iHash]);
            break;
            
        case IP_PROTO_UDP:                                              /*  UDP ����                    */
            _List_Line_Del(&pnatcb->NAT_lineManage, &_G_plineNatcbUdp[iHash]);
            break;
            
        case IP_PROTO_ICMP:                                             /*  ICMP ����                   */
            _List_Line_Del(&pnatcb->NAT_lineManage, &_G_plineNatcbIcmp[iHash]);
            break;
            
        default:                                                        /*  ��Ӧ�����е�����            */
            _BugHandle(LW_TRUE, LW_TRUE, "NAT Protocol error!\r\n");
            break;
        }
    }

    pnatcb->NAT_ucProto          = ucProto;
    pnatcb->NAT_usMapHash        = 0;
    pnatcb->NAT_ipaddrLocal.addr = pipaddr->addr;
    pnatcb->NAT_usLocalPort      = usPort;
    
    pnatcb->NAT_uiLocalSequence = 0;
    pnatcb->NAT_uiLocalRcvNext  = 0;
    
    pnatcb->NAT_ulIdleTimer = 0;
    pnatcb->NAT_iStatus     = __NAT_STATUS_OPEN;

    iHash = __NAT_HASH(pnatcb->NAT_usLocalPort);                        /*  �����µ� Hash ���          */

    switch (ucProto) {
        
    case IP_PROTO_TCP:                                                  /*  TCP ����                    */
        _List_Line_Add_Ahead(&pnatcb->NAT_lineManage, &_G_plineNatcbTcp[iHash]);
        break;
        
    case IP_PROTO_UDP:                                                  /*  UDP ����                    */
        _List_Line_Add_Ahead(&pnatcb->NAT_lineManage, &_G_plineNatcbUdp[iHash]);
        break;
        
    case IP_PROTO_ICMP:                                                 /*  ICMP ����                   */
        _List_Line_Add_Ahead(&pnatcb->NAT_lineManage, &_G_plineNatcbIcmp[iHash]);
        break;
        
    default:                                                            /*  ��Ӧ�����е�����            */
        _BugHandle(LW_TRUE, LW_TRUE, "NAT Protocol error!\r\n");
        break;
    }
    
    return  (pnatcb);
}
/*********************************************************************************************************
** ��������: __natClose
** ��������: ɾ��һ�� NAT ����.
** �䡡��  : pnatcb            NAT ���ƿ�
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static VOID  __natClose (__PNAT_CB  pnatcb)
{
    INT  iHash;

    if (pnatcb == LW_NULL) {
        return;
    }
    
    __natAssHashDelete(pnatcb);

    iHash = __NAT_HASH(pnatcb->NAT_usLocalPort);                        /*  ���� Hash ���              */

    switch (pnatcb->NAT_ucProto) {
    
    case IP_PROTO_TCP:                                                  /*  TCP ����                    */
        _List_Line_Del(&pnatcb->NAT_lineManage, &_G_plineNatcbTcp[iHash]);
        break;
        
    case IP_PROTO_UDP:                                                  /*  UDP ����                    */
        _List_Line_Del(&pnatcb->NAT_lineManage, &_G_plineNatcbUdp[iHash]);
        break;
        
    case IP_PROTO_ICMP:                                                 /*  ICMP ����                   */
        _List_Line_Del(&pnatcb->NAT_lineManage, &_G_plineNatcbIcmp[iHash]);
        break;
        
    default:                                                            /*  ��Ӧ�����е�����            */
        _BugHandle(LW_TRUE, LW_TRUE, "NAT Protocol error!\r\n");
        break;
    }

    __natPoolFree(pnatcb);
}
/*********************************************************************************************************
** ��������: __natTimer
** ��������: NAT ��ʱ��.
** �䡡��  : ulPeriod      ��ʱ���� (��)
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static VOID  __natTimer (ULONG  ulPeriod)
{
    static ULONG    ulIdlTo = (LW_CFG_NET_NAT_IDLE_TIMEOUT * 60);
    INT             i;
    __PNAT_CB       pnatcb;
    PLW_LIST_LINE   plineTemp;
    
    __NAT_LOCK();
    __NAT_HASH_FOR_EACH(i) {
        plineTemp = _G_plineNatcbTcp[i];
        while (plineTemp) {
            pnatcb = _LIST_ENTRY(plineTemp, __NAT_CB, NAT_lineManage);
            plineTemp = _list_line_get_next(plineTemp);                 /*  ָ����һ���ڵ�              */

            pnatcb->NAT_ulIdleTimer += ulPeriod;
            if (pnatcb->NAT_ulIdleTimer >= ulIdlTo) {                   /*  ����ʱ������ر�            */
                __natClose(pnatcb);

            } else {
                if ((pnatcb->NAT_iStatus == __NAT_STATUS_SYN) ||
                    (pnatcb->NAT_iStatus == __NAT_STATUS_CLOSING)) {    /*  SYN ���� CLOSING            */
                    if (pnatcb->NAT_ulIdleTimer >= 60) {                /*  ������ʱ                    */
                        __natClose(pnatcb);
                    }
                }
            }
        }
    }
    
    __NAT_HASH_FOR_EACH(i) {
        plineTemp = _G_plineNatcbUdp[i];
        while (plineTemp) {
            pnatcb = _LIST_ENTRY(plineTemp, __NAT_CB, NAT_lineManage);
            plineTemp = _list_line_get_next(plineTemp);                 /*  ָ����һ���ڵ�              */

            pnatcb->NAT_ulIdleTimer += ulPeriod;
            if (pnatcb->NAT_ulIdleTimer >= ulIdlTo) {                   /*  ����ʱ������ر�            */
                __natClose(pnatcb);
            }
        }
    }
    
    __NAT_HASH_FOR_EACH(i) {
        plineTemp = _G_plineNatcbIcmp[i];
        while (plineTemp) {
            pnatcb = _LIST_ENTRY(plineTemp, __NAT_CB, NAT_lineManage);
            plineTemp = _list_line_get_next(plineTemp);                 /*  ָ����һ���ڵ�              */

            pnatcb->NAT_ulIdleTimer += ulPeriod;
            if (pnatcb->NAT_ulIdleTimer >= ulIdlTo) {                   /*  ����ʱ������ر�            */
                __natClose(pnatcb);
            }
        }
    }
    __NAT_UNLOCK();
}
/*********************************************************************************************************
** ��������: __natApInput
** ��������: NAT AP ����ӿ�����.
** �䡡��  : p             ���ݰ�
**           netifIn       ����ӿ�
** �䡡��  : 0: ok 1: eaten
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static INT  __natApInput (struct pbuf *p, struct netif *netifIn)
{
    static u16_t             iphdrlen;

    struct ip_hdr           *iphdr   = (struct ip_hdr *)p->payload;
    struct tcp_hdr          *tcphdr  = LW_NULL;
    struct udp_hdr          *udphdr  = LW_NULL;
    struct icmp_echo_hdr    *icmphdr = LW_NULL;
    
    u32_t                    u32OldAddr;
    u16_t                    usDestPort;
    u8_t                     ucProto;
    
    INT                      iAssHash;
    __PNAT_CB                pnatcb  = LW_NULL;
    __PNAT_MAP               pnatmap = LW_NULL;
    PLW_LIST_LINE            plineTemp;
    LW_LIST_LINE_HEADER      plineHeader;
    
    iphdrlen  = (u16_t)IPH_HL(iphdr);                                   /*  ��� IP ��ͷ����            */
    iphdrlen *= 4;
    
    if (p->len < iphdrlen) {                                            /*  �������                    */
        return  (__NAT_STRONG_RULE);
    }
    
    LWIP_ASSERT("NAT Input fragment error", 
                !(IPH_OFFSET(iphdr) & PP_HTONS(IP_OFFMASK | IP_MF)));   /*  �����ϲ����з�Ƭ��־        */
    
    ucProto = IPH_PROTO(iphdr);
    switch (ucProto) {
    
    case IP_PROTO_TCP:                                                  /*  TCP ���ݱ�                  */
        if (p->len < (iphdrlen + TCP_HLEN)) {
            return  (__NAT_STRONG_RULE);
        }
        tcphdr = (struct tcp_hdr *)(((u8_t *)p->payload) + iphdrlen);
        usDestPort  = tcphdr->dest;
        iAssHash    = __NAT_HASH(usDestPort);
        plineHeader = _G_plineAssHashTcp[iAssHash];
        break;
        
    case IP_PROTO_UDP:                                                  /*  UDP ���ݱ�                  */
        if (p->len < (iphdrlen + UDP_HLEN)) {
            return  (__NAT_STRONG_RULE);
        }
        udphdr = (struct udp_hdr *)(((u8_t *)p->payload) + iphdrlen);
        usDestPort  = udphdr->dest;
        iAssHash    = __NAT_HASH(usDestPort);
        plineHeader = _G_plineAssHashUdp[iAssHash];
        break;
        
    case IP_PROTO_ICMP:
        if (p->len < (iphdrlen + sizeof(struct icmp_echo_hdr))) {
            return  (__NAT_STRONG_RULE);
        }
        icmphdr = (struct icmp_echo_hdr *)(((u8_t *)p->payload) + iphdrlen);
        usDestPort  = icmphdr->id;
        iAssHash    = __NAT_HASH(usDestPort);
        plineHeader = _G_plineAssHashIcmp[iAssHash];
        break;
    
    default:
        return  (__NAT_STRONG_RULE);                                    /*  ���ܴ����Э��              */
    }

    if ((ucProto != IP_PROTO_ICMP) &&
        ((PP_NTOHS(usDestPort) < LW_CFG_NET_NAT_MIN_PORT) || 
         (PP_NTOHS(usDestPort) > LW_CFG_NET_NAT_MAX_PORT))) {           /*  Ŀ��˿ڲ��ڴ���˿�֮��    */

        if ((ucProto == IP_PROTO_UDP) &&
            (udphdr->src == PP_HTONS(LWIP_IANA_PORT_DHCP_SERVER)) &&
            (udphdr->dest == PP_HTONS(LWIP_IANA_PORT_DHCP_CLIENT))) {   /*  ����Ϊ DHCP �ͻ���          */
            return  (0);                                                /*  ֱ�ӷ���                    */
        }

        for (plineTemp  = _G_plineNatmap;
             plineTemp != LW_NULL;
             plineTemp  = _list_line_get_next(plineTemp)) {
             
            pnatmap = _LIST_ENTRY(plineTemp, __NAT_MAP, NATM_lineManage);
            if ((usDestPort == pnatmap->NATM_usAssPort) &&
                (ucProto    == pnatmap->NATM_ucProto)) {
                break;                                                  /*  �ҵ��� NAT ӳ����ƿ�       */
            }
        }
        if (plineTemp == LW_NULL) {
            pnatmap = LW_NULL;                                          /*  û���ҵ�                    */
        }
        
        if (pnatmap) {
            ip4_addr_p_t  ipaddr;                                       /*  ����ӳ������� IP           */
            u16_t         usMapHash = iphdr->src.addr % pnatmap->NATM_usLocalCnt;
                                                                        /*  ���� hash                   */
            for (plineTemp  = plineHeader;                              /*  ��������ӳ��                */
                 plineTemp != LW_NULL;
                 plineTemp  = _list_line_get_next(plineTemp)) {

                pnatcb = _LIST_ENTRY(plineTemp, __NAT_CB, NAT_lineAssHash);
                if ((usMapHash  == pnatcb->NAT_usMapHash) &&
                    (usDestPort == pnatcb->NAT_usAssPort) &&
                    (ucProto    == pnatcb->NAT_ucProto)) {
                    break;                                              /*  �ҵ��� NAT ���ƿ�           */
                }
            }
            if (plineTemp == LW_NULL) {
                if (pnatmap->NATM_ipaddrLocalIp.addr == IPADDR_ANY) {
                    ipaddr.addr = netif_ip4_addr(netifIn)->addr;        /*  ����ӳ��                    */

                } else {
                    u32_t   uiHost;

                    uiHost  = (u32_t)PP_NTOHL(pnatmap->NATM_ipaddrLocalIp.addr);
                    uiHost += usMapHash;                                /*  ����Դ��ַɢ��������        */
                    ipaddr.addr = (u32_t)PP_HTONL(uiHost);
                }

                pnatcb = __natNew(&ipaddr,                              /*  �½����ƿ�                  */
                                  pnatmap->NATM_usLocalPort, ucProto);

                pnatcb->NAT_usMapHash     = usMapHash;
                pnatcb->NAT_usAssPortSave = pnatcb->NAT_usAssPort;      /*  ����˿���Դ                */
                pnatcb->NAT_usAssPort     = pnatmap->NATM_usAssPort;

                __natAssHashAdd(pnatcb);                                /*  ��ӵ� Ass Hash ��          */

            } else {
                ipaddr.addr = pnatcb->NAT_ipaddrLocal.addr;

                pnatcb->NAT_ulIdleTimer = 0;                            /*  ˢ�¿���ʱ��                */
            }
        
            /*
             *  �����ݰ�Ŀ��תΪ��������Ŀ���ַ
             */
            u32OldAddr = iphdr->dest.addr;
            ((ip4_addr_t *)&(iphdr->dest))->addr = ipaddr.addr;
            inet_chksum_adjust((u8_t *)&IPH_CHKSUM(iphdr), (u8_t *)&u32OldAddr, 4, (u8_t *)&iphdr->dest.addr, 4);
        
            /*
             *  �������͵����������ݰ�Ŀ��˿�Ϊ NAT_usLocalPort
             */
            if (IPH_PROTO(iphdr) == IP_PROTO_TCP) {
                inet_chksum_adjust((u8_t *)&tcphdr->chksum, (u8_t *)&u32OldAddr, 4, (u8_t *)&iphdr->dest.addr, 4);
                tcphdr->dest = pnatmap->NATM_usLocalPort;
                inet_chksum_adjust((u8_t *)&tcphdr->chksum, (u8_t *)&usDestPort, 2, (u8_t *)&tcphdr->dest, 2);
            
            } else if (IPH_PROTO(iphdr) == IP_PROTO_UDP) {
                if (udphdr->chksum != 0) {
                    inet_chksum_adjust((u8_t *)&udphdr->chksum, (u8_t *)&u32OldAddr, 4, (u8_t *)&iphdr->dest.addr, 4);
    	            udphdr->dest = pnatmap->NATM_usLocalPort;
    	            inet_chksum_adjust((u8_t *)&udphdr->chksum, (u8_t *)&usDestPort, 2, (u8_t *)&udphdr->dest, 2);
                }
                
            } else if ((IPH_PROTO(iphdr) == IP_PROTO_ICMP) && 
                       ((ICMPH_CODE(icmphdr) == ICMP_ECHO || ICMPH_CODE(icmphdr) == ICMP_ER))) {
                icmphdr->id = pnatmap->NATM_usLocalPort;
                inet_chksum_adjust((u8_t *)&icmphdr->chksum, (u8_t *)&usDestPort, 2, (u8_t *)&icmphdr->id, 2);
            }
        
        } else {
            return  (__NAT_STRONG_RULE);                                /*  �޷��ҵ� MAP �˿�           */
        }
    
    } else {                                                            /*  Ŀ��˿��ڴ���˿�֮��      */
        for (plineTemp  = plineHeader;
             plineTemp != LW_NULL;
             plineTemp  = _list_line_get_next(plineTemp)) {
            
            pnatcb = _LIST_ENTRY(plineTemp, __NAT_CB, NAT_lineAssHash);
            if ((usDestPort == pnatcb->NAT_usAssPort) &&
                (ucProto    == pnatcb->NAT_ucProto)) {
                break;                                                  /*  �ҵ��� NAT ���ƿ�           */
            }
        }
        if (plineTemp == LW_NULL) {
            pnatcb = LW_NULL;                                           /*  û���ҵ�                    */
        }
    
        if (pnatcb) {                                                   /*  ����ҵ����ƿ�              */
            pnatcb->NAT_ulIdleTimer = 0;                                /*  ˢ�¿���ʱ��                */
            
            /*
             *  �����ݰ�Ŀ��תΪ��������Ŀ���ַ
             */
            u32OldAddr = iphdr->dest.addr;
            ((ip4_addr_t *)&(iphdr->dest))->addr = pnatcb->NAT_ipaddrLocal.addr;
            inet_chksum_adjust((u8_t *)&IPH_CHKSUM(iphdr), (u8_t *)&u32OldAddr, 4, (u8_t *)&iphdr->dest.addr, 4);
            
            /*
             *  �������͵����������ݰ�Ŀ��˿�Ϊ NAT_usLocalPort
             */
            if (IPH_PROTO(iphdr) == IP_PROTO_TCP) {
                inet_chksum_adjust((u8_t *)&tcphdr->chksum, (u8_t *)&u32OldAddr, 4, (u8_t *)&iphdr->dest.addr, 4);
                tcphdr->dest = pnatcb->NAT_usLocalPort;
                inet_chksum_adjust((u8_t *)&tcphdr->chksum, (u8_t *)&usDestPort, 2, (u8_t *)&tcphdr->dest, 2);
            
            } else if (IPH_PROTO(iphdr) == IP_PROTO_UDP) {
                if (udphdr->chksum != 0) {
                    inet_chksum_adjust((u8_t *)&udphdr->chksum, (u8_t *)&u32OldAddr, 4, (u8_t *)&iphdr->dest.addr, 4);
    	            udphdr->dest = pnatcb->NAT_usLocalPort;
    	            inet_chksum_adjust((u8_t *)&udphdr->chksum, (u8_t *)&usDestPort, 2, (u8_t *)&udphdr->dest, 2);
                }
                
            } else if ((IPH_PROTO(iphdr) == IP_PROTO_ICMP) && 
                       ((ICMPH_CODE(icmphdr) == ICMP_ECHO || ICMPH_CODE(icmphdr) == ICMP_ER))) {
                icmphdr->id = pnatcb->NAT_usLocalPort;
                inet_chksum_adjust((u8_t *)&icmphdr->chksum, (u8_t *)&usDestPort, 2, (u8_t *)&icmphdr->id, 2);
            }
            
            /*
             *  NAT ���ƿ�״̬����
             */
            if (IPH_PROTO(iphdr) == IP_PROTO_TCP) {
                if (TCPH_FLAGS(tcphdr) & (TCP_RST | TCP_FIN)) {
                    u32_t   RemoteSeq     = PP_NTOHL(tcphdr->seqno);
                    u32_t   RemoteAck     = PP_NTOHL(tcphdr->ackno);
                    u32_t   LocalSequence = PP_NTOHL(pnatcb->NAT_uiLocalSequence);
                    u32_t   LocalRcvNext  = PP_NTOHL(pnatcb->NAT_uiLocalRcvNext);
                    
                    /*
                     *  ��ֹ RST FIN ����.
                     */
                    if ((RemoteAck == LocalSequence + 1) || (RemoteSeq == LocalRcvNext)) {
                        if (TCPH_FLAGS(tcphdr) & TCP_RST) {
                            pnatcb->NAT_iStatus = __NAT_STATUS_CLOSING;
                        
                        } else if (TCPH_FLAGS(tcphdr) & TCP_FIN) {
                            if (pnatcb->NAT_iStatus == __NAT_STATUS_OPEN) {
            	                pnatcb->NAT_iStatus = __NAT_STATUS_FIN;
            	            
            	            } else {
            	                pnatcb->NAT_iStatus = __NAT_STATUS_CLOSING;
            	            }
                        }
                    }
                }
            }
                                                                        /*  ������� PING ���ݰ�        */
        } else if ((ucProto != IP_PROTO_ICMP) ||
                   !ip4_addr_cmp(&iphdr->dest, netif_ip4_addr(netifIn))) {
            return  (__NAT_STRONG_RULE);                                /*  Ŀ�겻�Ǳ���                */
        }
    }

    return  (0);
}
/*********************************************************************************************************
** ��������: __natApOutput
** ��������: NAT AP ����ӿ����.
** �䡡��  : p             ���ݰ�
**           pnetifIn      ���ݰ���������ӿ�
**           netif         ���ݰ��������ӿ�
** �䡡��  : 0: ok 1: eaten
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static INT  __natApOutput (struct pbuf *p, struct netif  *pnetifIn, struct netif *netifOut)
{
    static u16_t             iphdrlen;
    struct ip_hdr           *iphdr   = (struct ip_hdr *)p->payload;
    struct tcp_hdr          *tcphdr  = LW_NULL;
    struct udp_hdr          *udphdr  = LW_NULL;
    struct icmp_echo_hdr    *icmphdr = LW_NULL;
    
    u32_t                    u32OldAddr;
    u16_t                    usSrcPort;
    u8_t                     ucProto;
    
    INT                      iHash;
    __PNAT_CB                pnatcb    = LW_NULL;
    __PNAT_ALIAS             pnatalias = LW_NULL;
    PLW_LIST_LINE            plineTemp;
    LW_LIST_LINE_HEADER      plineHeader;
    
    iphdrlen  = (u16_t)IPH_HL(iphdr);                                   /*  ��� IP ��ͷ����            */
    iphdrlen *= 4;
    
    if (p->len < iphdrlen) {                                            /*  �������                    */
        return  (1);                                                    /*  ɾ�������ݰ�                */
    }

    LWIP_ASSERT("NAT Output fragment error", 
                !(IPH_OFFSET(iphdr) & PP_HTONS(IP_OFFMASK | IP_MF)));   /*  �����ϲ����з�Ƭ��־        */
    
    ucProto = IPH_PROTO(iphdr);
    switch (ucProto) {
    
    case IP_PROTO_TCP:                                                  /*  TCP ���ݱ�                  */
        if (p->len < (iphdrlen + TCP_HLEN)) {
            return  (1);
        }
        tcphdr = (struct tcp_hdr *)(((u8_t *)p->payload) + iphdrlen);
        usSrcPort   = tcphdr->src;
        iHash       = __NAT_HASH(usSrcPort);
        plineHeader = _G_plineNatcbTcp[iHash];
        break;
        
    case IP_PROTO_UDP:                                                  /*  UDP ���ݱ�                  */
        if (p->len < (iphdrlen + UDP_HLEN)) {
            return  (1);
        }
        udphdr = (struct udp_hdr *)(((u8_t *)p->payload) + iphdrlen);
        usSrcPort   = udphdr->src;
        iHash       = __NAT_HASH(usSrcPort);
        plineHeader = _G_plineNatcbUdp[iHash];
        break;
        
    case IP_PROTO_ICMP:
        if (p->len < (iphdrlen + sizeof(struct icmp_echo_hdr))) {
            return  (1);
        }
        icmphdr = (struct icmp_echo_hdr *)(((u8_t *)p->payload) + iphdrlen);
        usSrcPort   = icmphdr->id;
        iHash       = __NAT_HASH(usSrcPort);
        plineHeader = _G_plineNatcbIcmp[iHash];
        break;
    
    default:
        return  (1);                                                    /*  ���ܴ����Э��              */
    }
    
    if (!pnetifIn && (ucProto == IP_PROTO_UDP) &&
        (udphdr->src == PP_HTONS(LWIP_IANA_PORT_DHCP_CLIENT)) &&
        (udphdr->dest == PP_HTONS(LWIP_IANA_PORT_DHCP_SERVER))) {       /*  ����Ϊ DHCP �ͻ���          */
        return  (0);                                                    /*  ֱ�ӷ���                    */
    }

    for (plineTemp  = plineHeader;
         plineTemp != LW_NULL;
         plineTemp  = _list_line_get_next(plineTemp)) {
        
        pnatcb = _LIST_ENTRY(plineTemp, __NAT_CB, NAT_lineManage);
        
        /*
         *  ���ƿ��ڵ�Դ�� IP �� Դ�� PORT ʹ��Э����ȫһ��.
         */
        if ((ip4_addr_cmp(&iphdr->src, &pnatcb->NAT_ipaddrLocal)) &&
            (usSrcPort == pnatcb->NAT_usLocalPort) &&
            (ucProto   == pnatcb->NAT_ucProto)) {
            break;                                                      /*  �ҵ��� NAT ���ƿ�           */
        }
    }
    if (plineTemp == LW_NULL) {
        pnatcb = __natNew(&iphdr->src, usSrcPort, ucProto);             /*  �½����ƿ�                  */

        __natAssHashAdd(pnatcb);
    }
    
    if (pnatcb) {
        pnatcb->NAT_ulIdleTimer = 0;                                    /*  ˢ�¿���ʱ��                */
        
        /*
         *  �����ݰ�ת��Ϊ���� AP ��������ӿڷ��͵����ݰ�
         *  ���������Ч, ��ʹ�ñ�����ΪԴ��ַ.
         */
        u32OldAddr = iphdr->src.addr;
        for (plineTemp  = _G_plineNatalias;
             plineTemp != LW_NULL;
             plineTemp  = _list_line_get_next(plineTemp)) {             /*  ���ұ�����                  */
        
            pnatalias = _LIST_ENTRY(plineTemp, __NAT_ALIAS, NATA_lineManage);
            if ((pnatalias->NATA_ipaddrSLocalIp.addr <= ((ip4_addr_t *)&(iphdr->src))->addr) &&
                (pnatalias->NATA_ipaddrELocalIp.addr >= ((ip4_addr_t *)&(iphdr->src))->addr)) {
                break;
            }
        }
        if (plineTemp) {                                                /*  Դ IP ʹ�ñ��� IP           */
            ((ip4_addr_t *)&(iphdr->src))->addr = pnatalias->NATA_ipaddrAliasIp.addr;
            
        } else {                                                        /*  Դ IP ʹ�� AP �ӿ� IP       */
            ((ip4_addr_t *)&(iphdr->src))->addr = netif_ip4_addr(netifOut)->addr;
        }
        inet_chksum_adjust((u8_t *)&IPH_CHKSUM(iphdr), (u8_t *)&u32OldAddr, 4, (u8_t *)&iphdr->src.addr, 4);
        
        /*
         *  �������͵����������ݰ�ʹ�� NAT_usAssPort (Ψһ�ķ���˿�)
         */
        switch (IPH_PROTO(iphdr)) {
        
        case IP_PROTO_TCP:
            inet_chksum_adjust((u8_t *)&tcphdr->chksum, (u8_t *)&u32OldAddr, 4, (u8_t *)&iphdr->src.addr, 4);
            tcphdr->src = pnatcb->NAT_usAssPort;
            inet_chksum_adjust((u8_t *)&tcphdr->chksum, (u8_t *)&usSrcPort, 2, (u8_t *)&tcphdr->src, 2);
            break;
            
        case IP_PROTO_UDP:
            if (udphdr->chksum != 0) {
                inet_chksum_adjust((u8_t *)&udphdr->chksum, (u8_t *)&u32OldAddr, 4, (u8_t *)&iphdr->src.addr, 4);
        	    udphdr->src = pnatcb->NAT_usAssPort;
        	    inet_chksum_adjust((u8_t *)&udphdr->chksum, (u8_t *)&usSrcPort, 2, (u8_t *)&udphdr->src, 2);
        	}
            break;
            
        case IP_PROTO_ICMP:
            icmphdr->id = pnatcb->NAT_usAssPort;
            inet_chksum_adjust((u8_t *)&icmphdr->chksum, (u8_t *)&usSrcPort, 2, (u8_t *)&icmphdr->id, 2);
            break;
            
        default:
            _BugHandle(LW_TRUE, LW_TRUE, "NAT Protocol error!\r\n");
            break;
        }
        
        /*
         *  NAT ���ƿ�״̬����
         */
        if (IPH_PROTO(iphdr) == IP_PROTO_TCP) {
            pnatcb->NAT_uiLocalSequence = tcphdr->seqno;
            if (TCPH_FLAGS(tcphdr) & TCP_ACK) {
                pnatcb->NAT_uiLocalRcvNext = tcphdr->ackno;
            }

            if (TCPH_FLAGS(tcphdr) & TCP_SYN) {
                pnatcb->NAT_iStatus = __NAT_STATUS_SYN;

            } else if (TCPH_FLAGS(tcphdr) & TCP_RST) {
                pnatcb->NAT_iStatus = __NAT_STATUS_CLOSING;

            } else if (TCPH_FLAGS(tcphdr) & TCP_FIN) {
                if (pnatcb->NAT_iStatus == __NAT_STATUS_OPEN) {
                    pnatcb->NAT_iStatus = __NAT_STATUS_FIN;

                } else {
                    pnatcb->NAT_iStatus = __NAT_STATUS_CLOSING;
                }

            } else {
                pnatcb->NAT_iStatus = __NAT_STATUS_OPEN;
            }
        }
        
    } else {
        return  (1);
    }

    return  (0);
}
/*********************************************************************************************************
** ��������: __natIpInput
** ��������: NAT IP ����ص�
** �䡡��  : p         ���ݰ�
**           pnetifIn  ��������ӿ�
**           pnetifOut �������ӿ�
** �䡡��  : pbuf
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static struct pbuf *__natIpInput (struct pbuf  *p, struct netif  *pnetifIn, struct netif  *pnetifOut)
{
    struct ip_hdr  *iphdr;
    const ip4_addr_t *ipaddr;
    
    iphdr  = (struct ip_hdr *)p->payload;
    ipaddr = netif_ip4_addr(pnetifIn);
    
    if ((ipaddr->addr != IPADDR_ANY) && !ip4_addr_cmp(&iphdr->dest, ipaddr)) {
        pbuf_free(p);                                                   /*  ֻ������������            */
        return  (LW_NULL);
    }

    if (IPH_OFFSET(iphdr) & PP_HTONS(IP_OFFMASK | IP_MF)) {             /*  ��Ƭ���ݰ�                  */
        switch (IPH_PROTO(iphdr)) {

        case IP_PROTO_TCP:
            if (!_G_bNatTcpFrag) {
                return  (p);
            }
            break;

        case IP_PROTO_UDP:
            if (!_G_bNatUdpFrag) {
                return  (p);
            }
            break;

        case IP_PROTO_ICMP:
            if (!_G_bNatIcmpFrag) {
                return  (p);
            }
            break;

        default:
            return  (p);
        }

#if IP_REASSEMBLY
        p = ip4_reass(p);                                               /*  ��ǰ���з�Ƭ����            */
        if (p == LW_NULL) {
            return  (p);                                                /*  ��Ƭ��ȫ                    */
        }
#else                                                                   /*  IP_REASSEMBLY               */
        return  (p);
#endif                                                                  /*  !IP_REASSEMBLY              */
    }

    if (__natApInput(p, pnetifIn)) {                                    /*  NAT ����                    */
        pbuf_free(p);
        p = LW_NULL;
    }
    
    return  (p);
}
/*********************************************************************************************************
** ��������: __natIpOutput
** ��������: NAT IP ����ص�
** �䡡��  : p         ���ݰ�
**           pnetifIn  ��������ӿ�
**           pnetifOut �������ӿ�
** �䡡��  : pbuf
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static struct pbuf *__natIpOutput (struct pbuf  *p, struct netif  *pnetifIn, struct netif  *pnetifOut)
{
    struct ip_hdr  *iphdr;
    
    iphdr = (struct ip_hdr *)p->payload;

    if (!pnetifIn) {                                                    /*  ��������                    */
        if (!(IPH_OFFSET(iphdr) & PP_HTONS(IP_OFFMASK | IP_MF))) {
            __natApOutput(p, pnetifIn, pnetifOut);                      /*  NAT ���                    */
        }
        return  (p);
    }

    if (IPH_OFFSET(iphdr) & PP_HTONS(IP_OFFMASK | IP_MF)) {             /*  ��Ƭ���ݰ�                  */
        switch (IPH_PROTO(iphdr)) {

        case IP_PROTO_TCP:
            if (!_G_bNatTcpFrag) {
                pbuf_free(p);
                return  (LW_NULL);
            }
            break;

        case IP_PROTO_UDP:
            if (!_G_bNatUdpFrag) {
                pbuf_free(p);
                return  (LW_NULL);
            }
            break;

        case IP_PROTO_ICMP:
            if (!_G_bNatIcmpFrag) {
                pbuf_free(p);
                return  (LW_NULL);
            }
            break;

        default:
            pbuf_free(p);
            return  (LW_NULL);
        }

#if IP_REASSEMBLY
        p = ip4_reass(p);                                               /*  ��ǰ���з�Ƭ����            */
        if (p == LW_NULL) {
            return  (p);                                                /*  ��Ƭ��ȫ                    */
        }
#else
        pbuf_free(p);
        return  (LW_NULL);
#endif
    }

    if (__natApOutput(p, pnetifIn, pnetifOut)) {                        /*  NAT ���                    */
        pbuf_free(p);
        p = LW_NULL;
    }
    
    return  (p);
}
/*********************************************************************************************************
** ��������: __natIphook
** ��������: NAT IP �ص�
** �䡡��  : iIpType   Э������
**           iHookType �ص�����
**           p         ���ݰ�
**           pnetifIn  ��������ӿ�
**           pnetifOut �������ӿ�
** �䡡��  : pbuf
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static struct pbuf *__natIphook (INT  iIpType, INT  iHookType, struct pbuf  *p, 
                                 struct netif  *pnetifIn, struct netif  *pnetifOut)
{
    if (iIpType != IP_HOOK_V4) {
        return  (p);
    }
    
    switch (iHookType) {
    
    case IP_HT_NAT_PRE_ROUTING:
        return  (__natIpInput(p, pnetifIn, pnetifOut));
        
    case IP_HT_NAT_POST_ROUTING:
        return  (__natIpOutput(p, pnetifIn, pnetifOut));
        
    default:
        break;
    }
    
    return  (p);
}
/*********************************************************************************************************
** ��������: __natInit
** ��������: NAT ��ʼ��
** �䡡��  : NONE
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
VOID  __natInit (VOID)
{
    __natPoolInit();
    _G_ulNatcbTimer = API_TimerCreate("nat_timer", LW_OPTION_ITIMER | LW_OPTION_OBJECT_GLOBAL, LW_NULL);
    _BugHandle((_G_ulNatcbTimer == LW_OBJECT_HANDLE_INVALID), LW_TRUE, "can not create 'nat_timer'\r\n");
}
/*********************************************************************************************************
** ��������: __natStart
** ��������: NAT ����
** �䡡��  : pcLocal    ��������ӿ�
**           pcAp       ��������ӿ�
** �䡡��  : ERROR or OK
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
INT  __natStart (CPCHAR  pcLocal, CPCHAR  pcAp)
{
    if (_G_bNatStart) {
        _ErrorHandle(EBUSY);
        return  (PX_ERROR);
    }
    
    if (net_ip_hook_nat_add(__natIphook)) {
        return  (PX_ERROR);
    }

    lib_strlcpy(_G_natifAp[0].NATIF_cIfName, pcAp, IF_NAMESIZE);
    lib_strlcpy(_G_natifLocal[0].NATIF_cIfName, pcLocal, IF_NAMESIZE);
    
    __NAT_LOCK();
    _G_natifLocal[0].NATIF_pnetif = netif_find(pcLocal);
    if (_G_natifLocal[0].NATIF_pnetif) {
        _G_natifLocal[0].NATIF_pnetif->nat_mode = NETIF_NAT_LOCAL;
    }

    _G_natifAp[0].NATIF_pnetif = netif_find(pcAp);
    if (_G_natifAp[0].NATIF_pnetif) {
        _G_natifAp[0].NATIF_pnetif->nat_mode = NETIF_NAT_AP;
    }
    __NAT_UNLOCK();
    
    API_TimerStart(_G_ulNatcbTimer, (LW_TICK_HZ * 10), LW_OPTION_AUTO_RESTART,
                   (PTIMER_CALLBACK_ROUTINE)__natTimer, (PVOID)10);
    
    _G_bNatStart = LW_TRUE;
    KN_SMP_WMB();

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: __natStop
** ��������: NAT ����
** �䡡��  : NONE
** �䡡��  : ERROR or OK
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
INT  __natStop (VOID)
{
    INT  i;

    if (net_ip_hook_nat_delete(__natIphook)) {
        return  (PX_ERROR);
    }
    
    API_TimerCancel(_G_ulNatcbTimer);
    __natTimer(0);
    
    __NAT_LOCK();
    for (i = 1; i < LW_CFG_NET_NAT_MAX_AP_IF; i++) {
        _G_natifAp[i].NATIF_cIfName[0] = PX_EOS;
        if (_G_natifAp[i].NATIF_pnetif) {
            _G_natifAp[i].NATIF_pnetif->nat_mode = NETIF_NAT_NONE;
            _G_natifAp[i].NATIF_pnetif = LW_NULL;
        }
    }
    
    for (i = 1; i < LW_CFG_NET_NAT_MAX_LOCAL_IF; i++) {
        _G_natifLocal[i].NATIF_cIfName[0] = PX_EOS;
        if (_G_natifLocal[i].NATIF_pnetif) {
            _G_natifLocal[i].NATIF_pnetif->nat_mode = NETIF_NAT_NONE;
            _G_natifLocal[i].NATIF_pnetif = LW_NULL;
        }
    }
    _G_bNatStart = LW_FALSE;
    __NAT_UNLOCK();
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: __natIpFragSet
** ��������: NAT ���� IP ��Ƭ֧��
** �䡡��  : ucProto    ����Э�� IPPROTO_UDP / IPPROTO_TCP / IPPROTO_ICMP
**           bOn        �Ƿ�ʹ�ܷ�Ƭ
** �䡡��  : ERROR or OK
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
INT  __natIpFragSet (UINT8  ucProto, BOOL  bOn)
{
    switch (ucProto) {
    
    case IP_PROTO_TCP:
        _G_bNatTcpFrag = bOn;
        break;
    
    case IP_PROTO_UDP:
        _G_bNatUdpFrag = bOn;
        break;
    
    case IP_PROTO_ICMP:
        _G_bNatIcmpFrag = bOn;
        break;
        
    default:
        return  (PX_ERROR);
    }
    
    KN_SMP_MB();
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: __natIpFragGet
** ��������: NAT ��ȡ IP ��Ƭ֧��
** �䡡��  : ucProto    ����Э�� IPPROTO_UDP / IPPROTO_TCP / IPPROTO_ICMP
**           pbOn       �Ƿ�ʹ�ܷ�Ƭ
** �䡡��  : ERROR or OK
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
INT  __natIpFragGet (UINT8  ucProto, BOOL  *pbOn)
{
    switch (ucProto) {
    
    case IP_PROTO_TCP:
        *pbOn = _G_bNatTcpFrag;
        break;
    
    case IP_PROTO_UDP:
        *pbOn = _G_bNatUdpFrag;
        break;
    
    case IP_PROTO_ICMP:
        *pbOn = _G_bNatIcmpFrag;
        break;
        
    default:
        return  (PX_ERROR);
    }
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: __natAddLocal
** ��������: NAT ����һ����������ӿ�
** �䡡��  : pcLocal    ��������ӿ�
** �䡡��  : ERROR or OK
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
INT  __natAddLocal (CPCHAR  pcLocal)
{
    INT  i;

    __NAT_LOCK();
    if (!_G_bNatStart) {
        __NAT_UNLOCK();
        _ErrorHandle(ENOTCONN);
        return  (PX_ERROR);
    }
    for (i = 0; i < LW_CFG_NET_NAT_MAX_LOCAL_IF; i++) {
        if (_G_natifLocal[i].NATIF_cIfName[0] == PX_EOS) {
            lib_strlcpy(_G_natifLocal[i].NATIF_cIfName, pcLocal, IF_NAMESIZE);
            break;
        }
    }
    if (i >= LW_CFG_NET_NAT_MAX_LOCAL_IF) {
        __NAT_UNLOCK();
        _ErrorHandle(ENOSPC);
        return  (PX_ERROR);
    }
    __NAT_UNLOCK();

    _G_natifLocal[i].NATIF_pnetif = netif_find(pcLocal);
    if (_G_natifLocal[i].NATIF_pnetif) {
        _G_natifLocal[i].NATIF_pnetif->nat_mode = NETIF_NAT_LOCAL;
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: __natDeleteLocal
** ��������: NAT ɾ��һ����������ӿ�
** �䡡��  : pcLocal    ��������ӿ�
** �䡡��  : ERROR or OK
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
INT  __natDeleteLocal (CPCHAR  pcLocal)
{
    INT  i;

    __NAT_LOCK();
    if (!_G_bNatStart) {
        __NAT_UNLOCK();
        _ErrorHandle(ENOTCONN);
        return  (PX_ERROR);
    }
    for (i = 0; i < LW_CFG_NET_NAT_MAX_LOCAL_IF; i++) {
        if (_G_natifLocal[i].NATIF_cIfName[0] &&
            !lib_strcmp(_G_natifLocal[i].NATIF_cIfName, pcLocal)) {
            _G_natifLocal[i].NATIF_cIfName[0] = PX_EOS;
            if (_G_natifLocal[i].NATIF_pnetif) {
                _G_natifLocal[i].NATIF_pnetif->nat_mode = NETIF_NAT_NONE;
                _G_natifLocal[i].NATIF_pnetif = NULL;
            }
            break;
        }
    }
    if (i >= LW_CFG_NET_NAT_MAX_LOCAL_IF) {
        __NAT_UNLOCK();
        _ErrorHandle(ENODEV);
        return  (PX_ERROR);
    }
    __NAT_UNLOCK();

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: __natAddAp
** ��������: NAT ����һ����������ӿ�
** �䡡��  : pcAp       ��������ӿ�
** �䡡��  : ERROR or OK
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
INT  __natAddAp (CPCHAR  pcAp)
{
    INT  i;

    __NAT_LOCK();
    if (!_G_bNatStart) {
        __NAT_UNLOCK();
        _ErrorHandle(ENOTCONN);
        return  (PX_ERROR);
    }
    for (i = 0; i < LW_CFG_NET_NAT_MAX_AP_IF; i++) {
        if (_G_natifAp[i].NATIF_cIfName[0] == PX_EOS) {
            lib_strlcpy(_G_natifAp[i].NATIF_cIfName, pcAp, IF_NAMESIZE);
            break;
        }
    }
    if (i >= LW_CFG_NET_NAT_MAX_AP_IF) {
        __NAT_UNLOCK();
        _ErrorHandle(ENOSPC);
        return  (PX_ERROR);
    }
    __NAT_UNLOCK();

    _G_natifAp[i].NATIF_pnetif = netif_find(pcAp);
    if (_G_natifAp[i].NATIF_pnetif) {
        _G_natifAp[i].NATIF_pnetif->nat_mode = NETIF_NAT_AP;
    }
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: __natDeleteAp
** ��������: NAT ɾ��һ����������ӿ�
** �䡡��  : pcAp       ��������ӿ�
** �䡡��  : ERROR or OK
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
INT  __natDeleteAp (CPCHAR  pcAp)
{
    INT  i;

    __NAT_LOCK();
    if (!_G_bNatStart) {
        __NAT_UNLOCK();
        _ErrorHandle(ENOTCONN);
        return  (PX_ERROR);
    }
    for (i = 0; i < LW_CFG_NET_NAT_MAX_AP_IF; i++) {
        if (_G_natifAp[i].NATIF_cIfName[0] &&
            !lib_strcmp(_G_natifAp[i].NATIF_cIfName, pcAp)) {
            _G_natifAp[i].NATIF_cIfName[0] = PX_EOS;
            if (_G_natifAp[i].NATIF_pnetif) {
                _G_natifAp[i].NATIF_pnetif->nat_mode = NETIF_NAT_NONE;
                _G_natifAp[i].NATIF_pnetif = NULL;
            }
            break;
        }
    }
    if (i >= LW_CFG_NET_NAT_MAX_AP_IF) {
        __NAT_UNLOCK();
        _ErrorHandle(ENODEV);
        return  (PX_ERROR);
    }
    __NAT_UNLOCK();

    return  (ERROR_NONE);
}
/*********************************************************************************************************
  NAT proc
*********************************************************************************************************/
#if LW_CFG_PROCFS_EN > 0
/*********************************************************************************************************
  NAT proc �ļ���������
*********************************************************************************************************/
static ssize_t  __procFsNatSummaryRead(PLW_PROCFS_NODE  p_pfsn, 
                                       PCHAR            pcBuffer, 
                                       size_t           stMaxBytes,
                                       off_t            oft);
static ssize_t  __procFsNatAssNodeRead(PLW_PROCFS_NODE  p_pfsn, 
                                       PCHAR            pcBuffer, 
                                       size_t           stMaxBytes,
                                       off_t            oft);
/*********************************************************************************************************
  NAT proc �ļ�����������
*********************************************************************************************************/
static LW_PROCFS_NODE_OP        _G_pfsnoNatSummaryFuncs = {
    __procFsNatSummaryRead, LW_NULL
};
static LW_PROCFS_NODE_OP        _G_pfsnoNatAssNodeFuncs = {
    __procFsNatAssNodeRead, LW_NULL
};
/*********************************************************************************************************
  NAT proc �ļ�Ŀ¼��
*********************************************************************************************************/
static LW_PROCFS_NODE           _G_pfsnNat[] = 
{
    LW_PROCFS_INIT_NODE("nat", 
                        (S_IFDIR | S_IRUSR | S_IRGRP | S_IROTH), 
                        LW_NULL, 
                        LW_NULL,  
                        0),
                        
    LW_PROCFS_INIT_NODE("info", 
                        (S_IFREG | S_IRUSR | S_IRGRP | S_IROTH), 
                        &_G_pfsnoNatSummaryFuncs, 
                        "I",
                        0),
                        
    LW_PROCFS_INIT_NODE("assnode", 
                        (S_IFREG | S_IRUSR | S_IRGRP | S_IROTH), 
                        &_G_pfsnoNatAssNodeFuncs, 
                        "A",
                        0),
};
/*********************************************************************************************************
** ��������: __procFsNatSummaryRead
** ��������: procfs ��һ���ں� nat/info proc �ļ�
** �䡡��  : p_pfsn        �ڵ���ƿ�
**           pcBuffer      ������
**           stMaxBytes    ��������С
**           oft           �ļ�ָ��
** �䡡��  : ʵ�ʶ�ȡ����Ŀ
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static ssize_t  __procFsNatSummaryRead (PLW_PROCFS_NODE  p_pfsn, 
                                        PCHAR            pcBuffer, 
                                        size_t           stMaxBytes,
                                        off_t            oft)
{
    const CHAR   cAliasInfoHeader[] = "\n"
    "     ALIAS        LOCAL START     LOCAL END\n"
    "--------------- --------------- ---------------\n";

    const CHAR   cMapInfoHeader[] = "\n"
    " ASS PORT  LOCAL PORT    LOCAL IP      IP CNT   PROTO\n"
    "---------- ---------- --------------- -------- -------\n";
    
          PCHAR     pcFileBuffer;
          size_t    stRealSize;                                         /*  ʵ�ʵ��ļ����ݴ�С          */
          size_t    stCopeBytes;
          
    /*
     *  ����Ԥ���ڴ��СΪ 0 , ���Դ򿪺��һ�ζ�ȡ��Ҫ�ֶ������ڴ�.
     */
    pcFileBuffer = (PCHAR)API_ProcFsNodeBuffer(p_pfsn);
    if (pcFileBuffer == LW_NULL) {                                      /*  ��û�з����ڴ�              */
        size_t          stNeedBufferSize;
        UINT            i;
        UINT            uiAssNum = 0;
        
        PLW_LIST_LINE   plineTemp;
        PCHAR           pcProto;
        __PNAT_ALIAS    pnatalias;
        __PNAT_MAP      pnatmap;

        stNeedBufferSize = 512;                                         /*  ��ʼ��С                    */
        
        __NAT_LOCK();
        for (plineTemp  = _G_plineNatalias;
             plineTemp != LW_NULL;
             plineTemp  = _list_line_get_next(plineTemp)) {             /*  ������                      */
            stNeedBufferSize += 64;
        }
        for (plineTemp  = _G_plineNatmap;
             plineTemp != LW_NULL;
             plineTemp  = _list_line_get_next(plineTemp)) {
            stNeedBufferSize += 64;
        }
        __NAT_UNLOCK();
        
        if (API_ProcFsAllocNodeBuffer(p_pfsn, stNeedBufferSize)) {
            _ErrorHandle(ENOMEM);
            return  (0);
        }
        pcFileBuffer = (PCHAR)API_ProcFsNodeBuffer(p_pfsn);             /*  ���»���ļ���������ַ      */
        
        __NAT_LOCK();
        stRealSize = bnprintf(pcFileBuffer, stNeedBufferSize, 0, 
                              "NAT networking alias setting >>\n");
        stRealSize = bnprintf(pcFileBuffer, stNeedBufferSize, stRealSize, 
                              "%s", cAliasInfoHeader);
        
        for (plineTemp  = _G_plineNatalias;
             plineTemp != LW_NULL;
             plineTemp  = _list_line_get_next(plineTemp)) {             /*  ������                      */
        
            CHAR    cIpBuffer1[INET_ADDRSTRLEN];
            CHAR    cIpBuffer2[INET_ADDRSTRLEN];
            CHAR    cIpBuffer3[INET_ADDRSTRLEN];
            
            pnatalias  = _LIST_ENTRY(plineTemp, __NAT_ALIAS, NATA_lineManage);
            stRealSize = bnprintf(pcFileBuffer, stNeedBufferSize, stRealSize, 
                                  "%-15s %-15s %-15s\n", 
                                  ip4addr_ntoa_r(&pnatalias->NATA_ipaddrAliasIp,
                                                 cIpBuffer1, INET_ADDRSTRLEN),
                                  ip4addr_ntoa_r(&pnatalias->NATA_ipaddrSLocalIp, 
                                                 cIpBuffer2, INET_ADDRSTRLEN),
                                  ip4addr_ntoa_r(&pnatalias->NATA_ipaddrELocalIp, 
                                                 cIpBuffer3, INET_ADDRSTRLEN));
        }
        
        stRealSize = bnprintf(pcFileBuffer, stNeedBufferSize, stRealSize, 
                              "\nNAT networking direct map setting >>\n");
        stRealSize = bnprintf(pcFileBuffer, stNeedBufferSize, stRealSize, 
                              "%s", cMapInfoHeader);
                              
        for (plineTemp  = _G_plineNatmap;
             plineTemp != LW_NULL;
             plineTemp  = _list_line_get_next(plineTemp)) {             /*  ӳ���                      */
            
            CHAR    cIpBuffer[INET_ADDRSTRLEN];
            
            pnatmap = _LIST_ENTRY(plineTemp, __NAT_MAP, NATM_lineManage);
            switch (pnatmap->NATM_ucProto) {
            
            case IP_PROTO_ICMP: pcProto = "ICMP"; break;
            case IP_PROTO_UDP:  pcProto = "UDP";  break;
            case IP_PROTO_TCP:  pcProto = "TCP";  break;
            default:            pcProto = "?";    break;
            }
            
            stRealSize = bnprintf(pcFileBuffer, stNeedBufferSize, stRealSize, 
                                  "%10d %10d %-15s %8d %s\n",
                                  PP_NTOHS(pnatmap->NATM_usAssPort),
                                  PP_NTOHS(pnatmap->NATM_usLocalPort),
                                  ip4addr_ntoa_r(&pnatmap->NATM_ipaddrLocalIp, 
                                                 cIpBuffer, INET_ADDRSTRLEN),
                                  pnatmap->NATM_usLocalCnt,
                                  pcProto);
        }
        
        stRealSize = bnprintf(pcFileBuffer, stNeedBufferSize, stRealSize, 
                              "\nNAT networking summary >>\n");
        
        if (!_G_bNatStart) {
            __NAT_UNLOCK();
            stRealSize = bnprintf(pcFileBuffer, stNeedBufferSize, stRealSize, 
                                  "    NAT networking off!\n");
        } else {
            __NAT_HASH_FOR_EACH(i) {
                for (plineTemp  = _G_plineNatcbTcp[i];
                     plineTemp != LW_NULL;
                     plineTemp  = _list_line_get_next(plineTemp)) {
                    uiAssNum++;
                }
            }
            __NAT_HASH_FOR_EACH(i) {
                for (plineTemp  = _G_plineNatcbUdp[i];
                     plineTemp != LW_NULL;
                     plineTemp  = _list_line_get_next(plineTemp)) {
                    uiAssNum++;
                }
            }
            __NAT_HASH_FOR_EACH(i) {
                for (plineTemp  = _G_plineNatcbIcmp[i];
                     plineTemp != LW_NULL;
                     plineTemp  = _list_line_get_next(plineTemp)) {
                    uiAssNum++;
                }
            }
            
            stRealSize = bnprintf(pcFileBuffer, stNeedBufferSize, stRealSize, 
                                  "    LAN: ");
                                  
            for (i = 0; i < LW_CFG_NET_NAT_MAX_LOCAL_IF; i++) {
                if (_G_natifLocal[i].NATIF_cIfName[0]) {
                    stRealSize = bnprintf(pcFileBuffer, stNeedBufferSize, stRealSize, 
                                          "%s ", _G_natifLocal[i].NATIF_cIfName);
                }
            }
            
            stRealSize = bnprintf(pcFileBuffer, stNeedBufferSize, stRealSize, 
                                  "\n    WAN: ");
            
            for (i = 0; i < LW_CFG_NET_NAT_MAX_AP_IF; i++) {
                if (_G_natifAp[i].NATIF_cIfName[0]) {
                    stRealSize = bnprintf(pcFileBuffer, stNeedBufferSize, stRealSize, 
                                          "%s ", _G_natifAp[i].NATIF_cIfName);
                }
            }
            __NAT_UNLOCK();
            
            stRealSize = bnprintf(pcFileBuffer, stNeedBufferSize, stRealSize, 
                                  "\n    Total Ass-node: %d\n", LW_CFG_NET_NAT_MAX_SESSION);
            stRealSize = bnprintf(pcFileBuffer, stNeedBufferSize, stRealSize, 
                                  "    Used  Ass-node: %d\n", uiAssNum);
        }
        
        stRealSize = bnprintf(pcFileBuffer, stNeedBufferSize, stRealSize, 
                              "    IP Fragment: TCP-%s UDP-%s ICMP-%s\n", 
                              _G_bNatTcpFrag  ? "Enable" : "Disable",
                              _G_bNatUdpFrag  ? "Enable" : "Disable",
                              _G_bNatIcmpFrag ? "Enable" : "Disable");
        
        API_ProcFsNodeSetRealFileSize(p_pfsn, stRealSize);
    } else {
        stRealSize = API_ProcFsNodeGetRealFileSize(p_pfsn);
    }
    if (oft >= stRealSize) {
        _ErrorHandle(ENOSPC);
        return  (0);
    }
    
    stCopeBytes  = __MIN(stMaxBytes, (size_t)(stRealSize - oft));       /*  ����ʵ�ʿ������ֽ���        */
    lib_memcpy(pcBuffer, (CPVOID)(pcFileBuffer + oft), (UINT)stCopeBytes);
    
    return  ((ssize_t)stCopeBytes);
}
/*********************************************************************************************************
** ��������: __procFsNatAssNodeRead
** ��������: procfs ��һ���ں� nat/assnode proc �ļ�
** �䡡��  : p_pfsn        �ڵ���ƿ�
**           pcBuffer      ������
**           stMaxBytes    ��������С
**           oft           �ļ�ָ��
** �䡡��  : ʵ�ʶ�ȡ����Ŀ
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static ssize_t  __procFsNatAssNodeRead (PLW_PROCFS_NODE  p_pfsn, 
                                        PCHAR            pcBuffer, 
                                        size_t           stMaxBytes,
                                        off_t            oft)
{
    const CHAR   cNatInfoHeader[] = "\n"
    "    LOCAL IP    LOCAL PORT ASS PORT PROTO IDLE(sec)  STATUS\n"
    "--------------- ---------- -------- ----- --------- --------\n";
    
          PCHAR     pcFileBuffer;
          size_t    stRealSize;                                         /*  ʵ�ʵ��ļ����ݴ�С          */
          size_t    stCopeBytes;
          
    /*
     *  ����Ԥ���ڴ��СΪ 0 , ���Դ򿪺��һ�ζ�ȡ��Ҫ�ֶ������ڴ�.
     */
    pcFileBuffer = (PCHAR)API_ProcFsNodeBuffer(p_pfsn);
    if (pcFileBuffer == LW_NULL) {                                      /*  ��û�з����ڴ�              */
        size_t          stNeedBufferSize;
        PCHAR           pcProto;
        PCHAR           pcStatus;
        INT             i;
        
        __PNAT_CB       pnatcb = LW_NULL;
        PLW_LIST_LINE   plineTemp;

        stNeedBufferSize = 256;                                         /*  ��ʼ��С                    */
        
        __NAT_LOCK();
        __NAT_HASH_FOR_EACH(i) {
            for (plineTemp  = _G_plineNatcbTcp[i];
                 plineTemp != LW_NULL;
                 plineTemp  = _list_line_get_next(plineTemp)) {
                stNeedBufferSize += 64;
            }
        }
        __NAT_HASH_FOR_EACH(i) {
            for (plineTemp  = _G_plineNatcbUdp[i];
                 plineTemp != LW_NULL;
                 plineTemp  = _list_line_get_next(plineTemp)) {
                stNeedBufferSize += 64;
            }
        }
        __NAT_HASH_FOR_EACH(i) {
            for (plineTemp  = _G_plineNatcbIcmp[i];
                 plineTemp != LW_NULL;
                 plineTemp  = _list_line_get_next(plineTemp)) {
                stNeedBufferSize += 64;
            }
        }
        __NAT_UNLOCK();
    
        if (API_ProcFsAllocNodeBuffer(p_pfsn, stNeedBufferSize)) {
            _ErrorHandle(ENOMEM);
            return  (0);
        }
        pcFileBuffer = (PCHAR)API_ProcFsNodeBuffer(p_pfsn);             /*  ���»���ļ���������ַ      */
        
        stRealSize = bnprintf(pcFileBuffer, stNeedBufferSize, 0, cNatInfoHeader); 
                                                                        /*  ��ӡͷ��Ϣ                  */
        __NAT_LOCK();
        __NAT_HASH_FOR_EACH(i) {
            for (plineTemp  = _G_plineNatcbTcp[i];
                 plineTemp != LW_NULL;
                 plineTemp  = _list_line_get_next(plineTemp)) {

                struct in_addr  inaddr;
                CHAR            cIpBuffer[INET_ADDRSTRLEN];

                pnatcb = _LIST_ENTRY(plineTemp, __NAT_CB, NAT_lineManage);
                inaddr.s_addr = pnatcb->NAT_ipaddrLocal.addr;

                switch (pnatcb->NAT_iStatus) {

                case __NAT_STATUS_OPEN:
                    pcStatus = "OPEN";
                    break;

                case __NAT_STATUS_SYN:
                    pcStatus = "SYN";
                    break;

                case __NAT_STATUS_FIN:
                    pcStatus = "FIN";
                    break;

                case __NAT_STATUS_CLOSING:
                    pcStatus = "CLOSING";
                    break;

                default:
                    pcStatus = "?";
                    break;
                }

                inet_ntoa_r(inaddr, cIpBuffer, INET_ADDRSTRLEN);
                stRealSize = bnprintf(pcFileBuffer, stNeedBufferSize, stRealSize,
                                      "%-15s %10d %8d %-5s %9ld %-8s\n", cIpBuffer,
                                      PP_NTOHS(pnatcb->NAT_usLocalPort),
                                      PP_NTOHS(pnatcb->NAT_usAssPort),
                                      "TCP",
                                      pnatcb->NAT_ulIdleTimer,
                                      pcStatus);
            }
        }
        
        __NAT_HASH_FOR_EACH(i) {
            for (plineTemp  = _G_plineNatcbUdp[i];
                 plineTemp != LW_NULL;
                 plineTemp  = _list_line_get_next(plineTemp)) {

                struct in_addr  inaddr;
                CHAR            cIpBuffer[INET_ADDRSTRLEN];

                pnatcb = _LIST_ENTRY(plineTemp, __NAT_CB, NAT_lineManage);
                inaddr.s_addr = pnatcb->NAT_ipaddrLocal.addr;
                if (pnatcb->NAT_iStatus == __NAT_STATUS_OPEN) {
                    pcStatus = "OPEN";
                } else {
                    pcStatus = "?";
                }

                inet_ntoa_r(inaddr, cIpBuffer, INET_ADDRSTRLEN);
                stRealSize = bnprintf(pcFileBuffer, stNeedBufferSize, stRealSize,
                                      "%-15s %10d %8d %-5s %9ld %-8s\n", cIpBuffer,
                                      PP_NTOHS(pnatcb->NAT_usLocalPort),
                                      PP_NTOHS(pnatcb->NAT_usAssPort),
                                      "UDP",
                                      pnatcb->NAT_ulIdleTimer,
                                      pcStatus);
            }
        }
        
        __NAT_HASH_FOR_EACH(i) {
            for (plineTemp  = _G_plineNatcbIcmp[i];
                 plineTemp != LW_NULL;
                 plineTemp  = _list_line_get_next(plineTemp)) {

                struct in_addr  inaddr;
                CHAR            cIpBuffer[INET_ADDRSTRLEN];

                pnatcb = _LIST_ENTRY(plineTemp, __NAT_CB, NAT_lineManage);
                inaddr.s_addr = pnatcb->NAT_ipaddrLocal.addr;
                if (pnatcb->NAT_ucProto == IP_PROTO_ICMP) {
                    pcProto = "ICMP";
                } else {
                    pcProto = "?";
                }

                if (pnatcb->NAT_iStatus == __NAT_STATUS_OPEN) {
                    pcStatus = "OPEN";
                } else {
                    pcStatus = "?";
                }

                inet_ntoa_r(inaddr, cIpBuffer, INET_ADDRSTRLEN);
                stRealSize = bnprintf(pcFileBuffer, stNeedBufferSize, stRealSize,
                                      "%-15s %10d %8d %-5s %9ld %-8s\n", cIpBuffer,
                                      PP_NTOHS(pnatcb->NAT_usLocalPort),
                                      PP_NTOHS(pnatcb->NAT_usAssPort),
                                      pcProto,
                                      pnatcb->NAT_ulIdleTimer,
                                      pcStatus);
            }
        }
        __NAT_UNLOCK();
        
        API_ProcFsNodeSetRealFileSize(p_pfsn, stRealSize);
    } else {
        stRealSize = API_ProcFsNodeGetRealFileSize(p_pfsn);
    }
    if (oft >= stRealSize) {
        _ErrorHandle(ENOSPC);
        return  (0);
    }
    
    stCopeBytes  = __MIN(stMaxBytes, (size_t)(stRealSize - oft));       /*  ����ʵ�ʿ������ֽ���        */
    lib_memcpy(pcBuffer, (CPVOID)(pcFileBuffer + oft), (UINT)stCopeBytes);
    
    return  ((ssize_t)stCopeBytes);
}
/*********************************************************************************************************
** ��������: __procFsNatInit
** ��������: procfs ��ʼ���ں� NAT �ļ�
** �䡡��  : NONE
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
VOID  __procFsNatInit (VOID)
{
    API_ProcFsMakeNode(&_G_pfsnNat[0], "/net");
    API_ProcFsMakeNode(&_G_pfsnNat[1], "/net/nat");
    API_ProcFsMakeNode(&_G_pfsnNat[2], "/net/nat");
}

#endif                                                                  /*  LW_CFG_PROCFS_EN > 0        */
#endif                                                                  /*  LW_CFG_NET_EN > 0           */
                                                                        /*  LW_CFG_NET_ROUTER > 0       */
                                                                        /*  LW_CFG_NET_NAT_EN > 0       */
/*********************************************************************************************************
  END
*********************************************************************************************************/
