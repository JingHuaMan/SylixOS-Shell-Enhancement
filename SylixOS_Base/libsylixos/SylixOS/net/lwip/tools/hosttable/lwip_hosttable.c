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
** ��   ��   ��: lwip_hosttable.c
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2009 �� 08 �� 19 ��
**
** ��        ��: lwip ��̬ DNS ����������.
*********************************************************************************************************/
#define  __SYLIXOS_STDIO
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
#include "../SylixOS/system/include/s_system.h"
/*********************************************************************************************************
  �ü�����
*********************************************************************************************************/
#if LW_CFG_NET_EN > 0
#include "limits.h"
#include "unistd.h"
#include "netdb.h"
#include "lwip/def.h"
#include "lwip/dns.h"
#include "lwip/inet.h"
#include "netinet6/in6.h"
/*********************************************************************************************************
  �궨��
*********************************************************************************************************/
#define __LW_HOSTTABLE_HASH_SIZE            17                          /*  hash ���С                 */
/*********************************************************************************************************
  hash ��������
*********************************************************************************************************/
INT  __hashHorner(CPCHAR  pcKeyword, INT  iTableSize);
/*********************************************************************************************************
  �����Ͷ���
*********************************************************************************************************/
typedef struct {
    LW_LIST_LINE                HSTTBL_lineManage;                      /*  ��������                    */
    struct in_addr              HSTTBL_inaddr;                          /*  ��ַ                        */
    CHAR                        HSTTBL_cHostName[1];                    /*  ������                      */
} __LW_HOSTTABLE_ITEM;
typedef __LW_HOSTTABLE_ITEM    *__PLW_HOSTTABLE_ITEM;
/*********************************************************************************************************
  ȫ�ֱ���
*********************************************************************************************************/
static LW_OBJECT_HANDLE         _G_ulHostTableLock;
static LW_LIST_LINE_HEADER      _G_plineHostTableHash[__LW_HOSTTABLE_HASH_SIZE];
/*********************************************************************************************************
  ��������
*********************************************************************************************************/
static INT  __tshellHostTable(INT  iArgC, PCHAR  *ppcArgV);
/*********************************************************************************************************
** ��������: __inetHostTableInit
** ��������: ��ʼ�����ض�̬����������
** �䡡��  : NONE
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
VOID  __inetHostTableInit (VOID)
{
    
    _G_ulHostTableLock = API_SemaphoreBCreate("hosttable_lock", LW_TRUE, 
                                              LW_OPTION_OBJECT_GLOBAL, LW_NULL);

#if LW_CFG_SHELL_EN > 0
    API_TShellKeywordAdd("hosttable", __tshellHostTable);
    API_TShellFormatAdd("hosttable",  " [-s host addr] | [-d host] ");
    API_TShellHelpAdd("hosttable",    "show/add/delete local host table.\n");
#endif                                                                  /*  LW_CFG_SHELL_EN > 0         */
}
/*********************************************************************************************************
** ��������: __inetHostTableGetItem
** ��������: lwip ���ô˺�����ѯ���ض�̬����������.
** �䡡��  : pcHost        ������
**           stLen         ����
**           addr          ���ص�ַ
**           ucAddrType    ��ַ����
** �䡡��  : ERR_OK or ERR_IF.
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
INT  __inetHostTableGetItem (CPCHAR  pcHost, size_t  stLen, PVOID  pvAddr, UINT8  ucAddrType)
{
    REGISTER INT                    iHash;
    REGISTER PLW_LIST_LINE          plineTemp;
             __PLW_HOSTTABLE_ITEM   phsttablitem;
             ip_addr_t             *addr   = (ip_addr_t *)pvAddr;
             u32_t                  uiAddr = INADDR_NONE;
             CHAR                   cHostName[HOST_NAME_MAX + 1];
             CHAR                   cHostsBuffer[MAX_FILENAME_LENGTH];
    struct hostent                 *phostent;
             FILE                  *fpHosts;
             BOOL                   bHostsFund = LW_FALSE;
             INT                    i;
    
    if (!addr || !pcHost) {
        return  (ERR_IF);
    }
    
    gethostname(cHostName, HOST_NAME_MAX + 1);                          /*  ����������ַ                */
    if (lwip_strnicmp(cHostName, pcHost, stLen) == 0) {
        if (ucAddrType != LWIP_DNS_ADDRTYPE_IPV6) {
            ip_2_ip4(addr)->addr = htonl(INADDR_LOOPBACK);
            IP_SET_TYPE(addr, IPADDR_TYPE_V4);
        
#if LWIP_IPV6
        } else {
            lib_memcpy(ip_2_ip6(addr)->addr, &in6addr_loopback, 16);
            IP_SET_TYPE(addr, IPADDR_TYPE_V6);
#endif                                                                  /*  LWIP_IPV6                   */
        }
        return  (ERR_OK);
    }
    
    fpHosts = fopen(_PATH_HOSTS, "r");                                  /*  /etc/hosts                  */
    if (fpHosts) {
        phostent = gethostent2_r(fpHosts, cHostsBuffer, sizeof(cHostsBuffer));
        while (phostent) {
            if (lwip_strnicmp(pcHost, phostent->h_name, stLen) == 0) {
                bHostsFund = LW_TRUE;
                break;
            }
            for (i = 0; phostent->h_aliases[i]; i++) {
                if (lwip_strnicmp(pcHost, phostent->h_aliases[i], stLen) == 0) {
                    bHostsFund = LW_TRUE;
                    goto    __fund_check;
                }
            }
            phostent = gethostent2_r(fpHosts, cHostsBuffer, sizeof(cHostsBuffer));
        }
        
__fund_check:
        fclose(fpHosts);
        
        if (bHostsFund) {
            if ((phostent->h_length == 4) &&
                (ucAddrType != LWIP_DNS_ADDRTYPE_IPV6)) {               /*  ipv4                        */
                lib_memcpy(&uiAddr, phostent->h_addr, sizeof(uiAddr));
                ip_2_ip4(addr)->addr = uiAddr;
                IP_SET_TYPE(addr, IPADDR_TYPE_V4);
                return  (ERR_OK);
            
#if LWIP_IPV6
            } else if ((phostent->h_length == 16) &&
                       (ucAddrType == LWIP_DNS_ADDRTYPE_IPV6)) {        /*  ipv6                        */
                lib_memcpy(&ip_2_ip6(addr)->addr, phostent->h_addr, phostent->h_length);
                IP_SET_TYPE(addr, IPADDR_TYPE_V6);
                return  (ERR_OK);
#endif                                                                  /*  LWIP_IPV6                   */
            }
        }
    }

    iHash = __hashHorner(pcHost, __LW_HOSTTABLE_HASH_SIZE);
    
    API_SemaphoreBPend(_G_ulHostTableLock, LW_OPTION_WAIT_INFINITE);    /*  �����ٽ���                  */
    for (plineTemp  = _G_plineHostTableHash[iHash];
         plineTemp != LW_NULL;
         plineTemp  = _list_line_get_next(plineTemp)) {
         
        phsttablitem = _LIST_ENTRY(plineTemp, __LW_HOSTTABLE_ITEM, HSTTBL_lineManage);
        if (lwip_strnicmp(phsttablitem->HSTTBL_cHostName, pcHost, stLen) == 0) {
            uiAddr = phsttablitem->HSTTBL_inaddr.s_addr;
            break;
        }
    }
    API_SemaphoreBPost(_G_ulHostTableLock);                             /*  �˳��ٽ���                  */
    
    if ((uiAddr != INADDR_NONE) &&
        (ucAddrType != LWIP_DNS_ADDRTYPE_IPV6)) {                       /*  ipv4                        */
        ip_2_ip4(addr)->addr = uiAddr;
        IP_SET_TYPE(addr, IPADDR_TYPE_V4);
        return  (ERR_OK);
    
    } else {
        return  (ERR_IF);
    }
}
/*********************************************************************************************************
** ��������: API_INetHostTableGetItem
** ��������: ��ѯ���ض�̬����������.
** �䡡��  : pcHost        ������
**           pinaddr       ��ַ
** �䡡��  : ERROR CODE
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API
INT  API_INetHostTableGetItem (CPCHAR  pcHost, struct in_addr  *pinaddr)
{
    ip_addr_t   addr;
    
    if (!pcHost) {
        return  (PX_ERROR);
    }

    if (__inetHostTableGetItem(pcHost, lib_strlen(pcHost), &addr, LWIP_DNS_ADDRTYPE_IPV4)) {
        return  (PX_ERROR);
    }
    
    if (pinaddr) {
        pinaddr->s_addr = ip_2_ip4(&addr)->addr;
    }
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: API_INetHostTableAddItem
** ��������: �򱾵ض�̬�����������в���һ����Ŀ.
** �䡡��  : pcHost        ������
**           inaddr        ��ַ
** �䡡��  : ERROR CODE
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API
INT  API_INetHostTableAddItem (CPCHAR  pcHost, struct in_addr  inaddr)
{
    REGISTER INT                    iHash;
    REGISTER PLW_LIST_LINE          plineTemp;
             __PLW_HOSTTABLE_ITEM   phsttablitem;
             
             BOOL                   bIsExist = LW_FALSE;

    if (pcHost == LW_NULL) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    
    iHash = __hashHorner(pcHost, __LW_HOSTTABLE_HASH_SIZE);

    API_SemaphoreBPend(_G_ulHostTableLock, LW_OPTION_WAIT_INFINITE);    /*  �����ٽ���                  */
    for (plineTemp  = _G_plineHostTableHash[iHash];
         plineTemp != LW_NULL;
         plineTemp  = _list_line_get_next(plineTemp)) {
         
        phsttablitem = _LIST_ENTRY(plineTemp, __LW_HOSTTABLE_ITEM, HSTTBL_lineManage);
        if (lib_strcmp(phsttablitem->HSTTBL_cHostName, pcHost) == 0) {
            bIsExist = LW_TRUE;
            break;
        }
    }
    /*
     *  ��������������.
     */
    if (bIsExist == LW_FALSE) {
        phsttablitem = (__PLW_HOSTTABLE_ITEM)__SHEAP_ALLOC(sizeof(__LW_HOSTTABLE_ITEM) + 
                                                           lib_strlen(pcHost));
        if (phsttablitem == LW_NULL) {
            API_SemaphoreBPost(_G_ulHostTableLock);                     /*  �˳��ٽ���                  */
            _ErrorHandle(ERROR_SYSTEM_LOW_MEMORY);
            return  (PX_ERROR);
        }
        phsttablitem->HSTTBL_inaddr = inaddr;
        lib_strcpy(phsttablitem->HSTTBL_cHostName, pcHost);
        
        _List_Line_Add_Ahead(&phsttablitem->HSTTBL_lineManage,
                             &_G_plineHostTableHash[iHash]);            /*  ���� hash ��                */
    }
    API_SemaphoreBPost(_G_ulHostTableLock);                             /*  �˳��ٽ���                  */
    
    if (bIsExist) {
        _ErrorHandle(EADDRINUSE);                                       /*  ��ַ�Ѿ���ʹ����            */
        return  (PX_ERROR);
    
    } else {
        return  (ERROR_NONE);
    }
}
/*********************************************************************************************************
** ��������: API_INetHostTableDelItem
** ��������: �ӱ��ض�̬������������ɾ��һ����Ŀ.
** �䡡��  : pcHost        ������
** �䡡��  : ERROR CODE
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API
INT  API_INetHostTableDelItem (CPCHAR  pcHost)
{
    REGISTER INT                    iHash;
    REGISTER PLW_LIST_LINE          plineTemp;
             __PLW_HOSTTABLE_ITEM   phsttablitem;

    if (pcHost == LW_NULL) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    
    iHash = __hashHorner(pcHost, __LW_HOSTTABLE_HASH_SIZE);

    API_SemaphoreBPend(_G_ulHostTableLock, LW_OPTION_WAIT_INFINITE);    /*  �����ٽ���                  */
    for (plineTemp  = _G_plineHostTableHash[iHash];
         plineTemp != LW_NULL;
         plineTemp  = _list_line_get_next(plineTemp)) {
         
        phsttablitem = _LIST_ENTRY(plineTemp, __LW_HOSTTABLE_ITEM, HSTTBL_lineManage);
        if (lib_strcmp(phsttablitem->HSTTBL_cHostName, pcHost) == 0) {
            _List_Line_Del(&phsttablitem->HSTTBL_lineManage,
                           &_G_plineHostTableHash[iHash]);              /*  �� hash ����ɾ��            */
            __SHEAP_FREE(phsttablitem);
            break;
        }
    }
    API_SemaphoreBPost(_G_ulHostTableLock);                             /*  �˳��ٽ���                  */
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: __tshellHostTable
** ��������: shell ָ�� "hosttable"
** �䡡��  : iArgC         ��������
**           ppcArgV       ������
** �䡡��  : 0 or -1
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
#if LW_CFG_SHELL_EN > 0

static INT  __tshellHostTable (INT  iArgC, PCHAR  *ppcArgV)
{
    static const CHAR               cHostTableInfoHdr[] = "\n"
          "      addr                       HOST\n"
          "--------------- ------------------------------------------\n";
    REGISTER PLW_LIST_LINE          plineTemp;
             __PLW_HOSTTABLE_ITEM   phsttablitem;
             
             CHAR                   cBuffer[INET_ADDRSTRLEN];
             ip4_addr_t             ipaddr;
    struct in_addr                  inaddr;
    
             INT                    i;

    if (iArgC == 1) {                                                   /*  ����ӡ��ǰ��ַ����Ϣ        */
        printf(cHostTableInfoHdr);
        
        API_SemaphoreBPend(_G_ulHostTableLock, LW_OPTION_WAIT_INFINITE);/*  �����ٽ���                  */
        /*
         *  ������ʾ������.
         */
        for (i = 0; i < __LW_HOSTTABLE_HASH_SIZE; i++) {
            for (plineTemp  = _G_plineHostTableHash[i];
                 plineTemp != LW_NULL;
                 plineTemp  = _list_line_get_next(plineTemp)) {
                 
                phsttablitem = _LIST_ENTRY(plineTemp, __LW_HOSTTABLE_ITEM, HSTTBL_lineManage);
                
                ipaddr.addr = phsttablitem->HSTTBL_inaddr.s_addr;
                
                /*
                 *  ����û��ʹ�� inet_ntoa() ��Ϊ inet_ntoa() ��������.
                 */
                snprintf(cBuffer, sizeof(cBuffer), 
                         "%d.%d.%d.%d", 
                         ip4_addr1(&ipaddr),
                         ip4_addr2(&ipaddr),
                         ip4_addr3(&ipaddr),
                         ip4_addr4(&ipaddr));
                printf("%-15s %s\n", cBuffer, phsttablitem->HSTTBL_cHostName);
            }
        }
        API_SemaphoreBPost(_G_ulHostTableLock);                         /*  �˳��ٽ���                  */
        
        return  (ERROR_NONE);
    
    } else if ((iArgC == 4) && 
        (lib_strncmp(ppcArgV[1], "-s", 3) == 0)) {                      /*  ���������ַӳ��            */
        
        if (!inet_aton(ppcArgV[3], &inaddr)) {
            fprintf(stderr, "inaddr error.\n");
            return  (-ERROR_TSHELL_EPARAM);
        }
        
        return  (API_INetHostTableAddItem(ppcArgV[2], inaddr));         /*  �����Ŀ                    */
        
    } else if ((iArgC == 3) && 
               (lib_strncmp(ppcArgV[1], "-d", 3) == 0)) {               /*  ɾ��������Ϣ                */
        
        return  (API_INetHostTableDelItem(ppcArgV[2]));                 /*  ɾ����Ŀ                    */
        
    } else {
        fprintf(stderr, "arguments error!\n");                           /*  ��������                    */
        return  (-ERROR_TSHELL_EPARAM);
    }
}

#endif                                                                  /*  LW_CFG_SHELL_EN > 0         */
#endif                                                                  /*  LW_CFG_NET_EN               */
/*********************************************************************************************************
  END
*********************************************************************************************************/
