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
** ��   ��   ��: lwip_hosttable.h
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2009 �� 08 �� 19 ��
**
** ��        ��: lwip ��̬ DNS ����������.
*********************************************************************************************************/

#ifndef __LWIP_HOSTTABLE_H
#define __LWIP_HOSTTABLE_H

#include "lwip/inet.h"
/*********************************************************************************************************
  ��������̬��ַת�������
*********************************************************************************************************/

LW_API INT          API_INetHostTableGetItem(CPCHAR  pcHost, struct in_addr  *pinaddr);
LW_API INT          API_INetHostTableAddItem(CPCHAR  pcHost, struct in_addr  inaddr);
LW_API INT          API_INetHostTableDelItem(CPCHAR  pcHost);

#define inetHostTableGetItem        API_INetHostTableGetItem
#define inetHostTableAddItem        API_INetHostTableAddItem
#define inetHostTableDelItem        API_INetHostTableDelItem

#endif                                                                  /*  __LWIP_HOSTTABLE_H          */
/*********************************************************************************************************
  END
*********************************************************************************************************/
