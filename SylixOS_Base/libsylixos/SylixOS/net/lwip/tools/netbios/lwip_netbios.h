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
** ��   ��   ��: lwip_netbios.h
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2009 �� 05 �� 06 ��
**
** ��        ��: lwip netbios �������ַ�����. (���Դ����Դ�� contrib)
*********************************************************************************************************/

#ifndef __NETBIOS_H
#define __NETBIOS_H

/*********************************************************************************************************
  �ü�����
*********************************************************************************************************/
#if (LW_CFG_NET_EN > 0) && (LW_CFG_NET_NETBIOS_EN > 0)

LW_API VOID     API_INetNetBiosInit(VOID);
LW_API INT      API_INetNetBiosBindDev(UINT  uiIndex);
LW_API ULONG    API_INetNetBiosNameSet(CPCHAR  pcLocalName);
LW_API ULONG    API_INetNetBiosNameGet(PCHAR   pcLocalNameBuffer, INT  iMaxLen);

#define inetNetBiosInit         API_INetNetBiosInit
#define inetNetBiosBindDev      API_INetNetBiosBindDev
#define inetNetBiosNameSet      API_INetNetBiosNameSet
#define inetNetBiosNameGet      API_INetNetBiosNameGet

#endif                                                                  /*  (LW_CFG_NET_EN > 0)         */
                                                                        /*  (LW_CFG_NET_NETBIOS_EN > 0) */
#endif                                                                  /*  __NETBIOS_H                 */
/*********************************************************************************************************
  END
*********************************************************************************************************/
