/**
 * @file
 * Lwip platform independent net bonding.
 * This set of driver interface shields the netif details, 
 * as much as possible compatible with different versions of LwIP
 * Verification using sylixos(tm) real-time operating system
 */

/*
 * Copyright (c) 2006-2018 SylixOS Group.
 * All rights reserved. 
 * 
 * Redistribution and use in source and binary forms, with or without modification, 
 * are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission. 
 * 4. This code has been or is applying for intellectual property protection 
 *    and can only be used with acoinfo software products.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED 
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF 
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT 
 * SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, 
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT 
 * OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS 
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN 
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING 
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY 
 * OF SUCH DAMAGE.
 * 
 * Author: Han.hui <hanhui@acoinfo.com>
 *
 */

#ifndef __NETBONDING_H
#define __NETBONDING_H

#include "netdev.h"

#if LW_CFG_NET_DEV_BONDING_EN > 0

/* add and delete net bonding virtual device */
int  netbd_add(const char *bddev, const char *ip, 
               const char *netmask, const char *gw, 
               int mode, int mon_mode, int interval,
               int alive, int *index);
int  netbd_delete(const char *bddev, int bdindex);

/* change net bonding virtual device */
int  netbd_change(const char *bddev, int mode,
                  int mon_mode, int interval,
                  int alive);

/* add or delete a net device to net bonding virtual device */
int  netbd_add_dev(const char *bddev, int bdindex, const char *sub, int sub_is_ifname);
int  netbd_delete_dev(const char *bddev, int bdindex, const char *sub, int sub_is_ifname);

/* net bonding set master device */
int  netbd_master_dev(const char *bddev, int bdindex, const char *sub, int sub_is_ifname);

/* add or delete a arp detect target to net bonding virtual device */
int  netbd_add_arp(const char *bddev, int bdindex, const char *ip);
int  netbd_delete_arp(const char *bddev, int bdindex, const char *ip);

/* net bonding show all device in bonding virtual device */
int  netbd_show_dev(const char *bddev, int bdindex, int fd);

#endif /* LW_CFG_NET_DEV_BONDING_EN */
#endif /* __NETBONDING_H */
/*
 * end
 */
