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
** ��   ��   ��: lwip_ftplib.h
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2009 �� 11 �� 21 ��
**
** ��        ��: ftp Э����ظ�ʽ�����.
*********************************************************************************************************/

#ifndef __LWIP_FTPLIB_H
#define __LWIP_FTPLIB_H

/*********************************************************************************************************
  ��������Ϣ
*********************************************************************************************************/
#define __FTPD_SERVER_MESSAGE   "SylixOS FTP server ready." __SYLIXOS_VERINFO
#define __FTPD_SYSTYPE          "UNIX Type: L8"

/*********************************************************************************************************
  ��������Ӧ��
*********************************************************************************************************/
#define __FTPD_RETCODE_SERVER_RESTART_ACK       110                     /*  �����������Ӧ��            */
#define __FTPD_RETCODE_SERVER_READY_MIN         120                     /*  ������nnn������׼����       */
#define __FTPD_RETCODE_SERVER_DATALINK_READY    125                     /*  ����������׼����            */
#define __FTPD_RETCODE_SERVER_FILE_OK           150                     /*  �ļ�״̬���ã�����������  */
#define __FTPD_RETCODE_SERVER_CMD_OK            200                     /*  ����ɹ�                    */
#define __FTPD_RETCODE_SERVER_CMD_UNSUPPORT     202                     /*  ����δʵ��                  */
#define __FTPD_RETCODE_SERVER_STATUS_HELP       211                     /*  ϵͳ״̬��ϵͳ������Ӧ      */
#define __FTPD_RETCODE_SERVER_DIR_STATUS        212                     /*  Ŀ¼״̬                    */
#define __FTPD_RETCODE_SERVER_FILE_STATUS       213                     /*  �ļ�״̬                    */
#define __FTPD_RETCODE_SERVER_HELP_INFO         214                     /*  ������Ϣ, ���������û�����  */
#define __FTPD_RETCODE_SERVER_NAME_SYS_TYPE     215                     /*  ����ϵͳ����                */
#define __FTPD_RETCODE_SERVER_READY             220                     /*  ����������                  */
#define __FTPD_RETCODE_SERVER_BYEBYE            221                     /*  ����رտ������ӿ����˳���¼*/
#define __FTPD_RETCODE_SERVER_DATALINK_NODATA   225                     /*  �������Ӵ򿪣��޴������ڽ���*/
#define __FTPD_RETCODE_SERVER_DATACLOSE_NOERR   226                     /*  �ر���������, �ļ������ɹ�  */
#define __FTPD_RETCODE_SERVER_INTO_PASV         227                     /*  ���뱻��ģʽ                */
#define __FTPD_RETCODE_SERVER_USER_LOGIN        230                     /*  �û���¼                    */
#define __FTPD_RETCODE_SERVER_FILE_OP_OK        250                     /*  ������ļ��������          */
#define __FTPD_RETCODE_SERVER_MAKE_DIR_OK       257                     /*  ����Ŀ¼�ɹ�                */
#define __FTPD_RETCODE_SERVER_PW_REQ            331                     /*  �û�����ȷ����Ҫ����        */
#define __FTPD_RETCODE_SERVER_NEED_INFO         350                     /*  ������Ҫ��һ������Ϣ        */
#define __FTPD_RETCODE_SERVER_DATALINK_FAILED   425                     /*  ���ܴ���������            */
#define __FTPD_RETCODE_SERVER_DATALINK_ABORT    426                     /*  �ر����ӣ���ֹ����          */
#define __FTPD_RETCODE_SERVER_REQ_NOT_RUN       450                     /*  ��������δִ��              */
#define __FTPD_RETCODE_SERVER_REQ_ABORT         451                     /*  ��ֹ����Ĳ��� �����д�     */
#define __FTPD_RETCODE_SERVER_DONOT_RUN_REQ     452                     /*  δִ������Ĳ���,�ռ䲻��   */
#define __FTPD_RETCODE_SERVER_CMD_ERROR         500                     /*  �����ʶ��                */
#define __FTPD_RETCODE_SERVER_SYNTAX_ERR        501                     /*  �����﷨����                */
#define __FTPD_RETCODE_SERVER_UNSUP_WITH_ARG    504                     /*  �˲����µ������δʵ��    */
#define __FTPD_RETCODE_SERVER_LOGIN_FAILED      530                     /*  �û���¼ʧ��                */
#define __FTPD_RETCODE_SERVER_REQ_FAILED        550                     /*  δִ������Ĳ���            */
#define __FTPD_RETCODE_SERVER_DREQ_ABORT        551                     /*  ����������ֹ                */
#define __FTPD_RETCODE_SERVER_FILE_NAME_ERROR   553                     /*  �ļ������Ϸ�                */

#endif                                                                  /*  __LWIP_FTP_H                */
/*********************************************************************************************************
  END
*********************************************************************************************************/
