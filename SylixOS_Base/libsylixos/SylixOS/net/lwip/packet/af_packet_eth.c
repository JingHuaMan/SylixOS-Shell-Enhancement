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
** ��   ��   ��: af_packet_eth.c
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2014 �� 03 �� 21 ��
**
** ��        ��: AF_PACKET ��̫������֧��.
**
** BUG:
2015.04.16  ���� __packetEthRawSendto() �ڴ�й¶����.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
#include "../SylixOS/system/include/s_system.h"
/*********************************************************************************************************
  �ü�����
*********************************************************************************************************/
#if LW_CFG_NET_EN > 0
#include "sys/socket.h"
#include "net/if_arp.h"
#include "net/if_ether.h"
#include "netif/etharp.h"
#include "lwip/pbuf.h"
#include "lwip/tcpip.h"
#include "af_packet.h"
/*********************************************************************************************************
** ��������: __packetEthRawSendto
** ��������: ����һ��ԭʼ ETH PACKET
** �䡡��  : pafpacket      afpacket file
**           pvPacket       ���ݰ�
**           stBytes        ���ݰ�����
**           psockaddrll    ��·��ַ��Ϣ
** �䡡��  : ������
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
errno_t  __packetEthRawSendto (AF_PACKET_T          *pafpacket,
                               CPVOID                pvPacket, 
                               size_t                stBytes, 
                               struct sockaddr_ll   *psockaddrll)
{
    struct netif *pnetif;
    struct pbuf  *pbuf_hdr;
    struct pbuf  *pbuf_dat;
           err_t  err;
    
    if ((stBytes < ETH_HLEN) || (stBytes > ETH_FRAME_LEN)) {
        return  (EMSGSIZE);
    }
    
    LOCK_TCPIP_CORE();
    if (psockaddrll) {
        pnetif = netif_get_by_index(psockaddrll->sll_ifindex);
    } else {
        pnetif = netif_get_by_index(pafpacket->PACKET_iIfIndex);
    }
    if (pnetif == LW_NULL) {
        UNLOCK_TCPIP_CORE();
        return  (ENODEV);
    }
    if (!(pnetif->flags & (NETIF_FLAG_ETHARP | NETIF_FLAG_ETHERNET))) {
        UNLOCK_TCPIP_CORE();
        return  (ENXIO);
    }
    if (!netif_is_up(pnetif) || !netif_is_link_up(pnetif)) {
        UNLOCK_TCPIP_CORE();
        return  (ENETDOWN);
    }
    
#if ETH_PAD_SIZE
    pbuf_hdr = pbuf_alloc(PBUF_RAW, ETH_HLEN + ETH_PAD_SIZE, PBUF_RAM); /*  ������� PAD �ı�ͷ         */
    if (pbuf_hdr == LW_NULL) {
        UNLOCK_TCPIP_CORE();
        return  (ENOMEM);
    }
    lib_memcpy(((u8_t *)pbuf_hdr->payload) + ETH_PAD_SIZE, pvPacket, ETH_HLEN);
    
    pbuf_dat = pbuf_alloc(PBUF_RAW, (u16_t)(stBytes - ETH_HLEN), PBUF_REF);
    if (pbuf_dat == LW_NULL) {                                          /*  �������� pbuf               */
        pbuf_free(pbuf_hdr);
        UNLOCK_TCPIP_CORE();
        return  (ENOMEM);
    }
    pbuf_dat->payload = (void *)((u8_t *)pvPacket + ETH_HLEN);
    
    pbuf_cat(pbuf_hdr, pbuf_dat);
    
#else
    (VOID)pbuf_dat;

    pbuf_hdr = pbuf_alloc(PBUF_RAW, (u16_t)stBytes, PBUF_REF);          /*  ֱ�����ü���                */
    if (pbuf_hdr == LW_NULL) {
        UNLOCK_TCPIP_CORE();
        return  (ENOMEM);
    }
    pbuf_hdr->payload = (void *)pvPacket;
#endif
    
    err = pnetif->linkoutput(pnetif, pbuf_hdr);
    UNLOCK_TCPIP_CORE();
    
    pbuf_free(pbuf_hdr);

    if (err) {
        return  (EIO);
    
    } else {
        return  (ERROR_NONE);
    }
}
/*********************************************************************************************************
** ��������: __packetEthDgramSendto
** ��������: ����һ�������б�ͷ�� ETH PACKET
** �䡡��  : pafpacket      afpacket file
**           pvPacket       ���ݰ�
**           stBytes        ���ݰ�����
**           psockaddrll    ��·��ַ��Ϣ
** �䡡��  : ������
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
errno_t  __packetEthDgramSendto (AF_PACKET_T          *pafpacket,
                                 CPVOID                pvPacket, 
                                 size_t                stBytes, 
                                 struct sockaddr_ll   *psockaddrll)
{
    struct netif   *pnetif;
    struct pbuf    *pbuf_hdr;
    struct pbuf    *pbuf_dat;
    struct eth_hdr *pethhdr;
           err_t    err;
    
    if (stBytes > ETH_DATA_LEN) {
        return  (EMSGSIZE);
    }
    
    LOCK_TCPIP_CORE();
    pnetif = netif_get_by_index(psockaddrll->sll_ifindex);
    if (pnetif == LW_NULL) {
        UNLOCK_TCPIP_CORE();
        return  (ENODEV);
    }
    if (!(pnetif->flags & (NETIF_FLAG_ETHARP | NETIF_FLAG_ETHERNET))) {
        UNLOCK_TCPIP_CORE();
        return  (ENXIO);
    }
    if (!netif_is_up(pnetif) || !netif_is_link_up(pnetif)) {
        UNLOCK_TCPIP_CORE();
        return  (ENETDOWN);
    }
    
    pbuf_hdr = pbuf_alloc(PBUF_RAW, ETH_HLEN + ETH_PAD_SIZE, PBUF_RAM); /*  ������� PAD �ı�ͷ         */
    if (pbuf_hdr == LW_NULL) {
        UNLOCK_TCPIP_CORE();
        return  (ENOMEM);
    }
    
    pethhdr = (struct eth_hdr *)pbuf_hdr->payload;
    pethhdr->dest.addr[0] = psockaddrll->sll_addr[0];
    pethhdr->dest.addr[1] = psockaddrll->sll_addr[1];
    pethhdr->dest.addr[2] = psockaddrll->sll_addr[2];
    pethhdr->dest.addr[3] = psockaddrll->sll_addr[3];
    pethhdr->dest.addr[4] = psockaddrll->sll_addr[4];
    pethhdr->dest.addr[5] = psockaddrll->sll_addr[5];
    
    pethhdr->src.addr[0] = pnetif->hwaddr[0];
    pethhdr->src.addr[1] = pnetif->hwaddr[1];
    pethhdr->src.addr[2] = pnetif->hwaddr[2];
    pethhdr->src.addr[3] = pnetif->hwaddr[3];
    pethhdr->src.addr[4] = pnetif->hwaddr[4];
    pethhdr->src.addr[5] = pnetif->hwaddr[5];
    
    pethhdr->type = psockaddrll->sll_protocol;
    
    pbuf_dat = pbuf_alloc(PBUF_RAW, (u16_t)stBytes, PBUF_REF);
    if (pbuf_dat == LW_NULL) {                                          /*  �������� pbuf               */
        pbuf_free(pbuf_hdr);
        UNLOCK_TCPIP_CORE();
        return  (ENOMEM);
    }
    
    pbuf_dat->payload = (void *)pvPacket;
    
    pbuf_cat(pbuf_hdr, pbuf_dat);
    
    err = pnetif->linkoutput(pnetif, pbuf_hdr);
    UNLOCK_TCPIP_CORE();
    
    pbuf_free(pbuf_hdr);

    if (err) {
        return  (EIO);
    
    } else {
        return  (ERROR_NONE);
    }
}
/*********************************************************************************************************
** ��������: __packetEthHeaderInfo
** ��������: ETH PACKET ��ַ��Ϣ
** �䡡��  : pktm           ������Ϣ
**           psockaddrll    ��·��ַ��Ϣ
** �䡡��  : ����ͷ����
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
size_t  __packetEthHeaderInfo (AF_PACKET_N  *pktm, struct sockaddr_ll *paddrll)
{
    struct eth_hdr  *pethhdr = (struct eth_hdr *)pktm->PKTM_p->payload;
    
    if (paddrll) {
        paddrll->sll_len      = sizeof(struct sockaddr_ll);
        paddrll->sll_family   = AF_PACKET;
        paddrll->sll_protocol = pethhdr->type;
        paddrll->sll_ifindex  = pktm->PKTM_ucIndex;
        paddrll->sll_hatype   = ARPHRD_ETHER;
        paddrll->sll_halen    = ETHARP_HWADDR_LEN;
        
        if (pktm->PKTM_ucOutgo) {
            paddrll->sll_addr[0] = pethhdr->dest.addr[0];
            paddrll->sll_addr[1] = pethhdr->dest.addr[1];
            paddrll->sll_addr[2] = pethhdr->dest.addr[2];
            paddrll->sll_addr[3] = pethhdr->dest.addr[3];
            paddrll->sll_addr[4] = pethhdr->dest.addr[4];
            paddrll->sll_addr[5] = pethhdr->dest.addr[5];
        
        } else {
            paddrll->sll_addr[0] = pethhdr->src.addr[0];
            paddrll->sll_addr[1] = pethhdr->src.addr[1];
            paddrll->sll_addr[2] = pethhdr->src.addr[2];
            paddrll->sll_addr[3] = pethhdr->src.addr[3];
            paddrll->sll_addr[4] = pethhdr->src.addr[4];
            paddrll->sll_addr[5] = pethhdr->src.addr[5];
        }
        
        if (pktm->PKTM_ucOutgo) {
            paddrll->sll_pkttype = PACKET_OUTGOING;
            
        } else if (pethhdr->dest.addr[0] & 0x01) {
            if (eth_addr_cmp(&pethhdr->dest, &ethbroadcast)) {
                paddrll->sll_pkttype = PACKET_BROADCAST;
            
            } else {
                paddrll->sll_pkttype = PACKET_MULTICAST;
            }
        
        } else if (pktm->PKTM_ucForme) {
            paddrll->sll_pkttype = PACKET_HOST;
        
        } else {
            paddrll->sll_pkttype = PACKET_OUTGOING;
        }
    }
    
    if (pethhdr->type == PP_HTONS(ETHTYPE_VLAN)) {
        return  (ETH_HLEN + 4);
    
    } else {
        return  (ETH_HLEN);
    }
}
/*********************************************************************************************************
** ��������: __packetEthHeaderInfo2
** ��������: ETH PACKET ��ַ��Ϣ
** �䡡��  : p              ���ݰ�
**           inp            ��Ӧ����ӿ�
**           bOutgo         �Ƿ�Ϊ���
**           psockaddrll    ��·��ַ��Ϣ
** �䡡��  : ����ͷ����
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
size_t  __packetEthHeaderInfo2 (struct pbuf *p, struct netif *inp, BOOL bOutgo, 
                                struct sockaddr_ll *paddrll)
{
    struct eth_hdr  *pethhdr = (struct eth_hdr *)p->payload;
    
    if (paddrll) {
        paddrll->sll_len      = sizeof(struct sockaddr_ll);
        paddrll->sll_family   = AF_PACKET;
        paddrll->sll_protocol = pethhdr->type;
        paddrll->sll_ifindex  = netif_get_index(inp);
        paddrll->sll_hatype   = ARPHRD_ETHER;
        paddrll->sll_halen    = ETHARP_HWADDR_LEN;
        
        if (bOutgo) {
            paddrll->sll_addr[0] = pethhdr->dest.addr[0];
            paddrll->sll_addr[1] = pethhdr->dest.addr[1];
            paddrll->sll_addr[2] = pethhdr->dest.addr[2];
            paddrll->sll_addr[3] = pethhdr->dest.addr[3];
            paddrll->sll_addr[4] = pethhdr->dest.addr[4];
            paddrll->sll_addr[5] = pethhdr->dest.addr[5];
        
        } else {
            paddrll->sll_addr[0] = pethhdr->src.addr[0];
            paddrll->sll_addr[1] = pethhdr->src.addr[1];
            paddrll->sll_addr[2] = pethhdr->src.addr[2];
            paddrll->sll_addr[3] = pethhdr->src.addr[3];
            paddrll->sll_addr[4] = pethhdr->src.addr[4];
            paddrll->sll_addr[5] = pethhdr->src.addr[5];
        }
        
        if (bOutgo) {
            paddrll->sll_pkttype = PACKET_OUTGOING;
            
        } else if (pethhdr->dest.addr[0] & 0x01) {
            if (eth_addr_cmp(&pethhdr->dest, &ethbroadcast)) {
                paddrll->sll_pkttype = PACKET_BROADCAST;
            
            } else {
                paddrll->sll_pkttype = PACKET_MULTICAST;
            }
        
        } else if (lib_memcmp(pethhdr->dest.addr, inp->hwaddr, 
                   ETHARP_HWADDR_LEN) == 0) {
            paddrll->sll_pkttype = PACKET_HOST;
        
        } else {
            paddrll->sll_pkttype = PACKET_OUTGOING;
        }
    }
    
    if (pethhdr->type == PP_HTONS(ETHTYPE_VLAN)) {
        return  (ETH_HLEN + 4);
    
    } else {
        return  (ETH_HLEN);
    }
}
/*********************************************************************************************************
** ��������: __packetEthIfInfo
** ��������: ������ַ��Ϣ
** �䡡��  : iIndex         ����
**           psockaddrll    ��·��ַ��Ϣ
** �䡡��  : �����
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
errno_t  __packetEthIfInfo (INT  iIndex, struct sockaddr_ll *paddrll)
{
    struct netif   *pnetif;
    
    if (iIndex < 0) {
        return  (ENODEV);
    }
    
    LOCK_TCPIP_CORE();
    pnetif = netif_get_by_index(iIndex);
    if (pnetif == LW_NULL) {
        UNLOCK_TCPIP_CORE();
        return  (ENODEV);
    }
    if (!(pnetif->flags & (NETIF_FLAG_ETHARP | NETIF_FLAG_ETHERNET))) {
        UNLOCK_TCPIP_CORE();
        return  (ENXIO);
    }
    
    paddrll->sll_len      = sizeof(struct sockaddr_ll);
    paddrll->sll_family   = AF_PACKET;
    paddrll->sll_protocol = ETH_P_ALL;
    paddrll->sll_ifindex  = iIndex;
    paddrll->sll_hatype   = ARPHRD_ETHER;
    paddrll->sll_halen    = ETHARP_HWADDR_LEN;
    paddrll->sll_pkttype  = PACKET_HOST;
    
    paddrll->sll_addr[0] = pnetif->hwaddr[0];
    paddrll->sll_addr[1] = pnetif->hwaddr[1];
    paddrll->sll_addr[2] = pnetif->hwaddr[2];
    paddrll->sll_addr[3] = pnetif->hwaddr[3];
    paddrll->sll_addr[4] = pnetif->hwaddr[4];
    paddrll->sll_addr[5] = pnetif->hwaddr[5];
    UNLOCK_TCPIP_CORE();
    
    return  (ERROR_NONE);
}

#endif                                                                  /*  LW_CFG_NET_EN               */
/*********************************************************************************************************
  END
*********************************************************************************************************/
