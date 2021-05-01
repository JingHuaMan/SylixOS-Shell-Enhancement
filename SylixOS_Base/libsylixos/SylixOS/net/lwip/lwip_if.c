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
** ��   ��   ��: lwip_if.c
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2011 �� 03 �� 10 ��
**
** ��        ��: posix net/if �ӿ�.

** BUG:
2011.07.07  _G_ulNetifLock ����.
2014.03.22  �Ż��������ӿ�.
2014.06.24  ���� if_down �� if_up API.
2014.12.01  ֹͣ���������ʹ�� dhcp ��Ҫֹͣ��Լ.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
#include "../SylixOS/system/include/s_system.h"
/*********************************************************************************************************
  �ü�����
*********************************************************************************************************/
#if LW_CFG_NET_EN > 0
#include "net/if.h"
#include "net/if_lock.h"
#include "lwip/netif.h"
#include "lwip/netifapi.h"
#include "lwip/tcpip.h"
/*********************************************************************************************************
  ȫ�ֱ���
*********************************************************************************************************/
static LW_OBJECT_HANDLE  _G_hNetListRWLock;
/*********************************************************************************************************
** ��������: if_list_init
** ��������: ����ӿ�������ʼ��
** �䡡��  : NONE
** �䡡��  : �Ƿ�ɹ�
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
INT  if_list_init (VOID)
{
    _G_hNetListRWLock = API_SemaphoreRWCreate("if_list", LW_OPTION_OBJECT_GLOBAL | 
                                              LW_OPTION_DELETE_SAFE | LW_OPTION_WAIT_PRIORITY, LW_NULL);
    if (_G_hNetListRWLock == LW_OBJECT_HANDLE_INVALID) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "network interface list lock create error.\r\n");
        return  (PX_ERROR);
    }
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: if_list_lock
** ��������: ��������ӿ���
** �䡡��  : bWrite        TRUE: д FALSE: ��
** �䡡��  : �Ƿ�ɹ�
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
INT  if_list_lock (BOOL  bWrite)
{
    LW_OBJECT_HANDLE  hMe;
    LW_OBJECT_HANDLE  hCLOwner;
    ULONG             ulError;

    hMe = API_ThreadIdSelf();
    if (!hMe) {
        return  (PX_ERROR);
    }
    
    API_SemaphoreMStatusEx(lock_tcpip_core, LW_NULL, LW_NULL, LW_NULL, &hCLOwner);
    if (hMe == hCLOwner) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "network dead lock detected.\r\n");
        return  (PX_ERROR);
    }
    
    if (bWrite) {
        ulError = API_SemaphoreRWPendW(_G_hNetListRWLock, LW_OPTION_WAIT_INFINITE);
    
    } else {
        ulError = API_SemaphoreRWPendR(_G_hNetListRWLock, LW_OPTION_WAIT_INFINITE);
    }
    
    return  (ulError ? PX_ERROR : ERROR_NONE);
}
/*********************************************************************************************************
** ��������: if_list_unlock
** ��������: ��������ӿ���
** �䡡��  : NONE
** �䡡��  : �Ƿ�ɹ�
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
INT  if_list_unlock (VOID)
{
    API_SemaphoreRWPost(_G_hNetListRWLock);
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: if_down
** ��������: �ر�����
** �䡡��  : ifname        if name
** �䡡��  : �ر��Ƿ�ɹ�
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
INT  if_down (const char *ifname)
{
    struct netif  *pnetif;
    
    LWIP_IF_LIST_LOCK(LW_FALSE);                                        /*  �����ٽ���                  */
    pnetif = netif_find(ifname);
    if (pnetif == LW_NULL) {
        LWIP_IF_LIST_UNLOCK();                                          /*  �˳��ٽ���                  */
        _ErrorHandle(ENXIO);
        return  (PX_ERROR);
    }
    if (!netif_is_up(pnetif)) {
        LWIP_IF_LIST_UNLOCK();                                          /*  �˳��ٽ���                  */
        _ErrorHandle(EALREADY);
        return  (PX_ERROR);
    }
    
#if LWIP_DHCP > 0
    netifapi_dhcp_release_and_stop(pnetif);
#endif                                                                  /*  LWIP_DHCP > 0               */
#if LWIP_AUTOIP > 0
    netifapi_autoip_stop(pnetif);
#endif                                                                  /*  LWIP_AUTOIP > 0             */
#if LWIP_IPV6_DHCP6 > 0
    netifapi_dhcp6_disable(pnetif);
#endif                                                                  /*  LWIP_IPV6_DHCP6 > 0         */
    netifapi_netif_set_down(pnetif);
    LWIP_IF_LIST_UNLOCK();                                              /*  �˳��ٽ���                  */
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: if_up
** ��������: ������
** �䡡��  : ifname        if name
** �䡡��  : �����Ƿ��
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
INT  if_up (const char *ifname)
{
    struct netif  *pnetif;
    
    LWIP_IF_LIST_LOCK(LW_FALSE);                                        /*  �����ٽ���                  */
    pnetif = netif_find(ifname);
    if (pnetif == LW_NULL) {
        LWIP_IF_LIST_UNLOCK();                                          /*  �˳��ٽ���                  */
        _ErrorHandle(ENXIO);
        return  (PX_ERROR);
    }
    if (netif_is_up(pnetif)) {
        LWIP_IF_LIST_UNLOCK();                                          /*  �˳��ٽ���                  */
        _ErrorHandle(EALREADY);
        return  (PX_ERROR);
    }
    
    netifapi_netif_set_up(pnetif);
    
#if LWIP_DHCP > 0
    if (pnetif->flags2 & NETIF_FLAG2_DHCP) {
        netifapi_netif_set_addr(pnetif, IP4_ADDR_ANY4, IP4_ADDR_ANY4, IP4_ADDR_ANY4);
        netifapi_dhcp_start(pnetif);
    }
#endif                                                                  /*  LWIP_DHCP > 0               */

#if LWIP_IPV6_DHCP6 > 0
    if (pnetif->flags2 & NETIF_FLAG2_DHCP6) {
        netifapi_dhcp6_enable_stateless(pnetif);
    }
#endif                                                                  /*  LWIP_IPV6_DHCP6 > 0         */
    LWIP_IF_LIST_UNLOCK();                                              /*  �˳��ٽ���                  */
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: if_isup
** ��������: �����Ƿ�ʹ��
** �䡡��  : ifname        if name
** �䡡��  : �����Ƿ�ʹ�� 0: ����  1: ʹ��  -1: �������ִ���
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
INT  if_isup (const char *ifname)
{
    INT            iRet;
    struct netif  *pnetif;
    
    LWIP_IF_LIST_LOCK(LW_FALSE);                                        /*  �����ٽ���                  */
    pnetif = netif_find(ifname);
    if (pnetif == LW_NULL) {
        LWIP_IF_LIST_UNLOCK();                                          /*  �˳��ٽ���                  */
        _ErrorHandle(ENXIO);
        return  (PX_ERROR);
    }
    
    iRet = netif_is_up(pnetif);
    LWIP_IF_LIST_UNLOCK();                                              /*  �˳��ٽ���                  */
    
    return  (iRet);
}
/*********************************************************************************************************
** ��������: if_islink
** ��������: �����Ƿ��Ѿ�����
** �䡡��  : ifname        if name
** �䡡��  : �����Ƿ����� 0: ����  1: û������  -1: �������ִ���
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
INT  if_islink (const char *ifname)
{
    INT            iRet;
    struct netif  *pnetif;
    
    LWIP_IF_LIST_LOCK(LW_FALSE);                                        /*  �����ٽ���                  */
    pnetif = netif_find(ifname);
    if (pnetif == LW_NULL) {
        LWIP_IF_LIST_UNLOCK();                                          /*  �˳��ٽ���                  */
        _ErrorHandle(ENXIO);
        return  (PX_ERROR);
    }
    
    iRet = netif_is_link_up(pnetif);
    LWIP_IF_LIST_UNLOCK();                                              /*  �˳��ٽ���                  */

    return  (iRet);
}
/*********************************************************************************************************
** ��������: if_set_dhcp
** ��������: �������� dhcp ѡ�� (��������������ʱ����)
** �䡡��  : ifname        if name
**           en            1: ʹ�� dhcp  0: ���� dhcp
** �䡡��  : OK or ERROR
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
INT  if_set_dhcp (const char *ifname, int en)
{
#if LWIP_DHCP > 0
    struct netif  *pnetif;
    
    LWIP_IF_LIST_LOCK(LW_FALSE);                                        /*  �����ٽ���                  */
    pnetif = netif_find(ifname);
    if (pnetif == LW_NULL) {
        LWIP_IF_LIST_UNLOCK();                                          /*  �˳��ٽ���                  */
        _ErrorHandle(ENXIO);
        return  (PX_ERROR);
    }
    if (netif_is_up(pnetif)) {
        LWIP_IF_LIST_UNLOCK();                                          /*  �˳��ٽ���                  */
        _ErrorHandle(EISCONN);
        return  (PX_ERROR);
    }
    
    if (en) {
        pnetif->flags2 |= NETIF_FLAG2_DHCP;
    } else {
        pnetif->flags2 &= ~NETIF_FLAG2_DHCP;
    }
    LWIP_IF_LIST_UNLOCK();                                              /*  �˳��ٽ���                  */
    
    return  (ERROR_NONE);

#else
    _ErrorHandle(ENOSYS);
    return  (PX_ERROR);
#endif                                                                  /*  LWIP_DHCP > 0               */
}
/*********************************************************************************************************
** ��������: if_get_dhcp
** ��������: ��ȡ���� dhcp ѡ��
** �䡡��  : ifname        if name
** �䡡��  : 1: ʹ�� dhcp  0: ���� dhcp -1: �������ִ���
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
INT  if_get_dhcp (const char *ifname)
{
    INT            iRet;
    struct netif  *pnetif;
    
    LWIP_IF_LIST_LOCK(LW_FALSE);                                        /*  �����ٽ���                  */
    pnetif = netif_find((char *)ifname);
    if (pnetif == LW_NULL) {
        LWIP_IF_LIST_UNLOCK();                                          /*  �˳��ٽ���                  */
        _ErrorHandle(ENXIO);
        return  (PX_ERROR);
    }
    
    iRet = (pnetif->flags2 & NETIF_FLAG2_DHCP) ? 1 : 0;
    LWIP_IF_LIST_UNLOCK();                                              /*  �˳��ٽ���                  */
    
    return  (iRet);
}
/*********************************************************************************************************
** ��������: if_set_dhcp6
** ��������: �������� dhcp v6 ѡ�� (��������������ʱ����)
** �䡡��  : ifname        if name
**           en            1: ʹ�� dhcp  0: ���� dhcp
**           stateless     1: stateless  0: stateful (NOT support now)
** �䡡��  : OK or ERROR
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
INT  if_set_dhcp6 (const char *ifname, int en, int stateless)
{
#if LWIP_IPV6_DHCP6 > 0
    struct netif  *pnetif;
    
    LWIP_IF_LIST_LOCK(LW_FALSE);                                        /*  �����ٽ���                  */
    pnetif = netif_find(ifname);
    if (pnetif == LW_NULL) {
        LWIP_IF_LIST_UNLOCK();                                          /*  �˳��ٽ���                  */
        _ErrorHandle(ENXIO);
        return  (PX_ERROR);
    }
    if (netif_is_up(pnetif)) {
        LWIP_IF_LIST_UNLOCK();                                          /*  �˳��ٽ���                  */
        _ErrorHandle(EISCONN);
        return  (PX_ERROR);
    }
    
    if (en) {
        pnetif->flags2 |= NETIF_FLAG2_DHCP6;
    } else {
        pnetif->flags2 &= ~NETIF_FLAG2_DHCP6;
    }
    LWIP_IF_LIST_UNLOCK();                                              /*  �˳��ٽ���                  */

    return  (ERROR_NONE);
    
#else
    _ErrorHandle(ENOSYS);
    return  (PX_ERROR);
#endif                                                                  /*  LWIP_IPV6_DHCP6 > 0         */
}
/*********************************************************************************************************
** ��������: if_get_dhcp6
** ��������: ��ȡ���� dhcp v6 ѡ��
** �䡡��  : ifname        if name
** �䡡��  : 1: ʹ�� dhcp  0: ���� dhcp -1: �������ִ���
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
INT  if_get_dhcp6 (const char *ifname)
{
    INT            iRet;
    struct netif  *pnetif;
    
    LWIP_IF_LIST_LOCK(LW_FALSE);                                        /*  �����ٽ���                  */
    pnetif = netif_find((char *)ifname);
    if (pnetif == LW_NULL) {
        LWIP_IF_LIST_UNLOCK();                                          /*  �˳��ٽ���                  */
        _ErrorHandle(ENXIO);
        return  (PX_ERROR);
    }
    
    iRet = (pnetif->flags2 & NETIF_FLAG2_DHCP6) ? 1 : 0;
    LWIP_IF_LIST_UNLOCK();                                              /*  �˳��ٽ���                  */
    
    return  (iRet);
}
/*********************************************************************************************************
** ��������: if_nametoindex
** ��������: map a network interface name to its corresponding index
** �䡡��  : ifname        if name
** �䡡��  : index
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
unsigned  if_nametoindex (const char *ifname)
{
    unsigned  uiIndex = 0;
    
    LWIP_IF_LIST_LOCK(LW_FALSE);                                        /*  �����ٽ���                  */
    uiIndex = netif_name_to_index(ifname);
    LWIP_IF_LIST_UNLOCK();                                              /*  �˳��ٽ���                  */
    
    if (uiIndex == 0) {
        _ErrorHandle(ENXIO);
    }
    
    return  (uiIndex);
}
/*********************************************************************************************************
** ��������: if_indextoname
** ��������: map a network interface index to its corresponding name
** �䡡��  : ifindex       if index
**           ifname        if name buffer at least {IF_NAMESIZE} bytes
** �䡡��  : index
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
char *if_indextoname (unsigned  ifindex, char *ifname)
{
    char  *ret;

    if (!ifname) {
        errno = EINVAL;
    }
    
    LWIP_IF_LIST_LOCK(LW_FALSE);                                        /*  �����ٽ���                  */
    ret = netif_index_to_name(ifindex, ifname);
    LWIP_IF_LIST_UNLOCK();                                              /*  �˳��ٽ���                  */
    
    if (ret == LW_NULL) {
        _ErrorHandle(ENXIO);
    }
    
    return  (ret);
}
/*********************************************************************************************************
** ��������: if_nameindex
** ��������: return all network interface names and indexes
** �䡡��  : NONE
** �䡡��  : An array of structures identifying local interfaces
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
struct if_nameindex *if_nameindex (void)
{
    struct netif           *pnetif;
    int                     i = 0, iNum = 1;                            /*  ��Ҫһ�����е�λ��          */
    struct if_nameindex    *pifnameindexArry;
    
    LWIP_IF_LIST_LOCK(LW_FALSE);                                        /*  �����ٽ���                  */
    NETIF_FOREACH(pnetif) {
        iNum++;
    }
    pifnameindexArry = (struct if_nameindex *)__SHEAP_ALLOC(sizeof(struct if_nameindex) * (size_t)iNum);
    if (pifnameindexArry == LW_NULL) {
        LWIP_IF_LIST_UNLOCK();                                          /*  �˳��ٽ���                  */
        _ErrorHandle(ENOMEM);
        return  (LW_NULL);
    }
    
    NETIF_FOREACH(pnetif) {
        pifnameindexArry[i].if_index = netif_get_index(pnetif);
        pifnameindexArry[i].if_name  = netif_index_to_name(pifnameindexArry[i].if_index, 
                                                           pifnameindexArry[i].if_name_buf);
        i++;
    }
    
    pifnameindexArry[i].if_index       = 0;
    pifnameindexArry[i].if_name_buf[0] = PX_EOS;
    pifnameindexArry[i].if_name        = pifnameindexArry[i].if_name_buf;
    LWIP_IF_LIST_UNLOCK();                                              /*  �˳��ٽ���                  */
    
    return  (pifnameindexArry);
}
/*********************************************************************************************************
** ��������: if_nameindex_bufsize
** ��������: ���� if_nameindex ����Ļ����С
** �䡡��  : NONE
** �䡡��  : �����С
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
size_t if_nameindex_bufsize (void)
{
    struct netif  *pnetif;
    INT            iNum = 1;

    LWIP_IF_LIST_LOCK(LW_FALSE);                                        /*  �����ٽ���                  */
    NETIF_FOREACH(pnetif) {
        iNum++;
    }
    LWIP_IF_LIST_UNLOCK();                                              /*  �˳��ٽ���                  */
    
    return  (sizeof(struct if_nameindex) * (size_t)iNum);
}
/*********************************************************************************************************
** ��������: if_nameindex
** ��������: return all network interface names and indexes
** �䡡��  : buffer    ����λ��
**           bufsize   �����С
** �䡡��  : An array of structures identifying local interfaces
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
struct if_nameindex *if_nameindex_rnp (void *buffer, size_t bufsize)
{
    struct netif           *pnetif;
    int                     iMax = bufsize / sizeof(struct if_nameindex);
    int                     i = 0;
    struct if_nameindex    *pifnameindexArry;
    
    if (!buffer || (bufsize < sizeof(struct if_nameindex))) {
        errno = EINVAL;
        return  (LW_NULL);
    }
    
    LWIP_IF_LIST_LOCK(LW_FALSE);                                        /*  �����ٽ���                  */
    pifnameindexArry = (struct if_nameindex *)buffer;
    NETIF_FOREACH(pnetif) {
        if (i >= (iMax - 1)) {
            break;
        }
        pifnameindexArry[i].if_index = netif_get_index(pnetif);
        pifnameindexArry[i].if_name  = netif_index_to_name(pifnameindexArry[i].if_index, 
                                                           pifnameindexArry[i].if_name_buf);
        i++;
    }
    
    pifnameindexArry[i].if_index       = 0;
    pifnameindexArry[i].if_name_buf[0] = PX_EOS;
    pifnameindexArry[i].if_name        = pifnameindexArry[i].if_name_buf;
    LWIP_IF_LIST_UNLOCK();                                              /*  �˳��ٽ���                  */
    
    return  (pifnameindexArry);
}
/*********************************************************************************************************
** ��������: if_freenameindex
** ��������: free memory allocated by if_nameindex
             the application shall not use the array of which ptr is the address.
** �䡡��  : ptr           shall be a pointer that was returned by if_nameindex().
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
void  if_freenameindex (struct if_nameindex *ptr)
{
    if (ptr) {
        __SHEAP_FREE(ptr);
    }
}

#endif                                                                  /*  LW_CFG_NET_EN               */
/*********************************************************************************************************
  END
*********************************************************************************************************/
