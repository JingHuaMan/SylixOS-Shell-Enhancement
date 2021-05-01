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
** ��   ��   ��: lwip_ppp.c
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2014 �� 01 �� 08 ��
**
** ��        ��: lwip ppp ���ӹ�����.

** BUG:
2020.06.23  ���� PPPoS ���ջ���.
*********************************************************************************************************/
#define  __SYLIXOS_STDIO
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
#include "../SylixOS/system/include/s_system.h"
#include "../SylixOS/net/include/net_net.h"
/*********************************************************************************************************
  �ü�����
*********************************************************************************************************/
#if (LW_CFG_NET_EN > 0) && (LW_CFG_LWIP_PPP > 0)
#include "lwip/snmp.h"
#include "lwip/netif.h"
#include "netif/ppp/pppapi.h"
#include "netif/ppp/pppos.h"
#include "lwip_ppp.h"
#include "net/if_lock.h"
#include "net/if_event.h"
#include "net/if_ether.h"
/*********************************************************************************************************
  �����ӡ
*********************************************************************************************************/
#ifndef printk
#define printk
#define KERN_ERR
#endif                                                                  /*  printk                      */
/*********************************************************************************************************
  Ĭ�ϵ� PPPoS ��������С
*********************************************************************************************************/
#define PPPRBUF_SIZE    LW_CFG_LWIP_PPP_RBUF_SIZE
#define PPPWBUF_SIZE    LW_CFG_LWIP_PPP_WBUF_SIZE
/*********************************************************************************************************
  PPP ˽������
*********************************************************************************************************/
typedef struct {
#define PPP_OS          0
#define PPP_OE          1
#define PPP_OL2TP       2
    UINT                CTXP_uiType;                                    /*  ��������                    */
    LW_OBJECT_HANDLE    CTXP_ulInput;                                   /*  pppos �����߳�              */
    BOOL                CTXP_bNeedDelete;                               /*  �Ƿ���Ҫɾ��                */
    UINT8               CTXP_ucPhase;                                   /*  ���һ��״̬                */
    PCHAR               CTXP_cUser;                                     /*  �����û���                  */
    PCHAR               CTXP_cPass;                                     /*  ��������                    */
    INT                 CTXP_iFd;                                       /*  �����ļ�������              */
    ppp_pcb            *CTXP_pcb;                                       /*  ��ָ�����ӿ��ƿ�            */
    struct netif        CTXP_netif;                                     /*  ����ӿ�                    */
} PPP_CTX_PRIV;
/*********************************************************************************************************
** ��������: __pppLinkStatCb
** ��������: ����������ص�
** �䡡��  : pcb           pcb ���ƿ�
**           iErrCode      ������
**           pvCtx         PPP_CTX_PRIV �ṹ
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static VOID  __pppLinkStatCb (ppp_pcb *pcb, INT iErrCode, PVOID  pvCtx)
{
    switch (iErrCode) {

    case PPPERR_NONE:
        break;

    case PPPERR_PARAM:
        printk(KERN_ERR "[PPP]PPPERR_PARAM\n");
        break;

    case PPPERR_OPEN:
        printk(KERN_ERR "[PPP]PPPERR_OPEN\n");
        break;

    case PPPERR_DEVICE:
        printk(KERN_ERR "[PPP]PPPERR_DEVICE\n");
        break;

    case PPPERR_ALLOC:
        printk(KERN_ERR "[PPP]PPPERR_ALLOC\n");
        break;

    case PPPERR_USER:
        printk(KERN_ERR "[PPP]PPPERR_USER\n");
        break;

    case PPPERR_CONNECT:
        printk(KERN_ERR "[PPP]PPPERR_CONNECT\n");
        break;

    case PPPERR_AUTHFAIL:
        netEventIfAuthFail(ppp_netif(pcb));
        printk(KERN_ERR "[PPP]PPPERR_AUTHFAIL\n");
        break;

    case PPPERR_PROTOCOL:
        printk(KERN_ERR "[PPP]PPPERR_PROTOCOL\n");
        break;

    case PPPERR_PEERDEAD:
        netEventIfPppExt(ppp_netif(pcb), NET_EVENT_PPP_TIMEOUT);
        printk(KERN_ERR "[PPP]PPPERR_PEERDEAD\n");
        break;

    case PPPERR_IDLETIMEOUT:
        printk(KERN_ERR "[PPP]PPPERR_IDLETIMEOUT\n");
        break;

    case PPPERR_CONNECTTIME:
        printk(KERN_ERR "[PPP]PPPERR_CONNECTTIME\n");
        break;

    case PPPERR_LOOPBACK:
        printk(KERN_ERR "[PPP]PPPERR_LOOPBACK\n");
        break;

    default:
        printk(KERN_ERR "[PPP]PPPERR_<unknown>\n");
        break;
    }
}
/*********************************************************************************************************
** ��������: __pppNotifyPhaseCb
** ��������: ״̬�ı�ص�
** �䡡��  : pcb           pcb ���ƿ�
**           ucPhase       ״̬��
**           pvCtx         PPP_CTX_PRIV �ṹ
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static VOID  __pppNotifyPhaseCb (ppp_pcb *pcb, u8_t ucPhase, PVOID  pvCtx)
{
    PPP_CTX_PRIV *pctxp;

    pctxp = (PPP_CTX_PRIV *)pcb->ctx_cb;
    if (pctxp->CTXP_ucPhase == ucPhase) {
        return;
    } else {
        pctxp->CTXP_ucPhase = ucPhase;
    }

    switch (ucPhase) {

    case PPP_PHASE_DEAD:
        netEventIfPppExt(ppp_netif(pcb), NET_EVENT_PPP_DEAD);
        break;

    case PPP_PHASE_INITIALIZE:
        netEventIfPppExt(ppp_netif(pcb), NET_EVENT_PPP_INIT);
        break;

    case PPP_PHASE_SERIALCONN:
        break;

    case PPP_PHASE_DORMANT:
        break;

    case PPP_PHASE_ESTABLISH:
        break;

    case PPP_PHASE_AUTHENTICATE:
        netEventIfPppExt(ppp_netif(pcb), NET_EVENT_PPP_AUTH);
        break;

    case PPP_PHASE_CALLBACK:
        break;

    case PPP_PHASE_NETWORK:
        break;

    case PPP_PHASE_RUNNING:
        netEventIfPppExt(ppp_netif(pcb), NET_EVENT_PPP_RUN);
        break;

    case PPP_PHASE_TERMINATE:
        break;

    case PPP_PHASE_DISCONNECT:
        netEventIfPppExt(ppp_netif(pcb), NET_EVENT_PPP_DISCONN);
        break;

    case PPP_PHASE_HOLDOFF:
        break;

    case PPP_PHASE_MASTER:
        break;

    default:
        printk(KERN_ERR "[PPP]PHASE_<unknown>\n");
        break;
    }
}
/*********************************************************************************************************
** ��������: __pppOsThread
** ��������: PPPoS �����߳�
** �䡡��  : pcb           pcb ���ƿ�
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static VOID  __pppOsThread (ppp_pcb *pcb)
{
    PPP_CTX_PRIV *pctxp;
    size_t        stBufSz = PPPRBUF_SIZE >> 2;                          /*  ���ջ��� 1/4                */
    ssize_t       sstRead;
    UCHAR         ucStatic[ETH_FRAME_LEN];
    PUCHAR        pucBuffer;

    pctxp     = (PPP_CTX_PRIV *)pcb->ctx_cb;
    pucBuffer = (PUCHAR)__SHEAP_ALLOC(stBufSz);
    if (!pucBuffer) {
        pucBuffer = ucStatic;
        stBufSz   = sizeof(ucStatic);
    }

    for (;;) {
        sstRead = read(pctxp->CTXP_iFd, pucBuffer, stBufSz);
        if (sstRead > 0) {
            pppos_input(pcb, pucBuffer, (INT)sstRead);

        } else {
            if (iosFdGetType(pctxp->CTXP_iFd, LW_NULL) < ERROR_NONE) {
                if (pcb->phase != PPP_PHASE_DEAD) {
                    pppapi_close(pcb, 1);                               /*  ǿ�ƹر� ppp ����           */
                }
                pctxp->CTXP_bNeedDelete = LW_TRUE;
            }
        }

        if ((pctxp->CTXP_bNeedDelete) &&
            (pcb->phase == PPP_PHASE_DEAD)) {                           /*  ��Ҫɾ�� PPP ����           */
            LWIP_IF_LIST_LOCK(LW_TRUE);
            pppapi_free(pcb);
            LWIP_IF_LIST_UNLOCK();

            if (pctxp->CTXP_iFd >= 0) {
                close(pctxp->CTXP_iFd);
            }
            if (pctxp->CTXP_cUser) {
                lib_free(pctxp->CTXP_cUser);
            }
            if (pctxp->CTXP_cPass) {
                lib_free(pctxp->CTXP_cPass);
            }
            __SHEAP_FREE(pctxp);
            if (pucBuffer != ucStatic) {
                __SHEAP_FREE(pucBuffer);
            }
            break;
        }
    }

    LW_THREAD_UNSAFE();                                                 /*  �˳���ȫģʽ                */
}
/*********************************************************************************************************
** ��������: __pppOsOutput
** ��������: PPPoS ���ͺ���
** �䡡��  : pcb           pcb ���ƿ�
**           data          Ҫ���͵�����
**           len           ���ͳ���
**           ctx           �����Ĳ���
** �䡡��  : ��������
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static u32_t  __pppOsOutput (ppp_pcb *pcb, u8_t *data, u32_t len, void *ctx)
{
    PPP_CTX_PRIV   *pctxp = (PPP_CTX_PRIV *)ctx;
    u32_t           uiRet;

    if (!pcb || !data || !ctx) {
        return  (PX_ERROR);
    }

    if (!len) {
        return  (0);
    }

    __KERNEL_SPACE_ENTER();
    uiRet = (u32_t)write(pctxp->CTXP_iFd, data, len);
    __KERNEL_SPACE_EXIT();

    return  (uiRet);
}
/*********************************************************************************************************
** ��������: __pppGet
** ��������: ͨ����������ȡ PPP ���ƿ�
** �䡡��  : pcb           pcb ���ƿ�
**           ucPhase       ״̬��
**           pvCtx         PPP_CTX_PRIV �ṹ
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static ppp_pcb  *__pppGet (CPCHAR  pcIfName)
{
    ppp_pcb       *pcb;
    struct netif  *netif;

    if ((pcIfName[0] != 'p') ||
        (pcIfName[1] != 'p')) {
        _ErrorHandle(ENODEV);
        return  (LW_NULL);
    }

    netif = netif_find(pcIfName);
    if ((netif == LW_NULL) || (netif->link_type != snmp_ifType_ppp)) {
        _ErrorHandle(ENODEV);
        return  (LW_NULL);
    }

    pcb = (ppp_pcb *)netif->state;

    return  (pcb);
}
/*********************************************************************************************************
** ��������: API_PppOsCreate
** ��������: ����һ�� PPPoS ����������
** �䡡��  : pcSerial      ʹ�õĴ��нӿ��豸��
**           ptty          ���ڹ�������
**           pcIfName      ��������ɹ�, �򷵻������豸����
**           stMaxSize     ifname ��������С
** �䡡��  : PPP ���������Ƿ�װ�ɹ�
** ȫ�ֱ���:
** ����ģ��:
                                           API ����
*********************************************************************************************************/
LW_API 
INT  API_PppOsCreate (CPCHAR  pcSerial, LW_PPP_TTY  *ptty, PCHAR  pcIfName, size_t  stMaxSize)
{
    INT             iErrLevel = 0;
    INT             iFd;
    PPP_CTX_PRIV   *pctxp;
    
    if (!pcSerial || !pcIfName || (stMaxSize < IF_NAMESIZE)) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    
    pctxp = (PPP_CTX_PRIV *)__SHEAP_ALLOC(sizeof(PPP_CTX_PRIV));
    if (pctxp == LW_NULL) {
        _ErrorHandle(ENOMEM);
        return  (PX_ERROR);
    }
    lib_bzero(pctxp, sizeof(PPP_CTX_PRIV));
    
    __KERNEL_SPACE_ENTER();                                             /*  �ļ�Ϊ�ں� IO �ļ�          */
    iFd = open(pcSerial, O_RDWR);
    if (iFd < 0) {
        __KERNEL_SPACE_EXIT();
        iErrLevel = 1;
        goto    __error_handle;
    }
    
    if (!isatty(iFd)) {
        __KERNEL_SPACE_EXIT();
        _ErrorHandle(ENOTTY);
        iErrLevel = 1;
        goto    __error_handle;
    }
    
    ioctl(iFd, FIOSETOPTIONS, OPT_RAW);
    ioctl(iFd, FIORTIMEOUT,   LW_NULL);
    ioctl(iFd, FIOWTIMEOUT,   LW_NULL);
    ioctl(iFd, FIORBUFSET,    LW_OSIOD_LARG(PPPRBUF_SIZE));
    ioctl(iFd, FIOWBUFSET,    LW_OSIOD_LARG(PPPWBUF_SIZE));

    if (ptty) {                                                         /*  ���ô��ڲ���                */
        INT  iHwOpt = CREAD | CS8 | HUPCL;
        if (ptty->stop_bits == 2) {
            iHwOpt |= STOPB;
        }
        if (ptty->parity) {
            iHwOpt |= PARENB;
            if (ptty->parity == 1) {
                iHwOpt |= PARODD;
            }
        }
        ioctl(iFd, FIOBAUDRATE,     LW_OSIOD_LARG(ptty->baud));
        ioctl(iFd, SIO_HW_OPTS_SET, iHwOpt);
    }
    __KERNEL_SPACE_EXIT();
    
    pctxp->CTXP_uiType      = PPP_OS;
    pctxp->CTXP_ulInput     = LW_OBJECT_HANDLE_INVALID;
    pctxp->CTXP_bNeedDelete = LW_FALSE;
    pctxp->CTXP_ucPhase     = PPP_PHASE_DEAD;
    pctxp->CTXP_iFd         = iFd;

    pctxp->CTXP_pcb = pppapi_pppos_create(&pctxp->CTXP_netif, __pppOsOutput, __pppLinkStatCb, pctxp);
    if (pctxp->CTXP_pcb == LW_NULL) {
        _ErrorHandle(ENODEV);
        iErrLevel = 2;
        goto    __error_handle;
    }

    pppapi_set_notify_phase_callback(pctxp->CTXP_pcb, __pppNotifyPhaseCb);
    
    netif_get_name(&pctxp->CTXP_netif, pcIfName);
    
    return  (ERROR_NONE);
    
__error_handle:
    if (iErrLevel > 1) {
        close(iFd);
    }
    if (iErrLevel > 0) {
        __SHEAP_FREE(pctxp);
    }
    
    return  (PX_ERROR);
}
/*********************************************************************************************************
** ��������: API_PppOeCreate
** ��������: ����һ�� PPPoE ����������
** �䡡��  : pcEthIf       ʹ�õ���̫��������
**           pcIfName      ��������ɹ�, �򷵻������豸����
**           stMaxSize     ifname ��������С
** �䡡��  : PPP ���������Ƿ�װ�ɹ�
** ȫ�ֱ���:
** ����ģ��:
                                           API ����
*********************************************************************************************************/
LW_API 
INT  API_PppOeCreate (CPCHAR  pcEthIf, PCHAR  pcIfName, size_t  stMaxSize)
{
    INT             iErrLevel = 0;
    PPP_CTX_PRIV   *pctxp;
    struct netif   *netif;

    if (!pcEthIf || !pcIfName || (stMaxSize < IF_NAMESIZE)) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }

    pctxp = (PPP_CTX_PRIV *)__SHEAP_ALLOC(sizeof(PPP_CTX_PRIV));
    if (pctxp == LW_NULL) {
        _ErrorHandle(ENOMEM);
        return  (PX_ERROR);
    }
    lib_bzero(pctxp, sizeof(PPP_CTX_PRIV));

    netif = netif_find(pcEthIf);
    if ((netif == LW_NULL) || ((netif->flags & (NETIF_FLAG_ETHARP | NETIF_FLAG_ETHERNET)) == 0)) {
        _ErrorHandle(ENODEV);
        iErrLevel = 1;
        goto    __error_handle;
    }

    pctxp->CTXP_uiType      = PPP_OE;
    pctxp->CTXP_ulInput     = LW_OBJECT_HANDLE_INVALID;
    pctxp->CTXP_bNeedDelete = LW_FALSE;
    pctxp->CTXP_ucPhase     = PPP_PHASE_DEAD;
    pctxp->CTXP_iFd         = PX_ERROR;

    pctxp->CTXP_pcb = pppapi_pppoe_create(&pctxp->CTXP_netif, netif,
                                          LW_NULL, LW_NULL, __pppLinkStatCb, pctxp);
    if (pctxp->CTXP_pcb == LW_NULL) {
        _ErrorHandle(ENODEV);
        iErrLevel = 1;
        goto    __error_handle;
    }

    pppapi_set_notify_phase_callback(pctxp->CTXP_pcb, __pppNotifyPhaseCb);

    netif_get_name(&pctxp->CTXP_netif, pcIfName);

    return  (ERROR_NONE);

__error_handle:
    if (iErrLevel > 0) {
        __SHEAP_FREE(pctxp);
    }

    return  (PX_ERROR);
}
/*********************************************************************************************************
** ��������: API_PppOl2tpCreate
** ��������: ����һ�� PPPoL2TP ����������
** �䡡��  : pcEthIf       ʹ�õ���̫��������
**           pcIp          ��������ַ
**           usPort        �������˿�
**           pcSecret      ��ȫ��Կ
**           stSecretLen   ��ȫ��Կ����
**           pcIfName      ��������ɹ�, �򷵻������豸����
**           stMaxSize     ifname ��������С
** �䡡��  : PPP ���������Ƿ�װ�ɹ�
** ȫ�ֱ���:
** ����ģ��:
                                           API ����
*********************************************************************************************************/
LW_API 
INT  API_PppOl2tpCreate (CPCHAR  pcEthIf, 
                         CPCHAR  pcIp,
                         UINT16  usPort,
                         CPCHAR  pcSecret,
                         size_t  stSecretLen,
                         PCHAR   pcIfName, 
                         size_t  stMaxSize)
{
    INT             iErrLevel = 0;
    UINT8          *pucSecret;
    PPP_CTX_PRIV   *pctxp;
    struct netif   *netif;
    ip_addr_t       ipaddr;

    if (!pcIp || !usPort || !pcIfName || (stMaxSize < IF_NAMESIZE)) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }

    if (!ipaddr_aton(pcIp, &ipaddr)) {                                  /*  ��������ַ��Ч              */
        _ErrorHandle(EADDRNOTAVAIL);
        return  (PX_ERROR);
    }

    if (pcSecret && stSecretLen) {
        pctxp = (PPP_CTX_PRIV *)__SHEAP_ALLOC(sizeof(PPP_CTX_PRIV) + stSecretLen);
    } else {
        pctxp = (PPP_CTX_PRIV *)__SHEAP_ALLOC(sizeof(PPP_CTX_PRIV));
    }
    if (pctxp == LW_NULL) {
        _ErrorHandle(ENOMEM);
        return  (PX_ERROR);
    }
    lib_bzero(pctxp, sizeof(PPP_CTX_PRIV));

    if (pcEthIf) {
        netif = netif_find(pcEthIf);
        if (netif == LW_NULL) {
            _ErrorHandle(ENODEV);
            iErrLevel = 1;
            goto    __error_handle;
        }
    } else {
        netif = LW_NULL;                                                /*  ��ָ����������ӿ�          */
    }

    pctxp->CTXP_uiType      = PPP_OL2TP;
    pctxp->CTXP_ulInput     = LW_OBJECT_HANDLE_INVALID;
    pctxp->CTXP_bNeedDelete = LW_FALSE;
    pctxp->CTXP_ucPhase     = PPP_PHASE_DEAD;
    pctxp->CTXP_iFd         = PX_ERROR;

    if (pcSecret && stSecretLen) {
        pucSecret = (UINT8 *)pctxp + sizeof(PPP_CTX_PRIV);
        lib_memcpy(pucSecret, pcSecret, stSecretLen);
    } else {
        pucSecret   = LW_NULL;
        stSecretLen = 0;
    }

    pctxp->CTXP_pcb = pppapi_pppol2tp_create(&pctxp->CTXP_netif, netif, &ipaddr, ntohs(usPort),
                                             pucSecret, (u8_t)stSecretLen, __pppLinkStatCb, pctxp);
    if (pctxp->CTXP_pcb == LW_NULL) {
        _ErrorHandle(ENODEV);
        iErrLevel = 1;
        goto    __error_handle;
    }

    pppapi_set_notify_phase_callback(pctxp->CTXP_pcb, __pppNotifyPhaseCb);

    netif_get_name(&pctxp->CTXP_netif, pcIfName);

    return  (ERROR_NONE);

__error_handle:
    if (iErrLevel > 0) {
        __SHEAP_FREE(pctxp);
    }

    return  (PX_ERROR);
}
/*********************************************************************************************************
** ��������: API_PppDelete
** ��������: ɾ��һ�� PPP ����������
** �䡡��  : pcIfName      �����豸����
** �䡡��  : PPP ����ɾ���Ƿ�ɹ�
** ȫ�ֱ���:
** ����ģ��:
                                           API ����
*********************************************************************************************************/
LW_API 
INT  API_PppDelete (CPCHAR  pcIfName)
{
    ppp_pcb      *pcb;
    PPP_CTX_PRIV *pctxp;

    if (!pcIfName) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }

    pcb = __pppGet(pcIfName);
    if (pcb == LW_NULL) {
        return  (PX_ERROR);
    }

    if (pcb->phase != PPP_PHASE_DEAD) {                                 /*  �������볹�׹رղ���ɾ��    */
        _ErrorHandle(EBUSY);
        return  (PX_ERROR);
    }

    pctxp = (PPP_CTX_PRIV *)pcb->ctx_cb;
    if (pctxp->CTXP_ulInput) {                                          /*  PPPoS ������                */
        pctxp->CTXP_bNeedDelete = LW_TRUE;
        __KERNEL_SPACE_ENTER();
        ioctl(pctxp->CTXP_iFd, FIOCANCEL);
        __KERNEL_SPACE_EXIT();

    } else {
        LWIP_IF_LIST_LOCK(LW_TRUE);
        pppapi_free(pcb);                                               /*  ֱ��ɾ��                    */
        LWIP_IF_LIST_UNLOCK();

        if (pctxp->CTXP_cUser) {
            lib_free(pctxp->CTXP_cUser);
        }
        if (pctxp->CTXP_cPass) {
            lib_free(pctxp->CTXP_cPass);
        }
        __SHEAP_FREE(pctxp);
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: API_PppConnect
** ��������: ��������
** �䡡��  : pcIfName      �����豸����
**           pdial         ���Ų���
** �䡡��  : ���Ž��
** ȫ�ֱ���:
** ����ģ��:
                                           API ����
*********************************************************************************************************/
LW_API 
INT  API_PppConnect (CPCHAR  pcIfName, LW_PPP_DIAL *pdial)
{
    PPP_CTX_PRIV        *pctxp;
    ppp_pcb             *pcb;
    LW_CLASS_THREADATTR  attr;

    if (!pcIfName) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }

    pcb = __pppGet(pcIfName);
    if (pcb == LW_NULL) {
        return  (PX_ERROR);
    }

    if ((pcb->phase != PPP_PHASE_DEAD) &&
        (pcb->phase != PPP_PHASE_HOLDOFF)) {                            /*  ��������״̬, ���ܲ���      */
        _ErrorHandle(EBUSY);
        return  (PX_ERROR);
    }

    pctxp = (PPP_CTX_PRIV *)pcb->ctx_cb;

    if (pdial && pdial->user && pdial->passwd) {
        if (pctxp->CTXP_cUser) {
            if (lib_strcmp(pctxp->CTXP_cUser, pdial->user) == 0) {
                goto    __user_same;
            }
            lib_free(pctxp->CTXP_cUser);
        }
        pctxp->CTXP_cUser = lib_strdup(pdial->user);
        if (pctxp->CTXP_cUser == LW_NULL) {
            _ErrorHandle(ENOMEM);
            return  (PX_ERROR);
        }
        
__user_same:
        if (pctxp->CTXP_cPass) {
            if (lib_strcmp(pctxp->CTXP_cPass, pdial->passwd) == 0) {
                goto    __passwd_same;
            }
            lib_free(pctxp->CTXP_cPass);
        }
        pctxp->CTXP_cPass = lib_strdup(pdial->passwd);
        if (pctxp->CTXP_cPass == LW_NULL) {
            _ErrorHandle(ENOMEM);
            return  (PX_ERROR);
        }

__passwd_same:
        ppp_set_auth(pcb, PPPAUTHTYPE_ANY, pctxp->CTXP_cUser, pctxp->CTXP_cPass);
    
    } else {
        ppp_set_auth(pcb, PPPAUTHTYPE_ANY, LW_NULL, LW_NULL);
    }

    if ((pctxp->CTXP_uiType  == PPP_OS) &&
        (pctxp->CTXP_ulInput == LW_OBJECT_HANDLE_INVALID)) {
        API_ThreadAttrBuild(&attr, LW_CFG_LWIP_DEF_STK_SIZE, LW_PRIO_T_NETPROTO,
                            LW_OPTION_THREAD_STK_CHK |
                            LW_OPTION_THREAD_SAFE |
                            LW_OPTION_OBJECT_GLOBAL, (PVOID)pcb);
        pctxp->CTXP_ulInput = API_ThreadCreate("t_pppos",
                            (PTHREAD_START_ROUTINE)__pppOsThread,
                            &attr, LW_NULL);                            /*  ���� PPPoS �����߳�         */
    }

    pppapi_connect(pcb, 0);                                             /*  ��������                    */

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: API_PppDisconnect
** ��������: �Ҷ�����
** �䡡��  : pcIfName      �����豸����
**           bForce        �Ƿ�ǿ�ƹҶ�
** �䡡��  : �ҶϽ��
** ȫ�ֱ���:
** ����ģ��:
                                           API ����
*********************************************************************************************************/
LW_API 
INT  API_PppDisconnect (CPCHAR  pcIfName, BOOL  bForce)
{
    ppp_pcb      *pcb;

    if (!pcIfName) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }

    pcb = __pppGet(pcIfName);
    if (pcb == LW_NULL) {
        return  (PX_ERROR);
    }

    if (pcb->phase == PPP_PHASE_DEAD) {
        return  (ERROR_NONE);
    }

    if (bForce) {
        pppapi_close(pcb, 1);                                           /*  ǿ���ж� PPP ����           */

    } else {
        pppapi_close(pcb, 0);
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: API_PppGetPhase
** ��������: ��õ�ǰ����״̬
** �䡡��  : pcIfName      �����豸����
**           piPhase       ����״̬
** �䡡��  : OK or ERROR
** ȫ�ֱ���:
** ����ģ��:
                                           API ����
*********************************************************************************************************/
LW_API 
INT  API_PppGetPhase (CPCHAR  pcIfName, INT  *piPhase)
{
    ppp_pcb      *pcb;

    if (!pcIfName || !piPhase) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }

    pcb = __pppGet(pcIfName);
    if (pcb == LW_NULL) {
        return  (PX_ERROR);
    }

    *piPhase = pcb->phase;
    
    return  (ERROR_NONE);
}

#endif                                                                  /*  LW_CFG_NET_EN > 0           */
                                                                        /*  LW_CFG_LWIP_PPP > 0         */
/*********************************************************************************************************
  END
*********************************************************************************************************/
