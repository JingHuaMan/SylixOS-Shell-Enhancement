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
** ��   ��   ��: utmp.h
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2013 �� 01 �� 17 ��
**
** ��        ��: �û����ݿ�, ��Ҫ�ⲿ��֧��.
*********************************************************************************************************/

#ifndef __UTMP_H
#define __UTMP_H

#include <sys/types.h>

#define _PATH_UTMP          "/var/run/utmp"
#define _PATH_WTMP          "/var/log/wtmp"
#define _PATH_LASTLOG       "/var/log/lastlog"

#define UT_NAMESIZE         8
#define UT_LINESIZE         8
#define UT_HOSTSIZE         16

struct lastlog {
    time_t      ll_time;
    char        ll_line[UT_LINESIZE];
    char        ll_host[UT_HOSTSIZE];
};

struct utmp {
    char        ut_line[UT_LINESIZE];
    char        ut_name[UT_NAMESIZE];
    char        ut_host[UT_HOSTSIZE];
    time_t      ut_time;
};

__BEGIN_DECLS

int             utmpname(const char *);
void            setutent(void);
struct utmp    *getutent(void);
void            endutent(void);

__END_DECLS

#endif                                                                  /*  __UTMP_H                    */
/*********************************************************************************************************
  END
*********************************************************************************************************/
