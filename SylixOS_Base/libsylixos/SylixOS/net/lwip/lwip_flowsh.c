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
** ��   ��   ��: lwip_flowsh.c
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2017 �� 12 �� 21 ��
**
** ��        ��: ������������.
*********************************************************************************************************/
#define  __SYLIXOS_STDIO
#define  __SYLIXOS_KERNEL
#include "SylixOS.h"
/*********************************************************************************************************
  �ü�����
*********************************************************************************************************/
#if LW_CFG_NET_EN > 0 && LW_CFG_SHELL_EN > 0 && LW_CFG_NET_FLOWCTL_EN > 0
#include "net/flowctl.h"
/*********************************************************************************************************
** ��������: __tshellFlowctl
** ��������: ϵͳ���� "flowctl"
** �䡡��  : iArgC         ��������
**           ppcArgV       ������
** �䡡��  : ERROR
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static INT  __tshellFlowctl (INT  iArgC, PCHAR  *ppcArgV)
{
    fprintf(stderr, "Flow control NOT include in open source version!\n");
    return  (PX_ERROR);
}
/*********************************************************************************************************
** ��������: __tshellFlowctlInit
** ��������: ע��������������
** �䡡��  : NONE
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
VOID __tshellFlowctlInit (VOID)
{
    API_TShellKeywordAdd("flowctl", __tshellFlowctl);
    API_TShellFormatAdd("flowctl", " [add | del | chg] [ip | if] [...]");
    API_TShellHelpAdd("flowctl",   "show, add, delete, change flow control status.\n"
    "eg. flowctl\n"
    "    flowctl add ip 192.168.1.1 192.168.1.10 tcp 20 80 dev en1 50 100 64\n"
    "       add a flow control rule: iface: en1(LAN Port) ip frome 192.168.1.1 to 192.168.1.10 tcp protocol port(20 ~ 80)\n"
    "       uplink 50KBytes downlink 100KBytes buffer is 64KBytes\n\n"
    
    "    flowctl add ip 192.168.1.1 192.168.1.10 udp 20 80 dev en1 50 100\n"
    "       add a flow control rule: iface: en1(LAN Port) ip frome 192.168.1.1 to 192.168.1.10 udp protocol port(20 ~ 80)\n"
    "       uplink 50KBytes downlink 100KBytes buffer is default size\n\n"
    
    "    flowctl add ip 192.168.1.1 192.168.1.10 all dev en1 50 100\n"
    "       add a flow control rule: iface: en1(LAN Port) ip frome 192.168.1.1 to 192.168.1.10 all protocol\n"
    "       uplink 50KBytes downlink 100KBytes buffer is default size\n\n"
    
    "    flowctl chg ip 192.168.1.1 192.168.1.10 tcp 20 80 dev en1 50 100\n"
    "       change flow control rule: iface: en1(LAN Port) ip frome 192.168.1.1 to 192.168.1.10 tcp protocol port(20 ~ 80)\n"
    "       uplink 50KBytes downlink 100KBytes buffer is default size\n\n"
    
    "    flowctl del ip 192.168.1.1 192.168.1.10 tcp 20 80 dev en1\n"
    "       delete a flow control rule: iface: en1(LAN Port) ip frome 192.168.1.1 to 192.168.1.10 tcp protocol port(20 ~ 80)\n\n"
    
    "    flowctl add if dev en1 50 100 64\n"
    "       add flow control rule: iface: en1(LAN Port)\n"
    "       uplink 50KBytes downlink 100KBytes buffer 64K\n\n"
    
    "    flowctl chg if dev en1 50 100\n"
    "       change flow control rule: iface: en1(LAN Port)\n"
    "       uplink 50KBytes downlink 100KBytes buffer is default size\n\n"
    
    "    flowctl del if dev en1\n"
    "       delete a flow control rule: iface: en1(LAN Port)\n");
}

#endif                                                                  /*  LW_CFG_NET_EN               */
                                                                        /*  LW_CFG_SHELL_EN > 0         */
                                                                        /*  LW_CFG_NET_FLOWCTL_EN > 0   */
/*********************************************************************************************************
  END
*********************************************************************************************************/
