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
** ��   ��   ��: lwip_wlspy.c
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2014 �� 06 �� 26 ��
**
** ��        ��: lwip �����������.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "SylixOS.h"
/*********************************************************************************************************
  �ü�����
*********************************************************************************************************/
#if (LW_CFG_NET_EN > 0) && (LW_CFG_NET_WIRELESS_EN > 0)
#include "netdev.h"
#include "net/if.h"
#include "net/if_arp.h"
#include "net/if_wireless.h"
#include "net/if_whandler.h"
/*********************************************************************************************************
** ��������: get_spydata
** ��������: ��� iw_spy_data
** �䡡��  : dev       ����ӿ�
** �䡡��  : spy_data
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static inline struct iw_spy_data *get_spydata (struct netdev *dev)
{
    struct iw_public_data *public_data;
    
    /* 
     * This is the new way 
     */
    if (dev->wireless_data) {
        public_data = (struct iw_public_data *)dev->wireless_data;
        return  (public_data->spy_data);
    }
    
    return  (NULL);
}
/*********************************************************************************************************
** ��������: iw_handler_set_spy
** ��������: set spy
** �䡡��  : dev       ����ӿ�
**           info      iw_request_info
**           wrqu      iwreq_data
**           extra     extra data
** �䡡��  : OK or ERROR
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
int iw_handler_set_spy (struct netdev          *dev,
                        struct iw_request_info *info,
                        union iwreq_data       *wrqu,
                        char                   *extra)
{
    struct iw_spy_data  *spydata = get_spydata(dev);
    struct sockaddr     *address = (struct sockaddr *)extra;

    /* Make sure driver is not buggy or using the old API */
    if (!spydata) {
        return  (-EOPNOTSUPP);
    }

    /* Disable spy collection while we copy the addresses.
     * While we copy addresses, any call to wireless_spy_update()
     * will NOP. This is OK, as anyway the addresses are changing. */
    spydata->spy_number = 0;

    /* We want to operate without locking, because wireless_spy_update()
     * most likely will happen in the interrupt handler, and therefore
     * have its own locking constraints and needs performance.
     * The rtnl_lock() make sure we don't race with the other iw_handlers.
     * This make sure wireless_spy_update() "see" that the spy list
     * is temporarily disabled. */
    KN_SMP_WMB();

    /* Are there are addresses to copy? */
    if (wrqu->data.length > 0) {
        int i;

        /* Copy addresses */
        for (i = 0; i < wrqu->data.length; i++) {
            lib_memcpy(spydata->spy_address[i], address[i].sa_data, ETH_ALEN);
        }
        
        /* Reset stats */
        lib_memset(spydata->spy_stat, 0, sizeof(struct iw_quality) * IW_MAX_SPY);
    }

    /* Make sure above is updated before re-enabling */
    KN_SMP_WMB();

    /* Enable addresses */
    spydata->spy_number = wrqu->data.length;

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: iw_handler_get_spy
** ��������: get spy
** �䡡��  : dev       ����ӿ�
**           info      iw_request_info
**           wrqu      iwreq_data
**           extra     extra data
** �䡡��  : OK or ERROR
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
int iw_handler_get_spy (struct netdev           *dev,
                        struct iw_request_info  *info,
                        union iwreq_data        *wrqu,
                        char                    *extra)
{
    struct iw_spy_data  *spydata = get_spydata(dev);
    struct sockaddr     *address = (struct sockaddr *)extra;
    int                  i;

    /* Make sure driver is not buggy or using the old API */
    if (!spydata) {
        return  (-EOPNOTSUPP);
    }

    wrqu->data.length = spydata->spy_number;

    /* Copy addresses. */
    for (i = 0; i < spydata->spy_number; i++) {
        lib_memcpy(address[i].sa_data, spydata->spy_address[i], ETH_ALEN);
        address[i].sa_family = AF_UNIX;
    }
    
    /* Copy stats to the user buffer (just after). */
    if (spydata->spy_number > 0) {
        lib_memcpy(extra  + (sizeof(struct sockaddr) *spydata->spy_number),
                   spydata->spy_stat,
                   sizeof(struct iw_quality) * spydata->spy_number);
    }
    
    /* Reset updated flags. */
    for (i = 0; i < spydata->spy_number; i++) {
        spydata->spy_stat[i].updated &= ~IW_QUAL_ALL_UPDATED;
    }
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: set spy threshold
** ��������: get spy
** �䡡��  : dev       ����ӿ�
**           info      iw_request_info
**           wrqu      iwreq_data
**           extra     extra data
** �䡡��  : OK or ERROR
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
int iw_handler_set_thrspy (struct netdev          *dev,
                           struct iw_request_info *info,
                           union iwreq_data       *wrqu,
                           char                   *extra)
{
    struct iw_spy_data  *spydata = get_spydata(dev);
    struct iw_thrspy    *threshold = (struct iw_thrspy *)extra;

    /* Make sure driver is not buggy or using the old API */
    if (!spydata) {
        return  (-EOPNOTSUPP);
    }

    /* Just do it */
    lib_memcpy(&(spydata->spy_thr_low), &(threshold->low),
               2 * sizeof(struct iw_quality));

    /* Clear flag */
    lib_memset(spydata->spy_thr_under, '\0', sizeof(spydata->spy_thr_under));

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: get spy threshold
** ��������: get spy
** �䡡��  : dev       ����ӿ�
**           info      iw_request_info
**           wrqu      iwreq_data
**           extra     extra data
** �䡡��  : OK or ERROR
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
int iw_handler_get_thrspy (struct netdev            *dev,
                           struct iw_request_info   *info,
                           union iwreq_data         *wrqu,
                           char                     *extra)
{
    struct iw_spy_data  *spydata = get_spydata(dev);
    struct iw_thrspy    *threshold = (struct iw_thrspy *)extra;

    /* Make sure driver is not buggy or using the old API */
    if (!spydata) {
        return  (-EOPNOTSUPP);
    }

    /* Just do it */
    lib_memcpy(&(threshold->low), &(spydata->spy_thr_low),
               2 * sizeof(struct iw_quality));

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: Prepare and send a Spy Threshold event
** ��������: get spy
** �䡡��  : dev       ����ӿ�
**           spydata   spy data
**           address   address
**           wstats    wireless quality
** �䡡��  : OK or ERROR
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static void iw_send_thrspy_event (struct netdev         *dev,
                                  struct iw_spy_data    *spydata,
                                  unsigned char         *address,
                                  struct iw_quality     *wstats)
{
    union iwreq_data    wrqu;
    struct iw_thrspy    threshold;

    /* Init */
    wrqu.data.length = 1;
    wrqu.data.flags = 0;
    
    /* Copy address */
    lib_memcpy(threshold.addr.sa_data, address, ETH_ALEN);
    threshold.addr.sa_family = ARPHRD_ETHER;
    
    /* Copy stats */
    lib_memcpy(&(threshold.qual), wstats, sizeof(struct iw_quality));
    
    /* Copy also thresholds */
    lib_memcpy(&(threshold.low), &(spydata->spy_thr_low),
               2 * sizeof(struct iw_quality));

    /* Send event to user space */
    wireless_send_event(dev, SIOCGIWTHRSPY, &wrqu, (char *)&threshold);
}
/*********************************************************************************************************
** ��������: wireless_spy_update
** ��������: get spy
** �䡡��  : dev       ����ӿ�
**           address   address
**           wstats    wireless quality
** �䡡��  : OK or ERROR
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
void wireless_spy_update (struct netdev     *dev,
                          unsigned char     *address,
                          struct iw_quality *wstats)
{
    struct iw_spy_data *spydata = get_spydata(dev);
    int                 i;
    int                 match = -1;

    /* Make sure driver is not buggy or using the old API */
    if (!spydata) {
        return;
    }

    /* Update all records that match */
    for (i = 0; i < spydata->spy_number; i++) {
        if (lib_memcmp(address, spydata->spy_address[i], ETH_ALEN) == 0) {
            lib_memcpy(&(spydata->spy_stat[i]), wstats,
                       sizeof(struct iw_quality));
            match = i;
        }
    }

    /* Generate an event if we cross the spy threshold.
     * To avoid event storms, we have a simple hysteresis : we generate
     * event only when we go under the low threshold or above the
     * high threshold. */
    if (match >= 0) {
        if (spydata->spy_thr_under[match]) {
            if (wstats->level > spydata->spy_thr_high.level) {
                spydata->spy_thr_under[match] = 0;
                iw_send_thrspy_event(dev, spydata,
                                     address, wstats);
            }
        } else {
            if (wstats->level < spydata->spy_thr_low.level) {
                spydata->spy_thr_under[match] = 1;
                iw_send_thrspy_event(dev, spydata,
                                     address, wstats);
            }
        }
    }
}

#endif                                                                  /*  LW_CFG_NET_EN > 0           */
                                                                        /*  LW_CFG_NET_WIRELESS_EN > 0  */
/*********************************************************************************************************
  END
*********************************************************************************************************/
