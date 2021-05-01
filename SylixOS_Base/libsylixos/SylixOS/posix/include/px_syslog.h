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
** ��   ��   ��: px_syslog.h
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2011 �� 03 �� 10 ��
**
** ��        ��: ���� posix syslog ��.
*********************************************************************************************************/

#ifndef __PX_SYSLOG_H
#define __PX_SYSLOG_H

#include "SylixOS.h"                                                    /*  ����ϵͳͷ�ļ�              */

/*********************************************************************************************************
  �ü�֧��
*********************************************************************************************************/
#if LW_CFG_POSIX_EN > 0

#ifdef __cplusplus
extern "C" {
#endif                                                                  /*  __cplusplus                 */

/*********************************************************************************************************
  syslog facility 
*********************************************************************************************************/

#define LOG_KERN        (0  << 3)                                       /* kernel messages              */
#define LOG_USER        (1  << 3)                                       /* random user-level messages   */
#define LOG_MAIL        (2  << 3)                                       /* mail system                  */
#define LOG_DAEMON      (3  << 3)                                       /* system daemons               */
#define LOG_AUTH        (4  << 3)                                       /* security/authorization messag*/
#define LOG_SYSLOG      (5  << 3)                                       /* messages generated internally*/
                                                                        /* by syslogd                   */
#define LOG_LPR         (6  << 3)                                       /* line printer subsystem       */
#define LOG_NEWS        (7  << 3)                                       /* network news subsystem       */
#define LOG_UUCP        (8  << 3)                                       /* UUCP subsystem               */
#define LOG_CRON        (9  << 3)                                       /* clock daemon                 */
#define LOG_AUTHPRIV    (10 << 3)                                       /* security/authorization       */
                                                                        /* messages (private)           */
#define LOG_FTP         (11 << 3)                                       /* ftp daemon                   */

#define LOG_LOCAL0      (16 << 3)                                       /* reserved for local use       */
#define LOG_LOCAL1      (17 << 3)                                       /* reserved for local use       */
#define LOG_LOCAL2      (18 << 3)                                       /* reserved for local use       */
#define LOG_LOCAL3      (19 << 3)                                       /* reserved for local use       */
#define LOG_LOCAL4      (20 << 3)                                       /* reserved for local use       */
#define LOG_LOCAL5      (21 << 3)                                       /* reserved for local use       */
#define LOG_LOCAL6      (22 << 3)                                       /* reserved for local use       */
#define LOG_LOCAL7      (23 << 3)                                       /* reserved for local use       */

#define LOG_NFACILITIES 24                                              /* current number of facilities */

#define LOG_FACMASK     0x03f8                                          /* mask to extract facility part*/
                                                                        /* facility of pri              */
#define LOG_FAC(p)      (((p) & LOG_FACMASK) >> 3)

/*********************************************************************************************************
  Option flags for openlog.
*********************************************************************************************************/

#define LOG_PID         0x01                                            /* log the pid with each message*/
#define LOG_CONS        0x02                                            /* log on the console if errors */
                                                                        /* in sending                   */
#define LOG_ODELAY      0x04                                            /* delay open until first       */
                                                                        /* syslog() (default)           */
#define LOG_NDELAY      0x08                                            /* don't delay open             */
                                                                        /* openlog opens and connects   */
                                                                        /* the socket                   */
#define LOG_NOWAIT      0x10                                            /* don't wait for console forks:*/
                                                                        /* DEPRECATED                   */
#define LOG_PERROR      0x20                                            /* log to stderr as well        */

/*********************************************************************************************************
  iSeverity priority
*********************************************************************************************************/

#define LOG_EMERG       0                                               /* system is unusable           */
#define LOG_ALERT       1                                               /* action must be taken         */
                                                                        /* immediately                  */
#define LOG_CRIT        2                                               /* critical conditions          */
#define LOG_ERR         3                                               /* error conditions             */
#define LOG_WARNING     4                                               /* warning conditions           */
#define LOG_NOTICE      5                                               /* normal but significant       */
                                                                        /* condition                    */
#define LOG_INFO        6                                               /* informational                */
#define LOG_DEBUG       7                                               /* debug-level messages         */

#define LOG_PRIMASK     0x07                                            /* mask to extract priority part*/
                                                                        /* (internal)                   */
                                                                        /* extract priority             */
                                                                        
#define LOG_MASK(pri)   (1 << (pri))                                    /* mask for one priority        */
#define LOG_UPTO(pri)   ((1 << ((pri)+1)) - 1)                          /* all priorities through pri   */

#define LOG_PRI(p)              ((p) & LOG_PRIMASK)
#define LOG_MAKEPRI(fac, pri)   (((fac) << 3) | (pri))

/*********************************************************************************************************
  
*********************************************************************************************************/

struct syslog_data {
    int             log_file;
    int             connected;
    int             opened;
    int             log_stat;
    const char     *log_tag;
    int             log_fac;
    int             log_mask;
};

#define SYSLOG_DATA_INIT    {-1, 0, 0, 0, NULL, LOG_USER, 0xff}
     
/*********************************************************************************************************
  default log file name (AF_UNIX socket SOCK_DGRAM)
  
  ��� SYSLOGD_HOST ����������Ч, ��ͬʱ���͵�ָ���ķ�����, eg. SYSLOGD_HOST="192.168.0.1:514"
*********************************************************************************************************/

#define LOG_DEFAULT_SIZE        1024
#define LOG_DEFAULT_FILE        "/dev/log"
#define LOG_DEFAULT_HOST_ENV    "SYSLOGD_HOST"

/*********************************************************************************************************
  api 
  sylixos sylixos ���黷������ SYSLOGD_HOST , �������Ч�� syslog ������, ��Ὣ��Ϣ���͸� syslog ������
  �����Ҫ����ȷ��������, ����Ҫ�������û�������, Ȼ����� closelog, ����ϵͳ�ڷ�����һ����Ϣʱ, ��������
  ȷ��������.
*********************************************************************************************************/

LW_API void     openlog(const char *ident, int logopt, int facility);
LW_API void     closelog(void);
LW_API int      setlogmask(int maskpri);
LW_API int      setlogmask_r(int maskpri, struct syslog_data *data);
LW_API void     syslog(int priority, const char *message, ...);
LW_API void     syslog_r(int priority, struct syslog_data *data, const char *message, ...);

#ifdef __cplusplus
}
#endif                                                                  /*  __cplusplus                 */

#endif                                                                  /*  LW_CFG_POSIX_EN > 0         */
#endif                                                                  /*  __PX_SYSLOG_H               */
/*********************************************************************************************************
  END
*********************************************************************************************************/
