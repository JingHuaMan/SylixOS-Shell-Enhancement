/**
 * @file
 * netif API (to be used from non-TCPIP threads)
 */

/*
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
 * This file is part of the lwIP TCP/IP stack.
 *
 */
#ifndef LWIP_HDR_NETIFAPI_H
#define LWIP_HDR_NETIFAPI_H

#include "lwip/opt.h"

#if LWIP_NETIF_API /* don't build if not configured for use in lwipopts.h */

#include "lwip/sys.h"
#include "lwip/netif.h"
#include "lwip/dhcp.h"
#include "lwip/dhcp6.h" /* SylixOS Add */
#include "lwip/autoip.h"
#include "lwip/priv/tcpip_priv.h"
#include "lwip/priv/api_msg.h"
#include "lwip/prot/ethernet.h"

#ifdef __cplusplus
extern "C" {
#endif

/* API for application */
#if LWIP_ARP && LWIP_IPV4
/* Used for netfiapi_arp_* APIs */
enum netifapi_arp_entry {
  NETIFAPI_ARP_PERM /* Permanent entry */
  /* Other entry types can be added here */
};

/** @ingroup netifapi_arp */
err_t netifapi_arp_add(const ip4_addr_t *ipaddr, struct eth_addr *ethaddr, enum netifapi_arp_entry type);
/** @ingroup netifapi_arp */
err_t netifapi_arp_remove(const ip4_addr_t *ipaddr, enum netifapi_arp_entry type);

#ifdef SYLIXOS /* SylixOS Add this safe function */
void netifapi_arp_traversal(struct netif *netif, int (*callback)(), void *arg0, void *arg1,
                            void *arg2, void *arg3, void *arg4, void *arg5);
void netifapi_arp_cleanup(struct netif *netif);
err_t netifapi_arp_update(struct netif *netif, const ip4_addr_t *ipaddr, struct eth_addr *ethaddr, u8_t flags);
#endif /* SYLIXOS */
#endif /* LWIP_ARP && LWIP_IPV4 */

#ifdef SYLIXOS /* SylixOS Add this safe function */
#if LWIP_IPV6
void netifapi_nd6_traversal(struct netif *netif, int (*callback)(), void *arg0, void *arg1,
                            void *arg2, void *arg3, void *arg4, void *arg5);
void netifapi_nd6_cleanup(struct netif *netif);
#endif /* LWIP_IPV6*/
#endif /* SYLIXOS */

err_t netifapi_netif_add(struct netif *netif,
#if LWIP_IPV4
                         const ip4_addr_t *ipaddr, const ip4_addr_t *netmask, const ip4_addr_t *gw,
#endif /* LWIP_IPV4 */
                         void *state, netif_init_fn init, netif_input_fn input);

#if LWIP_IPV4
err_t netifapi_netif_set_addr(struct netif *netif, const ip4_addr_t *ipaddr,
                              const ip4_addr_t *netmask, const ip4_addr_t *gw);
#endif /* LWIP_IPV4*/

err_t netifapi_netif_common(struct netif *netif, netifapi_void_fn voidfunc,
                            netifapi_errt_fn errtfunc);

/** @ingroup netifapi_netif */
err_t netifapi_netif_name_to_index(const char *name, u8_t *index);
/** @ingroup netifapi_netif */
err_t netifapi_netif_index_to_name(u8_t index, char *name);

/** @ingroup netifapi_netif
  * @see netif_remove()
  */
#define netifapi_netif_remove(n)        netifapi_netif_common(n, netif_remove, NULL)
/** @ingroup netifapi_netif
  * @see netif_set_up()
  */
#define netifapi_netif_set_up(n)        netifapi_netif_common(n, netif_set_up, NULL)
/** @ingroup netifapi_netif
  * @see netif_set_down()
  */
#define netifapi_netif_set_down(n)      netifapi_netif_common(n, netif_set_down, NULL)
/** @ingroup netifapi_netif
  * @see netif_set_default()
  */
#define netifapi_netif_set_default(n)   netifapi_netif_common(n, netif_set_default, NULL)
/** @ingroup netifapi_netif
  * @see netif_set_link_up()
  */
#define netifapi_netif_set_link_up(n)   netifapi_netif_common(n, netif_set_link_up, NULL)
/** @ingroup netifapi_netif
  * @see netif_set_link_down()
  */
#define netifapi_netif_set_link_down(n) netifapi_netif_common(n, netif_set_link_down, NULL)

/**
 * @defgroup netifapi_dhcp4 DHCPv4
 * @ingroup netifapi
 * To be called from non-TCPIP threads
 */
/** @ingroup netifapi_dhcp4
  * @see dhcp_start()
  */
#define netifapi_dhcp_start(n)            netifapi_netif_common(n, NULL, dhcp_start)
/**
 * @ingroup netifapi_dhcp4
 * @deprecated Use netifapi_dhcp_release_and_stop() instead.
 */
#define netifapi_dhcp_stop(n)             netifapi_netif_common(n, dhcp_stop, NULL)
/** @ingroup netifapi_dhcp4
  * @see dhcp_inform()
  */
#define netifapi_dhcp_inform(n)           netifapi_netif_common(n, dhcp_inform, NULL)
/** @ingroup netifapi_dhcp4
  * @see dhcp_renew()
  */
#define netifapi_dhcp_renew(n)            netifapi_netif_common(n, NULL, dhcp_renew)
/**
 * @ingroup netifapi_dhcp4
 * @deprecated Use netifapi_dhcp_release_and_stop() instead.
 */
#define netifapi_dhcp_release(n)          netifapi_netif_common(n, NULL, dhcp_release)
/** @ingroup netifapi_dhcp4
  * @see dhcp_release_and_stop()
  */
#define netifapi_dhcp_release_and_stop(n) netifapi_netif_common(n, dhcp_release_and_stop, NULL)

/**
 * @defgroup netifapi_autoip AUTOIP
 * @ingroup netifapi
 * To be called from non-TCPIP threads
 */
/** @ingroup netifapi_autoip
  * @see autoip_start()
  */
#define netifapi_autoip_start(n)      netifapi_netif_common(n, NULL, autoip_start)
/** @ingroup netifapi_autoip
  * @see autoip_stop()
  */
#define netifapi_autoip_stop(n)       netifapi_netif_common(n, NULL, autoip_stop)

#ifdef SYLIXOS /* SylixOS Add this safe function */
#if LWIP_IPV6 && LWIP_IPV6_DHCP6
/** @ingroup netifapi_dhcp6
  * @see dhcp6_enable_stateful()
  */
#define netifapi_dhcp6_enable_stateful(n)   netifapi_netif_common(n, NULL, dhcp6_enable_stateful)
/** @ingroup netifapi_dhcp6
  * @see dhcp6_enable_stateless()
  */
#define netifapi_dhcp6_enable_stateless(n)  netifapi_netif_common(n, NULL, dhcp6_enable_stateless)
/** @ingroup netifapi_dhcp6
  * @see dhcp6_disable()
  */
#define netifapi_dhcp6_disable(n)           netifapi_netif_common(n, dhcp6_disable, NULL)
#endif /* LWIP_IPV6 && LWIP_IPV6_DHCP6 */
#endif /* SYLIXOS */

#ifdef __cplusplus
}
#endif

#endif /* LWIP_NETIF_API */

#endif /* LWIP_HDR_NETIFAPI_H */
