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
** ��   ��   ��: route.h
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2013 �� 01 �� 15 ��
**
** ��        ��: SylixOS ����·�ɽӿ�.
*********************************************************************************************************/

#ifndef __SYS_ROUTE_H
#define __SYS_ROUTE_H

#include "types.h"
#include "socket.h"

/*********************************************************************************************************
  �ü�����
*********************************************************************************************************/
#if (LW_CFG_NET_EN > 0) && (LW_CFG_NET_ROUTER > 0)

#include <net/if.h>
#include <net/route.h>

#ifdef __cplusplus
extern "C" {
#endif                                                                  /*  __cplusplus                 */

LW_API int  route_add(const struct rtentry *prtentry);
LW_API int  route_delete(const struct rtentry *prtentry);
LW_API int  route_change(const struct rtentry *prtentry);
LW_API int  route_get(struct rtentry *prtentry);
LW_API int  route_list(INT  iFamily, struct rtentry_list *prtentrylist);

#ifdef __cplusplus
}
#endif                                                                  /*  __cplusplus                 */

#endif                                                                  /*  LW_CFG_NET_EN > 0           */
                                                                        /*  LW_CFG_NET_ROUTER > 0       */
#endif                                                                  /*  __SYS_ROUTE_H               */
/*********************************************************************************************************
  END
*********************************************************************************************************/
