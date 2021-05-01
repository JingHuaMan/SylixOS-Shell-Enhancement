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
** ��   ��   ��: lwip_netevent.h
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2014 �� 06 �� 28 ��
**
** ��        ��: �����¼��ӿ�.
*********************************************************************************************************/

#ifndef __LWIP_NETEVENT_H
#define __LWIP_NETEVENT_H

/*********************************************************************************************************
  �ü�����
*********************************************************************************************************/
#if LW_CFG_NET_EN > 0

#include "net/if.h"

#ifdef __cplusplus
extern "C" {
#endif                                                                  /*  __cplusplus                 */

/*********************************************************************************************************
  �����¼��ļ�
*********************************************************************************************************/

#define NET_EVENT_DEV_PATH              "/dev/netevent"                 /*  �����¼��豸·��            */
#define NET_EVENT_DEV_MAX_MSGSIZE       (4 + IFNAMSIZ + (4 * 4))        /*  �����Ϣ����                */

/*********************************************************************************************************
  �����¼�����
*********************************************************************************************************/
/*********************************************************************************************************
  ͨ���¼�����
*********************************************************************************************************/

#define NET_EVENT_STD           0                                       /*  ������׼�¼�                */

#define NET_EVENT_ADD           (NET_EVENT_STD + 0)                     /*  �������                    */
#define NET_EVENT_REMOVE        (NET_EVENT_STD + 1)                     /*  ����ɾ��                    */

#define NET_EVENT_UP            (NET_EVENT_STD + 2)                     /*  ����ʹ��                    */
#define NET_EVENT_DOWN          (NET_EVENT_STD + 3)                     /*  ��������                    */

#define NET_EVENT_LINK          (NET_EVENT_STD + 4)                     /*  ����������                  */
#define NET_EVENT_UNLINK        (NET_EVENT_STD + 5)                     /*  �����Ͽ�����                */

#define NET_EVENT_ADDR          (NET_EVENT_STD + 6)                     /*  ������ַ�仯                */
#define NET_EVENT_ADDR_CONFLICT (NET_EVENT_STD + 9)                     /*  ������ַ��ͻ                */
#define NET_EVENT_AUTH_FAIL     (NET_EVENT_STD + 7)                     /*  ������֤ʧ��                */

/*********************************************************************************************************
  PPP �¼�����
*********************************************************************************************************/

#define NET_EVENT_PPP           100

#define NET_EVENT_PPP_DEAD      (NET_EVENT_PPP + 0)                     /*  ����ֹͣ                    */
#define NET_EVENT_PPP_INIT      (NET_EVENT_PPP + 1)                     /*  �����ʼ������              */
#define NET_EVENT_PPP_AUTH      (NET_EVENT_PPP + 2)                     /*  �����û���֤                */
#define NET_EVENT_PPP_RUN       (NET_EVENT_PPP + 3)                     /*  ������ͨ                    */
#define NET_EVENT_PPP_DISCONN   (NET_EVENT_PPP + 4)                     /*  ���������ж�                */
#define NET_EVENT_PPP_TIMEOUT   (NET_EVENT_PPP + 5)                     /*  ���ӳ�ʱ                    */

/*********************************************************************************************************
  wireless �¼�����
*********************************************************************************************************/

#define NET_EVENT_WL            200                                     /*  ���������¼�                */

#define NET_EVENT_WL_QUAL       (NET_EVENT_WL + 0)                      /*  �������߻����仯(�ź�ǿ�ȵ�)*/
#define NET_EVENT_WL_SCAN       (NET_EVENT_WL + 1)                      /*  �������� AP ɨ�����        */
#define NET_EVENT_WL_EXT        (NET_EVENT_WL + 50)                     /*  �û��Զ��������¼�          */
#define NET_EVENT_WL_EXT2       (NET_EVENT_WL + 51)                     /*  �û��Զ��������¼�2         */

/*********************************************************************************************************
  �ں� API
*********************************************************************************************************/

#ifdef __SYLIXOS_KERNEL
VOID  netEventIfAdd(struct netif *pnetif);
VOID  netEventIfRemove(struct netif *pnetif);
VOID  netEventIfUp(struct netif *pnetif);
VOID  netEventIfDown(struct netif *pnetif);
VOID  netEventIfLink(struct netif *pnetif);
VOID  netEventIfUnlink(struct netif *pnetif);
VOID  netEventIfAddr(struct netif *pnetif);
VOID  netEventIfAddrConflict(struct netif *pnetif, UINT8 ucHw[], UINT uiHwLen);
VOID  netEventIfAuthFail(struct netif *pnetif);
VOID  netEventIfPppExt(struct netif *pnetif, UINT32  uiEvent);
VOID  netEventIfWlQual(struct netif *pnetif);
VOID  netEventIfWlScan(struct netif *pnetif);
VOID  netEventIfWlExt(struct netif *pnetif, 
                      UINT32        uiEvent, 
                      UINT32        uiArg0,
                      UINT32        uiArg1,
                      UINT32        uiArg2,
                      UINT32        uiArg3);
VOID  netEventIfWlExt2(struct netif *pnetif,
                       PVOID         pvEvent,
                       UINT32        uiArg);
#endif                                                                  /*  __SYLIXOS_KERNEL            */

#ifdef __cplusplus
}
#endif                                                                  /*  __cplusplus                 */

#endif                                                                  /*  LW_CFG_NET_EN > 0           */
#endif                                                                  /*  __LWIP_NETEVENT_H           */
/*********************************************************************************************************
  END
*********************************************************************************************************/
