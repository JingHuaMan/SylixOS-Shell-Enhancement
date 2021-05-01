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
** ��   ��   ��: lwip_nat.h
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2010 �� 03 �� 19 ��
**
** ��        ��: lwip NAT ֧�ְ�.
*********************************************************************************************************/

#ifndef __LWIP_NAT_H
#define __LWIP_NAT_H

/*********************************************************************************************************
  �ü�����
*********************************************************************************************************/
#if (LW_CFG_NET_EN > 0) && (LW_CFG_NET_ROUTER > 0) && (LW_CFG_NET_NAT_EN > 0)

LW_API VOID         API_INetNatInit(VOID);
LW_API INT          API_INetNatStart(CPCHAR  pcLocalNetif, CPCHAR  pcApNetif);
LW_API INT          API_INetNatStop(VOID);
LW_API INT          API_INetNatIpFragSet(UINT8  ucProto, BOOL  bOn);
LW_API INT          API_INetNatIpFragGet(UINT8  ucProto, BOOL  *pbOn);
LW_API INT          API_INetNatLocalAdd(CPCHAR  pcLocalNetif);
LW_API INT          API_INetNatLocalDelete(CPCHAR  pcLocalNetif);
LW_API INT          API_INetNatWanAdd(CPCHAR  pcApNetif);
LW_API INT          API_INetNatWanDelete(CPCHAR  pcApNetif);
LW_API INT          API_INetNatMapAdd(CPCHAR  pcLocalIp, UINT16  usIpCnt, UINT16  usLocalPort, 
                                      UINT16  usAssPort, UINT8   ucProto);
LW_API INT          API_INetNatMapDelete(CPCHAR  pcLocalIp, UINT16  usLocalPort, 
                                         UINT16  usAssPort, UINT8   ucProto);
LW_API INT          API_INetNatAliasAdd(CPCHAR  pcAliasIp, CPCHAR  pcSLocalIp, CPCHAR  pcELocalIp);
LW_API INT          API_INetNatAliasDelete(CPCHAR  pcAliasIp);

#define inetNatInit         API_INetNatInit
#define inetNatStart        API_INetNatStart
#define inetNatStop         API_INetNatStop
#define inetNatIpFragSet    API_INetNatIpFragSet
#define inetNatIpFragGet    API_INetNatIpFragGet
#define inetNatLocalAdd     API_INetNatLocalAdd
#define inetNatLocalDelete  API_INetNatLocalDelete
#define inetNatWanAdd       API_INetNatWanAdd
#define inetNatWanDelete    API_INetNatWanDelete
#define inetNatMapAdd       API_INetNatMapAdd
#define inetNatMapDelete    API_INetNatMapDelete
#define inetNatAliasAdd     API_INetNatAliasAdd
#define inetNatAliasDelete  API_INetNatAliasDelete

#endif                                                                  /*  LW_CFG_NET_EN > 0           */
                                                                        /*  LW_CFG_NET_ROUTER > 0       */
                                                                        /*  LW_CFG_NET_NAT_EN > 0       */
#endif                                                                  /*  __LWIP_NAT_H                */
/*********************************************************************************************************
  END
*********************************************************************************************************/
