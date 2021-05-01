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

#define __SYLIXOS_KERNEL
#include "stdio.h"
#include "string.h"
#include "netbonding.h"

#if LW_CFG_NET_DEV_BONDING_EN > 0

#include "lwip/mem.h"
#include "lwip/netif.h"
#include "lwip/netifapi.h"
#include "net/if_bonding.h"
#include "net/if_lock.h"
#include "net/if_flags.h"
#include "net/if_ether.h"
#include "netif/etharp.h"

#ifndef NETBONDING_POLL_INTERVAL
#define NETBONDING_POLL_INTERVAL 10 /* Default 10 milliseconds poll period */
#endif /* NETBONDING_POLL_INTERVAL */

/* arp packet send */
extern err_t etharp_raw(struct netif *netif, const struct eth_addr *ethsrc_addr,
                        const struct eth_addr *ethdst_addr,
                        const struct eth_addr *hwsrc_addr, const ip4_addr_t *ipsrc_addr,
                        const struct eth_addr *hwdst_addr, const ip4_addr_t *ipdst_addr,
                        const u16_t opcode);
                  
/* broadcast and zero mac address */
extern const struct eth_addr ethbroadcast;
extern const struct eth_addr ethzero;

/* net bonding arp target list */
typedef struct netbd_arp {
  LW_LIST_LINE list; /* arp monitiong list */
  ip4_addr_t ipaddr; /* target ip address */
} netbd_arp_t;

/* net bonding sub device */
typedef struct netbd_eth {
  LW_LIST_RING ring; /* sub ethernet device ring */
  int alive; /* time to alive (milliseconds) */
  
  u16_t old_vlanid; /* old_vlanid */
  u8_t old_hwaddr[ETH_ALEN]; /* old hwaddr */
  u8_t old_hwaddr_valid; /* old hwaddr is valid */
  
  netdev_t *netdev; /* sub ethernet device */
  netdev_t *netdev_bd; /* bonding net device */
  netif_input_fn input; /* old input function */
} netbd_eth_t;

/* net bonding device */
typedef struct netbd {
#define NETBONDING_MAGIC   0xf7e34a83
  UINT32 magic_no;  /* MUST be NETBONDING_MAGIC */
  LW_LIST_LINE list; /* all net bonding list */
  
  UINT32 mode; /* 'balance-rr' or 'active-backup' mode */
  UINT32 mon_mode; /* 'traffic' or 'arp' */
  UINT32 eth_cnt; /* sub ethernet device */
  
  int timer; /* arp detect timer */
  int interval; /* arp detect interval (milliseconds) */
  int alive; /* time to alive setting (milliseconds) */
  
  sys_mutex_t lock; /* net bonding lock */
  netdev_t netdev_bd;  /* net bonding net device */
  netbd_eth_t *master; /* master device */
  netbd_eth_t *working; /* current device */
  int working_change; /* current device change */

  LW_LIST_RING_HEADER eth_ring; /* sub ethernet device ring */
  LW_LIST_LINE_HEADER arp_list; /* arp detect list */
} netbd_t;

/* net bonding device lock */
#define NETBD_LOCK(netbd)   sys_mutex_lock(&((netbd)->lock))
#define NETBD_UNLOCK(netbd) sys_mutex_unlock(&((netbd)->lock))

/* net bonding list */
static LW_LIST_LINE_HEADER netdb_list;

/* net bonding list lock */
static sys_mutex_t netdb_list_lock;

#define NETBD_LIST_LOCK()   sys_mutex_lock(&netdb_list_lock)
#define NETBD_LIST_UNLOCK() sys_mutex_unlock(&netdb_list_lock)

/* ethernet zero address */
static UINT8 netbd_zeroaddr[ETH_ALEN] = { 0, 0, 0, 0, 0, 0 };

/* net bonding link check */
static void netbd_link_check (netbd_t *netbd)
{
  int i;
  int netbd_link_ok = 0;
  int link_status;
  netbd_eth_t *netbd_eth;
  netdev_t *netdev_bd = &netbd->netdev_bd;

  for (i = 0; i < netbd->eth_cnt; i++) {
    netbd_eth = (netbd_eth_t *)netbd->eth_ring;
    if (netbd_eth->netdev->if_flags & IFF_RUNNING) { /* is linkup ? */
      netbd_link_ok = 1;
      break;
    }
    netbd->eth_ring = _list_ring_get_next(&netbd_eth->ring); /* next sub net device (Rotation) */
  }

  netdev_get_linkup(netdev_bd, &link_status);
  if (netbd_link_ok) {
    if (!link_status) {
      netdev_set_linkup(netdev_bd, 1, 0);
    }
  } else {
    if (link_status) {
      netdev_set_linkup(netdev_bd, 0, 0);
    }
  }
}

/* net bonding select device */
static void netbd_select_working (netbd_t *netbd)
{
  int i, ok = 0;
  netbd_eth_t *netbd_eth;

  if (netbd->master) {
    netbd_eth = netbd->master; /* first check master device */
    if (netbd_eth->alive && (netbd_eth->netdev->if_flags & IFF_RUNNING)) { /* is alive and linkup ? */
      netbd->working = netbd_eth;
      ok = 1;
    }
  }

  if (!ok) {
    if (!netbd->working->alive || !(netbd->working->netdev->if_flags & IFF_RUNNING)) {
      for (i = 0; i < netbd->eth_cnt; i++) {
        netbd_eth = (netbd_eth_t *)netbd->eth_ring;
        if (netbd->master != netbd_eth) { /* not master */
          if (netbd_eth->alive && (netbd_eth->netdev->if_flags & IFF_RUNNING)) { /* is alive and linkup ? */
            netbd->working = netbd_eth;
            break;
          }
        }
        netbd->eth_ring = _list_ring_get_next(&netbd_eth->ring); /* next sub net device (Rotation) */
      }
    }
  }
}

/* net bonding traffic detect */
static void netbd_traffic_detect (netbd_t *netbd)
{
  int i;
  netbd_eth_t *netbd_eth;
  struct netif *netdbif;
  struct netif *sendif;
  struct eth_addr macsrc;
  ip4_addr_t ipaddr;
  netbd_eth_t *curent_working = netbd->working;
  
  NETBD_LOCK(netbd);
  netbd_eth = (netbd_eth_t *)netbd->eth_ring;
  for (i = 0; i < netbd->eth_cnt; i++) {
    if ((netbd_eth->netdev->if_flags & IFF_RUNNING) && (netbd_eth->alive > NETBONDING_POLL_INTERVAL)) {
      netbd_eth->alive -= NETBONDING_POLL_INTERVAL;

    } else {
      if (netbd_eth->alive <= NETBONDING_POLL_INTERVAL) {
        netbd_eth->alive = 0; /* dead! not use this device transmit */
      }
    }
    netbd_eth = (netbd_eth_t *)_list_ring_get_next(&netbd_eth->ring);
  }

  if (!netbd->eth_cnt) { /* no sub device */
    NETBD_UNLOCK(netbd);
    return;
  }
  netbd_select_working(netbd);

  if (curent_working != netbd->working) { /* woking change */
    netbd->working_change = 1;
  }

  /*
   * When a working link change is detected, a broadcast arp is actively sent to
   * let the corresponding port learn the MAC address of the netbd.
   */
  if (netbd->working_change) {
    ipaddr.addr = 0x5A01A8C0; /* any ip */
    netdbif = (struct netif *)(netbd->netdev_bd.sys);
    MEMCPY(&macsrc.addr, netdbif->hwaddr, ETH_ALEN);
    sendif = (struct netif *)(netbd->working->netdev->sys);
    etharp_raw(sendif, &macsrc, &ethbroadcast, &macsrc, netif_ip4_addr(netdbif),
               &ethzero, &ipaddr, ARP_REQUEST); /* send ARP request */
    netbd->working_change = 0;
  }
  NETBD_UNLOCK(netbd);
}

/* net bonding arp detect */
static void netbd_arp_detect (netbd_t *netbd)
{
  int i;
  netbd_eth_t *netbd_eth;
  netbd_arp_t *netbd_arp;
  struct netif *netdbif;
  struct netif *sendif;
  struct eth_addr macsrc;
  LW_LIST_LINE *pline;
  
  if (!netbd->arp_list) { /* no arp target */
    return;
  }
  
  NETBD_LOCK(netbd);
  netbd_eth = (netbd_eth_t *)netbd->eth_ring;
  for (i = 0; i < netbd->eth_cnt; i++) {
    if (netbd->working == netbd_eth) {
      if (netbd_eth->alive > NETBONDING_POLL_INTERVAL) {
        netbd_eth->alive -= NETBONDING_POLL_INTERVAL;
      } else {
        netbd_eth->alive = 0; /* dead! not use this device transmit */
      }

    } else {
      if (netbd_eth->netdev->if_flags & IFF_RUNNING) {
        netbd_eth->alive = netbd->alive; /* not current, if linkup we think it is a valid */
      } else {
        netbd_eth->alive = 0;
      }
    }
    netbd_eth = (netbd_eth_t *)_list_ring_get_next(&netbd_eth->ring);
  }
  NETBD_UNLOCK(netbd);

  if (netbd->timer > NETBONDING_POLL_INTERVAL) {
    netbd->timer -= NETBONDING_POLL_INTERVAL;
    
  } else {
    netbd->timer = netbd->interval;
    if (!netbd->eth_cnt) { /* no sub device */
      return;
    }
    
    NETBD_LOCK(netbd);
    netdbif = (struct netif *)(netbd->netdev_bd.sys);
    MEMCPY(&macsrc.addr, netdbif->hwaddr, ETH_ALEN);

    netbd_select_working(netbd);
  
    if (netbd->working->netdev->if_flags & IFF_RUNNING) {
      for (pline = netbd->arp_list; pline != NULL; pline = _list_line_get_next(pline)) {
        netbd_arp = (netbd_arp_t *)pline;
        sendif = (struct netif *)(netbd->working->netdev->sys);
        etharp_raw(sendif, &macsrc, &ethbroadcast, &macsrc, netif_ip4_addr(netdbif),
                   &ethzero, &netbd_arp->ipaddr, ARP_REQUEST); /* send ARP request */
      }
    }
    NETBD_UNLOCK(netbd);
  }
}

/* net bonding arp process */
static void netbd_arp_process (netbd_t *netbd, netbd_eth_t *netbd_eth, struct etharp_hdr *hdr)
{
  netbd_arp_t *netbd_arp;
  struct netif *netdbif;
  ip4_addr_t srcip, dstip;
  LW_LIST_LINE *pline;
  
  if (hdr->opcode != PP_HTONS(ARP_REPLY)) {
    return; /* we only need reply packet */
  }
  
  IPADDR_WORDALIGNED_COPY_TO_IP4_ADDR_T(&srcip, &hdr->sipaddr);
  IPADDR_WORDALIGNED_COPY_TO_IP4_ADDR_T(&dstip, &hdr->dipaddr);

  NETBD_LOCK(netbd);
  netdbif = (struct netif *)(netbd->netdev_bd.sys);
  if (!ip4_addr_cmp(netif_ip4_addr(netdbif), &dstip)) {
    NETBD_UNLOCK(netbd);
    return; /* not for me */
  }

  for (pline = netbd->arp_list; pline != NULL; pline = _list_line_get_next(pline)) {
    netbd_arp = (netbd_arp_t *)pline;
    if (ip4_addr_cmp(&netbd_arp->ipaddr, &srcip)) {
      if (!netbd->working->alive) {
        netbd->working = netbd_eth; /* use this net device as working */
      }
      netbd_eth->alive = netbd->alive; /* refresh time alive counter */
      break;
    }
  }
  NETBD_UNLOCK(netbd);
}

/* net bonding transmit
   'netdev' is net bonding device */
static int  netbd_transmit (struct netdev *netdev, struct pbuf *p)
{
  int i, ok = 0;
  netbd_t *netbd = (netbd_t *)netdev->priv;
  netbd_eth_t *netbd_eth;
  struct ethhdr *eh = (struct ethhdr *)p->payload;
  netbd_eth_t *curent_working = netbd->working;
  
  if (!netbd->eth_cnt) { /* no sub device */
    netdev_linkinfo_err_inc(netdev);
    netdev_statinfo_errors_inc(netdev, LINK_OUTPUT);
    return (-1);
  }

  NETBD_LOCK(netbd);
  if (netbd->mode == NETBD_MODE_BALANCE_RR) { /* 'balance-rr' */
    for (i = 0; i < netbd->eth_cnt; i++) {
      netbd_eth = (netbd_eth_t *)netbd->eth_ring;
      netbd->eth_ring = _list_ring_get_next(&netbd_eth->ring); /* next sub net device (Rotation) */
      if (netbd_eth->netdev->if_flags & IFF_RUNNING) { /* is linkup ? */
        netbd_eth->netdev->drv->transmit(netbd_eth->netdev, p); /* transmit */
        ok = 1;
        break;
      }
    }
    
  } else if (netbd->mode == NETBD_MODE_ACTIVE_BACKUP) { /* 'active-backup' */
    if (netbd->master) {
      netbd_eth = netbd->master; /* first check master device */
      if (netbd_eth->alive && (netbd_eth->netdev->if_flags & IFF_RUNNING)) { /* is alive and linkup ? */
        netbd->working = netbd_eth;
        netbd_eth->netdev->drv->transmit(netbd_eth->netdev, p); /* transmit */
        ok = 1;
      }
    }

    if (!ok) {
      if (!netbd->working->alive || !(netbd->working->netdev->if_flags & IFF_RUNNING)) { /* working dead ? */
        for (i = 0; i < netbd->eth_cnt; i++) {
          netbd_eth = (netbd_eth_t *)netbd->eth_ring;
          if (netbd->master != netbd_eth) { /* not master */
            if (netbd_eth->alive && (netbd_eth->netdev->if_flags & IFF_RUNNING)) { /* is alive and linkup ? */
              netbd->working = netbd_eth;
              netbd_eth->netdev->drv->transmit(netbd_eth->netdev, p); /* transmit */
              ok = 1;
              break;
            }
          }
          netbd->eth_ring = _list_ring_get_next(&netbd_eth->ring); /* next sub net device (Rotation) */
        }
      } else {
        netbd->working->netdev->drv->transmit(netbd->working->netdev, p); /* transmit */
        ok = 1;
      }
    }
    
  } else { /* broadcast */
    netbd_eth = (netbd_eth_t *)netbd->eth_ring;
    for (i = 0; i < netbd->eth_cnt; i++) {
      if (netbd_eth->netdev->if_flags & IFF_RUNNING) { /* is alive and linkup ? */
        netbd_eth->netdev->drv->transmit(netbd_eth->netdev, p); /* transmit */
        ok = 1;
      }
      netbd_eth = (netbd_eth_t *)_list_ring_get_next(&netbd_eth->ring);
    }
  }

  if (curent_working != netbd->working) { /* woking change */
    netbd->working_change = 1;
  }

  NETBD_UNLOCK(netbd);
  
  if (!ok) {
    netdev_linkinfo_err_inc(netdev);
    netdev_statinfo_errors_inc(netdev, LINK_OUTPUT);
    return (-1);
  }
  
  netdev_linkinfo_xmit_inc(netdev);
  netdev_statinfo_total_add(netdev, LINK_OUTPUT, p->tot_len);
  if (eh->h_dest[0] & 1) {
    netdev_statinfo_mcasts_inc(netdev, LINK_OUTPUT);
  } else {
    netdev_statinfo_ucasts_inc(netdev, LINK_OUTPUT);
  }
  
  return (0);
}

/* net bonding receive 
   SylixOS net bonding do not use this function */
static void  netbd_receive (struct netdev *netdev, int (*input)(struct netdev *, struct pbuf *))
{
  _DebugHandle(__ERRORMESSAGE_LEVEL, "netbd_receive() called!\r\n");
}

/* net bonding rxmode */
static int netbd_rxmode (struct netdev *netdev, int flags)
{
  int i;
  netbd_t *netbd = (netbd_t *)netdev->priv;
  netbd_eth_t *netbd_eth;
  
  NETBD_LOCK(netbd);
  netbd_eth = (netbd_eth_t *)netbd->eth_ring;
  for (i = 0; i < netbd->eth_cnt; i++) {
    if (netbd_eth->netdev->drv->rxmode) {
      netbd_eth->netdev->drv->rxmode(netbd_eth->netdev, flags);
    }
    netbd_eth = (netbd_eth_t *)_list_ring_get_next(&netbd_eth->ring);
  }
  NETBD_UNLOCK(netbd);
  
  return (0);
}

/* net bonding input 
   'netif' is sub erhernet device */
static err_t  netbd_input (struct pbuf *p, struct netif *netif)
{
  netdev_t *netdev = (netdev_t *)(netif->state); /* sub ethernet device */
  netbd_eth_t *netbd_eth = (netbd_eth_t *)netif->ext_eth;
  netdev_t *netdev_bd = netbd_eth->netdev_bd; /* bonding device */
  netbd_t *netbd = (netbd_t *)netdev_bd->priv;
  struct netif *netif_bd = (struct netif *)netdev_bd->sys;
  struct eth_hdr *eh = (struct eth_hdr *)p->payload;
  int mcast = eh->dest.addr[0] & 1;
  netbd_eth_t *curent_working = netbd->working;
  
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
      goto to_bd;

    default:
      goto to_sub;
    }

to_sub:
    if (netbd_eth->input(p, netif)) {
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

to_bd:
  if (!(netif_bd->flags & NETIF_FLAG_UP)) {
    return (ERR_IF); /* bonding was down */
  }
  
  if (!lib_memcmp(&eh->src.addr, netif_bd->hwaddr, ETH_ALEN)) {
    pbuf_free(p); /* Packet loop */
    return (ERR_OK);
  }
  
  if (netbd->mode == NETBD_MODE_ACTIVE_BACKUP) { /* active backup */
    if (netbd->mon_mode == NETBD_MON_MODE_TRAFFIC) {
      if (!(netbd->working->netdev->if_flags & IFF_RUNNING) || !(netbd->working->alive)) {
        netbd->working = netbd_eth; /* use this net device as working */
      }
      netbd_eth->alive = netbd->alive; /* refresh time alive counter */
  
    } else { /* NETBD_MON_MODE_ARP */
      u16_t type = eh->type;
      u16_t next_hdr_offset = SIZEOF_ETH_HDR;
      struct etharp_hdr *hdr;
      
      if (type == PP_HTONS(ETHTYPE_VLAN)) {
        struct eth_vlan_hdr *vlan = (struct eth_vlan_hdr *)(((char *)eh) + SIZEOF_ETH_HDR);
        next_hdr_offset = SIZEOF_ETH_HDR + SIZEOF_VLAN_HDR;
        if (p->len <= SIZEOF_ETH_HDR + SIZEOF_VLAN_HDR) {
          goto input;
        }
        type = vlan->tpid;
      }
    
      if (type == PP_HTONS(ETHTYPE_ARP)) {
        if (p->len <= SIZEOF_ETH_HDR + SIZEOF_VLAN_HDR + SIZEOF_ETHARP_HDR) {
          goto input;
        }
        hdr = (struct etharp_hdr *)((u8_t *)(p->payload) + next_hdr_offset);
        netbd_arp_process(netbd, netbd_eth, hdr); /* process arp packet */
      }
    }
  }
  if (curent_working != netbd->working) {   /* woking change */
    netbd->working_change = 1;
  }
  
input: /* TODO: this function may be parallelization, and statistical variables should be locked */
  if (netbd->mode == NETBD_MODE_ACTIVE_BACKUP) {
    if (netbd->working != netbd_eth) {
      pbuf_free(p);
      return (ERR_OK); /* not working net device */
    }
  }

  if (netif_bd->input(p, netif_bd)) { /* send to our tcpip stack */
    netdev_linkinfo_drop_inc(netdev_bd);
    netdev_statinfo_discards_inc(netdev_bd, LINK_INPUT);
    return (ERR_IF);
    
  } else {
    netdev_linkinfo_recv_inc(netdev_bd);
    netdev_statinfo_total_add(netdev_bd, LINK_INPUT, (p->tot_len - ETH_PAD_SIZE));
    if (mcast) {
      netdev_statinfo_mcasts_inc(netdev_bd, LINK_INPUT);
    } else {
      netdev_statinfo_ucasts_inc(netdev_bd, LINK_INPUT);
    }
    return (ERR_OK);
  }
}

/* net bonding thread */
static void  netbd_proc (void *arg)
{
  netbd_t *netbd;
  LW_LIST_LINE *pline;
  
  for (;;) {
    sys_msleep(NETBONDING_POLL_INTERVAL);
  
    NETBD_LIST_LOCK();
    for (pline = netdb_list; pline != NULL; pline = _list_line_get_next(pline)) {
      netbd = _LIST_ENTRY(pline, netbd_t, list);
      if (netbd->mode == NETBD_MODE_ACTIVE_BACKUP) {
        if (netbd->mon_mode == NETBD_MON_MODE_TRAFFIC) {
          netbd_traffic_detect(netbd);
        } else {
          netbd_arp_detect(netbd);
        }
      }
      netbd_link_check(netbd);
    }
    NETBD_LIST_UNLOCK();
  }
}

/* add a net device to net bonding 
   'bddev' bonding device name (not ifname) 
   'subdev' sub ethernet device name 
            sub_is_ifname == 0: dev name
            sub_is_ifname == 1: if name */
int  netbd_add_dev (const char *bddev, int bdindex, const char *sub, int sub_is_ifname)
{
  int found, flags, need_up = 0;
  struct netif *netif;
  struct netif *netif_bd;
  netbd_t *netbd;
  netbd_eth_t *netbd_eth;
  netdev_t *netdev_bd;
  netdev_t *netdev;
  struct ifreq ifreq;

  if (!sub) {
    errno = EINVAL;
    return (-1);
  }
  
  LWIP_IF_LIST_LOCK(FALSE);
  found = 0;
  if (bddev && bddev[0]) {
    netdev_bd = netdev_find_by_devname(bddev);
  } else {
    netdev_bd = netdev_find_by_index(bdindex);
  }
  if (netdev_bd && (netdev_bd->drv->transmit == netbd_transmit)) {
    netbd = (netbd_t *)netdev_bd->priv;
    if (netbd && netbd->magic_no == NETBONDING_MAGIC) {
      netif_bd = (struct netif *)netdev_bd->sys;
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
  
  netbd_eth = (netbd_eth_t *)mem_malloc(sizeof(netbd_eth_t));
  if (!netbd_eth) {
    errno = ENOMEM;
    return (-1);
  }
  lib_bzero(netbd_eth, sizeof(netbd_eth_t));
  
  netbd_eth->netdev = netdev; /* save net deivce */
  netbd_eth->netdev_bd = netdev_bd;
  
  if (!(netdev->init_flags & NETDEV_INIT_TIGHT)) {
    if (netdev_delete(netdev) < 0) { /* remove net device from protocol stack */
      mem_free(netbd_eth);
      return (-1);
    }
  }
  
  if (netif->flags & NETIF_FLAG_UP) {
    netif->flags &= ~NETIF_FLAG_UP;
    if (netif->down) {
      netif->down(netif); /* make down */
    }
  }
  
  if (!netbd->eth_ring) {
    netifapi_netif_set_down(netif_bd); /* make bonding down */
    need_up = 1;
    netbd->working = netbd_eth; /* first working net device */
  }

  if (memcmp(netdev_bd->hwaddr, netbd_zeroaddr, ETH_ALEN) == 0) {
    MEMCPY(netdev_bd->hwaddr, netdev->hwaddr, ETH_ALEN); /* use this port mac address */
    MEMCPY(netif_bd->hwaddr, netdev->hwaddr, ETH_ALEN);
  }
  
  MEMCPY(netbd_eth->old_hwaddr, netdev->hwaddr, ETH_ALEN); /* save old hwaddr */
  netbd_eth->alive = netbd->alive;

  NETBD_LOCK(netbd);
  netbd_eth->old_vlanid = netif->vlanid; /* use net bonding vlan id */
  netif->vlanid = netif_bd->vlanid;
  
  if (!need_up) { /* not first sub device */
    if (netif->ioctl) {
      ifreq.ifr_name[0] = 0;
      MEMCPY(ifreq.ifr_hwaddr.sa_data, netdev_bd->hwaddr, ETH_ALEN);
      if (netif->ioctl(netif, SIOCSIFHWADDR, &ifreq) == 0) { /* try set mac */
        MEMCPY(netif->hwaddr, netdev_bd->hwaddr, ETH_ALEN);
        netbd_eth->old_hwaddr_valid = 1;
      }
    }
  }
  
  netdev_bd->chksum_flags |= netdev->chksum_flags; /* we must use the most efficient checksum flags */
  netif_bd->chksum_flags |= netdev->chksum_flags;
  _List_Ring_Add_Last(&netbd_eth->ring, &netbd->eth_ring);
  netbd->eth_cnt++;
  NETBD_UNLOCK(netbd);
  
  netbd_eth->input = netif->input; /* save the old input function */
  netif->input  = netbd_input; /* set new input function */
  netif->ext_eth = (void *)netbd_eth;
  netif->ext_ctl = (void *)netbd;
  
  if (need_up) {
    netifapi_netif_set_up(netif_bd); /* make bonding up */
  }
  
  if (!(netif->flags & NETIF_FLAG_UP)) {
    netif->flags |= NETIF_FLAG_UP;
    if (netif->up) {
      netif->up(netif); /* make up */
    }
  }
  
  if (!need_up && !netbd_eth->old_hwaddr_valid) { /* not first and change mac fail! */
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
  }
  
  return (0);
}

/* delete a net device from net bonding 
   'bddev' bonding device name (not ifname) 
   'subdev' sub ethernet device name 
            sub_is_ifname == 0: dev name
            sub_is_ifname == 1: if name */
int  netbd_delete_dev (const char *bddev, int bdindex, const char *sub, int sub_is_ifname)
{
  int i, found, flags;
  char subif[IFNAMSIZ];
  struct netif *netif;
  netbd_t *netbd;
  netbd_eth_t *netbd_eth;
  netdev_t *netdev_bd;
  netdev_t *netdev;
  struct ifreq ifreq;
  
  if (!sub) {
    errno = EINVAL;
    return (-1);
  }
  
  LWIP_IF_LIST_LOCK(FALSE);
  found = 0;
  if (bddev && bddev[0]) {
    netdev_bd = netdev_find_by_devname(bddev);
  } else {
    netdev_bd = netdev_find_by_index(bdindex);
  }
  if (netdev_bd && (netdev_bd->drv->transmit == netbd_transmit)) {
    netbd = (netbd_t *)netdev_bd->priv;
    if (netbd && netbd->magic_no == NETBONDING_MAGIC) {
      found = 1;
    }
  }
  if (!found) {
    LWIP_IF_LIST_UNLOCK();
    errno = ENXIO;
    return (-1);
  }
  
  found = 0; /* search from sub ethernet device list */
  netbd_eth = (netbd_eth_t *)netbd->eth_ring;
  for (i = 0; i < netbd->eth_cnt; i++) {
    netdev = netbd_eth->netdev;
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
    netbd_eth = (netbd_eth_t *)_list_ring_get_next(&netbd_eth->ring);
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
  
  if (netbd_eth->old_hwaddr_valid) { /* return old mac */
    if (netif->ioctl) {
      ifreq.ifr_name[0] = 0;
      MEMCPY(ifreq.ifr_hwaddr.sa_data, netbd_eth->old_hwaddr, ETH_ALEN);
      netif->ioctl(netif, SIOCSIFHWADDR, &ifreq); /* return old mac */
      MEMCPY(netif->hwaddr, netbd_eth->old_hwaddr, ETH_ALEN);
    }
  
  } else { /* IFF_PROMISC off */
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
  }
  
  netif->input = netbd_eth->input; /* restore old input function */
  netif->ext_ctl = netif->ext_eth = NULL;

  NETBD_LOCK(netbd);
  if (netbd->master == netbd_eth) {
    netbd->master = NULL;
  }
  netif->vlanid = netbd_eth->old_vlanid;
  _List_Ring_Del(&netbd_eth->ring, &netbd->eth_ring);
  netbd->eth_cnt--;
  if (netbd->working == netbd_eth) {
    if (netbd->eth_cnt) {
      netbd_select_working(netbd);
    } else {
      netbd->working = NULL;
    }
  }
  NETBD_UNLOCK(netbd);
  
  mem_free(netbd_eth);
  
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

/* net bonding set master device */
int  netbd_master_dev (const char *bddev, int bdindex, const char *sub, int sub_is_ifname)
{
  int i, found;
  char subif[IFNAMSIZ];
  netbd_t *netbd;
  netbd_eth_t *netbd_eth;
  netdev_t *netdev_bd;
  netdev_t *netdev;
  
  if (!sub) {
    errno = EINVAL;
    return (-1);
  }
  
  LWIP_IF_LIST_LOCK(FALSE);
  found = 0;
  if (bddev && bddev[0]) {
    netdev_bd = netdev_find_by_devname(bddev);
  } else {
    netdev_bd = netdev_find_by_index(bdindex);
  }
  if (netdev_bd && (netdev_bd->drv->transmit == netbd_transmit)) {
    netbd = (netbd_t *)netdev_bd->priv;
    if (netbd && netbd->magic_no == NETBONDING_MAGIC) {
      found = 1;
    }
  }
  if (!found) {
    LWIP_IF_LIST_UNLOCK();
    errno = ENXIO;
    return (-1);
  }
  
  found = 0; /* search from sub ethernet device list */
  netbd_eth = (netbd_eth_t *)netbd->eth_ring;
  for (i = 0; i < netbd->eth_cnt; i++) {
    netdev = netbd_eth->netdev;
    if (sub_is_ifname) {
      netdev_ifname(netdev, subif);
      if (!lib_strcmp(subif, sub)) {
        found = 1;
        break;
      }
    
    } else {
      if (!lib_strcmp(netdev->dev_name, sub)) {
        found = 1;
        break;
      }
    }
    netbd_eth = (netbd_eth_t *)_list_ring_get_next(&netbd_eth->ring);
  }
  if (!found) {
    LWIP_IF_LIST_UNLOCK();
    errno = ENXIO;
    return (-1);
  }
  LWIP_IF_LIST_UNLOCK();
  
  NETBD_LOCK(netbd);
  netbd->master = netbd_eth;
  NETBD_UNLOCK(netbd);
  
  return (0);
}

/* add a arp detect target to net bonding virtual device */
int  netbd_add_arp (const char *bddev, int bdindex, const char *ip)
{
  int found;
  netbd_t *netbd;
  netdev_t *netdev_bd;
  netbd_arp_t *netbd_arp;
  ip4_addr_t ipaddr;
  
  if (!ip) {
    errno = EINVAL;
    return (-1);
  }
  
  if (!ip4addr_aton(ip, &ipaddr)) {
    errno = EINVAL;
    return (-1);
  }
  
  LWIP_IF_LIST_LOCK(FALSE);
  found = 0;
  if (bddev && bddev[0]) {
    netdev_bd = netdev_find_by_devname(bddev);
  } else {
    netdev_bd = netdev_find_by_index(bdindex);
  }
  if (netdev_bd && (netdev_bd->drv->transmit == netbd_transmit)) {
    netbd = (netbd_t *)netdev_bd->priv;
    if (netbd && netbd->magic_no == NETBONDING_MAGIC) {
      found = 1;
    }
  }
  if (!found) {
    LWIP_IF_LIST_UNLOCK();
    errno = ENXIO;
    return (-1);
  }
  LWIP_IF_LIST_UNLOCK();
  
  netbd_arp = (netbd_arp_t *)mem_malloc(sizeof(netbd_arp_t));
  if (!netbd_arp) {
    errno = ENOMEM;
    return (-1);
  }
  lib_bzero(netbd_arp, sizeof(netbd_arp));
  
  ip4_addr_copy(netbd_arp->ipaddr, ipaddr);
  
  NETBD_LOCK(netbd);
  _List_Line_Add_Ahead(&netbd_arp->list, &netbd->arp_list);
  NETBD_UNLOCK(netbd);
  
  return (0);
}

/* delete a arp detect target to net bonding virtual device */
int  netbd_delete_arp (const char *bddev, int bdindex, const char *ip)
{
  int found;
  netbd_t *netbd;
  netdev_t *netdev_bd;
  netbd_arp_t *netbd_arp;
  ip4_addr_t ipaddr;
  LW_LIST_LINE *pline;
  
  if (!ip) {
    errno = EINVAL;
    return (-1);
  }
  
  if (!ip4addr_aton(ip, &ipaddr)) {
    errno = EINVAL;
    return (-1);
  }
  
  LWIP_IF_LIST_LOCK(FALSE);
  found = 0;
  if (bddev && bddev[0]) {
    netdev_bd = netdev_find_by_devname(bddev);
  } else {
    netdev_bd = netdev_find_by_index(bdindex);
  }
  if (netdev_bd && (netdev_bd->drv->transmit == netbd_transmit)) {
    netbd = (netbd_t *)netdev_bd->priv;
    if (netbd && netbd->magic_no == NETBONDING_MAGIC) {
      found = 1;
    }
  }
  if (!found) {
    LWIP_IF_LIST_UNLOCK();
    errno = ENXIO;
    return (-1);
  }
  LWIP_IF_LIST_UNLOCK();
  
  NETBD_LOCK(netbd);
  for (pline = netbd->arp_list; pline != NULL; pline = _list_line_get_next(pline)) {
    netbd_arp = (netbd_arp_t *)pline;
    if (ip4_addr_cmp(&netbd_arp->ipaddr, &ipaddr)) {
      break;
    }
  }
  if (pline) {
    _List_Line_Del(&netbd_arp->list, &netbd->arp_list);
  }
  NETBD_UNLOCK(netbd);
  
  if (pline) {
    mem_free(netbd_arp);
  }
  
  return (0);
}

/* add net bonding 
   'bddev' bonding device name (not ifname) */
int  netbd_add (const char *bddev, const char *ip, 
                const char *netmask, const char *gw, 
                int mode, int mon_mode, int interval,
                int alive, int *index)
{
  static u8_t init = 0;
  static struct netdev_funcs  netbd_drv;
  struct netif *netif_bd;
  netbd_t *netbd;
  netdev_t *netdev_bd;
  
  if ((mode != NETBD_MODE_BALANCE_RR) && 
      (mode != NETBD_MODE_ACTIVE_BACKUP) &&
      (mode != NETBD_MODE_BROADCAST)) {
    errno = EINVAL;
    return (-1);
  }
  
  if (mode == NETBD_MODE_ACTIVE_BACKUP) {
    if ((mon_mode != NETBD_MON_MODE_TRAFFIC) && (mon_mode != NETBD_MON_MODE_ARP)) {
      errno = EINVAL;
      return (-1);
    }
    
    if (alive < 0) {
      errno = EINVAL;
      return (-1);
    }
    
    if (mon_mode == NETBD_MON_MODE_ARP) {
      if ((interval < 0) || (interval > alive)) {
        errno = EINVAL;
        return (-1);
      }
    }
  }
  
  if (!bddev || (lib_strnlen(bddev, IF_NAMESIZE) >= IF_NAMESIZE)) {
    errno = EINVAL;
    return (-1);
  }
  
  if (!init) {
    if (sys_mutex_new(&netdb_list_lock) != ERR_OK) {
      return (-1);
    }
    
    if (!sys_thread_new(NETBONDING_THREAD_NAME, netbd_proc, 
                        NULL, NETBONDING_THREAD_STACKSIZE, NETBONDING_THREAD_PRIO)) {
      sys_mutex_free(&netdb_list_lock);
      return (-1);
    }
    
    init = 1;
  }
  
  netbd = (netbd_t *)mem_malloc(sizeof(netbd_t));
  if (!netbd) {
    errno = ENOMEM;
    return (-1);
  }
  lib_bzero(netbd, sizeof(netbd_t));
  
  netbd->mode = mode;
  netbd->mon_mode = mon_mode;
  netbd->timer = interval;
  netbd->interval = interval;
  netbd->alive = alive;
  
  netbd->magic_no = NETBONDING_MAGIC;
  netdev_bd = &netbd->netdev_bd;
  netif_bd = (struct netif *)netdev_bd->sys;
  
  netdev_bd->magic_no = NETDEV_MAGIC;
  lib_strlcpy(netdev_bd->dev_name, bddev, IF_NAMESIZE);
  lib_strlcpy(netdev_bd->if_name, "bd", IF_NAMESIZE);
  
  netdev_bd->if_hostname = "SylixOS Bonding";
  netdev_bd->init_flags = NETDEV_INIT_LOAD_PARAM
                        | NETDEV_INIT_LOAD_DNS
                        | NETDEV_INIT_IPV6_AUTOCFG;
  netdev_bd->chksum_flags = NETDEV_CHKSUM_DISABLE_ALL;
  netdev_bd->net_type = NETDEV_TYPE_ETHERNET; /* must ethernet device */
  netdev_bd->speed = 0;
  netdev_bd->mtu = ETH_DATA_LEN;
  netdev_bd->hwaddr_len = ETH_ALEN;
  
  netbd_drv.rxmode = netbd_rxmode;
  netbd_drv.transmit = netbd_transmit;
  netbd_drv.receive = netbd_receive;
  
  netdev_bd->drv = &netbd_drv;
  netdev_bd->priv = (void *)netbd;

  if (sys_mutex_new(&netbd->lock) != ERR_OK) {
    mem_free(netbd);
    return (-1);
  }
  
  if (netdev_add(netdev_bd, ip, netmask, gw, /* add to system */
                 IFF_RUNNING | IFF_BROADCAST | IFF_MULTICAST) < 0) {
    sys_mutex_free(&netbd->lock);
    mem_free(netbd);
    return (-1);
  }
  
  netif_bd->priv_flags = IFF_BONDING; /* bonding device */
  
  if (index) {
    netdev_index(netdev_bd, (unsigned int *)index);
  }
  
  NETBD_LIST_LOCK();
  _List_Line_Add_Ahead(&netbd->list, &netdb_list);
  NETBD_LIST_UNLOCK();
  
  return (0);
}

/* delete net bonding 
   'bddev' bonding device name (not ifname) */
int  netbd_delete (const char *bddev, int bdindex)
{
  int found, flags;
  struct netif *netif;
  netbd_t *netbd;
  netbd_eth_t *netbd_eth;
  netdev_t *netdev_bd;
  netdev_t *netdev;
  struct ifreq ifreq;
  
  LWIP_IF_LIST_LOCK(FALSE);
  found = 0;
  if (bddev && bddev[0]) {
    netdev_bd = netdev_find_by_devname(bddev);
  } else {
    netdev_bd = netdev_find_by_index(bdindex);
  }
  if (netdev_bd && (netdev_bd->drv->transmit == netbd_transmit)) {
    netbd = (netbd_t *)netdev_bd->priv;
    if (netbd && netbd->magic_no == NETBONDING_MAGIC) {
      netbd->magic_no = 0;
      found = 1;
    }
  }
  if (!found) {
    LWIP_IF_LIST_UNLOCK();
    errno = ENXIO;
    return (-1);
  }
  LWIP_IF_LIST_UNLOCK();
  
  netdev_delete(netdev_bd);
  
  NETBD_LIST_LOCK();
  _List_Line_Del(&netbd->list, &netdb_list);
  NETBD_LIST_UNLOCK();
  
  NETBD_LOCK(netbd);
  netbd->master = NULL;
  while (netbd->eth_ring) {
    netbd_eth = (netbd_eth_t *)netbd->eth_ring;
    netdev = netbd_eth->netdev;
    netif = (struct netif *)netdev->sys;
    netif->vlanid = netbd_eth->old_vlanid;
    _List_Ring_Del(&netbd_eth->ring, &netbd->eth_ring);
    netbd->eth_cnt--;
    NETBD_UNLOCK(netbd);
    
    if (netif->flags & NETIF_FLAG_UP) {
      netif->flags &= ~NETIF_FLAG_UP;
      if (netif->down) {
        netif->down(netif);
      }
    }
    
    if (netbd_eth->old_hwaddr_valid) { /* return old mac */
      if (netif->ioctl) {
        ifreq.ifr_name[0] = 0;
        MEMCPY(ifreq.ifr_hwaddr.sa_data, netbd_eth->old_hwaddr, ETH_ALEN);
        netif->ioctl(netif, SIOCSIFHWADDR, &ifreq); /* return old mac */
        MEMCPY(netif->hwaddr, netbd_eth->old_hwaddr, ETH_ALEN);
      }
  
    } else { /* IFF_PROMISC off */
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
    }
  
    netif->input = netbd_eth->input; /* restore input function */
    netif->ext_ctl = netif->ext_eth = NULL;
  
    mem_free(netbd_eth);
  
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
               
    NETBD_LOCK(netbd);
  }
  NETBD_UNLOCK(netbd);
  
  sys_mutex_free(&netbd->lock);
  mem_free(netbd);
  
  return (0);
}

/* change net bonding
   'bddev' bonding device name (not ifname) */
int  netbd_change (const char *bddev, int mode,
                   int mon_mode, int interval,
                   int alive)
{
  int found;
  netdev_t *netdev_bd;
  netbd_t *netbd;

  if (!bddev) {
    errno = EINVAL;
    return (-1);
  }

  if ((mode != NETBD_MODE_BALANCE_RR) &&
      (mode != NETBD_MODE_ACTIVE_BACKUP) &&
      (mode != NETBD_MODE_BROADCAST)) {
    errno = EINVAL;
    return (-1);
  }

  if (mode == NETBD_MODE_ACTIVE_BACKUP) {
    if ((mon_mode != NETBD_MON_MODE_TRAFFIC) && (mon_mode != NETBD_MON_MODE_ARP)) {
      errno = EINVAL;
      return (-1);
    }

    if (alive < 0) {
      errno = EINVAL;
      return (-1);
    }

    if (mon_mode == NETBD_MON_MODE_ARP) {
      if ((interval < 0) || (interval > alive)) {
        errno = EINVAL;
        return (-1);
      }
    }
  }

  LWIP_IF_LIST_LOCK(FALSE);
  found = 0;
  netdev_bd = netdev_find_by_devname(bddev);
  if (netdev_bd && (netdev_bd->drv->transmit == netbd_transmit)) {
    netbd = (netbd_t *)netdev_bd->priv;
    if (netbd && netbd->magic_no == NETBONDING_MAGIC) {
      found = 1;
    }
  }

  if (!found) {
    LWIP_IF_LIST_UNLOCK();
    errno = ENXIO;
    return (-1);
  }

  NETBD_LOCK(netbd);
  netbd->mode = mode;
  netbd->mon_mode = mon_mode;
  netbd->timer = interval;
  netbd->interval = interval;
  netbd->alive = alive;
  NETBD_UNLOCK(netbd);

  LWIP_IF_LIST_UNLOCK();

  return (0);
}

/* net bonding sub device delete hook
   NOTICE: this is in LWIP_IF_LIST_LOCK status */
void  netbd_sub_delete_hook (netdev_t *netdev)
{
  int i, flags;
  struct netif *netif;
  netbd_t *netbd;
  netbd_eth_t *netbd_eth;
  struct ifreq ifreq;
  
  netif = (struct netif *)netdev->sys;
  netbd = (netbd_t *)netif->ext_ctl;
  if (!netbd || netbd->magic_no != NETBONDING_MAGIC) {
    return;
  }
  netbd_eth = (netbd_eth_t *)netbd->eth_ring;
  for (i = 0; i < netbd->eth_cnt; i++) {
    if (netbd_eth->netdev == netdev) {
      break;
    }
    netbd_eth = (netbd_eth_t *)_list_ring_get_next(&netbd_eth->ring);
  }
  if (i >= netbd->eth_cnt) {
    return;
  }
  
  if (netif->flags & NETIF_FLAG_UP) {
    netif->flags &= ~NETIF_FLAG_UP;
    if (netif->down) {
      netif->down(netif); /* make down */
    }
  }
  
  if (netbd_eth->old_hwaddr_valid) { /* return old mac */
    if (netif->ioctl) {
      ifreq.ifr_name[0] = 0;
      MEMCPY(ifreq.ifr_hwaddr.sa_data, netbd_eth->old_hwaddr, ETH_ALEN);
      netif->ioctl(netif, SIOCSIFHWADDR, &ifreq); /* return old mac */
      MEMCPY(netif->hwaddr, netbd_eth->old_hwaddr, ETH_ALEN);
    }
  
  } else { /* IFF_PROMISC off */
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
  }
  
  NETBD_LOCK(netbd);
  if (netbd->master == netbd_eth) {
    netbd->master = NULL;
  }
  netif->vlanid = netbd_eth->old_vlanid;
  _List_Ring_Del(&netbd_eth->ring, &netbd->eth_ring);
  netbd->eth_cnt--;
  NETBD_UNLOCK(netbd);
  
  netif->input = netbd_eth->input; /* restore old input function */
  netif->ext_ctl = netif->ext_eth = NULL;
  
  mem_free(netbd_eth);
}

/* net bonding show all device in bonding virtual device */
int  netbd_show_dev (const char *bddev, int bdindex, int fd)
{
#define NETBD_ARP_BUF_MAX 16
  int found;
  int i = 0, j;
  struct netif *netif;
  netbd_t *netbd;
  netbd_eth_t *netbd_eth;
  netbd_arp_t *netbd_arp;
  netdev_t *netdev_bd;
  netdev_t *netdev;
  ip4_addr_t ipaddr[NETBD_ARP_BUF_MAX];
  char speed[32];
  char ifname[NETIF_NAMESIZE];
  LW_LIST_LINE *pline;
  
  if (fd < 0) {
    errno = EINVAL;
    return (-1);
  }
  
  LWIP_IF_LIST_LOCK(FALSE);
  found = 0;
  if (bddev && bddev[0]) {
    netdev_bd = netdev_find_by_devname(bddev);
  } else {
    netdev_bd = netdev_find_by_index(bdindex);
  }
  if (netdev_bd && (netdev_bd->drv->transmit == netbd_transmit)) {
    netbd = (netbd_t *)netdev_bd->priv;
    if (netbd && netbd->magic_no == NETBONDING_MAGIC) {
      found = 1;
    }
  }
  if (!found) {
    LWIP_IF_LIST_UNLOCK();
    errno = ENXIO;
    return (-1);
  }
  
  fdprintf(fd, "Net bonding summary >>\n\n");
  fdprintf(fd, "Device      : %s\n", bddev);
  fdprintf(fd, "Ifname      : %s\n", netif_get_name((struct netif *)netdev_bd->sys, ifname));
  if (netbd->mode == NETBD_MODE_BALANCE_RR) {
    fdprintf(fd, "Mode        : Balance RR\n");
  } else if (netbd->mode == NETBD_MODE_ACTIVE_BACKUP) {
    fdprintf(fd, "Mode        : Active Backup\n");
  } else {
    fdprintf(fd, "Mode        : Broadcast\n");
  }
  
  if (netbd->mode == NETBD_MODE_ACTIVE_BACKUP) {
    fdprintf(fd, "Moniting    : %s\n", (netbd->mon_mode == NETBD_MON_MODE_TRAFFIC) ? "Traffic" : "ARP Detect");
    fdprintf(fd, "Timeout     : %d (milliseconds)\n", netbd->alive);
    
    if (netbd->working) {
      netdev = netbd->working->netdev;
      netif = (struct netif *)netdev->sys;
      fdprintf(fd, "Working     : Dev: %s Prev-Ifname: %s\n", netdev->dev_name, netif_get_name(netif, ifname));
    }

    if (netbd->mon_mode == NETBD_MON_MODE_ARP) {
      fdprintf(fd, "ARP Interval: %d (milliseconds)\n", netbd->interval);
      fdprintf(fd, "\nNet bonding ARP target list >>\n\n");
      
      NETBD_LOCK(netbd);
      i = 0;
      for (pline = netbd->arp_list; pline != NULL; pline = _list_line_get_next(pline)) {
        if (i >= NETBD_ARP_BUF_MAX) {
          break;
        }
        netbd_arp = (netbd_arp_t *)pline;
        ip4_addr_copy(ipaddr[i], netbd_arp->ipaddr);
        i++;
      }
      NETBD_UNLOCK(netbd);
      
      for (j = 0; j < i; j++) {
        fdprintf(fd, "<%d> %s\n", j, ip4addr_ntoa_r(&ipaddr[j], speed, 32));
      }
    }
  }
  
  fdprintf(fd, "\nNet bonding device list >>\n\n");
  netbd_eth = (netbd_eth_t *)netbd->eth_ring;
  for (i = 0; i < netbd->eth_cnt; i++) {
    netdev = netbd_eth->netdev;
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
    fdprintf(fd, "<%d> Dev: %s Prev-Ifname: %s Spd: %s Active: %s Master: %s Linkup: %s Prev-HWaddr: ", i,
             netdev->dev_name, netif_get_name(netif, ifname),
             speed, netbd_eth->alive ? "Yes" : "No",
             (netbd_eth == netbd->master) ? "Yes" : "No",
             netdev->if_flags & IFF_RUNNING ? "Enable" : "Disable");
    for (j = 0; j < netif->hwaddr_len - 1; j++) {
      fdprintf(fd, "%02x:", netbd_eth->old_hwaddr[j]);
    }
    fdprintf(fd, "%02x\n", netbd_eth->old_hwaddr[netif->hwaddr_len - 1]);
    netbd_eth = (netbd_eth_t *)_list_ring_get_next(&netbd_eth->ring);
  }
  LWIP_IF_LIST_UNLOCK();
  
  return (0);
}

#endif /* LW_CFG_NET_DEV_BONDING_EN */
/*
 * end
 */
