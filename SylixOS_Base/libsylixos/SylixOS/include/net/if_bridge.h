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
** ��   ��   ��: if_bridge.h
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2017 �� 12 �� 05 ��
**
** ��        ��: �Ž�����ӿ�.
*********************************************************************************************************/

#ifndef __IF_BRIDGE_H
#define __IF_BRIDGE_H

#include <sys/types.h>
#include <sys/ioctl.h>

/*********************************************************************************************************
  �ü�����
*********************************************************************************************************/
#if (LW_CFG_NET_EN > 0) && (LW_CFG_NET_DEV_BRIDGE_EN > 0)

#include "if.h"

#ifdef __cplusplus
extern "C" {
#endif                                                      /*  __cplusplus                             */

/*********************************************************************************************************
  ���ſ��Ʋ���.
*********************************************************************************************************/
struct net_bridge_ctl {
    int     br_index;                                       /*  NETBR_CTL_ADD return                    */
    char    br_dev[IFNAMSIZ];                               /*  Bridge device name                      */
    char    eth_dev[IFNAMSIZ];                              /*  Sub ethernet device name                */
};

#define NETBR_CTL_ADD           _IOWR('b', 0, struct net_bridge_ctl)
#define NETBR_CTL_DELETE        _IOW( 'b', 1, struct net_bridge_ctl)
#define NETBR_CTL_ADD_DEV       _IOW( 'b', 2, struct net_bridge_ctl)
#define NETBR_CTL_DELETE_DEV    _IOW( 'b', 3, struct net_bridge_ctl)
#define NETBR_CTL_ADD_IF        _IOW( 'b', 4, struct net_bridge_ctl)
#define NETBR_CTL_DELETE_IF     _IOW( 'b', 5, struct net_bridge_ctl)
#define NETBR_CTL_CACHE_FLUSH   FIOFLUSH

#define NETBR_CTL_PATH          "/dev/netbr"

#ifdef __cplusplus
}
#endif                                                      /*  __cplusplus                             */

#endif                                                      /*  LW_CFG_NET_EN                           */
                                                            /*  LW_CFG_NET_DEV_BRIDGE_EN                */
#endif                                                      /*  __IF_BRIDGE_H                           */
/*********************************************************************************************************
  END
*********************************************************************************************************/
