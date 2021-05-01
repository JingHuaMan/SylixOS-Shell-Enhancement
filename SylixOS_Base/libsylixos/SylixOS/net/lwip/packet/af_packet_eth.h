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
** ��   ��   ��: af_packet_eth.h
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2014 �� 03 �� 21 ��
**
** ��        ��: AF_PACKET ��̫������֧��
*********************************************************************************************************/

#ifndef __AF_PACKET_ETH_H
#define __AF_PACKET_ETH_H

/*********************************************************************************************************
  �ڲ�����
*********************************************************************************************************/

errno_t  __packetEthRawSendto(AF_PACKET_T          *pafpacket,
                              CPVOID                pvPacket, 
                              size_t                stBytes, 
                              struct sockaddr_ll   *psockaddrll);
errno_t  __packetEthDgramSendto(AF_PACKET_T          *pafpacket,
                                CPVOID                pvPacket, 
                                size_t                stBytes, 
                                struct sockaddr_ll   *psockaddrll);
size_t   __packetEthHeaderInfo(AF_PACKET_N  *pktm, struct sockaddr_ll *paddrll);
size_t   __packetEthHeaderInfo2(struct pbuf *p, struct netif *inp, BOOL bOutgo, 
                                struct sockaddr_ll *paddrll);
errno_t  __packetEthIfInfo(INT  iIndex, struct sockaddr_ll *paddrll);

#endif                                                                  /*  __AF_PACKET_ETH_H           */
/*********************************************************************************************************
  END
*********************************************************************************************************/
