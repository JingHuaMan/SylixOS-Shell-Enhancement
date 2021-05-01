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
** 文   件   名: if_vlan.h
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2017 年 12 月 29 日
**
** 描        述: vlan.
*********************************************************************************************************/

#ifndef __IF_VLAN_H
#define __IF_VLAN_H

#include <sys/types.h>
#include <sys/ioctl.h>

/*********************************************************************************************************
  裁剪控制
*********************************************************************************************************/
#if LW_CFG_NET_EN > 0 && LW_CFG_NET_VLAN_EN > 0

#include "if.h"

#ifdef __cplusplus
extern "C" {
#endif

/*********************************************************************************************************
  Configuration structure for SIOCSETVLAN and SIOCGETVLAN ioctls.
  vlr_tag = 0       : All packet we want received, we send standard ethernet packet.
  vlr_tag = 4095    : Close this interface VLAN setting, Only standard ethernet packet can received.
  vlr_tag = 1..4094 : Set VLAN tag, we only received same VLAN tag packet.
*********************************************************************************************************/

struct vlanreq {
    char        vlr_ifname[IFNAMSIZ];
    u_short     vlr_tag;                            /*  12-bit vlan id, 1..4094, 0 for priority-tagged  */
    u_short     vlr_pri;                            /*  3-bit vlan priority, 0..7                       */
};

struct vlanreq_list {
    u_long          vlrl_bcnt;                      /* struct vlanreq buffer count                      */
    u_long          vlrl_num;                       /* system return how many vlanreq entry in buffer   */
    u_long          vlrl_total;                     /* system return total number vlanreq entry         */
    struct vlanreq *vlrl_buf;                       /* vlanreq list buffer                              */
};

#define SIOCGETVLAN  _IOWR('i', 50, struct vlanreq)
#define SIOCSETVLAN  _IOWR('i', 51, struct vlanreq)
#define SIOCLSTVLAN  _IOWR('i', 52, struct vlanreq_list)

#ifdef __cplusplus
}
#endif

#endif                                              /*  LW_CFG_NET_EN                                   */
                                                    /*  LW_CFG_NET_VNETDEV_EN                           */
#endif                                              /*  __IF_VLAN_H                                     */
/*********************************************************************************************************
  END
*********************************************************************************************************/
