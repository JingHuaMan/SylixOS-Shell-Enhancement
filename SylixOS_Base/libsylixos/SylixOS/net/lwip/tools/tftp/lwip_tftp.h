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
** ��   ��   ��: lwip_tftp.h
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2009 �� 05 �� 29 ��
**
** ��        ��: tftp ���繤��. (��֧��ѡ��)
*********************************************************************************************************/

#ifndef __LWIP_TFTP_H
#define __LWIP_TFTP_H

/*********************************************************************************************************
  �ü�����
*********************************************************************************************************/
#if (LW_CFG_NET_EN > 0) && (LW_CFG_NET_TFTP_EN > 0)

/*********************************************************************************************************
  tftp ����
*********************************************************************************************************/

#define LWIP_TFTP_TIMEOUT           3000                                /*  ��ʱʱ��                    */

/*********************************************************************************************************
  tftp Э�����
*********************************************************************************************************/

#define LWIP_TFTP_OPC_RDREQ         1                                   /*  ������                      */
#define LWIP_TFTP_OPC_WRREQ         2                                   /*  д����                      */
#define LWIP_TFTP_OPC_DATA          3                                   /*  �ļ�����                    */
#define LWIP_TFTP_OPC_ACK           4                                   /*  ����ȷ��                    */
#define LWIP_TFTP_OPC_ERROR         5                                   /*  error ��Ϣ                  */

#define LWIP_TFTP_DATA_HEADERLEN    4                                   /*  ���ݰ�ͷ����                */
#define LWIP_TFTP_ERROR_HEADERLEN   4                                   /*  ������Ϣ��ͷ����            */

/*********************************************************************************************************
  tftp �����
*********************************************************************************************************/

#define LWIP_TFTP_EC_NONE           0                                   /*  NONE                        */
#define LWIP_TFTP_EC_NOFILE         1                                   /*  �ļ�δ�ҵ�                  */
#define LWIP_TFTP_EC_NOACCESS       2                                   /*  ����ʧ��                    */
#define LWIP_TFTP_EC_DISKFULL       3                                   /*  ������                      */
#define LWIP_TFTP_EC_TFTP           4                                   /*  TFTP ����                   */
#define LWIP_TFTP_EC_ID             5                                   /*  ���� ID                     */
#define LWIP_TFTP_EC_FILEEXIST      6                                   /*  �ļ�����                    */
#define LWIP_TFTP_EC_NOUSER         7                                   /*  �û�����                    */

/*********************************************************************************************************
  tftp client API
*********************************************************************************************************/

LW_API INT          API_INetTftpReceive(CPCHAR  pcRemoteHost, 
                                        CPCHAR  pcFileName, 
                                        CPCHAR  pcLocalFileName);       /*  ����Զ���ļ�                */

LW_API INT          API_INetTftpSend(CPCHAR  pcRemoteHost, 
                                     CPCHAR  pcFileName, 
                                     CPCHAR  pcLocalFileName);          /*  ���ͱ����ļ�                */
                                     
#define inetTftpReceive             API_INetTftpReceive
#define inetTftpSend                API_INetTftpSend

/*********************************************************************************************************
  tftp server API
*********************************************************************************************************/

LW_API VOID         API_INetTftpServerInit(CPCHAR  pcPath);             /*  ���� TFTP ���׷�����        */
                                                                        /*  ��֧�ֶ����ƴ���            */
LW_API INT          API_INetTftpServerPath(CPCHAR  pcPath);             /*  ���÷�����·��              */

LW_API INT          API_INetTftpServerBindDev(UINT  uiIndex);


#define inetTftpServerInit          API_INetTftpServerInit
#define inetTftpServerPath          API_INetTftpServerPath
#define inetTftpServerBindDev       API_INetTftpServerBindDev

#endif                                                                  /*  LW_CFG_NET_EN > 0           */
                                                                        /*  LW_CFG_NET_TFTP_EN > 0      */
#endif                                                                  /*  __LWIP_TFTP_H               */
/*********************************************************************************************************
  END
*********************************************************************************************************/
