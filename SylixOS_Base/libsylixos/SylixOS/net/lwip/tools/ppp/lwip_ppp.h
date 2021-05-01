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
** ��   ��   ��: lwip_ppp.h
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2014 �� 01 �� 08 ��
**
** ��        ��: lwip ppp ���ӹ�����.
*********************************************************************************************************/

#ifndef __LWIP_PPP_H
#define __LWIP_PPP_H

/*********************************************************************************************************
  �ü�����
*********************************************************************************************************/
#if (LW_CFG_NET_EN > 0) && (LW_CFG_LWIP_PPP > 0)

#ifdef __cplusplus
extern "C" {
#endif                                                                  /*  __cplusplus                 */

/*********************************************************************************************************
  ppp ���Ŵ��ڲ���
*********************************************************************************************************/

typedef struct {
    INT     baud;
    INT     stop_bits;                                                  /*  ֹͣλ�� 1, 2               */
    INT     parity;                                                     /*  0:��У�� 1:��У�� 2:żУ��  */
} LW_PPP_TTY;

/*********************************************************************************************************
  ppp ���Ų���
*********************************************************************************************************/

typedef struct {
    char   *user;
    char   *passwd;
} LW_PPP_DIAL;

/*********************************************************************************************************
  api
*********************************************************************************************************/

LW_API INT  API_PppOsCreate(CPCHAR  pcSerial, LW_PPP_TTY  *ptty, PCHAR  pcIfName, size_t  stMaxSize);
LW_API INT  API_PppOeCreate(CPCHAR  pcEthIf,  PCHAR  pcIfName, size_t  stMaxSize);
LW_API INT  API_PppOl2tpCreate(CPCHAR  pcEthIf, CPCHAR  pcIp, UINT16  usPort, CPCHAR  pcSecret,
                               size_t  stSecretLen, PCHAR   pcIfName, size_t  stMaxSize);
LW_API INT  API_PppDelete(CPCHAR  pcIfName);
LW_API INT  API_PppConnect(CPCHAR  pcIfName, LW_PPP_DIAL *pdial);
LW_API INT  API_PppDisconnect(CPCHAR  pcIfName, BOOL  bForce);
LW_API INT  API_PppGetPhase(CPCHAR  pcIfName, INT  *piPhase);

#define pppOsCreate     API_PppOsCreate
#define pppOeCreate     API_PppOeCreate
#define pppOl2tpCreate  API_PppOl2tpCreate
#define pppIfDelete     API_PppDelete
#define pppConnect      API_PppConnect
#define pppDisconnect   API_PppDisconnect
#define pppGetPhase     API_PppGetPhase

#ifdef __cplusplus
}
#endif                                                                  /*  __cplusplus                 */

#endif                                                                  /*  LW_CFG_NET_EN > 0           */
                                                                        /*  LW_CFG_LWIP_PPP > 0         */
#endif                                                                  /*  __LWIP_PPPFD_H              */
/*********************************************************************************************************
  END
*********************************************************************************************************/
