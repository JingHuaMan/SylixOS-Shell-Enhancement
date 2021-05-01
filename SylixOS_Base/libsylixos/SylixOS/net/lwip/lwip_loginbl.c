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
** ��   ��   ��: lwip_loginbl.c
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2017 �� 01 �� 09 ��
**
** ��        ��: �����¼����������.
**
** BUG:
2017.04.10  ���Ӱ���������.
2017.12.28  �������ܲ����ľܾ����񹥻�ȱ��.
*********************************************************************************************************/
#define  __SYLIXOS_STDIO
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
#include "../SylixOS/system/include/s_system.h"
#include "lwip/opt.h"
/*********************************************************************************************************
  �ü�����
*********************************************************************************************************/
#if LW_CFG_NET_EN > 0
#include "sys/socket.h"
#if LW_CFG_NET_LOGINBL_EN > 0
#include "lwip/tcpip.h"
#include "lwip/pbuf.h"
#include "lwip/ip4.h"
/*********************************************************************************************************
  �������ڵ�
*********************************************************************************************************/
typedef struct {
    LW_LIST_LINE    LBL_lineManage;                                     /*  management list             */
    ip4_addr_t      LBL_ipaddr;
    UINT            LBL_uiRep;                                          /*  ��Ч����                    */
    UINT            LBL_uiCnt;                                          /*  �ظ�����                    */
    UINT            LBL_uiSec;                                          /*  ��ʱʱ��                    */
} LW_LOGINBL_NODE;
typedef LW_LOGINBL_NODE     *PLW_LOGINBL_NODE;
/*********************************************************************************************************
  ȫ�ֱ���
*********************************************************************************************************/
#define LW_LBL_TABLESIZE    64                                          /*  2 ^ 6 �����                */
#define LW_LBL_TABLEMASK    (LW_LBL_TABLESIZE - 1)
#define LW_LBL_TABLESHIFT   (32 - 6)                                    /*  ʹ�� ip ��ַ�� 6 λΪ hash  */

#define LW_LBL_HASHINDEX(pipaddr)    \
        (((pipaddr)->addr >> LW_LBL_TABLESHIFT) & LW_LBL_TABLEMASK)
        
static PLW_LIST_LINE    _G_plineLblHashHeader[LW_LBL_TABLESIZE];
static PLW_LIST_LINE    _G_plineLwlHashHeader[LW_LBL_TABLESIZE];
static LW_OBJECT_HANDLE _G_ulLblTimer = LW_OBJECT_HANDLE_INVALID;
static UINT             _G_uiLblCnt   = 0;
static UINT             _G_uiLwlCnt   = 0;

#if LWIP_TCPIP_CORE_LOCKING < 1
#error sylixos need LWIP_TCPIP_CORE_LOCKING > 0
#endif                                                                  /*  LWIP_TCPIP_CORE_LOCKING     */
/*********************************************************************************************************
** ��������: loginbl_input_hook
** ��������: �����ַ������ ip ����ص�
** �䡡��  : p         ���ݰ�
**           inp       ����ӿ�
** �䡡��  : 1: ������ͨ��  0: ����
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
INT  loginbl_input_hook (struct pbuf *p, struct netif *inp)
{
    struct ip_hdr      *iphdr = (struct ip_hdr *)p->payload;
    INT                 iHash = LW_LBL_HASHINDEX(&iphdr->src);
    PLW_LIST_LINE       plineTemp;
    PLW_LOGINBL_NODE    plbl;
    
    if (IPH_V(iphdr) != 4) {
        return  (0);                                                    /*  IPv4 ��Ч                   */
    }
    
    for (plineTemp  = _G_plineLwlHashHeader[iHash];
         plineTemp != LW_NULL;
         plineTemp  = _list_line_get_next(plineTemp)) {
        
        plbl = _LIST_ENTRY(plineTemp, LW_LOGINBL_NODE, LBL_lineManage);
        if (plbl->LBL_ipaddr.addr == iphdr->src.addr) {
            return  (0);                                                /*  �ڰ�������                  */
        }
    }
    
    for (plineTemp  = _G_plineLblHashHeader[iHash];
         plineTemp != LW_NULL;
         plineTemp  = _list_line_get_next(plineTemp)) {
         
        plbl = _LIST_ENTRY(plineTemp, LW_LOGINBL_NODE, LBL_lineManage);
        if (plbl->LBL_ipaddr.addr == iphdr->src.addr) {
            if (plbl->LBL_uiCnt >= plbl->LBL_uiRep) {
                pbuf_free(p);                                           /*  �ں�������                  */
                return  (1);
            
            } else {
                break;                                                  /*  ���Դ���δ��, ��ʱ����      */
            }
        }
    }
    
    return  (0);
}
/*********************************************************************************************************
** ��������: __LoginBlFind
** ��������: ����ָ�� IP ���ƿ�
** �䡡��  : ipaddr        IP ��ַ
**           piHash        ���� hash index
** �䡡��  : ���������ƿ�
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static PLW_LOGINBL_NODE  __LoginBlFind (ip4_addr_t  ipaddr)
{
    INT                 iHash = LW_LBL_HASHINDEX(&ipaddr);
    PLW_LIST_LINE       plineTemp;
    PLW_LOGINBL_NODE    plbl;
    
    for (plineTemp  = _G_plineLblHashHeader[iHash];
         plineTemp != LW_NULL;
         plineTemp  = _list_line_get_next(plineTemp)) {
         
        plbl = _LIST_ENTRY(plineTemp, LW_LOGINBL_NODE, LBL_lineManage);
        if (plbl->LBL_ipaddr.addr == ipaddr.addr) {
            return  (plbl);
        }
    }
    
    return  (LW_NULL);
}
/*********************************************************************************************************
** ��������: __LoginWlFind
** ��������: ����ָ�� IP ���ƿ�
** �䡡��  : ipaddr        IP ��ַ
**           piHash        ���� hash index
** �䡡��  : ���������ƿ�
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static PLW_LOGINBL_NODE  __LoginWlFind (ip4_addr_t  ipaddr)
{
    INT                 iHash = LW_LBL_HASHINDEX(&ipaddr);
    PLW_LIST_LINE       plineTemp;
    PLW_LOGINBL_NODE    plbl;
    
    for (plineTemp  = _G_plineLwlHashHeader[iHash];
         plineTemp != LW_NULL;
         plineTemp  = _list_line_get_next(plineTemp)) {
         
        plbl = _LIST_ENTRY(plineTemp, LW_LOGINBL_NODE, LBL_lineManage);
        if (plbl->LBL_ipaddr.addr == ipaddr.addr) {
            return  (plbl);
        }
    }
    
    return  (LW_NULL);
}
/*********************************************************************************************************
** ��������: __LoginBlTimer
** ��������: ��ʱ���ص�
** �䡡��  : NONE
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static VOID  __LoginBlTimer (VOID)
{
    INT                 i;
    PLW_LIST_LINE       plineTemp;
    PLW_LIST_LINE       plineFree = LW_NULL;
    PLW_LOGINBL_NODE    plbl;
    
    if (!_G_uiLblCnt) {
        return;
    }
    
    LOCK_TCPIP_CORE();                                                  /*  ����Э��ջ                  */
    for (i = 0; i < LW_LBL_TABLESIZE; i++) {
        plineTemp = _G_plineLblHashHeader[i];
        while (plineTemp) {
            plbl = _LIST_ENTRY(plineTemp, LW_LOGINBL_NODE, LBL_lineManage);
            plineTemp = _list_line_get_next(plineTemp);
            
            if (plbl->LBL_uiSec) {
                plbl->LBL_uiSec--;
                if (plbl->LBL_uiSec == 0) {
                    _G_uiLblCnt--;
                    _List_Line_Del(&plbl->LBL_lineManage, &_G_plineLblHashHeader[i]);
                    _List_Line_Add_Tail(&plbl->LBL_lineManage, &plineFree);
                }
            }
        }
    }
    UNLOCK_TCPIP_CORE();                                                /*  ����Э��ջ                  */
    
    while (plineFree) {
        plbl = _LIST_ENTRY(plineFree, LW_LOGINBL_NODE, LBL_lineManage);
        _List_Line_Del(&plbl->LBL_lineManage, &plineFree);
        __SHEAP_FREE(plbl);
    }
}
/*********************************************************************************************************
** ��������: __LoginBlIsLocal
** ��������: ����Ƿ�Ϊ������ַ
** �䡡��  : pipaddr   IP ��ַ
** �䡡��  : �Ƿ�Ϊֱ������
** ȫ�ֱ���: 
** ����ģ��: 
** ע  ��  : ���Ǽ���α��Դ��ַ�����ݰ����ܹ����ص�������, 
             ����ʹ�ô˷���������Ч�������������ľܾ����񹥻�.
*********************************************************************************************************/
static BOOL  __LoginBlIsLocal (ip4_addr_t *pipaddr)
{
#define LW_LBL_NETIF_AVLID(pnetif) \
        ((netif_ip4_addr(pnetif)->addr != IPADDR_ANY) && \
         (netif_ip4_netmask(pnetif)->addr != IPADDR_ANY) && \
         (netif_ip4_gw(pnetif)->addr != IPADDR_ANY))

    struct netif *pnetif;
    
    NETIF_FOREACH(pnetif) {
        if (LW_LBL_NETIF_AVLID(pnetif)) {
            if (ip4_addr_netcmp(netif_ip4_addr(pnetif), 
                                pipaddr, netif_ip4_netmask(pnetif))) {
                return  (LW_TRUE);
            }
        }
    }
    
    return  (LW_FALSE);
}
/*********************************************************************************************************
** ��������: API_LoginBlAdd
** ��������: ��һ�������ַ��ӵ�������
** �䡡��  : addr      �����ַ
**           uiRep     �������ֶ��ٴ������
**           uiSec     ������ʱ�� (0 Ϊ���ô���)
** �䡡��  : ERROR or OK
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API
INT  API_LoginBlAdd (const struct sockaddr *addr, UINT  uiRep, UINT  uiSec)
{
    PLW_LOGINBL_NODE    plbl;
    PLW_LOGINBL_NODE    plblNew;
    ip4_addr_t          ipaddr;
    
    if (!addr || (addr->sa_family != AF_INET)) {
        _ErrorHandle(EINVAL);
        return  (ERROR_NONE);
    }
    
    if (uiRep > 10) {
        uiRep = 10;
    }
    
    if (uiSec > 100000) {
        uiSec = 100000;
    }

    if (_G_ulLblTimer == LW_OBJECT_HANDLE_INVALID) {
        _G_ulLblTimer =  API_TimerCreate("loginbl", LW_OPTION_ITIMER, LW_NULL);
        if (_G_ulLblTimer) {
            API_TimerStart(_G_ulLblTimer, LW_TICK_HZ, LW_OPTION_AUTO_RESTART,
                           (PTIMER_CALLBACK_ROUTINE)__LoginBlTimer, LW_NULL);
        }
    }
    
    ipaddr.addr = ((struct sockaddr_in *)addr)->sin_addr.s_addr;
    
    LOCK_TCPIP_CORE();                                                  /*  ����Э��ջ                  */
    if (__LoginBlIsLocal(&ipaddr)) {
        UNLOCK_TCPIP_CORE();
        return  (ERROR_NONE);                                           /*  ���������ػ���              */
    }
    
    plbl = __LoginBlFind(ipaddr);
    if (plbl) {
        plbl->LBL_uiRep = uiRep;
        plbl->LBL_uiSec = uiSec;
        plbl->LBL_uiCnt++;
        if (plbl->LBL_uiCnt >= uiRep) {
            plbl->LBL_uiCnt =  uiRep;
        }
        UNLOCK_TCPIP_CORE();
        return  (ERROR_NONE);
    
    } else if (_G_uiLblCnt >= LW_CFG_NET_LOGINBL_MAX_NODE) {            /*  �Ѿ�����                    */
        UNLOCK_TCPIP_CORE();
        _ErrorHandle(EXFULL);
        return  (PX_ERROR);
    }
    UNLOCK_TCPIP_CORE();                                                /*  ����Э��ջ                  */
    
    plblNew = (PLW_LOGINBL_NODE)__SHEAP_ALLOC(sizeof(LW_LOGINBL_NODE));
    if (plblNew == LW_NULL) {
        _ErrorHandle(ERROR_SYSTEM_LOW_MEMORY);
        return  (PX_ERROR);
    }
    
    plblNew->LBL_uiRep  = uiRep;
    plblNew->LBL_uiSec  = uiSec;
    plblNew->LBL_uiCnt  = 1;
    plblNew->LBL_ipaddr = ipaddr;
    
    LOCK_TCPIP_CORE();                                                  /*  ����Э��ջ                  */
    plbl = __LoginBlFind(ipaddr);
    if (plbl) {
        plbl->LBL_uiRep = uiRep;
        plbl->LBL_uiSec = uiSec;
        plbl->LBL_uiCnt++;
        if (plbl->LBL_uiCnt >= uiRep) {
            plbl->LBL_uiCnt =  uiRep;
        }
    } else {
        INT  iHash = LW_LBL_HASHINDEX(&ipaddr);
        _G_uiLblCnt++;
        _List_Line_Add_Tail(&plblNew->LBL_lineManage, &_G_plineLblHashHeader[iHash]);
        UNLOCK_TCPIP_CORE();
        return  (ERROR_NONE);
    }
    UNLOCK_TCPIP_CORE();                                                /*  ����Э��ջ                  */
    
    __SHEAP_FREE(plblNew);
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: API_LoginWlAdd
** ��������: ��һ�������ַ��ӵ�������
** �䡡��  : addr      �����ַ
** �䡡��  : ERROR or OK
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API
INT  API_LoginWlAdd (const struct sockaddr *addr)
{
    PLW_LOGINBL_NODE    plbl;
    PLW_LOGINBL_NODE    plblNew;
    ip4_addr_t          ipaddr;
    
    if (!addr || (addr->sa_family != AF_INET)) {
        _ErrorHandle(EINVAL);
        return  (ERROR_NONE);
    }
    
    ipaddr.addr = ((struct sockaddr_in *)addr)->sin_addr.s_addr;
    
    LOCK_TCPIP_CORE();                                                  /*  ����Э��ջ                  */
    plbl = __LoginWlFind(ipaddr);
    if (plbl) {
        UNLOCK_TCPIP_CORE();
        return  (ERROR_NONE);
    }
    UNLOCK_TCPIP_CORE();                                                /*  ����Э��ջ                  */
    
    plblNew = (PLW_LOGINBL_NODE)__SHEAP_ALLOC(sizeof(LW_LOGINBL_NODE));
    if (plblNew == LW_NULL) {
        _ErrorHandle(ERROR_SYSTEM_LOW_MEMORY);
        return  (PX_ERROR);
    }
    
    plblNew->LBL_uiRep  = 0;
    plblNew->LBL_uiSec  = 0;
    plblNew->LBL_uiCnt  = 0;
    plblNew->LBL_ipaddr = ipaddr;
    
    LOCK_TCPIP_CORE();                                                  /*  ����Э��ջ                  */
    plbl = __LoginWlFind(ipaddr);
    if (plbl == LW_NULL) {
        INT  iHash = LW_LBL_HASHINDEX(&ipaddr);
        _G_uiLwlCnt++;
        _List_Line_Add_Tail(&plblNew->LBL_lineManage, &_G_plineLwlHashHeader[iHash]);
        UNLOCK_TCPIP_CORE();
        return  (ERROR_NONE);
    }
    UNLOCK_TCPIP_CORE();                                                /*  ����Э��ջ                  */
    
    __SHEAP_FREE(plblNew);
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: API_LoginBlDelete
** ��������: ��һ�������ַ�Ӻ�����ɾ��
** �䡡��  : addr      �����ַ (NULL ��ʾȫ�����)
** �䡡��  : ERROR or OK
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API
INT  API_LoginBlDelete (const struct sockaddr *addr)
{
    ip4_addr_t          ipaddr;
    PLW_LOGINBL_NODE    plbl;
    
    if (!addr || (addr->sa_family != AF_INET)) {
        _ErrorHandle(EINVAL);
        return  (ERROR_NONE);
    }
    
    ipaddr.addr = ((struct sockaddr_in *)addr)->sin_addr.s_addr;
    
    LOCK_TCPIP_CORE();                                                  /*  ����Э��ջ                  */
    plbl = __LoginBlFind(ipaddr);
    if (plbl) {
        INT  iHash = LW_LBL_HASHINDEX(&ipaddr);
        _G_uiLblCnt--;
        _List_Line_Del(&plbl->LBL_lineManage, &_G_plineLblHashHeader[iHash]);
    }
    UNLOCK_TCPIP_CORE();                                                /*  ����Э��ջ                  */
    
    if (plbl) {
        __SHEAP_FREE(plbl);
        return  (ERROR_NONE);
    
    } else {
        _ErrorHandle(EINVAL);
        return  (ERROR_NONE);
    }
}
/*********************************************************************************************************
** ��������: API_LoginWlDelete
** ��������: ��һ�������ַ�Ӱ�����ɾ��
** �䡡��  : addr      �����ַ (NULL ��ʾȫ�����)
** �䡡��  : ERROR or OK
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API
INT  API_LoginWlDelete (const struct sockaddr *addr)
{
    ip4_addr_t          ipaddr;
    PLW_LOGINBL_NODE    plbl;
    
    if (!addr || (addr->sa_family != AF_INET)) {
        _ErrorHandle(EINVAL);
        return  (ERROR_NONE);
    }
    
    ipaddr.addr = ((struct sockaddr_in *)addr)->sin_addr.s_addr;
    
    LOCK_TCPIP_CORE();                                                  /*  ����Э��ջ                  */
    plbl = __LoginWlFind(ipaddr);
    if (plbl) {
        INT  iHash = LW_LBL_HASHINDEX(&ipaddr);
        _G_uiLwlCnt--;
        _List_Line_Del(&plbl->LBL_lineManage, &_G_plineLwlHashHeader[iHash]);
    }
    UNLOCK_TCPIP_CORE();                                                /*  ����Э��ջ                  */
    
    if (plbl) {
        __SHEAP_FREE(plbl);
        return  (ERROR_NONE);
    
    } else {
        _ErrorHandle(EINVAL);
        return  (ERROR_NONE);
    }
}
/*********************************************************************************************************
** ��������: API_LoginBlShow
** ��������: ��ʾ�����ַ������
** �䡡��  : NONE
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API
VOID  API_LoginBlShow (VOID)
{
    static const CHAR   cInfoHdr[] = 
    "      IP          SEC\n"
    "--------------- -------\n";

    PLW_LOGINBL_NODE    plbl;
    PLW_LIST_LINE       plineTemp;
    INT                 i;
    UINT                uiCnt;
    PCHAR               pcBuf;
    CHAR                cIp[IP4ADDR_STRLEN_MAX];
    size_t              stSize;
    size_t              stOffset = 0;
    
    LOCK_TCPIP_CORE();
    uiCnt = _G_uiLblCnt;
    UNLOCK_TCPIP_CORE();
    
    printf(cInfoHdr);
    if (!uiCnt) {
        return;
    }
    
    stSize = (size_t)uiCnt * 32;
    pcBuf  = (PCHAR)__SHEAP_ALLOC(stSize);
    if (pcBuf == LW_NULL) {
        fprintf(stderr, "no memory\n");
        _ErrorHandle(ERROR_SYSTEM_LOW_MEMORY);
        return;
    }
    
    LOCK_TCPIP_CORE();                                                  /*  ����Э��ջ                  */
    for (i = 0; i < LW_LBL_TABLESIZE; i++) {
        for (plineTemp  = _G_plineLblHashHeader[i];
             plineTemp != LW_NULL;
             plineTemp  = _list_line_get_next(plineTemp)) {
             
            plbl = _LIST_ENTRY(plineTemp, LW_LOGINBL_NODE, LBL_lineManage);
            if (plbl->LBL_uiCnt >= plbl->LBL_uiRep) {
                if (plbl->LBL_uiSec) {
                    stOffset = bnprintf(pcBuf, stSize, stOffset, 
                                        "%-15s %7u\n", 
                                        ip4addr_ntoa_r(&plbl->LBL_ipaddr, cIp, IP4ADDR_STRLEN_MAX),
                                        plbl->LBL_uiSec);
                } else {
                    stOffset = bnprintf(pcBuf, stSize, stOffset, 
                                        "%-15s %7s\n", 
                                        ip4addr_ntoa_r(&plbl->LBL_ipaddr, cIp, IP4ADDR_STRLEN_MAX),
                                        "Forever");
                }
            }
        }
    }
    UNLOCK_TCPIP_CORE();                                                /*  ����Э��ջ                  */
    
    if (stOffset > 0) {
        fwrite(pcBuf, stOffset, 1, stdout);
        fflush(stdout);                                                 /*  �������ȷ��������        */
    }
    
    __SHEAP_FREE(pcBuf);
}
/*********************************************************************************************************
** ��������: API_LoginWlShow
** ��������: ��ʾ�����ַ������
** �䡡��  : NONE
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API
VOID  API_LoginWlShow (VOID)
{
    static const CHAR   cInfoHdr[] = 
    "      IP\n"
    "---------------\n";

    PLW_LOGINBL_NODE    plbl;
    PLW_LIST_LINE       plineTemp;
    INT                 i;
    UINT                uiCnt;
    PCHAR               pcBuf;
    CHAR                cIp[IP4ADDR_STRLEN_MAX];
    size_t              stSize;
    size_t              stOffset = 0;
    
    LOCK_TCPIP_CORE();
    uiCnt = _G_uiLwlCnt;
    UNLOCK_TCPIP_CORE();
    
    printf(cInfoHdr);
    if (!uiCnt) {
        return;
    }
    
    stSize = (size_t)uiCnt * 24;
    pcBuf  = (PCHAR)__SHEAP_ALLOC(stSize);
    if (pcBuf == LW_NULL) {
        fprintf(stderr, "no memory\n");
        _ErrorHandle(ERROR_SYSTEM_LOW_MEMORY);
        return;
    }
    
    LOCK_TCPIP_CORE();                                                  /*  ����Э��ջ                  */
    for (i = 0; i < LW_LBL_TABLESIZE; i++) {
        for (plineTemp  = _G_plineLwlHashHeader[i];
             plineTemp != LW_NULL;
             plineTemp  = _list_line_get_next(plineTemp)) {
             
            plbl = _LIST_ENTRY(plineTemp, LW_LOGINBL_NODE, LBL_lineManage);
            stOffset = bnprintf(pcBuf, stSize, stOffset, 
                                "%-15s\n", 
                                ip4addr_ntoa_r(&plbl->LBL_ipaddr, cIp, IP4ADDR_STRLEN_MAX));
        }
    }
    UNLOCK_TCPIP_CORE();                                                /*  ����Э��ջ                  */
    
    if (stOffset > 0) {
        fwrite(pcBuf, stOffset, 1, stdout);
        fflush(stdout);                                                 /*  �������ȷ��������        */
    }
    
    __SHEAP_FREE(pcBuf);
}

#endif                                                                  /*  LW_CFG_NET_LOGINBL_EN > 0   */
#endif                                                                  /*  LW_CFG_NET_EN > 0           */
/*********************************************************************************************************
  END
*********************************************************************************************************/
