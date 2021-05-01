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
** ��   ��   ��: smethnd.h
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2017 �� 07 �� 24 ��
**
** ��        ��: �����ڴ�������������֧��.
*********************************************************************************************************/

#ifndef __SMETHND_H
#define __SMETHND_H

#include "netdev.h"

/*********************************************************************************************************
  share memory ethernet device configuration
*********************************************************************************************************/
struct smethnd_mem {
    addr_t config_base;                                                 /*  this net device config      */
    size_t config_size;                                                 /*  must biger than 128 bytes   */

    addr_t packet_base;                                                 /*  this net device buffer      */
    size_t packet_size;                                                 /*  must biger than 64 Kbytes   */
};

struct smethnd_netdev {
    struct netdev netdev;                                               /*  net device                  */

    /*
     *  user MUST set following members before calling this module api.
     *  If you want use this netdev with ip forward, you MUST set chksum_flags NETDEV_CHKSUM_ENABLE_ALL
     *  or all NETDEV_CHKSUM_GEN_*
     */
    UINT32 chksum_flags;                                                /*  netdev chksum_flags         */

    int localno;                                                        /*  this number 0 ~ 127         */
    int totalnum;                                                       /*  total number MAX 128        */
    struct smethnd_mem mem;                                             /*  this net device mem info    */

    int  (*meminfo)(struct smethnd_netdev *smethnd, int remoteno, struct smethnd_mem *mem);
    void (*notify)(struct smethnd_netdev *smethnd, int remoteno);       /*  notify remote net device    */
};

/*********************************************************************************************************
  share memory ethernet driver functions
*********************************************************************************************************/

INT  smethndInit(struct smethnd_netdev *smethnd, const char *ip, const char *netmask, const char *gw);
VOID smethndNotify(struct smethnd_netdev *smethnd);

#endif                                                                  /*  __SMETHND_H                 */
/*********************************************************************************************************
  END
*********************************************************************************************************/
