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
** ��   ��   ��: af_packet.c
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2014 �� 03 �� 21 ��
**
** ��        ��: AF_PACKET ֧��
**
** BUG:
2018.01.11  ֧���鲥����.
2018.08.21  ���� __packetMapInput() �� 64 λģʽ���Ļ���״̬�жϴ���.
2019.03.12  ֧�� connect() �� send() ����.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
#include "../SylixOS/system/include/s_system.h"
/*********************************************************************************************************
  �ü�����
*********************************************************************************************************/
#if LW_CFG_NET_EN > 0
#include "netdev.h"
#include "net/if.h"
#include "net/if_arp.h"
#include "net/if_lock.h"
#include "net/if_ether.h"
#include "net/if_flags.h"
#include "sys/socket.h"
#include "netif/etharp.h"
#include "lwip/mem.h"
#include "lwip/pbuf.h"
#include "af_packet.h"
#include "af_packet_eth.h"
/*********************************************************************************************************
  ��������
*********************************************************************************************************/
extern const struct eth_addr ethbroadcast;                              /*  ��̫���㲥��ַ              */
extern void  __socketEnotify(void *file, LW_SEL_TYPE type, INT  iSoErr);
/*********************************************************************************************************
  ������
*********************************************************************************************************/
#define __AF_PACKET_BUF_MAX         (LW_CFG_KB_SIZE * 256)              /*  Ĭ��Ϊ 64K ���ջ���         */
#define __AF_PACKET_BUF_MIN         (LW_CFG_KB_SIZE * 16)               /*  ��С���ջ����С            */
#define __AF_PACKET_BUF_DEF         (LW_CFG_KB_SIZE * 64)
#define __AF_PACKET_PKT_NODES       LW_CFG_LWIP_NUM_POOLS
/*********************************************************************************************************
  ȫ�ֱ���
*********************************************************************************************************/
static LW_LIST_LINE_HEADER          _G_plineAfPacket;
static LW_OBJECT_HANDLE             _G_hAfPacketMutex;

#define __AF_PACKET_LOCK()          API_SemaphoreMPend(_G_hAfPacketMutex, LW_OPTION_WAIT_INFINITE)
#define __AF_PACKET_UNLOCK()        API_SemaphoreMPost(_G_hAfPacketMutex)

static LW_OBJECT_HANDLE             _G_hAfPacketNodes;
#define __AF_PACKET_PKT_NODES_SIZE  (__AF_PACKET_PKT_NODES * (sizeof(AF_PACKET_N) / sizeof(LW_STACK)))
static LW_STACK                     _G_stackPacketNodes[__AF_PACKET_PKT_NODES_SIZE];
/*********************************************************************************************************
  �ȴ��ж�
*********************************************************************************************************/
#define __AF_PACKET_IS_NBIO(pafpacket, flags)   \
        ((pafpacket->PACKET_iFlag & O_NONBLOCK) || (flags & MSG_DONTWAIT))
#define __AF_PACKET_TYPE(pafpacket) (pafpacket->PACKET_iType)
/*********************************************************************************************************
** ��������: __packetBufAlloc
** ��������: �������ݰ�����
** �䡡��  : stLen         ����
** �䡡��  : ������������Ϣ�ڵ�
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static AF_PACKET_N  *__packetBufAlloc (size_t  stLen)
{
    AF_PACKET_N  *pktm;
    
    pktm = (AF_PACKET_N *)API_PartitionGet(_G_hAfPacketNodes);
    if (pktm == LW_NULL) {
        return  (LW_NULL);
    }
    
#if LW_CFG_NET_PACKET_POOL > 0
    pktm->PKTM_p = pbuf_alloc(PBUF_RAW, (u16_t)stLen, PBUF_POOL);
#else                                                                   /*  LW_CFG_NET_PACKET_POOL      */
    pktm->PKTM_p = pbuf_alloc(PBUF_RAW, (u16_t)stLen, PBUF_RAM);
#endif                                                                  /*  !LW_CFG_NET_PACKET_POOL     */
    if (pktm->PKTM_p == LW_NULL) {
        API_PartitionPut(_G_hAfPacketNodes, pktm);
        return  (LW_NULL);
    }
    
    return  (pktm);
}
/*********************************************************************************************************
** ��������: __packetBufFree
** ��������: �������ݰ�����
** �䡡��  : pktm          ������ڵ�
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static VOID  __packetBufFree (AF_PACKET_N  *pktm)
{
    if (pktm->PKTM_p) {
        pbuf_free(pktm->PKTM_p);
    }
    
    API_PartitionPut(_G_hAfPacketNodes, pktm);
}
/*********************************************************************************************************
** ��������: __packetBufFreeAll
** ��������: �����������ݰ�����
** �䡡��  : pafpacket     ���ƿ�
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static VOID  __packetBufFreeAll (AF_PACKET_T *pafpacket)
{
    AF_PACKET_N     *pktm;
    AF_PACKET_Q     *pktq;
    
    pktq = &pafpacket->PACKET_pktq;
    
    while (pktq->PKTQ_pmonoHeader) {
        pktm = (AF_PACKET_N *)pktq->PKTQ_pmonoHeader;
        _list_mono_allocate_seq(&pktq->PKTQ_pmonoHeader,
                                &pktq->PKTQ_pmonoTail);
        __packetBufFree(pktm);
    }
    
    pktq->PKTQ_stTotal = 0;
}
/*********************************************************************************************************
** ��������: __packetBufRecv
** ��������: ����һ�����ݰ�
** �䡡��  : pafpacket     ���ƿ�
**           pvBuffer      ���ջ���
**           stMaxBytes    ��������С
**           paddrll       ���ͷ���Ϣ
**           flags         ���� flags
**           msg_flags     return flags
** �䡡��  : ���ݰ���С
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static ssize_t  __packetBufRecv (AF_PACKET_T *pafpacket, PVOID  pvBuffer, size_t  stMaxBytes,
                                 struct sockaddr_ll *paddrll, int flags, int *msg_flags)
{
    AF_PACKET_N     *pktm;
    AF_PACKET_Q     *pktq;
    u16_t            usCpyOft;
    size_t           stHdrLen;
    size_t           stMsgLen;
    size_t           stRetLen;
    
    pktq = &pafpacket->PACKET_pktq;
    
    if (pktq->PKTQ_pmonoHeader == LW_NULL) {                            /*  û����Ϣ�ɽ���              */
        return  (0);
    }
    
    pktm = (AF_PACKET_N *)pktq->PKTQ_pmonoHeader;
    stHdrLen = __packetEthHeaderInfo(pktm, paddrll);
    
    if (pafpacket->PACKET_iType == SOCK_DGRAM) {
        stMsgLen = pktm->PKTM_p->tot_len - stHdrLen - ETH_PAD_SIZE;
        usCpyOft = (u16_t)(stHdrLen + ETH_PAD_SIZE);
        
    } else {
        stMsgLen = pktm->PKTM_p->tot_len - ETH_PAD_SIZE;
        usCpyOft = ETH_PAD_SIZE;
    }
    
    if (stMsgLen > stMaxBytes) {
        pbuf_copy_partial(pktm->PKTM_p, pvBuffer, (u16_t)stMaxBytes, usCpyOft);
        if (flags & MSG_TRUNC) {
            stRetLen = stMsgLen;
        } else {
            stRetLen = stMaxBytes;
        }
        if (msg_flags) {
            (*msg_flags) |= MSG_TRUNC;
        }
    
    } else {
        pbuf_copy_partial(pktm->PKTM_p, pvBuffer, (u16_t)stMsgLen, usCpyOft);
        stRetLen = stMsgLen;
    }
    
    if ((flags & MSG_PEEK) == 0) {
        _list_mono_allocate_seq(&pktq->PKTQ_pmonoHeader,
                                &pktq->PKTQ_pmonoTail);                 /*  ����Ϣ�Ӷ�����ɾ��          */
        __packetBufFree(pktm);
        
        pktq->PKTQ_stTotal -= stMsgLen;                                 /*  ���»������е�������        */
    }
    
    return  ((ssize_t)stRetLen);
}
/*********************************************************************************************************
** ��������: __packetUpdateReader
** ��������: ��һ�� packet �ڵ㷢�Ϳɶ�
** �䡡��  : pafpacket      ���ƿ�
**           iSoErr         ��������
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static VOID  __packetUpdateReader (AF_PACKET_T *pafpacket, INT  iSoErr)
{
    API_SemaphoreBPost(pafpacket->PACKET_hCanRead);
    
    __socketEnotify(pafpacket->PACKET_sockFile, SELREAD, iSoErr);       /*  ���� select �ɶ�            */
}
/*********************************************************************************************************
** ��������: __packetSetMembership
** ��������: ���� packet �鲥����
** �䡡��  : pafpacket      ���ƿ�
**           iCmd           PACKET_ADD_MEMBERSHIP / PACKET_DROP_MEMBERSHIP
**           pmreq          �鲥����
** �䡡��  : ����ֵ
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static INT  __packetSetMembership (AF_PACKET_T *pafpacket, INT  iCmd, struct packet_mreq  *pmreq)
{
    struct netif       *pnetif;
    struct netdev      *pnetdev;
    struct ifreq        ifreq;
    INT                 iRet = PX_ERROR;
    
    LWIP_IF_LIST_LOCK(LW_FALSE);
    pnetdev = netdev_find_by_index((UINT)pmreq->mr_ifindex);
    if (!pnetdev) {
        LWIP_IF_LIST_UNLOCK();
        _ErrorHandle(ENXIO);
        return  (PX_ERROR);
    }
    
    if (pnetdev->net_type != NETDEV_TYPE_ETHERNET) {
        LWIP_IF_LIST_UNLOCK();
        _ErrorHandle(EOPNOTSUPP);
        return  (PX_ERROR);
    }
    
    pnetif = (struct netif *)pnetdev->sys;
    
    switch (pmreq->mr_type) {
    
    case PACKET_MR_MULTICAST:                                           /*  �鲥��ַ                    */
        if (iCmd == PACKET_ADD_MEMBERSHIP) {
            iRet = netdev_macfilter_add(pnetdev, pmreq->mr_address);
        } else {
            iRet = netdev_macfilter_delete(pnetdev, pmreq->mr_address);
        }
        break;
        
    case PACKET_MR_PROMISC:                                             /*  ����ģʽ                    */
        ifreq.ifr_flags = netif_get_flags(pnetif);
        if (iCmd == PACKET_ADD_MEMBERSHIP) {
            if (!(ifreq.ifr_flags & IFF_PROMISC)) {
                ifreq.ifr_flags |= IFF_PROMISC;
                iRet = pnetif->ioctl(pnetif, SIOCSIFFLAGS, &ifreq);
                if (iRet == ERROR_NONE) {
                    pnetif->flags2 |= NETIF_FLAG2_PROMISC;
                }
            } else {
                iRet = ERROR_NONE;
            }
        } else {
            if (ifreq.ifr_flags & IFF_PROMISC) {
                ifreq.ifr_flags &= ~IFF_PROMISC;
                iRet = pnetif->ioctl(pnetif, SIOCSIFFLAGS, &ifreq);
                if (iRet == ERROR_NONE) {
                    pnetif->flags2 &= ~NETIF_FLAG2_PROMISC;
                }
            } else {
                iRet = ERROR_NONE;
            }
        }
        break;
        
    case PACKET_MR_ALLMULTI:                                            /*  ȫ���鲥                    */
        ifreq.ifr_flags = netif_get_flags(pnetif);
        if (iCmd == PACKET_ADD_MEMBERSHIP) {
            if (!(ifreq.ifr_flags & IFF_ALLMULTI)) {
                ifreq.ifr_flags |= IFF_ALLMULTI;
                iRet = pnetif->ioctl(pnetif, SIOCSIFFLAGS, &ifreq);
                if (iRet == ERROR_NONE) {
                    pnetif->flags2 |= NETIF_FLAG2_ALLMULTI;
                }
            } else {
                iRet = ERROR_NONE;
            }
        } else {
            if (ifreq.ifr_flags & IFF_ALLMULTI) {
                ifreq.ifr_flags &= ~IFF_ALLMULTI;
                iRet = pnetif->ioctl(pnetif, SIOCSIFFLAGS, &ifreq);
                if (iRet == ERROR_NONE) {
                    pnetif->flags2 &= ~NETIF_FLAG2_ALLMULTI;
                }
            } else {
                iRet = ERROR_NONE;
            }
        }
        break;
        
    default:
        _ErrorHandle(EINVAL);
        break;
    }
    LWIP_IF_LIST_UNLOCK();
    
    return  (iRet);
}
/*********************************************************************************************************
** ��������: __packetRingTraversal
** ��������: �����ڴ���, ���� frame ����
** �䡡��  : pmmap          �����ڴ�����
**           pfunc          �������� (���ط� 0 ��������)
**           pvArg[0 ~ 5]   ������������
** �䡡��  : ����ֵ
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
#if LW_CFG_NET_PACKET_MMAP > 0

static VOID  __packetRingTraversal (AF_PACKET_MMAP *pmmap,
                                    FUNCPTR         pfunc, 
                                    PVOID           pvArg0,
                                    PVOID           pvArg1,
                                    PVOID           pvArg2,
                                    PVOID           pvArg3,
                                    PVOID           pvArg4,
                                    PVOID           pvArg5)
{
    UINT                 i, j;
    UINT8               *pucBlock; 
    UINT8               *pucFrame;
    struct tpacket_req  *preq;
    
    preq = &pmmap->PKTB_reqbuf;
    
    pucBlock = (UINT8 *)pmmap->PKTB_pvVirMem;
    for (i = 0; i < preq->tp_block_nr; i++) {
        pucFrame = pucBlock;
        for (j = 0; j < pmmap->PKTB_uiFramePerBlock; j++) {
            if (pfunc(pucFrame, pvArg0, pvArg1, pvArg2, pvArg3, pvArg4, pvArg5)) {
                return;
            }
            pucFrame += preq->tp_frame_size;
        }
        pucBlock += preq->tp_block_size;
    }
}
/*********************************************************************************************************
** ��������: __packetCanReadCb
** ��������: �����ڴ���, �Ƿ�ɶ������ص�
** �䡡��  : pmmap          �����ڴ�����
**           pafpacket      ���ƿ�
**           pbCanRead      ��д���жϽ��
** �䡡��  : �Ƿ��˳�����
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static INT  __packetCanReadCb (UINT8 *pucFrame, AF_PACKET_T *pafpacket, BOOL *pbCanRead)
{
    switch (pafpacket->PACKET_tpver) {
    
    case TPACKET_V1:
        if (((struct tpacket_hdr *)pucFrame)->tp_status == TP_STATUS_USER) {
            *pbCanRead = LW_TRUE;
            return  (1);
        }
        break;
        
    case TPACKET_V2:
        if (((struct tpacket2_hdr *)pucFrame)->tp_status == TP_STATUS_USER) {
            *pbCanRead = LW_TRUE;
            return  (1);
        }
        break;
    }
    
    return  (0);
}
/*********************************************************************************************************
** ��������: __packetShareInitCb
** ��������: �����ڴ�����ʼ��
** �䡡��  : pmmap          �����ڴ�����
**           pafpacket      ���ƿ�
** �䡡��  : �Ƿ��˳�����
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static INT  __packetShareInitCb (UINT8 *pucFrame, AF_PACKET_T *pafpacket)
{
    switch (pafpacket->PACKET_tpver) {
    
    case TPACKET_V1:
        ((struct tpacket_hdr *)pucFrame)->tp_status = TP_STATUS_KERNEL;
        break;
        
    case TPACKET_V2:
        ((struct tpacket2_hdr *)pucFrame)->tp_status = TP_STATUS_KERNEL;
        break;
    }
    
    return  (0);
}
/*********************************************************************************************************
** ��������: __packetSetRing
** ��������: ���� packet ring
** �䡡��  : pafpacket      ���ƿ�
**           preq           ����������
** �䡡��  : ����ֵ
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static INT  __packetSetRing (AF_PACKET_T *pafpacket, struct tpacket_req  *preq)
{
    AF_PACKET_MMAP *pmmapRx = &pafpacket->PACKET_mmapRx;
    ULONG           ulFlag  = LW_VMM_FLAG_RDWR;
    ULONG           ulBlock;
    errno_t         err     = EBUSY;

    if (pafpacket->PACKET_bMmap || pafpacket->PACKET_bMapBusy) {
        goto    __error_handle;
    }
    
    API_SemaphoreBStatus(pafpacket->PACKET_hCanRead, LW_NULL, LW_NULL, &ulBlock);
    if (ulBlock) {
        goto    __error_handle;
    }
    
    err = EINVAL;
    if (preq->tp_block_nr == 0) {
        goto    __error_handle;
    }
    
    if (preq->tp_block_size & (LW_CFG_VMM_PAGE_SIZE - 1)) {
        goto    __error_handle;
    }
    
    if (preq->tp_frame_size < (pafpacket->PACKET_uiHdrLen + pafpacket->PACKET_uiReserve)) {
        goto    __error_handle;
    }
    
    if (preq->tp_frame_size & (TPACKET_ALIGNMENT - 1)) {
        goto    __error_handle;
    }
    
    pmmapRx->PKTB_uiFramePerBlock = preq->tp_block_size / preq->tp_frame_size;
    if (pmmapRx->PKTB_uiFramePerBlock == 0) {
        goto    __error_handle;
    }
    
    if ((pmmapRx->PKTB_uiFramePerBlock * preq->tp_block_nr) != preq->tp_frame_nr) {
        goto    __error_handle;
    }
    
    pafpacket->PACKET_bMapBusy = LW_TRUE;                               /*  ��ʼ����ӳ������            */
    __AF_PACKET_UNLOCK();
    
    pmmapRx->PKTB_uiFramePtr = 0;
    pmmapRx->PKTB_uiFrameMax = pmmapRx->PKTB_uiFramePerBlock * preq->tp_block_nr;
    
#if LW_CFG_CACHE_EN > 0
    if (API_CacheAliasProb()) {                                         /*  ����� CACHE ��������       */
        ulFlag &= ~(LW_VMM_FLAG_CACHEABLE | LW_VMM_FLAG_WRITETHROUGH);  /*  �����ڴ治�����κ���ʽ CACHE*/
    }
#endif                                                                  /*  LW_CFG_CACHE_EN > 0         */
    
    err = ENOMEM;
    pmmapRx->PKTB_stSize   = preq->tp_block_nr * preq->tp_block_size;
    pmmapRx->PKTB_pvPhyMem = API_VmmPhyAlloc(pmmapRx->PKTB_stSize);
    if (pmmapRx->PKTB_pvPhyMem == LW_NULL) {
        __AF_PACKET_LOCK();
        goto    __error_handle;
    }
    
    pmmapRx->PKTB_pvVirMem = API_VmmMallocAreaEx(pmmapRx->PKTB_stSize, LW_NULL, LW_NULL,
                                                 LW_VMM_SHARED_CHANGE, ulFlag);
    if (pmmapRx->PKTB_pvVirMem == LW_NULL) {
        API_VmmPhyFree(pmmapRx->PKTB_pvPhyMem);
        __AF_PACKET_LOCK();
        goto    __error_handle;
    }
    
    if (API_VmmRemapArea(pmmapRx->PKTB_pvVirMem, pmmapRx->PKTB_pvPhyMem, 
                         pmmapRx->PKTB_stSize, ulFlag,
                         LW_NULL, LW_NULL)) {                           /*  �������ڴ�ӳ�䵽�����ڴ�    */
        API_VmmFreeArea(pmmapRx->PKTB_pvVirMem);
        API_VmmPhyFree(pmmapRx->PKTB_pvPhyMem);
        __AF_PACKET_LOCK();
        goto    __error_handle;
    }
    
    pmmapRx->PKTB_reqbuf = *preq;                                       /*  ���滺��������              */
    
    __packetRingTraversal(&pafpacket->PACKET_mmapRx, __packetShareInitCb,
                          pafpacket, LW_NULL, LW_NULL, LW_NULL, 
                          LW_NULL, LW_NULL);
    
    KN_SMP_MB();
    pafpacket->PACKET_bMmap    = LW_TRUE;                               /*  ���ù����ڴ�ģʽ            */
    pafpacket->PACKET_bMapBusy = LW_FALSE;
    __AF_PACKET_LOCK();
    
    __packetBufFreeAll(pafpacket);                                      /*  ɾ�����еȴ���ȡ�����ݰ�    */
    
    return  (ERROR_NONE);

__error_handle:
    _ErrorHandle(err);
    return  (PX_ERROR);
}
/*********************************************************************************************************
** ��������: __packetGetFrame
** ��������: ���һ�� ring frame
** �䡡��  : pmmap     �����ڴ滷������
** �䡡��  : frame �׵�ַ
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static PVOID  __packetGetFrame (AF_PACKET_MMAP  *pmmap)
{
    addr_t  ulAddr;
    UINT    uiBlock = pmmap->PKTB_uiFramePtr / pmmap->PKTB_uiFramePerBlock;
    UINT    uiFrame = pmmap->PKTB_uiFramePtr % pmmap->PKTB_uiFramePerBlock;
    
    ulAddr  = (addr_t)pmmap->PKTB_pvVirMem + (addr_t)(uiBlock * pmmap->PKTB_reqbuf.tp_block_size);
    ulAddr += (addr_t)(uiFrame * pmmap->PKTB_reqbuf.tp_frame_size);
    
    return  ((PVOID)ulAddr);
}

#endif                                                                  /*  LW_CFG_NET_PACKET_MMAP      */
/*********************************************************************************************************
** ��������: __packetCanWrite
** ��������: �ж��Ƿ������ָ���Ľ��սڵ㷢������
** �䡡��  : pafpacket      ���ƿ�
** �䡡��  : �Ƿ��д
** ȫ�ֱ���: 
** ����ģ��: 
** ע  ��  : ����Ĭ���������Է���.
*********************************************************************************************************/
static BOOL  __packetCanWrite (AF_PACKET_T *pafpacket)
{
    return  (LW_TRUE);
}
/*********************************************************************************************************
** ��������: __packetCanRead
** ��������: ��ǰ�ڵ��Ƿ�ɶ�
** �䡡��  : pafpacket      ���ƿ�
**           flags          MSG_PEEK or MSG_WAITALL 
**           stLen          ����� MSG_WAITALL ��Ҫ�жϳ���.
** �䡡��  : �Ƿ�ɶ�
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static BOOL  __packetCanRead (AF_PACKET_T *pafpacket, INT  flags, size_t  stLen)
{
    AF_PACKET_Q *pktq;
    
#if LW_CFG_NET_PACKET_MMAP > 0
    if (pafpacket->PACKET_bMmap == LW_FALSE) {
#endif                                                                  /*  LW_CFG_NET_PACKET_MMAP > 0  */
        pktq = &pafpacket->PACKET_pktq;
        if (pktq->PKTQ_pmonoHeader == LW_NULL) {                        /*  û����Ϣ�ɽ���              */
            return  (LW_FALSE);
        }
        
        if (flags & MSG_WAITALL) {
            if (pktq->PKTQ_stTotal < stLen) {                           /*  ���ݲ����޷�����            */
                return  (LW_FALSE);
            }
        }
        
#if LW_CFG_NET_PACKET_MMAP > 0
    } else {
        BOOL  bCanRead = LW_FALSE;
        
        __packetRingTraversal(&pafpacket->PACKET_mmapRx, __packetCanReadCb,
                              pafpacket, &bCanRead, LW_NULL, LW_NULL, 
                              LW_NULL, LW_NULL);
        return  (bCanRead);
    }
#endif                                                                  /*  LW_CFG_NET_PACKET_MMAP > 0  */

    return  (LW_TRUE);
}
/*********************************************************************************************************
** ��������: __packetCreate
** ��������: ����һ�� packet socket
** �䡡��  : iType          SOCK_DGRAM / SOCK_RAW
**           iProtocol      Э��
** �䡡��  : packet socket
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static AF_PACKET_T  *__packetCreate (INT  iType, INT  iProtocol)
{
    AF_PACKET_T *pafpacket;
    
    pafpacket = (AF_PACKET_T *)__SHEAP_ALLOC(sizeof(AF_PACKET_T));
    if (pafpacket == LW_NULL) {
        return  (LW_NULL);
    }
    lib_bzero(pafpacket, sizeof(AF_PACKET_T));
    
    pafpacket->PACKET_iFlag         = O_RDWR;
    pafpacket->PACKET_iType         = iType;
    pafpacket->PACKET_iProtocol     = iProtocol;
    pafpacket->PACKET_iShutDFlag    = 0;
    pafpacket->PACKET_bRecvOut      = LW_TRUE;
    pafpacket->PACKET_iIfIndex      = 0;                                /*  δ���κ�����ӿ�          */
    pafpacket->PACKET_stMaxBufSize  = __AF_PACKET_BUF_DEF;
    pafpacket->PACKET_ulRecvTimeout = LW_OPTION_WAIT_INFINITE;
    pafpacket->PACKET_tpver         = TPACKET_V1;
    pafpacket->PACKET_uiHdrLen      = TPACKET_HDRLEN;
    pafpacket->PACKET_uiReserve     = 0;
    
#if LW_CFG_NET_PACKET_MMAP > 0
    pafpacket->PACKET_bMapBusy = LW_FALSE;
    pafpacket->PACKET_bMmap    = LW_FALSE;
#endif                                                                  /*  LW_CFG_NET_PACKET_MMAP > 0  */
    
    pafpacket->PACKET_hCanRead = API_SemaphoreBCreate("packet_rlock", LW_FALSE, 
                                                      LW_OPTION_OBJECT_GLOBAL, LW_NULL);
    if (pafpacket->PACKET_hCanRead == LW_OBJECT_HANDLE_INVALID) {
        __SHEAP_FREE(pafpacket);
        return  (LW_NULL);
    }
    
    __AF_PACKET_LOCK();
    _List_Line_Add_Ahead(&pafpacket->PACKET_lineManage, &_G_plineAfPacket);
    __AF_PACKET_UNLOCK();
    
    return  (pafpacket);
}
/*********************************************************************************************************
** ��������: __packetDelete
** ��������: ɾ��һ�� packet socket ���ƿ�
** �䡡��  : pafpacket      ���ƿ�
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static VOID  __packetDelete (AF_PACKET_T *pafpacket)
{
    __AF_PACKET_LOCK();
    __packetBufFreeAll(pafpacket);
    _List_Line_Del(&pafpacket->PACKET_lineManage, &_G_plineAfPacket);
    __AF_PACKET_UNLOCK();
    
#if LW_CFG_NET_PACKET_MMAP > 0
    if (pafpacket->PACKET_bMmap) {
        API_VmmFreeArea(pafpacket->PACKET_mmapRx.PKTB_pvVirMem);
        API_VmmPhyFree(pafpacket->PACKET_mmapRx.PKTB_pvPhyMem);
    }
#endif                                                                  /*  LW_CFG_NET_PACKET_MMAP > 0  */

    API_SemaphoreBDelete(&pafpacket->PACKET_hCanRead);
    __SHEAP_FREE(pafpacket);
}
/*********************************************************************************************************
** ��������: __packetMsToTicks
** ��������: ����ת��Ϊ ticks
** �䡡��  : ulMs        ����
** �䡡��  : tick
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static ULONG  __packetMsToTicks (ULONG  ulMs)
{
    ULONG   ulTicks;
    
    if (ulMs == 0) {
        ulTicks = LW_OPTION_WAIT_INFINITE;
    } else {
        ulTicks = LW_MSECOND_TO_TICK_1(ulMs);
    }
    
    return  (ulTicks);
}
/*********************************************************************************************************
** ��������: __packetTvToTicks
** ��������: timeval ת��Ϊ ticks
** �䡡��  : ptv       ʱ��
** �䡡��  : tick
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static ULONG  __packetTvToTicks (struct timeval  *ptv)
{
    ULONG   ulTicks;

    if ((ptv->tv_sec == 0) && (ptv->tv_usec == 0)) {
        return  (LW_OPTION_WAIT_INFINITE);
    }
    
    ulTicks = __timevalToTick(ptv);
    if (ulTicks == 0) {
        ulTicks = 1;
    }
    
    return  (ulTicks);
}
/*********************************************************************************************************
** ��������: __packetTicksToMs
** ��������: ticks ת��Ϊ����
** �䡡��  : ulTicks       tick
** �䡡��  : ����
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static ULONG  __packetTicksToMs (ULONG  ulTicks)
{
    ULONG  ulMs;
    
    if (ulTicks == LW_OPTION_WAIT_INFINITE) {
        ulMs = 0;
    } else {
        ulMs = (ulTicks * 1000) / LW_TICK_HZ;
    }
    
    return  (ulMs);
}
/*********************************************************************************************************
** ��������: __packetTicksToTv
** ��������: ticks ת��Ϊ timeval
** �䡡��  : ulTicks       tick
** �䡡��  : timeval
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static VOID __packetTicksToTv (ULONG  ulTicks, struct timeval *ptv)
{
    if (ulTicks == LW_OPTION_WAIT_INFINITE) {
        ptv->tv_sec  = 0;
        ptv->tv_usec = 0;
    } else {
        __tickToTimeval(ulTicks, ptv);
    }
}
/*********************************************************************************************************
** ��������: __packetBufInput
** ��������: ͨ�����巽ʽ����һ�����ݰ�
** �䡡��  : pafpacket      ���ƿ�
**           p              ���ݰ�
**           inp            ��Ӧ����ӿ�
**           bOutgo         �Ƿ�Ϊ���
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static VOID  __packetBufInput (AF_PACKET_T *pafpacket, struct pbuf *p, struct netif *inp, BOOL bOutgo)
{
    AF_PACKET_N     *pktm;
    AF_PACKET_Q     *pktq;
    size_t           stMsgLen;
    struct eth_hdr  *pethhdr;

    pktq    = &pafpacket->PACKET_pktq;
    pethhdr = (struct eth_hdr *)p->payload;
    
    pafpacket->PACKET_stats.tp_packets++;
    
    if (pafpacket->PACKET_iType == SOCK_RAW) {
        stMsgLen = p->tot_len - ETH_PAD_SIZE;
    
    } else {
        if (pethhdr->type == PP_HTONS(ETHTYPE_VLAN)) {
            stMsgLen = p->tot_len - ETH_HLEN - SIZEOF_VLAN_HDR - ETH_PAD_SIZE;
        
        } else {
            stMsgLen = p->tot_len - ETH_HLEN - ETH_PAD_SIZE;
        }
    }
    
    if ((stMsgLen + pktq->PKTQ_stTotal) > 
        (pafpacket->PACKET_stMaxBufSize)) {                             /*  ������������, ����          */
        pafpacket->PACKET_stats.tp_drops++;
        return;
    }

    pktm = __packetBufAlloc((size_t)p->tot_len);
    if (pktm == LW_NULL) {
        pafpacket->PACKET_stats.tp_drops++;
        return;                                                         /*  �ڴ治��                    */
    }
    pbuf_copy(pktm->PKTM_p, p);                                         /*  ������������                */
    
    if (lib_memcmp(pethhdr->dest.addr, inp->hwaddr, 
                   ETHARP_HWADDR_LEN) == 0) {
        pktm->PKTM_ucForme = 1;
    
    } else {
        pktm->PKTM_ucForme = 0;
    }
    
    pktm->PKTM_ucIndex = netif_get_index(inp);
    pktm->PKTM_ucOutgo = (u8_t)bOutgo;
    
    _list_mono_free_seq(&pktq->PKTQ_pmonoHeader,
                        &pktq->PKTQ_pmonoTail,
                        &pktm->PKTM_monoManage);                        /*  ͨ�� mono free ����������� */
                        
    pktq->PKTQ_stTotal += stMsgLen;                                     /*  ���»������е�������        */
    
    __packetUpdateReader(pafpacket, ERROR_NONE);                        /*  �ɶ�                        */
}
/*********************************************************************************************************
** ��������: __packetMapInput
** ��������: ͨ���ڴ湲��ʽ����һ�����ݰ�
** �䡡��  : pafpacket      ���ƿ�
**           p              ���ݰ�
**           inp            ��Ӧ����ӿ�
**           bOutgo         �Ƿ�Ϊ���
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
#if LW_CFG_NET_PACKET_MMAP > 0

static VOID  __packetMapInput (AF_PACKET_T *pafpacket, struct pbuf *p, struct netif *inp, BOOL bOutgo)
{
    union {
        struct tpacket_hdr  *h1;
        struct tpacket2_hdr *h2;
        void *raw;
    } hdr;
    
    struct sockaddr_ll *psll;
    struct timespec     ts;
    AF_PACKET_MMAP     *pmmapRx = &pafpacket->PACKET_mmapRx;
    
    size_t              stPktLen;
    size_t              stMsgLen;
    size_t              stCaplen;
    size_t              stMaclen;
    
    UINT16              usSllOff;
    UINT16              usMacoff;
    UINT16              usNetoff;
    
    pafpacket->PACKET_stats.tp_packets++;
    
    usSllOff = (UINT16)(pafpacket->PACKET_uiHdrLen - sizeof(struct sockaddr_ll));
    usNetoff = (UINT16)(TPACKET_ALIGN(pafpacket->PACKET_uiHdrLen) + 
                        TPACKET_ALIGN(pafpacket->PACKET_uiReserve + SIZEOF_ETH_HDR + SIZEOF_VLAN_HDR));
    
    hdr.raw = __packetGetFrame(pmmapRx);
    if (pafpacket->PACKET_tpver == TPACKET_V1) {
        if (hdr.h1->tp_status != TP_STATUS_KERNEL) {
            pafpacket->PACKET_stats.tp_drops++;
            return;
        }
        
    } else {
        if (hdr.h2->tp_status != TP_STATUS_KERNEL) {
            pafpacket->PACKET_stats.tp_drops++;
            return;
        }
    }
    
    pmmapRx->PKTB_uiFramePtr++;
    if (pmmapRx->PKTB_uiFramePtr >= pmmapRx->PKTB_uiFrameMax) {
        pmmapRx->PKTB_uiFramePtr  = 0;
    }
    
    psll     = (struct sockaddr_ll *)((addr_t)hdr.raw + usSllOff);
    stMaclen = __packetEthHeaderInfo2(p, inp, bOutgo, psll);
    stPktLen = p->tot_len - ETH_PAD_SIZE;
    
    if (pafpacket->PACKET_iType == SOCK_RAW) {
        stMsgLen = stPktLen;
        usMacoff = (UINT16)(usNetoff - stMaclen);
        if (pmmapRx->PKTB_reqbuf.tp_frame_size >= (usMacoff + stMsgLen)) {
            stCaplen = stMsgLen;
        } else {
            stCaplen = pmmapRx->PKTB_reqbuf.tp_frame_size - usMacoff;
        }
    
    } else {
        stMsgLen = stPktLen - stMaclen;
        usMacoff = usNetoff;
        if (pmmapRx->PKTB_reqbuf.tp_frame_size >= (usNetoff + stMsgLen)) {
            stCaplen = stMsgLen;
        } else {
            stCaplen = pmmapRx->PKTB_reqbuf.tp_frame_size - usNetoff;
        }
    }
    
    lib_clock_gettime(CLOCK_REALTIME, &ts);
    
    switch (pafpacket->PACKET_tpver) {
    
    case TPACKET_V1:
        hdr.h1->tp_status  = TP_STATUS_COPY;
        hdr.h1->tp_len     = (UINT)stMsgLen;
        hdr.h1->tp_snaplen = (UINT)stCaplen;
        hdr.h1->tp_mac     = usMacoff;
        hdr.h1->tp_net     = usNetoff;
        hdr.h1->tp_sec     = (UINT32)ts.tv_sec;
        hdr.h1->tp_usec    = (UINT32)ts.tv_nsec / 1000;
        break;
        
    case TPACKET_V2:
        hdr.h2->tp_status  = TP_STATUS_COPY;
        hdr.h2->tp_len     = (UINT)stMsgLen;
        hdr.h2->tp_snaplen = (UINT)stCaplen;
        hdr.h2->tp_mac     = usMacoff;
        hdr.h2->tp_net     = usNetoff;
        hdr.h2->tp_sec     = (UINT32)ts.tv_sec;
        hdr.h2->tp_nsec    = (UINT32)ts.tv_nsec;
        {
            struct eth_hdr *ethhdr = (struct eth_hdr *)p->payload;
            
            if (ethhdr->type == PP_HTONS(ETHTYPE_VLAN) && (p->len >= SIZEOF_ETH_HDR + 4)) {
                struct eth_vlan_hdr *vlan = (struct eth_vlan_hdr *)(((u8_t *)ethhdr) + SIZEOF_ETH_HDR);
                hdr.h2->tp_vlan_tci  = vlan->prio_vid;
                hdr.h2->tp_vlan_tpid = vlan->tpid;
            
            } else {
                hdr.h2->tp_vlan_tci  = 0;
                hdr.h2->tp_vlan_tpid = 0;
            }
        }
        break;
    }
    
    if (pafpacket->PACKET_iType == SOCK_RAW) {
        pbuf_copy_partial(p, (void *)((addr_t)hdr.raw + usMacoff), 
                          (u16_t)stCaplen, ETH_PAD_SIZE);
    } else {
        pbuf_copy_partial(p, (void *)((addr_t)hdr.raw + usNetoff), 
                          (u16_t)stCaplen, (u16_t)(stMaclen + ETH_PAD_SIZE));
    }
    
    KN_SMP_MB();
    if (pafpacket->PACKET_tpver == TPACKET_V1) {
        hdr.h1->tp_status = TP_STATUS_USER;
    } else {
        hdr.h2->tp_status = TP_STATUS_USER;
    }
    
    __packetUpdateReader(pafpacket, ERROR_NONE);                        /*  �ɶ�                        */
}

#endif                                                                  /*  LW_CFG_NET_PACKET_MMAP > 0  */
/*********************************************************************************************************
** ��������: packet_link_input
** ��������: ������·��������ݰ��ص� (û���� CORELOCK ����)
** �䡡��  : p         ���ݰ�
**           inp       ��Ӧ����ӿ�
**           bOutgo    �Ƿ�Ϊ���
** �䡡��  : �Ƿ� eaten
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
INT  packet_link_input (struct pbuf *p, struct netif *inp, BOOL bOutgo)
{
#if BYTE_ORDER == LITTLE_ENDIAN
    static UINT16    usAll = 0x0300;                                    /*  ETH_P_ALL                   */
#else
    static UINT16    usAll = 0x0003;
#endif

    AF_PACKET_T          *pafpacket;
    PLW_LIST_LINE         plineTemp;
    struct eth_hdr       *pethhdr;
    struct eth_vlan_hdr  *pethvlanhdr;
    BOOL                  bRecv;

    if ((_G_plineAfPacket == LW_NULL) ||
        (p->tot_len < (ETH_HLEN + ETH_PAD_SIZE)) ||
        !(inp->flags & (NETIF_FLAG_ETHARP | NETIF_FLAG_ETHERNET))) {
        return  (0);
    }
    
    pethhdr = (struct eth_hdr *)p->payload;
    if (pethhdr->type == PP_HTONS(ETHTYPE_VLAN)) {
        pethvlanhdr = (struct eth_vlan_hdr *)(((char *)pethhdr) + SIZEOF_ETH_HDR);
    
    } else {
        pethvlanhdr = LW_NULL;
    }
    
    __AF_PACKET_LOCK();
    for (plineTemp  = _G_plineAfPacket;
         plineTemp != LW_NULL;
         plineTemp  = _list_line_get_next(plineTemp)) {
    
        pafpacket = (AF_PACKET_T *)plineTemp;
        if ((pafpacket->PACKET_iIfIndex > 0) &&
            (pafpacket->PACKET_iIfIndex != netif_get_index(inp))) {     /*  ���ǰ󶨵�����              */
            continue;
        }
        
#if LW_CFG_NET_PACKET_MMAP > 0
        if (pafpacket->PACKET_bMapBusy) {                               /*  busy ״̬                   */
            continue;
        }
#endif                                                                  /*  LW_CFG_NET_PACKET_MMAP > 0  */
        
        if (bOutgo && (pafpacket->PACKET_bRecvOut == LW_FALSE)) {
            continue;
        }
        
        bRecv = LW_FALSE;
        if (pethvlanhdr) {
            if ((pethhdr->type     == (UINT16)pafpacket->PACKET_iProtocol) ||
                (pethvlanhdr->tpid == (UINT16)pafpacket->PACKET_iProtocol) ||
                (usAll             == (UINT16)pafpacket->PACKET_iProtocol)) {
                bRecv = LW_TRUE;
            }
        
        } else {
            if ((pethhdr->type == (UINT16)pafpacket->PACKET_iProtocol) ||
                (usAll         == (UINT16)pafpacket->PACKET_iProtocol)) {
                bRecv = LW_TRUE;
            }
        }
        
        if (bRecv) {                                                    /*  Э��ƥ��                    */
#if LW_CFG_NET_PACKET_MMAP > 0
            if (pafpacket->PACKET_bMmap == LW_FALSE) {
#endif                                                                  /*  LW_CFG_NET_PACKET_MMAP > 0  */
                __packetBufInput(pafpacket, p, inp, bOutgo);

#if LW_CFG_NET_PACKET_MMAP > 0
            } else {
                __packetMapInput(pafpacket, p, inp, bOutgo);
            }
#endif                                                                  /*  LW_CFG_NET_PACKET_MMAP > 0  */
        }
    }
    __AF_PACKET_UNLOCK();

    return  (0);
}
/*********************************************************************************************************
** ��������: packet_init
** ��������: ��ʼ�� af packet Э��
** �䡡��  : NONE
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
VOID  packet_init (VOID)
{
    _G_hAfPacketMutex = API_SemaphoreMCreate("afpacket_lock", LW_PRIO_DEF_CEILING, 
                                             LW_OPTION_WAIT_PRIORITY | LW_OPTION_DELETE_SAFE |
                                             LW_OPTION_INHERIT_PRIORITY | LW_OPTION_OBJECT_GLOBAL,
                                             LW_NULL);
                                             
    _G_hAfPacketNodes = API_PartitionCreate("afpacket_nodes", _G_stackPacketNodes, __AF_PACKET_PKT_NODES,
                                            sizeof(AF_PACKET_N), LW_OPTION_OBJECT_GLOBAL, LW_NULL);
}
/*********************************************************************************************************
** ��������: packet_traversal
** ��������: �������� af_packet ���ƿ�
** �䡡��  : pfunc                ��������
**           pvArg[0 ~ 5]         ������������
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
VOID  packet_traversal (VOIDFUNCPTR    pfunc, 
                        PVOID          pvArg0,
                        PVOID          pvArg1,
                        PVOID          pvArg2,
                        PVOID          pvArg3,
                        PVOID          pvArg4,
                        PVOID          pvArg5)
{
    AF_PACKET_T     *pafpacketTemp;
    PLW_LIST_LINE    plineTemp;
    
    __AF_PACKET_LOCK();
    for (plineTemp  = _G_plineAfPacket;
         plineTemp != LW_NULL;
         plineTemp  = _list_line_get_next(plineTemp)) {
        
        pafpacketTemp = (AF_PACKET_T *)plineTemp;
        pfunc(pafpacketTemp, pvArg0, pvArg1, pvArg2, pvArg3, pvArg4, pvArg5);
    }
    __AF_PACKET_UNLOCK();
}
/*********************************************************************************************************
** ��������: packet_socket
** ��������: packet socket
** �䡡��  : iDomain        ��, ������ AF_PACKET
**           iType          SOCK_DGRAM / SOCK_RAW
**           iProtocol      Э��
** �䡡��  : packet socket
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
AF_PACKET_T  *packet_socket (INT  iDomain, INT  iType, INT  iProtocol)
{
    AF_PACKET_T *pafpacket;
    
    if (iDomain != AF_PACKET) {
        _ErrorHandle(EINVAL);
        return  (LW_NULL);
    }
    
    if ((iType != SOCK_DGRAM)  &&
        (iType != SOCK_RAW)) {
        _ErrorHandle(EINVAL);
        return  (LW_NULL);
    }
    
    pafpacket = __packetCreate(iType, iProtocol);
    if (pafpacket == LW_NULL) {
        _ErrorHandle(ENOMEM);
    }
    
    return  (pafpacket);
}
/*********************************************************************************************************
** ��������: packet_bind
** ��������: bind
** �䡡��  : pafpacket afpacket file
**           name      address
**           namelen   address len
** �䡡��  : ERROR
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
INT  packet_bind (AF_PACKET_T *pafpacket, const struct sockaddr *name, socklen_t namelen)
{
    struct netif       *pnetif;
    struct sockaddr_ll *paddrll = (struct sockaddr_ll *)name;
    
    if (!name || (namelen < sizeof(struct sockaddr_ll))) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    
    if (paddrll->sll_ifindex) {
        LWIP_IF_LIST_LOCK(LW_FALSE);
        pnetif = netif_get_by_index(paddrll->sll_ifindex);
        if (!pnetif) {
            LWIP_IF_LIST_UNLOCK();
            _ErrorHandle(ENODEV);
            return  (PX_ERROR);
        }
        if (!(pnetif->flags & (NETIF_FLAG_ETHARP | NETIF_FLAG_ETHERNET))) {
            LWIP_IF_LIST_UNLOCK();
            _ErrorHandle(ENXIO);
            return  (PX_ERROR);
        }
        LWIP_IF_LIST_UNLOCK();
    }
    
    pafpacket->PACKET_iIfIndex = paddrll->sll_ifindex;
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: packet_listen
** ��������: listen
** �䡡��  : pafpacket afpacket file
**           backlog   back log num
** �䡡��  : ERROR
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
INT  packet_listen (AF_PACKET_T *pafpacket, INT  backlog)
{
    _ErrorHandle(EOPNOTSUPP);
    return  (PX_ERROR);
}
/*********************************************************************************************************
** ��������: packet_accept
** ��������: accept
** �䡡��  : pafpacket afpacket file
**           addr      address
**           addrlen   address len
** �䡡��  : packet socket
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
AF_PACKET_T  *packet_accept (AF_PACKET_T *pafpacket, struct sockaddr *addr, socklen_t *addrlen)
{
    _ErrorHandle(EOPNOTSUPP);
    return  (LW_NULL);
}
/*********************************************************************************************************
** ��������: packet_connect
** ��������: connect
** �䡡��  : pafpacket afpacket file
**           name      address
**           namelen   address len
** �䡡��  : ERROR
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
INT  packet_connect (AF_PACKET_T *pafpacket, const struct sockaddr *name, socklen_t namelen)
{
    struct netif       *pnetif;
    struct sockaddr_ll *paddrll = (struct sockaddr_ll *)name;

    if (!name || (namelen < sizeof(struct sockaddr_ll))) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }

    if (paddrll->sll_hatype != ARPHRD_ETHER) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }

    if (paddrll->sll_ifindex) {
        LWIP_IF_LIST_LOCK(LW_FALSE);
        pnetif = netif_get_by_index(paddrll->sll_ifindex);
        if (!pnetif) {
            LWIP_IF_LIST_UNLOCK();
            _ErrorHandle(ENODEV);
            return  (PX_ERROR);
        }
        if (!(pnetif->flags & (NETIF_FLAG_ETHARP | NETIF_FLAG_ETHERNET))) {
            LWIP_IF_LIST_UNLOCK();
            _ErrorHandle(ENXIO);
            return  (PX_ERROR);
        }
        LWIP_IF_LIST_UNLOCK();
    }

    __AF_PACKET_LOCK();
    lib_memcpy(&pafpacket->PACKET_saddrll, paddrll, sizeof(struct sockaddr_ll));
    if (pafpacket->PACKET_saddrll.sll_ifindex == 0) {
        pafpacket->PACKET_saddrll.sll_ifindex = pafpacket->PACKET_iIfIndex;
    }
    __AF_PACKET_UNLOCK();

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: packet_recvfrom
** ��������: recvfrom
** �䡡��  : pafpacket afpacket file
**           mem       buffer
**           len       buffer len
**           flags     flag
**           from      packet from
**           fromlen   name len
**           msg_flags return flags
** �䡡��  : NUM
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static ssize_t  packet_recvfrom2 (AF_PACKET_T *pafpacket, void *mem, size_t len, int flags,
                                  struct sockaddr *from, socklen_t *fromlen, int *msg_flags)
{
    ssize_t     sstTotal = 0;
    ULONG       ulError;

    if (!mem || !len) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    
    if (from && fromlen && (*fromlen < sizeof(struct sockaddr_ll))) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    
    __AF_PACKET_LOCK();
    do {
        if (pafpacket->PACKET_iShutDFlag == __AF_PACKET_SHUTD_R) {
            __AF_PACKET_UNLOCK();
            _ErrorHandle(ENOTCONN);                                     /*  �����Ѿ��ر�                */
            return  (sstTotal);
        }
        
        if (__packetCanRead(pafpacket, flags, len)) {                   /*  ���Խ���                    */
            sstTotal = __packetBufRecv(pafpacket, mem, len, 
                                       (struct sockaddr_ll *)from, flags, msg_flags);
            if (sstTotal > 0) {
                if (fromlen) {
                    *fromlen = sizeof(struct sockaddr_ll);
                }
                break;
            }
        }
        
        if (__AF_PACKET_IS_NBIO(pafpacket, flags)) {                    /*  ������ IO                   */
            __AF_PACKET_UNLOCK();
            _ErrorHandle(EWOULDBLOCK);                                  /*  ��Ҫ���¶�                  */
            return  (sstTotal);
        }
        
        __AF_PACKET_UNLOCK();
        ulError = API_SemaphoreBPend(pafpacket->PACKET_hCanRead, 
                                     pafpacket->PACKET_ulRecvTimeout);
        if (ulError) {
            _ErrorHandle(EWOULDBLOCK);                                  /*  �ȴ���ʱ                    */
            return  (sstTotal);
        }
        __AF_PACKET_LOCK();
    } while (1);
    __AF_PACKET_UNLOCK();
    
    return  (sstTotal);
}
/*********************************************************************************************************
** ��������: packet_recvfrom
** ��������: recvfrom
** �䡡��  : pafpacket afpacket file
**           mem       buffer
**           len       buffer len
**           flags     flag
**           from      packet from
**           fromlen   name len
** �䡡��  : NUM
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
ssize_t  packet_recvfrom (AF_PACKET_T *pafpacket, void *mem, size_t len, int flags,
                          struct sockaddr *from, socklen_t *fromlen)
{
    return  (packet_recvfrom2(pafpacket, mem, len, flags, from, fromlen, LW_NULL));
}
/*********************************************************************************************************
** ��������: packet_recv
** ��������: recv
** �䡡��  : pafpacket afpacket file
**           mem       buffer
**           len       buffer len
**           flags     flag
** �䡡��  : NUM
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
ssize_t  packet_recv (AF_PACKET_T *pafpacket, void *mem, size_t len, int flags)
{
    return  (packet_recvfrom2(pafpacket, mem, len, flags, LW_NULL, LW_NULL, LW_NULL));
}
/*********************************************************************************************************
** ��������: packet_recvmsg
** ��������: packet recvmsg
** �䡡��  : pafpacket afpacket file
**           msg         ��Ϣ
**           flags       flag
** �䡡��  : ���յ����ݳ���
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
ssize_t  packet_recvmsg (AF_PACKET_T *pafpacket, struct msghdr *msg, int flags)
{
    msg->msg_flags      = 0;
    msg->msg_controllen = 0;
    
    if (msg->msg_iovlen == 1) {
        return  (packet_recvfrom2(pafpacket, msg->msg_iov->iov_base, msg->msg_iov->iov_len, flags,
                                  (struct sockaddr *)msg->msg_name, &msg->msg_namelen, &msg->msg_flags));
    
    } else {
        struct iovec    liovec, *msg_iov;
        size_t          msg_iovlen;
        unsigned int    i, totalsize;
        ssize_t         size;
        char           *lbuf;
        char           *temp;
        
        msg_iov    = msg->msg_iov;
        msg_iovlen = msg->msg_iovlen;
        
        for (i = 0, totalsize = 0; i < msg_iovlen; i++) {
            if ((msg_iov[i].iov_len == 0) || (msg_iov[i].iov_base == LW_NULL)) {
                _ErrorHandle(EINVAL);
                return  (PX_ERROR);
            }
            totalsize += (unsigned int)msg_iov[i].iov_len;
        }
        
        /*
         * TODO: �ڴ˹����б� kill ���ڴ�й©����.
         */
        lbuf = (char *)mem_malloc(totalsize);
        if (lbuf == LW_NULL) {
            _ErrorHandle(ENOMEM);
            return  (PX_ERROR);
        }
        
        liovec.iov_base = (PVOID)lbuf;
        liovec.iov_len  = (size_t)totalsize;
        
        size = packet_recvfrom2(pafpacket, liovec.iov_base, liovec.iov_len, flags, 
                                (struct sockaddr *)msg->msg_name, &msg->msg_namelen, &msg->msg_flags);
        
        temp = lbuf;
        for (i = 0; size > 0 && i < msg_iovlen; i++) {
            size_t   qty = (size_t)((size > msg_iov[i].iov_len) ? msg_iov[i].iov_len : size);
            lib_memcpy(msg_iov[i].iov_base, temp, qty);
            temp += qty;
            size -= qty;
        }
        
        mem_free(lbuf);
        
        return  (size);
    }
}
/*********************************************************************************************************
** ��������: packet_sendto
** ��������: sendto
** �䡡��  : pafpacket afpacket file
**           data      send buffer
**           size      send len
**           flags     flag
**           to        packet to
**           tolen     name len
** �䡡��  : NUM
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
ssize_t  packet_sendto (AF_PACKET_T *pafpacket, const void *data, size_t size, int flags,
                        const struct sockaddr *to, socklen_t tolen)
{
    struct sockaddr_ll *psockaddrll;
           error_t      err;
           
    if (!data || !size) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    
    if (to && (tolen >= sizeof(struct sockaddr_ll))) {
        psockaddrll = (struct sockaddr_ll *)to;
    
    } else {
        if ((pafpacket->PACKET_iType != SOCK_RAW) ||
            (pafpacket->PACKET_iIfIndex <= 0)) {
            _ErrorHandle(ENOTCONN);
            return  (PX_ERROR);
        }
        psockaddrll = LW_NULL;
    }
    
    if (pafpacket->PACKET_iShutDFlag & __AF_PACKET_SHUTD_W) {
        _ErrorHandle(ESHUTDOWN);
        return  (PX_ERROR);
    }
    
    if (pafpacket->PACKET_iType == SOCK_RAW) {                          /*  SOCK_RAW                    */
        if (psockaddrll && (psockaddrll->sll_hatype != ARPHRD_ETHER)) {
            _ErrorHandle(ENOTSUP);
            return  (PX_ERROR);
        }
        
        err = __packetEthRawSendto(pafpacket, data, size, psockaddrll);
    
    } else {                                                            /*  SOCK_DGRAM                  */
        if (psockaddrll->sll_hatype != ARPHRD_ETHER) {
            _ErrorHandle(ENOTSUP);
            return  (PX_ERROR);
        }
        
        err = __packetEthDgramSendto(pafpacket, data, size, psockaddrll);
    }
    
    if (err) {
        _ErrorHandle(err);
        return  (0);
    
    } else {
        return  ((ssize_t)size);
    }
}
/*********************************************************************************************************
** ��������: packet_sendmsg
** ��������: packet sendmsg
** �䡡��  : lwipfd      lwip �ļ�
**           msg         ��Ϣ
**           flags       flag
** �䡡��  : �������ݳ���
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
ssize_t  packet_sendmsg (AF_PACKET_T *pafpacket, const struct msghdr *msg, int flags)
{
    if (msg->msg_iovlen == 1) {
        return  (packet_sendto(pafpacket, msg->msg_iov->iov_base, msg->msg_iov->iov_len, flags,
                               (const struct sockaddr *)msg->msg_name, msg->msg_namelen));
    
    } else {
        struct iovec    liovec,*msg_iov;
        size_t          msg_iovlen;
        unsigned int    i, totalsize;
        ssize_t         size;
        char           *lbuf;
        char           *temp;
        
        msg_iov    = msg->msg_iov;
        msg_iovlen = msg->msg_iovlen;
        
        for (i = 0, totalsize = 0; i < msg_iovlen; i++) {
            if ((msg_iov[i].iov_len == 0) || (msg_iov[i].iov_base == LW_NULL)) {
                _ErrorHandle(EINVAL);
                return  (PX_ERROR);
            }
            totalsize += (unsigned int)msg_iov[i].iov_len;
        }
        
        /*
         * TODO: �ڴ˹����б� kill ���ڴ�й©����.
         */
        lbuf = (char *)mem_malloc(totalsize);
        if (lbuf == LW_NULL) {
            _ErrorHandle(ENOMEM);
            return  (PX_ERROR);
        }
        
        liovec.iov_base = (PVOID)lbuf;
        liovec.iov_len  = (size_t)totalsize;
        
        size = totalsize;
        
        temp = lbuf;
        for (i = 0; size > 0 && i < msg_iovlen; i++) {
            int     qty = msg_iov[i].iov_len;
            lib_memcpy(temp, msg_iov[i].iov_base, qty);
            temp += qty;
            size -= qty;
        }
        
        size = packet_sendto(pafpacket, liovec.iov_base, liovec.iov_len, flags, 
                             (const struct sockaddr *)msg->msg_name, msg->msg_namelen);
                           
        mem_free(lbuf);
        
        return  (size);
    }
}
/*********************************************************************************************************
** ��������: packet_send
** ��������: send
** �䡡��  : pafpacket afpacket file
**           data      send buffer
**           size      send len
**           flags     flag
** �䡡��  : ERROR
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
ssize_t  packet_send (AF_PACKET_T *pafpacket, const void *data, size_t size, int flags)
{
    struct sockaddr_ll *psockaddrll = &pafpacket->PACKET_saddrll;
    socklen_t           tolen       = sizeof(struct sockaddr_ll);

    if (psockaddrll->sll_hatype != ARPHRD_ETHER) {
        psockaddrll = LW_NULL;                                          /*  û������Ŀ��                */
        tolen = 0;
    }

    return  (packet_sendto(pafpacket, data, size, flags, (struct sockaddr *)psockaddrll, tolen));
}
/*********************************************************************************************************
** ��������: packet_close
** ��������: close
** �䡡��  : pafpacket afpacket file
** �䡡��  : ERROR
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
INT  packet_close (AF_PACKET_T *pafpacket)
{
    __packetDelete(pafpacket);
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: packet_shutdown
** ��������: shutdown
** �䡡��  : pafpacket afpacket file
**           how       SHUT_RD  SHUT_WR  SHUT_RDWR
** �䡡��  : ERROR
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
INT  packet_shutdown (AF_PACKET_T *pafpacket, int how)
{
    if ((how != SHUT_RD) && (how != SHUT_WR) && (how != SHUT_RDWR)) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    
    __AF_PACKET_LOCK();
    if (how == SHUT_RD) {
        pafpacket->PACKET_iShutDFlag |= __AF_PACKET_SHUTD_R;
        __packetBufFreeAll(pafpacket);
        __packetUpdateReader(pafpacket, ENOTCONN);                      /*  �����ҵĶ��ȴ�              */
        
    } else if (how == SHUT_WR) {                                        /*  �رձ���д                  */
        pafpacket->PACKET_iShutDFlag |= __AF_PACKET_SHUTD_W;
        
    } else {                                                            /*  �رձ��ض�д                */
        pafpacket->PACKET_iShutDFlag |= (__AF_PACKET_SHUTD_R | __AF_PACKET_SHUTD_W);
        __packetBufFreeAll(pafpacket);
        __packetUpdateReader(pafpacket, ENOTCONN);                      /*  �����ҵĶ��ȴ�              */
    }
    __AF_PACKET_UNLOCK();

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: packet_getsockname
** ��������: getsockname
** �䡡��  : pafpacket afpacket file
**           name      address
**           namelen   address len
** �䡡��  : ERROR
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
INT  packet_getsockname (AF_PACKET_T *pafpacket, struct sockaddr *name, socklen_t *namelen)
{
    error_t      err;

    if (!name || !namelen || (*namelen < sizeof(struct sockaddr_ll))) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    
    __AF_PACKET_LOCK();
    err = __packetEthIfInfo(pafpacket->PACKET_iIfIndex, (struct sockaddr_ll *)name);
    __AF_PACKET_UNLOCK();
    
    if (err) {
        _ErrorHandle(err);
        return  (PX_ERROR);
    
    } else {
        if (namelen) {
            *namelen = sizeof(struct sockaddr_ll);
        }
        return  (ERROR_NONE);
    }
}
/*********************************************************************************************************
** ��������: packet_getpeername
** ��������: getpeername
** �䡡��  : pafpacket afpacket file
**           name      address
**           namelen   address len
** �䡡��  : ERROR
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
INT  packet_getpeername (AF_PACKET_T *pafpacket, struct sockaddr *name, socklen_t *namelen)
{
    _ErrorHandle(EOPNOTSUPP);
    return  (PX_ERROR);
}
/*********************************************************************************************************
** ��������: packet_setsockopt
** ��������: setsockopt
** �䡡��  : pafpacket afpacket file
**           level     level
**           optname   option
**           optval    option value
**           optlen    option value len
** �䡡��  : ERROR
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
INT  packet_setsockopt (AF_PACKET_T *pafpacket, int level, int optname, 
                        const void *optval, socklen_t optlen)
{
    INT                     iRet = PX_ERROR;
#if LW_CFG_NET_PACKET_MMAP > 0
    struct tpacket_req     *preq;
#endif
    struct packet_mreq     *pmreq;
    
    if (!optval || optlen < sizeof(INT)) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    
    __AF_PACKET_LOCK();
    switch (level) {
    
    case SOL_PACKET:
        switch (optname) {
        
        case PACKET_ADD_MEMBERSHIP:
        case PACKET_DROP_MEMBERSHIP:
            pmreq = (struct packet_mreq *)optval;
            if (pmreq) {
                __AF_PACKET_UNLOCK();
                iRet = __packetSetMembership(pafpacket, optname, pmreq);
                __AF_PACKET_LOCK();
            } else {
                _ErrorHandle(EINVAL);
            }
            break;
        
        case PACKET_RECV_OUTPUT:
            if (*(INT *)optval) {
                pafpacket->PACKET_bRecvOut = LW_TRUE;
            } else {
                pafpacket->PACKET_bRecvOut = LW_FALSE;
            }
            iRet = ERROR_NONE;
            break;
            
#if LW_CFG_NET_PACKET_MMAP > 0
        case PACKET_RX_RING:
            preq = (struct tpacket_req *)optval;
            if (preq) {
                iRet = __packetSetRing(pafpacket, preq);
            } else {
                _ErrorHandle(EINVAL);
            }
            break;
#endif                                                                  /*  LW_CFG_NET_PACKET_MMAP > 0  */

        case PACKET_VERSION:
            switch (*(enum tpacket_versions *)optval) {
            
            case TPACKET_V1:
                pafpacket->PACKET_uiHdrLen = TPACKET_HDRLEN;
                pafpacket->PACKET_tpver    = *(enum tpacket_versions *)optval;
                iRet = ERROR_NONE;
                break;
            
            case TPACKET_V2:
                pafpacket->PACKET_uiHdrLen = TPACKET2_HDRLEN;
                pafpacket->PACKET_tpver    = *(enum tpacket_versions *)optval;
                iRet = ERROR_NONE;
                break;
                
            default:
                _ErrorHandle(EINVAL);
                break;
            }
            break;
            
        case PACKET_RESERVE:
            pafpacket->PACKET_uiReserve = *(UINT *)optval;
            iRet = ERROR_NONE;
            break;
            
        default:
            _ErrorHandle(ENOSYS);
            break;
        }
        break;
        
    case SOL_SOCKET:
        switch (optname) {
        
        case SO_RCVBUF:
            pafpacket->PACKET_stMaxBufSize = *(INT *)optval;
            if (pafpacket->PACKET_stMaxBufSize < __AF_PACKET_BUF_MIN) {
                pafpacket->PACKET_stMaxBufSize = __AF_PACKET_BUF_MIN;
            } else if (pafpacket->PACKET_stMaxBufSize > __AF_PACKET_BUF_MAX) {
                pafpacket->PACKET_stMaxBufSize = __AF_PACKET_BUF_MAX;
            }
            iRet = ERROR_NONE;
            break;
            
        case SO_RCVTIMEO:
            if (optlen == sizeof(struct timeval)) {
                pafpacket->PACKET_ulRecvTimeout = __packetTvToTicks((struct timeval *)optval);
            } else {
                pafpacket->PACKET_ulRecvTimeout = __packetMsToTicks(*(INT *)optval);
            }
            iRet = ERROR_NONE;
            break;
        
        default:
            _ErrorHandle(ENOSYS);
            break;
        }
        break;
        
    default:
        _ErrorHandle(ENOPROTOOPT);
        break;
    }
    __AF_PACKET_UNLOCK();

    return  (iRet);
}
/*********************************************************************************************************
** ��������: packet_getsockopt
** ��������: getsockopt
** �䡡��  : pafpacket afpacket file
**           level     level
**           optname   option
**           optval    option value
**           optlen    option value len
** �䡡��  : ERROR
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
INT  packet_getsockopt (AF_PACKET_T *pafpacket, int level, int optname, void *optval, socklen_t *optlen)
{
    INT                     iRet = PX_ERROR;
    struct tpacket_stats   *pstats;
    
    if (!optval || *optlen < sizeof(INT)) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    
    __AF_PACKET_LOCK();
    switch (level) {
    
    case SOL_PACKET:
        switch (optname) {
        
        case PACKET_RECV_OUTPUT:
            if (optval) {
                *(INT *)optval = pafpacket->PACKET_bRecvOut;
            }
            iRet = ERROR_NONE;
            break;
        
        case PACKET_STATISTICS:
            pstats = (struct tpacket_stats *)optval;
            if (pstats) {
                *pstats = pafpacket->PACKET_stats;
                pafpacket->PACKET_stats.tp_packets = 0;
                pafpacket->PACKET_stats.tp_drops   = 0;
                iRet = ERROR_NONE;
            } else {
                _ErrorHandle(EINVAL);
            }
            break;
            
        case PACKET_VERSION:
            if (optval) {
                *(enum tpacket_versions *)optval = pafpacket->PACKET_tpver;
                iRet = ERROR_NONE;
            } else {
                _ErrorHandle(EINVAL);
            }
            break;
            
        case PACKET_HDRLEN:
            switch (*(INT *)optval) {
            
            case TPACKET_V1:
                *(INT *)optval = sizeof(struct tpacket_hdr);
                iRet = ERROR_NONE;
                break;
            
            case TPACKET_V2:
                *(INT *)optval = sizeof(struct tpacket2_hdr);
                iRet = ERROR_NONE;
                break;
                
            default:
                _ErrorHandle(EINVAL);
                break;
            }
            break;
            
        case PACKET_RESERVE:
            if (optval) {
                *(UINT *)optval = pafpacket->PACKET_uiReserve;
                iRet = ERROR_NONE;
            } else {
                _ErrorHandle(EINVAL);
            }
            break;
            
        default:
            _ErrorHandle(ENOSYS);
            break;
        }
        break;
        
    case SOL_SOCKET:
        switch (optname) {
        
        case SO_TYPE:
            *(INT *)optval = pafpacket->PACKET_iType;
            iRet = ERROR_NONE;
            break;
        
        case SO_RCVBUF:
            *(INT *)optval = pafpacket->PACKET_stMaxBufSize;
            iRet = ERROR_NONE;
            break;
            
        case SO_RCVTIMEO:
            if (*optlen == sizeof(struct timeval)) {
                __packetTicksToTv(pafpacket->PACKET_ulRecvTimeout, (struct timeval *)optval);
            } else {
                *(INT *)optval = (INT)__packetTicksToMs(pafpacket->PACKET_ulRecvTimeout);
            }
            iRet = ERROR_NONE;
            break;
        
        default:
            _ErrorHandle(ENOSYS);
            break;
        }
        break;
        
    default:
        _ErrorHandle(ENOPROTOOPT);
        break;
    }
    __AF_PACKET_UNLOCK();

    return  (iRet);
}
/*********************************************************************************************************
** ��������: packet_ioctl
** ��������: ioctl
** �䡡��  : pafpacket afpacket file
**           iCmd      ����
**           pvArg     ����
** �䡡��  : ERROR
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
INT  packet_ioctl (AF_PACKET_T *pafpacket, INT  iCmd, PVOID  pvArg)
{
    INT     iRet = ERROR_NONE;
    
    switch (iCmd) {
    
    case FIOGETFL:
        if (pvArg) {
            *(INT *)pvArg = pafpacket->PACKET_iFlag;
        }
        break;
        
    case FIOSETFL:
        if ((INT)(LONG)pvArg & O_NONBLOCK) {
            pafpacket->PACKET_iFlag |= O_NONBLOCK;
        } else {
            pafpacket->PACKET_iFlag &= ~O_NONBLOCK;
        }
        break;
        
    case FIONREAD:
        if (pvArg) {
            *(INT *)pvArg = (INT)pafpacket->PACKET_pktq.PKTQ_stTotal;
        }
        break;
        
    case FIONBIO:
        if (pvArg && *(INT *)pvArg) {
            pafpacket->PACKET_iFlag |= O_NONBLOCK;
        } else {
            pafpacket->PACKET_iFlag &= ~O_NONBLOCK;
        }
        break;
        
    default:
        _ErrorHandle(ENOSYS);
        iRet = PX_ERROR;
        break;
    }

    return  (iRet);
}
/*********************************************************************************************************
** ��������: packet_mmap
** ��������: mmap
** �䡡��  : pafpacket afpacket file
**           pdmap     �����ڴ�����
** �䡡��  : ERROR
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
INT  packet_mmap (AF_PACKET_T *pafpacket, PLW_DEV_MMAP_AREA  pdmap)
{
#if LW_CFG_NET_PACKET_MMAP > 0
    addr_t  ulPhysical;
    ULONG   ulFlag = LW_VMM_FLAG_RDWR;

    if (pafpacket->PACKET_bMmap == LW_FALSE) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    
#if LW_CFG_CACHE_EN > 0
    if (API_CacheAliasProb()) {                                         /*  �������� CACHE ��������     */
        ulFlag &= ~(LW_VMM_FLAG_CACHEABLE | LW_VMM_FLAG_WRITETHROUGH);  /*  �����ڴ治�����κ���ʽ CACHE*/
    }
#endif                                                                  /*  LW_CFG_CACHE_EN > 0         */
    
    __AF_PACKET_LOCK();
    if (pafpacket->PACKET_bMapBusy) {
        __AF_PACKET_UNLOCK();
        _ErrorHandle(EBUSY);
        return  (PX_ERROR);
    }
    pafpacket->PACKET_bMapBusy = LW_TRUE;                               /*  ��ʼ����ӳ������            */
    __AF_PACKET_UNLOCK();
    
    ulPhysical  = (addr_t)pafpacket->PACKET_mmapRx.PKTB_pvPhyMem;
    ulPhysical += (addr_t)(pdmap->DMAP_offPages << LW_CFG_VMM_PAGE_SHIFT);
    
    if (API_VmmRemapArea(pdmap->DMAP_pvAddr, (PVOID)ulPhysical, 
                         pdmap->DMAP_stLen, ulFlag, 
                         LW_NULL, LW_NULL)) {
        KN_SMP_MB();
        pafpacket->PACKET_bMapBusy = LW_FALSE;
        return  (PX_ERROR);
    }
    
    KN_SMP_MB();
    pafpacket->PACKET_bMapBusy = LW_FALSE;
    
    return  (ERROR_NONE);
#else
    _ErrorHandle(ENOSYS);
    return  (PX_ERROR);
#endif                                                                  /*  LW_CFG_NET_PACKET_MMAP > 0  */
}
/*********************************************************************************************************
** ��������: packet_unmap
** ��������: unmap
** �䡡��  : pafpacket afpacket file
**           pdmap     �����ڴ�����
** �䡡��  : ERROR
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
INT  packet_unmap (AF_PACKET_T *pafpacket, PLW_DEV_MMAP_AREA  pdmap)
{
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: __packet_have_event
** ��������: ����Ӧ�Ŀ��ƿ��Ƿ���ָ�����¼�
** �䡡��  : pafunix   unix file
**           type      �¼�����
**           piSoErr   ����ȴ����¼���Ч����� SO_ERROR
** �䡡��  : ERROR
** ȫ�ֱ���: 
** ����ģ��: 
** ˵  ��  :

SELREAD:
    1>.�׽ӿ������ݿɶ�.
    2>.�����ӵĶ���һ��ر� (Ҳ���ǽ�����FIN��TCP����). ���������׽ӿڽ��ж�������������������0.
    3>.���׽ӿ���һ�������׽ӿ�������ɵ���������Ϊ0. 
    4>.������һ���׽ӿڴ�����������������׽ӿڵĶ�������������������-1��������errno,
       ����ͨ������ SO_ERROR ѡ����� getsockopt �������.
       
SELWRITE:
    1>.�׽ӿ��п�����д�Ŀռ�.
    2>.�����ӵ�д��һ��رգ����������׽ӿڽ���д����������SIGPIPE�ź�.
    3>.���׽ӿ�ʹ�÷������ķ�ʽconnect�������ӣ����������Ѿ��첽����������connect�Ѿ���ʧ�ܸ���.
    4>.������һ���׽ӿڴ��������.
    
SELEXCEPT
    1>.�������ӵ��׽ӿ�û������.
*********************************************************************************************************/
int __packet_have_event (AF_PACKET_T *pafpacket, int type, int  *piSoErr)
{
    INT     iEvent = 0;

    switch (type) {

    case SELREAD:                                                       /*  �Ƿ�ɶ�                    */
        if (__packetCanRead(pafpacket, 0, 0)) {
            *piSoErr = ERROR_NONE;
            iEvent   = 1;
        }
        break;
        
    case SELWRITE:                                                      /*  �Ƿ��д                    */
        if (__packetCanWrite(pafpacket)) {
            *piSoErr = ERROR_NONE;
            iEvent   = 1;
        }
        break;
        
    case SELEXCEPT:                                                     /*  �Ƿ��쳣                    */
        break;
    }
    
    return  (iEvent);
}
/*********************************************************************************************************
** ��������: __packet_set_sockfile
** ��������: ���ö�Ӧ�� socket �ļ�
** �䡡��  : pafunix   unix file
**           file      �ļ�
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
void  __packet_set_sockfile (AF_PACKET_T *pafpacket, void *file)
{
    pafpacket->PACKET_sockFile = file;
}

#endif                                                                  /*  LW_CFG_NET_EN               */
/*********************************************************************************************************
  END
*********************************************************************************************************/
