/*********************************************************************************************************
**
**                                    中国软件开源组织
**
**                                   嵌入式实时操作系统
**
**                                SylixOS(TM)  LW : long wing
**
**                               Copyright All Rights Reserved
**
**--------------文件信息--------------------------------------------------------------------------------
**
** 文   件   名: if_bonding.h
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2018 年 11 月 22 日
**
** 描        述: Bonding 网络接口.
*********************************************************************************************************/

#ifndef __IF_BONDING_H
#define __IF_BONDING_H

#include <sys/types.h>
#include <sys/ioctl.h>

/*********************************************************************************************************
  裁剪控制
*********************************************************************************************************/
#if (LW_CFG_NET_EN > 0) && (LW_CFG_NET_DEV_BONDING_EN > 0)

#include "if.h"

#ifdef __cplusplus
extern "C" {
#endif                                                      /*  __cplusplus                             */

/*********************************************************************************************************
  Bonding 网络控制参数.
  
  bd_index: NETBD_CTL_ADD 返回.
  bd_mode, bd_mon_mode, bd_interval, bd_alive 只针对 NETBD_CTL_ADD 有效.
  bd_mon_mode, bd_interval, bd_alive 只针对 NETBD_MODE_ACTIVE_BACKUP 模式有效.
  bd_interval 只针对 NETBD_MON_MODE_ARP 监控模式有效.
*********************************************************************************************************/
struct net_bonding_ctl {
    int     bd_index;                                       /*  NETBD_CTL_ADD return                    */
    int     bd_mode;                                        /*  Only for NETBD_CTL_ADD set              */
    int     bd_mon_mode;                                    /*  Only for NETBD_CTL_ADD set              */
    int     bd_interval;                                    /*  ARP detect period (milliseconds) mode 1 */
    int     bd_alive;                                       /*  Timeout (milliseconds) mode 1           */
    char    bd_dev[IFNAMSIZ];                               /*  Bonding device name                     */
    char    eth_dev[IFNAMSIZ];                              /*  Sub ethernet device name                */
};

#define NETBD_MODE_BALANCE_RR       0                       /*  High fault tolerance and load balancing */
#define NETBD_MODE_ACTIVE_BACKUP    1                       /*  High fault tolerance (MII ARP Moniting) */
#define NETBD_MODE_BROADCAST        2                       /*  Broadcast transmit                      */

#define NETBD_MON_MODE_TRAFFIC      0                       /*  Traffic detect (default) 'active-backup'*/
#define NETBD_MON_MODE_ARP          1                       /*  ARP detect 'active-backup'              */

#define NETBD_CTL_ADD           _IOWR('b', 0, struct net_bonding_ctl)
#define NETBD_CTL_DELETE        _IOW( 'b', 1, struct net_bonding_ctl)
#define NETBD_CTL_ADD_DEV       _IOW( 'b', 2, struct net_bonding_ctl)
#define NETBD_CTL_DELETE_DEV    _IOW( 'b', 3, struct net_bonding_ctl)
#define NETBD_CTL_ADD_IF        _IOW( 'b', 4, struct net_bonding_ctl)
#define NETBD_CTL_DELETE_IF     _IOW( 'b', 5, struct net_bonding_ctl)
#define NETBD_CTL_CHANGE        _IOW( 'b', 6, struct net_bonding_ctl)

/*********************************************************************************************************
  Bonding 网络设置 MASTER 网卡. (NETBD_MODE_ACTIVE_BACKUP mode only!)
*********************************************************************************************************/
struct net_bonding_device {
    int     bd_index;
    char    bd_dev[IFNAMSIZ];                               /*  Bonding device name                     */
    char    eth_dev[IFNAMSIZ];                              /*  Sub ethernet device name                */
};

#define NETBD_CTL_MASTER_DEV    _IOW( 'b', 6, struct net_bonding_device)
#define NETBD_CTL_MASTER_IF     _IOW( 'b', 7, struct net_bonding_device)

/*********************************************************************************************************
  Bonding 网络监控目标. (NETBD_MODE_ACTIVE_BACKUP mode only!)
*********************************************************************************************************/
struct net_bonding_arp {
    int     bd_index;
    char    bd_dev[IFNAMSIZ];                               /*  Bonding device name                     */
    char    arp_ip_target[64];
};

#define NETBD_CTL_ARP_ADD       _IOW( 'b', 8, struct net_bonding_arp)
#define NETBD_CTL_ARP_DELETE    _IOW( 'b', 9, struct net_bonding_arp)

#define NETBD_CTL_PATH          "/dev/netbd"

#ifdef __cplusplus
}
#endif                                                      /*  __cplusplus                             */

#endif                                                      /*  LW_CFG_NET_EN                           */
                                                            /*  LW_CFG_NET_DEV_BONDING_EN               */
#endif                                                      /*  __IF_BONDING_H                          */
/*********************************************************************************************************
  END
*********************************************************************************************************/
