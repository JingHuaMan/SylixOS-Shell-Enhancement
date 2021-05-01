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
** ��   ��   ��: lwip_qos.c
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2018 �� 08 �� 06 ��
**
** ��        ��: QoS �û������ù���.
*********************************************************************************************************/
#define  __SYLIXOS_STDIO
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
#include "../SylixOS/system/include/s_system.h"
#include "../SylixOS/net/include/net_net.h"
/*********************************************************************************************************
  �ü�����
*********************************************************************************************************/
#if (LW_CFG_NET_EN > 0) && (LW_CFG_NET_QOS_EN > 0)
#include "net/if.h"
#include "net/if_lock.h"
#include "lwip/netif.h"
#include "lwip/tcpip.h"
#include "lwip/inet.h"
#include "lwip/prot/udp.h"
#include "lwip/prot/tcp.h"
#include "lwip/priv/tcp_priv.h"
/*********************************************************************************************************
  �궨��
*********************************************************************************************************/
#define __QOS_NETIF_RULE_MAX        3                                   /*  ���������                  */
#define __QOS_NETIF_HASH_SIZE       16                                  /*  ����ӿ� hash ���С        */
#define __QOS_NETIF_HASH_MASK       (__QOS_NETIF_HASH_SIZE - 1)         /*  hash ����                   */
/*********************************************************************************************************
  ����������
*********************************************************************************************************/
static LW_OBJECT_HANDLE             _G_ulQosLock;

#define __QOS_LOCK_R()              API_SemaphoreRWPendR(_G_ulQosLock, LW_OPTION_WAIT_INFINITE)
#define __QOS_LOCK_W()              API_SemaphoreRWPendW(_G_ulQosLock, LW_OPTION_WAIT_INFINITE)
#define __QOS_UNLOCK()              API_SemaphoreRWPost(_G_ulQosLock)
/*********************************************************************************************************
  ������ƽṹ
*********************************************************************************************************/
typedef struct {
    LW_LIST_LINE            QOSRI_lineManage;                           /*  IP �����������             */
    INT                     QOSRI_iRule;
    INT                     QOSRI_iCmpMethod;                           /*  srouce, destination, both   */
    ip4_addr_t              QOSRI_ipaddrHboS;                           /*  IP ����ʼ IP ��ַ           */
    ip4_addr_t              QOSRI_ipaddrHboE;                           /*  IP �ν��� IP ��ַ           */
    u8_t                    QOSRI_ucPrio;                               /*  QoS ���ȼ�                  */
    u8_t                    QOSRI_ucDontDrop;                           /*  QoS �ɿ�����                */
} __QOS_RULE_IP;
typedef __QOS_RULE_IP      *__PQOS_RULE_IP;

typedef struct {
    LW_LIST_LINE            QOSRU_lineManage;                           /*  UDP �����������            */
    INT                     QOSRU_iRule;
    INT                     QOSRU_iCmpMethod;                           /*  srouce, destination, both   */
    ip4_addr_t              QOSRU_ipaddrHboS;                           /*  IP ����ʼ IP ��ַ           */
    ip4_addr_t              QOSRU_ipaddrHboE;                           /*  IP �ν��� IP ��ַ           */
    u16_t                   QOSRU_usPortHboS;                           /*  �˿���ʼ ������             */
    u16_t                   QOSRU_usPortHboE;                           /*  �˿ڽ���                    */
    u8_t                    QOSRU_ucPrio;                               /*  QoS ���ȼ�                  */
    u8_t                    QOSRU_ucDontDrop;                           /*  QoS �ɿ�����                */
} __QOS_RULE_UDP;
typedef __QOS_RULE_UDP     *__PQOS_RULE_UDP;

typedef struct {
    LW_LIST_LINE            QOSRT_lineManage;                           /*  TCP �����������            */
    INT                     QOSRT_iRule;
    INT                     QOSRT_iCmpMethod;                           /*  srouce, destination, both   */
    ip4_addr_t              QOSRT_ipaddrHboS;                           /*  IP ����ʼ IP ��ַ           */
    ip4_addr_t              QOSRT_ipaddrHboE;                           /*  IP �ν��� IP ��ַ           */
    u16_t                   QOSRT_usPortHboS;                           /*  �˿���ʼ ������             */
    u16_t                   QOSRT_usPortHboE;                           /*  �˿ڽ���                    */
    u8_t                    QOSRT_ucPrio;                               /*  QoS ���ȼ�                  */
    u8_t                    QOSRT_ucDontDrop;                           /*  QoS �ɿ�����                */
} __QOS_RULE_TCP;
typedef __QOS_RULE_TCP     *__PQOS_RULE_TCP;
/*********************************************************************************************************
  QoS ����ӿڽṹ
*********************************************************************************************************/
typedef struct {
    LW_LIST_LINE            QOSNI_lineHash;                             /*  hash ��                     */
    LW_LIST_LINE_HEADER     QOSNI_qosrnRule[__QOS_NETIF_RULE_MAX];      /*  �����                      */
    CHAR                    QOSNI_cName[IF_NAMESIZE];                   /*  ����ӿ���                  */
    BOOL                    QOSNI_bAttached;                            /*  �Ƿ��Ѿ�����                */
} __QOS_NETIF_CB;
typedef __QOS_NETIF_CB     *__PQOS_NETIF_CB;
/*********************************************************************************************************
  ���� hash ��
*********************************************************************************************************/
static LW_LIST_LINE_HEADER  _G_plineQosHash[__QOS_NETIF_HASH_SIZE];     /*  ͨ�� name ����ɢ��          */
static ULONG                _G_ulQosCounter = 0ul;                      /*  ���������                  */
/*********************************************************************************************************
  ��������
*********************************************************************************************************/
static UINT8  qos_netif_hook(struct netif *pnetif, struct pbuf *p, u8_t ipver, 
                             u8_t prio, u16_t iphdr_offset, u8_t *dont_drop);
#if LW_CFG_SHELL_EN > 0
static INT    __tshellNetQosShow(INT  iArgC, PCHAR  *ppcArgV);
static INT    __tshellNetQosRuleAdd(INT  iArgC, PCHAR  *ppcArgV);
static INT    __tshellNetQosRuleDel(INT  iArgC, PCHAR  *ppcArgV);
#endif                                                                  /*  LW_CFG_SHELL_EN > 0         */
#if LW_CFG_PROCFS_EN > 0
static VOID   __procFsQosInit(VOID);
#endif                                                                  /*  LW_CFG_PROCFS_EN > 0        */
/*********************************************************************************************************
** ��������: __qosNetifGet
** ��������: ��� QoS ����ӿڽṹ
** �䡡��  : pnetif         ����ӿ�
** �䡡��  : �ҵ��� QoS ����ӿڽṹ
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static LW_INLINE __PQOS_NETIF_CB  __qosNetifGet (struct netif *pnetif)
{
    return  ((__PQOS_NETIF_CB)pnetif->inner_qos_stat);
}
/*********************************************************************************************************
** ��������: __qosNetifFind
** ��������: ����ָ���� QoS ����ӿڽṹ
** �䡡��  : pcIfname       ����ӿ�����
** �䡡��  : �ҵ��� QoS ����ӿڽṹ
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static __PQOS_NETIF_CB  __qosNetifFind (CPCHAR  pcIfname)
{
    REGISTER INT                    iIndex;
             PLW_LIST_LINE          plineTemp;
             __PQOS_NETIF_CB        pqosniTemp;

    iIndex = (pcIfname[0] + pcIfname[1] + pcIfname[2]) & __QOS_NETIF_HASH_MASK;

    for (plineTemp  = _G_plineQosHash[iIndex];
         plineTemp != LW_NULL;
         plineTemp  = _list_line_get_next(plineTemp)) {

        pqosniTemp = _LIST_ENTRY(plineTemp, __QOS_NETIF_CB, QOSNI_lineHash);
        if (lib_strncmp(pqosniTemp->QOSNI_cName, pcIfname, IF_NAMESIZE) == 0) {
            return  (pqosniTemp);
        }
    }

    return  (LW_NULL);
}
/*********************************************************************************************************
** ��������: __qosNetifInsertHash
** ��������: ��ָ���� QoS ����ӿڽṹ���� hash ��
** �䡡��  : pqosni         QoS ����ӿڽṹ
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static VOID  __qosNetifInsertHash (__PQOS_NETIF_CB  pqosni)
{
    REGISTER INT                    iIndex;
    REGISTER LW_LIST_LINE_HEADER   *ppline;

    iIndex = (pqosni->QOSNI_cName[0]
           +  pqosni->QOSNI_cName[1]
           +  pqosni->QOSNI_cName[2])
           & __QOS_NETIF_HASH_MASK;

    ppline = &_G_plineQosHash[iIndex];

    _List_Line_Add_Ahead(&pqosni->QOSNI_lineHash, ppline);
}
/*********************************************************************************************************
** ��������: __qosNetifDeleteHash
** ��������: ��ָ���� QoS ����ӿڽṹ�� hash �����Ƴ�
** �䡡��  : pqosni         QoS ����ӿڽṹ
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static VOID  __qosNetifDeleteHash (__PQOS_NETIF_CB  pqosni)
{
    REGISTER INT                    iIndex;
    REGISTER LW_LIST_LINE_HEADER   *ppline;

    iIndex = (pqosni->QOSNI_cName[0]
           +  pqosni->QOSNI_cName[1]
           +  pqosni->QOSNI_cName[2])
           & __QOS_NETIF_HASH_MASK;

    ppline = &_G_plineQosHash[iIndex];

    _List_Line_Del(&pqosni->QOSNI_lineHash, ppline);
}
/*********************************************************************************************************
** ��������: __qosNetifCreate
** ��������: ����һ�� QoS ����ӿڽṹ (���������ֱ�ӷ������е�)
** �䡡��  : pcIfname         ����ӿ���
** �䡡��  : �������� QoS ����ӿڽṹ
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static __PQOS_NETIF_CB  __qosNetifCreate (CPCHAR  pcIfname)
{
    INT                    i;
    __PQOS_NETIF_CB        pqosni;
    struct netif          *pnetif;

    pqosni = __qosNetifFind(pcIfname);
    if (pqosni == LW_NULL) {
        pqosni =  (__PQOS_NETIF_CB)__SHEAP_ALLOC(sizeof(__QOS_NETIF_CB));
        if (pqosni == LW_NULL) {
            _DebugHandle(__ERRORMESSAGE_LEVEL, "system low memory.\r\n");
            _ErrorHandle(ERROR_SYSTEM_LOW_MEMORY);
            return  (LW_NULL);
        }
        lib_strlcpy(pqosni->QOSNI_cName, pcIfname, IF_NAMESIZE);
        for (i = 0; i < __QOS_NETIF_RULE_MAX; i++) {
            pqosni->QOSNI_qosrnRule[i] = LW_NULL;                       /*  û���κι���                */
        }
        __qosNetifInsertHash(pqosni);
        
        pnetif = netif_find(pcIfname);
        if (pnetif) {
            pnetif->inner_qos_stat  = (void *)pqosni;
            pnetif->inner_qos       = qos_netif_hook;
            pqosni->QOSNI_bAttached = LW_TRUE;
        
        } else {
            pqosni->QOSNI_bAttached = LW_FALSE;
        }
    }

    return  (pqosni);
}
/*********************************************************************************************************
** ��������: __qosNetifDelete
** ��������: ɾ��һ�� QoS ����ӿڽṹ
** �䡡��  : pqosni        QoS ����ӿڽṹ
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static VOID  __qosNetifDelete (__PQOS_NETIF_CB  pqosni)
{
    __qosNetifDeleteHash(pqosni);
    __SHEAP_FREE(pqosni);
}
/*********************************************************************************************************
** ��������: __qosIpRuleCheck
** ��������: ��� IP ����
** �䡡��  : pqosni        QoS ����ӿ�
             piphdr        IP ��ͷ
             ucIpPrio      IP ��ͷ���ȼ�
             pucDontDrop   �Ƿ��ܶ���
** �䡡��  : ���ȼ�
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static UINT8  __qosIpRuleCheck (__PQOS_NETIF_CB  pqosni, struct ip_hdr *piphdr, 
                                UINT8  ucIpPrio, UINT8 *pucDontDrop)
{
    PLW_LIST_LINE       plineTemp;
    __PQOS_RULE_IP      pqosri;
    ip4_addr_t          ipaddrSrcHbo;
    ip4_addr_t          ipaddrDestHbo;
    
    ipaddrSrcHbo.addr  = PP_NTOHL(piphdr->src.addr);
    ipaddrDestHbo.addr = PP_NTOHL(piphdr->dest.addr);
    
    for (plineTemp  = pqosni->QOSNI_qosrnRule[LWIP_QOS_RULE_IP];
         plineTemp != LW_NULL;
         plineTemp  = _list_line_get_next(plineTemp)) {                 /*  ���������                  */

        pqosri = _LIST_ENTRY(plineTemp, __QOS_RULE_IP, QOSRI_lineManage);
        if (pqosri->QOSRI_iCmpMethod & LWIP_QOS_CMP_METHOD_SRC) {
            if ((ipaddrSrcHbo.addr >= pqosri->QOSRI_ipaddrHboS.addr) &&
                (ipaddrSrcHbo.addr <= pqosri->QOSRI_ipaddrHboE.addr)) {
                *pucDontDrop = pqosri->QOSRI_ucDontDrop;
                return  (pqosri->QOSRI_ucPrio);
            }
        
        } else if (pqosri->QOSRI_iCmpMethod & LWIP_QOS_CMP_METHOD_DEST) {
            if ((ipaddrDestHbo.addr >= pqosri->QOSRI_ipaddrHboS.addr) &&
                (ipaddrDestHbo.addr <= pqosri->QOSRI_ipaddrHboE.addr)) {
                *pucDontDrop = pqosri->QOSRI_ucDontDrop;
                return  (pqosri->QOSRI_ucPrio);
            }
        }
    }

    return  (ucIpPrio);
}
/*********************************************************************************************************
** ��������: __qosUdpRuleCheck
** ��������: ��� UDP ����
** �䡡��  : pqosni        QoS ����ӿ�
             piphdr        IP ��ͷ
             pudphdr       UDP ��ͷ
             ucIpPrio      IP ��ͷ���ȼ�
             pucDontDrop   �Ƿ��ܶ���
** �䡡��  : ���ȼ�
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static UINT8  __qosUdpRuleCheck (__PQOS_NETIF_CB  pqosni, struct ip_hdr *piphdr,
                                 struct udp_hdr *pudphdr, UINT8  ucIpPrio, UINT8 *pucDontDrop)
{
    PLW_LIST_LINE       plineTemp;
    __PQOS_RULE_UDP     pqosru;
    ip4_addr_t          ipaddrSrcHbo;
    ip4_addr_t          ipaddrDestHbo;
    u16_t               usPortSrcHbo;
    u16_t               usPortDestHbo;
    
    ipaddrSrcHbo.addr  = PP_NTOHL(piphdr->src.addr);
    ipaddrDestHbo.addr = PP_NTOHL(piphdr->dest.addr);
    
    usPortSrcHbo  = PP_NTOHS(pudphdr->src);
    usPortDestHbo = PP_NTOHS(pudphdr->dest);

    for (plineTemp  = pqosni->QOSNI_qosrnRule[LWIP_QOS_RULE_UDP];
         plineTemp != LW_NULL;
         plineTemp  = _list_line_get_next(plineTemp)) {                 /*  ���������                  */

        pqosru = _LIST_ENTRY(plineTemp, __QOS_RULE_UDP, QOSRU_lineManage);
        if (pqosru->QOSRU_iCmpMethod & LWIP_QOS_CMP_METHOD_SRC) {
            if ((ipaddrSrcHbo.addr >= pqosru->QOSRU_ipaddrHboS.addr) &&
                (ipaddrSrcHbo.addr <= pqosru->QOSRU_ipaddrHboE.addr) &&
                (usPortSrcHbo      >= pqosru->QOSRU_usPortHboS)      &&
                (usPortSrcHbo      <= pqosru->QOSRU_usPortHboE)) {
                *pucDontDrop = pqosru->QOSRU_ucDontDrop;
                return  (pqosru->QOSRU_ucPrio);
            }
            
        } else if (pqosru->QOSRU_iCmpMethod & LWIP_QOS_CMP_METHOD_DEST) {
            if ((ipaddrDestHbo.addr >= pqosru->QOSRU_ipaddrHboS.addr) &&
                (ipaddrDestHbo.addr <= pqosru->QOSRU_ipaddrHboE.addr) &&
                (usPortDestHbo      >= pqosru->QOSRU_usPortHboS)      &&
                (usPortDestHbo      <= pqosru->QOSRU_usPortHboE)) {
                *pucDontDrop = pqosru->QOSRU_ucDontDrop;
                return  (pqosru->QOSRU_ucPrio);
            }
        }
    }

    return  (ucIpPrio);
}
/*********************************************************************************************************
** ��������: __qosTcpRuleCheck
** ��������: ��� TCP ����
** �䡡��  : pqosni        QoS ����ӿ�
             piphdr        IP ��ͷ
             ptcphdr       TCP ��ͷ
             ucIpPrio      IP ��ͷ���ȼ�
             pucDontDrop   �Ƿ��ܶ���
** �䡡��  : ���ȼ�
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static UINT8  __qosTcpRuleCheck (__PQOS_NETIF_CB  pqosni, struct ip_hdr *piphdr, 
                                 struct tcp_hdr *ptcphdr, UINT8  ucIpPrio, UINT8 *pucDontDrop)
{
    PLW_LIST_LINE       plineTemp;
    __PQOS_RULE_TCP     pqosrt;
    ip4_addr_t          ipaddrSrcHbo;
    ip4_addr_t          ipaddrDestHbo;
    u16_t               usPortSrcHbo;
    u16_t               usPortDestHbo;
    
    ipaddrSrcHbo.addr  = PP_NTOHL(piphdr->src.addr);
    ipaddrDestHbo.addr = PP_NTOHL(piphdr->dest.addr);
    
    usPortSrcHbo  = PP_NTOHS(ptcphdr->src);
    usPortDestHbo = PP_NTOHS(ptcphdr->dest);

    for (plineTemp  = pqosni->QOSNI_qosrnRule[LWIP_QOS_RULE_TCP];
         plineTemp != LW_NULL;
         plineTemp  = _list_line_get_next(plineTemp)) {                 /*  ���������                  */

        pqosrt = _LIST_ENTRY(plineTemp, __QOS_RULE_TCP, QOSRT_lineManage);
        if (pqosrt->QOSRT_iCmpMethod & LWIP_QOS_CMP_METHOD_SRC) {
            if ((ipaddrSrcHbo.addr >= pqosrt->QOSRT_ipaddrHboS.addr) &&
                (ipaddrSrcHbo.addr <= pqosrt->QOSRT_ipaddrHboE.addr) &&
                (usPortSrcHbo      >= pqosrt->QOSRT_usPortHboS)      &&
                (usPortSrcHbo      <= pqosrt->QOSRT_usPortHboE)) {
                *pucDontDrop = pqosrt->QOSRT_ucDontDrop;
                return  (pqosrt->QOSRT_ucPrio);
            }
        
        } else if (pqosrt->QOSRT_iCmpMethod & LWIP_QOS_CMP_METHOD_DEST) {
            if ((ipaddrDestHbo.addr >= pqosrt->QOSRT_ipaddrHboS.addr) &&
                (ipaddrDestHbo.addr <= pqosrt->QOSRT_ipaddrHboE.addr) &&
                (usPortDestHbo      >= pqosrt->QOSRT_usPortHboS)      &&
                (usPortDestHbo      <= pqosrt->QOSRT_usPortHboE)) {
                *pucDontDrop = pqosrt->QOSRT_ucDontDrop;
                return  (pqosrt->QOSRT_ucPrio);
            }
        }
    }

    return  (ucIpPrio);
}
/*********************************************************************************************************
** ��������: qos_netif_hook
** ��������: ���������, �������������ݱ���������Э��ջ
** �䡡��  : pnetif        ����ӿ�
**           p             ���ݰ�
**           ipver         IP �汾
**           prio          IP ͷ���ȼ�
**           iphdr_offset  IP ͷƫ����
**           dont_drop     �Ƿ��ܶ���
** �䡡��  : ���ȼ�
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static UINT8  qos_netif_hook (struct netif *pnetif, struct pbuf *p, u8_t ipver, 
                              u8_t prio, u16_t iphdr_offset, u8_t *dont_drop)
{
    __PQOS_NETIF_CB     pqosni;                                         /*  QoS ����ӿ�                */
    INT                 iOffset = 0;                                    /*  ��ʼƫ����                  */

    struct ip_hdr      *piphdr;
    struct udp_hdr     *pudphdr;
    struct tcp_hdr     *ptcphdr;

    if (ipver != 4) {                                                   /*  �ݲ����� IPv6               */
        return  (prio);
    }

    __QOS_LOCK_R();                                                     /*  ���� QoS ��                 */
    pqosni = __qosNetifGet(pnetif);                                     /*  QoS ����ӿ�                */
    if (pqosni == LW_NULL) {                                            /*  û���ҵ���Ӧ�Ŀ��ƽṹ      */
        __QOS_UNLOCK();                                                 /*  ���� QoS ��                 */
        return  (prio);
    }
    
    piphdr  = (struct ip_hdr *)((char *)p->payload + iphdr_offset);
    prio    = __qosIpRuleCheck(pqosni, piphdr, prio, dont_drop);
    iOffset = iphdr_offset + (IPH_HL(piphdr) << 2);
    
    if ((IPH_OFFSET(piphdr) & PP_HTONS(IP_OFFMASK)) != 0) {             /*  ��Ƭ���ݰ�                  */
        __QOS_UNLOCK();                                                 /*  ���� QoS ��                 */
        return  (prio);
    }
    
    switch (IPH_PROTO(piphdr)) {
    
    case IP_PROTO_UDP:
    case IP_PROTO_UDPLITE:
        if (p->len < (iOffset + sizeof(struct udp_hdr))) {
            return  (prio);
        }
        pudphdr = (struct udp_hdr *)((char *)p->payload + iOffset);
        prio    = __qosUdpRuleCheck(pqosni, piphdr, pudphdr, prio, dont_drop);
        break;
        
    case IP_PROTO_TCP:
        if (p->len < (iOffset + sizeof(struct tcp_hdr))) {
            return  (prio);
        }
        ptcphdr = (struct tcp_hdr *)((char *)p->payload + iOffset);
        prio    = __qosTcpRuleCheck(pqosni, piphdr, ptcphdr, prio, dont_drop);
        break;
        
    default:
        break;
    }
    __QOS_UNLOCK();                                                     /*  ���� QoS ��                 */
    
    return  (prio);
}
/*********************************************************************************************************
** ��������: qos_netif_attach
** ��������: ������ӿ����ʱ, ���õ� hook ����
** �䡡��  : pnetif       ����ӿ�
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
VOID  qos_netif_attach (struct netif  *pnetif)
{
    CHAR                cIfname[IF_NAMESIZE];
    __PQOS_NETIF_CB     pqosni;
    
    if (!_G_ulQosLock) {
        return;
    }
    
    netif_get_name(pnetif, cIfname);
    
    __QOS_LOCK_R();                                                     /*  ���� QoS ��                 */
    pqosni = __qosNetifFind(cIfname);
    if (pqosni == LW_NULL) {
        __QOS_UNLOCK();                                                 /*  ���� QoS ��                 */
        return;
    }
    
    pnetif->inner_qos_stat  = (void *)pqosni;
    pnetif->inner_qos       = qos_netif_hook;
    pqosni->QOSNI_bAttached = LW_TRUE;
    __QOS_UNLOCK();                                                     /*  ���� QoS ��                 */
}
/*********************************************************************************************************
** ��������: qos_netif_detach
** ��������: ������ӿ�ɾ��ʱ, ���õ� hook ����
** �䡡��  : pnetif       ����ӿ�
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
VOID  qos_netif_detach (struct netif  *pnetif)
{
    INT                 i;
    __PQOS_NETIF_CB     pqosni;

    __QOS_LOCK_W();                                                     /*  ���� QoS ��                 */
    pqosni = __qosNetifGet(pnetif);
    if (pqosni == LW_NULL) {
        __QOS_UNLOCK();                                                 /*  ���� QoS ��                 */
        return;
    }
    
    pqosni->QOSNI_bAttached = LW_FALSE;
    pnetif->inner_qos_stat  = LW_NULL;
    pnetif->inner_qos       = LW_NULL;                                  /*  ����ʹ�� QoS ���뺯��       */

    for (i = 0; i < __QOS_NETIF_RULE_MAX; i++) {
        if (pqosni->QOSNI_qosrnRule[i]) {
            break;
        }
    }
    if (i >= __QOS_NETIF_RULE_MAX) {                                    /*  ������ӿڿ��ƽ��û�й���  */
        __qosNetifDelete(pqosni);
    }
    __QOS_UNLOCK();                                                     /*  ���� QoS ��                 */
}
/*********************************************************************************************************
** ��������: API_INetQosRuleAdd
** ��������: QoS ���һ������
** �䡡��  : pcIfname          ��Ӧ������ӿ���
**           iRule             ��Ӧ�Ĺ���, IP/UDP/TCP/...
**           iCmpMethod        �ȽϷ���
**           pcAddrStart       IP ��ַ��ʼ, Ϊ IP ��ַ�ַ���, ��ʽΪ: ???.???.???.???
**           pcAddrEnd         IP ��ַ����, Ϊ IP ��ַ�ַ���, ��ʽΪ: ???.???.???.???
**           usPortStart       ��ʼ�˿ں� (�����ֽ���), �������� UDP/TCP ����
**           usPortEnd         �����˿ں� (�����ֽ���), �������� UDP/TCP ����
** �䡡��  : ������
** ȫ�ֱ���:
** ����ģ��:
                                           API ����
*********************************************************************************************************/
LW_API
PVOID  API_INetQosRuleAdd (CPCHAR  pcIfname,
                           INT     iRule,
                           INT     iCmpMethod,
                           CPCHAR  pcAddrStart,
                           CPCHAR  pcAddrEnd,
                           UINT16  usPortStart,
                           UINT16  usPortEnd,
                           UINT8   ucPrio,
                           BOOL    bDontDrop)
{
    __PQOS_NETIF_CB        pqosni;

    if (!pcIfname) {
        _ErrorHandle(EINVAL);
        return  (LW_NULL);
    }
    
    if (ucPrio > LWIP_QOS_PRIO_HIGHEST) {
        _ErrorHandle(EINVAL);
        return  (LW_NULL);
    }

    if (iRule == LWIP_QOS_RULE_IP) {                                    /*  ��� IP ����                */
        __PQOS_RULE_IP      pqosri;

        if (!pcAddrStart || !pcAddrEnd) {
            _ErrorHandle(EINVAL);
            return  (LW_NULL);
        }

        pqosri = (__PQOS_RULE_IP)__SHEAP_ALLOC(sizeof(__QOS_RULE_IP));  /*  ��������                    */
        if (pqosri == LW_NULL) {
            _DebugHandle(__ERRORMESSAGE_LEVEL, "system low memory.\r\n");
            _ErrorHandle(ERROR_SYSTEM_LOW_MEMORY);
            return  (LW_NULL);
        }
        pqosri->QOSRI_iRule      = iRule;
        pqosri->QOSRI_iCmpMethod = iCmpMethod;
        
        pqosri->QOSRI_ipaddrHboS.addr = PP_NTOHL(ipaddr_addr(pcAddrStart));
        pqosri->QOSRI_ipaddrHboE.addr = PP_NTOHL(ipaddr_addr(pcAddrEnd));
        pqosri->QOSRI_ucPrio          = ucPrio;
        pqosri->QOSRI_ucDontDrop      = bDontDrop ? 1 : 0;

        __QOS_LOCK_W();                                                 /*  ���� QoS ��                 */
        pqosni = __qosNetifCreate(pcIfname);                            /*  �������ƿ�                  */
        if (pqosni == LW_NULL) {
            __QOS_UNLOCK();                                             /*  ���� QoS ��                 */
            return  (LW_NULL);
        }
        _List_Line_Add_Ahead(&pqosri->QOSRI_lineManage,
                             &pqosni->QOSNI_qosrnRule[iRule]);
        _G_ulQosCounter++;
        __QOS_UNLOCK();                                                 /*  ���� QoS ��                 */

        return  ((PVOID)pqosri);

    } else if (iRule == LWIP_QOS_RULE_UDP) {                            /*  ��� UDP ����               */
        __PQOS_RULE_UDP   pqosru;

        if (!pcAddrStart || !pcAddrEnd) {
            _ErrorHandle(EINVAL);
            return  (LW_NULL);
        }

        pqosru = (__PQOS_RULE_UDP)__SHEAP_ALLOC(sizeof(__QOS_RULE_UDP));/*  ��������                    */
        if (pqosru == LW_NULL) {
            _DebugHandle(__ERRORMESSAGE_LEVEL, "system low memory.\r\n");
            _ErrorHandle(ERROR_SYSTEM_LOW_MEMORY);
            return  (LW_NULL);
        }
        pqosru->QOSRU_iRule      = iRule;
        pqosru->QOSRU_iCmpMethod = iCmpMethod;
        
        pqosru->QOSRU_ipaddrHboS.addr = PP_NTOHL(ipaddr_addr(pcAddrStart));
        pqosru->QOSRU_ipaddrHboE.addr = PP_NTOHL(ipaddr_addr(pcAddrEnd));
        pqosru->QOSRU_usPortHboS      = PP_NTOHS(usPortStart);
        pqosru->QOSRU_usPortHboE      = PP_NTOHS(usPortEnd);
        pqosru->QOSRU_ucPrio          = ucPrio;
        pqosru->QOSRU_ucDontDrop      = bDontDrop ? 1 : 0;

        __QOS_LOCK_W();                                                 /*  ���� QoS ��                 */
        pqosni = __qosNetifCreate(pcIfname);                            /*  �������ƿ�                  */
        if (pqosni == LW_NULL) {
            __QOS_UNLOCK();                                             /*  ���� QoS ��                 */
            return  (LW_NULL);
        }
        _List_Line_Add_Ahead(&pqosru->QOSRU_lineManage,
                             &pqosni->QOSNI_qosrnRule[iRule]);
        _G_ulQosCounter++;
        __QOS_UNLOCK();                                                 /*  ���� QoS ��                 */

        return  ((PVOID)pqosru);

    } else if (iRule == LWIP_QOS_RULE_TCP) {                            /*  ��� TCP ����               */
        __PQOS_RULE_TCP   pqosrt;

        if (!pcAddrStart || !pcAddrEnd) {
            _ErrorHandle(EINVAL);
            return  (LW_NULL);
        }

        pqosrt = (__PQOS_RULE_TCP)__SHEAP_ALLOC(sizeof(__QOS_RULE_TCP));/*  ��������                    */
        if (pqosrt == LW_NULL) {
            _DebugHandle(__ERRORMESSAGE_LEVEL, "system low memory.\r\n");
            _ErrorHandle(ERROR_SYSTEM_LOW_MEMORY);
            return  (LW_NULL);
        }
        pqosrt->QOSRT_iRule      = iRule;
        pqosrt->QOSRT_iCmpMethod = iCmpMethod;
        
        pqosrt->QOSRT_ipaddrHboS.addr = PP_NTOHL(ipaddr_addr(pcAddrStart));
        pqosrt->QOSRT_ipaddrHboE.addr = PP_NTOHL(ipaddr_addr(pcAddrEnd));
        pqosrt->QOSRT_usPortHboS      = PP_NTOHS(usPortStart);
        pqosrt->QOSRT_usPortHboE      = PP_NTOHS(usPortEnd);
        pqosrt->QOSRT_ucPrio          = ucPrio;
        pqosrt->QOSRT_ucDontDrop      = bDontDrop ? 1 : 0;

        __QOS_LOCK_W();                                                 /*  ���� QoS ��                 */
        pqosni = __qosNetifCreate(pcIfname);                            /*  �������ƿ�                  */
        if (pqosni == LW_NULL) {
            __QOS_UNLOCK();                                             /*  ���� QoS ��                 */
            return  (LW_NULL);
        }
        _List_Line_Add_Ahead(&pqosrt->QOSRT_lineManage,
                             &pqosni->QOSNI_qosrnRule[iRule]);
        _G_ulQosCounter++;
        __QOS_UNLOCK();                                                 /*  ���� QoS ��                 */

        return  ((PVOID)pqosrt);

    } else {
        _ErrorHandle(EINVAL);
        return  (LW_NULL);
    }
}
/*********************************************************************************************************
** ��������: API_INetQosRuleDel
** ��������: QoS ɾ��һ������
** �䡡��  : pcIfname          ��Ӧ������ӿ���
**           pvRule            ������ (����Ϊ NULL, Ϊ NULL ʱ��ʾʹ�ù������к�)
**           iSeqNum           ָ������ӿڵĹ������к� (�� 0 ��ʼ)
** �䡡��  : ERROR_NONE or PX_ERROR
** ȫ�ֱ���:
** ����ģ��:
                                           API ����
*********************************************************************************************************/
LW_API
INT  API_INetQosRuleDel (CPCHAR  pcIfname,
                         PVOID   pvRule,
                         INT     iSeqNum)
{
    INT                    i;
    __PQOS_NETIF_CB        pqosni;
    __PQOS_RULE_IP         pqosri;

    if (!pcIfname) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }

    if (!pvRule && (iSeqNum < 0)) {                                     /*  ��������������ͬʱ��Ч      */
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }

    __QOS_LOCK_W();                                                     /*  ���� QOS ��                 */
    pqosni = __qosNetifFind(pcIfname);
    if (pqosni == LW_NULL) {
        __QOS_UNLOCK();                                                 /*  ���� QOS ��                 */
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }

    if (pvRule) {                                                       /*  ͨ�����ɾ��                */
        pqosri = _LIST_ENTRY(pvRule, __QOS_RULE_IP, QOSRI_lineManage);
        _List_Line_Del(&pqosri->QOSRI_lineManage,
                       &pqosni->QOSNI_qosrnRule[pqosri->QOSRI_iRule]);
        _G_ulQosCounter--;
        
    } else {                                                            /*  ͨ�����ɾ��                */
        PLW_LIST_LINE   plineTemp;

        for (i = 0; i < __QOS_NETIF_RULE_MAX; i++) {
            for (plineTemp  = pqosni->QOSNI_qosrnRule[i];
                 plineTemp != LW_NULL;
                 plineTemp  = _list_line_get_next(plineTemp)) {
                if (iSeqNum == 0) {
                    goto    __rule_find;
                }
                iSeqNum--;
            }
        }

__rule_find:
        if (iSeqNum || (plineTemp == LW_NULL)) {
            __QOS_UNLOCK();                                             /*  ���� QoS ��                 */
            _ErrorHandle(EINVAL);                                       /*  iSeqNum ��������            */
            return  (PX_ERROR);
        }
        _List_Line_Del(plineTemp,
                       &pqosni->QOSNI_qosrnRule[i]);
        _G_ulQosCounter--;
        pvRule = plineTemp;
    }

    /*
     *  ��� pqosni ��û���κι���, ͬʱҲû���κ���������. pqosni ���Ա�ɾ��.
     */
    if (!pqosni->QOSNI_bAttached) {                                     /*  û�����ӵ�����              */
        INT     i;

        for (i = 0; i < __QOS_NETIF_RULE_MAX; i++) {
            if (pqosni->QOSNI_qosrnRule[i]) {
                break;
            }
        }

        if (i >= __QOS_NETIF_RULE_MAX) {                                /*  ������ӿڿ��ƽ��û�й���  */
            __qosNetifDelete(pqosni);
        }
    }
    __QOS_UNLOCK();                                                     /*  ���� QOS ��                 */

    __SHEAP_FREE(pvRule);                                               /*  �ͷ��ڴ�                    */

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: API_INetQosInit
** ��������: QoS ��ʼ��
** �䡡��  : NONE
** �䡡��  : ERROR_NONE or PX_ERROR
** ȫ�ֱ���:
** ����ģ��:
                                           API ����
*********************************************************************************************************/
LW_API
INT  API_INetQosInit (VOID)
{
    static BOOL bIsInit = LW_FALSE;

    if (bIsInit) {
        return  (ERROR_NONE);
    }

    _G_ulQosLock = API_SemaphoreRWCreate("sem_qoslcok", LW_OPTION_DELETE_SAFE |
                                         LW_OPTION_WAIT_FIFO | LW_OPTION_OBJECT_GLOBAL, LW_NULL);
    if (_G_ulQosLock == LW_OBJECT_HANDLE_INVALID) {
        return  (PX_ERROR);
    }

#if LW_CFG_SHELL_EN > 0
    /*
     *  ���� SHELL ����.
     */
    API_TShellKeywordAdd("qoss", __tshellNetQosShow);
    API_TShellHelpAdd("qoss",    "show QoS rule(s).\n");

    API_TShellKeywordAdd("qosruleadd", __tshellNetQosRuleAdd);
    API_TShellFormatAdd("qosruleadd",  " [netifname] [rule] [args...] [s|d|b] [prio] [dont_drop]");
    API_TShellHelpAdd("qosruleadd",    "add a rule into QoS.\n"
                                       "    qosruleadd en1 ip 192.168.0.5 192.168.0.10 s 5 no\n"
                                       "    qosruleadd lo0 udp 0.0.0.0 255.255.255.255 433 500 b 6 yes\n"
                                       "    qosruleadd wl2 tcp 192.168.0.1 192.168.0.200 169 169 d 2 no\n");

    API_TShellKeywordAdd("qosruledel", __tshellNetQosRuleDel);
    API_TShellFormatAdd("qosruledel",  " [netifname] [rule sequence num]");
    API_TShellHelpAdd("qosruledel",    "del a rule from QoS.\n");
#endif                                                                  /*  LW_CFG_SHELL_EN > 0         */

#if LW_CFG_PROCFS_EN > 0
    __procFsQosInit();
#endif                                                                  /*  LW_CFG_PROCFS_EN > 0        */

    bIsInit = LW_TRUE;

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: API_INetQosShow
** ��������: QoS ��ʾ��ǰ���еĹ�����Ŀ
** �䡡��  : iFd           ��ӡĿ���ļ�������
** �䡡��  : ERROR_NONE or PX_ERROR
** ȫ�ֱ���:
** ����ģ��:
                                           API ����
*********************************************************************************************************/
LW_API
INT  API_INetQosShow (INT  iFd)
{
    INT     iQosFd;
    CHAR    cBuffer[512];
    ssize_t sstNum;
    
    iQosFd = open("/proc/net/qos", O_RDONLY);
    if (iQosFd < 0) {
        fprintf(stderr, "can not open /proc/net/qos : %s\n", lib_strerror(errno));
        return  (PX_ERROR);
    }
    
    do {
        sstNum = read(iQosFd, cBuffer, sizeof(cBuffer));
        if (sstNum > 0) {
            write(iFd, cBuffer, (size_t)sstNum);
        }
    } while (sstNum > 0);
    
    close(iQosFd);

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: __tshellNetQosShow
** ��������: ϵͳ���� "qoss"
** �䡡��  : iArgC         ��������
**           ppcArgV       ������
** �䡡��  : 0
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
#if LW_CFG_SHELL_EN > 0

static INT  __tshellNetQosShow (INT  iArgC, PCHAR  *ppcArgV)
{
    return  (API_INetQosShow(STD_OUT));
}
/*********************************************************************************************************
** ��������: __tshellNetQosRuleAdd
** ��������: ϵͳ���� "qosruleadd"
** �䡡��  : iArgC         ��������
**           ppcArgV       ������
** �䡡��  : 0
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  __tshellNetQosRuleAdd (INT  iArgC, PCHAR  *ppcArgV)
{
#define __QOS_TSHELL_RADD_ARG_NETIF     1
#define __QOS_TSHELL_RADD_ARG_RULE      2
#define __QOS_TSHELL_RADD_ARG_IPS       3
#define __QOS_TSHELL_RADD_ARG_IPE       4
#define __QOS_TSHELL_RADD_ARG_PORTS     5
#define __QOS_TSHELL_RADD_ARG_PORTE     6
    PVOID    pvRule;
    INT      iPrio;
    INT      iCmpMethod;
    BOOL     bDontDrop;

    if (iArgC < 8) {
        fprintf(stderr, "arguments error!\n");
        return  (-ERROR_TSHELL_EPARAM);
    }

    if (lib_strcmp(ppcArgV[__QOS_TSHELL_RADD_ARG_RULE], "ip") == 0) {
        if (iArgC != 8) {
            fprintf(stderr, "arguments error!\n");
            return  (-ERROR_TSHELL_EPARAM);
        }
        switch (*ppcArgV[__QOS_TSHELL_RADD_ARG_IPE + 1]) {
        
        case 's':
        case 'S':
            iCmpMethod = LWIP_QOS_CMP_METHOD_SRC;
            break;
        
        case 'd':
        case 'D':
            iCmpMethod = LWIP_QOS_CMP_METHOD_DEST;
            break;
            
        case 'b':
        case 'B':
            iCmpMethod = LWIP_QOS_CMP_METHOD_BOTH;
            break;
            
        default:
            fprintf(stderr, "compare method arguments error!\n");
            return  (-ERROR_TSHELL_EPARAM);
        }
        if (sscanf(ppcArgV[__QOS_TSHELL_RADD_ARG_IPE + 2], "%d", &iPrio) != 1) {
            fprintf(stderr, "priority error!\n");
            return  (-ERROR_TSHELL_EPARAM);
        }
        bDontDrop = (*ppcArgV[__QOS_TSHELL_RADD_ARG_IPE + 3] == 'y') ? LW_TRUE : LW_FALSE;
        
        pvRule = API_INetQosRuleAdd(ppcArgV[__QOS_TSHELL_RADD_ARG_NETIF],
                                    LWIP_QOS_RULE_IP, iCmpMethod,
                                    ppcArgV[__QOS_TSHELL_RADD_ARG_IPS],
                                    ppcArgV[__QOS_TSHELL_RADD_ARG_IPE], 0, 0,
                                    (UINT8)iPrio, bDontDrop);
        if (pvRule == LW_NULL) {
            fprintf(stderr, "can not add ip rule, error: %s\n", lib_strerror(errno));
            return  (PX_ERROR);
        }

    } else if (lib_strcmp(ppcArgV[__QOS_TSHELL_RADD_ARG_RULE], "udp") == 0) {
        INT     iPortS = -1;
        INT     iPortE = -1;

        if (iArgC != 10) {
            fprintf(stderr, "arguments error!\n");
            return  (-ERROR_TSHELL_EPARAM);
        }
        if (sscanf(ppcArgV[__QOS_TSHELL_RADD_ARG_PORTS], "%i", &iPortS) != 1) {
            fprintf(stderr, "port argument error!\n");
            return  (-ERROR_TSHELL_EPARAM);
        }
        if (sscanf(ppcArgV[__QOS_TSHELL_RADD_ARG_PORTE], "%i", &iPortE) != 1) {
            fprintf(stderr, "port argument error!\n");
            return  (-ERROR_TSHELL_EPARAM);
        }
        switch (*ppcArgV[__QOS_TSHELL_RADD_ARG_PORTE + 1]) {
        
        case 's':
        case 'S':
            iCmpMethod = LWIP_QOS_CMP_METHOD_SRC;
            break;
        
        case 'd':
        case 'D':
            iCmpMethod = LWIP_QOS_CMP_METHOD_DEST;
            break;
            
        case 'b':
        case 'B':
            iCmpMethod = LWIP_QOS_CMP_METHOD_BOTH;
            break;
            
        default:
            fprintf(stderr, "compare method arguments error!\n");
            return  (-ERROR_TSHELL_EPARAM);
        }
        if (sscanf(ppcArgV[__QOS_TSHELL_RADD_ARG_PORTE + 2], "%d", &iPrio) != 1) {
            fprintf(stderr, "priority error!\n");
            return  (-ERROR_TSHELL_EPARAM);
        }
        bDontDrop = (*ppcArgV[__QOS_TSHELL_RADD_ARG_PORTE + 3] == 'y') ? LW_TRUE : LW_FALSE;
        
        pvRule = API_INetQosRuleAdd(ppcArgV[__QOS_TSHELL_RADD_ARG_NETIF],
                                    LWIP_QOS_RULE_UDP, iCmpMethod,
                                    ppcArgV[__QOS_TSHELL_RADD_ARG_IPS],
                                    ppcArgV[__QOS_TSHELL_RADD_ARG_IPE],
                                    htons((u16_t)iPortS),
                                    htons((u16_t)iPortE),
                                    (UINT8)iPrio, bDontDrop);
        if (pvRule == LW_NULL) {
            fprintf(stderr, "can not add udp rule, error: %s\n", lib_strerror(errno));
            return  (PX_ERROR);
        }

    } else if (lib_strcmp(ppcArgV[__QOS_TSHELL_RADD_ARG_RULE], "tcp") == 0) {
        INT     iPortS = -1;
        INT     iPortE = -1;

        if (iArgC != 10) {
            fprintf(stderr, "arguments error!\n");
            return  (-ERROR_TSHELL_EPARAM);
        }
        if (sscanf(ppcArgV[__QOS_TSHELL_RADD_ARG_PORTS], "%i", &iPortS) != 1) {
            fprintf(stderr, "port argument error!\n");
            return  (-ERROR_TSHELL_EPARAM);
        }
        if (sscanf(ppcArgV[__QOS_TSHELL_RADD_ARG_PORTE], "%i", &iPortE) != 1) {
            fprintf(stderr, "port argument error!\n");
            return  (-ERROR_TSHELL_EPARAM);
        }
        switch (*ppcArgV[__QOS_TSHELL_RADD_ARG_PORTE + 1]) {
        
        case 's':
        case 'S':
            iCmpMethod = LWIP_QOS_CMP_METHOD_SRC;
            break;
        
        case 'd':
        case 'D':
            iCmpMethod = LWIP_QOS_CMP_METHOD_DEST;
            break;
            
        case 'b':
        case 'B':
            iCmpMethod = LWIP_QOS_CMP_METHOD_BOTH;
            break;
            
        default:
            fprintf(stderr, "compare method arguments error!\n");
            return  (-ERROR_TSHELL_EPARAM);
        }
        if (sscanf(ppcArgV[__QOS_TSHELL_RADD_ARG_PORTE + 2], "%d", &iPrio) != 1) {
            fprintf(stderr, "priority error!\n");
            return  (-ERROR_TSHELL_EPARAM);
        }
        bDontDrop = (*ppcArgV[__QOS_TSHELL_RADD_ARG_PORTE + 3] == 'y') ? LW_TRUE : LW_FALSE;
        
        pvRule = API_INetQosRuleAdd(ppcArgV[__QOS_TSHELL_RADD_ARG_NETIF],
                                    LWIP_QOS_RULE_TCP, iCmpMethod,
                                    ppcArgV[__QOS_TSHELL_RADD_ARG_IPS],
                                    ppcArgV[__QOS_TSHELL_RADD_ARG_IPE],
                                    htons((u16_t)iPortS),
                                    htons((u16_t)iPortE),
                                    (UINT8)iPrio, bDontDrop);
        if (pvRule == LW_NULL) {
            fprintf(stderr, "can not add tcp rule, error: %s\n", lib_strerror(errno));
            return  (PX_ERROR);
        }

    } else {
        fprintf(stderr, "rule type argument error!\n");
        return  (-ERROR_TSHELL_EPARAM);
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: __tshellNetQosRuleDel
** ��������: ϵͳ���� "qosruledel"
** �䡡��  : iArgC         ��������
**           ppcArgV       ������
** �䡡��  : 0
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  __tshellNetQosRuleDel (INT  iArgC, PCHAR  *ppcArgV)
{
    INT     iError;
    INT     iSeqNum = -1;

    if (iArgC != 3) {
        fprintf(stderr, "arguments error!\n");
        return  (-ERROR_TSHELL_EPARAM);
    }

    if (sscanf(ppcArgV[2], "%i", &iSeqNum) != 1) {
        fprintf(stderr, "arguments error!\n");
        return  (-ERROR_TSHELL_EPARAM);
    }

    iError = API_INetQosRuleDel(ppcArgV[1], LW_NULL, iSeqNum);
    if (iError) {
        if (errno == EINVAL) {
            fprintf(stderr, "arguments error!\n");
        } else {
            fprintf(stderr, "can not delete rule, error: %s\n", lib_strerror(errno));
        }
    }

    return  (iError);
}

#endif                                                                  /*  LW_CFG_SHELL_EN > 0         */
/*********************************************************************************************************
  /proc/net/qos
*********************************************************************************************************/
#if LW_CFG_PROCFS_EN > 0
/*********************************************************************************************************
  ���� proc �ļ���������
*********************************************************************************************************/
static ssize_t  __procFsQosRead(PLW_PROCFS_NODE  p_pfsn, 
                                PCHAR            pcBuffer, 
                                size_t           stMaxBytes,
                                off_t            oft);
/*********************************************************************************************************
  ���� proc �ļ�����������
*********************************************************************************************************/
static LW_PROCFS_NODE_OP    _G_pfsnoQosFuncs = {
    __procFsQosRead, LW_NULL
};
/*********************************************************************************************************
  ���� proc �ļ��ڵ�
*********************************************************************************************************/
static LW_PROCFS_NODE       _G_pfsnQos[] = 
{
    LW_PROCFS_INIT_NODE("qos", 
                        (S_IFREG | S_IRUSR | S_IRGRP | S_IROTH), 
                        &_G_pfsnoQosFuncs, 
                        "q",
                        0),
};
/*********************************************************************************************************
** ��������: __procFsQosPrint
** ��������: ��ӡ���� qos �ļ�
** �䡡��  : pcBuffer      ����
**           stTotalSize   ��������С
**           pstOft        ��ǰƫ����
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static VOID  __procFsQosPrint (PCHAR  pcBuffer, size_t  stTotalSize, size_t *pstOft)
{
    INT              i, iSeqNum;
    CHAR             cIpBuffer1[INET_ADDRSTRLEN];
    CHAR             cIpBuffer2[INET_ADDRSTRLEN];
    PCHAR            pcCmpMethod;

    __PQOS_NETIF_CB  pqosni;
    PLW_LIST_LINE    plineTempQosni;
    PLW_LIST_LINE    plineTemp;
    
    for (i = 0; i < __QOS_NETIF_HASH_SIZE; i++) {
        for (plineTempQosni  = _G_plineQosHash[i];
             plineTempQosni != LW_NULL;
             plineTempQosni  = _list_line_get_next(plineTempQosni)) {

            pqosni  = _LIST_ENTRY(plineTempQosni, __QOS_NETIF_CB, QOSNI_lineHash);
            iSeqNum = 0;

            /*
             *  ���� IP �����
             */
            for (plineTemp  = pqosni->QOSNI_qosrnRule[LWIP_QOS_RULE_IP];
                 plineTemp != LW_NULL;
                 plineTemp  = _list_line_get_next(plineTemp)) {
                __PQOS_RULE_IP  pqosri;
                ip4_addr_t      ipaddrS, ipaddrE;

                pqosri = _LIST_ENTRY(plineTemp, __QOS_RULE_IP, QOSRI_lineManage);
                
                ipaddrS.addr = PP_HTONL(pqosri->QOSRI_ipaddrHboS.addr);
                ipaddrE.addr = PP_HTONL(pqosri->QOSRI_ipaddrHboE.addr);
                
                if ((pqosri->QOSRI_iCmpMethod & LWIP_QOS_CMP_METHOD_BOTH) == LWIP_QOS_CMP_METHOD_BOTH) {
                    pcCmpMethod = "BOTH";
                } else if (pqosri->QOSRI_iCmpMethod & LWIP_QOS_CMP_METHOD_SRC) {
                    pcCmpMethod = "SRC";
                } else if (pqosri->QOSRI_iCmpMethod & LWIP_QOS_CMP_METHOD_DEST) {
                    pcCmpMethod = "DEST";
                } else {
                    pcCmpMethod = "<unknown>";
                }
                
                *pstOft = bnprintf(pcBuffer, stTotalSize, *pstOft, 
                         "%-5s %-6s %6d %-4s %-10s %4d %9s %-15s %-15s %-6s %-6s\n",
                         pqosni->QOSNI_cName, (pqosni->QOSNI_bAttached) ? "YES" : "NO",
                         iSeqNum, "IP", pcCmpMethod, pqosri->QOSRI_ucPrio,
                         pqosri->QOSRI_ucDontDrop ? "YES" : "NO",
                         ip4addr_ntoa_r(&ipaddrS, cIpBuffer1, INET_ADDRSTRLEN),
                         ip4addr_ntoa_r(&ipaddrE, cIpBuffer2, INET_ADDRSTRLEN),
                         "N/A", "N/A");
                iSeqNum++;
            }

            /*
             *  ���� UDP �����
             */
            for (plineTemp  = pqosni->QOSNI_qosrnRule[LWIP_QOS_RULE_UDP];
                 plineTemp != LW_NULL;
                 plineTemp  = _list_line_get_next(plineTemp)) {
                __PQOS_RULE_UDP  pqosru;
                ip4_addr_t       ipaddrS, ipaddrE;

                pqosru = _LIST_ENTRY(plineTemp, __QOS_RULE_UDP, QOSRU_lineManage);
                
                ipaddrS.addr = PP_HTONL(pqosru->QOSRU_ipaddrHboS.addr);
                ipaddrE.addr = PP_HTONL(pqosru->QOSRU_ipaddrHboE.addr);

                if ((pqosru->QOSRU_iCmpMethod & LWIP_QOS_CMP_METHOD_BOTH) == LWIP_QOS_CMP_METHOD_BOTH) {
                    pcCmpMethod = "BOTH";
                } else if (pqosru->QOSRU_iCmpMethod & LWIP_QOS_CMP_METHOD_SRC) {
                    pcCmpMethod = "SRC";
                } else if (pqosru->QOSRU_iCmpMethod & LWIP_QOS_CMP_METHOD_DEST) {
                    pcCmpMethod = "DEST";
                } else {
                    pcCmpMethod = "<unknown>";
                }
                
                *pstOft = bnprintf(pcBuffer, stTotalSize, *pstOft, 
                         "%-5s %-6s %6d %-4s %-10s %4d %9s %-15s %-15s %-6d %-6d\n",
                         pqosni->QOSNI_cName, (pqosni->QOSNI_bAttached) ? "YES" : "NO",
                         iSeqNum, "UDP", pcCmpMethod, pqosru->QOSRU_ucPrio,
                         pqosru->QOSRU_ucDontDrop ? "YES" : "NO",
                         ip4addr_ntoa_r(&ipaddrS, cIpBuffer1, INET_ADDRSTRLEN),
                         ip4addr_ntoa_r(&ipaddrE, cIpBuffer2, INET_ADDRSTRLEN),
                         pqosru->QOSRU_usPortHboS,
                         pqosru->QOSRU_usPortHboE);
                iSeqNum++;
            }

            /*
             *  ���� TCP �����
             */
            for (plineTemp  = pqosni->QOSNI_qosrnRule[LWIP_QOS_RULE_TCP];
                 plineTemp != LW_NULL;
                 plineTemp  = _list_line_get_next(plineTemp)) {
                __PQOS_RULE_TCP  pqosrt;
                ip4_addr_t       ipaddrS, ipaddrE;

                pqosrt = _LIST_ENTRY(plineTemp, __QOS_RULE_TCP, QOSRT_lineManage);
                
                ipaddrS.addr = PP_HTONL(pqosrt->QOSRT_ipaddrHboS.addr);
                ipaddrE.addr = PP_HTONL(pqosrt->QOSRT_ipaddrHboE.addr);

                if ((pqosrt->QOSRT_iCmpMethod & LWIP_QOS_CMP_METHOD_BOTH) == LWIP_QOS_CMP_METHOD_BOTH) {
                    pcCmpMethod = "BOTH";
                } else if (pqosrt->QOSRT_iCmpMethod & LWIP_QOS_CMP_METHOD_SRC) {
                    pcCmpMethod = "SRC";
                } else if (pqosrt->QOSRT_iCmpMethod & LWIP_QOS_CMP_METHOD_DEST) {
                    pcCmpMethod = "DEST";
                } else {
                    pcCmpMethod = "<unknown>";
                }

                *pstOft = bnprintf(pcBuffer, stTotalSize, *pstOft, 
                         "%-5s %-6s %6d %-4s %-10s %4d %9s %-15s %-15s %-6d %-6d\n",
                         pqosni->QOSNI_cName, (pqosni->QOSNI_bAttached) ? "YES" : "NO",
                         iSeqNum, "TCP", pcCmpMethod, pqosrt->QOSRT_ucPrio,
                         pqosrt->QOSRT_ucDontDrop ? "YES" : "NO",
                         ip4addr_ntoa_r(&ipaddrS, cIpBuffer1, INET_ADDRSTRLEN),
                         ip4addr_ntoa_r(&ipaddrE, cIpBuffer2, INET_ADDRSTRLEN),
                         pqosrt->QOSRT_usPortHboS,
                         pqosrt->QOSRT_usPortHboE);
                iSeqNum++;
            }
        }
    }
    
    *pstOft = bnprintf(pcBuffer, stTotalSize, *pstOft, 
                       "\nQoS State: %s\n", tcpip_qos_get() ? "On" : "Off");
}
/*********************************************************************************************************
** ��������: __procFsQosRead
** ��������: procfs ��һ����ȡ���� qos �ļ�
** �䡡��  : p_pfsn        �ڵ���ƿ�
**           pcBuffer      ������
**           stMaxBytes    ��������С
**           oft           �ļ�ָ��
** �䡡��  : ʵ�ʶ�ȡ����Ŀ
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static ssize_t  __procFsQosRead (PLW_PROCFS_NODE  p_pfsn, 
                                 PCHAR            pcBuffer, 
                                 size_t           stMaxBytes,
                                 off_t            oft)
{
    const CHAR      cQosInfoHdr[] = 
    "NETIF ATTACH SEQNUM RULE CMP_METHOD PRIO DONT_DROP IPs             IPe             PORTs  PORTe\n";
    
          PCHAR     pcFileBuffer;
          size_t    stRealSize;                                         /*  ʵ�ʵ��ļ����ݴ�С          */
          size_t    stCopeBytes;
    
    /*
     *  ����Ԥ���ڴ��СΪ 0 , ���Դ򿪺��һ�ζ�ȡ��Ҫ�ֶ������ڴ�.
     */
    pcFileBuffer = (PCHAR)API_ProcFsNodeBuffer(p_pfsn);
    if (pcFileBuffer == LW_NULL) {                                      /*  ��û�з����ڴ�              */
        size_t  stNeedBufferSize = 0;
        
        __QOS_LOCK_R();                                                 /*  ���� QoS ��                 */
        stNeedBufferSize = (size_t)(_G_ulQosCounter * 128);
        __QOS_UNLOCK();                                                 /*  ���� QoS ��                 */
        
        stNeedBufferSize += sizeof(cQosInfoHdr) + 64;
        
        if (API_ProcFsAllocNodeBuffer(p_pfsn, stNeedBufferSize)) {
            _ErrorHandle(ENOMEM);
            return  (0);
        }
        pcFileBuffer = (PCHAR)API_ProcFsNodeBuffer(p_pfsn);             /*  ���»���ļ���������ַ      */
        
        stRealSize = bnprintf(pcFileBuffer, stNeedBufferSize, 0, cQosInfoHdr);
                                                                        /*  ��ӡͷ��Ϣ                  */
                                                                        
        __QOS_LOCK_R();                                                 /*  ���� QoS ��                 */
        __procFsQosPrint(pcFileBuffer, stNeedBufferSize, &stRealSize);
        __QOS_UNLOCK();                                                 /*  ���� QoS ��                 */
        
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
** ��������: __procFsQosInit
** ��������: procfs ��ʼ������ qos �ļ�
** �䡡��  : NONE
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
VOID  __procFsQosInit (VOID)
{
    API_ProcFsMakeNode(&_G_pfsnQos[0],  "/net");
}

#endif                                                                  /*  LW_CFG_PROCFS_EN > 0        */
#endif                                                                  /*  LW_CFG_NET_EN > 0           */
                                                                        /*  LW_CFG_NET_QOS_EN > 0       */
/*********************************************************************************************************
  END
*********************************************************************************************************/
