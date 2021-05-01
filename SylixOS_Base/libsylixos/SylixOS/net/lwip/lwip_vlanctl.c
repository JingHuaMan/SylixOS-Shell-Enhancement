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
** ��   ��   ��: lwip_vlanctl.c
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2017 �� 12 �� 29 ��
**
** ��        ��: ioctl vlan ֧��.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "SylixOS.h"
/*********************************************************************************************************
  �ü�����
*********************************************************************************************************/
#if LW_CFG_NET_EN > 0 && LW_CFG_NET_VLAN_EN > 0
#include "net/if_vlan.h"
#include "net/if_lock.h"
#include "vlan/eth_vlan.h"
/*********************************************************************************************************
** ��������: __ifVlanSet
** ��������: ���� VLAN ����
** �䡡��  : pvlanreq    ����
** �䡡��  : ERROR or OK
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static INT  __ifVlanSet (const struct vlanreq  *pvlanreq)
{
    INT  iRet = ethernet_vlan_set(pvlanreq);
    
    if (iRet) {
        if (iRet == ETH_VLAN_ENODEV) {
            _ErrorHandle(ENODEV);
        } else {
            _ErrorHandle(ENOTSUP);
        }
    }
    
    return  (iRet);
}
/*********************************************************************************************************
** ��������: __ifVlanGet
** ��������: ���� VLAN ����
** �䡡��  : pvlanreq    ����
** �䡡��  : ERROR or OK
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static INT  __ifVlanGet (struct vlanreq  *pvlanreq)
{
    INT  iRet = ethernet_vlan_get(pvlanreq);
    
    if (iRet) {
        if (iRet == ETH_VLAN_ENODEV) {
            _ErrorHandle(ENODEV);
        } else {
            _ErrorHandle(ENOTSUP);
        }
    }
    
    return  (iRet);
}
/*********************************************************************************************************
** ��������: __ifVlanWalk
** ��������: ���� IPv4 ·����Ϣ
** �䡡��  : pvlanreq   vlan ��Ϣ
**           pvlrlist   �û�����
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static VOID  __ifVlanWalk (struct vlanreq *pvlanreq, 
                           struct vlanreq_list *pvlrlist)
{
    if (pvlrlist->vlrl_num < pvlrlist->vlrl_bcnt) {
        struct vlanreq *pvlanreqSave = &pvlrlist->vlrl_buf[pvlrlist->vlrl_num];
        *pvlanreqSave = *pvlanreq;
        pvlrlist->vlrl_num++;
    }
}
/*********************************************************************************************************
** ��������: __ifVlanLst
** ��������: ��ȡ���� VLAN ����
** �䡡��  : prtelist  ·����Ϣ����
** �䡡��  : ERROR or OK
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static INT  __ifVlanLst (struct vlanreq_list *pvlrlist)
{
    UINT    uiTotal;
    
    pvlrlist->vlrl_num = 0;
    
    ethernet_vlan_total(&uiTotal);
    pvlrlist->vlrl_total = uiTotal;
    if (!pvlrlist->vlrl_bcnt || !pvlrlist->vlrl_buf) {
        return  (ERROR_NONE);
    }
    ethernet_vlan_traversal(__ifVlanWalk, pvlrlist, LW_NULL, LW_NULL, LW_NULL, LW_NULL, LW_NULL);
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: __ifIoctlVlan
** ��������: SIOCGETVLAN / SIOCSETVLAN / SIOCLSTVLAN �����ӿ�
** �䡡��  : iCmd       SIOCGETVLAN / SIOCSETVLAN / SIOCLSTVLAN
**           pvArg      struct vlanreq / struct vlanreq_list
** �䡡��  : ERROR or OK
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
INT  __ifIoctlVlan (INT  iCmd, PVOID  pvArg)
{
    struct vlanreq  *pvlanreq = (struct vlanreq *)pvArg;
    INT              iRet     = PX_ERROR;
    
    if (!pvlanreq) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    
    switch (iCmd) {
    
    case SIOCSETVLAN:
        LWIP_IF_LIST_LOCK(LW_FALSE);
        iRet = __ifVlanSet(pvlanreq);
        LWIP_IF_LIST_UNLOCK();
        break;
        
    case SIOCGETVLAN:
        LWIP_IF_LIST_LOCK(LW_FALSE);
        iRet = __ifVlanGet(pvlanreq);
        LWIP_IF_LIST_UNLOCK();
        break;
    
    case SIOCLSTVLAN:
        LWIP_IF_LIST_LOCK(LW_FALSE);
        iRet = __ifVlanLst((struct vlanreq_list *)pvArg);
        LWIP_IF_LIST_UNLOCK();
        break;
        
    default:
        _ErrorHandle(ENOSYS);
        break;
    }
    
    return  (iRet);
}

#endif                                                                  /*  LW_CFG_NET_EN               */
                                                                        /*  LW_CFG_NET_VLAN_EN          */
/*********************************************************************************************************
  END
*********************************************************************************************************/
