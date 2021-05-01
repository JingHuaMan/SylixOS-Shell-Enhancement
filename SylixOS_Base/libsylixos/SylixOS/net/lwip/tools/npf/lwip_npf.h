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
** ��   ��   ��: lwip_npf.h
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2011 �� 02 �� 11 ��
**
** ��        ��: lwip net packet filter ����.
*********************************************************************************************************/

#ifndef __LWIP_NPF_H
#define __LWIP_NPF_H

/*********************************************************************************************************
  �ü�����
*********************************************************************************************************/
#if (LW_CFG_NET_EN > 0) && (LW_CFG_NET_NPF_EN > 0)

#define LWIP_NPF_RULE_MAC   0
#define LWIP_NPF_RULE_IP    1
#define LWIP_NPF_RULE_UDP   2
#define LWIP_NPF_RULE_TCP   3

LW_API INT      API_INetNpfInit(VOID);

LW_API PVOID    API_INetNpfRuleAdd(CPCHAR  pcIfname,
                                   INT     iRule,
                                   UINT8   pucMac[],
                                   CPCHAR  pcAddrStart,
                                   CPCHAR  pcAddrEnd,
                                   UINT16  usPortStart,
                                   UINT16  usPortEnd);
LW_API PVOID    API_INetNpfRuleAddEx(CPCHAR  pcIfname,
                                     INT     iRule,
                                     BOOL    bIn,
                                     BOOL    bAllow,
                                     UINT8   pucMac[],
                                     CPCHAR  pcAddrStart,
                                     CPCHAR  pcAddrEnd,
                                     UINT16  usPortStart,
                                     UINT16  usPortEnd);
                                     
LW_API INT      API_INetNpfRuleDel(CPCHAR  pcIfname,
                                   PVOID   pvRule,
                                   INT     iSeqNum);
LW_API INT      API_INetNpfRuleDelEx(CPCHAR  pcIfname,
                                     BOOL    bIn,
                                     PVOID   pvRule,
                                     INT     iSeqNum);
                                     
LW_API ULONG    API_INetNpfDropGet(VOID);
LW_API ULONG    API_INetNpfAllowGet(VOID);
LW_API INT      API_INetNpfShow(INT  iFd);

#define inetNpfInit         API_INetNpfInit
#define inetNpfRuleAdd      API_INetNpfRuleAdd
#define inetNpfRuleAddEx    API_INetNpfRuleAddEx
#define inetNpfRuleDel      API_INetNpfRuleDel
#define inetNpfRuleDelEx    API_INetNpfRuleDelEx
#define inetNpfDropGet      API_INetNpfDropGet
#define inetNpfAllowGet     API_INetNpfAllowGet
#define inetNpfShow         API_INetNpfShow

#endif                                                                  /*  LW_CFG_NET_EN > 0           */
                                                                        /*  LW_CFG_NET_NPF_EN > 0       */
#endif                                                                  /*  __LWIP_NPF_H                */
/*********************************************************************************************************
  END
*********************************************************************************************************/
