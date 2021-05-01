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
** ��   ��   ��: lwip_mrtctl.c
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2018 �� 01 �� 04 ��
**
** ��        ��: ioctl �鲥·�ɱ�֧��.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "SylixOS.h"
/*********************************************************************************************************
  �ü�����
*********************************************************************************************************/
#if LW_CFG_NET_EN > 0 && LW_CFG_NET_MROUTER > 0
#include "sys/socket.h"
#include "lwip/tcpip.h"
#include "mroute/ip4_mrt.h"
#include "mroute/ip6_mrt.h"
/*********************************************************************************************************
** ��������: __mrtGetVifCnt4
** ��������: ��� IPv4 vif cnt ��Ϣ
** �䡡��  : pvArg     ��Ϣ�ṹ
** �䡡��  : ERROR or OK
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static INT  __mrtGetVifCnt4 (PVOID  pvArg)
{
    INT  iErrNo;
    
    MRT_LOCK();
    iErrNo = ip4_mrt_ioctl(SIOCGETVIFCNT, pvArg);
    if (iErrNo) {
        MRT_UNLOCK();
        _ErrorHandle(iErrNo);
        return  (PX_ERROR);
    }
    MRT_UNLOCK();
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: __mrtGetVifCnt6
** ��������: ��� IPv6 vif cnt ��Ϣ
** �䡡��  : pvArg     ��Ϣ�ṹ
** �䡡��  : ERROR or OK
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
#if LWIP_IPV6

static INT  __mrtGetVifCnt6 (PVOID  pvArg)
{
    INT  iErrNo;
    
    MRT6_LOCK();
    iErrNo = ip6_mrt_ioctl(SIOCGETVIFCNT, pvArg);
    if (iErrNo) {
        MRT6_UNLOCK();
        _ErrorHandle(iErrNo);
        return  (PX_ERROR);
    }
    MRT6_UNLOCK();
    
    return  (ERROR_NONE);
}

#endif
/*********************************************************************************************************
** ��������: __mrtGetSgCnt4
** ��������: ��� IPv4 sg cnt ��Ϣ
** �䡡��  : pvArg     ��Ϣ�ṹ
** �䡡��  : ERROR or OK
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static INT  __mrtGetSgCnt4 (PVOID  pvArg)
{
    INT  iErrNo;
    
    MRT_LOCK();
    iErrNo = ip4_mrt_ioctl(SIOCGETSGCNT, pvArg);
    if (iErrNo) {
        MRT_UNLOCK();
        _ErrorHandle(iErrNo);
        return  (PX_ERROR);
    }
    MRT_UNLOCK();
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: __mrtGetSgCnt6
** ��������: ��� IPv6 sg cnt ��Ϣ
** �䡡��  : pvArg     ��Ϣ�ṹ
** �䡡��  : ERROR or OK
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
#if LWIP_IPV6

static INT  __mrtGetSgCnt6 (PVOID  pvArg)
{
    INT  iErrNo;
    
    MRT6_LOCK();
    iErrNo = ip6_mrt_ioctl(SIOCGETSGCNT, pvArg);
    if (iErrNo) {
        MRT6_UNLOCK();
        _ErrorHandle(iErrNo);
        return  (PX_ERROR);
    }
    MRT6_UNLOCK();
    
    return  (ERROR_NONE);
}

#endif
/*********************************************************************************************************
** ��������: __mrtIoctlInet
** ��������: SIOCGETVIFCNT / SIOCGETSGCNT ... �����ӿ�
** �䡡��  : iFamily    AF_INET / AF_INET6
**           iCmd       SIOCADDRT / SIOCDELRT
**           pvArg      struct arpreq
** �䡡��  : ERROR or OK
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
INT  __mrtIoctlInet (INT  iFamily, INT  iCmd, PVOID  pvArg)
{
    INT  iRet = PX_ERROR;

    if (!pvArg) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    
    switch (iCmd) {
    
    case SIOCGETVIFCNT:
        if (iFamily == AF_INET) {
            iRet = __mrtGetVifCnt4(pvArg);
        } 
#if LWIP_IPV6
          else {
            iRet = __mrtGetVifCnt6(pvArg);
        }
#endif
        break;
        
    case SIOCGETSGCNT:
        if (iFamily == AF_INET) {
            iRet = __mrtGetSgCnt4(pvArg);
        } 
#if LWIP_IPV6
          else {
            iRet = __mrtGetSgCnt6(pvArg);
        }
#endif
        break;
        
    default:
        _ErrorHandle(ENOSYS);
        break;
    }
    
    return  (iRet);
}

#endif                                                                  /*  LW_CFG_NET_EN > 0           */
                                                                        /*  LW_CFG_NET_MROUTER > 0      */
/*********************************************************************************************************
  END
*********************************************************************************************************/
