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
** ��   ��   ��: lwip_ftp.h
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2009 �� 11 �� 21 ��
**
** ��        ��: ftp Э�鹤��.
** ��        ��: ftp ������. (������ʾ���� SylixOS Ŀ¼)
** ע        ��: ftp ������ 1 ���Ӳ�����, �����Զ��Ͽ�, ���ͷ���Դ.
                 ��� ftp �������ĸ�Ŀ¼����ϵͳ��Ŀ¼, ��Щ�汾 windows ���ʴ˷�������Щ����������, 
                 ����: ������. �Ƽ��� ftp ��Ŀ¼����Ϊ /
                 �Ƽ�ʹ�� CuteFTP, FileZilla ��רҵ FTP �ͻ��˷��ʴ˷�����.
*********************************************************************************************************/

#ifndef __LWIP_FTP_H
#define __LWIP_FTP_H

/*********************************************************************************************************
  �ü�����
*********************************************************************************************************/
#if (LW_CFG_NET_EN > 0) && (LW_CFG_NET_FTPD_EN > 0)

LW_API VOID     API_INetFtpServerInit(CPCHAR  pcPath);
LW_API VOID     API_INetFtpServerShow(VOID);
LW_API INT      API_INetFtpServerPath(CPCHAR  pcPath);
LW_API INT      API_INetFtpServerBindDev(UINT  uiIndex);

#define inetFtpServerInit           API_INetFtpServerInit
#define inetFtpServerShow           API_INetFtpServerShow
#define inetFtpServerPath           API_INetFtpServerPath
#define inetFtpServerBindDev        API_INetFtpServerBindDev

#endif                                                                  /*  LW_CFG_NET_EN > 0           */
                                                                        /*  LW_CFG_NET_FTPD_EN > 0      */
#endif                                                                  /*  __LWIP_FTP_H                */
/*********************************************************************************************************
  END
*********************************************************************************************************/
