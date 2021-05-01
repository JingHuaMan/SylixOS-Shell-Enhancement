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
** 文   件   名: lwip_bonding.c
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2018 年 11 月 23 日
**
** 描        述: 网络 Bonding 管理.
*********************************************************************************************************/
#define  __SYLIXOS_STDIO
#define  __SYLIXOS_KERNEL
#include "SylixOS.h"
/*********************************************************************************************************
  裁剪控制
*********************************************************************************************************/
#if (LW_CFG_NET_EN > 0) && (LW_CFG_NET_DEV_BONDING_EN > 0)
#include "net/if_bonding.h"
#include "bonding/netbonding.h"
/*********************************************************************************************************
  驱动与设备
*********************************************************************************************************/
static LW_DEV_HDR       _G_devhdrNetbd;
static INT              _G_iNetbdDrvNum = PX_ERROR;
/*********************************************************************************************************
** 函数名称: _netbdOpen
** 功能描述: 打开网络 Bonding 控制设备
** 输　入  : pdevhdr          控制设备
**           pcName           名称
**           iFlags           方式
**           iMode            方法
** 输　出  : ERROR
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static LONG  _netbdOpen (PLW_DEV_HDR   pdevhdr, 
                         PCHAR         pcName,
                         INT           iFlags, 
                         INT           iMode)
{
    if (pcName == LW_NULL) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "device name invalidate.\r\n");
        _ErrorHandle(ERROR_IO_NO_DEVICE_NAME_IN_PATH);
        return  (PX_ERROR);
    
    } else {
        LW_DEV_INC_USE_COUNT(pdevhdr);
        return  ((LONG)pdevhdr);
    }
}
/*********************************************************************************************************
** 函数名称: _netbdClose
** 功能描述: 关闭网络 Bonding 控制设备
** 输　入  : pdevhdr          控制设备
** 输　出  : ERROR
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static INT  _netbdClose (PLW_DEV_HDR   pdevhdr)
{
    if (pdevhdr) {
        LW_DEV_DEC_USE_COUNT(pdevhdr);
        return  (ERROR_NONE);
    }
    
    return  (PX_ERROR);
}
/*********************************************************************************************************
** 函数名称: _netbdIoctl
** 功能描述: 控制网络 Bonding 控制设备
** 输　入  : pdevhdr          控制设备
**           iCmd             功能
**           lArg             参数
** 输　出  : ERROR
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static INT  _netbdIoctl (PLW_DEV_HDR   pdevhdr, 
                         INT           iCmd, 
                         LONG          lArg)
{
    struct net_bonding_ctl    *pnetbdcrl;
    struct net_bonding_device *pnetbddev;
    struct net_bonding_arp    *pnetbdarp;
    struct stat               *pstat;
    INT                        iRet = PX_ERROR;
    
    switch (iCmd) {
    
    case NETBD_CTL_ADD:
        pnetbdcrl = (struct net_bonding_ctl *)lArg;
        if (pnetbdcrl) {
            iRet = netbd_add(pnetbdcrl->bd_dev, LW_NULL, LW_NULL, LW_NULL, 
                             pnetbdcrl->bd_mode, pnetbdcrl->bd_mon_mode, 
                             pnetbdcrl->bd_interval, pnetbdcrl->bd_alive,
                             &pnetbdcrl->bd_index);
        }
        return  (iRet);
    
    case NETBD_CTL_DELETE:
        pnetbdcrl = (struct net_bonding_ctl *)lArg;
        if (pnetbdcrl) {
            iRet = netbd_delete(pnetbdcrl->bd_dev, pnetbdcrl->bd_index);
        }
        return  (iRet);

    case NETBD_CTL_CHANGE:
        pnetbdcrl = (struct net_bonding_ctl *)lArg;
        if (pnetbdcrl) {
            iRet = netbd_change(pnetbdcrl->bd_dev, pnetbdcrl->bd_mode,
                                pnetbdcrl->bd_mon_mode, pnetbdcrl->bd_interval,
                                pnetbdcrl->bd_alive);
        }
        return  (iRet);

    case NETBD_CTL_ADD_DEV:
        pnetbdcrl = (struct net_bonding_ctl *)lArg;
        if (pnetbdcrl) {
            iRet = netbd_add_dev(pnetbdcrl->bd_dev, pnetbdcrl->bd_index, pnetbdcrl->eth_dev, 0);
        }
        return  (iRet);
    
    case NETBD_CTL_DELETE_DEV:
        pnetbdcrl = (struct net_bonding_ctl *)lArg;
        if (pnetbdcrl) {
            iRet = netbd_delete_dev(pnetbdcrl->bd_dev, pnetbdcrl->bd_index, pnetbdcrl->eth_dev, 0);
        }
        return  (iRet);
        
    case NETBD_CTL_ADD_IF:
        pnetbdcrl = (struct net_bonding_ctl *)lArg;
        if (pnetbdcrl) {
            iRet = netbd_add_dev(pnetbdcrl->bd_dev, pnetbdcrl->bd_index, pnetbdcrl->eth_dev, 1);
        }
        return  (iRet);
    
    case NETBD_CTL_DELETE_IF:
        pnetbdcrl = (struct net_bonding_ctl *)lArg;
        if (pnetbdcrl) {
            iRet = netbd_delete_dev(pnetbdcrl->bd_dev, pnetbdcrl->bd_index, pnetbdcrl->eth_dev, 1);
        }
        return  (iRet);
        
    case NETBD_CTL_MASTER_DEV:
        pnetbddev = (struct net_bonding_device *)lArg;
        if (pnetbddev) {
            iRet = netbd_master_dev(pnetbddev->bd_dev, pnetbddev->bd_index, pnetbddev->eth_dev, 0);
        }
        return  (iRet);
        
    case NETBD_CTL_MASTER_IF:
        pnetbddev = (struct net_bonding_device *)lArg;
        if (pnetbddev) {
            iRet = netbd_master_dev(pnetbddev->bd_dev, pnetbddev->bd_index, pnetbddev->eth_dev, 1);
        }
        return  (iRet);
        
    case NETBD_CTL_ARP_ADD:
        pnetbdarp = (struct net_bonding_arp *)lArg;
        if (pnetbdarp) {
            iRet = netbd_add_arp(pnetbdarp->bd_dev, pnetbdarp->bd_index, pnetbdarp->arp_ip_target);
        }
        return  (iRet);
    
    case NETBD_CTL_ARP_DELETE:
        pnetbdarp = (struct net_bonding_arp *)lArg;
        if (pnetbdarp) {
            iRet = netbd_delete_arp(pnetbdarp->bd_dev, pnetbdarp->bd_index, pnetbdarp->arp_ip_target);
        }
        return  (iRet);
    
    case FIOFSTATGET:
        pstat = (struct stat *)lArg;
        if (pstat) {
            pstat->st_dev     = LW_DEV_MAKE_STDEV(pdevhdr);
            pstat->st_ino     = (ino_t)0;                               /*  相当于唯一节点              */
            pstat->st_mode    = 0600 | S_IFCHR;
            pstat->st_nlink   = 1;
            pstat->st_uid     = 0;
            pstat->st_gid     = 0;
            pstat->st_rdev    = 1;
            pstat->st_size    = 0;
            pstat->st_blksize = 0;
            pstat->st_blocks  = 0;
            pstat->st_atime   = API_RootFsTime(LW_NULL);
            pstat->st_mtime   = API_RootFsTime(LW_NULL);
            pstat->st_ctime   = API_RootFsTime(LW_NULL);
            iRet              = ERROR_NONE;
        }
        return  (iRet);
    
    default:
        _ErrorHandle(ERROR_IO_UNKNOWN_REQUEST);
        return  (PX_ERROR);
    }
    
    return  (iRet);
}
/*********************************************************************************************************
** 函数名称: _netBondingDrvInit
** 功能描述: 初始化网络 Bonding 管理
** 输　入  : NONE
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static INT  _netBondingDrvInit (VOID)
{
    if (_G_iNetbdDrvNum <= 0) {
        _G_iNetbdDrvNum  = iosDrvInstall(_netbdOpen,
                                         LW_NULL,
                                         _netbdOpen,
                                         _netbdClose,
                                         LW_NULL,
                                         LW_NULL,
                                         _netbdIoctl);
        DRIVER_LICENSE(_G_iNetbdDrvNum,     "Dual BSD/GPL->Ver 1.0");
        DRIVER_AUTHOR(_G_iNetbdDrvNum,      "Han.hui");
        DRIVER_DESCRIPTION(_G_iNetbdDrvNum, "net bonding management driver.");
    }
    
    return  ((_G_iNetbdDrvNum == (PX_ERROR)) ? (PX_ERROR) : (ERROR_NONE));
}
/*********************************************************************************************************
** 函数名称: _netBridgeDevCreate
** 功能描述: 安装 Bonding 设备
** 输　入  : NONE
** 输　出  : 设备是否创建成功
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static INT  _netBondingDevCreate (VOID)
{
    if (_G_iNetbdDrvNum <= 0) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "no driver.\r\n");
        _ErrorHandle(ERROR_IO_NO_DRIVER);
        return  (PX_ERROR);
    }
    
    if (iosDevAddEx(&_G_devhdrNetbd, NETBD_CTL_PATH, _G_iNetbdDrvNum, DT_CHR)) {
        return  (PX_ERROR);
    }
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: __tshellNetbd
** 功能描述: 系统命令 "netbonding"
** 输　入  : iArgC         参数个数
**           ppcArgV       参数表
** 输　出  : 0
** 全局变量:
** 调用模块:
*********************************************************************************************************/
#if LW_CFG_SHELL_EN > 0

static INT  __tshellNetbd (INT  iArgC, PCHAR  *ppcArgV)
{
    INT    iRet;
    INT    iIndex;
    INT    iMode, iMonMode, iInterval, iAlive;

    if (iArgC < 3) {
        goto    __arg_error;
    }
    
    if ((lib_strcmp(ppcArgV[1], "addbd")    == 0) ||
        (lib_strcmp(ppcArgV[1], "changebd") == 0)) {
        if (iArgC < 7) {
            goto    __arg_error;
        }
        
        if (lib_strcmp(ppcArgV[3], "ab") == 0) {
            iMode = NETBD_MODE_ACTIVE_BACKUP;
        } else if (lib_strcmp(ppcArgV[3], "bl") == 0) {
            iMode = NETBD_MODE_BALANCE_RR;
        } else if (lib_strcmp(ppcArgV[3], "bc") == 0) {
            iMode = NETBD_MODE_BROADCAST;
        } else {
            goto    __arg_error;
        }
        
        if (lib_strcmp(ppcArgV[4], "-t") == 0) {
            iMonMode = NETBD_MON_MODE_TRAFFIC;
        } else if (lib_strcmp(ppcArgV[4], "-a") == 0) {
            iMonMode = NETBD_MON_MODE_ARP;
        } else {
            goto    __arg_error;
        }
        
        if (sscanf(ppcArgV[5], "%d", &iInterval) != 1) {
            goto    __arg_error;
        }
        if (sscanf(ppcArgV[6], "%d", &iAlive) != 1) {
            goto    __arg_error;
        }

        if (lib_strcmp(ppcArgV[1], "addbd") == 0) {
            iRet = netbd_add(ppcArgV[2], LW_NULL, LW_NULL, LW_NULL,
                             iMode, iMonMode, iInterval, iAlive, &iIndex);
            if (iRet) {
                fprintf(stderr, "can not add net bonding device: %s!\n", lib_strerror(errno));
            } else {
                printf("net bonding device add ok, if index: %d.\n", iIndex);
            }

        } else {
            iRet = netbd_change(ppcArgV[2], iMode, iMonMode, iInterval, iAlive);
            if (iRet) {
                fprintf(stderr, "can not change net bonding device: %s!\n", lib_strerror(errno));
            } else {
                printf("net bonding device change ok.\n");
            }
        }
    
    } else {
        if (iArgC == 3) {
            if (lib_strcmp(ppcArgV[1], "delbd") == 0) {
                iRet = netbd_delete(ppcArgV[2], 0);
                if (iRet) {
                    fprintf(stderr, "can not delete net bonding device: %s!\n", lib_strerror(errno));
                } else {
                    printf("net bonding device delete.\n");
                }
                
            } else if (lib_strcmp(ppcArgV[1], "show") == 0) {
                iRet = netbd_show_dev(ppcArgV[2], 0, STD_OUT);
                if (iRet) {
                    fprintf(stderr, "can not show net bonding device: %s!\n", lib_strerror(errno));
                }
            
            }  else {
                goto    __arg_error;
            }
        
        } else if (iArgC == 4) {
            if (lib_strcmp(ppcArgV[1], "adddev") == 0) {
                iRet = netbd_add_dev(ppcArgV[2], 0, ppcArgV[3], 0);
                if (iRet) {
                    fprintf(stderr, "can not add device '%s' to bonding '%s'!\n", ppcArgV[3], ppcArgV[2]);
                }
            
            } else if (lib_strcmp(ppcArgV[1], "deldev") == 0) {
                iRet = netbd_delete_dev(ppcArgV[2], 0, ppcArgV[3], 0);
                if (iRet) {
                    fprintf(stderr, "can not delete device '%s' from bonding '%s'!\n", ppcArgV[3], ppcArgV[2]);
                }
            
            } else if (lib_strcmp(ppcArgV[1], "addif") == 0) {
                iRet = netbd_add_dev(ppcArgV[2], 0, ppcArgV[3], 1);
                if (iRet) {
                    fprintf(stderr, "can not add if '%s' to bonding '%s'!\n", ppcArgV[3], ppcArgV[2]);
                }
            
            } else if (lib_strcmp(ppcArgV[1], "delif") == 0) {
                iRet = netbd_delete_dev(ppcArgV[2], 0, ppcArgV[3], 1);
                if (iRet) {
                    fprintf(stderr, "can not delete if '%s' from bonding '%s'!\n", ppcArgV[3], ppcArgV[2]);
                }
            
            } else if (lib_strcmp(ppcArgV[1], "masterdev") == 0) {
                iRet = netbd_master_dev(ppcArgV[2], 0, ppcArgV[3], 0);
                if (iRet) {
                    fprintf(stderr, "can not set device '%s' to bonding '%s' MASTER device!\n", ppcArgV[3], ppcArgV[2]);
                }
            
            } else if (lib_strcmp(ppcArgV[1], "masterif") == 0) {
                iRet = netbd_master_dev(ppcArgV[2], 0, ppcArgV[3], 1);
                if (iRet) {
                    fprintf(stderr, "can not set if '%s' to bonding '%s' MASTER device!\n", ppcArgV[3], ppcArgV[2]);
                }
                
            } else if (lib_strcmp(ppcArgV[1], "addarp") == 0) {
                iRet = netbd_add_arp(ppcArgV[2], 0, ppcArgV[3]);
                if (iRet) {
                    fprintf(stderr, "can not add ip '%s' to bonding '%s' ARP detect target!\n", ppcArgV[3], ppcArgV[2]);
                }
            
            } else if (lib_strcmp(ppcArgV[1], "delarp") == 0) {
                iRet = netbd_delete_arp(ppcArgV[2], 0, ppcArgV[3]);
                if (iRet) {
                    fprintf(stderr, "can not delete ip '%s' from bonding '%s' ARP detect target!\n", ppcArgV[3], ppcArgV[2]);
                }
            
            } else {
                goto    __arg_error;
            }
        
        } else {
            goto    __arg_error;
        }
    }
    
    return  (iRet);

__arg_error:
    fprintf(stderr, "arguments error!\n");
    return  (-ERROR_TSHELL_EPARAM);
}

#endif                                                                  /*  LW_CFG_SHELL_EN > 0         */
/*********************************************************************************************************
** 函数名称: _netBondingInit
** 功能描述: 安装 Bonding 设备
** 输　入  : NONE
** 输　出  : 设备是否创建成功
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
INT  _netBondingInit (VOID)
{
    if (_netBondingDrvInit()) {
        return  (PX_ERROR);
    }
    
    if (_netBondingDevCreate()) {
        return  (PX_ERROR);
    }
    
#if LW_CFG_SHELL_EN > 0
    API_TShellKeywordAdd("netbonding", __tshellNetbd);
    API_TShellFormatAdd("netbonding", " [...]");
    API_TShellHelpAdd("netbonding",   "add / delete / control net bonding.\n"
                      "eg. netbonding show bond0              (Show all net device in 'bond0' net bonding)\n"
                      "    netbonding addbd bond0 [...]       (Add a net bonding named 'bond0')\n"
                      "    netbonding changebd bond0 [...]    (Change a net bonding named 'bond0')\n\n"
                      "    [...]: [ab|bl|bc] [-t|-a] [interval] [time to alive]\n\n"
                      "    ab           : Active Backup mode\n"
                      "    bl           : Balance RR mode\n"
                      "    bc           : Broadcast mode\n"
                      "    -t           : Traffic detect in 'Active Backup' mode\n"
                      "    -a           : ARP detect in 'Active Backup' mode\n"
                      "    interval     : ARP detect interval (milliseconds)\n"
                      "    time to alive: When detect OK how long does it take to active (milliseconds)\n\n"
                      "    netbonding delbd bond0             (Delete a net bonding named 'bond0')\n"
                      "    netbonding adddev bond0 ethdev0    (Add a net device in net bonding named 'bond0')\n"
                      "    netbonding deldev bond0 ethdev0    (Delete a net device from net bonding named 'bond0')\n"
                      "    netbonding addif bond0 en1         (Same as 'adddev' but use interface name)\n"
                      "    netbonding delif bond0 en1         (Same as 'deldev' but use interface name)\n"
                      "    netbonding masterdev bond0 ethdev0 (Make net device 'ethdev0' as a master device in 'bond0')\n"
                      "    netbonding masterif bond0 en1      (Same as 'masterdev' but use interface name)\n"
                      "    netbonding addarp bond0 10.0.0.1   (Add a ARP detect target to net bonding named 'bond0')\n"
                      "    netbonding delarp bond0 10.0.0.1   (Delete a ARP detect target from net bonding named 'bond0')\n");
#endif                                                                  /*  LW_CFG_SHELL_EN > 0         */

    return  (ERROR_NONE);
}

#endif                                                                  /*  LW_CFG_NET_EN               */
                                                                        /*  LW_CFG_NET_DEV_BONDING_EN   */
/*********************************************************************************************************
  END
*********************************************************************************************************/
