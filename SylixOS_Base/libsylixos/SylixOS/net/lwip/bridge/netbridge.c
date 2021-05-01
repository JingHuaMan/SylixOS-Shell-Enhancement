/**
 * @file
 * Lwip platform independent net bridge.
 * This set of driver interface shields the netif details, 
 * as much as possible compatible with different versions of LwIP
 * Verification using sylixos(tm) real-time operating system
 */

/*
 * Copyright (c) 2006-2017 SylixOS Group.
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

#define __SYLIXOS_KERNEL
#include "stdio.h"
#include "string.h"
#include "netbridge.h"

#if LW_CFG_NET_DEV_BRIDGE_EN > 0

#include "lwip/mem.h"
#include "lwip/netif.h"
#include "lwip/netifapi.h"
#include "net/if_lock.h"
#include "net/if_flags.h"
#include "net/if_ether.h"
#include "netif/etharp.h"

/* net bridge sub device mac table */
#define NETBRIDGE_MAC_CACHE_SHIFT   (5) /* default 32 mac address cache for each ethernet port */
#define NETBRIDGE_MAC_CACHE_SIZE    (1 << NETBRIDGE_MAC_CACHE_SHIFT)
#define NETBRIDGE_MAC_CACHE_MASK    (NETBRIDGE_MAC_CACHE_SIZE - 1)
#define NETBRIDGE_MAC_CACHE_TTL     (50) /* default time to live is (s) */

/* net bridge mac hash */
typedef struct netbr_mcache {
  int ttl; /* mac cache time to live */
  UINT8 mac[ETH_ALEN]; /* mac cache */
} netbr_mcache_t;

/* net bridge sub device */
typedef struct netbr_eth {
  LW_LIST_LINE list; /* sub ethernet device list */
  netdev_t *netdev; /* sub ethernet device */
  netdev_t *netdev_br; /* bridge net device */
  netif_input_fn input; /* old input function */
  netbr_mcache_t mcache[NETBRIDGE_MAC_CACHE_SIZE]; /* mac cache */
} netbr_eth_t;

/* net bridge device */
typedef struct netbr {
#define NETBRIDGE_MAGIC   0xf7e34a82
  UINT32 magic_no;  /* MUST be NETBRIDGE_MAGIC */
  netdev_t netdev_br;  /* net bridge net device */
  sys_mutex_t lock; /* net bridge lock */
  LW_LIST_LINE_HEADER eth_list; /* sub ethernet device list */
} netbr_t;

/* ethernet zero address */
static UINT8 netbr_zeroaddr[ETH_ALEN] = { 0, 0, 0, 0, 0, 0 };

/* net bridge device lock */
#define NETBR_LOCK(netbr)   sys_mutex_lock(&((netbr)->lock))
#define NETBR_UNLOCK(netbr) sys_mutex_unlock(&((netbr)->lock))

/* net bridge transmit
   'netdev' is net bridge device */
static int  netbr_transmit (struct netdev *netdev, struct pbuf *p)
{
  int found, key;
  netbr_t *netbr = (netbr_t *)netdev->priv;
  netbr_eth_t *netbr_eth;
  netbr_mcache_t *mac;
  LW_LIST_LINE *pline;
  struct ethhdr *eh = (struct ethhdr *)p->payload;
  
  found = 0;
  
  NETBR_LOCK(netbr);
  if (!(eh->h_dest[0] & 1)) { /* not broadcast */
    for (pline = netbr->eth_list; pline != NULL; pline = _list_line_get_next(pline)) {
      netbr_eth = (netbr_eth_t *)pline;
      key = (eh->h_dest[ETH_ALEN - 1] & NETBRIDGE_MAC_CACHE_MASK);
      mac = &netbr_eth->mcache[key];
      if (mac->ttl && !lib_memcmp(mac->mac, eh->h_dest, ETH_ALEN)) {
        found = 1; /* found mac cache */
        if (netbr_eth->netdev->if_flags & IFF_RUNNING) { /* is linkup ? */
          netbr_eth->netdev->drv->transmit(netbr_eth->netdev, p); /* send packet to all port with mac address in cache */
        }
      }
    }
  }
  
  if (!found) { /* send packet to all ports */
    for (pline = netbr->eth_list; pline != NULL; pline = _list_line_get_next(pline)) {
      netbr_eth = (netbr_eth_t *)pline;
      if (netbr_eth->netdev->if_flags & IFF_RUNNING) { /* is linkup ? */
        netbr_eth->netdev->drv->transmit(netbr_eth->netdev, p);
      }
    }
  }
  NETBR_UNLOCK(netbr);
  
  netdev_linkinfo_xmit_inc(netdev);
  netdev_statinfo_total_add(netdev, LINK_OUTPUT, p->tot_len);
  if (eh->h_dest[0] & 1) {
    netdev_statinfo_mcasts_inc(netdev, LINK_OUTPUT);
  } else {
    netdev_statinfo_ucasts_inc(netdev, LINK_OUTPUT);
  }
  
  return (0);
}

/* net bridge receive 
   SylixOS net bridge do not use this function */
static void  netbr_receive (struct netdev *netdev, int (*input)(struct netdev *, struct pbuf *))
{
  _DebugHandle(__ERRORMESSAGE_LEVEL, "netbr_receive() called!\r\n");
}

/* net bridge rxmode */
static int netbr_rxmode (struct netdev *netdev, int flags)
{
  /* We work in a promisc mode, here do nothing */
  return (0);
}

/* net bridge input 
   'netif' is sub erhernet device */
static err_t  netbr_input (struct pbuf *p, struct netif *netif)
{
  int found, key;
  netdev_t *netdev = (netdev_t *)(netif->state); /* sub ethernet device */
  netbr_eth_t *netbr_eth = (netbr_eth_t *)netif->ext_eth;
  netdev_t *netdev_br = netbr_eth->netdev_br; /* bridge device */
  netbr_t *netbr = (netbr_t *)netdev_br->priv;
  struct netif *netif_br = (struct netif *)netdev_br->sys;
  netbr_mcache_t *mac;
  LW_LIST_LINE *pline;
  struct eth_hdr *eh = (struct eth_hdr *)p->payload;
  int mcast = eh->dest.addr[0] & 1;
  
  SYS_ARCH_DECL_PROTECT(lev);
  
  if (netdev->init_flags & NETDEV_INIT_TIGHT) {
    u16_t type;
    if (LW_UNLIKELY(p->len < SIZEOF_ETH_HDR)) {
      goto to_sub;
    }
    type = eh->type;
    if (type == PP_HTONS(ETHTYPE_VLAN)) {
      struct eth_vlan_hdr *vlan = (struct eth_vlan_hdr *)(((char *)eh) + SIZEOF_ETH_HDR);
      if (LW_UNLIKELY(p->len < SIZEOF_ETH_HDR + SIZEOF_VLAN_HDR)) {
        goto to_sub;
      }
      type = vlan->tpid;
    }
    switch (type) {
    case PP_HTONS(ETHTYPE_ARP):
    case PP_HTONS(ETHTYPE_RARP):
    case PP_HTONS(ETHTYPE_PPPOE):
    case PP_HTONS(ETHTYPE_PPPOEDISC):
    case PP_HTONS(ETHTYPE_IP):
    case PP_HTONS(ETHTYPE_IPV6):
      goto to_br;

    default:
      goto to_sub;
    }

to_sub:
    if (netbr_eth->input(p, netif)) {
      netdev_linkinfo_drop_inc(netdev);
      netdev_statinfo_discards_inc(netdev, LINK_INPUT);
      return (ERR_IF);

    } else {
      netdev_linkinfo_recv_inc(netdev);
      netdev_statinfo_total_add(netdev, LINK_INPUT, (p->tot_len - ETH_PAD_SIZE));
      if (mcast) {
        netdev_statinfo_mcasts_inc(netdev, LINK_INPUT);
      } else {
        netdev_statinfo_ucasts_inc(netdev, LINK_INPUT);
      }
      return (ERR_OK);
    }
  }

to_br:
  if (!(netif_br->flags & NETIF_FLAG_UP)) {
    return (ERR_IF); /* bridge was down */
  }
  
  /* update mac cache */
  key = (eh->src.addr[ETH_ALEN - 1] & NETBRIDGE_MAC_CACHE_MASK);
  mac = &netbr_eth->mcache[key];
  
  SYS_ARCH_PROTECT(lev);
  MEMCPY(mac->mac, eh->src.addr, ETH_ALEN);
  mac->ttl = NETBRIDGE_MAC_CACHE_TTL; /* update the cache table */
  SYS_ARCH_UNPROTECT(lev);
  
  if (eh->dest.addr[0] & 1) {  /* broadcast */
#if ETH_PAD_SIZE
    pbuf_header(p, -ETH_PAD_SIZE);
#endif

    NETBR_LOCK(netbr);
    for (pline = netbr->eth_list; pline != NULL; pline = _list_line_get_next(pline)) {
      netbr_eth = (netbr_eth_t *)pline;
      if (netbr_eth->netdev != netdev) {
        if (netbr_eth->netdev->if_flags & IFF_RUNNING) { /* is linkup ? */
          netbr_eth->netdev->drv->transmit(netbr_eth->netdev, p); /* send to all ports */
        }
      }
    }
    NETBR_UNLOCK(netbr);
    
#if ETH_PAD_SIZE
    pbuf_header(p, ETH_PAD_SIZE);
#endif
    goto input_p;
  
  } else {  /* unicast */
    if (!lib_memcmp(eh->dest.addr, netdev_br->hwaddr, ETH_ALEN)) { /* to me? */
      goto input_p;

    } else {  /* forward */
#if ETH_PAD_SIZE
      pbuf_header(p, -ETH_PAD_SIZE);
#endif
      found = 0;
      
      NETBR_LOCK(netbr);
      for (pline = netbr->eth_list; pline != NULL; pline = _list_line_get_next(pline)) {
        netbr_eth = (netbr_eth_t *)pline;
        if (netbr_eth->netdev != netdev) {
          key = (eh->dest.addr[ETH_ALEN - 1] & NETBRIDGE_MAC_CACHE_MASK);
          mac = &netbr_eth->mcache[key];
          if (mac->ttl && !lib_memcmp(mac->mac, eh->dest.addr, ETH_ALEN)) {
            found = 1;
            if (netbr_eth->netdev->if_flags & IFF_RUNNING) { /* is linkup ? */
              netbr_eth->netdev->drv->transmit(netbr_eth->netdev, p); /* send packet to all port with mac address in cache */
            }
          }
        }
      }
      
      if (!found) {
        for (pline = netbr->eth_list; pline != NULL; pline = _list_line_get_next(pline)) {
          netbr_eth = (netbr_eth_t *)pline;
          if (netbr_eth->netdev != netdev) { /* not me */
            if (netbr_eth->netdev->if_flags & IFF_RUNNING) { /* is linkup ? */
              netbr_eth->netdev->drv->transmit(netbr_eth->netdev, p); /* send to all ports */
            }
          }
        }
      }
      NETBR_UNLOCK(netbr);
      
#if ETH_PAD_SIZE
      pbuf_header(p, ETH_PAD_SIZE);
#endif
      if (netif_br->flags2 & NETIF_FLAG2_PROMISC) { /* bridge port is promisc? */
        goto input_p;
      }
      
      pbuf_free(p);
      return (ERR_OK);
    }
  }
  
input_p: /* TODO: this function may be parallelization, and statistical variables should be locked */
  if (netif_br->input(p, netif_br)) { /* send to our tcpip stack */
    netdev_linkinfo_drop_inc(netdev_br);
    netdev_statinfo_discards_inc(netdev_br, LINK_INPUT);
    return (ERR_IF);
    
  } else {
    netdev_linkinfo_recv_inc(netdev_br);
    netdev_statinfo_total_add(netdev_br, LINK_INPUT, (p->tot_len - ETH_PAD_SIZE));
    if (mcast) {
      netdev_statinfo_mcasts_inc(netdev_br, LINK_INPUT);
    } else {
      netdev_statinfo_ucasts_inc(netdev_br, LINK_INPUT);
    }
    return (ERR_OK);
  }
}

/* net bridge link watchdog function */
static void  netbr_wd (netdev_t *netdev_br)
{
  netbr_t *netbr = (netbr_t *)netdev_br->priv;
  netbr_eth_t *netbr_eth;
  LW_LIST_LINE *pline;
  int i;
  
  SYS_ARCH_DECL_PROTECT(lev);
  
  NETBR_LOCK(netbr);
  for (pline = netbr->eth_list; pline != NULL; pline = _list_line_get_next(pline)) {
    netbr_eth = (netbr_eth_t *)pline;
    for (i = 0; i < NETBRIDGE_MAC_CACHE_SIZE; i++) {
      if (netbr_eth->mcache[i].ttl >= (int)LW_HOTPLUG_SEC) {
        SYS_ARCH_PROTECT(lev);
        netbr_eth->mcache[i].ttl -= (int)LW_HOTPLUG_SEC;
        SYS_ARCH_UNPROTECT(lev);
      }
    }
  }
  NETBR_UNLOCK(netbr);
}

/* add a net device to net bridge 
   'brdev' bridge device name (not ifname) 
   'subdev' sub ethernet device name 
            sub_is_ifname == 0: dev name
            sub_is_ifname == 1: if name */
int  netbr_add_dev (const char *brdev, int brindex, const char *sub, int sub_is_ifname)
{
  int found, flags, need_up = 0;
  struct netif *netif;
  struct netif *netif_br;
  netbr_t *netbr;
  netbr_eth_t *netbr_eth;
  netdev_t *netdev_br;
  netdev_t *netdev;
  struct ifreq ifreq;
  
  if (!sub) {
    errno = EINVAL;
    return (-1);
  }
  
  LWIP_IF_LIST_LOCK(FALSE);
  found = 0;
  if (brdev && brdev[0]) {
    netdev_br = netdev_find_by_devname(brdev);
  } else {
    netdev_br = netdev_find_by_index(brindex);
  }
  if (netdev_br && (netdev_br->drv->transmit == netbr_transmit)) {
    netbr = (netbr_t *)netdev_br->priv;
    if (netbr && netbr->magic_no == NETBRIDGE_MAGIC) {
      netif_br = (struct netif *)netdev_br->sys;
      found = 1;
    }
  }
  if (!found) {
    LWIP_IF_LIST_UNLOCK();
    errno = ENXIO;
    return (-1);
  }
  
  found = 0;
  if (sub_is_ifname) {
    netdev = netdev_find_by_ifname(sub);
  } else {
    netdev = netdev_find_by_devname(sub);
  }
  if (netdev) {
    netif = (struct netif *)netdev->sys;
    if (!netif_is_mipif(netif)) { /* not a multi ip fake interface */
      found = 1;
    }
  }
  if (!found || netif->ext_ctl || (netdev->net_type != NETDEV_TYPE_ETHERNET)) {
    LWIP_IF_LIST_UNLOCK();
    errno = ENXIO;
    return (-1);
  }
  LWIP_IF_LIST_UNLOCK();
  
  netbr_eth = (netbr_eth_t *)mem_malloc(sizeof(netbr_eth_t));
  if (!netbr_eth) {
    errno = ENOMEM;
    return (-1);
  }
  lib_bzero(netbr_eth, sizeof(netbr_eth_t));
  
  netbr_eth->netdev = netdev; /* save net deivce */
  netbr_eth->netdev_br = netdev_br;
  
  if (!(netdev->init_flags & NETDEV_INIT_TIGHT)) {
    if (netdev_delete(netdev) < 0) { /* remove net device from protocol stack */
      mem_free(netbr_eth);
      return (-1);
    }
  }
  
  if (netif->flags & NETIF_FLAG_UP) {
    netif->flags &= ~NETIF_FLAG_UP;
    if (netif->down) {
      netif->down(netif); /* make down */
    }
  }
  
  if (!netbr->eth_list) {
    netifapi_netif_set_down(netif_br); /* make bridge down */
    need_up = 1;
  }

  if (memcmp(netdev_br->hwaddr, netbr_zeroaddr, ETH_ALEN) == 0) {
    MEMCPY(netdev_br->hwaddr, netdev->hwaddr, ETH_ALEN); /* use this port mac address */
    MEMCPY(netif_br->hwaddr, netdev->hwaddr, ETH_ALEN);
  }
  
  NETBR_LOCK(netbr);
  netdev_br->chksum_flags |= netdev->chksum_flags; /* we must use the most efficient checksum flags */
  netif_br->chksum_flags |= netdev->chksum_flags;
  _List_Line_Add_Ahead(&netbr_eth->list, &netbr->eth_list);
  NETBR_UNLOCK(netbr);
  
  netbr_eth->input = netif->input; /* save the old input function */
  netif->input  = netbr_input; /* set new input function */
  netif->ext_eth = (void *)netbr_eth;
  netif->ext_ctl = (void *)netbr;

  if (need_up) {
    netifapi_netif_set_up(netif_br); /* make bridge up */
  }
  
  if (!(netif->flags & NETIF_FLAG_UP)) {
    netif->flags |= NETIF_FLAG_UP;
    if (netif->up) {
      netif->up(netif); /* make up */
    }
  }
  
  flags = netif_get_flags(netif);
  if (!(flags & IFF_PROMISC)) {
    ifreq.ifr_name[0] = 0;
    ifreq.ifr_flags   = flags | IFF_PROMISC;
    if (netif->ioctl) {
      if (netif->ioctl(netif, SIOCSIFFLAGS, &ifreq)) { /* make IFF_PROMISC */
        _PrintHandle("sub ethernet device can not support IFF_PROMISC!\r\n");
      } else {
        netif->flags2 |= NETIF_FLAG2_PROMISC;
      }
    }
  }

  return (0);
}

/* delete a net device from net bridge 
   'brdev' bridge device name (not ifname) 
   'subdev' sub ethernet device name 
            sub_is_ifname == 0: dev name
            sub_is_ifname == 1: if name */
int  netbr_delete_dev (const char *brdev, int brindex, const char *sub, int sub_is_ifname)
{
  int found, flags;
  char subif[IFNAMSIZ];
  struct netif *netif;
  netbr_t *netbr;
  netbr_eth_t *netbr_eth;
  netdev_t *netdev_br;
  netdev_t *netdev;
  LW_LIST_LINE *pline;
  struct ifreq ifreq;
  
  if (!sub) {
    errno = EINVAL;
    return (-1);
  }
  
  LWIP_IF_LIST_LOCK(FALSE);
  found = 0;
  if (brdev && brdev[0]) {
    netdev_br = netdev_find_by_devname(brdev);
  } else {
    netdev_br = netdev_find_by_index(brindex);
  }
  if (netdev_br && (netdev_br->drv->transmit == netbr_transmit)) {
    netbr = (netbr_t *)netdev_br->priv;
    if (netbr && netbr->magic_no == NETBRIDGE_MAGIC) {
      found = 1;
    }
  }
  if (!found) {
    LWIP_IF_LIST_UNLOCK();
    errno = ENXIO;
    return (-1);
  }
  
  found = 0; /* search from sub ethernet device list */
  for (pline = netbr->eth_list; pline != NULL; pline = _list_line_get_next(pline)) {
    netbr_eth = (netbr_eth_t *)pline;
    netdev = netbr_eth->netdev;
    if (sub_is_ifname) {
      netdev_ifname(netdev, subif);
      if (!lib_strcmp(subif, sub)) {
        netif = (struct netif *)netdev->sys;
        found = 1;
        break;
      }
    
    } else {
      if (!lib_strcmp(netdev->dev_name, sub)) {
        netif = (struct netif *)netdev->sys;
        found = 1;
        break;
      }
    }
  }
  if (!found) {
    LWIP_IF_LIST_UNLOCK();
    errno = ENXIO;
    return (-1);
  }
  LWIP_IF_LIST_UNLOCK();
  
  if (netif->flags & NETIF_FLAG_UP) {
    netif->flags &= ~NETIF_FLAG_UP;
    if (netif->down) {
      netif->down(netif); /* make down */
    }
  }
  
  flags = netif_get_flags(netif);
  if (flags & IFF_PROMISC) {
    ifreq.ifr_name[0] = 0;
    ifreq.ifr_flags   = flags & ~IFF_PROMISC;
    if (netif->ioctl) {
      if (netif->ioctl(netif, SIOCSIFFLAGS, &ifreq)) { /* make Non IFF_PROMISC */
        _PrintHandle("sub ethernet device can not support IFF_PROMISC!\r\n");
      } else {
        netif->flags2 &= ~NETIF_FLAG2_PROMISC;
      }
    }
  }
  
  netif->input = netbr_eth->input; /* restore old input function */
  netif->ext_ctl = netif->ext_eth = NULL;

  NETBR_LOCK(netbr);
  _List_Line_Del(&netbr_eth->list, &netbr->eth_list);
  NETBR_UNLOCK(netbr);
  
  mem_free(netbr_eth);
  
  if (!(netdev->init_flags & NETDEV_INIT_TIGHT)) {
    netdev->init_flags |= NETDEV_INIT_DO_NOT;
    if (netdev_add(netdev, NULL, NULL, NULL,
                   IFF_UP | IFF_RUNNING | IFF_BROADCAST | IFF_MULTICAST) < 0) { /* add to system */
      netdev->init_flags &= ~NETDEV_INIT_DO_NOT;
      return (-1);
    }
    netdev->init_flags &= ~NETDEV_INIT_DO_NOT;

  } else {
    if (!(netif->flags & NETIF_FLAG_UP)) {
      netif->flags |= NETIF_FLAG_UP;
      if (netif->up) {
        netif->up(netif); /* make up */
      }
    }
  }
  
  return (0);
}

/* add net bridge 
   'brdev' bridge device name (not ifname) */
int  netbr_add (const char *brdev, const char *ip, 
                const char *netmask, const char *gw, int *index)
{
  static struct netdev_funcs  netbr_drv;
  struct netif *netif_br;
  netbr_t *netbr;
  netdev_t *netdev_br;
  
  if (!brdev || (lib_strnlen(brdev, IF_NAMESIZE) >= IF_NAMESIZE)) {
    errno = EINVAL;
    return (-1);
  }
  
  netbr = (netbr_t *)mem_malloc(sizeof(netbr_t));
  if (!netbr) {
    errno = ENOMEM;
    return (-1);
  }
  lib_bzero(netbr, sizeof(netbr_t));
  
  netbr->magic_no = NETBRIDGE_MAGIC;
  netdev_br = &netbr->netdev_br;
  netif_br = (struct netif *)netdev_br->sys;
  
  netdev_br->magic_no = NETDEV_MAGIC;
  lib_strlcpy(netdev_br->dev_name, brdev, IF_NAMESIZE);
  lib_strlcpy(netdev_br->if_name, "br", IF_NAMESIZE);
  
  netdev_br->if_hostname = "SylixOS Bridge";
  netdev_br->init_flags = NETDEV_INIT_LOAD_PARAM
                        | NETDEV_INIT_LOAD_DNS
                        | NETDEV_INIT_IPV6_AUTOCFG;
  netdev_br->chksum_flags = NETDEV_CHKSUM_DISABLE_ALL;
  netdev_br->net_type = NETDEV_TYPE_ETHERNET; /* must ethernet device */
  netdev_br->speed = 0;
  netdev_br->mtu = ETH_DATA_LEN;
  netdev_br->hwaddr_len = ETH_ALEN;
  
  netbr_drv.rxmode = netbr_rxmode;
  netbr_drv.transmit = netbr_transmit;
  netbr_drv.receive = netbr_receive;
  
  netdev_br->drv = &netbr_drv;
  netdev_br->priv = (void *)netbr;

  if (sys_mutex_new(&netbr->lock) != ERR_OK) {
    mem_free(netbr);
    return (-1);
  }

  if (netdev_add(netdev_br, ip, netmask, gw, /* add to system */
                 IFF_RUNNING | IFF_BROADCAST | IFF_MULTICAST) < 0) {
    sys_mutex_free(&netbr->lock);
    mem_free(netbr);
    return (-1);
  }
  
  netif_br->priv_flags = IFF_EBRIDGE; /* bridge device */
  
  netdev_linkup_wd_add(netdev_br, netbr_wd); /* add watchdog function */

  if (index) {
    netdev_index(netdev_br, (unsigned int *)index);
  }

  return (0);
}

/* delete net bridge 
   'brdev' bridge device name (not ifname) */
int  netbr_delete (const char *brdev, int brindex)
{
  int found, flags;
  struct netif *netif;
  netbr_t *netbr;
  netbr_eth_t *netbr_eth;
  netdev_t *netdev_br;
  netdev_t *netdev;
  struct ifreq ifreq;
  
  LWIP_IF_LIST_LOCK(FALSE);
  found = 0;
  if (brdev && brdev[0]) {
    netdev_br = netdev_find_by_devname(brdev);
  } else {
    netdev_br = netdev_find_by_index(brindex);
  }
  if (netdev_br && (netdev_br->drv->transmit == netbr_transmit)) {
    netbr = (netbr_t *)netdev_br->priv;
    if (netbr && netbr->magic_no == NETBRIDGE_MAGIC) {
      netbr->magic_no = 0;
      found = 1;
    }
  }
  if (!found) {
    LWIP_IF_LIST_UNLOCK();
    errno = ENXIO;
    return (-1);
  }
  LWIP_IF_LIST_UNLOCK();
  
  netdev_delete(netdev_br);
  
  netdev_linkup_wd_delete(netdev_br, netbr_wd); /* delete watchdog function */
  
  NETBR_LOCK(netbr);
  while (netbr->eth_list) {
    netbr_eth = (netbr_eth_t *)netbr->eth_list;
    netdev = netbr_eth->netdev;
    netif = (struct netif *)netdev->sys;
    _List_Line_Del(&netbr_eth->list, &netbr->eth_list);
    NETBR_UNLOCK(netbr);
    
    if (netif->flags & NETIF_FLAG_UP) {
      netif->flags &= ~NETIF_FLAG_UP;
      if (netif->down) {
        netif->down(netif);
      }
    }
    
    flags = netif_get_flags(netif);
    if (flags & IFF_PROMISC) {
      ifreq.ifr_name[0] = 0;
      ifreq.ifr_flags   = flags & ~IFF_PROMISC;
      if (netif->ioctl) {
        if (netif->ioctl(netif, SIOCSIFFLAGS, &ifreq)) {
          _PrintHandle("sub ethernet device can not support IFF_PROMISC!\r\n");
        } else {
          netif->flags2 &= ~NETIF_FLAG2_PROMISC;
        }
      }
    }
    
    netif->input = netbr_eth->input; /* restore input function */
    netif->ext_ctl = netif->ext_eth = NULL;
  
    mem_free(netbr_eth);
  
    if (!(netdev->init_flags & NETDEV_INIT_TIGHT)) {
      netdev->init_flags |= NETDEV_INIT_DO_NOT;
      if (netdev_add(netdev, NULL, NULL, NULL,
                     IFF_UP | IFF_RUNNING | IFF_BROADCAST | IFF_MULTICAST) < 0) {
        _PrintHandle("sub ethernet device can not mount to system!\r\n");
      }
      netdev->init_flags &= ~NETDEV_INIT_DO_NOT;

    } else {
      if (!(netif->flags & NETIF_FLAG_UP)) {
        netif->flags |= NETIF_FLAG_UP;
        if (netif->up) {
          netif->up(netif); /* make up */
        }
      }
    }
               
    NETBR_LOCK(netbr);
  }
  NETBR_UNLOCK(netbr);
  
  sys_mutex_free(&netbr->lock);
  mem_free(netbr);
  
  return (0);
}

/* net bridge mac cache flush 
   'brdev' bridge device name (not ifname) */
int  netbr_flush_cache (const char *brdev, int brindex)
{
  int found;
  netbr_t *netbr;
  netbr_eth_t *netbr_eth;
  netdev_t *netdev_br;
  LW_LIST_LINE *pline;
  int i;
  
  LWIP_IF_LIST_LOCK(FALSE);
  found = 0;
  if (brdev && brdev[0]) {
    netdev_br = netdev_find_by_devname(brdev);
  } else {
    netdev_br = netdev_find_by_index(brindex);
  }
  if (netdev_br && (netdev_br->drv->transmit == netbr_transmit)) {
    netbr = (netbr_t *)netdev_br->priv;
    if (netbr && netbr->magic_no == NETBRIDGE_MAGIC) {
      found = 1;
    }
  }
  if (!found) {
    LWIP_IF_LIST_UNLOCK();
    errno = ENXIO;
    return (-1);
  }
  LWIP_IF_LIST_UNLOCK();
  
  NETBR_LOCK(netbr);
  for (pline = netbr->eth_list; pline != NULL; pline = _list_line_get_next(pline)) {
    netbr_eth = (netbr_eth_t *)pline;
    for (i = 0; i < NETBRIDGE_MAC_CACHE_SIZE; i++) {
      netbr_eth->mcache[i].ttl = 0;
    }
  }
  NETBR_UNLOCK(netbr);
  
  return (0);
}

/* net bridge sub device delete hook
   NOTICE: this is in LWIP_IF_LIST_LOCK status */
void  netbr_sub_delete_hook (netdev_t *netdev)
{
  int flags;
  struct netif *netif;
  netbr_t *netbr;
  netbr_eth_t *netbr_eth;
  struct ifreq ifreq;
  LW_LIST_LINE *pline;

  netif = (struct netif *)netdev->sys;
  netbr = (netbr_t *)netif->ext_ctl;
  if (!netbr || netbr->magic_no != NETBRIDGE_MAGIC) {
    return;
  }
  for (pline = netbr->eth_list; pline != NULL; pline = _list_line_get_next(pline)) {
    netbr_eth = (netbr_eth_t *)pline;
    if (netbr_eth->netdev == netdev) {
      break;
    }
  }
  if (!pline) {
    return;
  }
  
  if (netif->flags & NETIF_FLAG_UP) {
    netif->flags &= ~NETIF_FLAG_UP;
    if (netif->down) {
      netif->down(netif); /* make down */
    }
  }
  
  flags = netif_get_flags(netif);
  if (flags & IFF_PROMISC) {
    ifreq.ifr_name[0] = 0;
    ifreq.ifr_flags   = flags & ~IFF_PROMISC;
    if (netif->ioctl) {
      if (netif->ioctl(netif, SIOCSIFFLAGS, &ifreq)) { /* make Non IFF_PROMISC */
        _PrintHandle("sub ethernet device can not support IFF_PROMISC!\r\n");
      } else {
        netif->flags2 &= ~NETIF_FLAG2_PROMISC;
      }
    }
  }
  
  NETBR_LOCK(netbr);
  _List_Line_Del(&netbr_eth->list, &netbr->eth_list);
  NETBR_UNLOCK(netbr);
  
  netif->input = netbr_eth->input; /* restore old input function */
  netif->ext_ctl = netif->ext_eth = NULL;
  
  mem_free(netbr_eth);
}

/* net bridge show all device in bridge */
int  netbr_show_dev (const char *brdev, int brindex, int fd)
{
  int found;
  int i = 0, j;
  struct netif *netif;
  netbr_t *netbr;
  netbr_eth_t *netbr_eth;
  netdev_t *netdev_br;
  netdev_t *netdev;
  char speed[32];
  char ifname[NETIF_NAMESIZE];
  LW_LIST_LINE *pline;
  
  if (fd < 0) {
    errno = EINVAL;
    return (-1);
  }
  
  LWIP_IF_LIST_LOCK(FALSE);
  found = 0;
  if (brdev && brdev[0]) {
    netdev_br = netdev_find_by_devname(brdev);
  } else {
    netdev_br = netdev_find_by_index(brindex);
  }
  if (netdev_br && (netdev_br->drv->transmit == netbr_transmit)) {
    netbr = (netbr_t *)netdev_br->priv;
    if (netbr && netbr->magic_no == NETBRIDGE_MAGIC) {
      found = 1;
    }
  }
  if (!found) {
    LWIP_IF_LIST_UNLOCK();
    errno = ENXIO;
    return (-1);
  }
  
  for (pline = netbr->eth_list; pline != NULL; pline = _list_line_get_next(pline)) {
    netbr_eth = (netbr_eth_t *)pline;
    netdev = netbr_eth->netdev;
    netif = (struct netif *)netdev->sys;
    if (netdev->speed == 0) {
      lib_strlcpy(speed, "N/A", sizeof(speed));
    } else if (netdev->speed < 1000ull) {
      snprintf(speed, sizeof(speed), "%qu bps", netdev->speed);
    } else if (netdev->speed < 5000000ull) {
      snprintf(speed, sizeof(speed), "%qu Kbps", netdev->speed / 1000);
    } else if (netdev->speed < 5000000000ull) {
      snprintf(speed, sizeof(speed), "%qu Mbps", netdev->speed / 1000000);
    } else {
      snprintf(speed, sizeof(speed), "%qu Gbps", netdev->speed / 1000000000);
    }
    fdprintf(fd, "<%d> Dev: %s Prev-Ifname: %s Spd: %s Linkup: %s Prev-HWaddr: ", i,
             netdev->dev_name, netif_get_name(netif, ifname),
             speed, netdev->if_flags & IFF_RUNNING ? "Enable" : "Disable");
    for (j = 0; j < netif->hwaddr_len - 1; j++) {
      fdprintf(fd, "%02x:", netif->hwaddr[j]);
    }
    fdprintf(fd, "%02x\n", netif->hwaddr[netif->hwaddr_len - 1]);
    i++;
  }
  LWIP_IF_LIST_UNLOCK();
  
  return (0);
}

#endif /* LW_CFG_NET_DEV_BRIDGE_EN */
/*
 * end
 */
