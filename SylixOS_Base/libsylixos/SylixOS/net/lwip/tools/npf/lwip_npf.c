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
** ��   ��   ��: lwip_npf.c
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2011 �� 02 �� 11 ��
**
** ��        ��: lwip net packet filter ����.

** BUG:
2011.07.07  ����һЩƴд����.
2013.09.11  ���� /proc/net/netfilter �ļ�.
            UDP/TCP �˲�����˿ڷ�Χ.
2013.09.12  ʹ�� bnprintf ר�û�������ӡ����.
2016.04.13  __npfInput() �Լ��ͷ����ݰ�����.
2018.01.24  ʹ���µ� firewall �ӿ�.
2018.08.06  ��Ϊ��д��, ɾ���ֶ� attach detach �ӿ�, ��Ϊ�Զ�ģʽ.
2018.08.17  ֧�� [in | out] [allow | deny] ѡ��, ʵ�ֽ�Ϊ�����ķ���ǽ����.
*********************************************************************************************************/
#define  __SYLIXOS_STDIO
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
#include "../SylixOS/system/include/s_system.h"
#include "../SylixOS/net/include/net_net.h"
/*********************************************************************************************************
  �ü�����
*********************************************************************************************************/
#if (LW_CFG_NET_EN > 0) && (LW_CFG_NET_NPF_EN > 0)
#include "net/if.h"
#include "net/if_lock.h"
#include "lwip/netif.h"
#include "lwip/tcpip.h"
#include "lwip/inet.h"
#include "lwip/prot/ip.h"
#include "lwip/prot/udp.h"
#include "lwip/prot/tcp.h"
#include "lwip/priv/tcp_priv.h"
#include "netif/etharp.h"
/*********************************************************************************************************
  �궨��
*********************************************************************************************************/
#define __NPF_NETIF_RULE_FAST       1                                   /*  ���ټ��ģʽ                */
#define __NPF_NETIF_RULE_MAX        4                                   /*  ���������                  */
#define __NPF_NETIF_HASH_SIZE       16                                  /*  ����ӿ� hash ���С        */
#define __NPF_NETIF_HASH_MASK       (__NPF_NETIF_HASH_SIZE - 1)         /*  hash ����                   */
/*********************************************************************************************************
  ����������
*********************************************************************************************************/
static LW_OBJECT_HANDLE             _G_ulNpfLock;

#define __NPF_LOCK_R()              API_SemaphoreRWPendR(_G_ulNpfLock, LW_OPTION_WAIT_INFINITE)
#define __NPF_LOCK_W()              API_SemaphoreRWPendW(_G_ulNpfLock, LW_OPTION_WAIT_INFINITE)
#define __NPF_UNLOCK()              API_SemaphoreRWPost(_G_ulNpfLock)
/*********************************************************************************************************
  ͳ����Ϣ (���治��͹�����ֹ, ������ɶ�����������1)
*********************************************************************************************************/
static ULONG    _G_ulNpfDropPacketCounter  = 0;                         /*  �������ݱ��ĸ���            */
static ULONG    _G_ulNpfAllowPacketCounter = 0;                         /*  �������ݱ��ĸ���            */

#define __NPF_PACKET_DROP_INC()     (_G_ulNpfDropPacketCounter++)
#define __NPF_PACKET_ALLOW_INC()    (_G_ulNpfAllowPacketCounter++)
#define __NPF_PACKET_DROP_GET()     (_G_ulNpfDropPacketCounter)
#define __NPF_PACKET_ALLOW_GET()    (_G_ulNpfAllowPacketCounter)
/*********************************************************************************************************
  ������ƽṹ

  ע��:
       �����Ҫ���� UDP/TCP ĳһ�˿�, ��ʼ�ͽ��� IP ��ַ�ֶξ���Ϊ 0.0.0.0 ~ FF.FF.FF.FF
       NPFR?_iRule �ֶ���Ϊ�˴Ӿ����ȡ���������
*********************************************************************************************************/
typedef struct {
    LW_LIST_LINE            NPFRM_lineManage;                           /*  MAC �����������            */
    INT                     NPFRM_iRule;
    BOOL                    NPFRM_bAllow;
    u8_t                    NPFRM_ucMac[NETIF_MAX_HWADDR_LEN];          /*  ��������յ� MAC ��ַ       */
} __NPF_RULE_MAC;
typedef __NPF_RULE_MAC     *__PNPF_RULE_MAC;

typedef struct {
    LW_LIST_LINE            NPFRI_lineManage;                           /*  IP �����������             */
    INT                     NPFRI_iRule;
    BOOL                    NPFRI_bAllow;
    ip4_addr_t              NPFRI_ipaddrHboS;                           /*  IP ����ʼ IP ��ַ           */
    ip4_addr_t              NPFRI_ipaddrHboE;                           /*  IP �ν��� IP ��ַ           */
} __NPF_RULE_IP;
typedef __NPF_RULE_IP      *__PNPF_RULE_IP;

typedef struct {
    LW_LIST_LINE            NPFRU_lineManage;                           /*  UDP �����������            */
    INT                     NPFRU_iRule;
    BOOL                    NPFRU_bAllow;
    ip4_addr_t              NPFRU_ipaddrHboS;                           /*  IP ����ʼ IP ��ַ           */
    ip4_addr_t              NPFRU_ipaddrHboE;                           /*  IP �ν��� IP ��ַ           */
    u16_t                   NPFRU_usPortHboS;                           /*  �˿���ʼ ������             */
    u16_t                   NPFRU_usPortHboE;                           /*  �˿ڽ���                    */
} __NPF_RULE_UDP;
typedef __NPF_RULE_UDP     *__PNPF_RULE_UDP;

typedef struct {
    LW_LIST_LINE            NPFRT_lineManage;                           /*  TCP �����������            */
    INT                     NPFRT_iRule;
    BOOL                    NPFRT_bAllow;
    ip4_addr_t              NPFRT_ipaddrHboS;                           /*  IP ����ʼ IP ��ַ           */
    ip4_addr_t              NPFRT_ipaddrHboE;                           /*  IP �ν��� IP ��ַ           */
    u16_t                   NPFRT_usPortHboS;                           /*  �˿���ʼ ������             */
    u16_t                   NPFRT_usPortHboE;                           /*  �˿ڽ���                    */
} __NPF_RULE_TCP;
typedef __NPF_RULE_TCP     *__PNPF_RULE_TCP;
/*********************************************************************************************************
  ����������ӿڽṹ
*********************************************************************************************************/
typedef struct {
    LW_LIST_LINE            NPFNI_lineHash;                             /*  hash ��                     */
    LW_LIST_LINE_HEADER     NPFNI_npfrnIn[__NPF_NETIF_RULE_MAX];        /*  ��������                  */
    LW_LIST_LINE_HEADER     NPFNI_npfrnOut[__NPF_NETIF_RULE_MAX];       /*  ��������                  */
    CHAR                    NPFNI_cName[IF_NAMESIZE];                   /*  ����ӿ���                  */
    BOOL                    NPFNI_bAttached;                            /*  �Ƿ��Ѿ�����                */
} __NPF_NETIF_CB;
typedef __NPF_NETIF_CB     *__PNPF_NETIF_CB;
/*********************************************************************************************************
  ���� hash ��
*********************************************************************************************************/
static LW_LIST_LINE_HEADER  _G_plineNpfHash[__NPF_NETIF_HASH_SIZE];     /*  ͨ�� name & num ����ɢ��    */
static ULONG                _G_ulNpfCounter = 0ul;                      /*  ���������                  */
/*********************************************************************************************************
  ��������
*********************************************************************************************************/
static INT    npf_netif_firewall(struct netif *pnetif, struct pbuf *p);
#if LW_CFG_SHELL_EN > 0
static INT    __tshellNetNpfShow(INT  iArgC, PCHAR  *ppcArgV);
static INT    __tshellNetNpfRuleAdd(INT  iArgC, PCHAR  *ppcArgV);
static INT    __tshellNetNpfRuleDel(INT  iArgC, PCHAR  *ppcArgV);
#endif                                                                  /*  LW_CFG_SHELL_EN > 0         */
#if LW_CFG_PROCFS_EN > 0
static VOID   __procFsNpfInit(VOID);
#endif                                                                  /*  LW_CFG_PROCFS_EN > 0        */
/*********************************************************************************************************
** ��������: __npfNetifGet
** ��������: ���ָ���Ĺ���������ӿڽṹ
** �䡡��  : pnetif         ����ӿ�
** �䡡��  : �ҵ��Ĺ���������ӿڽṹ
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static LW_INLINE __PNPF_NETIF_CB  __npfNetifGet (struct netif *pnetif)
{
    return  ((__PNPF_NETIF_CB)pnetif->inner_fw_stat);
}
/*********************************************************************************************************
** ��������: __npfNetifFind
** ��������: ����ָ���Ĺ���������ӿڽṹ
** �䡡��  : pcIfname       ����ӿ�����
** �䡡��  : �ҵ��Ĺ���������ӿڽṹ
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static __PNPF_NETIF_CB  __npfNetifFind (CPCHAR  pcIfname)
{
    REGISTER INT                    iIndex;
             PLW_LIST_LINE          plineTemp;
             __PNPF_NETIF_CB        pnpfniTemp;


    iIndex = (pcIfname[0] + pcIfname[1] + pcIfname[2]) & __NPF_NETIF_HASH_MASK;

    for (plineTemp  = _G_plineNpfHash[iIndex];
         plineTemp != LW_NULL;
         plineTemp  = _list_line_get_next(plineTemp)) {

        pnpfniTemp = _LIST_ENTRY(plineTemp, __NPF_NETIF_CB, NPFNI_lineHash);
        if (lib_strncmp(pnpfniTemp->NPFNI_cName, pcIfname, IF_NAMESIZE) == 0) {
            return  (pnpfniTemp);
        }
    }

    return  (LW_NULL);
}
/*********************************************************************************************************
** ��������: __npfNetifInsertHash
** ��������: ��ָ���Ĺ���������ӿڽṹ���� hash ��
** �䡡��  : pnpfni        ����������ӿڽṹ
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static VOID  __npfNetifInsertHash (__PNPF_NETIF_CB  pnpfni)
{
    REGISTER INT                    iIndex;
    REGISTER LW_LIST_LINE_HEADER   *ppline;

    iIndex = (pnpfni->NPFNI_cName[0]
           +  pnpfni->NPFNI_cName[1]
           +  pnpfni->NPFNI_cName[2])
           & __NPF_NETIF_HASH_MASK;

    ppline = &_G_plineNpfHash[iIndex];

    _List_Line_Add_Ahead(&pnpfni->NPFNI_lineHash, ppline);
}
/*********************************************************************************************************
** ��������: __npfNetifDeleteHash
** ��������: ��ָ���Ĺ���������ӿڽṹ�� hash �����Ƴ�
** �䡡��  : pnpfni        ����������ӿڽṹ
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static VOID  __npfNetifDeleteHash (__PNPF_NETIF_CB  pnpfni)
{
    REGISTER INT                    iIndex;
    REGISTER LW_LIST_LINE_HEADER   *ppline;

    iIndex = (pnpfni->NPFNI_cName[0]
           +  pnpfni->NPFNI_cName[1]
           +  pnpfni->NPFNI_cName[2])
           & __NPF_NETIF_HASH_MASK;

    ppline = &_G_plineNpfHash[iIndex];

    _List_Line_Del(&pnpfni->NPFNI_lineHash, ppline);
}
/*********************************************************************************************************
** ��������: __npfNetifCreate
** ��������: ����һ������������ӿڽṹ (���������ֱ�ӷ������е�)
** �䡡��  : pcIfname         ����ӿ���
** �䡡��  : �������Ĺ���������ӿڽṹ
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static __PNPF_NETIF_CB  __npfNetifCreate (CPCHAR  pcIfname)
{
    INT                    i;
    __PNPF_NETIF_CB        pnpfni;
    struct netif          *pnetif;

    pnpfni = __npfNetifFind(pcIfname);
    if (pnpfni == LW_NULL) {
        pnpfni =  (__PNPF_NETIF_CB)__SHEAP_ALLOC(sizeof(__NPF_NETIF_CB));
        if (pnpfni == LW_NULL) {
            _DebugHandle(__ERRORMESSAGE_LEVEL, "system low memory.\r\n");
            _ErrorHandle(ERROR_SYSTEM_LOW_MEMORY);
            return  (LW_NULL);
        }
        lib_strlcpy(pnpfni->NPFNI_cName, pcIfname, IF_NAMESIZE);
        for (i = 0; i < __NPF_NETIF_RULE_MAX; i++) {
            pnpfni->NPFNI_npfrnIn[i]  = LW_NULL;                        /*  û���κι���                */
            pnpfni->NPFNI_npfrnOut[i] = LW_NULL;
        }
        __npfNetifInsertHash(pnpfni);
        
        pnetif = netif_find(pcIfname);
        if (pnetif) {
            pnetif->inner_fw_stat  = (void *)pnpfni;
            pnetif->inner_fw       = npf_netif_firewall;
            pnpfni->NPFNI_bAttached = LW_TRUE;
        
        } else {
            pnpfni->NPFNI_bAttached = LW_FALSE;
        }
    }

    return  (pnpfni);
}
/*********************************************************************************************************
** ��������: __npfNetifDelete
** ��������: ɾ��һ������������ӿڽṹ
** �䡡��  : pnpfni        ����������ӿڽṹ
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static VOID  __npfNetifDelete (__PNPF_NETIF_CB  pnpfni)
{
    __npfNetifDeleteHash(pnpfni);
    __SHEAP_FREE(pnpfni);
}
/*********************************************************************************************************
** ��������: __npfMacRuleCheck
** ��������: ��� MAC �����Ƿ��������ݱ�ͨ��
** �䡡��  : pnpfni        ����������ӿ�
             bIn           IN or OUT
             pethhdr       ��̫��ͷ
** �䡡��  : �Ƿ��������ݱ�ͨ��
** ȫ�ֱ���:
** ����ģ��:
** ע  ��  : ֻҪ��һ�� allow �ڵ�, ������������ͨ��.
*********************************************************************************************************/
static BOOL  __npfMacRuleCheck (__PNPF_NETIF_CB  pnpfni, BOOL  bIn, struct eth_hdr *pethhdr)
{
    BOOL                bAllow = LW_TRUE;
    PLW_LIST_LINE       plineTemp;
    __PNPF_RULE_MAC     pnpfrm;
    
    plineTemp = bIn ? 
                pnpfni->NPFNI_npfrnIn[LWIP_NPF_RULE_MAC] : 
                pnpfni->NPFNI_npfrnOut[LWIP_NPF_RULE_MAC];

    while (plineTemp) {
        pnpfrm = _LIST_ENTRY(plineTemp, __NPF_RULE_MAC, NPFRM_lineManage);
        if (pnpfrm->NPFRM_bAllow) {
            if (lib_memcmp(pnpfrm->NPFRM_ucMac,
                           pethhdr->src.addr,
                           ETHARP_HWADDR_LEN) == 0) {                   /*  �Ƚ� hwaddr                 */
                return  (LW_TRUE);                                      /*  ������, ��������            */
            }
            bAllow = LW_FALSE;                                          /*  ��ֹͨ��                    */
            
        } else {
            if (lib_memcmp(pnpfrm->NPFRM_ucMac,
                           pethhdr->src.addr,
                           ETHARP_HWADDR_LEN) == 0) {                   /*  �Ƚ� hwaddr                 */
                bAllow = LW_FALSE;                                      /*  ��ֹͨ��                    */
            }
        }
        plineTemp = _list_line_get_next(plineTemp);
    }
    
    return  (bAllow);
}
/*********************************************************************************************************
** ��������: __npfIpRuleCheck
** ��������: ��� IP �����Ƿ��������ݱ�ͨ��
** �䡡��  : pnpfni        ����������ӿ�
             bIn           IN or OUT
             piphdr        IP ��ͷ
** �䡡��  : �Ƿ��������ݱ�ͨ��
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static BOOL  __npfIpRuleCheck (__PNPF_NETIF_CB  pnpfni, BOOL  bIn, struct ip_hdr *piphdr)
{
    BOOL                bAllow = LW_TRUE;
    PLW_LIST_LINE       plineTemp;
    __PNPF_RULE_IP      pnpfri;
    ip4_addr_t          ipaddrHbo;

    if (bIn) {
        plineTemp = pnpfni->NPFNI_npfrnIn[LWIP_NPF_RULE_IP];
        ipaddrHbo.addr = PP_NTOHL(piphdr->src.addr);
        
    } else {
        plineTemp = pnpfni->NPFNI_npfrnOut[LWIP_NPF_RULE_IP];
        ipaddrHbo.addr = PP_NTOHL(piphdr->dest.addr);
    }
    
    while (plineTemp) {
        pnpfri = _LIST_ENTRY(plineTemp, __NPF_RULE_IP, NPFRI_lineManage);
        if (pnpfri->NPFRI_bAllow) {
            if ((ipaddrHbo.addr >= pnpfri->NPFRI_ipaddrHboS.addr) &&
                (ipaddrHbo.addr <= pnpfri->NPFRI_ipaddrHboE.addr)) {
                return  (LW_TRUE);                                      /*  ������, ��������            */
            }
            bAllow = LW_FALSE;                                          /*  ��ֹͨ��                    */

        } else {
            if ((ipaddrHbo.addr >= pnpfri->NPFRI_ipaddrHboS.addr) &&
                (ipaddrHbo.addr <= pnpfri->NPFRI_ipaddrHboE.addr)) {
                bAllow = LW_FALSE;                                      /*  ��ֹͨ��                    */
            }
        }
        plineTemp = _list_line_get_next(plineTemp);
    }

    return  (bAllow);
}
/*********************************************************************************************************
** ��������: __npfUdpRuleCheck
** ��������: ��� UDP �����Ƿ��������ݱ�ͨ��
** �䡡��  : pnpfni        ����������ӿ�
             bIn           IN or OUT
             piphdr        IP ��ͷ
             pudphdr       UDP ��ͷ
** �䡡��  : �Ƿ��������ݱ�ͨ��
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static BOOL  __npfUdpRuleCheck (__PNPF_NETIF_CB  pnpfni, BOOL  bIn, 
                                struct ip_hdr *piphdr, struct udp_hdr *pudphdr)
{
    BOOL                bAllow = LW_TRUE;
    PLW_LIST_LINE       plineTemp;
    __PNPF_RULE_UDP     pnpfru;
    ip4_addr_t          ipaddrHbo;
    u16_t               usPortHbo;
    
    if (bIn) {
        plineTemp = pnpfni->NPFNI_npfrnIn[LWIP_NPF_RULE_UDP];
        ipaddrHbo.addr = PP_NTOHL(piphdr->src.addr);
        usPortHbo      = PP_NTOHS(pudphdr->dest);
        
    } else {
        plineTemp = pnpfni->NPFNI_npfrnOut[LWIP_NPF_RULE_UDP];
        ipaddrHbo.addr = PP_NTOHL(piphdr->dest.addr);
        usPortHbo      = PP_NTOHS(pudphdr->dest);
    }

    while (plineTemp) {
        pnpfru = _LIST_ENTRY(plineTemp, __NPF_RULE_UDP, NPFRU_lineManage);
        if (pnpfru->NPFRU_bAllow) {
            if ((ipaddrHbo.addr >= pnpfru->NPFRU_ipaddrHboS.addr) &&
                (ipaddrHbo.addr <= pnpfru->NPFRU_ipaddrHboE.addr) &&
                (usPortHbo      >= pnpfru->NPFRU_usPortHboS)      &&
                (usPortHbo      <= pnpfru->NPFRU_usPortHboE)) {
                return  (LW_TRUE);                                      /*  ������, ��������            */
            }
            bAllow = LW_FALSE;                                          /*  ��ֹͨ��                    */

        } else {
            if ((ipaddrHbo.addr >= pnpfru->NPFRU_ipaddrHboS.addr) &&
                (ipaddrHbo.addr <= pnpfru->NPFRU_ipaddrHboE.addr) &&
                (usPortHbo      >= pnpfru->NPFRU_usPortHboS)      &&
                (usPortHbo      <= pnpfru->NPFRU_usPortHboE)) {
                bAllow = LW_FALSE;                                      /*  ��ֹͨ��                    */
            }
        }
        plineTemp = _list_line_get_next(plineTemp);
    }

    return  (bAllow);
}
/*********************************************************************************************************
** ��������: __npfTcpRuleCheck
** ��������: ��� TCP �����Ƿ��������ݱ�ͨ��
** �䡡��  : pnpfni        ����������ӿ�
             piphdr        IP ��ͷ
             ptcphdr       TCP ��ͷ
** �䡡��  : �Ƿ��������ݱ�ͨ��
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static BOOL  __npfTcpRuleCheck (__PNPF_NETIF_CB  pnpfni, BOOL  bIn, 
                                struct ip_hdr *piphdr, struct tcp_hdr *ptcphdr)
{
    BOOL                bAllow = LW_TRUE;
    PLW_LIST_LINE       plineTemp;
    __PNPF_RULE_TCP     pnpfrt;
    ip4_addr_t          ipaddrHbo;
    u16_t               usPortHbo;
    
    if (bIn) {
        plineTemp = pnpfni->NPFNI_npfrnIn[LWIP_NPF_RULE_TCP];
        ipaddrHbo.addr = PP_NTOHL(piphdr->src.addr);
        usPortHbo      = PP_NTOHS(ptcphdr->dest);
        
    } else {
        plineTemp = pnpfni->NPFNI_npfrnOut[LWIP_NPF_RULE_TCP];
        ipaddrHbo.addr = PP_NTOHL(piphdr->dest.addr);
        usPortHbo      = PP_NTOHS(ptcphdr->dest);
    }

    while (plineTemp) {
        pnpfrt = _LIST_ENTRY(plineTemp, __NPF_RULE_TCP, NPFRT_lineManage);
        if (pnpfrt->NPFRT_bAllow) {
            if ((ipaddrHbo.addr >= pnpfrt->NPFRT_ipaddrHboS.addr) &&
                (ipaddrHbo.addr <= pnpfrt->NPFRT_ipaddrHboE.addr) &&
                (usPortHbo      >= pnpfrt->NPFRT_usPortHboS)      &&
                (usPortHbo      <= pnpfrt->NPFRT_usPortHboE)) {
                return  (LW_TRUE);                                      /*  ������, ��������            */
            }
            bAllow = LW_FALSE;                                          /*  ��ֹͨ��                    */

        } else {
            if ((ipaddrHbo.addr >= pnpfrt->NPFRT_ipaddrHboS.addr) &&
                (ipaddrHbo.addr <= pnpfrt->NPFRT_ipaddrHboE.addr) &&
                (usPortHbo      >= pnpfrt->NPFRT_usPortHboS)      &&
                (usPortHbo      <= pnpfrt->NPFRT_usPortHboE)) {
                bAllow = LW_FALSE;                                      /*  ��ֹͨ��                    */
            }
        }
        plineTemp = _list_line_get_next(plineTemp);
    }

    return  (bAllow);
}
/*********************************************************************************************************
** ��������: npf_netif_firewall
** ��������: ���������, �������������ݱ���������Э��ջ
** �䡡��  : pnetif        ����ӿ�
**           p             ���ݰ�
** �䡡��  : �Ƿ� eaten
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  npf_netif_firewall (struct netif *pnetif, struct pbuf *p)
{
    __PNPF_NETIF_CB     pnpfni;                                         /*  ����������ӿ�              */
    INT                 iOffset = 0;                                    /*  �������ݵ���ʼƫ����        */

#if __NPF_NETIF_RULE_FAST > 0
    struct eth_hdr     *pethhdr;                                        /*  eth ͷ                      */
    struct ip_hdr      *piphdr;                                         /*  ip ͷ                       */
    struct udp_hdr     *pudphdr;                                        /*  udp ͷ                      */
    struct tcp_hdr     *ptcphdr;                                        /*  tcp ͷ                      */

    __NPF_LOCK_R();                                                     /*  ���� NPF ��                 */
    pnpfni = __npfNetifGet(pnetif);                                     /*  ����������ӿ�              */
    if (pnpfni == LW_NULL) {                                            /*  û���ҵ���Ӧ�Ŀ��ƽṹ      */
        __NPF_PACKET_ALLOW_INC();
        __NPF_UNLOCK();                                                 /*  ���� NPF ��                 */
        return  (0);
    }
    
    /*
     *  ethernet ���˴���
     */
    if (pnetif->flags & (NETIF_FLAG_ETHARP | NETIF_FLAG_ETHERNET)) {    /*  ��̫���ӿ�                  */
        if (p->len < sizeof(struct eth_hdr)) {
            goto    __allow_input;                                      /*  �޷���ȡ eth hdr ��������   */
        }
        
        pethhdr = (struct eth_hdr *)((char *)p->payload + iOffset);
        if (__npfMacRuleCheck(pnpfni, LW_TRUE, pethhdr)) {              /*  ��ʼ�����Ӧ�� MAC ���˹��� */
            iOffset += sizeof(struct eth_hdr);                          /*  ����ͨ��                    */
        
        } else {
            goto    __drop_input;
        }
    }
    
    /*
     *  ip ���˴���
     */
    if (iOffset == sizeof(struct eth_hdr)) {                            /*  ǰ���� eth hdr �ֶ�         */
        if (pethhdr->type == PP_HTONS(ETHTYPE_VLAN)) {
            struct eth_vlan_hdr *pvlanhdr;
            
            if (p->len < iOffset + sizeof(struct eth_vlan_hdr)) {
                goto    __allow_input;                                  /*  �޷���ȡ vlan hdr ��������  */
            }
            
            pvlanhdr = (struct eth_vlan_hdr *)((char *)p->payload + iOffset);
            if (pvlanhdr->tpid != PP_HTONS(ETHTYPE_IP)) {
                goto    __allow_input;
            }
            iOffset += sizeof(struct eth_vlan_hdr);
        
        } else if (pethhdr->type != PP_HTONS(ETHTYPE_IP)) {
            goto    __allow_input;                                      /*  ���� ip ���ݰ�, ����        */
        }
    }
    
    piphdr = (struct ip_hdr *)((char *)p->payload + iOffset);
    if (IPH_V(piphdr) != 4) {                                           /*  �� ipv4 ���ݰ�              */
        goto    __allow_input;                                          /*  ����                        */
    }

    if (__npfIpRuleCheck(pnpfni, LW_TRUE, piphdr)) {                    /*  ��ʼ�����Ӧ�� ip ���˹���  */
        iOffset += (IPH_HL(piphdr) << 2);                               /*  ����ͨ��                    */
    
    } else {
        goto    __drop_input;
    }
    
    if ((IPH_OFFSET(piphdr) & PP_HTONS(IP_OFFMASK)) != 0) {             /*  ��Ƭ���ݰ�                  */
        goto    __allow_input;
    }
    
    switch (IPH_PROTO(piphdr)) {                                        /*  ��� ip ���ݱ�����          */

    case IP_PROTO_UDP:                                                  /*  udp ���˴���                */
    case IP_PROTO_UDPLITE:
        if (p->len < (iOffset + sizeof(struct udp_hdr))) {
            goto    __allow_input;
        }
        pudphdr = (struct udp_hdr *)((char *)p->payload + iOffset);
        if (__npfUdpRuleCheck(pnpfni, LW_TRUE, piphdr, pudphdr) == LW_FALSE) {
            goto    __drop_input;
        }
        break;

    case IP_PROTO_TCP:                                                  /*  tcp ���˴���                */
        if (p->len < (iOffset + sizeof(struct tcp_hdr))) {
            goto    __allow_input;
        }
        ptcphdr = (struct tcp_hdr *)((char *)p->payload + iOffset);
        if (__npfTcpRuleCheck(pnpfni, LW_TRUE, piphdr, ptcphdr) == LW_FALSE) {
            goto    __drop_input;
        }
        break;

    default:
        break;
    }
    
#else                                                                   /*  __NPF_NETIF_RULE_FAST       */
    struct eth_hdr      ethhdrChk;                                      /*  eth ͷ                      */
    struct ip_hdr       iphdrChk;                                       /*  ip ͷ                       */
    struct udp_hdr      udphdrChk;                                      /*  udp ͷ                      */
    struct tcp_hdr      tcphdrChk;                                      /*  tcp ͷ                      */

    __NPF_LOCK_R();                                                     /*  ���� NPF ��                 */
    pnpfni = __npfNetifGet();                                           /*  ����������ӿ�              */
    if (pnpfni == LW_NULL) {                                            /*  û���ҵ���Ӧ�Ŀ��ƽṹ      */
        __NPF_PACKET_ALLOW_INC();
        __NPF_UNLOCK();                                                 /*  ���� NPF ��                 */
        return  (0);
    }

    /*
     *  ethernet ���˴���
     */
    if (pnetif->flags & (NETIF_FLAG_ETHARP | NETIF_FLAG_ETHERNET)) {    /*  ��̫���ӿ�                  */
        if (pbuf_copy_partial(p, (void *)&ethhdrChk,
                              sizeof(struct eth_hdr), (u16_t)iOffset) != sizeof(struct eth_hdr)) {
            goto    __allow_input;                                      /*  �޷���ȡ eth hdr ��������   */
        }
        
        if (__npfMacRuleCheck(pnpfni, LW_TRUE, &ethhdrChk)) {           /*  ��ʼ�����Ӧ�� MAC ���˹��� */
            iOffset += sizeof(struct eth_hdr);                          /*  ����ͨ��                    */
        
        } else {
            goto    __drop_input;
        }
    }

    /*
     *  ip ���˴���
     */
    if (iOffset == sizeof(struct eth_hdr)) {                            /*  ǰ���� eth hdr �ֶ�         */
        if (ethhdrChk.type == PP_HTONS(ETHTYPE_VLAN)) {
            struct eth_vlan_hdr vlanhdrChk;
            
            if (pbuf_copy_partial(p, (void *)&vlanhdrChk,
                                  sizeof(struct eth_vlan_hdr), (u16_t)iOffset) != 
                                  sizeof(struct eth_vlan_hdr)) {        /*  VLAN ��ͷ                   */
                goto    __allow_input;
            }
            if (vlanhdrChk.tpid != PP_HTONS(ETHTYPE_IP)) {
                goto    __allow_input;
            }
            iOffset += sizeof(struct eth_vlan_hdr);
        
        } else if (ethhdrChk.type != PP_HTONS(ETHTYPE_IP)) {
            goto    __allow_input;                                      /*  ���� ip ���ݰ�, ����        */
        }
    }
    
    if (pbuf_copy_partial(p, (void *)&iphdrChk,
                          sizeof(struct ip_hdr), (u16_t)iOffset) != sizeof(struct ip_hdr)) {
        goto    __allow_input;                                          /*  �޷���ȡ ip hdr ����        */
    }
    
    if (IPH_V((&iphdrChk)) != 4) {                                      /*  �� ipv4 ���ݰ�              */
        goto    __allow_input;                                          /*  ����                        */
    }

    if (__npfIpRuleCheck(pnpfni, LW_TRUE, &iphdrChk)) {                 /*  ��ʼ�����Ӧ�� ip ���˹���  */
        iOffset += (IPH_HL((&iphdrChk)) * 4);                           /*  ����ͨ��                    */
    
    } else {
        goto    __drop_input;
    }
    
    if ((IPH_OFFSET(&iphdrChk) & PP_HTONS(IP_OFFMASK)) != 0) {          /*  ��Ƭ���ݰ�                  */
        goto    __allow_input;
    }

    switch (IPH_PROTO((&iphdrChk))) {                                   /*  ��� ip ���ݱ�����          */

    case IP_PROTO_UDP:                                                  /*  udp ���˴���                */
    case IP_PROTO_UDPLITE:
        if (pbuf_copy_partial(p, (void *)&udphdrChk,
                              sizeof(struct udp_hdr), (u16_t)iOffset) != sizeof(struct udp_hdr)) {
            goto    __allow_input;                                      /*  �޷���ȡ udp hdr ����       */
        }
        if (__npfUdpRuleCheck(pnpfni, LW_TRUE, &iphdrChk, &udphdrChk) == LW_FALSE) {
            goto    __drop_input;
        }
        break;

    case IP_PROTO_TCP:                                                  /*  tcp ���˴���                */
        if (pbuf_copy_partial(p, (void *)&tcphdrChk,
                              sizeof(struct tcp_hdr), (u16_t)iOffset) != sizeof(struct tcp_hdr)) {
            goto    __allow_input;                                      /*  �޷���ȡ tcp hdr ����       */
        }
        if (__npfTcpRuleCheck(pnpfni, LW_TRUE, &iphdrChk, &tcphdrChk) == LW_FALSE) {
            goto    __drop_input;
        }
        break;

    default:
        break;
    }
#endif                                                                  /*  !__NPF_NETIF_RULE_FAST      */

__allow_input:
    __NPF_PACKET_ALLOW_INC();
    __NPF_UNLOCK();                                                     /*  ���� NPF ��                 */
    return  (0);                                                        /*  ��������                    */
    
__drop_input:
    __NPF_PACKET_DROP_INC();
    __NPF_UNLOCK();                                                     /*  ���� NPF ��                 */
    pbuf_free(p);                                                       /*  �ͷ����ݰ�                  */
    return  (1);                                                        /*  eaten                       */
}
/*********************************************************************************************************
** ��������: npf_netif_attach
** ��������: ������ӿ����ʱ, ���õ� hook ����
** �䡡��  : pnetif       ����ӿ�
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
VOID  npf_netif_attach (struct netif  *pnetif)
{
    CHAR                cIfname[IF_NAMESIZE];
    __PNPF_NETIF_CB     pnpfni;
    
    if (!_G_ulNpfLock) {
        return;
    }
    
    netif_get_name(pnetif, cIfname);
    
    __NPF_LOCK_R();                                                     /*  ���� NPF ��                 */
    pnpfni = __npfNetifFind(cIfname);
    if (pnpfni == LW_NULL) {
        __NPF_UNLOCK();                                                 /*  ���� NPF ��                 */
        return;
    }
    
    pnetif->inner_fw_stat   = (void *)pnpfni;
    pnetif->inner_fw        = npf_netif_firewall;
    pnpfni->NPFNI_bAttached = LW_TRUE;
    __NPF_UNLOCK();                                                     /*  ���� NPF ��                 */
}
/*********************************************************************************************************
** ��������: npf_netif_detach
** ��������: ������ӿ�ɾ��ʱ, ���õ� hook ����
** �䡡��  : pcNetifName       ��Ӧ������ӿ���
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
VOID  npf_netif_detach (struct netif  *pnetif)
{
    INT                 i;
    __PNPF_NETIF_CB     pnpfni;

    __NPF_LOCK_W();                                                     /*  ���� NPF ��                 */
    pnpfni = __npfNetifGet(pnetif);
    if (pnpfni == LW_NULL) {
        __NPF_UNLOCK();                                                 /*  ���� NPF ��                 */
        return;
    }
    
    pnpfni->NPFNI_bAttached = LW_FALSE;
    pnetif->inner_fw_stat   = LW_NULL;
    pnetif->inner_fw        = LW_NULL;                                  /*  ����ʹ�� npf ���뺯��       */

    for (i = 0; i < __NPF_NETIF_RULE_MAX; i++) {
        if (pnpfni->NPFNI_npfrnIn[i] || 
            pnpfni->NPFNI_npfrnOut[i]) {
            break;
        }
    }
    if (i >= __NPF_NETIF_RULE_MAX) {                                    /*  ������ӿڿ��ƽ��û�й���  */
        __npfNetifDelete(pnpfni);
    }
    __NPF_UNLOCK();                                                     /*  ���� NPF ��                 */
}
/*********************************************************************************************************
** ��������: API_INetNpfRuleAdd
** ��������: net packet filter ���һ������
** �䡡��  : pcIfname          ��Ӧ������ӿ���
**           iRule             ��Ӧ�Ĺ���, MAC/IP/UDP/TCP/...
**           pucMac            ��ֹͨ�ŵ� MAC ��ַ����,
**           pcAddrStart       ��ֹͨ�� IP ��ַ��ʼ, Ϊ IP ��ַ�ַ���, ��ʽΪ: ???.???.???.???
**           pcAddrEnd         ��ֹͨ�� IP ��ַ����, Ϊ IP ��ַ�ַ���, ��ʽΪ: ???.???.???.???
**           usPortStart       ��ֹͨ�ŵı�����ʼ�˿ں�(�����ֽ���), �������� UDP/TCP ����
**           usPortEnd         ��ֹͨ�ŵı��ؽ����˿ں�(�����ֽ���), �������� UDP/TCP ����
** �䡡��  : ������
** ȫ�ֱ���:
** ����ģ��:
                                           API ����
*********************************************************************************************************/
LW_API
PVOID  API_INetNpfRuleAdd (CPCHAR  pcIfname,
                           INT     iRule,
                           UINT8   pucMac[],
                           CPCHAR  pcAddrStart,
                           CPCHAR  pcAddrEnd,
                           UINT16  usPortStart,
                           UINT16  usPortEnd)
{
    return  (API_INetNpfRuleAddEx(pcIfname, iRule, LW_TRUE, LW_FALSE, pucMac, 
                                  pcAddrStart, pcAddrEnd, usPortStart, usPortEnd));
}
/*********************************************************************************************************
** ��������: API_INetNpfRuleAddEx
** ��������: net packet filter ���һ������
** �䡡��  : pcIfname          ��Ӧ������ӿ���
**           iRule             ��Ӧ�Ĺ���, MAC/IP/UDP/TCP/...
**           bIn               TRUE: INPUT FALSE: OUTPUT
**           bAllow            TRUE: ���� FALSE: ��ֹ
**           pucMac            ͨ�ŵ� MAC ��ַ����,
**           pcAddrStart       ͨ�� IP ��ַ��ʼ, Ϊ IP ��ַ�ַ���, ��ʽΪ: ???.???.???.???
**           pcAddrEnd         ͨ�� IP ��ַ����, Ϊ IP ��ַ�ַ���, ��ʽΪ: ???.???.???.???
**           usPortStart       ͨ�ŵı�����ʼ�˿ں�(�����ֽ���), �������� UDP/TCP ����
**           usPortEnd         ͨ�ŵı��ؽ����˿ں�(�����ֽ���), �������� UDP/TCP ����
** �䡡��  : ������
** ȫ�ֱ���:
** ����ģ��:
                                           API ����
*********************************************************************************************************/
LW_API
PVOID  API_INetNpfRuleAddEx (CPCHAR  pcIfname,
                             INT     iRule,
                             BOOL    bIn,
                             BOOL    bAllow,
                             UINT8   pucMac[],
                             CPCHAR  pcAddrStart,
                             CPCHAR  pcAddrEnd,
                             UINT16  usPortStart,
                             UINT16  usPortEnd)
{
    __PNPF_NETIF_CB       pnpfni;
    LW_LIST_LINE_HEADER  *pplineHeader;

    if (!pcIfname) {
        _ErrorHandle(EINVAL);
        return  (LW_NULL);
    }

    if (iRule == LWIP_NPF_RULE_MAC) {                                   /*  ��� MAC ����               */
        __PNPF_RULE_MAC   pnpfrm;

        if (!pucMac) {
            _ErrorHandle(EINVAL);
            return  (LW_NULL);
        }

        pnpfrm = (__PNPF_RULE_MAC)__SHEAP_ALLOC(sizeof(__NPF_RULE_MAC));/*  ��������                    */
        if (pnpfrm == LW_NULL) {
            _DebugHandle(__ERRORMESSAGE_LEVEL, "system low memory.\r\n");
            _ErrorHandle(ERROR_SYSTEM_LOW_MEMORY);
            return  (LW_NULL);
        }
        pnpfrm->NPFRM_iRule  = iRule;
        pnpfrm->NPFRM_bAllow = bAllow;
        lib_memcpy(&pnpfrm->NPFRM_ucMac, pucMac, ETHARP_HWADDR_LEN);    /*  ���� 6 �ֽ�                 */

        __NPF_LOCK_W();                                                 /*  ���� NPF ��                 */
        pnpfni = __npfNetifCreate(pcIfname);                            /*  �������ƿ�                  */
        if (pnpfni == LW_NULL) {
            __NPF_UNLOCK();                                             /*  ���� NPF ��                 */
            return  (LW_NULL);
        }
        pplineHeader = bIn ? 
                       &pnpfni->NPFNI_npfrnIn[iRule] :
                       &pnpfni->NPFNI_npfrnOut[iRule];
        _List_Line_Add_Ahead(&pnpfrm->NPFRM_lineManage, pplineHeader);
        _G_ulNpfCounter++;
        __NPF_UNLOCK();                                                 /*  ���� NPF ��                 */

        return  ((PVOID)pnpfrm);

    } else if (iRule == LWIP_NPF_RULE_IP) {                             /*  ��� IP ����                */
        __PNPF_RULE_IP      pnpfri;

        if (!pcAddrStart || !pcAddrEnd) {
            _ErrorHandle(EINVAL);
            return  (LW_NULL);
        }

        pnpfri = (__PNPF_RULE_IP)__SHEAP_ALLOC(sizeof(__NPF_RULE_IP));  /*  ��������                    */
        if (pnpfri == LW_NULL) {
            _DebugHandle(__ERRORMESSAGE_LEVEL, "system low memory.\r\n");
            _ErrorHandle(ERROR_SYSTEM_LOW_MEMORY);
            return  (LW_NULL);
        }
        pnpfri->NPFRI_iRule  = iRule;
        pnpfri->NPFRI_bAllow = bAllow;
        
        pnpfri->NPFRI_ipaddrHboS.addr = PP_NTOHL(ipaddr_addr(pcAddrStart));
        pnpfri->NPFRI_ipaddrHboE.addr = PP_NTOHL(ipaddr_addr(pcAddrEnd));

        __NPF_LOCK_W();                                                 /*  ���� NPF ��                 */
        pnpfni = __npfNetifCreate(pcIfname);                            /*  �������ƿ�                  */
        if (pnpfni == LW_NULL) {
            __NPF_UNLOCK();                                             /*  ���� NPF ��                 */
            return  (LW_NULL);
        }
        pplineHeader = bIn ? 
                       &pnpfni->NPFNI_npfrnIn[iRule] :
                       &pnpfni->NPFNI_npfrnOut[iRule];
        _List_Line_Add_Ahead(&pnpfri->NPFRI_lineManage, pplineHeader);
        _G_ulNpfCounter++;
        __NPF_UNLOCK();                                                 /*  ���� NPF ��                 */

        return  ((PVOID)pnpfri);

    } else if (iRule == LWIP_NPF_RULE_UDP) {                            /*  ��� UDP ����               */
        __PNPF_RULE_UDP   pnpfru;

        if (!pcAddrStart || !pcAddrEnd) {
            _ErrorHandle(EINVAL);
            return  (LW_NULL);
        }

        pnpfru = (__PNPF_RULE_UDP)__SHEAP_ALLOC(sizeof(__NPF_RULE_UDP));/*  ��������                    */
        if (pnpfru == LW_NULL) {
            _DebugHandle(__ERRORMESSAGE_LEVEL, "system low memory.\r\n");
            _ErrorHandle(ERROR_SYSTEM_LOW_MEMORY);
            return  (LW_NULL);
        }
        pnpfru->NPFRU_iRule  = iRule;
        pnpfru->NPFRU_bAllow = bAllow;
        
        pnpfru->NPFRU_ipaddrHboS.addr = PP_NTOHL(ipaddr_addr(pcAddrStart));
        pnpfru->NPFRU_ipaddrHboE.addr = PP_NTOHL(ipaddr_addr(pcAddrEnd));
        pnpfru->NPFRU_usPortHboS      = PP_NTOHS(usPortStart);
        pnpfru->NPFRU_usPortHboE      = PP_NTOHS(usPortEnd);

        __NPF_LOCK_W();                                                 /*  ���� NPF ��                 */
        pnpfni = __npfNetifCreate(pcIfname);                            /*  �������ƿ�                  */
        if (pnpfni == LW_NULL) {
            __NPF_UNLOCK();                                             /*  ���� NPF ��                 */
            return  (LW_NULL);
        }
        pplineHeader = bIn ? 
                       &pnpfni->NPFNI_npfrnIn[iRule] :
                       &pnpfni->NPFNI_npfrnOut[iRule];
        _List_Line_Add_Ahead(&pnpfru->NPFRU_lineManage, pplineHeader);
        _G_ulNpfCounter++;
        __NPF_UNLOCK();                                                 /*  ���� NPF ��                 */

        return  ((PVOID)pnpfru);

    } else if (iRule == LWIP_NPF_RULE_TCP) {                            /*  ��� TCP ����               */
        __PNPF_RULE_TCP   pnpfrt;

        if (!pcAddrStart || !pcAddrEnd) {
            _ErrorHandle(EINVAL);
            return  (LW_NULL);
        }

        pnpfrt = (__PNPF_RULE_TCP)__SHEAP_ALLOC(sizeof(__NPF_RULE_TCP));/*  ��������                    */
        if (pnpfrt == LW_NULL) {
            _DebugHandle(__ERRORMESSAGE_LEVEL, "system low memory.\r\n");
            _ErrorHandle(ERROR_SYSTEM_LOW_MEMORY);
            return  (LW_NULL);
        }
        pnpfrt->NPFRT_iRule  = iRule;
        pnpfrt->NPFRT_bAllow = bAllow;
        
        pnpfrt->NPFRT_ipaddrHboS.addr = PP_NTOHL(ipaddr_addr(pcAddrStart));
        pnpfrt->NPFRT_ipaddrHboE.addr = PP_NTOHL(ipaddr_addr(pcAddrEnd));
        pnpfrt->NPFRT_usPortHboS      = PP_NTOHS(usPortStart);
        pnpfrt->NPFRT_usPortHboE      = PP_NTOHS(usPortEnd);

        __NPF_LOCK_W();                                                 /*  ���� NPF ��                 */
        pnpfni = __npfNetifCreate(pcIfname);                            /*  �������ƿ�                  */
        if (pnpfni == LW_NULL) {
            __NPF_UNLOCK();                                             /*  ���� NPF ��                 */
            return  (LW_NULL);
        }
        pplineHeader = bIn ? 
                       &pnpfni->NPFNI_npfrnIn[iRule] :
                       &pnpfni->NPFNI_npfrnOut[iRule];
        _List_Line_Add_Ahead(&pnpfrt->NPFRT_lineManage, pplineHeader);
        _G_ulNpfCounter++;
        __NPF_UNLOCK();                                                 /*  ���� NPF ��                 */

        return  ((PVOID)pnpfrt);

    } else {
        _ErrorHandle(EINVAL);
        return  (LW_NULL);
    }
}
/*********************************************************************************************************
** ��������: API_INetNpfRuleDel
** ��������: net packet filter ɾ��һ������
** �䡡��  : pcIfname          ��Ӧ������ӿ���
**           pvRule            ������ (����Ϊ NULL, Ϊ NULL ʱ��ʾʹ�ù������к�)
**           iSeqNum           ָ������ӿڵĹ������к� (�� 0 ��ʼ)
** �䡡��  : ERROR_NONE or PX_ERROR
** ȫ�ֱ���:
** ����ģ��:
                                           API ����
*********************************************************************************************************/
LW_API
INT  API_INetNpfRuleDel (CPCHAR  pcIfname,
                         PVOID   pvRule,
                         INT     iSeqNum)
{
    return  (API_INetNpfRuleDelEx(pcIfname, LW_TRUE, pvRule, iSeqNum));
}
/*********************************************************************************************************
** ��������: API_INetNpfRuleDelEx
** ��������: net packet filter ɾ��һ������
** �䡡��  : pcIfname          ��Ӧ������ӿ���
**           bIn               TRUE: INPUT FALSE: OUTPUT
**           pvRule            ������ (����Ϊ NULL, Ϊ NULL ʱ��ʾʹ�ù������к�)
**           iSeqNum           ָ������ӿڵĹ������к� (�� 0 ��ʼ)
** �䡡��  : ERROR_NONE or PX_ERROR
** ȫ�ֱ���:
** ����ģ��:
                                           API ����
*********************************************************************************************************/
LW_API
INT  API_INetNpfRuleDelEx (CPCHAR  pcIfname,
                           BOOL    bIn,
                           PVOID   pvRule,
                           INT     iSeqNum)
{
    INT                    i;
    __PNPF_NETIF_CB        pnpfni;
    __PNPF_RULE_MAC        pnpfrm;

    if (!pcIfname) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }

    if (!pvRule && (iSeqNum < 0)) {                                     /*  ��������������ͬʱ��Ч      */
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }

    __NPF_LOCK_W();                                                     /*  ���� NPF ��                 */
    pnpfni = __npfNetifFind(pcIfname);
    if (pnpfni == LW_NULL) {
        __NPF_UNLOCK();                                                 /*  ���� NPF ��                 */
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }

    if (pvRule) {                                                       /*  ͨ�����ɾ��                */
        pnpfrm = _LIST_ENTRY(pvRule, __NPF_RULE_MAC, NPFRM_lineManage);
        if (bIn) {
            _List_Line_Del(&pnpfrm->NPFRM_lineManage, 
                           &pnpfni->NPFNI_npfrnIn[pnpfrm->NPFRM_iRule]);
        } else {
            _List_Line_Del(&pnpfrm->NPFRM_lineManage, 
                           &pnpfni->NPFNI_npfrnOut[pnpfrm->NPFRM_iRule]);
        }
        _G_ulNpfCounter--;
        
    } else {                                                            /*  ͨ�����ɾ��                */
        PLW_LIST_LINE   plineTemp;

        for (i = 0; i < __NPF_NETIF_RULE_MAX; i++) {
            plineTemp = bIn ? pnpfni->NPFNI_npfrnIn[i] : pnpfni->NPFNI_npfrnOut[i];
            while (plineTemp) {
                if (iSeqNum == 0) {
                    goto    __rule_find;
                }
                iSeqNum--;
                plineTemp = _list_line_get_next(plineTemp);
            }
        }

__rule_find:
        if (iSeqNum || (plineTemp == LW_NULL)) {
            __NPF_UNLOCK();                                             /*  ���� NPF ��                 */
            _ErrorHandle(EINVAL);                                       /*  iSeqNum ��������            */
            return  (PX_ERROR);
        }
        if (bIn) {
            _List_Line_Del(plineTemp, &pnpfni->NPFNI_npfrnIn[i]);
        } else {
            _List_Line_Del(plineTemp, &pnpfni->NPFNI_npfrnOut[i]);
        }
        _G_ulNpfCounter--;
        pvRule = plineTemp;
    }

    /*
     *  ��� pnpfni ��û���κι���, ͬʱҲû���κ���������. pnpfni ���Ա�ɾ��.
     */
    if (!pnpfni->NPFNI_bAttached) {                                     /*  û�����ӵ�����              */
        INT     i;

        for (i = 0; i < __NPF_NETIF_RULE_MAX; i++) {
            if (pnpfni->NPFNI_npfrnIn[i] ||
                pnpfni->NPFNI_npfrnOut[i]) {
                break;
            }
        }

        if (i >= __NPF_NETIF_RULE_MAX) {                                /*  ������ӿڿ��ƽ��û�й���  */
            __npfNetifDelete(pnpfni);
        }
    }
    __NPF_UNLOCK();                                                     /*  ���� NPF ��                 */

    __SHEAP_FREE(pvRule);                                               /*  �ͷ��ڴ�                    */

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: API_INetNpfInit
** ��������: net packet filter ��ʼ��
** �䡡��  : NONE
** �䡡��  : ERROR_NONE or PX_ERROR
** ȫ�ֱ���:
** ����ģ��:
                                           API ����
*********************************************************************************************************/
LW_API
INT  API_INetNpfInit (VOID)
{
    static BOOL bIsInit = LW_FALSE;

    if (bIsInit) {
        return  (ERROR_NONE);
    }

    _G_ulNpfLock = API_SemaphoreRWCreate("sem_npflcok", LW_OPTION_DELETE_SAFE |
                                         LW_OPTION_WAIT_FIFO | LW_OPTION_OBJECT_GLOBAL, LW_NULL);
    if (_G_ulNpfLock == LW_OBJECT_HANDLE_INVALID) {
        return  (PX_ERROR);
    }

#if LW_CFG_SHELL_EN > 0
    /*
     *  ���� SHELL ����.
     */
    API_TShellKeywordAdd("npfs", __tshellNetNpfShow);
    API_TShellHelpAdd("npfs",    "show net packet filter rule(s).\n");

    API_TShellKeywordAdd("npfruleadd", __tshellNetNpfRuleAdd);
    API_TShellFormatAdd("npfruleadd",  " [netifname] [rule] [input | output] [allow | deny] [args...]");
    API_TShellHelpAdd("npfruleadd",    "add a rule into net packet filter.\n"
                                       "eg. npfruleadd en1 mac input deny 11:22:33:44:55:66\n"
                                       "    npfruleadd en1 ip input allow 192.168.0.5 192.168.0.10\n"
                                       "    npfruleadd lo0 udp input deny 0.0.0.0 255.255.255.255 433 500\n"
                                       "    npfruleadd wl2 tcp input deny 192.168.0.1 192.168.0.200 169 169\n");

    API_TShellKeywordAdd("npfruledel", __tshellNetNpfRuleDel);
    API_TShellFormatAdd("npfruledel",  " [netifname] [input | output] [rule sequence num]");
    API_TShellHelpAdd("npfruledel",    "delete a rule from net packet filter.\n");
#endif                                                                  /*  LW_CFG_SHELL_EN > 0         */

#if LW_CFG_PROCFS_EN > 0
    __procFsNpfInit();
#endif                                                                  /*  LW_CFG_PROCFS_EN > 0        */

    bIsInit = LW_TRUE;

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: API_INetNpfDropGet
** ��������: net packet filter ���������ݰ����� (���������Թ��˺ͻ��治����ɵĶ���)
** �䡡��  : NONE
** �䡡��  : ���������ݰ�����
** ȫ�ֱ���:
** ����ģ��:
                                           API ����
*********************************************************************************************************/
LW_API
ULONG  API_INetNpfDropGet (VOID)
{
    return  (__NPF_PACKET_DROP_GET());
}
/*********************************************************************************************************
** ��������: API_INetNpfAllowGet
** ��������: net packet filter ����ͨ�������ݰ�����
** �䡡��  : NONE
** �䡡��  : ����ͨ�������ݰ�����
** ȫ�ֱ���:
** ����ģ��:
                                           API ����
*********************************************************************************************************/
LW_API
ULONG  API_INetNpfAllowGet (VOID)
{
    return  (__NPF_PACKET_ALLOW_GET());
}
/*********************************************************************************************************
** ��������: API_INetNpfShow
** ��������: net packet filter ��ʾ��ǰ���еĹ�����Ŀ
** �䡡��  : iFd           ��ӡĿ���ļ�������
** �䡡��  : ERROR_NONE or PX_ERROR
** ȫ�ֱ���:
** ����ģ��:
                                           API ����
*********************************************************************************************************/
LW_API
INT  API_INetNpfShow (INT  iFd)
{
    INT     iFilterFd;
    CHAR    cBuffer[512];
    ssize_t sstNum;
    
    iFilterFd = open("/proc/net/netfilter", O_RDONLY);
    if (iFilterFd < 0) {
        fprintf(stderr, "can not open /proc/net/netfilter : %s\n", lib_strerror(errno));
        return  (PX_ERROR);
    }
    
    do {
        sstNum = read(iFilterFd, cBuffer, sizeof(cBuffer));
        if (sstNum > 0) {
            write(iFd, cBuffer, (size_t)sstNum);
        }
    } while (sstNum > 0);
    
    close(iFilterFd);

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: __tshellNetNpfShow
** ��������: ϵͳ���� "npfs"
** �䡡��  : iArgC         ��������
**           ppcArgV       ������
** �䡡��  : 0
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
#if LW_CFG_SHELL_EN > 0

static INT  __tshellNetNpfShow (INT  iArgC, PCHAR  *ppcArgV)
{
    return  (API_INetNpfShow(STD_OUT));
}
/*********************************************************************************************************
** ��������: __tshellNetNpfRuleAdd
** ��������: ϵͳ���� "npfruleadd"
** �䡡��  : iArgC         ��������
**           ppcArgV       ������
** �䡡��  : 0
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  __tshellNetNpfRuleAdd (INT  iArgC, PCHAR  *ppcArgV)
{
#define __NPF_TSHELL_RADD_ARG_NETIF     1
#define __NPF_TSHELL_RADD_ARG_RULE      2
#define __NPF_TSHELL_RADD_ARG_IN        3
#define __NPF_TSHELL_RADD_ARG_ALLOW     4
#define __NPF_TSHELL_RADD_ARG_MAC       5
#define __NPF_TSHELL_RADD_ARG_IPS       5
#define __NPF_TSHELL_RADD_ARG_IPE       6
#define __NPF_TSHELL_RADD_ARG_PORTS     7
#define __NPF_TSHELL_RADD_ARG_PORTE     8
    PVOID    pvRule;
    BOOL     bIn;
    BOOL     bAllow;

    if (iArgC < 6) {
        fprintf(stderr, "arguments error!\n");
        return  (-ERROR_TSHELL_EPARAM);
    }
    
    if (!lib_strcmp(ppcArgV[__NPF_TSHELL_RADD_ARG_IN], "input")) {
        bIn = LW_TRUE;
    } else {
        bIn = LW_FALSE;
    }
    
    if (!lib_strcmp(ppcArgV[__NPF_TSHELL_RADD_ARG_ALLOW], "allow")) {
        bAllow = LW_TRUE;
    } else {
        bAllow = LW_FALSE;
    }

    if (lib_strcmp(ppcArgV[__NPF_TSHELL_RADD_ARG_RULE], "mac") == 0) {
        INT         i;
        UINT8       ucMac[NETIF_MAX_HWADDR_LEN];
        INT         iMac[NETIF_MAX_HWADDR_LEN];

        if (sscanf(ppcArgV[__NPF_TSHELL_RADD_ARG_MAC], "%x:%x:%x:%x:%x:%x",
                   &iMac[0], &iMac[1], &iMac[2], &iMac[3], &iMac[4], &iMac[5]) != 6) {
            fprintf(stderr, "mac argument error!\n");
            return  (-ERROR_TSHELL_EPARAM);
        }
        for (i = 0; i < ETHARP_HWADDR_LEN; i++) {
            ucMac[i] = (UINT8)iMac[i];
        }
        pvRule = API_INetNpfRuleAddEx(ppcArgV[__NPF_TSHELL_RADD_ARG_NETIF],
                                      LWIP_NPF_RULE_MAC, bIn, bAllow,
                                      ucMac, LW_NULL, LW_NULL, 0, 0);
        if (pvRule == LW_NULL) {
            fprintf(stderr, "can not add mac rule, error: %s\n", lib_strerror(errno));
            return  (PX_ERROR);
        }

    } else if (lib_strcmp(ppcArgV[__NPF_TSHELL_RADD_ARG_RULE], "ip") == 0) {
        if (iArgC != 7) {
            fprintf(stderr, "arguments error!\n");
            return  (-ERROR_TSHELL_EPARAM);
        }
        pvRule = API_INetNpfRuleAddEx(ppcArgV[__NPF_TSHELL_RADD_ARG_NETIF],
                                      LWIP_NPF_RULE_IP, bIn, bAllow,
                                      LW_NULL,
                                      ppcArgV[__NPF_TSHELL_RADD_ARG_IPS],
                                      ppcArgV[__NPF_TSHELL_RADD_ARG_IPE], 0, 0);
        if (pvRule == LW_NULL) {
            fprintf(stderr, "can not add ip rule, error: %s\n", lib_strerror(errno));
            return  (PX_ERROR);
        }

    } else if (lib_strcmp(ppcArgV[__NPF_TSHELL_RADD_ARG_RULE], "udp") == 0) {
        INT     iPortS = -1;
        INT     iPortE = -1;

        if (iArgC != 9) {
            fprintf(stderr, "arguments error!\n");
            return  (-ERROR_TSHELL_EPARAM);
        }
        if (sscanf(ppcArgV[__NPF_TSHELL_RADD_ARG_PORTS], "%i", &iPortS) != 1) {
            fprintf(stderr, "port argument error!\n");
            return  (-ERROR_TSHELL_EPARAM);
        }
        if (sscanf(ppcArgV[__NPF_TSHELL_RADD_ARG_PORTE], "%i", &iPortE) != 1) {
            fprintf(stderr, "port argument error!\n");
            return  (-ERROR_TSHELL_EPARAM);
        }
        
        pvRule = API_INetNpfRuleAddEx(ppcArgV[__NPF_TSHELL_RADD_ARG_NETIF],
                                      LWIP_NPF_RULE_UDP, bIn, bAllow,
                                      LW_NULL,
                                      ppcArgV[__NPF_TSHELL_RADD_ARG_IPS],
                                      ppcArgV[__NPF_TSHELL_RADD_ARG_IPE],
                                      htons((u16_t)iPortS),
                                      htons((u16_t)iPortE));
        if (pvRule == LW_NULL) {
            fprintf(stderr, "can not add udp rule, error: %s\n", lib_strerror(errno));
            return  (PX_ERROR);
        }

    } else if (lib_strcmp(ppcArgV[__NPF_TSHELL_RADD_ARG_RULE], "tcp") == 0) {
        INT     iPortS = -1;
        INT     iPortE = -1;

        if (iArgC != 9) {
            fprintf(stderr, "arguments error!\n");
            return  (-ERROR_TSHELL_EPARAM);
        }
        if (sscanf(ppcArgV[__NPF_TSHELL_RADD_ARG_PORTS], "%i", &iPortS) != 1) {
            fprintf(stderr, "port argument error!\n");
            return  (-ERROR_TSHELL_EPARAM);
        }
        if (sscanf(ppcArgV[__NPF_TSHELL_RADD_ARG_PORTE], "%i", &iPortE) != 1) {
            fprintf(stderr, "port argument error!\n");
            return  (-ERROR_TSHELL_EPARAM);
        }
        
        pvRule = API_INetNpfRuleAddEx(ppcArgV[__NPF_TSHELL_RADD_ARG_NETIF],
                                      LWIP_NPF_RULE_TCP, bIn, bAllow,
                                      LW_NULL,
                                      ppcArgV[__NPF_TSHELL_RADD_ARG_IPS],
                                      ppcArgV[__NPF_TSHELL_RADD_ARG_IPE],
                                      htons((u16_t)iPortS),
                                      htons((u16_t)iPortE));
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
** ��������: __tshellNetNpfRuleDel
** ��������: ϵͳ���� "npfruledel"
** �䡡��  : iArgC         ��������
**           ppcArgV       ������
** �䡡��  : 0
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  __tshellNetNpfRuleDel (INT  iArgC, PCHAR  *ppcArgV)
{
    BOOL    bIn;
    INT     iError;
    INT     iSeqNum = -1;

    if (iArgC != 4) {
        fprintf(stderr, "arguments error!\n");
        return  (-ERROR_TSHELL_EPARAM);
    }
    
    if (!lib_strcmp(ppcArgV[2], "input")) {
        bIn = LW_TRUE;
    } else {
        bIn = LW_FALSE;
    }

    if (sscanf(ppcArgV[3], "%i", &iSeqNum) != 1) {
        fprintf(stderr, "arguments error!\n");
        return  (-ERROR_TSHELL_EPARAM);
    }

    iError = API_INetNpfRuleDelEx(ppcArgV[1], bIn, LW_NULL, iSeqNum);
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
  /proc/net/netfilter
*********************************************************************************************************/
#if LW_CFG_PROCFS_EN > 0
/*********************************************************************************************************
  ���� proc �ļ���������
*********************************************************************************************************/
static ssize_t  __procFsNetFilterRead(PLW_PROCFS_NODE  p_pfsn, 
                                      PCHAR            pcBuffer, 
                                      size_t           stMaxBytes,
                                      off_t            oft);
/*********************************************************************************************************
  ���� proc �ļ�����������
*********************************************************************************************************/
static LW_PROCFS_NODE_OP    _G_pfsnoNetFilterFuncs = {
    __procFsNetFilterRead, LW_NULL
};
/*********************************************************************************************************
  ���� proc �ļ��ڵ�
*********************************************************************************************************/
static LW_PROCFS_NODE       _G_pfsnNetFilter[] = 
{
    LW_PROCFS_INIT_NODE("netfilter", 
                        (S_IFREG | S_IRUSR | S_IRGRP | S_IROTH), 
                        &_G_pfsnoNetFilterFuncs, 
                        "F",
                        0),
};
/*********************************************************************************************************
** ��������: __procFsNetFilterPrint
** ��������: ��ӡ���� netfilter �ļ�
** �䡡��  : bIn           TRUE: INPUT FALSE: OUTPUT
**           pcBuffer      ����
**           stTotalSize   ��������С
**           pstOft        ��ǰƫ����
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static VOID  __procFsNetFilterPrint (BOOL  bIn, PCHAR  pcBuffer, size_t  stTotalSize, size_t *pstOft)
{
    INT              i, iSeqNum;
    CHAR             cIpBuffer1[INET_ADDRSTRLEN];
    CHAR             cIpBuffer2[INET_ADDRSTRLEN];

    __PNPF_NETIF_CB  pnpfni;
    PLW_LIST_LINE    plineTempNpfni;
    PLW_LIST_LINE    plineTemp;
    
    for (i = 0; i < __NPF_NETIF_HASH_SIZE; i++) {
        for (plineTempNpfni  = _G_plineNpfHash[i];
             plineTempNpfni != LW_NULL;
             plineTempNpfni  = _list_line_get_next(plineTempNpfni)) {

            pnpfni  = _LIST_ENTRY(plineTempNpfni, __NPF_NETIF_CB, NPFNI_lineHash);
            iSeqNum = 0;

            plineTemp = bIn ? 
                        pnpfni->NPFNI_npfrnIn[LWIP_NPF_RULE_MAC] :
                        pnpfni->NPFNI_npfrnOut[LWIP_NPF_RULE_MAC];
            while (plineTemp) {
                __PNPF_RULE_MAC  pnpfrm;

                pnpfrm = _LIST_ENTRY(plineTemp, __NPF_RULE_MAC, NPFRM_lineManage);

                *pstOft = bnprintf(pcBuffer, stTotalSize, *pstOft, 
                         "%-5s %-6s %6d %-4s %-5s "
                         "%02x:%02x:%02x:%02x:%02x:%02x %-15s %-15s %-6s %-6s\n",
                         pnpfni->NPFNI_cName, (pnpfni->NPFNI_bAttached) ? "YES" : "NO",
                         iSeqNum, "MAC", (pnpfrm->NPFRM_bAllow) ? "YES" : "NO",
                         pnpfrm->NPFRM_ucMac[0],
                         pnpfrm->NPFRM_ucMac[1],
                         pnpfrm->NPFRM_ucMac[2],
                         pnpfrm->NPFRM_ucMac[3],
                         pnpfrm->NPFRM_ucMac[4],
                         pnpfrm->NPFRM_ucMac[5],
                         "N/A", "N/A", "N/A", "N/A");
                iSeqNum++;
                plineTemp = _list_line_get_next(plineTemp);
            }

            plineTemp = bIn ? 
                        pnpfni->NPFNI_npfrnIn[LWIP_NPF_RULE_IP] :
                        pnpfni->NPFNI_npfrnOut[LWIP_NPF_RULE_IP];
            while (plineTemp) {
                __PNPF_RULE_IP  pnpfri;
                ip4_addr_t      ipaddrS, ipaddrE;

                pnpfri = _LIST_ENTRY(plineTemp, __NPF_RULE_IP, NPFRI_lineManage);
                
                ipaddrS.addr = PP_HTONL(pnpfri->NPFRI_ipaddrHboS.addr);
                ipaddrE.addr = PP_HTONL(pnpfri->NPFRI_ipaddrHboE.addr);
                
                *pstOft = bnprintf(pcBuffer, stTotalSize, *pstOft, 
                         "%-5s %-6s %6d %-4s %-5s %-17s %-15s %-15s %-6s %-6s\n",
                         pnpfni->NPFNI_cName, (pnpfni->NPFNI_bAttached) ? "YES" : "NO",
                         iSeqNum, "IP", (pnpfri->NPFRI_bAllow) ? "YES" : "NO", "N/A",
                         ip4addr_ntoa_r(&ipaddrS, cIpBuffer1, INET_ADDRSTRLEN),
                         ip4addr_ntoa_r(&ipaddrE, cIpBuffer2, INET_ADDRSTRLEN),
                         "N/A", "N/A");
                iSeqNum++;
                plineTemp = _list_line_get_next(plineTemp);
            }

            plineTemp = bIn ? 
                        pnpfni->NPFNI_npfrnIn[LWIP_NPF_RULE_UDP] :
                        pnpfni->NPFNI_npfrnOut[LWIP_NPF_RULE_UDP];
            while (plineTemp) {
                __PNPF_RULE_UDP  pnpfru;
                ip4_addr_t       ipaddrS, ipaddrE;

                pnpfru = _LIST_ENTRY(plineTemp, __NPF_RULE_UDP, NPFRU_lineManage);
                
                ipaddrS.addr = PP_HTONL(pnpfru->NPFRU_ipaddrHboS.addr);
                ipaddrE.addr = PP_HTONL(pnpfru->NPFRU_ipaddrHboE.addr);

                *pstOft = bnprintf(pcBuffer, stTotalSize, *pstOft, 
                         "%-5s %-6s %6d %-4s %-5s %-17s %-15s %-15s %-6d %-6d\n",
                         pnpfni->NPFNI_cName, (pnpfni->NPFNI_bAttached) ? "YES" : "NO",
                         iSeqNum, "UDP", (pnpfru->NPFRU_bAllow) ? "YES" : "NO", "N/A",
                         ip4addr_ntoa_r(&ipaddrS, cIpBuffer1, INET_ADDRSTRLEN),
                         ip4addr_ntoa_r(&ipaddrE, cIpBuffer2, INET_ADDRSTRLEN),
                         pnpfru->NPFRU_usPortHboS,
                         pnpfru->NPFRU_usPortHboE);
                iSeqNum++;
                plineTemp = _list_line_get_next(plineTemp);
            }

            plineTemp = bIn ? 
                        pnpfni->NPFNI_npfrnIn[LWIP_NPF_RULE_TCP] :
                        pnpfni->NPFNI_npfrnOut[LWIP_NPF_RULE_TCP];
            while (plineTemp) {
                __PNPF_RULE_TCP  pnpfrt;
                ip4_addr_t       ipaddrS, ipaddrE;

                pnpfrt = _LIST_ENTRY(plineTemp, __NPF_RULE_TCP, NPFRT_lineManage);
                
                ipaddrS.addr = PP_HTONL(pnpfrt->NPFRT_ipaddrHboS.addr);
                ipaddrE.addr = PP_HTONL(pnpfrt->NPFRT_ipaddrHboE.addr);

                *pstOft = bnprintf(pcBuffer, stTotalSize, *pstOft, 
                         "%-5s %-6s %6d %-4s %-5s %-17s %-15s %-15s %-6d %-6d\n",
                         pnpfni->NPFNI_cName, (pnpfni->NPFNI_bAttached) ? "YES" : "NO",
                         iSeqNum, "TCP", (pnpfrt->NPFRT_bAllow) ? "YES" : "NO", "N/A",
                         ip4addr_ntoa_r(&ipaddrS, cIpBuffer1, INET_ADDRSTRLEN),
                         ip4addr_ntoa_r(&ipaddrE, cIpBuffer2, INET_ADDRSTRLEN),
                         pnpfrt->NPFRT_usPortHboS,
                         pnpfrt->NPFRT_usPortHboE);
                iSeqNum++;
                plineTemp = _list_line_get_next(plineTemp);
            }
        }
    }
    
    *pstOft = bnprintf(pcBuffer, stTotalSize, *pstOft, "\n");
}
/*********************************************************************************************************
** ��������: __procFsNetFilterRead
** ��������: procfs ��һ����ȡ���� netfilter �ļ�
** �䡡��  : p_pfsn        �ڵ���ƿ�
**           pcBuffer      ������
**           stMaxBytes    ��������С
**           oft           �ļ�ָ��
** �䡡��  : ʵ�ʶ�ȡ����Ŀ
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static ssize_t  __procFsNetFilterRead (PLW_PROCFS_NODE  p_pfsn, 
                                       PCHAR            pcBuffer, 
                                       size_t           stMaxBytes,
                                       off_t            oft)
{
    const CHAR      cFilterInfoHdr[] = 
    "NETIF ATTACH SEQNUM RULE ALLOW MAC               IPs             IPe             PORTs  PORTe\n";
    
          PCHAR     pcFileBuffer;
          size_t    stRealSize;                                         /*  ʵ�ʵ��ļ����ݴ�С          */
          size_t    stCopeBytes;
    
    /*
     *  ����Ԥ���ڴ��СΪ 0 , ���Դ򿪺��һ�ζ�ȡ��Ҫ�ֶ������ڴ�.
     */
    pcFileBuffer = (PCHAR)API_ProcFsNodeBuffer(p_pfsn);
    if (pcFileBuffer == LW_NULL) {                                      /*  ��û�з����ڴ�              */
        size_t  stNeedBufferSize = 0;
        
        __NPF_LOCK_R();                                                 /*  ���� NPF ��                 */
        stNeedBufferSize = (size_t)(_G_ulNpfCounter * 128);
        __NPF_UNLOCK();                                                 /*  ���� NPF ��                 */
        
        stNeedBufferSize += (sizeof(cFilterInfoHdr) * 2) + 64;
        
        if (API_ProcFsAllocNodeBuffer(p_pfsn, stNeedBufferSize)) {
            _ErrorHandle(ENOMEM);
            return  (0);
        }
        pcFileBuffer = (PCHAR)API_ProcFsNodeBuffer(p_pfsn);             /*  ���»���ļ���������ַ      */
        
        stRealSize = bnprintf(pcFileBuffer, stNeedBufferSize, 0, "input >>\n\n");
        stRealSize = bnprintf(pcFileBuffer, stNeedBufferSize, stRealSize, cFilterInfoHdr);
                                                                        /*  ��ӡͷ��Ϣ                  */
        __NPF_LOCK_R();                                                 /*  ���� NPF ��                 */
        __procFsNetFilterPrint(LW_TRUE, pcFileBuffer, stNeedBufferSize, &stRealSize);
        __NPF_UNLOCK();                                                 /*  ���� NPF ��                 */
        
        stRealSize = bnprintf(pcFileBuffer, stNeedBufferSize, stRealSize, "output >>\n\n");
        stRealSize = bnprintf(pcFileBuffer, stNeedBufferSize, stRealSize, cFilterInfoHdr);
                                                                        /*  ��ӡͷ��Ϣ                  */
        __NPF_LOCK_R();                                                 /*  ���� NPF ��                 */
        __procFsNetFilterPrint(LW_FALSE, pcFileBuffer, stNeedBufferSize, &stRealSize);
        __NPF_UNLOCK();                                                 /*  ���� NPF ��                 */
        
        stRealSize = bnprintf(pcFileBuffer, stNeedBufferSize, stRealSize, 
                              "drop:%d  allow:%d\n", 
                              __NPF_PACKET_DROP_GET(), __NPF_PACKET_ALLOW_GET());
        
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
** ��������: __procFsNpfInit
** ��������: procfs ��ʼ������ netfilter �ļ�
** �䡡��  : NONE
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
VOID  __procFsNpfInit (VOID)
{
    API_ProcFsMakeNode(&_G_pfsnNetFilter[0],  "/net");
}

#endif                                                                  /*  LW_CFG_PROCFS_EN > 0        */
#endif                                                                  /*  LW_CFG_NET_EN > 0           */
                                                                        /*  LW_CFG_NET_NPF_EN > 0       */
/*********************************************************************************************************
  END
*********************************************************************************************************/
