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
** 文   件   名: if_hwaddr.h
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2019 年 05 月 11 日
**
** 描        述: ARP.
*********************************************************************************************************/

#ifndef __IF_HWADDR_H
#define __IF_HWADDR_H

#include <sys/types.h>

/*********************************************************************************************************
  裁剪控制
*********************************************************************************************************/
#if LW_CFG_NET_EN > 0

#define IFHWADDR_MAXLEN     25

#ifdef __cplusplus
extern "C" {
#endif                                                                  /*  __cplusplus                 */

struct sockaddr_ha {
    u_char  sha_type_hi;                                                /*  hal type high => sa_len     */
    u_char  sha_type_lo;                                                /*  hal type low  => sa_family  */
    u_char  sha_ha[IFHWADDR_MAXLEN];                                    /*  hal hwaddr                  */
    u_char  sha_alen;                                                   /*  hal hwaddr length           */
};

#define HALTYPE_FROM_SA(sa)     (((u_short)(sa)->sa_len << 8) + (sa)->sa_family)
#define HALTYPE_FROM_SAHA(saha) (((u_short)(saha)->sha_type_hi << 8) + (saha)->sha_type_lo)

#define HALALEN_FROM_SA(sa)     (sa)->sa_data[25]
#define HALALEN_FROM_SAHA(saha) (saha)->sha_alen

#ifdef __cplusplus
}
#endif                                                                  /*  __cplusplus                 */

#endif                                                                  /*  LW_CFG_NET_EN               */
#endif                                                                  /*  __IF_HWADDR_H               */
/*********************************************************************************************************
  END
*********************************************************************************************************/
